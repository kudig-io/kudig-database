---
title: MPI Operator 与分布式训练故障排查指南 [topic-structural-trouble-shooting]
description: 'title: MPI Operator 与分布式训练故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- scheduler
- prometheus
- coredns
- docker
- opa
- daemonset
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- MPI Operator 与分布式训练故障排查指南 是什么
- 如何 MPI Operator 与分布式训练故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- MPI Operator 与分布式训练故障排查指南 故障排查
- MPI Operator 与分布式训练故障排查指南 排障步骤
trigger_keywords:
- MPI
- Operator
- 与分布式训练故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- pod-lifecycle
- troubleshooting-methodology
- prometheus-basics
- gpu-scheduling-basics
- policy-basics
created: "2026-05-23"
---

title: MPI Operator 与分布式训练故障排查指南
description: '# MPI Operator 与分布式训练故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- scheduler
- [[Prometheus|prometheus]]
- [[CoreDNS|coredns]]
- opa
- daemonset
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- MPI Operator 与分布式训练故障排查指南 是什么
- 如何 MPI Operator 与分布式训练故障排查指南
- MPI Operator 与分布式训练故障排查指南 故障排查
- MPI Operator 与分布式训练故障排查指南 排障步骤
trigger_keywords:
- MPI
- Operator
- 与分布式训练故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# MPI Operator 与分布式训练故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | MPI Operator v0.5+ | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **MPIJob 状态**：`kubectl get mpijobs -A`，确认 `Launcher` 和 `Worker` Pod 状态。
2. **Launcher 日志**：`kubectl logs -l mpi-job-name=<job-name> -l mpi-role=launcher --tail=100`，查看 `mpirun` 输出。
3. **Worker  readiness**：`kubectl get pods -l mpi-job-name=<job-name> -l mpi-role=worker`，确认所有 Worker 为 Running。
4. **SSH-less 连通性**：进入 Launcher Pod，执行 `mpirun --hostfile /etc/mpi/hostfile hostname`，验证 Worker 可达。
5. **GPU/网络可见性**：检查 Worker Pod 内 `nvidia-smi` 和 `ibstat`（如使用 InfiniBand）。
6. **快速缓解**：
   - Worker 启动缓慢：检查镜像拉取和节点资源。
   - 通信失败：确认 NetworkPolicy 允许 Worker 之间通信。
   - 进程数不匹配：检查 `slots` 配置与节点 GPU/CPU 数是否一致。
7. **证据留存**：保存 Launcher/Worker 日志、hostfile、MPIJob YAML、节点资源状态。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 MPIJob 启动失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Launcher Pod 无法启动 | `mpirun could not find executable` | Launcher Pod | `kubectl logs launcher-pod` |
| Worker Pod 未创建 | `failed to create worker pods` | MPI Operator | Operator 日志 |
| 镜像拉取失败 | `ImagePullBackOff` | kubelet | `kubectl get pods` |
| 资源不足 | `0/X nodes are available` | Scheduler | `kubectl describe mpijob` |
| RBAC 权限不足 | `serviceaccount cannot create pods` | MPI Operator | Operator 日志 |

#### 1.1.2 分布式通信失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| MPI 初始化失败 | `MPI_Init failed` | Launcher stdout | Launcher 日志 |
| 进程连接超时 | `ORTE was unable to reliably initiate connection` | OpenMPI | Launcher 日志 |
| SSH 认证失败 | `Permission denied (publickey)` | ssh | Launcher 日志 |
| Host 不可达 | `failed to find hostname in hostfile` | mpirun | Launcher 日志 |
| 端口冲突 | `bind failed: Address already in use` | OpenMPI | Worker 日志 |

