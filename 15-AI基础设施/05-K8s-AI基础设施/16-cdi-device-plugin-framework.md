---
title: "CDI 与 Device Plugin 框架"
description: "K8s 设备管理全栈：Device Plugin gRPC 架构、CDI 规范、NVIDIA CDI 配置、DRA 动态资源分配与设备拓扑感知调度"
summary: "深入解析 Kubernetes 设备管理框架：Device Plugin 生命周期（Registration/ListAndWatch/Allocate）、CDI Container Device Interface 规范与 NVIDIA CDI 配置、自定义 Device Plugin 开发、DRA Dynamic Resource Allocation（K8s 1.26+）、设备拓扑感知调度、设备注册失败与 Allocate 超时故障排查"
category: AI基础设施
tags:
- cdi
- device-plugin
- dra
- nvidia
- gpu
- topology
- kubernetes
- grpc
- resource-allocation
- scheduling
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
- "Device Plugin 的工作原理是什么"
- "CDI 和 Device Plugin 有什么区别"
- "DRA 动态资源分配怎么配置"
trigger_keywords:
- CDI
- Device Plugin
- DRA
- 设备管理
- GPU分配
prerequisites:
- kubectl-basics
- helm-basics
- gpu-scheduling-basics
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

# CDI 与 Device Plugin 框架

## 概述

Kubernetes 原生调度器只理解 CPU 和 Memory 两种资源。要让 Pod 使用 GPU、FPGA、RDMA NIC 等异构硬件设备，需要一套设备管理框架将硬件能力"翻译"为 K8s 可调度的扩展资源。这套框架经历了三代演进：Device Plugin（K8s 1.10+）→ CDI（K8s 1.28+ GA）→ DRA（K8s 1.30+ Beta），每一代都在解决前一代的局限性。

Device Plugin 通过 gRPC 接口向 kubelet 注册设备、上报健康状态、分配设备给容器；CDI（Container Device Interface）将设备注入逻辑从 Device Plugin 中解耦，提供声明式的设备配置规范；DRA（Dynamic Resource Allocation）则引入了类似 PVC/PV 的动态分配模型，支持设备参数协商和延迟绑定。

本文覆盖三代设备管理框架的架构原理、生产配置、自定义开发与故障排查。

## 架构与核心概念

### Device Plugin 架构

Device Plugin 是运行在每个节点上的 DaemonSet，通过 Unix Domain Socket 与 kubelet 通信：

```
┌─────────────────────────────────────────────────────────────┐
│                        Node                                   │
│                                                              │
│  ┌──────────┐    gRPC (UDS)    ┌─────────────────────────┐  │
│  │  kubelet │◄────────────────►│   Device Plugin          │  │
│  │          │                  │   (DaemonSet Pod)        │  │
│  │  Device  │  Registration    │                          │  │
│  │  Manager │─────────────────►│  ┌────────────────────┐  │  │
│  │          │  ListAndWatch    │  │ ListAndWatch()     │  │  │
│  │          │◄─────────────────│  │ → 持续上报设备状态   │  │  │
│  │          │  Allocate        │  └────────────────────┘  │  │
│  │          │─────────────────►│  ┌────────────────────┐  │  │
│  │          │                  │  │ Allocate()         │  │  │
│  └──────────┘                  │  │ → 返回设备配置      │  │  │
│       │                        │  └────────────────────┘  │  │
│       │ 创建容器时注入设备       │  ┌────────────────────┐  │  │
│       ▼                        │  │ GetPreferredAlloc()│  │  │
│  ┌──────────┐                  │  │ → 拓扑感知分配      │  │  │
│  │ Container│                  │  └────────────────────┘  │  │
│  │ Runtime  │                  └─────────────────────────┘  │
│  │(containerd)│                                            │
│  └──────────┘                                              │
│       │                                                     │
│       ▼                                                     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  /dev/nvidia0  /dev/nvidia1  /dev/infiniband/...     │  │
│  │              物理设备                                  │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**Device Plugin gRPC 接口**：

| 方法 | 方向 | 功能 |
|------|------|------|
| `Register()` | Plugin → kubelet | 注册设备类型（如 `nvidia.com/gpu`）和 socket 路径 |
| `ListAndWatch()` | kubelet ← Plugin | 持续流式上报设备列表和健康状态 |
| `Allocate()` | kubelet → Plugin | Pod 调度后，请求设备配置（环境变量、挂载、设备节点） |
| `GetPreferredAllocation()` | kubelet → Plugin | 可选，拓扑感知的设备选择建议 |
| `PreStartContainer()` | kubelet → Plugin | 可选，容器启动前的设备初始化 |

### CDI（Container Device Interface）规范

CDI 是 CNCF 容器运行时规范，定义了设备注入容器的标准格式。CDI 将"设备是什么"和"如何注入容器"分离：

```yaml
# CDI 规范文件示例：/etc/cdi/nvidia.yaml
cdiVersion: "0.7.0"
kind: "nvidia.com/gpu"
devices:
- name: gpu0
  containerEdits:
    deviceNodes:
    - path: /dev/nvidia0
      permissions: rw
    - path: /dev/nvidiactl
      permissions: rw
    - path: /dev/nvidia-uvm
      permissions: rw
    env:
    - NVIDIA_VISIBLE_DEVICES=0
    - NVIDIA_DRIVER_CAPABILITIES=compute,utility
    mounts:
    - hostPath: /usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1
      containerPath: /usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1
      options: ["ro", "nosuid", "nodev", "bind"]
    hooks:
    - hookName: createContainer
      path: /usr/bin/nvidia-cdi-hook
      args: ["create-symlinks", "--links", "libcuda.so.1::/usr/lib/x86_64-linux-gnu/libcuda.so"]
