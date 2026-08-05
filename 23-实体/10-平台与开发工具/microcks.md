---
title: Microcks (entities)
description: '## 概述'
summary: 'Microcks 是一个 API Mock 和测试平台，用于将 OpenAPI、AsyncAPI、gRPC、GraphQL 和 SOAP 的契约规范自动转换为 Mock 服务和集成测试。它帮助开发团队在微服务开发中实现 API 优先（API-First）的工作流，加速并行开发和契约测试。'
category: entities
tags:
- k8s
- cncf
- orchestration
- microcks
- containerd
- rook
- kafka
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Microcks 是什么
- 如何 Microcks
trigger_keywords:
- Microcks
prerequisites:
- kubectl-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Microcks

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Java, TypeScript

## 概述

Microcks 是由 Orange（法国电信）工程师团队开发的开源 API Mock 和测试平台，2020 年进入 CNCF Sandbox。它的核心能力是将 **API 契约规范（Contract Spec）**——包括 OpenAPI、AsyncAPI、gRPC Proto、GraphQL Schema 和 SOAP WSDL——**自动转换为 Mock 服务和集成测试**。这使得开发团队能够在微服务开发中实践**API 优先（API-First）**工作流。

Microcks 解决了微服务并行开发的核心痛点：前端/客户端依赖后端 API，但后端尚未开发完成。传统 Mock 工具需要手动编写 Mock 响应，维护成本高。Microcks 直接解析 API 规范文件（如 OpenAPI YAML），根据规范中定义的请求/响应示例自动生成可调用的 Mock 端点。当 API 规范更新时，Mock 自动更新。它还支持**契约测试（Contract Testing）**——验证实际服务实现是否符合 API 规范。

## Key Features

- **多协议支持**：REST (OpenAPI)、异步 (AsyncAPI/Kafka/MQTT)、gRPC、GraphQL、SOAP
- **自动 Mock 生成**：从 API 规范中的示例自动生成 Mock 端点
- **动态 Mock**：支持基于请求参数的动态响应（Dispatcher 规则）
- **契约测试**：自动生成测试用例，验证服务实现是否符合规范
- **Postman/SoapUI 集成**：导入 Postman Collection 作为测试用例
- **CI/CD 友好**：REST API 和 CLI 工具支持自动化集成

## Architecture

Microcks 由 **Microcks API**（后端 Java/Spring Boot 服务，管理 API 规范、Mock 和测试）、**Microcks UI**（TypeScript/Svelte 前端，Web 界面管理 Mock 和测试）和 **MongoDB**（存储 API 规范、Mock 配置和测试结果）组成。API 规范通过 Web UI、Git 仓库同步或 REST API 导入。Mock 服务由 Microcks 的 endpoint adapter 自动生成——REST Mock 通过内置 HTTP Server 响应，异步 Mock 通过 Kafka/MQTT 消息发布，gRPC Mock 通过 gRPC Server 响应。

## K8s 集成

Microcks 通过 **Microcks Operator**（Helm Chart）部署到 Kubernetes。Operator 管理 Microcks 核心服务、MongoDB（或使用外部 DB）、可选的 Kafka（用于 AsyncAPI Mock）和 Keycloak（认证授权）。Microcks 可以通过 Git 同步机制（post-receive webhook）自动从 Git 仓库拉取最新的 API 规范更新 Mock。

## 生产部署要点

- **API-First**：先定义 API 规范，Microcks 生成 Mock，前后端并行开发
- **契约测试**：在 CI 中运行契约测试，确保服务实现符合 API 规范
- **异步 API**：使用 AsyncAPI 规范 Mock Kafka/MQTT 消息
- **环境配置**：为开发、测试环境部署独立的 Microcks 实例
- **版本管理**：API 规范版本化管理，Mock 跟随版本自动更新

## 生产场景

1. **前后端并行开发**：后端开发前，前端直接调用 Microcks Mock 进行开发
2. **微服务契约测试**：CI 中自动验证每个微服务是否符合 API 契约
3. **异步 API Mock**：模拟 Kafka 消息生产者，测试消费者逻辑
4. **API 文档 + Mock 一体化**：API 规范同时作为文档和 Mock 来源

## 安装与配置

```bash
# Helm 安装 Microcks
helm repo add microcks https://microcks.io/helm
helm repo update
helm install microcks microcks/microcks -n microcks --create-namespace \
  --set mongodb.enabled=true \
  --set keycloak.enabled=true \
  --set features.async.enabled=true \
  --set features.async.kafka.enabled=true \
  --set features.async.kafka.url=kafka.kafka.svc:9092

# 等待就绪
kubectl wait --for=condition=available deployment/microcks -n microcks --timeout=180s

# 访问 Microcks UI
kubectl port-forward svc/microcks 8080:8080 -n microcks
# 打开 http://localhost:8080
```

