---
title: "AI 高性能网络（RDMA/InfiniBand/GPUDirect/NCCL）"
description: "K8s 集群中 RDMA/InfiniBand/GPUDirect RDMA 网络部署、NCCL 通信优化及 AI 训练网络性能调优"
summary: "覆盖 RoCE v2 vs InfiniBand 选型、NVIDIA Network Operator 部署、GPUDirect RDMA/Storage、NCCL 环境变量调优、SR-IOV + Multus 设备暴露、网络拓扑感知调度及 perftest/nccl-tests 性能基准"
category: AI基础设施
tags:
- rdma
- infiniband
- gpudirect
- nccl
- roce
- sriov
- high-performance-networking
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "K8s 集群如何配置 RDMA 网络给 AI 训练用"
- "NCCL timeout 怎么排查"
- "GPUDirect RDMA 怎么部署"
trigger_keywords:
- rdma
- infiniband
- gpudirect
- nccl
- roce
- sriov
- multus
prerequisites:
- kubectl-basics
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# AI 高性能网络（RDMA/InfiniBand/GPUDirect/NCCL）

## 概述

大规模 AI 训练（如千卡 LLM 训练）对网络带宽和延迟有极端要求。传统 TCP/IP 网络栈的 CPU 中断、内存拷贝、协议处理开销在 AllReduce 等集合通信中成为严重瓶颈。RDMA（Remote Direct Memory Access）技术绕过内核网络栈，实现 GPU 间零拷贝、低延迟、高带宽通信，是万卡集群训练的基础设施。

本文覆盖 RDMA 技术基础（RoCE v2 vs InfiniBand）、NVIDIA Network Operator 部署、GPUDirect RDMA/Storage 配置、NCCL 通信优化、SR-IOV + Multus 设备暴露、网络拓扑感知调度以及性能基准测试。帮助 AI 基础设施团队构建和运维高性能训练网络。

