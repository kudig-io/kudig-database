#!/usr/bin/env python3
"""
kudig-database 批量质量修复脚本
功能:
  1. 为缺少 YAML front matter 的文档批量补充标准化元数据
  2. 为已有 front matter 但缺少关键字段的文档补充缺失字段
  3. 基于文件路径/内容自动推断: category, tags, difficulty, audience, reading_level
  4. 生成 intent_queries 和 trigger_keywords
  5. 生成 cross_refs 交叉引用
"""

import os
import re
import sys
import yaml
from pathlib import Path
from datetime import datetime

BASE_DIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")

# ============================================================
# 分类映射表
# ============================================================
DOMAIN_MAP = {
    "domain-1-architecture-fundamentals": {"category": "architecture-fundamentals", "tags": ["k8s", "architecture", "kubernetes"], "audience": ["架构师", "SRE", "平台工程师"], "reading_level": "advanced"},
    "domain-2-design-principles": {"category": "design-principles", "tags": ["k8s", "design", "principles"], "audience": ["架构师", "SRE"], "reading_level": "advanced"},
    "domain-3-control-plane": {"category": "control-plane", "tags": ["k8s", "control-plane", "etcd", "apiserver", "scheduler", "controller-manager"], "audience": ["SRE", "平台工程师", "运维工程师"], "reading_level": "advanced"},
    "domain-4-workloads": {"category": "workloads", "tags": ["k8s", "workload", "pod", "deployment", "statefulset"], "audience": ["SRE", "开发工程师", "运维工程师"], "reading_level": "intermediate"},
    "domain-5-networking": {"category": "networking", "tags": ["k8s", "networking", "service", "ingress", "cni"], "audience": ["SRE", "网络工程师", "运维工程师"], "reading_level": "advanced"},
    "domain-6-storage": {"category": "storage", "tags": ["k8s", "storage", "pv", "pvc", "storageclass"], "audience": ["SRE", "存储工程师", "运维工程师"], "reading_level": "advanced"},
    "domain-7-security": {"category": "security", "tags": ["k8s", "security", "rbac", "authentication", "authorization"], "audience": ["安全工程师", "SRE", "架构师"], "reading_level": "advanced"},
    "domain-8-observability": {"category": "observability", "tags": ["k8s", "observability", "monitoring", "logging", "tracing"], "audience": ["SRE", "运维工程师", "监控工程师"], "reading_level": "intermediate"},
    "domain-9-platform-ops": {"category": "platform-ops", "tags": ["k8s", "platform", "operations", "devops"], "audience": ["SRE", "平台工程师", "运维工程师"], "reading_level": "intermediate"},
    "domain-10-extensions": {"category": "extensions", "tags": ["k8s", "extensions", "crd", "operator", "webhook"], "audience": ["SRE", "开发工程师", "架构师"], "reading_level": "advanced"},
    "domain-11-ai-infra": {"category": "ai-infra", "tags": ["k8s", "ai", "gpu", "ml", "training", "inference"], "audience": ["AI 工程师", "MLOps 工程师", "SRE"], "reading_level": "advanced"},
    "domain-12-troubleshooting": {"category": "troubleshooting", "tags": ["k8s", "troubleshooting", "debugging", "fault-analysis"], "audience": ["SRE", "运维工程师", "技术支持"], "reading_level": "advanced"},
    "domain-13-docker": {"category": "docker", "tags": ["docker", "container", "image"], "audience": ["开发工程师", "运维工程师", "SRE"], "reading_level": "intermediate"},
    "domain-14-linux": {"category": "linux", "tags": ["linux", "system", "kernel"], "audience": ["运维工程师", "SRE", "系统管理员"], "reading_level": "intermediate"},
    "domain-15-network-fundamentals": {"category": "network-fundamentals", "tags": ["network", "tcp", "ip", "dns"], "audience": ["网络工程师", "SRE", "运维工程师"], "reading_level": "intermediate"},
    "domain-16-storage-fundamentals": {"category": "storage-fundamentals", "tags": ["storage", "filesystem", "block"], "audience": ["存储工程师", "SRE", "运维工程师"], "reading_level": "intermediate"},
    "domain-17-cloud-provider": {"category": "cloud-provider", "tags": ["k8s", "cloud", "eks", "gke", "aks", "ack"], "audience": ["SRE", "云架构师", "运维工程师"], "reading_level": "advanced"},
    "domain-18-production-operations": {"category": "production-operations", "tags": ["k8s", "production", "operations", "best-practices"], "audience": ["SRE", "运维工程师", "平台工程师"], "reading_level": "advanced"},
    "domain-19-papers": {"category": "papers", "tags": ["k8s", "papers", "research"], "audience": ["架构师", "技术决策者", "研究员"], "reading_level": "expert"},
    "domain-20-enterprise-monitoring-alerting": {"category": "enterprise-monitoring-alerting", "tags": ["k8s", "monitoring", "alerting", "prometheus"], "audience": ["SRE", "监控工程师", "运维工程师"], "reading_level": "intermediate"},
    "domain-21-logging-management-analytics": {"category": "logging-management-analytics", "tags": ["k8s", "logging", "efk", "loki"], "audience": ["SRE", "运维工程师", "数据工程师"], "reading_level": "intermediate"},
    "domain-22-container-image-management": {"category": "container-image-management", "tags": ["k8s", "container", "image", "registry", "harbor"], "audience": ["SRE", "运维工程师", "开发工程师"], "reading_level": "intermediate"},
    "domain-23-gitops-ci-cd": {"category": "gitops-ci-cd", "tags": ["k8s", "gitops", "ci-cd", "argocd", "flux"], "audience": ["DevOps 工程师", "SRE", "开发工程师"], "reading_level": "advanced"},
    "domain-24-infrastructure-as-code": {"category": "infrastructure-as-code", "tags": ["k8s", "iac", "terraform", "pulumi"], "audience": ["平台工程师", "SRE", "DevOps 工程师"], "reading_level": "advanced"},
    "domain-25-cloud-native-security": {"category": "cloud-native-security", "tags": ["k8s", "security", "cloud-native", "falco", "opa"], "audience": ["安全工程师", "SRE", "架构师"], "reading_level": "advanced"},
    "domain-26-service-mesh-microservices": {"category": "service-mesh-microservices", "tags": ["k8s", "service-mesh", "istio", "envoy", "microservices"], "audience": ["架构师", "SRE", "开发工程师"], "reading_level": "advanced"},
    "domain-27-multi-cloud-hybrid": {"category": "multi-cloud-hybrid", "tags": ["k8s", "multi-cloud", "hybrid-cloud"], "audience": ["云架构师", "SRE", "平台工程师"], "reading_level": "advanced"},
    "domain-28-enterprise-database-middleware": {"category": "enterprise-database-middleware", "tags": ["k8s", "database", "middleware", "mysql", "redis"], "audience": ["DBA", "SRE", "后端开发"], "reading_level": "advanced"},
    "domain-29-automated-testing-quality": {"category": "automated-testing-quality", "tags": ["k8s", "testing", "quality", "automation"], "audience": ["QA 工程师", "SRE", "开发工程师"], "reading_level": "intermediate"},
    "domain-30-disaster-recovery-business-continuity": {"category": "disaster-recovery", "tags": ["k8s", "disaster-recovery", "backup", "ha"], "audience": ["SRE", "运维工程师", "架构师"], "reading_level": "advanced"},
    "domain-31-hardware": {"category": "hardware", "tags": ["k8s", "hardware", "server", "gpu", "network"], "audience": ["基础设施工程师", "SRE", "运维工程师"], "reading_level": "intermediate"},
    "domain-32-yaml-manifests": {"category": "yaml-manifests", "tags": ["k8s", "yaml", "manifest", "template"], "audience": ["SRE", "开发工程师", "运维工程师"], "reading_level": "intermediate"},
    "domain-33-kubernetes-events": {"category": "kubernetes-events", "tags": ["k8s", "events", "troubleshooting"], "audience": ["SRE", "运维工程师", "技术支持"], "reading_level": "advanced"},
    "domain-34-cncf-landscape": {"category": "cncf-landscape", "tags": ["k8s", "cncf", "cloud-native", "ecosystem"], "audience": ["架构师", "技术决策者", "SRE"], "reading_level": "intermediate"},
    "domain-35-ebpf-technology": {"category": "ebpf-technology", "tags": ["k8s", "ebpf", "cilium", "networking", "observability"], "audience": ["SRE", "网络工程师", "内核工程师"], "reading_level": "expert"},
    "domain-36-platform-engineering": {"category": "platform-engineering", "tags": ["k8s", "platform-engineering", "developer-experience", "idp"], "audience": ["平台工程师", "SRE", "架构师"], "reading_level": "advanced"},
    "domain-37-edge-computing": {"category": "edge-computing", "tags": ["k8s", "edge", "iot", "kubeedge"], "audience": ["边缘计算工程师", "SRE", "IoT 工程师"], "reading_level": "advanced"},
    "domain-38-webassembly-cloud-native": {"category": "webassembly-cloud-native", "tags": ["k8s", "wasm", "webassembly", "cloud-native"], "audience": ["架构师", "开发工程师", "SRE"], "reading_level": "advanced"},
    "domain-39-supply-chain-security": {"category": "supply-chain-security", "tags": ["k8s", "supply-chain", "security", "sbom", "slsa"], "audience": ["安全工程师", "SRE", "架构师"], "reading_level": "advanced"},
    "domain-40-cloud-native-api-gateway": {"category": "cloud-native-api-gateway", "tags": ["k8s", "api-gateway", "envoy", "apisix", "higress"], "audience": ["SRE", "架构师", "运维工程师"], "reading_level": "advanced"},
}

