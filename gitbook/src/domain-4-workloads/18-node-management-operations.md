# 27 - 节点与节点池管理 (Node & NodePool Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [ACK NodePool](https://help.aliyun.com/document_detail/160490.html)

## 节点池 (NodePool) 核心架构

| 功能 (Feature) | 描述 (Description) | 生产建议 (Best Practice) |
|---------------|-------------------|--------------------------|
| **弹性伸缩 (ASG)** | 自动增加/减少 ECS 实例 | 开启 `cluster-autoscaler` 配合 HPA/VPA, 为不同业务建独立 NodePool |
| **多规格混合** | 选定多种 ECS 规格 | 将 Spot 与按量/包年实例混用, 设置合适 `expander` 策略(如 `least-waste`) |
| **自定义 OS** | 支持 Alibaba Cloud Linux / ContainerOS | 生产环境推荐 ContainerOS, 统一 OS 版本, 禁止手工登录改配置 |
| **自动修复** | 节点 NotReady 时自动重启或替换 | 关键生产环境必须开启, 同时结合 `PodDisruptionBudget` 控制重启节奏 |
| **分级隔离** | 不同安全等级/环境的节点池 | 通过 Node 标签/污点严格区分 `prod/staging/dev` 与 `internet/intranet` |

## 节点生命周期与运维流程 (Node Lifecycle)

| 阶段 | 关键操作 | 建议命令 | 注意事项 |
|------|----------|----------|----------|
| **准备 (Provision)** | 通过 NodePool 创建/扩容节点 | ACK 控制台或 `cluster-autoscaler` | 统一镜像与 kubelet 配置, 预置监控/日志 DaemonSet |
| **接入 (Join)** | 节点加入集群并打标签 | `kubectl label nodes` / `kubectl taint nodes` | 加入后立刻补齐 `env=prod`、`zone=xxx` 等业务标签 |
| **维护 (Maintain)** | 打补丁/升级内核/重启宿主机 | `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` | 搭配 PDB, 控制同时维护的节点数量 |
| **下线 (Decommission)** | 永久移除节点 | `kubectl drain` → `kubectl delete node` | 先确认无绑定本地盘/本地日志, 相关 Pod 已在其他节点稳定运行 |

```bash
# 查看节点 & 池
kubectl get nodes -o wide
kubectl get nodepool -A 2>/dev/null || echo "在 ACK 控制台查看 NodePool 配置"

# 按标签筛选节点
kubectl get nodes -l env=prod
```

## 调度与隔离: 标签与污点 (Label & Taint)

| 能力 | 示例 | 作用 |
|------|------|------|
| **节点标签 (Label)** | `node.kubernetes.io/instance-type=ecs.g7.xlarge` | 匹配大规格计算节点, 用于 CPU 密集型服务 |
| | `zone=cn-hangzhou-h` | 控制跨 AZ 分布, 与 PV 拓扑、SLB 匹配 |
| **节点污点 (Taint)** | `kubectl taint nodes node1 role=system:NoSchedule` | 仅允许带对应容忍 (Toleration) 的系统 Pod 调度上去 |
| **Pod 亲和/反亲和** | `topologyKey: kubernetes.io/hostname` | 同一业务副本分散到不同节点/机架, 提升高可用 |

```yaml
# 专用 GPU 节点池示例
apiVersion: v1
kind: Pod
metadata:
  name: gpu-job
spec:
  nodeSelector:
    aliyun.accelerator/nvidia_name: "V100"
  tolerations:
  - key: "gpu-only"
    operator: "Equal"
    value: "true"
    effect: "NoSchedule"
```

## 资源预留 (Resource Reservation)

通过 `kubelet` 参数控制系统稳定性：
- `--system-reserved`: CPU/Memory 为 OS 进程预留。
- `--kube-reserved`: 为 K8s 组件 (Kubelet, Proxy) 预留。
- `--eviction-hard`: 设置硬驱逐阈值 (如 `memory<500Mi`) 防范宿主机崩溃。

> **生产建议**: 将系统/组件预留总和控制在节点容量的 10%~20%, 对大节点适当上浮。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)