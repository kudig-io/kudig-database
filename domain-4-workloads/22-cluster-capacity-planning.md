# 集群容量规划

## 概述

容量规划是确保 Kubernetes 集群能够满足当前和未来工作负载需求的关键实践。本文档详细介绍集群规模限制、节点容量规划、资源预留策略和容量监控方法。

## 容量规划架构

### 容量计算模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              Kubernetes 容量规划模型                                 │
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                            集群层 (Cluster Level)                               │ │
│  │                                                                                 │ │
│  │   总集群容量 = Σ(各节点可分配资源)                                              │ │
│  │                                                                                 │ │
│  │   ┌─────────────────────────────────────────────────────────────────────────┐  │ │
│  │   │                      控制面限制 (Control Plane Limits)                   │  │ │
│  │   │                                                                          │  │ │
│  │   │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │  │ │
│  │   │  │  etcd        │  │  API Server  │  │  Scheduler   │                   │  │ │
│  │   │  │  Max: 100GB  │  │  QPS: 3000   │  │  Throughput  │                   │  │ │
│  │   │  │  Keys: 1.5M  │  │  Concurrent  │  │  Pending:    │                   │  │ │
│  │   │  │              │  │  Requests    │  │  5000 Pods   │                   │  │ │
│  │   │  └──────────────┘  └──────────────┘  └──────────────┘                   │  │ │
│  │   │                                                                          │  │ │
│  │   └─────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                                                                 │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                           │                                          │
│                                           │                                          │
│  ┌────────────────────────────────────────┼───────────────────────────────────────┐ │
│  │                           节点层 (Node Level)                                   │ │
│  │                                                                                 │ │
│  │   ┌─────────────────────────────────────────────────────────────────────────┐  │ │
│  │   │                        节点总容量 (Node Capacity)                        │  │ │
│  │   │                                                                          │  │ │
│  │   │  ┌───────────────────────────────────────────────────────────────────┐  │  │ │
│  │   │  │                    系统预留 (System Reserved)                      │  │  │ │
│  │   │  │   • 操作系统进程          • 系统服务                               │  │  │ │
│  │   │  │   • SSH/Agents            • 日志收集                               │  │  │ │
│  │   │  └───────────────────────────────────────────────────────────────────┘  │  │ │
│  │   │                                                                          │  │ │
│  │   │  ┌───────────────────────────────────────────────────────────────────┐  │  │ │
│  │   │  │                    K8s 预留 (Kube Reserved)                        │  │  │ │
│  │   │  │   • kubelet               • kube-proxy                             │  │  │ │
│  │   │  │   • containerd            • Node-local DNS                         │  │  │ │
│  │   │  └───────────────────────────────────────────────────────────────────┘  │  │ │
│  │   │                                                                          │  │ │
│  │   │  ┌───────────────────────────────────────────────────────────────────┐  │  │ │
│  │   │  │                    驱逐阈值 (Eviction Threshold)                   │  │  │ │
│  │   │  │   • memory.available      • nodefs.available                       │  │  │ │
│  │   │  │   • imagefs.available     • pid.available                          │  │  │ │
│  │   │  └───────────────────────────────────────────────────────────────────┘  │  │ │
│  │   │                                                                          │  │ │
│  │   │  ┌───────────────────────────────────────────────────────────────────┐  │  │ │
│  │   │  │                   可分配资源 (Allocatable)                         │  │  │ │
│  │   │  │                                                                    │  │  │ │
│  │   │  │   Allocatable = Capacity - System - Kube - Eviction               │  │  │ │
│  │   │  │                                                                    │  │  │ │
│  │   │  │   ┌─────────────────────────────────────────────────────────────┐ │  │  │ │
│  │   │  │   │                 Pod 资源需求 (Pod Resources)                 │ │  │  │ │
│  │   │  │   │  • Requests (调度保证)    • Limits (使用上限)                │ │  │  │ │
│  │   │  │   │  • QoS 类别               • 资源配额                         │ │  │  │ │
│  │   │  │   └─────────────────────────────────────────────────────────────┘ │  │  │ │
│  │   │  │                                                                    │  │  │ │
│  │   │  └───────────────────────────────────────────────────────────────────┘  │  │ │
│  │   │                                                                          │  │ │
│  │   └─────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                                                                 │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 多维容量视图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              多维容量规划视图                                        │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                           资源维度 (Resource Dimensions)                     │   │
│   │                                                                              │   │
│   │    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐            │   │
│   │    │   CPU    │    │  Memory  │    │  Storage │    │  Network │            │   │
│   │    │          │    │          │    │          │    │          │            │   │
│   │    │ Cores    │    │ GiB      │    │ GiB/IOPS │    │ Gbps     │            │   │
│   │    │ Threads  │    │ Huge     │    │ PV Count │    │ PPS      │            │   │
│   │    │ NUMA     │    │ Pages    │    │ SC Types │    │ Conn     │            │   │
│   │    └──────────┘    └──────────┘    └──────────┘    └──────────┘            │   │
│   │                                                                              │   │
│   │    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐            │   │
│   │    │   GPU    │    │   FPGA   │    │  SR-IOV  │    │  Custom  │            │   │
│   │    │          │    │          │    │          │    │  Device  │            │   │
│   │    │ nvidia   │    │ xilinx   │    │ VF Count │    │ Extended │            │   │
│   │    │ amd      │    │ intel    │    │ PCI      │    │ Resource │            │   │
│   │    └──────────┘    └──────────┘    └──────────┘    └──────────┘            │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                            时间维度 (Time Dimensions)                        │   │
│   │                                                                              │   │
│   │   当前需求                  增长预测                    峰值规划             │   │
│   │   ┌────────┐               ┌────────┐                 ┌────────┐            │   │
│   │   │ Base   │ ──────────►  │ Growth │ ──────────────► │ Peak   │            │   │
│   │   │ Load   │              │ +20%/Q │                 │ 2x~3x  │            │   │
│   │   └────────┘               └────────┘                 └────────┘            │   │
│   │                                                                              │   │
│   │   日常基线          ────►  季度增长预测   ────────►    峰值/DR 容量          │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          拓扑维度 (Topology Dimensions)                      │   │
│   │                                                                              │   │
│   │   ┌───────────────────────────────────────────────────────────────────────┐ │   │
│   │   │                          Region: us-east-1                            │ │   │
│   │   │                                                                       │ │   │
│   │   │  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                │ │   │
│   │   │  │  AZ: 1a     │   │  AZ: 1b     │   │  AZ: 1c     │                │ │   │
│   │   │  │             │   │             │   │             │                │ │   │
│   │   │  │  Cluster A  │   │  Cluster B  │   │  Cluster C  │                │ │   │
│   │   │  │  100 Nodes  │   │  100 Nodes  │   │  100 Nodes  │                │ │   │
│   │   │  └─────────────┘   └─────────────┘   └─────────────┘                │ │   │
│   │   │                                                                       │ │   │
│   │   └───────────────────────────────────────────────────────────────────────┘ │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## 集群规模限制

