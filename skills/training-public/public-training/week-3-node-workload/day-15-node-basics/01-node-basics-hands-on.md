---
title: 'Day 15: Node 节点基础实操'
description: '- kubernetes 节点管理'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- prometheus
- containerd
- docker
- daemonset
- job
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 15: Node 节点基础实操 是什么'
- '如何 Day 15: Node 节点基础实操'
trigger_keywords:
- Day
- '15:'
- Node
- 节点基础实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

---
title: Day 15: Node 节点基础实操
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 节点管理
  - kubectl cordon drain uncordon
  - 节点状态 NotReady 排查
  - Pod 调度到特定节点
trigger_keywords:
  - 节点
  - cordon
  - drain
  - uncordon
  - 污点
  - 标签
  - 调度
  - NotReady
  - node
reading_level: intermediate
audience:
  - sre-engineer
  - ops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-07-platform-engineering
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-17-nodepool/01-nodepool-basics-hands-on
---

# Day 15: Node 节点基础实操

> **日期**: Week 3 Day 1 | **主题**: 节点概念、状态与管理操作 | **版本**: K8s 1.28-1.33

---

## 1. 节点核心概念

### 1.1 节点状态

| 状态 | 含义 | 处理方式 |
|------|------|---------|
| `Ready` | 节点健康，Pod 可调度 | 正常 |
| `NotReady` | [[kubelet|kubelet]] 无法上报心跳 | 检查 kubelet/网络 |
| `SchedulingDisabled` | 已 cordoned，新 Pod 不会调度 | 维护前准备 |
| `Unknown` | API Server 无法获取状态 | 检查网络/节点 |
| `MemoryPressure` | 内存不足 | 疏散 Pod 或扩容 |
| `DiskPressure` | 磁盘不足 | 清理或扩容 |

### 1.2 节点元数据

```bash
# 查看节点标签
kubectl get nodes --show-labels

# 常用系统标签
kubernetes.io/hostname          # 主机名
topology.kubernetes.io/zone     # 可用区
topology.kubernetes.io/region   # 区域
node.kubernetes.io/instance-type # 实例类型
```

---

## 2. 节点状态诊断

### 2.1 节点状态速查

```bash
# 查看所有节点状态
kubectl get nodes -o wide

# 查找非 Ready 节点
kubectl get nodes | grep -v Ready

# 查看节点详情（包括 Conditions）
kubectl describe node <node-name>

# 查看节点资源使用
kubectl top nodes
```

### 2.2 NotReady 排查流程

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 1. SSH 到问题节点
ssh <node-ip>

# 2. 检查 kubelet 状态
sudo systemctl status kubelet

# 3. 查看 kubelet 日志
sudo journalctl -u kubelet --since "30m" | tail -50

# 4. 检查网络连通性
ping -c 3 <api-server-ip>

# 5. 检查 Docker/containerd 状态
sudo systemctl status docker  # Kind
sudo systemctl status containerd  # K3s

# 6. 重启 kubelet
sudo systemctl restart kubelet

# 7. 验证恢复
kubectl get nodes | grep <node-name>  # 确认 Ready
```

### 2.3 节点资源压力排查

```bash
# 检查节点内存压力
kubectl describe node <node-name> | grep -A5 "MemoryPressure"

# 查看各 Pod 内存使用
kubectl top pods -A --sort-by=memory | head -20

# 检查节点磁盘压力
ssh <node-ip>
df -h
docker system df  # 如使用 Docker

# 检查 OOM Killer 日志
dmesg | grep -i "out of memory"
sudo journalctl --since "1h" | grep -i "oom"
```

---

## 3. 节点维护操作

### 3.1 Cordon（隔离节点）

**目的**: 阻止新 Pod 调度到该节点（不影响现有 Pod）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

```bash
# 单节点 cordon
kubectl cordon <node-name>

# 批量 cordon（排除 master）
kubectl cordon $(kubectl get nodes --no-headers | grep -v "master" | awk '{print $1}')

# 查看已 cordon 的节点
kubectl get nodes | grep SchedulingDisabled
```

### 3.2 Drain（驱逐 Pod）

**目的**: 安全驱逐节点上所有 Pod（允许优雅终止）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 标准 drain（忽略 DaemonSet）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# drain 并设置超时时间
kubectl drain <node-name> --ignore-daemonsets --grace-period=30 --timeout=120s

# drain 并跳过某些 Pod（如临时 Pod）
kubectl drain <node-name> --ignore-daemonsets \
  --exclude-pods=kube-system/kube-proxy-xxxx \
  --exclude-pods=monitoring/prometheus-xxxx

# 强制 drain（跳过所有检查）
kubectl drain <node-name> --force
```

### 3.3 Uncordon（恢复调度）

