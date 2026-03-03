# Day 18: 节点池进阶

> **学习时间**: 4-5 小时 | **主题**: 节点池扩缩容与生命周期管理

---

## 今日目标

- [ ] 掌握节点池手动扩缩容操作
- [ ] 理解自动伸缩 (Cluster Autoscaler) 配置
- [ ] 了解节点池滚动更新与删除
- [ ] 能够设计合理的节点池架构

---

## 理论学习 (2h)

### 必读文档

1. **集群自动伸缩排障**
   - 文件: `../../../domain-12-troubleshooting/28-cluster-autoscaler-troubleshooting.md`
   - 重点: 自动伸缩原理与常见问题

2. **ACK ECS 计算**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/240-ack-ecs-compute.md`
   - 重点: 实例规格与伸缩组

---

## 实践任务 (2.5h)

### 任务 1: 手动扩缩容 (45min)

```bash
# 扩容节点池
aliyun cs POST /clusters/<cluster_id>/nodepools/<nodepool_id> \
  --body '{"count": 1}'

# 等待新节点 Ready
kubectl get nodes -w

# 缩容 (移除节点)
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<nodepool_id>/nodes \
  --body '{"nodes":["<node-id>"],"release_node":true,"drain_node":true}'
```

### 任务 2: 自动伸缩配置 (45min)

```bash
# 启用节点池自动伸缩
aliyun cs PUT /clusters/<cluster_id>/nodepools/<nodepool_id> \
  --body '{
    "auto_scaling": {
      "enable": true,
      "min_instances": 2,
      "max_instances": 10,
      "type": "cpu",
      "is_bond_eip": false
    }
  }'

# 验证 Cluster Autoscaler 运行状态
kubectl get pods -n kube-system -l app=cluster-autoscaler
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=20
```

### 任务 3: 节点池更新与删除 (30min)

```bash
# 更新节点池配置 (如修改标签)
aliyun cs PUT /clusters/<cluster_id>/nodepools/<nodepool_id> \
  --body '{
    "kubernetes_config": {
      "labels": [
        {"key": "workload", "value": "web"},
        {"key": "tier", "value": "frontend"}
      ]
    }
  }'

# 删除节点池
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<nodepool_id>
```

### 任务 4: 节点池架构设计 (30min)

```
# 生产环境节点池设计示例:

# 1. system-pool: 系统组件专用
#    - 实例: ecs.g6.xlarge (4C16G)
#    - 数量: 2-3
#    - 污点: CriticalAddonsOnly=true:NoSchedule
#    - 用途: monitoring, ingress-controller

# 2. app-pool: 业务应用
#    - 实例: ecs.g6.2xlarge (8C32G)
#    - 数量: 3-10 (自动伸缩)
#    - 标签: workload=app

# 3. gpu-pool: GPU 工作负载 (可选)
#    - 实例: ecs.gn6i-c4g1.xlarge
#    - 数量: 1-5
#    - 污点: nvidia.com/gpu=true:NoSchedule
```

---

## 费曼复述 (0.5h)

1. **Cluster Autoscaler 如何决定何时扩容/缩容？**
2. **为什么生产环境推荐将系统组件和业务分到不同节点池？**
3. **节点池扩容失败的常见原因有哪些？**

---

## 今日检验

- [ ] 能通过 API 进行节点池扩缩容
- [ ] 能配置自动伸缩策略
- [ ] 了解节点池更新和删除操作
- [ ] 能设计合理的多节点池架构

---

## 核心概念总结

| 伸缩方式 | 触发条件 | 适用场景 |
|----------|---------|---------|
| 手动扩缩容 | 人工触发 | 预期内的流量变化 |
| 自动伸缩 | Pod Pending (资源不足) | 突发流量、弹性场景 |

---

## 明日预告

Day 19 将学习 Pod 容器组的生命周期与基本操作。
