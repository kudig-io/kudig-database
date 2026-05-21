---
title: 证书管理与 TLS 配置
description: '# 证书管理与 TLS 配置'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 证书管理与 TLS 配置 是什么
- 如何 证书管理与 TLS 配置
- Kubernetes 7 security 最佳实践
trigger_keywords:
- 证书管理与
- TLS
- 配置
- security
prerequisites:
- kubectl-basics
- rbac-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- tls-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/certificate-fta.md
  label: '故障树: certificate'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
---

# 证书管理与 TLS 配置

<!-- chunk: 概述 -->
## 概述

证书管理是 Kubernetes 安全体系的核心组成部分,涉及集群组件通信加密、服务间 mTLS、Ingress HTTPS 等多个层面。本文档详细介绍 Kubernetes 证书体系、cert-manager 部署配置和证书生命周期管理。

<!-- chunk: 证书架构 -->
## 证书架构

### Kubernetes 证书体系

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Kubernetes 证书体系架构                                    │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          集群 PKI 层级结构                                   │   │
│   │                                                                              │   │
│   │                        ┌─────────────────┐                                  │   │
│   │                        │   Cluster CA    │                                  │   │
│   │                        │   (根证书)       │                                  │   │
│   │                        │   10年有效期     │                                  │   │
│   │                        └────────┬────────┘                                  │   │
│   │                                 │                                           │   │
│   │           ┌─────────────────────┼─────────────────────┐                    │   │
│   │           │                     │                     │                    │   │
│   │           ▼                     ▼                     ▼                    │   │
│   │   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐           │   │
│   │   │  API Server  │      │    etcd      │      │ Front Proxy  │           │   │
│   │   │    证书       │      │    CA        │      │    CA        │           │   │
│   │   └──────┬───────┘      └──────┬───────┘      └──────┬───────┘           │   │
│   │          │                     │                     │                    │   │
│   │          ▼                     ▼                     ▼                    │   │
│   │   ┌──────────────┐      ┌──────────────┐      ┌──────────────┐           │   │
│   │   │ apiserver    │      │ etcd-server  │      │ front-proxy  │           │   │
│   │   │ apiserver-   │      │ etcd-peer    │      │   -client    │           │   │
│   │   │ kubelet-     │      │ etcd-        │      └──────────────┘           │   │
│   │   │   client     │      │ healthcheck  │                                 │   │
│   │   │ apiserver-   │      └──────────────┘                                 │   │
│   │   │   etcd-      │                                                       │   │
│   │   │   client     │                                                       │   │
│   │   └──────────────┘                                                       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          节点级证书                                          │   │
│   │                                                                              │   │
│   │   ┌──────────────────────────────────────────────────────────────────────┐  │   │
│   │   │                           Node                                        │  │   │
│   │   │                                                                       │  │   │
│   │   │   ┌─────────────────┐        ┌─────────────────┐                    │  │   │
│   │   │   │    kubelet      │        │   kube-proxy    │                    │  │   │
│   │   │   │                 │        │                 │                    │  │   │
│   │   │   │ • 客户端证书    │        │ • kubeconfig    │                    │  │   │
│   │   │   │ • 服务端证书    │        │   (证书认证)    │                    │  │   │
│   │   │   │ • 自动轮换      │        │                 │                    │  │   │
│   │   │   └─────────────────┘        └─────────────────┘                    │  │   │
│   │   │                                                                       │  │   │
│   │   └──────────────────────────────────────────────────────────────────────┘  │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                        应用层证书 (cert-manager)                             │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐                   │   │
│   │   │ ClusterIssuer│   │   Issuer     │   │ Certificate  │                   │   │
│   │   │              │   │              │   │              │                   │   │
│   │   │ • Let's      │   │ • Self-      │   │ • TLS Secret │                   │   │
│   │   │   Encrypt    │   │   Signed     │   │ • 自动续期   │                   │   │
│   │   │ • CA         │   │ • Vault      │   │              │                   │   │
│   │   │ • Vault      │   │ • Venafi     │   │              │                   │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘                   │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 证书通信流程

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              TLS 通信流程                                            │
│                                                                                      │
│   客户端                                                              服务端         │
│      │                                                                   │          │
│      │  1. Client Hello (支持的TLS版本、密码套件)                        │          │
│      │ ─────────────────────────────────────────────────────────────────►│          │
│      │                                                                   │          │
│      │  2. Server Hello + Server Certificate                            │          │
│      │ ◄─────────────────────────────────────────────────────────────────│          │
│      │                                                                   │          │
│      │  3. 验证服务端证书 (CA签名、有效期、CN/SAN)                       │          │
│      │  ┌────────────────────────────────────────┐                      │          │
│      │  │ • 检查证书链                            │                      │          │
│      │  │ • 验证签名                              │                      │          │
│      │  │ • 检查有效期                            │                      │          │
│      │  │ • 验证 CN/SAN 匹配                      │                      │          │
│      │  └────────────────────────────────────────┘                      │          │
│      │                                                                   │          │
│      │  4. Client Certificate (mTLS 模式)                               │          │
│      │ ─────────────────────────────────────────────────────────────────►│          │
│      │                                                                   │          │
│      │                                                  5. 验证客户端证书 │          │
│      │                                                                   │          │
│      │  6. Key Exchange + Finished                                       │          │
│      │ ◄────────────────────────────────────────────────────────────────►│          │
│      │                                                                   │          │
│      │  7. 加密通信 (Application Data)                                   │          │
│      │ ◄════════════════════════════════════════════════════════════════►│          │
│      │                                                                   │          │
└──────┴───────────────────────────────────────────────────────────────────┴──────────┘
```

<!-- chunk: 证书类型详解 -->
## 证书类型详解

### Kubernetes 集群证书

| 证书类型 | 文件路径 | 用途 | 有效期建议 | 签发者 |
|---------|---------|------|-----------|-------|
| **Cluster CA** | /etc/kubernetes/pki/ca.crt | 签发集群证书 | 10 年 | 自签名 |
| **API Server** | /etc/kubernetes/pki/apiserver.crt | API Server HTTPS | 1 年 | Cluster CA |
| **API Server kubelet Client** | /etc/kubernetes/pki/apiserver-kubelet-client.crt | API→kubelet 认证 | 1 年 | Cluster CA |
| **API Server etcd Client** | /etc/kubernetes/pki/apiserver-etcd-client.crt | API→etcd 认证 | 1 年 | etcd CA |
| **etcd CA** | /etc/kubernetes/pki/etcd/ca.crt | 签发 etcd 证书 | 10 年 | 自签名 |
| **etcd Server** | /etc/kubernetes/pki/etcd/server.crt | etcd 服务端 | 1 年 | etcd CA |
| **etcd Peer** | /etc/kubernetes/pki/etcd/peer.crt | etcd 集群通信 | 1 年 | etcd CA |
| **Front Proxy CA** | /etc/kubernetes/pki/front-proxy-ca.crt | 聚合层 CA | 10 年 | 自签名 |
| **Front Proxy Client** | /etc/kubernetes/pki/front-proxy-client.crt | API 聚合认证 | 1 年 | Front Proxy CA |
| **SA Key Pair** | /etc/kubernetes/pki/sa.key | ServiceAccount 签名 | - | N/A |

### kubeadm 证书目录结构

```
/etc/kubernetes/
├── pki/
│   ├── ca.crt                           # Cluster CA 证书
│   ├── ca.key                           # Cluster CA 私钥
│   ├── apiserver.crt                    # API Server 证书
│   ├── apiserver.key                    # API Server 私钥
│   ├── apiserver-kubelet-client.crt     # API→kubelet 客户端证书
│   ├── apiserver-kubelet-client.key
│   ├── apiserver-etcd-client.crt        # API→etcd 客户端证书
│   ├── apiserver-etcd-client.key
│   ├── front-proxy-ca.crt               # Front Proxy CA
│   ├── front-proxy-ca.key
│   ├── front-proxy-client.crt           # Front Proxy 客户端
│   ├── front-proxy-client.key
│   ├── sa.key                           # ServiceAccount 私钥
│   ├── sa.pub                           # ServiceAccount 公钥
│   └── etcd/
│       ├── ca.crt                       # etcd CA
│       ├── ca.key
│       ├── server.crt                   # etcd 服务端证书
│       ├── server.key
│       ├── peer.crt                     # etcd 集群通信证书
│       ├── peer.key
│       ├── healthcheck-client.crt       # 健康检查客户端
│       └── healthcheck-client.key
├── admin.conf                           # 管理员 kubeconfig
├── controller-manager.conf              # Controller Manager kubeconfig
├── scheduler.conf                       # Scheduler kubeconfig
└── kubelet.conf                         # kubelet kubeconfig