containerEdits:
  env:
  - NVIDIA_DRIVER_CAPABILITIES=compute,utility
  hooks:
  - hookName: createContainer
    path: /usr/bin/nvidia-cdi-hook
    args: ["update-ldcache", "--folder", "/usr/lib/x86_64-linux-gnu"]
```

### DRA（Dynamic Resource Allocation）

DRA 是 K8s 1.26 引入（1.30 Beta）的新一代设备分配机制，核心思想是将设备分配从调度时决策改为运行时协商：

```
┌────────────────────────────────────────────────────────────┐
│  DRA 对象模型                                               │
│                                                             │
│  ResourceClaim (类似 PVC)                                   │
│  ├── 由 Pod 或管理员创建                                     │
│  ├── 声明设备需求（类型、数量、参数）                          │
│  └── 绑定到 ResourceClaimTemplate                           │
│                                                             │
│  ResourceClass (类似 StorageClass)                          │
│  ├── 定义设备驱动和参数模板                                   │
│  └── 由集群管理员创建                                        │
│                                                             │
│  ResourceSlice (类似 PV)                                    │
│  ├── 由 DRA Driver 在节点上发布                              │
│  ├── 描述可用设备及其属性                                     │
│  └── 支持设备属性匹配（如 GPU 型号、显存大小）                 │
│                                                             │
│  分配流程：                                                  │
│  Pod → ResourceClaim → Scheduler 匹配 ResourceSlice         │
│      → DRA Driver NodePrepare → 容器注入设备                  │
└────────────────────────────────────────────────────────────┘
```

## 生产部署

### NVIDIA Device Plugin 部署

🟡 **中风险** — 使用 NVIDIA GPU Operator 部署 Device Plugin（推荐方式）：

```bash
# 部署 NVIDIA GPU Operator（包含 Device Plugin、DCGM Exporter、Driver 等）
helm repo add nvidia https://helm.ngc.nvidia.com/nvidia
helm repo update

helm install gpu-operator nvidia/gpu-operator \
  --namespace gpu-operator \
  --create-namespace \
  --version v24.9.0 \
  --set devicePlugin.enabled=true \
  --set devicePlugin.config.name=device-plugin-config \
  --set dcgmExporter.enabled=true \
  --set gfd.enabled=true \
  --set mig.strategy=mixed \
  --set toolkit.enabled=true \
  --set driver.enabled=false    # 如果节点已预装驱动
```

### NVIDIA CDI 配置

🟡 **中风险** — 启用 CDI 模式（替代传统 Device Plugin 环境变量注入）：

```yaml
# gpu-operator CDI 配置 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: device-plugin-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    flags:
      migStrategy: "none"
    sharing:
      timeSlicing:
        resources:
        - name: nvidia.com/gpu
          replicas: 1
    cdi:
      enabled: true              # 启用 CDI 模式
      annotationPrefix: "cdi.k8s.io"
---
# 验证 CDI 规范文件已生成
# 🟢 只读
# kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
#   ls /etc/cdi/
# 预期输出: nvidia.yaml
```

🟢 **只读** — 验证 CDI 设备注入：

```bash
# 创建测试 Pod 使用 CDI 注解
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: cdi-gpu-test
  namespace: default
  annotations:
    cdi.k8s.io/gpu: "nvidia.com/gpu=0"
