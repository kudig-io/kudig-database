---
title: K8s 术语表索引
description: '| 配置管理 | 6 | [[23-实体/15-参考与索引/configuration-terms.md|configuration-terms]]
  |'
summary: '| 配置管理 | 6 | [[23-实体/15-参考与索引/configuration-terms.md|configuration-terms]] |'
category: references
tags:
- k8s
- dictionary
- glossary
- index
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 术语表索引 是什么
- 如何 K8s 术语表索引
trigger_keywords:
- K8s
- 术语表索引
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 术语表索引（Glossary Index）

> 本页为 KUDIG 术语表主索引，汇总了 13 个领域共 205+ 个 Kubernetes 核心术语。

---

## 术语分类目录

| 领域 | 术语数量 | 参考页面 |
|------|----------|----------|
| 配置管理 | 6 | [[23-实体/15-参考与索引/configuration-terms.md|configuration-terms]] |
| 基础概念 | 24 | [[23-实体/15-参考与索引/fundamentals-terms.md|fundamentals-terms]] |
| 多云架构 | 3 | [[23-实体/15-参考与索引/multi-cloud-terms.md|multi-cloud-terms]] |
| 网络 | 17 | [[23-实体/15-参考与索引/networking-terms.md|networking-terms]] |
| 可观测性 | 10 | [[23-实体/15-参考与索引/observability-terms.md|observability-terms]] |
| 运维运营 | 20 | [[23-实体/15-参考与索引/operations-terms.md|operations-terms]] |
| 平台工程 | 19 | [[23-实体/15-参考与索引/platform-engineering-terms.md|platform-engineering-terms]] |
| 调度 | 16 | [[23-实体/15-参考与索引/scheduling-terms.md|scheduling-terms]] |
| 安全 | 27 | [[security-terms]] |
| 专用工作负载 | 10 | [[23-实体/15-参考与索引/specialized-workloads-terms.md|specialized-workloads-terms]] |
| 存储 | 17 | [[23-实体/15-参考与索引/storage-terms.md|storage-terms]] |
| 工具链 | 3 | [[23-实体/15-参考与索引/tooling-terms.md|tooling-terms]] |
| 工作负载 | 33 | [[23-实体/15-参考与索引/workloads-terms.md|workloads-terms]] |
| **合计** | **205** | |

---

## 全部术语页面

- [[23-实体/15-参考与索引/configuration-terms.md|配置管理术语参考]] (6 个词条)
- [[23-实体/15-参考与索引/fundamentals-terms.md|基础概念术语参考]] (24 个词条)
- [[23-实体/15-参考与索引/multi-cloud-terms.md|多云架构术语参考]] (3 个词条)
- [[23-实体/15-参考与索引/networking-terms.md|网络术语参考]] (17 个词条)
- [[23-实体/15-参考与索引/observability-terms.md|可观测性术语参考]] (10 个词条)
- [[23-实体/15-参考与索引/operations-terms.md|运维运营术语参考]] (20 个词条)
- [[23-实体/15-参考与索引/platform-engineering-terms.md|平台工程术语参考]] (19 个词条)
- [[23-实体/15-参考与索引/scheduling-terms.md|调度术语参考]] (16 个词条)
- [[security-terms|安全术语参考]] (27 个词条)
- [[23-实体/15-参考与索引/specialized-workloads-terms.md|专用工作负载术语参考]] (10 个词条)
- [[23-实体/15-参考与索引/storage-terms.md|存储术语参考]] (17 个词条)
- [[23-实体/15-参考与索引/tooling-terms.md|工具链术语参考]] (3 个词条)
- [[23-实体/15-参考与索引/workloads-terms.md|工作负载术语参考]] (33 个词条)

## 相关资源

- [[23-实体/15-参考与索引/k8s-knowledge-map.md|k8s-knowledge-map]] - 知识图谱总览
- [[23-实体/15-参考与索引/k8s-architecture-fundamentals.md|k8s-architecture-fundamentals]] - 架构基础
- [[23-实体/15-参考与索引/k8s-workloads-domain-guide.md|k8s-workloads-domain-guide]] - 工作负载指南
- [[23-实体/15-参考与索引/k8s-networking-domain-guide.md|k8s-networking-domain-guide]] - 网络指南
- [[23-实体/15-参考与索引/k8s-security-compliance.md|k8s-security-compliance]] - 安全合规
- [[23-实体/15-参考与索引/k8s-observability-ecosystem.md|k8s-observability-ecosystem]] - 可观测性生态
- [[23-实体/15-参考与索引/k8s-storage-ecosystem.md|k8s-storage-ecosystem]] - 存储生态
- [[23-实体/15-参考与索引/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]] - 标签字典

