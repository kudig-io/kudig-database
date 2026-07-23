---
title: Pod Pending 与调度失败诊断
description: 针对 Pod Pending 状态的完整诊断技能，覆盖资源不足、污点/容忍、亲和性冲突、PVC 未绑定、配额超限等全部调度失败场景
summary: Pod Pending 表示已被 API Server 接受但未成功调度，本技能提供从 Events 解读到根因修复的标准化诊断路径
category: skill
tags:
- k8s
- pod
- troubleshooting
- pending
- scheduling
- taint
- toleration
- affinity
- resource-quota
- pvc
sources:
- 故障诊断/topic-skills/03-pod-pending.md
- 故障诊断/核心排障/05-pod-pending-diagnosis.md
- 故障诊断/FTA故障树/list/pod-fta.md
- code/kubernetes-release-1.28/pkg/apis/core/types.go
- code/kubernetes-1.36.2/pkg/apis/core/types.go
- code/kube-scheduler-master/framework/types.go
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- Pod 一直 Pending 怎么办
- Pod 调度失败什么原因
- FailedScheduling 怎么排查
- 节点资源不足导致 Pod 无法调度
- Pod 无法分配到节点
trigger_keywords:
- Pending
- FailedScheduling
- 调度失败
- 资源不足
- Insufficient cpu
- Insufficient memory
- Unschedulable
- taint
- toleration
- nodeSelector
prerequisites:
- kubectl-basics
- pod-lifecycle
- scheduling-basics
skill_id: SKILL-POD-002
skill_name: Pod Pending 与调度失败诊断
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-3 -> IE-3.1 -> BE-3.1/BE-3.2/BE-3.3/BE-3.4
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod Pending 与调度失败诊断

> **Skill ID**: SKILL-POD-002
> **Agent 执行模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批
> **预计修复时间**: 5-30 分钟
> **FTA 路径**: TE-3 → IE-3.1 → BE-3.1~3.4

---

## 1. 概述

Pod 处于 Pending 状态表示 Pod 已被 Kubernetes API Server 接受，但尚未被调度到节点或容器镜像尚未拉取。

| 阶段 | 状态 | 说明 | 诊断入口 |
|------|------|------|---------|
| **调度前** | Pending (无 nodeName) | 等待调度器分配节点 | `kubectl describe pod` Events |
| **调度后** | Pending (有 nodeName) | 已分配节点，等待容器启动 | `kubectl describe pod` Conditions |
| **镜像拉取** | Pending + ImagePullBackOff | 镜像拉取失败 | 转 [03-pod-imagepull-container.md](03-pod-imagepull-container.md) |
| **初始化** | Pending + Init:X/Y | Init 容器未完成 | Init 容器日志 |

---

## 2. 诊断决策树

```
Pod Pending
    │
    ├── Events 包含 "FailedScheduling"?
    │       │
    │       ├── "Insufficient cpu/memory" → 资源不足 (BE-3.1)
    │       ├── "node(s) didn't match selector" → 节点选择器不匹配 (BE-3.2)
    │       ├── "node(s) had taint" → 污点阻止调度 (BE-3.3)
    │       ├── "exceeded quota" → 配额超限 (BE-3.4)
    │       ├── "didn't match pod affinity" → 亲和性冲突
    │       └── "no nodes available" → 所有节点不可用
    │
    ├── 无 FailedScheduling 事件?
    │       │
    │       ├── 检查 kube-scheduler 是否运行
    │       └── 检查 Pod 是否有 nodeName（已调度但启动失败）
    │
    └── PVC 相关?
            ├── "persistentvolumeclaim not found" → PVC 不存在
            └── "pod has unbound immediate PVC" → PVC 未绑定
```

---

## 3. 诊断流程

### Phase 1: 快速定位

**Step 1.1**: 确认 Pod 状态和事件

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod> -n <namespace> -o wide
kubectl describe pod <pod> -n <namespace> | grep -A 20 "Events:"
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod> --sort-by=.lastTimestamp
```

**Step 1.2**: 根据事件关键词路由

| 事件关键词 | 根因 | 修复方向 |
|-----------|------|---------|
| `Insufficient cpu/memory` | 节点资源不足 | 扩容节点 / 调整 requests |
| `node(s) didn't match selector` | NodeSelector 不匹配 | 检查标签 / 修改选择器 |
| `node(s) had taint` | Taint/Toleration 不匹配 | 添加 Toleration / 移除 Taint |
| `persistentvolumeclaim not found` | PVC 未绑定 | 检查 PVC 状态和 StorageClass |
| `Unschedulable` | 节点不可调度 | 检查节点 SchedulingDisabled |
| `pod has unbound immediate PVC` | PVC 立即绑定未就绪 | 等待 PVC Ready 或检查 PV |
| `exceeded quota` | 命名空间配额超限 | 调整 ResourceQuota |

