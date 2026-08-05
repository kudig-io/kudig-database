---
sources:
- "集群基础/控制平面/36-node-maintenance-cordon-drain-shutdown.md"
title: 节点维护 (Cordon/Drain/Shutdown/Swap) Runbook
summary: 系统化汇总 Kubernetes 节点维护全流程：cordon、drain、graceful shutdown、swap 支持与节点重启回归。
category: concepts
tags:
- node-maintenance
- cordon
- drain
- graceful-shutdown
- swap
- pdb
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台运维
estimated_read_time: 25min
intent_queries:
- 如何安全 drain 节点
- cordon 与 drain 区别
- graceful node shutdown 如何配置
- Kubernetes 支持 swap 吗
trigger_keywords:
- cordon
- drain
- graceful shutdown
- swap
- 节点维护
- PDB
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标节点与 Namespace 是否正确；是否具备足够的 RBAC 权限（nodes 的 update、pods/eviction 的 create 等）；是否已在非生产环境验证 drain 流程与 PDB 行为。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 节点维护 (Cordon/Drain/Shutdown/Swap) Runbook

> **文档类型**: 生产运行手册 | **适用版本**: K8s 1.28–1.33 | **最后更新**: 2026-07
> **使用场景**: 节点滚动升级、内核补丁、硬件更换、实例迁移、GPU 维护、节点漂移重建，以及 graceful shutdown 与 swap 配置咨询

---

## 1. 概述

节点维护（Node Maintenance）是生产集群日常运维中最高频、风险最集中的操作之一。无论是给内核打安全补丁、更换故障内存条、迁移云实例，还是配合 [[01-集群基础/03-控制平面/39-cluster-upgrade-runbook.md|集群升级 Runbook]] 滚动升级 kubelet，核心动作都收敛到一条标准链路：

```
cordon（停止调度）→ drain（驱逐业务）→ 维护操作 → uncordon（节点回归）
```

理解这条链路背后的调度、驱逐、PDB、graceful shutdown 机制，是做到**零意外中断**的前提。本篇为独立 runbook 专题，系统汇总：

- **cordon** 原理：给节点打 `node.kubernetes.io/unschedulable`，仅阻止新 Pod 调度，不影响存量 Pod。
- **drain** 原理：cordon + 优雅驱逐（eviction），受 PDB 保护，需处理 DaemonSet、emptyDir、finalizer。
- **graceful node shutdown**：kubelet 监听 systemd 关机事件，在节点断电/重启前主动优雅停 Pod。
- **node swap 支持**：1.22 alpha、1.28 GA 的 `NodeSwap` 特性，LimitedSwap vs UnlimitedSwap。
- **重启节点完整流程**与**节点回归验证**。

### 1.1 与已有文档的关系

| 主题 | 已有覆盖 | 本篇定位 |
|------|---------|---------|
| cordon/drain | `31-kubectl-complete-reference.md`、`35-cluster-upgrade-runbook.md`、`32-kubeadm-upgrade-complete-guide.md` 散见命令 | 系统化原理 + 完整 runbook |
| graceful shutdown | `15-kubelet-deep-dive.md:567` 仅两行配置 | 完整机制、systemd 监听、阶段、限制 |
| node swap | `15-kubelet-deep-dive.md:872` 仅 `vm.swappiness=0` 一行 | 1.22–1.28 特性演进、QoS 语义、生产建议 |
| PDB | [[17-系统基础/06-知识字典/operations/pdb.md\|PodDisruptionBudget]] 字典条目 | drain 中 PDB 的实战行为与排障 |

---

## 2. 节点维护场景总览

不同维护场景所需的操作子集不同。盲目对一次常规补丁也执行 `--force` drain，或对内核升级只 cordon 不 drain，都会酿成事故。下表是生产场景的标准操作矩阵。

| 维护场景 | 是否需 drain | 是否需重启 | 是否停机 | 典型时长 | 关键风险 |
|---------|:----------:|:--------:|:------:|:-------:|---------|
| **内核/OS 安全补丁** | 是 | 是（重启生效） | 是 | 15–30min | 重启后 CNI/CSI 异常、Pod 漂移 |
| **kubelet/kubeadm 升级** | 是 | 否（重启 kubelet 服务） | 部分 | 10–20min | 版本不一致、证书过期 |
| **容器运行时升级**（containerd/CRI-O） | 是 | 否 | 部分 | 10–15min | 运行时重启中断存量 Pod |
| **硬件更换**（内存/磁盘/GPU） | 是 | 是 | 是 | 30–90min | 数据卷 detach/attach 失败 |
| **云实例迁移/重启**（spot 回收、维护事件） | 是 | 是 | 是 | 5–15min | 实例消失导致 Pod 卡 Terminating |
| **GPU 驱动升级** | 是 | 是 | 是 | 20–40min | GPU 设备插件未就绪 |
| **常规配置调整**（sysctl、kubelet flag） | 视情况 | 视情况 | 视情况 | 5–10min | 配置不生效需重启 |
| **节点漂移重建**（不可救节点换新） | 否（直接删 Node） | N/A | 是 | 10–30min | 新节点标签/污点缺失 |

> **决策原则**：只要维护动作会**让节点上的容器运行时或 kubelet 重启/停止**，就必须先 drain；仅修改不影响存量的配置（如打标签）可只 cordon 或不操作。对已彻底失联的节点，跳过 drain 直接重建（场景：节点漂移重建），否则 drain 会卡死。

