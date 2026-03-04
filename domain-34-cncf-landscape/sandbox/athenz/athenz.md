# Athenz

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.athenz.io/ |
| **GitHub** | https://github.com/AthenZ/athenz |
| **许可证** | Apache-2.0 |
| **开发语言** | Java, Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Athenz 是由 Yahoo (Verizon Media) 开发的开源平台，提供基于 X.509 证书的服务身份认证和细粒度的基于角色的访问控制 (RBAC)。它为微服务架构提供零信任安全模型，每个服务都获得唯一的 X.509 身份证书，所有服务间通信通过 mTLS 加密和验证。Athenz 同时支持集中式和去中心化的授权模式。

### 核心特性

- **服务身份**: 基于 X.509 证书的服务身份认证和 mTLS
- **细粒度 RBAC**: 域-角色-策略的三层授权模型
- **去中心化授权**: 策略缓存到本地 ZPE，无需每次调用中心服务
- **多云支持**: AWS、GCP、Azure 等云平台的身份引导
- **Kubernetes 集成**: 通过 CSI 驱动为 Pod 注入 X.509 证书
- **AWS 临时凭证**: 将 Athenz 身份映射为 AWS IAM 临时角色凭证

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│                Athenz Platform                     │
│                                                    │
│  ┌────────────┐  ┌─────────────┐  ┌───────────┐  │
│  │    ZMS      │  │    ZTS      │  │   UI      │  │
│  │(Management) │  │(Token Svc)  │  │(管理界面) │  │
│  │ 域/角色/策略│  │ X.509 签发  │  │           │  │
│  │ 管理        │  │ Token 签发  │  │           │  │
│  └──────┬─────┘  └──────┬──────┘  └───────────┘  │
│         │               │                          │
│         └───────┬───────┘                          │
└─────────────────┼──────────────────────────────────┘
                  │
    ┌─────────────▼─────────────────┐
    │     策略分发 & 缓存            │
    └─────────────┬─────────────────┘
                  │
┌─────────────────▼──────────────────────────────────┐
│              Service Hosts                           │
│                                                      │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐       │
│  │  SIA      │  │   ZPE     │  │  Service  │       │
│  │(Identity  │  │(Policy    │  │(应用服务) │       │
│  │ Agent)    │  │ Engine)   │  │           │       │
│  │X.509 引导 │  │本地授权   │  │mTLS 通信  │       │
│  └───────────┘  └───────────┘  └───────────┘       │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Docker Compose 快速启动
git clone https://github.com/AthenZ/athenz.git
cd athenz
docker compose up -d

# 或在 Kubernetes 上部署
helm repo add athenz https://athenz.github.io/athenz/charts
helm install athenz athenz/athenz \
  --namespace athenz \
  --create-namespace
```

### 创建域和服务

```bash
# 使用 zms-cli 创建顶级域
zms-cli -z https://zms.example.com:4443/zms/v1 \
  add-domain my-org admin.user

# 添加服务
zms-cli add-service my-org api-gateway 0 api-gateway-key.pem

# 创建角色
zms-cli add-group-role my-org backend-services \
  my-org.api-gateway \
  my-org.user-service
```

### 定义访问策略

```bash
# 创建策略：允许 backend-services 角色的成员访问 /api/* 路径
zms-cli add-policy my-org api-access \
  grant GET,POST,PUT,DELETE \
  to backend-services \
  on "my-org:resource./api/*"

# 细粒度策略
zms-cli add-policy my-org admin-access \
  grant * to admin-role \
  on "my-org:resource.*"
```

### Kubernetes CSI 集成

```yaml
# 通过 Athenz CSI 驱动为 Pod 注入 X.509 证书
apiVersion: v1
kind: Pod
metadata:
  name: my-service
spec:
  containers:
    - name: app
      image: my-service:latest
      volumeMounts:
        - name: athenz-identity
          mountPath: /var/run/athenz
          readOnly: true
      env:
        - name: ATHENZ_DOMAIN
          value: "my-org"
        - name: ATHENZ_SERVICE
          value: "my-service"
  volumes:
    - name: athenz-identity
      csi:
        driver: athenz.io
        readOnly: true
        volumeAttributes:
          athenz.io/domain: "my-org"
          athenz.io/service: "my-service"
```

---

## 客户端 SDK 使用

### Java 客户端

```java
import com.yahoo.athenz.zpe.AuthZpeClient;

// 初始化 ZPE 客户端 (使用本地缓存的策略)
AuthZpeClient.init();

// 检查授权
AccessCheckStatus status = AuthZpeClient.allowAccess(
    "my-org.api-gateway",           // 请求者身份
    "my-org:resource./api/users",    // 资源
    "GET"                            // 操作
);

if (status == AccessCheckStatus.ALLOW) {
    // 允许访问
}
```

### Go 客户端

```go
import "github.com/AthenZ/athenz/libs/go/athenzutils"

// 使用 X.509 证书建立 mTLS 连接
tlsConfig, _ := athenzutils.GetTLSConfigFromFiles(
    "/var/run/athenz/cert.pem",
    "/var/run/athenz/key.pem",
    "/var/run/athenz/ca.pem",
)

client := &http.Client{
    Transport: &http.Transport{
        TLSClientConfig: tlsConfig,
    },
}
```

---

## 与其他方案对比

| 特性 | Athenz | SPIFFE/SPIRE | Keycloak | OPA |
|:---|:---|:---|:---|:---|
| 服务身份 | X.509 证书 | X.509 SVID | JWT/OIDC | 无 |
| 授权模型 | RBAC | 无 (仅身份) | RBAC/ABAC | Rego 策略 |
| 去中心化 | ZPE 本地缓存 | 需自建 | 集中式 | OPA 本地 |
| 云集成 | AWS/GCP/Azure | 多云 | 多云 | 无 |
| 证书管理 | 内置 CA | 内置 CA | 需外部 | 无 |
| 适用场景 | 服务间认证+授权 | 服务身份 | 用户认证 | 策略引擎 |

---

## 最佳实践

1. **域规划**: 按组织/产品线划分域，保持域的边界清晰
2. **最小权限**: 策略遵循最小权限原则，避免使用通配符
3. **证书轮换**: 配置自动证书轮换，通常 24 小时更新一次
4. **本地授权**: 使用 ZPE 进行本地授权决策，减少对中心服务的依赖
5. **审计日志**: 启用 ZMS 审计日志，记录所有策略变更操作

---

## 参考资源

- [Athenz 官方文档](https://www.athenz.io/docs/)
- [Athenz GitHub](https://github.com/AthenZ/athenz)
- [Athenz K8s 集成](https://github.com/AthenZ/k8s-athenz-sia)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
