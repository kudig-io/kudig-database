# cert-manager 自动证书管理实践指南

> **适用版本**: cert-manager v1.17  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、架构与组件](#一架构与组件)
- [二、Helm 部署](#二helm-部署)
- [三、Issuer vs ClusterIssuer](#三issuer-vs-clusterissuer)
- [四、ACME / Let's Encrypt 自动签发](#四acme--lets-encrypt-自动签发)
- [五、私有 CA 与内部证书](#五私有-ca-与内部证书)
- [六、证书自动轮换](#六证书自动轮换)
- [七、Ingress TLS 自动化](#七ingress-tls-自动化)
- [八、监控与告警](#八监控与告警)
- [九、与外部 Vault CA 集成](#九与外部-vault-ca-集成)

---

## 一、架构与组件

```
cert-manager 架构
├── cert-manager Controller
│   ├── Issuer Controller     ← 管理 Issuer/ClusterIssuer
│   ├── Certificate Controller ← 管理 Certificate 资源
│   ├── Order Controller      ← ACME 订单管理
│   ├── Challenge Controller  ← ACME 域名验证
│   └── CertificateRequest    ← CSR 生命周期
│
├── CRD 资源
│   ├── Issuer / ClusterIssuer  ← 证书颁发机构配置
│   ├── Certificate             ← 证书请求声明
│   ├── CertificateRequest      ← 内部 CSR 对象
│   ├── Order                   ← ACME 订单
│   └── Challenge               ← ACME 域名验证挑战
│
└── 支持的 Issuer 类型
    ├── ACME (Let's Encrypt, ZeroSSL, Google Trust Services)
    ├── Vault (HashiCorp Vault PKI)
    ├── CA (内部私有 CA)
    ├── SelfSigned (自签名)
    ├── Venafi (企业级)
    └── AWS PCA / Google CAS (云托管 CA)
```

---

## 二、Helm 部署

```bash
helm repo add jetstack https://charts.jetstack.io
helm repo update

helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager \
  --create-namespace \
  --version v1.17.0 \
  --set installCRDs=true \
  --set prometheus.enabled=true \
  --set prometheus.servicemonitor.enabled=true
```

### 生产级 values

```yaml
# values-cert-manager.yaml
replicaCount: 2

resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: 500m
    memory: 512Mi

podDisruptionBudget:
  enabled: true
  minAvailable: 1

# 高可用
affinity:
  podAntiAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchExpressions:
              - key: app.kubernetes.io/name
                operator: In
                values:
                  - cert-manager
          topologyKey: kubernetes.io/hostname

# Prometheus 监控
prometheus:
  enabled: true
  servicemonitor:
    enabled: true
    namespace: monitoring

# 启用所有实验性功能
featureGables:
  - AdditionalCertificateOutputFormats
  - ExperimentalGatewayAPISupport
```

---

## 三、Issuer vs ClusterIssuer

| 类型 | 作用域 | 适用场景 |
|:---|:---|:---|
| Issuer | 单个 Namespace | 团队级隔离、多租户环境 |
| ClusterIssuer | 全局集群 | 共享 CA、集中管理 |

---

## 四、ACME / Let's Encrypt 自动签发

### 4.1 HTTP-01 挑战 (最常用)

```yaml
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-key
    solvers:
    - http01:
        ingress:
          class: nginx
          # 或指定 Ingress 名称
          # name: my-ingress
          # 或 Gateway API
          # gatewayHTTPRoute:
          #   parentRefs:
          #   - name: my-gateway
      selector:
        dnsZones:
        - "example.com"
---
# 开发/测试环境
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-staging
spec:
  acme:
    server: https://acme-staging-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-staging-key
    solvers:
    - http01:
        ingress:
          class: nginx
```

### 4.2 DNS-01 挑战 (通配符证书)

```yaml
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-dns
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-dns-key
    solvers:
    - dns01:
        route53:
          region: us-east-1
          hostedZoneID: Z1234567890ABC
          role: arn:aws:iam::123456789012:role/cert-manager-dns
        # 或 Cloudflare
        # cloudflare:
        #   apiTokenSecretRef:
        #     name: cloudflare-api-token
        #     key: api-token
        # 或 Alibaba Cloud DNS
        # alidns:
        #   accessKeyIdSecretRef:
        #     name: alidns-secret
        #     key: access-key-id
      selector:
        dnsZones:
        - "example.com"
        - "*.example.com"
```

### 4.3 申请证书

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: example-com
  namespace: default
spec:
  secretName: example-com-tls
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
    - example.com
    - www.example.com
    - api.example.com
  # 私钥算法
  privateKey:
    algorithm: RSA
    encoding: PKCS1
    size: 2048
  # 或 ECDSA (更现代)
  # privateKey:
  #   algorithm: ECDSA
  #   size: 256
  # 证书用途
  usages:
    - digital signature
    - key encipherment
```

---

## 五、私有 CA 与内部证书

### 5.1 自建 CA Issuer

```bash
# 创建 CA 私钥和证书
openssl genrsa -out ca.key 4096
openssl req -x509 -new -nodes -key ca.key \
  -sha256 -days 3650 \
  -out ca.crt \
  -subj "/CN=MyOrg Internal CA/O=MyOrg"

# 创建 Secret
kubectl create secret tls ca-key-pair \
  --cert=ca.crt \
  --key=ca.key \
  --namespace=cert-manager
```

```yaml
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: myorg-ca
spec:
  ca:
    secretName: ca-key-pair
```

### 5.2 自动签发内部证书

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: internal-app
  namespace: production
spec:
  secretName: internal-app-tls
  issuerRef:
    name: myorg-ca
    kind: ClusterIssuer
  dnsNames:
    - app.production.svc.cluster.local
    - app.production
    - app
  # 短 TTL 适合内部证书自动轮换
  duration: 2160h      # 90 天
  renewBefore: 360h    # 15 天前开始续期
```

---

## 六、证书自动轮换

```
证书生命周期
  │
  ▼
创建 Certificate ──► 签发 ──► 使用 ──► 续期窗口 ──► 新证书 ──► 旧证书过期
  │                              ▲                              │
  │                              │                              ▼
  │                         renewBefore                    自动删除
  │                         (默认 30 天)
  │
  └── 失败重试: 指数退避 (1h, 2h, 4h, 8h, 16h, 32h)
```

### 监控轮换状态

```bash
# 查看证书状态
kubectl get certificates --all-namespaces
kubectl describe certificate example-com

# 查看证书有效期
kubectl get secret example-com-tls -o jsonpath='{.data.tls\.crt}' | \
  base64 -d | openssl x509 -noout -dates

# 查看 cert-manager 日志
kubectl logs -n cert-manager deployment/cert-manager
```

---

## 七、Ingress TLS 自动化

### 7.1 自动证书注解

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: myapp
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    # 或使用 namespace 级 Issuer
    # cert-manager.io/issuer: "my-issuer"
    
    # 自动重定向 HTTP → HTTPS
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-example-com-tls    # cert-manager 自动创建
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: myapp
            port:
              number: 80
```

### 7.2 Gateway API TLS (实验性)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: example-gateway
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  gatewayClassName: nginx
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    hostname: "*.example.com"
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: wildcard-example-com-tls
---
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: wildcard-example-com
spec:
  secretName: wildcard-example-com-tls
  issuerRef:
    name: letsencrypt-dns
    kind: ClusterIssuer
  dnsNames:
    - "*.example.com"
```

---

## 八、监控与告警

### 8.1 Prometheus 告警规则

```yaml
- alert: CertificateExpiringSoon
  expr: |
    (
      certmanager_certificate_expiration_timestamp_seconds - time()
    ) / 86400 < 14
  for: 1h
  labels:
    severity: warning
  annotations:
    summary: "证书 {{ $labels.name }} 将在 14 天内过期"

- alert: CertificateExpired
  expr: |
    certmanager_certificate_expiration_timestamp_seconds - time() < 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "证书 {{ $labels.name }} 已过期"

- alert: CertificateNotReady
  expr: |
    certmanager_certificate_ready_status{condition="False"} == 1
  for: 15m
  labels:
    severity: critical
  annotations:
    summary: "证书 {{ $labels.name }} 未就绪"

- alert: ACMEOrderFailed
  expr: |
    rate(certmanager_acme_client_request_count{status="failed"}[5m]) > 0
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "ACME 订单失败"
```

### 8.2 证书审计脚本

```bash
#!/bin/bash
# 检查所有命名空间证书有效期
echo "证书有效期检查:"
echo "=================="

kubectl get certificates --all-namespaces -o json | \
  jq -r '.items[] | 
    .metadata.namespace + "/" + .metadata.name + 
    " | Not After: " + .status.notAfter + 
    " | Ready: " + (.status.conditions[]? | select(.type=="Ready") | .status)' |
  while read line; do
    echo "$line"
  done
```

---

## 九、与外部 Vault CA 集成

```yaml
apiVersion: cert-manager.io/v1
kind: Issuer
metadata:
  name: vault-issuer
  namespace: production
spec:
  vault:
    server: https://vault.vault.svc.cluster.local:8200
    path: pki/sign/myapp
    auth:
      kubernetes:
        mountPath: /v1/auth/kubernetes
        role: cert-manager
        secretRef:
          name: cert-manager-vault-token
          key: token
---
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: vault-signed-cert
  namespace: production
spec:
  secretName: vault-cert-tls
  issuerRef:
    name: vault-issuer
    kind: Issuer
  dnsNames:
    - app.production.svc.cluster.local
  # Vault 自动处理轮换
```

---

## 参考链接

- [cert-manager 官方文档](https://cert-manager.io/docs/)
- [cert-manager Helm Chart](https://artifacthub.io/packages/helm/jetstack/cert-manager)
- [Let's Encrypt 文档](https://letsencrypt.org/docs/)
- [Gateway API TLS](https://cert-manager.io/docs/usage/gateway/)
- [Vault PKI 集成](https://cert-manager.io/docs/configuration/vault/)
