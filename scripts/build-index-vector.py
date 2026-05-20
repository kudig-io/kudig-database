#!/usr/bin/env python3
"""
将 topic-index 转换为向量检索索引
支持深度研究场景下的语义搜索

使用方法:
    python3 build-index-vector.py           # 构建向量索引
    python3 build-index-vector.py --search   # 测试搜索
"""

import json
import os
from pathlib import Path
from typing import Dict, List, Optional

# ========== 配置 ==========
TOPIC_INDEX_DIR = Path("topic-index")
INTENT_CORPUS = Path("P0-1-intent-corpus-expanded.jsonl")
OUTPUT_VECTOR_INDEX = Path("topic-index/vector-index.json")
OUTPUT_HYBRID_SEARCH = Path("topic-index/hybrid-search-meta.json")

# ========== 向量索引构建 ==========

def parse_frontmatter(content: str) -> Optional[Dict]:
    """解析 YAML frontmatter"""
    if not content.startswith("---"):
        return None

    frontmatter_end = content.find("---", 3)
    if frontmatter_end == -1:
        return None

    fm_text = content[3:frontmatter_end].strip()
    metadata = {}

    for line in fm_text.split("\n"):
        line = line.strip()

        # index_metadata 部分
        if line.startswith("keyword:"):
            metadata["keyword"] = line.split(":", 1)[1].strip().strip('"')
        elif line.startswith("category:"):
            metadata["category"] = line.split(":", 1)[1].strip().strip('"')
        elif line.startswith("severity_hint:"):
            metadata["severity_hint"] = line.split(":", 1)[1].strip().strip('"')
        elif line.startswith("description:"):
            metadata["description"] = line.split(":", 1)[1].strip().strip('"')
        elif line.startswith("- "):
            # 列表项
            if "related_skills" not in metadata:
                metadata["related_skills"] = []
            metadata["related_skills"].append(line[2:].strip().strip('"'))
        elif line.startswith("fta_codes:"):
            if "fta_codes" not in metadata:
                metadata["fta_codes"] = []

    return metadata if metadata else None

def extract_section_links(content: str) -> List[Dict]:
    """提取各章节的链接"""
    import re
    links = []

    # 匹配 markdown 链接
    link_pattern = r'\[([^\]]+)\]\(([^\)]+)\)'

    for line in content.split("\n"):
        match = re.search(link_pattern, line)
        if match:
            title = match.group(1)
            url = match.group(2)

            # 判断章节
            section = ""
            if "## 设计原理" in content[:content.find(line)]:
                section = "design"
            elif "## 控制平面" in content[:content.find(line)]:
                section = "control_plane"
            elif "## 故障排查" in content[:content.find(line)]:
                section = "troubleshooting"
            elif "## FTA" in content[:content.find(line)]:
                section = "fta"
            elif "## 技能" in content[:content.find(line)]:
                section = "skills"

            links.append({
                "title": title,
                "url": url,
                "section": section
            })

    return links

def build_vector_index() -> Dict:
    """构建向量索引"""
    import re

    index_data = {}

    for filepath in sorted(TOPIC_INDEX_DIR.glob("*-index.md")):
        filename = filepath.name

        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # 解析 frontmatter
        metadata = parse_frontmatter(content)
        if not metadata:
            continue

        # 提取链接
        links = extract_section_links(content)

        # 统计链接数量
        section_counts = {}
        for link in links:
            sec = link["section"]
            section_counts[sec] = section_counts.get(sec, 0) + 1

        index_data[filename] = {
            "keyword": metadata["keyword"],
            "category": metadata["category"],
            "severity_hint": metadata["severity_hint"],
            "description": metadata["description"],
            "related_skills": metadata.get("related_skills", []),
            "fta_codes": metadata.get("fta_codes", []),
            "section_counts": section_counts,
            "total_links": len(links),
            "links": links[:20]  # 只保留前20个链接作为预览
        }

    return index_data

def build_hybrid_search_metadata() -> Dict:
    """构建混合搜索元数据"""

    # 读取 intent corpus 构建 keyword -> category 映射
    keyword_to_category = {}
    category_to_keywords = {}

    if INTENT_CORPUS.exists():
        with open(INTENT_CORPUS, 'r', encoding='utf-8') as f:
            for line in f:
                try:
                    item = json.loads(line.strip())
                    category = item.get("category", "")
                    keywords = item.get("keywords", [])

                    for kw in keywords:
                        if kw not in keyword_to_category:
                            keyword_to_category[kw] = []
                        keyword_to_category[kw].append(category)

                    if category not in category_to_keywords:
                        category_to_keywords[category] = set()
                    category_to_keywords[category].update(keywords)
                except:
                    pass

    # 构建搜索映射
    hybrid_meta = {
        "keyword_to_category": keyword_to_category,
        "category_to_keywords": {k: list(v) for k, v in category_to_keywords.items()},
        "intent_corpus_stats": {
            "total_intents": len(keyword_to_category),
            "categories": list(category_to_keywords.keys())
        }
    }

    return hybrid_meta

