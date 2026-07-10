---
title: Topic Dictionary 知识字典
description: 基于 Kubernetes 官方文档概念与生产环境最佳实践构建的系统性知识库。
summary: 基于 Kubernetes 官方文档概念与生产环境最佳实践构建的系统性知识库。
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kubelet
- scheduler
- istio
- cilium
- argocd
- flux
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Topic Dictionary 知识字典 是什么
- 如何 Topic Dictionary 知识字典
trigger_keywords:
- Topic
- Dictionary
- 知识字典
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- service-mesh-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- gpu-scheduling-basics
- policy-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Topic Dictionary 知识字典

> 基于 [[Kubernetes|Kubernetes]] 官方文档概念与生产环境最佳实践构建的系统性知识库。
> 
> **整理原则**：按照 CNCF 云原生技术栈、SRE 运维成熟度模型、平台工程（[[concepts/platform-engineering-sre.md|Platform Engineering]]）的行业最佳实践进行领域划分，便于检索、学习与持续演进。

---

## 目录结构与分类说明

### 1. `fundamentals/` — 基础概念与架构（24 篇）
**定位**：Kubernetes 的核心概念、对象模型、组件架构与基本操作原理。

**涵盖内容**：
- Kubernetes 核心组件（API Server、Scheduler、Controller Manager、[[kubelet|Kubelet]]、[[etcd|etcd]] 等）
- 对象模型（Pod、Deployment、Service、Namespace、Label、Annotation 等）
- 集群架构（Node、Control Plane、Controller、Lease、Garbage Collection 等）
- 概念参考百科（`kubernetes-concepts-reference.md`）

**适合读者**：初学者建立知识体系，中高级工程师查阅概念定义。

---

### 2. `workloads/` — 工作负载与容器（33 篇）
**定位**：Kubernetes 上运行应用所需的全部工作负载类型与容器运行时技术，以及可抢占工作负载管理。

**涵盖内容**：
- Pod 及其生命周期（Init Containers、Sidecar、Ephemeral Containers、QoS 等）
- 工作负载控制器（Deployment、StatefulSet、DaemonSet、Job、CronJob、ReplicaSet 等）
- 自动伸缩（HPA、VPA、Autoscaling）
- 容器技术（Images、Runtime Class、CRI、Container Lifecycle Hooks、Container Environment）

**适合读者**：应用开发者、DevOps 工程师、平台工程师。

---

### 3. `networking/` — 网络与服务（17 篇）
**定位**：Kubernetes 集群内外的网络通信、服务发现、流量管理与现代网络技术。

**涵盖内容**：
- Service 与 EndpointSlice
- Ingress、Ingress Controller、Gateway API
- 网络策略（Network Policies）
- DNS、IPv4/IPv6 双栈、拓扑感知路由（Topology Aware Routing）
- 集群网络（Cluster Networking）
- eBPF 与 Cilium 网络
- Service Mesh（Istio / Linkerd / Cilium Service Mesh / Ambient Mesh）

**适合读者**：网络工程师、SRE、平台架构师。

---

### 4. `storage/` — 存储与数据（17 篇）
**定位**：为 Pod 提供持久化与临时存储的完整存储体系，以及对象存储与数据流水线。

**涵盖内容**：
- Volumes、Persistent Volumes、Ephemeral Volumes、Projected Volumes
- Storage Classes、Volume Attributes Classes、Dynamic Provisioning
- CSI 快照与克隆（Volume Snapshots、CSI Volume Cloning）
- 存储容量管理、健康监测、Windows Storage
- 对象存储与数据流水线（S3/MinIO、Lakehouse、Argo Workflows）

**适合读者**：存储管理员、SRE、数据平台工程师。

---

### 5. `configuration/` — 配置与密钥（6 篇）
**定位**：应用配置注入、健康探针与资源声明。

**涵盖内容**：
- ConfigMaps、Secrets
- Liveness / Readiness / Startup Probes
- 资源管理（Requests/Limits、Windows 资源管理）
- kubeconfig 组织与访问配置

**适合读者**：应用开发者、DevOps 工程师。

---

### 6. `security/` — 安全与治理（27 篇）
**定位**：集群安全、访问控制、多租户隔离、供应链安全、运行时防护与合规治理。

