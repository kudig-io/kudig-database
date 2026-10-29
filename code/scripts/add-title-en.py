#!/usr/bin/env python3
"""为 topic-dictionary 批量补充 title_en 字段"""

import os
import re
import yaml
from pathlib import Path

BASE_DIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")

# 中文→英文术语映射表 (高频术语)
TITLE_EN_MAP = {
    "命名空间": "Namespaces",
    "Pod": "Pods",
    "容器": "Containers",
    "部署": "Deployments",
    "副本集": "ReplicaSets",
    "有状态副本集": "StatefulSets",
    "守护进程集": "DaemonSets",
    "任务": "Jobs",
    "定时任务": "CronJobs",
    "水平自动伸缩": "Horizontal Pod Autoscaler",
    "垂直自动伸缩": "Vertical Pod Autoscaler",
    "服务": "Services",
    "入口": "Ingress",
    "网络策略": "Network Policies",
    "存储卷": "Volumes",
    "持久卷": "Persistent Volumes",
    "持久卷声明": "Persistent Volume Claims",
    "存储类": "Storage Classes",
    "配置映射": "ConfigMaps",
    "密钥": "Secrets",
    "服务账户": "Service Accounts",
    "基于角色的访问控制": "Role-Based Access Control",
    "准入控制": "Admission Controllers",
    "自定义资源定义": "Custom Resource Definitions",
    "操作符": "Operators",
    "调度": "Scheduling",
    "污点与容忍": "Taints and Tolerations",
    "节点亲和性": "Node Affinity",
    "Pod 亲和性": "Pod Affinity",
    "拓扑分布约束": "Topology Spread Constraints",
    "资源配额": "Resource Quotas",
    "限制范围": "Limit Ranges",
    "服务质量": "Quality of Service",
    "就绪探针": "Readiness Probes",
    "存活探针": "Liveness Probes",
    "启动探针": "Startup Probes",
    "滚动更新": "Rolling Updates",
    "回滚": "Rollbacks",
    "金丝雀发布": "Canary Deployments",
    "蓝绿部署": "Blue-Green Deployments",
    "A/B 测试": "A/B Testing",
    "服务网格": "Service Mesh",
    "容器运行时": "Container Runtime",
    "容器网络接口": "Container Network Interface",
    "容器存储接口": "Container Storage Interface",
    "指标": "Metrics",
    "日志": "Logging",
    "追踪": "Tracing",
    "告警": "Alerting",
    "仪表盘": "Dashboards",
    "证书": "Certificates",
    "审计日志": "Audit Logs",
    "事件": "Events",
    "端点": "Endpoints",
    "租户": "Tenants",
    "集群": "Clusters",
    "节点": "Nodes",
    "控制平面": "Control Plane",
    "工作节点": "Worker Nodes",
    "API 服务器": "API Server",
    "控制器管理器": "Controller Manager",
    "调度器": "Scheduler",
    "etcd": "etcd",
    "Kubelet": "Kubelet",
    "Kube-proxy": "Kube-proxy",
    "CoreDNS": "CoreDNS",
    "容器组": "Pods",
    "无服务器": "Serverless",
    "边缘计算": "Edge Computing",
    "联邦": "Federation",
    "多集群": "Multi-Cluster",
    "灾备": "Disaster Recovery",
    "高可用": "High Availability",
    "负载均衡": "Load Balancing",
    "服务发现": "Service Discovery",
    "配置管理": "Configuration Management",
    "密钥管理": "Secret Management",
    "镜像管理": "Image Management",
    "镜像扫描": "Image Scanning",
    "供应链安全": "Supply Chain Security",
    "策略引擎": "Policy Engine",
    "合规": "Compliance",
    "可观测性": "Observability",
    "分布式追踪": "Distributed Tracing",
    "日志聚合": "Log Aggregation",
    "指标采集": "Metric Collection",
    "告警规则": "Alert Rules",
    "容量规划": "Capacity Planning",
    "成本优化": "Cost Optimization",
    "FinOps": "FinOps",
    "混沌工程": "Chaos Engineering",
    "故障注入": "Fault Injection",
    "性能测试": "Performance Testing",
    "压力测试": "Load Testing",
    "基准测试": "Benchmarking",
    "CI/CD": "CI/CD",
    "GitOps": "GitOps",
    "基础设施即代码": "Infrastructure as Code",
    "平台工程": "Platform Engineering",
    "开发者体验": "Developer Experience",
    "内部开发者平台": "Internal Developer Platform",
    "门户": "Portal",
    "自服务能力": "Self-Service",
    "标签": "Labels",
    "注解": "Annotations",
    "选择器": "Selectors",
    "终止宽限期": "Termination Grace Period",
    "优雅关闭": "Graceful Shutdown",
    "健康检查": "Health Checks",
    "就绪": "Readiness",
    "存活": "Liveness",
    "初始化容器": "Init Containers",
    "临时容器": "Ephemeral Containers",
    "边车容器": "Sidecar Containers",
    "多容器 Pod": "Multi-Container Pods",
    "静态 Pod": "Static Pods",
    "Pod 安全标准": "Pod Security Standards",
    "Pod 安全准入": "Pod Security Admission",
    "Pod 优先级": "Pod Priority",
    "抢占": "Preemption",
    "驱逐": "Eviction",
    "垃圾回收": "Garbage Collection",
    "最终一致性": "Eventual Consistency",
    "声明式 API": "Declarative API",
    "命令式 API": "Imperative API",
    "观察模式": "Watch Pattern",
    "控制器模式": "Controller Pattern",
    "水平伸缩": "Horizontal Scaling",
    "垂直伸缩": "Vertical Scaling",
    "自动伸缩": "Autoscaling",
    "集群自动伸缩": "Cluster Autoscaling",
    "Karpenter": "Karpenter",
    "Headless Service": "Headless Services",
    "ExternalName Service": "ExternalName Services",
    "NodePort Service": "NodePort Services",
    "ClusterIP Service": "ClusterIP Services",
    "LoadBalancer Service": "LoadBalancer Services",
    "网关 API": "Gateway API",
    "Envoy": "Envoy",
    "Istio": "Istio",
    "Cilium": "Cilium",
    "Calico": "Calico",
    "Flannel": "Flannel",
    "Helm": "Helm",
    "Kustomize": "Kustomize",
    "Argo CD": "Argo CD",
    "Flux": "Flux",
    "Prometheus": "Prometheus",
    "Grafana": "Grafana",
    "Jaeger": "Jaeger",
    "OpenTelemetry": "OpenTelemetry",
    "Falco": "Falco",
    "OPA": "Open Policy Agent",
    "Kyverno": "Kyverno",
    "Velero": "Velero",
    "Harbor": "Harbor",
    "Crossplane": "Crossplane",
    "Backstage": "Backstage",
    "Dapr": "Dapr",
    "Knative": "Knative",
    "KubeEdge": "KubeEdge",
    "Kubeflow": "Kubeflow",
    "Kueue": "Kueue",
    "Karpenter": "Karpenter",
    "Volcano": "Volcano",
    "Koordinator": "Koordinator",
    "GPU 调度": "GPU Scheduling",
    "GPU 共享": "GPU Sharing",
    "MIG": "Multi-Instance GPU",
    "vGPU": "Virtual GPU",
    "大语言模型": "Large Language Models",
    "推理优化": "Inference Optimization",
    "量化": "Quantization",
    "连续批处理": "Continuous Batching",
    "KV Cache": "KV Cache",
    "FlashAttention": "FlashAttention",
    "模型并行": "Model Parallelism",
    "数据并行": "Data Parallelism",
    "张量并行": "Tensor Parallelism",
    "流水线并行": "Pipeline Parallelism",
    "混合专家": "Mixture of Experts",
    "RAG": "Retrieval Augmented Generation",
    "向量数据库": "Vector Database",
    "嵌入": "Embeddings",
    "微调": "Fine-Tuning",
    "LoRA": "Low-Rank Adaptation",
    "RLHF": "Reinforcement Learning from Human Feedback",
    "DPO": "Direct Preference Optimization",
    "eBPF": "eBPF",
    "WebAssembly": "WebAssembly",
    "gRPC": "gRPC",
    "WebSocket": "WebSocket",
    "TCP/IP": "TCP/IP",
    "DNS": "DNS",
    "VXLAN": "VXLAN",
    "BGP": "BGP",
    "WireGuard": "WireGuard",
    "mTLS": "Mutual TLS",
    "零信任": "Zero Trust",
    "供应链攻击": "Supply Chain Attacks",
    "镜像签名": "Image Signing",
    "SBOM": "Software Bill of Materials",
    "SLSA": "Supply-chain Levels for Software Artifacts",
    "安全基线": "Security Baseline",
    "CIS Benchmark": "CIS Benchmarks",
    "NSA Hardening": "NSA Kubernetes Hardening",
}


