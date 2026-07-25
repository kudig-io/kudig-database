#!/usr/bin/env python3
"""
cross_refs 批量生成脚本
为所有 domain-* 和 topic-* 文档建立交叉引用 (cross_refs 字段)

交叉引用策略:
  1. domain → 相关 domain (基于知识图谱依赖)
  2. domain → topic-fta/list (对应组件的故障树)
  3. domain → topic-skills (相关运维技能)
  4. domain → topic-cheat-sheet (相关速查卡)
  5. domain → topic-structural-trouble-shooting (结构化排障)
  6. topic-fta → domain (对应组件深度文档)
  7. topic-fta → topic-skills (相关技能)
  8. topic-application-architecture → 相关 domain
"""

import os
import re
import yaml
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")

# ============================================================
# 组件→domain 映射 (用于 FTA → domain 关联)
# ============================================================
COMPONENT_DOMAIN_MAP = {
    'etcd': 'domain-3-control-plane',
    'apiserver': 'domain-3-control-plane',
    'scheduler': 'domain-3-control-plane',
    'controller-manager': 'domain-3-control-plane',
    'kubelet': 'domain-3-control-plane',
    'kube-proxy': 'domain-5-networking',
    'coredns': 'domain-5-networking',
    'ingress': 'domain-5-networking',
    'service': 'domain-5-networking',
    'networkpolicy': 'domain-5-networking',
    'pv': 'domain-6-storage',
    'pvc': 'domain-6-storage',
    'storageclass': 'domain-6-storage',
    'statefulset': 'domain-4-workloads',
    'deployment': 'domain-4-workloads',
    'daemonset': 'domain-4-workloads',
    'job': 'domain-4-workloads',
    'cronjob': 'domain-4-workloads',
    'pod': 'domain-4-workloads',
    'hpa': 'domain-4-workloads',
    'rbac': 'domain-7-security',
    'serviceaccount': 'domain-7-security',
    'secret': 'domain-7-security',
    'networkpolicy': 'domain-7-security',
    'prometheus': 'domain-8-observability',
    'grafana': 'domain-8-observability',
    'fluentd': 'domain-8-observability',
    'loki': 'domain-8-observability',
    'jaeger': 'domain-8-observability',
    'containerd': 'domain-13-docker',
    'docker': 'domain-13-docker',
    'cri-o': 'domain-13-docker',
    'istio': 'domain-26-service-mesh-microservices',
    'envoy': 'domain-26-service-mesh-microservices',
    'cilium': 'domain-5-networking',
    'flannel': 'domain-5-networking',
    'calico': 'domain-5-networking',
    'helm': 'domain-10-extensions',
    'argocd': 'domain-23-gitops-ci-cd',
    'flux': 'domain-23-gitops-ci-cd',
    'harbor': 'domain-22-container-image-management',
    'node': 'domain-3-control-plane',
    'cluster': 'domain-1-architecture-fundamentals',
}

# domain 间的依赖关系 (来自知识图谱)
DOMAIN_DEPENDENCIES = {
    'domain-1-architecture-fundamentals': ['domain-13-docker', 'domain-2-design-principles'],
    'domain-2-design-principles': ['domain-1-architecture-fundamentals', 'domain-3-control-plane'],
    'domain-3-control-plane': ['domain-2-design-principles', 'domain-4-workloads', 'domain-5-networking', 'domain-6-storage', 'domain-7-security'],
    'domain-4-workloads': ['domain-3-control-plane', 'domain-8-observability'],
    'domain-5-networking': ['domain-3-control-plane', 'domain-15-network-fundamentals', 'domain-8-observability'],
    'domain-6-storage': ['domain-3-control-plane', 'domain-16-storage-fundamentals'],
    'domain-7-security': ['domain-3-control-plane', 'domain-8-observability'],
    'domain-8-observability': ['domain-3-control-plane', 'domain-4-workloads', 'domain-5-networking', 'domain-9-platform-ops'],
    'domain-9-platform-ops': ['domain-8-observability', 'domain-10-extensions', 'domain-12-troubleshooting'],
    'domain-10-extensions': ['domain-9-platform-ops'],
    'domain-11-ai-infra': ['domain-4-workloads', 'domain-5-networking'],
    'domain-12-troubleshooting': ['domain-3-control-plane', 'domain-5-networking', 'domain-8-observability'],
    'domain-23-gitops-ci-cd': ['domain-9-platform-ops', 'domain-24-infrastructure-as-code'],
    'domain-26-service-mesh-microservices': ['domain-5-networking', 'domain-7-security'],
}

