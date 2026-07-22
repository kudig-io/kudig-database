---
title: K8S 专项技术
summary: K8S 专项技术：eBPF 已成为云原生基础设施的核心技术：
category: concepts
tags:
- ebpf
- wasm
- edge
- serverless
- knative
- dapr
- k8s
tier: core
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8S 专项技术

## eBPF 生态

eBPF 已成为云原生基础设施的核心技术：

- **[[Cilium]]**：CNCF 毕业项目，基于 eBPF 的 CNI/Service Mesh，替代 kube-proxy 提供高性能网络、可观测性与安全策略
- **[[Tetragon]]** v1.7+：Cilium 子项目，eBPF 驱动的运行时安全，内核级拦截进程、文件、网络行为，无需 sidecar
- **[[Pixie]]**：CNCF 沙箱，基于 eBPF 的自动遥测，零代码采集 K8S 集群的全栈指标与追踪
- **[[Falco]]**：支持 eBPF 探针的运行时威胁检测，替代传统内核模块方案

## WebAssembly on K8S

WASM 正在成为 K8S 的第二运行时：

- **[[SpinKube]]**：CNCF Sandbox，将 Spin 应用编排为 K8S 工作负载，支持 CRD、Helm 部署
- **[[wasmCloud]]**：CNCF Sandbox，分布式 WASM 应用平台，actor 模型 + 可组合能力
- **runwasi**：containerd 官方 WASM shim，使 kubelet 透明调度 WASM 工作负载

WASM 工作负载特点：亚毫秒启动、极小内存占用、跨平台字节码。参见 [[container-runtime-evolution]]。

## 边缘计算

- **[[KubeEdge]]**：CNCF 毕业项目（v1.22+），将 K8S 能力延伸至边缘节点，支持离线自治
- **[[K3s]]**：轻量级 K8S 发行版（<100MB 二进制），适用于 IoT/ARM/边缘场景
- **MicroK8s**：Canonical 维护的单节点 K8S，snap 包分发，适合开发与边缘
- **[[Akri]]**：CNCF 沙箱，自动发现边缘设备（摄像头、传感器等）并暴露为 K8S 资源

## Serverless / FaaS

- **[[Knative]]**：CNCF 毕业项目，提供事件驱动与请求驱动的 serverless 平台，核心能力为 **scale-to-zero** 与自动扩缩
- **[[Dapr]]**：CNCF 毕业项目，分布式应用运行时，通过 sidecar 提供服务调用、状态管理、发布订阅等构建块 API
- **[[OpenFunction]]**：CNCF 沙箱，云原生 FaaS 平台，支持多种运行时（Node.js、Go、WASM 等）

## ARM64 全面支持

- Kubernetes 核心组件已全面支持 ARM64
- 主流 CNI（Cilium、Calico）、CSI、Ingress Controller 均提供 ARM64 镜像
- AWS Graviton、Ampere Altra、Apple Silicon 成为主流 ARM 服务器平台
- 多架构镜像（multi-arch）已成为容器镜像构建标准实践

## Windows 容器

- Kubernetes 支持 Windows Server 容器节点（Windows Server 2019/2022）
- containerd 成为 Windows 容器默认运行时（替代 dockershim）
- HPA、资源限制、Network Policy 等核心功能在 Windows 节点上可用
- 混合 Linux/Windows 集群可通过 nodeSelector 和 taint/toleration 调度

## 技术深度解析

### eBPF 工作原理

eBPF（extended Berkeley Packet Filter）允许在内核中安全运行沙箱程序，无需修改内核源码或加载内核模块：

```
用户态:
  Cilium / Tetragon / Falco → 编译 eBPF 程序 → bpf() 系统调用加载到内核

内核态:
  eBPF Verifier → 验证安全性（无无限循环、内存安全）
  → 挂载到 hook 点（XDP / TC / Tracepoint / Kprobe）
  → 在网络包/系统调用路径上执行 → 数据通过 ring buffer / map 传回用户态
```

**Cilium 替代 kube-proxy 的数据路径**：

| 传统 kube-proxy | Cilium eBPF |
|----------------|-------------|
| iptables 规则链（O(n) 查找） | eBPF hashmap（O(1) 查找） |
| 每次 Service 变更重写全部规则 | 增量更新 eBPF map |
| 数据包路径：Pod → iptables → Pod | 数据包路径：Pod → eBPF → Pod（更短） |

### WASM 运行时集成

