#!/usr/bin/env python3
"""批量修复 .md 文件里的 wikilink 与相对路径。

覆盖两类语法:
  [[path|alias]]   → 替换 path 部分
  [text](path)     → 替换 path 部分

替换规则: 把旧前缀替换为新前缀（字符串前缀匹配，最长优先）。
"""
import os
import re
import sys
from pathlib import Path

# (old_prefix, new_prefix) —— 2026-11-25 P2 一键迁移 31→27 域映射
# 来源: reorg-mapping-manifest.json moves + plan §3.1 吸收/拆解/迁出条目
# 注意: 旧 batch-2/4 映射已全部移除(其目标路径已不存在)
MAPPING = [
    # ===== 特殊结构（长度排序自动先于通用域前缀匹配） =====
    ("wiki/17-系统基础/00-总览/", "wiki/01-集群基础与系统/08-生产就绪/"),
    ("wiki/17-系统基础/01-Linux/", "wiki/01-集群基础与系统/09-Linux/"),
    ("wiki/17-系统基础/02-硬件/", "wiki/01-集群基础与系统/10-硬件/"),
    ("wiki/17-系统基础/03-网络基础/", "wiki/05-网络/08-系统网络基础/"),
    ("wiki/17-系统基础/04-K8s事件/", "wiki/01-集群基础与系统/11-K8s事件/"),
    ("wiki/17-系统基础/05-速查卡/", "wiki/01-集群基础与系统/12-速查卡/"),
    ("wiki/17-系统基础/06-知识字典/", "wiki/21-知识字典/"),
    ("wiki/19-故障诊断/08-技能体系/", "skills/09-技能体系/"),
    ("wiki/23-实体/13-云厂商与发行版/", "wiki/16-云厂商/10-云厂商与发行版/"),
    ("wiki/20-最佳实践/04-migration/", "wiki/09-发布变更/07-迁移方案/"),
    # ===== 07-数据库中间件 二级提升（plan §3.2 二级并列保留原语义） =====
    ("wiki/07-数据库中间件/00-总览/", "wiki/06-存储与数据/08-数据库中间件总览/"),
    ("wiki/07-数据库中间件/01-数据库/", "wiki/06-存储与数据/09-数据库/"),
    ("wiki/07-数据库中间件/02-缓存/", "wiki/06-存储与数据/10-缓存/"),
    ("wiki/07-数据库中间件/03-消息队列/", "wiki/06-存储与数据/11-消息队列/"),
    ("wiki/07-数据库中间件/04-时序数据库/", "wiki/06-存储与数据/12-时序数据库/"),
    ("wiki/07-数据库中间件/05-Operator管理/", "wiki/06-存储与数据/13-Operator管理/"),
    ("wiki/07-数据库中间件/06-数据流/", "wiki/06-存储与数据/14-数据流/"),
    ("wiki/07-数据库中间件/07-搜索引擎/", "wiki/06-存储与数据/15-搜索引擎/"),
    ("wiki/07-数据库中间件/08-新型数据库/", "wiki/06-存储与数据/16-新型数据库/"),
    ("wiki/07-数据库中间件/", "wiki/06-存储与数据/"),
    # ===== 域级改号（manifest moves；条目带尾斜杠避免 06-存储误匹配 06-存储与数据） =====
    ("wiki/01-集群基础/", "wiki/01-集群基础与系统/"),
    ("wiki/06-存储/", "wiki/06-存储与数据/"),
    ("wiki/08-安全/", "wiki/07-安全/"),
    ("wiki/09-可观测性/", "wiki/08-可观测性/"),
    ("wiki/10-平台工程/", "wiki/12-平台工程/"),
    ("wiki/11-发布变更/", "wiki/09-发布变更/"),
    ("wiki/12-可靠性/", "wiki/10-可靠性/"),
    ("wiki/13-生产运维/", "wiki/11-生产运维/"),
    ("wiki/14-容器运行时/", "wiki/13-容器运行时/"),
    ("wiki/15-AI基础设施/", "wiki/14-AI基础设施/"),
    ("wiki/16-专项技术/", "wiki/15-专项技术/"),
    ("wiki/18-云厂商/", "wiki/16-云厂商/"),
    ("wiki/19-故障诊断/", "wiki/17-故障诊断/"),
    ("wiki/20-最佳实践/", "wiki/18-最佳实践/"),
    ("wiki/21-生态参考/", "wiki/19-生态参考/"),
    ("wiki/22-概念/", "wiki/20-概念/"),
    ("wiki/23-实体/", "wiki/22-实体/"),
    ("wiki/24-综合/", "wiki/23-综合与研究/"),
    ("wiki/25-研究/", "wiki/23-综合与研究/研究/"),
    ("wiki/26-标签/", "wiki/24-标签/"),
    ("wiki/27-资产/", "wiki/25-资产/"),
    ("wiki/28-源码分析/", "wiki/26-源码分析/"),
    ("wiki/29-消化/", "wiki/23-综合与研究/消化/"),
    ("wiki/31-归档/", "wiki/99-归档/"),
    ("wiki/30-百炼智能体/", "maintaining/01-文档/百炼智能体/"),
    ("wiki/17-系统基础/", "wiki/01-集群基础与系统/"),  # 拆解残余兜底（子域条目更长优先）
]

