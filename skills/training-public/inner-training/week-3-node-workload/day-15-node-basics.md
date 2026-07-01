---
title: 'Day 15: Node 节点基础'
description: '## 概述'
summary: '## 概述'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- calico
- coredns
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 15: Node 节点基础 是什么'
- '如何 Day 15: Node 节点基础'
trigger_keywords:
- Day
- '15:'
- Node
- 节点基础
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- cni-basics
- etcd-basics
---



---
title: Day 15: Node 节点基础
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] Node architecture [[kubelet|kubelet]] kube-proxy
  - Node status conditions Ready NotReady
  - Node capacity allocatable resource management
  - Kubernetes node monitoring troubleshooting
  - containerd CRI interface
trigger_keywords:
  - Node
  - kubelet
  - kube-proxy
  - containerd
  - Ready
  - NotReady
  - MemoryPressure
  - DiskPressure
  - capacity
  - allocatable
  - resource management
reading_level: intermediate
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-3-node
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - node-overview
  - node-management
  - node-notready-diagnosis
---

# Day 15: Node 节点基础

## 概述

Node（节点）是 Kubernetes 集群的工作引擎，是实际运行容器应用的地方。每个 Node 都是一台物理机或虚拟机，上面运行着三个核心组件：kubelet、kube-proxy 和容器运行时（containerd）。理解 Node 的架构、状态机制和管理操作是 K8s 运维的基础。

今天的学习从理解 Node 在 K8s 架构中的角色开始，逐步深入到节点状态监控、资源分析和核心进程管理。掌握这些内容后，你将能够快速判断节点是否健康、识别资源瓶颈、排查节点级别的问题。

### 学习目标

- 深入理解 Node 在 K8s 中的角色与核心组件（kubelet、kube-proxy、containerd）
- 掌握节点状态（Conditions）的含义及其对 Pod 调度的影响
- 能够通过 kubectl 和 debug 工具查看节点详细信息和资源使用情况
- 了解节点上运行的核心进程及其排障方法

---

## 核心概念详解

### Node 在 K8s 架构中的角色

K8s 采用 Master-Node 架构。Master 节点运行控制平面组件（API Server、etcd、Scheduler、Controller Manager），负责全局决策。Worker 节点（Node）负责运行用户的工作负载（Pod）。在 ACK 托管版中，Master 节点由阿里云管理，你只需要关注 Worker 节点。

Node 注册到集群的方式有两种：

- **自动注册**: kubelet 启动时使用 `--kubeconfig` 指定的引导配置向 API Server 注册自己。ACK 创建的节点会自动注册
- **手动管理**: 通过创建 Node 资源手动添加（较少使用）

Node 的核心属性包括：

- **Addresses**: 节点的网络地址（Hostname、InternalIP、ExternalIP）
- **Capacity 和 Allocatable**: 节点的资源总量和可分配量。Allocatable = Capacity - System Reserved - Kube Reserved - Eviction Threshold
- **Conditions**: 节点的健康状态
- **Info**: 节点的系统信息（操作系统、内核版本、容器运行时版本、kubelet 版本）

### kubelet 深入解析

kubelet 是 Node 上最重要的组件，它负责：

**Pod 生命周期管理**: kubelet 持续监听 API Server 上分配到本节点的 Pod 定义，并执行以下操作：

- 根据 Pod Spec 创建容器（调用容器运行时）
- 挂载 Volume（调用 CSI 驱动）
- 执行健康检查（liveness、readiness、startup 探针）
- 报告 Pod 状态（Phase、Conditions、Container Status）
- 在 Pod 被删除时执行优雅终止（发送 SIGTERM，等待 TerminationGracePeriodSeconds 后发送 SIGKILL）

**节点状态汇报**: kubelet 定期向 API Server 报告节点的 Conditions、Capacity/Allocatable 和节点信息。报告间隔默认为 10 秒（通过 `--node-status-update-frequency` 配置）。如果 API Server 在 `node-monitor-grace-period`（默认 40 秒）内未收到 kubelet 的报告，节点状态将被标记为 Unknown。

