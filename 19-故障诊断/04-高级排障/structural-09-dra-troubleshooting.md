---
title: DRA（动态资源分配）故障排查指南
description: '# DRA（动态资源分配）故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- kubelet
- scheduler
- containerd
- operator
- gpu
- cuda
- nvidia
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- DRA（动态资源分配）故障排查指南 是什么
- 如何 DRA（动态资源分配）故障排查指南
- DRA（动态资源分配）故障排查指南 故障排查
- DRA（动态资源分配）故障排查指南 排障步骤
trigger_keywords:
- DRA
- 动态资源分配
- 故障排查指南
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# DRA（动态资源分配）故障排查指南

> **文档类型**: 故障排查手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 处理 DRA（GPU/FPGA/网络设备）相关的问题，如 Pod 卡在 Allocating、设备不可见

---

## 1. DRA 概述与核心概念

### 1.1 DRA vs 传统 Volume 的区别

| 维度 | 传统 Volume | DRA (Dynamic Resource Allocation) |
|------|------------|--------------------------------|
| 资源类型 | 存储（块/文件） | 硬件设备（GPU/FPGA/智能网卡） |
| 调度时机 | Pod 创建后 | Pod 创建前（调度时即分配） |
| 资源选择 | StorageClass | ResourceClaim + ClaimTemplate |
| 生命周期 | PVC 独立于 Pod | 设备随 Pod 一起分配/释放 |
| K8s 版本 | 始终支持 | 1.28 Beta, 1.30 GA |

### 1.2 DRA 核心资源

```
ResourceClass (集群级)
    ↓ 管理员创建
ResourceClaim (命名空间级，Pod 引用)
    ↓ 由 ClaimTemplate 生成或直接创建
DevicePlugin (节点级，kubelet 注册)
    ↓ 提供设备
DeviceSelection (Claim 中选择的设备)
```

### 1.3 DRA 流程

```
1. 用户创建 ResourceClaim / ClaimTemplate
2. Pod spec 引用 ResourceClaim（如 resourceClaims[].name）
3. 调度器在调度时查询 DRA 司机，计算可用设备
4. 设备分配（调度时完成，而非创建后）
5. kubelet 将设备映射到容器
6. Pod 运行中使用设备
7. Pod 终止后设备自动释放
```

---

## 2. 常见问题场景

### 2.1 Pod 卡在 Allocating 状态

**问题现象**: `kubectl get pods` 显示 Pod 状态正常但 `resourceClaims` 中有 claim 处于 `Allocating` 状态

**可能原因**：

| 原因 | 诊断方法 | 解决方案 |
|------|---------|---------|
| 节点上没有对应的 DevicePlugin | `kubectl get node <node-name> -o jsonpath='{.status.capacity}'` | 确认节点打了正确标签 |
| 设备数量不足（申请数 > 可用数） | `kubectl get device -o yaml` | 减少 Pod 副本数或等待设备释放 |
| DevicePlugin 未注册到 kubelet | `kubectl logs -n kube-system <device-plugin-pod>` | 重启 DevicePlugin |
| 调度器 DRA 司机不工作 | `kubectl describe pod <pod-name>` Events | 检查调度器日志 |

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pod 的 ResourceClaim 状态
kubectl get pod <pod-name> -o jsonpath='{.status.resourceClaimStatuses}'
# 期望输出包含 "Allocated" 而非 "Allocating"

# 2. 查看相关 ResourceClaim
kubectl get resourceclaim <claim-name> -o yaml

# 3. 查看 claim 的设备分配结果
kubectl get resourceclaim <claim-name> -o jsonpath='{.status.devices}'

# 4. 查看节点上的设备插件
kubectl get pods -n kube-system | grep -i deviceplugin
kubectl logs -n kube-system <device-plugin-pod> --tail=50

# 5. 在节点上检查设备
nvidia-smi  # NVIDIA GPU
lspci | grep -i gpu  # 所有 GPU
ls /dev/  # 设备节点
```
### 2.2 设备节点不存在

**问题现象**: Pod 启动后容器内 `/dev/` 目录中看不到预期的设备（如 /dev/nvidia0）

**可能原因**：

| 原因 | 诊断方法 | 解决方案 |
|------|---------|---------|
| kubelet 未正确映射设备到容器 | `kubectl describe pod` 查看 deviceplugin 配置 | 检查 device plugin 配置 |
| 节点标签与 ResourceClass 选择器不匹配 | `kubectl get node <node-name> --show-labels` | 添加正确标签 |
| 容器运行时不支持 DRA 设备发现 | `crictl info | grep -i dr` | 升级 containerd 到支持版本 |

**排查步骤**：
```bash
# 1. 在节点上查看设备
ls -la /dev/nvidia*
lspci | grep -i nvidia

