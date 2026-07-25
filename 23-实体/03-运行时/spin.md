---
title: Spin (entities)
description: '## 概述'
summary: 'Spin 是由 Fermyon 开发的 WebAssembly (Wasm) 微服务框架，用于构建和运行基于事件驱动的 Wasm 应用。它提供极快的冷启动时间（亚毫秒级），支持多种编程语言（Rust、Go、Python、JavaScript、C#等），并内置 HTTP 触发器、Redis 触发器、键值存储、SQL 数据库等能力。'
category: entities
tags:
- k8s
- cncf
- runtime
- spin
- argocd
- containerd
- redis
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
- Spin 是什么
- 如何 Spin
trigger_keywords:
- Spin
prerequisites:
- kubectl-basics
- gitops-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Spin

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Spin 是由 Fermyon 于 2021 年开发的开源 WebAssembly（Wasm）应用框架，2023 年进入 CNCF Sandbox。它专注于构建和运行基于 Wasm 组件的微服务和边缘应用。与传统容器不同，Spin 应用编译为 Wasm 模块，冷启动时间在亚毫秒级（通常 < 1ms），内存占用仅几 MB。

Spin 使用 `spin.toml` 清单文件定义应用，包含多个组件（Component），每个组件绑定一个触发器（Trigger，如 HTTP、Redis、定时器）和一个 Wasm 模块。内置的 Key-Value 存储、SQLite 数据库和 Outbound HTTP 能力让开发者无需额外配置即可构建完整应用。Spin 支持通过 OCI Registry 分发应用，也支持通过 SpinKube 项目在 Kubernetes 中原生运行 Wasm 工作负载。

## Key Features

- **亚毫秒冷启动**：Wasm 模块加载速度极快，适合 Serverless 和边缘场景
- **多语言 SDK**：提供 Rust、Go、Python、JavaScript、TypeScript、C# 的 SDK 和模板
- **内置存储**：无需外部依赖即可使用 KV Store、SQLite、Redis 等存储能力
- **OCI 分发**：支持将 Spin 应用打包为 OCI 镜像推送到标准 Registry
- **触发器模型**：通过 `[[trigger]]` 声明式绑定 HTTP、Redis、定时器等事件源
- **安全沙箱**：基于 Wasm 的 capability-based 安全模型，必须显式声明 `allowed_outbound_hosts`

## Architecture

Spin 运行时基于 `wasmtime` 引擎，通过 `spin.toml` 清单文件解析应用定义。每个组件在独立的 Wasm 实例中运行，通过 Spin Host Interface 访问宿主提供的 API（KV、SQL、HTTP、时钟）。触发器（Trigger）监听外部事件并在匹配时实例化对应的 Wasm 组件执行业务逻辑。组件之间通过共享的 KV Store 或外部消息系统通信。

## K8s 集成

Spin 通过 **SpinKube** 项目（CNCF Sandbox）与 Kubernetes 深度集成。SpinKube 提供 `SpinAppExecutor` 和 `SpinApp` CRD，通过 containerd-shim-spin 在节点上原生运行 Wasm 工作负载，无需传统容器镜像。也可通过 Spin Operator（Helm Chart）在标准 Kubernetes 集群上部署 Spin 运行时。

## 生产部署要点

- **组件粒度**：每个路由前缀使用独立组件，实现最小权限和独立部署
- **最小权限**：通过 `allowed_outbound_hosts` 限制组件的外部访问范围
- **存储选择**：简单 KV 用内置 KV Store，关系数据用 SQLite 或外部 DB
- **OCI 分发**：使用 OCI Registry 管理 Spin 应用版本
- **Wasm 优化**：使用 `wasm-opt` 优化 Wasm 二进制大小

## 生产场景

1. **边缘 IoT 数据处理**：在资源受限设备上运行轻量数据处理逻辑
2. **Serverless API**：按需加载的 Wasm 微服务，零冷启动延迟
3. **实时数据管道**：Redis 触发器驱动的事件处理组件
4. **插件化扩展**：安全沙箱中运行第三方代码，宿主系统不受影响

## 安装与配置

```bash
# 安装 Spin CLI
curl -fsSL https://developer.fermyon.com/downloads/install.sh | bash
sudo mv spin /usr/local/bin/
spin --version

# 安装模板
spin templates install --git https://github.com/fermyon/spin

# 创建新应用
spin new http-rust myapp
cd myapp

# 构建和运行
spin build
spin up
# 访问 http://localhost:3000

# 推送到 OCI Registry
spin registry push ghcr.io/myorg/myapp:v1.0

# 从 Registry 运行
spin up --from-registry ghcr.io/myorg/myapp:v1.0
```

```toml
# spin.toml 应用清单示例
spin_manifest_version = 2

[application]
name = "my-web-app"
version = "1.0.0"
description = "A simple web application"

[[trigger.http]]
route = "/..."
component = "api"

[component.api]
source = "target/wasm32-wasi/release/api.wasm"
allowed_outbound_hosts = ["https://api.example.com"]

[component.api.variables]
database_url = "sqlite://data.db"

[[trigger.http]]
route = "/static/..."
component = "static"

[component.static]
source = "target/wasm32-wasi/release/static.wasm"
files = [{ source = "assets", destination = "/" }]
```

