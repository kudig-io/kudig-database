#!/usr/bin/env python3
"""Round 3: 生成 k8s-glossary 缺失术语 + 高频 CN 生态引用术语"""

import os
from pathlib import Path

BASE = Path("系统基础/topic-dictionary")

def write_file(cat_dir, filename, title_zh, title_en, tags, overview, core, mechanism, use_cases, refs, related_links=""):
    fp = BASE / cat_dir / f"{filename}.md"
    if fp.exists():
        return False

    tks = list(dict.fromkeys([title_zh, title_en, "dictionary"]))
    tk_lines = "\n".join(f"- {kw}" for kw in tks)

    rel = related_links if related_links else f"- [[系统基础/topic-dictionary/k8s-glossary|K8s Glossary]]"

    content = f"""---
title: {title_zh}
description: '{overview[:80]}...'
category: dictionary
tags:
- k8s
- glossary
{chr(10).join(f"- {t}" for t in tags)}
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- {title_zh} 是什么
- {title_en} 详解
trigger_keywords:
{tk_lines}
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# {title_zh}

> **英文名**: {title_en}

## 概述

{overview}

## 核心概念/原理

{core}

## 关键机制或特性

{mechanism}

## 使用场景与最佳实践

{use_cases}

## 参考链接

{refs}

## Related

{rel}
"""
    fp.parent.mkdir(parents=True, exist_ok=True)
    fp.write_text(content)
    return True


# ── Part A: k8s-glossary 缺失的 9 个术语 ──────────────────────