/var/lib/kubelet/pki/
├── kubelet.crt                          # kubelet 服务端证书
├── kubelet.key
├── kubelet-client-current.pem           # kubelet 客户端证书 (自动轮换)
└── kubelet-client-*.pem                 # 历史客户端证书
```

<!-- chunk: 证书管理操作 -->
## 证书管理操作

### 证书检查命令

```bash
#!/bin/bash
# certificate-check.sh
# Kubernetes 证书检查脚本

set -e

echo "=========================================="
echo "     Kubernetes 证书检查报告"
echo "=========================================="
echo ""

# 使用 kubeadm 检查证书过期时间
echo "=== 1. kubeadm 证书状态 ==="
kubeadm certs check-expiration 2>/dev/null || echo "kubeadm 不可用,使用 openssl 检查"
echo ""

# 定义证书路径
CERT_DIR="/etc/kubernetes/pki"
ETCD_DIR="/etc/kubernetes/pki/etcd"

# 检查单个证书的函数
check_cert() {
    local cert_file=$1
    local cert_name=$2
    
    if [ -f "$cert_file" ]; then
        local expiry=$(openssl x509 -in "$cert_file" -noout -enddate 2>/dev/null | cut -d= -f2)
        local expiry_epoch=$(date -d "$expiry" +%s 2>/dev/null || date -j -f "%b %d %T %Y %Z" "$expiry" +%s 2>/dev/null)
        local now_epoch=$(date +%s)
        local days_left=$(( (expiry_epoch - now_epoch) / 86400 ))
        
        local status="✅"
        if [ $days_left -lt 30 ]; then
            status="🔴"
        elif [ $days_left -lt 90 ]; then
            status="🟡"
        fi
        
        printf "%-40s %s 剩余 %d 天 (%s)\n" "$cert_name" "$status" "$days_left" "$expiry"
    else
        printf "%-40s ❌ 文件不存在\n" "$cert_name"
    fi
}