**资源预留**: kubelet 可以为系统进程和 K8s 组件预留资源，防止 Pod 耗尽节点资源：

| 预留类型 | 说明 | 推荐 CPU | 推荐内存 |
|---------|------|---------|---------|
| system-reserved | 操作系统进程 | 100m | 500Mi |
| kube-reserved | K8s 组件 | 200m | 1Gi |
| eviction-hard | 驱逐阈值 | N/A | 500Mi |

**Pod 驱逐（Eviction）**: 当节点资源紧张时（MemoryPressure、DiskPressure），kubelet 会按照优先级驱逐 Pod 以释放资源。驱逐优先级（从先到后）：BestEffort Pod（未设置资源限制）→ Burstable Pod（设置了部分资源限制）→ Guaranteed Pod（设置了 requests = limits）。kubelet 还会考虑 Pod 的 QoS 类别和优先级（PriorityClass）。

**kubelet 关键启动参数**:

| 参数 | 默认值 | 说明 |
|------|--------|------|
| --node-status-update-frequency | 10s | 向 API Server 汇报状态的间隔 |
| --max-pods | 110 | 节点允许的最大 Pod 数 |
| --pod-cidr | 节点 Pod CIDR | 分配给节点的 Pod IP 地址段 |
| --eviction-hard | memory.available<100Mi | 触发驱逐的硬阈值 |
| --eviction-soft | 无 | 触发驱逐的软阈值 |
| --eviction-soft-grace-period | 无 | 软阈值的宽限期 |
| --kube-reserved | 无 | 为 K8s 组件预留的资源 |
| --system-reserved | 无 | 为操作系统预留的资源 |

### kube-proxy 工作原理

kube-proxy 负责实现 Service 的网络转发规则。它监听 API Server 上 Service 和 Endpoints 的变化，并在节点上维护相应的转发规则。

**iptables 模式**（默认）: kube-proxy 在 iptables 的 PREROUTING 和 OUTPUT 链中添加规则，将目标为 Service ClusterIP 的流量 DNAT 到实际的 Pod IP。对于多后端的 Service，使用随机概率规则实现负载均衡。iptables 模式的优点是稳定、低延迟；缺点是在大规模集群中（数千条 Service）规则数量线性增长，影响匹配性能。

**IPVS 模式**: kube-proxy 使用 IPVS（IP Virtual Server）内核模块实现负载均衡。IPVS 使用哈希表查找后端，在大规模场景下性能优于 iptables。IPVS 还支持更多的负载均衡算法（rr、lc、wrr、wlc 等）。在 ACK 中，可以通过集群配置开启 IPVS 模式。

| 对比维度 | iptables 模式 | IPVS 模式 |
|---------|-------------|----------|
| 规则查找 | 线性遍历 | 哈希查找 |
| 大规模性能 | O(n)，数千条规则后下降 | O(1)，性能稳定 |
| 负载均衡算法 | 随机概率 | rr/lc/wrr/wlc/sh/dh 等 |
| 配置复杂度 | 低 | 中（需加载内核模块） |
| 适用场景 | 中小规模集群 | 大规模集群（100+ Service） |

### 节点 Conditions 详解

节点的健康状态通过 Conditions 来报告，每个 Condition 包含 Type（类型）、Status（True/False/Unknown）、Reason（原因）和 Message（详细信息）。

**Ready**: 最核心的 Condition。

- True: 节点健康，可以接收 Pod 调度
- False: kubelet 正在运行但节点不健康（如磁盘满、网络异常）
- Unknown: kubelet 未在规定时间内上报状态，可能是 kubelet 崩溃或节点失联

**MemoryPressure**:

