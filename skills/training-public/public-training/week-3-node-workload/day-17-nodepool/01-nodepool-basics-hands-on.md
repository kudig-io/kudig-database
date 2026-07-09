---
title: 'Day 17: 节点池基础实操'
description: '- 工作负载调度到节点池'
summary: '- 工作负载调度到节点池'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- daemonset
- operator
- gpu
- nvidia
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 17: 节点池基础实操 是什么'
- '如何 Day 17: 节点池基础实操'
trigger_keywords:
- Day
- '17:'
- 节点池基础实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 17: 节点池基础实操
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 节点池创建
  - 新节点加入集群
  - 节点池扩缩容
  - 工作负载调度到节点池
trigger_keywords:
  - 节点池
  - node-pool
  - kubeadm
  - 扩缩容
  - 调度
  - 标签
  - 污点
reading_level: intermediate
audience:
  - sre-engineer
  - ops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - 平台工程
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
  - 生产运维/topic-learn/public-training/week-3-node-workload/day-18-nodepool-advanced/01-nodepool-advanced-hands-on
---

# Day 17: 节点池基础实操

> **日期**: Week 3 Day 3 | **主题**: 节点池概念与创建配置 | **版本**: K8s 1.28-1.33

---

## 1. 节点池核心概念

### 1.1 节点池架构

```
集群
├── 系统节点池 (system)
│   └── 运行 kube-system Pod（3 节点，高可用）
├── 通用计算节点池 (general-compute)
│   └── 运行无状态业务应用（按需扩缩）
├── GPU 计算节点池 (gpu-compute)
│   └── 运行 ML/AI 工作负载（预留实例）
└── 内存优化节点池 (memory-optimized)
    └── 运行大数据/缓存工作负载
```

### 1.2 节点池属性

| 属性 | 说明 | 示例 |
|------|------|------|
| 实例类型 | 节点规格 | c6.2xlarge (8C16G) |
| 标签 | 节点池标识 | node-pool=general |
| 污点 | 专用节点标记 | dedicated=compute:NoSchedule |
| 启动脚本 | 节点初始化 | 安装 NVIDIA 驱动 |
| 存储 | 节点系统盘 | 100GB SSD |

---

## 2. 创建节点池（kubeadm 方式）

### 2.1 准备节点加入配置

```bash
# 在 control plane 节点生成加入令牌
kubeadm token create --print-join-command
# 输出示例：
# kubeadm join 10.0.0.1:6443 --token xxx --discovery-token-ca-cert-hash sha256:xxx

# 获取节点池特定配置（如 GPU 标签）
cat > node-pool-label.yaml <<'EOF'
node.kubernetes.io/node-pool: gpu-compute
topology.kubernetes.io/zone: us-east-1a
EOF
```

### 2.2 新节点加入集群

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在新节点上执行（Ubuntu/Debian）
sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl

# 加入集群（标准节点）
sudo kubeadm join 10.0.0.1:6443 --token xxx \
  --discovery-token-ca-cert-hash sha256:xxx

# 加入集群（GPU 节点，需要预先打标签）
sudo kubeadm join 10.0.0.1:6443 --token xxx \
  --discovery-token-ca-cert-hash sha256:xxx \
  --node-label node-pool=gpu-compute \
  --node-taint nvidia.com/gpu=present:NoSchedule

# 验证节点加入
kubectl get nodes --show-labels | grep node-pool
```
### 2.3 云厂商节点池（EKS/GKE/ACK）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# AWS EKS - 创建节点池
eksctl create nodegroup \
  --cluster my-cluster \
  --name gpu-nodes \
  --instance-type p3.2xlarge \
  --nodes 2 \
  --nodes-min 1 \
  --nodes-max 4 \
  --Labels "node-pool=gpu,workload=ml" \
  --asg-access \
  --full-access

# GKE - 创建节点池
gcloud container clusters create my-cluster \
  --zone us-central1-a \
  --node-pool gpu-pool

# 查看节点池
kubectl get nodes -l node-pool=gpu
```
---

## 3. 节点池配置管理

### 3.1 为节点池添加标签

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 批量添加标签（所有 general-compute 节点）
for node in $(kubectl get nodes -l node-pool=general-compute --no-headers | awk '{print $1}'); do
  kubectl label node $node workload-type=application
done

# 添加污点（防止普通 Pod 调度到 GPU 节点）
kubectl taint node -l node-pool=gpu-compute dedicated=compute:NoSchedule
```
### 3.2 节点池资源配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点 allocatable
kubectl get nodes -l node-pool=gpu-compute \
  -o jsonpath='{.items[*].status.allocatable}'

# 节点池资源分布
kubectl get nodes -L node-pool | awk '{print $1, $5}' | sort | uniq -c

# 资源预留配置（kubelet）
# 编辑 /var/lib/kubelet/config.yaml
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
systemReserved:
  cpu: "200m"
  memory: "512Mi"
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
```
### 3.3 节点池生命周期

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看节点池状态
kubectl get nodes -L node-pool