echo "=== 2. 控制平面证书 ==="
check_cert "$CERT_DIR/ca.crt" "Cluster CA"
check_cert "$CERT_DIR/apiserver.crt" "API Server"
check_cert "$CERT_DIR/apiserver-kubelet-client.crt" "API Server Kubelet Client"
check_cert "$CERT_DIR/apiserver-etcd-client.crt" "API Server etcd Client"
check_cert "$CERT_DIR/front-proxy-ca.crt" "Front Proxy CA"
check_cert "$CERT_DIR/front-proxy-client.crt" "Front Proxy Client"
echo ""

echo "=== 3. etcd 证书 ==="
check_cert "$ETCD_DIR/ca.crt" "etcd CA"
check_cert "$ETCD_DIR/server.crt" "etcd Server"
check_cert "$ETCD_DIR/peer.crt" "etcd Peer"
check_cert "$ETCD_DIR/healthcheck-client.crt" "etcd Healthcheck Client"
echo ""

echo "=== 4. kubelet 证书 ==="
check_cert "/var/lib/kubelet/pki/kubelet.crt" "kubelet Server"
check_cert "/var/lib/kubelet/pki/kubelet-client-current.pem" "kubelet Client"
echo ""

echo "=== 5. 证书详细信息 ==="
echo "--- API Server 证书 SAN ---"
openssl x509 -in "$CERT_DIR/apiserver.crt" -noout -text 2>/dev/null | grep -A1 "Subject Alternative Name" || echo "无法读取"
echo ""

echo "--- 证书签名算法 ---"
openssl x509 -in "$CERT_DIR/apiserver.crt" -noout -text 2>/dev/null | grep "Signature Algorithm" | head -1 || echo "无法读取"
echo ""

echo "=========================================="
echo "       检查完成"
echo "=========================================="
```

### kubeadm 证书更新

```bash
#!/bin/bash
# certificate-renew.sh
# kubeadm 证书更新脚本

set -e

echo "=== Kubernetes 证书更新 ==="
echo ""