- True: 节点可用内存低于阈值（默认 750Mi）。此时 kubelet 不再调度新的 Pod（除非设置了对应的 Toleration），并开始驱逐低优先级的 Pod
- 常见原因：内存泄漏、Pod 使用量超出预期、节点内存规格过小

**DiskPressure**:

- True: 节点磁盘空间或 inode 数量低于阈值。此时 kubelet 不再调度新的 Pod，并开始清理未使用的容器镜像和已停止的容器
- 常见原因：容器日志未清理、镜像占用过多空间、临时文件未清理

**PIDPressure**:

- True: 节点进程数量接近内核限制。较少见，通常发生在运行大量短生命周期进程的场景

**NetworkUnavailable**:

- True: 节点网络未正确配置。通常在 CNI 插件初始化完成后被设置为 False

| Condition | True 含义 | 对调度的影响 |
|-----------|----------|-------------|
| Ready | 节点正常 | False/Unknown 时不调度新 Pod |
| MemoryPressure | 内存不足 | 驱逐低优先级 Pod，不调度新 Pod |
| DiskPressure | 磁盘不足 | 清理镜像/容器，不调度新 Pod |
| PIDPressure | 进程数不足 | 不调度新 Pod |
| NetworkUnavailable | 网络未就绪 | 不调度新 Pod |

### 节点资源管理

节点的资源分为 Capacity（总量）和 Allocatable（可分配量）：

```
Allocatable = Capacity - System Reserved - Kube Reserved - Eviction Hard
```

示例计算（ecs.g6.xlarge, 4C16G）：

```
Capacity:       CPU=4, Memory=16384Mi
System Reserved: CPU=100m, Memory=500Mi
Kube Reserved:   CPU=200m, Memory=1Gi
Eviction Hard:   Memory=500Mi
─────────────────────────────────────
Allocatable:     CPU=3.7, Memory=13884Mi
```

---

## 实战演练

### 任务 1: 节点信息查看与分析 (45min)

```bash
# 查看节点基本信息
kubectl get nodes -o wide

# 示例输出:
# NAME            STATUS   ROLES    AGE   VERSION            INTERNAL-IP    OS-IMAGE                KERNEL-VERSION   CONTAINER-RUNTIME
# node-192-168-0-1   Ready    <none>   30d   v1.28.3-aliyun.1   192.168.0.1   AliyunLinux 3.2104   5.10.134-16     containerd://1.6.20
# node-192-168-0-2   Ready    <none>   30d   v1.28.3-aliyun.1   192.168.0.2   AliyunLinux 3.2104   5.10.134-16     containerd://1.6.20

# 查看节点详细信息
kubectl describe node <node-name>

# 自定义列查看关键信息
kubectl get nodes -o custom-columns='
NAME:.metadata.name,
STATUS:.status.conditions[?(@.type=="Ready")].status,
VERSION:.status.nodeInfo.kubeletVersion,
OS:.status.nodeInfo.osImage,
KERNEL:.status.nodeInfo.kernelVersion,
RUNTIME:.status.nodeInfo.containerRuntimeVersion,
CPU:.status.capacity.cpu,
MEMORY:.status.capacity.memory
'

# 查看节点资源使用率（需要 Metrics Server）
kubectl top nodes

# 示例输出:
# NAME            CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-192-168-0-1   850m        21%    4521Mi          27%
# node-192-168-0-2   1200m       30%    6789Mi          41%

# 比较节点的 Capacity 和 Allocatable
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): CPU=\(.status.capacity.cpu) Allocatable=\(.status.allocatable.cpu), Memory=\(.status.capacity.memory) Allocatable=\(.status.allocatable.memory)"'

# 示例输出:
# node-192-168-0-1: CPU=4 Allocatable=4, Memory=16384Mi Allocatable=13884Mi
# node-192-168-0-2: CPU=4 Allocatable=4, Memory=16384Mi Allocatable=13884Mi

# 查看节点标签
kubectl get nodes --show-labels
kubectl get nodes -l node-role=system
kubectl get nodes -l workload=frontend
```

