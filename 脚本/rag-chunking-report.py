#!/usr/bin/env python3
"""
KUDIG-DATABASE RAG Chunking 优化报告
1. 为 domain-1~12 核心文档添加 chunking 标记
2. 生成 > 500 行长文档报告
"""

import re
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path(__file__).parent.parent
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules',
                '.obsidian', '.zread', '.claude', '.codebuddy', '.comate',
                '.github', '.understand-anything'}


def add_chunk_markers(filepath: Path) -> bool:
    """Add chunk markers based on headings in a file."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return False

    # Check if already has chunk markers
    if "<!-- chunk:" in content:
        return False

    # Add chunk markers before H2 headings
    new_lines = []
    for line in content.split("\n"):
        if re.match(r'^##\s+\S', line):
            heading_text = re.sub(r'^##\s+', '', line).strip()
            heading_text = re.sub(r'\s*\{.*\}\s*$', '', heading_text).strip()
            new_lines.append(f"<!-- chunk: {heading_text} -->")
        new_lines.append(line)

    new_content = "\n".join(new_lines)
    if new_content != content:
        filepath.write_text(new_content, encoding="utf-8")
        return True
    return False


def main():
    # 1. Add chunk markers to domain-1~12
    print("=" * 70)
    print("RAG Chunking 优化")
    print("=" * 70)

    chunked = 0
    for i in range(1, 13):
        domain_dir = BASE_DIR / f"domain-{i}-"
        # Find the actual directory (might have different name suffix)
        for d in BASE_DIR.iterdir():
            if d.is_dir() and d.name.startswith(f"domain-{i}-"):
                domain_dir = d
                break
        else:
            continue

        if not domain_dir.exists():
            continue

        for f in sorted(domain_dir.glob("*.md")):
            if f.name in ("README.md", "MOC.md"):
                continue
            if add_chunk_markers(f):
                chunked += 1

    print(f"\nChunking 标记添加: {chunked} 文件 (domain-1~12)")

    # 2. Report long documents
    print(f"\n长文档报告 (> 500 行):")
    long_docs = []
    for d in sorted(BASE_DIR.iterdir()):
        if not d.is_dir() or d.name in EXCLUDE_DIRS:
            continue
        for f in d.rglob("*.md"):
            if f.name in ("README.md", "MOC.md"):
                continue
            try:
                with open(f, 'r', encoding='utf-8') as fh:
                    lines = sum(1 for _ in fh)
                if lines > 500:
                    long_docs.append((str(f.relative_to(BASE_DIR)), lines))
            except Exception:
                pass

    long_docs.sort(key=lambda x: -x[1])
    print(f"  总计: {len(long_docs)} 篇文档超过 500 行")
    print(f"\n  Top 20 最长文档:")
    for path, lines in long_docs[:20]:
        print(f"    {lines:5d} 行  {path}")

    # 3. Write report
    report_path = BASE_DIR / "reports" / "rag-chunking-report.md"
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write(f"# RAG Chunking 优化报告\n\n")
        f.write(f"> 生成日期: 2026-05-20\n\n")
        f.write(f"## Chunking 标记\n\n")
        f.write(f"- 已添加 chunk 标记: {chunked} 文件\n")
        f.write(f"- 标记位置: domain-1 ~ domain-12 核心文档\n")
        f.write(f"- 标记格式: `<!-- chunk: 章节标题 -->`\n\n")
        f.write(f"## 长文档报告\n\n")
        f.write(f"共 {len(long_docs)} 篇文档超过 500 行，建议拆分:\n\n")
        f.write(f"| 行数 | 文件 |\n|---|---|\n")
        for path, lines in long_docs[:50]:
            f.write(f"| {lines} | {path} |\n")

    print(f"\n报告已写入: {report_path}")


if __name__ == "__main__":
    main()
