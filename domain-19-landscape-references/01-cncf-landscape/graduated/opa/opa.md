---
title: Open Policy Agent (OPA)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- envoy
- opa
- crd
- wasm
- rag
- agent
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Open Policy Agent (OPA) 是什么
- 如何 Open Policy Agent (OPA)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Open
- Policy
- Agent
- OPA
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
- policy-basics
---

title: Open Policy Agent (OPA)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- envoy
- opa
- crd
- wasm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Open Policy Agent (OPA) 是什么
- 如何 Open Policy Agent (OPA)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Open
- Policy
- Agent
- OPA
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Open Policy Agent (OPA)

> **成熟度**: Graduated | **加入时间**: 2018-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.openpolicyagent.org |
| **GitHub** | https://github.com/open-policy-agent/opa |
| **文档** | https://www.openpolicyagent.org/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
OPA (Open Policy Agent) 是一个通用的策略引擎，使用声明式语言 Rego 定义策略规则。它将策略决策从应用程序中解耦出来，提供统一的策略执行框架，适用于微服务、Kubernetes、CI/CD、API 网关等各种场景。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2016 | Styra 公司创建 OPA |
| 2018-04 | 加入 CNCF Sandbox |
| 2019-04 | 晋升为 CNCF Incubating |
| 2021-02 | 晋升为 CNCF Graduated |

### 核心定位
OPA 是云原生生态中策略即代码(Policy as Code)的标准解决方案，实现了策略的统一定义、版本控制和自动化执行。

---

## 架构设计

### 策略决策模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    OPA 策略决策模型                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────┐                                            │
│  │   Application   │                                            │
│  │                 │                                            │
│  │  ┌───────────┐  │         ┌─────────────────────────────┐   │
│  │  │ Decision  │  │ Query   │           OPA               │   │
│  │  │  Point    │──┼────────►│  ┌─────────────────────┐   │   │
│  │  │           │  │         │  │   Rego Policy       │   │   │
│  │  │           │◄─┼─────────│  │                     │   │   │
│  │  │           │  │ Decision│  │  package authz      │   │   │
│  │  └───────────┘  │ (allow/ │  │  allow { ... }     │   │   │
│  │                 │  deny)   │  │                     │   │   │
│  └─────────────────┘         │  └─────────────────────┘   │   │
│                              │            +                │   │
│         ┌────────────────────┤  ┌─────────────────────┐   │   │
│         │                    │  │       Data          │   │   │
│         │   Input (请求上下文)│  │  (外部数据源)       │   │   │
│         │   {                │  │  • 用户角色        │   │   │
│         │     "user": "bob", │  │  • 资源权限        │   │   │
│         │     "action": "read"│  │  • 配置信息        │   │   │
│         │     "resource": "x"│  │                     │   │   │
│         │   }                │  └─────────────────────┘   │   │
│         └────────────────────┤                             │   │
│                              └─────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 部署模式

```
┌─────────────────────────────────────────────────────────────────┐
│                    OPA 部署模式                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  模式 1: Sidecar                   模式 2: 独立服务              │
│  ┌─────────────────────────┐      ┌─────────────────────────┐   │
│  │  Pod                    │      │  Service A    Service B │   │
│  │  ┌───────────┐          │      │      │            │     │   │
│  │  │    App    │◄──────┐  │      │      └──────┬─────┘     │   │
│  │  └───────────┘       │  │      │             │           │   │
│  │  ┌───────────┐       │  │      │             ▼           │   │
│  │  │    OPA    │───────┘  │      │      ┌───────────┐      │   │
│  │  │ (Sidecar) │          │      │      │    OPA    │      │   │
│  │  └───────────┘          │      │      │  Service  │      │   │
│  └─────────────────────────┘      │      └───────────┘      │   │
│  延迟: < 1ms                      └─────────────────────────┘   │
│  适用: 高性能场景                  延迟: 网络延迟                │
│                                    适用: 集中管理                │
│                                                                  │
│  模式 3: 库嵌入 (Go)               模式 4: WASM                  │
│  ┌─────────────────────────┐      ┌─────────────────────────┐   │
│  │  Go Application         │      │  Browser / Edge         │   │
│  │  ┌───────────────────┐  │      │  ┌───────────────────┐  │   │
│  │  │    import opa     │  │      │  │  OPA WASM Module  │  │   │
│  │  │    rego.New()     │  │      │  │                   │  │   │
│  │  │    rego.Eval()    │  │      │  │  (编译后的策略)   │  │   │
│  │  └───────────────────┘  │      │  └───────────────────┘  │   │
│  └─────────────────────────┘      └─────────────────────────┘   │
│  延迟: 微秒级                      延迟: 微秒级                  │
│  适用: Go 应用                     适用: 浏览器/边缘             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Rego 语言

### 基础语法

```rego
# 包声明
package authz

