---
title: 文档阅读顺序索引
description: '## 概述'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- beginner-devops
- developer
- platform-engineer
estimated_read_time: 10min
intent_queries:
- k8s 学习路径 四周计划
- kubernetes 文档阅读顺序推荐
- kudig-database 知识库学习顺序
- 从入门到进阶学习路线
trigger_keywords:
- 学习路径
- 阅读顺序
- 四周计划
- 学习路线
- 入门
- 进阶
- 知识体系
- 文档索引
related_domains:
- domain-1-architecture-fundamentals
- domain-4-workloads
- domain-5-networking
- domain-7-security
- domain-8-observability
- domain-12-troubleshooting
related_topics:
- topic-learn/public-training/one-month/resources/knowledge-map
- topic-learn/public-training/one-month/resources/commands-cheatsheet
---


# 文档阅读顺序索引

## 概述

本文档按学习计划顺序整理了 kudig-database 知识库中的关键文档，帮助你按照从基础到进阶、从理论到实践的路径系统化地学习 Kubernetes 运维知识。

知识库中的文档按域名（domain）组织，每个域名覆盖一个技术领域。但学习时需要按特定顺序跨域名阅读，而不是按域名逐一学习。本文档的作用就是为你规划最优的阅读路径。

### 使用方法

- 按周次和天次顺序阅读
- 标有 ⭐ 的为核心文档，必须精读
- 其他文档按需阅读，可根据时间调整深度
- 每个文档预计阅读时间 30-60 分钟
- 建议阅读时做笔记，记录关键概念和疑问点

---

## Week 1: 地基建设期

本周的学习目标是建立容器技术、Linux 运维和 K8s 架构的完整认知。这三者是所有后续学习的基础。

### Day 1-2: Docker 基础与进阶

Docker 是 Kubernetes 运行容器的底层技术。这两天的学习帮助你理解"容器到底是什么"，并掌握 Docker 的基本操作。

1. `domain-13-docker/01-docker-architecture-overview.md`
   - Docker Engine 的架构组成：Client、Daemon、Registry 三者的交互流程
   - 镜像（Image）与容器（Container）的关系：类（Class）与实例（Instance）的类比
   - Docker 与 K8s 的协作方式：K8s 通过 CRI 调用容器运行时

2. `domain-13-docker/03-docker-container-lifecycle.md`
   - 容器状态转换：Created → Running → Paused → Stopped → Deleted
   - 容器与 Pod 生命周期的类比：Pod 中可以包含多个容器，它们共享网络和存储命名空间
   - 容器的退出码含义：0 表示正常退出，1-127 表示应用错误，137 表示 OOMKilled

3. `domain-13-docker/04-docker-networking-deep-dive.md`
   - Docker 网络模型：bridge、host、overlay、none 四种模式
   - 容器间通信原理：veth pair + bridge + iptables
   - 与 K8s 网络模型的对比：K8s 要求每个 Pod 有独立 IP 且 Pod 间可以直接通信

4. `domain-13-docker/05-docker-storage-volumes.md`
   - Docker Volume 的三种类型：volume、bind mount、tmpfs
   - 镜像分层与 UnionFS：只读层 + 可写层的叠加
   - 与 K8s Volume 的对应关系：K8s 扩展了 Volume 的类型和生命周期管理

5. `domain-13-docker/07-docker-security-best-practices.md`
   - 容器安全基础：以非 root 运行、只读文件系统、资源限制
   - 镜像安全：使用可信基础镜像、定期扫描漏洞、多阶段构建
   - 运行时安全：Seccomp、AppArmor、能力（Capabilities）限制

6. `domain-13-docker/99-docker-commands-reference.md`
   - Docker CLI 完整命令参考
   - 按功能分类：镜像管理、容器操作、网络管理、存储管理
   - 建议作为速查手册使用

### Day 3-4: Linux 运维基础

Linux 是 K8s 节点的操作系统，掌握 Linux 运维基础是排障能力的根基。

1. `domain-14-linux/01-linux-system-architecture.md`
   - Linux 内核架构：进程调度、内存管理、文件系统、网络栈
   - 用户空间与内核空间的隔离
   - 容器技术依赖的内核特性：namespace、cgroup、netfilter