#### 1.1.3 NCCL/Horovod 训练问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| NCCL 初始化错误 | `NCCL error: unhandled system error` | PyTorch/TensorFlow | 训练日志 |
| 梯度同步失败 | `Horovod: ncclAllReduce failed` | Horovod | 训练日志 |
| 进程 rank 不一致 | `Rank mismatch` | NCCL | 训练日志 |
| 网络超时 | `NCCL timeout` | NCCL | 训练日志 |
| 显存不足 | `CUDA out of memory` | PyTorch | 训练日志 |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **大规模训练启动缓慢** | 256 Worker 的 MPIJob 启动耗时 30 分钟+ | 顺序拉取镜像，节点磁盘 I/O 饱和 | 使用 imagePullSecrets + 镜像预热 DaemonSet |
| **跨节点 NCCL 超时** | 多节点训练随机出现 NCCL timeout | 网络策略阻断或 MTU 不匹配 | 放通 Worker 通信端口，统一 MTU |
| **GPU 拓扑感知调度失败** | Worker 分散在不同交换机，all-reduce 性能差 | 调度器未考虑 GPU 拓扑 | 配置 Pod 亲和性，优先同节点/同交换机 |
| **Spot 实例中断导致训练失败** | Spot 节点被回收，整个 MPIJob 失败 | MPI 不支持动态成员变更 | 使用 Checkpoint + Elastic Training |

### 1.2 报错查看方式汇总

```bash
# MPIJob 状态
kubectl get mpijobs -A -o wide

# Launcher 日志
kubectl logs -l mpi-job-name=<job-name> -l mpi-role=launcher --tail=200

# Worker 日志
kubectl logs -l mpi-job-name=<job-name> -l mpi-role=worker --tail=100

# MPI Operator 日志
kubectl logs -n mpi-operator deployment/mpi-operator --tail=200

# Worker Pod 详情
kubectl describe pod <worker-pod-name>

# 节点 GPU 状态
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.nvidia\.com/gpu}{"\n"}{end}'

# 进入 Launcher 诊断
kubectl exec -it <mpijob-launcher-pod> -- /bin/bash
mpirun --version
mpirun --hostfile /etc/mpi/hostfile -np <N> hostname
```

---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

MPI Operator 基于 Kubernetes CRD 实现 MPI 作业调度，其核心架构：

```
用户提交 MPIJob
        │
        ▼
┌─────────────────────┐
│   MPI Operator       │ ──► 监听 MPIJob，创建 Launcher + Worker Pods
│   (mpi-operator)     │
└──────────┬──────────┘
           │
    ┌──────┴──────┐
    ▼             ▼
┌─────────┐   ┌─────────┐
│ Launcher │   │ Worker  │
│ Pod      │   │ Pods    │
│          │   │         │
│ mpirun   │──►│ 训练进程 │
│ ssh client│   │ sshd    │
└─────────┘   └─────────┘
```

**关键机制**：
- **Hostfile 生成**：MPI Operator 自动为 Launcher 生成 `/etc/mpi/hostfile`，包含所有 Worker 的 DNS 名称和 slots 数
- **SSH-less 通信**：通过 `mpiexec` + `ssh` 或 `pmi` 实现 Launcher 到 Worker 的进程启动
- **进程映射**：`slots` 决定每个 Worker 上启动的进程数，通常等于 GPU 数或 CPU 核心数

### 2.2 排查逻辑决策树

