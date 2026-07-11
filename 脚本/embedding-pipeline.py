#!/usr/bin/env python3
"""
KUDIG Embedding Pipeline — 全库向量化索引构建

功能：
  1. 读取 元数据/corpus-config/profiles/*.yaml 配置
  2. 按策略分块（by_h2 / by_h3 / by_section / full_doc）
  3. 提取并增强元数据（frontmatter + domain + path）
  4. 调用 Embedding Provider 生成向量
  5. 输出 JSONL chunks + manifest（支持增量更新）

用法：
  python3 脚本/embedding-pipeline.py --profile 元数据/corpus-config/profiles/rag-sre-profile.yaml
  python3 脚本/embedding-pipeline.py --profile 元数据/corpus-config/profiles/rag-full-profile.yaml --incremental
  python3 scripts/embedding-pipeline.py --search "node notready" --top-k 5

输出：
  元数据/corpus-config/profiles/.vector-cache/<profile-name>/
    ├── chunks.jsonl        # 所有 chunk（文本 + 元数据）
    ├── embeddings.npy      # 向量矩阵（float32, N×D）
    ├── manifest.json       # 文件 hash 映射（增量更新用）
    └── index.faiss         # FAISS 索引（可选，需安装 faiss）

Embedding Provider 配置（环境变量）：
  EMBEDDING_PROVIDER=local      # 默认：sentence-transformers 本地模型，更适合中文
  EMBEDDING_PROVIDER=mock       # 确定性伪向量（快速验证，需显式设置）
  EMBEDDING_PROVIDER=openai     # OpenAI text-embedding-3-small
  OPENAI_API_KEY=sk-xxx
  LOCAL_MODEL_NAME=BAAI/bge-m3  # 默认本地模型，首次下载较慢

依赖：
  pip install sentence-transformers
"""

import argparse
import hashlib
import json
import os
import re
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import yaml

# ========== 常量 ==========
PROJECT_ROOT = Path(__file__).resolve().parent.parent
CACHE_DIR = PROJECT_ROOT / "元数据" / "corpus-config" / "profiles" / ".vector-cache"
DEFAULT_EMBEDDING_DIM = 1024  # BAAI/bge-m3


# ========== 数据模型 ==========

@dataclass
class Chunk:
    chunk_id: str
    source_path: str
    text: str
    metadata: Dict[str, Any] = field(default_factory=dict)
    embedding: Optional[List[float]] = None

    def to_dict(self) -> Dict[str, Any]:
        def _serialize(v: Any) -> Any:
            if isinstance(v, (set, frozenset)):
                return list(v)
            if isinstance(v, dict):
                return {k: _serialize(vv) for k, vv in v.items()}
            if isinstance(v, list):
                return [_serialize(vv) for vv in v]
            # YAML may parse dates as datetime.date objects
            import datetime
            if isinstance(v, (datetime.date, datetime.datetime)):
                return v.isoformat()
            return v

        return {
            "chunk_id": self.chunk_id,
            "source_path": self.source_path,
            "text": self.text,
            "metadata": _serialize(self.metadata),
            "embedding": self.embedding,
        }


# ========== Markdown 解析 ==========

def parse_frontmatter(content: str) -> Tuple[Dict[str, Any], str]:
    """解析 YAML frontmatter，返回 (metadata, body_without_frontmatter)"""
    if not content.startswith("---"):
        return {}, content

    end = content.find("---", 3)
    if end == -1:
        return {}, content

    fm_text = content[3:end].strip()
    body = content[end + 3:].strip()

    try:
        metadata = yaml.safe_load(fm_text) or {}
    except Exception:
        metadata = {}

    return metadata, body


def split_by_headers(body: str, header_level: str = "##") -> List[Tuple[str, str]]:
    """按 Markdown 标题分块，返回 [(section_title, section_text), ...]"""
    pattern = re.compile(rf"^({re.escape(header_level)} .+)$", re.MULTILINE)
    parts = pattern.split(body)

    chunks = []
    if parts and not parts[0].strip().startswith(header_level):
        # 第一个部分是无标题的前言
        prelude = parts[0].strip()
        if prelude:
            chunks.append(("prelude", prelude))
        parts = parts[1:]

    for i in range(0, len(parts), 2):
        title = parts[i].strip()
        text = parts[i + 1].strip() if i + 1 < len(parts) else ""
        if text:
            chunks.append((title, f"{title}\n\n{text}"))

    return chunks


