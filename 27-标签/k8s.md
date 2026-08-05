---
title: k8s
description: Kubernetes 核心知识标签枢纽 — 涵盖架构、工作负载、网络、存储、安全、可观测性、平台工程等全部领域的完整知识索引与深度解析
category: tag-index
tags:
- k8s
- kubernetes
- container-orchestration
- cloud-native
tier: core
difficulty: all-levels
domain: cluster-fundamentals
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# k8s Tag Hub

> Kubernetes 核心知识索引 — 涵盖架构、工作负载、网络、存储、安全、可观测性、平台工程等全部领域。

## 核心定义

**Kubernetes**（简称 K8s）是一个开源的容器编排平台，用于自动化容器化应用的部署、扩展和管理。它最初由 Google 设计开发，现由 CNCF（Cloud Native Computing Foundation）维护，是云原生基础设施的核心基石。

### 核心能力矩阵

| 能力维度 | 描述 | 关键组件 |
|---------|------|----------|
| 容器编排 | 自动化部署、调度、扩缩容 | kube-scheduler, kube-controller-manager |
| 服务发现 | DNS/Service/EndpointSlice | CoreDNS, kube-proxy |
| 存储编排 | 动态供给、卷管理 | CSI, PV/PVC, StorageClass |
| 网络管理 | Pod 网络、Service、Ingress | CNI, kube-proxy, Gateway API |
| 安全治理 | RBAC、Pod Security、NetworkPolicy | kube-apiserver, Admission Controllers |
| 可观测性 | 指标、日志、追踪 | Metrics Server, Events API |
| 自愈能力 | 健康检查、自动重启、重调度 | kubelet, liveness/readiness probes |
| 配置管理 | ConfigMap、Secret、Downward API | kube-apiserver |
| 批量执行 | Job、CronJob、并行任务 | job-controller |
| 水平扩展 | HPA、VPA、Cluster Autoscaler | metrics-server, custom-metrics-adapter |

### 设计哲学

1. **声明式 API**：用户描述期望状态，系统自动收敛到目标状态
2. **控制器模式**：Watch → Compare → Act 的无限循环（Reconciliation Loop）
3. **不可变基础设施**：镜像不变，通过替换而非修改实现更新
4. **松耦合组件**：各组件通过 API Server 通信，无直接依赖
5. **可扩展性**：CRD、Operator、Admission Webhook、CSI、CNI 等扩展点

## 架构全景

### 控制平面 (Control Plane)