### Kubernetes 官方规模限制

| 维度 | 限制值 | 说明 | 影响因素 |
|-----|--------|------|---------|
| **节点数量** | 5000 节点 | 单集群最大节点数 | etcd 性能、API Server 负载 |
| **Pod 总数** | 150,000 Pod | 单集群最大 Pod 数 | 调度器吞吐量、etcd 存储 |
| **每节点 Pod 数** | 110 Pod | 单节点默认最大 Pod 数 | kubelet 性能、IP 地址空间 |
| **每节点容器数** | 300 容器 | 单节点推荐最大容器数 | 运行时性能 |
| **Service 数量** | 10,000 Service | 单集群最大 Service 数 | kube-proxy 规则数量 |
| **Endpoints 每 Service** | 5000 | 单 Service 最大端点数 | iptables/IPVS 性能 |
| **ConfigMap 大小** | 1 MiB | 单个 ConfigMap 最大大小 | etcd 限制 |
| **Secret 大小** | 1 MiB | 单个 Secret 最大大小 | etcd 限制 |
| **etcd 数据大小** | 8 GB (默认) | etcd 数据库大小 | 可配置至 100GB |
| **API 请求大小** | 1.5 MiB | 单次 API 请求最大大小 | API Server 配置 |

### 控制平面规模参考

| 集群规模 | 节点数 | Pod 数 | etcd 配置 | API Server 配置 |
|---------|--------|--------|-----------|----------------|
| **小型** | 1-100 | <3000 | 3 节点, 8GB RAM | 2 副本, 4 vCPU |
| **中型** | 100-500 | 3000-15000 | 3 节点, 16GB RAM | 3 副本, 8 vCPU |
| **大型** | 500-2000 | 15000-60000 | 5 节点, 32GB RAM | 3 副本, 16 vCPU |
| **超大型** | 2000-5000 | 60000-150000 | 5 节点, 64GB RAM | 5 副本, 32 vCPU |

### 组件资源需求计算

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                            控制面组件资源计算                                        │
│                                                                                      │
│   etcd 资源需求:                                                                     │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │   CPU: 基础 2 核 + (节点数/500) × 2 核                                       │   │
│   │   Memory: 基础 4GB + (对象数/10000) × 1GB                                    │   │
│   │   Disk: SSD, IOPS > 3000, 延迟 < 10ms                                        │   │
│   │   Network: 带宽 > 1Gbps, 延迟 < 2ms (集群内)                                 │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   API Server 资源需求:                                                               │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │   CPU: 基础 2 核 + (QPS/1000) × 2 核                                         │   │
│   │   Memory: 基础 4GB + (Watch 数量/10000) × 2GB                                │   │
│   │   参数调优: --max-requests-inflight, --max-mutating-requests-inflight       │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   Scheduler 资源需求:                                                                │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │   CPU: 基础 2 核 + (Pod 数/10000) × 1 核                                     │   │
│   │   Memory: 基础 2GB + (节点数/1000) × 1GB                                     │   │
│   │   参数调优: percentageOfNodesToScore                                         │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   Controller Manager 资源需求:                                                       │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │   CPU: 基础 2 核 + (控制器数量) × 0.5 核                                     │   │
│   │   Memory: 基础 4GB + (对象数/50000) × 2GB                                    │   │
│   │   参数调优: --concurrent-*-syncs                                             │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## 节点容量规划

### 节点类型对比

| 节点类型 | CPU (vCPU) | 内存 (GiB) | 存储 | 适用场景 | 每节点 Pod 数 |
|---------|-----------|-----------|------|---------|--------------|
| **通用型** | 4-16 | 16-64 | 100GB SSD | Web 应用、微服务 | 30-60 |
| **计算优化型** | 32-96 | 64-192 | 100GB SSD | 批处理、编译 | 20-40 |
| **内存优化型** | 8-32 | 128-512 | 100GB SSD | 数据库、缓存 | 10-30 |
| **存储优化型** | 8-16 | 32-64 | 2-16TB NVMe | 数据密集型 | 20-40 |
| **GPU 节点** | 8-32 | 64-256 | 200GB SSD | ML/AI 训练 | 5-20 |
| **高密度节点** | 64-128 | 256-512 | 500GB SSD | 小 Pod 高密度 | 100-250 |

### 资源预留计算公式

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              节点资源预留计算                                        │
│                                                                                      │
│   可分配资源 (Allocatable) = 节点容量 - 系统预留 - K8s预留 - 驱逐阈值               │
│                                                                                      │
│   ╔═══════════════════════════════════════════════════════════════════════════════╗ │
│   ║                          CPU 资源预留公式                                      ║ │
│   ╠═══════════════════════════════════════════════════════════════════════════════╣ │
│   ║                                                                                ║ │
│   ║   系统预留 CPU:                                                                ║ │
│   ║   • 核心数 ≤ 4:     100m                                                       ║ │
│   ║   • 核心数 5-16:    100m + (cores - 4) × 25m                                   ║ │
│   ║   • 核心数 17-64:   400m + (cores - 16) × 12.5m                                ║ │
│   ║   • 核心数 > 64:    1000m + (cores - 64) × 6.25m                               ║ │
│   ║                                                                                ║ │
│   ║   K8s 预留 CPU:                                                                ║ │
│   ║   • 固定值: 100m - 200m                                                        ║ │
│   ║                                                                                ║ │
│   ╚═══════════════════════════════════════════════════════════════════════════════╝ │
│                                                                                      │
│   ╔═══════════════════════════════════════════════════════════════════════════════╗ │
│   ║                         内存资源预留公式                                       ║ │
│   ╠═══════════════════════════════════════════════════════════════════════════════╣ │
│   ║                                                                                ║ │
│   ║   系统预留 Memory:                                                             ║ │
│   ║   • 内存 ≤ 4GB:     255Mi                                                      ║ │
│   ║   • 内存 4-8GB:     255Mi + (memory - 4GB) × 25.6Mi/GB                         ║ │
│   ║   • 内存 8-16GB:    358Mi + (memory - 8GB) × 20.48Mi/GB                        ║ │
│   ║   • 内存 16-128GB:  522Mi + (memory - 16GB) × 10.24Mi/GB                       ║ │
│   ║   • 内存 > 128GB:   1669Mi + (memory - 128GB) × 5.12Mi/GB                      ║ │
│   ║                                                                                ║ │
│   ║   K8s 预留 Memory:                                                             ║ │
│   ║   • 固定值: 256Mi - 512Mi                                                      ║ │
│   ║                                                                                ║ │
│   ║   驱逐阈值 Memory:                                                             ║ │
│   ║   • 硬驱逐: 100Mi                                                              ║ │
│   ║   • 软驱逐: 200Mi (可选)                                                       ║ │
│   ║                                                                                ║ │
│   ╚═══════════════════════════════════════════════════════════════════════════════╝ │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 常见节点规格资源预留参考