def split_by_section(body: str) -> List[Tuple[str, str]]:
    """按 ## 或 ### 分块（混合策略）"""
    # 先尝试按 ## 分，如果块太大则按 ### 再分
    h2_chunks = split_by_headers(body, "##")
    result = []
    for title, text in h2_chunks:
        if len(text) > 8000:
            sub_chunks = split_by_headers(text, "###")
            result.extend(sub_chunks)
        else:
            result.append((title, text))
    return result


def chunk_file(filepath: Path, strategy: str) -> List[Chunk]:
    """对单个 Markdown 文件执行分块"""
    content = filepath.read_text(encoding="utf-8")
    metadata, body = parse_frontmatter(content)

    # 选择分块策略
    if strategy == "by_h2":
        sections = split_by_headers(body, "##")
    elif strategy == "by_h3":
        sections = split_by_headers(body, "###")
    elif strategy == "by_section":
        sections = split_by_section(body)
    elif strategy == "full_doc":
        sections = [(metadata.get("title", filepath.stem), body)]
    else:
        sections = split_by_headers(body, "##")

    chunks = []
    rel_path = str(filepath.relative_to(PROJECT_ROOT))

    for idx, (section_title, text) in enumerate(sections):
        chunk_id = hashlib.sha256(f"{rel_path}:{idx}:{text[:200]}".encode()).hexdigest()[:16]
        chunk_meta = {
            "source": rel_path,
            "domain": rel_path.split("/")[0] if "/" in rel_path else "root",
            "filename": filepath.name,
            "section_title": section_title,
            "chunk_index": idx,
            "total_chunks": len(sections),
            **{k: v for k, v in metadata.items() if k not in ("created", "updated")},
        }
        chunks.append(Chunk(
            chunk_id=chunk_id,
            source_path=rel_path,
            text=text,
            metadata=chunk_meta,
        ))

    return chunks


# ========== 语料收集 ==========

def collect_files(profile: Dict[str, Any]) -> List[Tuple[Path, str]]:
    """
    根据 profile 配置收集所有 Markdown 文件及其分块策略。
    返回: [(Path, chunking_strategy), ...]
    """
    files: List[Tuple[Path, str]] = []
    seen = set()

    # 解析 include 规则
    include_rules = []
    for key in ("include", "core", "methodology", "reference"):
        if key in profile:
            items = profile[key]
            if isinstance(items, list):
                include_rules.extend(items)
            elif isinstance(items, dict):
                include_rules.append(items)

    for rule in include_rules:
        if isinstance(rule, dict):
            pattern = rule.get("path", "")
            strategy = rule.get("chunking", "by_h2")
        else:
            pattern = str(rule)
            strategy = "by_h2"

        base = PROJECT_ROOT / pattern
        if base.is_dir():
            for fp in base.rglob("*.md"):
                if fp not in seen:
                    files.append((fp, strategy))
                    seen.add(fp)
        elif base.is_file() and base.suffix == ".md":
            if base not in seen:
                files.append((base, strategy))
                seen.add(base)
        elif "*" in pattern:
            # glob 模式
            for fp in PROJECT_ROOT.glob(pattern):
                if fp.suffix == ".md" and fp not in seen:
                    files.append((fp, strategy))
                    seen.add(fp)

    # 解析 exclude 规则
    exclude_patterns = []
    for p in profile.get("exclude", []):
        exclude_patterns.append(p)

    def is_excluded(fp: Path) -> bool:
        rel = str(fp.relative_to(PROJECT_ROOT))
        for pat in exclude_patterns:
            if "/" in pat:
                if rel.startswith(pat.rstrip("/")) or rel == pat:
                    return True
            else:
                if fp.match(pat):
                    return True
        return False

    filtered = [(fp, strat) for fp, strat in files if not is_excluded(fp)]
    return filtered


# ========== Embedding Provider ==========

