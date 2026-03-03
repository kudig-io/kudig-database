# Day 16: Node 节点进阶

> **学习时间**: 4-5 小时 | **主题**: 节点维护、标签与调度约束

---

## 今日目标

- [ ] 掌握节点标签 (Labels) 的管理和用途
- [ ] 理解污点 (Taints) 和容忍 (Tolerations) 机制
- [ ] 掌握 cordon/drain/uncordon 节点维护操作
- [ ] 了解节点维护的最佳实践

---

## 理论学习 (2h)

### 必读文档

1. **节点排障**
   - 文件: `../../../domain-12-troubleshooting/09-node-comprehensive-troubleshooting.md`
   - 重点: 节点维护操作与故障处理

2. **工作负载调度**
   - 文件: `../../../domain-4-workloads/02-deployment-production-patterns.md`
   - 重点: 调度约束配置

---

## 实践任务 (2.5h)

### 任务 1: 节点标签管理 (45min)

```bash
# 查看节点标签
kubectl get nodes --show-labels
kubectl get nodes -l kubernetes.io/os=linux

# 添加自定义标签
kubectl label node <node-name> env=production
kubectl label node <node-name> team=backend

# 使用标签进行调度
cat > label-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: label-test
spec:
  nodeSelector:
    env: production
  containers:
  - name: nginx
    image: nginx:alpine
EOF
kubectl apply -f label-pod.yaml

# 删除标签
kubectl label node <node-name> env-
```

### 任务 2: 污点与容忍 (45min)

```bash
# 查看节点污点
kubectl describe node <node-name> | grep -A 5 Taints

# 添加污点
kubectl taint nodes <node-name> dedicated=gpu:NoSchedule

# 创建带容忍的 Pod
cat > toleration-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: toleration-test
spec:
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
  containers:
  - name: nginx
    image: nginx:alpine
EOF
kubectl apply -f toleration-pod.yaml

# 删除污点
kubectl taint nodes <node-name> dedicated=gpu:NoSchedule-
```

### 任务 3: 节点维护操作 (45min)

```bash
# 1. 标记节点不可调度 (cordon)
kubectl cordon <node-name>
kubectl get nodes  # 状态变为 SchedulingDisabled

# 2. 排水节点 (drain)
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
# 所有非 DaemonSet Pod 会被驱逐到其他节点

# 3. 确认 Pod 已迁移
kubectl get pods -A -o wide | grep <node-name>

# 4. 执行维护操作 (升级、修复等)

# 5. 恢复节点调度 (uncordon)
kubectl uncordon <node-name>
kubectl get nodes
```

### 任务 4: ACK 节点运维操作 (30min)

```bash
# 在 ACK 控制台进行节点运维
# 控制台 -> 集群 -> 节点管理 -> 节点列表

# 操作选项:
# - 排水: 等同于 cordon + drain
# - 移除: 从集群中移除节点
# - 停止调度: 等同于 cordon
# - 恢复调度: 等同于 uncordon
# - 标签管理: 批量添加/删除标签

# 通过 API 排水节点
aliyun cs POST /clusters/<cluster_id>/nodes/drain \
  --body '{"nodes":["<node-name>"],"drain_timeout":300}'
```

---

## 费曼复述 (0.5h)

1. **Taints 和 Tolerations 的工作机制是什么？**
2. **drain 和 cordon 的区别是什么？什么时候用哪个？**
3. **节点维护时如何保证业务不中断？**

---

## 今日检验

- [ ] 能管理节点标签并使用 nodeSelector
- [ ] 能配置 Taints 和 Tolerations
- [ ] 能执行完整的节点维护流程 (cordon -> drain -> 维护 -> uncordon)
- [ ] 了解 ACK 控制台的节点运维操作

---

## 核心概念总结

| 操作 | 效果 | 使用场景 |
|------|------|---------|
| cordon | 禁止新 Pod 调度到该节点 | 计划维护前准备 |
| drain | cordon + 驱逐现有 Pod | 节点维护、升级 |
| uncordon | 恢复节点调度 | 维护完成后恢复 |
| taint | 标记节点特殊用途 | GPU 节点、专用节点 |

---

## 明日预告

Day 17 将学习 ACK 节点池的基础概念与创建配置。
