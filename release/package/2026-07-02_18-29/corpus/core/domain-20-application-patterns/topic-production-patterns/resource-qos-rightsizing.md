---
title: 资源 QoS 与 Right-sizing 生产模式
description: 生产级资源管理：requests/limits 设计、QoS 等级、VPA 自动调优与 right-sizing 实践
summary: 生产级资源管理：requests/limits 设计、QoS 等级、VPA 自动调优与 right-sizing 实践，含容量规划与成本优化清单。
category: application-patterns
tags:
- resource
- qos
- vpa
- right-sizing
- finops
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 16min
intent_queries:
- 资源 QoS 生产模式是什么
- 如何做 K8s right-sizing
trigger_keywords:
- requests
- limits
- QoS
- VPA
- right-sizing
- 资源管理
prerequisites:
- kubectl-basics
- resource-management
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。

# 资源 QoS 与 Right-sizing 生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

资源 requests/limits 的设置直接决定 Pod 的调度、驱逐行为和成本。错误的资源配置是生产环境三大常见根因之一：requests 过低导致节点超卖和 OOM/驱逐，requests 过高导致资源浪费和成本翻倍。本文涵盖 QoS 等级机制、生产配置原则、VPA 自动调优和 right-sizing 方法论。

---

## 1. QoS 等级机制

Kubernetes 根据 requests/limits 配置将 Pod 分为三个 QoS 等级，决定资源不足时的驱逐优先级：

| QoS 等级 | 条件 | 驱逐顺序 | 适用场景 |
|---|---|---|---|
| **Guaranteed** | CPU/内存 requests == limits（所有容器） | 最后被驱逐 | 核心服务、延迟敏感型 |
| **Burstable** | 至少一个容器有 requests（但 requests ≠ limits） | 中间 | 可弹性突发的工作负载 |
| **BestEffort** | 无 requests/limits | 最先被驱逐 | 批处理、可丢弃任务 |

> ⚠️ **生产红线**: 核心服务**必须避免 BestEffort**。节点资源压力时 BestEffort Pod 最先被 evict，导致不可预期中断。

### 驱逐触发条件

```
节点内存压力 (memory.available < evictionHard[memory]):
  驱逐顺序: BestEffort → Burstable → Guaranteed

节点磁盘压力 (imagefs.available / nodefs.available < 阈值):
  优先清理已终止 Pod 和未使用镜像，再按 QoS 驱逐
```

---

## 2. 生产配置原则

### 2.1 CPU 与内存的不同本质

| 维度 | CPU | 内存 |
|---|---|---|
| 可压缩性 | 可压缩（throttling） | 不可压缩（OOM Kill） |
| requests 含义 | 调度权重 + CFS 配额下限 | 调度依据 + OOM 分数基准 |
| limits 超限后果 | CPU throttling（延迟飙升） | OOMKilled（进程死亡） |
| 生产建议 | 设 limits（防突发影响邻居）| **limits ≤ 节点内存 80%**，留余量 |

> ⚠️ **CPU throttling 隐患**: 即使 CPU 使用率看起来不高（如 60%），如果设置了 limits 且使用接近 limits，CFS 会 throttling 导致 P99 延迟飙升。对延迟敏感服务，考虑不设 CPU limits 或使用 cpu manager static 策略绑定独占核心。

### 2.2 生产资源配置模板

```yaml
containers:
  - name: api
    resources:
      requests:
        cpu: "500m"      # 保证最小算力 → 调度依据
        memory: "512Mi"  # 保证最小内存 → OOM 基准
      limits:
        memory: "1Gi"    # 设内存上限防 OOM 影响邻居
        # cpu 不设 limits → 允许突发利用空闲 CPU（Guaranteed 场景除外）
```

### 2.3 关键场景决策

| 场景 | CPU requests | CPU limits | Memory requests | Memory limits | QoS |
|---|---|---|---|---|---|
| 核心 API（延迟敏感） | 1000m | 1000m | 2Gi | 2Gi | Guaranteed |
| 普通 Web 服务 | 250m | 不设 | 256Mi | 512Mi | Burstable |
| 批处理 Job | 500m | 2000m | 1Gi | 1Gi | Burstable |
| 可丢弃任务 | 不设 | 不设 | 不设 | 不设 | BestEffort |

---

## 3. VPA 自动调优

Vertical Pod Autoscaler 根据 历史 metrics 自动推荐/应用 requests 值。

### 3.1 VPA 工作模式

| 模式 | 行为 | 生产建议 |
|---|---|---|
| `Off` | 仅生成建议，不修改 | 首次接入时用此模式观察 |
| `Initial` | 仅在 Pod 创建时设置 requests | ✅ 推荐：不影响运行中 Pod |
| `Recreate` | 运行中也修改（重启 Pod） | 谨慎使用：会触发重启 |
| `Auto` | 等价于 Recreate | 不推荐生产直接用 |