```
┌─────────────────────────────────────────────────────────┐
│                    Control Plane                         │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ kube-api-   │  │ kube-sched-  │  │ kube-control- │  │
│  │ server      │  │ uler         │  │ ler-manager   │  │
│  └──────┬──────┘  └──────────────┘  └───────────────┘  │
│         │         ┌──────────────┐  ┌───────────────┐  │
│         │         │ cloud-       │  │ etcd          │  │
│         │         │ controller-  │  │ (数据存储)     │  │
│         │         │ manager      │  └───────────────┘  │
│         │         └──────────────┘                      │
└─────────┼───────────────────────────────────────────────┘
          │
┌─────────┼───────────────────────────────────────────────┐
│         ▼          Worker Nodes                         │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ kubelet     │  │ kube-proxy   │  │ Container     │  │
│  │             │  │              │  │ Runtime       │  │
│  └─────────────┘  └──────────────┘  └───────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 核心组件职责

| 组件 | 职责 | 关键配置 |
|------|------|----------|
| kube-apiserver | API 网关，所有操作的唯一入口 | --enable-admission-plugins, --audit-log-path |
| etcd | 分布式 KV 存储，集群状态持久化 | --quota-backend-bytes, --snapshot-count |
| kube-scheduler | Pod 调度决策 | --config (KubeSchedulerConfiguration) |
| kube-controller-manager | 运行核心控制器循环 | --controllers, --concurrent-deployment-syncs |
| kubelet | 节点代理，管理 Pod 生命周期 | --config (KubeletConfiguration) |
| kube-proxy | Service 网络代理 | --proxy-mode (iptables/ipvs/nftables) |
| CoreDNS | 集群 DNS 服务发现 | Corefile 配置 |

## 知识体系结构

### 一级知识域映射

| 知识域 | 目录 | 核心话题 |
|--------|------|----------|
| 集群基础 | 集群基础/ | 架构、控制平面、kubectl、升级、性能调优 |
| 工作负载 | 工作负载/ | Deployment、StatefulSet、DaemonSet、Job、Pod 模式 |
| 网络 | 网络/ | CNI、Service、Ingress、Gateway API、Service Mesh、eBPF |
| 存储 | 存储/ | PV/PVC、CSI、StorageClass、分布式存储、备份 |
| 安全 | 安全/ | RBAC、Pod Security、NetworkPolicy、供应链、合规 |
| 可观测性 | 可观测性/ | Prometheus、Grafana、Loki、Jaeger、SLO/SLI |
| 可靠性 | 可靠性/ | SRE、混沌工程、灾备、容量规划、事后复盘 |
| 故障诊断 | 故障诊断/ | FTA、FEBM、排障流程、工具、技能体系 |
| 平台工程 | 平台工程/ | IDP、GitOps、集群生命周期、治理 |
| 生产运维 | 生产运维/ | 巡检、值班、事件响应、成本治理 |
| 容器运行时 | 容器运行时/ | containerd、CRI-O、Docker、镜像构建 |
| AI 基础设施 | AI基础设施/ | GPU 调度、MLOps、推理服务、Agent |

## 学习路径

### 初级路径（0-6个月）

1. 理解容器基础概念（镜像、容器、namespace、cgroup）
2. 掌握 kubectl 基本操作（get/describe/logs/exec/apply）
3. 理解 Pod、Deployment、Service 核心资源
4. 掌握 ConfigMap、Secret 配置管理
5. 理解 PV/PVC 存储基础
6. 掌握基本的故障排查方法

### 中级路径（6-18个月）

1. 深入理解控制平面组件交互
2. 掌握网络模型（CNI、Service、Ingress）
3. 理解调度器原理与资源管理
4. 掌握 RBAC 与安全最佳实践
5. 理解 Operator 模式与 CRD 开发
6. 掌握生产环境运维（升级、备份、监控）

### 高级路径（18个月+）

1. 大规模集群性能调优（5000+ 节点）
2. 多集群架构与联邦调度
3. 自定义调度器与 Admission Webhook 开发
4. eBPF 网络与安全深度应用
5. 平台工程与 IDP 建设
6. SRE 实践与可靠性工程

## 生产实践要点

### 集群规划基准

| 集群规模 | 节点数 | etcd 配置 | API Server | 调度器 |
|---------|--------|-----------|------------|--------|
| 小型 | < 50 | 单盘 SSD | 1 副本 | 默认 |
| 中型 | 50-500 | SSD + 调优 | 2-3 副本 LB | 调优并发 |
| 大型 | 500-2000 | 高性能 SSD | 3+ 副本 | 多调度器 |
| 超大型 | 2000-5000 | 专用 etcd 集群 | 5+ 副本分片 | 调度器分片 |

### 关键生产指标

- **API Server 延迟**：P99 < 1s（LIST < 5s）
- **Pod 启动时间**：P95 < 30s（含镜像拉取）
- **调度吞吐量**：> 100 pods/s
- **etcd 写入延迟**：P99 < 10ms
- **DNS 解析延迟**：P99 < 5ms
- **节点就绪时间**：< 60s（从启动到 Ready）

### 版本支持策略

| 版本 | 状态 | 关键特性 |
|------|------|----------|
| 1.34 | 最新稳定 | Sidecar Containers GA、Gateway API v1.3 |
| 1.32 | 活跃支持 | AppArmor GA、RecursiveReadOnlyMounts |
| 1.30 | 维护中 | Pod Scheduling Readiness GA |
| 1.28 | 安全修复 | Sidecar Containers Beta |
| < 1.28 | EOL | 建议升级 |

## 集群基础 (Cluster Fundamentals)

- [[01-集群基础/01-架构总览/01-kubernetes-architecture-overview|Kubernetes 架构总览]]
- [[01-集群基础/03-控制平面/15-kubelet-deep-dive|kubelet 深度解析]]
- [[01-集群基础/03-控制平面/36-kubeadm-upgrade-complete-guide|kubeadm 升级完整路径指南]]
- [[01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook|证书 PKI 生命周期 Runbook]]
- [[01-集群基础/07-性能调优/03-cluster-performance-tuning|集群性能调优]]
- [[01-集群基础/02-设计原则/02-production-architecture-design-principles|生产架构设计原则]]
- [[01-集群基础/00-总览/02-kubernetes-production-architecture-blueprint|Kubernetes 生产架构蓝图]]
- [[01-集群基础/05-kubectl/02-kubectl-commands-reference|kubectl 命令完整参考]]

## 工作负载 (Workloads)

- [[02-工作负载/01-核心工作负载/01-workload-overview-architecture|Kubernetes 工作负载架构概览]]
- [[02-工作负载/01-核心工作负载/02-deployment-production-patterns|Deployment 生产模式]]
- [[02-工作负载/01-核心工作负载/12-advanced-pod-patterns|高级 Pod 模式]]
- [[02-工作负载/01-核心工作负载/15-container-runtime-interfaces|容器运行时接口]]
- [[02-工作负载/00-总览/01-kubernetes-deployment-patterns-architecture|部署模式架构]]
- [[02-工作负载/02-Java-on-K8s/03-java-operator-sdk-development|Java Operator SDK 开发]]

## 网络 (Networking)

- [[05-网络/01-K8s网络核心/02-network-architecture-overview|网络架构概览]]
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals|CNI 架构基础]]
- [[05-网络/01-K8s网络核心/07-service-concepts-types|Service 概念与类型]]
- [[05-网络/01-K8s网络核心/12-dns-service-discovery-coredns|DNS 服务发现 CoreDNS]]
- [[05-网络/01-K8s网络核心/17-networkpolicy-deep-practice|NetworkPolicy 深度实践]]
- [[05-网络/01-K8s网络核心/20-ingress-fundamentals|Ingress 基础]]
- [[05-网络/01-K8s网络核心/32-service-mesh-deep-dive|Service Mesh 深度指南]]
- [[05-网络/01-K8s网络核心/37-gateway-api-overview|Gateway API 概览]]

## 存储 (Storage)

- [[06-存储/01-K8s存储/01-storage-architecture-overview|存储架构概览]]
- [[06-存储/01-K8s存储/04-pvc-patterns-practices|PVC 模式与实践]]
- [[06-存储/01-K8s存储/06-csi-drivers-integration|CSI 驱动集成]]
- [[06-存储/01-K8s存储/10-pv-pvc-troubleshooting|PV/PVC 故障排查]]
- [[06-存储/01-K8s存储/11-storage-backup-disaster-recovery|存储备份与灾备]]

## 安全 (Security)

- [[08-安全/01-身份与访问/01-authentication-authorization-system|认证授权体系]]
- [[08-安全/04-策略治理/01-kyverno-enterprise-policy-management|Kyverno 企业级策略管理]]
- [[08-安全/05-供应链/01-supply-chain-security-overview|供应链安全概览]]
- [[08-安全/03-运行时安全/01-falco-cloud-native-security|Falco 云原生安全]]
- [[08-安全/06-合规审计/08-kubernetes-security-hardening|Kubernetes 安全加固]]
- [[08-安全/00-总览/01-production-readiness-operations-guide|安全生产就绪指南]]

## 可观测性 (Observability)

- [[09-可观测性/01-总览/01-observability-architecture-overview|可观测性架构概览]]
- [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring|Prometheus 企业级监控]]
- [[09-可观测性/03-日志/04-loki-enterprise-log-aggregation|Loki 企业级日志聚合]]
- [[09-可观测性/04-链路追踪/05-distributed-tracing|分布式追踪]]
- [[09-可观测性/06-SLO-SLI/01-slo-engineering-practice|SLO 工程实践]]
- [[09-可观测性/06-SLO-SLI/09-slo-operations-guide|SLO 运营指南]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/01-构建/01-platform-engineering-overview|平台工程概览]]
- [[10-平台工程/01-构建/03-backstage-deployment|Backstage 部署]]
- [[10-平台工程/02-运维/02-cluster-lifecycle-management|集群生命周期管理]]
- [[10-平台工程/02-运维/10-multi-cluster-management|多集群管理]]
- [[10-平台工程/02-运维/12-production-troubleshooting|生产环境故障排查]]
- [[10-平台工程/02-运维/17-karpenter-node-autoscaling-guide|Karpenter 节点弹性伸缩指南]]

## 生产运维 (Production Operations)

- [[13-生产运维/07-运维手册/01-production-sre-daily-ops|生产环境日常巡检]]
- [[13-生产运维/03-事件响应/04-on-call-playbook|值班手册与告警响应]]
- [[13-生产运维/03-事件响应/05-incident-response-template|事故响应模板]]
- [[13-生产运维/07-运维手册/08-security-operations-runbook|安全运营 Runbook]]
- [[13-生产运维/01-成本治理/05-kubernetes-cost-governance|Kubernetes 成本治理]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/02-资源排障/01-node-comprehensive-troubleshooting|节点综合排障]]
- [[19-故障诊断/02-资源排障/02-service-comprehensive-troubleshooting|Service 综合排障]]
- [[19-故障诊断/02-资源排障/06-pvc-storage-troubleshooting|PVC 存储排障]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pod-crashloop/SKILL-DEEP-DIVE|Pod CrashLoopBackOff 深度解析]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE|K8s Node NotReady 深度解析]]

## 清单模式 (Manifest Patterns)

- [[03-清单模式/01-YAML参考/01-yaml-syntax-resource-conventions|YAML 语法与资源规范]]
- [[03-清单模式/01-YAML参考/03-pod-specification-complete|Pod 完整规格]]
- [[03-清单模式/01-YAML参考/08-service-all-types|Service 全类型配置]]
- [[03-清单模式/06-安全模式/01-pod-security-standards-reference|Pod 安全标准参考]]
- [[03-清单模式/08-韧性模式/01-pdb-patterns|PDB 模式]]

## 容器运行时 (Container Runtime)

- [[14-容器运行时/03-containerd-CRI-O/01-containerd-deep-guide|containerd 深度指南]]
- [[14-容器运行时/01-Docker/01-docker-architecture-overview|Docker 架构概述]]
- [[14-容器运行时/02-镜像管理/01-harbor-enterprise-image-registry|Harbor 企业级镜像仓库]]

## 数据库中间件 (Database Middleware)

- [[07-数据库中间件/00-总览/01-database-on-kubernetes-guide|数据库在 K8s 上的运行指南]]
- [[07-数据库中间件/05-Operator管理/01-database-operator-patterns|数据库 Operator 模式]]

## 生态参考 (Ecosystem References)

- [[21-生态参考/README|Landscape & References]]
- [[21-生态参考/02-论文/01-kubernetes-production-readiness-assessment|生产就绪性评估框架]]
- [[21-生态参考/02-论文/02-kubernetes-large-scale-performance-optimization|大规模集群性能优化]]

## 实体 (Entities)

- [[23-实体/02-K8s核心组件/kubernetes|Kubernetes]]
- [[23-实体/15-参考与索引/k8s-architecture-domain-guide|Kubernetes Architecture Domain Guide]]
- [[23-实体/15-参考与索引/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]]
- [[23-实体/15-参考与索引/k8s-workloads-domain-guide|Kubernetes Workloads Domain Guide]]
- [[23-实体/15-参考与索引/k8s-storage-ecosystem|Kubernetes Storage Ecosystem]]
- [[23-实体/15-参考与索引/k8s-security-compliance|Kubernetes Security Compliance]]
- [[23-实体/15-参考与索引/k8s-production-operations|Kubernetes Production Operations]]

## 故障排查常用命令

```bash
# 集群状态总览
kubectl get nodes -o wide
kubectl get componentstatuses
kubectl cluster-info dump

