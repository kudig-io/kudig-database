---
title: 04 - Terway 运维手册 (Operations Manual)
description: '# 04 - Terway 运维手册 (Operations Manual)'
summary: '`terway-cli` 是内嵌在 Terway Pod 中的命令行诊断工具，可直接进入 Pod 执行。'
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- kubelet
- prometheus
- grafana
- cilium
- calico
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 15min
intent_queries:
- Terway 运维手册 (Operations Manual) 是什么
- 如何 Terway 运维手册 (Operations Manual)
trigger_keywords:
- Terway
- 运维手册
- Operations
- Manual
- terway
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 04 - Terway 运维手册 (Operations Manual)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

## 目录

- [1. 健康检查](#1-健康检查)
- [2. GC (垃圾回收) 机制](#2-gc-垃圾回收-机制)
- [3. 监控与告警](#3-监控与告警)
- [4. 升级策略](#4-升级策略)
- [5. 常见故障排查](#5-常见故障排查)
- [6. IP 泄漏紧急处理](#6-ip-泄漏紧急处理)
- [7. 日常巡检清单](#7-日常巡检清单)
- [8. SRE 运维红线](#8-sre-运维红线)
- [附录 A: 错误信息速查目录](#附录-a-错误信息速查目录)
- [9. 交叉引用](#9-交叉引用)

---

## 1. 健康检查

### 1.1 快速诊断命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n kube-system -l app=terway -o wide
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe node <node-name> | grep -A 5 aliyun.com
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system -l app=terway -c terway --tail=100
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ds terway-eniip -n kube-system -o wide
```
### 1.2 terway-cli 诊断工具

`terway-cli` 是内嵌在 Terway Pod 中的命令行诊断工具，可直接进入 Pod 执行。

**查看 IP 分配状态:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show
```
输出包含: 本地 IP 池、已分配 IP、关联 Pod、ENI 辅助 IP 列表。

**查看 ENI 详细信息:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show eni
```
输出包含: 节点上所有 ENI 的 ID、状态、辅助 IP 数量、挂载状态。

**GC 预演 (不实际清理):**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli garbage-collect --dry-run
```
输出候选清理的孤儿 IP 列表，不执行实际释放。用于确认 GC 行为是否符合预期。

**强制同步本地状态:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli sync
```
触发本地 IPAM 与 [[Kubernetes|Kubernetes]]es API|Kubernetes API]] 全量同步，适用于状态不一致场景。

**查看帮助:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli --help
```
### 1.3 完整健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
set -euo pipefail

command -v jq >/dev/null 2>&1 || { echo "错误: 需要 jq 工具，请先安装"; exit 1; }
command -v kubectl >/dev/null 2>&1 || { echo "错误: 需要 kubectl 工具"; exit 1; }

NAMESPACE="kube-system"
EXIT_CODE=0

echo "========================================="
echo " Terway 健康检查"
echo " 时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "========================================="
echo ""

echo "[1/5] 检查 Terway Pod 状态..."
NOT_RUNNING=$(kubectl get pods -n ${NAMESPACE} -l app=terway -o json | \
  jq '[.items[] | select(.status.phase != "Running" or 
    (.status.containerStatuses // [] | any(.ready == false)))] | length')
TOTAL=$(kubectl get pods -n ${NAMESPACE} -l app=terway --no-headers 2>/dev/null | wc -l | tr -d ' ')
if [ "${NOT_RUNNING}" -gt 0 ]; then
  echo "  CRITICAL: ${NOT_RUNNING}/${TOTAL} 个 Terway Pod 不正常"
  kubectl get pods -n ${NAMESPACE} -l app=terway -o wide | grep -v Running || true
  EXIT_CODE=2
else
  echo "  OK: ${TOTAL} 个 Terway Pod 全部 Running"
fi
echo ""

echo "[2/5] 检查 ENI/IP 配额..."
CRITICAL_NODES=""
for NODE in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  ALLOCATED=$(kubectl get node ${NODE} -o json | \
    jq -r '.metadata.annotations["k8s.aliyun.com/allocated-eniips"] // "0/0"')
  echo "  ${NODE}: ${ALLOCATED}"
  USED=$(echo "${ALLOCATED}" | cut -d'/' -f1)
  MAX=$(echo "${ALLOCATED}" | cut -d'/' -f2)
  if [ "${MAX}" != "0" ] && [ "${MAX}" != "" ]; then
    RATIO=$((USED * 100 / MAX))
    if [ "${RATIO}" -gt 90 ]; then
      CRITICAL_NODES="${CRITICAL_NODES} ${NODE}(${RATIO}%)"
    fi
  fi
done
if [ -n "${CRITICAL_NODES}" ]; then
  echo "  WARNING: 以下节点 IP 使用率 >90%: ${CRITICAL_NODES}"
  EXIT_CODE=$((EXIT_CODE > 1 ? EXIT_CODE : 1))
fi
echo ""

echo "[3/5] 检查固定 IP 冲突..."
DUPLICATES=$(kubectl get ipinstances -A -o json | \
  jq -r '.items | group_by(.spec.ip.ipv4) | .[] | select(length > 1) | 
    .[0].spec.ip.ipv4 + " (" + (length | tostring) + " 条记录)"')
if [ -n "${DUPLICATES}" ]; then
  echo "  WARNING: 发现重复 IP 分配:"
  echo "${DUPLICATES}" | sed 's/^/    /'
  EXIT_CODE=$((EXIT_CODE > 1 ? EXIT_CODE : 1))
else
  echo "  OK: 无固定 IP 冲突"
fi
echo ""

echo "[4/5] 检查 VPC 路由..."
ROUTE_COUNT=$(kubectl get nodes -o json | jq '.items | length')
echo "  集群节点数: ${ROUTE_COUNT}"
echo "  提示: 请在阿里云控制台确认 VPC 路由表条目数 >= ${ROUTE_COUNT}"
echo "  路由表配额默认 48 条，超出需提交工单提升"
echo ""

echo "[5/5] 检查最近 10 分钟错误日志..."
ERRORS=$(kubectl logs -n ${NAMESPACE} -l app=terway -c terway \
  --since=10m 2>/dev/null | grep -ciE 'error|fatal|panic' || echo "0")
if [ "${ERRORS}" -gt 0 ]; then
  echo "  WARNING: 最近 10 分钟发现 ${ERRORS} 条错误日志"
  kubectl logs -n ${NAMESPACE} -l app=terway -c terway --since=10m 2>/dev/null | \
    grep -iE 'error|fatal|panic' | tail -5 | sed 's/^/    /'
  EXIT_CODE=$((EXIT_CODE > 1 ? EXIT_CODE : 1))
else
  echo "  OK: 最近 10 分钟无错误日志"
fi
echo ""

echo "========================================="
if [ ${EXIT_CODE} -eq 0 ]; then
  echo " 结果: 全部通过"
elif [ ${EXIT_CODE} -eq 1 ]; then
  echo " 结果: 存在警告，请关注"
else
  echo " 结果: 存在严重问题，请立即处理"
fi
echo "========================================="
exit ${EXIT_CODE}
```
---

## 2. GC (垃圾回收) 机制

> 本节内容综合整理自 [网络/38-terway-gc-mechanism.md](../网络/38-terway-gc-mechanism.md)

### 2.1 为什么需要 GC

在 ENIIP 模式下，Pod 与 VPC 内的 ENI 辅助 IP 直接绑定。当 Pod 被删除、驱逐或异常退出时，理论上其占用的 IP 应当及时归还 IP 池。但在以下场景中，正常的 IP 释放流程可能失效:

| 场景 | 原因 | 后果 |
|:---|:---|:---|
| [[kubelet|kubelet]] 强制驱逐 | 节点压力大，跳过 CNI DEL 回调 | IP 残留在 ENI 辅助 IP 列表 |
| 节点异常重启 | Terway Agent 进程未优雅退出 | 本地 IPAM 状态丢失 |
| Terway Agent 重启/升级 | 内存状态与持久化状态不一致 | 孤儿 IP 无法追踪 |
| CRD Finalizer 阻塞 | PodENI/IPInstance 删除卡住 | IP 永久占用 |
| 阿里云 API 超时/失败 | 网络抖动导致辅助 IP 释放失败 | 云平台记录与本地不一致 |

GC 的核心目标: **周期性对账，发现并清理孤儿资源，确保 IP 池与 ENI 资源的最终一致性。**

### 2.2 设计原则

**最终一致性 (Eventual Consistency):**
Kubernetes Pod 状态与 VPC ENI/IP 实际状态两侧数据源定期对比，差异部分作为 GC 候选。

**安全优先 (Safety First):**
- 多轮确认: IP 必须连续 N 个 GC 周期被标记为孤儿才触发清理
- 宽限期: 新分配 IP 在 grace period 内不参与 GC 判定
- 白名单: ReservedIP / 固定 IP 跳过 GC

**最小影响 (Minimal Impact):**
- GC 操作在 Terway Agent 后台 goroutine 异步执行
- 限速清理: 单次 GC 周期最多清理 N 个资源，避免云 API 雪崩
- 退避策略: 清理失败后指数退避重试

### 2.3 GC 架构

GC 涉及以下核心组件:

| 组件 | 位置 | 职责 |
|:---|:---|:---|
| **GC Controller** | 控制面 | 周期性对账，扫描 CRD 孤儿资源 |
| **ENI Reconciler** | 控制面 | ENI 生命周期管理，空闲 ENI 回收 |
| **IP Reconciler** | 控制面 | IP 生命周期管理，孤儿 IP 清理 |
| **Local IPAM** | 节点 Agent | 本地 IP 分配表维护 |
| **GC Worker** | 节点 Agent | 本地 GC 执行，策略路由清理 |
| **ENI Manager** | 节点 Agent | ENI 辅助 IP 的分配与释放 |

GC 涉及的资源类型:

| 资源类型 | GC 对象 | 清理动作 |
|:---|:---|:---|
| ENI 辅助 IP | 未关联任何 Pod 的辅助 IP | 调用 `UnassignPrivateIpAddresses` 释放 |
| ENI | 无任何辅助 IP 且未被节点使用的 ENI | DetachNetworkInterface + DeleteNetworkInterface |
| PodENI CRD | 对应 Pod 已不存在的 PodENI | 删除 CRD 对象 |
| IPInstance CRD | 对应 Pod 已不存在的 IPInstance | 删除 CRD 对象，释放关联 IP |
| ReservedIP CRD | 超过保留时长的 ReservedIP | 根据 `reclaimPolicy` 释放或保留 |
| 本地 IPAM 缓存 | 与 Pod/CRI 不一致的分配记录 | 清除本地记录 |

### 2.4 GC 触发类型

| 触发方式 | 说明 | 典型场景 |
|:---|:---|:---|
| **周期性定时器 (Primary)** | 每 `gc_min_interval` (默认 300s) 执行一次全量扫描 | 常规运维 |
| **事件驱动 (Reactive)** | Pod 删除时 CNI DEL 回调失败，标记待 GC | CNI DEL 异常 |
| **启动对账 (Startup)** | Terway Agent 启动/重启后立即执行全量对账 | Agent 重启/升级 |
| **池水位驱动 (Pool-driven)** | 空闲 IP 超过 `max_pool_size` 时触发缩容 GC | Pod 大量删除后 |

### 2.5 关键 ConfigMap 参数

**eni-config (节点 Agent 级):**

| 参数 | 默认值 | 说明 | 生产建议 |
|:---|:---|:---|:---|
| `gc_min_interval` | 300 (秒) | 两次 GC 扫描最小间隔 | 大规模集群 (>100 节点) 可调大至 600 |
| `gc_grace_period` | 120 (秒) | 新分配 IP 的 GC 豁免期 | 大镜像/慢启动场景调至 300 |
| `gc_max_cleanup_per_cycle` | 5 | 单次 GC 周期最大清理数量 | 紧急时可临时调至 20 |
| `eni_idle_timeout` | 600 (秒) | ENI 空闲后回收超时 | 频繁扩缩容场景调至 1800 |
| `gc_stale_threshold` | 2 | 孤儿资源需连续被标记次数 | 保持默认即可 |
| `max_pool_size` | 25 | IP 池最大空闲数 | 节点 Pod 密度的 60% |
| `min_pool_size` | 10 | IP 池最小空闲数 | 节点 Pod 密度的 30% |

**terway-controlplane (控制面级):**

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `ipinstance_gc_interval` | 600 (秒) | IPInstance CRD GC 扫描周期 |
| `podeni_gc_interval` | 600 (秒) | PodENI CRD GC 扫描周期 |
| `reservedip_expiry_check_interval` | 3600 (秒) | ReservedIP 过期检查周期 |
| `gc_concurrency` | 2 | 控制面 GC 并发数 |

参数协作关系:

```
gc_min_interval --> 控制扫描频率
       |
gc_grace_period --> 新 IP 豁免 --> GC 扫描判定 --> 标记孤儿
       |                                         |
max_pool_size --> 池上限判定                   gc_stale_threshold (2次)
min_pool_size --> 池下限保护                       |
                                               触发清理动作
gc_max_cleanup_per_cycle --> 单次限额 -----------|
eni_idle_timeout --> ENI 空闲判定 --> ENI 回收 --> 完成
```

### 2.6 手动触发 GC

**方法一: 重启 Terway Pod (触发启动对账)**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
NODE="<node-name>"
TERWAY_POD=$(kubectl get pods -n kube-system -l app=terway \
  --field-selector spec.nodeName=${NODE} -o jsonpath='{.items[0].metadata.name}')
kubectl delete pod -n kube-system ${TERWAY_POD}
```
Terway 重启后会执行一次全量对账 GC (源码中 `wait.PollUntilContextCancel` 的 `immediate=true`)。

**方法二: 通过 terway-cli 手动清理**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli garbage-collect --dry-run
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli garbage-collect
```
**方法三: 手动清理孤儿 IPInstance CRD**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ipinstances -A -o json | jq -r '
  .items[] | 
  select(.spec.pod.name != null) |
  "\(.metadata.name)\t\(.spec.pod.namespace)\t\(.spec.pod.name)"' | \
while IFS=$'\t' read -r name ns pod; do
  if ! kubectl get pod ${pod} -n ${ns} &>/dev/null; then
    echo "ORPHAN: ${name} (was: ${ns}/${pod})"
  fi
done
```
### 2.7 GC 参数调整场景

**场景 A: 加速 GC (IP 泄漏严重时临时调整)**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson |
    .gc_min_interval = 60 |
    .gc_max_cleanup_per_cycle = 20 |
    .gc_grace_period = 60 |
    tostring)' | kubectl apply -f -

kubectl rollout restart ds/terway-eniip -n kube-system
```
**场景 B: 大规模集群优化 (减少 API 压力)**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson |
    .gc_min_interval = 600 |
    .gc_max_cleanup_per_cycle = 3 |
    .eni_idle_timeout = 1800 |
    tostring)' | kubectl apply -f -

kubectl rollout restart ds/terway-eniip -n kube-system
```
| 调整项 | 加速 GC | 大集群优化 |
|:---|:---|:---|
| `gc_min_interval` | 60s | 600s |
| `gc_grace_period` | 60s | 120s (默认) |
| `gc_max_cleanup_per_cycle` | 20 | 3 |
| `eni_idle_timeout` | 600s (默认) | 1800s |

### 2.8 GC 问题速查

| 问题 | 现象 | 处理方案 |
|:---|:---|:---|
| IP 泄漏累积 | 节点可用 IP 持续减少，Pod Pending | 检查 GC 日志，降低 `gc_min_interval`，手动触发 GC |
| GC 误回收 | 正在启动的 Pod IP 被回收 | 增大 `gc_grace_period` 至 300s |
| ENI 反复创删 | 频繁 ENI Attach/Detach | 增大 `eni_idle_timeout` 至 1800s |
| GC 执行失败 | 日志出现 `GC.*failed` | 检查 RAM 权限，增大 `gc_max_backoff` |
| CRD Finalizer 阻塞 | IPInstance/PodENI 无法删除 | 重启 Controller，必要时手动移除 Finalizer |
| 固定 IP 被意外回收 | StatefulSet Pod 重建后 IP 变化 | 检查 ReservedIP `retention.duration`，确认 `reclaimPolicy: Retain` |

### 2.9 GC 源码级执行流程

> 本节内容提取自 [网络/38-terway-gc-mechanism.md](../网络/38-terway-gc-mechanism.md) 第 4 节。

#### 2.9.1 源码默认值表

| 常量 | 源码位置 | 默认值 | 说明 |
|:---|:---|:---|:---|
| `gcPeriod` | `daemon/daemon.go` | 5 分钟 | GC 扫描周期，硬编码常量 |
| `listTimeout` | `daemon/daemon.go` | 60 秒 | List 操作超时 |
| `defaultStickTimeForSts` | `pkg/k8s/k8s.go` | 5 分钟 | StatefulSet Pod 删除后 IP 保留时间 |
| 新 Pod 跳过阈值 | `gcPods()` | 2 分钟 | `time.Since(createTime) < 2min` 的 Pod 跳过 |
| resourceDB 路径 | `daemon/daemon.go` | `/var/lib/cni/terway/ResRelation.db` | BoltDB 持久化路径 |
| 泄漏规则清理开关 | `gcPods()` | `TERWAY_GC_RULES=true` | 环境变量控制，默认关闭，仅首次执行一次 |

> **关键细节**: `gcPeriod` 是硬编码常量，不可通过 ConfigMap 动态调整；`wait.PollUntilContextCancel` 的 `immediate=true`，Agent 启动后立即执行第一次 GC；`gcPods()` 持有写锁时会阻塞 AllocIP/ReleaseIP。

#### 2.9.2 gcPods() 4 阶段流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│           gcPods() 完整执行流程 (源码: daemon/daemon.go)                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   Phase 1: 获取写锁 + 数据采集                                              │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  n.Lock()                   // 持有写锁，阻塞 AllocIP/ReleaseIP │       │
│   │  ① k8s.GetLocalPods()        ② resourceDB.List()               │       │
│   │     (从本节点 kubelet 获取)     (从 BoltDB 获取)                │       │
│   │     ↓                            ↓                              │       │
│   │  exist = map[podID]bool       podResources = [...]              │       │
│   │  existIPs = sets.Set[string]  (含 Resources, PodInfo, NetConf)  │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   Phase 2: 遍历对账 (for podRes in podResources)                            │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  ① Pod 仍在运行 (exist[podID] == true):                        │       │
│   │     if createTime < 2min ago → skip (新 Pod 不同步规则)          │       │
│   │     else → ruleSync(ctx, podRes)  // 同步策略路由规则            │       │
│   │  ② Pod 本节点不存在 (exist[podID] == false):                    │       │
│   │     k8s.PodExist() → 再次通过 API Server 确认                    │       │
│   │     ├─ Pod 仍存在 → skip (可能在其他节点)                       │       │
│   │     └─ Pod 真正不存在 → 触发清理                                │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   Phase 3: 执行清理 (对已确认不存在的 Pod)                                  │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  for resource in podRes.Resources:                              │       │
│   │    1. gcPolicyRoutes(mac, containerIP)  → 清理策略路由          │       │
│   │    2. eniMgr.Release(cni, resource)     → 释放网络资源          │       │
│   │    3. deletePodResource(podID)           → 从 resourceDB 删除   │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   Phase 4: 可选泄漏规则清理                                                 │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  if os.Getenv("TERWAY_GC_RULES") == "true":                    │       │
│   │    gcRulesOnce.Do(func() {                                      │       │
│   │      gcLeakedRules(existIPs)    // 仅执行一次 (sync.Once)       │       │
│   │    })                                                           │       │
│   │  cleanRuntimeNode(ctx, uidInLocal)  // 清理节点运行时记录       │       │
│   │  n.Unlock()                         // 释放写锁                 │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 2.9.3 ENI GC 决策树

```
                    扫描节点上所有挂载的 ENI
                              │
                              ▼
                    ┌────────────────────┐     Yes     ┌────────────────────┐
                    │ ENI 有辅助 IP ?    │────────────►│ 跳过，交给 IP GC   │
                    └───────┬────────────┘             └────────────────────┘
                            │ No
                            ▼
                    ┌────────────────────┐     No      ┌────────────────────┐
                    │ 空闲时间 >         │────────────►│ 保留，下次再检查   │
                    │ eni_idle_timeout ? │             └────────────────────┘
                    └───────┬────────────┘
                            │ Yes
                            ▼
                    ┌────────────────────┐     No      ┌────────────────────┐
                    │ 剩余 ENI > 1 ?    │────────────►│ 保留最后一个 ENI   │
                    │ (保留主 ENI)       │             │ 确保节点可用       │
                    └───────┬────────────┘             └────────────────────┘
                            │ Yes
                            ▼
                    ┌────────────────────────────┐
                    │ DetachNetworkInterface      │
                    │ → 等待 Detach 完成          │
                    │ → DeleteNetworkInterface    │
                    │ → 清理 NodeNetworking CRD   │
                    └────────────────────────────┘
```

#### 2.9.4 CRD GC 流程

| CRD 类型 | GC 触发条件 | 清理动作 |
|:---|:---|:---|
| **IPInstance** | Pod UID 不匹配 (Pod 被重建) 或 Pod 不存在 | 移除 Finalizer → `UnassignPrivateIpAddresses` → 删除 CRD |
| **PodENI** | Owner Pod 不存在 | 释放关联 ENI 资源 → 移除 Finalizer → 删除 CRD |
| **ReservedIP** | Pod 不存在 + `retention.enabled` + 超过 `retention.duration` | 根据 `reclaimPolicy`: Retain 保留 / Delete 释放 IP 并删除 CRD |

#### 2.9.5 IP 孤儿判定 5 条件

IP 被判定为孤儿需**同时满足**以下所有条件:

1. **分配时间 > gc_grace_period** — 已过宽限期，排除新分配 IP
2. **无 Running Pod 引用该 IP** — 通过 Kubernetes API + CRI 双重确认
3. **不在 ReservedIP 白名单中** — 固定 IP / ReservedIP 跳过 GC
4. **连续 gc_stale_threshold 次 GC 周期被标记** — 防止瞬态不一致导致误清理
5. **IP 不属于正在创建中的 Pod** — 检查 Pod `phase != Pending with scheduled`

#### 2.9.6 GC 清理优先级

```
高 ← ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ → 低

  优先级 1                   优先级 2
  标记次数最多的孤儿 IP       分配时间最早的孤儿 IP
  (最大 gc_mark_count)        (最久未使用)

  优先级 3                   优先级 4
  已无辅助 IP 的空闲 ENI      池中超限的多余空闲 IP
  (资源浪费最大)               (缩容回收)
```

#### 2.9.7 GC 相关监控指标

| 指标 | 类型 | 说明 | 告警阈值建议 |
|:---|:---|:---|:---|
| `terway_gc_cleaned_ips` | Counter | GC 已清理的 IP 总数 | - |
| `terway_ip_pool_available` | Gauge | IP 池当前可用 IP 数 | < min_pool_size |
| `terway_eni_count` | Gauge | 节点当前 ENI 数量 | 接近实例上限 |
| `terway_gc_duration_seconds` | Histogram | GC 执行耗时 | P99 > 30s |

#### 2.9.8 TerwayGCSlowExecution 告警规则

```yaml
- alert: TerwayGCSlowExecution
  expr: histogram_quantile(0.99, terway_gc_duration_seconds_bucket) > 30
  for: 10m
  labels:
    severity: warning
  annotations:
    summary: "节点 {{ $labels.node }} GC 执行耗时过长"
    description: >-
      GC P99 延迟 {{ $value }}s，可能受 API 限流影响。
      检查阿里云 API 调用延迟和节点网络连通性。
    runbook_url: "https://internal-docs/terway-ops#gc-slow"
```

---

## 3. 监控与告警

### 3.1 核心监控指标

| 指标 | 类型 | 说明 | 告警阈值建议 |
|:---|:---|:---|:---|
| `terway_alloc_ip_duration_ms` | Histogram | IP 分配耗时 (ms) | > 5000ms |
| `aliyun_terway_allocated_ip` | Gauge | 节点已分配 IP 数 | 配合 `ip_max` 算使用率 |
| `aliyun_terway_ip_max` | Gauge | 节点 IP 上限 | - |
| `aliyun_terway_allocated_eni` | Gauge | 节点已分配 ENI 数 | 配合 `eni_max` 算使用率 |
| `aliyun_terway_eni_max` | Gauge | 节点 ENI 上限 | - |
| `terway_pod_allocate_duration_seconds` | Histogram | Pod 创建网络耗时 | P99 > 30s |
| `terway_gc_total` | Counter | GC 执行总次数 | - |
| `terway_gc_duration_seconds` | Histogram | GC 执行耗时 | P99 > 30s |
| `terway_gc_orphan_ips` | Gauge | 当前孤儿 IP 数 | > 10 |
| `terway_gc_errors_total` | Counter | GC 失败次数 | 5 分钟内 > 3 |

### 3.2 PrometheusRule 完整告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-alerts
  namespace: monitoring
spec:
  groups:
    - name: terway
      rules:
        - alert: TerwayENIQuotaExhausted
          expr: aliyun_terway_allocated_eni / aliyun_terway_eni_max > 0.85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} ENI 使用率超过 85%"
            description: >-
              节点 {{ $labels.node }} ENI 使用率 {{ $value | humanizePercentage }}，
              接近配额上限。新 Pod 可能无法获得网络资源。
            runbook_url: "https://internal-docs/terway-ops#eni-quota"

        - alert: TerwayIPPoolExhausted
          expr: aliyun_terway_allocated_ip / aliyun_terway_ip_max > 0.9
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "节点 {{ $labels.node }} IP 池使用率超过 90%"
            description: >-
              节点 {{ $labels.node }} IP 使用率 {{ $value | humanizePercentage }}，
              即将耗尽。需立即排查 IP 泄漏或扩容 vSwitch。
            runbook_url: "https://internal-docs/terway-ops#ip-exhausted"

        - alert: TerwayPodAllocationSlow
          expr: histogram_quantile(0.99, rate(terway_pod_allocate_duration_seconds_bucket[5m])) > 30
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} Pod IP 分配 P99 延迟超过 30 秒"
            description: >-
              节点 {{ $labels.node }} Pod 网络分配 P99 延迟 {{ $value }}s，
              可能原因: OpenAPI 限流、IP 池不足、ENI 配额耗尽。
            runbook_url: "https://internal-docs/terway-ops#alloc-slow"
```

### 3.3 GC 专项告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-gc-alerts
  namespace: monitoring
spec:
  groups:
    - name: terway-gc
      rules:
        - alert: TerwayOrphanIPsAccumulating
          expr: terway_gc_orphan_ips > 10
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} 存在 {{ $value }} 个孤儿 IP"
            description: "孤儿 IP 持续累积，GC 可能未正常工作。"

        - alert: TerwayGCFailure
          expr: increase(terway_gc_errors_total[10m]) > 5
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} Terway GC 持续失败"
            description: "10 分钟内 GC 失败 {{ $value }} 次，请检查 API 权限和网络。"
```

### 3.5 Grafana 仪表盘模板

以下仪表盘覆盖 Terway 核心运维指标: ENI/IP 使用率与分配延迟、GC 清理与耗时、API 调用延迟与 CNI 错误率。导入后可通过 `node` 变量筛选特定节点。

```json
{
  "annotations": { "list": [] },
  "description": "Terway CNI 网络插件运维概览: ENI/IP 使用率、分配延迟、GC 指标、API 调用与错误率",
  "editable": true,
  "fiscalYearStartMonth": 0,
  "graphTooltip": 1,
  "id": null,
  "links": [],
  "panels": [
    {
      "collapsed": false,
      "gridPos": { "h": 1, "w": 24, "x": 0, "y": 0 },
      "id": 1,
      "panels": [],
      "title": "Terway Overview",
      "type": "row"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": {
        "defaults": {
          "mappings": [],
          "max": 1, "min": 0,
          "thresholds": {
            "steps": [
              { "color": "green", "value": null },
              { "color": "yellow", "value": 0.7 },
              { "color": "red", "value": 0.9 }
            ]
          },
          "unit": "percentunit"
        }
      },
      "gridPos": { "h": 4, "w": 6, "x": 0, "y": 1 },
      "id": 2,
      "options": { "colorMode": "value", "graphMode": "area", "justifyMode": "auto" },
      "targets": [{ "expr": "aliyun_terway_allocated_eni / aliyun_terway_eni_max", "legendFormat": "{{node}}" }],
      "title": "ENI 使用率",
      "type": "stat"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": {
        "defaults": {
          "mappings": [],
          "max": 1, "min": 0,
          "thresholds": {
            "steps": [
              { "color": "green", "value": null },
              { "color": "yellow", "value": 0.7 },
              { "color": "red", "value": 0.9 }
            ]
          },
          "unit": "percentunit"
        }
      },
      "gridPos": { "h": 4, "w": 6, "x": 6, "y": 1 },
      "id": 3,
      "options": { "colorMode": "value", "graphMode": "area", "justifyMode": "auto" },
      "targets": [{ "expr": "aliyun_terway_allocated_ip / aliyun_terway_ip_max", "legendFormat": "{{node}}" }],
      "title": "IP 使用率",
      "type": "stat"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": { "unit": "s" }, "overrides": [] },
      "gridPos": { "h": 8, "w": 12, "x": 12, "y": 1 },
      "id": 4,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "histogram_quantile(0.99, sum(rate(terway_pod_allocate_duration_seconds_bucket{node=~\"$node\"}[5m])) by (le, node))",
        "legendFormat": "{{node}} p99"
      }],
      "title": "IP 分配 P99 延迟",
      "type": "timeseries"
    },
    {
      "collapsed": false,
      "gridPos": { "h": 1, "w": 24, "x": 0, "y": 9 },
      "id": 10,
      "panels": [],
      "title": "GC Metrics",
      "type": "row"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": { "unit": "ops" }, "overrides": [] },
      "gridPos": { "h": 8, "w": 8, "x": 0, "y": 10 },
      "id": 11,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "rate(terway_gc_cleaned_ips_total{node=~\"$node\"}[5m])",
        "legendFormat": "{{node}}"
      }],
      "title": "GC 清理 IP 数",
      "type": "timeseries"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": {}, "overrides": [] },
      "gridPos": { "h": 8, "w": 8, "x": 8, "y": 10 },
      "id": 12,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "terway_ip_pool_available{node=~\"$node\"}",
        "legendFormat": "{{node}}"
      }],
      "title": "IP 池可用数",
      "type": "timeseries"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": { "unit": "s" }, "overrides": [] },
      "gridPos": { "h": 8, "w": 8, "x": 16, "y": 10 },
      "id": 13,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "histogram_quantile(0.99, sum(rate(terway_gc_duration_seconds_bucket{node=~\"$node\"}[5m])) by (le, node))",
        "legendFormat": "{{node}} p99"
      }],
      "title": "GC 执行耗时 P99",
      "type": "timeseries"
    },
    {
      "collapsed": false,
      "gridPos": { "h": 1, "w": 24, "x": 0, "y": 18 },
      "id": 20,
      "panels": [],
      "title": "API & Error Metrics",
      "type": "row"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": { "unit": "s" }, "overrides": [] },
      "gridPos": { "h": 8, "w": 8, "x": 0, "y": 19 },
      "id": 21,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "histogram_quantile(0.99, sum(rate(terway_api_call_duration_seconds_bucket{node=~\"$node\"}[5m])) by (le, node, api))",
        "legendFormat": "{{node}} {{api}} p99"
      }],
      "title": "API 调用延迟",
      "type": "timeseries"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": { "unit": "ops" }, "overrides": [] },
      "gridPos": { "h": 8, "w": 8, "x": 8, "y": 19 },
      "id": 22,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "rate(terway_cni_error_total{node=~\"$node\"}[5m])",
        "legendFormat": "{{node}}"
      }],
      "title": "CNI 操作失败率",
      "type": "timeseries"
    },
    {
      "datasource": { "type": "prometheus", "uid": "${datasource}" },
      "fieldConfig": { "defaults": {}, "overrides": [] },
      "gridPos": { "h": 8, "w": 8, "x": 16, "y": 19 },
      "id": 23,
      "options": { "legend": { "displayMode": "list", "placement": "bottom" }, "tooltip": { "mode": "multi" } },
      "targets": [{
        "expr": "terway_eni_count{node=~\"$node\"}",
        "legendFormat": "{{node}}"
      }],
      "title": "ENI 数量",
      "type": "timeseries"
    }
  ],
  "refresh": "30s",
  "schemaVersion": 38,
  "style": "dark",
  "tags": ["terway", "network", "cni"],
  "templating": {
    "list": [
      {
        "current": { "selected": true, "text": "Prometheus", "value": "Prometheus" },
        "hide": 0,
        "includeAll": false,
        "name": "datasource",
        "options": [],
        "query": "prometheus",
        "type": "datasource"
      },
      {
        "allValue": ".*",
        "current": { "selected": true, "text": "All", "value": "$__all" },
        "datasource": { "type": "prometheus", "uid": "${datasource}" },
        "definition": "label_values(aliyun_terway_allocated_eni, node)",
        "hide": 0,
        "includeAll": true,
        "multi": true,
        "name": "node",
        "options": [],
        "query": "label_values(aliyun_terway_allocated_eni, node)",
        "refresh": 2,
        "type": "query"
      }
    ]
  },
  "time": { "from": "now-1h", "to": "now" },
  "timepicker": {},
  "timezone": "",
  "title": "Terway Network Overview",
  "uid": "terway-overview",
  "version": 1
}
```

**导入方式:**

方式一 — Grafana UI 导入: 进入 **Dashboards → Import → Import via panel json**，粘贴上方 JSON 后点击 Load。

方式二 — 通过 ConfigMap 存储并挂载 (Grafana sidecar 自动发现):

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create configmap terway-grafana-dashboard \
  --from-literal='terway-overview.json=<粘贴上方 JSON 内容>' \
  -n monitoring

kubectl label configmap terway-grafana-dashboard \
  grafana_dashboard=1 -n monitoring
```
---

## 4. 升级策略

### 4.1 查看当前版本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ds -n kube-system terway-eniip -o jsonpath='{.spec.template.spec.containers[?(@.name=="terway")].image}'
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].spec.containers[?(@.name=="terway")].image}'
```
### 4.2 滚动升级

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl set image ds/terway-eniip -n kube-system \
  terway=registry-vpc.cn-hangzhou.aliyuncs.com/acs/terway:v1.5.6

kubectl rollout status ds/terway-eniip -n kube-system --timeout=300s
```
DaemonSet 滚动升级策略为逐节点更新，每节点 Terway Pod 重建后会触发启动对账 GC。

### 4.3 回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout history ds/terway-eniip -n kube-system
kubectl rollout undo ds/terway-eniip -n kube-system
kubectl rollout status ds/terway-eniip -n kube-system
```
### 4.4 升级前检查清单

| 检查项 | 命令 | 通过标准 |
|:---|:---|:---|
| vSwitch IP 余量 | 阿里云控制台 > VPC > 交换机 > 可用 IP 数 | 每个交换机可用 IP >= 节点数 * max_pool_size |
| Terway 版本兼容性 | 查看 Release Notes | 确认目标版本与 ACK 集群版本兼容 |
| 备份 ConfigMap | `kubectl get cm eni-config -n kube-system -o yaml > eni-config-backup.yaml` | 备份文件已保存 |
| Terway Pod 全部 Running | `kubectl get pods -n kube-system -l app=terway` | 无非 Running Pod |
| 无 Finalizer 阻塞 CRD | 见 2.8 节 CRD Finalizer 检查命令 | 无被阻塞的 IPInstance/PodENI |
| 当前 IP 使用率 | `kubectl describe nodes | grep aliyun.com` | 所有节点 < 85% |

---

## 5. 常见故障排查

### 5.1 问题现象速查表

| 现象 | 常见原因 | 快速排查命令 |
|:---|:---|:---|
| Pod ContainerCreating 卡住 | IP 池耗尽 / ENI 配额不足 | `terway-cli show` + `describe node | grep aliyun.com` |
| 跨节点 Pod 不通 | 安全组规则 / VPC 路由缺失 | 阿里云控制台检查安全组 + 路由表 |
| NetworkPolicy 不生效 | 未启用策略引擎 / 配置错误 | 检查 eni-config `policy` 字段; `iptables -L -n` |
| ENI 绑定失败 | ECS 实例配额 / 规格限制 | 阿里云控制台查看 ENI 配额; `describe node` |
| IP 泄漏 (可用 IP 持续减少) | GC 未正常回收 / Finalizer 阻塞 | `terway-cli garbage-collect --dry-run` |
| Pod 获得错误 IP / 通信异常 | 固定 IP 冲突 / vSwitch 配置错误 | `kubectl get ipinstances -A` 检查重复 IP |
| Terway Pod CrashLoopBackOff | 配置错误 / 权限不足 / 内核不兼容 | `kubectl logs -n kube-system <pod> --previous` |
| Pod 分配 IP 极慢 | OpenAPI 限流 / vSwitch IP 不足 | 检查 `terway_alloc_ip_duration_ms` 指标 |

### 5.2 排障流程 (5 步法)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
Step 1: 检查 Terway Pod 状态
  kubectl get pods -n kube-system -l app=terway -o wide
  --> 如有非 Running Pod: kubectl describe / logs --previous

Step 2: 查看 Terway 日志
  kubectl logs -n kube-system <terway-pod> -c terway --tail=500
  --> 关注关键字: error, failed, timeout, quota exceeded

Step 3: 检查节点 ENI/IP 配额
  kubectl describe node <node> | grep -A 10 aliyun.com
  --> 确认 allocated/max 比率，是否接近上限

Step 4: 检查 VPC 路由表和安全组
  阿里云控制台:
  - VPC 路由表: 确认每个节点 CIDR 有对应路由条目
  - 安全组: 确认入/出规则允许 Pod CIDR 互通

Step 5: 运行 terway-cli 诊断
  kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show
  kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show eni
  kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli garbage-collect --dry-run
```
### 5.3 IP 分配失败

#### 决策树

```
# 🟢 低风险：只读/信息收集，通常无副作用
IP 分配失败
  |
  +-- ENI 配额耗尽?
  |     +-- 是: 检查 ECS 实例规格支持的 ENI 数
  |     |        考虑升级实例规格或切换至 ENIIP 模式
  |     +-- 否: 继续
  |
  +-- IP 池耗尽?
  |     +-- 是: 检查 vSwitch 可用 IP 数
  |     |        检查是否有 IP 泄漏 (terway-cli show)
  |     |        增加 vSwitch 或扩展 CIDR
  |     +-- 否: 继续
  |
  +-- 固定 IP 冲突?
  |     +-- 是: 检查 IPInstance CRD 是否有重复 IP
  |     |        kubectl get ipinstances -A -o json | jq 'group_by(.spec.ip.ipv4) | select(length>1)'
  |     +-- 否: 继续
  |
  +-- OpenAPI 调用失败?
        +-- 是: 检查 RAM 角色权限
        |        检查 API 限流 (阿里云控制台 > OpenAPI 调用统计)
        |        检查节点到 VPC API 网络连通性
        +-- 否: 查看 Terway 日志获取具体错误信息
```
#### 子场景详解

**ENI 配额耗尽:**

| 检查项 | 命令/操作 | 风险等级 |
|:---|:---|:---|
| 查看当前 ENI 使用 | `terway-cli show eni` | 无风险 |
| 查看 ECS 规格 ENI 上限 | 阿里云控制台 > 实例规格 | 无风险 |
| 升级实例规格 | 修改节点池实例规格 | 中风险 (需排水) |
| 清理空闲 ENI | 降低 `eni_idle_timeout`，手动触发 GC | 低风险 |

**IP 池耗尽:**

| 检查项 | 命令/操作 | 风险等级 |
|:---|:---|:---|
| 查看 vSwitch 可用 IP | 阿里云控制台 > VPC > 交换机 | 无风险 |
| 增加 vSwitch | 修改 eni-config 添加新 vSwitch | 中风险 |
| 扩展 CIDR | 在 VPC 中新增网段并创建交换机 | 高风险 (需规划) |
| 检查 IP 泄漏 | `terway-cli garbage-collect --dry-run` | 无风险 |

**固定 IP 冲突:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ipinstances -A -o json | jq -r '
  .items | group_by(.spec.ip.ipv4) | .[] | select(length > 1) |
  "DUPLICATE IP: \(.[0].spec.ip.ipv4) -> instances: \([.[].metadata.name] | join(", "))"'
```
解决方案: 确认哪个 IPInstance 属于已终止的 Pod，手动删除孤儿 CRD。

**OpenAPI 调用失败:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system <terway-pod> -c terway --tail=200 | grep -iE 'api.*error|throttl|permission|forbidden'
```
常见原因: RAM 角色权限不足、API 限流、网络抖动。

### 5.4 跨节点通信失败

**排查步骤:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
跨节点 Pod 不通
  |
  +-- 1. 确认 Pod IP 是否正确分配
  |     kubectl get pod -o wide
  |     从源 Pod ping 目标 Pod IP
  |
  +-- 2. 检查 VPC 路由表
  |     阿里云控制台 > VPC > 路由表
  |     确认目标节点 CIDR 指向正确的下一跳 (ENI 或 ECS)
  |
  +-- 3. 检查安全组规则
  |     确认安全组入规则允许 Pod CIDR 互通
  |     ENIIP 模式: 检查 ENI 关联的安全组
  |
  +-- 4. 检查路由同步状态
        kubectl exec -n kube-system <terway-pod> -- terway-cli show
        查看策略路由是否正确配置
```
**VPC 路由检查要点:**

- 路由表条目数是否达到配额上限 (默认 48 条)
- 每个节点 Pod CIDR 是否有对应路由
- 路由下一跳是否指向正确的 ECS 实例或 ENI

**安全组检查要点:**

- 入规则: 允许 Pod CIDR 段的 TCP/UDP/ICMP
- 出规则: 允许所有流量 (或至少允许 Pod CIDR)
- ENI 关联的安全组是否正确 (ENIIP 模式)

### 5.5 NetworkPolicy 问题

| 现象 | 可能原因 | 解决方案 |
|:---|:---|:---|
| NetworkPolicy 配置后无效果 | eni-config 中 `policy` 未启用 | 设置 `"policy": true` 并重启 Terway |
| 部分流量未被拦截 | selector 匹配范围不正确 | 检查 podSelector/namespaceSelector |
| 配置后全部不通 | default deny 规则过于严格 | 逐步放行，先 allow-all 再收窄 |
| 性能下降明显 | NetworkPolicy 数量过多 | 合并规则，减少 selector 复杂度 |

### 5.6 性能问题

| 症状 | 排查方向 | 参考指标 |
|:---|:---|:---|
| Pod 创建慢 | IP 分配耗时 | `terway_alloc_ip_duration_ms` > 5s |
| 网络吞吐低 | 网卡多队列未开启 | `ethtool -l eth0` 检查 |
| 跨节点延迟高 | VPC 路由/安全组 | 同节点 vs 跨节点对比 |
| CPU 软中断高 | 网卡中断绑核 | `cat /proc/interrupts` |

> 详细性能调优参考: [06-performance.md](./06-performance.md)

### 5.7 Calico + Terway 集成排查

当集群同时使用 Terway CNI 和 Calico NetworkPolicy 引擎时，可能出现兼容性问题。

**检查步骤:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Calico 组件状态
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide
kubectl get pods -n kube-system -l k8s-app=calico-kube-controllers -o wide

# 2. 检查 Felix 配置
kubectl get configmap -n kube-system calico-config -o yaml
kubectl logs -n kube-system <calico-node-pod> -c calico-node | grep -i "policy"

# 3. 检查 Terway 模式
kubectl get cm eni-config -n kube-system -o jsonpath='{.data.eni_conf}' | jq .network_type
```
**已知问题: ENI 模式 + Calico 无法阻断同节点 Pod 流量**

ENI 独占模式下，同节点 Pod 间流量直接通过 ENI 转发，不经过宿主机网络栈，Calico 的 iptables 规则无法拦截。

**解决方案:**

| 方案 | 要求 | 说明 |
|:---|:---|:---|
| 升级 Terway + Calico | Terway v1.4+ 且 Calico v3.24+ | 新版本通过 hook 点修复 |
| 启用 eBPF 数据面 | Terway v1.5+ + Cilium 1.14+ | eBPF 在网卡层拦截，绕过限制 |
| 切换至 ENIIP 模式 | Terway ENIIP 模式 | 流量经过宿主机 veth pair，iptables 可拦截 |

**安全组 + NetworkPolicy 优先级矩阵:**

| 流量方向 | 安全组 | NetworkPolicy | 实际效果 | 说明 |
|:---|:---|:---|:---|:---|
| 入站 | 拒绝 | 允许 | **拒绝** | 安全组在网络层先于策略引擎 |
| 入站 | 允许 | 拒绝 | **拒绝** | NetworkPolicy 在协议栈上层拦截 |
| 入站 | 允许 | 允许 | **允许** | 两层均放通 |
| 出站 | 拒绝 | 允许 | **拒绝** | 安全组优先 |
| 出站 | 允许 | 拒绝 | **拒绝** | 策略生效 |

> **排查建议**: 同时检查安全组规则和 NetworkPolicy 规则，确保无冲突。

---

## 6. IP 泄漏紧急处理

### 6.1 紧急响应流程

```
发现 IP 泄漏 (告警 / 巡检发现)
  |
  +-- Step 1: 评估影响范围
  |     - 有多少节点受影响?
  |     - IP 使用率是否超过 90%?
  |     - 是否有 Pod 因 IP 不足而 Pending?
  |
  +-- Step 2: 止损 - 加速 GC
  |     - 调低 gc_min_interval 至 60s
  |     - 调高 gc_max_cleanup_per_cycle 至 20
  |     - 重启受影响节点的 Terway Pod
  |
  +-- Step 3: 手动清理 (如 GC 仍不够)
  |     - terway-cli garbage-collect
  |     - 手动删除孤儿 IPInstance CRD
  |     - 手动移除阻塞的 Finalizer
  |
  +-- Step 4: 根因分析
  |     - 检查 GC 日志为何未正常回收
  |     - 是否有 Finalizer 阻塞?
  |     - 是否 OpenAPI 调用失败?
  |
  +-- Step 5: 恢复与复盘
        - 恢复 GC 参数为默认值
        - 更新告警阈值
        - 编写事故报告
```

### 6.2 紧急处理脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
set -euo pipefail

echo "=== Terway IP 泄漏紧急处理 ==="
echo "时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

echo "[Step 1] 确认 Pod Pending 规模..."
PENDING_COUNT=$(kubectl get pods -A --field-selector status.phase=Pending \
  -o json 2>/dev/null | jq '[.items[] | select(.status.conditions[]? |
  select(.reason == "Unschedulable" and (.message | test("IP|ip|ENI"))))] | length' || echo 0)
echo "  IP 不足导致 Pending 的 Pod 数: ${PENDING_COUNT}"
echo ""

echo "[Step 2] 各节点 IP 使用情况..."
kubectl get nodes -o json | jq -r '
  .items[] |
  "\(.metadata.name)\t\(.metadata.annotations["k8s.aliyun.com/allocated-eniips"] // "N/A")"' | \
  column -t -s $'\t'
echo ""

echo "[Step 3] 识别泄漏节点..."
kubectl get ipinstances -A -o json 2>/dev/null | jq -r '
  [.items[] | {node: .status.nodeName, ip: .spec.ip.ipv4, pod: .spec.pod.name, ns: .spec.pod.namespace}] |
  group_by(.node) | .[] |
  {node: .[0].node, total: length} |
  "\(.node)\tTotal: \(.total)"' 2>/dev/null | sort -t$'\t' -k2 -rn | column -t -s $'\t' || echo "  无法获取 IPInstance 数据"
echo ""

echo "[Step 4] 临时加速 GC 命令 (请手动执行)..."
echo ""
echo "  # 临时加速 GC"
echo "  kubectl get cm eni-config -n kube-system -o json | \\"
echo "    jq '.data.eni_conf = (.data.eni_conf | fromjson |"
echo "      .gc_min_interval = 60 | .gc_max_cleanup_per_cycle = 20 | tostring)' | \\"
echo "    kubectl apply -f -"
echo "  kubectl rollout restart ds/terway-eniip -n kube-system"
echo ""
echo "  # 问题解决后恢复默认值"
echo "  kubectl get cm eni-config -n kube-system -o json | \\"
echo "    jq '.data.eni_conf = (.data.eni_conf | fromjson |"
echo "      .gc_min_interval = 300 | .gc_max_cleanup_per_cycle = 5 | tostring)' | \\"
echo "    kubectl apply -f -"
echo "  kubectl rollout restart ds/terway-eniip -n kube-system"
echo ""

echo "[Step 5] 检查 Finalizer 阻塞..."
BLOCKED=$(kubectl get ipinstances -A -o json 2>/dev/null | jq -r '
  [.items[] | select(.metadata.deletionTimestamp != null)] | length' || echo "N/A")
echo "  被删除但未清除的 IPInstance: ${BLOCKED}"
echo ""
echo "=== 处理完成 ==="
```
### 6.3 Finalizer 阻塞处理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get ipinstances -A -o json | jq -r '
  .items[] | select(.metadata.deletionTimestamp != null) |
  "\(.metadata.name)\t\(.metadata.finalizers | join(","))"' | \
  column -t -s $'\t'

kubectl patch ipinstance <name> --type='json' \
  -p='[{"op": "remove", "path": "/metadata/finalizers"}]'
```
### 6.4 CRD Finalizer 阻塞深度排查

#### 检测方法

CRD Finalizer 阻塞表现为 IPInstance 或 PodENI 长期处于 `Terminating` 状态，导致关联 IP 无法释放。

**批量检测命令:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找所有 Terminating 状态的 IPInstance (可能被 Finalizer 阻塞)
kubectl get ipinstances -A -o json | jq -r '
  .items[] | select(.metadata.deletionTimestamp != null) |
  "\(.metadata.name)\tNS:\(.metadata.namespace)\tDeleting since: \(.metadata.deletionTimestamp)\tFinalizers: \(.metadata.finalizers | join(","))"' | \
  column -t -s $'\t'

# 查找所有 Terminating 状态的 PodENI
kubectl get podenis -A -o json | jq -r '
  .items[] | select(.metadata.deletionTimestamp != null) |
  "\(.metadata.namespace)/\(.metadata.name)\tFinalizers: \(.metadata.finalizers | join(","))"' | \
  column -t -s $'\t'

# 统计被阻塞的 CRD 数量
echo "阻塞的 IPInstance: $(kubectl get ipinstances -A -o json | jq '[.items[] | select(.metadata.deletionTimestamp != null)] | length')"
echo "阻塞的 PodENI: $(kubectl get podenis -A -o json | jq '[.items[] | select(.metadata.deletionTimestamp != null)] | length')"
```
**安全 Finalizer 移除步骤:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 确认关联的 Pod 已真正不存在
IPINSTANCE_NAME="<name>"
POD_NAME=$(kubectl get ipinstance ${IPINSTANCE_NAME} -o jsonpath='{.spec.pod.name}')
POD_NS=$(kubectl get ipinstance ${IPINSTANCE_NAME} -o jsonpath='{.spec.pod.namespace}')
kubectl get pod ${POD_NAME} -n ${POD_NS} 2>&1 | grep "NotFound" || echo "WARNING: Pod still exists!"

# Step 2: 确认关联 IP 已在阿里云释放 (防止双重释放)
# 登录阿里云控制台 > VPC > 弹性网卡 > 查看 IP 是否已释放

# Step 3: 移除 Finalizer
kubectl patch ipinstance ${IPINSTANCE_NAME} --type='json' \
  -p='[{"op": "remove", "path": "/metadata/finalizers"}]'

# Step 4: 确认 CRD 已被删除
kubectl get ipinstance ${IPINSTANCE_NAME} 2>&1 | grep "NotFound" || echo "WARNING: CRD still exists"
```
**批量安全移除 (谨慎使用):**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 批量移除所有被阻塞超过 1 小时的 IPInstance Finalizer
kubectl get ipinstances -A -o json | jq -r '
  .items[] | select(.metadata.deletionTimestamp != null) |
  select((.metadata.deletionTimestamp | sub("\\.[0-9]+Z$"; "Z") | strptime("%Y-%m-%dT%H:%M:%SZ") | mktime) < (now - 3600)) |
  .metadata.name' | while read name; do
    echo "Removing finalizer for: ${name}"
    kubectl patch ipinstance ${name} --type='json' \
      -p='[{"op": "remove", "path": "/metadata/finalizers"}]'
  done

```
**预防措施:**

| 措施 | 说明 |
|:---|:---|
| 保持 Controller 健康 | 确保 `terway-controlplane` Deployment 副本数 >= 2 |
| 升级 Terway 版本 | v1.4+ 增强了 Finalizer 处理逻辑，减少阻塞概率 |
| 配置 Finalizer 告警 | 检测 Terminating 超过 30 分钟的 IPInstance/PodENI |
| 定期巡检 | 每周检查 `kubectl get ipinstances,podenis -A | grep Terminating` |
| 避免 Force Delete | 不直接 `kubectl delete --force` Pod，让 Terway 正常清理 |

---

## 7. 日常巡检清单

### 7.1 每日巡检

| 检查项 | 命令 | 关注点 |
|:---|:---|:---|
| Terway Pod 状态 | `kubectl get pods -n kube-system -l app=terway` | 全部 Running |
| IP 使用率 | `kubectl describe nodes | grep aliyun.com` | 各节点 < 85% |
| 错误日志 | `kubectl logs -n kube-system -l app=terway --since=1h | grep -ic error` | 错误数 < 10 |
| Pod Pending | `kubectl get pods -A --field-selector status.phase=Pending` | 无因 IP 不足的 Pending |
| GC 执行状态 | `kubectl logs -n kube-system -l app=terway --since=1h | grep -i gc | tail -20` | GC 正常执行，无持续失败 |

### 7.2 每周巡检

| 检查项 | 命令/操作 | 关注点 |
|:---|:---|:---|
| IP 泄漏检测 | 执行 IP 泄漏检测脚本 (见 2.6 节) | 孤儿 IP 数 < 5 |
| vSwitch IP 余量 | 阿里云控制台 > 交换机 > 可用 IP | 每个交换机可用 > 节点数 * 5 |
| ENI 配额余量 | 阿里云控制台 > 实例 > ENI 配额 | 各节点 ENI 使用率 < 80% |
| 告警规则有效性 | Prometheus Alerts 页面 | 告警规则正常触发 |
| ConfigMap 审计 | `kubectl get cm eni-config -n kube-system -o yaml` | 配置未被意外修改 |

### 7.3 每月巡检

| 检查项 | 命令/操作 | 关注点 |
|:---|:---|:---|
| Terway 版本 | 见 4.1 节 | 是否需要升级 |
| ReservedIP 过期检查 | `kubectl get reservedips -A` | 过期 IP 已正确回收 |
| CRD 资源清理 | `kubectl get ipinstances,podenis -A` | 无大量孤儿 CRD |
| 安全组规则审计 | 阿里云控制台 > 安全组 | 规则最小化，无过度放行 |
| 容量规划 | 综合节点数、Pod 数、IP 使用率趋势 | 预测未来 3 个月资源需求 |
| 灾备演练 | 模拟节点问题，验证 Pod IP 恢复 | 固定 IP 场景下 IP 恢复正常 |

---

## 8. SRE 运维红线

以下为生产环境不可违反的硬性规则:

1. **严禁在 IP 资源不足的情况下扩容集群** -- 扩容前必须确认 vSwitch 可用 IP 数量充足 (>= 预期新增节点数 * max_pool_size)，否则新节点 Pod 将全部无法创建网络。

2. **严禁直接修改运行中的 Terway ConfigMap 而不滚动重启** -- ConfigMap 修改后必须执行 `kubectl rollout restart ds/terway-eniip -n kube-system`，否则新旧配置共存导致行为不一致。

3. **严禁手动删除正在使用中的 IPInstance/PodENI CRD** -- 必须先确认关联 Pod 已终止，否则会导致 IP 泄漏或网络中断。如必须操作，先 `terway-cli garbage-collect --dry-run` 确认。

4. **升级 Terway 前必须确认 vSwitch IP 余量** -- 升级过程中所有节点 Terway 会重启并触发启动对账，如果 vSwitch IP 不足，GC 期间可能出现短暂的 IP 分配失败。

5. **固定 IP 场景必须配置 TTL 回收策略** -- StatefulSet 使用固定 IP 时，必须设置 `releaseStrategy: TTL` 和合理的 `releaseAfter`，否则 Pod 删除后 IP 永不释放，最终导致 IP 耗尽。

6. **严禁在 GC 加速模式下长期运行** -- 临时调整 `gc_min_interval` 至 60s 后，必须在问题解决后恢复默认值 300s，否则会持续增大阿里云 API 调用压力，可能导致限流。

7. **生产环境禁止使用 VPC 路由模式** -- VPC 路由模式受路由条目配额限制 (默认 48 条)，且性能较差。生产环境必须使用 ENIIP 或 IPVlan 模式。

---

## 附录 A: 错误信息速查目录

> 综合整理自 Terway 日志、kubelet Events 和阿里云 OpenAPI 返回。

| 错误信息 | 来源 | 根因 | 处理方案 |
|:---|:---|:---|:---|
| `failed to allocate pod IP: no available IP` | terway Pod | IP 池耗尽，无空闲 IP 可分配 | 检查 `terway-cli show`；释放孤儿 IP；增大 max_pool_size；扩展 vSwitch CIDR |
| `failed to allocate eni: exceeded eni quota` | terway Pod | 节点 ENI 数量达到 ECS 实例规格上限 | 升级实例规格；释放空闲 ENI；切换至 ENIIP 模式 |
| `pool is empty` | terway IPAM | 本地 IP 池为空，且无法从 vSwitch 分配新 IP | 检查 vSwitch 可用 IP；检查 RAM 权限；增大 min_pool_size |
| `fixed IP already in use` | terway Pod | 固定 IP 地址已被其他 Pod 占用 | 检查 IPInstance CRD 重复；清理已终止 Pod 的 IPInstance |
| `instance type eni limit exceeded` | terway | 当前 ECS 实例规格不支持更多 ENI | 升级实例规格或使用 ENIIP 模式 |
| `Throttling.User` | 阿里云 OpenAPI | API 调用频率超限 | 降低 Pod 创建速率；增大 IP 池减少 API 调用；申请提高限流阈值 |
| `InvalidVSwitchId.NotFound` | 阿里云 OpenAPI | 配置的 vSwitch ID 不存在或已删除 | 检查 eni-config 中 vswitches 配置；更新为有效的 vSwitch ID |
| `InvalidSecurityGroupId.NotFound` | 阿里云 OpenAPI | 安全组 ID 不存在 | 检查 eni-config 中 security_group 配置 |
| `Forbidden.RAM` | 阿里云 OpenAPI | RAM 角色权限不足，缺少 ECS ENI 管理权限 | 检查 ECS 实例角色策略，确认包含 `AliyunECSNetworkInterfaceManagementAccess` |
| `Connection timed out` / `dial tcp: i/o timeout` | 应用日志 | 跨节点通信超时，安全组或路由问题 | 检查 VPC 路由表；检查安全组入/出规则；`ip route get <target-ip>` |
| `Destination Host Unreachable` | Pod 内 ping | 目标 Pod IP 不可达，路由缺失或 ENI 异常 | 检查 VPC 路由表；检查节点 ENI 状态；重启 Terway Pod 触发路由同步 |
| `no route to host` | Pod 内命令 | VPC 路由表缺少目标 CIDR 条目 | 阿里云控制台检查路由表；确认无自定义路由冲突 |
| `route conflict detected` | terway | 自定义 VPC 路由与 Terway 自动路由冲突 | 检查并移除冲突的自定义路由条目 |
| `soft lockup` / `watchdog timeout` | 内核日志 (dmesg) | 网卡中断风暴或内核网络栈卡死 | 检查网卡多队列配置；更新内核版本；检查 NUMA 亲和性 |
| `GC.*failed` / `gc.*error` | terway GC 日志 | GC 清理失败，通常因 API 限流或权限不足 | 检查 RAM 权限；增大 gc_max_backoff；检查节点到 VPC API 连通性 |
| `connection refused` | 应用日志 | 目标 Service/Pod 未监听或 NetworkPolicy 拒绝 | 检查 Service Endpoint；检查 NetworkPolicy 规则；`iptables -L -n` |
| `Permission denied` (BoltDB) | terway Pod | resourceDB 文件权限异常 | 检查 `/var/lib/cni/terway/` 目录权限；重启 Terway Pod |

---

## 9. 交叉引用

### 本专题内

| 文档 | 说明 |
|:---|:---|
| [01-product.md](./01-product.md) | Terway 产品概览: 定位、版本历史、模式总览 |
| [02-architecture.md](./02-architecture.md) | 架构原理: ENI/ENIIP/IPVlan 模式、IPAM 机制、CRD 模型 |
| [03-usage.md](./[[网络/Terway/03-usage.md|03-usage]].md) | 使用指南: 安装配置、NetworkPolicy、固定 IP |
| [05-testing.md](./05-testing.md) | 测试验证: 连通性测试、NetworkPolicy 测试、GC 验证 |
| [06-performance.md](./06-performance.md) | 性能调优: 模式性能对比、内核调优、基准测试 |

### Domain 知识库

| 文档 | 说明 |
|:---|:---|
| [网络/38-terway-gc-mechanism.md](../网络/38-terway-gc-mechanism.md) | GC 机制详解: 架构、源码分析、执行流程 |
| [网络/37-terway-resources-crud-operations.md](../网络/37-terway-resources-crud-operations.md) | CRD 资源 CRUD 操作 |
| [网络/05-terway-advanced-guide.md](../网络/05-terway-advanced-guide.md) | Terway 高级指南: 容量规划、模式对比 |

### 关联 Topic

| 文档 | 说明 |
|:---|:---|
| [故障诊断/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md](../故障诊断/高级排障/03-networking/07-terway-troubleshooting.md) | 结构化故障排查 |
| [故障诊断/topic-fta/list/terway-fta.md](../故障诊断/FTA故障树/list/terway-fta.md) | Terway FTA 故障树 |
| [生产运维/topic-presentations/kubernetes-terway-presentation.md](../生产运维/topic-presentations/kubernetes-terway-presentation.md) | 全栈培训演示 |

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]

```

<!-- risk-assessed -->
