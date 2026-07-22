---
title: Confidential Containers (CoCo)
description: '## 概述'
summary: 'Confidential Containers (CoCo) 是一个为 Kubernetes 提供机密计算能力的项目，使容器工作负载能够在硬件 TEE（可信执行环境）中运行。通过利用 AMD SEV、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据免受云提供商、管理员和其他特权软件的访问。'
category: entities
tags:
- k8s
- cncf
- security
- confidential-containers
- opa
- crd
- operator
- agent
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Confidential Containers (CoCo) 是什么
- 如何 Confidential Containers (CoCo)
trigger_keywords:
- Confidential
- Containers
- CoCo
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Confidential Containers (CoCo)

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust, Go

## 概述

Confidential Containers（CoCo）是 CNCF Sandbox 项目，为 Kubernetes 提供机密计算（Confidential Computing）能力，使容器工作负载能够在硬件 TEE（Trusted Execution Environment）中运行。通过利用 AMD SEV-SNP、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据（Data-in-Use）免受云提供商、管理员和其他特权软件的访问。它解决了云原生场景下"数据在处理时"的安全保护问题。

## 核心特性

- **硬件 TEE 隔离**: 支持 AMD SEV-SNP、Intel TDX、IBM SE 等硬件隔离技术
- **加密镜像**: 容器镜像在 TEE 内解密，防止主机窥探镜像内容
- **远程证明**: 启动前验证 TEE 可信状态（Attestation）
- **KBS 密钥管理**: Key Broker Service 安全分发镜像解密密钥
- **策略框架**: 基于 OPA 的证明策略评估
- **标准 K8s 接口**: 通过 RuntimeClass 集成，不改变用户工作负载 API

## 架构

CoCo 架构分为 Guest 侧和 Host 侧。Host 侧通过自定义 RuntimeClass（如 `kata-qemu`）与 CRI 兼容的运行时集成。当 Pod 指定 CoCo RuntimeClass 时，containerd/shim 将 Pod 启动在基于 Kata Containers 的 TEE VM 中。Guest 侧包括：confidential-image-rs（拉取并解密镜像）、attestation-agent（执行远程证明获取密钥）、kata-agent（管理容器生命周期）。KBS（Key Broker Service）作为密钥代理，仅在远程证明通过后分发解密密钥。

## Kubernetes 集成

CoCo 通过 Kubernetes RuntimeClass 集成。用户在 Pod Spec 中指定 `runtimeClassName: kata-qemu` 即可将 Pod 运行在 TEE 中。节点需配置相应的硬件支持和 Kata Containers runtime。CoCo Operator 自动管理节点上的运行时安装和配置。加密镜像通过标准的 OCI Distribution 分发，密钥通过 KBS 在证明通过后注入。支持标准的 Kubernetes Pod API，用户无需修改工作负载定义。

## 生产使用场景

1. **金融数据处理**: 在公有云上安全处理敏感金融数据，TEE 保证数据不泄露
2. **医疗数据隐私**: 符合 HIPAA 合规要求，在云端安全处理医疗记录
3. **多方安全计算**: 多个组织在不暴露原始数据的情况下进行联合分析
4. **AI 模型保护**: 保护专有 AI 模型权重不被云提供商或攻击者获取

## 安装与配置

```bash
# 安装 CoCo Operator
kubectl apply -k "github.com/confidential-containers/operator/config/release?ref=v0.11.0"
# 部署 CoCo Runtime
kubectl apply -f - <<EOF
apiVersion: confidentialcontainers.org/v1beta1
kind: CcRuntime
metadata:
  name: coco-runtime
  namespace: confidential-containers-system
spec:
  runtimeName: kata
  config:
    attestation:
      url: "http://kbs:8080"
    image:
      encrypted: true
EOF
# 使用 TEE 运行 Pod
kubectl run secret-app --image=encrypted-app:latest \
  --overrides='{"spec":{"runtimeClassName":"kata-qemu"}}'
```

### 加密镜像构建

```bash
# 使用 image-rs 加密镜像
image-rs encrypt \
  --source docker.io/myorg/secret-app:v1 \
  --target registry.internal/encrypted/secret-app:v1 \
  --key-provider ocicrypt \
  --recipient jwe:pubkey.pem

# 配置 KBS 密钥分发
curl -X POST http://kbs:8080/kbs/v0/resource \
  -d '{"path": "/default/key/secret-app", "data": "<base64-key>"}'
```

### 证明策略配置

```yaml
# OPA 证明策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: attestation-policy
data:
  policy.rego: |
    package policy
    default allow = false
    allow {
      input.tee == "sev-snp"
      input.measurement == "expected-hash"
      input.timestamp > time.now_ns() - 3600000000000
    }
```

## 运维操作

