---
title: 设计原理：声明式 API、控制器模式与 etcd 共识
description: '# 设计原理：声明式 API、控制器模式与 etcd 共识'
summary: 'Kubernetes 采用**声明式（Declarative）API** 而非命令式（Imperative）：'
category: reference
tags:
- k8s
- design-patterns
- declarative-api
- controller
- etcd
- raft
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 设计原理：声明式 API、控制器模式与 etcd 共识 是什么
- 如何 设计原理：声明式 API、控制器模式与 etcd 共识
trigger_keywords:
- 设计原理：声明式
- API
- 控制器模式与
- etcd
- 共识
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 设计原理：声明式 API、控制器模式与 etcd 共识

## 概述

Kubernetes 的核心设计哲学建立在三大支柱之上：声明式 API、控制器模式和 etcd Raft 共识。这些设计原则使 Kubernetes 成为可扩展、自愈和最终一致的容器编排平台。理解这些底层原理是掌握 Kubernetes 运维和扩展开发的基础。

## 声明式 API 哲学

Kubernetes 采用**声明式（Declarative）API** 而非命令式（Imperative）：
- **命令式**：告诉系统"做什么"（`docker run nginx`）
- **声明式**：告诉系统"要什么"（YAML 中定义 `replicas: 3`）

声明式 API 的核心优势：
1. **幂等性**：重复提交同一 YAML 不会产生副作用，便于 GitOps 自动化
2. **可审计**：所有状态变更有完整的 resourceVersion 记录和审计日志
3. **自愈能力**：控制器持续调谐，确保实际状态匹配期望状态，节点故障后自动恢复

## 控制器模式详解

每个 Kubernetes 控制器遵循统一的调谐循环（Reconciliation Loop）：

```
Watch(Kubernetes API) → 获取变更事件
    ↓
Compare(期望状态 vs 实际状态)
    ↓
Act(执行操作使实际状态趋向期望状态)
    ↓
Report(更新状态到 API Server)
```

关键设计原则：
- **Level Triggered** 而非 Edge Triggered：基于当前状态而非事件序列，避免漏处理事件
- **幂等操作**：同一调谐多次执行结果一致，安全重试
- **最终一致性**：不要求即时同步，容许短暂不一致
- **Owner Reference**：通过所有者引用实现级联删除和垃圾回收

典型控制器包括 Deployment Controller（管理 ReplicaSet 滚动更新）、Node Controller（节点健康检测与驱逐）、Endpoint Controller（维护 Service 到 Pod 的映射）。

## Watch-List 机制

Kubernetes 的 Watch 机制是控制器获取变更的核心方式：
- **List**：首次全量获取资源对象（通过 LIST 请求）
- **Watch**：基于 resourceVersion 增量监听变更（通过 WATCH 请求）
- **bookmark**：优化 Watch 连接，减少不必要的事件传输和重连

Resource Version 是 etcd 的全局递增版本号（mod_revision），用于：
- 并发控制（乐观锁 CAS）
- 增量 Watch 起始点
- 防止过期数据覆盖新数据（409 Conflict）

Informer 框架封装了 List-Watch 逻辑，提供本地缓存（Store）和事件队列（Delta FIFO），大幅减少 API Server 压力。

## etcd Raft 共识

etcd 使用 Raft 共识协议保证分布式一致性：
- **Leader 选举**：集群中一个节点被选为 Leader，处理所有写请求
- **日志复制**：Leader 将写操作日志复制到多数节点后才提交（Majority Quorum）
- **安全性**：已提交的日志条目不会被覆盖（Leader Completeness）
- **成员变更**：支持运行时添加/移除节点

生产部署建议：
- 节点数：3（容忍 1 节点故障）或 5（容忍 2 节点故障），奇数节点
- 磁盘：SSD，最低 500 IOPS，建议 fio benchmark 验证
- 网络：节点间延迟 < 10ms，带宽 > 1Gbps
- 压缩：配置 `--auto-compaction-retention` 防止 etcd 数据库膨胀

## 实践应用

理解这些设计原理后的实践要点：
- 开发 Operator 时遵循调谐循环模式，确保幂等性
- 排查问题时从 etcd 状态出发，通过 `kubectl get --raw` 检查底层状态
- 性能调优时关注 Watch 缓存命中率和 etcd 写延迟

---

> 来源：.zread/wiki/drafts/6-she-ji-yuan-li-sheng-ming-shi-api-kong-zhi-qi-mo-shi-yu-etcd-gong-shi.md

## Related

- [[生态参考/98-merged-indexes/index.md|release-notes-core-deps]] — 发布说明索引 — 核心依赖
- [[实体/k8s-architecture-fundamentals.md|k8s-architecture-fundamentals]] — K8s 架构基础与核心组件原理
- [[docker]] — Docker
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