# ========== 搜索功能 ==========

def semantic_search(query: str, index_data: Dict, top_k: int = 5) -> List[Dict]:
    """
    简单的语义搜索（基于关键词匹配）

    实际使用时应该用 embedding 模型进行向量相似度匹配
    这里提供一个基于规则的简化实现
    """
    import re

    query_lower = query.lower()
    results = []

    for filename, data in index_data.items():
        score = 0
        matched_tags = []

        # 1. 精确匹配 keyword
        if data["keyword"].lower() in query_lower:
            score += 10

        # 2. 匹配 description
        if data["description"] and any(word in query_lower for word in data["description"].split()):
            score += 5

        # 3. 匹配 search_tags（在 description 中）
        for word in data["description"].split():
            if word.lower() in query_lower:
                matched_tags.append(word)
                score += 2

        # 4. 匹配 related_skills
        for skill in data.get("related_skills", []):
            if skill.lower() in query_lower:
                score += 8

        # 5. 匹配 fta_codes
        for fta in data.get("fta_codes", []):
            if fta.lower() in query_lower:
                score += 6

        if score > 0:
            results.append({
                "filename": filename,
                "keyword": data["keyword"],
                "category": data["category"],
                "severity": data["severity_hint"],
                "score": score,
                "matched_tags": matched_tags,
                "description": data["description"]
            })

    # 排序
    results.sort(key=lambda x: x["score"], reverse=True)
    return results[:top_k]

def intent_route(query: str, hybrid_meta: Dict) -> Dict:
    """
    意图路由：根据查询返回最可能的 category
    """
    query_lower = query.lower()
    category_votes = {}

    # 统计 keyword 匹配
    keyword_to_cat = hybrid_meta.get("keyword_to_category", {})

    for keyword, categories in keyword_to_cat.items():
        if keyword.lower() in query_lower:
            for cat in categories:
                category_votes[cat] = category_votes.get(cat, 0) + 1

    if not category_votes:
        return {"category": "unknown", "confidence": 0, "votes": {}}

    # 找出最高票
    top_cat = max(category_votes.items(), key=lambda x: x[1])
    total_votes = sum(category_votes.values())

    return {
        "category": top_cat[0],
        "confidence": top_cat[1] / total_votes if total_votes > 0 else 0,
        "votes": category_votes
    }

# ========== 主函数 ==========

def main():
    import argparse
    import re

    parser = argparse.ArgumentParser(description="topic-index 向量索引构建")
    parser.add_argument("--search", type=str, help="搜索 query")
    parser.add_argument("--route", type=str, help="意图路由 query")
    args = parser.parse_args()

    if args.search:
        # 搜索模式
        print(f"🔍 搜索: {args.search}")
        index_data = build_vector_index()
        results = semantic_search(args.search, index_data)
        print("\n结果:")
        for i, r in enumerate(results, 1):
            print(f"  {i}. [{r['keyword']}] {r['category']} (score: {r['score']})")
            print(f"     {r['description']}")
        return

    if args.route:
        # 意图路由模式
        print(f"🎯 意图路由: {args.route}")
        hybrid_meta = build_hybrid_search_metadata()
        result = intent_route(args.route, hybrid_meta)
        print(f"\n结果:")
        print(f"  Category: {result['category']}")
        print(f"  Confidence: {result['confidence']:.2f}")
        print(f"  Votes: {result['votes']}")
        return

    # 构建索引
    print("📦 构建向量索引...")

    # 构建 index_data
    index_data = build_vector_index()

    # 保存向量索引
    with open(OUTPUT_VECTOR_INDEX, 'w', encoding='utf-8') as f:
        json.dump(index_data, f, ensure_ascii=False, indent=2)
    print(f"✅ 向量索引已保存: {OUTPUT_VECTOR_INDEX}")

    # 构建混合搜索元数据
    print("📦 构建混合搜索元数据...")
    hybrid_meta = build_hybrid_search_metadata()

    with open(OUTPUT_HYBRID_SEARCH, 'w', encoding='utf-8') as f:
        json.dump(hybrid_meta, f, ensure_ascii=False, indent=2)
    print(f"✅ 混合搜索元数据已保存: {OUTPUT_HYBRID_SEARCH}")

    print(f"\n📊 统计:")
    print(f"  索引文件数: {len(index_data)}")
    print(f"  Intent 关键词数: {len(hybrid_meta.get('keyword_to_category', {}))}")
    print(f"  Category 数: {len(hybrid_meta.get('category_to_keywords', {}))}")

    # 示例搜索
    print("\n🔍 示例搜索:")
    test_queries = [
        "node notready kubelet",
        "pod crashloop oom",
        "certificate expired",
        "dns resolution failed"
    ]

    for query in test_queries:
        results = semantic_search(query, index_data, top_k=3)
        print(f"\n  查询: '{query}'")
        for r in results:
            print(f"    → [{r['keyword']}] {r['category']} (score: {r['score']})")

if __name__ == "__main__":
    main()