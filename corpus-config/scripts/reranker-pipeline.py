#!/usr/bin/env python3
"""
KUDIG-DATABASE 高信噪比 RAG Pipeline 示例
包含：Hybrid Search (BM25 + 向量) → Cross-Encoder Reranker → 去重过滤 → Priority 加权

依赖安装:
    pip install langchain langchain-community chromadb rank-bm25 sentence-transformers

用法:
    python3 reranker-pipeline.py --query "Pod CrashLoopBackOff 怎么排查" --k 5
"""

import argparse
import json
import os
import re
from pathlib import Path
from typing import List

# LangChain 组件
from langchain.schema import Document
from langchain.text_splitter import MarkdownHeaderTextSplitter
from langchain.vectorstores import Chroma
from langchain.embeddings import OpenAIEmbeddings  # 或 HuggingFaceEmbeddings

# Hybrid Search
from rank_bm25 import BM25Okapi

# Reranker (Cross-Encoder)
from sentence_transformers import CrossEncoder


# ───────────────────────────────────────────────
# 1. 加载与分块（带元数据增强）
# ───────────────────────────────────────────────

def load_documents(root: Path, metadata_json: Path) -> List[Document]:
    """加载 Markdown 文件并注入增强元数据"""
    with open(metadata_json, "r", encoding="utf-8") as f:
        meta_db = json.load(f)["documents"]

    docs = []
    for rel_path, meta in meta_db.items():
        full_path = root / rel_path
        if not full_path.exists():
            continue

        text = full_path.read_text(encoding="utf-8")
        # 移除 YAML front matter
        if text.startswith("---"):
            parts = text.split("---", 2)
            if len(parts) >= 3:
                text = parts[2]

        # 构造 LangChain Document（整篇作为单 chunk，也可继续拆分）
        docs.append(Document(
            page_content=text,
            metadata={
                "source": rel_path,
                "domain": meta["domain"],
                "content_type": meta["content_type"],
                "quality_score": meta["quality_score"],
                "priority": meta["priority"],
            }
        ))
    return docs


def chunk_documents(docs: List[Document]) -> List[Document]:
    """按 Markdown H2 标题分块"""
    splitter = MarkdownHeaderTextSplitter(
        headers_to_split_on=[("#", "title"), ("##", "section")]
    )
    chunks = []
    for doc in docs:
        try:
            # MarkdownHeaderTextSplitter 需要纯文本，不含 front matter（已移除）
            for chunk in splitter.split_text(doc.page_content):
                # 继承父文档元数据
                chunk.metadata.update({
                    k: v for k, v in doc.metadata.items()
                    if k not in chunk.metadata
                })
                chunks.append(chunk)
        except Exception:
            # 分块失败时保留整篇
            chunks.append(doc)
    return chunks


# ───────────────────────────────────────────────
# 2. Hybrid Retriever (BM25 + 向量)
# ───────────────────────────────────────────────

class HybridRetriever:
    """融合 BM25 稀疏检索 和 向量密集检索"""

    def __init__(self, docs: List[Document], embeddings, vector_dir: str = "./chroma_db"):
        self.docs = docs
        self.texts = [d.page_content for d in docs]

        # 1) 向量存储
        self.vectorstore = Chroma.from_documents(
            docs, embeddings, persist_directory=vector_dir
        )

        # 2) BM25 索引
        tokenized = [self._tokenize(t) for t in self.texts]
        self.bm25 = BM25Okapi(tokenized)

    @staticmethod
    def _tokenize(text: str) -> List[str]:
        # 中文按字分，英文按词分，简单实现
        tokens = re.findall(r"[\u4e00-\u9fff]|[a-zA-Z0-9]+", text.lower())
        return tokens

    def search(self, query: str, k: int = 20) -> List[Document]:
        """返回混合检索的 Top-k 文档（去重后）"""
        # 向量检索 Top-k*2
        vec_results = self.vectorstore.similarity_search(query, k=k * 2)

        # BM25 检索 Top-k*2
        bm25_scores = self.bm25.get_scores(self._tokenize(query))
        bm25_top = sorted(range(len(bm25_scores)), key=lambda i: bm25_scores[i], reverse=True)[:k * 2]
        bm25_results = [self.docs[i] for i in bm25_top]

        # 融合：RRF (Reciprocal Rank Fusion)
        scores = {}
        for rank, doc in enumerate(vec_results):
            scores[id(doc)] = scores.get(id(doc), 0) + 1.0 / (rank + 60)
        for rank, doc in enumerate(bm25_results):
            scores[id(doc)] = scores.get(id(doc), 0) + 1.0 / (rank + 60)

        # 去重并排序
        seen = set()
        unique = []
        for doc in vec_results + bm25_results:
            if id(doc) not in seen:
                seen.add(id(doc))
                doc.metadata["rrf_score"] = scores.get(id(doc), 0)
                unique.append(doc)

        unique.sort(key=lambda d: d.metadata.get("rrf_score", 0), reverse=True)
        return unique[:k]


# ───────────────────────────────────────────────
# 3. Cross-Encoder Reranker
# ───────────────────────────────────────────────