## 高频术语速查

### 核心架构术语

| 术语 | 英文 | 定义 | 关联页面 |
|------|------|------|----------|
| 控制平面 | Control Plane | K8s 集群的管理层，包含 API Server、etcd、Scheduler、Controller Manager | [[23-实体/15-参考与索引/fundamentals-terms.md]] |
| 数据平面 | Data Plane | 运行工作负载的节点层，包含 kubelet、kube-proxy、容器运行时 | [[23-实体/15-参考与索引/fundamentals-terms.md]] |
| API Server | kube-apiserver | 集群前端代理，所有操作的唯一入口 | [[23-实体/02-K8s核心组件/kube-apiserver.md]] |
| etcd | etcd | 分布式 KV 存储，保存集群所有状态 | [[23-实体/02-K8s核心组件/etcd.md]] |
| 调度器 | kube-scheduler | 负责将 Pod 分配到合适节点 | [[23-实体/15-参考与索引/scheduling-terms.md]] |

### 工作负载术语

| 术语 | 英文 | 定义 | 关联页面 |
|------|------|------|----------|
| Pod | Pod | K8s 最小调度单元，包含一个或多个容器 | [[23-实体/15-参考与索引/workloads-terms.md]] |
| Deployment | Deployment | 无状态应用管理器，支持滚动更新和回滚 | [[23-实体/15-参考与索引/workloads-terms.md]] |
| StatefulSet | StatefulSet | 有状态应用管理器，提供稳定网络标识和存储 | [[23-实体/15-参考与索引/workloads-terms.md]] |
| DaemonSet | DaemonSet | 确保每个节点运行一个 Pod 副本 | [[23-实体/15-参考与索引/workloads-terms.md]] |
| Job/CronJob | Job/CronJob | 一次性/定时任务执行器 | [[23-实体/15-参考与索引/workloads-terms.md]] |
| HPA | Horizontal Pod Autoscaler | 基于指标自动调整 Pod 副本数 | [[23-实体/15-参考与索引/scheduling-terms.md]] |

### 网络术语

| 术语 | 英文 | 定义 | 关联页面 |
|------|------|------|----------|
| Service | Service | 稳定的网络端点，负载均衡到后端 Pod | [[23-实体/15-参考与索引/networking-terms.md]] |
| Ingress | Ingress | HTTP/HTTPS 路由规则，外部流量入口 | [[23-实体/15-参考与索引/networking-terms.md]] |
| CNI | Container Network Interface | 容器网络插件标准 | [[23-实体/15-参考与索引/networking-terms.md]] |
| NetworkPolicy | NetworkPolicy | Pod 级别的网络访问控制策略 | [[23-实体/02-K8s核心组件/networkpolicy.md]] |
| EndpointSlice | EndpointSlice | Service 后端端点的分片表示 | [[23-实体/15-参考与索引/networking-terms.md]] |
| Gateway API | Gateway API | 下一代流量管理 API，替代 Ingress | [[23-实体/15-参考与索引/networking-terms.md]] |

### 存储术语

| 术语 | 英文 | 定义 | 关联页面 |
|------|------|------|----------|
| PV | PersistentVolume | 集群级别的存储资源 | [[23-实体/15-参考与索引/storage-terms.md]] |
| PVC | PersistentVolumeClaim | 用户对存储资源的申请 | [[23-实体/15-参考与索引/storage-terms.md]] |
| StorageClass | StorageClass | 定义存储类型和动态供给参数 | [[23-实体/15-参考与索引/storage-terms.md]] |
| CSI | Container Storage Interface | 容器存储插件标准 | [[23-实体/15-参考与索引/storage-terms.md]] |
| VolumeSnapshot | VolumeSnapshot | 存储卷的时间点快照 | [[23-实体/15-参考与索引/storage-terms.md]] |

### 安全术语