```yaml
# WASM 工作负载通过 RuntimeClass 调度
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasm
handler: wasm
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-function
spec:
  template:
    spec:
      runtimeClassName: wasm           # 使用 WASM 运行时
      containers:
      - name: spin-app
        image: registry/spin-app:latest  # WASM 模块打包为 OCI 镜像
```

### Knative Serverless 配置

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: event-processor
spec:
  template:
    metadata:
      annotations:
        autoscaling.knative.dev/scale-to-zero: "true"
        autoscaling.knative.dev/target: "10"     # 每实例 10 并发
        autoscaling.knative.dev/min-scale: "0"   # 允许缩容到零
    spec:
      containers:
      - image: processor:latest
```

## 最佳实践

- **eBPF 优先于内核模块**：Falco、Cilium 等 eBPF 方案在生产环境中应优先于传统内核模块方案——eBPF 无需修改内核、不会导致 panic、可热更新
- **WASM 工作负载隔离测试**：WASM 虽有安全沙箱优势，但生态尚不成熟——建议先在非关键场景验证再推广
- **边缘节点使用 K3s 而非完整 K8s**：资源受限环境下 K3s（<100MB）远优于完整 K8s 发行版
- **多架构镜像成为标准**：ARM 节点越来越普遍，所有镜像构建必须支持 multi-arch（`docker buildx`）
- **Serverless 场景评估冷启动容忍度**：Knative scale-to-zero 有冷启动延迟（镜像拉取+初始化），延迟敏感型服务不适合

## 常见陷阱

- **eBPF 程序内核版本依赖**：eBPF 程序的行为可能因内核版本差异而不同——需要针对目标内核版本测试，Cilium 有严格的内核版本兼容矩阵
- **WASM 性能限制**：WASM 目前不支持 GPU 直通和复杂系统调用，计算密集型 AI 工作负载仍需传统容器
- **Windows 节点网络限制**：Windows 容器不支持所有 CNI 功能（如 Cilium eBPF datapath），混合集群中需单独处理 Windows 节点的网络方案

## 源码实现分析

### eBPF 程序加载与挂载

```c
// cilium/bpf/bpf_xgress.c
// Cilium eBPF 数据路径：在 tc ingress 钩子处理网络包
__section_entry
int handle_xgress(struct __ctx_buff *ctx) {
    void *data = (void *)(long)ctx->data;
    struct ethhdr *eth = data;
    
    // 1. 解析以太网头
    if (eth->h_proto == bpf_htons(ETH_P_IP)) {
        // 2. 查找 endpoint（目标 Pod）
        struct endpoint_info *ep = lookup_ep_by_ip(dst_ip);
        if (!ep) return CTX_ACT_DROP;
        
        // 3. 执行网络策略检查
        if (!policy_allows(ep, src_identity, dst_port)) {
            send_drop_notify(ctx, ep);
            return CTX_ACT_DROP;
        }
        // 4. 转发到目标 Pod
        return redirect_ep(ctx, ep);
    }
    return CTX_ACT_OK;
}
```

### WASM 运行时集成 (containerd)

```go
// github.com/containerd/containerd/pkg/cri/server/container_start.go
// containerd 通过 RuntimeClass 选择 WASM 运行时
func (c *criService) StartContainer(ctx context.Context, r *runtime.StartContainerRequest) {
    // 根据 RuntimeClass 选择运行时
    switch runtimeClass {
    case "wasmtime":
        // 使用 wasmtime shim 而非 runc
        task, _ = container.NewTask(ctx, cio.NewCreator(
            cio.WithStdio),
            containerd.WithRuntime("io.containerd.wasmtime.v1"),
        )
    case "wasmedge":
        task, _ = container.NewTask(ctx, cio.NewCreator(
            cio.WithStdio),
            containerd.WithRuntime("io.containerd.wasmedge.v1"),
        )
    default:
        // 标准 runc 运行时
        task, _ = container.NewTask(ctx, cio.NewCreator())
    }
    task.Start(ctx)
}
```

### 专项技术架构对比

```
┌───────────────────────────────────────────────────────────┐
│            专项技术架构对比                             │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  eBPF (内核级可编程)                                     │
│  ────────────────────                                    │
│  用户空间程序 → BPF 字节码 → 内核验证器 → JIT 编译   │
│       → 挂载到 tc/XDP/tracepoint → 零拷贝处理         │
│                                                           │
│  WASM (沙箱运行时)                                       │
│  ────────────────────                                    │
│  .wasm 模块 → WASI 接口 → 沙箱执行                    │
│       → 无 root、无内核访问、内存隔离               │
│                                                           │
│  Knative (Serverless)                                    │
│  ────────────────────                                    │
│  Service CR → Activator (缓冲请求) → Pod (scale 0~N) │
│       → KPA (自动扩缩) → 冷启动优化                 │
│                                                           │
│  K3s (轻量 K8s)                                          │
│  ────────────────────                                    │
│  单二进制 <100MB → SQLite/etcd → 边缘/IoT 场景      │
│       → 去除云提供商依赖、内置 Traefik             │
└───────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：eBPF 网络策略与可观测性（🟢 只读观察）

