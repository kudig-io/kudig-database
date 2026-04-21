# 节点弹性伸缩

## 源码路径

`cluster-autoscaler/`
`pkg/controller/nodelifecycle/`

---

## Cluster Autoscaler 原理

```
集群状态:
  ┌─────────────────────────────────────────────────────────────┐
  │  Pod 调度失败 ( Insufficient CPU/memory)                    │
  └─────────────────────────────────────────────────────────────┘
                           │
                           ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  Cluster Autoscaler 检测到 unschedulable Pod                │
  └─────────────────────────────────────────────────────────────┘
                           │
                           ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  向云厂商 API 请求创建新节点                                  │
  └─────────────────────────────────────────────────────────────┘
                           │
                           ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  新节点加入集群 (kubelet bootstrap)                          │
  └─────────────────────────────────────────────────────────────┘
                           │
                           ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  Pod 调度到新节点                                           │
  └─────────────────────────────────────────────────────────────┘
```

---

## 部署 Cluster Autoscaler

```yaml
# AWS EKS
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cluster-autoscaler
  template:
    metadata:
      labels:
        app: cluster-autoscaler
    spec:
      containers:
      - image: registry.k8s.io/autoscaling/cluster-autoscaler:v1.28.0
        name: cluster-autoscaler
        args:
        - --cloud-provider=aws
        - --nodes=1:10:default
        - --scale-down-delay-after-add=10m
        - --scale-down-unneeded-time=10m
        env:
        - name: AWS_REGION
          value: us-east-1
```

---

## Scale Down

```bash
# 节点空闲缩容条件:
# 1. 节点上所有 Pod 已迁移
# 2. 节点空闲时间超过 --scale-down-unneeded-time (默认 10 分钟)
# 3. Pod 可被驱逐 (无 PDB 阻止)

# 缩容流程:
# 1. 选择空闲节点
# 2. 驱逐所有 Pod
# 3. 向云厂商 API 请求删除节点
```

---

## 节点组 (Node Pool)

```bash
# AWS: Auto Scaling Group (ASG)
# GCP: Instance Group Manager (IGM)
# Azure: Virtual Machine Scale Set (VMSS)

# Cluster Autoscaler 监控多个节点组:
--nodes=1:10:pool1   # pool1: min=1, max=10
--nodes=1:5:pool2    # pool2: min=1, max=5
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Pod 一直 Pending | 无可扩容的节点组 | 检查 autoscaler 日志 |
| 节点无法缩容 | PDB 阻止 | 调整 PDB |
| 新节点无法加入 | Token 过期 | 刷新 bootstrap token |
