---
title: WebAssembly 生产部署指南
description: 面向在 Kubernetes 上生产化部署 WebAssembly（Wasm）工作负载的指南，覆盖 Wasm 运行时选择（Spin/containerd-wasm-shim）、调度、网络与存储、可观测性、安全与 CI/CD。
summary: 面向 Kubernetes 上 Wasm 生产部署的指南，覆盖运行时选择、调度、网络/存储、可观测性、安全与 CI/CD。
category: specialized-tech
tags:
- production
- best-practices
- playbook
- specialized-tech
- webassembly
- wasm
- spin
- containerd-wasm-shim
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 上如何生产化部署 Wasm
- Spin 与 containerd-wasm-shim 怎么选
- Wasm 工作负载调度与可观测性
- Wasm 生产安全与 CI/CD 实践
trigger_keywords:
- WebAssembly
- Wasm
- Spin
- containerd-wasm-shim
- WasmEdge
- Wasm runtime
prerequisites:
- kubectl-basics
- containerd-basics
- webassembly-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# WebAssembly 生产部署指南

本指南面向希望在 Kubernetes 上以生产标准运行 WebAssembly（Wasm）工作负载的 SRE 与平台工程师，提供 Wasm 运行时选型、节点配置、调度、网络/存储、可观测性、安全与 CI/CD 的完整操作路径。WebAssembly 以其快速启动、小体积和强隔离特性，逐渐成为云原生场景中运行无状态函数和微服务的有效补充。然而，Wasm 生态仍在快速演进，运行时、工具链和可观测性与传统容器存在显著差异。本指南中的命令可直接在已安装 `kubectl`、`containerd` 与相关 CLI 的环境中执行，所有变更应先在测试环境验证，并遵循 [[32-发布/package/2026-07-02_18-29/corpus/core/domain-15-specialized-tech/03-production-readiness-operations-guide|生产就绪运维框架]] 中的变更管理要求。

## 1. 适用场景与范围

本指南适用于以下场景：

- 在 Kubernetes 上部署基于 Spin、WasmEdge 或其他 Wasm 运行时的微服务。
- 需要配置 containerd shim 以支持 Wasm 镜像运行。
- 需要为 Wasm 工作负载设计调度、网络、存储与可观测性方案。
- 排查 Wasm Pod 启动失败、运行时异常、网络/存储挂载问题。
- 希望将 Wasm 与传统容器工作负载混合部署在同一集群中。

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必需工具
kubectl version
ctr version
spin --version

# 推荐运行时
# containerd-wasm-shim: https://github.com/containerd/runwasi
# SpinKube / Spin Operator: https://www.spinkube.dev/
# WasmEdge: https://wasmedge.org/
```
要求：
- 集群使用 containerd 作为 CRI。
- 节点已安装对应 Wasm runtime 二进制或 shim。
- RuntimeClass 已注册（如 `wasmtime-spin`、`wasmedge`）。
- 镜像仓库支持 OCI artifacts 或 Wasm 模块格式。

## 3. 核心概念与架构

### 3.1 Wasm 运行时选型

| 运行时 | 特点 | 适用场景 |
|---|---|---|
| **Spin（Fermyon）** | 事件驱动、HTTP 触发器、组件模型支持 | 无状态微服务、API、函数计算 |
| **WasmEdge** | 高性能、支持多种语言、AI/媒体扩展 | 边缘推理、实时处理 |
| **containerd-wasm-shim（runwasi）** | 标准 containerd shim，兼容 OCI | 需要与容器混合部署 |

生产建议：新服务优先使用 Spin + SpinKube Operator；需要与现有容器深度集成时使用 containerd-wasm-shim。

### 3.2 部署模式

- **OCI 镜像模式**：将 Wasm 模块打包为 OCI 镜像，由 containerd 通过 shim 启动。这种方式与现有容器镜像供应链兼容，便于使用现有 CI/CD 与镜像仓库。
- **Spin App 模式**：使用 SpinKube CRD（SpinApp）直接描述 Wasm 应用，由 Spin Operator 管理生命周期。这种方式更贴近 Wasm 原生语义，支持自动扩展与事件触发。

## 4. 标准操作流程

### 4.1 安装 containerd-wasm-shim

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
# 在节点上下载 shim（以 Spin 为例）
VERSION=v0.11.0
curl -LO https://github.com/containerd/runwasi/releases/download/${VERSION}/containerd-shim-spin-v2-linux-x86_64.tar.gz
sudo tar -xzf containerd-shim-spin-v2-linux-x86_64.tar.gz -C /usr/local/bin/

# 配置 containerd /etc/containerd/config.toml
sudo tee -a /etc/containerd/config.toml <<EOF
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.spin]
  runtime_type = "io.containerd.spin.v2"
EOF

sudo systemctl restart containerd
```
### 4.2 创建 RuntimeClass

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime-spin
handler: spin
scheduling:
  nodeSelector:
    runtime: wasm
EOF
```
### 4.3 部署 Wasm 工作负载（OCI 模式）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-hello
  namespace: prod
spec:
  replicas: 3
  selector:
    matchLabels:
      app: wasm-hello
  template:
    metadata:
      labels:
        app: wasm-hello
    spec:
      runtimeClassName: wasmtime-spin
      nodeSelector:
        runtime: wasm
      containers:
      - name: hello
        image: ghcr.io/deislabs/containerd-wasm-shims/examples/spin-rust-hello:latest
        resources:
          requests:
            cpu: "0.1"
            memory: 32Mi
          limits:
            cpu: "0.5"
            memory: 128Mi
---
apiVersion: v1
kind: Service
metadata:
  name: wasm-hello
  namespace: prod
spec:
  selector:
    app: wasm-hello
  ports:
  - port: 80
    targetPort: 8080
EOF
```
### 4.4 使用 SpinKube Operator 部署

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 SpinKube
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.2.0/spin-operator.runtime-class.yaml
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.2.0/spin-operator.crds.yaml
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.2.0/spin-operator.yaml