class EmbeddingProvider:
    def embed(self, texts: List[str]) -> List[List[float]]:
        raise NotImplementedError


class MockProvider(EmbeddingProvider):
    """
    确定性伪向量 Provider。
    基于文本内容的 hash 生成固定向量，用于快速验证 Pipeline 正确性。
    向量本身无真实语义，但相同文本总是产生相同向量。
    """
    def __init__(self, dim: int = DEFAULT_EMBEDDING_DIM):
        self.dim = dim

    def embed(self, texts: List[str]) -> List[List[float]]:
        import numpy as np
        results = []
        for text in texts:
            seed = int(hashlib.sha256(text.encode()).hexdigest(), 16) % (2**31)
            rng = np.random.default_rng(seed)
            vec = rng.standard_normal(self.dim).astype(np.float32)
            vec = vec / (np.linalg.norm(vec) + 1e-8)
            results.append(vec.tolist())
        return results


class OpenAIProvider(EmbeddingProvider):
    def __init__(self, model: str = "text-embedding-3-small", api_key: Optional[str] = None):
        self.model = model
        self.api_key = api_key or os.getenv("OPENAI_API_KEY")
        if not self.api_key:
            raise ValueError("OpenAI API key not found. Set OPENAI_API_KEY env var.")
        try:
            import openai
            self.client = openai.OpenAI(api_key=self.api_key)
        except ImportError:
            raise ImportError("pip install openai")

    def embed(self, texts: List[str]) -> List[List[float]]:
        # 分批处理（OpenAI 限制 batch size）
        batch_size = 100
        all_embeddings = []
        for i in range(0, len(texts), batch_size):
            batch = texts[i:i + batch_size]
            resp = self.client.embeddings.create(model=self.model, input=batch)
            all_embeddings.extend([d.embedding for d in resp.data])
        return all_embeddings


class LocalProvider(EmbeddingProvider):
    def __init__(self, model_name: str = "BAAI/bge-m3"):
        try:
            from sentence_transformers import SentenceTransformer
        except ImportError:
            raise ImportError("pip install sentence-transformers")

        # 自动选择设备：优先 GPU，其次 CPU
        device = "cpu"
        try:
            import torch
            if torch.cuda.is_available():
                device = "cuda"
            elif torch.backends.mps.is_available():
                device = "mps"
        except ImportError:
            pass

        self.model = SentenceTransformer(model_name, device=device)

    def embed(self, texts: List[str]) -> List[List[float]]:
        embeddings = self.model.encode(texts, show_progress_bar=False, convert_to_numpy=True)
        return embeddings.tolist()


def get_provider() -> EmbeddingProvider:
    provider_name = os.getenv("EMBEDDING_PROVIDER", "local").lower()
    if provider_name == "openai":
        return OpenAIProvider()
    elif provider_name == "local":
        model = os.getenv("LOCAL_MODEL_NAME", "BAAI/bge-m3")
        return LocalProvider(model)
    else:
        dim = int(os.getenv("MOCK_EMBEDDING_DIM", str(DEFAULT_EMBEDDING_DIM)))
        return MockProvider(dim)


# ========== 增量更新 ==========

def compute_file_hash(filepath: Path) -> str:
    """计算文件内容的 SHA256"""
    h = hashlib.sha256()
    h.update(filepath.read_bytes())
    return h.hexdigest()


def load_manifest(cache_dir: Path) -> Dict[str, str]:
    manifest_path = cache_dir / "manifest.json"
    if manifest_path.exists():
        return json.loads(manifest_path.read_text(encoding="utf-8"))
    return {}


def save_manifest(cache_dir: Path, manifest: Dict[str, str]) -> None:
    manifest_path = cache_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")


def filter_changed_files(
    files: List[Tuple[Path, str]],
    manifest: Dict[str, str]
) -> List[Tuple[Path, str]]:
    changed = []
    for fp, strategy in files:
        rel = str(fp.relative_to(PROJECT_ROOT))
        current_hash = compute_file_hash(fp)
        if manifest.get(rel) != current_hash:
            changed.append((fp, strategy))
    return changed


# ========== 构建流程 ==========