### 任务 2: 节点 Conditions 监控 (45min)

```bash
# 查看所有节点的 Conditions
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{range .status.conditions[*]}  {.type}={.status} ({.reason}){"\n"}{end}{"\n"}{end}'

# 示例输出:
# node-192-168-0-1
#   Ready=True (KubeletReady)
#   MemoryPressure=False (KubeletHasSufficientMemory)
#   DiskPressure=False (KubeletHasNoDiskPressure)
#   PIDPressure=False (KubeletHasSufficientPID)
#   NetworkUnavailable=False (RouteCreated)

# 只关注异常状态
kubectl get nodes -o json | jq -r '.items[] | . as $node | .status.conditions[] | select(.status == "True" and .type != "Ready") | "\($node.metadata.name): \(.type)=\(.status) Reason=\(.reason) Message=\(.message)"'

# 检查 Ready=False 或 Unknown 的节点
kubectl get nodes -o json | jq -r '.items[] | select(.status.conditions[] | .type == "Ready" and .status != "True") | "\(.metadata.name): Not Ready"'

# 持续监控节点状态变化
kubectl get nodes -w

# 检查节点最近的事件
kubectl get events --field-selector involvedObject.kind=Node --sort-by='.lastTimestamp'

# 查看节点系统信息
kubectl get nodes -o json | jq -r '.items[] | {
  name: .metadata.name,
  os: .status.nodeInfo.osImage,
  kernel: .status.nodeInfo.kernelVersion,
  kubelet: .status.nodeInfo.kubeletVersion,
  runtime: .status.nodeInfo.containerRuntimeVersion,
  arch: .status.nodeInfo.architecture
}'
```

### 任务 3: 节点资源分配分析 (30min)

```bash
# 查看节点资源分配详情
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 示例输出:
# Allocated resources:
#   (Total limits may be over 100 percent, i.e., overcommitted.)
#   Resource           Requests     Limits
#   --------           --------     ------
#   cpu                800m (20%)   1600m (40%)
#   memory             2Gi (14%)    4Gi (28%)
#   ephemeral-storage  0 (0%)       0 (0%)
#   hugepages-1Gi      0 (0%)       0 (0%)
#   hugepages-2Mi      0 (0%)       0 (0%)

# 查看节点上运行的所有 Pod
kubectl get pods -A --field-selector spec.nodeName=<node-name> -o wide

# 示例输出:
# NAMESPACE     NAME                                       READY   STATUS    RESTARTS   AGE   IP             NODE
# kube-system   calico-node-xxxxx                          1/1     Running   0          30d   192.168.0.1    node-192-168-0-1
# kube-system   kube-proxy-xxxxx                           1/1     Running   0          30d   192.168.0.1    node-192-168-0-1
# kube-system   csi-plugin-xxxxx                           1/1     Running   0          30d   192.168.0.1    node-192-168-0-1
# kube-system   csi-provisioner-xxxxx                      1/1     Running   0          30d   192.168.0.1    node-192-168-0-1
# kube-system   logtail-ds-xxxxx                           1/1     Running   0          30d   192.168.0.1    node-192-168-0-1
# default       frontend-app-7d9f8c6b5-xk2lm               1/1     Running   0          5d    10.244.1.15    node-192-168-0-1
# default       frontend-app-7d9f8c6b5-pq8rs               1/1     Running   0          5d    10.244.1.16    node-192-168-0-1

# 统计每个节点的 Pod 数量和资源使用
kubectl get pods -A -o json | jq -r '
  .items | group_by(.spec.nodeName) | 
  .[] | {
    node: .[0].spec.nodeName,
    pods: length,
    cpu_requests: (map(.spec.containers[].resources.requests.cpu // "0m") | join("+") | . + " total"),
    memory_requests: (map(.spec.containers[].resources.requests.memory // "0Mi") | join("+") | . + " total")
  }
'

# 查看占用资源最多的 Pod
kubectl top pods -A --sort-by=memory | head -20
kubectl top pods -A --sort-by=cpu | head -20

# 示例输出:
# NAMESPACE     NAME                        CPU(cores)   MEMORY(bytes)
# monitoring    prometheus-k8s-0            320m         3842Mi
# monitoring    grafana-7d8f9c6b5-xk2lm     45m          512Mi
# kube-system   coredns-7d8f9c6b5-xk2lm     12m          128Mi
# default       frontend-app-7d9f8c6b5-xk   85m          256Mi
```

