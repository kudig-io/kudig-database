# Keycloak

> **成熟度**: Incubating | **加入时间**: 2023-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.keycloak.org |
| **GitHub** | https://github.com/keycloak/keycloak |
| **许可证** | Apache-2.0 |
| **主要语言** | Java |
| **CNCF 分类** | Security & Identity |

---

## 项目概述

Keycloak 是开源的身份和访问管理（IAM）解决方案，提供单点登录（SSO）、身份联合、用户管理和细粒度授权功能。它支持 OpenID Connect、OAuth 2.0 和 SAML 2.0 标准协议。

## 核心特性

- **单点登录 (SSO)**: 一次登录访问多个应用
- **身份联合**: 集成 LDAP、Active Directory、社交登录
- **标准协议**: OpenID Connect、OAuth 2.0、SAML 2.0
- **多租户**: Realm 隔离的多租户架构
- **细粒度授权**: 基于角色、资源、策略的访问控制
- **高可用**: 支持集群部署和数据库复制
- **可扩展**: SPI 插件机制，自定义认证流程

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Client Applications                          │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────────┐    │
│  │   Web   │  │ Mobile  │  │   API   │  │  Microservices  │    │
│  │   App   │  │   App   │  │ Gateway │  │                 │    │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────────┬────────┘    │
└───────┼────────────┼────────────┼────────────────┼──────────────┘
        │            │            │                │
        │     OIDC / OAuth 2.0 / SAML              │
        └────────────┴────────────┴────────────────┘
                              │
┌─────────────────────────────┼───────────────────────────────────┐
│                    Keycloak Server                               │
├─────────────────────────────┼───────────────────────────────────┤
│  ┌──────────────────────────┴──────────────────────────────┐   │
│  │                   Authentication SPI                     │   │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌─────────────────┐  │   │
│  │  │Password│ │  OTP   │ │WebAuthn│ │  Social Login   │  │   │
│  │  └────────┘ └────────┘ └────────┘ └─────────────────┘  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌────────────────────┐    ┌─────────────────────────────────┐  │
│  │   Realm Manager    │    │      User Federation SPI        │  │
│  │  ┌──────────────┐  │    │  ┌──────┐ ┌──────┐ ┌────────┐  │  │
│  │  │   Realm 1    │  │    │  │ LDAP │ │  AD  │ │ Custom │  │  │
│  │  │   Realm 2    │  │    │  └──────┘ └──────┘ └────────┘  │  │
│  │  └──────────────┘  │    └─────────────────────────────────┘  │
│  └────────────────────┘                                         │
│                            ┌─────────────────────────────────┐  │
│  ┌────────────────────┐    │     Authorization Services       │  │
│  │  Session Manager   │    │  ┌────────┐ ┌────────────────┐  │  │
│  │  ┌──────────────┐  │    │  │ Roles  │ │    Policies    │  │  │
│  │  │   Infinispan │  │    │  └────────┘ └────────────────┘  │  │
│  │  │    Cache     │  │    │  ┌────────┐ ┌────────────────┐  │  │
│  │  └──────────────┘  │    │  │Resource│ │   Permissions  │  │  │
│  └────────────────────┘    │  └────────┘ └────────────────┘  │  │
│                            └─────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    │     Database      │
                    │ (PostgreSQL/MySQL)│
                    └───────────────────┘
```

### 核心概念

| 概念 | 说明 |
|------|------|
| Realm | 租户隔离单元，包含用户、角色、客户端 |
| Client | 请求认证的应用程序 |
| User | 最终用户实体 |
| Role | 权限集合（Realm Role / Client Role） |
| Group | 用户分组，继承角色 |
| Identity Provider | 外部身份源（LDAP、社交登录） |

---

## 快速开始

### Docker 部署

```bash
# 开发模式快速启动
docker run -p 8080:8080 \
  -e KEYCLOAK_ADMIN=admin \
  -e KEYCLOAK_ADMIN_PASSWORD=admin \
  quay.io/keycloak/keycloak:23.0 start-dev

# 生产模式（使用外部数据库）
docker run -p 8080:8080 \
  -e KC_DB=postgres \
  -e KC_DB_URL=jdbc:postgresql://postgres:5432/keycloak \
  -e KC_DB_USERNAME=keycloak \
  -e KC_DB_PASSWORD=keycloak \
  -e KC_HOSTNAME=auth.example.com \
  -e KEYCLOAK_ADMIN=admin \
  -e KEYCLOAK_ADMIN_PASSWORD=admin \
  quay.io/keycloak/keycloak:23.0 start
```

### Kubernetes 部署 (Operator)

```yaml
# keycloak-operator.yaml
apiVersion: k8s.keycloak.org/v2alpha1
kind: Keycloak
metadata:
  name: keycloak
  namespace: keycloak
spec:
  instances: 3
  db:
    vendor: postgres
    host: postgres-db
    usernameSecret:
      name: keycloak-db-secret
      key: username
    passwordSecret:
      name: keycloak-db-secret
      key: password
  http:
    tlsSecret: keycloak-tls-secret
  hostname:
    hostname: auth.example.com
  ingress:
    enabled: true
    className: nginx
```

### 创建 Realm 和 Client

```yaml
# keycloak-realm.yaml
apiVersion: k8s.keycloak.org/v2alpha1
kind: KeycloakRealmImport
metadata:
  name: my-realm
  namespace: keycloak