相关页面：[[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]、[[23-实体/11-AI与边缘/kubeflow|Kubeflow训练平台]]、[[23-实体/02-K8s核心组件/networkpolicy|K8s网络策略与CNI]]、[[15-AI基础设施/05-K8s-AI基础设施/14-gpu-cost-attribution-multitenant|AI集群多租户隔离]]、[[22-概念/08-可靠性与运维/node-lifecycle-management|K8s节点管理与运维]]

## 架构与核心概念

### RDMA 技术对比

| 维度 | InfiniBand | RoCE v2 | iWARP |
|------|-----------|---------|-------|
| 传输层 | 专用 IB 网络 | 以太网（UDP/IP） | 以太网（TCP/IP） |
| 延迟 | ~1μs | ~2μs | ~5-10μs |
| 带宽 | NDR 400Gbps | 100/200/400Gbps | 10/25/100Gbps |
| 交换机 | InfiniBand 交换机 | 标准以太网交换机 | 标准以太网交换机 |
| 成本 | 高（专用设备） | 中（需 RDMA 网卡） | 低 |
| 扩展性 | 子网管理器限制 | 大规模友好 | 大规模友好 |
| 拥塞控制 | 硬件级（Credit-based） | DCQCN/ECN | TCP 拥塞控制 |
| AI 训练适用性 | 最佳（NVIDIA 推荐） | 良好（性价比） | 不推荐 |
| 典型场景 | 超大规模训练集群 | 中大规模训练/推理 | 存储网络 |

### GPUDirect 技术栈

```
GPUDirect 技术层次:

GPUDirect P2P (Peer-to-Peer):
  - 同一节点内 GPU 间直接通信
  - 通过 NVLink/NVSwitch 或 PCIe
  - 无需经过 CPU 内存

GPUDirect RDMA:
  - 跨节点 GPU 显存直接 RDMA 读写
  - GPU → NIC → 网络 → NIC → GPU
  - 绕过 CPU 和系统内存
  - 需要 NIC 和 GPU 在同一 PCIe switch 下

GPUDirect Storage:
  - GPU 直接访问 NVMe/网络存储
  - 用于 Checkpoint 读写加速
  - 绕过 CPU bounce buffer
  - 需要 NVIDIA Magnum IO / cuFile

NCCL (NVIDIA Collective Communications Library):
  - AllReduce / AllGather / Broadcast 等集合通信
  - 自动检测并使用最优通信路径
  - 支持 NVLink + InfiniBand/RoCE 混合
  - Ring / Tree 算法选择
```

### 网络架构设计

```
典型 AI 训练集群网络架构:

计算网络（东西向）:
  - 每个 GPU 节点配备 8× ConnectX-7 NDR 400G HCA
  - Fat-tree 或 Rail-optimized 拓扑
  - 每个 GPU 绑定一个 HCA（GPU-HCA affinity）
  - 无阻塞带宽：节点间 3.2Tbps (8×400G)

存储网络（南北向）:
  - 独立 100GbE 网络
  - 连接并行文件系统（Lustre/GPFS/WekaFS）
  - GPUDirect Storage 加速 Checkpoint

管理网络:
  - 独立 25GbE
  - K8s 管理流量、SSH、监控

Rail-optimized 拓扑:
  - 所有节点的 GPU-0 连接到同一 Leaf 交换机
  - 所有节点的 GPU-1 连接到另一 Leaf
  - 减少跨交换机流量，降低延迟
```

## 生产部署

### NVIDIA Network Operator 部署

```bash
# 🟡 中风险：部署 NVIDIA Network Operator（管理 RDMA 驱动和设备）
helm repo add mellanox https://mellanox.github.io/network-operator
helm repo update

# 确认节点标签
kubectl label nodes --all node-role.kubernetes.io/worker="" --overwrite
kubectl label nodes gpu-node-01 nvidia.com/gpu.present=true --overwrite
```

```yaml
# 🟡 中风险：Network Operator 配置
# network-operator-values.yaml
operator:
  tag: "24.10.0"

# OFED 驱动（InfiniBand/RoCE）
ofedDriver:
  deploy: true
  image: doca-driver
  repository: nvcr.io/nvidia/mellanox
  version: 24.10-1.1.4.0-0

# RDMA 设备插件
rdmaSharedDevicePlugin:
  deploy: true
  resources:
  - name: rdma_shared_device_a
    vendors: [15b3]  # Mellanox
    devices: [101e]  # ConnectX-7
    ifNames: [enp65s0f0np0]

# SR-IOV 设备插件（VF 直通）
sriovDevicePlugin:
  deploy: true

# Multus CNI
multus:
  deploy: true
  config:
    cni_conf:
      clusterNetwork: default

# Whereabouts IPAM（RDMA 网络 IP 分配）
whereabouts:
  deploy: true

# Secondary Network（RDMA 专用网络）
secondaryNetwork:
  deploy: true
  cniPlugins:
    deploy: true
  ipamPlugin:
    deploy: true
```

```bash
# 🟡 中风险：安装 Network Operator
helm install network-operator mellanox/network-operator \
  --namespace nvidia-network-operator \
  --create-namespace \
  -f network-operator-values.yaml \
  --wait --timeout 600s

# 验证 RDMA 设备暴露
kubectl get nodes -o json | jq '.items[].status.allocatable | with_entries(select(.key | contains("rdma")))'
```

### SR-IOV + Multus 网络配置

```yaml
# 🟡 中风险：创建 RDMA 网络定义（NetworkAttachmentDefinition）
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: rdma-network
  namespace: ai-training
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "rdma-network",
      "plugins": [
        {
          "type": "sriov",
          "vlan": 100,
          "ipam": {
            "type": "whereabouts",
            "range": "192.168.100.0/24",
            "exclude": ["192.168.100.0/32", "192.168.100.254/32"]
          }
        }
      ]
    }
---
# 🟡 中风险：SR-IOV VF 配置（每个 GPU 节点 8 个 VF）
apiVersion: sriovnetwork.openshift.io/v1
kind: SriovNetworkNodePolicy
metadata:
  name: rdma-vf-policy
  namespace: nvidia-network-operator
spec:
  nodeSelector:
    feature.node.kubernetes.io/network-sriov.capable: "true"
  resourceName: rdma_vf
  numVfs: 8
  nicSelector:
    vendor: "15b3"
    deviceID: "101e"
  deviceType: netdevice
  isRdma: true
  linkType: ETH  # RoCE v2; 用 IB 则改为 "IB"
```

### AI 训练 Pod RDMA 网络配置

```yaml
# 🟡 中风险：训练 Pod 挂载 RDMA 网络
apiVersion: v1
kind: Pod
metadata:
  name: nccl-test-pod
  namespace: ai-training
  annotations:
    k8s.v1.cni.cncf.io/networks: |
      [
        {"name": "rdma-network", "interface": "net1"},
        {"name": "rdma-network", "interface": "net2"},
        {"name": "rdma-network", "interface": "net3"},
        {"name": "rdma-network", "interface": "net4"}
      ]
spec:
  containers:
  - name: trainer
    image: nvcr.io/nvidia/pytorch:24.10-py3
    securityContext:
      capabilities:
        add: ["IPC_LOCK", "SYS_RESOURCE"]
    resources:
      limits:
        nvidia.com/gpu: "8"
        rdma/rdma_shared_device_a: "4"
        memory: "512Gi"
      requests:
        nvidia.com/gpu: "8"
        rdma/rdma_shared_device_a: "4"
        cpu: "96"
        memory: "256Gi"
    env:
    - name: NCCL_DEBUG
      value: "INFO"
    - name: NCCL_IB_HCA
      value: "mlx5_0,mlx5_1,mlx5_2,mlx5_3"
    - name: NCCL_SOCKET_IFNAME
      value: "net1,net2,net3,net4"
    - name: NCCL_IB_GID_INDEX
      value: "3"
    - name: NCCL_NET_GDR_LEVEL
      value: "5"
    volumeMounts:
    - name: shm
      mountPath: /dev/shm
  volumes:
  - name: shm
    emptyDir:
      medium: Memory
      sizeLimit: "64Gi"
  tolerations:
  - key: nvidia.com/gpu
    operator: Exists
    effect: NoSchedule
```

## 运维操作

### NCCL 通信优化

```bash
# 🟢 低风险：NCCL 关键环境变量说明
# NCCL_SOCKET_IFNAME: 指定 NCCL 使用的网络接口（避免走管理网络）
# NCCL_IB_HCA: 指定 InfiniBand HCA 设备
# NCCL_IB_GID_INDEX: RoCE v2 必须设为 3
# NCCL_NET_GDR_LEVEL: GPUDirect RDMA 级别（5=PIX，同 PCIe switch）
# NCCL_IB_TC: InfiniBand Traffic Class（QoS）
# NCCL_IB_TIMEOUT: IB 超时（默认 18，增大避免拥塞超时）
# NCCL_ALGO: 算法选择（Ring/Tree/CollnetDirect）
# NCCL_PROTO: 协议（Simple/LL/LL128）
# NCCL_CROSS_NIC: 跨 NIC 通信（多 rail 时设为 1）

# 🟢 低风险：验证 RDMA 连通性
kubectl exec -n ai-training nccl-test-pod -- ibstat
kubectl exec -n ai-training nccl-test-pod -- ibv_devinfo
kubectl exec -n ai-training nccl-test-pod -- rdma link show
```

### 性能基准测试

```bash
# 🟢 低风险：perftest 带宽测试（两节点间）
# 服务端（节点 A）
kubectl exec -n ai-training nccl-test-pod-a -- \
  ib_write_bw -d mlx5_0 -F --report_gbits -s 1048576

# 客户端（节点 B）
kubectl exec -n ai-training nccl-test-pod-b -- \
  ib_write_bw -d mlx5_0 -F --report_gbits -s 1048576 192.168.100.1

# 🟢 低风险：NCCL AllReduce 性能测试
kubectl exec -n ai-training nccl-test-pod -- bash -c "
export NCCL_DEBUG=INFO
export NCCL_IB_HCA=mlx5_0,mlx5_1,mlx5_2,mlx5_3
export NCCL_SOCKET_IFNAME=net1,net2,net3,net4
export NCCL_NET_GDR_LEVEL=5
/usr/local/bin/all_reduce_perf -b 8 -e 4G -f 2 -g 8 -n 100
"
# 预期结果（8×A100 + 4×NDR 400G）:
# 4GB AllReduce: busbw > 350 GB/s
```

### 网络拓扑验证

```bash
# 🟢 低风险：检查 GPU-NIC 拓扑亲和性
kubectl exec -n ai-training nccl-test-pod -- nvidia-smi topo -m
# 输出示例:
#         GPU0  GPU1  GPU2  GPU3  mlx5_0  mlx5_1
# GPU0     X    NV18  NV18  NV18  PIX     SYS
# GPU1    NV18   X    NV18  NV18  SYS     PIX
# PIX = 同一 PCIe switch（最优）
# SYS = 跨 NUMA node（需避免）

# 🟢 低风险：检查 NUMA 拓扑
kubectl exec -n ai-training nccl-test-pod -- numactl --hardware
kubectl exec -n ai-training nccl-test-pod -- cat /sys/class/infiniband/mlx5_0/device/numa_node
```

## 故障排查

### NCCL Timeout

```bash
# 🟢 低风险：诊断 NCCL 超时
# Step 1: 查看训练日志中的 NCCL 错误
kubectl logs <training-pod> -n ai-training --tail=200 | grep -i "nccl\|timeout\|unhandled"
# 典型错误: "NCCL WARN Cuda failure" 或 "NCCL timeout 1800s"

# Step 2: 检查网络连通性
kubectl exec <training-pod> -n ai-training -- ping -c 3 192.168.100.2
kubectl exec <training-pod> -n ai-training -- ibping -S  # 服务端
kubectl exec <training-pod> -n ai-training -- ibping -L <remote_lid>  # 客户端

# Step 3: 检查 RDMA 设备状态
kubectl exec <training-pod> -n ai-training -- ibstat
# 确认 State: Active, Physical state: LinkUp

# Step 4: 检查 NCCL 环境变量
kubectl exec <training-pod> -n ai-training -- env | grep NCCL

# 常见原因:
# 1. NCCL_SOCKET_IFNAME 未设置或指向错误接口
# 2. 防火墙/安全组阻断 RDMA 端口
# 3. IB 子网管理器未运行（InfiniBand）
# 4. GPU-NIC 不在同一 PCIe switch（GDR 失败回退到慢路径）
# 5. MTU 不匹配（RoCE 需要 jumbo frame 9000）
```

### RDMA 连接失败

```bash
# 🟢 低风险：RDMA 连接诊断
# 检查 RDMA 设备是否就绪
kubectl exec <pod> -n ai-training -- rdma link show
# 期望: link mlx5_0/1 state ACTIVE physical_state LINK_UP

# 检查 OFED 驱动版本
kubectl exec <pod> -n ai-training -- ofed_info -s
# 确认与 Network Operator 部署的版本一致

# 检查 GPUDirect RDMA 模块
kubectl exec <pod> -n ai-training -- lsmod | grep nv_peer_mem
# 或 (新版驱动)
kubectl exec <pod> -n ai-training -- cat /sys/kernel/mm/memory_peers/nv_mem/version

# 🔴 高风险：重启 RDMA 设备（会中断所有 RDMA 连接）
kubectl exec <pod> -n ai-training -- bash -c "
echo 0 > /sys/class/net/enp65s0f0np0/device/sriov_numvfs
echo 8 > /sys/class/net/enp65s0f0np0/device/sriov_numvfs
"
```

### 带宽不达标

| 故障现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| AllReduce busbw < 预期 50% | NCCL 未使用 RDMA | `NCCL_DEBUG=INFO` 查看 transport | 设置 NCCL_IB_HCA/NCCL_SOCKET_IFNAME |
| 单流带宽 < 350Gbps (NDR) | PCIe 带宽瓶颈或 MTU 问题 | `ib_write_bw -s 1M` | 检查 PCIe Gen5 x16、MTU 9000 |
| 多流带宽不线性扩展 | 拥塞或 PFC 配置问题 | 检查交换机 ECN/PFC 计数 | 调整 NCCL_IB_TC、检查交换机 QoS |
| GPUDirect 未生效 | nv_peer_mem 未加载或拓扑不对 | `nvidia-smi topo -m` | 确认 GPU-NIC PIX 关系、加载驱动 |
| 间歇性超时 | 网络拥塞或 IB SM 不稳定 | 检查 IB SM 日志、端口错误计数 | 增加 NCCL_IB_TIMEOUT、检查 SM |

## 最佳实践

### NCCL 环境变量推荐配置

```bash
# 🟢 低风险：生产环境 NCCL 推荐配置（8×GPU + 4×NDR HCA）
NCCL_DEBUG=WARN                    # 生产用 WARN，调试用 INFO
NCCL_IB_HCA=mlx5_0,mlx5_1,mlx5_2,mlx5_3
NCCL_SOCKET_IFNAME=net1,net2,net3,net4
NCCL_IB_GID_INDEX=3               # RoCE v2 必须
NCCL_NET_GDR_LEVEL=5              # PIX（同 PCIe switch）
NCCL_IB_TC=106                    # DSCP 26，匹配交换机 QoS
NCCL_IB_TIMEOUT=22                # 增大超时避免拥塞误判
NCCL_CROSS_NIC=1                  # 允许跨 NIC 通信
NCCL_ALGO=Ring,Tree               # 允许 NCCL 自动选择
NCCL_MIN_NCHANNELS=16            # 增加并行通道
NCCL_MAX_NCHANNELS=32
NCCL_NTHREADS=512                 # 增加线程数
```

### 网络拓扑感知调度

```yaml
# 🟡 中风险：Topology Manager 配置（kubelet 参数）
# 确保 GPU 和 NIC 在同一 NUMA node / PCIe switch
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
topologyManagerPolicy: single-numa-node
topologyManagerScope: pod
cpuManagerPolicy: static
memoryManagerPolicy: Static
reservedMemory:
- numaNode: 0
  limits:
    memory: "4Gi"
```

### 部署检查清单

1. **硬件验证**：`nvidia-smi topo -m` 确认 GPU-NIC PIX 关系
2. **驱动版本**：OFED 驱动与 NVIDIA GPU 驱动兼容
3. **网络配置**：RoCE v2 需启用 PFC/ECN，MTU 9000
4. **NCCL 验证**：nccl-tests AllReduce busbw 达到理论带宽 80%+
5. **故障恢复**：NCCL_IB_TIMEOUT 适当增大，训练框架支持 elastic restart
6. **监控**：采集 RDMA 端口错误计数、PFC pause 帧、ECN 标记率

### 性能基准参考

| 配置 | AllReduce 4GB busbw | 预期 |
|------|-------------------|------|
| 8×A100 + 8×HDR 200G (单节点) | ~280 GB/s | NVLink 带宽上限 |
| 2×8×A100 + 4×NDR 400G (跨节点) | ~350 GB/s | 网络带宽上限 |
| 2×8×H100 + 8×NDR 400G (跨节点) | ~600 GB/s | NVSwitch + NDR |
| 64×H100 (8 节点) + NDR Fat-tree | ~550 GB/s | 多跳略有损耗 |

## Related

- [[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]
- [[23-实体/11-AI与边缘/kubeflow|Kubeflow训练平台]]
- [[23-实体/02-K8s核心组件/networkpolicy|K8s网络策略与CNI]]
- [[15-AI基础设施/05-K8s-AI基础设施/14-gpu-cost-attribution-multitenant|AI集群多租户隔离]]
- [[22-概念/08-可靠性与运维/node-lifecycle-management|K8s节点管理与运维]]