K8S_MISSING = [
    # (cat_dir, filename, title_zh, title_en, tags, overview, core, mechanism, use_cases, refs, related)
    ("fundamentals", "control-plane-node", "控制平面节点", "Control Plane Node",
     ["control-plane", "node"],
     "Control Plane Node（控制平面节点）是运行 Kubernetes 控制平面组件（kube-apiserver、kube-scheduler、kube-controller-manager、etcd）的专用节点。在高可用集群中通常部署 3 个或 5 个控制平面节点。",
     "### 与 Master Node 的关系\n\nControl Plane Node 是 Master Node 的现代称谓，强调该节点运行控制平面组件而非「主从」关系。\n\n### 控制平面组件\n\n| 组件 | 职责 |\n|------|------|\n| kube-apiserver | API 入口，REST 请求处理 |\n| etcd | 集群状态存储 |\n| kube-scheduler | Pod 调度决策 |\n| kube-controller-manager | 控制器循环运行 |\n| cloud-controller-manager | 云厂商 API 交互 |",
     "- 控制平面节点通常标记 `node-role.kubernetes.io/control-plane` 污点，默认不接受用户 Pod。\n- 高可用部署使用 kubeadm 的 `--control-plane-endpoint` 配置负载均衡。\n- etcd 可以堆叠（stacked）在控制平面节点上，也可以外部独立部署。",
     "- 生产集群至少部署 3 个控制平面节点实现高可用。\n- 控制平面节点应有独立的计算资源，不与工作负载混用。\n- 使用 `kubeadm init --upload-certs` 加入额外控制平面节点。\n- 定期检查 etcd 集群健康状态和证书过期时间。",
     "- [Control Plane Node - Kubernetes Docs](https://kubernetes.io/docs/concepts/architecture/nodes/#control-plane-node)",
     "- [[系统基础/topic-dictionary/fundamentals/master-node|Master Node]]\n- [[系统基础/topic-dictionary/fundamentals/control-plane|Control Plane]]\n- [[系统基础/topic-dictionary/fundamentals/worker-node|Worker Node]]\n- [[系统基础/topic-dictionary/fundamentals/node|Node]]\n- [[系统基础/topic-dictionary/fundamentals/etcd|Etcd]]"),

    ("fundamentals", "controller-manager", "控制器管理器", "kube-controller-manager",
     ["controller-manager", "control-plane"],
     "kube-controller-manager 是 Kubernetes 控制平面组件，负责运行各种控制器（Controller）的循环逻辑。每个控制器是独立的 goroutine，通过 apiserver 监听资源变化并执行调谐（reconcile）操作。",
     "### 内置控制器\n\n| 控制器 | 职责 |\n|--------|------|\n| Node Controller | 监控节点健康状态 |\n| Deployment Controller | 管理 Deployment 和 ReplicaSet |\n| ReplicaSet Controller | 维持 Pod 副本数 |\n| EndpointSlice Controller | 维护 Service Endpoints |\n| ServiceAccount Controller | 为新命名空间创建默认 SA |\n| Job Controller | 管理 Job 生命周期 |\n| Namespace Controller | 处理命名空间删除 |\n| PV Controller | 管理 PersistentVolume 绑定 |",
     "- 所有控制器共享一个进程，通过 `--controllers` 标志控制启用/禁用。\n- 控制器采用 watch + reconcile 模式，持续将系统状态推向期望状态。\n- leader election 确保多副本部署时只有一个活跃实例。",
     "- 通过 `--concurrent-*` 参数调优控制器的并发处理能力。\n- 监控 `workqueue_depth` 和 `workqueue_latency` 指标检测控制器性能。\n- 自定义控制器推荐使用 controller-runtime 或 Kubebuilder 框架。",
     "- [kube-controller-manager - Kubernetes Docs](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-controller-manager/)",
     "- [[系统基础/topic-dictionary/fundamentals/kube-apiserver|Kube-apiserver]]\n- [[系统基础/topic-dictionary/fundamentals/kube-scheduler|Kube-scheduler]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/workloads/replicaset|ReplicaSet]]\n- [[系统基础/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]"),

    ("networking", "endpoint", "端点", "Endpoints",
     ["endpoint", "service"],
     "Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定 selector 时，需要手动创建 Endpoints 资源。EndpointSlice 是 Endpoints 的现代替代方案，适用于大规模集群。",
     "### Endpoints vs EndpointSlice\n\n| 特性 | Endpoints | EndpointSlice |\n|------|-----------|---------------|\n| API | v1 | discovery.k8s.io/v1 |\n| 扩展性 | 单个对象包含所有后端 | 分片存储，每片 100 个 |\n| 适用场景 | 小规模集群 | 大规模集群（推荐） |\n\n### 工作原理\n\n当 Service 定义了 selector，kube-controller-manager 自动创建对应的 Endpoints/EndpointSlice 对象。",
     "- 每个 Endpoint 包含 IP、端口和就绪状态。\n- EndpointSlice 按拓扑分区，支持 `topology.kubernetes.io/zone` 标签。\n- 外部服务可通过手动 Endpoints + ExternalName Service 接入。",
     "- 大规模集群优先使用 EndpointSlice API。\n- 排查 Service 不通时，检查 Endpoints 是否包含预期的后端 Pod。\n- 使用 `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` 查看。\n- Headless Service 的 Endpoints 直接返回 Pod IP。",
     "- [Endpoints - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/service/#endpoints)",
     "- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/headless-service|Headless Service]]\n- [[系统基础/topic-dictionary/networking/clusterip|ClusterIP]]\n- [[系统基础/topic-dictionary/networking/coredns|CoreDNS]]\n- [[系统基础/topic-dictionary/networking/networkpolicy|NetworkPolicy]]"),

    ("networking", "networkpolicy", "网络策略", "NetworkPolicy",
     ["networkpolicy", "security", "cni"],
     "NetworkPolicy 是 Kubernetes 中控制 Pod 之间以及 Pod 与外部网络之间流量的资源对象。它采用默认允许（default allow）模型，通过定义 ingress 和 egress 规则实现网络隔离。",
     "### 核心概念\n\n- **Pod Selector**：选择策略作用的目标 Pod。\n- **Ingress 规则**：控制入站流量（谁可以访问目标 Pod）。\n- **Egress 规则**：控制出站流量（目标 Pod 可以访问谁）。\n- **Policy Types**：指定 `Ingress`、`Egress` 或两者。\n\n### 默认行为\n\n| 场景 | 行为 |\n|------|------|\n| 无 NetworkPolicy | 允许所有流量 |\n| 仅有 Ingress 策略 | 入站受限，出站不受限 |\n| 同时有 Ingress + Egress | 双向受限 |",
     "- NetworkPolicy 需要 CNI 插件支持（Calico、Cilium、Weave 等）。\n- 不支持的 CNI 会静默忽略 NetworkPolicy 资源。\n- 规则中的 `namespaceSelector` 和 `podSelector` 可以组合使用。\n- `ipBlock` 支持 CIDR 匹配（除 `except` 子网外）。",
     "- 为每个命名空间创建默认 deny-all 策略，再按需放行。\n- 使用标签选择器精确控制流量，避免过度宽松的策略。\n- 定期审计 NetworkPolicy 覆盖情况，确保无遗漏。\n- 生产环境建议配合 Cilium 或 Calico 的高级网络策略功能。",
     "- [NetworkPolicy - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/network-policies/)",
     "- [[系统基础/topic-dictionary/networking/cni|CNI]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/security/security-context|Security Context]]"),

    ("scheduling", "scheduler", "调度器", "kube-scheduler",
     ["scheduler", "control-plane", "scheduling"],
     "kube-scheduler 是 Kubernetes 控制平面组件，负责将新创建的 Pod 分配到最合适的节点上。它通过一系列过滤（Filtering）和打分（Scoring）算法实现智能调度决策。",
     "### 调度流程\n\n1. **Filtering（过滤）**：排除不满足 Pod 要求的节点（资源不足、污点不匹配、亲和性约束等）。\n2. **Scoring（打分）**：对剩余节点评分（资源均衡、数据本地性、拓扑分布等）。\n3. **Binding（绑定）**：将 Pod 绑定到得分最高的节点。\n\n### 内置调度插件\n\n| 插件 | 阶段 | 功能 |\n|------|------|------|\n| NodeResourcesFit | Filter + Score | 资源匹配和均衡 |\n| TaintToleration | Filter + Score | 污点容忍检查 |\n| PodTopologySpread | Filter + Score | 拓扑分布约束 |\n| InterPodAffinity | Filter + Score | Pod 亲和/反亲和 |\n| VolumeBinding | Filter + Reserve | 存储卷绑定 |",
     "- 调度器以 Scheduling Framework 架构运行，支持插件化扩展。\n- Scheduler Extender 和 Scheduling Plugin 两种扩展方式。\n- Priority Class 影响 Pod 的调度优先级和抢占（Preemption）行为。\n- 调度器指标通过 `/metrics` 端点暴露（scheduling_duration、binding_duration 等）。",
     "- 使用 `--percentage-of-nodes-to-score` 调优大规模集群的调度性能。\n- 自定义调度需求优先使用 Scheduling Plugin 而非 Extender。\n- 为关键工作负载设置 PriorityClass 确保调度优先级。\n- 使用 Descheduler 周期性重新平衡集群中的 Pod 分布。",
     "- [kube-scheduler - Kubernetes Docs](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-scheduler/)",
     "- [[系统基础/topic-dictionary/scheduling/affinity|Affinity]]\n- [[系统基础/topic-dictionary/scheduling/taint|Taint]]\n- [[系统基础/topic-dictionary/scheduling/toleration|Toleration]]\n- [[系统基础/topic-dictionary/scheduling/topology-spread-constraints|Topology Spread Constraints]]\n- [[系统基础/topic-dictionary/scheduling/resource-request|Resource Request]]"),

    ("networking", "dns-resolution", "DNS 解析", "DNS Resolution",
     ["dns", "coredns", "networking"],
     "DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP 地址的过程。集群内部 DNS 由 CoreDNS 提供，遵循 `<service>.<namespace>.svc.cluster.local` 的命名格式。",
     "### DNS 记录格式\n\n| 资源类型 | DNS 格式 | 示例 |\n|----------|----------|------|\n| Service (ClusterIP) | `<svc>.<ns>.svc.cluster.local` | `nginx.default.svc.cluster.local` |\n| Headless Service | 返回所有 Pod IP | `db.default.svc.cluster.local` → 多个 A 记录 |\n| Pod | `<pod-ip-dashed>.<ns>.pod.cluster.local` | `10-244-0-5.default.pod.cluster.local` |\n| SRV 记录 | `_<port>._<proto>.<svc>.<ns>.svc.cluster.local` | 用于发现命名端口 |\n\n### 解析流程\n\nPod 内的 DNS 查询 → Pod DNS Config → CoreDNS → 上游 DNS（如需要）",
     "- CoreDNS 以 Deployment 形式运行在 kube-system 命名空间。\n- Pod 的 `/etc/resolv.conf` 由 kubelet 自动配置指向 CoreDNS。\n- `dnsPolicy` 控制 Pod 的 DNS 行为：`ClusterFirst`（默认）、`Default`、`None`。\n- NodeLocal DNSCache 减少 CoreDNS 压力，提升解析性能。",
     "- 排查 DNS 问题时使用 `nslookup` 或 `dig` 测试解析。\n- 大集群启用 NodeLocal DNSCache 避免 CoreDNS 成为瓶颈。\n- 使用 `ndots:2` 减少不必要的域名后缀搜索。\n- 外部 DNS 查询使用 ExternalDNS 管理云 DNS 记录。",
     "- [DNS for Services and Pods - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)",
     "- [[系统基础/topic-dictionary/networking/coredns|CoreDNS]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/headless-service|Headless Service]]\n- [[系统基础/topic-dictionary/networking/endpoint|Endpoints]]\n- [[系统基础/topic-dictionary/networking/networkpolicy|NetworkPolicy]]"),

    ("networking", "ipip", "IPIP", "IPIP",
     ["ipip", "tunnel", "networking", "cni"],
     "IPIP（IP-in-IP）是一种网络隧道协议，将一个 IP 数据包封装在另一个 IP 数据包中传输。在 Kubernetes 网络中，IPIP 常用于跨节点 Pod 通信，是 Calico 等 CNI 插件支持的封装模式之一。",
     "### IPIP 封装原理\n\n```\n原始包: [IP Header | Payload (Pod→Pod)]\n封装后: [Outer IP Header (Node→Node) | IP Header | Payload]\n```\n\n### 与其他隧道协议对比\n\n| 协议 | 封装层 | 开销 | MTU 影响 | 典型使用 |\n|------|--------|------|----------|----------|\n| IPIP | IP-in-IP | 20 bytes | -20 | Calico IPIP 模式 |\n| VXLAN | Ethernet-in-UDP | 50+ bytes | -50 | Calico/Cilium VXLAN |\n| Geneve | 类似 VXLAN | 可变 | 可变 | OVN-Kubernetes |",
     "- IPIP 模式的 MTU 比 VXLAN 小 20 字节（外层 IP 头开销）。\n- IPIP 不支持跨子网（不同 L2 域）通信，仅限同子网节点。\n- Calico 支持 IPIP Always（所有跨节点流量）和 CrossSubnet（仅跨子网）两种模式。\n- IPIP 流量在节点上是 `tunl0` 接口。",
     "- 同子网集群优先使用 IPIP 模式，开销最小。\n- 跨子网或需要 L2 隔离时使用 VXLAN 模式。\n- 排查 IPIP 问题时检查 `tunl0` 接口状态和路由表。\n- 注意 IPIP 与 IPsec 的兼容性。",
     "- [Calico IPIP Mode - Project Calico](https://docs.tigera.io/calico/latest/networking/configure-ip-addresses/ipip)",
     "- [[系统基础/topic-dictionary/networking/vxlan|VXLAN]]\n- [[系统基础/topic-dictionary/networking/cni|CNI]]\n- [[系统基础/topic-dictionary/networking/networkpolicy|NetworkPolicy]]\n- [[系统基础/topic-dictionary/networking/clusterip|ClusterIP]]\n- [[系统基础/topic-dictionary/networking/nodeport|NodePort]]"),

    ("networking", "nat", "网络地址转换", "NAT (Network Address Translation)",
     ["nat", "networking", "kube-proxy"],
     "NAT（Network Address Translation，网络地址转换）是将一个 IP 地址和端口映射到另一个的过程。在 Kubernetes 中，NAT 是 Service 实现流量转发的核心机制，由 kube-proxy 通过 iptables 或 IPVS 规则执行 SNAT 和 DNAT。",
     "### Kubernetes 中的 NAT 类型\n\n| 类型 | 方向 | 用途 |\n|------|------|------|\n| DNAT | 入站 | Service ClusterIP → Pod IP |\n| SNAT (Masquerade) | 出站 | Pod 访问外部时隐藏源 IP |\n\n### 工作原理\n\n```\nClient → Service ClusterIP:Port\n       → [kube-proxy DNAT] → Pod IP:Port\nPod → External\n       → [Masquerade SNAT] → Node IP → External\n```",
     "- kube-proxy 的 iptables 模式通过 `KUBE-SERVICES` 和 `KUBE-SVC-*` 链实现 DNAT。\n- IPVS 模式使用内核 IPVS 模块，性能优于 iptables。\n- `externalTrafficPolicy: Local` 保留客户端源 IP（不做 SNAT）。\n- `masquerade-all` 配置强制对所有出站流量做 SNAT。",
     "- 需要保留客户端源 IP 时使用 `externalTrafficPolicy: Local`。\n- 大规模集群优先使用 IPVS 模式替代 iptables。\n- 排查 NAT 问题时使用 `iptables -t nat -L -n` 检查规则。\n- 注意 SNAT 对网络策略和日志的影响（源 IP 变为节点 IP）。",
     "- [NAT - Wikipedia](https://en.wikipedia.org/wiki/Network_address_translation)",
     "- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/clusterip|ClusterIP]]\n- [[系统基础/topic-dictionary/networking/nodeport|NodePort]]\n- [[系统基础/topic-dictionary/networking/loadbalancer|LoadBalancer]]\n- [[系统基础/topic-dictionary/fundamentals/kube-proxy|Kube-proxy]]"),

    ("fundamentals", "kubernetes", "Kubernetes", "Kubernetes (K8s)",
     ["kubernetes", "k8s", "container-orchestration"],
     "Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术栈的核心基础设施。",
     "### 核心架构\n\n```\n┌─────────────────────────────────┐\n│         Control Plane           │\n│  ┌──────────┐  ┌─────────────┐  │\n│  │apiserver │  │  scheduler  │  │\n│  └──────────┘  └─────────────┘  │\n│  ┌──────────────────────────┐   │\n│  │  controller-manager      │   │\n│  └──────────────────────────┘   │\n│  ┌──────┐                       │\n│  │ etcd │                       │\n│  └──────┘                       │\n└─────────────────────────────────┘\n         ↕\n┌─────────────────────────────────┐\n│         Worker Nodes            │\n│  ┌────────┐  ┌───────────────┐  │\n│  │kubelet │  │  kube-proxy   │  │\n│  └────────┘  └───────────────┘  │\n│  ┌─────────────────────────┐    │\n│  │  Container Runtime      │    │\n│  └─────────────────────────┘    │\n└─────────────────────────────────┘\n```\n\n### 声明式模型\n\nKubernetes 采用声明式 API：用户描述「期望状态」（Desired State），控制器持续将「实际状态」推向「期望状态」。",
     "- **自动调度**：根据资源需求和约束将 Pod 调度到最佳节点。\n- **自愈能力**：Pod 崩溃自动重启，节点故障自动迁移。\n- **水平扩缩**：通过 HPA/VPA 自动调整资源。\n- **服务发现与负载均衡**：通过 Service 和 Ingress 暴露应用。\n- **滚动更新与回滚**：零停机部署和快速回滚。\n- **声明式配置**：GitOps 友好的基础设施即代码。",
     "- 使用 kubeadm 初始化生产级集群。\n- 遵循最小权限原则配置 RBAC。\n- 为所有工作负载设置 resource requests/limits。\n- 使用命名空间隔离不同团队或环境的工作负载。\n- 启用审计日志（Audit Log）追踪 API 操作。\n- 定期升级集群版本，关注弃用 API 迁移。",
     "- [Kubernetes Official Documentation](https://kubernetes.io/docs/)",
     "- [[系统基础/topic-dictionary/fundamentals/pod|Pod]]\n- [[系统基础/topic-dictionary/fundamentals/node|Node]]\n- [[系统基础/topic-dictionary/fundamentals/namespace|Namespace]]\n- [[系统基础/topic-dictionary/fundamentals/cluster|Cluster]]\n- [[系统基础/topic-dictionary/fundamentals/cncf|CNCF]]"),
]

