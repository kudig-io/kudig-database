#!/usr/bin/env python3
"""
KUDIG-DATABASE Intent-Action Pairs 生成脚本
为 domain-1~12 核心文档生成 intent_queries 字段。

基于文件标题和内容自动生成自然语言查询。
"""

import re
import yaml
from pathlib import Path

# 知识库 domain 目录（已从 domain-NN-slug 改为中文命名）
DOMAINS = {'集群基础','工作负载','网络','存储','安全','可观测性','平台工程','发布变更','可靠性','故障诊断','生产运维','云厂商','容器运行时','AI基础设施','专项技术','数据库中间件','系统基础','清单模式','生态参考','应用模式'}

BASE_DIR = Path(__file__).parent.parent

# 基于文件名的意图模板
INTENT_TEMPLATES = {
    "overview": [
        "{title}是什么？",
        "{title}的核心概念有哪些？",
        "如何理解{title}？",
    ],
    "deep-dive": [
        "{title}的工作原理是什么？",
        "{title}的内部机制详解",
        "{title}的技术深度分析",
    ],
    "complete-guide": [
        "{title}的完整指南",
        "如何全面掌握{title}？",
        "{title}的从入门到精通",
    ],
    "best-practices": [
        "{title}的最佳实践是什么？",
        "生产环境中{title}的注意事项",
        "{title}的推荐配置",
    ],
    "troubleshooting": [
        "{title}常见故障有哪些？",
        "如何排查{title}相关问题？",
        "{title}的故障处理方法",
    ],
    "performance": [
        "如何优化{title}的性能？",
        "{title}的性能调优指南",
        "{title}的瓶颈在哪里？",
    ],
    "security": [
        "{title}的安全加固怎么做？",
        "{title}的安全最佳实践",
        "{title}有哪些安全风险？",
    ],
    "configuration": [
        "{title}的配置参数有哪些？",
        "如何配置{title}？",
        "{title}的完整配置参考",
    ],
    "reference": [
        "{title}的完整参考",
        "{title}的所有命令/字段",
        "{title}速查手册",
    ],
    "upgrade": [
        "如何升级{title}？",
        "{title}的升级路径和策略",
        "{title}升级注意事项",
    ],
    "deployment": [
        "如何部署{title}？",
        "{title}的部署模式和最佳实践",
        "{title}生产环境部署",
    ],
    "monitoring": [
        "如何监控{title}？",
        "{title}的关键指标有哪些？",
        "{title}的告警规则",
    ],
    "architecture": [
        "{title}的架构设计",
        "{title}的组件和交互",
        "{title}的系统设计",
    ],
    "production": [
        "{title}生产环境怎么配置？",
        "{title}的生产级实践",
        "生产环境中{title}的注意事项",
    ],
    "quick-start": [
        "如何快速上手{title}？",
        "{title}的快速入门指南",
        "{title}的5分钟教程",
    ],
}

# 组件名关键词映射
COMPONENT_KEYWORDS = {
    "pod": "Pod",
    "deployment": "Deployment",
    "statefulset": "StatefulSet",
    "daemonset": "DaemonSet",
    "service": "Service",
    "ingress": "Ingress",
    "etcd": "etcd",
    "apiserver": "API Server",
    "scheduler": "调度器",
    "controller-manager": "Controller Manager",
    "kubelet": "Kubelet",
    "kube-proxy": "Kube-Proxy",
    "rbac": "RBAC",
    "network-policy": "Network Policy",
    "persistentvolume": "PV",
    "pvc": "PVC",
    "storageclass": "StorageClass",
    "csi": "CSI",
    "hpa": "HPA",
    "vpa": "VPA",
    "prometheus": "Prometheus",
    "grafana": "Grafana",
    "helm": "Helm",
    "crd": "CRD",
    "operator": "Operator",
    "webhook": "Webhook",
    "gpu": "GPU",
    "istio": "Istio",
    "envoy": "Envoy",
    "cilium": "Cilium",
    "coredns": "CoreDNS",
    "logging": "日志",
    "tracing": "追踪",
    "gitops": "GitOps",
    "argocd": "ArgoCD",
    "flux": "Flux",
    "terraform": "Terraform",
    "kubernetes": "Kubernetes",
    "k8s": "K8s",
    "docker": "Docker",
    "container": "容器",
    "cluster": "集群",
    "node": "节点",
    "namespace": "命名空间",
    "configmap": "ConfigMap",
    "secret": "Secret",
    "certificate": "证书",
    "backup": "备份",
    "restore": "恢复",
    "upgrade": "升级",
    "migration": "迁移",
    "security": "安全",
    "monitor": "监控",
    "network": "网络",
    "storage": "存储",
}


