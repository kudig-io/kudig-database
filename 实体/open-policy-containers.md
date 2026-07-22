---
title: Open Policy Containers (OPCR)
description: '## 概述'
summary: 'Open Policy Containers (OPCR) 是一个将 OPA (Open Policy Agent) 策略打包为 OCI 兼容镜像并分发的标准和工具集。'
category: entities
tags:
- k8s
- cncf
- policy
- open-policy-containers
- opa
- crd
- operator
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Policy Containers (OPCR) 是什么
- 如何 Open Policy Containers (OPCR)
trigger_keywords:
- Open
- Policy
- Containers
- OPCR
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[实体/open-policy-containers.md|Open Policy Containers]] (OPCR)

> **CNCF 状态**: Sandbox | **类别**: Policy | **主要语言**: Go

## 概述

Open Policy Containers (OPCR) 是一个将 OPA (Open Policy Agent) 策略打包为 OCI 兼容镜像并分发的标准和工具集。它定义了 Policy as Code 的打包格式，使安全策略可以像容器镜像一样存储在任意 OCI Registry 中，并支持签名、版本化和分发。OPCR 让安全策略的管理和部署与云原生工作流无缝集成——策略开发者使用 `opcr build/push` 打包和分发 Rego 策略，运维人员通过 `opcr pull` 拉取策略到 OPA 部署中。OPCR 还支持基于 cosign 的策略签名验证，确保策略在分发过程中不被篡改。

## 核心能力

- **OCI 策略打包**: 将 Rego 策略文件打包为 OCI 兼容镜像
- **标准 Registry 兼容**: 推送到 Docker Hub、Harbor、ECR、GCR 等标准 OCI Registry
- **版本管理**: 通过 OCI 标签实现策略版本控制（v1.0.0、latest 等）
- **签名验证**: 基于 cosign 的策略签名，防止策略被篡改
- **opcr CLI**: 类似 Docker 的 CLI 接口（build/push/pull/sign）
- **Kubernetes 集成**: 通过 Operator 或 Gatekeeper 自动同步策略镜像

## 架构

OPCR 采用 OCI 制品规范打包策略：

- **opcr CLI**: 命令行工具，管理策略镜像的 build/push/pull/sign
- **Policy Bundle**: 包含 Rego 文件和 manifest 的 OCI 层
- **OCI Manifest**: 描述策略镜像的 OCI 1.1 manifest
- **Cosign**: 策略签名工具（可选，基于 Sigstore）
- **Registry**: 标准 OCI Registry 存储（Harbor/Docker Hub/ECR）
- **Policy Controller (OPA/Gatekeeper)**: 消费策略镜像的运行时

打包流程：`Rego 文件 → opcr build → OCI 镜像 → opcr push → Registry → opcr pull → OPA/Gatekeeper`

## K8s 集成

OPCR 打包的策略镜像可以被 Kubernetes 中的 OPA 或 Gatekeeper 消费。Gatekeeper 支持从 OCI Registry 拉取策略镜像作为 ConstraintTemplate。OPCR 还可以与策略编排工具（如 OPA Operator）集成——Operator 监听 Registry 中的策略镜像更新，自动拉取新版本策略并加载到 OPA。通过 cosign 签名验证确保只有受信任的策略被部署。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的准入控制（Admission Webhook）和供应链安全机制集成。

## 生产场景

1. **策略即代码**: 将安全策略（如"禁止 latest 标签镜像"）打包为 OCI 镜像，GitOps 管理
2. **多集群策略分发**: 通过中央 Registry 向所有集群统一分发安全策略
3. **合规策略管理**: 将 PCI-DSS/SOC 2 合规策略打包版本化，审计可追溯
4. **策略市场**: 在组织内共享通用安全策略包

## 安装与配置

```bash
# 安装 opcr CLI
curl -L https://github.com/opcr-io/opcr/releases/latest/download/opcr_$(uname -s)_$(uname -m).tar.gz | tar xz
mv opcr /usr/local/bin/
opcr version

# 登录 Registry
opcr login myregistry.io -u username -p password
```

### 策略构建与发布

```bash
# 创建策略文件
cat > policy.rego <<'REGO'
package k8srequiredlabels

violation[{"msg": msg, "details": {"missing_labels": missing}}] {
  provided := {label | input.review.object.metadata.labels[label]}
  required := {label | label := input.parameters.labels[_]}
  missing := required - provided
  count(missing) > 0
  msg := sprintf("you must provide labels: %v", [missing])
}
REGO

# 构建策略镜像
opcr build -t myregistry.io/policies/k8s-required-labels:v1.0.0 .

# 推送到 Registry
opcr push myregistry.io/policies/k8s-required-labels:v1.0.0

# 签名（使用 cosign）
cosign sign --key cosign.key myregistry.io/policies/k8s-required-labels:v1.0.0

# 拉取到 OPA/Gatekeeper
opcr pull myregistry.io/policies/k8s-required-labels:v1.0.0
```