> ⚠️ **VPA 与 HPA 冲突**: 若 HPA 基于 CPU 使用率，VPA 不能同时调 CPU requests（两者会互相打架）。VPA 调内存 + HPA 调副本数是常见组合。

### 3.2 VPA 建议模式部署示例

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: api-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: "apps/v1"
    kind: Deployment
    name: api-server
  updatePolicy:
    updateMode: "Off"   # 仅观察，生成建议
  resourcePolicy:
    containerPolicies:
      - containerName: '*'
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: 4000m
          memory: 8Gi
```

查看建议: 🟢
```bash
kubectl describe vpa api-vpa -n production | grep -A10 Recommendation
```

---

## 4. Right-sizing 方法论

### 4.1 四步 Right-sizing 流程

```
Step 1: 采集基线 (≥ 7 天)
  → 使用 metrics-server / Prometheus 采集真实使用量
  → 关注 P95/P99 而非平均值

Step 2: 计算推荐值
  → 推荐 requests = max(P95 使用量 × 1.2, 当前 requests × 0.7)
  → VPA Off 模式可自动化此步

Step 3: 灰度验证
  → 先在非生产环境调整，观察 3 天
  → 逐步推进到生产 (Canary 副本)

Step 4: 周期复审
  → 每月评审一次，配合 FinOps 报表
```

### 4.2 快速诊断命令 🟢

```bash
# 查看 Pod 实际资源使用 vs requests
kubectl top pods -n production --sort-by=memory

# 对比 requests 与实际使用（需 metrics-server）
kubectl get pods -n production -o json | jq '.items[] | {
  name: .metadata.name,
  req_cpu: .spec.containers[0].resources.requests.cpu,
  req_mem: .spec.containers[0].resources.requests.memory
}'

# 找出 requests 远超使用的 Pod（浪费资源）
# 使用 kubectl-resources 或 Kubecost 报表
```

### 4.3 LimitRange 兜底

防止遗漏配置的 Pod 影响节点：

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: production
spec:
  limits:
    - default:           # 未设 limits 时的默认值
        cpu: 1000m
        memory: 1Gi
      defaultRequest:    # 未设 requests 时的默认值
        cpu: 100m
        memory: 128Mi
      max:               # 单容器上限
        cpu: 8000m
        memory: 16Gi
      type: Container
```

---

## 5. 生产检查清单

| # | 检查项 | 验证命令 | 合格标准 |
|---|---|---|---|
| 1 | 核心服务无 BestEffort Pod | `kubectl get pods -A -o json \| jq` 检查 resources | 所有生产 Pod 有 requests |
| 2 | 内存 limits 已设置 | `kubectl get deploy -A -o yaml \| grep -A3 limits` | 核心服务均有 memory limits |
| 3 | 节点资源超售率可控 | `kubectl describe node \| grep -A5 Allocatable` | CPU requests 总和 < 节点容量 × 80% |
| 4 | LimitRange 已配置 | `kubectl get limitrange -A` | 每个 namespace 有兜底 |
| 5 | VPA 建议模式已部署 | `kubectl get vpa -A` | 核心服务有 VPA Off 模式采集 |
| 6 | 定期 right-sizing 复审 | FinOps 月报 | requests 与 P95 使用偏差 < 30% |

---

## 6. 排障速查

| 症状 | 可能根因 | 诊断命令 | 修复 |
|---|---|---|---|
| Pod OOMKilled | memory limits 过低 / 内存泄漏 | `kubectl describe pod` 看 Reason | 提升 limits 或修复内存泄漏 |
| P99 延迟飙升但 CPU 不高 | CPU throttling (limits 过低) | 检查 cfs throttling 指标 | 提升/移除 CPU limits 或调大 CPU |
| Pod 频繁被 Evict | BestEffort / 节点资源压力 | `kubectl get events \| grep Evicted` | 设 requests 升级 QoS 或扩容节点 |
| 调度 Pending | requests 过大超节点容量 | `kubectl describe pod` 看 Events | 降低 requests 或加节点 |
| 节点 NotReady | 节点内存耗尽（超卖） | `kubectl describe node` 看 conditions | 收紧 requests 审计 + 加 ResourceQuota |

---

## 7. 跨域协作

- **Pod 可用性与 PDB**: 见 [[生产模式/pod-availability-lifecycle|Pod 可用性生产模式]]
- **调度与拓扑分布**: 见 [[生产模式/scheduling-topology-patterns|调度与拓扑分布模式]]
- **FinOps 成本治理**: 见 `domain-11-production-operations/01-finops/14-finops-cost-governance-runbook.md`


<!-- risk-assessed -->