| 节点规格 | 总 CPU | 总内存 | 系统预留 CPU | 系统预留内存 | K8s 预留 CPU | K8s 预留内存 | 可分配 CPU | 可分配内存 |
|---------|--------|--------|-------------|-------------|-------------|-------------|-----------|-----------|
| 4c8g | 4000m | 8Gi | 100m | 358Mi | 100m | 256Mi | 3800m | 7.4Gi |
| 8c16g | 8000m | 16Gi | 200m | 522Mi | 150m | 256Mi | 7650m | 15.2Gi |
| 16c32g | 16000m | 32Gi | 400m | 686Mi | 200m | 512Mi | 15400m | 30.8Gi |
| 32c64g | 32000m | 64Gi | 600m | 1014Mi | 200m | 512Mi | 31200m | 62.5Gi |
| 64c128g | 64000m | 128Gi | 1000m | 1669Mi | 300m | 512Mi | 62700m | 125.8Gi |
| 96c192g | 96000m | 192Gi | 1200m | 1997Mi | 300m | 512Mi | 94500m | 189.5Gi |

## 容量规划维度

### 工作负载分类与资源需求

| 工作负载类型 | CPU 特征 | 内存特征 | 存储特征 | 网络特征 | 典型 Pod 数/节点 |
|-------------|---------|---------|---------|---------|-----------------|
| **无状态 Web** | 低-中 | 低-中 | 临时存储 | 中等带宽 | 30-50 |
| **API 服务** | 中 | 中 | 临时存储 | 中等 QPS | 20-40 |
| **后台任务** | 高 | 中 | 临时存储 | 低 | 10-20 |
| **数据处理** | 高 | 高 | 高 IOPS | 高带宽 | 5-15 |
| **内存缓存** | 低 | 极高 | 无 | 中等 | 2-5 |
| **数据库** | 中-高 | 高 | 高 IOPS | 中等 | 1-3 |
| **ML 训练** | GPU 密集 | 高 | 高吞吐 | 高带宽 | 1-4 |
| **ML 推理** | GPU/CPU | 中-高 | 中等 | 中等延迟 | 5-15 |

### 容量计算示例

```yaml
# capacity-calculation-example.yaml
# 容量需求计算示例

# 场景: 电商平台容量规划
# 目标: 支撑 10 万 DAU, 峰值 1000 QPS

---
# 1. 工作负载分析
workload_analysis:
  web_frontend:
    replicas: 20
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
    total_requests:
      cpu: "10000m"    # 20 × 500m
      memory: "10Gi"   # 20 × 512Mi
      
  api_gateway:
    replicas: 10
    resources:
      requests:
        cpu: "1000m"
        memory: "1Gi"
      limits:
        cpu: "2000m"
        memory: "2Gi"
    total_requests:
      cpu: "10000m"    # 10 × 1000m
      memory: "10Gi"   # 10 × 1Gi
      
  order_service:
    replicas: 15
    resources:
      requests:
        cpu: "500m"
        memory: "1Gi"
      limits:
        cpu: "1000m"
        memory: "2Gi"
    total_requests:
      cpu: "7500m"     # 15 × 500m
      memory: "15Gi"   # 15 × 1Gi
      
  payment_service:
    replicas: 8
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
    total_requests:
      cpu: "4000m"     # 8 × 500m
      memory: "4Gi"    # 8 × 512Mi
      
  redis_cluster:
    replicas: 6
    resources:
      requests:
        cpu: "500m"
        memory: "4Gi"
      limits:
        cpu: "1000m"
        memory: "8Gi"
    total_requests:
      cpu: "3000m"     # 6 × 500m
      memory: "24Gi"   # 6 × 4Gi
      
  mysql_cluster:
    replicas: 3
    resources:
      requests:
        cpu: "2000m"
        memory: "8Gi"
      limits:
        cpu: "4000m"
        memory: "16Gi"
    total_requests:
      cpu: "6000m"     # 3 × 2000m
      memory: "24Gi"   # 3 × 8Gi
      
  monitoring_stack:
    prometheus:
      cpu: "2000m"
      memory: "8Gi"
    grafana:
      cpu: "500m"
      memory: "1Gi"
    alertmanager:
      cpu: "200m"
      memory: "256Mi"
    total_requests:
      cpu: "2700m"
      memory: "9.25Gi"

---
# 2. 总资源需求汇总
total_requirements:
  workload_cpu: "43200m"        # 43.2 vCPU
  workload_memory: "96.25Gi"
  
  # 系统组件开销 (约 15%)
  system_overhead_cpu: "6480m"
  system_overhead_memory: "14.4Gi"
  
  # 峰值缓冲 (约 30%)
  peak_buffer_cpu: "12960m"
  peak_buffer_memory: "28.9Gi"
  
  # 总需求
  total_cpu: "62640m"           # ~63 vCPU
  total_memory: "139.55Gi"      # ~140 Gi

---
# 3. 节点规划
node_planning:
  # 选择 16c32g 节点
  node_spec:
    cpu: "16000m"
    memory: "32Gi"
    allocatable_cpu: "15400m"   # 扣除预留
    allocatable_memory: "30.8Gi"
  
  # 计算节点数量
  nodes_by_cpu: 5               # 63000/15400 ≈ 4.1 → 5 节点
  nodes_by_memory: 5            # 140/30.8 ≈ 4.5 → 5 节点
  
  # 高可用要求 (N+1)
  recommended_nodes: 6
  
  # 跨 AZ 分布
  az_distribution:
    az-a: 2
    az-b: 2
    az-c: 2

---
# 4. 扩展性预估
scaling_projection:
  current:
    dau: 100000
    nodes: 6
    pods: ~62
    
  quarter_growth: "+20%"
  
  q1_projection:
    dau: 120000
    nodes: 7
    pods: ~74
    
  q2_projection:
    dau: 144000
    nodes: 9
    pods: ~89
    
  q3_projection:
    dau: 173000
    nodes: 11
    pods: ~107
    
  q4_projection:
    dau: 207000
    nodes: 13
    pods: ~128
```