# 导入
import future.keywords.if
import future.keywords.in
import future.keywords.contains

# 默认值
default allow := false

# 规则定义
allow if {
    input.user == "admin"
}

allow if {
    input.method == "GET"
    input.path[0] == "public"
}

# 集合推导
public_endpoints := {path |
    some endpoint in data.endpoints
    endpoint.public == true
    path := endpoint.path
}

# 对象推导
user_roles := {user: roles |
    some user, roles in data.user_role_mapping
}

# 函数定义
is_admin(user) if {
    user in data.admins
}
```

### 实用示例

```rego
package kubernetes.admission

import future.keywords.if
import future.keywords.in
import future.keywords.contains

# 拒绝没有资源限制的 Pod
deny contains msg if {
    input.request.kind.kind == "Pod"
    container := input.request.object.spec.containers[_]
    not container.resources.limits
    msg := sprintf("Container '%v' must have resource limits", [container.name])
}

# 拒绝使用 latest 标签
deny contains msg if {
    input.request.kind.kind == "Pod"
    container := input.request.object.spec.containers[_]
    endswith(container.image, ":latest")
    msg := sprintf("Container '%v' cannot use 'latest' tag", [container.name])
}

# 拒绝特权容器
deny contains msg if {
    input.request.kind.kind == "Pod"
    container := input.request.object.spec.containers[_]
    container.securityContext.privileged == true
    msg := sprintf("Container '%v' cannot be privileged", [container.name])
}

# 必须来自可信仓库
deny contains msg if {
    input.request.kind.kind == "Pod"
    container := input.request.object.spec.containers[_]
    not startswith(container.image, "registry.example.com/")
    not startswith(container.image, "gcr.io/")
    msg := sprintf("Container '%v' image must be from trusted registry", [container.name])
}
```

---

## Kubernetes 集成 (Gatekeeper)

### 安装 Gatekeeper

```bash
# 安装 Gatekeeper
kubectl apply -f https://raw.githubusercontent.com/open-policy-agent/gatekeeper/v3.14.0/deploy/gatekeeper.yaml

# 验证安装
kubectl get pods -n gatekeeper-system
```

### 创建约束模板

```yaml
# ConstraintTemplate: 定义策略模板
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredlabels
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredLabels
      validation:
        openAPIV3Schema:
          type: object
          properties:
            labels:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiredlabels

        violation[{"msg": msg, "details": {"missing_labels": missing}}] {
          provided := {label | input.review.object.metadata.labels[label]}
          required := {label | label := input.parameters.labels[_]}
          missing := required - provided
          count(missing) > 0
          msg := sprintf("Missing required labels: %v", [missing])
        }
```

### 应用约束

```yaml
# Constraint: 应用策略
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-team-label
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Namespace"]
    excludedNamespaces:
      - kube-system
      - gatekeeper-system
  parameters:
    labels:
      - "team"
      - "environment"
```

### 常用策略库

```bash
# Gatekeeper 策略库
git clone https://github.com/open-policy-agent/gatekeeper-library.git

# 包含的策略:
# - 容器安全 (特权、root 运行、只读文件系统)
# - 镜像策略 (可信仓库、禁止 latest)
# - 网络策略 (禁止 hostNetwork、hostPort)
# - 资源配额 (limits、requests)
# - 命名规范 (标签、名称)
```

---

## 其他集成场景

### API 网关 (Envoy)

```yaml
# Envoy External Authorization
http_filters:
  - name: envoy.ext_authz
    typed_config:
      "@type": type.googleapis.com/envoy.extensions.filters.http.ext_authz.v3.ExtAuthz
      grpc_service:
        envoy_grpc:
          cluster_name: opa
        timeout: 0.5s
      transport_api_version: V3
```

```rego
# OPA 策略 for Envoy
package envoy.authz

import future.keywords.if

default allow := false

allow if {
    input.attributes.request.http.method == "GET"
    glob.match("/api/public/*", [], input.attributes.request.http.path)
}

