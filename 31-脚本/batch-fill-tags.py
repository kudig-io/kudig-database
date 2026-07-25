#!/usr/bin/env python3
"""
KUDIG-DATABASE 批量补齐标签脚本
基于目录映射为所有文档补充标准化 tags 字段。

不删除任何已有标签，只追加缺失的标签。
"""

import sys
import yaml
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path(__file__).parent.parent
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules',
                '.obsidian', '.zread', '.claude', '.codebuddy', '.comate',
                '.github', '.understand-anything'}

# 目录 → 标准标签
DIR_TAGS = {
    "domain-1-architecture-fundamentals": ["k8s", "architecture", "deep-dive"],
    "domain-2-design-principles": ["k8s", "design-principles"],
    "domain-3-control-plane": ["k8s", "control-plane", "deep-dive"],
    "domain-4-workloads": ["k8s", "workload", "pod", "deployment"],
    "domain-5-networking": ["k8s", "networking", "service", "ingress"],
    "domain-6-storage": ["k8s", "storage", "pv", "pvc"],
    "domain-7-security": ["k8s", "security", "rbac", "best-practice"],
    "domain-8-observability": ["k8s", "observability", "prometheus", "monitoring"],
    "domain-9-platform-ops": ["k8s", "devops", "daily-ops"],
    "domain-10-extensions": ["k8s", "crd", "operator"],
    "domain-11-ai-infra": ["k8s", "ai", "gpu", "deep-dive"],
    "domain-12-troubleshooting": ["k8s", "troubleshooting", "guide"],
    "domain-13-docker": ["docker", "container", "best-practice"],
    "domain-14-linux": ["linux", "system-admin", "guide"],
    "domain-15-network-fundamentals": ["networking", "fundamentals"],
    "domain-16-storage-fundamentals": ["storage", "fundamentals"],
    "domain-17-cloud-provider": ["cloud", "multi-cloud"],
    "domain-18-production-operations": ["k8s", "production", "best-practice"],
    "domain-19-papers": ["paper", "research"],
    "domain-20-enterprise-monitoring-alerting": ["observability", "monitoring", "alerting"],
    "domain-21-logging-management-analytics": ["observability", "logging"],
    "domain-22-container-image-management": ["docker", "image", "security"],
    "domain-23-gitops-ci-cd": ["gitops", "cicd", "devops"],
    "domain-24-infrastructure-as-code": ["iac", "terraform"],
    "domain-25-cloud-native-security": ["security", "cloud-native"],
    "domain-26-service-mesh-microservices": ["mesh", "microservices", "istio"],
    "domain-27-multi-cloud-hybrid": ["cloud", "hybrid"],
    "domain-28-enterprise-database-middleware": ["database", "middleware"],
    "domain-29-automated-testing-quality": ["quality", "testing"],
    "domain-30-disaster-recovery-business-continuity": ["disaster-recovery", "backup-restore"],
    "domain-31-hardware": ["hardware"],
    "domain-32-yaml-manifests": ["yaml", "reference"],
    "domain-33-kubernetes-events": ["k8s", "events"],
    "domain-34-cncf-landscape": ["cncf", "ecosystem"],
    "domain-35-ebpf-technology": ["ebpf", "cilium"],
    "domain-36-platform-engineering": ["platform", "idp"],
    "domain-37-edge-computing": ["edge", "kubeedge"],
    "domain-38-webassembly-cloud-native": ["wasm", "cloud-native"],
    "domain-39-supply-chain-security": ["security", "supply-chain"],
    "domain-40-cloud-native-api-gateway": ["gateway", "api"],
}

TOPIC_TAGS = {
    "topic-ai-agent": ["ai", "ai-agent"],
    "topic-ai-coding": ["ai", "ai-coding"],
    "topic-application-architecture": ["architecture", "best-practice"],
    "topic-cheat-sheet": ["cheatsheet", "quick-reference"],
    "topic-deployment": ["deployment"],
    "topic-dictionary": ["dictionary", "reference"],
    "topic-febm": ["febm", "troubleshooting"],
    "topic-fta": ["fta", "troubleshooting"],
    "topic-functions": ["reference"],
    "topic-index": ["index", "reference"],
    "domain-java-kubernetes": ["java", "k8s"],
    "topic-learn": ["learning", "tutorial"],
    "topic-migration": ["migration", "upgrade"],
    "topic-presentations": ["presentation"],
    "topic-publish": ["publish"],
    "topic-qa-corpus": ["qa"],
    "topic-release-notes": ["release-notes"],
    "topic-skills": ["skill", "daily-ops"],
    "topic-structural-trouble-shooting": ["troubleshooting", "guide"],
    "topic-terway": ["terway", "networking", "cni"],
}

