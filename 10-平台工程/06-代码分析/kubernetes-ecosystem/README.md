---
title: kubernetes-ecosystem 生态集成源码分析系列总览
description: Kubernetes 生态上下游组件源码级集成分析系列导航：CRI 运行时、CNI 网络、CSI 存储、服务网格、可观测性、CI/CD GitOps、镜像仓库/DNS/负载均衡七大方向
summary: 以 K8s 的扩展点契约（CRI/CNI/CSI/webhook/informer/APIService）为主线，源码级剖析 containerd、Calico/Flannel/Cilium/Terway、CSI 驱动、Istio/Linkerd、Prometheus/EFK、ArgoCD/Tekton/Helm、Harbor/CoreDNS/云 LB 与 Kubernetes 的集成点、交互机制与依赖关系。
category: source-analysis
tags:
- k8s
- source-code
- ecosystem
- cri
- cni
- csi
- service-mesh
- gitops
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 10min
intent_queries:
- K8s 生态组件如何与集群集成
- CRI CNI CSI 三大接口的关系
- 生态组件源码分析系列有哪些
trigger_keywords:
- 生态集成
- 扩展点
- CRI
- CNI
- CSI
- 上下游组件
related_domains:
- 集群基础
- 网络
- 存储
- 可观测性
- 平台工程
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kubernetes-ecosystem 生态集成源码分析系列

> 姊妹系列：[[10-平台工程/06-代码分析/kubernetes-core/README.md|kubernetes-core（K8s 本体源码剖析，01~09 篇）]]
> 源码基线：`33-源码/` 各域本地源码树（行号实测，含 containerd-2.3.3/cilium-1.19.6/argo-cd-3.4.5 生态侧源码）；无本地源码的组件从 **K8s 侧集成点源码** + 机制级分析切入，均在各篇头部声明。

## 一条主线：K8s 用「契约」而非「代码」集成生态

K8s 本体不实现运行时、网络、存储、网关——它定义扩展点契约，生态组件各自实现。本系列每篇解剖一类契约的两侧：

| 契约/扩展点 | K8s 侧源码锚点 | 生态实现 |
|------------|---------------|---------|
| CRI（gRPC） | cri-api proto / cri-client | containerd、CRI-O |
| CNI（exec） | 运行时经 libcni 调用 | Calico、Flannel、Cilium、Terway |
| CSI（gRPC + sidecar） | pkg/volume/csi + external sidecars | 各云盘/NAS/分布式存储驱动 |
| Admission Webhook | mutating dispatcher | Istio/Linkerd 注入、策略引擎 |
| Informer/List-Watch | client-go（06 篇机制） | Prometheus SD、ArgoCD、各类 Operator |
| APIService 聚合 | apiserver 聚合层 | metrics-server、custom metrics |
| cloud-provider 接口 | CCM Service 控制器 | 各云 LB、MetalLB |

## 系列目录

| 篇 | 主题 | 核心内容 |
|----|------|---------|
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md\|01 容器运行时与 CRI]] | containerd / CRI-O | CRI proto 契约、kubelet→shim→runc 完整链、RuntimeClass、cgroup driver 陷阱 |
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/02-cni-network-plugins.md\|02 CNI 网络插件]] | Flannel / Terway / Calico / Cilium | libcni 插件链、VXLAN 与 VPC 原生两条路线、eBPF kube-proxy replacement |
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/03-csi-storage-drivers.md\|03 CSI 存储驱动]] | CSI 驱动体系 | 三服务协议、external sidecar 分工、卷六步生命周期、拓扑感知 |
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/04-service-mesh-integration.md\|04 服务网格]] | Istio / Linkerd | webhook 注入、iptables 拦截、xDS 翻译器、ambient 演进 |
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/05-observability-integration.md\|05 监控与日志]] | Prometheus / metrics-server / EFK | kubernetes SD 与抓取循环、Metrics API→HPA、日志落盘与采集链 |
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/06-cicd-gitops-integration.md\|06 CI/CD 与 GitOps]] | Helm / ArgoCD / Tekton / Jenkins | push vs pull、release 机制、diff/sync 调谐、流水线 Pod 化 |
| [[10-平台工程/06-代码分析/kubernetes-ecosystem/07-registry-dns-loadbalancer.md\|07 仓库/DNS/负载均衡]] | Harbor / CoreDNS / 云 LB / Ingress | 镜像拉取认证链、kubernetes 插件查表、EnsureLoadBalancer、Gateway API |

## 阅读路径

- **按数据面层次（自下而上）**：01 运行时 → 02 网络 → 03 存储 → 07 接入 —— Pod 从创建到可服务依赖的全部底座
- **按控制面模式（机制复用）**：先读 [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|core 06 Informer 机制]]，再看 04/05/06 篇——网格、监控、GitOps 全是同一控制器模式的外部复刻
- **按排障场景**：每篇末尾均有「生产排障速查」表，症状 → 集成点定位 → 检查手段

## 与其他体系的衔接

- 概念与运维层：[[01-集群基础/README.md|01-集群基础]] · [[05-网络/README.md|05-网络]] · [[06-存储/README.md|06-存储]] · [[09-可观测性/README.md|09-可观测性]] · [[14-容器运行时/README.md|14-容器运行时]]
- 生态选型对比：[[21-生态参考/README.md|21-生态参考]]（CNCF 全景、版本矩阵）
- 源码树清单：[[33-源码/README.md|33-源码]]（本地已入库/待补充组件）