def add_title_en(filepath: Path) -> bool:
    try:
        content = filepath.read_text(encoding='utf-8')
    except:
        return False

    if not content.lstrip().startswith('---'):
        return False

    end = content.index('---', 3)
    yaml_str = content[3:end]
    body = content[end+3:]

    try:
        fm = yaml.safe_load(yaml_str) or {}
    except:
        return False

    if 'title_en' in fm and fm['title_en']:
        return False

    title = fm.get('title', '')
    if not title:
        return False

    # 尝试从映射表查找
    title_en = None
    for cn, en in TITLE_EN_MAP.items():
        if cn in title:
            title_en = en
            break

    if not title_en:
        # 从文件名推断
        stem = filepath.stem
        # 去掉数字前缀
        name = re.sub(r'^\d+-', '', stem)
        title_en = name.replace('-', ' ').title()

    fm['title_en'] = title_en

    new_yaml = yaml.dump(fm, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
    new_content = f"---\n{new_yaml}---\n{body}"
    filepath.write_text(new_content, encoding='utf-8')
    return True


def main():
    dict_dir = BASE_DIR / 'topic-dictionary'
    if not dict_dir.exists():
        print("topic-dictionary 目录不存在")
        return

    updated = 0
    for md_file in sorted(dict_dir.rglob('*.md')):
        if md_file.name == 'README.md':
            continue
        if add_title_en(md_file):
            updated += 1

    print(f"title_en 补充完成: {updated} 个术语词典文件已更新")


if __name__ == '__main__':
    main()