**涵盖内容**：
- 认证与授权（RBAC、Service Accounts、API Access Control）
- Pod 安全（Pod Security Standards / Admission / Policies）
- 节点安全（Linux / Windows Security、Kernel Constraints）
- 安全加固指南（Authentication Mechanisms、Scheduler Configuration）
- 治理策略（Limit Ranges、Resource Quotas、PID Limiting、Node Resource Managers）
- 安全清单与多租户实践
- 云原生安全最佳实践（`cloud-native-security-practices.md`）
- 软件供应链安全（SBOM / Cosign / SLSA）
- 策略即代码（OPA / Gatekeeper / Kyverno）
- 运行时安全（Falco / KubeArmor / eBPF）

**适合读者**：安全工程师、合规专员、SRE、平台架构师。

> **说明**：Limit Range、Resource Quota 等策略性资源虽然与资源管理相关，但在生产环境中主要承担**多租户隔离与安全防护**职责，故归入安全治理领域。

---

### 7. `scheduling/` — 调度与资源管理（16 篇）
**定位**：Pod 调度机制、节点亲和性、资源分配与驱逐策略。

**涵盖内容**：
- Kubernetes Scheduler 与 Scheduling Framework
- 节点亲和性（Node Affinity）、污点与容忍（Taints & Tolerations）
- Pod 拓扑分布约束（Topology Spread Constraints）、Pod Overhead、Gang Scheduling
- 动态资源分配（DRA）、资源装箱（Bin Packing）、调度器性能调优
- Pod 优先级与抢占（Priority & Preemption）
- 节点压力驱逐（Node-pressure Eviction）、API-initiated Eviction

**适合读者**：平台工程师、SRE、调度优化专家。

---

### 8. `observability/` — 可观测性（10 篇）
**定位**：监控、日志、链路追踪与集群可观测体系建设。

**涵盖内容**：
- 可观测性架构（Metrics、Logs、Traces）
- Kubernetes 系统组件指标（System Metrics）
- 对象状态指标（kube-state-metrics）
- 日志架构（Logging Architecture）、系统日志（System Logs）
- 分布式链路追踪（Traces for System Components）
- OpenTelemetry 与统一可观测性标准

**适合读者**：可观测性工程师、SRE、运维开发工程师。

---

### 9. `operations/` — 运维与 SRE（20 篇）
**定位**：生产环境运维、事故管理、容量规划、变更控制、性能优化与成本治理。

**涵盖内容**：
- 运维最佳实践、企业运维实践、SRE 成熟度模型
- 故障模式分析、事故管理剧本、生产故障排查剧本
- 性能调优专家指南、容量规划与预测
- 变更管理与发布策略、SLI/SLO/SLA 工程实践
- 集群运维（节点关闭、Swap 管理、节点自动伸缩、证书管理、Addons 安装）
- FinOps 与成本优化（OpenCost / Kubecost / Spot 实例）
- GreenOps 与碳感知计算
- Spot 与可抢占工作负载管理

**适合读者**：SRE、运维工程师、平台负责人、技术管理者。

---

### 10. `platform-engineering/` — 平台工程与扩展（19 篇）
**定位**：扩展 Kubernetes 能力、构建内部平台与高级控制平面特性，以及现代交付模式。

**涵盖内容**：
- 扩展 Kubernetes API（Custom Resources、Aggregation Layer）
- Operator 模式、Network Plugins、Device Plugins
- 计算/存储/网络扩展（Compute/Storage/Net Extensions）
- 准入控制（Admission Webhooks）
- API 优先级与公平性（API Priority & Fairness）
- 协调领导者选举（Coordinated Leader Election）、代理机制（Proxies）
- 控制平面兼容性版本、动态资源分配最佳实践
- GitOps 与持续交付（ArgoCD / Flux）
- 集群舰队管理（Cluster API）
- KubeVirt 虚拟机
- WebAssembly 工作负载

**适合读者**：平台工程师、Kubernetes 扩展开发者、架构师。

---

### 11. `specialized-workloads/` — 专业化工作负载（10 篇）
**定位**：特定场景或特殊运行环境下的工作负载管理，尤其是 AI/ML 基础设施。