```
MPI 训练问题
    ├── MPIJob 无法启动
    │   ├── Launcher/Worker Pod 未创建？
    │   │   ├── MPI Operator 未运行？──► 重启/部署 operator
    │   │   ├── RBAC 权限不足？──► 为 serviceAccount 添加 pod create 权限
    │   │   └── 资源配额超限？──► 检查 namespace resourceQuota
    │   ├── Pod 处于 Pending？
    │   │   ├── 节点资源不足？──► 添加节点或降低 requests
    │   │   ├── 镜像拉取失败？──► 检查 imagePullSecrets
    │   │   └── 调度约束不满足？──► 检查 nodeSelector/affinity/tolerations
    │   └── Launcher 启动后立即失败？
    │       ├── 训练脚本不存在？──► 检查 workingDir 和 command
    │       └── 依赖缺失？──► 检查镜像中是否包含 mpi/mpich/openmpi
    ├── 通信初始化失败
    │   ├── SSH 连接失败？
    │   │   ├── hostfile 不正确？──► 检查 /etc/mpi/hostfile
    │   │   └── SSH key 权限错误？──► 检查 secret 挂载权限 (0600)
    │   ├── Worker 进程未启动？
    │   │   ├── Worker 容器未就绪？──► 检查 Worker Pod 状态
    │   │   └── 端口被占用？──► 检查是否有残留进程
    │   └── 网络不可达？
    │       ├── DNS 解析失败？──► 检查 CoreDNS
    │       ├── NetworkPolicy 阻断？──► 放通 Worker 间通信
    │       └── CNI 插件异常？──► 检查 CNI 状态
    └── 训练过程中失败
        ├── NCCL/Horovod 报错
        │   ├── GPU 不可见？──► 检查 Device Plugin 和 nvidia-smi
        │   ├── 网络超时？──► 检查网络带宽/延迟，调大 NCCL timeout
        │   └── 进程数与 GPU 不匹配？──► 检查 slots 和进程映射
        ├── OOM/CUDA 错误
        │   ├── 显存不足？──► 降低 batch size 或模型大小
        │   └── 主机内存不足？──► 增加 Pod memory limits
        └── Worker 被驱逐
            ├── 节点压力？──► 检查节点资源压力
            └── Spot 实例中断？──► 使用 Checkpoint + Elastic Training
```

### 2.3 详细诊断命令

#### MPIJob 全景诊断

```bash
#!/bin/bash
# MPIJob 全景诊断脚本
# 用法: ./diagnose-mpi.sh <mpijob-name> <namespace>

MPIJOB_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -z "$MPIJOB_NAME" ]; then
  echo "用法: $0 <mpijob-name> [namespace]"
  exit 1
fi

echo "=== MPIJob $NAMESPACE/$MPIJOB_NAME 全景诊断 ==="

# 1. MPIJob 总体状态
echo "1. MPIJob 状态:"
kubectl get mpijob $MPIJOB_NAME -n $NAMESPACE -o json | jq -r '
  {
    phase: .status.conditions[-1].type,
    status: .status.conditions[-1].status,
    reason: .status.conditions[-1].reason,
    message: .status.conditions[-1].message,
    launcherStatus: .status.replicaStatuses.Launcher,
    workerStatus: .status.replicaStatuses.Worker
  }'

# 2. Launcher Pod 状态
echo ""
echo "2. Launcher Pod:"
kubectl get pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=launcher -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), hostIP=\(.status.hostIP)"
'

# 3. Worker Pods 状态
echo ""
echo "3. Worker Pods:"
kubectl get pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=worker -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), node=\(.spec.nodeName), hostIP=\(.status.hostIP)"
'

# 4. Worker 节点 GPU 状态
echo ""
echo "4. Worker 节点 GPU:"
for node in $(kubectl get pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=worker -o jsonpath='{.items[*].spec.nodeName}' | tr ' ' '\n' | sort -u); do
  GPU_COUNT=$(kubectl get node $node -o jsonpath='{.status.capacity.nvidia\.com/gpu}' 2>/dev/null || echo "0")
  echo "  $node: $GPU_COUNT GPUs"
done

# 5. Launcher 日志摘要
echo ""
echo "5. Launcher 日志摘要:"
LAUNCHER_POD=$(kubectl get pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=launcher -o jsonpath='{.items[0].metadata.name}')
if [ -n "$LAUNCHER_POD" ]; then
  kubectl logs -n $NAMESPACE $LAUNCHER_POD --tail=100 2>/dev/null | grep -iE "error|fail|timeout|unable|cannot" | tail -15
else
  echo "  ✗ 未找到 Launcher Pod"
fi

# 6. 检查 hostfile
echo ""
echo "6. Hostfile 内容:"
if [ -n "$LAUNCHER_POD" ]; then
  kubectl exec -n $NAMESPACE $LAUNCHER_POD -- cat /etc/mpi/hostfile 2>/dev/null || echo "  无法读取 hostfile"
else
  echo "  Launcher Pod 不存在"
fi
```