```bash
# 🟢 查看 CoCo Runtime 状态
kubectl get ccruntime -n confidential-containers-system

# 🟢 查看 TEE Pod 状态
kubectl get pods -o custom-columns=NAME:.metadata.name,RUNTIME:.spec.runtimeClassName

# 🟢 检查节点 TEE 硬件支持
kubectl get nodes -o custom-columns=NAME:.metadata.name,TEE:.status.allocatable.'tee\.confidentialcontainers\.org'

# 🟢 查看 KBS 状态
kubectl get pods -l app=kbs -n confidential-containers-system

# 🟢 查看证明日志
kubectl logs -n confidential-containers-system -l app=attestation-agent

# 🟡 更新证明策略
kubectl apply -f attestation-policy.yaml
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending | 节点无 TEE 硬件 | `kubectl describe pod` | 调度到支持 TEE 的节点 |
| 镜像解密失败 | KBS 不可达 | `kubectl logs attestation-agent` | 检查 KBS Service 地址 |
| 证明失败 | TEE 度量不匹配 | 检查证明日志 | 更新期望的度量哈希 |
| 性能下降 | TEE 加密开销 | `kubectl top pod` | 评估可接受的性能损失 |
| RuntimeClass 不存在 | Operator 未安装 | `kubectl get runtimeclass` | 重新安装 CoCo Operator |

### 排查流程

```
CoCo 异常
├─ Pod 无法调度？
│  ├─ 无 TEE 节点 → 检查硬件支持 (AMD SEV/Intel TDX)
│  ├─ RuntimeClass 缺失 → 检查 Operator 状态
│  └─ 资源不足 → TEE VM 需要额外内存
├─ 镜像拉取失败？
│  ├─ 解密失败 → 检查 KBS 连接和密钥
│  ├─ 证明失败 → 检查 TEE 度量值
│  └─ 网络问题 → 检查 Registry 连通性
└─ 运行时异常？
   ├─ Kata 错误 → 检查 kata-runtime 日志
   └─ TEE 崩溃 → 检查固件版本和 BIOS 设置
```

## 生产案例

### 案例 1: 金融数据云端安全处理

**场景**: 某银行需在公有云上处理客户敏感数据，监管要求数据在处理时不得被云商访问。

**方案**:
1. 使用 AMD SEV-SNP 节点部署 CoCo
2. 数据处理应用以加密镜像形式部署
3. 远程证明确保 TEE 可信后才分发解密密钥
4. 审计日志记录所有证明事件

**效果**: 满足监管合规要求，数据在处理时全程加密，云商无法访问明文。

### 案例 2: AI 模型权重保护

**场景**: AI 公司需在客户环境部署推理服务，但不能暴露模型权重。

**方案**:
1. 模型文件加密打包到容器镜像
2. 使用 CoCo 在客户环境的 TEE 中解密运行
3. 客户无法提取模型权重

**效果**: 模型权重全程保护，客户可使用但无法获取原始模型。

## 对比与替代方案

| 维度 | CoCo | Enarx | Gramine | Occlum |
|------|------|-------|---------|--------|
| K8s 原生 | ✅ | ❌ | ❌ | ❌ |
| 硬件支持 | SEV/TDX/SE | 多架构 | 仅 x86 | 仅 SGX |
| 镜像加密 | ✅ | ❌ | ❌ | ❌ |
| 远程证明 | ✅ | ✅ | 部分 | ✅ |
| 性能开销 | 中 (VM) | 低 | 低 | 低 |
| 成熟度 | 中 | 低 | 中 | 中 |

## 检查清单

- [ ] 节点硬件支持 TEE（AMD SEV-SNP / Intel TDX）
- [ ] BIOS 中已启用 TEE 功能
- [ ] CoCo Operator 已安装并运行
- [ ] RuntimeClass 已创建（kata-qemu）
- [ ] KBS 已部署并可访问
- [ ] 证明策略已配置并测试
- [ ] 加密镜像构建流程已建立
- [ ] 性能开销已评估可接受

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **CoCo** | K8s 原生、标准化 | 性能开销、硬件依赖 |
| Enarx | 架构灵活、跨平台 | 尚不成熟、社区较小 |
| Gramine | 不需要硬件 TEE | 仅 x86、非 K8s 原生 |
| Occlum | 高性能 LibOS | 仅 Intel SGX |

## 架构定位

在 CNCF 生态中，CoCo 属于 **Security** 类别，是机密计算在 Kubernetes 上的标准化入口。它与 Kata Containers、OPA、SPIFFE 等项目协同工作。

## 参考链接

- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[spire]] — SPIRE
- [[akri]] — Akri
- [[实体/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- confidential-containers
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[实体/tetragon.md|Tetragon]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