spec:
  containers:
  - name: cuda-test
    image: nvidia/cuda:12.4.0-base-ubuntu22.04
    command: ["nvidia-smi"]
    resources:
      limits:
        nvidia.com/gpu: "1"
  restartPolicy: Never
EOF

# 查看 Pod 日志验证 GPU 可见
kubectl logs cdi-gpu-test

# 清理
kubectl delete pod cdi-gpu-test
```

### DRA 动态资源分配配置（K8s 1.30+）

🟡 **中风险** — 启用 DRA Feature Gate 并配置 NVIDIA DRA Driver：

```yaml
# kube-apiserver / kube-scheduler / kubelet 启用 DRA Feature Gate
# （通过 kubeadm 配置或云厂商托管集群控制台）
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  DynamicResourceAllocation: true
---
# ResourceClass 定义（集群管理员创建）
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClass
metadata:
  name: gpu-a100
driverName: nvidia.com/gpu
parametersRef:
  name: gpu-a100-params
  kind: ResourceClassParameters
  apiGroup: resource.nvidia.com
---
# ResourceClaim 示例（用户创建，请求 2 张 A100）
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClaim
metadata:
  name: training-gpu-claim
  namespace: team-nlp
spec:
  resourceClassName: gpu-a100
  devices:
    requests:
    - name: gpu-req
      deviceClassName: gpu-a100
      selectors:
      - cel:
          expression: |
            device.attributes["nvidia.com"].productName == "A100-SXM4-80GB" &&
            device.attributes["nvidia.com"].memoryTotal >= 80
      count: 2
    constraints:
    - requests: ["gpu-req"]
      matchAttribute: "nvidia.com/numa-node"  # 同 NUMA 节点
---
# Pod 引用 ResourceClaim
apiVersion: v1
kind: Pod
metadata:
  name: dra-training-pod
  namespace: team-nlp
spec:
  resourceClaims:
  - name: gpu-claim
    resourceClaimName: training-gpu-claim
  containers:
  - name: trainer
    image: nvcr.io/nvidia/pytorch:24.05-py3
    resources:
      claims:
      - name: gpu-claim
```

### 自定义 Device Plugin 开发

🟡 **中风险** — 自定义 Device Plugin 骨架（Go 实现）：

```go
// main.go - 自定义 FPGA Device Plugin 骨架
package main