def build_index(profile_path: Path, incremental: bool = False) -> Path:
    profile = yaml.safe_load(profile_path.read_text(encoding="utf-8"))
    profile_name = profile.get("name", profile_path.stem)
    cache_dir = CACHE_DIR / profile_name
    cache_dir.mkdir(parents=True, exist_ok=True)

    print(f"📦 Profile: {profile_name}")
    print(f"   Config: {profile_path}")
    print(f"   Cache:  {cache_dir}")

    # 收集文件
    print("\n🔍 收集语料文件...")
    files = collect_files(profile)
    print(f"   找到 {len(files)} 个 Markdown 文件")

    # 增量更新
    manifest = load_manifest(cache_dir)
    if incremental and manifest:
        files = filter_changed_files(files, manifest)
        print(f"   增量模式：{len(files)} 个文件发生变更")
        if not files:
            print("   ✅ 无变更，跳过构建")
            return cache_dir

    # 分块
    print("\n✂️  分块处理...")
    all_chunks: List[Chunk] = []
    for fp, strategy in files:
        try:
            chunks = chunk_file(fp, strategy)
            all_chunks.extend(chunks)
        except Exception as e:
            print(f"   ⚠️  {fp}: {e}")

    print(f"   生成 {len(all_chunks)} 个 chunk")

    # Embedding
    print("\n🧠 生成向量...")
    provider = get_provider()
    texts = [c.text for c in all_chunks]
    start = time.time()
    embeddings = provider.embed(texts)
    elapsed = time.time() - start
    print(f"   耗时 {elapsed:.1f}s，平均 {elapsed / len(texts):.3f}s/chunk")

    for chunk, emb in zip(all_chunks, embeddings):
        chunk.embedding = emb

    # 保存 chunks.jsonl
    chunks_path = cache_dir / "chunks.jsonl"
    with open(chunks_path, "w", encoding="utf-8") as f:
        for chunk in all_chunks:
            f.write(json.dumps(chunk.to_dict(), ensure_ascii=False) + "\n")
    print(f"   💾 chunks.jsonl: {chunks_path}")

    # 保存 embeddings.npy
    try:
        import numpy as np
        emb_matrix = np.array([c.embedding for c in all_chunks], dtype=np.float32)
        np.save(cache_dir / "embeddings.npy", emb_matrix)
        print(f"   💾 embeddings.npy: {emb_matrix.shape}")
    except ImportError:
        print("   ⚠️  numpy 不可用，跳过 .npy 输出")

    # 尝试构建 FAISS 索引
    try:
        import faiss
        import numpy as np
        dim = len(all_chunks[0].embedding)
        index = faiss.IndexFlatIP(dim)  # 内积（余弦相似度需先归一化）
        emb_matrix = np.array([c.embedding for c in all_chunks], dtype=np.float32)
        # 确保归一化
        faiss.normalize_L2(emb_matrix)
        index.add(emb_matrix)
        faiss.write_index(index, str(cache_dir / "index.faiss"))
        print(f"   💾 index.faiss: {index.ntotal} vectors")
    except ImportError:
        print("   ℹ️  FAISS 未安装，跳过 faiss 索引（pip install faiss-cpu）")

    # 更新 manifest
    new_manifest = {}
    for fp, _ in files:
        rel = str(fp.relative_to(PROJECT_ROOT))
        new_manifest[rel] = compute_file_hash(fp)
    save_manifest(cache_dir, new_manifest)
    print(f"   💾 manifest.json: {len(new_manifest)} entries")

    print(f"\n✅ 索引构建完成: {cache_dir}")
    return cache_dir


# ========== 搜索功能 ==========

