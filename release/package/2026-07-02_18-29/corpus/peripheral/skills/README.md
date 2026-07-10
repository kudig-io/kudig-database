---
title: FTA 故障树清单索引 (skills)
description: '## 概述'
summary: '本目录包含 [[Kubernetes|Kubernetes]] 生产环境各组件的故障树分析（FTA）文档。每个 FTA 文件提供：'
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- FTA 故障树清单索引 是什么
- 如何 FTA 故障树清单索引
trigger_keywords:
- FTA
- 故障树清单索引
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
- gpu-scheduling-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# FTA 故障树清单索引

# FTA 故障树清单索引

> **文档数量**: 36 个故障树 | **总大小**: ~1.2 MB | **最后更新**: 2026-03-02

---

## 概述

本目录包含 [[Kubernetes|Kubernetes]] 生产环境各组件的故障树分析（FTA）文档。每个 FTA 文件提供：
- 完整的 Mermaid 故障树图（OR/AND 门结构）
- 底事件详细定义（severity/probability/MTTR/detection/remediation）
- JSON 工作流（支持 Agent 自动化遍历）
- K8s 版本兼容说明（1.19–1.30）

---

## 文件大小分布

| 分类 | 文件数 | 大小范围 |
|:---|:---:|:---|
| 大型 (>40 KB) | 8 | 44.0 KB – 58.8 KB |
| 中型 (25–40 KB) | 15 | 25.9 KB – 38.9 KB |
| 标准 (20–25 KB) | 9 | 20.3 KB – 24.9 KB |
| 紧凑 (<20 KB) | 4 | 14.8 KB – 18.9 KB |

---

## 按领域分类索引

### 1. 核心工作负载

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [pod-fta.md](pod-fta.md) | 58.8 KB | Pod 全生命周期异常（调度/镜像/运行时/健康检查/网络/存储/安全/节点/控制面） | ~80 |
| [deployment-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/deployment-fta.md|deployment-fta]].md) | 21.4 KB | Deployment 滚动更新/副本管理/选择器/镜像拉取 | ~25 |
| [statefulset-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/statefulset-fta.md|statefulset-fta]].md) | 20.8 KB | [[StatefulSet|StatefulSet]] 有序部署/持久卷/网络标识/扩缩容 | ~24 |
| [daemonset-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/daemonset-fta.md|daemonset-fta]].md) | 29.9 KB | [[DaemonSet|DaemonSet]] 节点调度/污点容忍/滚动更新/资源竞争 | ~35 |
| [job-cronjob-fta.md](job-cronjob-fta.md) | 28.8 KB | Job/CronJob 调度/并发/完成策略/超时/时区 | ~32 |

### 2. 网络与流量

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [dns-fta.md](dns-fta.md) | 24.2 KB | CoreDNS/集群 DNS/外部 DNS 解析/缓存/NXDOMAIN | ~28 |
| [service-fta.md](service-fta.md) | 25.9 KB | Service 类型/Endpoints/kube-proxy/负载均衡/会话亲和 | ~30 |
| [ingress-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/ingress-fta.md|ingress-fta]].md) | 26.3 KB | Ingress Controller/TLS 终止/路由/后端健康/注解 | ~30 |
| [networkpolicy-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/networkpolicy-fta.md|networkpolicy-fta]].md) | 21.7 KB | NetworkPolicy 入站/出站/选择器/CNI 支持/调试 | ~25 |
| [gateway-api-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/gateway-api-fta.md|gateway-api-fta]].md) | 24.1 KB | Gateway API/HTTPRoute/GRPCRoute/TLSRoute/ReferenceGrant | ~28 |
| [terway-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/terway-fta.md|terway-fta]].md) | 16.8 KB | Terway ENI/IP 池/VPC 路由/安全组/控制面依赖 | ~20 |

### 3. 控制面组件

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [apiserver-fta.md](apiserver-fta.md) | 36.1 KB | API Server 认证/授权/准入/etcd 连接/限流/审计 | ~42 |
| [scheduler-fta.md](scheduler-fta.md) | 30.3 KB | Scheduler 过滤/打分/抢占/亲和性/资源/扩展点 | ~35 |
| [controller-manager-fta.md]([[domain-10-troubleshooting-diagnostics/FTA故障树/list/controller-manager-fta.md|controller-manager-fta]].md) | 29.4 KB | Controller Manager Leader 选举/控制器/同步/限速 | ~34 |
| [etcd-fta.md](etcd-fta.md) | 27.4 KB | etcd 集群/Raft/存储/快照/认证/性能 | ~32 |

### 4. 存储

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [csi-fta.md](csi-fta.md) | 18.9 KB | CSI Controller/Node Plugin/卷挂载/性能/认证/后端 | ~22 |

### 5. 安全与准入

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [rbac-fta.md](rbac-fta.md) | 24.2 KB | RBAC Role/ClusterRole/Binding/ServiceAccount/权限不足 | ~28 |
| [certificate-fta.md](certificate-fta.md) | 52.6 KB | 证书签发/轮换/过期/CA 链/cert-manager/TLS | ~60 |
| [webhook-admission-fta.md](webhook-admission-fta.md) | 50.5 KB | Webhook 超时/TLS/失败策略/副作用/匹配规则 | ~58 |
| [psp-scc-fta.md](psp-scc-fta.md) | 44.0 KB | PSP/SCC/PSA 策略迁移/安全上下文/特权容器 | ~50 |
| [resource-quota-fta.md](resource-quota-fta.md) | 38.9 KB | ResourceQuota/LimitRange/配额计算/命名空间限制 | ~45 |