def parse_frontmatter(content):
    """Parse frontmatter."""
    stripped = content.lstrip()
    if not stripped.startswith("---"):
        return None
    end = stripped.find("---", 3)
    if end == -1:
        return None
    try:
        fm = yaml.safe_load(stripped[3:end].strip())
        return fm if fm else {}
    except Exception:
        return None


def generate_intent_queries(filepath: Path, fm: dict) -> list:
    """Generate intent queries based on filename and title."""
    stem = filepath.stem.lower()
    title = fm.get("title", filepath.stem)

    # Find matching template
    matched_template = None
    for key, queries in INTENT_TEMPLATES.items():
        if key in stem:
            matched_template = queries
            break

    if not matched_template:
        # Try component keywords
        for keyword, name in COMPONENT_KEYWORDS.items():
            if keyword in stem:
                matched_template = [
                    f"{name}是什么？",
                    f"如何使用{name}？",
                    f"{name}的最佳实践是什么？",
                ]
                break

    if not matched_template:
        # Use title directly
        clean_title = re.sub(r'^[\d一二三四五六七八九十]+[\s\-、.]', '', title).strip()
        if clean_title:
            matched_template = [
                f"{clean_title}是什么？",
                f"{clean_title}的使用方法",
                f"{clean_title}的最佳实践",
            ]
        else:
            return []

    # Generate queries
    queries = []
    for t in matched_template[:5]:
        query = t.replace("{title}", title[:50])
        queries.append(query)

    return queries


def fix_file(filepath: Path) -> bool:
    """Add intent_queries to a file's frontmatter."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return False

    fm = parse_frontmatter(content)
    if fm is None:
        return False

    # Skip if already has intent_queries
    if "intent_queries" in fm and fm["intent_queries"]:
        return False

    queries = generate_intent_queries(filepath, fm)
    if not queries:
        return False

    fm["intent_queries"] = queries

    # Find frontmatter boundaries
    stripped = content.lstrip()
    leading = len(content) - len(stripped)
    end = content.find("---", 3 + leading)
    if end == -1:
        return False

    # Rebuild
    new_fm_yaml = yaml.dump(fm, default_flow_style=False, allow_unicode=True, sort_keys=False)
    new_fm_block = "---\n" + new_fm_yaml + "---"
    new_content = content[:leading] + new_fm_block + content[end + 3:]
    filepath.write_text(new_content, encoding="utf-8")
    return True


def main():
    print("=" * 70)
    print("生成 Intent-Action Pairs...")
    print("=" * 70)

    md_files = []
    for d in sorted(BASE_DIR.iterdir()):
        if d.is_dir() and d.name in DOMAINS:
            for f in d.glob("*.md"):
                if f.name not in ("README.md", "MOC.md"):
                    md_files.append(f)

    print(f"扫描范围: {len(md_files)} 文件 (所有 domain)")

    fixed = 0
    skipped = 0
    for f in md_files:
        if fix_file(f):
            fixed += 1
        else:
            skipped += 1

    print(f"\n修复完成:")
    print(f"  修改: {fixed} 文件")
    print(f"  跳过: {skipped} 文件 (已有 intent 或无 frontmatter)")


if __name__ == "__main__":
    main()
