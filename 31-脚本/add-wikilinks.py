#!/usr/bin/env python3
"""
KUDIG-DATABASE 双向链接增强脚本
扫描所有 Markdown 文档，在末尾追加 Obsidian 风格的 [[wikilinks]]。

策略:
1. 基于目录邻近性: 同目录文档互相添加链接
2. 基于文件名关键词匹配: 在正文中提到其他文档文件名时添加链接
3. 基于 MOC 关联: 在每篇文档中引用所属 MOC
4. 基于跨域关联: domain-12(troubleshooting) 链接所有 domain 的故障排查章节

重要: 只在文件末尾追加 "## 相关文档" 章节，不修改已有内容。
"""

import re
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path(__file__).parent.parent
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules',
                '.obsidian', '.zread', '.claude', '.codebuddy', '.comate',
                '.github', '.understand-anything', 'topic-qa-corpus',
                'topic-index', 'topic-dictionary', 'topic-release-notes'}


def find_md_files(directory: Path, recursive: bool = True) -> list:
    """Find markdown files in directory."""
    pattern = "**/*.md" if recursive else "*.md"
    return sorted(directory.glob(pattern))


def parse_frontmatter(content: str) -> dict:
    """Parse YAML frontmatter."""
    content = content.lstrip()
    if not content.startswith("---"):
        return {}
    end = content.find("---", 3)
    if end == -1:
        return {}
    try:
        fm = __import__("yaml").safe_load(content[3:end].strip())
        return fm if fm else {}
    except Exception:
        return {}


def has_section(content: str, section_title: str) -> bool:
    """Check if content already has a section with given title."""
    return bool(re.search(rf'^##\s+{re.escape(section_title)}\s*$', content, re.MULTILINE))


def get_all_docs_by_dir() -> dict:
    """Build index of all docs by directory."""
    doc_index = defaultdict(list)
    for d in sorted(BASE_DIR.iterdir()):
        if not d.is_dir():
            continue
        if d.name in EXCLUDE_DIRS:
            continue
        for f in find_md_files(d, recursive=False):
            if f.name in ("README.md", "MOC.md"):
                continue
            doc_index[d.name].append(f)
    return doc_index


def get_title(filepath: Path) -> str:
    """Extract title from a markdown file."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return filepath.stem

    fm = parse_frontmatter(content)
    if "title" in fm:
        return fm["title"]

    heading_match = re.search(r'^#{1,2}\s+(.+?)(?:\s*\{.*\})?$', content, re.MULTILINE)
    if heading_match:
        return re.sub(r'[\U0001f300-\U0001f9ff]', '', heading_match.group(1)).strip()

    return filepath.stem


def get_title_short(filepath: Path, max_len: int = 60) -> str:
    """Get shortened title."""
    title = get_title(filepath)
    if len(title) > max_len:
        title = title[:max_len] + "..."
    return title


def add_links_to_file(filepath: Path, links: list, base_dir: Path) -> bool:
    """Add related documents section to a file. Returns True if modified."""
    if not links:
        return False

    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return False

    # Check if already has related docs section
    if has_section(content, "相关文档"):
        return False

    # Check if already has wikilinks section
    if "## Obsidian 相关文档" in content:
        return False

    # Build the related docs section
    link_lines = []
    for link_path in links[:15]:  # Limit to 15 links
        title = get_title_short(link_path)
        rel = link_path.relative_to(base_dir)
        link_lines.append(f"- [[{rel}|{title}]]")

    section = f"\n\n---\n\n## Obsidian 相关文档\n\n" + "\n".join(link_lines) + "\n"

    # Append to end of file
    content = content.rstrip() + section
    filepath.write_text(content, encoding="utf-8")
    return True


def main():
    import yaml  # ensure yaml is available

    doc_index = get_all_docs_by_dir()
    total_modified = 0
    total_skipped = 0

    # Build global filename -> filepath index
    filename_index = defaultdict(list)
    for dir_name, files in doc_index.items():
        for f in files:
            filename_index[f.stem].append(f)
            # Also index by key words in filename
            for part in re.split(r'[-_]', f.stem):
                if len(part) > 3:
                    filename_index[part].append(f)

    print("=" * 60)
    print("双向链接增强扫描...")
    print("=" * 60)

    for dir_name in sorted(doc_index.keys()):
        files = doc_index[dir_name]
        dir_modified = 0

        for filepath in files:
            links = []

            # Strategy 1: Link to MOC
            moc_path = BASE_DIR / dir_name / "MOC.md"
            if moc_path.exists():
                links.append(moc_path)

            # Strategy 2: Link to other docs in same directory (up to 10)
            same_dir = [f for f in files if f != filepath]
            if same_dir:
                # Sort by filename proximity (first 10 neighbors)
                stem = filepath.stem
                # Prefer files with similar prefixes
                prefix = re.match(r'^(\d+)', stem)
                prefix_num = prefix.group(1) if prefix else None

                neighbors = []
                if prefix_num:
                    # Find adjacent numbered files
                    for f in same_dir:
                        f_prefix = re.match(r'^(\d+)', f.stem)
                        if f_prefix:
                            neighbors.append(f)

                # If not enough neighbors, add remaining
                if len(neighbors) < 10:
                    for f in same_dir:
                        if f not in neighbors:
                            neighbors.append(f)
                        if len(neighbors) >= 10:
                            break

                links.extend(neighbors[:10])

            # Strategy 3: Cross-domain troubleshooting links
            if "troubleshoot" in dir_name.lower() or "troubleshoot" in filepath.name.lower():
                # Link to relevant FTA
                for fta_file in sorted((BASE_DIR / "topic-fta" / "list").glob("*.md")):
                    if fta_file.name != "README.md":
                        links.append(fta_file)
                        if len(links) > 20:
                            break

            # Strategy 4: Link to README of same directory
            readme = BASE_DIR / dir_name / "README.md"
            if readme.exists() and readme != filepath:
                links.insert(1, readme)  # After MOC

            modified = add_links_to_file(filepath, links, BASE_DIR)
            if modified:
                dir_modified += 1
                total_modified += 1
            else:
                total_skipped += 1

        if dir_modified > 0:
            print(f"  {dir_name}: {dir_modified} files modified")

    print()
    print("=" * 60)
    print(f"双向链接增强完成:")
    print(f"  修改: {total_modified} 文件")
    print(f"  跳过: {total_skipped} 文件 (已有相关章节)")
    print("=" * 60)


if __name__ == "__main__":
    main()
