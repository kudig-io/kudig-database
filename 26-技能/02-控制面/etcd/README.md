---
title: etcd 故障诊断与备份恢复技能集
description: etcd 集群异常（Raft 共识、存储性能、快照、认证、碎片化）的故障树诊断及备份恢复操作技能体系
summary: etcd 技能集入口，涵盖 etcd 异常诊断、备份恢复操作、备份恢复故障树三大技能
category: skill
tags:
- k8s
- etcd
- raft
- control-plane
- backup
- restore
- troubleshooting
- fta
- disaster-recovery
sources:
- 故障诊断/FTA故障树/list/etcd-fta.md
- 故障诊断/FTA故障树/list/backup-restore-fta.md
- code/etcd-3.7.0/
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- etcd 故障排查从哪里开始
- etcd 备份恢复怎么做
- etcd 集群不可用如何诊断
- etcd 性能问题怎么排查
trigger_keywords:
- etcd
- Raft
- 备份
- 恢复
- snapshot
- quorum
- leader
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本技能集包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd 故障诊断与备份恢复技能集

## 概述

本技能集覆盖 etcd 集群的完整运维诊断能力：

- **etcd 异常诊断**：成员可用性、Raft 共识、磁盘 IO、网络与时钟、证书与访问控制、性能与碎片化
- **备份恢复操作**：快照创建/验证、定时备份策略、单节点/全集群恢复、灾难恢复 SOP
- **备份恢复故障树**：etcd 快照异常、Velero 应用级备份、存储后端、加密校验、恢复流程、依赖调度

**适用场景**：
- etcd 集群不可用/性能劣化
- API Server 连接 etcd 超时
- etcd 磁盘空间告警/NOSPACE
- etcd 证书过期/认证失败
- 备份任务失败/恢复操作
- 数据丢失灾难恢复

---

## 技能文件索引

| # | 文件 | 覆盖场景 | 难度 | 预计阅读 |
|---|------|---------|------|---------|
| 01 | [etcd-fta.md](etcd-fta.md) | etcd FTA 故障树（成员/Raft/磁盘/网络/证书/性能） | 高级 | 25min |
| 02 | [backup-restore-etcd.md](backup-restore-etcd.md) | etcd 备份与恢复标准操作流程（SOP） | 高级 | 20min |
| 03 | [backup-restore-fta.md](backup-restore-fta.md) | 备份恢复故障树（etcd 快照/Velero/存储/加密/恢复流程） | 高级 | 20min |

---

## 快速诊断入口

```bash
# 🟢 低风险：只读/信息收集，通常无副作用

# Step 1: 检查 etcd 集群健康
ETCDCTL_API=3 etcdctl endpoint health --cluster \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# Step 2: 查看成员状态（leader/DB SIZE）
ETCDCTL_API=3 etcdctl endpoint status --cluster --write-out=table \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# Step 3: 检查 etcd Pod 状态（静态 Pod 部署）
kubectl get pods -n kube-system -l component=etcd -o wide

# Step 4: 检查 K8s 控制面健康
kubectl get --raw /healthz?verbose | grep -v ok
```

---

## 状态速查表

| 症状 | 常见原因 | 优先检查项 | 对应技能 |
|:---|:---|:---|:---|
| API Server 报 etcd timeout | 磁盘 IO 慢/网络延迟/过载 | `etcdctl endpoint status` + 磁盘 IO | etcd-fta RC-006/008 |
| etcd NOSPACE 告警 | 未压缩/碎片化/quota 超限 | DB SIZE vs quota | etcd-fta RC-005 |
| leader 频繁切换 | 网络抖动/磁盘慢/资源不足 | 网络延迟 + IO 指标 | etcd-fta RC-003 |
| kubectl 命令极慢 | etcd 性能劣化 | WAL fsync 延迟 | etcd-fta RC-006 |
| 证书过期连接拒绝 | 证书到期未续期 | `openssl x509 -dates` | etcd-fta RC-011 |
| 备份文件为空/失败 | 脚本错误/证书路径/磁盘满 | 备份脚本返回码 + 文件大小 | backup-restore RC-001 |
| 恢复后数据不一致 | 仅恢复部分节点 | 全节点从同一快照恢复 | backup-restore RC-006 |

---

## FTA 故障树路径映射

| 顶层事件 | 中间事件 | 底事件 | 对应技能 |
|---------|---------|--------|---------|
| TE-CP 控制面异常 | IE-1 etcd 不可用 | BE-1.1 成员宕机/OOM | etcd-fta RC-001 |
| TE-CP 控制面异常 | IE-1 etcd 不可用 | BE-1.2 leader 选举异常 | etcd-fta RC-003 |
| TE-CP 控制面异常 | IE-2 etcd 性能劣化 | BE-2.1 WAL fsync 延迟高 | etcd-fta RC-006 |
| TE-CP 控制面异常 | IE-2 etcd 性能劣化 | BE-2.2 碎片化/空间超限 | etcd-fta RC-005/013 |
| TE-CP 控制面异常 | IE-3 证书/认证异常 | BE-3.1 证书过期 | etcd-fta RC-011 |
| TE-DR 数据丢失 | IE-1 备份失败 | BE-1.1 脚本/凭据/存储异常 | backup-restore-fta |
| TE-DR 数据丢失 | IE-2 恢复失败 | BE-2.1 版本/顺序/冲突 | backup-restore-fta |

---

## 版本兼容性矩阵

| etcd 版本 | K8s 版本 | 关键差异 |
|----------|---------|---------|
| 3.4.x | 1.18-1.21 | 默认 quota 2GB |
| 3.5.x | 1.22-1.28 | 默认 quota 8GB；3.5.0-3.5.3 有数据损坏 Bug |
| 3.6.x | 1.29-1.32 | 新 Raft 实现；性能优化 |
| 3.7.x | 1.34-1.36 | 进一步性能优化 |

> **关键原则**：etcd 快照恢复必须使用与备份时**相同大版本**的二进制。

---

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]] — 执行引擎
- [[26-技能/02-控制面/apiserver/apiserver-fta.md|API Server 故障树]] — 同域关联
- [[26-技能/02-控制面/scheduler/scheduler-fta.md|Scheduler 故障树]] — 同域关联
- [[26-技能/03-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]] — 跨域关联
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]] — 知识索引
- [[21-生态参考/03-领域索引/backup-dr-index.md|Backup & DR 知识图谱索引]] — 知识索引