---

### Phase 2: 分支深度诊断

#### 路径 A: 资源不足 (BE-3.1)

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点资源分配情况
kubectl describe nodes | grep -A 5 "Allocated resources"

# 查看节点实际使用率
kubectl top nodes

# 查看 Pod 的资源请求
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"requests: cpu="}{.resources.requests.cpu}{" memory="}{.resources.requests.memory}{"\n"}{end}'
```

**修复方案**:
```bash
# 🟡 中风险：调整资源请求
kubectl patch deployment <deployment> -n <namespace> -p \
  '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"requests":{"cpu":"100m","memory":"128Mi"}}}]}}}}'
```

#### 路径 B: 污点/容忍不匹配 (BE-3.3)

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点污点
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints[*].key

# 查看 Pod 的容忍配置
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.tolerations}'
```

**修复方案**:
```yaml
# 在 Pod spec 中添加 tolerations
tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
```

#### 路径 C: 节点选择器/亲和性冲突 (BE-3.2)

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 的节点选择器
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.nodeSelector}'

# 查看 Pod 的亲和性规则
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.affinity}' | python3 -m json.tool

# 查看节点标签
kubectl get nodes --show-labels
```

#### 路径 D: 配额超限 (BE-3.4)

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe resourcequota -n <namespace>
kubectl get limitrange -n <namespace> -o yaml
```

#### 路径 E: PVC 未绑定

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>
kubectl get sc
kubectl get pods -n kube-system | grep csi
```

#### 路径 F: 调度器异常

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n kube-system -l component=kube-scheduler
kubectl logs -n kube-system -l component=kube-scheduler --tail=50
```

---

## 4. 根因分类与修复

| RC-ID | 根因 | 概率 | 修复方案 | 风险 |
|-------|------|------|---------|------|
| RC-001 | 节点资源不足 | 35% | 扩容节点/降低 requests | 🟡 |
| RC-002 | 节点选择器/亲和性不匹配 | 20% | 修正标签/放宽亲和性 | 🟡 |
| RC-003 | 污点未容忍 | 20% | 添加 tolerations | 🟡 |
| RC-004 | 资源配额超限 | 10% | 调整 ResourceQuota | 🟡 |
| RC-005 | PVC 未绑定 | 10% | 修复 StorageClass/PV | 🟡 |
| RC-006 | 调度器异常 | 5% | 重启 kube-scheduler | 🔴 |

---

## 5. 紧急处理流程

```
触发告警: Pod Pending > 15min 或 批量 Pending
                │
                ▼
┌──────────────────────────────────────┐
│ Step 1: 快速评估 (2min)              │
│ kubectl get pods -A                  │
│   --field-selector=status.phase=     │
│   Pending                            │
└──────────────────────────────────────┘
                │
    ┌───────────┴───────────┐
    │                       │
单个Pod                  批量Pod
    │                       │
    ▼                       ▼
常规诊断              检查调度器状态
(按决策树)            kubectl get pods -n kube-system
                      -l component=kube-scheduler
```

---

## 6. 生产案例

### 案例: 污点容忍度配置错误导致 Pod 无法调度

**现象**: 新部署的 GPU 工作负载 Pod 持续 Pending

**诊断**:
```
Events:
  Warning  FailedScheduling  default-scheduler
  0/10 nodes are available: 3 node(s) had taint {nvidia.com/gpu: present},
  that the pod didn't tolerate, 7 Insufficient cpu.
```

**根因**: GPU 节点有 `nvidia.com/gpu: present:NoSchedule` 污点，Deployment 未配置对应 tolerations

**修复**: 🟡 在 Pod spec 添加 tolerations + nodeSelector 指向 GPU 节点

---

## 7. 监控告警配置

```yaml
groups:
  - name: pod-pending
    rules:
      - alert: PodPendingTooLong
        expr: kube_pod_status_phase{phase="Pending"} == 1
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 10 分钟"

      - alert: SchedulerDown
        expr: up{job="kube-scheduler"} == 0
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "kube-scheduler 不可用，新 Pod 将无法调度"
```

---

## 8. 版本差异（基于 code/ 源码实证）

> 基于 `code/kubernetes-release-1.28`、`-1.32`、`-1.34`、`kubernetes-1.36.2` 的 `pkg/apis/core/types.go` 与 `code/kube-scheduler-master/framework/types.go` 直接比对，影响调度失败诊断的版本敏感点。

