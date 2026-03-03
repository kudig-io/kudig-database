# Week 3: 节点与工作负载管理 (Days 15-21)

## 本周目标

- 掌握 Node 节点基础与进阶管理操作
- 理解节点池概念与生命周期管理
- 掌握 Pod 容器组基础与进阶配置
- 了解 K8S 核心组件运维方法
- **产出**: 能够管理节点池、排查 Pod 问题、维护 K8S 核心组件

---

## 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 15 | Node 节点基础 | [day-15-node-basics.md](./day-15-node-basics.md) |
| Day 16 | Node 节点进阶 | [day-16-node-advanced.md](./day-16-node-advanced.md) |
| Day 17 | 节点池基础 | [day-17-nodepool-basics.md](./day-17-nodepool-basics.md) |
| Day 18 | 节点池进阶 | [day-18-nodepool-advanced.md](./day-18-nodepool-advanced.md) |
| Day 19 | Pod 容器组基础 | [day-19-pod-basics.md](./day-19-pod-basics.md) |
| Day 20 | Pod 容器组进阶 | [day-20-pod-advanced.md](./day-20-pod-advanced.md) |
| Day 21 | K8S 组件运维 | [day-21-component-ops.md](./day-21-component-ops.md) |

---

## 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

---

## 本周实践项目

**项目 P3**: [节点与工作负载运维实战](../projects/p3-node-workload-management.md)

---

## 学习建议

1. **Day 15-16**: 节点是集群的基石，理解状态、标签、污点是关键
2. **Day 17-18**: 节点池是 ACK 特色功能，掌握它能大幅提升运维效率
3. **Day 19-20**: Pod 是最核心的工作单元，生命周期和调度是重中之重
4. **Day 21**: 组件运维能力决定了你能否独立处理集群级故障

---

## 关键概念清单

- [ ] Node 状态 (Ready/NotReady) 与 Condition
- [ ] 标签 (Labels) 和污点 (Taints) 的作用
- [ ] cordon、drain、uncordon 操作
- [ ] ACK 节点池的创建、扩缩容、配置
- [ ] 托管节点池 vs 自管理节点池
- [ ] Pod 生命周期与状态
- [ ] 探针: liveness、readiness、startup
- [ ] Pod 调度: nodeSelector、affinity、toleration
- [ ] kube-system 核心组件状态检查与恢复