#### 分布式通信诊断

```bash
#!/bin/bash
# 分布式通信诊断脚本
# 在 Launcher Pod 内运行

echo "=== 分布式通信诊断 ==="

# 1. 基础连通性
echo "1. Worker 主机名解析:"
if [ -f /etc/mpi/hostfile ]; then
  while read host slots; do
    echo -n "  $host: "
    getent hosts $host >/dev/null 2>&1 && echo "DNS 解析成功" || echo "DNS 解析失败"
  done < /etc/mpi/hostfile
else
  echo "  ✗ hostfile 不存在"
fi

# 2. MPI 基础测试
echo ""
echo "2. MPI 基础通信测试:"
mpirun --allow-run-as-root --hostfile /etc/mpi/hostfile -np 4 hostname 2>&1 | sort

# 3. MPI 带宽测试（如安装 OSU Micro-Benchmarks）
echo ""
echo "3. MPI 带宽测试:"
if command -v osu_bw &>/dev/null; then
  mpirun --allow-run-as-root --hostfile /etc/mpi/hostfile -np 2 osu_bw 2>&1 | tail -5
else
  echo "  osu_bw 未安装，跳过带宽测试"
fi

# 4. NCCL 测试（如使用 NCCL）
echo ""
echo "4. NCCL 测试:"
if command -v all_reduce_perf &>/dev/null; then
  mpirun --allow-run-as-root --hostfile /etc/mpi/hostfile -np $(cat /etc/mpi/hostfile | awk '{s+=$2} END {print s}') \
    all_reduce_perf -b 8 -e 128M -f 2 -g 1 2>&1 | tail -10
else
  echo "  all_reduce_perf 未安装，跳过 NCCL 测试"
fi

# 5. 检查环境变量
echo ""
echo "5. 关键环境变量:"
env | grep -E "^(OMPI|NCCL|MPI|CUDA|HOROVOD)" | sort

# 6. 检查网络接口
echo ""
echo "6. 网络接口:"
ip addr show | grep -E "^\d+:|inet " | head -20
```

---

## 3. 解决方案与风险控制

### 3.1 MPIJob 配置优化

#### 方案一：高性能 MPIJob 配置

```yaml
# 高性能多节点 GPU 训练 MPIJob
apiVersion: kubeflow.org/v2beta1
kind: MPIJob
metadata:
  name: gpu-training-job
  namespace: default
spec:
  slotsPerWorker: 8           # 每个 Worker 的 slots 数，通常等于 GPU 数
  runPolicy:
    cleanUpPolicy: RunningOnly  # 只清理 Running 状态，保留 Completed/Failed 便于调试
    ttlSecondsAfterFinished: 86400  # 24 小时后自动清理
  mpiReplicaSpecs:
    Launcher:
      replicas: 1
      template:
        spec:
          containers:
          - name: mpi-launcher
            image: my-registry/mpi-training:v1.0
            command:
            - mpirun
            args:
            - "--allow-run-as-root"
            - "-np"
            - "32"                    # 总进程数 = slotsPerWorker * worker replicas
            - "--bind-to"
            - "none"
            - "-x"
            - "NCCL_DEBUG=INFO"
            - "-x"
            - "NCCL_SOCKET_IFNAME=eth0"
            - "-x"
            - "NCCL_IB_DISABLE=0"
            - "-x"
            - "CUDA_VISIBLE_DEVICES=0,1,2,3,4,5,6,7"
            - "python"
            - "train.py"
            - "--epochs=100"
            resources:
              limits:
                cpu: "4"
                memory: "8Gi"
              requests:
                cpu: "2"
                memory: "4Gi"
    Worker:
      replicas: 4                  # 4 个 Worker 节点
      template:
        spec:
          containers:
          - name: mpi-worker
            image: my-registry/mpi-training:v1.0
            resources:
              limits:
                nvidia.com/gpu: "8"  # 每个 Worker 8 张 GPU
                cpu: "64"
                memory: "512Gi"
              requests:
                nvidia.com/gpu: "8"
                cpu: "64"
                memory: "512Gi"
            volumeMounts:
            - name: shared-data
              mountPath: /data
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: shared-data
            persistentVolumeClaim:
              claimName: training-data-pvc
          - name: shm
            emptyDir:
              medium: Memory          # 共享内存使用 tmpfs，加速 NCCL
              sizeLimit: 64Gi
          affinity:
            podAntiAffinity:
              preferredDuringSchedulingIgnoredDuringExecution:
              - weight: 100
                podAffinityTerm:
                  labelSelector:
                    matchExpressions:
                    - key: mpi-job-name
                      operator: In
                      values:
                      - gpu-training-job
                  topologyKey: kubernetes.io/hostname
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
```

