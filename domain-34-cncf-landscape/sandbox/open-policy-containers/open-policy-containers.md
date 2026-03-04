# Open Policy Containers (OPCR)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://openpolicycontainers.com/ |
| **GitHub** | https://github.com/opcr-io/policy |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Open Policy Containers (OPCR) 是一个将 OPA (Open Policy Agent) 策略打包为 OCI 兼容镜像并分发的标准和工具集。它定义了 Policy as Code 的打包格式，使策略可以像容器镜像一样存储在任意 OCI Registry 中，并支持签名、版本化和分发。OPCR 让安全策略的管理和部署与云原生工作流无缝集成。

### 核心特性

- **OCI 兼容**: 将 Rego 策略打包为 OCI Artifact，存储在标准 Registry
- **策略签名**: 使用 Sigstore/Cosign 签名策略包，保障策略供应链安全
- **版本管理**: 策略版本通过 OCI tag 管理，支持回滚和审计
- **多种运行时**: 支持 OPA、Gatekeeper、Envoy 等策略消费端
- **CLI 工具**: 提供 policy CLI 进行打包、推送、拉取操作
- **与 OPA 生态集成**: 与 OPA、Gatekeeper、Conftest 等工具无缝协作

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                 Policy Development                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │           Policy Source (.rego files)          │    │
│  │                                                │    │
│  │  package authz                                 │    │
│  │                                                │    │
│  │  default allow = false                         │    │
│  │  allow { ... }                                 │    │
│  │                                                │    │
│  └────────────────────┬─────────────────────────┘    │
│                       │                               │
│  ┌────────────────────▼─────────────────────────┐    │
│  │              OPCR CLI (policy)                 │    │
│  │                                                │    │
│  │  policy build . -t registry.io/policies/authz │    │
│  │  policy push registry.io/policies/authz:v1.0  │    │
│  │  policy sign registry.io/policies/authz:v1.0  │    │
│  │                                                │    │
│  └────────────────────┬─────────────────────────┘    │
│                       │                               │
│  ┌────────────────────▼─────────────────────────┐    │
│  │         OCI Artifact (Policy Package)          │    │
│  │  ┌───────────────────────────────────────┐   │    │
│  │  │ Layer 1: Compiled Policy (bundle.tar.gz)│   │    │
│  │  │ Layer 2: Source (.rego files)          │   │    │
│  │  │ Layer 3: Data (data.json)              │   │    │
│  │  │ Manifest: annotations + signatures     │   │    │
│  │  └───────────────────────────────────────┘   │    │
│  └────────────────────┬─────────────────────────┘    │
└───────────────────────┼──────────────────────────────┘
                        │ Push
                        ▼
             ┌──────────────────┐
             │  OCI Registry     │
             └────────┬─────────┘
                      │ Pull
       ┌──────────────┼──────────────┐
       │              │              │
 ┌─────▼─────┐ ┌──────▼─────┐ ┌─────▼──────┐
 │   OPA      │ │ Gatekeeper │ │  Envoy     │
 │  Server    │ │  (K8s)     │ │  (AuthZ)   │
 └───────────┘ └────────────┘ └────────────┘
```

---

## 快速开始

### 安装 Policy CLI

```bash
# macOS
brew install opcr-io/tap/policy

# Linux
curl -fsSL https://openpolicycontainers.com/install.sh | bash

# 验证安装
policy version
```

### 创建策略项目

```bash
mkdir my-policies && cd my-policies

# 创建 Rego 策略
cat > authz.rego << 'EOF'
package authz

default allow = false

# 允许管理员访问所有资源
allow {
    input.user.roles[_] == "admin"
}

# 允许用户访问自己的资源
allow {
    input.user.id == input.resource.owner
}
EOF

# 创建测试数据
cat > data.json << 'EOF'
{
    "exempt_users": ["system-admin"]
}
EOF
```

### 构建策略包

```bash
# 构建 OCI 镜像
policy build . -t myregistry.io/policies/authz:v1.0

# 查看镜像内容
policy images

# 本地测试策略
policy repl myregistry.io/policies/authz:v1.0
```

### 推送和签名

```bash
# 登录 Registry
policy login myregistry.io

# 推送策略
policy push myregistry.io/policies/authz:v1.0

# 使用 Cosign 签名
policy sign myregistry.io/policies/authz:v1.0
```

### 拉取和使用

```bash
# 拉取策略
policy pull myregistry.io/policies/authz:v1.0

# 验证签名
policy verify myregistry.io/policies/authz:v1.0

# 导出为 OPA bundle
policy save myregistry.io/policies/authz:v1.0 -o authz-bundle.tar.gz
```

---

## 高级功能

### 与 OPA 集成

```bash
# OPA 直接从 Registry 拉取策略
opa run \
  --server \
  --bundle "oci://myregistry.io/policies/authz:v1.0" \
  --addr :8181
```

### 与 Gatekeeper 集成

```yaml
# 创建 External Data Provider 从 OPCR 加载策略
apiVersion: externaldata.gatekeeper.sh/v1alpha1
kind: Provider
metadata:
  name: opcr-provider
spec:
  url: http://opcr-proxy:8080
  timeout: 10
---
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredlabels
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredLabels
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiredlabels
        # 策略从 OPCR bundle 加载
```

### 策略测试

```bash
# 运行策略测试
policy test myregistry.io/policies/authz:v1.0

# 带输入数据测试
policy eval \
  --input '{"user":{"roles":["admin"]},"resource":{"path":"/api"}}' \
  myregistry.io/policies/authz:v1.0 \
  "data.authz.allow"
```

### CI/CD 集成

```yaml
# GitHub Actions 示例
name: Policy CI
on: [push]
jobs:
  build-and-push:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install policy CLI
        run: |
          curl -fsSL https://openpolicycontainers.com/install.sh | bash
      
      - name: Build policy
        run: policy build . -t ${{ secrets.REGISTRY }}/policies/authz:${{ github.sha }}
      
      - name: Test policy
        run: policy test ${{ secrets.REGISTRY }}/policies/authz:${{ github.sha }}
      
      - name: Push policy
        run: |
          policy login ${{ secrets.REGISTRY }}
          policy push ${{ secrets.REGISTRY }}/policies/authz:${{ github.sha }}
          policy sign ${{ secrets.REGISTRY }}/policies/authz:${{ github.sha }}
```

---

## 与其他方案对比

| 特性 | OPCR | OPA Bundle | Conftest | Git-based |
|:---|:---|:---|:---|:---|
| 存储 | OCI Registry | HTTP/S3/GCS | 本地/Git | Git |
| 签名 | Cosign 原生 | 无 | 无 | Git 签名 |
| 版本 | OCI Tag | URL 路径 | Git Tag | Git Tag |
| 分发 | 任意 Registry | HTTP 服务 | 本地 | Git Clone |
| K8s 集成 | OPA/Gatekeeper | OPA | Conftest | 需同步 |

---

## 最佳实践

1. **语义版本**: 使用语义化版本号管理策略版本
2. **签名验证**: 生产环境始终验证策略签名，防止策略被篡改
3. **测试先行**: 每次策略变更都运行完整的测试套件
4. **分层策略**: 将通用策略和业务策略分开打包，便于复用
5. **审计日志**: 记录策略版本变更和部署历史

---

## 参考资源

- [OPCR 官方文档](https://openpolicycontainers.com/docs/)
- [OPCR GitHub](https://github.com/opcr-io/policy)
- [Open Policy Agent](https://www.openpolicyagent.org/)
- [OCI Distribution Spec](https://github.com/opencontainers/distribution-spec)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