## 节点池策略

### 节点池架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              节点池架构设计                                          │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                        System Node Pool (系统节点池)                         │   │
│   │                                                                              │   │
│   │   节点数: 3-5                     规格: 4c16g                               │   │
│   │   用途: 系统组件                   Taint: CriticalAddonsOnly                │   │
│   │                                                                              │   │
│   │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐      │   │
│   │   │   CoreDNS    │ │  Ingress     │ │  Monitoring  │ │  Logging     │      │   │
│   │   │   kube-dns   │ │  Controller  │ │  Prometheus  │ │  Fluentd     │      │   │
│   │   └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘      │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                        General Node Pool (通用节点池)                        │   │
│   │                                                                              │   │
│   │   节点数: 10-100 (自动伸缩)        规格: 8c32g                              │   │
│   │   用途: 无状态服务                 Labels: workload-type=general            │   │
│   │                                                                              │   │
│   │   ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐   │   │
│   │   │ Frontend  │ │   API     │ │  Backend  │ │  Workers  │ │  Cron     │   │   │
│   │   │  Pods     │ │   Pods    │ │   Pods    │ │   Pods    │ │  Jobs     │   │   │
│   │   └───────────┘ └───────────┘ └───────────┘ └───────────┘ └───────────┘   │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                       Memory Node Pool (内存优化节点池)                      │   │
│   │                                                                              │   │
│   │   节点数: 3-10                     规格: 8c64g / 16c128g                    │   │
│   │   用途: 内存密集型                 Labels: workload-type=memory             │   │
│   │   Taint: memory-optimized=true:NoSchedule                                   │   │
│   │                                                                              │   │
│   │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                       │   │
│   │   │    Redis     │ │ Elasticsearch│ │ In-Memory DB │                       │   │
│   │   │   Cluster    │ │   Cluster    │ │   (SAP HANA) │                       │   │
│   │   └──────────────┘ └──────────────┘ └──────────────┘                       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                        GPU Node Pool (GPU 节点池)                           │   │
│   │                                                                              │   │
│   │   节点数: 2-20                     规格: 8c64g + 4×A100                     │   │
│   │   用途: ML/AI 工作负载             Labels: accelerator=nvidia-a100          │   │
│   │   Taint: nvidia.com/gpu=true:NoSchedule                                     │   │
│   │                                                                              │   │
│   │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                       │   │
│   │   │ ML Training  │ │ ML Inference │ │  Video       │                       │   │
│   │   │   Jobs       │ │  Services    │ │  Processing  │                       │   │
│   │   └──────────────┘ └──────────────┘ └──────────────┘                       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                       Spot Node Pool (抢占式节点池)                          │   │
│   │                                                                              │   │
│   │   节点数: 0-50 (弹性)              规格: 混合 (8c32g, 16c64g)               │   │
│   │   用途: 批处理/可中断任务          Labels: spot=true                        │   │
│   │   Taint: spot=true:NoSchedule                                               │   │
│   │   成本: 正常价格的 30-70%                                                   │   │
│   │                                                                              │   │
│   │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                       │   │
│   │   │ Batch Jobs   │ │ CI/CD        │ │ Data         │                       │   │
│   │   │              │ │ Runners      │ │ Processing   │                       │   │
│   │   └──────────────┘ └──────────────┘ └──────────────┘                       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 节点池配置示例

```yaml
# node-pools-config.yaml
# 多节点池配置示例

---
# 系统节点池
apiVersion: v1
kind: ConfigMap
metadata:
  name: system-nodepool-config
  namespace: kube-system
data:
  nodepool.yaml: |
    name: system-pool
    nodeCount:
      min: 3
      max: 5
      desired: 3
    instanceTypes:
      - m5.xlarge       # AWS
      - Standard_D4s_v3 # Azure
      - n1-standard-4   # GCP
    labels:
      node.kubernetes.io/pool: system
      workload-type: system
    taints:
      - key: CriticalAddonsOnly
        effect: NoSchedule
    kubeletConfig:
      maxPods: 30
      kubeReserved:
        cpu: "200m"
        memory: "512Mi"
      systemReserved:
        cpu: "200m"
        memory: "512Mi"

---
# 通用节点池
apiVersion: v1
kind: ConfigMap
metadata:
  name: general-nodepool-config
  namespace: kube-system
data:
  nodepool.yaml: |
    name: general-pool
    nodeCount:
      min: 5
      max: 100
      desired: 10
    instanceTypes:
      - m5.2xlarge      # AWS
      - Standard_D8s_v3 # Azure
      - n1-standard-8   # GCP
    labels:
      node.kubernetes.io/pool: general
      workload-type: general
    kubeletConfig:
      maxPods: 58
      kubeReserved:
        cpu: "200m"
        memory: "512Mi"
      systemReserved:
        cpu: "300m"
        memory: "1Gi"
    autoscaling:
      enabled: true
      scaleDownDelayAfterAdd: 10m
      scaleDownUnneededTime: 10m
      scaleDownUtilizationThreshold: 0.5

---
# 内存优化节点池
apiVersion: v1
kind: ConfigMap
metadata:
  name: memory-nodepool-config
  namespace: kube-system
data:
  nodepool.yaml: |
    name: memory-pool
    nodeCount:
      min: 2
      max: 20
      desired: 5
    instanceTypes:
      - r5.2xlarge      # AWS
      - Standard_E8s_v3 # Azure
      - n1-highmem-8    # GCP
    labels:
      node.kubernetes.io/pool: memory
      workload-type: memory
    taints:
      - key: memory-optimized
        value: "true"
        effect: NoSchedule
    kubeletConfig:
      maxPods: 30
      kubeReserved:
        cpu: "200m"
        memory: "1Gi"
      systemReserved:
        cpu: "300m"
        memory: "2Gi"

---
# GPU 节点池
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-nodepool-config
  namespace: kube-system
data:
  nodepool.yaml: |
    name: gpu-pool
    nodeCount:
      min: 0
      max: 20
      desired: 2
    instanceTypes:
      - p3.8xlarge      # AWS (4×V100)
      - Standard_NC24s_v3 # Azure (4×V100)
      - n1-standard-8-nvidia-tesla-v100-4 # GCP
    labels:
      node.kubernetes.io/pool: gpu
      workload-type: gpu
      accelerator: nvidia-tesla-v100
    taints:
      - key: nvidia.com/gpu
        value: "true"
        effect: NoSchedule
    kubeletConfig:
      maxPods: 20
      kubeReserved:
        cpu: "500m"
        memory: "2Gi"
        nvidia.com/gpu: "0"
      systemReserved:
        cpu: "500m"
        memory: "2Gi"

---
# Spot/抢占式节点池
apiVersion: v1
kind: ConfigMap
metadata:
  name: spot-nodepool-config
  namespace: kube-system
data:
  nodepool.yaml: |
    name: spot-pool
    nodeCount:
      min: 0
      max: 50
      desired: 0
    spotInstances: true
    instanceTypes:
      - m5.2xlarge
      - m5.4xlarge
      - m5a.2xlarge
      - m5a.4xlarge
    labels:
      node.kubernetes.io/pool: spot
      kubernetes.io/lifecycle: spot
      spot: "true"
    taints:
      - key: spot
        value: "true"
        effect: NoSchedule
    kubeletConfig:
      maxPods: 58
    spotConfig:
      maxPrice: "0.10"
      interruptionBehavior: terminate
```