#### 方案二：SSH-less 认证配置

```yaml
# MPIJob SSH Secret 配置
apiVersion: v1
kind: Secret
metadata:
  name: mpi-ssh-keys
  namespace: default
type: Opaque
data:
  # 生成方法:
  # ssh-keygen -t rsa -b 4096 -f /tmp/mpi_rsa -N ""
  # kubectl create secret generic mpi-ssh-keys --from-file=id_rsa=/tmp/mpi_rsa --from-file=id_rsa.pub=/tmp/mpi_rsa.pub --from-file=authorized_keys=/tmp/mpi_rsa.pub
  id_rsa: <base64-private-key>
  id_rsa.pub: <base64-public-key>
  authorized_keys: <base64-public-key>
---
# MPIJob 引用 SSH Secret
apiVersion: kubeflow.org/v2beta1
kind: MPIJob
metadata:
  name: ssh-mpi-job
  namespace: default
spec:
  sshAuthMountPath: /root/.ssh  # SSH key 挂载路径
  mpiReplicaSpecs:
    Launcher:
      replicas: 1
      template:
        spec:
          containers:
          - name: mpi-launcher
            image: mpioperator/mpi-pi:latest
            volumeMounts:
            - name: ssh-auth
              mountPath: /root/.ssh
          volumes:
          - name: ssh-auth
            secret:
              secretName: mpi-ssh-keys
              defaultMode: 0600  # 关键：必须设置为 0600，否则 SSH 会拒绝使用
    Worker:
      replicas: 2
      template:
        spec:
          containers:
          - name: mpi-worker
            image: mpioperator/mpi-pi:latest
            volumeMounts:
            - name: ssh-auth
              mountPath: /root/.ssh
          volumes:
          - name: ssh-auth
            secret:
              secretName: mpi-ssh-keys
              defaultMode: 0600
```

### 3.2 NCCL 网络优化

```yaml
# NCCL 环境变量优化（在 MPIJob 的 env 中配置）
env:
# NCCL 调试
- name: NCCL_DEBUG
  value: "INFO"                   # 可选: WARN, INFO, TRACE
- name: NCCL_DEBUG_SUBSYS
  value: "ALL"

# 网络接口选择
- name: NCCL_SOCKET_IFNAME
  value: "eth0"                   # 指定通信网卡
- name: NCCL_IB_DISABLE
  value: "0"                      # 0=启用 InfiniBand, 1=禁用
- name: NCCL_IB_CUDA_SUPPORT
  value: "1"                      # 启用 GPUDirect RDMA
- name: NCCL_NET_GDR_LEVEL
  value: "5"                      # GPU Direct RDMA 级别

# 性能调优
- name: NCCL_BUFFSIZE
  value: "8388608"                # 8MB 通信缓冲区
- name: NCCL_NSOCKS_PERTHREAD
  value: "4"
- name: NCCL_SOCKET_NTHREADS
  value: "2"
- name: NCCL_TREE_THRESHOLD
  value: "0"                      # 始终使用 tree 算法

# 超时配置
- name: NCCL_TIMEOUT
  value: "1800"                   # 30 分钟超时
```

