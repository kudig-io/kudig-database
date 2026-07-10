#!/usr/bin/env python3
"""
KUDIG-DATABASE 脚本公共工具函数
共享 frontmatter 解析、路径计算等基础功能
"""

import yaml
from pathlib import Path
from typing import Optional, Dict, Tuple


def parse_frontmatter(content: str) -> Optional[Dict]:
    """
    解析 Markdown 文件的 YAML frontmatter。

    Args:
        content: Markdown 文件完整内容

    Returns:
        解析后的 frontmatter 字典，无 frontmatter 时返回 None
    """
    content = content.lstrip()
    if not content.startswith("---"):
        return None

    end = content.find("---", 3)
    if end == -1:
        return None

    fm_text = content[3:end].strip()
    if not fm_text:
        return None

    try:
        return yaml.safe_load(fm_text) or {}
    except Exception:
        return None


def split_frontmatter(content: str) -> Tuple[Optional[Dict], str]:
    """
    分离 frontmatter 和正文。

    Args:
        content: Markdown 文件完整内容

    Returns:
        (frontmatter_dict, body) 元组。无 frontmatter 时 frontmatter_dict 为 None
    """
    fm = parse_frontmatter(content)
    if fm is None:
        return None, content

    end = content.lstrip().find("---", 3)
    # Adjust end for leading whitespace
    leading = len(content) - len(content.lstrip())
    end = content.find("---", 3 + leading)
    body = content[end + 3:].lstrip("\n")
    return fm, body


def has_frontmatter(content: str) -> bool:
    """检查文件是否包含 YAML frontmatter。"""
    return parse_frontmatter(content) is not None


def find_markdown_files(directory: Path, recursive: bool = True) -> list:
    """
    查找目录下的 Markdown 文件。

    Args:
        directory: 目标目录
        recursive: 是否递归搜索子目录

    Returns:
        排序后的文件路径列表（相对于 directory）
    """
    pattern = "**/*.md" if recursive else "*.md"
    return sorted(directory.glob(pattern))