# topic-cheat-sheet 与 domain 的关联
CHEATSHEET_DOMAIN_MAP = {
    'k8s': ['domain-1-architecture-fundamentals', 'domain-3-control-plane', 'domain-4-workloads'],
    'linux': ['domain-14-linux'],
    'docker': ['domain-13-docker'],
    'networking': ['domain-5-networking', 'domain-15-network-fundamentals'],
    'tls-pki': ['domain-7-security', 'domain-25-cloud-native-security'],
    'promql': ['domain-8-observability', 'domain-20-enterprise-monitoring-alerting'],
    'sql': ['domain-28-enterprise-database-middleware'],
    'go': ['domain-11-ai-infra'],
    'git': ['domain-23-gitops-ci-cd'],
    'kubectl-scene-cheatsheet': ['domain-1-architecture-fundamentals', 'domain-3-control-plane'],
}


def get_component_from_filename(filename: str) -> str:
    """从文件名提取组件名"""
    name = filename.replace('.md', '').replace('-fta', '').replace('-troubleshooting', '')
    # 去掉数字前缀
    name = re.sub(r'^\d+-', '', name)
    return name.lower()


def build_file_index() -> dict:
    """构建文件索引: {component_name: [filepath, ...]}"""
    index = defaultdict(list)
    exclude_dirs = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules', '.obsidian', '.zread', '.claude', '.codebuddy', '.comate', '.github'}

    for root, dirs, files in os.walk(BASE_DIR):
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in files:
            if f.endswith('.md'):
                fp = Path(root) / f
                rel = fp.relative_to(BASE_DIR)
                component = get_component_from_filename(f)
                index[component].append(rel)
    return index


def parse_frontmatter(content: str) -> tuple:
    """解析 YAML front matter"""
    stripped = content.lstrip()
    if not stripped.startswith('---'):
        return {}, content
    end_match = re.search(r'\n---\s*\n', stripped[4:])
    if not end_match:
        return {}, content
    yaml_str = stripped[4:end_match.start() + 4]
    body = stripped[end_match.end() + 4:]
    try:
        fm = yaml.safe_load(yaml_str) or {}
        return fm, body
    except yaml.YAMLError:
        return {}, content