---

## 3. Cordon 原理与操作

### 3.1 机制：unschedulable 标记

`kubectl cordon <node>` 的本质是给节点设置 `unschedulable: true`，等价于以下命令：

```bash
# 🟡 中风险：会减少集群可调度容量
kubectl patch node <node-name> -p '{"spec":{"unschedulable":true}}'
```

设置后：

1. **Scheduler 行为**：kube-scheduler 在 `Filter` 阶段会拒绝把该节点作为候选，新创建的 Pod 不会被调度上来。
2. **存量 Pod 不受影响**：已经在节点上运行的 Pod **不会被驱逐、不会被重启**，这是 cordon 与 drain 的核心区别。
3. **不等于 taint**：cordon 只设置 `unschedulable` 字段，不添加 taint（虽然 scheduler 内部会据此过滤）。已有 Pod 上的 toleration 与此无关。

### 3.2 标准操作

```bash
# 🟡 中风险：节点将不再接收新 Pod，集群调度容量下降
# 执行前确认剩余节点有足够容量承载增量调度
kubectl cordon node1

# 🟢 低风险：恢复节点调度，存量 Pod 不受影响
kubectl uncordon node1

# 🟢 低风险：查看节点是否可调度
kubectl get node node1 -o jsonpath='{.spec.unschedulable}{"\n"}'
# 输出 "true" 表示已 cordon，无输出/空表示可调度
```

### 3.3 cordon 的典型用法

- **drain 的前置步骤**：`kubectl drain` 内部会自动 cordon，无需手动执行。
- **临时摘除调度**：发现节点有性能问题但暂不维护，先 cordon 阻止新 Pod 落地，再排查。
- **配合自动扩缩容**：在 Cluster Autoscaler 场景下，cordon 一个节点后，CA 可能会认为节点空闲而缩容，需谨慎（可加 `"cluster-autoscaler.kubernetes.io/scale-down-disabled": "true"` 注解防缩容）。

### 3.4 误用陷阱

| 误区 | 后果 | 正确做法 |
|------|------|---------|
| 只 cordon 就去重启节点 | 存量 Pod 被强制 SIGKILL，数据丢失 | 必须 drain 后再维护 |
| cordon 后期望 Pod 自动迁移 | Pod 留在原节点，重启时中断 | drain 才会驱逐 |
| 用 `kubectl taint node <node> NoSchedule` 代替 cordon | 已容忍该 taint 的 Pod 仍会调度 | cordon 用 `unschedulable`，更彻底 |

---

## 4. Drain 原理（核心）

`kubectl drain <node>` = **cordon + 优雅驱逐（eviction）所有可驱逐 Pod**。它是对节点做"安全清空"的标准手段，也是最常踩坑的运维动作。

### 4.1 drain 的执行序列

```
1. cordon 节点（设置 unschedulable=true）
2. 列出节点上所有 Pod
3. 对每个 Pod 调用 Eviction API（POST /api/v1/namespaces/<ns>/pods/<pod>/eviction）
4. Eviction 子资源被 apiserver 接收后，等价于带 grace period 的删除
5. 等待所有 Pod 终止完成
6. 若超时或 PDB 阻止，返回错误
```

### 4.2 哪些 Pod 会被 drain 跳过

drain 默认会拒绝执行，除非显式处理以下三类 Pod：

| Pod 类型 | drain 默认行为 | 处理参数 |
|---------|--------------|---------|
| **DaemonSet Pod** | 报错并停止（"error: cannot delete DaemonSet-managed Pods"） | `--ignore-daemonsets` 跳过 |
| **裸 Pod**（无 controller 的 Pod） | 报错并停止（"error: cannot delete Pods with local storage" / bare pod） | `--force` 强制删除（⚠️ 不会重建） |
| **使用 emptyDir 的 Pod** | 报错并停止（emptyDir 数据会丢失） | `--delete-emptydir-data` 允许清空 |

**DaemonSet 不驱逐的原因**：DaemonSet Pod 与节点绑定（节点 agent、日志收集、CNI、CSI node plugin、监控 exporter 等），驱逐后在同一节点会被立即重建，无意义。graceful shutdown 才是它们停机的正确处理方式（见第 6 节）。

### 4.3 Eviction API 与 PDB 保护

drain 不是直接 `delete pod`，而是创建 **Eviction** 子资源：

```yaml
# Eviction 请求体（由 kubectl drain 内部构造）
apiVersion: policy/v1
kind: Eviction
metadata:
  namespace: prod
  name: web-7b8f-x9
delocation: 
  ...
deleteOptions:
  gracePeriodSeconds: 30
```

apiserver 收到 Eviction 请求后，会检查该 Pod 所属应用是否被 **PodDisruptionBudget** 保护：

- 若驱逐会使 PDB 的 `allowedDisruptions` 变为 0（即违反 `minAvailable` 或 `maxUnavailable`），apiserver **返回 429 Too Many Requests**，drain 进入等待/失败。
- 若 PDB 允许，Eviction 被接受，转为带 grace 的删除流程。

> **关键认知**：PDB 只保护**自愿中断**（drain、升级、CA 缩容）。节点宕机、OOM Kill、kubelet 硬驱逐属于**非自愿中断**，PDB 不拦。详见 [[17-系统基础/06-知识字典/operations/pdb.md|PodDisruptionBudget]]。

### 4.4 drain 的关键参数