class Reranker:
    """基于 Cross-Encoder 的精排"""

    def __init__(self, model_name: str = "BAAI/bge-reranker-large"):
        print(f"Loading reranker: {model_name} ...")
        self.model = CrossEncoder(model_name, device="cpu")

    def rerank(self, query: str, docs: List[Document], top_n: int = 5) -> List[Document]:
        if not docs:
            return docs

        pairs = [(query, d.page_content[:2000]) for d in docs]  # 截断避免超长
        scores = self.model.predict(pairs)

        for doc, score in zip(docs, scores):
            doc.metadata["rerank_score"] = float(score)

        # 结合 quality_score 做最终排序
        def final_score(doc):
            rerank = doc.metadata.get("rerank_score", 0.5)
            quality = doc.metadata.get("quality_score", 0.5)
            # 高质量文档加权；低质量即使相关也降级
            return rerank * (0.7 + 0.3 * quality)

        docs.sort(key=final_score, reverse=True)
        return docs[:top_n]


# ───────────────────────────────────────────────
# 4. 去重过滤器（Embedding 级）
# ───────────────────────────────────────────────

class DedupFilter:
    """基于余弦相似度的 chunk 去重"""

    def __init__(self, threshold: float = 0.92):
        self.threshold = threshold

    def filter(self, docs: List[Document]) -> List[Document]:
        from sentence_transformers import SentenceTransformer
        model = SentenceTransformer("BAAI/bge-small-zh-v1.5")
        texts = [d.page_content[:500] for d in docs]
        embeddings = model.encode(texts, normalize_embeddings=True)

        keep = []
        for i, doc in enumerate(docs):
            is_dup = False
            for j in range(i):
                sim = float(embeddings[i] @ embeddings[j])
                if sim > self.threshold:
                    is_dup = True
                    break
            if not is_dup:
                keep.append(doc)
        return keep


# ───────────────────────────────────────────────
# 5. Priority 过滤器（基于 rag-full-profile.yaml）
# ───────────────────────────────────────────────

def filter_by_priority(docs: List[Document], min_priority: str = "medium") -> List[Document]:
    pmap = {"high": 3, "medium": 2, "low": 1}
    min_val = pmap.get(min_priority, 1)
    return [d for d in docs if pmap.get(d.metadata.get("priority"), 1) >= min_val]


# ───────────────────────────────────────────────
# 6. 完整 Pipeline
# ───────────────────────────────────────────────

def build_pipeline(root: Path, metadata_json: Path):
    """构建完整的检索管道（仅首次需要索引）"""
    docs = load_documents(root, metadata_json)
    print(f"Loaded {len(docs)} documents")

    chunks = chunk_documents(docs)
    print(f"Chunked into {len(chunks)} chunks")

    # 去重
    dedup = DedupFilter(threshold=0.92)
    chunks = dedup.filter(chunks)
    print(f"After dedup: {len(chunks)} chunks")

    # Embedding 模型（可替换为 bge-large-zh-v1.5 / text-embedding-3-large）
    from langchain.embeddings import HuggingFaceEmbeddings
    embeddings = HuggingFaceEmbeddings(
        model_name="BAAI/bge-small-zh-v1.5",
        model_kwargs={"device": "cpu"},
        encode_kwargs={"normalize_embeddings": True}
    )

    # Hybrid Retriever
    retriever = HybridRetriever(chunks, embeddings, vector_dir="./chroma_db_kudig")
    reranker = Reranker(model_name="BAAI/bge-reranker-large")

    return retriever, reranker


def search(retriever, reranker, query: str, k: int = 5, min_priority: str = "medium"):
    """端到端检索：Hybrid → Rerank → Priority Filter"""
    print(f"\n🔍 Query: {query}")

    # Step 1: Hybrid 粗排
    candidates = retriever.search(query, k=k * 4)
    print(f"  Hybrid candidates: {len(candidates)}")

    # Step 2: Priority 过滤
    candidates = filter_by_priority(candidates, min_priority)
    print(f"  After priority filter (≥{min_priority}): {len(candidates)}")

    # Step 3: Cross-Encoder 精排
    results = reranker.rerank(query, candidates, top_n=k)
    print(f"  Final results: {len(results)}\n")

    return results


def main():
    parser = argparse.ArgumentParser(description="KUDIG 高信噪比 RAG Pipeline")
    parser.add_argument("--query", "-q", default="Pod CrashLoopBackOff 怎么排查", help="检索查询")
    parser.add_argument("--k", type=int, default=5, help="返回结果数")
    parser.add_argument("--root", default=".", help="知识库根目录")
    parser.add_argument("--metadata", default="corpus-config/metadata-enhanced.json", help="元数据 JSON")
    parser.add_argument("--priority", default="medium", choices=["high", "medium", "low"],
                        help="最低 priority 过滤")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    meta_path = root / args.metadata

    if not meta_path.exists():
        print(f"元数据文件不存在: {meta_path}")
        print("请先运行: python3 corpus-config/scripts/enhance-metadata.py")
        return

    print("=" * 60)
    print("KUDIG 高信噪比 RAG Pipeline")
    print("=" * 60)

    retriever, reranker = build_pipeline(root, meta_path)
    results = search(retriever, reranker, args.query, args.k, args.priority)

    print("结果展示:")
    for i, doc in enumerate(results, 1):
        meta = doc.metadata
        source = meta.get("source", "unknown")
        ctype = meta.get("content_type", "unknown")
        quality = meta.get("quality_score", 0)
        priority = meta.get("priority", "unknown")
        rrf = meta.get("rrf_score", 0)
        rerank = meta.get("rerank_score", 0)

        # 提取前 120 字作为摘要
        snippet = doc.page_content.replace("\n", " ")[:120].strip()

        print(f"\n[{i}] {source}")
        print(f"    type={ctype} | priority={priority} | quality={quality:.2f} | rrf={rrf:.4f} | rerank={rerank:.4f}")
        print(f"    {snippet}...")


if __name__ == "__main__":
    main()