## 自动伸缩配置

### Cluster Autoscaler 配置

```yaml
# cluster-autoscaler-config.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
  labels:
    app: cluster-autoscaler
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
      serviceAccountName: cluster-autoscaler
      priorityClassName: system-cluster-critical
      securityContext:
        runAsNonRoot: true
        runAsUser: 65534
        fsGroup: 65534
      containers:
        - name: cluster-autoscaler
          image: registry.k8s.io/autoscaling/cluster-autoscaler:v1.28.0
          command:
            - ./cluster-autoscaler
            - --v=4
            - --stderrthreshold=info
            - --cloud-provider=aws
            - --skip-nodes-with-local-storage=false
            - --skip-nodes-with-system-pods=false
            - --expander=least-waste
            - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/my-cluster
            # 扩容配置
            - --scale-down-enabled=true
            - --scale-down-delay-after-add=10m
            - --scale-down-delay-after-delete=0s
            - --scale-down-delay-after-failure=3m
            - --scale-down-unneeded-time=10m
            - --scale-down-unready-time=20m
            - --scale-down-utilization-threshold=0.5
            # 节点选择
            - --balance-similar-node-groups=true
            - --expendable-pods-priority-cutoff=-10
            # 限制
            - --max-node-provision-time=15m
            - --max-nodes-total=500
            - --cores-total=0:10000
            - --memory-total=0:100000
            # 性能调优
            - --scan-interval=10s
            - --max-empty-bulk-delete=10
            - --max-graceful-termination-sec=600
          resources:
            requests:
              cpu: 100m
              memory: 300Mi
            limits:
              cpu: 200m
              memory: 600Mi
          volumeMounts:
            - name: ssl-certs
              mountPath: /etc/ssl/certs/ca-certificates.crt
              readOnly: true
      volumes:
        - name: ssl-certs
          hostPath:
            path: /etc/ssl/certs/ca-certificates.crt

---
# RBAC 配置
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cluster-autoscaler
  namespace: kube-system

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-autoscaler
rules:
  - apiGroups: [""]
    resources: ["events", "endpoints"]
    verbs: ["create", "patch"]
  - apiGroups: [""]
    resources: ["pods/eviction"]
    verbs: ["create"]
  - apiGroups: [""]
    resources: ["pods/status"]
    verbs: ["update"]
  - apiGroups: [""]
    resources: ["endpoints"]
    resourceNames: ["cluster-autoscaler"]
    verbs: ["get", "update"]
  - apiGroups: [""]
    resources: ["nodes"]
    verbs: ["watch", "list", "get", "update"]
  - apiGroups: [""]
    resources: ["namespaces", "pods", "services", "replicationcontrollers", "persistentvolumeclaims", "persistentvolumes"]
    verbs: ["watch", "list", "get"]
  - apiGroups: ["extensions", "apps"]
    resources: ["daemonsets", "replicasets", "statefulsets"]
    verbs: ["watch", "list", "get"]
  - apiGroups: ["policy"]
    resources: ["poddisruptionbudgets"]
    verbs: ["watch", "list"]
  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses", "csinodes", "csidrivers", "csistoragecapacities"]
    verbs: ["watch", "list", "get"]
  - apiGroups: ["batch"]
    resources: ["jobs", "cronjobs"]
    verbs: ["watch", "list", "get"]
  - apiGroups: ["coordination.k8s.io"]
    resources: ["leases"]
    verbs: ["create"]
  - apiGroups: ["coordination.k8s.io"]
    resourceNames: ["cluster-autoscaler"]
    resources: ["leases"]
    verbs: ["get", "update"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-autoscaler
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-autoscaler
subjects:
  - kind: ServiceAccount
    name: cluster-autoscaler
    namespace: kube-system
```

### VPA (Vertical Pod Autoscaler) 配置

```yaml
# vpa-config.yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
  namespace: default
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: Auto              # Off, Initial, Recreate, Auto
  resourcePolicy:
    containerPolicies:
      - containerName: '*'
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: 4
          memory: 8Gi
        controlledResources: ["cpu", "memory"]
        controlledValues: RequestsAndLimits

---
# VPA 推荐器配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: vpa-recommender-config
  namespace: kube-system
data:
  config.yaml: |
    recommender:
      # CPU 推荐配置
      cpuHistogramDecayHalfLife: 24h
      cpuPercentile: 90
      # 内存推荐配置  
      memoryHistogramDecayHalfLife: 24h
      memoryPercentile: 90
      # 安全边际
      safetyMarginFraction: 0.15
      # 最小变更阈值
      podMinCPUMillicores: 25
      podMinMemoryMb: 250
```