```yaml
# SpinKube 部署 (Kubernetes)
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata:
  name: my-web-app
  namespace: wasm
spec:
  image: ghcr.io/myorg/myapp:v1.0
  executor: containerd-shim-spin
  replicas: 2
  resources:
    requests:
      cpu: 100m
      memory: 64Mi
    limits:
      cpu: 500m
      memory: 128Mi
---
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinAppExecutor
metadata:
  name: containerd-shim-spin
  namespace: wasm
spec:
  deploymentTemplate:
    spec:
      runtimeClassName: wasmtime-spin
```

## 运维操作

```bash
# 🟢 本地运行 Spin 应用
spin up
spin up --listen 0.0.0.0:8080

# 🟢 构建应用
spin build
spin build --up  # 构建并运行

# 🟢 推送到 Registry
spin registry push ghcr.io/myorg/myapp:v1.0

# 🟢 检查 SpinKube 状态
kubectl get spinapps -A
kubectl get spinappexecutors -A
kubectl get pods -n wasm -l core.spinkube.dev/app-name=my-web-app

# 🟢 查看应用日志
kubectl logs -n wasm -l core.spinkube.dev/app-name=my-web-app

# 🟡 扩展副本数
kubectl scale spinapp my-web-app -n wasm --replicas=5

# 🟡 更新应用版本
kubectl set image spinapp/my-web-app *=ghcr.io/myorg/myapp:v1.1 -n wasm

# 🟢 检查 RuntimeClass
kubectl get runtimeclass wasmtime-spin
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| spin build 失败 | 依赖缺失/编译错误 | 检查构建日志 | 安装 Rust/Go 工具链 |
| spin up 无响应 | 端口冲突/路由错误 | 检查 spin.toml 配置 | 修改路由/端口 |
| SpinApp Pending | RuntimeClass 未配置 | `kubectl get runtimeclass` | 安装 containerd-shim-spin |
| Wasm 模块崩溃 | 内存不足/panic | 检查 Pod 日志 | 增加内存限制 |
| 外部 HTTP 失败 | allowed_outbound_hosts 未配置 | 检查 spin.toml | 添加允许的主机 |
| Registry 推送失败 | 认证问题 | `spin registry login` | 配置 Registry 凭证 |

### 排查流程

```
Spin 应用异常
├── 本地开发问题
│   ├── spin build → 检查编译错误
│   ├── spin up --log-dir ./logs → 查看详细日志
│   ├── 检查 spin.toml 路由配置
│   └── 检查 allowed_outbound_hosts
├── Kubernetes 部署问题
│   ├── kubectl get spinapps → 检查状态
│   ├── kubectl describe spinapp → 查看事件
│   ├── kubectl get pods → 检查 Pod 状态
│   └── kubectl get runtimeclass → 确认运行时
└── 运行时问题
    ├── 检查 Wasm 模块日志
    ├── 检查内存/CPU 限制
    └── 检查外部依赖可达性
```

## 生产案例

### 案例 1: Serverless API 零冷启动

- **场景**: API 服务需要快速响应，但传统容器冷启动 1-2 秒
- **排查**: 容器冷启动导致首次请求延迟高；用户等待体验差
- **方案**: 将 API 服务编译为 Wasm 组件；Spin 运行时亚毫秒冷启动；SpinKube 部署到 K8s
- **效果**: 冷启动从 1.5s 降至 <1ms；P99 延迟降低 80%；资源占用减少 90%

### 案例 2: 边缘 IoT 数据处理

- **场景**: 50 个边缘节点需要运行数据处理逻辑，资源受限 (1C2G)
- **排查**: 传统容器每实例需 128MB+ 内存；边缘节点无法承载多个服务
- **方案**: Spin Wasm 组件每实例仅 5-10MB；单节点运行 20+ 个 Wasm 组件；Redis 触发器驱动事件处理
- **效果**: 单节点服务密度提升 10 倍；内存占用降低 95%

## 对比与替代方案

| 维度 | Spin | wasmCloud | WasmEdge | Knative |
|------|------|-----------|----------|----------|
| 开发模型 | 应用框架 | 分布式平台 | 嵌入式运行时 | 容器 Serverless |
| 触发器 | HTTP/Redis/Timer | NATS | 命令行/API | HTTP/Kafka |
| 冷启动 | <1ms | <1ms | <1ms | ~1s |
| 内存占用 | ~5MB | ~10MB | ~5MB | ~128MB |
| K8s 集成 | SpinKube | Operator | Kubernetes | 原生 |
| 多语言 | ✅ SDK | ✅ WIT | ✅ | ✅ 容器 |
| 适用场景 | Serverless/边缘 | 分布式 Wasm | 嵌入式/AI | 容器 Serverless |

## 检查清单

- [ ] Spin CLI 已安装
- [ ] spin.toml 配置正确
- [ ] allowed_outbound_hosts 已配置 (最小权限)
- [ ] Wasm 模块已优化 (wasm-opt)
- [ ] OCI Registry 认证已配置
- [ ] SpinKube RuntimeClass 已配置 (K8s)
- [ ] 资源限制已设置
- [ ] 监控覆盖应用健康状态

## 参考链接

- [[containerd]]
- [[23-实体/08-交付与制品/argocd.md|argocd]]
- [[operator-pattern]]

## Related

- [[spinkube]] — SpinKube
- [[wasmedge]] — WasmEdge
- [[23-实体/15-参考与索引/cncf-runtime.md|cncf-runtime]] — CNCF 容器运行时与工具链项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/03-运行时/wasmcloud.md|wasmcloud]] — wasmCloud

<!-- risk-assessed -->