spec:
  keycloakCRName: keycloak
  realm:
    realm: my-realm
    enabled: true
    registrationAllowed: true
    clients:
      - clientId: my-app
        enabled: true
        publicClient: true
        standardFlowEnabled: true
        directAccessGrantsEnabled: true
        redirectUris:
          - "http://localhost:3000/*"
          - "https://myapp.example.com/*"
        webOrigins:
          - "http://localhost:3000"
          - "https://myapp.example.com"
```

---

## 集成示例

### React 应用集成 (OIDC)

```javascript
// keycloak.js
import Keycloak from 'keycloak-js';

const keycloak = new Keycloak({
  url: 'https://auth.example.com',
  realm: 'my-realm',
  clientId: 'my-app'
});

// 初始化
keycloak.init({ 
  onLoad: 'check-sso',
  pkceMethod: 'S256'
}).then(authenticated => {
  if (authenticated) {
    console.log('User is authenticated');
    console.log('Token:', keycloak.token);
  }
});

// 登录
keycloak.login();

// 登出
keycloak.logout();

// 刷新 Token
keycloak.updateToken(30).then(refreshed => {
  if (refreshed) {
    console.log('Token refreshed');
  }
});
```

### Spring Boot 集成

```yaml
# application.yml
spring:
  security:
    oauth2:
      resourceserver:
        jwt:
          issuer-uri: https://auth.example.com/realms/my-realm
          jwk-set-uri: https://auth.example.com/realms/my-realm/protocol/openid-connect/certs
```

```java
// SecurityConfig.java
@Configuration
@EnableWebSecurity
public class SecurityConfig {
    
    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            .authorizeHttpRequests(auth -> auth
                .requestMatchers("/api/public/**").permitAll()
                .requestMatchers("/api/admin/**").hasRole("ADMIN")
                .anyRequest().authenticated()
            )
            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> jwt
                    .jwtAuthenticationConverter(jwtAuthConverter())
                )
            );
        return http.build();
    }
    
    private JwtAuthenticationConverter jwtAuthConverter() {
        JwtGrantedAuthoritiesConverter converter = new JwtGrantedAuthoritiesConverter();
        converter.setAuthoritiesClaimName("realm_access.roles");
        converter.setAuthorityPrefix("ROLE_");
        
        JwtAuthenticationConverter jwtConverter = new JwtAuthenticationConverter();
        jwtConverter.setJwtGrantedAuthoritiesConverter(converter);
        return jwtConverter;
    }
}
```

### Nginx / Kong 集成

```nginx
# nginx.conf - 使用 lua-resty-openidc
location /protected {
    access_by_lua_block {
        local opts = {
            discovery = "https://auth.example.com/realms/my-realm/.well-known/openid-configuration",
            client_id = "nginx-client",
            client_secret = "secret",
            redirect_uri = "https://app.example.com/callback",
            scope = "openid profile email"
        }
        local res, err = require("resty.openidc").authenticate(opts)
        if err then
            ngx.status = 403
            ngx.exit(ngx.HTTP_FORBIDDEN)
        end
        ngx.req.set_header("X-User", res.id_token.preferred_username)
    }
    proxy_pass http://backend;
}
```

---

## 高级配置

### 自定义认证流程

```json
{
  "alias": "custom-browser",
  "description": "Custom browser flow with MFA",
  "providerId": "basic-flow",
  "topLevel": true,
  "builtIn": false,
  "authenticationExecutions": [
    {
      "authenticator": "auth-cookie",
      "requirement": "ALTERNATIVE",
      "priority": 10
    },
    {
      "authenticator": "auth-username-password-form",
      "requirement": "REQUIRED",
      "priority": 20
    },
    {
      "authenticator": "auth-otp-form",
      "requirement": "REQUIRED",
      "priority": 30
    }
  ]
}
```

### 细粒度授权 (Authorization Services)

```json
{
  "resources": [
    {
      "name": "Document",
      "type": "document",
      "ownerManagedAccess": true,
      "scopes": ["read", "write", "delete"]
    }
  ],
  "policies": [
    {
      "name": "Only Owner",
      "type": "js",
      "logic": "POSITIVE",
      "code": "$evaluation.grant();"
    }
  ],
  "permissions": [
    {
      "name": "Document Permission",
      "type": "resource",
      "resources": ["Document"],
      "policies": ["Only Owner"],
      "scopes": ["read", "write"]
    }
  ]
}
```

---

## 监控与运维

### Prometheus 指标

```yaml
# Keycloak 23+ 内置 metrics
/metrics  # 需要启用 metrics-enabled=true
```

### 关键指标

| 指标 | 说明 |
|------|------|
| keycloak_logins | 登录次数 |
| keycloak_failed_login_attempts | 失败登录尝试 |
| keycloak_registrations | 用户注册数 |
| keycloak_request_duration | 请求延迟 |

---

## 最佳实践

1. **生产安全**: 强制 HTTPS，配置合适的 Token 过期时间
2. **密码策略**: 启用密码复杂度、历史记录、暴力破解保护
3. **多因素认证**: 为敏感操作启用 OTP/WebAuthn
4. **会话管理**: 配置会话超时，启用 SSO Session Idle
5. **审计日志**: 启用 Event Logging，集成 SIEM
6. **备份恢复**: 定期备份 Realm 配置和数据库

---

## 参考资源

- [官方文档](https://www.keycloak.org/documentation)
- [GitHub Repo](https://github.com/keycloak/keycloak)
- [Keycloak Operator](https://www.keycloak.org/operator/installation)
- [Server Administration Guide](https://www.keycloak.org/docs/latest/server_admin/)

---

**维护者**: Kudig Team | **许可证**: MIT