| 参数 | 含义 | 默认值 | 风险 |
|------|------|-------|------|
| `--grace-period=<s>` | 优雅终止时长，传给 Pod 的 gracePeriodSeconds | 30 | 调过小可能中断未完成的请求 |
| `--ignore-daemonsets` | 跳过 DaemonSet Pod | false（不跳过会报错） | 一般必加 |
| `--delete-emptydir-data` | 允许删除带 emptyDir 的 Pod | false | emptyDir 数据丢失 |
| `--force` | 强制删除（忽略 PDB、删除裸 Pod） | false | 🔴 可能违反可用性约束、裸 Pod 不重建 |
| `--timeout=<dur>` | drain 总超时（如 `5m`） | 无限等待 | 卡死时及时失败 |
| `--disable-eviction` | 用 delete 代替 eviction，绕过 PDB | false | 🔴 等同忽略 PDB |
| `--pod-selector=<sel>` | 只驱逐匹配的 Pod | 全部 | 用于选择性维护 |
| `--skip-wait-for-delete-timeout=<s>` | Pod 进入 Terminating 超过该秒数则不再等待 | 0 | 处理卡 Terminating |

### 4.5 grace period 的语义

`--grace-period` 传给 kubelet 的是"建议值"，但 Pod 自身的 `terminationGracePeriodSeconds`（默认 30）是上限：

- 若 `--grace-period` > Pod 的 `terminationGracePeriodSeconds`，实际生效的是 Pod 的较小值。
- 若想真正缩短，需 `--grace-period=0 --force`（🔴 危险，绕过 preStop hook）。

drain 卡住时，理解这个上限很重要：一个 `terminationGracePeriodSeconds: 300` 的 Pod，即使 drain `--grace-period=10`，也会等满 300s。

### 4.6 drain 卡住的四大常见原因

| 现象 | 根因 | 处理 |
|------|------|------|
| `error: cannot delete DaemonSet-managed Pods` | 未加 `--ignore-daemonsets` | 加该参数 |
| `error: cannot delete Pods with local storage` | Pod 用了 emptyDir | 加 `--delete-emptydir-data` |
| `error: ... cannot evict ... PDB ...` | PDB 阻止（minAvailable 不满足） | 等待副本恢复 / 评估 force / 调整 PDB |
| 长时间无输出、Pod 卡 Terminating | Pod finalizer 未清 / 节点已失联 | 清 finalizer 或 `--force --grace-period=0` 删 Pod（见排障） |

---

## 5. Drain 实战 Runbook（步骤化）

本节是可直接落地的生产流程。每一步都标注风险等级与失败分支。

### 步骤 1：评估影响（🟢 只读）

drain 前必须搞清楚两件事：**节点上跑了什么**、**哪些应用有 PDB 保护**。

```bash
# 🟢 低风险：列出目标节点上的所有 Pod（含命名空间、控制器、QoS）
kubectl get pod -o wide --field-selector spec.nodeName=node1 -A

# 🟢 低风险：查看节点资源水位（确认其他节点有容量承接）
kubectl describe node node1 | grep -A5 "Allocatable"
kubectl top nodes

# 🟢 低风险：列出全集群所有 PDB，重点关注目标节点上的应用
kubectl get pdb -A -o wide

# 🟢 低风险：检查节点上的关键负载分布（确认驱逐后仍有副本在其它节点）
kubectl get pod -o wide -A --field-selector spec.nodeName=node1 | \
  awk '{print $1,$2}' | sort
```

**评估清单**：
- [ ] 节点上有无**单副本**应用（StatefulSet 副本数=1）？这些 drain 时会中断。
- [ ] PDB 的 `minAvailable` 在驱逐后是否仍满足？若 PDB=1 且只有 1 个副本，drain 必失败。
- [ ] 有无使用 `emptyDir` 存放重要数据（缓存可丢，临时数据库不可丢）？
- [ ] 有无 `ReplicaSet=1` 的裸 Deployment（实际无冗余）？
- [ ] 其它节点的 allocatable 是否够承接目标节点的 request 总量？

### 步骤 2：Cordon 节点（🟡 中风险）

虽然 drain 会自动 cordon，但**先手动 cordon** 有两个好处：阻止 drain 漫长过程中新 Pod 被调度上来；失败时可安全中止而节点仍处于摘除状态。

```bash
# 🟡 中风险：节点停止接收新 Pod
kubectl cordon node1
# 预期输出: node/node1 cordoned
```

### 步骤 3：执行 drain（🟡 中风险）

生产推荐的"安全 drain"参数组合：

```bash
# 🟡 中风险：驱逐业务 Pod，受 PDB 保护，超时 5 分钟
kubectl drain node1 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --timeout=5m
```

参数解释：
- `--ignore-daemonsets`：跳过 DS Pod（必加，否则必然报错）。
- `--delete-emptydir-data`：允许清空 emptyDir（绝大多数 Pod 的日志/缓存可接受）。
- `--timeout=5m`：5 分钟未完成则失败，避免无限挂起。
- **不加** `--force`：让 PDB 正常生效，这是安全 drain 的标志。

预期输出：
```
node/node1 cordoned
evicting pod prod/web-7b8f-x9
evicting pod prod/cache-abc12
pod/cache-abc12 evicted
pod/web-7b8f-x9 evicted
node/node1 drained
```

### 步骤 4：处理 drain 失败（🟡 中风险）

#### 4a. PDB 阻止