```bash
# Cilium Hubble 实时观察 Pod 间流量
hubble observe --namespace production \
  --from-pod "app=frontend" \
  --to-pod "app=backend" \
  --protocol http

# 查看 eBPF 程序加载状态
bpftool prog list | grep -A2 "tc\|xdp"

# CiliumNetworkPolicy L7 策略
kubectl apply -f - <<EOF
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-policy
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "8080"
      rules:
        http:
        - method: GET
          path: /api/.*
EOF
```

### 场景二：WASM 工作负载部署（🟡 需要 RuntimeClass）

```yaml
# 前提：安装 wasmtime/wasmedge containerd shim
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime
handler: wasmtime
---
apiVersion: v1
kind: Pod
metadata:
  name: wasm-hello
spec:
  runtimeClassName: wasmtime  # 使用 WASM 运行时
  containers:
  - name: hello
    image: wasm-hello:latest  # .wasm 模块打包为 OCI 镜像
    resources:
      limits:
        cpu: 100m
        memory: 64Mi  # WASM 内存占用极小
```

### 场景三：K3s 边缘节点部署（🔴 生产基础设施）

```bash
# 边缘节点安装 K3s（<100MB 单二进制）
curl -sfL https://get.k3s.io | INSTALL_K3S_EXEC="\
  --disable traefik \
  --disable servicelb \
  --node-label edge=true \
  --data-dir /var/lib/rancher/k3s" sh -

# 验证轻量集群状态
kubectl get nodes -l edge=true
kubectl top nodes  # 资源占用远低于完整 K8s
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| eBPF 可以完全替代 iptables | eBPF 性能更优但需内核 5.4+，老内核仍需 iptables |
| WASM 可以运行任何应用 | WASM 不支持 GPU、复杂 syscall，仅适合轻量计算 |
| K3s 是玩具级 K8s | K3s 是 CNCF 认证的完整 K8s，适合生产边缘场景 |
| Knative scale-to-zero 无延迟 | 冷启动有 1-10s 延迟，延迟敏感服务不适合 |
| eBPF 程序无需测试 | eBPF 行为依赖内核版本，必须针对目标内核测试 |
| 边缘节点不需要监控 | 边缘节点更难物理访问，监控和自愈更重要 |

## 面试要点

1. **eBPF 相比传统内核模块的优势？**
   - 安全性：内核验证器保证不会 crash/panic
   - 性能：JIT 编译接近原生速度，零拷贝
   - 灵活性：热更新无需重启内核
   - 应用：Cilium 网络、Falco 安全、Pixie 可观测

2. **WASM 在 K8s 中的定位和限制？**
   - 定位：轻量、安全沙箱、快速启动（<1ms）
   - 限制：无 GPU、无复杂 syscall、生态不成熟
   - 场景：边缘计算、插件系统、多租户隔离

3. **Knative scale-to-zero 的实现原理？**
   - Activator 组件缓冲请求，无流量时缩容到 0
   - 新请求到达 → Activator 触发 KPA 扩容 → Pod 启动
   - 冷启动优化：镜像预拉取、snapshot 恢复

4. **边缘计算场景 K3s vs K8s 如何选型？**
   - K3s：资源受限（<1GB RAM）、网络不稳定、远程管理
   - K8s：边缘数据中心、充足资源、需要完整生态
   - 关键：离线安装、轻量存储、断网自愈能力

## 相关页面

- [[概念/container-runtime-evolution.md|容器运行时演进]] — WASM 与机密容器
- [[概念/edge-cloud-continuum.md|边缘云连续体]] — KubeEdge 边缘计算
- [[概念/k8s-networking-evolution.md|K8S 网络技术演进]] — Cilium/eBPF 网络


<!-- risk-assessed -->