TOPIC_MAP = {
    "topic-application-architecture": {"category": "application-architecture", "tags": ["k8s", "architecture", "industry"], "audience": ["架构师", "SRE", "技术决策者"], "reading_level": "advanced"},
    "topic-ai-agent": {"category": "ai-agent", "tags": ["ai", "agent", "llm", "rag", "multi-agent"], "audience": ["AI 工程师", "架构师", "SRE"], "reading_level": "advanced"},
    "topic-ai-coding": {"category": "ai-coding", "tags": ["ai", "coding", "copilot", "code-generation"], "audience": ["开发工程师", "AI 工程师"], "reading_level": "intermediate"},
    "topic-cheat-sheet": {"category": "cheatsheet", "tags": ["cheatsheet", "quick-reference"], "audience": ["所有工程师"], "reading_level": "intermediate"},
    "topic-deployment": {"category": "deployment", "tags": ["k8s", "deployment", "rolling-update"], "audience": ["SRE", "运维工程师"], "reading_level": "intermediate"},
    "topic-dictionary": {"category": "dictionary", "tags": ["k8s", "glossary", "terminology"], "audience": ["所有工程师"], "reading_level": "beginner"},
    "topic-febm": {"category": "febm", "tags": ["k8s", "forensics", "evidence-based", "methodology"], "audience": ["SRE", "运维专家", "技术支持"], "reading_level": "expert"},
    "topic-fta": {"category": "fta", "tags": ["k8s", "fault-tree", "root-cause", "troubleshooting"], "audience": ["SRE", "运维工程师", "技术支持"], "reading_level": "advanced"},
    "topic-functions": {"category": "functions", "tags": ["k8s", "operations", "cluster-management"], "audience": ["SRE", "运维工程师", "平台工程师"], "reading_level": "advanced"},
    "topic-index": {"category": "index", "tags": ["k8s", "index", "catalog"], "audience": ["所有工程师"], "reading_level": "beginner"},
    "topic-java-kubernetes": {"category": "java-kubernetes", "tags": ["java", "k8s", "spring", "jvm"], "audience": ["Java 开发工程师", "SRE"], "reading_level": "advanced"},
    "topic-k8s-lecturer": {"category": "k8s-lecturer", "tags": ["k8s", "training", "lecturer"], "audience": ["培训师", "技术经理"], "reading_level": "advanced"},
    "topic-learn": {"category": "learning", "tags": ["k8s", "training", "hands-on"], "audience": ["所有工程师"], "reading_level": "beginner"},
    "topic-migration": {"category": "migration", "tags": ["k8s", "migration", "modernization"], "audience": ["架构师", "SRE", "运维工程师"], "reading_level": "advanced"},
    "topic-presentations": {"category": "presentations", "tags": ["k8s", "presentation", "slides"], "audience": ["技术经理", "培训师"], "reading_level": "intermediate"},
    "topic-publish": {"category": "publish", "tags": ["k8s", "publish", "release"], "audience": ["SRE", "运维工程师"], "reading_level": "intermediate"},
    "topic-release-notes": {"category": "release-notes", "tags": ["k8s", "release-notes", "changelog"], "audience": ["所有工程师"], "reading_level": "intermediate"},
    "topic-skills": {"category": "skills", "tags": ["k8s", "skills", "sop", "runbook"], "audience": ["SRE", "运维工程师", "技术支持"], "reading_level": "advanced"},
    "topic-structural-trouble-shooting": {"category": "structural-troubleshooting", "tags": ["k8s", "troubleshooting", "decision-tree"], "audience": ["SRE", "运维工程师", "技术支持"], "reading_level": "advanced"},
    "topic-terway": {"category": "terway", "tags": ["k8s", "terway", "networking", "alicloud"], "audience": ["SRE", "网络工程师"], "reading_level": "advanced"},
}


