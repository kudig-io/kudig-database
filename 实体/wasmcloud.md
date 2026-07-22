---
title: wasmCloud (entities)
description: '## 概述'
summary: 'wasmCloud 是用于构建分布式 WebAssembly 应用的平台。它提供安全、可移植的应用运行环境，通过能力模型（Capability Model）实现组件与外部资源的解耦，支持跨云、边缘和本地的统一部署。'
category: entities
tags:
- k8s
- cncf
- runtime
- wasmcloud
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
- wasmCloud 是什么
- 如何 wasmCloud
trigger_keywords:
- wasmCloud
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的可执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# wasmCloud

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Rust

## 概述

wasmCloud 是由 Cosmonic 公司开发的开源分布式应用平台，专注于使用 WebAssembly（Wasm）组件构建可移植、安全的云原生应用。2023 年从 CNCF Sandbox 毕业至 Incubating 阶段。wasmCloud 的核心理念是通过 **Capability-based Security（能力安全模型）** 将业务逻辑（Component）与外部资源访问（Provider）彻底解耦——业务代码运行在 Wasm 沙箱中，通过声明式的能力合约（Capability Contract）获得对数据库、消息队列、HTTP 等资源的访问权限。

wasmCloud 使用 NATS 作为通信总线（Lattice），支持组件在任意节点间自由迁移，实现"一次编写，到处运行"的真正可移植性。应用通过 OCI 镜像格式分发，版本管理和供应链安全开箱即用。

## Key Features

- **WebAssembly 运行时**：基于 wasmtime 的安全沙箱执行，毫秒级冷启动
- **能力模型（Capability Model）**：组件通过接口契约（如 `wasmcloud:httpserver`）声明所需能力，由 Provider 提供实际实现
- **位置透明**：组件可在任意节点运行，Lattice 网络自动路由消息
- **热更新**：无停机更新组件和配置，旧实例优雅终止
- **多语言支持**：Rust、Go、JavaScript、Python、C# 等，通过 WIT（Wasm Interface Types）互操作
- **分布式网络**：NATS 消息总线（Lattice）连接所有节点，支持跨云、边缘部署

## Architecture

wasmCloud 由 **Host**（运行 Wasm 组件和 Provider 的节点）、**Lattice**（基于 NATS 的通信网格）、**Component**（业务逻辑的 Wasm 模块）和 **Provider**（提供外部资源能力的原生进程，如 HTTP Server、PostgreSQL、Kafka）构成。组件通过 WIT 接口与 Provider 通信，Provider 代理实际的 I/O 操作。wascL（wasmCloud Control Interface）提供声明式 API 管理 Lattice 中的所有组件和 Provider 实例。

## K8s 集成

wasmCloud 通过 **wasmCloud Operator** 或 Helm Chart 部署到 Kubernetes。Operator 管理 wasmCloud Host 的 Deployment，自动协调组件和 Provider 的期望状态。也支持通过 NATS JetStream 持久化 Lattice 状态。组件以 OCI 镜像存储在标准 Registry 中，Host 通过拉取镜像部署组件。

## 生产部署要点

- **组件轻量化**：保持组件专注于业务逻辑，外部资源访问通过 Provider
- **版本管理**：使用 OCI 镜像仓库管理组件版本
- **测试策略**：使用 `wash` 测试工具验证组件行为
- **监控集成**：配置 OTEL 导出追踪和指标
- **安全边界**：利用 Wasm 沙箱和能力模型实现最小权限

## 生产场景

1. **边缘 IoT 数据聚合**：轻量 Wasm 组件在边缘节点处理传感器数据，通过 Lattice 回传
2. **跨云可移植微服务**：同一 Wasm 组件在 AWS、Azure、本地数据中心一致运行
3. **插件化 SaaS 平台**：第三方插件在安全沙箱中运行，宿主通过能力模型控制权限
4. **实时事件处理**：Kafka Provider 驱动的流处理组件

## 安装与配置

```bash
# 安装 wash CLI
curl -sSf https://pkgwasmclouddev.s3.amazonaws.com/install.sh | bash
wash --version

# 启动本地 wasmCloud Host (开发环境)
wash up -d

# 部署一个 HTTP 组件
wash start component ghcr.io/wasmcloud/components/http-hello-world-rust:0.1.0 hello

# 启动 HTTP Server Provider
wash start provider wasmcloud/http-server:0.21.0 httpserver

# 声明能力合约 (Link)
wash link put hello httpserver wasmcloud:httpserver/incoming-handler \
  ADDRESS=0.0.0.0:8080

# 查看运行状态
wash get inventory
wash get claims

# 停止组件
wash stop component hello
```

```yaml
# Kubernetes 部署 (Helm)
# helm install wasmcloud wasmcloud/wasmcloud -n wasmcloud --create-namespace
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasmcloud-host
  namespace: wasmcloud
spec:
  replicas: 3
  selector:
    matchLabels:
      app: wasmcloud-host
  template:
    metadata:
      labels:
        app: wasmcloud-host
    spec:
      containers:
        - name: wasmcloud
          image: wasmcloud/wasmcloud:1.0.0
          env:
            - name: WASMCLOUD_LATTICE
              value: "production"
            - name: WASMCLOUD_RPC_HOST
              value: "nats.wasmcloud.svc.cluster.local"
            - name: WASMCLOUD_RPC_PORT
              value: "4222"
            - name: WASMCLOUD_CTL_HOST
              value: "nats.wasmcloud.svc.cluster.local"
            - name: WASMCLOUD_CTL_PORT
              value: "4222"
          ports:
            - containerPort: 8080
          resources:
            requests:
              cpu: 250m
              memory: 256Mi
            limits:
              cpu: "1"
              memory: 512Mi
---
apiVersion: v1
kind: Service
metadata:
  name: nats
  namespace: wasmcloud
spec:
  selector:
    app: nats
  ports:
    - port: 4222
      name: client
    - port: 6222
      name: cluster
```