```
error when evicting pods/"web-7b8f-x9" -n "prod" (will retry after 5s):
"The eviction request is not allowed at this time because the pod
is not ready and the PDB does not allow disruptions."
```

**处理**：
```bash
# 🟢 低风险：查看该应用副本分布与 PDB 状态
kubectl get pdb -n prod web-pdb
# 关注 ALLOWED DISRUPTIONS 列，若为 0 则当前不可驱逐

kubectl get pod -n prod -l app=web -o wide
# 查看是否有副本未就绪（Pending/CrashLoopBackOff）
```

- 若副本在恢复中：等待，drain 会在 PDB 允许后继续。
- 若副本长期不就绪：先修复应用（扩容、修镜像），再 drain。
- 紧急且可接受降级：评估是否走步骤 5 的 force（需变更审批）。

#### 4b. Pod finalizer 阻止

Pod 长期 `Terminating`，但节点仍在线：

```bash
# 🟢 低风险：查看 finalizer
kubectl get pod <pod> -n <ns> -o jsonpath='{.metadata.finalizers}{"\n"}'

# 🟡 中风险：移除 finalizer（前提：确认对应控制器已无副作用，如 PV 已释放）
kubectl patch pod <pod> -n <ns> --type=json \
  -p='[{"op":"remove","path":"/metadata/finalizers"}]'
```

#### 4c. 超时未完成

```
error: timed out waiting for the condition
```

**处理**：
```bash
# 🟢 低风险：看哪个 Pod 还没走
kubectl get pod -A --field-selector spec.nodeName=node1
```
若只剩 DaemonSet（正常）→ drain 实际已完成，可忽略。若有其它 Pod 卡住，按 4b 处理或评估 force。

### 步骤 5：紧急 force drain（🔴 高风险，仅紧急情况）

> ⚠️ **变更审批要求**：此操作绕过 PDB，可能导致应用低于 SLA、数据丢失。必须经变更窗口审批，且确认集群其它副本可承载。

```bash
# 🔴 高风险：忽略 PDB，强制驱逐，可能违反可用性 SLO
kubectl drain node1 \
  --force \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --timeout=2m
```

**`--force` 的危险面**：
- 忽略所有 PDB → 应用可能被打到 0 副本。
- 删除**裸 Pod**（无 controller 的 Pod）→ **不会被重建**，需提前确认这些 Pod 的处置。
- 不等于 `--disable-eviction`（后者用 delete 绕过 eviction 整套机制）。

仅以下场景考虑 force：
- 节点即将被云厂商强制回收（spot 实例 2 分钟警告），且应用有自愈能力。
- 灾难演练中故意触发降级以验证韧性。
- 维护窗口极短且已与业务方确认可承受短暂中断。

### 步骤 6：完成维护后回归（🟢 低风险）

维护操作（升级、重启、换件）完成且节点组件健康后：

```bash
# 🟢 低风险：恢复调度
kubectl uncordon node1
# 预期输出: node/node1 uncordoned
```

回归验证见第 9 节。

### 5.7 完整 bash 脚本模板（含前置检查、重试、验证）

```bash
#!/usr/bin/env bash
# safedrain.sh — 安全 drain + 维护 + 回归一体化脚本
# 用法: ./safedrain.sh <node-name> [maintenance-command...]

set -euo pipefail
NODE="${1:?用法: $0 <node-name> [cmd...]}"
shift || true
MAINT_CMD="${*:-echo 'no maintenance command, pausing for manual ops; press Enter when done' && read}"

# ---- 前置检查 ----
echo "[1/6] 前置检查: 节点存在且 Ready"
kubectl get node "$NODE" || { echo "节点不存在"; exit 1; }
kubectl get node "$NODE" -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
echo

echo "[2/6] 前置检查: 节点上的 Pod 清单"
kubectl get pod -A -o wide --field-selector spec.nodeName="$NODE"

echo "[3/6] 前置检查: 全集群 PDB"
kubectl get pdb -A -o wide

echo "[4/6] cordon 节点"
kubectl cordon "$NODE"

# ---- drain（带重试，最多 3 次）----
echo "[5/6] drain 节点（重试 3 次，每次超时 5m）"
for i in 1 2 3; do
  echo "  尝试 #$i"
  if kubectl drain "$NODE" \
       --ignore-daemonsets \
       --delete-emptydir-data \
       --timeout=5m; then
    echo "  drain 成功"
    break
  fi
  if [ "$i" -eq 3 ]; then
    echo "  drain 三次失败，节点仍处于 cordon 状态，请人工介入"
    echo "  评估后可手动: kubectl uncordon $NODE"
    exit 2
  fi
  echo "  drain 失败，10s 后重试..."
  sleep 10
done

# ---- 维护操作 ----
echo "[6/6] 执行维护操作"
eval "$MAINT_CMD"

echo "维护完成。确认节点组件健康后，执行回归:"
echo "  kubectl uncordon $NODE"
echo "  kubectl get node $NODE"
echo "  kubectl get pod -A -o wide --field-selector spec.nodeName=$NODE"
```

---

## 6. Graceful Node Shutdown（kubelet 配置）

### 6.1 问题：为什么需要 graceful shutdown

当节点被直接断电、`reboot`、`halt` 或被云厂商强制停止时，**kubelet 来不及通知 apiserver**，会发生：