# 备份现有证书
BACKUP_DIR="/etc/kubernetes/pki.backup.$(date +%Y%m%d%H%M%S)"
echo "1. 备份现有证书到 $BACKUP_DIR"
cp -r /etc/kubernetes/pki "$BACKUP_DIR"
cp /etc/kubernetes/*.conf "$BACKUP_DIR/" 2>/dev/null || true
echo "   备份完成"
echo ""

# 更新所有证书
echo "2. 更新所有证书"
kubeadm certs renew all
echo ""

# 或者更新特定证书
# echo "2. 更新特定证书"
# kubeadm certs renew apiserver
# kubeadm certs renew apiserver-kubelet-client
# kubeadm certs renew apiserver-etcd-client
# kubeadm certs renew front-proxy-client
# kubeadm certs renew etcd-server
# kubeadm certs renew etcd-peer
# kubeadm certs renew etcd-healthcheck-client

# 更新 kubeconfig 文件
echo "3. 更新 kubeconfig 文件"
kubeadm certs renew admin.conf
kubeadm certs renew controller-manager.conf
kubeadm certs renew scheduler.conf
echo ""

# 重启控制平面组件
echo "4. 重启控制平面组件"

# 方法1: 如果使用静态 Pod
echo "   重启 kubelet..."
systemctl restart kubelet

# 等待组件重启
echo "   等待组件重启..."
sleep 30

# 方法2: 手动删除静态 Pod (强制重新创建)
# mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
# mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/
# mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/
# sleep 10
# mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
# mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/
# mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/

# 验证更新
echo "5. 验证证书更新"
kubeadm certs check-expiration
echo ""

# 更新 ~/.kube/config
echo "6. 更新用户 kubeconfig"
cp /etc/kubernetes/admin.conf ~/.kube/config
chown $(id -u):$(id -g) ~/.kube/config
echo ""

# 验证集群状态
echo "7. 验证集群状态"
kubectl get nodes
kubectl get pods -n kube-system
echo ""

echo "=== 证书更新完成 ==="
```

<!-- chunk: cert-manager 部署 -->
## cert-manager 部署

### Helm 安装

```bash
#!/bin/bash
# deploy-cert-manager.sh
# cert-manager 部署脚本

set -e

VERSION="v1.14.0"
NAMESPACE="cert-manager"

echo "=== 部署 cert-manager $VERSION ==="

# 添加 Helm 仓库
helm repo add jetstack https://charts.jetstack.io
helm repo update

# 创建命名空间
kubectl create namespace $NAMESPACE --dry-run=client -o yaml | kubectl apply -f -

# 安装 cert-manager
helm upgrade --install cert-manager jetstack/cert-manager \
  --namespace $NAMESPACE \
  --version $VERSION \
  --set installCRDs=true \
  --set prometheus.enabled=true \
  --set webhook.timeoutSeconds=30 \
  --set extraArgs='{--dns01-recursive-nameservers-only,--dns01-recursive-nameservers=8.8.8.8:53\,1.1.1.1:53}' \
  --wait

# 等待 Pod 就绪
echo "等待 cert-manager Pod 就绪..."
kubectl wait --for=condition=Ready pod \
  -l app.kubernetes.io/instance=cert-manager \
  -n $NAMESPACE \
  --timeout=120s

# 验证安装
echo "验证 cert-manager 安装..."
kubectl get pods -n $NAMESPACE
cmctl check api --wait=2m || echo "cmctl 未安装,跳过 API 检查"

echo "=== cert-manager 部署完成 ==="
```

### cert-manager CRD 说明

| CRD | 作用域 | 用途 | 说明 |
|-----|-------|------|------|
| **Issuer** | Namespaced | 命名空间级证书签发者 | 只能签发同命名空间的证书 |
| **ClusterIssuer** | Cluster | 集群级证书签发者 | 可签发任意命名空间的证书 |
| **Certificate** | Namespaced | 证书请求 | 定义所需证书的规格 |
| **CertificateRequest** | Namespaced | 证书签发请求 | 由 Certificate 自动创建 |
| **Order** | Namespaced | ACME 订单 | ACME 协议订单状态 |
| **Challenge** | Namespaced | ACME 挑战 | DNS01/HTTP01 挑战记录 |

<!-- chunk: Issuer 配置 -->
## Issuer 配置

### Let's Encrypt 配置

```yaml
# letsencrypt-issuer.yaml
# Let's Encrypt 证书签发者配置

---
# 生产环境 ClusterIssuer (Let's Encrypt Production)
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    # Let's Encrypt 生产服务器
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    # 私钥存储
    privateKeySecretRef:
      name: letsencrypt-prod-account-key
    # 验证方式
    solvers:
      # HTTP01 验证 (适用于 Ingress)
      - http01:
          ingress:
            class: nginx
        selector:
          dnsZones:
            - "example.com"
            
      # DNS01 验证 (适用于通配符证书)
      - dns01:
          cloudflare:
            email: admin@example.com
            apiTokenSecretRef:
              name: cloudflare-api-token
              key: api-token
        selector:
          dnsZones:
            - "example.com"
            
---
# 测试环境 ClusterIssuer (Let's Encrypt Staging)
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-staging
spec:
  acme:
    # Let's Encrypt 测试服务器 (不受速率限制)
    server: https://acme-staging-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-staging-account-key
    solvers:
      - http01:
          ingress:
            class: nginx

---
# Cloudflare API Token Secret
apiVersion: v1
kind: Secret
metadata:
  name: cloudflare-api-token
  namespace: cert-manager
type: Opaque
stringData:
  api-token: "your-cloudflare-api-token"
```

### 自签名和 CA Issuer

```yaml
# ca-issuer.yaml
# CA 和自签名证书签发者配置

---
# 自签名 Issuer (用于创建根 CA)
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: selfsigned-issuer
spec:
  selfSigned: {}

---
# 创建内部 CA 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: internal-ca
  namespace: cert-manager
spec:
  isCA: true
  commonName: internal-ca
  secretName: internal-ca-key-pair
  duration: 87600h  # 10年
  renewBefore: 8760h  # 1年前续期
  privateKey:
    algorithm: ECDSA
    size: 256
  issuerRef:
    name: selfsigned-issuer
    kind: ClusterIssuer
    group: cert-manager.io

---
# 使用内部 CA 的 ClusterIssuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: internal-ca-issuer
spec:
  ca:
    secretName: internal-ca-key-pair

---
# Vault Issuer (HashiCorp Vault PKI)
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: vault-issuer
spec:
  vault:
    path: pki/sign/kubernetes
    server: https://vault.example.com
    # 认证方式
    auth:
      kubernetes:
        role: cert-manager
        mountPath: /v1/auth/kubernetes
        secretRef:
          name: vault-token
          key: token
```

### 云厂商 DNS01 配置

```yaml
# cloud-dns-issuers.yaml
# 各云厂商 DNS01 验证配置

---
# AWS Route53
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-route53
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-route53-account
    solvers:
      - dns01:
          route53:
            region: us-east-1
            # 使用 IRSA (推荐)
            # 或使用 accessKeyID + secretAccessKeySecretRef

---
# Google Cloud DNS
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-clouddns
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-clouddns-account
    solvers:
      - dns01:
          cloudDNS:
            project: my-gcp-project
            serviceAccountSecretRef:
              name: clouddns-service-account
              key: credentials.json

---
# Azure DNS
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-azuredns
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-azuredns-account
    solvers:
      - dns01:
          azureDNS:
            subscriptionID: "subscription-id"
            resourceGroupName: "dns-resource-group"
            hostedZoneName: "example.com"
            environment: AzurePublicCloud
            # 使用 Managed Identity 或 Service Principal

---
# 阿里云 DNS
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-alidns
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-alidns-account
    solvers:
      - dns01:
          webhook:
            groupName: acme.yourcompany.com
            solverName: alidns
            config:
              regionId: cn-hangzhou
              accessKeySecretRef:
                name: alidns-credentials
                key: access-key
              secretKeySecretRef:
                name: alidns-credentials
                key: secret-key
```

<!-- chunk: Certificate 配置 -->
## Certificate 配置

### 完整 Certificate 示例

```yaml
# certificate-examples.yaml
# Certificate 配置示例

---
# 基础 HTTPS 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: myapp-tls
  namespace: production
spec:
  # 证书存储的 Secret 名称
  secretName: myapp-tls-secret
  
  # 证书有效期
  duration: 2160h      # 90天
  renewBefore: 360h    # 15天前自动续期
  
  # 证书属性
  isCA: false
  
  # 私钥配置
  privateKey:
    algorithm: RSA
    encoding: PKCS1
    size: 2048
    rotationPolicy: Always  # 续期时轮换私钥
    
  # 使用方式
  usages:
    - server auth
    - client auth
    
  # Subject 配置
  subject:
    organizations:
      - MyCompany
    organizationalUnits:
      - Engineering
      
  # DNS 名称 (SAN)
  dnsNames:
    - myapp.example.com
    - www.myapp.example.com
    - api.myapp.example.com
    
  # IP 地址 (SAN)
  ipAddresses:
    - 192.168.1.100
    
  # URI (SAN)
  uris:
    - spiffe://cluster.local/ns/production/sa/myapp
    
  # 签发者引用
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
    group: cert-manager.io

---
# 通配符证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: wildcard-tls
  namespace: production
spec:
  secretName: wildcard-tls-secret
  duration: 2160h
  renewBefore: 360h
  privateKey:
    algorithm: ECDSA
    size: 256
  dnsNames:
    - "*.example.com"
    - "example.com"
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer

---
# 内部服务 mTLS 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: backend-mtls
  namespace: production
spec:
  secretName: backend-mtls-secret
  duration: 8760h      # 1年
  renewBefore: 720h    # 30天前续期
  isCA: false
  privateKey:
    algorithm: ECDSA
    size: 256
  usages:
    - server auth
    - client auth
  dnsNames:
    - backend.production.svc.cluster.local
    - backend.production.svc
    - backend
  issuerRef:
    name: internal-ca-issuer
    kind: ClusterIssuer
```

### Ingress 自动证书

```yaml
# ingress-auto-tls.yaml
# Ingress 自动证书配置

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: myapp-ingress
  namespace: production
  annotations:
    # cert-manager 注解
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    cert-manager.io/common-name: "myapp.example.com"
    
    # 可选: 指定私钥算法
    cert-manager.io/private-key-algorithm: "ECDSA"
    cert-manager.io/private-key-size: "256"
    
    # 可选: 续期策略
    cert-manager.io/duration: "2160h"
    cert-manager.io/renew-before: "360h"
    
    # Nginx Ingress 注解
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
    - hosts:
        - myapp.example.com
        - www.myapp.example.com
      secretName: myapp-tls-auto  # cert-manager 自动创建
  rules:
    - host: myapp.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: myapp
                port:
                  number: 80
    - host: www.myapp.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: myapp
                port:
                  number: 80

---
# Gateway API 自动证书 (cert-manager v1.14+)
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: myapp-gateway
  namespace: production
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  gatewayClassName: nginx
  listeners:
    - name: https
      protocol: HTTPS
      port: 443
      hostname: "myapp.example.com"
      tls:
        mode: Terminate
        certificateRefs:
          - name: myapp-gateway-tls  # 自动创建
```

<!-- chunk: mTLS 配置 -->
## mTLS 配置

### Istio mTLS

```yaml
# istio-mtls.yaml
# Istio 服务网格 mTLS 配置

---
# 全局 mTLS 策略
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT  # STRICT: 强制 mTLS, PERMISSIVE: 兼容模式

---
# 命名空间级别 mTLS
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata:
  name: production-mtls
  namespace: production
spec:
  mtls:
    mode: STRICT
  # 特定端口例外
  portLevelMtls:
    8080:
      mode: PERMISSIVE  # 允许健康检查等

---
# 工作负载级别 mTLS
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata:
  name: backend-mtls
  namespace: production
spec:
  selector:
    matchLabels:
      app: backend
  mtls:
    mode: STRICT

---
# DestinationRule 配置客户端 mTLS
apiVersion: networking.istio.io/v1
kind: DestinationRule
metadata:
  name: backend-mtls
  namespace: production
spec:
  host: backend.production.svc.cluster.local
  trafficPolicy:
    tls:
      mode: ISTIO_MUTUAL  # 使用 Istio 证书
      # 或使用自定义证书
      # mode: MUTUAL
      # clientCertificate: /etc/certs/client.crt
      # privateKey: /etc/certs/client.key
      # caCertificates: /etc/certs/ca.crt
```

### 应用级 mTLS

```yaml
# app-mtls.yaml
# 应用级 mTLS 配置

---
# 服务端证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: server-mtls
  namespace: production
spec:
  secretName: server-mtls-secret
  duration: 8760h
  renewBefore: 720h
  usages:
    - server auth
  dnsNames:
    - server.production.svc.cluster.local
  issuerRef:
    name: internal-ca-issuer
    kind: ClusterIssuer

---
# 客户端证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: client-mtls
  namespace: production
spec:
  secretName: client-mtls-secret
  duration: 8760h
  renewBefore: 720h
  usages:
    - client auth
  commonName: client.production
  issuerRef:
    name: internal-ca-issuer
    kind: ClusterIssuer

---
# 使用 mTLS 的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mtls-server
  namespace: production
spec:
  replicas: 2
  selector:
    matchLabels:
      app: mtls-server
  template:
    metadata:
      labels:
        app: mtls-server
    spec:
      containers:
        - name: server
          image: nginx:alpine
          ports:
            - containerPort: 443
          volumeMounts:
            - name: server-certs
              mountPath: /etc/ssl/server
              readOnly: true
            - name: ca-certs
              mountPath: /etc/ssl/ca
              readOnly: true
            - name: nginx-conf
              mountPath: /etc/nginx/conf.d
      volumes:
        - name: server-certs
          secret:
            secretName: server-mtls-secret
        - name: ca-certs
          secret:
            secretName: internal-ca-key-pair
            items:
              - key: ca.crt
                path: ca.crt
        - name: nginx-conf
          configMap:
            name: nginx-mtls-config

---
# Nginx mTLS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-mtls-config
  namespace: production
data:
  default.conf: |
    server {
        listen 443 ssl;
        
        ssl_certificate /etc/ssl/server/tls.crt;
        ssl_certificate_key /etc/ssl/server/tls.key;
        
        # 启用客户端证书验证
        ssl_client_certificate /etc/ssl/ca/ca.crt;
        ssl_verify_client on;
        ssl_verify_depth 2;
        
        # TLS 配置
        ssl_protocols TLSv1.2 TLSv1.3;
        ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256;
        ssl_prefer_server_ciphers on;
        
        location / {
            root /usr/share/nginx/html;
            index index.html;
        }
    }
```

<!-- chunk: 监控告警 -->
## 监控告警

### Prometheus 监控规则

```yaml
# cert-monitoring-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-monitoring
  namespace: monitoring
spec:
  groups:
    - name: certificate.alerts
      interval: 1h
      rules:
        # 证书即将过期告警
        - alert: CertificateExpiringSoon
          expr: |
            certmanager_certificate_expiration_timestamp_seconds - time() < 86400 * 30
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "证书 {{ $labels.name }} 将在 30 天内过期"
            description: |
              命名空间: {{ $labels.namespace }}
              证书: {{ $labels.name }}
              剩余时间: {{ $value | humanizeDuration }}
              
        - alert: CertificateExpiringSoonCritical
          expr: |
            certmanager_certificate_expiration_timestamp_seconds - time() < 86400 * 7
          for: 1h
          labels:
            severity: critical
          annotations:
            summary: "证书 {{ $labels.name }} 将在 7 天内过期"
            
        # 证书已过期
        - alert: CertificateExpired
          expr: |
            certmanager_certificate_expiration_timestamp_seconds < time()
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "证书 {{ $labels.name }} 已过期"
            
        # 证书签发失败
        - alert: CertificateNotReady
          expr: |
            certmanager_certificate_ready_status{condition="True"} == 0
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "证书 {{ $labels.name }} 未就绪"
            
        # ACME 订单失败
        - alert: ACMEOrderFailed
          expr: |
            increase(certmanager_http_acme_client_request_count{status="error"}[1h]) > 5
          labels:
            severity: warning
          annotations:
            summary: "ACME 请求错误增加"
            
    - name: certificate.recording
      rules:
        # 证书剩余天数
        - record: certificate:expiry:days
          expr: |
            (certmanager_certificate_expiration_timestamp_seconds - time()) / 86400
            
        # 即将过期的证书数量
        - record: certificate:expiring:count
          expr: |
            count(certmanager_certificate_expiration_timestamp_seconds - time() < 86400 * 30)
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "Certificate Management Dashboard",
    "panels": [
      {
        "title": "Certificates Expiring Soon",
        "type": "stat",
        "targets": [
          {
            "expr": "count(certmanager_certificate_expiration_timestamp_seconds - time() < 86400 * 30)",
            "legendFormat": "Expiring < 30 days"
          }
        ]
      },
      {
        "title": "Certificate Expiry Timeline",
        "type": "table",
        "targets": [
          {
            "expr": "(certmanager_certificate_expiration_timestamp_seconds - time()) / 86400",
            "legendFormat": "{{ namespace }}/{{ name }}"
          }
        ]
      },
      {
        "title": "cert-manager Request Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(certmanager_http_acme_client_request_count[5m])",
            "legendFormat": "{{ status }}"
          }
        ]
      }
    ]
  }
}
```

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **v1.25** | kubelet 证书自动轮换 GA | 简化节点证书管理 |
| **v1.27** | ClusterTrustBundle Alpha | 集群级信任包 |
| **v1.28** | ClusterTrustBundle Beta | 更好的 CA 分发 |
| **v1.29** | 证书签名改进 | 更灵活的签名选项 |
| **v1.30** | ServiceAccount Token 改进 | 更安全的 Token 管理 |
| **cert-manager 1.14** | Gateway API 支持 | 自动为 Gateway 签发证书 |

<!-- chunk: 最佳实践总结 -->
## 最佳实践总结

### 证书管理检查清单

- [ ] 使用 cert-manager 管理应用证书
- [ ] 配置自动续期 (renewBefore)
- [ ] 监控证书过期时间
- [ ] 使用短期证书 (90天或更短)
- [ ] 启用 kubelet 证书自动轮换
- [ ] 定期检查集群证书状态
- [ ] 备份 CA 证书和私钥
- [ ] 使用 ECDSA 替代 RSA (性能更好)

### 安全建议

| 建议 | 说明 |
|-----|------|
| 短期证书 | 使用 90 天或更短有效期 |
| 自动轮换 | 启用证书自动续期 |
| 强加密算法 | 使用 ECDSA P-256 或 RSA 2048+ |
| TLS 1.2+ | 禁用 TLS 1.0/1.1 |
| mTLS | 服务间启用双向认证 |
| 证书透明度 | Let's Encrypt 自动提交 CT 日志 |

---

**参考资料**:
- [cert-manager 文档](https://cert-manager.io/docs/)
- [Kubernetes PKI 证书和要求](https://kubernetes.io/docs/setup/best-practices/certificates/)
- [Let's Encrypt 文档](https://letsencrypt.org/docs/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-05-security-compliance/MOC.md|domain-05-security-compliance MOC]]
- [[domain-05-security-compliance/README.md|Security Domain]]
- [[domain-05-security-compliance/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- [[domain-05-security-compliance/01-authentication-authorization-system.md|Kubernetes 认证授权体系详解]]
- [[domain-05-security-compliance/02-network-security-policies.md|网络安全策略与零信任架构]]
- [[domain-05-security-compliance/03-runtime-security-defense.md|运行时安全防护与威胁检测]]
- [[domain-05-security-compliance/04-audit-logging-compliance.md|04 - 审计日志与合规性管理]]
- [[domain-05-security-compliance/05-policy-validation-tools.md|05 - 策略校验与准入控制工具 (Policy Validation)]]
- [[domain-05-security-compliance/06-pod-security-standards.md|06 - Pod安全标准详解]]
- [[domain-05-security-compliance/07-rbac-matrix-configuration.md|07 - RBAC权限矩阵表]]
- [[domain-05-security-compliance/08-security-best-practices.md|08 - 安全最佳实践表]]
- [[domain-05-security-compliance/09-security-hardening-production.md|Kubernetes 安全加固]]

## See Also

- [[domain-05-security-compliance/08-security-best-practices.md|08-security-best-practices]]
- [[domain-05-security-compliance/09-security-hardening-production.md|09-security-hardening-production]]
- [[domain-05-security-compliance/11-secret-management-tools.md|11-secret-management-tools]]
- [[domain-05-security-compliance/12-compliance-certification.md|12-compliance-certification]]

- [[domain-05-security-compliance/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
