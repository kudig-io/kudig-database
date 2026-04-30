# Java 应用 Kubernetes 安全加固指南

> **适用版本**: JDK 21 (LTS) / Spring Boot 3.4+ / Kubernetes v1.29-v1.33  
> **最后更新**: 2026-04-30  
> **难度**: 高级

---

## 📋 目录

- [一、Java 安全架构全景](#一java-安全架构全景)
- [二、容器运行时安全](#二容器运行时安全)
- [三、密钥与证书管理](#三密钥与证书管理)
- [四、KeyStore/TrustStore 与 K8s Secrets](#四keystoretruststore-与-k8s-secrets)
- [五、Spring Security 集成](#五spring-security-集成)
- [六、依赖安全与 SBOM](#六依赖安全与-sbom)
- [七、安全编码实践](#七安全编码实践)
- [八、网络安全策略](#八网络安全策略)
- [九、审计与合规](#九审计与合规)
- [十、安全检查清单](#十安全检查清单)

---

## 一、Java 安全架构全景

```mermaid
graph TD
    A[Java 应用安全] --> B[容器安全]
    A --> C[运行时安全]
    A --> D[网络安全]
    A --> E[密钥管理]
    A --> F[依赖安全]
    A --> G[代码安全]

    B --> B1[非 root 运行]
    B --> B2[只读文件系统]
    B --> B3[Distroless 镜像]
    B --> B4[镜像签名/扫描]

    C --> R1[SecurityManager (已弃用)]
    R1 --> R2[用 K8s SecurityContext 替代]
    R2 --> R3[Seccomp / AppArmor]

    D --> D1[mTLS (Istio/Linkerd)]
    D --> D2[NetworkPolicy]
    D --> D3[Pod Security Standards]

    E --> E1[K8s Secrets + CSI]
    E --> E2[Vault 集成]
    E --> E3[证书轮换]

    F --> F1[SBOM 生成]
    F --> F2[漏洞扫描]
    F --> F3[Log4Shell 防御]

    G --> G1[输入验证]
    G --> G2[SQL 注入防护]
    G --> G3[无硬编码密钥]

    style A fill:#ef4444,stroke:#b91c1c,color:#fff
    style B fill:#f59e0b,stroke:#b45309,color:#fff
    style D fill:#326ce5,stroke:#1a3a8f,color:#fff
    style E fill:#a855f7,stroke:#6b21a8,color:#fff
```

### 1.1 SecurityManager 弃用说明

JDK 21 中 `SecurityManager` 已标记为弃用 (JEP 411)，将在未来版本移除。K8s 环境下的安全替代方案：

| SecurityManager 功能 | K8s 替代方案 |
|---------------------|-------------|
| 文件系统访问控制 | `readOnlyRootFilesystem: true` + emptyDir |
| 网络端口绑定 | `SecurityContext` + NetworkPolicy |
| 进程执行限制 | Seccomp Profile + `allowPrivilegeEscalation: false` |
| 类加载限制 | Pod Security Standards + OPA/Kyverno |

---

## 二、容器运行时安全

### 2.1 SecurityContext 完整配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spring-app
spec:
  template:
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 1001
        runAsGroup: 1001
        fsGroup: 1001
        fsGroupChangePolicy: "OnRootMismatch"
        seccompProfile:
          type: RuntimeDefault
        supplementalGroups:
          - 1001
      containers:
        - name: app
          image: registry.example.com/spring-app:v1.0.0
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            runAsNonRoot: true
            runAsUser: 1001
            capabilities:
              drop:
                - ALL
          volumeMounts:
            - name: tmp
              mountPath: /tmp
            - name: app-logs
              mountPath: /var/log/app
      volumes:
        - name: tmp
          emptyDir:
            medium: Memory
            sizeLimit: "64Mi"
        - name: app-logs
          emptyDir: {}
```

### 2.2 Pod Security Standards

```yaml
# 命名空间级别强制 Pod Security Standard
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.33
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 2.3 Kyverno 策略: 强制 Java 安全基线

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: java-security-baseline
spec:
  rules:
    - name: require-non-root
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "Java 容器必须以非 root 用户运行"
        pattern:
          spec:
            containers:
              - (name): "*"
                securityContext:
                  runAsNonRoot: true
    - name: disallow-privilege-escalation
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "禁止权限提升"
        pattern:
          spec:
            containers:
              - (name): "*"
                securityContext:
                  allowPrivilegeEscalation: false
    - name: drop-all-capabilities
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "必须丢弃所有 Linux capabilities"
        pattern:
          spec:
            containers:
              - (name): "*"
                securityContext:
                  capabilities:
                    drop:
                      - ALL
```

---

## 三、密钥与证书管理

### 3.1 Secret 注入最佳实践

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spring-app
spec:
  template:
    spec:
      containers:
        - name: app
          env:
            # 方式 1: 单个 Secret 键值
            - name: SPRING_DATASOURCE_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: password
            # 方式 2: 整个 Secret 作为环境变量
          envFrom:
            - secretRef:
                name: app-secrets
          # 方式 3: 挂载为文件
          volumeMounts:
            - name: tls-certs
              mountPath: /certs
              readOnly: true
      volumes:
        - name: tls-certs
          secret:
            secretName: app-tls
            defaultMode: 0400
```

### 3.2 禁止硬编码密钥

```java
@Configuration
public class SecureConfig {

    @Value("${spring.datasource.password}")
    private String dbPassword;

    @Bean
    public DataSource dataSource() {
        return DataSourceBuilder.create()
            .url(env.getProperty("spring.datasource.url"))
            .username(env.getProperty("spring.datasource.username"))
            .password(dbPassword)
            .build();
    }
}
```

```yaml
# Kyverno: 检测镜像中的硬编码密钥 (通过环境变量)
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-secrets-in-env
spec:
  rules:
    - name: use-secret-ref-instead
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "禁止在 env 中直接使用明文密钥，请使用 secretKeyRef"
        pattern:
          spec:
            containers:
              - (name): "*"
                ~(env):
                  - value: "*password*|*secret*|*token*|*key*"
```

---

## 四、KeyStore/TrustStore 与 K8s Secrets

### 4.1 TLS 证书管理

```yaml
# 使用 cert-manager 自动管理证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: spring-app-tls
spec:
  secretName: spring-app-tls
  duration: 720h
  renewBefore: 168h
  issuerRef:
    name: cluster-ca
    kind: ClusterIssuer
  dnsNames:
    - spring-app.production.svc.cluster.local
    - spring-app.production
  privateKey:
    algorithm: ECDSA
    size: 256
```

### 4.2 Java KeyStore 初始化

```yaml
# Init Container 将 K8s Secret 转换为 Java KeyStore
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spring-app
spec:
  template:
    spec:
      initContainers:
        - name: create-keystore
          image: eclipse-temurin:21-jre
          command:
            - sh
            - -c
            - |
              # 导入 CA 证书到 TrustStore
              keytool -importcert \
                -noprompt \
                -alias ca-cert \
                -file /certs/tls.crt \
                -keystore /truststore/truststore.jks \
                -storepass changeit \
                -storetype JKS

              # 从 PKCS12 创建 KeyStore
              openssl pkcs12 -export \
                -in /certs/tls.crt \
                -inkey /certs/tls.key \
                -out /keystore/keystore.p12 \
                -passout pass:changeit
          volumeMounts:
            - name: tls-certs
              mountPath: /certs
              readOnly: true
            - name: truststore
              mountPath: /truststore
            - name: keystore
              mountPath: /keystore
      containers:
        - name: app
          env:
            - name: JAVA_OPTS
              value: >-
                -Djavax.net.ssl.trustStore=/truststore/truststore.jks
                -Djavax.net.ssl.trustStorePassword=changeit
                -Djavax.net.ssl.keyStore=/keystore/keystore.p12
                -Djavax.net.ssl.keyStorePassword=changeit
                -Djavax.net.ssl.keyStoreType=PKCS12
          volumeMounts:
            - name: truststore
              mountPath: /truststore
              readOnly: true
            - name: keystore
              mountPath: /keystore
              readOnly: true
      volumes:
        - name: tls-certs
          secret:
            secretName: spring-app-tls
        - name: truststore
          emptyDir:
            medium: Memory
        - name: keystore
          emptyDir:
            medium: Memory
```

### 4.3 Vault 集成 (CSI Driver)

```yaml
# Vault CSI Secret Provider
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: vault-spring-app
spec:
  provider: vault
  parameters:
    vaultAddress: "https://vault.production.svc.cluster.local:8200"
    roleName: "spring-app"
    objects: |
      - objectName: "db-password"
        secretPath: "secret/data/production/database"
        secretKey: "password"
      - objectName: "api-key"
        secretPath: "secret/data/production/external-api"
        secretKey: "apiKey"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spring-app
spec:
  template:
    spec:
      containers:
        - name: app
          volumeMounts:
            - name: secrets-store
              mountPath: /mnt/secrets
              readOnly: true
      volumes:
        - name: secrets-store
          csi:
            driver: secrets-store.csi.k8s.io
            readOnly: true
            volumeAttributes:
              secretProviderClass: vault-spring-app
```

---

## 五、Spring Security 集成

### 5.1 OAuth2 / OIDC 集成

```yaml
# application.yml
spring:
  security:
    oauth2:
      resourceserver:
        jwt:
          issuer-uri: https://keycloak.production.svc.cluster.local/realms/myapp
          jwk-set-uri: https://keycloak.production.svc.cluster.local/realms/myapp/protocol/openid-connect/certs
```

```java
@Configuration
@EnableWebSecurity
public class SecurityConfig {

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            .authorizeHttpRequests(auth -> auth
                .requestMatchers("/actuator/health/**").permitAll()
                .requestMatchers("/actuator/prometheus").permitAll()
                .requestMatchers("/api/**").authenticated()
                .anyRequest().denyAll()
            )
            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> jwt
                    .jwtAuthenticationConverter(jwtAuthenticationConverter())
                )
            )
            .sessionManagement(session -> session
                .sessionCreationPolicy(SessionCreationPolicy.STATELESS)
            )
            .csrf(csrf -> csrf.disable())
            .headers(headers -> headers
                .contentSecurityPolicy(csp -> csp
                    .policyDirectives("default-src 'self'")
                )
                .frameOptions(HeadersConfigurer.FrameOptionsConfig::deny)
            );
        return http.build();
    }
}
```

### 5.2 K8s Service Account Token 验证

```java
@Configuration
public class K8sServiceAccountSecurity {

    @Bean
    public JwtDecoder k8sJwtDecoder() {
        return NimbusJwtDecoder
            .withJwkSetUri("https://kubernetes.default.svc/openid/v1/jwks")
            .build();
    }
}
```

---

## 六、依赖安全与 SBOM

### 6.1 Maven 依赖扫描

```bash
# OWASP Dependency-Check
./mvnw org.owasp:dependency-check-maven:check

# CycloneDX SBOM 生成
./mvnw org.cyclonedx:cyclonedx-maven-plugin:makeBom

# Trivy 扫描 JAR
trivy fs --scanners vuln target/

# Syft SBOM (容器级)
syft registry.example.com/spring-app:v1.0.0 -o cyclonedx-json > sbom.json

# Grype 漏洞扫描
grype sbom:./sbom.json --fail-on high
```

### 6.2 Log4Shell 防御

```yaml
env:
  - name: JAVA_OPTS
    value: >-
      -Dlog4j2.formatMsgNoLookups=true
      -Dlog4j2.disableJmx=true
```

### 6.3 CI/CD 安全扫描集成

```yaml
# Tekton Pipeline 安全扫描
apiVersion: tekton.dev/v1
kind: Task
metadata:
  name: java-security-scan
spec:
  steps:
    - name: dependency-check
      image: owasp/dependency-check-action:latest
      args:
        - --project
        - spring-app
        - --scan
        - /workspace/source
        - --format
        - JSON
        - --out
        - /workspace/output
    - name: trivy-scan
      image: aquasec/trivy:latest
      args:
        - fs
        - --severity
        - HIGH,CRITICAL
        - --exit-code
        - "1"
        - /workspace/source/target
```

---

## 七、安全编码实践

### 7.1 输入验证

```java
@RestController
@RequestMapping("/api/orders")
public class OrderController {

    @PostMapping
    public ResponseEntity<OrderDto> createOrder(
            @Valid @RequestBody CreateOrderRequest request) {
        return ResponseEntity.ok(orderService.createOrder(request));
    }
}

record CreateOrderRequest(
    @NotBlank @Size(max = 100) String productName,
    @NotNull @Positive Integer quantity,
    @DecimalMin("0.01") @DecimalMax("999999.99") BigDecimal price
) {}
```

### 7.2 SQL 注入防护

```java
@Repository
public class OrderRepository {

    private final JdbcTemplate jdbc;

    public List<Order> findByCustomerId(Long customerId) {
        return jdbc.query(
            "SELECT * FROM orders WHERE customer_id = ?",
            (rs, rowNum) -> mapOrder(rs),
            customerId
        );
    }
}
```

### 7.3 敏感数据脱敏

```yaml
# application.yml
logging:
  pattern:
    console: "%d{yyyy-MM-dd HH:mm:ss.SSS} [%thread] %-5level %logger{36} - %msg%n"
```

---

## 八、网络安全策略

### 8.1 NetworkPolicy: Java 微服务

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: spring-app-netpol
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: spring-app
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: ingress-nginx
      ports:
        - port: 8080
          protocol: TCP
    - from:
        - namespaceSelector:
            matchLabels:
              name: monitoring
      ports:
        - port: 8081
          protocol: TCP
  egress:
    - to:
        - podSelector:
            matchLabels:
              app: postgres
      ports:
        - port: 5432
          protocol: TCP
    - to:
        - podSelector:
            matchLabels:
              app: redis
      ports:
        - port: 6379
          protocol: TCP
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - port: 53
          protocol: UDP
        - port: 53
          protocol: TCP
```

---

## 九、审计与合规

### 9.1 Spring Boot Actuator 审计日志

```yaml
management:
  endpoints:
    web:
      exposure:
        include: health,info,prometheus
  endpoint:
    auditevents:
      enabled: true
```

### 9.2 K8s 审计策略

```yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["secrets"]
    namespaces: ["production"]
  - level: Metadata
    resources:
      - group: ""
        resources: ["pods", "deployments"]
    verbs: ["delete", "patch"]
```

---

## 十、安全检查清单

### 10.1 Java K8s 安全基线

| 类别 | 检查项 | 命令/方法 | 优先级 |
|------|--------|----------|--------|
| **容器** | 非 root 运行 | `runAsNonRoot: true` | P0 |
| **容器** | 只读文件系统 | `readOnlyRootFilesystem: true` | P0 |
| **容器** | 丢弃所有能力 | `capabilities.drop: [ALL]` | P0 |
| **容器** | Distroless 基础镜像 | `gcr.io/distroless/java21` | P1 |
| **密钥** | 无硬编码密码 | 代码审计 | P0 |
| **密钥** | Secret 而非 ConfigMap | K8s Manifest 审计 | P0 |
| **密钥** | KeyStore 内存挂载 | `emptyDir.medium: Memory` | P1 |
| **网络** | NetworkPolicy | K8s Manifest | P1 |
| **网络** | mTLS (服务网格) | Istio/Linkerd | P2 |
| **依赖** | SBOM 生成 | `syft` / `cyclonedx-maven-plugin` | P1 |
| **依赖** | 漏洞扫描 | `trivy` / `grype` | P1 |
| **依赖** | Log4Shell 防御 | `-Dlog4j2.formatMsgNoLookups=true` | P0 |
| **镜像** | 镜像签名 | `cosign sign` | P1 |
| **镜像** | 固定版本标签 | 不使用 `latest` | P0 |
| **代码** | 输入验证 | `@Valid` / Bean Validation | P0 |
| **代码** | SQL 参数化 | JPA / JdbcTemplate 参数绑定 | P0 |
| **认证** | OAuth2/OIDC | Spring Security | P1 |
| **审计** | 操作审计日志 | AuditEvents | P2 |

---

## 🔗 相关文档

- [Spring Boot on K8s](../domain-4-workloads/99-spring-boot-kubernetes-guide.md) — Spring Boot 部署实践
- [Vault K8s Secrets](./99-vault-k8s-secrets-guide.md) — Vault 密钥管理
- [Kyverno Policy](./99-kyverno-policy-guide.md) — K8s 策略管理
- [供应链安全](../domain-39-supply-chain-security/99-slsa-supply-chain-security-guide.md) — SBOM/SLSA 实践
- [Java 容器化](../domain-13-docker/12-java-containerization-guide.md) — 容器安全基础