def update_frontmatter(filepath: Path, cross_refs: list) -> bool:
    """更新文件的 front matter 中的 cross_refs 字段"""
    try:
        content = filepath.read_text(encoding='utf-8')
    except Exception:
        return False

    fm, body = parse_frontmatter(content)
    if not fm:
        return False

    # 如果已有 cross_refs 且非空, 跳过
    if fm.get('cross_refs'):
        return False

    fm['cross_refs'] = cross_refs
    fm_yaml = yaml.dump(fm, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
    new_content = f"---\n{fm_yaml}---\n\n{body.lstrip()}"
    filepath.write_text(new_content, encoding='utf-8')
    return True


def generate_cross_refs_for_domain(filepath: Path, file_index: dict) -> list:
    """为 domain-* 文档生成 cross_refs"""
    rel = filepath.relative_to(BASE_DIR)
    parts = rel.parts
    domain_dir = [p for p in parts if p.startswith('domain-')]
    if not domain_dir:
        return []
    domain = domain_dir[0]
    filename = filepath.stem
    component = get_component_from_filename(filename)
    refs = []

    # 1. 相关 domain
    for dep in DOMAIN_DEPENDENCIES.get(domain, []):
        dep_dir = BASE_DIR / dep
        if dep_dir.exists():
            refs.append({"type": "domain", "path": f"../{dep}/", "label": f"相关知识域: {dep}"})

    # 2. 对应 FTA
    fta_dir = BASE_DIR / 'topic-fta' / 'list'
    if fta_dir.exists():
        # 尝试匹配组件名
        for fta_file in fta_dir.glob('*-fta.md'):
            fta_component = fta_file.stem.replace('-fta', '')
            if fta_component in component or component in fta_component:
                refs.append({"type": "fta", "path": f"../topic-fta/list/{fta_file.name}", "label": f"故障树: {fta_component}"})
                break

    # 3. 对应 skills
    skills_dir = BASE_DIR / 'topic-skills'
    if skills_dir.exists():
        for skill_file in skills_dir.glob('*.md'):
            skill_name = skill_file.stem
            if component in skill_name or skill_name.split('-', 1)[-1] in component:
                refs.append({"type": "skill", "path": f"../topic-skills/{skill_file.name}", "label": f"运维技能: {skill_name}"})

    # 4. 对应速查卡
    for cs_name, cs_domains in CHEATSHEET_DOMAIN_MAP.items():
        if domain in cs_domains:
            cs_file = BASE_DIR / 'topic-cheat-sheet' / f'{cs_name}.md'
            if cs_file.exists():
                refs.append({"type": "cheatsheet", "path": f"../topic-cheat-sheet/{cs_name}.md", "label": f"速查卡: {cs_name}"})

    return refs[:8]  # 最多 8 个引用


def generate_cross_refs_for_fta(filepath: Path, file_index: dict) -> list:
    """为 topic-fta/list 文档生成 cross_refs"""
    filename = filepath.stem
    component = filename.replace('-fta', '')
    refs = []

    # 1. 对应 domain
    domain = COMPONENT_DOMAIN_MAP.get(component)
    if domain:
        domain_dir = BASE_DIR / domain
        if domain_dir.exists():
            # 尝试找到最匹配的文件
            for df in domain_dir.glob('*.md'):
                if component in df.stem.lower():
                    refs.append({"type": "domain", "path": f"../{domain}/{df.name}", "label": f"深度文档: {df.stem}"})
                    break
            if not refs:
                refs.append({"type": "domain", "path": f"../{domain}/", "label": f"知识域: {domain}"})

    # 2. 对应 skills
    skills_dir = BASE_DIR / 'topic-skills'
    if skills_dir.exists():
        for skill_file in skills_dir.glob('*.md'):
            if component in skill_file.stem:
                refs.append({"type": "skill", "path": f"../topic-skills/{skill_file.name}", "label": f"运维技能: {skill_file.stem}"})

    # 3. 结构化排障
    sts_dir = BASE_DIR / 'topic-structural-trouble-shooting'
    if sts_dir.exists():
        for sts_sub in sts_dir.iterdir():
            if sts_sub.is_dir():
                for sts_file in sts_sub.glob('*.md'):
                    if component in sts_file.stem.lower():
                        refs.append({"type": "structural", "path": f"../topic-structural-trouble-shooting/{sts_sub.name}/{sts_file.name}", "label": f"结构化排障: {sts_file.stem}"})

    return refs[:6]


def main():
    print("构建文件索引...")
    file_index = build_file_index()
    print(f"  索引完成: {sum(len(v) for v in file_index.values())} 文件\n")

    stats = {"updated": 0, "skipped": 0, "no_frontmatter": 0}

    exclude_dirs = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules', '.obsidian', '.zread', '.claude', '.codebuddy', '.comate', '.github'}

    for root, dirs, files in os.walk(BASE_DIR):
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in files:
            if not f.endswith('.md'):
                continue
            filepath = Path(root) / f
            rel = filepath.relative_to(BASE_DIR)

            try:
                content = filepath.read_text(encoding='utf-8')
            except Exception:
                continue

            fm, body = parse_frontmatter(content)
            if not fm:
                stats["no_frontmatter"] += 1
                continue

            # 已有 cross_refs, 跳过
            if fm.get('cross_refs'):
                stats["skipped"] += 1
                continue

            # 根据路径生成 cross_refs
            cross_refs = []
            if any(p.startswith('domain-') for p in rel.parts):
                cross_refs = generate_cross_refs_for_domain(filepath, file_index)
            elif 'topic-fta' in rel.parts and 'list' in rel.parts:
                cross_refs = generate_cross_refs_for_fta(filepath, file_index)

            if cross_refs:
                if update_frontmatter(filepath, cross_refs):
                    stats["updated"] += 1
                    print(f"  updated {rel} -> {len(cross_refs)} refs")
            else:
                stats["skipped"] += 1

    print(f"\n{'='*60}")
    print(f"cross_refs 修复统计:")
    print(f"  已更新:     {stats['updated']}")
    print(f"  跳过:       {stats['skipped']}")
    print(f"  无frontmatter: {stats['no_frontmatter']}")


if __name__ == '__main__':
    main()