2. `domain-14-linux/02-linux-process-management.md`
   - 进程状态：Running、Sleeping、Stopped、Zombie
   - 信号机制：SIGTERM（优雅终止）、SIGKILL（强制终止）与 K8s 的 Pod 终止流程
   - 进程树与孤儿进程：理解容器 PID 1 的重要性

3. `domain-14-linux/04-linux-networking-configuration.md`
   - TCP/IP 协议栈基础
   - 网络接口、路由表、DNS 配置
   - 网络排障工具：tcpdump、ss、ip、nslookup

4. `domain-14-linux/06-linux-performance-tuning.md`
   - CPU 性能分析：top、mpstat、perf
   - 内存性能分析：free、vmstat、slabtop
   - IO 性能分析：iostat、iotop
   - 网络性能分析：sar、nethogs

5. `domain-14-linux/08-linux-container-fundamentals.md`
   - Linux namespace 详解：PID、NET、MNT、UTS、IPC、USER、CGROUP
   - cgroup 资源限制：CPU、内存、IO、网络
   - 容器运行时的实现原理

6. `domain-14-linux/99-linux-commands-reference.md`
   - Linux 命令速查手册
   - 按场景分类：系统信息、进程管理、网络、存储、安全

### Day 5-6: K8s 架构

这两天的内容是整个学习计划的"技术地图"。

1. `domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md` ⭐
   - K8s 整体架构：Master/Node 两层架构
   - 控制平面组件的功能和交互方式
   - 数据平面组件的职责
   - K8s 的设计哲学：声明式、水平扩展、自愈

2. `domain-1-architecture-fundamentals/02-core-components-deep-dive.md` ⭐
   - kube-apiserver：请求处理链（认证→授权→准入→验证→写入）
   - etcd：Raft 共识协议、数据模型、性能调优
   - kube-scheduler：调度算法（Filter → Score）
   - kube-controller-manager：Reconcile 循环
   - kubelet：Pod 生命周期管理、探针执行
   - kube-proxy：iptables/IPVS 模式

3. `domain-1-architecture-fundamentals/03-api-versions-features.md`
   - API 版本演进：Alpha → Beta → GA
   - API Group 的组织方式
   - 资源废弃策略和迁移方法

4. `domain-1-architecture-fundamentals/05-kubectl-commands-reference.md`
   - kubectl 完整命令参考
   - 常用命令分类：资源查看、创建删除、调试排障、集群管理
   - 输出格式化：-o wide、-o yaml、-o json、jsonpath

5. `domain-1-architecture-fundamentals/06-cluster-configuration-parameters.md`
   - 集群配置参数详解
   - kubelet 参数、kube-apiserver 参数
   - 网络参数、存储参数

6. `topic-cheat-sheet/k8s.md`
   - K8s 速查手册
   - 常用命令、资源对象、标签速查

---

## Week 2: 核心技术构建期

本周深入 K8s 的四大核心技术领域。

### Day 8-9: 控制平面深入

1. `domain-3-control-plane/11-etcd-deep-dive.md`
   - etcd 集群部署：3 节点 vs 5 节点
   - Raft 协议的 Leader 选举和日志复制
   - 数据压缩和碎片整理
   - 备份恢复策略

2. `domain-3-control-plane/12-apiserver-deep-dive.md`
   - 请求处理流水线：认证→授权→准入→验证→etcd写入
   - Watch 机制的实现原理
   - 性能调优：请求限流、缓存优化

3. `domain-3-control-plane/13-kube-controller-manager-deep-dive.md`
   - 控制器模式（Controller Pattern）
   - Deployment Controller 的工作流程
   - ReplicaSet Controller 的调和循环

4. `domain-3-control-plane/20-kube-scheduler-deep-dive.md`
   - 调度框架（Scheduling Framework）
   - Predicate 和 Priority 函数
   - 亲和性/反亲和性、污点/容忍
   - 自定义调度器

5. `domain-2-design-principles/03-controller-pattern.md`
   - 声明式 API 的设计理念
   - Reconcile 循环的实现模式
   - 理解 K8s 的核心设计模式