def search_index(query: str, profile_name: str, top_k: int = 5) -> List[Dict[str, Any]]:
    cache_dir = CACHE_DIR / profile_name
    chunks_path = cache_dir / "chunks.jsonl"

    if not chunks_path.exists():
        print(f"❌ 索引不存在: {cache_dir}")
        print("   请先运行: python3 scripts/embedding-pipeline.py --profile <yaml>")
        sys.exit(1)

    # 加载 chunks
    chunks: List[Chunk] = []
    with open(chunks_path, "r", encoding="utf-8") as f:
        for line in f:
            data = json.loads(line)
            chunks.append(Chunk(
                chunk_id=data["chunk_id"],
                source_path=data["source_path"],
                text=data["text"],
                metadata=data["metadata"],
                embedding=data.get("embedding"),
            ))

    # 生成 query embedding
    provider = get_provider()
    query_emb = provider.embed([query])[0]

    # 相似度计算
    import numpy as np
    query_vec = np.array(query_emb, dtype=np.float32)
    query_vec = query_vec / (np.linalg.norm(query_vec) + 1e-8)

    doc_matrix = np.array([c.embedding for c in chunks], dtype=np.float32)
    # 归一化（假设已归一化，但再归一化一次保险）
    norms = np.linalg.norm(doc_matrix, axis=1, keepdims=True)
    doc_matrix = doc_matrix / (norms + 1e-8)

    scores = doc_matrix @ query_vec  # 余弦相似度
    top_indices = np.argsort(scores)[::-1][:top_k]

    results = []
    for idx in top_indices:
        chunk = chunks[idx]
        results.append({
            "score": float(scores[idx]),
            "chunk_id": chunk.chunk_id,
            "source": chunk.source_path,
            "title": chunk.metadata.get("title", chunk.metadata.get("section_title", "")),
            "domain": chunk.metadata.get("domain", ""),
            "text_preview": chunk.text[:200].replace("\n", " "),
        })

    return results


# ========== 评估功能 ==========

def evaluate_profile(profile_path: Path) -> Dict[str, Any]:
    """评估语料质量指标"""
    profile = yaml.safe_load(profile_path.read_text(encoding="utf-8"))
    files = collect_files(profile)

    total_size = sum(fp.stat().st_size for fp, _ in files)
    avg_size = total_size / len(files) if files else 0

    # 统计 frontmatter 完整度
    fm_stats = {"total": 0, "has_title": 0, "has_tags": 0, "has_category": 0}
    for fp, _ in files:
        fm_stats["total"] += 1
        content = fp.read_text(encoding="utf-8")
        meta, _ = parse_frontmatter(content)
        if meta.get("title"):
            fm_stats["has_title"] += 1
        if meta.get("tags"):
            fm_stats["has_tags"] += 1
        if meta.get("category"):
            fm_stats["has_category"] += 1

    return {
        "profile": profile.get("name", profile_path.stem),
        "file_count": len(files),
        "total_bytes": total_size,
        "avg_file_bytes": avg_size,
        "frontmatter": {
            k: f"{v}/{fm_stats['total']} ({v / fm_stats['total'] * 100:.1f}%)"
            for k, v in fm_stats.items() if k != "total"
        },
    }


# ========== 主函数 ==========

def main():
    parser = argparse.ArgumentParser(description="KUDIG Embedding Pipeline")
    parser.add_argument("--profile", type=Path, required=True, help="YAML profile 路径")
    parser.add_argument("--incremental", action="store_true", help="增量更新模式")
    parser.add_argument("--search", type=str, help="语义搜索 query")
    parser.add_argument("--top-k", type=int, default=5, help="返回结果数")
    parser.add_argument("--evaluate", action="store_true", help="评估语料质量")
    args = parser.parse_args()

    if not args.profile.exists():
        print(f"❌ Profile 不存在: {args.profile}")
        sys.exit(1)

    if args.evaluate:
        print("📊 语料质量评估...")
        stats = evaluate_profile(args.profile)
        print(json.dumps(stats, ensure_ascii=False, indent=2))
        return

    if args.search:
        profile = yaml.safe_load(args.profile.read_text(encoding="utf-8"))
        profile_name = profile.get("name", args.profile.stem)
        print(f"🔍 搜索: '{args.search}' (profile: {profile_name})")
        results = search_index(args.search, profile_name, args.top_k)
        for i, r in enumerate(results, 1):
            print(f"\n  {i}. [{r['score']:.3f}] {r['title']}")
            print(f"     📄 {r['source']}")
            print(f"     🏷️  {r['domain']}")
            print(f"     💬 {r['text_preview']}...")
        return

    # 默认：构建索引
    build_index(args.profile, incremental=args.incremental)


if __name__ == "__main__":
    main()