# 创建 SpinApp
cat <<EOF | kubectl apply -f -
apiVersion: core.spinoperator.dev/v1alpha1
kind: SpinApp
metadata:
  name: prod-spin-app
  namespace: prod
spec:
  image: ghcr.io/spinkube/spin-operator/samples/spin-rust-hello:latest
  replicas: 3
  executor: containerd-shim-spin
  resources:
    requests:
      cpu: "0.1"
      memory: 32Mi
EOF
```
### 4.5 网络与存储

Wasm 模块通常无状态，持久化需求通过外部服务（对象存储、数据库）满足。如需临时存储：

```yaml
spec:
  containers:
  - name: app
    image: <wasm-image>
    volumeMounts:
    - name: tmp
      mountPath: /tmp
  volumes:
  - name: tmp
    emptyDir:
      sizeLimit: 100Mi
```

网络：
- 使用标准 Kubernetes Service/Ingress 暴露 HTTP 服务。
- 通过 NetworkPolicy 限制 Wasm Pod 的出站访问。
- 对于需要访问外部服务的 Wasm 应用，使用 Kubernetes ExternalName Service 或 Sidecar 代理。

### 4.6 可观测性

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 暴露 metrics（依赖运行时支持）
kubectl logs -l app=wasm-hello -n prod --tail=100

# 使用 Prometheus 抓取 /metrics
# 配置 ServiceMonitor
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: wasm-hello
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: wasm-hello
  endpoints:
  - port: metrics
    path: /metrics
EOF
```
### 4.7 CI/CD

```yaml
# GitHub Actions 示例片段
- name: Build Wasm module
  run: spin build

- name: Push OCI artifact
  run: spin registry push ghcr.io/<org>/wasm-hello:${{ github.sha }}

- name: Deploy to Kubernetes
  run: kubectl set image deployment/wasm-hello hello=ghcr.io/<org>/wasm-hello:${{ github.sha }} -n prod
```

建议为 Wasm 镜像启用 cosign 签名，并在部署前通过 Kyverno 或 OPA 验证签名。

## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| RuntimeClass | `kubectl get runtimeclass` | `wasmtime-spin` 存在 |
| 节点标签 | `kubectl get nodes -L runtime` | Wasm 节点带有 runtime=wasm |
| Pod 状态 | `kubectl get pods -n prod -l app=wasm-hello` | Running，无 CrashLoop |
| 服务访问 | `kubectl port-forward svc/wasm-hello 8080:80 -n prod` | curl 返回 200 |
| 资源使用 | `kubectl top pods -n prod` | 资源使用符合预期 |
| 日志 | `kubectl logs -n prod -l app=wasm-hello` | 无异常堆栈 |

## 6. 常见故障与 remediation

| 现象 | 根因 | 处理命令/步骤 |
|---|---|---|
| Pod 启动失败 `runtime not found` | containerd 未配置 shim 或 RuntimeClass 不匹配 | 检查节点 shim 安装与 containerd 配置 |
| 镜像拉取失败 | OCI artifact 仓库不支持 Wasm 类型 | 确认 registry 支持 OCI artifacts；检查镜像 tag |
| 服务无法访问 | 端口映射错误、Ingress 配置缺失 | 检查 Container Port、Service targetPort、Ingress 规则 |
| 性能低于预期 | Wasm 模块未优化、CPU 限制过低 | 分析模块性能；调整 resources limits |
| 存储写入失败 | Wasm 模块无文件系统权限 | 使用 emptyDir 或外部存储；检查 WASI 权限 |
| 监控指标缺失 | 运行时未暴露 /metrics | 检查应用代码与 ServiceMonitor 配置 |

## 7. 风险与注意事项

1. **Wasm 生态仍在快速演进**：生产使用前确认运行时与 Operator 的版本稳定性与支持周期。
2. **并非所有工作负载都适合 Wasm**：I/O 密集、依赖原生库的场景应继续使用容器。
3. **安全沙箱不等于绝对安全**：仍需遵循最小权限、NetworkPolicy、镜像签名与漏洞扫描。
4. **资源限制需谨慎**：Wasm 启动快但可能被误认为无资源消耗，仍需设置 requests/limits 避免节点过载。
5. **调试工具有限**：熟悉 `spin logs`、`kubectl logs` 与 `ctr` 命令，必要时结合 netshoot sidecar。
6. **镜像供应链安全**：使用 cosign/notation 对 Wasm OCI artifact 签名，防止未授权模块运行。
7. **存储能力受限**：Wasm WASI 文件系统能力有限，复杂持久化需求应通过外部服务实现。

## 8. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-15-specialized-tech/03-production-readiness-operations-guide|生产运维域生产就绪运维指南]]
- [[01-wasm-fundamentals-cloud-native.md|Wasm 云原生基础]]
- [[02-containerd-wasm-shim.md|containerd-wasm-shim]]
- [[03-spinkube-framework.md|SpinKube 框架]]
- [[10-wasm-security-sandbox.md|Wasm 安全沙箱]]
- [[domain-13-container-runtime/README.md|容器运行时域]]
- [[domain-05-security-compliance/README.md|安全合规域]]


<!-- risk-assessed -->
