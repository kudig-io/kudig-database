---
title: kubernetes-core 源码解析系列总览
description: 基于 kubernetes-1.36.2 与 etcd-3.7.0 真实源码树的核心组件源码深度剖析系列导航，覆盖架构、apiserver、KCM、scheduler、etcd、Informer 机制与组件数据流
summary: 九篇系列文档以真实源码树为基线（全部函数行号实测可跳转），覆盖控制平面与节点组件（kubelet/kube-proxy），从目录结构到单组件深剖再到跨组件数据流，构成「概念层文档 ↔ 源码实现」的桥梁层。
category: source-analysis
tags:
- k8s
- source-code
- index
- apiserver
- controller-manager
- scheduler
- etcd
- informer
- kubelet
- kube-proxy
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 10min
intent_queries:
- Kubernetes 源码解析系列导航
- 如何系统性阅读 Kubernetes 源码
- kubernetes-core 系列包含哪些内容
trigger_keywords:
- 源码解析
- kubernetes-core
- 源码阅读路径
- source analysis
related_domains:
- 集群基础
- 工作负载
- 网络
- 存储
- 安全
- 可观测性
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kubernetes-core 源码解析系列总览

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/`（完整源码树）+ `33-源码/控制平面/etcd-3.7.0/`
> 系列内所有函数行号均在上述源码树中**实测验证**，可直接跳转复核。源码目录约定见 [[33-源码/README.md|源码目录总索引]]。

## 定位

知识库中源码相关内容分三层，本系列是中间的桥梁层：

```
概念层   01-集群基础/01-架构总览、02-设计原则、03-控制平面   —— 讲"是什么/为什么"
源码层   本系列（kubernetes-core）                        —— 讲"代码怎么实现的"，行号实测
操作层   06-代码分析/cluster-create、cluster-cert 等       —— 讲"运维动作怎么做"
```

与概念层的 deep-dive 文档（如 [[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|APIServer Deep Dive]]）互补而不重复：概念层给机制全貌与运维视角，本系列给函数级源码落点与「症状 → 源码定位」的排障映射。

## 系列目录

| 篇 | 文档 | 主题 | 难度 |
|----|------|------|------|
| 01 | [[10-平台工程/06-代码分析/kubernetes-core/01-source-tree-architecture.md\|源码整体架构与目录结构]] | cmd/pkg/staging/vendor 布局、单向依赖、staging 机制 | advanced |
| 02 | [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md\|kube-apiserver 源码深度剖析]] | 三层委托链、Filter 链、genericregistry.Store、etcd3、Cacher | expert |
| 03 | [[10-平台工程/06-代码分析/kubernetes-core/03-kube-controller-manager-deep-dive.md\|kube-controller-manager 源码深度剖析]] | 控制器注册表、选主、Deployment/RS 调谐链、GC | expert |
| 04 | [[10-平台工程/06-代码分析/kubernetes-core/04-kube-scheduler-deep-dive.md\|kube-scheduler 源码深度剖析]] | 调度队列、Framework 扩展点、assume/bind、抢占 | expert |
| 05 | [[10-平台工程/06-代码分析/kubernetes-core/05-etcd-storage-deep-dive.md\|etcd 与存储链路源码剖析]] | Raft 提交、MVCC、watchableStore、resourceVersion 本体 | expert |
| 06 | [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md\|声明式 API 与 Informer 机制]] | Reflector/DeltaFIFO/Indexer/WorkQueue 完整流水线 | expert |
| 07 | [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md\|组件交互关系与数据流向]] | Hub-and-Spoke、Pod 创建 8 步主线、kubelet/kube-proxy | advanced |
| 08 | [[10-平台工程/06-代码分析/kubernetes-core/08-kubelet-deep-dive.md\|kubelet 源码深度剖析]] | podWorkers、computePodActions、PLEG、volumemanager、驱逐 | expert |
| 09 | [[10-平台工程/06-代码分析/kubernetes-core/09-kube-proxy-deep-dive.md\|kube-proxy 源码深度剖析]] | ChangeTracker、BoundedFrequencyRunner、iptables/ipvs/nftables 三模式 | expert |

## 推荐阅读路径

- **首次通读**：01 → 06 → 02 → 03 → 04 → 05 → 07（先掌握 Informer 公分母，再看各组件如何消费它），节点侧续读 08 → 09
- **排障导向**：直接进 07 篇的「8 步主线」，按 Pod 卡住的位置跳到对应分篇的排障速查表；节点/网络侧症状进 08/09 篇
- **Operator 开发者**：06 篇（client-go 原语与陷阱）→ 03 篇（标准控制器写法参照）→ [[01-集群基础/02-设计原则/13-operator-development-guide.md|Operator 开发指南]]
- **性能/稳定性调优**：02 篇（APF/watchCache）→ 05 篇（compaction/defrag/NOSPACE）→ 04 篇（调度吞吐采样）

## 与各技术域的衔接

跨域机制衔接总表见 [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 篇第四节]]，覆盖集群基础、工作负载、网络、存储、安全、可观测性、可靠性、容器运行时、AI 基础设施、故障诊断十个域的接口点。

生态上下游组件（CRI 运行时、CNI/CSI 插件、服务网格、监控日志、CI/CD、仓库/DNS/LB）的集成点源码分析见姊妹系列 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 相关文档

- [[33-源码/README.md|33-源码 目录总索引]]（源码 ↔ 领域文档双向映射）
- [[01-集群基础/01-架构总览/04-source-code-structure.md|源码结构导读]]（概念层）
- [[01-集群基础/02-设计原则/10-source-code-walkthrough.md|源码阅读指南]]（方法论）
- [[10-平台工程/06-代码分析/README.md|集群操作函数库]]（操作层）