1. Pod 进程被 systemd/init 直接 `SIGKILL`，preStop hook 与 `SIGTERM` 优雅关闭流程**完全不执行**。
2. apiserver 仍认为 Pod 在运行（节点失联），Pod 卡在 `Terminating` 状态可达数分钟（直到 `pod-eviction-timeout` 默认 5 分钟后 node-lifecycle-controller 标记删除）。
3. 有状态服务（数据库、消息队列）数据不一致、连接被粗暴切断。

**graceful node shutdown**（特性门控 `GracefulNodeShutdown`）让 kubelet 在节点真正关机前，主动按 QoS 优先级优雅停止 Pod。

### 6.2 特性演进

| 版本 | 状态 |
|------|------|
| 1.21 | beta（默认开启） |
| 1.28 | GA（NodeGracefulShutdown 正式稳定） |

1.28+ 生产集群默认具备该能力，但**仍需在 kubelet 配置中设置时长**才真正生效。

### 6.3 systemd 监听机制

kubelet 通过监听 **systemd** 的关机事件来触发 graceful shutdown：

```
节点执行 systemctl reboot / poweroff / halt
    │
    ├─ systemd 写入 /run/systemd/shutdown/scheduled（含关机倒计时）
    │
    ├─ kubelet 通过 dbus 监听 systemd 的 PrepareForShutdown 信号
    │
    ├─ kubelet 进入 shutdown 流程：
    │     阶段1: 停止 non-critical Pod（普通业务）
    │     阶段2: 停止 critical Pod（system-node-critical/system-cluster-critical）
    │
    └─ systemd 继续关机流程
```

关键文件 `/run/systemd/shutdown/scheduled` 中含 `USEC=` 字段，给出距实际关机的微秒数，kubelet 据此分配两个阶段的时长。

### 6.4 kubelet 配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  GracefulNodeShutdown: true          # 1.28+ 默认 true，显式声明更清晰

# graceful shutdown 总时长（覆盖所有 Pod）
shutdownGracePeriod: "30s"

# critical pod（system-node-critical / system-cluster-critical）的时长
# 必须小于等于 shutdownGracePeriod
shutdownGracePeriodCriticalPods: "10s"

# 1.28+ 可选：按 QoS 分阶段（beta 特性 GracefulNodeShutdownBasedOnPodPriority）
# shutdownGracePeriodByPodPriority:
#   - priority: 1000000      # system-node-critical
#     shutdownGracePeriodSeconds: 10
#   - priority: 10000        # system-cluster-critical
#     shutdownGracePeriodSeconds: 20
#   - priority: 0            # 普通业务
#     shutdownGracePeriodSeconds: 30
```

### 6.5 两阶段时长的语义

```
shutdownGracePeriod = 30s（总预算）
shutdownGracePeriodCriticalPods = 10s（critical pod 预算）

阶段1（non-critical pod）：30s - 10s = 20s
   → 业务 Pod 在这 20s 内被 SIGTERM + 等待优雅关闭
阶段2（critical pod）：10s
   → 系统级 Pod（如 CNI、CSI、监控 agent）在这 10s 内优雅关闭
```

**配置建议**：
- `shutdownGracePeriod` 应 ≥ 最慢业务 Pod 的 `terminationGracePeriodSeconds`，否则业务来不及优雅退出。
- critical pod 阶段用于保证节点级组件（CNI/CSI）有足够时间清理，通常 10–15s 足够。
- 高负载节点可设 `shutdownGracePeriod: "60s"` 或更长。

### 6.6 验证 graceful shutdown 生效

```bash
# 🟢 低风险：检查 kubelet 配置已加载
ssh node1 'cat /var/lib/kubelet/config.yaml | grep -A2 shutdownGrace'

# 🟢 低风险：检查 kubelet 日志中的 shutdown 事件
ssh node1 'journalctl -u kubelet --since "1 hour ago" | grep -i "shutdown\|graceful"'

# 🟢 低风险：确认 systemd 是 init 系统（graceful shutdown 依赖 systemd）
ssh node1 'ps -p 1 -o comm='
# 应输出 systemd（若输出 init/upstart 则不支持）
```

### 6.7 限制与不适用场景

- **必须使用 systemd 作为 init 系统**。非 systemd（如某些嵌入式、传统 SysV init）的节点，kubelet 无法监听关机事件，graceful shutdown 不生效。
- **云厂商硬停止**：若云厂商直接断电（spot 实例强制回收、宿主机硬件故障），systemd 关机流程本身不执行，graceful shutdown 无能为力。这类场景需依赖 PDB + 多副本拓扑分布来兜底。
- **kernel panic / 硬死锁**：同上，无 systemd 关机事件，无法触发。
- 不替代 drain：graceful shutdown 处理"计划内但短暂的关机"，drain 处理"计划内且需迁移业务"的长时间维护。两者互补。

---

## 7. Node Swap 支持

### 7.1 历史与演进

Kubernetes 长期**默认禁用 swap**（kubelet 启动时 `--fail-swap-on=true`，节点有 swap 直接拒绝启动）。这是出于性能可预测性的考量：swap 会让 Pod 延迟暴增、驱逐阈值失真。

但随着大数据、Java、Node.js 等内存弹性需求场景增加，社区逐步引入 swap 支持：

| 版本 | 里程碑 |
|------|-------|
| < 1.22 | 默认禁用，`--fail-swap-on=false` 可绕过但不安全 |
| 1.22 | alpha：`NodeSwap` 特性门控引入，仅 Linux |
| 1.27 | beta：默认可用，引入 LimitedSwap |
| **1.28** | **GA**：`NodeSwap` 正式 stable |

### 7.2 两种 swap 行为

kubelet 通过 `--eviction-hard` 与 `memory.swap_behavior`（或 `--config` 中的 `failSwapOn`、`memorySwap`）控制：

| 模式 | 说明 | 安全性 |
|------|------|-------|
| **LimitedSwap**（1.28 默认） | 仅 QoS=Burstable/BestEffort 的 Pod 可用 swap，**Guaranteed Pod 不能用 swap**；可用 swap 量按节点内存比例计算 | 生产可接受 |
| **UnlimitedSwap** | 所有 Pod（含 Guaranteed）无限制使用 swap | ⚠️ 不安全，仅特殊场景 |

**为什么 Guaranteed Pod 默认禁用 swap**：Guaranteed Pod 的语义是"资源严格隔离、可预测"。一旦允许 swap，内存超卖后的延迟与 OOM 行为不可预测，破坏了 QoS 承诺。详见 [[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|kubelet 深度剖析]] 中 QoS 章节。

### 7.3 LimitedSwap 的配额计算

```
节点 machineMemory = 64Gi
节点 swap = 16Gi