def estimate_read_time(content: str) -> str:
    """根据内容长度估算阅读时间 (中文约 400 字/分钟)"""
    char_count = len(content)
    minutes = max(1, char_count // 1200)  # 中英文混合, 约 1200 字符/分钟
    if minutes <= 5:
        return "5min"
    elif minutes <= 15:
        return "15min"
    elif minutes <= 25:
        return "25min"
    elif minutes <= 35:
        return "35min"
    elif minutes <= 45:
        return "45min"
    elif minutes <= 60:
        return "1h"
    else:
        return f"{(minutes // 30 + 1) * 30}min"


def extract_title(content: str, filepath: Path) -> str:
    """从内容中提取标题"""
    # 从第一个 # 标题提取
    match = re.search(r'^#\s+(.+?)(?:\s*\{.*\})?$', content, re.MULTILINE)
    if match:
        title = match.group(1).strip()
        # 去掉 emoji
        title = re.sub(r'[\U0001f300-\U0001f9ff]', '', title).strip()
        return title
    # 从文件名推断
    name = filepath.stem
    # 去掉数字前缀
    name = re.sub(r'^\d+-', '', name)
    return name.replace('-', ' ').title()


def extract_tags_from_content(content: str, base_tags: list) -> list:
    """从内容中提取额外标签"""
    tags = list(base_tags)
    # 常见技术关键词提取
    keyword_patterns = {
        'etcd': 'etcd', 'apiserver': 'apiserver', 'kubelet': 'kubelet',
        'scheduler': 'scheduler', 'controller-manager': 'controller-manager',
        'prometheus': 'prometheus', 'grafana': 'grafana', 'jaeger': 'jaeger',
        'istio': 'istio', 'envoy': 'envoy', 'cilium': 'cilium',
        'flannel': 'flannel', 'calico': 'calico', 'coredns': 'coredns',
        'helm': 'helm', 'argocd': 'argocd', 'flux': 'flux',
        'containerd': 'containerd', 'cri-o': 'cri-o', 'docker': 'docker',
        'harbor': 'harbor', 'opa': 'opa', 'falco': 'falco',
        'rook': 'rook', 'ceph': 'ceph', 'minio': 'minio',
        'redis': 'redis', 'mysql': 'mysql', 'postgresql': 'postgresql',
        'kafka': 'kafka', 'elasticsearch': 'elasticsearch',
        'hpa': 'hpa', 'vpa': 'vpa', 'pdb': 'pdb',
        'statefulset': 'statefulset', 'daemonset': 'daemonset',
        'job': 'job', 'cronjob': 'cronjob',
        'ingress': 'ingress', 'gateway': 'gateway',
        'rbac': 'rbac', 'networkpolicy': 'networkpolicy',
        'crd': 'crd', 'operator': 'operator', 'webhook': 'webhook',
        'gpu': 'gpu', 'cuda': 'cuda', 'nvidia': 'nvidia',
        'ebpf': 'ebpf', 'wasm': 'wasm', 'serverless': 'serverless',
        'kubeflow': 'kubeflow', 'kserve': 'kserve', 'vllm': 'vllm',
        'llm': 'llm', 'rag': 'rag', 'agent': 'agent',
    }
    content_lower = content.lower()
    for pattern, tag in keyword_patterns.items():
        if pattern in content_lower and tag not in tags:
            tags.append(tag)
    return tags[:10]  # 最多 10 个标签


def generate_intent_queries(title: str, filepath: Path, content: str) -> list:
    """生成 intent_queries"""
    queries = []
    # 从标题生成
    clean_title = re.sub(r'^\d+\s*[-–]\s*', '', title)
    queries.append(f"{clean_title} 是什么")
    queries.append(f"如何 {clean_title}")

    # 从目录结构推断
    parts = filepath.parts
    if 'domain-' in str(filepath):
        domain = [p for p in parts if p.startswith('domain-')]
        if domain:
            domain_name = domain[0].replace('domain-', '').replace('-', ' ')
            queries.append(f"Kubernetes {domain_name} 最佳实践")
    if 'troubleshooting' in str(filepath) or 'trouble' in str(filepath):
        queries.append(f"{clean_title} 故障排查")
        queries.append(f"{clean_title} 排障步骤")
    if 'topic-fta' in str(filepath):
        queries.append(f"{clean_title} 根因分析")
        queries.append(f"{clean_title} 故障树")
    return queries[:6]


def generate_trigger_keywords(title: str, filepath: Path, content: str) -> list:
    """生成 trigger_keywords"""
    keywords = []
    # 从标题提取关键词
    clean_title = re.sub(r'^\d+\s*[-–]\s*', '', title)
    # 中文分词 (简单按空格和标点分割)
    words = re.split(r'[\s,，、/()（）]+', clean_title)
    keywords.extend([w for w in words if len(w) >= 2])
    # 从路径提取
    parts = filepath.parts
    for p in parts:
        if p.startswith('domain-') or p.startswith('topic-'):
            name = re.sub(r'^(domain|topic)-\d*-?', '', p)
            keywords.extend(name.split('-'))
    return [k for k in keywords if len(k) >= 2][:8]


def generate_prerequisites(filepath: Path, content: str) -> list:
    """生成 prerequisites 前置知识列表"""
    prereqs = []
    path_str = str(filepath).lower()
    content_lower = content.lower()

    # 通用基础
    prereqs.append("kubectl-basics")

    # 根据 domain 路径推断
    domain_mappings = [
        ("domain-01", "kubernetes-concepts"),
        ("cluster-fundamentals", "kubernetes-concepts"),
        ("domain-02", "pod-lifecycle"),
        ("workloads", "pod-lifecycle"),
        ("domain-03", "networking-basics"),
        ("domain-04", "storage-basics"),
        ("domain-05", "rbac-basics"),
        ("domain-06", "observability-basics"),
        ("domain-07", "platform-engineering-basics"),
        ("domain-08", "gitops-basics"),
        ("domain-09", "sre-practices"),
        ("domain-10", "troubleshooting-methodology"),
        ("domain-11", "gpu-ml-basics"),
        ("domain-12", "troubleshooting-methodology"),
        ("domain-17", "cloud-provider-basics"),
        ("domain-19", "cncf-ecosystem"),
        ("domain-20", "prometheus-basics"),
        ("domain-23", "gitops-basics"),
        ("domain-25", "security-fundamentals"),
        ("domain-26", "service-mesh-basics"),
        ("domain-35", "ebpf-basics"),
        ("domain-36", "platform-engineering-basics"),
    ]
    for pattern, prereq in domain_mappings:
        if pattern in path_str:
            prereqs.append(prereq)

    # 根据内容关键词推断
    content_mappings = [
        ("helm", "helm-basics"),
        ("istio", "service-mesh-basics"),
        ("prometheus", "prometheus-basics"),
        ("grafana", "monitoring-basics"),
        ("argocd", "gitops-basics"),
        ("terraform", "iac-basics"),
        ("ebpf", "ebpf-basics"),
        ("cilium", "cilium-basics"),
        ("calico", "cni-basics"),
        ("etcd", "etcd-basics"),
        ("kafka", "kafka-basics"),
        ("redis", "redis-basics"),
        ("mysql", "mysql-basics"),
        ("gpu", "gpu-scheduling-basics"),
        ("cert-manager", "tls-basics"),
        ("opa", "policy-basics"),
        ("kyverno", "policy-basics"),
        ("velero", "backup-basics"),
        ("fluentd", "logging-basics"),
        ("loki", "logging-basics"),
        ("jaeger", "tracing-basics"),
        ("opentelemetry", "observability-basics"),
    ]
    for keyword, prereq in content_mappings:
        if keyword in content_lower:
            prereqs.append(prereq)

    # 去重并保持顺序
    return list(dict.fromkeys(prereqs))


def has_yaml_frontmatter(content: str) -> bool:
    """检查是否已有 YAML front matter"""
    return content.lstrip().startswith('---\n') or content.lstrip().startswith('---\r\n')


def parse_existing_frontmatter(content: str) -> tuple:
    """解析已有的 YAML front matter, 返回 (dict, body)"""
    stripped = content.lstrip()
    if not stripped.startswith('---'):
        return {}, content
    # 找到第二个 ---
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


def build_frontmatter(filepath: Path, content: str, existing_fm: dict) -> str:
    """构建完整的 YAML front matter"""
    # 确定分类
    parts = filepath.parts
    category_info = None

    for part in parts:
        if part in DOMAIN_MAP:
            category_info = DOMAIN_MAP[part]
            break
        if part in TOPIC_MAP:
            category_info = TOPIC_MAP[part]
            break

    if not category_info:
        category_info = {"category": "general", "tags": ["k8s"], "audience": ["所有工程师"], "reading_level": "intermediate"}

    # 提取标题
    title = existing_fm.get('title') or extract_title(content, filepath)

    # 合并标签
    base_tags = existing_fm.get('tags', []) or category_info.get('tags', [])
    if isinstance(base_tags, str):
        base_tags = [base_tags]
    tags = extract_tags_from_content(content, base_tags)

    # 难度推断
    difficulty = existing_fm.get('difficulty') or existing_fm.get('reading_level') or category_info.get('reading_level', 'intermediate')

    # 构建 front matter
    fm = {}
    fm['title'] = title
    if not existing_fm.get('description'):
        # 从第一个段落提取描述
        desc_match = re.search(r'(?:^>\s*|^)(.*?(?:介绍|解析|指南|详解|深度|概述|专题|方案|实践|架构|设计|管理|部署|配置|运维|排查|优化|安全|监控|日志|网络|存储|调度|认证|授权).*?)(?:\n|$)', content, re.MULTILINE)
        if desc_match:
            fm['description'] = desc_match.group(1).strip()[:200]
        else:
            fm['description'] = f"{title} — Kubernetes 生产运维知识库"
    else:
        fm['description'] = existing_fm['description']

    fm['category'] = existing_fm.get('category') or category_info['category']
    fm['tags'] = tags
    fm['last_updated'] = existing_fm.get('last_updated') or datetime.now().strftime('%Y-%m')
    fm['difficulty'] = difficulty
    fm['reading_level'] = difficulty
    fm['audience'] = existing_fm.get('audience') or category_info.get('audience', ['所有工程师'])
    fm['estimated_read_time'] = existing_fm.get('estimated_read_time') or estimate_read_time(content)

    # intent_queries
    if not existing_fm.get('intent_queries'):
        fm['intent_queries'] = generate_intent_queries(title, filepath, content)
    else:
        fm['intent_queries'] = existing_fm['intent_queries']

    # trigger_keywords
    if not existing_fm.get('trigger_keywords'):
        fm['trigger_keywords'] = generate_trigger_keywords(title, filepath, content)
    else:
        fm['trigger_keywords'] = existing_fm['trigger_keywords']

    # prerequisites
    if not existing_fm.get('prerequisites'):
        fm['prerequisites'] = generate_prerequisites(filepath, content)
    else:
        fm['prerequisites'] = existing_fm['prerequisites']

    # FTA 专用字段
    relpath = str(filepath)
    is_fta = 'topic-fta/' in relpath or relpath.endswith('-fta.md')
    is_skill = 'topic-skills/' in relpath or relpath.endswith('-skill.md')

    if is_fta:
        if not existing_fm.get('fta_id'):
            # 从文件名生成 FTA ID, 如 kubeadm-fta.md -> FTA-KUBEADM-001
            stem = filepath.stem.replace('-fta', '').replace('-', '_').upper()
            fm['fta_id'] = f"FTA-{stem}-001"
        if not existing_fm.get('component'):
            fm['component'] = filepath.stem.replace('-fta', '').replace('-', ' ').title()
        if not existing_fm.get('severity'):
            # 基于内容推断严重程度
            content_lower = content.lower()
            if any(k in content_lower for k in ['critical', 'p0', '生产事故', '集群不可用', '数据丢失', 'crashloopbackoff']):
                fm['severity'] = 'critical'
            elif any(k in content_lower for k in ['high', 'p1', '服务降级', '性能问题', '内存泄漏']):
                fm['severity'] = 'high'
            elif any(k in content_lower for k in ['medium', 'p2', '配置错误', '连接超时']):
                fm['severity'] = 'medium'
            else:
                fm['severity'] = 'high'  # 默认 high

    # SKILL 专用字段
    if is_skill:
        if not existing_fm.get('skill_id'):
            # 从文件名生成 SKILL ID
            stem = filepath.stem.replace('-skill', '').replace('-', '_').upper()
            fm['skill_id'] = f"SKILL-{stem}-001"
        if not existing_fm.get('skill_name'):
            fm['skill_name'] = title
        if not existing_fm.get('version'):
            fm['version'] = '1.0.0'

    # 保留已有的其他字段
    for key in ['k8s_versions', 'authors', 'cross_refs', 'related_docs', 'prerequisites', 'related_domains', 'related_topics']:
        if key in existing_fm and existing_fm[key]:
            fm[key] = existing_fm[key]

    # 清理 None 值
    fm = {k: v for k, v in fm.items() if v is not None}

    return yaml.dump(fm, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)


def process_file(filepath: Path, dry_run: bool = False) -> dict:
    """处理单个文件"""
    try:
        content = filepath.read_text(encoding='utf-8')
    except Exception as e:
        return {"file": str(filepath), "status": "error", "error": str(e)}

    if not content.strip():
        return {"file": str(filepath), "status": "skipped", "reason": "empty"}

    # 跳过模板文件
    if '/templates/' in str(filepath):
        return {"file": str(filepath), "status": "skipped", "reason": "template"}

    has_fm = has_yaml_frontmatter(content)

    if has_fm:
        existing_fm, body = parse_existing_frontmatter(content)
        # 检查是否有缺失字段
        missing = []
        base_fields = ['title', 'description', 'category', 'tags', 'last_updated', 'difficulty', 'reading_level', 'audience', 'estimated_read_time', 'intent_queries', 'trigger_keywords', 'prerequisites']
        for field in base_fields:
            if field not in existing_fm or not existing_fm[field]:
                missing.append(field)

        # FTA 文件额外字段
        relpath = str(filepath)
        is_fta = 'topic-fta/' in relpath or relpath.endswith('-fta.md')
        is_skill = 'topic-skills/' in relpath or relpath.endswith('-skill.md')

        if is_fta:
            for field in ['fta_id', 'component', 'severity']:
                if field not in existing_fm or not existing_fm[field]:
                    missing.append(field)
        if is_skill:
            for field in ['skill_id', 'skill_name', 'version']:
                if field not in existing_fm or not existing_fm[field]:
                    missing.append(field)

        if not missing:
            return {"file": str(filepath), "status": "skipped", "reason": "complete"}

        # 补充缺失字段
        fm_yaml = build_frontmatter(filepath, body, existing_fm)
        new_content = f"---\n{fm_yaml}---\n\n{body.lstrip()}"

        action = "enriched"
    else:
        # 没有 front matter, 添加全新的
        fm_yaml = build_frontmatter(filepath, content, {})
        new_content = f"---\n{fm_yaml}---\n\n{content}"
        action = "added"

    if not dry_run:
        filepath.write_text(new_content, encoding='utf-8')

    return {"file": str(filepath), "status": action}


def main():
    dry_run = '--dry-run' in sys.argv
    if dry_run:
        print("[DRY RUN] 不会实际修改文件\n")

    stats = {"added": 0, "enriched": 0, "skipped": 0, "error": 0}

    # 收集所有 md 文件 (排除特殊目录)
    exclude_dirs = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules', '.obsidian', '.zread', '.claude', '.codebuddy', '.comate', '.github'}

    md_files = []
    for root, dirs, files in os.walk(BASE_DIR):
        # 过滤排除目录
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in files:
            if f.endswith('.md'):
                md_files.append(Path(root) / f)

    total = len(md_files)
    print(f"扫描到 {total} 个 Markdown 文件\n")

    for i, filepath in enumerate(md_files):
        result = process_file(filepath, dry_run)
        status = result["status"]
        stats[status] = stats.get(status, 0) + 1

        if status in ("added", "enriched"):
            rel = filepath.relative_to(BASE_DIR)
            print(f"  [{i+1}/{total}] {status:8s} {rel}")

    print(f"\n{'='*60}")
    print(f"修复统计:")
    print(f"  新增 front matter:  {stats['added']}")
    print(f"  补充缺失字段:      {stats['enriched']}")
    print(f"  跳过 (已完整):     {stats['skipped']}")
    print(f"  错误:              {stats['error']}")
    print(f"  总计处理:          {total}")


if __name__ == '__main__':
    main()
