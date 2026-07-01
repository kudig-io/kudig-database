#!/usr/bin/env python3
"""
根据 audit 报告修复 broken wikilink。
策略：
1. 以 README 结尾的目标 → 补全为 README.md
2. 其他不存在目标 → 转换为纯文本 display
"""

import re
from pathlib import Path


def parse_audit(report_path: Path) -> dict:
    """解析审计报告，返回 {source_file: [target, ...]}。"""
    result = {}
    text = report_path.read_text(encoding="utf-8")
    for line in text.splitlines():
        line = line.strip()
        if not line.startswith("-") or "->" not in line:
            continue
        # 格式: - `source` -> `[[target]]`
        parts = line.split("->")
        if len(parts) != 2:
            continue
        src_part = parts[0].strip("- `")
        tgt_part = parts[1].strip(" `")
        src = src_part.strip()
        tgt = tgt_part.strip("`[]")
        result.setdefault(src, []).append(tgt)
    return result


def fix_link(match, target: str, file_index: set):
    """修复单个 wikilink。"""
    inner = match.group(1)
    if "|" in inner:
        raw_target, display = inner.split("|", 1)
    else:
        raw_target = inner
        display = inner

    normalized = raw_target.strip()

    # 如果目标以 README 结尾，补全为 README.md
    if normalized.endswith("README"):
        fixed_target = normalized + ".md"
        if fixed_target in file_index:
            return f"[[{fixed_target}|{display.strip()}]]"

    # 否则转换为纯文本
    return display.strip()


def build_file_index(project_root: Path) -> set:
    """构建项目文件索引。"""
    index = set()
    for p in project_root.rglob("*"):
        if p.is_file():
            rel = p.relative_to(project_root)
            index.add(str(rel))
            index.add(str(rel.with_suffix("")))
            index.add(p.name)
            index.add(p.stem)
    return index


def main():
    project_root = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
    report_path = project_root / "_reports/new-wikilink-audit-2026-06-26.md"

    broken_map = parse_audit(report_path)
    print(f"发现 {len(broken_map)} 个文件存在 broken links")

    file_index = build_file_index(project_root)
    print(f"文件索引完成，共 {len(file_index)} 个条目")

    fixed_count = 0
    converted_count = 0

    for src, targets in broken_map.items():
        src_path = project_root / src
        if not src_path.exists():
            print(f"  跳过不存在的源文件: {src}")
            continue

        text = src_path.read_text(encoding="utf-8")
        original = text

        for target in targets:
            # 转义正则特殊字符
            escaped_target = re.escape(target)
            # 匹配 [[target|display]] 或 [[target]]
            pattern = re.compile(rf"\[\[{escaped_target}(?:\|([^\]]*))?\]\]")

            def repl(match):
                nonlocal fixed_count, converted_count
                if "|" in match.group(0):
                    display = match.group(1)
                else:
                    display = target

                # README 结尾补全
                if target.endswith("README"):
                    fixed_target = target + ".md"
                    if fixed_target in file_index or (project_root / fixed_target).exists():
                        fixed_count += 1
                        return f"[[{fixed_target}|{display.strip()}]]"

                # 其他转换为纯文本
                converted_count += 1
                return display.strip()

            text = pattern.sub(repl, text)

        if text != original:
            src_path.write_text(text, encoding="utf-8")
            print(f"  已修复: {src}")

    print(f"\n修复完成：")
    print(f"  README 补全: {fixed_count}")
    print(f"  转纯文本: {converted_count}")
    print(f"  涉及文件: {len(broken_map)}")


if __name__ == "__main__":
    main()