**涵盖内容**：
- Windows 容器（Windows Containers in Kubernetes、User Guide）
- AI/ML 基础设施专家指南（`ai-infra-specialist.md`）
- GPU 资源管理与分区（MIG、Time-Slicing、DRA、拓扑感知调度）
- KServe 模型服务平台
- Kueue 作业队列与 GPU 准入控制
- LLM 推理优化（vLLM、Continuous Batching、量化、Prefill/Decode 分离）
- 向量数据库与 RAG 基础设施
- MLOps 流水线与模型仓库

**适合读者**：异构基础设施工程师、AI 平台工程师、ML 工程师。

---

### 12. `tooling/` — 工具与生态（5 篇）
**定位**：CLI 操作、工具链与开源生态速查。

**涵盖内容**：
- kubectl CLI 命令大全（`cli-commands.md`）
- 工具与开源项目 URL 汇总（`tool-ecosystem.md`）
- 链接校验脚本（`validate-links.sh` / `validate-links.ps1`）

**适合读者**：所有 Kubernetes 使用者。

---

### 13. `multi-cloud/` — 多云与混合云（2 篇）
**定位**：跨云、混合云环境下的 Kubernetes 运维与管理策略，以及边缘计算。

**涵盖内容**：
- 多云运维实践（`multi-cloud-operations.md`）
- 边缘计算与轻量级 Kubernetes（K3s / MicroK8s / KubeEdge）

**适合读者**：云架构师、跨云运维团队、边缘基础设施工程师。

---

## 统计概览

| 领域 | 文件数量 | 关键词 |
|------|----------|--------|
| fundamentals | 24 | 概念、架构、组件 |
| workloads | 33 | Pod、控制器、容器、伸缩、Spot 工作负载 |
| networking | 17 | Service、Ingress、DNS、网络策略、eBPF、Service Mesh |
| storage | 17 | PV、Volume、CSI、快照、对象存储、数据流水线 |
| configuration | 6 | ConfigMap、Secret、Probe |
| security | 27 | RBAC、Pod Security、供应链安全、运行时安全、零信任身份、配额、治理 |
| scheduling | 16 | 调度器、亲和性、驱逐、资源分配、Karpenter |
| observability | 10 | Metrics、Logs、Traces、OpenTelemetry、Loki |
| operations | 20 | SRE、问题、容量、变更、优化、FinOps、GreenOps |
| platform-engineering | 19 | CRD、Operator、Webhook、GitOps、Cluster API、KubeVirt、Wasm |
| specialized-workloads | 10 | Windows、AI/ML、GPU、KServe、Kueue、LLM 优化、RAG |
| tooling | 5 | CLI、工具链、镜像优化 |
| multi-cloud | 3 | 多云、混合云、边缘计算 |
| **合计** | **209** | — |

---

## 使用建议

1. **按角色检索**：
   - **开发者**：重点查阅 `workloads/`、`configuration/`、`networking/`
   - **SRE/运维**：重点查阅 `operations/`、`observability/`、`scheduling/`、`security/`
   - **平台工程师**：重点查阅 `platform-engineering/`、`security/`、`scheduling/`
   - **架构师**：重点查阅 `fundamentals/`、`multi-cloud/`、`platform-engineering/`
   - **AI/ML 工程师**：重点查阅 `specialized-workloads/`、`scheduling/`、`storage/`

2. **按问题检索**：
   - 某个概念不懂 → `fundamentals/`
   - 应用部署失败 → `workloads/` + `operations/production-troubleshooting-playbook.md`
   - 性能问题 → `operations/performance-tuning-expert.md` + `scheduling/`
   - 安全加固 → `security/` + `operations/certificates.md`
   - 容量告警 → `operations/capacity-planning-forecasting.md` + `scheduling/`
   - 成本优化 → `operations/finops-and-cost-optimization.md` + `workloads/spot-and-preemptible-workloads.md`
   - LLM 推理部署 → `specialized-workloads/kserve-model-serving.md` + `specialized-workloads/llm-inference-optimization.md`
   - GPU 调度问题 → `specialized-workloads/gpu-resource-management-and-partitioning.md` + `scheduling/dynamic-resource-allocation.md`

3. **持续演进**：新增内容应按照上述领域边界归入对应目录；若出现跨领域内容，优先归入**最相关的单一领域**，并在文档中通过链接引用其他领域。

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