```yaml
# OpenAPI 规范示例（自动生成 Mock）
openapi: 3.0.3
info:
  title: User Service API
  version: 1.0.0
paths:
  /users/{id}:
    get:
      operationId: getUser
      parameters:
      - name: id
        in: path
        required: true
        schema:
          type: string
      responses:
        '200':
          description: User found
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/User'
              examples:
                default:
                  value:
                    id: "123"
                    name: "Alice"
                    email: "alice@example.com"
  /users:
    post:
      operationId: createUser
      requestBody:
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/User'
      responses:
        '201':
          description: User created
components:
  schemas:
    User:
      type: object
      properties:
        id:
          type: string
        name:
          type: string
        email:
          type: string
```

```bash
# 通过 REST API 导入 OpenAPI 规范
curl -X POST http://microcks.microcks.svc/api/artifact/upload \
  -H "Authorization: Bearer <token>" \
  -F "name=user-service" \
  -F "version=1.0.0" \
  -F "specification=@openapi.yaml"

# 调用自动生成的 Mock 端点
curl http://microcks.microcks.svc/rest/user-service/1.0.0/users/123
# 返回: {"id": "123", "name": "Alice", "email": "alice@example.com"}

# 运行契约测试
curl -X POST http://microcks.microcks.svc/api/tests \
  -H "Content-Type: application/json" \
  -d '{"serviceId": "user-service:1.0.0", "testEndpoint": "http://actual-service:8080"}'
```

## 运维操作

```bash
# 🟢 低风险：查看 Microcks 状态
kubectl get pods -n microcks
kubectl logs deploy/microcks -n microcks --tail=50

# 🟢 低风险：查看已导入的 API
kubectl port-forward svc/microcks 8080:8080 -n microcks &
curl http://localhost:8080/api/services

# 🟡 中风险：更新 API 规范（重新上传）
curl -X POST http://localhost:8080/api/artifact/upload \
  -H "Authorization: Bearer <token>" \
  -F "specification=@updated-openapi.yaml"

# 🟡 中风险：触发契约测试
curl -X POST http://localhost:8080/api/tests \
  -d '{"serviceId": "user-service:1.0.0", "testEndpoint": "http://svc:8080"}'

# 🔴 高风险：删除 API 服务
curl -X DELETE http://localhost:8080/api/services/<service-id>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Mock 返回 404 | API 规范未导入 | `curl /api/services` | 重新上传 API 规范 |
| Mock 响应不匹配 | 示例数据缺失 | 检查 OpenAPI examples 字段 | 添加完整的请求/响应示例 |
| 契约测试失败 | 服务实现不符合规范 | 查看测试报告详情 | 修复服务实现或更新规范 |
| Kafka Mock 不工作 | Kafka 连接失败 | `kubectl logs deploy/microcks` | 检查 Kafka URL 配置 |
| UI 无法访问 | Keycloak 未就绪 | `kubectl get pods -n microcks` | 等待 Keycloak Pod Ready |

```
排查流程：
├── Mock 不工作？
│   ├── 确认 API 规范已导入
│   ├── 检查规范中的 examples 是否完整
│   └── 验证 Mock URL 路径正确
├── 契约测试失败？
│   ├── 查看测试报告中的失败用例
│   ├── 对比实际响应与规范期望
│   └── 修复服务实现或更新规范
└── 异步 Mock 失败？
    ├── 检查 Kafka 连接配置
    ├── 确认 AsyncAPI 规范格式正确
    └── 查看 Microcks 日志中的错误
```

## 生产案例

### 案例 1：前后端并行开发加速

- **场景**：前端团队等待后端 API 开发完成才能开始联调，项目延期 2 周
- **排查**：后端 API 开发需要 3 周，前端只能等待
- **方案**：先定义 OpenAPI 规范，导入 Microcks 自动生成 Mock，前端立即开始开发
- **效果**：前后端完全并行，项目提前 1.5 周交付，联调时间从 1 周缩短至 2 天

### 案例 2：CI/CD 契约测试门禁

- **场景**：微服务更新后频繁破坏 API 契约，导致下游服务故障
- **排查**：缺乏自动化契约验证，问题在生产环境才被发现
- **方案**：在 CI 中集成 Microcks 契约测试，每次 PR 自动验证服务是否符合 OpenAPI 规范
- **效果**：契约破坏在 CI 阶段 100% 拦截，生产 API 故障减少 95%

## 对比

| 特性 | Microcks | WireMock | Postman Mock | Prism |
|------|----------|----------|--------------|-------|
| 多协议 | ✅ REST/gRPC/Async/GraphQL | ⚠️ REST only | ⚠️ REST | ✅ REST/Async |
| 从规范生成 | ✅ OpenAPI/AsyncAPI | ❌ 手动 | ⚠️ | ✅ |
| 契约测试 | ✅ | ⚠️ | ✅ | ⚠️ |
| 异步 Mock | ✅ Kafka/MQTT | ❌ | ❌ | ⚠️ |

## 参考链接

- [[operator-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[23-实体/03-运行时/01-containerd-v2-features]] — [[containerd|containerd]]rd 2.0 新特性|containerd 2.0 新特性]]
- [[karmada]] — Karmada
- [[rook]] — Rook
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- microcks
- [[23-实体/cncf-infrastructure.md|[[23-实体/15-参考与索引/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
