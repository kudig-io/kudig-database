# Hexa (Policy Orchestrator)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://hexaorchestration.org/ |
| **GitHub** | https://github.com/hexa-org/policy-orchestrator |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Hexa 是一个统一的策略编排引擎，使用 IDQL (Identity Query Language) 作为通用策略语言，实现跨多个云平台和授权系统的访问控制策略管理。它支持将策略从一个授权系统（如 AWS IAM、Azure RBAC、Google IAP）翻译和同步到另一个系统，避免了在不同平台上重复维护相似策略的问题。

### 核心特性

- **统一策略语言 (IDQL)**: 使用统一的 JSON 格式描述授权策略
- **多平台支持**: AWS Cognito/IAM、Azure AD、Google IAP、OPA、Cedar 等
- **策略翻译**: 自动在不同授权系统之间翻译策略
- **策略同步**: 从一个平台读取策略并同步到其他平台
- **策略发现**: 自动发现已有平台上的授权策略
- **IDQL SDK**: 提供 Go/Java SDK 用于应用内策略评估

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│          Hexa Policy Orchestrator            │
│                                              │
│  ┌─────────────────────────────────┐        │
│  │       IDQL Policy Engine        │        │
│  │  (统一策略表示 / 翻译 / 验证)    │        │
│  └────────────────┬────────────────┘        │
│                   │                          │
│  ┌────────────────▼────────────────┐        │
│  │     Provider Adapters           │        │
│  │                                  │        │
│  │ ┌─────┐ ┌─────┐ ┌─────┐       │        │
│  │ │ AWS │ │Azure│ │ GCP │       │        │
│  │ │ IAM │ │ AD  │ │ IAP │       │        │
│  │ └──┬──┘ └──┬──┘ └──┬──┘       │        │
│  │ ┌──┴──┐ ┌──┴──┐ ┌──┴──┐       │        │
│  │ │ OPA │ │Cedar│ │ABAC │       │        │
│  │ └─────┘ └─────┘ └─────┘       │        │
│  └─────────────────────────────────┘        │
└─────────────────────────────────────────────┘
         ▲           ▲           ▲
         │           │           │
    ┌────┴───┐  ┌────┴───┐  ┌───┴────┐
    │ AWS    │  │ Azure  │  │ Google │
    │Platform│  │Platform│  │Platform│
    └────────┘  └────────┘  └────────┘
```

---

## 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/hexa-org/policy-orchestrator.git
cd policy-orchestrator

# 构建
go build -o hexa ./cmd/orchestrator

# 或使用 Docker
docker run -p 8885:8885 hexaorg/policy-orchestrator:latest
```

### IDQL 策略格式

```json
{
  "policies": [
    {
      "meta": {
        "version": "0.7",
        "description": "Allow engineering team to access API"
      },
      "subjects": [
        "group:engineering",
        "user:admin@example.com"
      ],
      "actions": [
        {
          "actionUri": "http:GET",
          "actionUri": "http:POST"
        }
      ],
      "object": {
        "resource_id": "/api/v1/projects/*"
      },
      "condition": {
        "rule": "req.ip sw 10.0.0 and req.time bt 09:00 17:00",
        "action": "allow"
      }
    }
  ]
}
```

### 策略发现与同步

```bash
# 从 AWS Cognito 发现策略
hexa discover --provider aws-cognito \
  --region us-east-1 \
  --user-pool-id us-east-1_xxxxx

# 将策略翻译为 OPA Rego
hexa translate --from aws-cognito --to opa \
  --input policies.json \
  --output opa-bundle/

# 同步策略到 Azure AD
hexa sync --source aws-cognito --target azure-ad \
  --source-config aws-config.json \
  --target-config azure-config.json
```

---

## 集成配置

### OPA 集成

```yaml
# 将 IDQL 策略部署为 OPA Bundle
apiVersion: v1
kind: ConfigMap
metadata:
  name: opa-config
data:
  config.yaml: |
    services:
      hexa:
        url: http://hexa-orchestrator:8885
    bundles:
      hexa-policies:
        service: hexa
        resource: /bundles/policies
        polling:
          min_delay_seconds: 30
          max_delay_seconds: 60
```

### 应用内策略评估 (Go SDK)

```go
package main

import (
    "github.com/hexa-org/policy-mapper/sdk"
)

func main() {
    // 加载 IDQL 策略
    engine, _ := sdk.NewEngine("policies.json")

    // 评估请求
    decision := engine.Evaluate(sdk.Request{
        Subject:  "user:alice@example.com",
        Action:   "http:GET",
        Resource: "/api/v1/projects/123",
        Context: map[string]interface{}{
            "ip":   "10.0.0.5",
            "time": "14:30",
        },
    })

    if decision.Allow {
        // 允许访问
    }
}
```

---

## 支持的平台

| 平台 | 读取策略 | 写入策略 | 说明 |
|:---|:---|:---|:---|
| AWS Cognito | 支持 | 支持 | 用户池策略 |
| AWS IAM/Verified Permissions | 支持 | 支持 | Cedar 策略 |
| Azure AD | 支持 | 支持 | 应用角色和权限 |
| Google IAP | 支持 | 支持 | IAP 访问策略 |
| OPA/Rego | 支持 | 支持 | Rego Bundle 策略 |
| Cedar | 支持 | 支持 | Cedar 策略集 |
| IDQL (本地) | 支持 | 支持 | JSON 文件策略 |

---

## 最佳实践

1. **统一策略定义**: 使用 IDQL 作为策略的唯一真相源，避免在各平台分别维护
2. **版本控制**: 将 IDQL 策略文件纳入 Git 管理，启用策略变更审计
3. **渐进式迁移**: 先发现已有策略，验证翻译正确性后再同步
4. **最小权限**: IDQL 策略设计遵循最小权限原则，默认拒绝
5. **条件表达式**: 善用 condition 字段实现 IP 限制、时间窗口等动态策略

---

## 参考资源

- [Hexa 官方文档](https://hexaorchestration.org/docs/)
- [Hexa GitHub](https://github.com/hexa-org/policy-orchestrator)
- [IDQL 规范](https://github.com/hexa-org/policy-mapper)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
