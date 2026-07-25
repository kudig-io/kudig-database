---
title: Cluster 集群级故障诊断技能集
description: Kubernetes 集群级故障（控制面不可用、etcd 异常、证书过期、集群升级失败、集群扩缩容异常）的完整诊断技能体系，含 FTA 故障树、证据三元组、SOP 与生产案例
summary: 集群级故障诊断技能集入口，聚焦控制平面、etcd、证书、升级与扩缩容五大集群级故障域，遵循 12 章节生产标准
category: skill
tags:
- k8s
- cluster
- troubleshooting
- controlplane
- apiserver
- etcd
- certificate
- upgrade
- autoscaling
- fta
- sop
- runbook
sources:
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- 故障诊断-集群运维/kubeadm-fta.md
- 故障诊断-集群运维/cluster-upgrade/
- 故障诊断-集群运维/cluster-autoscaler/
- code/apiserver-master/
- code/etcd-3.7.0/
- code/kube-controller-manager-master/
- code/kube-scheduler-master/
- code/kubeadm-main/
created: '2026-07-23'
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
estimated_read_time: 8min
intent_queries:
- 集群不可用怎么排查
- apiserver 连不上怎么办
- etcd 故障如何恢复
- 集群证书过期怎么处理
- 集群升级失败如何回滚
- Cluster Autoscaler 不扩容原因
trigger_keywords:
- 集群不可用
- apiserver
- kube-apiserver
- etcd
- quorum
- 证书过期
- certificate expired
- 集群升级
- cluster upgrade
- Cluster Autoscaler
- 控制平面
- control plane
prerequisites:
- kubectl-basics
- cluster-architecture
- etcd-basics
- troubleshooting-methodology
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> **集群级操作特别警告**：本技能集涉及控制平面与 etcd 的操作属于全集群影响范围，任何写操作前必须完成 etcd 快照备份，并在变更窗口内由高级工程师双人复核执行。

# Cluster 集群级故障诊断技能集

## 概述

本技能集整合 Kubernetes **集群级**故障诊断的完整知识体系，覆盖控制平面不可用、etcd 异常、证书过期、集群升级失败与集群扩缩容异常五大故障域。与工作负载（pod）、节点（node）技能集互补——当故障影响面为**整个集群或控制平面**时，优先进入本技能集。

**适用场景**：

- 控制平面不可用（kube-apiserver / kube-controller-manager / kube-scheduler 异常）
- etcd 集群异常（失去 quorum、性能劣化、数据损坏、备份与恢复）
- 集群证书过期与轮换（kubeadm PKI、kubelet 证书）
- 集群版本升级失败与回滚
- 集群自动扩缩容异常（Cluster Autoscaler / Karpenter / 节点池）

**边界**（不在本技能集范围）：

- 单 Pod 崩溃/调度失败 → 转 [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- 单节点 NotReady/资源压力 → 转 [[26-技能/03-节点/node/README.md|Node 异常诊断技能集]]

---

## 技能文件索引

| # | 文件 | 覆盖场景 | 难度 | 预计阅读 |
|---|------|---------|------|---------|
| 01 | [01-apiserver-controlplane.md](01-apiserver-controlplane.md) | kube-apiserver 不可用、控制面组件崩溃、认证/准入失败、限流 | 高级 | 25min |
| 02 | [02-etcd-troubleshooting.md](02-etcd-troubleshooting.md) | etcd 失去 quorum、性能劣化、磁盘配额、备份与恢复 | 高级 | 25min |
| 03 | [03-cluster-cert-upgrade.md](03-cluster-cert-upgrade.md) | 证书过期与轮换、集群版本升级失败与回滚 | 高级 | 25min |
| 04 | [04-cluster-autoscaling-sop.md](04-cluster-autoscaling-sop.md) | Cluster Autoscaler 不扩缩、节点池异常、集群级 SOP/Runbook | 中级 | 20min |

---

## 快速诊断入口

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 控制平面组件健康
kubectl get --raw='/readyz?verbose' 2>/dev/null || echo "apiserver 不可达"

# Step 2: 核心组件 Pod 状态（kubeadm 集群）
kubectl get pods -n kube-system -l tier=control-plane -o wide

# Step 3: etcd 成员健康
kubectl -n kube-system exec etcd-<master> -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key endpoint health

# Step 4: 证书到期时间（kubeadm）
kubeadm certs check-expiration
```

---

## 集群级故障速查表

| 症状 | 常见原因 | 优先检查项 | 对应技能 |
|:---|:---|:---|:---|
| **kubectl 全部超时/拒绝** | apiserver 崩溃/etcd 不可用 | apiserver 进程 + etcd health | 01 / 02 |
| **apiserver 频繁重启** | etcd 慢/证书错误/资源不足 | apiserver 日志 + etcd 延迟 | 01 |
| **写操作全部失败，读正常** | etcd 失去 quorum | etcd 成员数与 leader | 02 |
| **etcd 拒绝写入 (mvcc: database space exceeded)** | etcd 磁盘配额超限 | etcd db size + defrag | 02 |
| **TLS handshake / x509 错误** | 证书过期 | `kubeadm certs check-expiration` | 03 |
| **升级后组件 CrashLoop** | 版本跳级/API 废弃 | 组件版本 skew + 日志 | 03 |
| **负载高但不扩容** | CA 配置/配额/污点 | Cluster Autoscaler 日志 | 04 |

---

## FTA 故障树路径映射

| 顶层事件 | 中间事件 | 底事件 | 对应技能 |
|---------|---------|--------|---------|
| TE-C 集群不可用 | IE-C.1 apiserver 异常 | BE-C.1 apiserver 崩溃/OOM | 01 |
| TE-C 集群不可用 | IE-C.2 etcd 异常 | BE-C.2 失去 quorum | 02 |
| TE-C 集群不可用 | IE-C.2 etcd 异常 | BE-C.3 db 配额超限 | 02 |
| TE-C 集群不可用 | IE-C.3 证书失效 | BE-C.4 PKI 过期 | 03 |
| TE-C 集群不可用 | IE-C.4 升级失败 | BE-C.5 版本 skew 违规 | 03 |
| TE-C 容量不足 | IE-C.5 扩缩容异常 | BE-C.6 CA 无法扩容 | 04 |

---

## 版本兼容性矩阵

| 技能 | 适用版本 | 版本敏感点 |
|------|---------|-----------|
| 01 apiserver | 1.18–1.36 | APF（API Priority & Fairness）1.29 GA；`--enable-priority-and-fairness` |
| 02 etcd | etcd 3.4–3.7 | `etcdctl` v3 API 默认；`defrag`/`compaction` 命令全版本通用 |
| 03 证书/升级 | 1.18–1.36 | 版本 skew 策略：kubelet 可落后 apiserver 最多 3 个小版本（1.28+） |
| 04 扩缩容 | CA 1.28+ | Karpenter 作为替代方案；节点池 API 云厂商差异大 |

---

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/技能建设最佳实践.md|技能建设最佳实践（生产标准）]]
- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[26-技能/03-节点/node/README.md|Node 异常诊断技能集]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[26-技能/01-集群运维/kubeadm-fta.md|kubeadm 故障树分析]]

## Related

- [[kube-apiserver]] — API Server
- [[etcd]] — 集群数据存储
- [[kubeadm]] — 集群引导工具