import (
    "context"
    "net"
    "os"
    "time"

    "google.golang.org/grpc"
    pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

const (
    resourceName = "example.com/fpga"
    serverSock   = pluginapi.DevicePluginPath + "example-fpga.sock"
)

type FPGAPlugin struct {
    devices map[string]*pluginapi.Device
    server  *grpc.Server
    stop    chan interface{}
}

// ListAndWatch 持续上报设备状态
func (p *FPGAPlugin) ListAndWatch(e *pluginapi.Empty, s pluginapi.DevicePlugin_ListAndWatchServer) error {
    s.Send(&pluginapi.ListAndWatchResponse{Devices: p.deviceList()})
    for {
        select {
        case <-p.stop:
            return nil
        case <-time.After(30 * time.Second):
            // 定期检查设备健康状态
            p.healthCheck()
            s.Send(&pluginapi.ListAndWatchResponse{Devices: p.deviceList()})
        }
    }
}

// Allocate 分配设备给容器
func (p *FPGAPlugin) Allocate(ctx context.Context, reqs *pluginapi.AllocateRequest) (*pluginapi.AllocateResponse, error) {
    responses := pluginapi.AllocateResponse{}
    for _, req := range reqs.ContainerRequests {
        response := pluginapi.ContainerAllocateResponse{
            Envs: map[string]string{
                "FPGA_DEVICES": req.DevicesIDs[0],
            },
            Devices: []*pluginapi.DeviceSpec{
                {
                    ContainerPath: "/dev/fpga0",
                    HostPath:      "/dev/fpga0",
                    Permissions:   "rw",
                },
            },
            Mounts: []*pluginapi.Mount{
                {
                    ContainerPath: "/opt/fpga/lib",
                    HostPath:      "/usr/lib/fpga",
                    Readonly:      true,
                },
            },
        }
        responses.ContainerResponses = append(responses.ContainerResponses, &response)
    }
    return &responses, nil
}

// Register 向 kubelet 注册
func (p *FPGAPlugin) Register() error {
    conn, err := grpc.Dial(pluginapi.KubeletSocket,
        grpc.WithInsecure(),
        grpc.WithDialer(func(addr string, timeout time.Duration) (net.Conn, error) {
            return net.DialTimeout("unix", addr, timeout)
        }))
    if err != nil {
        return err
    }
    defer conn.Close()

    client := pluginapi.NewRegistrationClient(conn)
    _, err = client.Register(context.Background(), &pluginapi.RegisterRequest{
        Version:      pluginapi.Version,
        Endpoint:     "example-fpga.sock",
        ResourceName: resourceName,
    })
    return err
}
```

## 运维操作

### 设备注册状态检查

🟢 **只读** — 检查 Device Plugin 注册与设备分配状态：

```bash
# 查看节点上的扩展资源
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
GPU_ALLOC:.status.allocatable.nvidia\\.com/gpu,\
GPU_CAP:.status.capacity.nvidia\\.com/gpu

# 查看 Device Plugin DaemonSet 状态
kubectl get pods -n gpu-operator -l app=nvidia-device-plugin-daemonset -o wide

# 查看 Device Plugin 日志（注册和分配事件）
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset --tail=100 | \
  grep -E "Register|Allocate|ListAndWatch|error"

# 查看节点上已分配的设备
kubectl describe node <node-name> | grep -A 10 "Allocated resources"

# 检查 CDI 规范文件
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- cat /etc/cdi/nvidia.yaml | head -50
```

### 设备健康状态监控

🟢 **只读** — 监控 GPU 设备健康与 XID 错误：

```bash
# 查看 GPU 健康状态（通过 DCGM）
kubectl exec -n gpu-operator ds/nvidia-dcgm-exporter -- \
  dcgmi diag -r 1 2>/dev/null || echo "使用 nvidia-smi 替代"

# 查看节点 GPU 详情
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- nvidia-smi

# 检查 XID 错误（GPU 硬件故障指示）
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
  nvidia-smi --query-gpu=gpu_uuid,gpu_name,temperature.gpu,ecc.errors.uncorrected.volatile.total --format=csv

# 查看 kubelet 设备管理日志
journalctl -u kubelet --since "1 hour ago" | grep -i "device\|plugin\|allocat"
```

### 设备拓扑查看

🟢 **只读** — 查看 GPU 拓扑与 NUMA 亲和性：

```bash
# 查看 GPU 互联拓扑（NVLink/PCIe）
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- nvidia-smi topo -m

# 查看 NUMA 节点分布
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- nvidia-smi topo -p2p n

# 查看节点标签中的拓扑信息（GPU Feature Discovery）
kubectl get node <node-name> -o json | jq '.metadata.labels | to_entries[] | select(.key | contains("nvidia"))'
```

## 故障排查

### 设备注册失败

**现象**：节点 `Allocatable` 中无 `nvidia.com/gpu` 资源，Pod 调度报 `Insufficient nvidia.com/gpu`。

**排查步骤**：

```bash
# 🟢 检查 Device Plugin Pod 是否运行
kubectl get pods -n gpu-operator -l app=nvidia-device-plugin-daemonset

# 🟢 查看 Device Plugin 日志
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset --tail=50

# 🟢 检查 kubelet 设备插件目录
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
  ls -la /var/lib/kubelet/device-plugins/

# 🟢 检查 NVIDIA 驱动是否正常
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- nvidia-smi

# 🟢 检查容器运行时 CDI 配置
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
  cat /etc/containerd/config.toml | grep -A 5 "nvidia"
```

**常见根因与修复**：
1. **驱动未加载**：`nvidia-smi` 报错 → 重新安装驱动或重启 gpu-operator Driver DaemonSet
2. **Socket 文件残留**：`/var/lib/kubelet/device-plugins/` 下有旧 socket → 重启 Device Plugin Pod
3. **containerd 配置缺失**：nvidia runtime 未注册 → 检查 `/etc/containerd/config.toml` 中 nvidia runtime 配置
4. **权限不足**：Device Plugin 无法访问 `/dev/nvidia*` → 检查 SecurityContext 和 SELinux/AppArmor

### Allocate 超时

**现象**：Pod 调度成功但容器启动超时，Events 显示 `Allocate failed: deadline exceeded`。

**排查步骤**：

```bash
# 🟢 查看 Pod Events
kubectl describe pod <pod-name> -n <namespace> | grep -A 10 "Events"

# 🟢 查看 kubelet 日志
journalctl -u kubelet --since "10 min ago" | grep -i "allocat\|device\|timeout"

# 🟢 检查设备是否被其他进程占用
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
  nvidia-smi --query-compute-apps=pid,process_name,used_memory --format=csv

# 🟢 检查 CDI hook 是否执行成功
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
  ls -la /usr/bin/nvidia-cdi-hook
```

**修复方案**：
1. 清理僵尸 GPU 进程（`nvidia-smi --gpu-reset` 需节点维护窗口）
2. 增大 kubelet `--device-plugin-timeout`（默认 30s）
3. 检查 CDI hook 二进制是否存在且可执行
4. 重启 Device Plugin DaemonSet：`kubectl rollout restart ds/nvidia-device-plugin-daemonset -n gpu-operator`

### MIG 设备分配错误

**现象**：请求 MIG 分片（如 `nvidia.com/mig-1g.5gb`）的 Pod 无法调度。

**排查步骤**：

```bash
# 🟢 检查 MIG 模式是否启用
kubectl exec -n gpu-operator ds/nvidia-device-plugin-daemonset -- \
  nvidia-smi mig -lgi

# 🟢 查看 MIG 配置策略
kubectl get configmap device-plugin-config -n gpu-operator -o yaml

# 🟢 查看节点 MIG 资源
kubectl describe node <node-name> | grep "mig"
```

**修复方案**：确认 `mig.strategy` 配置正确（`single` 或 `mixed`）；确认 GPU 已创建 MIG 实例（`nvidia-smi mig -cgi`）；参考 [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] 中的 MIG 配置章节。

## 最佳实践

### 三代框架对比

| 维度 | Device Plugin | CDI | DRA |
|------|--------------|-----|-----|
| K8s 版本 | 1.10+ GA | 1.28+ GA | 1.30+ Beta |
| 分配模型 | 静态计数（整数） | 静态计数 + 声明式注入 | 动态协商（类似 PVC） |
| 设备参数 | 不支持 | 支持（CDI spec） | 支持（ResourceClass） |
| 拓扑感知 | GetPreferredAllocation | 不支持 | 原生支持（CEL 表达式） |
| 设备共享 | Time-Slicing/MIG | 同 Device Plugin | 原生支持 |
| 多设备类型 | 每种需独立 Plugin | 统一规范 | 统一 ResourceClass |
| 生产就绪度 | 成熟 | 成熟 | Beta（1.32 趋近 GA） |
| 推荐场景 | 当前生产默认 | 新部署推荐 | 未来方向 |

### 生产部署建议

1. **优先使用 GPU Operator**：自动管理 Driver、Device Plugin、DCGM Exporter、GFD 的完整生命周期
2. **启用 CDI 模式**：新集群部署时启用 CDI，获得更清晰的设备注入语义和更好的安全性
3. **关注 DRA 进展**：K8s 1.32 中 DRA 接近 GA，未来将替代 Device Plugin 的计数模型
4. **拓扑感知调度**：多卡训练任务必须考虑 NVLink 拓扑，使用 Topology Manager 或 DRA CEL 表达式确保 GPU 在同一 NVSwitch 域
5. **健康检查自动化**：部署 DCGM Exporter + Prometheus AlertManager，对 XID 错误、ECC 错误、温度异常自动告警并标记节点不可调度
6. **设备插件高可用**：Device Plugin DaemonSet 配置 `updateStrategy: RollingUpdate`，避免全节点同时重启

### 安全注意事项

- Device Plugin 需要 `privileged: true` 或精细的 Linux Capabilities（`SYS_ADMIN`）
- CDI 规范文件（`/etc/cdi/`）应设为只读，防止容器逃逸篡改
- DRA ResourceClass 创建权限应限制为集群管理员（ClusterRole）
- 避免在 Device Plugin 中暴露宿主机敏感路径

## Related

- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]
- [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]
- [[15-AI基础设施/05-K8s-AI基础设施/13-model-serving-autoscaling-keda.md|推理服务自动伸缩]]
- [[15-AI基础设施/05-K8s-AI基础设施/14-gpu-cost-attribution-multitenant.md|GPU 成本分摊与多租户 AI 平台]]
- [[15-AI基础设施/05-K8s-AI基础设施/17-ai-platform-architecture-reference.md|企业 AI 平台参考架构]]