# ── Part B: 高频 CN 生态引用术语 ──────────────────────

CN_ECOSYSTEM = [
    ("tooling", "helm", "Helm", "Helm",
     ["helm", "package-manager", "gitops"],
     "Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，是 Kubernetes 生态中最广泛使用的工具之一。",
     "### 核心概念\n\n- **Chart**：Helm 包，包含模板化的 K8s 资源定义。\n- **Release**：Chart 的一次部署实例。\n- **Repository**：Chart 仓库（如 Artifact Hub）。\n\n### Chart 结构\n\n```\nmychart/\n├── Chart.yaml       # 元数据\n├── values.yaml      # 默认配置值\n├── templates/       # Go 模板文件\n│   ├── deployment.yaml\n│   ├── service.yaml\n│   └── _helpers.tpl\n└── charts/          # 依赖 Chart\n```",
     "- **Helm v3**：移除了 Tiller 服务端组件，直接通过 kubeconfig 与 apiserver 交互。\n- **Release 版本管理**：每次 `helm upgrade` 自动生成新版本，支持 `helm rollback`。\n- **模板引擎**：基于 Go text/template，支持 values 注入和条件渲染。\n- **Hook 机制**：在 install/upgrade/delete 前后执行 Job/Pod。\n- **OCI Registry**：Helm v3.8+ 支持将 Chart 推送到 OCI 兼容的容器仓库。",
     "- 使用 `helm template` 本地渲染 Chart 检查生成的 YAML。\n- 使用 `helm lint` 验证 Chart 语法和最佳实践。\n- 生产环境推荐配合 Helmfile 或 ArgoCD 进行声明式管理。\n- 避免在 Chart 中硬编码镜像版本，使用 values 注入。\n- 使用 `helm diff` 插件预览 upgrade 变更。",
     "- [Helm Official Documentation](https://helm.sh/docs/)",
     "- [[系统基础/topic-dictionary/tooling/kubectl|Kubectl]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/platform-engineering/manifest|Manifest]]\n- [[系统基础/topic-dictionary/operations/rolling-update|Rolling Update]]"),

    ("fundamentals", "containerd", "containerd", "containerd",
     ["containerd", "cri", "container-runtime"],
     "containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过 CRI 接口），负责容器的完整生命周期管理。",
     "### 架构层次\n\n```\nkubelet → CRI → containerd → runc → Linux Kernel\n                     ↓\n              shim (per-container)\n```\n\n### 核心组件\n\n| 组件 | 职责 |\n|------|------|\n| containerd daemon | 容器生命周期管理 |\n| containerd-shim | 每个容器的独立进程，与 daemon 解耦 |\n| runc | OCI 运行时规范实现 |\n| ctr / crictl | 命令行工具 |",
     "- **CRI 接口**：kubelet 通过 gRPC 调用 containerd 的 CRI 实现。\n- **shim 架构**：containerd-shim 为每个容器独立运行，containerd 重启不影响容器。\n- **镜像管理**：支持 OCI 和 Docker 镜像格式。\n- **快照管理**：overlayfs 等快照驱动管理容器文件系统层。\n- 配置文件位于 `/etc/containerd/config.toml`。",
     "- 使用 `crictl` 而非 `docker` 命令调试容器。\n- 配置 mirror 加速镜像拉取（特别是国内环境）。\n- 启用 `SystemdCgroup` 与 kubelet 保持一致。\n- 监控 containerd 的 gRPC 延迟和容器启动时间指标。",
     "- [containerd Official](https://containerd.io/)",
     "- [[系统基础/topic-dictionary/fundamentals/cri|CRI]]\n- [[系统基础/topic-dictionary/fundamentals/kubelet|Kubelet]]\n- [[系统基础/topic-dictionary/fundamentals/pod|Pod]]\n- [[系统基础/topic-dictionary/fundamentals/container|Container]]\n- [[系统基础/topic-dictionary/fundamentals/worker-node|Worker Node]]"),

    ("networking", "cilium", "Cilium", "Cilium",
     ["cilium", "cni", "ebpf", "networkpolicy"],
     "Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、细粒度的安全策略和深度可观测性，已成为云原生网络的事实标准之一。",
     "### 核心架构\n\n- **eBPF 数据平面**：在内核态处理网络包，替代 kube-proxy 的 iptables。\n- **Cilium Agent**：每节点 DaemonSet，管理策略和配置。\n- **Hubble**：内置的网络可观测性组件，提供流量可视化。\n- **Cilium Operator**：集群级管理组件（IPAM、身份管理）。\n\n### 与 iptables 对比\n\n| 特性 | iptables | Cilium (eBPF) |\n|------|----------|---------------|\n| 规则处理 | O(n) 线性扫描 | O(1) 哈希查找 |\n| 策略粒度 | L3/L4 | L3-L7（含 HTTP/gRPC） |\n| 性能 | 规则多时性能下降 | 恒定性能 |",
     "- 完全替代 kube-proxy，使用 eBPF 实现 Service 负载均衡。\n- 支持 FQDN Policy（基于域名的网络策略）。\n- 支持 Cluster Mesh 实现多集群网络互通。\n- Gateway API 原生支持。\n- Tetragon 提供运行时安全检测和进程级可观测性。",
     "- 新集群优先选择 Cilium 作为 CNI。\n- 启用 Cilium 的 kube-proxy 替代模式提升 Service 性能。\n- 使用 Hubble 进行网络故障排查和流量分析。\n- 配合 CiliumNetworkPolicy 实现 L7 层安全策略。\n- 使用 Cilium CLI 进行安装和诊断。",
     "- [Cilium Official Documentation](https://docs.cilium.io/)",
     "- [[系统基础/topic-dictionary/networking/cni|CNI]]\n- [[系统基础/topic-dictionary/networking/networkpolicy|NetworkPolicy]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/fundamentals/kube-proxy|Kube-proxy]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

    ("networking", "coredns", "CoreDNS", "CoreDNS",
     ["coredns", "dns", "networking"],
     "CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的 DNS 解析服务，是集群内服务发现的基础设施。",
     "### 架构\n\nCoreDNS 以 Deployment 形式运行在 kube-system 命名空间，通过 ConfigMap（`coredns`）配置插件链。\n\n### 核心插件\n\n| 插件 | 功能 |\n|------|------|\n| kubernetes | 解析集群内 Service/Pod DNS |\n| forward | 转发外部 DNS 查询 |\n| cache | DNS 响应缓存 |\n| loop | 检测 DNS 转发循环 |\n| errors | 错误日志 |\n| health | 健康检查端点 |\n| prometheus | 指标暴露 |",
     "- 插件链按 Corefile 中的顺序执行。\n- 支持 DNS-over-TLS 和 DNS-over-gRPC。\n- 通过 `hosts` 插件可添加自定义 DNS 记录。\n- `rewrite` 插件支持 DNS 记录重写。\n- 指标通过 `/metrics` 端点暴露给 Prometheus。",
     "- 大集群启用 NodeLocal DNSCache 减少 CoreDNS 压力。\n- 使用 `cache` 插件合理设置 TTL 减少查询量。\n- 排查 DNS 问题时检查 CoreDNS Pod 日志和资源使用。\n- 配置 `forward` 插件的上游 DNS 服务器。\n- 使用 `rewrite` 插件处理内部域名映射。",
     "- [CoreDNS Official](https://coredns.io/)",
     "- [[系统基础/topic-dictionary/networking/dns-resolution|DNS Resolution]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/headless-service|Headless Service]]\n- [[系统基础/topic-dictionary/networking/endpoint|Endpoints]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

    ("networking", "envoy", "Envoy", "Envoy Proxy",
     ["envoy", "service-mesh", "proxy", "cncf"],
     "Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo 等云原生项目的数据平面基础，广泛用于服务网格、API 网关和入口控制器场景。",
     "### 核心概念\n\n- **Listener**：监听入站连接的端口/地址。\n- **Filter Chain**：处理连接的过滤器链（认证、限流、路由等）。\n- **Cluster**：上游服务集群（后端端点集合）。\n- **Route**：路由规则，将请求映射到 Cluster。\n\n### xDS API\n\nEnvoy 通过 xDS（发现服务 API）动态获取配置：\n\n| xDS | 用途 |\n|-----|------|\n| LDS | Listener 发现 |\n| RDS | Route 发现 |\n| CDS | Cluster 发现 |\n| EDS | Endpoint 发现 |\n| SDS | Secret 发现 |",
     "- **Sidecar 模式**：作为 Pod 的 sidecar 容器运行（Istio 默认）。\n- **Gateway 模式**：作为入口/出口网关运行。\n- 支持 HTTP/1.1、HTTP/2、gRPC、TCP、UDP 协议。\n- 内置熔断、重试、超时、限流等弹性功能。\n- 支持 Wasm 扩展自定义过滤器。",
     "- 使用 Envoy 作为 API Gateway 的数据平面（Gateway API 支持）。\n- 配合 Istio 构建服务网格实现 mTLS 和流量管理。\n- 使用 Envoy Admin API（`/config_dump`、`/stats`）排查问题。\n- 监控 Envoy 的 upstream_rq_time 和 upstream_cx_connect_fail 指标。\n- 合理配置 Circuit Breaker 防止级联故障。",
     "- [Envoy Proxy Official](https://www.envoyproxy.io/)",
     "- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/networkpolicy|NetworkPolicy]]\n- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

    ("networking", "istio", "Istio", "Istio",
     ["istio", "service-mesh", "envoy"],
     "Istio 是最广泛使用的开源服务网格平台，为微服务通信提供流量管理、安全（mTLS）、可观测性和策略执行能力。它使用 Envoy 作为数据平面代理，通过控制平面（istiod）统一管理配置。",
     "### 核心架构\n\n- **istiod**：控制平面，合并了原来的 Pilot、Galley、Citadel。\n- **Envoy Sidecar**：自动注入到每个 Pod 的数据平面代理。\n- **Istio Gateway**：集群入口/出口流量管理。\n\n### 流量管理原语\n\n| 资源 | 功能 |\n|------|------|\n| VirtualService | 路由规则（权重、header 匹配等） |\n| DestinationRule | 上游策略（负载均衡、熔断、子集） |\n| Gateway | 入口/出口 L4-L6 配置 |\n| ServiceEntry | 网格外部服务声明 |",
     "- **mTLS**：自动为服务间通信启用双向 TLS 加密。\n- **流量拆分**：通过 VirtualService 实现金丝雀发布和 A/B 测试。\n- **故障注入**：模拟延迟和错误，验证服务韧性。\n- **可观测性**：自动生成分布式追踪、指标和访问日志。\n- Istio Ambient Mesh：无 sidecar 的新模式，降低资源开销。",
     "- 新集群评估是否需要服务网格（非所有场景都需要 Istio）。\n- 使用 STRICT mTLS 模式确保所有服务间通信加密。\n- 合理配置 DestinationRule 的 ConnectionPool 和 OutlierDetection。\n- 使用 Kiali 可视化服务网格拓扑和流量。\n- 关注 Istio Ambient Mesh 的发展，减少 sidecar 开销。",
     "- [Istio Official Documentation](https://istio.io/latest/docs/)",
     "- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]"),

    ("observability", "opentelemetry", "OpenTelemetry", "OpenTelemetry (OTel)",
     ["opentelemetry", "observability", "tracing", "metrics", "logging"],
     "OpenTelemetry（简称 OTel）是 CNCF 孵化项目，提供统一的分布式系统可观测性标准。它将 Traces、Metrics、Logs 三大支柱统一在一套 API 和 SDK 中，已成为可观测性数据采集的事实标准。",
     "### 三大支柱\n\n| 支柱 | 说明 | 代表工具 |\n|------|------|----------|\n| Traces | 请求在分布式系统中的完整路径 | Jaeger、Tempo |\n| Metrics | 系统/应用的数值指标 | Prometheus、VictoriaMetrics |\n| Logs | 结构化日志事件 | Loki、ELK |\n\n### 核心组件\n\n- **API**：语言无关的观测数据生成接口。\n- **SDK**：API 的实现，包含采样、处理和导出。\n- **OTLP**：OpenTelemetry Protocol，统一的数据传输协议。\n- **Collector**：数据收集、处理和路由的中间层。",
     "- **OTLP 协议**：基于 gRPC/HTTP，统一传输 Traces、Metrics、Logs。\n- **自动 Instrumentation**：Java Agent、Node.js SDK 等无需修改代码即可采集。\n- **Collector Pipeline**：receivers → processors → exporters，灵活路由数据。\n- **Context Propagation**：通过 W3C Trace Context 在微服务间传递追踪上下文。\n- 支持 Kubernetes Operator 自动注入 Instrumentation。",
     "- 新项目直接使用 OpenTelemetry SDK 替代 Jaeger/Zipkin 等独立方案。\n- 部署 OTel Collector 统一采集和路由观测数据。\n- 使用 auto-instrumentation 降低接入成本。\n- 配置合理的采样率避免数据爆炸（如 traceID ratio 采样）。\n- 结合 Grafana Tempo/Jaeger 可视化和查询追踪数据。",
     "- [OpenTelemetry Official](https://opentelemetry.io/docs/)",
     "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/jaeger|Jaeger]]\n- [[系统基础/topic-dictionary/observability/logging|Logging]]\n- [[系统基础/topic-dictionary/observability/alertmanager|Alertmanager]]"),

    ("observability", "jaeger", "Jaeger", "Jaeger",
     ["jaeger", "tracing", "observability", "cncf"],
     "Jaeger 是 CNCF 毕业项目，提供分布式追踪（Distributed Tracing）功能，用于监控和排查微服务架构中的请求链路。它兼容 OpenTelemetry，是 Kubernetes 生态中最常用的追踪后端之一。",
     "### 核心架构\n\n- **Jaeger Agent**：轻量级 sidecar，接收应用发送的 span 数据。\n- **Jaeger Collector**：接收和处理 span，写入存储后端。\n- **Jaeger Query**：提供 Web UI 和 API 查询追踪数据。\n- **Storage**：支持 Elasticsearch、Cassandra、Kafka + Flink 等。\n\n### 追踪模型\n\n```\nTrace\n├── Span A (Service A)\n│   ├── Span B (Service B)\n│   │   └── Span D (Database)\n│   └── Span C (Service C)\n```",
     "- 完全兼容 OpenTelemetry Collector 和 OTLP 协议。\n- 支持自适应采样（Adaptive Sampling），根据流量自动调整。\n- Jaeger v2 基于 OpenTelemetry Collector 架构重构。\n- 支持 Service Performance Monitoring（SPM）自动聚合指标。\n- 提供 Spark/Flink 作业进行离线数据分析。",
     "- 新部署推荐使用 Jaeger v2（基于 OTel Collector）。\n- 配合 OpenTelemetry SDK 采集追踪数据。\n- 使用 Jaeger UI 分析慢请求和错误链路。\n- 为高流量服务配置合理采样率（如 1%）。\n- 存储后端优先选择 Elasticsearch 或 Tempo。",
     "- [Jaeger Official](https://www.jaegertracing.io/)",
     "- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/logging|Logging]]\n- [[系统基础/topic-dictionary/networking/envoy|Envoy]]"),

    ("observability", "thanos", "Thanos", "Thanos",
     ["thanos", "prometheus", "observability", "cncf"],
     "Thanos 是 CNCF 孵化项目，为 Prometheus 提供高可用、长期存储和多集群全局视图能力。它解决了单实例 Prometheus 的存储和扩展瓶颈，是大规模 Kubernetes 监控的首选方案。",
     "### 核心组件\n\n| 组件 | 功能 |\n|------|------|\n| Sidecar | 与 Prometheus 同 Pod 部署，上传 TSDB 数据到对象存储 |\n| Store Gateway | 从对象存储查询历史数据 |\n| Query | 合并多个 Prometheus/Store Gateway 的查询结果 |\n| Compactor | 压缩和降采样对象存储中的历史数据 |\n| Ruler | 在 Thanos 级别执行告警规则 |\n| Receive | 接收远程写入的数据（Push 模式） |",
     "- **全局视图**：跨集群查询所有 Prometheus 实例的数据。\n- **无限保留**：TSDB 数据上传到 S3/GCS/MinIO 实现长期存储。\n- **降采样**：自动将历史数据降采样（5m/1h）减少查询开销。\n- **去重**：相同指标的多个副本自动去重。\n- 兼容 PromQL，无需修改现有查询。",
     "- 多集群环境使用 Thanos Query 提供统一查询入口。\n- 对象存储优先选择 S3 兼容的存储（MinIO、AWS S3）。\n- 配置 Compactor 的 retention 策略管理存储成本。\n- 使用 Thanos Ruler 实现全局告警规则。\n- 考虑 VictoriaMetrics 作为 Thanos 的替代方案。",
     "- [Thanos Official](https://thanos.io/)",
     "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/alertmanager|Alertmanager]]\n- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]"),

    ("operations", "argo", "Argo", "Argo",
     ["argo", "gitops", "cicd", "cncf"],
     "Argo 是 CNCF 毕业项目集合，包含 Argo CD（GitOps 持续部署）、Argo Workflows（容器原生工作流引擎）、Argo Rollouts（渐进式发布）和 Argo Events（事件驱动）。Argo CD 是 Kubernetes 生态中最主流的 GitOps 工具。",
     "### Argo 项目家族\n\n| 项目 | 功能 | 成熟度 |\n|------|------|--------|\n| **Argo CD** | GitOps 持续部署 | Graduated |\n| **Argo Workflows** | 容器原生 DAG 工作流 | Graduated |\n| **Argo Rollouts** | 金丝雀/蓝绿发布 | Incubating |\n| **Argo Events** | 事件驱动自动化 | Incubating |\n\n### Argo CD 核心概念\n\n- **Application**：声明式的 GitOps 应用定义。\n- **Sync Policy**：自动或手动将 Git 变更同步到集群。\n- **Health Check**：自定义资源健康状态判断。\n- **Hook**：Pre/Post Sync 操作（如数据库迁移）。",
     "- **GitOps 模型**：Git 仓库作为唯一的真实来源（Single Source of Truth）。\n- **Pull 模式**：Argo CD 主动从 Git 拉取变更，而非 CI push。\n- **多集群管理**：ApplicationSet 批量管理多集群部署。\n- **渐进式发布**：Argo Rollouts 支持金丝雀和蓝绿部署策略。\n- **SSO/RBAC**：集成 OIDC/LDAP 和应用级 RBAC。",
     "- 使用 Argo CD 管理所有 K8s 资源的 GitOps 部署。\n- 配置 auto-sync + self-heal 实现全自动运维。\n- 使用 ApplicationSet 管理多环境/多集群部署。\n- 配合 Argo Rollouts 实现金丝雀发布。\n- 启用 Argo CD 的 RBAC 和 SSO 控制访问权限。",
     "- [Argo CD Official](https://argo-cd.readthedocs.io/)",
     "- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/operations/rolling-update|Rolling Update]]\n- [[系统基础/topic-dictionary/operations/rollback|Rollback]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]"),

    ("security", "trivy", "Trivy", "Trivy",
     ["trivy", "security", "scanning", "cncf"],
     "Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏感信息，是 Kubernetes 安全扫描的事实标准工具。",
     "### 扫描能力\n\n| 扫描目标 | 内容 |\n|----------|------|\n| Container Image | CVE 漏洞、OS 包、应用依赖 |\n| Filesystem | IaC 错误配置、密钥泄露 |\n| Git Repository | 代码中的安全问题和密钥 |\n| Kubernetes Cluster | 集群配置错误和权限风险 |\n| SBOM | 软件物料清单生成 |\n\n### 支持的漏洞数据库\n\nNVD、Alpine、Debian、Ubuntu、Red Hat、Amazon Linux、GitHub Advisory 等。",
     "- **Trivy Operator**：Kubernetes 原生部署，自动扫描集群中的镜像和配置。\n- **CI/CD 集成**：作为 GitHub Action、GitLab CI 步骤扫描镜像。\n- **SBOM 生成**：输出 SPDX/CycloneDX 格式的软件物料清单。\n- **Misconfiguration**：扫描 Terraform、Kubernetes YAML、Dockerfile。\n- 支持 JSON/Table/SARIF 多种输出格式。",
     "- CI/CD 流水线中集成 `trivy image` 扫描构建的镜像。\n- 部署 Trivy Operator 持续扫描集群中的运行镜像。\n- 使用 `trivy config` 检查 Kubernetes YAML 的安全配置。\n- 将 Trivy 结果集成到 GitHub Security Advisory。\n- 定期生成 SBOM 满足合规要求。",
     "- [Trivy Official](https://aquasecurity.github.io/trivy/)",
     "- [[系统基础/topic-dictionary/security/pod-security-policy|Pod Security Policy]]\n- [[系统基础/topic-dictionary/security/security-context|Security Context]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/security/admission-controller|Admission Controller]]"),

    ("security", "falco", "Falco", "Falco",
     ["falco", "security", "runtime-security", "cncf", "ebpf"],
     "Falco 是 CNCF 毕业项目，提供云原生运行时安全检测能力。它通过系统调用（syscall）监控容器和主机的异常行为，是 Kubernetes 运行时安全的标准工具。",
     "### 核心架构\n\n- **Falco Engine**：规则匹配引擎，处理系统调用事件。\n- **Falco Drivers**：内核模块或 eBPF 探针，采集 syscall 数据。\n- **Falco Rules**：YAML 格式的安全规则定义。\n- **Falco Sidekick**：将告警转发到 Slack、Grafana、Kafka 等。\n\n### 规则示例\n\n```yaml\n- rule: Terminal shell in container\n  desc: A shell was opened in a container\n  condition: spawned_process and container and proc.name in (bash, sh, zsh)\n  output: >\n    Shell opened in container\n    (user=%user.name container=%container.name shell=%proc.name)\n  priority: WARNING\n```",
     "- **eBPF 探针**：现代部署推荐使用 eBPF 替代内核模块，更安全。\n- **规则优先级**：Emergency → Critical → Error → Warning → Notice → Info → Debug。\n- **宏和列表**：可组合的 reusable 规则构建块。\n- **插件系统**：支持扩展数据源（K8s Audit Log、CloudTrail 等）。\n- 与 Kubernetes Audit Log 结合实现 API 级别的安全监控。",
     "- 部署 Falco 作为 DaemonSet 监控所有节点的运行时行为。\n- 使用 Falco Talon 实现自动化响应（如杀死可疑进程）。\n- 配合 Falco Sidekick 将告警发送到 Slack/PagerDuty。\n- 自定义规则检测特定于业务的异常行为。\n- 定期审查 Falco 告警，减少误报。",
     "- [Falco Official](https://falco.org/docs/)",
     "- [[系统基础/topic-dictionary/security/trivy|Trivy]]\n- [[系统基础/topic-dictionary/security/security-context|Security Context]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

    ("operations", "cert-manager", "cert-manager", "cert-manager",
     ["cert-manager", "certificate", "tls", "cncf"],
     "cert-manager 是 Kubernetes 原生的证书管理工具，自动化 TLS 证书的签发、续期和吊销。它支持 Let's Encrypt、Vault、自签名 CA 等多种证书颁发源，是集群 TLS 自动化的标准方案。",
     "### 核心资源\n\n| 资源 | 功能 |\n|------|------|\n| Issuer | 命名空间级别的证书颁发源 |\n| ClusterIssuer | 集群级别的证书颁发源 |\n| Certificate | 声明式证书记义 |\n| CertificateRequest | 证书签发请求 |\n| Order/Challenge | ACME 协议交互 |\n\n### ACME 流程\n\n```\nCertificate → CertificateRequest → Order → Challenge (HTTP-01/DNS-01) → Let's Encrypt → 签发证书\n```",
     "- **ACME 协议**：支持 Let's Encrypt 的 HTTP-01 和 DNS-01 验证。\n- **自动续期**：证书到期前自动续期（默认 2/3 生命周期时触发）。\n- **Vault 集成**：支持 HashiCorp Vault 作为证书颁发源。\n- **istio-csr**：为 Istio 提供自动化的 mTLS 证书管理。\n- **approve**：内置 RBAC 控制证书审批流程。",
     "- 为 Ingress 资源自动签发 Let's Encrypt 证书（配合 cert-manager annotation）。\n- 生产环境使用 ClusterIssuer 统一管理证书颁发源。\n- DNS-01 验证适合通配符证书（*.example.com）。\n- 监控证书到期时间，设置 30 天到期告警。\n- 考虑使用 step-ca 或 Vault PKI 作为内部 CA。",
     "- [cert-manager Official](https://cert-manager.io/docs/)",
     "- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/security/certificate-authority|Certificate Authority]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/security/webhook|Webhook]]\n- [[系统基础/topic-dictionary/networking/istio|Istio]]"),

    ("storage", "rook", "Rook", "Rook",
     ["rook", "storage", "operator", "cncf"],
     "Rook 是 CNCF 毕业项目，为 Kubernetes 提供云原生存储编排平台。它通过 Operator 模式自动化部署和管理分布式存储系统（Ceph、EdgeFS 等），让存储系统在 Kubernetes 中像使用云服务一样简单。",
     "### 核心架构\n\n- **Rook Operator**：管理存储集群的生命周期（安装、升级、扩缩、故障恢复）。\n- **Ceph Cluster**：Rook 管理的分布式存储后端（最常用）。\n- **CSI Driver**：Rook-Ceph CSI 提供 PV 动态制备。\n\n### Rook-Ceph 存储能力\n\n| 类型 | K8s 资源 | 说明 |\n|------|----------|------|\n| Block (RBD) | ReadWriteOnce PV | 数据库、有状态应用 |\n| Filesystem (CephFS) | ReadWriteMany PV | 共享文件存储 |\n| Object (RGW) | S3 兼容 API | 对象存储、备份目标 |",
     "- **全自动运维**：OSD 故障自动恢复、数据自动重平衡。\n- **弹性扩缩**：动态添加/移除 OSD 节点。\n- **加密**：支持 OSD 级别的静态加密（encryption at rest）。\n- **Dashboard**：内置 Ceph Dashboard 监控存储健康。\n- 支持快照（Snapshot）和克隆（Clone）功能。",
     "- 需要 Kubernetes 原生存储能力时优先考虑 Rook-Ceph。\n- 确保至少有 3 个 OSD 节点实现数据冗余。\n- 为 RBD 和 CephFS 分别创建 StorageClass。\n- 监控 Ceph 集群健康状态（HEALTH_OK/WARN/ERR）。\n- 配置 Pool 的副本数和故障域（failureDomain）。",
     "- [Rook Official](https://rook.io/docs/rook/latest/)",
     "- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]\n- [[系统基础/topic-dictionary/storage/persistent-volume-claim|Persistent Volume Claim]]\n- [[系统基础/topic-dictionary/storage/storage-class|Storage Class]]\n- [[系统基础/topic-dictionary/storage/csi|CSI]]\n- [[系统基础/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]"),

    ("storage", "longhorn", "Longhorn", "Longhorn",
     ["longhorn", "storage", "cncf"],
     "Longhorn 是 SUSE（原 Rancher）开源的 Kubernetes 原生分布式块存储系统，现为 CNCF 孵化项目。它以轻量、易用和自动化著称，特别适合中小规模集群和边缘场景的持久化存储需求。",
     "### 核心特性\n\n- **微服务架构**：每个 Volume 有独立的 Engine 和 Replica 进程。\n- **增量快照与备份**：支持增量快照和备份到 S3/NFS。\n- **自动恢复**：Replica 故障自动重建。\n- **DR Volume**：跨集群灾备卷。\n\n### 与其他存储方案对比\n\n| 特性 | Longhorn | Rook-Ceph | NFS |\n|------|----------|-----------|-----|\n| 复杂度 | 低 | 高 | 中 |\n| 适用规模 | 中小集群 | 大集群 | 任意 |\n| 数据本地性 | 强 | 强 | 弱 |\n| RWX 支持 | NFS-based | CephFS | 原生 |",
     "- Longhorn UI 提供可视化存储管理。\n- 支持 Volume 的在线扩容和迁移。\n- 自动创建 Volume 的定期快照计划。\n- 支持 Volume 的加密和访问控制。\n- 通过 StorageClass 实现 PV 动态制备。",
     "- 中小集群或边缘场景优先考虑 Longhorn。\n- 配置至少 3 个 Replica 确保数据可靠性。\n- 启用自动快照和备份策略。\n- 监控 Longhorn 的 Engine/Replica 状态。\n- 使用 RecurringJob 自动化快照和备份任务。",
     "- [Longhorn Official](https://longhorn.io/docs/)",
     "- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]\n- [[系统基础/topic-dictionary/storage/storage-class|Storage Class]]\n- [[系统基础/topic-dictionary/storage/rook|Rook]]\n- [[系统基础/topic-dictionary/storage/csi|CSI]]\n- [[系统基础/topic-dictionary/workloads/statefulset|StatefulSet]]"),

    ("platform-engineering", "grpc", "gRPC", "gRPC",
     ["grpc", "rpc", "protobuf", "networking"],
     "gRPC 是 Google 开源的高性能远程过程调用（RPC）框架，使用 Protocol Buffers 作为接口定义语言和数据序列化格式。它是微服务间通信的主流方案之一，在 Kubernetes 生态中广泛用于控制平面和数据平面的内部通信。",
     "### 核心概念\n\n- **Protocol Buffers（protobuf）**：强类型的 IDL 和高效序列化格式。\n- **四种通信模式**：\n\n| 模式 | 说明 |\n|------|------|\n| Unary | 请求-响应 |\n| Server Streaming | 服务端流式推送 |\n| Client Streaming | 客户端流式上传 |\n| Bidirectional Streaming | 双向流式通信 |\n\n### 与 REST 对比\n\n| 特性 | REST/JSON | gRPC/Protobuf |\n|------|-----------|---------------|\n| 序列化 | 文本（JSON） | 二进制（Protobuf） |\n| 性能 | 较低 | 高 |\n| 流式 | 不原生支持 | 原生支持 |\n| 浏览器 | 直接支持 | 需要 gRPC-Web |",
     "- Kubernetes 的 kubelet ↔ apiserver、etcd ↔ apiserver 等组件间通信大量使用 gRPC。\n- Envoy 原生支持 gRPC 代理、负载均衡和重试。\n- gRPC Health Check 协议用于服务健康检查。\n- gRPC Reflection 支持运行时服务发现。\n- gRPC-Gateway 自动生成 REST API 代理。",
     "- 微服务间内部通信优先使用 gRPC。\n- 对外 API 使用 gRPC-Gateway 同时提供 REST 接口。\n- 配置合理的超时和重试策略（gRPC retry policy）。\n- 使用 grpcurl 工具调试 gRPC 服务。\n- 配合 OpenTelemetry 实现 gRPC 调用的分布式追踪。",
     "- [gRPC Official](https://grpc.io/)",
     "- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/istio|Istio]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[系统基础/topic-dictionary/fundamentals/kube-apiserver|Kube-apiserver]]"),
]


# ── Generator ──────────────────────

def main():
    created = []
    skipped = []

    # Part A: k8s-glossary 缺失术语
    for term in K8S_MISSING:
        cat_dir, fn, zh, en, tags, overview, core, mech, use, refs, related = term
        ok = write_file(cat_dir, fn, zh, en, tags, overview, core, mech, use, refs, related)
        if ok:
            created.append(f"[K8s] {cat_dir}/{fn}.md")
        else:
            skipped.append(f"[K8s] {cat_dir}/{fn}.md (已存在)")

    # Part B: CN 生态术语
    for term in CN_ECOSYSTEM:
        cat_dir, fn, zh, en, tags, overview, core, mech, use, refs, related = term
        ok = write_file(cat_dir, fn, zh, en, tags, overview, core, mech, use, refs, related)
        if ok:
            created.append(f"[CN] {cat_dir}/{fn}.md")
        else:
            skipped.append(f"[CN] {cat_dir}/{fn}.md (已存在)")

    print(f"\n新创建: {len(created)} 个文件")
    for f in created:
        print(f"  + {f}")
    if skipped:
        print(f"\n跳过（已存在）: {len(skipped)} 个")
        for f in skipped:
            print(f"  ~ {f}")

if __name__ == '__main__':
    main()