# 扩缩节点池（云厂商）
# AWS: eksctl scale nodegroup --name=general --nodes=5
# GKE: gcloud container clusters resize my-cluster --node-pool=general --num-nodes=5

# 删除节点池节点
# 1. Cordon 节点
kubectl cordon <node-name>
# 2. Drain 节点
kubectl drain <node-name> --ignore-daemonsets
# 3. 从集群移除
kubectl delete node <node-name>
# 4. 关闭节点（云控制台或 CLI）
```
---

## 4. 节点池调度实践

### 4.1 工作负载分配到节点池

```yaml
# 通用应用调度到 general-compute
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-frontend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-frontend
  template:
    spec:
      nodeSelector:
        node-pool: general-compute
      tolerations:
        - key: "node-pool"
          operator: "Equal"
          value: "general-compute"
          effect: "NoExecute"
      containers:
        - name: web
          image: nginx:latest
          resources:
            requests:
              cpu: "250m"
              memory: "256Mi"
            limits:
              cpu: "500m"
              memory: "512Mi"
```

### 4.2 多节点池组合调度

```yaml
# Frontend 调度到 general，Backend 调度到 memory-optimized
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
spec:
  template:
    spec:
      nodeSelector:
        node-pool: general-compute
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
spec:
  template:
    spec:
      nodeSelector:
        node-pool: memory-optimized
```

### 4.3 节点池资源配额

```yaml
# 限制 namespace 使用特定节点池资源
apiVersion: v1
kind: ResourceQuota
metadata:
  name: gpu-quota
  namespace: ml-team
spec:
  hard:
    pods: "20"
    requests.nvidia.com/gpu: "4"  # 限制最多 4 GPU
```

---

## 5. 节点池故障排查

### 5.1 节点无法加入集群

```bash
# 1. 检查令牌是否过期（24 小时有效期）
kubeadm token list

# 2. 重新生成令牌
NEW_TOKEN=$(kubeadm token create --print-join-command)

# 3. 在新节点执行
sudo kubeadm join $NEW_TOKEN

# 4. 检查网络连通性
ssh <new-node-ip> curl -k https://10.0.0.1:6443/healthz

# 5. 检查 kubelet 日志
ssh <new-node-ip> sudo journalctl -u kubelet --since "5m" | tail -50
```

### 5.2 节点池标签丢失

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 恢复节点标签
kubectl label node <node-name> node-pool=general-compute --overwrite

# 批量恢复标签
for node in $(kubectl get nodes --no-headers | awk '{print $1}'); do
  # 根据实例类型自动打标签
  INSTANCE_TYPE=$(kubectl get node $node -o jsonpath='{.metadata.labels.node\.kubernetes\.io/instance-type}')
  case $INSTANCE_TYPE in
    p3*) kubectl label node $node node-pool=gpu-compute --overwrite ;;
    r5*) kubectl label node $node node-pool=memory-optimized --overwrite ;;
    *) kubectl label node $node node-pool=general-compute --overwrite ;;
  esac
done
```
### 5.3 节点池扩缩容失败

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Cluster Autoscaler 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100

# 检查扩缩容限制
# 1. AWS: 检查 ASG 限制
aws autoscaling describe-account-limits

# 2. 检查节点池最大节点数
# 云控制台查看节点池配置

# 3. 手动扩容验证
kubectl scale deployment <deploy> --replicas=50
kubectl get events --sort-by='.lastTimestamp' | tail -20
```
---

## 6. 节点池镜像管理

### 6.1 节点 OS 版本管理

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看节点 OS 版本
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.osImage}{"\n"}'

# 批量升级节点（Ubuntu）
for node in $(kubectl get nodes -l node-pool=general-compute --no-headers | awk '{print $1}'); do
  echo "升级节点: $node"
  kubectl cordon $node
  kubectl drain $node --ignore-daemonsets
  ssh $node "sudo apt-get update && sudo apt-get upgrade -y"
  ssh $node "sudo systemctl restart kubelet"
  kubectl uncordon $node
done
```
### 6.2 节点 kubelet 版本管理

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 检查 kubelet 版本
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}'

# 升级 kubelet（逐节点）
for node in $(kubectl get nodes -l node-pool=general-compute --no-headers | awk '{print $1}'); do
  ssh $node "sudo apt-get install -y kubelet=1.30.0-*" && \
  ssh $node "sudo systemctl restart kubelet"
done
```
---

## 7. 实战练习

**练习 1**: 创建一个 GPU 节点池，包含 2 个节点，打上 `node-pool=gpu` 标签和 `nvidia.com/gpu` 污点

**练习 2**: 将 ML 工作负载 Deployment 调度到 GPU 节点池，配置容忍污点

**练习 3**: 验证 Cluster Autoscaler 能够在 Pod Pending 时自动扩容节点池

**练习 4**: 编写脚本批量管理节点池（cordon/drain/uncordon）

---


```

<!-- risk-assessed -->