### 3.3 弹性训练与容错

```yaml
# 弹性训练 MPIJob（结合 Elastic Horovod）
apiVersion: kubeflow.org/v2beta1
kind: MPIJob
metadata:
  name: elastic-training
  namespace: default
spec:
  runPolicy:
    cleanUpPolicy: RunningOnly
  mpiReplicaSpecs:
    Launcher:
      replicas: 1
      template:
        spec:
          containers:
          - name: mpi-launcher
            image: horovod/horovod:latest
            command:
            - horovodrun
            args:
            - "-np"
            - "4-8"                    # 弹性范围：最少 4 进程，最多 8 进程
            - "--min-np"
            - "4"
            - "--max-np"
            - "8"
            - "--host-discovery-script"
            - "/etc/mpi/discover_hosts.sh"
            - "python"
            - "train_elastic.py"
            env:
            - name: HOROVOD_ELASTIC
              value: "1"
            - name: HOROVOD_TIMEOUT
              value: "300"
    Worker:
      replicas: 4
      template:
        spec:
          containers:
          - name: mpi-worker
            image: horovod/horovod:latest
            resources:
              limits:
                nvidia.com/gpu: "2"
                memory: "64Gi"
              requests:
                nvidia.com/gpu: "2"
                memory: "32Gi"
```

### 3.4 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 调整 slotsPerWorker | ⭐⭐ 中 | 进程分布改变，可能影响性能 | 恢复原始值并重新提交 |
| 修改 NCCL 环境变量 | ⭐ 低 | 影响通信性能，不影响正确性 | 恢复原始环境变量 |
| 启用 InfiniBand | ⭐⭐ 中 | 驱动兼容性问题可能导致启动失败 | 设置 NCCL_IB_DISABLE=1 |
| 弹性训练模式 | ⭐⭐ 中 | Worker 动态增减可能影响收敛 | 切换为固定 Worker 数 |
| 更换 MPI 实现 | ⭐⭐⭐ 高 | OpenMPI/MPICH/Intel MPI 行为差异 | 恢复原始镜像 |
| 节点拓扑变更 | ⭐⭐ 中 | 跨交换机通信延迟增加 | 添加 Pod 拓扑亲和约束 |

### 3.5 验证与监控

#### MPI 训练健康检查

```bash
#!/bin/bash
# MPI 训练健康检查脚本

MPIJOB_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -z "$MPIJOB_NAME" ]; then
  echo "用法: $0 <mpijob-name> [namespace]"
  exit 1
fi

echo "=== MPIJob $NAMESPACE/$MPIJOB_NAME 健康检查 ==="

# 1. Pod 状态检查
echo "1. Pod 状态:"
for role in launcher worker; do
  PODS=$(kubectl get pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=$role -o jsonpath='{.items[*].metadata.name}')
  for pod in $PODS; do
    PHASE=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.phase}')
    RESTARTS=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].restartCount}')
    echo "  $pod: phase=$PHASE, restarts=$RESTARTS"
  done
done

# 2. 资源使用
echo ""
echo "2. 资源使用:"
kubectl top pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME 2>/dev/null || echo "  metrics-server 不可用"

# 3. 训练进度（假设日志输出 epoch/loss）
echo ""
echo "3. 训练进度:"
LAUNCHER=$(kubectl get pods -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=launcher -o jsonpath='{.items[0].metadata.name}')
if [ -n "$LAUNCHER" ]; then
  kubectl logs -n $NAMESPACE $LAUNCHER --tail=20 2>/dev/null | grep -E "epoch|loss|accuracy|step" | tail -5
fi

# 4. NCCL 健康（检查是否有 NCCL 错误）
echo ""
echo "4. NCCL 状态:"
kubectl logs -n $NAMESPACE -l mpi-job-name=$MPIJOB_NAME,mpi-role=worker --tail=50 2>/dev/null | \
  grep -i "nccl" | grep -iE "error|fail|warn" | tail -5 || echo "  未发现 NCCL 错误"
```

