# Microcks

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://microcks.io/ |
| **GitHub** | https://github.com/microcks/microcks |
| **许可证** | Apache-2.0 |
| **开发语言** | Java, TypeScript |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Microcks 是一个 API Mock 和测试平台，用于将 OpenAPI、AsyncAPI、gRPC、GraphQL 和 SOAP 的契约规范自动转换为 Mock 服务和集成测试。它帮助开发团队在微服务开发中实现 API 优先（API-First）的工作流，加速并行开发和契约测试。

### 核心特性

- **多协议 Mock**: OpenAPI/Swagger, AsyncAPI, gRPC, GraphQL, SOAP/WSDL
- **自动生成**: 从 API 规范自动生成 Mock 响应和测试用例
- **异步 API**: 支持 Kafka, MQTT, AMQP, WebSocket 等异步协议 Mock
- **契约测试**: 验证服务实现是否符合 API 契约
- **Postman 集成**: 导入 Postman Collection 作为 Mock 数据源
- **动态响应**: 基于请求参数的动态 Mock 响应和模板
- **Kubernetes 原生**: Helm Chart 和 Operator 部署

---

## 快速开始

### 安装

```bash
# Helm 安装
helm repo add microcks https://microcks.io/helm
helm install microcks microcks/microcks \
  --namespace microcks \
  --create-namespace \
  --set microcks.url=microcks.example.com \
  --set keycloak.url=keycloak.example.com

# Docker Compose 快速体验
docker compose -f docker-compose.yml up -d
```

### 导入 API 规范

```yaml
# petstore-openapi.yaml
openapi: "3.0.0"
info:
  title: Petstore API
  version: "1.0.0"
paths:
  /pets/{petId}:
    get:
      operationId: getPet
      parameters:
        - name: petId
          in: path
          required: true
          schema:
            type: integer
      responses:
        "200":
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/Pet'
              examples:
                cat:
                  value:
                    id: 1
                    name: "Tom"
                    tag: "cat"
                dog:
                  value:
                    id: 2
                    name: "Rex"
                    tag: "dog"
```

```bash
# 通过 CLI 导入
microcks-cli import petstore-openapi.yaml \
  --microcksURL=https://microcks.example.com/api \
  --keycloakClientId=microcks-serviceaccount \
  --keycloakClientSecret=<SECRET>
```

### 使用 Mock

```bash
# Microcks 自动创建 Mock 端点
# GET /rest/Petstore+API/1.0.0/pets/1
curl https://microcks.example.com/rest/Petstore+API/1.0.0/pets/1

# 根据不同 petId 返回不同的 Mock 响应
curl https://microcks.example.com/rest/Petstore+API/1.0.0/pets/2
```

---

## 高级功能

### AsyncAPI Mock (Kafka)

```yaml
# order-events.yaml
asyncapi: "2.6.0"
info:
  title: Order Events
  version: "1.0.0"
channels:
  orders/created:
    publish:
      message:
        payload:
          type: object
          properties:
            orderId:
              type: string
            amount:
              type: number
          examples:
            - orderId: "ORD-001"
              amount: 99.99
```

### 契约测试

```bash
# 运行契约测试验证服务实现
microcks-cli test \
  'Petstore API:1.0.0' \
  http://actual-service:8080 \
  OPEN_API_SCHEMA \
  --microcksURL=https://microcks.example.com/api \
  --waitFor=10sec
```

### CI/CD 集成

```yaml
# GitHub Actions
- name: Contract Testing
  uses: microcks/test-github-action@v1
  with:
    apiNameAndVersion: 'Petstore API:1.0.0'
    testEndpoint: 'http://api-service:8080'
    runner: OPEN_API_SCHEMA
    microcksURL: ${{ secrets.MICROCKS_URL }}
```

---

## 最佳实践

1. **API-First**: 先定义 API 规范，Microcks 生成 Mock，前后端并行开发
2. **契约测试**: 在 CI 中运行契约测试，确保服务实现符合 API 规范
3. **异步 API**: 使用 AsyncAPI 规范 Mock Kafka/MQTT 消息
4. **环境配置**: 为开发、测试环境部署独立的 Microcks 实例
5. **版本管理**: API 规范版本化管理，Mock 跟随版本自动更新

---

## 参考资源

- [Microcks 官方文档](https://microcks.io/documentation/)
- [Microcks GitHub](https://github.com/microcks/microcks)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