| 特性 / 字段 | 1.28 | 1.32 | 1.34 | 1.36 | 诊断影响 |
|------------|:----:|:----:|:----:|:----:|---------|
| `SchedulingGates` / `SchedulingGated` 状态 | 🅱 beta (`PodSchedulingReadiness`) | ✅ | ✅ GA（无 gate） | ✅ | 1.28+ 排查 Pending 新增分支：`STATUS=SchedulingGated` 表示被门控阻塞，非资源不足 |
| DRA `ResourceClaims`（动态资源分配） | 🅰 alpha | 🅰 alpha | 🅰 alpha | ✅ stable | 1.36 起 `kubectl describe` 可稳定看到 ResourceClaim 绑定；DRA 未就绪也会导致 Pending |
| Pod 级资源 `spec.resources` (`PodLevelResources`) | ❌ | ❌ | 🅰 alpha | 🅰 alpha | 1.34+ 调度器需同时考虑 Pod 级与容器级资源请求 |
| Gang/组调度 `SchedulingGroup`/`PodGroup` | ❌ | ❌ | ❌ | 🅰 alpha (`GenericWorkload`) | 1.36 新增；批调度场景 Pending 需检查组调度策略 |

**调度器事件（`kube-scheduler` framework）**：

- `UpdatePodSchedulingGatesEliminated`：门控清空后触发重新入队——排查"门控已删仍不调度"时确认调度器版本支持。
- `UpdatePodGeneratedResourceClaim`：依赖 DynamicResourceAllocation，DRA 场景 Pending 诊断相关。

**诊断适配要点**：

- ≤ 1.27 集群不存在 `SchedulingGated` 状态，Pending 分支可跳过门控检查。
- 1.28–1.32 上 `SchedulingGates` 为 beta，默认开启；1.34+ 已 GA，为默认行为。
- 基础调度诊断命令（`kubectl describe pod` 看 Events 中 `FailedScheduling`）在 1.18–1.36 全版本通用。

> [存疑：`kube-scheduler-master` 为主干快照，其组调度（PodGroup/Gang）能力领先于 1.36 稳定版，实际集群是否可用取决于所部署 kube-scheduler 的镜像版本]

完整版本矩阵见 [reference/pod-version-differences.md](reference/pod-version-differences.md)。

---

## 9. 快速分级（P0-P3）

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 集群大面积 Pending（跨 namespace），疑似调度器宕机或节点批量不可用 | 立即 | 检查 kube-scheduler 与节点池 |
| **P1** | 关键服务全部副本 Pending 无法扩容 | ≤15min | 快速扩容节点/放宽约束 |
| **P2** | 部分副本 Pending，服务仍可用 | ≤1h | 按决策树定位修复 |
| **P3** | 单 Pod 偶发 Pending 后自愈 | ≤1d | 观察资源水位 |

---

## 10. 证据三元组

```promql
# 🟢 Pending 时长判据
kube_pod_status_phase{phase="Pending"} == 1

# 🟢 节点可分配资源不足判据
sum(kube_pod_container_resource_requests{resource="cpu"}) by (node)
  / sum(kube_node_status_allocatable{resource="cpu"}) by (node) > 0.95
```

| 维度 | 来源 | 取值 |
|------|------|------|
| Metrics | Prometheus | 节点 CPU/内存 requests 占比、Pending 计数 |
| Events | `kubectl describe pod` | `FailedScheduling`：`Insufficient cpu` / `didn't tolerate taint` / `node(s) didn't match node selector` |

---

## 11. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | `kubectl get pod` | STATUS 由 Pending → Running |
| 短期监控 | Pending 告警 | 5min 内无新增 Pending |
| 解决标准 | 副本数达期望 | `readyReplicas == replicas` |
| 回归检测 | 下一次扩容 | 同规格 Pod 可正常调度 |

---

## 12. 升级协议

- 单 Pod Pending 且根因明确（污点/亲和性）→ Agent 提交修复建议，人工审批。
- 批量 Pending 或调度器异常 → 立即升级 P0/P1，禁止 Agent 自动重启 kube-scheduler（🔴 高风险需高级审批）。
- 升级交接信息包：Pending 计数、`FailedScheduling` 事件全文、节点资源水位、最近节点/配额变更记录。

### 常见误诊模式

| 误诊 | 纠正 |
|------|------|
| Pending 一律判为资源不足 | 需区分污点/亲和性/PVC 未绑定/SchedulingGated |
| SchedulingGated 当作调度失败 | 1.28+ 门控状态非资源问题，需查 schedulingGates 字段 |

---

## 相关链接

- [[技能/故障诊断-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[技能/故障诊断-工作负载/pod/01-pod-crashloop-oomkilled.md|CrashLoopBackOff 诊断]]
- [[技能/故障诊断-工作负载/pod/03-pod-imagepull-container.md|镜像拉取诊断]]
- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[生态参考/领域索引/pod-index.md|Pod 知识图谱索引]]
