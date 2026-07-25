---
title: SpinKube [entities]
description: '## 概述'
summary: 'SpinKube 是一个在 Kubernetes 上运行 WebAssembly (Wasm) 微服务和应用的开源平台。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度、更小的资源占用和更强的安全隔离。'
category: entities
tags:
- k8s
- cncf
- runtime
- spinkube
- prometheus
- containerd
- gateway
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SpinKube 是什么
- 如何 SpinKube
trigger_keywords:
- SpinKube
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SpinKube

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust, Go

## 概述

SpinKube 是由 Microsoft（Fermyon 团队）开源的 WebAssembly（Wasm）应用运行平台，2024 年加入 CNCF Sandbox。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度（毫秒级）、更小的资源占用（MB 级）和更强的安全隔离（Wasm 沙箱）。SpinKube 代表了 Wasm 作为容器补充运行时的方向。

## 核心特性

- **Wasm 原生**: 将 Spin Wasm 应用作为一等公民部署到 Kubernetes
- **极速启动**: Wasm 模块毫秒级启动，适合 Serverless 和事件驱动场景
- **低资源占用**: 每个 Wasm 实例仅几 MB 内存，高密度部署
- **SpinApp CRD**: 通过 CRD 声明式管理 Wasm 应用
- **OCI 分发**: Wasm 应用通过 OCI Artifact 分发，复用标准 Registry
- **containerd shim**: 通过 spin-shim 与 containerd 原生集成

## 架构

SpinKube 由 Spin Operator 和 containerd-shim-spin 组成。Spin Operator 监听 SpinApp CRD，管理 Wasm 应用的副本和调度。containerd-shim-spin 是 containerd 的 OCI Runtime Shim，使 containerd 能够直接运行 Wasm 模块而非容器镜像。当 Pod 指定 RuntimeClass 为 `wasmtime-spin-v2` 时，kubelet 通过 CRI 调用 containerd，containerd 通过 shim 加载 Wasm 模块并在 Wasmtime 运行时中执行。Wasm 应用通过 Spin SDK 访问 Key-Value Store、SQLite、HTTP 等组件能力。

## Kubernetes 集成

SpinKube 通过 RuntimeClass 与 Kubernetes 集成。`runtimeClassName: wasmtime-spin-v2` 指示 kubelet 使用 Wasm 运行时。SpinApp CRD 定义 Wasm 应用的镜像（OCI 引用）、副本数、环境变量和触发器。Operator 将 SpinApp 转换为标准 Deployment + Service。containerd 的 shim 层处理 Wasm 模块加载和执行，对 Kubernetes 控制平面完全透明。支持标准的 HPA、Service 和 Ingress。

## 生产使用场景

1. **Serverless 函数**: 事件驱动的 Wasm 函数，毫秒级冷启动
2. **API 微服务**: 轻量级 HTTP API 服务，高密度部署
3. **边缘计算**: 在资源受限的边缘节点上运行 Wasm 应用
4. **事件处理**: 消息队列消费者的轻量级处理函数

## 安装与配置

```bash
# 安装 Spin Operator
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.4.0/spin-operator.crds.yaml
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.4.0/spin-operator.runtime-class.yaml
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.4.0/spin-operator.deployment.yaml

# 部署 Spin 应用
kubectl apply -f - <<EOF
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata:
  name: hello-wasm
spec:
  image: ghcr.io/spinkube/containerd-shim-spin/examples/spin-rust-hello:v0.4.0
  replicas: 3
EOF
```

### SpinApp CRD 配置示例

```yaml
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata:
  name: api-service
  namespace: wasm-apps
spec:
  image: registry.example.com/spin/api-service:v1.0.0
  replicas: 5
  runtimeClassName: wasmtime-spin-v2
  env:
    - name: DATABASE_URL
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: url
  resources:
    limits:
      cpu: "500m"
      memory: "128Mi"
    requests:
      cpu: "100m"
      memory: "64Mi"
---
apiVersion: v1
kind: Service
metadata:
  name: api-service
  namespace: wasm-apps
spec:
  selector:
    app: api-service
  ports:
  - port: 80
    targetPort: 80
```

## 运维操作

```bash
# 🟢 查看 SpinApp 状态
kubectl get spinapp -A
kubectl describe spinapp hello-wasm

# 🟢 查看 Wasm Pod 状态
kubectl get pods -n wasm-apps -l app=hello-wasm
kubectl logs -n wasm-apps -l app=hello-wasm

# 🟢 查看 RuntimeClass
kubectl get runtimeclass wasmtime-spin-v2

# 🟡 扩缩容
kubectl scale spinapp hello-wasm --replicas=5

# 🟡 更新 Wasm 应用
kubectl set image spinapp/hello-wasm *=registry.example.com/spin/app:v2.0.0

# 🟢 查看 Spin Operator 日志
kubectl logs -n spin-operator -l app=spin-operator

# 🟢 测试 Wasm 服务
kubectl port-forward svc/hello-wasm 8080:80
curl http://localhost:8080/
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| SpinApp 未创建 Pod | Operator 异常 | `kubectl logs -n spin-operator` | 检查 Operator Pod 状态 |
| Pod CrashLoop | Wasm 模块加载失败 | `kubectl describe pod` | 检查 OCI 镜像和 RuntimeClass |
| 服务无响应 | 触发器配置错误 | 查看 Spin 应用日志 | 检查 HTTP trigger 配置 |
| 镜像拉取失败 | Registry 认证问题 | `kubectl get events` | 配置 imagePullSecrets |
| 内存不足 | Wasm 实例内存限制 | `kubectl top pods` | 调整 resources.limits |

## 生产案例

### 案例1: 高密度 Serverless 函数平台

**场景**: 需要支持 1000+ 并发函数的 Serverless 平台  
**方案**: SpinKube 替代容器，每个 Wasm 实例仅 4MB 内存  
**效果**: 单节点运行 500+ 实例，冷启动 < 10ms  

### 案例2: 边缘计算轻量级应用

**场景**: 边缘节点资源有限（2GB RAM），需运行多个微服务  
**方案**: SpinKube 部署 Wasm 微服务，替代容器降低资源占用  
**效果**: 资源占用降低 80%，启动速度提升 100倍  

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **SpinKube** | K8s 原生 Wasm、CRD 管理 | 较新、生态小 | Serverless/边缘 |
| WasmEdge + containerd | CNCF Wasm 运行时 | 需手动集成 | 自定义运行时 |
| Kuasar | 多沙箱运行时 | 通用方案 | 多租户隔离 |
| 容器 (containerd) | 最成熟、生态最大 | 启动慢、资源占用大 | 通用工作负载 |

## 架构定位

在 CNCF 生态中，SpinKube 属于 **Runtime / WebAssembly** 类别，是 Wasm 在 Kubernetes 上的代表性运行平台。它代表了容器与 Wasm 共存的未来方向。

## 检查清单

- [ ] 确认 RuntimeClass wasmtime-spin-v2 已创建
- [ ] Wasm 应用通过 OCI Registry 分发
- [ ] 配置合理的资源限制（Wasm 内存通常很小）
- [ ] 配置 HPA 实现自动扩缩
- [ ] 监控 Spin Operator 健康状态
- [ ] 测试冷启动延迟符合 SLA

## 参考链接

- [[containerd]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/07-调度与资源/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[kube-rs]] — kube-rs
- [[capsule]] — Capsule
- [[spin]] — Spin
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