```bash
# 单节点恢复
kubectl uncordon <node-name>

# 维护完成后批量恢复
kubectl uncordon $(kubectl get nodes --no-headers | grep "SchedulingDisabled" | awk '{print $1}')
```

### 3.4 维护流程 SOP

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# ========== 节点维护 SOP ==========
# 1. 通知相关团队（变更窗口）

# 2. Cordon 节点
kubectl cordon <node-name>
kubectl get nodes <node-name>  # 确认 SchedulingDisabled

# 3. 驱逐 Pod
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --grace-period=60

# 4. 执行维护（系统更新/硬件更换/重启）
# ... maintenance work ...

# 5. 重启节点
ssh <node-ip> "sudo reboot"

# 6. 等待节点恢复
sleep 30
kubectl get nodes <node-name>  # 等待 Ready

# 7. Uncordon 节点
kubectl uncordon <node-name>

# 8. 验证 Pod 恢复
kubectl get pods -A -o wide | grep <node-name>
# ========== 维护完成 ==========
```

---

## 4. 节点标签与污点管理

### 4.1 标签操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 添加标签
kubectl label node <node-name> topology.kubernetes.io/zone=us-east-1a

# 添加自定义标签
kubectl label node <node-name> workload-type=gpu-intensive

# 更新标签
kubectl label node <node-name> topology.kubernetes.io/zone=us-west-2a --overwrite

# 删除标签
kubectl label node <node-name> workload-type-

# 按标签筛选节点
kubectl get nodes -l "workload-type=gpu-intensive"
```

### 4.2 污点（Taints）操作

```bash
# 添加污点
kubectl taint node <node-name> dedicated=gpu:NoSchedule

# 添加污点（允许特定 Pod 容忍）
kubectl taint node <node-name> node-type=compute:NoExecute:300

# 删除污点
kubectl taint node <node-name> dedicated-

# 查看节点所有污点
kubectl describe node <node-name> | grep -A10 "Taints"
```

### 4.3 Pod 容忍度配置

```yaml
# 容忍特定污点
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  tolerations:
    - key: "dedicated"
      operator: "Equal"
      value: "gpu"
      effect: "NoSchedule"
    - key: "node.kubernetes.io/not-ready"
      operator: "Exists"
      effect: "NoExecute"
      tolerationSeconds: 300  # 节点问题后 300 秒才驱逐
  containers:
    - name: gpu-container
      image: nvidia/cuda:11.0
      resources:
        limits:
          nvidia.com/gpu: 1
```

---

## 5. 节点标签选择器实战

### 5.1 调度到特定节点

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 将 Pod 调度到 GPU 节点
kubectl run gpu-job --image=nvidia/cuda:11.0 \
  --restart=Never \
  -n default \
  --dry-run=client -o yaml | sed 's/containers:/containers:\n        nodeSelector:\n          workload-type: gpu-intensive\n/' | kubectl apply -f -

# 或使用节点亲和性（更灵活）
kubectl run gpu-job2 --image=nvidia/cuda:11.0 --restart=Never -n default
```

### 5.2 Deployment 固定节点池

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: backend
  template:
    spec:
      nodeSelector:
        node-pool: compute-optimized
      tolerations:
        - key: "node-pool"
          operator: "Equal"
          value: "compute-optimized"
          effect: "NoExecute"
      containers:
        - name: backend
          image: backend:v1
          resources:
            requests:
              cpu: "2"
              memory: "4Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
```

---

## 6. 节点问题案例

### 案例 1: 节点 NotReady 后自动恢复

```bash
# 现象：节点变为 NotReady，5 分钟后自动恢复
# 根因：kubelet 偶发性网络抖动
# 排查：journalctl -u kubelet 看到 "connection refused" 后恢复
# 预防：确保 kubelet 自动重启机制 enabled
sudo systemctl enable kubelet
```

### 案例 2: 磁盘压力导致 Pod 被驱逐

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

```bash
# 现象：Pod 被 Evicted，Reason: "DiskPressure"
# 根因：/var/lib/kubelet 使用率 > 85%
# 排查：
ssh <node-ip>
df -h /var/lib/kubelet
docker system prune -a  # 清理未使用的镜像和容器  # ⚠️ 强制清理，可能杀运行中容器

# 预防：设置 eviction threshold 或使用更大磁盘
```

---

## 7. 实战练习

**练习 1**: 将 `node-2` 设为 SchedulingDisabled，验证新 Pod 不会调度

**练习 2**: drain `node-2`，验证 Deployment 自动在其他节点重建 Pod

**练习 3**: 为 GPU 工作负载配置污点容忍，让 Pod 调度到 GPU 节点

**练习 4**: 模拟 NotReady 场景，排查并恢复节点

---


```