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

## 参考链接

- [Kubernetes 官方文档 - Resource Bin Packing](https://kubernetes.io/docs/concepts/scheduling-eviction/resource-bin-packing/)