Burstable/BestEffort Pod 可用的 swap 总量：
  swapLimit = (pod memory request 之和 / 节点所有 pod memory request 之和) × swapSize

示例：某 BestEffort Pod 无 request，则按比例分得极少 swap（接近 0）
     某 Burstable Pod request=4Gi，全节点 request=40Gi
     则该 Pod swap 上限 ≈ (4/40) × 16Gi = 1.6Gi
```

### 7.4 启用 swap 的 kubelet 配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  NodeSwap: true                  # 1.28+ 默认 true

failSwapOn: false                 # 允许节点有 swap 时启动 kubelet（旧参数）
                                 # 1.28+ 推荐用 memorySwap 配置块

memorySwap:
  swapBehavior: LimitedSwap       # 或 UnlimitedSwap（不推荐生产）
```

启动节点前还需在 OS 层开启 swap：

```bash
# 🟡 中风险：在节点上启用 swap（影响性能可预测性，需评估）
# 1. 创建 swap 文件
fallocate -l 16G /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile

# 2. 持久化
echo '/swapfile none swap sw 0 0' >> /etc/fstab

# 3. 重启 kubelet 使配置生效
systemctl restart kubelet
```

### 7.5 Pod 级 swap 语义

容器 `resources.limits.memory` 的后缀决定 swap 行为：

```yaml
spec:
  containers:
  - name: app
    resources:
      requests:
        memory: "1Gi"
      limits:
        # 写法 A（默认语义）: limits.memory = 1Gi 表示"内存+swap 总和上限"
        #   → 该容器 swap 可用量 = 1Gi - request = 0Gi（无 swap）
        memory: "1Gi"

        # 写法 B（显式 swap）: 加 Swap 后缀
        # memorySwap = 2Gi 表示"swap 单独上限 2Gi"，memory 仍是 1Gi
        # memorySwap: "2Gi"        # K8s 1.22+ alpha，需 NodeSwap
```

| 写法 | memory 含义 | swap 上限 |
|------|-----------|---------|
| `memory: 1Gi` | 内存上限 1Gi | 0（request=limit 时无 swap 余量） |
| `memory: 2Gi` + request `1Gi` | 内存上限 2Gi | swap = 2Gi - 1Gi = 1Gi |
| `memory: 1Gi` + `memorySwap: 2Gi` | 内存 1Gi | swap 2Gi（显式） |

### 7.6 生产建议：是否启用 swap

| 维度 | 结论 |
|------|------|
| **一般生产负载** | **仍建议禁用 swap**。可预测性 > 内存弹性。`15-kubelet-deep-dive.md:872` 的 `vm.swappiness=0` 仍是默认推荐。 |
| **大数据/JVM 应用** | 可评估开启 LimitedSwap，避免 JVM 堆引发的 OOM Kill，但需充分压测。 |
| **内存弹性场景**（突发流量） | LimitedSwap 提供缓冲，但会牺牲尾部延迟。 |
| **Guaranteed QoS 应用** | 不受影响（始终禁 swap），可直接迁移到开启 swap 的集群。 |

**核心权衡**：swap 换来的是"避免 OOM Kill 的缓冲"，代价是"偶发的磁盘级延迟"。对延迟敏感型业务（API、数据库），禁用；对批处理、可重试任务，可开。

### 7.7 检查节点 swap 状态

```bash
# 🟢 低风险：在节点上查看 swap
ssh node1 'swapon --show'
# NAME      TYPE  SIZE  USED  PRIO
# /swapfile file  16G   200M  -2

# 🟢 低风险：查看节点 allocatable 是否含 swap（1.28+）
kubectl describe node node1 | grep -A15 "Capacity:"
# 应能看到:
#   memory:          68719476736 (64Gi)
#   swap:            17179869184 (16Gi)   ← 仅启用 NodeSwap 后出现
```

---

## 8. 重启节点的完整流程

整合 cordon/drain/graceful shutdown 的节点重启 runbook。适用于内核升级、OS 补丁、硬件维护后的重启。

### 8.1 流程总览

```
1. drain 节点（第 5 节 runbook）
2. 维护操作（安装补丁 / 更换硬件）
3. 重启节点
4. 验证节点组件健康（kubelet / 容器运行时 / CNI / CSI）
5. uncordon 节点
6. 观察 Pod 重新调度与业务恢复
```

### 8.2 重启与组件验证