## 运维操作

```bash
# 🟢 检查 wasmCloud Host 状态
kubectl get pods -n wasmcloud -l app=wasmcloud-host
wash get inventory --lattice production

# 🟢 查看运行中的组件和 Provider
wash get components
wash get providers
wash get links

# 🟢 查看组件日志
wash logs component <component-id>

# 🟡 扩展组件实例数
wash scale component hello 5

# 🟡 更新组件版本 (热更新)
wash update component hello ghcr.io/myorg/hello:0.2.0

# 🟢 检查 Lattice 连接状态
wash get hosts
nats server info -s nats://nats.wasmcloud.svc:4222

# 🟡 停止组件
wash stop component <component-id>

# 🔴 停止 Provider (影响所有关联组件)
wash stop provider <provider-id>
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 组件启动失败 | OCI 镜像拉取失败 | `wash get inventory` | 检查 Registry 认证/网络 |
| Lattice 连接失败 | NATS 不可达 | `nats server info` | 检查 NATS Service/网络 |
| 组件无法访问外部资源 | Link 未配置 | `wash get links` | 配置能力合约 Link |
| HTTP 请求无响应 | Provider 未启动 | `wash get providers` | 启动 HTTP Server Provider |
| 组件崩溃 | Wasm 运行时错误 | `wash logs component` | 检查组件代码/资源限制 |
| 跨节点通信失败 | Lattice 网络分区 | `wash get hosts` | 检查 NATS 集群状态 |

### 排查流程

```
wasmCloud 组件异常
├── 组件未运行
│   ├── wash get inventory → 检查组件状态
│   ├── 检查 OCI 镜像是否可拉取
│   ├── 检查 Host 资源是否充足
│   └── wash logs → 查看启动错误
├── 组件运行但功能异常
│   ├── wash get links → 检查能力合约
│   ├── wash get providers → 检查 Provider 状态
│   ├── 检查 NATS 消息路由
│   └── 检查组件日志
└── Lattice 网络问题
    ├── wash get hosts → 检查节点状态
    ├── nats server info → 检查 NATS 连接
    └── 检查防火墙/网络策略
```

## 生产案例

### 案例 1: 边缘 IoT 数据聚合

- **场景**: 50 个边缘节点需要处理传感器数据，资源受限 (2C4G)
- **排查**: 传统容器方案每节点需 1GB+ 内存，边缘节点无法承载
- **方案**: 使用 wasmCloud 部署轻量 Wasm 组件 (每组件 <10MB)；NATS Lattice 连接所有边缘节点；数据聚合后回传云端
- **效果**: 每节点内存占用从 1GB 降至 100MB；冷启动 <5ms；统一管理 50 个边缘节点

### 案例 2: 插件化 SaaS 平台安全沙箱

- **场景**: SaaS 平台需要运行第三方插件，但担心恶意代码
- **排查**: 传统容器隔离不够彻底，插件可能访问宿主资源
- **方案**: 第三方插件编译为 Wasm 组件；通过能力模型仅授予必要权限；无网络/文件系统访问除非显式授权
- **效果**: 插件在完全沙箱中运行；零安全事件；插件启动时间 <10ms

## 对比与替代方案

| 维度 | wasmCloud | Spin/SpinKube | Dapr | Knative |
|------|-----------|---------------|------|----------|
| 运行时 | Wasm (wasmtime) | Wasm (wasmtime) | Sidecar 进程 | 容器 |
| 能力模型 | ✅ Contract | ✅ Trigger | ✅ Building Blocks | ❌ |
| 分布式 | ✅ Lattice/NATS | ❌ 单机 | ❌ | ✅ |
| 冷启动 | <5ms | <5ms | ~100ms | ~1s |
| 多语言 | ✅ WIT | ✅ WIT | ✅ SDK | ✅ 容器 |
| K8s 集成 | Operator | SpinKube | Dapr Operator | 原生 |
| 适用场景 | 分布式 Wasm | 单机 Wasm | 微服务抽象 | Serverless |

## 检查清单

- [ ] wasmCloud Host 所有 Pod Running
- [ ] NATS Lattice 连接正常
- [ ] 组件和 Provider 已正确部署
- [ ] 能力合约 (Link) 配置正确
- [ ] OCI Registry 认证配置正确
- [ ] 监控覆盖 Host 和组件状态
- [ ] 组件版本管理策略已定义
- [ ] 安全边界验证 (能力模型最小权限)

## 参考链接

- [[实体/vault.md|vault]]
- [[deployment]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[ko]] — ko
- [[openfunction]] — OpenFunction
- [[kubevirt]] — KubeVirt
- [[nats]] — NATS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference

<!-- risk-assessed -->
