# Resource Bin Packing

## 概述

资源装箱（Resource Bin Packing）是 kube-scheduler 中 `NodeResourcesFit` 插件的两种评分策略，用于提高集群资源利用率。这两种策略分别是 `MostAllocated` 和 `RequestedToCapacityRatio`。

## 核心概念/原理

### MostAllocated 策略

`MostAllocated` 策略基于资源利用率对节点进行评分，优先选择分配率更高的节点。对于每种资源类型，可以设置权重来修改其对节点评分的影响。

### RequestedToCapacityRatio 策略

`RequestedToCapacityRatio` 策略允许用户指定资源及其权重，根据请求量与容量之比来对节点评分。这使得用户可以通过适当的参数对扩展资源进行装箱，以提高稀缺资源在大型集群中的利用率。

## 关键机制或特性

### MostAllocated 配置示例

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
- pluginConfig:
  - args:
      scoringStrategy:
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
        - name: intel.com/foo
          weight: 3
        - name: intel.com/bar
          weight: 3
        type: MostAllocated
    name: NodeResourcesFit
```

### RequestedToCapacityRatio 配置示例

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
- pluginConfig:
  - args:
      scoringStrategy:
        resources:
        - name: intel.com/foo
          weight: 3
        - name: intel.com/bar
          weight: 3
        requestedToCapacityRatio:
          shape:
          - utilization: 0
            score: 0
          - utilization: 100
            score: 10
        type: RequestedToCapacityRatio
    name: NodeResourcesFit
```

### Shape 调优

`shape` 用于指定 `RequestedToCapacityRatio` 函数的行为：

- **装箱行为**（bin packing）：
  ```yaml
  shape:
    - utilization: 0
      score: 0
    - utilization: 100
      score: 10
  ```
  利用率为 0% 时得分为 0，100% 时得分为 10。

- **最少请求行为**（least requested）：
  ```yaml
  shape:
    - utilization: 0
      score: 10
    - utilization: 100
      score: 0
  ```

### Resources 参数

`resources` 参数默认包含 CPU 和 memory，权重均为 1。可以用来添加扩展资源：

```yaml
resources:
  - name: intel.com/foo
    weight: 5
  - name: cpu
    weight: 3
  - name: memory
    weight: 1
```

权重不能设置为负值，未指定时默认为 1。

## 使用场景

- 希望提高集群整体资源利用率，减少碎片化。
- 需要对稀缺扩展资源（如 FPGA、专用网卡）进行高效利用。
- 大型集群中希望通过装箱策略减少空闲节点数量，从而节省成本。

## 最佳实践/注意事项

- `MostAllocated` 更适合通用的装箱需求，配置简单。
- `RequestedToCapacityRatio` 更适合需要精细控制评分曲线的场景，特别是扩展资源。
- 权重设置应根据资源的重要程度进行调整，权重越高，该资源对最终节点评分的影响越大。
- 使用 `shape` 可以灵活切换装箱和最少请求两种行为模式。
- 配置完成后，通过 kube-scheduler 的 `--config` 参数引用配置文件。

## 生产 YAML 示例

### 生产环境 MostAllocated 配置（GPU 集群装箱）

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  - schedulerName: gpu-bin-packing-scheduler
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: MostAllocated
            resources:
              - name: nvidia.com/gpu
                weight: 10                 # GPU 权重最高，优先填满 GPU 节点
              - name: cpu
                weight: 2
              - name: memory
                weight: 1
    plugins:
      score:
        enabled:
          - name: NodeResourcesFit
            weight: 5
        disabled:
          - name: NodeResourcesBalancedAllocation  # 禁用均衡分配，改用装箱
```

### RequestedToCapacityRatio 精细调优（扩展资源装箱 + CPU 均衡）

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  - schedulerName: hybrid-scheduler
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: RequestedToCapacityRatio
            resources:
              - name: nvidia.com/gpu
                weight: 8                  # GPU 资源高权重 — 装箱行为
              - name: cpu
                weight: 2                  # CPU 低权重
              - name: memory
                weight: 1
            requestedToCapacityRatio:
              shape:
                - utilization: 0
                  score: 0
                - utilization: 50
                  score: 7                 # 50% 利用率得 7 分
                - utilization: 100
                  score: 10                # 100% 利用率得满分
```

## 评分策略对比

| 策略 | 行为 | 适用场景 | 配置复杂度 |
|------|------|----------|-----------|
| `LeastAllocated`（默认） | 优先选择空闲节点 | 通用工作负载，追求资源均衡 | 低 |
| `MostAllocated` | 优先填满节点 | GPU 集群、成本优化、减少节点数 | 低 |
| `RequestedToCapacityRatio` | 自定义利用率-分数曲线 | 精细控制，扩展资源优化 | 中 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| MostAllocated 后节点资源争抢严重 | Burstable Pod 实际使用超过 requests | 调整 Pod 的 requests 更接近实际使用；监控节点 CPU throttling |
| 装箱后部分节点完全空闲但不回收 | Cluster Autoscaler / Karpenter 未感知 | 检查 Autoscaler 的 scale-down 配置 |
| 扩展资源权重设置后评分无变化 | 资源名称拼写错误或节点未注册该资源 | `kubectl describe node` 检查 Capacity 中是否有该扩展资源 |
| Shape 调优后评分不符合预期 | utilization-score 曲线设置不合理 | 使用分段线性函数模拟计算验证 |

## 生产检查清单

- [ ] GPU 集群使用 `MostAllocated` 策略减少空闲 GPU 节点
- [ ] 通用集群保持默认 `LeastAllocated` 或 `BalancedAllocation`
- [ ] 为扩展资源（GPU、FPGA、SR-IOV）设置较高权重
- [ ] 配合 Karpenter / Cluster Autoscaler 在节点空闲时缩容
- [ ] 使用 `--config` 参数将配置文件传递给 kube-scheduler
- [ ] 监控节点资源利用率分布，验证装箱效果
- [ ] 避免在 CPU/Memory 上过度装箱导致性能劣化

## 命令快速参考

```bash
# 查看节点资源利用率
kubectl top nodes

# 查看节点 Allocatable vs Requests
kubectl describe node <node-name> | grep -A 15 "Allocated resources"

# 查看调度器配置
kubectl get cm -n kube-system kube-scheduler-config -o yaml

# 查看节点的扩展资源
kubectl get node <node-name> -o jsonpath='{.status.capacity}' | jq .

# 按 CPU 利用率排序节点
kubectl top nodes --sort-by=cpu

# 验证调度器使用的配置文件
kubectl logs -n kube-system -l component=kube-scheduler | grep -i "scoring strategy"
```

## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 评分阶段如何使用 NodeResourcesFit
- [调度框架](./scheduling-framework.md) — Score 扩展点与 NormalizeScore
- [调度器性能调优](./scheduler-performance-tuning.md) — 评分节点数量对装箱效果的影响
- [动态资源分配](./dynamic-resource-allocation.md) — DRA 设备与装箱策略的交互
- [Karpenter 自动扩缩容](./karpenter-autoscaling.md) — Karpenter 的整合机制与装箱互补

## 参考链接

- [Kubernetes 官方文档 - Resource Bin Packing](https://kubernetes.io/docs/concepts/scheduling-eviction/resource-bin-packing/)