6. `domain-2-design-principles/07-distributed-consensus-etcd.md`
   - 分布式一致性问题
   - Raft 协议的工作原理
   - etcd 在 K8s 中的作用

### Day 10-11: 工作负载管理

1. `domain-4-workloads/02-deployment-production-patterns.md`
   - 滚动更新策略：maxSurge 和 maxUnavailable 的调优
   - 蓝绿部署和金丝雀发布
   - 回滚策略和版本管理

2. `domain-4-workloads/03-statefulset-advanced-operations.md`
   - 有状态应用的管理挑战
   - StatefulSet 的网络标识和存储绑定
   - 分区更新和并行度控制

3. `domain-4-workloads/04-daemonset-management.md`
   - DaemonSet 的调度策略
   - 滚动更新和回滚
   - 典型使用场景

4. `domain-4-workloads/11-pod-lifecycle-events.md`
   - Pod 的完整生命周期：Pending → Running → Succeeded/Failed
   - Init Container 的执行顺序
   - 容器探针：liveness、readiness、startup
   - Pod 终止流程和优雅关闭

5. `domain-4-workloads/21-hpa-vpa-autoscaling.md`
   - HPA 的工作原理和指标类型
   - 基于 CPU/内存的自动伸缩
   - 基于自定义指标的自动伸缩
   - VPA（垂直自动伸缩）

6. `domain-4-workloads/23-resource-management.md`
   - requests 和 limits 的配置原则
   - QoS 类别：Guaranteed、Burstable、BestEffort
   - 资源配额和限制范围

### Day 12-13: 网络栈

1. `domain-5-networking/01-network-architecture-overview.md`
   - K8s 网络模型：每个 Pod 有独立 IP，Pod 间直接通信
   - 三层网络：Pod 网络、Service 网络、外部网络
   - 网络插件的选择标准

2. `domain-5-networking/02-cni-architecture-fundamentals.md`
   - CNI 规范和插件类型
   - 常见 CNI 插件对比：Calico、Flannel、Cilium
   - CNI 配置和排障

3. `domain-5-networking/06-service-concepts-types.md`
   - Service 四种类型的详细配置
   - Endpoints 和 EndpointSlices
   - Service 的负载均衡机制
   - 会话保持（Session Affinity）

4. `domain-5-networking/11-dns-service-discovery-coredns.md`
   - CoreDNS 的工作原理
   - Service DNS 记录格式
   - Pod DNS 配置
   - CoreDNS 自定义配置和排障

5. `domain-5-networking/16-networkpolicy-deep-practice.md`
   - NetworkPolicy 的语法和行为
   - 入站和出站规则
   - 常见网络隔离模式

6. `domain-5-networking/19-ingress-fundamentals.md`
   - Ingress 资源的定义和配置
   - Ingress Controller 的工作原理
   - TLS 配置和证书管理

7. `domain-5-networking/21-nginx-ingress-complete-guide.md`
   - Nginx Ingress Controller 的完整配置
   - 路径类型、注解、自定义模板
   - 性能调优

8. `domain-5-networking/22-ingress-tls-certificate.md`
   - TLS 证书的配置方式
   - cert-manager 自动证书管理
   - 证书轮转策略

### Day 14: 存储体系

1. `domain-6-storage/01-storage-architecture-overview.md`
   - K8s 存储架构：Volume → PV/PVC → StorageClass
   - 存储类型的对比：块存储、文件存储、对象存储
   - CSI 接口标准

2. `domain-6-storage/02-pv-architecture-fundamentals.md`
   - PV 和 PVC 的生命周期：Provision → Bind → Use → Reclaim
   - 访问模式：RWO、ROX、RWX
   - 回收策略：Retain、Delete、Recycle

3. `domain-6-storage/04-storageclass-dynamic-provisioning.md`
   - StorageClass 的定义和配置
   - 动态供给的工作流程
   - 卷扩展和快照

---

## Week 3: 运维作战能力期

### Day 15-16: 安全体系

1. `domain-7-security/01-authentication-authorization-system.md`
   - K8s 认证方式：证书、Token、OIDC
   - RBAC 授权模型详解
   - 准入控制器的工作机制