# 文件名关键词 → 场景标签
FILENAME_TAGS = {
    "troubleshoot": ["troubleshooting"],
    "troubleshooting": ["troubleshooting"],
    "diagnosis": ["troubleshooting"],
    "fault": ["troubleshooting"],
    "error": ["troubleshooting"],
    "debug": ["troubleshooting"],
    "monitor": ["monitoring"],
    "observability": ["observability"],
    "prometheus": ["prometheus"],
    "grafana": ["grafana"],
    "security": ["security"],
    "rbac": ["rbac"],
    "network": ["networking"],
    "storage": ["storage"],
    "deploy": ["deployment"],
    "backup": ["backup-restore"],
    "restore": ["backup-restore"],
    "migration": ["migration"],
    "upgrade": ["upgrade"],
    "performance": ["performance"],
    "tuning": ["performance"],
    "benchmark": ["performance"],
    "best-practice": ["best-practice"],
    "production": ["production"],
    "cheat": ["quick-reference"],
    "reference": ["reference"],
    "guide": ["guide"],
    "overview": ["deep-dive"],
    "architecture": ["architecture"],
    "setup": ["configuration"],
    "config": ["configuration"],
    "install": ["configuration"],
    "interview": ["interview"],
    "exam": ["exam"],
    "study": ["tutorial"],
    "learn": ["tutorial"],
    "tutorial": ["tutorial"],
    "lecture": ["tutorial"],
    "release": ["release-notes"],
    "change": ["release-notes"],
    "skill": ["daily-ops"],
    "daily": ["daily-ops"],
    "ops": ["daily-ops"],
    "cost": ["cost-optimization"],
    "capacity": ["capacity-planning"],
    "compliance": ["compliance"],
    "audit": ["compliance"],
    "emergency": ["emergency"],
    "incident": ["emergency"],
    "case": ["case-study"],
    "paper": ["paper"],
    "research": ["paper"],
}


def parse_frontmatter(content):
    """Parse frontmatter, return (fm_dict, start_offset, end_offset_in_original)."""
    stripped = content.lstrip()
    if not stripped.startswith("---"):
        return None, 0, 0
    leading = len(content) - len(stripped)
    end = content.find("---", 3 + leading)
    if end == -1:
        return None, 0, 0
    fm_text = stripped[3:end - leading].strip() if end > leading else stripped[3:].strip()
    try:
        fm = yaml.safe_load(fm_text)
        if not fm:
            fm = {}
        return fm, leading, end
    except Exception:
        return None, 0, 0


def get_new_tags(filepath: Path, dir_name: str) -> list:
    """Determine standard tags for a file based on directory and filename."""
    tags = list(DIR_TAGS.get(dir_name, TOPIC_TAGS.get(dir_name, [])))

    # Add filename-based tags
    stem = filepath.stem.lower()
    for keyword, ktags in FILENAME_TAGS.items():
        if keyword in stem:
            for t in ktags:
                if t not in tags:
                    tags.append(t)

    return tags


def fix_file(filepath: Path) -> bool:
    """Add missing tags to a file's frontmatter."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return False

    fm, leading, end_offset = parse_frontmatter(content)
    if fm is None:
        return False

    parts = filepath.relative_to(BASE_DIR).parts
    dir_name = parts[0] if parts else ""

    new_tags = get_new_tags(filepath, dir_name)
    if not new_tags:
        return False

    existing_tags = fm.get("tags", [])
    if not isinstance(existing_tags, list):
        existing_tags = []

    # Merge: add missing tags
    added = []
    for t in new_tags:
        if t not in existing_tags:
            existing_tags.append(t)
            added.append(t)

    if not added:
        return False

    fm["tags"] = existing_tags

    # Rebuild frontmatter block
    new_fm_yaml = yaml.dump(fm, default_flow_style=False, allow_unicode=True, sort_keys=False)
    new_fm_block = "---\n" + new_fm_yaml + "---"

    # Replace in content
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
    print("批量补齐标签...")
    print(f"扫描范围: {len(md_files)} 文件")
    print("=" * 70)

    fixed = 0
    skipped = 0
    dir_counts = defaultdict(int)
    for f in md_files:
        if fix_file(f):
            fixed += 1
            dir_counts[f.relative_to(BASE_DIR).parts[0]] += 1
        else:
            skipped += 1

    print(f"\n修复完成:")
    print(f"  修改: {fixed} 文件")
    print(f"  跳过: {skipped} 文件 (已有完整标签或无 frontmatter)")
    print(f"\n按目录统计:")
    for dir_name, count in sorted(dir_counts.items()):
        print(f"  {dir_name}: {count} 文件")


if __name__ == "__main__":
    main()