## 容量监控

### 监控指标体系

```yaml
# capacity-monitoring-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: capacity-monitoring-rules
  namespace: monitoring
spec:
  groups:
    # =================================================================
    # 集群容量告警
    # =================================================================
    - name: cluster.capacity.alerts
      interval: 1m
      rules:
        # CPU 容量告警
        - alert: ClusterCPUCapacityLow
          expr: |
            (
              sum(kube_node_status_allocatable{resource="cpu"}) - 
              sum(kube_pod_container_resource_requests{resource="cpu"})
            ) / sum(kube_node_status_allocatable{resource="cpu"}) < 0.2
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "集群 CPU 可用容量低于 20%"
            description: "集群 CPU 可分配资源剩余 {{ $value | humanizePercentage }}"
            
        - alert: ClusterCPUCapacityCritical
          expr: |
            (
              sum(kube_node_status_allocatable{resource="cpu"}) - 
              sum(kube_pod_container_resource_requests{resource="cpu"})
            ) / sum(kube_node_status_allocatable{resource="cpu"}) < 0.1
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "集群 CPU 可用容量严重不足 (<10%)"
            description: "集群 CPU 可分配资源剩余 {{ $value | humanizePercentage }}"
            
        # 内存容量告警
        - alert: ClusterMemoryCapacityLow
          expr: |
            (
              sum(kube_node_status_allocatable{resource="memory"}) - 
              sum(kube_pod_container_resource_requests{resource="memory"})
            ) / sum(kube_node_status_allocatable{resource="memory"}) < 0.2
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "集群内存可用容量低于 20%"
            description: "集群内存可分配资源剩余 {{ $value | humanizePercentage }}"
            
        # Pod 容量告警
        - alert: ClusterPodCapacityLow
          expr: |
            (
              sum(kube_node_status_allocatable{resource="pods"}) - 
              sum(kubelet_running_pods)
            ) / sum(kube_node_status_allocatable{resource="pods"}) < 0.2
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "集群 Pod 容量低于 20%"
            description: "集群可调度 Pod 数剩余 {{ $value | humanizePercentage }}"
            
    # =================================================================
    # 节点容量告警
    # =================================================================
    - name: node.capacity.alerts
      interval: 30s
      rules:
        - alert: NodeCPURequestsHigh
          expr: |
            sum(kube_pod_container_resource_requests{resource="cpu"}) by (node) /
            sum(kube_node_status_allocatable{resource="cpu"}) by (node) > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 CPU 请求使用率高"
            description: "节点 {{ $labels.node }} 的 CPU 请求使用率为 {{ $value | humanizePercentage }}"
            
        - alert: NodeMemoryRequestsHigh
          expr: |
            sum(kube_pod_container_resource_requests{resource="memory"}) by (node) /
            sum(kube_node_status_allocatable{resource="memory"}) by (node) > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点内存请求使用率高"
            description: "节点 {{ $labels.node }} 的内存请求使用率为 {{ $value | humanizePercentage }}"
            
        - alert: NodePodCountHigh
          expr: |
            kubelet_running_pods / 
            kube_node_status_allocatable{resource="pods"} > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 Pod 数量接近上限"
            description: "节点 {{ $labels.node }} 的 Pod 使用率为 {{ $value | humanizePercentage }}"
            
    # =================================================================
    # 资源使用率 vs 请求告警
    # =================================================================
    - name: resource.efficiency.alerts
      interval: 1m
      rules:
        - alert: CPUOvercommitted
          expr: |
            sum(kube_pod_container_resource_limits{resource="cpu"}) by (namespace) /
            sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace) > 3
          for: 30m
          labels:
            severity: info
          annotations:
            summary: "命名空间 CPU 超卖率过高"
            description: "命名空间 {{ $labels.namespace }} 的 CPU 超卖率为 {{ $value }}"
            
        - alert: MemoryOvercommitted
          expr: |
            sum(kube_pod_container_resource_limits{resource="memory"}) by (namespace) /
            sum(kube_pod_container_resource_requests{resource="memory"}) by (namespace) > 2
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "命名空间内存超卖率过高"
            description: "命名空间 {{ $labels.namespace }} 的内存超卖率为 {{ $value }}"
            
        - alert: LowCPUUtilization
          expr: |
            sum(rate(container_cpu_usage_seconds_total{container!=""}[5m])) by (namespace) /
            sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace) < 0.2
          for: 1h
          labels:
            severity: info
          annotations:
            summary: "命名空间 CPU 利用率低"
            description: "命名空间 {{ $labels.namespace }} 的实际 CPU 利用率仅为请求的 {{ $value | humanizePercentage }}"
            
    # =================================================================
    # 容量记录规则
    # =================================================================
    - name: capacity.recording
      interval: 30s
      rules:
        # 集群级别容量指标
        - record: cluster:capacity:cpu_allocatable
          expr: sum(kube_node_status_allocatable{resource="cpu"})
          
        - record: cluster:capacity:cpu_requests
          expr: sum(kube_pod_container_resource_requests{resource="cpu"})
          
        - record: cluster:capacity:cpu_available
          expr: |
            sum(kube_node_status_allocatable{resource="cpu"}) - 
            sum(kube_pod_container_resource_requests{resource="cpu"})
            
        - record: cluster:capacity:cpu_utilization
          expr: |
            sum(kube_pod_container_resource_requests{resource="cpu"}) / 
            sum(kube_node_status_allocatable{resource="cpu"})
            
        - record: cluster:capacity:memory_allocatable
          expr: sum(kube_node_status_allocatable{resource="memory"})
          
        - record: cluster:capacity:memory_requests
          expr: sum(kube_pod_container_resource_requests{resource="memory"})
          
        - record: cluster:capacity:memory_available
          expr: |
            sum(kube_node_status_allocatable{resource="memory"}) - 
            sum(kube_pod_container_resource_requests{resource="memory"})
            
        - record: cluster:capacity:pod_allocatable
          expr: sum(kube_node_status_allocatable{resource="pods"})
          
        - record: cluster:capacity:pod_running
          expr: sum(kubelet_running_pods)
          
        # 节点级别容量指标
        - record: node:capacity:cpu_utilization
          expr: |
            sum(kube_pod_container_resource_requests{resource="cpu"}) by (node) /
            sum(kube_node_status_allocatable{resource="cpu"}) by (node)
            
        - record: node:capacity:memory_utilization
          expr: |
            sum(kube_pod_container_resource_requests{resource="memory"}) by (node) /
            sum(kube_node_status_allocatable{resource="memory"}) by (node)
            
        - record: node:capacity:pod_utilization
          expr: |
            kubelet_running_pods / 
            kube_node_status_allocatable{resource="pods"}
```