### CI/CD 集成示例

```yaml
# GitHub Actions - 策略发布流水线
name: Policy Release
on:
  push:
    paths: ['policies/**']
jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install OPCR
        run: curl -L https://github.com/opcr-io/opcr/releases/latest/download/opcr_linux_amd64.tar.gz | tar xz
      - name: Build & Push
        run: |
          echo "${{ secrets.REGISTRY_TOKEN }}" | opcr login myregistry.io -u bot --password-stdin
          opcr build -t myregistry.io/policies/${{ github.event.repository.name }}:${{ github.sha }} ./policies
          opcr push myregistry.io/policies/${{ github.event.repository.name }}:${{ github.sha }}
      - name: Sign
        run: cosign sign --key env://COSIGN_KEY myregistry.io/policies/${{ github.event.repository.name }}:${{ github.sha }}
```

## 运维操作

```bash
# 🟢 查看本地策略镜像
opcr list

# 🟢 拉取策略到本地
opcr pull myregistry.io/policies/k8s-required-labels:v1.0.0

# 🟢 检查策略内容
opcr inspect myregistry.io/policies/k8s-required-labels:v1.0.0

# 🟡 构建并推送新版本
opcr build -t myregistry.io/policies/k8s-required-labels:v1.1.0 .
opcr push myregistry.io/policies/k8s-required-labels:v1.1.0

# 🟡 删除本地策略镜像
opcr rmi myregistry.io/policies/k8s-required-labels:v1.0.0

# 🔴 删除远程策略（不可恢复）
# 通过 Registry API 删除 manifest
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| build 失败 | Rego 语法错误 | `opa check policy.rego` | 修复 Rego 语法 |
| push 认证失败 | Token 过期 | `opcr login` 重新认证 | 更新 Registry 凭据 |
| pull 超时 | 网络/Registry 不可达 | `curl -v https://myregistry.io/v2/` | 检查网络和防火墙 |
| 签名验证失败 | 密钥不匹配 | `cosign verify --key cosign.pub <image>` | 确认使用正确密钥对 |
| Gatekeeper 未加载策略 | 拉取失败 | `kubectl logs -n gatekeeper-system` | 检查 imagePullSecrets |

```
排查流程:
├── 构建失败
│   ├── opa check *.rego → 语法检查
│   └── 确认 policy.rego 在构建上下文根目录
├── 推送/拉取失败
│   ├── opcr login → 重新认证
│   └── 检查 Registry TLS 证书有效性
└── 策略未生效
    ├── kubectl get constraints → 确认 Constraint 已创建
    └── kubectl logs gatekeeper-controller → 查看加载错误
```

## 生产案例

### 案例 1: 策略版本回滚

- **场景**: 新版标签策略误判合法 Pod，导致部署被拒绝
- **排查**: 新 Rego 规则未考虑带前缀的标签格式
- **方案**: 使用 OCI tag 回滚到上一版本 `opcr pull ...:v1.0.0`；Gatekeeper 重新加载旧策略
- **效果**: 5min 内恢复部署能力，后续增加策略单元测试

### 案例 2: 多集群策略分发

- **场景**: 50+ 集群需要统一安全策略，手动同步成本高
- **方案**: 策略打包为 OCI 镜像，通过 CI/CD 自动推送到各区域 Registry；集群侧 Gatekeeper 定时拉取
- **效果**: 策略同步时间从 2h 缩短到 5min，版本一致性 100%

## 对比

| 特性 | OPCR | OPA Bundles | Kyverno Policies | Gatekeeper Constraints | 适用场景 |
|------|------|-------------|------------------|------------------------|----------|
| OCI 打包 | ✅ | ❌ | ❌ | ❌ | 统一分发 |
| Registry 分发 | ✅ | ⚠️ HTTP | ❌ | ❌ | 多集群 |
| 签名验证 | ✅ cosign | ❌ | ❌ | ❌ | 供应链安全 |
| 版本管理 | ✅ tag | ⚠️ | ❌ | ❌ | 回滚能力 |
| CNCF 状态 | Sandbox | Graduated | Incubating | Incubating | 生态成熟度 |

## 架构定位

在 CNCF 生态中，OPCR 属于 **Policy** 类别，为云原生应用提供策略的 OCI 打包和分发能力。

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/secrets-management.md|secrets-management]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[artifact-hub]] — Artifact Hub
- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- open-policy-containers
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