```bash
# 🟡 中风险：在节点上执行重启（需 drain 完成）
ssh node1 'sudo systemctl reboot'

# 等待节点重新出现并 Ready（约 2–5 分钟）
kubectl get node -w | grep node1

# 🟢 低风险：节点 Ready 后逐项验证

# (1) kubelet 健康
ssh node1 'systemctl is-active kubelet'          # 应为 active
ssh node1 'sudo journalctl -u kubelet --since "5 min ago" | grep -i error'

# (2) 容器运行时健康（containerd 示例）
ssh node1 'sudo systemctl is-active containerd'  # 应为 active
ssh node1 'sudo crictl ps | head'                # 应能看到运行中的 Pod sandbox

# (3) CNI 正常（节点上有 CNI Pod 且 Running）
kubectl get pod -A -o wide --field-selector spec.nodeName=node1 | grep -E "calico|flannel|cilium|weave"

# (4) CSI node plugin 正常（若有状态服务）
kubectl get pod -A -o wide --field-selector spec.nodeName=node1 | grep -E "csi|storage"

# (5) 节点 condition 全部健康
kubectl get node node1 -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# 期望: Ready=True, MemoryPressure=False, DiskPressure=False, PIDPressure=False
```

### 8.3 常见重启后问题

| 现象 | 原因 | 处理 |
|------|------|------|
| 节点 `NotReady` 持续 | kubelet 未自启 | `systemctl enable kubelet && systemctl start kubelet` |
| 节点 Ready 但 Pod 不调度 | uncordon 未执行或仍有 taint | `kubectl uncordon`；查 `kubectl describe node` 的 Taints |
| CNI Pod 不起来 | 配置残留 / 内核模块缺失 | 查 CNI Pod 日志，重新 apply CNI daemonset |
| 旧 Pod 卡 Terminating | drain 时未完全清空 + 节点重启 | 见排障第 10.3 节 |

---

## 9. 节点回归（Uncordon 与验证）

uncordon 不是终点，**回归验证**才是。以下检查清单用于确认节点真正回归生产：

```bash
# 🟢 低风险：解除 cordon
kubectl uncordon node1

# 🟢 低风险：验证清单

# (1) 节点状态
kubectl get node node1
# STATUS 应为 Ready，ROLES 正确

# (2) 调度能力：确认新 Pod 会被调度上来
kubectl describe node node1 | grep -A5 "Taints"
# 应无 node.kubernetes.io/unschedulable taint

# (3) Pod 分布：观察其它节点上的副本是否回迁，或新调度均衡
kubectl get pod -A -o wide --field-selector spec.nodeName=node1

# (4) 资源水位
kubectl top node node1
kubectl describe node node1 | grep -A8 "Allocated resources"

# (5) 容器运行时
ssh node1 'sudo crictl info | head'

# (6) 网络/存储功能验证（部署一个临时测试 Pod）
kubectl run nettest --image=busybox --restart=Never --overrides='{"spec":{"nodeName":"node1"}}' -- \
  sh -c 'wget -qO- https://kubernetes.default.svc && echo OK'
kubectl delete pod nettest
```

**回归完成判据**：
- 节点 Ready 且无异常 condition。
- `unschedulable` 已清除。
- 至少有新 Pod 被调度到该节点（确认 scheduler 重新认可）。
- 容器运行时、CNI、CSI 全部健康。
- 业务监控指标恢复正常基线。

---

## 10. 排障

### 10.1 drain 卡住 / 无进展（🟢）

```bash
# 看节点上还剩哪些 Pod
kubectl get pod -A --field-selector spec.nodeName=node1

# 看具体某个 Pod 为什么不走（事件）
kubectl describe pod <pod> -n <ns> | tail -30

# 看 drain 相关事件
kubectl get events -A --field-selector reason=Eviction --sort-by=.lastTimestamp | tail
```

### 10.2 PDB 阻止 drain（🟢）

```bash
# 查看所有 PDB 的可驱逐数
kubectl get pdb -A
# 关注 ALLOWED DISRUPTIONS 列：0 表示当前不可驱逐

# 查看具体 PDB 详情
kubectl get pdb -n <ns> <name> -o yaml

# 看 PDB 选中的 Pod 实际健康度
kubectl get pod -n <ns> -l <pdb-selector> -o wide
# 若有 Pod 未就绪（CrashLoopBackOff/Pending），PDB 会拒绝驱逐
```

### 10.3 Pod 卡 Terminating（🟡 / 🔴）

```bash
# 🟢 低风险：先排查根因
kubectl get pod <pod> -n <ns> -o yaml | grep -A5 finalizers
# 若有 finalizer（如 foregroundDeletion / 自定义），是它阻止删除

# 检查节点是否还在（若节点已删除，Pod 会卡 Terminating）
kubectl get node <node>

# 🟡 中风险：节点仍在 → 移除 finalizer 让 Pod 完成
kubectl patch pod <pod> -n <ns> --type=json \
  -p='[{"op":"remove","path":"/metadata/finalizers"}]'

# 🔴 高风险：节点已失联/已删 → 强制删除（数据可能不一致）
# 仅当节点确定不会回来时使用
kubectl delete pod <pod> -n <ns> --grace-period=0 --force
```

### 10.4 graceful shutdown 未生效（🟢）

