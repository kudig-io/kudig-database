# Domain-1: Kubernetes架构基础

> **文档数量**: 18 篇 | **最后更新**: 2026-02 | **适用版本**: Kubernetes 1.20+

---

## 概述

Kubernetes架构基础域深入解析K8s核心架构设计原理，涵盖控制平面、数据平面、核心组件工作机制等内容。帮助读者建立扎实的K8s理论基础。

**核心价值**：
- 🏗️ **架构理解**：深入理解K8s核心架构设计思想
- 🔧 **组件剖析**：掌握各核心组件工作原理和交互机制  
- 📊 **数据流向**：清晰的数据流转和控制流分析
- 🎯 **设计原则**：学习K8s架构设计的最佳实践

---

## 文档目录

### 架构概览 (01-03)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 01 | [K8s架构全景图](./01-k8s-architecture-overview.md) | 整体架构、控制平面vs数据平面、核心组件关系 | ⭐⭐⭐⭐⭐ |
| 02 | [控制平面详解](./02-control-plane-deep-dive.md) | API Server、etcd、Scheduler、Controller Manager架构 | ⭐⭐⭐⭐⭐ |
| 03 | [数据平面剖析](./03-data-plane-analysis.md) | kubelet、container runtime、CNI、CSI工作机制 | ⭐⭐⭐⭐ |

### 核心组件 (04-08)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 04 | [API Server架构](./04-api-server-architecture.md) | REST API、认证授权、准入控制、存储接口 | ⭐⭐⭐⭐⭐ |
| 05 | [etcd存储机制](./05-etcd-storage-mechanism.md) | 分布式存储、数据一致性、性能调优、备份恢复 | ⭐⭐⭐⭐ |
| 06 | [调度器原理](./06-scheduler-principles.md) | 调度算法、亲和性、污点容忍、优先级抢占 | ⭐⭐⭐⭐ |
| 07 | [控制器模式](./07-controller-pattern.md) | 控制循环、声明式API、自愈机制、Operator模式 | ⭐⭐⭐⭐⭐ |
| 08 | [kubelet核心功能](./08-kubelet-core-functions.md) | Pod生命周期、容器管理、节点状态、卷挂载 | ⭐⭐⭐⭐ |

### 网络与存储 (09-12)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 09 | [网络模型基础](./09-network-model-fundamentals.md) | CNI插件、网络策略、服务发现、DNS解析 | ⭐⭐⭐⭐ |
| 10 | [存储架构设计](./10-storage-architecture-design.md) | CSI插件、PV/PVC、存储类、动态供应 | ⭐⭐⭐⭐ |
| 11 | [安全架构](./11-security-architecture.md) | RBAC、网络策略、Pod安全、密钥管理 | ⭐⭐⭐⭐⭐ |
| 12 | [可观测性体系](./12-observability-system.md) | Metrics、Logging、Tracing、监控告警 | ⭐⭐⭐⭐ |

### 高级主题 (13-18)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 13 | [扩展机制](./13-extension-mechanisms.md) | CRD、Admission Webhook、Aggregated API | ⭐⭐⭐⭐ |
| 14 | [多租户架构](./14-multi-tenancy-architecture.md) | 命名空间隔离、资源配额、网络隔离 | ⭐⭐⭐⭐ |
| 15 | [性能优化](./15-performance-optimization.md) | 架构层面性能调优、资源规划、瓶颈分析 | ⭐⭐⭐⭐ |
| 16 | [故障排查](./16-troubleshooting-guide.md) | 架构相关故障诊断、日志分析、性能分析 | ⭐⭐⭐⭐⭐ |
| 17 | [生产运维最佳实践](./17-production-operations-best-practices.md) | 高可用部署、安全加固、监控告警、备份恢复 | ⭐⭐⭐⭐⭐ |
| 18 | [升级迁移策略](./18-upgrade-migration-strategy.md) | K8s版本发展史、重大架构变化、升级策略 | ⭐⭐⭐⭐ |

---

## 学习路径建议

### 🎯 新手入门路径
**01 → 02 → 03 → 04 → 08**  
建立K8s架构基础认知，理解核心组件工作原理

### 🔧 进阶深入路径  
**05 → 06 → 07 → 09 → 10**  
深入学习存储、调度、控制器等核心机制

### 🏢 专家精通路径
**11 → 12 → 13 → 14 → 15 → 17**  
掌握安全、可观测性、扩展等高级架构主题，深入生产运维最佳实践

---

## 相关领域

- **[Domain-3: 控制平面](../domain-3-control-plane)** - 控制平面详细配置
- **[Domain-4: 工作负载](../domain-4-workloads)** - Pod、Deployment等资源管理
- **[Domain-5: 网络](../domain-5-networking)** - 网络插件和策略配置
- **[Domain-6: 存储](../domain-6-storage)** - 存储插件和持久化配置

---

**维护者**: Kusheet Architecture Team | **许可证**: MIT