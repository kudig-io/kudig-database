# Pod Group Policies

## 概述
Pod Group Policies 是 Workload API 的组成部分（Alpha，v1.35 默认禁用）。Workload 中定义的每个 Pod 组都必须声明一个调度策略，该策略决定调度器如何处理该组 Pod 的集合。

## 核心概念/原理
目前 API 支持两种策略类型，每个组必须且只能指定一种：
1. **basic（基本策略）**
   - 调度器将组内所有 Pod 视为独立实体，按标准 Kubernetes 行为逐个调度。
   - 主要用于组织 Pod 以提升可观测性和管理性，适用于不需要同时启动的组，或为未来引入非“全有或全无”约束做准备。
2. **gang（集体调度策略）**
   - 强制“全有或全无”调度（gang scheduling）。
   - 适用于紧耦合工作负载，部分启动会导致死锁或资源浪费的场景（如分布式训练、批处理作业）。
   - 需要指定 `minCount` 参数：只有当至少有 `minCount` 个 Pod 能够同时调度时，该组才会被允许绑定到节点。

## 关键机制或特性
- **策略冲突**：一个 Pod 组不能同时指定两种策略。
- **调度器行为**：
  - `basic`：Pod 独立参与调度，失败不会影响组内其他 Pod。
  - `gang`：组内 Pod 会等待彼此都被创建并满足 `minCount` 后，才一起进行绑定。

## 使用场景
- **basic**：逻辑上属于同一应用但启动顺序无关的组件，或当前仅需分组标签的场景。
- **gang**：
  - 需要所有 worker 同时运行的机器学习训练任务。
  - 基于仲裁或全连接的分布式计算框架（如 MPI）。

## 最佳实践/注意事项
- 使用 gang 策略时，确保 `minCount` 设置合理：过小可能导致资源碎片，过大可能导致调度失败。
- 需要 gang 调度时，务必启用 `GangScheduling` 特性门控。
- 对于不依赖同时启动的通用应用，优先使用 `basic` 策略，避免不必要的调度复杂度。

## 生产 YAML 示例

### Basic 策略 — 逻辑分组

```yaml
apiVersion: scheduling.k8s.io/v1alpha1
kind: Workload
metadata:
  name: microservice-app
  namespace: production
spec:
  controllerRef:
    apiGroup: apps
    kind: Deployment
    name: web-frontend
  podGroups:
  # 前端组：独立调度，仅用于可观测性分组
  - name: frontend
    policy:
      basic: {}
  # 后端组：独立调度
  - name: backend
    policy:
      basic: {}
  # 缓存组：独立调度
  - name: cache
    policy:
      basic: {}
```

### Gang 策略 — 分布式训练

```yaml
apiVersion: scheduling.k8s.io/v1alpha1
kind: Workload
metadata:
  name: pytorch-ddp-job
  namespace: ml-team
spec:
  controllerRef:
    apiGroup: batch
    kind: Job
    name: train-llama-3
  podGroups:
  # Coordinator：单副本，独立调度
  - name: coordinator
    policy:
      basic: {}
  # Workers：必须同时调度，否则全部等待
  - name: workers
    policy:
      gang:
        minCount: 16          # 16 GPU 全部就绪才开始训练
```

### Gang 策略 — 部分容错（minCount < 总数）

```yaml
apiVersion: scheduling.k8s.io/v1alpha1
kind: Workload
metadata:
  name: elastic-training
  namespace: ml-team
spec:
  podGroups:
  - name: elastic-workers
    policy:
      gang:
        minCount: 8           # 最少 8 个 worker 就能开始
                              # 如果集群有更多资源，可以调度更多
                              # 适用于支持弹性训练的框架（如 PyTorch Elastic）
```

## 策略对比矩阵

| 维度 | basic | gang |
|------|-------|------|
| 调度方式 | 逐个独立调度 | 全有或全无（满足 minCount） |
| 调度延迟 | 低（立即调度可用 Pod） | 高（等待所有 Pod 同时就绪） |
| 资源利用率 | 高（按需分配） | 可能因等待导致短暂浪费 |
| 死锁风险 | 无 | 低（通过 minCount 控制） |
| 部分启动 | 允许 | 不允许（低于 minCount） |
| 适用场景 | 微服务、无状态应用 | 分布式训练、MPI、批处理 |
| 额外特性门控 | 无 | 需要 `GangScheduling` |
| 失败影响 | 单 Pod 失败不影响组 | 可能触发整组回滚 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Gang 组 Pod 全部 Pending | 集群资源不足以同时满足 minCount 个 Pod | `kubectl describe nodes` 汇总可分配资源；考虑降低 minCount |
| 部分 Pod 已调度但未运行 | minCount 未满足，已调度 Pod 等待绑定 | `kubectl get pods -l ...` 检查 Running vs Pending 比例 |
| basic 策略 Pod 调度失败 | 与策略无关，可能是节点亲和性/资源不足 | 标准 Pod 调度排查流程 |
| Gang 组频繁超时 | minCount 设置过大或集群碎片化严重 | 减小 minCount 或增加节点；检查 ResourceQuota |
| 策略配置报错 | 同一 podGroup 同时指定了 basic 和 gang | 确保每个 podGroup 只声明一种策略 |

## 生产检查清单

- [ ] 确认 `GenericWorkload` 特性门控已启用
- [ ] gang 策略场景同时启用 `GangScheduling` 特性门控
- [ ] gang 的 `minCount` 不超过集群可分配资源能同时容纳的 Pod 数
- [ ] 对弹性训练框架，`minCount` 设置为最小可运行副本数（非总数）
- [ ] 不需要同时启动的组件使用 `basic` 策略，降低调度复杂度
- [ ] 监控 gang 组的调度等待时间，超过阈值告警
- [ ] ResourceQuota 设置合理，不会阻止 gang 组获得足够资源

## 命令快速参考

```bash
# 查看 Workload 及其 podGroups 定义
kubectl get workload <name> -n <ns> -o yaml

# 验证 gang 特性门控
kubectl get --raw /apis/scheduling.k8s.io/v1alpha1 | jq .

# 检查 gang 组的 Pod 状态分布
kubectl get pods -l workload=<name> --field-selector=status.phase=Pending

# 查看调度器事件中的 gang 调度决策
kubectl get events -n <ns> --field-selector reason=GangScheduling
```

## 交叉引用

- [Workload API](workload-api.md) — 完整 Workload 资源定义
- [Workload Reference](workload-reference.md) — Pod 端引用 Workload 的方式
- [Jobs](jobs.md) — 批处理场景下的 gang 调度应用
- [调度与驱逐](../scheduling/) — 调度器核心行为

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/workload-api/policies/
