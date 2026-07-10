#!/usr/bin/env python3
"""
KUDIG-DATABASE Chunk 元数据增强脚本
用途：扫描知识库，为每篇文档计算 quality_score 和 content_type，
      输出元数据映射 JSON 供向量化管道消费。

用法:
    python3 enhance-metadata.py --output corpus-config/metadata-enhanced.json
    python3 enhance-metadata.py --inplace   # 直接修改 front matter（慎用）
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path
from collections import defaultdict

# 知识库 domain 目录（已从 domain-NN-slug 改为中文命名）
DOMAINS = {'集群基础','工作负载','网络','存储','安全','可观测性','平台工程','发布变更','可靠性','故障诊断','生产运维','云厂商','容器运行时','AI基础设施','专项技术','数据库中间件','系统基础','清单模式','生态参考','应用模式'}

# 基于 rag-full-profile.yaml 的 include 规则
INCLUDE_PATTERNS = [
    "集群基础/**/*.md",
    "工作负载/**/*.md",
    "网络/**/*.md",
    "存储/**/*.md",
    "安全/**/*.md",
    "可观测性/**/*.md",
    "平台工程/**/*.md",
    "发布变更/**/*.md",
    "可靠性/**/*.md",
    "故障诊断/**/*.md",
    "生产运维/**/*.md",
    "云厂商/**/*.md",
    "容器运行时/**/*.md",
    "AI基础设施/**/*.md",
    "专项技术/**/*.md",
    "数据库中间件/**/*.md",
    "系统基础/**/*.md",
    "清单模式/**/*.md",
    "生态参考/**/*.md",
    "应用模式/**/*.md",
    "concepts/**/*.md",
    "skills/**/*.md",
    "entities/**/*.md",
    "references/**/*.md",
    "synthesis/**/*.md",
]

EXCLUDE_PATTERNS = [
    "**/_reports/**",
    "**/_meta/**",
    "**/_staging/**",
    "**/_raw/**",
    "**/.git/**",
    "**/.venv/**",
    "**/.ruff_cache/**",
    "**/.obsidian/**",
    "**/.zread/**",
    "**/.wiki-meta/**",
    "**/_archives/**",
    "**/node_modules/**",
    "**/*-archive/**",
    "**/archive/**",
    "**/topic-release-notes/**",
    "**/topic-index/**",
    "**/sandbox/**",
    "**/topic-dictionary/**",
    "**/98-merged-indexes/**",  # 索引页知识密度低
]


def should_include(path: Path) -> bool:
    """判断文件是否在 RAG 导入范围内"""
    path_str = str(path)
    for pat in EXCLUDE_PATTERNS:
        if path.match(pat):
            return False
    for pat in INCLUDE_PATTERNS:
        if path.match(pat):
            return True
    return False


def parse_frontmatter(text: str) -> tuple[dict, str]:
    """解析 YAML front matter，返回 (meta_dict, body)"""
    if text.startswith("---"):
        parts = text.split("---", 2)
        if len(parts) >= 3:
            import yaml
            try:
                meta = yaml.safe_load(parts[1]) or {}
                body = parts[2]
                return meta, body
            except Exception:
                pass
    return {}, text


def detect_content_type(path: Path, body: str, meta: dict) -> str:
    """基于路径和内容特征判断内容类型"""
    p = str(path).lower()

    # 路径级规则
    if "/topic-cheat-sheet/" in p or "/cheat-sheet/" in p:
        return "cheat-sheet"
    if "/topic-dictionary/" in p or "/dictionary/" in p or "/entities/" in p:
        return "glossary"
    if "/topic-skills/" in p or "/skills/" in p:
        return "skill"
    if "/references/" in p or "/manifests-patterns/" in p:
        return "reference"
    if "98-merged-indexes" in p or "/index" in p or path.name.startswith("00-"):
        return "index"
    if "/synthesis/" in p:
        return "synthesis"
    if "/topic-fta/" in p or "/topic-febm/" in p or "/troubleshooting-diagnostics/" in p:
        return "troubleshooting"
    if "/topic-learn/" in p or "/training-" in p or "/lecturer/" in p:
        return "training"
    if "/topic-presentations/" in p or "/presentations/" in p:
        return "presentation"

    # 内容级规则
    code_blocks = len(re.findall(r"```[a-z]*", body))
    tables = body.count("| ")
    headings = len(re.findall(r"^#{2,6} ", body, re.MULTILINE))
    words = len(body)

    if tables > 20 and words < 3000:
        return "cheat-sheet"
    if code_blocks > 5 and headings > 10 and words > 5000:
        return "deep-dive"
    if words < 800 and headings <= 3:
        return "overview"

    return "article"


def compute_quality_score(path: Path, body: str, meta: dict) -> float:
    """
    计算文档质量分 (0.0 ~ 1.0)
    基于 front matter 完整性、内容深度、结构化程度
    """
    score = 0.0

    # 1. Front matter 完整性 (最高 0.35)
    required_fields = ["title", "category", "tags", "sources", "created", "updated"]
    for field in required_fields:
        if field in meta and meta[field]:
            score += 0.35 / len(required_fields)

    # 2. 内容规模 (最高 0.25)
    words = len(body)
    if words > 10000:
        score += 0.25
    elif words > 5000:
        score += 0.20
    elif words > 2000:
        score += 0.15
    elif words > 500:
        score += 0.08
    else:
        score += 0.02

    # 3. 结构化程度 (最高 0.25)
    # Heading 深度
    h2 = len(re.findall(r"^## ", body, re.MULTILINE))
    h3 = len(re.findall(r"^### ", body, re.MULTILINE))
    h4_plus = len(re.findall(r"^#{4,6} ", body, re.MULTILINE))
    if h2 >= 5 and h3 >= 3:
        score += 0.15
    elif h2 >= 3:
        score += 0.08
    else:
        score += 0.02

    # 代码块 / 表格 / 列表
    code_blocks = len(re.findall(r"```[a-z]*", body))
    tables = len(re.findall(r"^\|.*\|", body, re.MULTILINE))
    lists = len(re.findall(r"^\s*[-*] ", body, re.MULTILINE))
    if code_blocks >= 3 or tables >= 5 or lists >= 10:
        score += 0.10
    elif code_blocks >= 1 or tables >= 1:
        score += 0.05

    # 4. 专业特征 (最高 0.15)
    # 包含 k8s 版本号、命令、配置示例
    has_k8s_version = bool(re.search(r"v?1\.[2-3][0-9]", body))
    has_command = bool(re.search(r"`kubectl |`helm |`istioctl ", body))
    has_yaml = "apiVersion:" in body or "kind:" in body
    if has_k8s_version:
        score += 0.05
    if has_command:
        score += 0.05
    if has_yaml:
        score += 0.05

    return round(min(score, 1.0), 3)


def detect_domain(path: Path) -> str:
    """从路径提取 domain 标签"""
    parts = path.parts
    for i, part in enumerate(parts):
        if part in DOMAINS:
            return part
    return "other"


def main():
    parser = argparse.ArgumentParser(description="KUDIG 元数据增强")
    parser.add_argument("--output", "-o", default="corpus-config/metadata-enhanced.json",
                        help="输出 JSON 文件路径")
    parser.add_argument("--inplace", action="store_true",
                        help="直接修改文件 front matter（默认只输出映射）")
    parser.add_argument("--root", default=".", help="知识库根目录")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    results = {}
    stats = defaultdict(int)
    quality_buckets = defaultdict(int)

    md_files = list(root.rglob("*.md"))
    print(f"扫描到 {len(md_files)} 个 Markdown 文件...")

    for md_path in md_files:
        rel_path = md_path.relative_to(root)
        if not should_include(rel_path):
            continue

        try:
            text = md_path.read_text(encoding="utf-8")
        except Exception as e:
            print(f"  跳过 (读取失败): {rel_path} — {e}", file=sys.stderr)
            continue

        meta, body = parse_frontmatter(text)
        content_type = detect_content_type(rel_path, body, meta)
        quality_score = compute_quality_score(rel_path, body, meta)
        domain = detect_domain(rel_path)

        # 基于 rag-full-profile.yaml 的 priority
        priority = "medium"
        p = str(rel_path)
        if any(x in p for x in [
            "domain-01-", "domain-02-", "domain-03-", "domain-04-",
            "domain-05-", "domain-06-", "domain-07-", "domain-08-",
            "domain-09-", "domain-10-", "concepts/", "skills/", "synthesis/"
        ]):
            priority = "high"
        elif any(x in p for x in [
            "domain-11/01-", "domain-11/02-", "domain-11/03-", "domain-11/04-",
            "domain-12-", "domain-13-", "domain-14-", "domain-15-",
            "domain-16-", "domain-18-", "entities/", "references/"
        ]):
            priority = "medium"
        elif any(x in p for x in ["domain-17-", "domain-19-", "domain-20-"]):
            priority = "low"

        record = {
            "path": str(rel_path),
            "domain": domain,
            "content_type": content_type,
            "quality_score": quality_score,
            "priority": priority,
            "word_count": len(body),
            "has_frontmatter": bool(meta),
            "frontmatter_fields": list(meta.keys()) if meta else [],
        }
        results[str(rel_path)] = record
        stats[content_type] += 1
        quality_buckets[f"{int(quality_score * 10) / 10:.1f}"] += 1

    # 写入输出
    output_path = root / args.output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump({
            "generated_at": "2026-05-21T23:14:37+08:00",
            "total_documents": len(results),
            "content_type_distribution": dict(stats),
            "quality_distribution": dict(quality_buckets),
            "documents": results,
        }, f, ensure_ascii=False, indent=2)

    print(f"\n✅ 元数据增强完成: {output_path}")
    print(f"   共处理 {len(results)} 篇文档\n")

    print("内容类型分布:")
    for ctype, count in sorted(stats.items(), key=lambda x: -x[1]):
        pct = count / len(results) * 100
        print(f"  {ctype:20s} {count:4d} ({pct:5.1f}%)")

    print("\n质量分分布:")
    for bucket in sorted(quality_buckets.keys()):
        count = quality_buckets[bucket]
        pct = count / len(results) * 100
        bar = "█" * int(pct / 2)
        print(f"  {bucket} {count:4d} ({pct:5.1f}%) {bar}")

    # 低质量文档警告
    low_quality = [r for r in results.values() if r["quality_score"] < 0.3]
    if low_quality:
        print(f"\n⚠️  发现 {len(low_quality)} 篇 quality_score < 0.3 的低质量文档，建议复查:")
        for r in sorted(low_quality, key=lambda x: x["quality_score"])[:10]:
            print(f"  {r['quality_score']:.2f}  {r['path']}")
        if len(low_quality) > 10:
            print(f"  ... 还有 {len(low_quality) - 10} 篇")


if __name__ == "__main__":
    main()