### 任务 4: 节点核心进程检查 (30min)

```bash
# 使用 kubectl debug 进入节点（K8s 1.18+）
kubectl debug node/<node-name> -it --image=busybox

# 在 debug 容器中检查 kubelet
# chroot /host
# systemctl status kubelet
# journalctl -u kubelet --no-pager -n 50
# cat /var/log/kubelet.log

# 检查 kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=20

# 检查容器运行时
# crictl ps
# crictl pods
# crictl info

# 检查节点网络配置
# ip addr
# ip route
# iptables -L -n -t nat | head -50

# 检查 kubelet 版本和配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig'
```

### 任务 5: 节点标签和污点管理 (30min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 查看节点标签
kubectl get nodes --show-labels

# 添加标签
kubectl label nodes <node-name> environment=production
kubectl label nodes <node-name> workload=frontend tier=web

# 删除标签
kubectl label nodes <node-name> environment-

# 通过标签筛选节点
kubectl get nodes -l workload=frontend
kubectl get nodes -l environment=production,tier=web

# 添加污点
kubectl taint nodes <node-name> dedicated=frontend:NoSchedule
kubectl taint nodes <node-name> special=true:NoExecute

# 查看污点
kubectl describe node <node-name> | grep Taints

# 删除污点
kubectl taint nodes <node-name> dedicated=frontend:NoSchedule-
kubectl taint nodes <node-name> special=true:NoExecute-

# 使用标签调度 Pod
cat > labeled-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: frontend
  template:
    metadata:
      labels:
        app: frontend
    spec:
      nodeSelector:
        workload: frontend
      tolerations:
      - key: dedicated
        value: frontend
        effect: NoSchedule
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
EOF

kubectl apply -f labeled-deployment.yaml
kubectl get pods -l app=frontend -o wide
```

---

## 配置参考

### kubelet 资源预留配置

```yaml
# kubelet 配置示例（/etc/kubernetes/kubelet-config.yml）
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
address: 0.0.0.0
port: 10250
serializeImagePulls: false
maxPods: 110
podCIDR: "10.244.0.0/24"
clusterDNS:
  - "10.96.0.10"
clusterDomain: "cluster.local"
systemReserved:
  cpu: "100m"
  memory: "500Mi"
  ephemeral-storage: "1Gi"
kubeReserved:
  cpu: "200m"
  memory: "1Gi"
  ephemeral-storage: "2Gi"
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "2m"
  nodefs.available: "5m"
evictionMaxPodGracePeriod: 120
evictionPressureTransitionPeriod: "5m"
```

### 节点状态检查脚本

```bash
#!/bin/bash
# node-health-check.sh - 节点健康状态检查脚本

echo "========================================"
echo "  K8s Node Health Check Report"
echo "  Date: $(date)"
echo "========================================"
echo ""

echo "=== 1. Node Overview ==="
kubectl get nodes -o wide
echo ""

echo "=== 2. NotReady Nodes ==="
NOT_READY=$(kubectl get nodes -o json | jq -r '.items[] | select(.status.conditions[] | .type == "Ready" and .status != "True") | .metadata.name')
if [ -z "$NOT_READY" ]; then
  echo "All nodes are Ready."
else
  echo "WARNING: The following nodes are NotReady:"
  echo "$NOT_READY"