2. `domain-7-security/06-pod-security-standards.md`
   - Pod 安全标准：Privileged、Baseline、Restricted
   - Pod Security Admission 的配置
   - 安全上下文（Security Context）

3. `domain-7-security/07-rbac-matrix-configuration.md`
   - RBAC 权限矩阵的设计方法
   - 常见权限模式和最佳实践
   - 权限审计和合规检查

4. `domain-7-security/10-certificate-management.md`
   - K8s 中的证书类型和用途
   - 证书轮转和更新策略
   - kubeconfig 管理

5. `domain-7-security/11-secret-management-tools.md`
   - Secret 的安全存储和传输
   - 外部 Secret 管理工具：Vault、Sealed Secrets
   - Secret 的最佳实践

6. `domain-7-security/14-policy-engines-opa-kyverno.md`
   - OPA Gatekeeper 和 Kyverno 的对比
   - 策略定义和执行
   - 常见安全策略示例

### Day 17-18: 可观测性

1. `domain-8-observability/01-observability-architecture-overview.md`
   - 可观测性三大支柱：Metrics、Logs、Traces
   - K8s 可观测性的架构设计
   - 工具选型和集成方式

2. `domain-8-observability/02-monitoring-metrics-system.md`
   - 指标类型：Counter、Gauge、Histogram、Summary
   - 监控体系的设计原则
   - 黄金信号：Latency、Traffic、Errors、Saturation

3. `domain-8-observability/03-logging-architecture.md`
   - K8s 日志架构：容器日志、节点日志、集群日志
   - 日志采集方案：Fluentd、Fluent Bit、Promtail
   - 日志存储和分析：ELK、Loki

4. `domain-8-observability/04-distributed-tracing.md`
   - 分布式追踪的原理：Trace → Span → Context Propagation
   - OpenTelemetry 标准
   - 追踪数据的采集和分析

5. `domain-8-observability/05-alerting-management.md`
   - 告警设计原则：分级、路由、抑制、静默
   - Alertmanager 的配置和优化
   - 告警疲劳的防治

6. `domain-8-observability/10-monitoring-metrics-prometheus.md`
   - Prometheus 的部署和配置
   - PromQL 查询语言
   - Service Discovery 和 Target 配置

7. `domain-8-observability/21-monitoring-playbooks.md`
   - 常见告警的响应手册
   - 监控排障的标准流程
   - 监控体系的持续优化

### Day 19-21: 故障排查

1. `topic-structural-trouble-shooting/README.md` ⭐
   - 结构化故障排查的方法论
   - 从现象到根因的系统化分析流程

2. `topic-fta/04-fta-core-principles.md` ⭐
   - 故障树分析（FTA）的核心原则
   - 故障树的构建方法
   - K8s 故障场景的 FTA 应用

3. `topic-febm/01-febm-theory-foundations.md` ⭐
   - 取证循证方法（FEBM）的理论基础
   - 证据收集→假设形成→验证的循环
   - 在 K8s 故障排查中应用 FEBM

4. `domain-12-troubleshooting/05-pod-pending-diagnosis.md`
   - Pod Pending 的常见原因
   - 调度失败的诊断方法

5. `domain-12-troubleshooting/06-node-notready-diagnosis.md`
   - Node NotReady 的排查步骤
   - kubelet 故障的诊断方法

6. `domain-12-troubleshooting/07-oom-memory-diagnosis.md`
   - OOMKilled 的分析方法
   - 内存泄漏的排查

7. `domain-12-troubleshooting/08-pod-comprehensive-troubleshooting.md`
   - Pod 排障的综合方法
   - 从创建到运行全阶段的问题诊断

8. `domain-12-troubleshooting/10-service-comprehensive-troubleshooting.md`
   - Service 网络问题的诊断
   - DNS 解析问题的排查

9. `domain-9-platform-ops/02-cluster-lifecycle-management.md`
   - 集群生命周期管理
   - 升级、扩容、迁移的最佳实践

10. `domain-9-platform-ops/12-backup-recovery-strategy.md`
    - 备份恢复策略
    - etcd 备份和应用数据备份