# 2. 查看 kubelet 设备分配日志
journalctl -u kubelet | grep -i dra
journalctl -u kubelet | grep -i deviceplugin

# 3. 检查 containerd 配置中是否启用了 DRA
cat /etc/containerd/config.toml | grep -i dr

# 4. 查看 device plugin 端点
curl -s http://localhost:54356/apis/pluginregistry.k8s.io/v1/deviceplugins
```

### 2.3 ResourceClaim 配置错误

**问题现象**: `kubectl describe resourceclaim` 报错 "no suitable node found" 或 "no devices available"

**可能原因**：

| 原因 | 诊断方法 | 解决方案 |
|------|---------|---------|
| ResourceClass 不存在 | `kubectl get resourceclass` | 创建 ResourceClass |
| 选择器无法匹配任何节点 | 检查 node selectors / topology | 修正选择条件 |
| 设备供应商的 plugin 未安装 | 检查 device plugin pod | 安装对应供应商的 device plugin |

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 ResourceClass 定义
kubectl get resourceclass <class-name> -o yaml

# 2. 查看所有 ResourceClaim
kubectl get resourceclaim -A

# 3. 查看节点标签（确认设备插件发现的节点）
kubectl get nodes --show-labels | grep -E "nvidia|gpu|fpga"

# 4. 查看 ResourceClaim 的 conditions
kubectl describe resourceclaim <claim-name>
```
---

## 3. 调度器 DRA 司机问题

### 3.1 调度器未正确加载 DRA 司机

**问题现象**: Pod 创建时错误 "scheduler drone plugin not enabled"

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看调度器配置
kubectl get configmap kube-scheduler -n kube-system -o yaml

# 2. 检查调度器是否启用了 DRA
grep -i dra /[[23-实体/02-K8s核心组件/kubernetes|kubernetes]]/manifests/kube-scheduler.yaml

# 3. 查看调度器日志
kubectl logs -n kube-system kube-scheduler-<node-name> --tail=100 | grep -i dra

# 4. 启用 DRA 调度（如需要，在调度器配置中添加）
# 编辑 /etc/kubernetes/manifests/kube-scheduler.yaml
# 在 --feature-gates 中添加: DRAPlugin=true
```
### 3.2 拓扑约束导致调度失败

**问题现象**: Pod 卡在 Pending，错误信息包含 "node(s) had no suitable topology"

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 ResourceClaim 的拓扑要求
kubectl get resourceclaim <claim-name> -o yaml | grep -i topology

# 2. 查看节点标签
kubectl get nodes --show-labels | grep -E "topology.kubernetes.io/zone"

# 3. 检查节点的 zone 标签与 claim 要求是否匹配
kubectl get node <node-name> -o jsonpath='{.metadata.labels}' | jq 'keys'
```
---

## 4. DevicePlugin 问题

### 4.1 常见 DevicePlugin 列表

| 设备类型 | DevicePlugin | 厂商 | 说明 |
|---------|-------------|------|------|
| NVIDIA GPU | nvidia-device-plugin | NVIDIA | K8s 官方插件 |
| AMD GPU | amdgpu-device-plugin | AMD | AMD GPU 支持 |
| Intel FPGA | intel-fpga-device-plugin | Intel | FPGA 支持 |
| Intel GPU | intel-gpu-device-plugin | Intel | Intel 集成显卡 |
| RDMA | kubernetes-device-plugin | RDMA | 网络加速 |
| Custom | 厂商提供或自研 | - | 按厂商文档 |

### 4.2 NVIDIA DevicePlugin 问题

**问题现象**: GPU Pod 卡在 ContainerCreating 或 `nvidia-smi` 在容器内不可用

**排查步骤**：
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认节点有 NVIDIA GPU
lspci | grep -i nvidia

# 2. 确认 NVIDIA 驱动已安装
nvidia-smi

# 3. 确认 device plugin pod 运行正常
kubectl get pods -n kube-system | grep nvidia-device-plugin

# 4. 查看 device plugin 日志
kubectl logs -n kube-system nvidia-device-plugin-xxx --tail=50

# 5. 确认节点打了 GPU 标签
kubectl get nodes -l nvidia.com/gpu=true