fi
echo ""

echo "=== 3. Resource Usage ==="
kubectl top nodes 2>/dev/null || echo "Metrics Server not available"
echo ""

echo "=== 4. Resource Pressure ==="
kubectl get nodes -o json | jq -r '.items[] | . as $node | .status.conditions[] | select(.status == "True" and .type != "Ready") | "\($node.metadata.name): \(.type) - \(.message)"'
echo ""

echo "=== 5. High CPU/Memory Pods ==="
kubectl top pods -A --sort-by=memory --no-headers 2>/dev/null | head -10 || echo "Metrics Server not available"
echo ""

echo "=== 6. Pods per Node ==="
kubectl get pods -A --field-selector=status.phase=Running -o json | jq -r '.items | group_by(.spec.nodeName) | .[] | "\(.[0].spec.nodeName // "unknown"): \(length) pods"'
echo ""

echo "========================================"
echo "  Health Check Complete"
echo "========================================"
```

---

## 常见问题

### Q1: 节点状态变成 Unknown 怎么办？

节点 Unknown 意味着 API Server 无法联系到 kubelet。排查步骤：1) 检查节点是否宕机（通过云控制台查看 ECS 实例状态）；2) 如果节点在运行，SSH 登录检查 kubelet 进程是否正常（`systemctl status kubelet`）；3) 检查节点与 Master 的网络连通性；4) 检查 kubelet 日志是否有错误信息。

### Q2: 节点 MemoryPressure=True 如何处理？

处理步骤：1) `kubectl top pods -A --field-selector spec.nodeName=<node>` 找出占用内存最多的 Pod；2) 检查该 Pod 是否有内存泄漏（对比历史趋势）；3) 如果是正常使用，考虑增加节点的内存规格或迁移部分 Pod 到其他节点；4) 检查 Pod 的内存 Limits 设置是否合理。

### Q3: Allocatable 远小于 Capacity 是正常的吗？

是的。正常的差距来自 System Reserved、Kube Reserved 和 Eviction Hard 预留。通常 Allocatable 约为 Capacity 的 85-90%。如果差距过大，可以检查 kubelet 的资源配置参数。

### Q4: 如何判断节点是否需要扩容？

判断依据：1) 节点的 CPU/Memory Requests 使用率持续超过 70%；2) Pod 因为资源不足无法调度（Pending）；3) 频繁触发 MemoryPressure 或 DiskPressure。建议在资源使用率达到 70% 时就开始规划扩容，避免等到 90% 以上才紧急扩容。

### Q5: iptables 模式和 IPVS 模式如何选择？

如果你的集群 Service 数量在 1000 以下，iptables 模式完全可以满足需求。如果 Service 数量超过 1000，或者你需要在 kube-proxy 层使用更丰富的负载均衡算法（如加权轮询、最少连接），建议切换到 IPVS 模式。ACK 集群创建时可以选择 kube-proxy 模式，创建后也可以修改。

---

## 要点总结

| 组件/概念 | 作用 | 关键要点 |
|-----------|------|---------|
| kubelet | Pod 生命周期管理 | 创建/销毁容器、执行探针、汇报状态 |
| kube-proxy | Service 网络转发 | iptables/IPVS 模式实现负载均衡 |
| containerd | 容器运行时 | 通过 CRI 接口与 kubelet 交互 |
| Conditions | 节点健康状态 | Ready/MemoryPressure/DiskPressure/PIDPressure |
| 资源管理 | Capacity vs Allocatable | 预留资源确保系统稳定性 |
| 标签/污点 | 调度控制 | nodeSelector/affinity 驱动调度，Taint 排斥 |

---

## 延伸阅读

- [K8s 核心组件深入](../../domain-01-cluster-fundamentals/02-core-components-deep-dive.md)
- [Node NotReady 诊断](../../domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md)
- [Pod 综合排障](../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md)
- [OOM 内存诊断](../../domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md)

```