```bash
# 检查 kubelet 版本（1.21+ 才支持）
ssh node1 'kubelet --version'

# 检查配置文件中是否声明了时长（仅开 featureGates 不够）
ssh node1 'grep -E "shutdownGrace|GracefulNodeShutdown" /var/lib/kubelet/config.yaml'

# 检查 init 系统是否 systemd
ssh node1 'ps -p 1 -o comm='
# 必须是 systemd

# 查 kubelet 是否成功监听 dbus（重启节点后看日志）
ssh node1 'journalctl -u kubelet --since "1 hour ago" | grep -i "graceful\|shutdown"'
```

### 10.5 强制删除卡 Terminating 的 Pod（节点不可达，🔴）

当节点彻底失联（硬件故障、云实例消失、网络分区），其上的 Pod 会长期卡在 Terminating，阻塞 StatefulSet/Deployment 重建：

```bash
# 🔴 高风险：强制删除，可能造成双写（原节点若复活会有两个同名 Pod）
# 前置条件：确认节点已永久下线（不可能复活）
kubectl delete pod <pod> -n <ns> --grace-period=0 --force

# 更安全的方式：删除 Pod 的 finalizer 让 controller 接管
kubectl patch pod <pod> -n <ns> --type=json \
  -p='[{"op":"remove","path":"/metadata/finalizers"},{"op":"replace","path":"/metadata/deletionGracePeriodSeconds","value":0}]'
```

> **数据一致性警告**：对挂载 PV 的 StatefulSet Pod，强制删除后若原节点复活，可能出现"双 Pod 同一 PV"的脑裂。务必先确认原节点不会回来（如云厂商已回收实例、物理机已下架）。

### 10.6 排障速查表

| 现象 | 第一步命令 | 可能根因 |
|------|----------|---------|
| drain 报 DaemonSet 错 | `kubectl get ds -A` | 未加 `--ignore-daemonsets` |
| drain 报 emptyDir 错 | `kubectl describe pod <p>` | 未加 `--delete-emptydir-data` |
| drain 报 PDB 错 | `kubectl get pdb -A` | 副本未就绪，等待或评估 force |
| Pod 卡 Terminating | `kubectl get pod -o yaml \| grep finalizer` | finalizer 未清 / 节点失联 |
| 节点重启后 NotReady | `ssh node 'systemctl status kubelet'` | kubelet 未自启 |
| graceful shutdown 无效 | `ssh node 'ps -p 1 -o comm='` | 非 systemd / 配置缺失 |
| uncordon 后仍不调度 | `kubectl describe node \| grep -i taint` | 残留 NoSchedule taint |

---

## 11. 关键决策矩阵（快速参考）

| 情境 | 推荐操作 | 风险等级 |
|------|---------|:-------:|
| 常规节点重启（打补丁） | drain（安全参数）→ 重启 → uncordon | 🟡 |
| 节点短期重启（< 5min） | 配置好 graceful shutdown 后直接重启 | 🟡 |
| 紧急腾空节点（spot 回收警告） | drain `--force`（审批后） | 🔴 |
| 节点已失联、不可恢复 | 跳过 drain，删 Node 对象重建 | 🔴 |
| 仅修改节点标签/注解 | 不需 cordon，直接 `kubectl label` | 🟢 |
| 临时摘除调度（排查问题） | cordon（不 drain） | 🟡 |
| 维护 DaemonSet（CNI/CSI） | drain 跳过 DS；用 graceful shutdown 处理 DS Pod | 🟡 |
| 节点配置 swap | 评估后启用 LimitedSwap；Guaranteed Pod 不受影响 | 🟡 |

---

## 12. 相关文档

### 12.1 集群内关联

- [[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|kubelet 深度剖析]] — kubelet 配置全貌、QoS、eviction、graceful shutdown 原始配置位
- [[01-集群基础/03-控制平面/37-kubelet-eviction-thresholds.md|kubelet 驱逐阈值]] — 节点压力下的 Pod 驱逐机制（与 drain 的自愿驱逐互补）
- [[01-集群基础/03-控制平面/39-cluster-upgrade-runbook.md|集群升级 Runbook]] — 升级场景下节点 drain 的滚动流程
- [[01-集群基础/03-控制平面/36-kubeadm-upgrade-complete-guide.md|kubeadm 升级完整指南]] — kubelet/kubeadm 升级中的节点维护
- [[12-可靠性/03-容量规划/03-resource-quota-limitrange.md|资源配额]] — request/limit 与 QoS，影响 drain 时的容量评估
- [[17-系统基础/06-知识字典/operations/pdb.md|PodDisruptionBudget]] — PDB 字典定义，drain 中 PDB 的保护逻辑

### 12.2 官方参考

- [Kubernetes Docs — Safely Drain a Node](https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/)
- [Kubernetes Docs — PodDisruptionBudgets](https://kubernetes.io/docs/tasks/run-application/configure-pdb/)
- [Kubernetes Blog — Graceful Node Shutdown](https://kubernetes.io/blog/2021/04/21/graceful-node-shutdown-beta/)
- [KEP-2269 — Node Graceful Shutdown](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/2269-graceful-node-shutdown)
- [KEP-2400 — Node Swap](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/2400-node-swap)

### 12.3 See Also

- 集群基础 KUDIG Database — Global MOC
- [[01-集群基础/03-控制平面/34-kubectl-complete-reference.md|kubectl 完整参考]] — cordon/drain/uncordon 命令速查
- [[01-集群基础/03-控制平面/06-plane-troubleshooting.md|控制平面故障排查手册]] — 节点级故障诊断

<!-- risk-assessed -->
