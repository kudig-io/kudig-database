---
title: Kubewarden
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- docker
- opa
- statefulset
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kubewarden 是什么
- 如何 Kubewarden
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kubewarden
- cncf
- landscape
---


# Kubewarden

> **成熟度**: Sandbox | **加入时间**: 2022-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.kubewarden.io |
| **GitHub** | https://github.com/kubewarden |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust, Go |
| **CNCF 分类** | Security & Compliance |
| **维护组织** | SUSE |

---

## 项目概述

Kubewarden 是一个 Kubernetes 策略引擎，使用 WebAssembly (Wasm) 运行准入策略。它允许使用任何编译为 Wasm 的编程语言 (Rust、Go、C#、Swift 等) 编写策略，并通过 OCI 镜像仓库分发。Kubewarden 支持动态准入控制和审计模式。

---

## 核心特性

- **WebAssembly 策略**: 使用 Wasm 编写和运行策略
- **多语言支持**: Rust、Go、C#、Swift、Rego 等
- **OCI 分发**: 策略通过 OCI 仓库分发
- **审计模式**: 不阻止请求，只记录违规
- **策略组**: 组合多个策略为逻辑组
- **上下文感知**: 策略可查询集群状态
- **兼容 Rego**: 复用 OPA/Gatekeeper 策略

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   Kubewarden Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  Kubernetes API Server                     │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │       Admission Webhook Configuration                │ │   │
│  │  │   (Validating / Mutating Webhooks)                   │ │   │
│  │  └─────────────────────────┬───────────────────────────┘ │   │
│  └────────────────────────────┼────────────────────────────┘   │
│                               │                                 │
│  ┌────────────────────────────▼────────────────────────────┐   │
│  │              Kubewarden Controller                        │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │ Manages ClusterAdmissionPolicy / AdmissionPolicy    │ │   │
│  │  │ Reconciles PolicyServer deployments                  │ │   │
│  │  │ Handles policy lifecycle and audit                   │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └────────────────────────────┬────────────────────────────┘   │
│                               │                                 │
│  ┌────────────────────────────▼────────────────────────────┐   │
│  │                  PolicyServer (Deployment)                │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Wasm Runtime (wasmtime)                  │ │   │
│  │  │                                                       │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │ Policy A    │  │ Policy B    │  │ Policy C   │  │ │   │
│  │  │  │ (Wasm)      │  │ (Wasm)      │  │ (Wasm)     │  │ │   │
│  │  │  │ Rust-based  │  │ Go-based    │  │ Rego-based │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  │                                                       │ │   │
│  │  │  ┌─────────────────────────────────────────────┐    │ │   │
│  │  │  │            Context-Aware Cache              │    │ │   │
│  │  │  │  Namespaces │ Services │ Ingresses │ ...    │    │ │   │
│  │  │  └─────────────────────────────────────────────┘    │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────┘  │
│                               │                                 │
│  ┌────────────────────────────▼────────────────────────────┐   │
│  │                    OCI Registry                           │   │
│  │  ┌──────────────────────────────────────────────────┐   │   │
│  │  │  Policy Artifacts (Wasm modules as OCI images)   │   │   │
│  │  │  ghcr.io/kubewarden/policies/pod-privileged:v0.3 │   │   │
│  │  └──────────────────────────────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Controller** | 管理策略 CRD 和 PolicyServer 生命周期 |
| **PolicyServer** | 运行 Wasm 策略的准入 Webhook 服务 |
| **wasmtime** | WebAssembly 运行时引擎 |
| **Policy Artifacts** | OCI 镜像格式的策略模块 |

---

## 快速开始

### Helm 安装

```bash
# 安装 cert-manager (前置依赖)
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/latest/download/cert-manager.yaml

# 安装 Kubewarden CRDs
helm repo add kubewarden https://charts.kubewarden.io
helm repo update

helm install kubewarden-crds kubewarden/kubewarden-crds

# 安装 Controller
helm install kubewarden-controller kubewarden/kubewarden-controller \
  --namespace kubewarden \
  --create-namespace

# 安装默认 PolicyServer
helm install kubewarden-defaults kubewarden/kubewarden-defaults \
  --namespace kubewarden

# 验证
kubectl get pods -n kubewarden
kubectl get policyserver
```

---

## 策略配置

### ClusterAdmissionPolicy (集群级别)

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: no-privileged-pods
spec:
  policyServer: default
  module: ghcr.io/kubewarden/policies/pod-privileged:v0.3.2
  rules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      resources: ["pods"]
      operations: ["CREATE", "UPDATE"]
  mutating: false
  settings: {}
```

### AdmissionPolicy (命名空间级别)

```yaml
apiVersion: policies.kubewarden.io/v1
kind: AdmissionPolicy
metadata:
  name: safe-labels
  namespace: production
spec:
  policyServer: default
  module: ghcr.io/kubewarden/policies/safe-labels:v0.2.0
  rules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      resources: ["pods"]
      operations: ["CREATE"]
  mutating: false
  settings:
    mandatory_labels:
      - owner
      - team
    constrained_labels:
      environment:
        - production
        - staging
```

### 审计模式

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: audit-only-policy
spec:
  policyServer: default
  module: ghcr.io/kubewarden/policies/pod-privileged:v0.3.2
  mode: monitor  # 审计模式，不阻止请求
  rules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      resources: ["pods"]
      operations: ["CREATE"]
```

---

## 常用策略

### 镜像仓库限制

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: trusted-registries
spec:
  module: ghcr.io/kubewarden/policies/trusted-repos:v0.2.0
  rules:
    - apiGroups: ["", "apps"]
      apiVersions: ["v1"]
      resources: ["pods", "deployments", "replicasets", "statefulsets"]
      operations: ["CREATE", "UPDATE"]
  settings:
    registries:
      - "ghcr.io/"
      - "docker.io/library/"
      - "registry.internal.company.com/"
```

### 资源限制强制

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: resource-limits
spec:
  module: ghcr.io/kubewarden/policies/container-resources:v0.1.0
  rules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      resources: ["pods"]
      operations: ["CREATE"]
  settings:
    requireLimits: true
    requireRequests: true
    maxLimitCpu: "4"
    maxLimitMemory: "8Gi"
```

### Ingress 主机名唯一性

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: unique-ingress-host
spec:
  module: ghcr.io/kubewarden/policies/unique-ingress:v0.1.0
  rules:
    - apiGroups: ["networking.k8s.io"]
      apiVersions: ["v1"]
      resources: ["ingresses"]
      operations: ["CREATE", "UPDATE"]
  contextAwareResources:
    - apiVersion: networking.k8s.io/v1
      kind: Ingress
```

---

## 策略组

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: security-policy-group
spec:
  module: ghcr.io/kubewarden/policies/policy-group:v0.1.0
  rules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      resources: ["pods"]
      operations: ["CREATE"]
  settings:
    policies:
      - url: ghcr.io/kubewarden/policies/pod-privileged:v0.3.2
        settings: {}
      - url: ghcr.io/kubewarden/policies/safe-labels:v0.2.0
        settings:
          mandatory_labels: ["owner"]
    expression: "pod_privileged() && safe_labels()"
```

---

## 自定义 PolicyServer

```yaml
apiVersion: policies.kubewarden.io/v1
kind: PolicyServer
metadata:
  name: high-priority
spec:
  image: ghcr.io/kubewarden/policy-server:latest
  replicas: 3
  serviceAccountName: policy-server
  resources:
    limits:
      cpu: "1"
      memory: 512Mi
    requests:
      cpu: 250m
      memory: 256Mi
  env:
    - name: KUBEWARDEN_LOG_LEVEL
      value: info
  securityContexts:
    container:
      runAsNonRoot: true
      readOnlyRootFilesystem: true
```

---

## 编写自定义策略 (Rust)

### 项目结构

```bash
cargo init --lib my-policy
cd my-policy
```

### Cargo.toml

```toml
[package]
name = "my-policy"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
kubewarden-policy-sdk = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
```

### 策略代码

```rust
// src/lib.rs
use kubewarden_policy_sdk::wapc_guest as wapc;
use kubewarden_policy_sdk::{protocol_version_guest, validate_settings, logging};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Default, Debug)]
struct Settings {
    denied_names: Vec<String>,
}

#[no_mangle]
pub extern "C" fn wapc_init() {
    wapc::register_function("validate", validate);
    wapc::register_function("validate_settings", |payload| {
        validate_settings::<Settings>(payload)
    });
    wapc::register_function("protocol_version", protocol_version_guest);
}

fn validate(payload: &[u8]) -> wapc::CallResult {
    let validation_request = kubewarden_policy_sdk::request::ValidationRequest::<Settings>::new(payload)?;
    let pod = validation_request.request.object;
    let settings = validation_request.settings;
    
    let pod_name = pod["metadata"]["name"].as_str().unwrap_or("");
    
    if settings.denied_names.contains(&pod_name.to_string()) {
        return kubewarden_policy_sdk::reject_request(
            Some(format!("Pod name '{}' is not allowed", pod_name)),
            None, None, None
        );
    }
    
    kubewarden_policy_sdk::accept_request()
}
```

### 构建和发布

```bash
# 编译为 Wasm
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release

# 使用 kwctl 测试
kwctl run -r test-request.json -s '{"denied_names":["bad-pod"]}' \
  target/wasm32-wasi/release/my_policy.wasm

# 推送到 OCI 仓库
kwctl push target/wasm32-wasi/release/my_policy.wasm \
  ghcr.io/my-org/my-policy:v0.1.0
```

---

## kwctl CLI

```bash
# 安装 kwctl
curl -fsSL https://github.com/kubewarden/kwctl/releases/latest/download/kwctl-linux-amd64 -o kwctl
chmod +x kwctl && sudo mv kwctl /usr/local/bin/

# 拉取策略
kwctl pull ghcr.io/kubewarden/policies/pod-privileged:v0.3.2

# 检查策略
kwctl inspect ghcr.io/kubewarden/policies/pod-privileged:v0.3.2

# 测试策略
kwctl run -r request.json ghcr.io/kubewarden/policies/pod-privileged:v0.3.2

# 生成 scaffold
kwctl scaffold manifest -t ClusterAdmissionPolicy \
  ghcr.io/kubewarden/policies/pod-privileged:v0.3.2
```

---

## 最佳实践

1. **渐进部署**: 先在 monitor 模式运行策略
2. **策略复用**: 优先使用官方策略库
3. **PolicyServer 隔离**: 为关键策略使用独立 PolicyServer
4. **版本锁定**: 使用精确版本标签引用策略
5. **测试**: 使用 kwctl 在部署前充分测试
6. **监控**: 监控 PolicyServer 资源使用和延迟

---

## 参考资源

- [官方文档](https://docs.kubewarden.io)
- [GitHub](https://github.com/kubewarden)
- [策略库](https://artifacthub.io/packages/search?kind=13)
- [Rust SDK](https://docs.rs/kubewarden-policy-sdk)
- [策略编写指南](https://docs.kubewarden.io/writing-policies)

---

**维护者**: Kudig Team | **许可证**: MIT
