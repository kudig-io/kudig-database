#!/usr/bin/env python3
"""
检查最近 24 小时内新增/修改的 Markdown 文件中的 wikilink 是否指向不存在的文件。
"""

import re
import sys
from datetime import datetime, timedelta
from pathlib import Path


def extract_wikilinks(text: str) -> list:
    # 基本 wikilink 模式
    pattern = r"\[\[([^\]|]+)(?:\|[^\]]*)?\]\]"
    candidates = re.findall(pattern, text)
    # 过滤 TOML 数组语法（如 [[buildpacks]]、[[order.group]]）
    filtered = []
    for c in candidates:
        target = c.split("|")[0].strip()
        # TOML 数组通常是 [[identifier]] 或 [[table.subtable]]，且不含空格
        if re.fullmatch(r"[a-zA-Z0-9_.-]+", target):
            continue
        filtered.append(c)
    return filtered


def normalize_target(target: str) -> str:
    target = target.split("#")[0]
    target = target.split("?")[0]
    return target.strip()


def build_file_index(project_root: Path) -> set:
    index = set()
    for p in project_root.rglob("*"):
        if p.is_file():
            rel = p.relative_to(project_root)
            index.add(str(rel))
            index.add(str(rel.with_suffix("")))
            index.add(p.name)
            index.add(p.stem)
    return index


def target_exists(target: str, file_index: set, project_root: Path) -> bool:
    target = normalize_target(target)
    if not target:
        return False
    if target in file_index:
        return True
    if (target + ".md") in file_index:
        return True
    direct_path = project_root / target
    if direct_path.exists():
        return True
    if (direct_path.with_suffix(".md")).exists():
        return True
    return False


def main():
    project_root = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
    cutoff = datetime.now() - timedelta(hours=24)

    print("正在构建文件索引...")
    file_index = build_file_index(project_root)
    print(f"文件索引完成，共 {len(file_index)} 个条目")

    recent_md_files = [
        p for p in project_root.rglob("*.md")
        if datetime.fromtimestamp(p.stat().st_mtime) >= cutoff
        and not str(p.relative_to(project_root)).startswith(("_archives/", "."))
    ]

    print(f"\n最近 24 小时新增/修改的 Markdown 文件: {len(recent_md_files)}")

    broken_links = []
    total_links = 0

    for path in recent_md_files:
        rel = path.relative_to(project_root)
        text = path.read_text(encoding="utf-8")
        links = extract_wikilinks(text)
        for link in links:
            total_links += 1
            if not target_exists(link, file_index, project_root):
                broken_links.append((str(rel), link))

    print(f"总 wikilink 数: {total_links}")
    print(f"Broken links: {len(broken_links)}")

    if broken_links:
        print("\n详细列表（前 50）：")
        for src, target in broken_links[:50]:
            print(f"  {src} -> [[{target}]]")
        if len(broken_links) > 50:
            print(f"  ... 还有 {len(broken_links) - 50} 个")

    # 写入报告
    report_path = project_root / "_reports/recent-wikilink-audit-2026-06-26.md"
    report_lines = [
        "---",
        "title: 最近 24 小时新增文档 Wikilink 审计（2026-06-26）",
        "description: 本轮全面补充后新增 Markdown 文件的 wikilink 指向检查",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- audit",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 最近 24 小时新增文档 Wikilink 质量审计",
        "",
        f"- 检查文件数: {len(recent_md_files)}",
        f"- 总 wikilink 数: {total_links}",
        f"- Broken links: {len(broken_links)}",
        "",
    ]
    if broken_links:
        report_lines.append("## Broken Links")
        report_lines.append("")
        for src, target in broken_links:
            safe_target = target.replace("[", "").replace("]", "")
            report_lines.append(f"- `{src}` -> `[[{safe_target}]]`")
    else:
        report_lines.append("未发现 broken wikilink。")
    report_path.write_text("\n".join(report_lines), encoding="utf-8")
    print(f"\n报告已写入: {report_path}")

    return len(broken_links)


if __name__ == "__main__":
    sys.exit(main())