---

## Week 4: 企业级进阶期

### Day 22-23: 企业级工具

1. `domain-20-enterprise-monitoring-alerting/01-prometheus-enterprise-monitoring.md`
   - Prometheus 高可用部署方案
   - Thanos/Cortex 跨集群监控
   - 长期存储和查询优化

2. `domain-20-enterprise-monitoring-alerting/02-grafana-enterprise-observability.md`
   - Grafana 企业级功能
   - Dashboard 设计最佳实践
   - 多数据源集成

3. `domain-21-logging-management-analytics/01-elk-stack-enterprise-logging.md`
   - ELK Stack 的企业级部署
   - 日志分析和可视化
   - 日志保留和归档策略

4. `domain-23-gitops-ci-cd/01-argo-cd-enterprise-gitops.md`
   - ArgoCD 的架构和工作原理
   - Application、Project、Sync Policy
   - 多集群和多团队管理

5. `domain-8-observability/18-slo-sli-system.md`
   - SLO/SLI 的设计方法
   - 错误预算的计算和使用
   - SLO 告警规则

### Day 24-25: 安全与最佳实践

1. `domain-25-cloud-native-security/04-kyverno-enterprise-policy-management.md`
   - Kyverno 策略的企业级管理
   - Validate、Mutate、Generate 三种策略类型
   - 策略报告和合规审计

2. `domain-25-cloud-native-security/05-vault-enterprise-secrets-management.md`
   - Vault 的架构和部署
   - K8s Auth Method 的配置
   - Secret 的自动轮转

3. `domain-18-production-operations/01-production-architecture-design-principles.md`
   - 生产架构设计原则
   - 高可用设计模式
   - 容量规划和性能优化

4. `domain-18-production-operations/07-zero-trust-security-architecture.md`
   - 零信任架构的核心理念
   - K8s 中的零信任实践
   - 服务网格和安全通信

5. `domain-18-production-operations/22-change-management-process.md`
   - 变更管理的标准流程
   - 变更风险评估和回滚策略
   - 变更日历和维护窗口

6. `domain-18-production-operations/23-incident-response-handling.md`
   - 事故响应的标准化流程
   - 事故等级划分和升级机制
   - 事故复盘和改进

7. `domain-18-production-operations/24-capacity-planning-forecasting.md`
   - 容量规划的方法论
   - 资源使用趋势分析
   - 扩容决策和成本优化

### Day 26-27: 专题深化

1. `topic-fta/23-fta-production-quick-start.md` ⭐
   - FTA 在生产环境中的快速应用
   - 常见故障场景的故障树模板

2. `topic-fta/kubernetes-fta-full-analysis.md`
   - K8s 全场景 FTA 分析
   - 从 Pod 到集群级别的故障树

3. `topic-febm/08-febm-production-quick-start.md` ⭐
   - FEBM 的生产快速入门
   - 证据收集的最佳实践

4. `topic-fta/10-agent-orchestration-patterns.md`
   - Agent 编排模式
   - 自动化故障诊断的设计思路

5. `domain-10-extensions/01-crd-development-guide.md`
   - CRD 的定义和使用
   - 自定义资源的开发流程

6. `domain-10-extensions/06-helm-charts-management.md`
   - Helm Chart 的结构和管理
   - Chart 仓库和版本管理

7. `domain-9-platform-ops/20-crd-operator-development.md`
   - Operator 模式的原理
   - Operator SDK 的使用
   - Controller 的开发流程

---

## 要点总结

本文档覆盖了 4 周学习计划中约 70 篇核心文档的阅读顺序。关键原则：

- **循序渐进**: 先基础后进阶，先理论后实践
- **重点优先**: 标 ⭐ 的核心文档必须精读，其他按需阅读
- **动手验证**: 每读完一篇文档，都应该在集群中验证所学内容
- **笔记沉淀**: 重要的概念和命令应该记录到自己的知识库中

---

## 延伸阅读

- [K8s 速查手册](./commands-cheatsheet.md) — 常用命令快速查找
- [知识图谱](./knowledge-map.md) — K8s 知识体系的全景视图
