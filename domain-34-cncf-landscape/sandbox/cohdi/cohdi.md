# CoHDI (Composable Hyperconverged Disaggregated Infrastructure)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/CoHDI/composable-resource-operator |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

CoHDI（Composable Hyperconverged Disaggregated Infrastructure）是一个 Kubernetes Operator，用于在分解式基础设施中动态组合和管理硬件资源。它支持通过 CXL（Compute Express Link）和 PCIe 总线动态地将远端 GPU、内存、存储等设备组合分配给 Kubernetes Pod，使得计算节点可以按需接入共享资源池中的加速器设备，提升硬件利用率并降低成本。

### 核心特性

- **动态资源组合**: 通过 CXL/PCIe Fabric 动态将远端 GPU/内存附加到计算节点
- **Kubernetes 原生**: 通过 CRD 和 Operator 模式管理分解式资源
- **资源池化**: 将 GPU、FPGA、内存等设备组织为共享资源池
- **拓扑感知调度**: 基于硬件拓扑优化资源分配
- **热插拔支持**: 支持在不中断服务的情况下动态分配和回收设备
- **多厂商支持**: 支持多种硬件厂商的 CXL/PCIe 交换设备

---

## 架构设计

```
┌─────────────────────────────────────────────────┐
│            Kubernetes Control Plane               │
│                                                   │
│  ┌────────────────────────────────────────┐      │
│  │     CoHDI Resource Operator            │      │
│  │  ┌──────────┐  ┌──────────────────┐   │      │
│  │  │ Resource  │  │ Topology-Aware   │   │      │
│  │  │ Manager   │  │ Scheduler Plugin │   │      │
│  │  └────┬─────┘  └────────┬─────────┘   │      │
│  └───────┼──────────────────┼─────────────┘      │
└──────────┼──────────────────┼─────────────────────┘
           │                  │
┌──────────▼──────────────────▼─────────────────────┐
│              CXL / PCIe Fabric                      │
│  ┌──────────────────────────────────────────┐      │
│  │          Composable Switch                │      │
│  │  (动态连接计算节点与资源设备)               │      │
│  └──────────────────────────────────────────┘      │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │ Compute  │  │ GPU Pool │  │ Memory   │         │
│  │ Node 1   │  │ GPU 0..N │  │ Pool     │         │
│  │          │◄─┤          │  │ CXL Mem  │         │
│  └──────────┘  └──────────┘  └──────────┘         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │ Compute  │  │ FPGA Pool│  │ Storage  │         │
│  │ Node 2   │◄─┤          │  │ Pool     │         │
│  │          │  │          │  │ NVMe-oF  │         │
│  └──────────┘  └──────────┘  └──────────┘         │
└─────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Operator

```bash
# 安装 CoHDI Operator
kubectl apply -f https://github.com/CoHDI/composable-resource-operator/releases/latest/download/install.yaml

# 或使用 Helm
helm install cohdi cohdi/composable-resource-operator \
  --namespace cohdi-system \
  --create-namespace
```

### 定义资源池

```yaml
apiVersion: cohdi.io/v1alpha1
kind: ComposableResourcePool
metadata:
  name: gpu-pool
spec:
  resourceType: gpu
  fabric: cxl
  devices:
    - id: gpu-0
      vendor: nvidia
      model: A100
      memory: 80Gi
    - id: gpu-1
      vendor: nvidia
      model: A100
      memory: 80Gi
    - id: gpu-2
      vendor: nvidia
      model: A100
      memory: 80Gi
  switchConfig:
    endpoint: fabric-switch.local:8443
    credentials:
      secretRef: fabric-switch-creds
```

### 请求组合资源

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ml-training
  annotations:
    cohdi.io/composable-resources: "true"
spec:
  containers:
    - name: trainer
      image: pytorch/pytorch:latest
      resources:
        limits:
          cohdi.io/composable-gpu: 2    # 动态组合 2 块远端 GPU
          cohdi.io/composable-memory: 128Gi  # 组合 CXL 内存
      command: ["python", "train.py"]
```

---

## 使用场景

### AI/ML 训练

```yaml
apiVersion: cohdi.io/v1alpha1
kind: ComposableResourceClaim
metadata:
  name: training-gpus
spec:
  resourceType: gpu
  count: 4
  requirements:
    minMemory: 40Gi
    vendor: nvidia
    interconnect: nvlink  # 优先 NVLink 连接的 GPU
  affinity:
    preferSameSwitch: true  # 优先同一交换机下的 GPU
```

### 弹性推理

```yaml
apiVersion: cohdi.io/v1alpha1
kind: ComposableResourcePolicy
metadata:
  name: inference-scaling
spec:
  targetPool: gpu-pool
  scaling:
    minDevices: 1
    maxDevices: 8
    metrics:
      - type: utilization
        threshold: 80
        action: compose      # GPU 利用率 >80% 时增加 GPU
      - type: utilization
        threshold: 20
        action: decompose    # GPU 利用率 <20% 时回收 GPU
```

---

## 与其他方案对比

| 特性 | CoHDI | NVIDIA GPU Operator | HAMi | 传统分配 |
|:---|:---|:---|:---|:---|
| 资源来源 | 远端池化设备 | 本地设备 | 本地设备 | 本地设备 |
| 动态组合 | CXL/PCIe Fabric | 不支持 | 不支持 | 不支持 |
| GPU 共享 | 设备级 | MIG/MPS | 虚拟化共享 | 不支持 |
| 硬件要求 | CXL 交换机 | 标准服务器 | 标准服务器 | 标准服务器 |
| 利用率提升 | 极高 (池化) | 中 | 高 | 低 |
| 适用场景 | 数据中心级 | 通用 K8s | 通用 K8s | 小规模 |

---

## 最佳实践

1. **硬件规划**: 确保网络结构支持 CXL/PCIe Fabric，合理规划交换机拓扑
2. **资源池分级**: 按性能等级划分资源池，高优先级任务使用高性能设备
3. **亲和性策略**: 为延迟敏感任务配置拓扑亲和性，减少跨交换机访问
4. **容量规划**: 监控资源池利用率，及时扩展物理设备
5. **故障隔离**: 配置设备健康检查，自动隔离故障设备

---

## 参考资源

- [CoHDI GitHub](https://github.com/CoHDI/composable-resource-operator)
- [CXL Consortium](https://www.computeexpresslink.org/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
