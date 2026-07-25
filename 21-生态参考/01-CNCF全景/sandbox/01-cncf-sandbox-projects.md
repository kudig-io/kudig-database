---
title: CNCF Sandbox Projects
description: CNCF 沙箱项目参考 — 早期创新项目，代表云原生技术前沿方向
summary: CNCF 沙箱阶段项目全景，涵盖 Wasm、eBPF、AI 基础设施、供应链安全等前沿领域
category: reference
tags:
- cncf
- sandbox
- wasm
- supply-chain
- ai-infrastructure
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
---
# CNCF 沙箱项目 Sandbox

> CNCF 最早期阶段项目——创新实验场，代表云原生未来方向。

## 前沿领域分布

| 领域 | 代表项目 | 创新点 |
|------|----------|--------|
| WebAssembly | WasmEdge, wasmCloud, Spin | 轻量级沙箱运行时 |
| AI/ML | KubeRay, KServe, OpenFGA | GPU 调度/模型服务 |
| 供应链安全 | Sigstore, in-toto, SLSA | 软件供应链完整性 |
| eBPF | Inspektor Gadget, Parca | 内核级可观测/Profiling |
| 平台工程 | Radius, Score, Oras | 应用抽象/制品管理 |
| 数据 | OpenFeature, CloudEvents | 特性标志/事件标准 |
| 网络 | Gateway API, Cilium(已毕业) | 下一代入口/网络 |

## 重点项目解析

### WasmEdge — WebAssembly 运行时

```yaml
# K8s 中运行 Wasm 工作负载（通过 containerd shim）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmedge
handler: wasmedge
---
apiVersion: v1
kind: Pod
metadata:
  name: wasm-hello
  annotations:
    module.wasm.image/variant: compat-smart
spec:
  runtimeClassName: wasmedge
  containers:
    - name: hello
      image: wasmedge/example-wasi:latest
      resources:
        limits:
          cpu: 100m
          memory: 64Mi
```

**Wasm vs 容器对比：**

| 维度 | 容器 | WebAssembly |
|------|------|-------------|
| 启动时间 | 100ms-1s | <1ms |
| 镜像大小 | 50MB-1GB | 1MB-10MB |
| 隔离性 | 命名空间/cgroup | 沙箱（能力模型） |
| 语言支持 | 任意 | Rust/C/Go/JS（编译到Wasm） |
| 适用场景 | 通用服务 | 边缘/插件/Serverless |

### Sigstore — 软件供应链签名

```bash
# 使用 cosign 签名容器镜像
cosign sign --key cosign.key registry.example.com/app:v1.0

# 验证签名
cosign verify --key cosign.pub registry.example.com/app:v1.0

# 无密钥签名（OIDC）
cosign sign registry.example.com/app:v1.0
# 自动使用 GitHub/Google OIDC 身份
```

```yaml
# Kyverno 验证镜像签名策略
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  rules:
    - name: verify-signature
      match:
        resources:
          kinds:
            - Pod
      verifyImages:
        - imageReferences:
            - "registry.example.com/*"
          attestors:
            - entries:
                - keys:
                    publicKeys: |-
                      -----BEGIN PUBLIC KEY-----
                      MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
                      -----END PUBLIC KEY-----
```

### KubeRay — AI 工作负载编排

```yaml
apiVersion: ray.io/v1
kind: RayCluster
metadata:
  name: llm-training
spec:
  rayVersion: '2.9.0'
  headGroupSpec:
    rayStartParams:
      dashboard-host: '0.0.0.0'
    template:
      spec:
        containers:
          - name: ray-head
            image: rayproject/ray-ml:2.9.0-gpu
            resources:
              limits:
                nvidia.com/gpu: 1
                memory: 32Gi
  workerGroupSpecs:
    - groupName: gpu-workers
      replicas: 4
      minReplicas: 2
      maxReplicas: 8
      rayStartParams: {}
      template:
        spec:
          containers:
            - name: ray-worker
              image: rayproject/ray-ml:2.9.0-gpu
              resources:
                limits:
                  nvidia.com/gpu: 4
                  memory: 128Gi
```

### Gateway API — 下一代入口

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: production-gateway
spec:
  gatewayClassName: cilium
  listeners:
    - name: https
      protocol: HTTPS
      port: 443
      tls:
        certificateRefs:
          - name: tls-cert
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-routes
spec:
  parentRefs:
    - name: production-gateway
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /api/v1
      backendRefs:
        - name: api-service
          port: 8080
          weight: 90
        - name: api-service-canary
          port: 8080
          weight: 10
```

## 沙箱项目评估维度

| 维度 | 评估要点 |
|------|----------|
| 创新性 | 是否解决新问题或提出新范式 |
| 社区活跃度 | 贡献者数量、PR 频率、Issue 响应 |
| 生产就绪度 | 是否有真实生产案例 |
| 治理成熟度 | 是否有完善的治理文档和流程 |
| 安全性 | 是否通过安全审计 |

## 投资关注方向（2024-2026）

1. **AI 基础设施**：KubeRay、KServe、GPU 共享调度
2. **WebAssembly**：边缘计算、插件系统、Serverless
3. **供应链安全**：Sigstore、SLSA、SBOM
4. **平台工程**：Backstage、Radius、内部开发者平台
5. **eBPF 深化**：零侵入可观测、安全、网络加速

## Related

- [[21-生态参考/01-CNCF全景/graduated/index.md|毕业项目]]
- [[21-生态参考/01-CNCF全景/incubating/index.md|孵化项目]]
- [[15-AI基础设施/index.md|AI 基础设施]]