### 6. 节点与基础设施

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [node-fta.md](node-fta.md) | 27.4 KB | 节点状态/kubelet/容器运行时/磁盘/内存/网络 | ~32 |
| [nodepool-fta.md](nodepool-fta.md) |

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[webhook-admission-fta]] — Admission Webhook 异常 FTA 树
- [[service-fta]] — Service 异常故障树分析
- [[resource-quota-fta]] — ResourceQuota 异常故障树分析
- [[psp-scc-fta]] — PSP/SCC 异常故障树分析
- [[nodepool-fta]] — NodePool 异常故障树分析

- [[domain-17-system-foundation/README.md|Domain-33: Kubernetes Events 全域事件大全]]
- [[domain-02-workloads-applications/README.md|Java on Kubernetes 综合实践指南]]
- [[domain-13-container-runtime/README.md|Docker 容器技术深度解析]]
- [[domain-05-security-compliance/README.md|Domain 05: 供应链安全 (Supply Chain Security)]]
- [[domain-08-release-change-management/README.md|Domain 08: 基础设施即代码 (Infrastructure as Code)]]
- [[domain-03-networking-traffic/README.md|Domain 03: 企业级服务网格与微服务治理 (Enterprise Service Mesh & Microservices Governance)]]
- [[domain-08-release-change-management/README.md|Domain 08: GitOps与CI/CD (GitOps & CI/CD)]]
- [[domain-19-landscape-references/README.md|Domain 19: Kubernetes 高级技术论文与最佳实践 (Advanced Technical Papers and Best Practices)]]
- [[domain-03-networking-traffic/README.md|Domain-15: 网络基础]]
- [[domain-18-manifests-patterns/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- [[domain-01-cluster-fundamentals/README.md|Domain-3: Kubernetes控制平面]]
- [[domain-07-platform-engineering/README.md|Domain 07: 平台工程 (Platform Engineering)]]
- [[domain-06-observability/README.md|Observability Domain (可观测性领域)]]
- [[domain-12-cloud-providers/README.md|Domain-17: 云厂商Kubernetes服务企业级深度指南]]
- [[domain-19-landscape-references/README.md|Domain-34: CNCF Landscape 开源项目]]
- [[domain-07-platform-engineering/README.md|Platform Ops Domain (平台运维领域)]]
- [[domain-14-ai-ml-infra/README.md|AI Agent 工程专题]]
- [[domain-11-production-operations/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- README-old
- [[domain-02-workloads-applications/README.md|Domain-4: Kubernetes工作负载管理]]
- [[domain-06-observability/README.md|Domain 06: 日志管理与分析 (Logging Management & Analytics)]]
- [[domain-06-observability/README.md|Domain 06: 企业级监控与告警 (Enterprise Monitoring & Alerting)]]
- [[domain-17-system-foundation/README.md|Domain-14: Linux 基础知识体系]]
- [[domain-01-cluster-fundamentals/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- [[domain-15-specialized-tech/README.md|Domain 15: 边缘计算 (Edge Computing)]]
- [[domain-20-application-patterns/README.md|Topic 应用层架构设计最佳实践]]
- [[domain-04-storage-data/README.md|Storage Domain 存储领域知识库]]
- [[domain-03-networking-traffic/README.md|Domain 03: Networking 网络]]
- [[domain-04-storage-data/README.md|Domain-16: 存储基础]]
- [[domain-09-reliability-engineering/README.md|Domain 09: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity)]]
- [[domain-16-database-middleware/README.md|Domain 16: 企业级数据库与中间件运维 (Enterprise Database & Middleware Operations)]]
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- [[domain-05-security-compliance/README.md|Security Domain]]
- [[domain-15-specialized-tech/README.md|Domain-10: Kubernetes 扩展生态]]
- [[domain-05-security-compliance/README.md|Domain 05: 云原生安全 (Cloud Native Security)]]
- [[domain-14-ai-ml-infra/README.md|Domain-11: AI基础设施]]
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- [[domain-12-cloud-providers/README.md|Domain 12: 多云与混合云架构管理]]
- [[domain-13-container-runtime/README.md|Domain 13: 容器镜像管理 (Container Image Management)]]
- [[domain-03-networking-traffic/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technology Stack)]]
- [[domain-03-networking-traffic/README.md|Domain 03: eBPF 技术体系 (eBPF Technology Stack)]]
- [[domain-15-specialized-tech/README.md|Domain 15: WebAssembly 云原生 (WebAssembly Cloud Native)]]
- [[domain-08-release-change-management/README.md|Domain 08: 自动化测试与质量保障 (Automated Testing & Quality Assurance)]]
- [[domain-17-system-foundation/README.md|Domain 31 - 硬件基础设施]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/README.md|topic-febm: FEBM 法医鉴定循证方法论深度解析]]
- [[domain-10-troubleshooting-diagnostics/tools/README.md|Domain-12 故障排查工具套件使用说明]]
- [[domain-10-troubleshooting-diagnostics/高级排障/README.md|Kubernetes 结构化故障排查知识库]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/README.md|FTA 故障树清单索引]]
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
