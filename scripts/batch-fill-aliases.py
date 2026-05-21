#!/usr/bin/env python3
"""
KUDIG-DATABASE 批量补齐 aliases 字段
基于文件名和标题为文档生成搜索别名。
"""

import re
import yaml
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path(__file__).parent.parent
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules',
                '.obsidian', '.zread', '.claude', '.codebuddy', '.comate',
                '.github', '.understand-anything'}

# 文件名模式 → 别名
STEM_ALIASES = {
    "architecture-overview": ["架构概览", "架构全景图", "系统架构", "architecture overview"],
    "core-components": ["核心组件", "组件深度", "core components"],
    "api-versions": ["API版本", "版本特性", "api versions"],
    "source-code-structure": ["源码结构", "代码目录", "source code"],
    "kubectl-commands": ["kubectl命令", "命令参考", "kubectl reference"],
    "cluster-configuration": ["集群配置", "配置参数", "cluster config"],
    "upgrade-paths": ["升级路径", "版本升级", "upgrade"],
    "multi-tenancy": ["多租户", "多租户架构", "multi-tenancy"],
    "troubleshooting-guide": ["故障排查", "排障指南", "troubleshooting"],
    "best-practices": ["最佳实践", "生产实践", "best practices"],
    "security-architecture": ["安全架构", "安全设计", "security"],
    "observability-architecture": ["可观测性架构", "监控架构", "observability"],
    "performance-tuning": ["性能调优", "性能优化", "performance"],
    "deployment-patterns": ["部署模式", "部署架构", "deployment"],
    "production-operations": ["生产运维", "运维最佳实践", "production ops"],
    "deep-dive": ["深度解析", "深入分析", "deep dive"],
    "complete-guide": ["完整指南", "完全指南", "complete guide"],
    "quick-start": ["快速入门", "快速开始", "quick start"],
    "reference": ["参考", "参考资料", "reference"],
    "overview": ["概述", "概览", "overview", "全景"],
    "fta": ["故障树", "FTA", "fault tree"],
    "cheat-sheet": ["速查卡", "速查", "cheat sheet", "quick reference"],
    "skill": ["技能", "操作技能", "skill"],
    "release-notes": ["发布说明", "版本说明", "release notes"],
    "learning-path": ["学习路径", "学习计划", "learning path"],
}

# 组件名 → 别名
COMPONENT_ALIASES = {
    "etcd": ["etcd", "键值存储", "KV store"],
    "apiserver": ["api-server", "kube-apiserver", "API Server"],
    "scheduler": ["调度器", "kube-scheduler"],
    "kubelet": ["kubelet", "节点代理"],
    "controller-manager": ["控制器管理器", "KCM"],
    "pod": ["Pod", "容器组"],
    "deployment": ["Deployment", "应用部署"],
    "statefulset": ["StatefulSet", "有状态集"],
    "service": ["Service", "服务"],
    "ingress": ["Ingress", "入口"],
    "prometheus": ["Prometheus", "监控"],
    "grafana": ["Grafana", "可视化"],
}


def parse_frontmatter(content):
    """Parse frontmatter, return (fm_dict, leading_whitespace, end_offset)."""
    stripped = content.lstrip()
    if not stripped.startswith("---"):
        return None, 0, 0
    leading = len(content) - len(stripped)
    end = content.find("---", 3 + leading)
    if end == -1:
        return None, 0, 0
    fm_text = content[3 + leading:end].strip()
    try:
        fm = yaml.safe_load(fm_text)
        return fm if fm else {}, leading, end
    except Exception:
        return None, 0, 0


def generate_aliases(filepath: Path, fm: dict) -> list:
    """Generate aliases for a file based on stem and content."""
    aliases = set()
    stem = filepath.stem.lower()

    # From stem patterns
    for pattern, als in STEM_ALIASES.items():
        if pattern in stem:
            aliases.update(als)

    # From component names in stem
    for comp, als in COMPONENT_ALIASES.items():
        if comp in stem:
            aliases.update(als)

    # From existing title
    title = fm.get("title", "")
    if title:
        # Add title as alias (without the file stem part)
        clean_title = re.sub(r'^[\d一二三四五六七八九十]+[\s\-、.]', '', title).strip()
        if clean_title and clean_title != title:
            aliases.add(clean_title[:80])
        # Add English title if present
        title_en = fm.get("title_en", "")
        if title_en:
            aliases.add(title_en[:80])

    # Limit to 10 aliases
    return sorted(list(aliases))[:10]


def fix_file(filepath: Path) -> bool:
    """Add aliases to a file's frontmatter."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return False

    fm, leading, end_offset = parse_frontmatter(content)
    if fm is None:
        return False

    # Skip if already has aliases
    if "aliases" in fm and fm["aliases"]:
        return False

    aliases = generate_aliases(filepath, fm)
    if not aliases:
        return False

    fm["aliases"] = aliases

    # Rebuild frontmatter
    new_fm_yaml = yaml.dump(fm, default_flow_style=False, allow_unicode=True, sort_keys=False)
    new_fm_block = "---\n" + new_fm_yaml + "---"
    new_content = content[:leading] + new_fm_block + content[end_offset + 3:]
    filepath.write_text(new_content, encoding="utf-8")
    return True


def main():
    md_files = []
    for d in sorted(BASE_DIR.iterdir()):
        if not d.is_dir() or d.name in EXCLUDE_DIRS:
            continue
        for f in d.rglob("*.md"):
            if f.name in ("README.md", "MOC.md"):
                continue
            md_files.append(f)

    print("=" * 70)
    print("批量补齐 aliases...")
    print(f"扫描范围: {len(md_files)} 文件")
    print("=" * 70)

    fixed = 0
    skipped = 0
    for f in md_files:
        if fix_file(f):
            fixed += 1
        else:
            skipped += 1

    print(f"\n修复完成:")
    print(f"  修改: {fixed} 文件")
    print(f"  跳过: {skipped} 文件 (已有别名或无 frontmatter)")


if __name__ == "__main__":
    main()