# 按长度倒序，确保最长前缀优先匹配
MAPPING.sort(key=lambda x: -len(x[0]))
WIKILINK_RE = re.compile(r"\[\[([^\]\|]+)(\|[^\]]+)?\]\]")
MDLINK_RE = re.compile(r"\[([^\]]*)\]\(([^)]+)\)")


# 单独子目录段映射（用于匹配路径中间的旧子目录名，例如 ../01-resource-troubleshooting/x.md）
# 注意: 仅当该段在路径中作为独立目录段出现时才替换，避免误改文件名。
SEGMENT_RENAME = {
    "01-集群基础": "01-集群基础与系统",
}

def apply_segment_rename(path):
    """把 path 中任何匹配 SEGMENT_RENAME 的目录段替换为新名。

    支持形式: 'old/x', '../old/x', '../../old/x', './old/x', 'foo/old/x',
              '中文域/old/x', '中文域/old' (无尾部 slash), '[[old/x]]'。
    """
    # 把路径按 '/' 切分，逐段替换；保留前导 '../' / './'
    changed = False
    # 用正则找每个路径段
    def _repl(m):
        nonlocal changed
        seg = m.group(0)
        if seg in SEGMENT_RENAME:
            changed = True
            return SEGMENT_RENAME[seg]
        return seg
    # 仅替换路径中作为目录段出现（后面紧跟 / 或结尾）的旧名
    # 先处理中间段：'old/' -> 'new/'
    new_path = re.sub(r"(?<![\w\-])([A-Za-z0-9_\-]+)(?=/)", lambda m: _repl_match(m), path)
    # 再处理末尾段（没有尾部 /），仅在前面是 '/' 的情况下
    def _tail(m):
        nonlocal changed
        seg = m.group(1)
        if seg in SEGMENT_RENAME:
            changed = True
            return "/" + SEGMENT_RENAME[seg]
        return m.group(0)
    new_path = re.sub(r"/([\w\-]+)$", _tail, new_path)
    return new_path


def _repl_match(m):
    seg = m.group(1)
    if seg in SEGMENT_RENAME:
        return SEGMENT_RENAME[seg]
    return seg


def rewrite_wikilink(m):
    target = m.group(1)
    alias = m.group(2) or ""
    # 优先用长前缀（域名+子目录）映射
    matched = False
    for old, new in MAPPING:
        if target.startswith(old):
            target = new + target[len(old):]
            matched = True
            break
        old_no_slash = old.rstrip("/")
        if target == old_no_slash or target.startswith(old_no_slash + "/"):
            target = new.rstrip("/") + target[len(old_no_slash):]
            matched = True
            break
    if not matched:
        # 兜底：按目录段重命名（处理 ../old/x 或 old/x 形式）
        target = apply_segment_rename(target)
    return f"[[{target}{alias}]]"


def rewrite_mdlink(m):
    text, href = m.group(1), m.group(2)
    if href.startswith(("http://", "https://", "mailto:", "#", "<")):
        return m.group(0)
    matched = False
    for old, new in MAPPING:
        if href.startswith(old):
            href = new + href[len(old):]
            matched = True
            break
        if href.startswith(old.rstrip("/") + ".md"):
            href = new.rstrip("/") + ".md" + href[len(old.rstrip("/") + ".md"):]
            matched = True
            break
    if not matched:
        href = apply_segment_rename(href)
    return f"[{text}]({href})"


def rewrite_file(path):
    try:
        with open(path, encoding="utf-8") as f:
            content = f.read()
    except UnicodeDecodeError:
        return 0
    new = WIKILINK_RE.sub(rewrite_wikilink, content)
    new = MDLINK_RE.sub(rewrite_mdlink, new)
    if new != content:
        Path(path).write_text(new, encoding="utf-8")
        return 1
    return 0


def main():
    root = os.path.abspath(".")
    changed = 0
    scanned = 0
    skip_dirs = {".git", ".venv", "node_modules", "__pycache__", ".obsidian", ".ruff_cache",
                 "_archives", "code", "99-归档", "assessments", "journal"}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip_dirs]
        for fn in filenames:
            if not fn.endswith(".md"):
                continue
            full = os.path.join(dirpath, fn)
            scanned += 1
            changed += rewrite_file(full)
    print(f"scanned={scanned} changed={changed}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