# Pod 排障
kubectl get pods -A --field-selector=status.phase!=Running
kubectl describe pod <pod-name> -n <namespace>
kubectl logs <pod-name> -c <container> --previous
kubectl exec -it <pod-name> -- /bin/sh

# 网络排障
kubectl get svc,ep,endpointslice -A
kubectl get networkpolicy -A
kubectl run debug --rm -it --image=nicolaka/netshoot -- bash

# 资源分析
kubectl top nodes
kubectl top pods -A --sort-by=memory
kubectl get events -A --sort-by='.lastTimestamp' | tail -20

# API Server 健康
kubectl get --raw /healthz?verbose
kubectl get --raw /metrics | grep apiserver_request_duration
```

## 工具生态

| 类别 | 工具 | 用途 |
|------|------|------|
| CLI | kubectl, kubectx, kubens, k9s | 集群操作与导航 |
| 部署 | Helm, Kustomize, ArgoCD, Flux | 应用部署与 GitOps |
| 监控 | Prometheus, Grafana, Thanos | 指标采集与可视化 |
| 日志 | Loki, Fluentd, Fluent Bit | 日志聚合与处理 |
| 追踪 | Jaeger, Tempo, OpenTelemetry | 分布式链路追踪 |
| 安全 | Falco, Trivy, Kyverno, OPA | 运行时安全与策略 |
| 网络 | Cilium, Calico, Istio, Linkerd | CNI 与 Service Mesh |
| 存储 | Rook, Longhorn, OpenEBS | 分布式存储 |
| 调试 | kubectl-debug, netshoot, ksniff | 故障诊断工具 |
| 负载测试 | k6, Locust, wrk2 | 性能与压力测试 |

## 常见问题与反模式

| 反模式 | 问题 | 正确做法 |
|---------|------|----------|
| 使用 latest 标签 | 无法回滚、不可重现 | 使用语义化版本或 SHA |
| 未设置资源限制 | 节点资源耗尽、OOM | 必须设置 requests/limits |
| 单副本生产部署 | 无高可用保障 | 生产至少 2-3 副本 + PDB |
| 忽略健康检查 | 流量打到不健康 Pod | 配置 liveness + readiness + startup |
| 直接在 Pod 中存储数据 | Pod 重启数据丢失 | 使用 PVC 或外部存储 |
| 过度使用 hostPath | 安全风险、节点亲和 | 使用 CSI 驱动 |
| 忽略 Namespace 隔离 | 资源争抢、安全边界模糊 | 按团队/环境划分 Namespace |
| 手动 kubectl edit | 变更不可追踪 | 使用 GitOps 声明式管理 |

## 参考资源

- [Kubernetes 官方文档](https://kubernetes.io/docs/)
- [Kubernetes GitHub](https://github.com/kubernetes/kubernetes)
- [CNCF Landscape](https://landscape.cncf.io/)
- [Kubernetes Enhancement Proposals (KEPs)](https://github.com/kubernetes/enhancements)
- [Kubernetes Release Notes](https://kubernetes.io/releases/)

## Related Tags

- [[27-标签/networking|networking — 网络技术]]
- [[27-标签/security|security — 安全治理]]
- [[27-标签/storage|storage — 存储体系]]
- [[27-标签/observability|observability — 可观测性]]
- [[27-标签/reliability|reliability — 可靠性工程]]
- [[27-标签/production|production — 生产运营]]
- [[27-标签/best-practices|best-practices — 最佳实践]]
- [[27-标签/gitops|gitops — GitOps 交付]]
- [[27-标签/helm|helm — 包管理]]
- [[27-标签/operator|operator — Operator 模式]]
- [[27-标签/containerd|containerd — 容器运行时]]
- [[27-标签/multi-cluster|multi-cluster — 多集群]]
- [[27-标签/gpu|gpu — GPU 调度]]
- [[27-标签/ai-ml-infra|ai-ml-infra — AI/ML 基础设施]]
- [[27-标签/troubleshooting|troubleshooting — 故障诊断]]
- [[27-标签/platform-engineering|platform-engineering — 平台工程]]
- [[27-标签/sre|sre — 站点可靠性工程]]