allow if {
    input.attributes.request.http.headers.authorization
    token := trim_prefix(input.attributes.request.http.headers.authorization, "Bearer ")
    claims := io.jwt.decode(token)[1]
    claims.role == "admin"
}
```

### Terraform

```hcl
# terraform.rego
package terraform

import future.keywords.if

# 禁止公开 S3 Bucket
deny contains msg if {
    resource := input.resource_changes[_]
    resource.type == "aws_s3_bucket"
    resource.change.after.acl == "public-read"
    msg := sprintf("S3 bucket '%s' cannot be public", [resource.address])
}

# 必须加密 RDS
deny contains msg if {
    resource := input.resource_changes[_]
    resource.type == "aws_db_instance"
    not resource.change.after.storage_encrypted
    msg := sprintf("RDS instance '%s' must be encrypted", [resource.address])
}
```

```bash
# 在 CI/CD 中使用
terraform plan -out=tfplan
terraform show -json tfplan > tfplan.json
opa eval -d terraform.rego -i tfplan.json "data.terraform.deny"
```

---

## OPA 服务运维

### 启动 OPA 服务

```bash
# 启动 OPA 服务
opa run --server \
  --addr :8181 \
  --bundle bundle.tar.gz \
  --log-level info

# 查询策略
curl -X POST http://localhost:8181/v1/data/authz/allow \
  -H "Content-Type: application/json" \
  -d '{"input": {"user": "alice", "action": "read"}}'
```

### Bundle 管理

```bash
# 创建 Bundle
opa build -b ./policies -o bundle.tar.gz

# Bundle 服务配置
services:
  acme:
    url: https://bundle-server.example.com

bundles:
  authz:
    service: acme
    resource: /bundles/authz.tar.gz
    polling:
      min_delay_seconds: 10
      max_delay_seconds: 60
```

---

## 参考资源

- [官方文档](https://www.openpolicyagent.org/docs)
- [GitHub Repo](https://github.com/open-policy-agent/opa)
- [CNCF 项目页面](https://www.cncf.io/projects/open-policy-agent/)
- [Rego Playground](https://play.openpolicyagent.org/)
- [Gatekeeper](https://github.com/open-policy-agent/gatekeeper)
- [Gatekeeper Library](https://github.com/open-policy-agent/gatekeeper-library)
- [Styra Academy](https://academy.styra.com/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/envoy.md|Envoy]]
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/multi-tenancy-isolation|Multi-Tenancy Isolation]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.43|opa v0.43 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.12|opa v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.26|opa v0.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.9|opa v1.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.8|opa v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.67|opa v0.67 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.36|opa v0.36 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.53|opa v0.53 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.22|opa v0.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.16|opa v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.47|opa v0.47 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.57|opa v0.57 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.32|opa v0.32 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.63|opa v0.63 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.23|opa v0.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.17|opa v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.46|opa v0.46 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.56|opa v0.56 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.33|opa v0.33 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.62|opa v0.62 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.42|opa v0.42 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.13|opa v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.27|opa v0.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.8|opa v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.66|opa v0.66 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.9|opa v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.37|opa v0.37 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.52|opa v0.52 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.49|opa v0.49 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.18|opa v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.3|opa v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.59|opa v0.59 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.28|opa v0.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.7|opa v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.12|opa v1.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.38|opa v0.38 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.69|opa v0.69 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.29|opa v0.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.6|opa v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.13|opa v1.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.39|opa v0.39 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.7|opa v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.68|opa v0.68 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.48|opa v0.48 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.19|opa v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.2|opa v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.58|opa v0.58 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.5|opa v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.10|opa v1.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.14|opa v1.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.15|opa v1.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.0|opa v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.4|opa v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.11|opa v1.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.20|opa v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.45|opa v0.45 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.14|opa v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.55|opa v0.55 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.61|opa v0.61 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.30|opa v0.30 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.10|opa v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.41|opa v0.41 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.24|opa v0.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.34|opa v0.34 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.65|opa v0.65 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.51|opa v0.51 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.11|opa v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.40|opa v0.40 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.25|opa v0.25 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.35|opa v0.35 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.64|opa v0.64 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.50|opa v0.50 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.21|opa v0.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.70|opa v0.70 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.44|opa v0.44 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.15|opa v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.54|opa v0.54 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.60|opa v0.60 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.31|opa v0.31 Release Notes]]