# 6. 在容器内测试
kubectl exec -it <pod-name> -- nvidia-smi
```
### 4.3 DevicePlugin 注册失败

**问题现象**: `kubectl logs` 显示 "failed to register device plugin: socket path already exists"

**排查步骤**：
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
# 1. 查看 kubelet 的 device plugin 目录
ls -la /var/lib/kubelet/device-plugins/

# 2. 删除旧的 socket 文件（谨慎！）
rm /var/lib/kubelet/device-plugins/kubelet.sock

# 3. 重启 kubelet
systemctl restart kubelet

# 4. 重启 device plugin
kubectl delete pod -n kube-system <device-plugin-pod>
```
---

## 5. DRA 与调度器集成问题

### 5.1 Pod 调度时设备分配流程

```
Pod 调度请求
  → 调度器遍历节点
  → 对每个节点调用 DRA 司机
  → DRA 司机查询 ResourceClaim 要求
  → DRA 司机向节点的 DevicePlugin 请求设备
  → DevicePlugin 返回可用设备列表
  → DRA 司机选择设备并预留
  → 调度器选择最优节点
  → 节点上 kubelet 在 Pod 启动时映射设备
```

### 5.2 调度失败的常见原因

| 原因 | 错误信息 | 解决方案 |
|------|---------|---------|
| 无节点有设备 | "0 nodes available with available resources" | 添加设备节点或检查 device plugin |
| 设备数量不足 | " insufficient nvidia.com/gpu" | 等待其他 Pod 释放设备或扩容 |
| 拓扑约束冲突 | "node(s) had no suitable topology" | 检查 zone/region 标签 |
| DRA 司机未加载 | "no drone plugin registered" | 检查调度器 feature gate |

---

## 6. 验证 DRA 配置

### 6.1 快速验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 DRA 功能是否启用（K8s 1.30+ 默认开启）
kubectl get featuregate DynamicResourceAllocation

# 2. 查看 ResourceClass
kubectl get resourceclass

# 3. 查看所有 ResourceClaim
kubectl get resourceclaim -A

# 4. 查看节点设备容量
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.nvidia\.com/gpu}{"\n"}{end}'

# 5. 查看 pod 的设备分配
kubectl get pod <pod-name> -o jsonpath='{.status.resourceClaimStatuses}'
```
### 6.2 示例 YAML

```yaml
# ResourceClass 示例（NVIDIA GPU）
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClass
metadata:
  name: nvidia-gpu
producerName: nvidia.com/gpu
parameters:
  vendor: nvidia
---
# ResourceClaim 示例
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClaim
metadata:
  name: gpu-claim-1
spec:
  resourceClassName: nvidia-gpu
  selectors:
  - matchExpressions:
    - key: nvidia.com/gpu.product
      operator: In
      values: ["Tesla-V100", "Tesla-A100"]
---
# Pod 引用示例
apiVersion: v1
kind: Pod
metadata:
  name: gpu-pod
spec:
  containers:
  - name: gpu-container
    image: nvidia/cuda:11.0-base
    command: ["nvidia-smi"]
    resources:
      limits:
        nvidia.com/gpu: 1
  resourceClaims:
  - name: gpu-claim-1
    source: resourceclaim/gpu-claim-1
```

---

## 7. 故障排查命令速查

| 问题场景 | 诊断命令 |
|---------|---------|
| Pod 卡在 Allocating | `kubectl get pod <pod> -o jsonpath='{.status.resourceClaimStatuses}'` |
| ResourceClaim 无法分配 | `kubectl describe resourceclaim <claim>` |
| DevicePlugin 不健康 | `kubectl get pods -n kube-system | grep deviceplugin` |
| 节点无设备 | `kubectl get nodes -o jsonpath='{.items[*].status.capacity}'` |
| 调度器 DRA 问题 | `kubectl logs -n kube-system kube-scheduler-xxx --tail=100 | grep -i dra` |
| 容器内无设备 | `kubectl exec <pod> -- ls /dev/ | grep nvidia` |
| 检查 GPU 标签 | `kubectl get nodes -l nvidia.com/gpu=true` |

---

```yaml
---
id: DRA-TROUBLESHOOTING-001
domain: troubleshooting
type: troubleshooting-guide
tags: [dra, dynamic-resource-allocation, gpu, device-plugin, k8s-1.28-1.33, agent-corpus]
intent_queries:
  - "DRA Pod 卡在 Allocating 怎么排查"
  - "GPU 设备在容器内不可见"
  - "ResourceClaim 分配失败怎么解决"
  - "K8s 1.28 DRA Beta 怎么使用"
  - "DevicePlugin 注册失败"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - 故障诊断/34-upgrade-migration-troubleshooting.md
  - 故障诊断/FTA故障树/list/gpu-fta.md
  - 集群基础/30-dynamic-resource-allocation.md
---
```

<!-- risk-assessed -->