| 术语 | 英文 | 定义 | 关联页面 |
|------|------|------|----------|
| RBAC | Role-Based Access Control | 基于角色的访问控制 | [[security-terms]] |
| Pod Security | Pod Security Standards | Pod 安全级别约束（Privileged/Baseline/Restricted） | [[security-terms]] |
| ServiceAccount | ServiceAccount | Pod 内进程的身份标识 | [[security-terms]] |
| NetworkPolicy | NetworkPolicy | 网络层微分段策略 | [[security-terms]] |
| Seccomp | Secure Computing | 系统调用过滤机制 | [[security-terms]] |
| mTLS | Mutual TLS | 双向 TLS 认证，服务间零信任 | [[security-terms]] |

### 可观测性术语

| 术语 | 英文 | 定义 | 关联页面 |
|------|------|------|----------|
| Metrics | Metrics | 时序数值指标（Prometheus 格式） | [[23-实体/15-参考与索引/observability-terms.md]] |
| Tracing | Distributed Tracing | 分布式调用链追踪 | [[23-实体/15-参考与索引/observability-terms.md]] |
| Logging | Logging | 结构化日志收集和分析 | [[23-实体/15-参考与索引/observability-terms.md]] |
| SLO | Service Level Objective | 服务级别目标（可用性/延迟） | [[23-实体/15-参考与索引/observability-terms.md]] |
| OpenTelemetry | OTel | CNCF 统一可观测性数据采集框架 | [[23-实体/15-参考与索引/observability-terms.md]] |

## 术语学习路径

### 初级工程师（0-1年）

1. 先掌握 [[23-实体/15-参考与索引/fundamentals-terms.md|基础概念术语]]（24个）
2. 再学习 [[23-实体/15-参考与索引/workloads-terms.md|工作负载术语]]（33个）
3. 然后理解 [[23-实体/15-参考与索引/networking-terms.md|网络术语]]（17个）
4. 最后了解 [[23-实体/15-参考与索引/storage-terms.md|存储术语]]（17个）

### 中级工程师（1-3年）

1. 深入 [[23-实体/15-参考与索引/scheduling-terms.md|调度术语]]（16个）
2. 掌握 [[security-terms|安全术语]]（27个）
3. 学习 [[23-实体/15-参考与索引/observability-terms.md|可观测性术语]]（10个）
4. 理解 [[23-实体/15-参考与索引/operations-terms.md|运维运营术语]]（20个）

### 高级工程师/架构师（3年+）

1. 精通 [[23-实体/15-参考与索引/platform-engineering-terms.md|平台工程术语]]（19个）
2. 研究 [[23-实体/15-参考与索引/multi-cloud-terms.md|多云架构术语]]（3个）
3. 探索 [[23-实体/15-参考与索引/specialized-workloads-terms.md|专用工作负载术语]]（10个）
4. 整合 [[23-实体/15-参考与索引/configuration-terms.md|配置管理]] + [[23-实体/15-参考与索引/tooling-terms.md|工具链]]

## 术语使用规范

### 文档中术语引用格式

```markdown
<!-- 首次出现时给出完整定义 -->
水平Pod自动扩缩（Horizontal Pod Autoscaler, HPA）是...

<!-- 后续使用缩写 -->
HPA 根据 CPU 利用率自动调整副本数...

<!-- 链接到术语表 -->
参见 [[23-实体/15-参考与索引/scheduling-terms.md#hpa|HPA 术语定义]]
```

### 常见术语误用

| 误用 | 正确用法 | 说明 |
|------|----------|------|
| “容器”指代 Pod | Pod 包含一个或多个容器 | Pod 是调度单元，容器是运行单元 |
| “重启 Pod” | 删除 Pod 由控制器重建 | Pod 本身不支持 restart |
| “Service IP 固定” | ClusterIP 在 Service 生命周期内固定 | 删除重建后 IP 会变 |
| “ConfigMap 加密” | ConfigMap 明文，Secret 才加密 | 敏感数据必须用 Secret |
| “DaemonSet 每节点一个” | 受 nodeSelector/taint 影响 | 不是所有节点都会运行 |

## 术语更新日志

| 版本 | 日期 | 变更内容 |
|------|------|----------|
| v2.1 | 2026-07 | 新增 Gateway API、CEL、Sidecar Containers 术语 |
| v2.0 | 2026-05 | 重构为 13 个领域分类，总计 205+ 术语 |
| v1.0 | 2026-03 | 初始版本，基础术语 80+ |

## Related

- [[23-实体/15-参考与索引/kudig-quality-indexes.md|kudig-quality-indexes]] — KUDIG 质量评估与索引体系
- [[23-实体/15-参考与索引/root-terms.md|root-terms]] — K8s Root术语参考
- [[INDEX]] — Wiki Index
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