#### Prometheus 监控告警

```yaml
# MPI 训练监控告警
groups:
- name: mpi-training
  rules:
  - alert: MPIJobFailed
    expr: |
      mpijob_status{phase="Failed"} == 1
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "MPIJob 失败"
      description: "MPIJob {{ $labels.mpijob_name }} 在 {{ $labels.namespace }} 中失败"

  - alert: MPIJobWorkerRestart
    expr: |
      rate(kube_pod_container_status_restarts_total{namespace=~".*", pod=~".*worker.*"}[10m]) > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "MPI Worker 容器重启"
      description: "MPI Worker Pod {{ $labels.pod }} 在过去 10 分钟内发生重启"

  - alert: MPILauncherStuck
    expr: |
      mpijob_status{phase="Running"} == 1
      and time() - mpijob_start_time > 3600 * 24
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "MPIJob 运行时间过长"
      description: "MPIJob {{ $labels.mpijob_name }} 已运行超过 24 小时，可能卡住"

  - alert: NCCLCommunicationError
    expr: |
      increase(nccl_errors_total[5m]) > 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "NCCL 通信错误"
      description: "MPIJob 中出现 NCCL 通信错误"
```

### 3.6 最佳实践

1. **镜像预热**：在训练节点上预先拉取训练镜像，避免启动时镜像拉取延迟
2. **Hostfile 验证**：在 Launcher 启动脚本中增加 `mpirun hostname` 预检步骤
3. **NCCL 环境一致性**：所有 Worker 使用相同的 NCCL、CUDA、MPI 版本
4. **共享内存大小**：为 NCCL 配置足够大的 `/dev/shm`（emptyDir medium: Memory）
5. **拓扑感知调度**：使用 Pod 亲和性将 Worker 调度到同机架/同交换机节点
6. **检查点策略**：长时间训练配置定期 checkpoint，防止 Spot 实例中断导致全部重来
7. **网络隔离**：为 MPI 通信分配独立网卡或 VLAN，避免与业务流量争用

### 典型问题案例

#### 案例一：多节点 NCCL all-reduce 超时

**问题描述**：4 节点 GPU 训练在 epoch 10 左右随机出现 `NCCL timeout`。

**根本原因**：节点分散在不同可用区，跨 AZ 网络延迟不稳定，偶发超过 NCCL 默认超时。

**解决方案**：
1. 将 NCCL_TIMEOUT 从默认 30 秒调整到 1800 秒
2. 配置 Pod 亲和性，优先将 Worker 调度到同 AZ
3. 使用支持 RDMA 的实例类型替代 TCP 通信

#### 案例二：Worker 节点 SSH 连接失败

**问题描述**：Launcher 日志显示 `Permission denied (publickey)`。

**根本原因**：MPI Operator 生成的 SSH Secret 挂载到 `/root/.ssh`，但文件权限为 0644，SSH 客户端拒绝使用权限过于开放的私钥。

**解决方案**：
1. 在 MPIJob 的 Secret volume 中设置 `defaultMode: 0600`
2. 或在启动脚本中执行 `chmod 600 /root/.ssh/id_rsa`

#### 案例三：slots 配置不当导致 GPU 利用率不均

**问题描述**：8 节点训练时，部分 GPU 利用率 100%，部分为 0%。

**根本原因**：`slotsPerWorker=4` 但每个 Worker 节点有 8 张 GPU，只使用了其中 4 张。

**解决方案**：
1. 将 `slotsPerWorker` 调整为 8，匹配节点 GPU 数
2. 或在 `mpirun` 参数中使用 `--map-by ppr:8:node` 显式指定映射策略

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting.md|01-ai-ml-workloads-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md|02-kubeflow-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting.md|01-ai-ml-workloads-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md|02-kubeflow-troubleshooting]]