### 容量诊断脚本

```bash
#!/bin/bash
# capacity-diagnostics.sh
# 集群容量诊断脚本

set -e

echo "=========================================="
echo "       Kubernetes 容量诊断报告"
echo "=========================================="
echo "时间: $(date)"
echo ""

echo "=== 1. 集群节点概览 ==="
kubectl get nodes -o wide
echo ""

echo "=== 2. 节点资源容量 ==="
echo "节点               CPU容量    CPU可分配   内存容量    内存可分配   Pod容量"
echo "-------------------------------------------------------------------"
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
CPU_CAPACITY:.status.capacity.cpu,\
CPU_ALLOCATABLE:.status.allocatable.cpu,\
MEM_CAPACITY:.status.capacity.memory,\
MEM_ALLOCATABLE:.status.allocatable.memory,\
PODS:.status.allocatable.pods
echo ""

echo "=== 3. 集群总容量汇总 ==="
echo "--- CPU ---"
TOTAL_CPU=$(kubectl get nodes -o jsonpath='{.items[*].status.allocatable.cpu}' | tr ' ' '\n' | awk '{s+=$1}END{print s}')
REQUESTED_CPU=$(kubectl get pods --all-namespaces -o jsonpath='{.items[*].spec.containers[*].resources.requests.cpu}' 2>/dev/null | tr ' ' '\n' | grep -v '^$' | awk '{s+=$1}END{print s}' || echo "0")
echo "总可分配 CPU: ${TOTAL_CPU}m"
echo "已请求 CPU: ${REQUESTED_CPU}m"
echo ""

echo "--- 内存 ---"
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.memory}{"\n"}{end}'
echo ""

echo "--- Pod 数量 ---"
TOTAL_PODS=$(kubectl get nodes -o jsonpath='{.items[*].status.allocatable.pods}' | tr ' ' '\n' | awk '{s+=$1}END{print s}')
RUNNING_PODS=$(kubectl get pods --all-namespaces --field-selector=status.phase=Running -o name | wc -l)
echo "总可调度 Pod: $TOTAL_PODS"
echo "运行中 Pod: $RUNNING_PODS"
echo "剩余容量: $((TOTAL_PODS - RUNNING_PODS))"
echo ""

echo "=== 4. 各节点 Pod 分布 ==="
kubectl get pods --all-namespaces -o wide --field-selector=status.phase=Running | \
    awk 'NR>1{pods[$8]++}END{for(n in pods)print n": "pods[n]" pods"}' | sort
echo ""

echo "=== 5. 各命名空间资源使用 ==="
echo "Namespace             CPU Requests    Memory Requests    Pod Count"
echo "-------------------------------------------------------------------"
for ns in $(kubectl get namespaces -o jsonpath='{.items[*].metadata.name}'); do
    CPU=$(kubectl get pods -n $ns -o jsonpath='{.items[*].spec.containers[*].resources.requests.cpu}' 2>/dev/null | tr ' ' '\n' | grep -v '^$' | awk '{s+=$1}END{print s+0}')
    MEM=$(kubectl get pods -n $ns -o jsonpath='{.items[*].spec.containers[*].resources.requests.memory}' 2>/dev/null | tr ' ' '\n' | grep -v '^$' | head -1)
    PODS=$(kubectl get pods -n $ns --field-selector=status.phase=Running -o name 2>/dev/null | wc -l)
    printf "%-20s %15s %18s %12s\n" "$ns" "${CPU}m" "${MEM:-0}" "$PODS"
done
echo ""

echo "=== 6. 节点条件状态 ==="
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
READY:.status.conditions[?@.type==\"Ready\"].status,\
MEM_PRESSURE:.status.conditions[?@.type==\"MemoryPressure\"].status,\
DISK_PRESSURE:.status.conditions[?@.type==\"DiskPressure\"].status,\
PID_PRESSURE:.status.conditions[?@.type==\"PIDPressure\"].status
echo ""

echo "=== 7. Pending Pod 检查 ==="
PENDING_PODS=$(kubectl get pods --all-namespaces --field-selector=status.phase=Pending -o name | wc -l)
echo "Pending Pod 数量: $PENDING_PODS"
if [ "$PENDING_PODS" -gt 0 ]; then
    echo ""
    echo "Pending Pod 详情:"
    kubectl get pods --all-namespaces --field-selector=status.phase=Pending \
        -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,REASON:.status.conditions[0].reason
fi
echo ""

echo "=== 8. 资源配额状态 ==="
kubectl get resourcequotas --all-namespaces -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
CPU_USED:.status.used.requests\\.cpu,\
CPU_HARD:.status.hard.requests\\.cpu,\
MEM_USED:.status.used.requests\\.memory,\
MEM_HARD:.status.hard.requests\\.memory 2>/dev/null || echo "未配置资源配额"
echo ""

echo "=== 9. 节点池分布 (如有标签) ==="
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
POOL:.metadata.labels.node\\.kubernetes\\.io/pool,\
ZONE:.metadata.labels.topology\\.kubernetes\\.io/zone,\
INSTANCE_TYPE:.metadata.labels.node\\.kubernetes\\.io/instance-type 2>/dev/null || echo "未配置节点池标签"
echo ""

echo "=== 10. 容量规划建议 ==="

# CPU 使用率检查
CPU_UTILIZATION=$(echo "scale=2; $REQUESTED_CPU / $TOTAL_CPU * 100" | bc 2>/dev/null || echo "N/A")
echo "当前 CPU 请求利用率: ${CPU_UTILIZATION}%"

# Pod 使用率检查
POD_UTILIZATION=$(echo "scale=2; $RUNNING_PODS / $TOTAL_PODS * 100" | bc 2>/dev/null || echo "N/A")
echo "当前 Pod 容量利用率: ${POD_UTILIZATION}%"

echo ""
if [ "$(echo "$CPU_UTILIZATION > 80" | bc 2>/dev/null)" -eq 1 ]; then
    echo "[警告] CPU 容量利用率超过 80%,建议扩容"
fi
if [ "$(echo "$POD_UTILIZATION > 80" | bc 2>/dev/null)" -eq 1 ]; then
    echo "[警告] Pod 容量利用率超过 80%,建议增加节点"
fi
if [ "$PENDING_PODS" -gt 0 ]; then
    echo "[警告] 存在 Pending Pod,请检查资源容量或调度约束"
fi

echo ""
echo "=========================================="
echo "       诊断报告结束"
echo "=========================================="
```

### 容量报告模板

```yaml
# capacity-report-template.yaml
# 容量规划报告模板

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: capacity-report-template
  namespace: monitoring
data:
  report-template.md: |
    # Kubernetes 集群容量规划报告
    
    ## 报告信息
    - 生成时间: {{ .GeneratedAt }}
    - 集群名称: {{ .ClusterName }}
    - 报告周期: {{ .ReportPeriod }}
    
    ## 1. 执行摘要
    
    | 指标 | 当前值 | 阈值 | 状态 |
    |-----|-------|------|------|
    | CPU 容量利用率 | {{ .CPUUtilization }}% | 80% | {{ if gt .CPUUtilization 80 }}⚠️{{ else }}✅{{ end }} |
    | 内存容量利用率 | {{ .MemoryUtilization }}% | 80% | {{ if gt .MemoryUtilization 80 }}⚠️{{ else }}✅{{ end }} |
    | Pod 容量利用率 | {{ .PodUtilization }}% | 80% | {{ if gt .PodUtilization 80 }}⚠️{{ else }}✅{{ end }} |
    | 节点健康率 | {{ .NodeHealthRate }}% | 100% | {{ if lt .NodeHealthRate 100 }}⚠️{{ else }}✅{{ end }} |
    
    ## 2. 集群资源概览
    
    ### 2.1 节点分布
    | 节点池 | 节点数 | 规格 | CPU 总量 | 内存总量 |
    |-------|-------|------|---------|---------|
    {{ range .NodePools }}
    | {{ .Name }} | {{ .NodeCount }} | {{ .InstanceType }} | {{ .TotalCPU }} | {{ .TotalMemory }} |
    {{ end }}
    
    ### 2.2 资源容量
    | 资源类型 | 总容量 | 已分配 | 可用 | 利用率 |
    |---------|-------|-------|------|-------|
    | CPU | {{ .TotalCPU }} | {{ .AllocatedCPU }} | {{ .AvailableCPU }} | {{ .CPUUtilization }}% |
    | 内存 | {{ .TotalMemory }} | {{ .AllocatedMemory }} | {{ .AvailableMemory }} | {{ .MemoryUtilization }}% |
    | Pod | {{ .TotalPods }} | {{ .RunningPods }} | {{ .AvailablePods }} | {{ .PodUtilization }}% |
    
    ## 3. 趋势分析
    
    ### 3.1 资源增长趋势 (过去 30 天)
    - CPU 请求增长: {{ .CPUGrowthRate }}%
    - 内存请求增长: {{ .MemoryGrowthRate }}%
    - Pod 数量增长: {{ .PodGrowthRate }}%
    
    ### 3.2 预测 (下一季度)
    基于当前增长率,预计下一季度资源需求:
    - CPU: {{ .ProjectedCPU }}
    - 内存: {{ .ProjectedMemory }}
    - Pod 数: {{ .ProjectedPods }}
    
    ## 4. 建议行动
    
    {{ range .Recommendations }}
    ### {{ .Priority }}: {{ .Title }}
    - **影响**: {{ .Impact }}
    - **建议**: {{ .Description }}
    - **时间窗口**: {{ .Timeline }}
    {{ end }}
    
    ## 5. 成本分析
    
    | 资源类型 | 当前成本 | 优化后预估 | 节省 |
    |---------|---------|-----------|------|
    | 计算资源 | {{ .CurrentComputeCost }} | {{ .OptimizedComputeCost }} | {{ .ComputeSavings }} |
    | 存储资源 | {{ .CurrentStorageCost }} | {{ .OptimizedStorageCost }} | {{ .StorageSavings }} |
    | 网络资源 | {{ .CurrentNetworkCost }} | {{ .OptimizedNetworkCost }} | {{ .NetworkSavings }} |
    
    ---
    *报告由 Kubernetes 容量规划系统自动生成*
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **v1.31** | 调度器吞吐量优化 | 支持更大规模集群 |
| | 新增 `NodeResourcesFitArgs` 评分调优 | 更精细的资源匹配 |
| **v1.30** | etcd 3.5.x 默认版本 | 性能提升 ~50% |
| | 新增 `DRA` (Dynamic Resource Allocation) | GPU 等资源更灵活分配 |
| **v1.29** | Cluster Autoscaler 性能优化 | 大规模集群扩缩容更快 |
| | 新增 `--max-pod-eviction-time` | 更可控的节点缩容 |
| **v1.28** | 引入 `SchedulerQueueingHints` | 调度性能提升 |
| | Kube Reserved 动态调整 GA | 更智能的资源预留 |

## 最佳实践总结

### 容量规划检查清单

- [ ] 定义工作负载资源需求基线
- [ ] 设置合理的资源预留 (kube-reserved, system-reserved)
- [ ] 配置驱逐阈值防止 OOM
- [ ] 按工作负载类型划分节点池
- [ ] 配置集群自动伸缩
- [ ] 建立容量监控告警
- [ ] 定期进行容量规划审查
- [ ] 制定容量扩展预案

### 关键监控指标

- `cluster:capacity:cpu_utilization` - 集群 CPU 利用率
- `cluster:capacity:memory_utilization` - 集群内存利用率
- `cluster:capacity:pod_utilization` - 集群 Pod 利用率
- `node:capacity:*` - 节点级容量指标
- `kube_node_status_condition` - 节点条件状态
- `kube_pod_status_phase{phase="Pending"}` - Pending Pod 数量

---

**参考资料**:
- [Kubernetes 大规模集群考量](https://kubernetes.io/docs/setup/best-practices/cluster-large/)
- [节点资源预留](https://kubernetes.io/docs/tasks/administer-cluster/reserve-compute-resources/)
- [Cluster Autoscaler](https://github.com/kubernetes/autoscaler/tree/master/cluster-autoscaler)
