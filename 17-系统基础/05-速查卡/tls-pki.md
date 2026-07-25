---
title: TLS/SSL 与 PKI 速查卡
description: 证书管理、TLS 配置和 PKI 运维的快速参考
summary: 证书管理、TLS 配置和 PKI 运维的快速参考
category: cheatsheet
tags:
- tls
- ssl
- pki
- certificate
- cheatsheet
- quick-reference
- security
- etcd
- apiserver
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- TLS/SSL 与 PKI 速查卡 是什么
- 如何 TLS/SSL 与 PKI 速查卡
trigger_keywords:
- TLS
- SSL
- PKI
- 速查卡
- cheat
- sheet
prerequisites:
- kubectl-basics
- cloud-provider-basics
- monitoring-basics
- etcd-basics
- tls-basics
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: ../故障诊断/topic-skills/06-certificate-expiry.md
  desc: 证书过期 Skill
- path: ../domain-7-security-compliance/01-authentication-authorization-system.md
  desc: 认证授权系统
- path: ../系统基础/topic-cheat-sheet/networking.md
  desc: 网络诊断速查卡
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# TLS/SSL 与 PKI 速查表

> 证书管理、TLS 配置和 PKI 运维的快速参考 | **最后更新**: 2026-05

---

## 证书格式速查

### 常见文件扩展名

| 扩展名 | 格式 | 说明 |
|:---|:---|:---|
| `.pem` | Base64 编码 | 最常见格式，可包含证书、私钥、证书链 |
| `.crt` / `.cert` | 证书文件 | 通常为 PEM 或 DER 格式 |
| `.key` | 私钥文件 | 通常为 PEM 或 DER 格式 |
| `.csr` | 证书签名请求 | Certificate Signing Request |
| `.p12` / `.pfx` | PKCS#12 | 包含证书和私钥的加密容器 |
| `.der` | 二进制编码 | DER 编码的证书 |
| `.crl` | 证书吊销列表 | Certificate Revocation List |

### 编码格式转换

```bash
# PEM → DER
openssl x509 -in cert.pem -outform der -out cert.der

# DER → PEM
openssl x509 -in cert.der -inform der -out cert.pem

# PEM → PKCS#12（包含私钥）
openssl pkcs12 -export -in cert.pem -inkey key.pem -out cert.p12

# PKCS#12 → PEM
openssl pkcs12 -in cert.p12 -out cert.pem -nodes

# 提取证书（从 PKCS#12）
openssl pkcs12 -in cert.p12 -clcerts -nokeys -out cert.pem

# 提取私钥（从 PKCS#12）
openssl pkcs12 -in cert.p12 -nocerts -nodes -out key.pem
```

---

## OpenSSL 常用命令

### 查看证书信息

```bash
# 查看证书详情
openssl x509 -in cert.pem -text -noout

# 查看证书主题（Subject）
openssl x509 -in cert.pem -noout -subject

# 查看证书颁发者（Issuer）
openssl x509 -in cert.pem -noout -issuer

# 查看有效期
openssl x509 -in cert.pem -noout -dates

# 查看序列号
openssl x509 -in cert.pem -noout -serial

# 查看指纹
openssl x509 -in cert.pem -noout -fingerprint
openssl x509 -in cert.pem -noout -fingerprint -sha256

# 查看 SAN（Subject Alternative Name）
openssl x509 -in cert.pem -noout -ext subjectAltName
```

### 验证证书

```bash
# 验证证书链
openssl verify -CAfile ca-bundle.crt cert.pem

# 验证服务器证书（指定 CA 目录）
openssl verify -CApath /etc/ssl/certs server.crt

# 验证证书和私钥匹配
openssl x509 -noout -modulus -in cert.pem | openssl md5
openssl rsa -noout -modulus -in key.pem | openssl md5

# 验证 PKCS#12 文件
openssl pkcs12 -in cert.p12 -info -noout
```

### 测试 TLS 连接

```bash
# 测试 HTTPS 连接
openssl s_client -connect example.com:443

# 显示完整证书链
openssl s_client -connect example.com:443 -showcerts

# 指定 SNI（Server Name Indication）
openssl s_client -connect 192.168.1.1:443 -servername example.com

# 测试特定 TLS 版本
openssl s_client -connect example.com:443 -tls1_3
openssl s_client -connect example.com:443 -tls1_2

# 使用特定 CA 验证
openssl s_client -connect example.com:443 -CAfile ca.crt

# 获取证书过期时间
echo | openssl s_client -connect example.com:443 2>/dev/null | \
  openssl x509 -noout -dates
```

### 生成密钥和证书

```bash
# 生成 RSA 私钥（2048 位）
openssl genrsa -out key.pem 2048

# 生成 RSA 私钥（4096 位，加密存储）
openssl genrsa -aes256 -out key.pem 4096

# 生成 ECDSA 私钥（P-256）
openssl ecparam -genkey -name prime256v1 -out key.pem

# 生成 ECDSA 私钥（P-384）
openssl ecparam -genkey -name secp384r1 -out key.pem

# 生成 Ed25519 私钥
openssl genpkey -algorithm Ed25519 -out key.pem

# 生成 CSR（证书签名请求）
openssl req -new -key key.pem -out csr.pem \
  -subj "/C=CN/ST=Beijing/L=Beijing/O=Example/CN=example.com"

# 生成自签名证书（有效期 365 天）
openssl req -x509 -new -key key.pem -out cert.pem -days 365 \
  -subj "/C=CN/O=Example/CN=example.com"

# 生成带 SAN 的自签名证书
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 \
  -subj "/CN=example.com" \
  -addext "subjectAltName=DNS:example.com,DNS:www.example.com,IP:192.168.1.1"

# 使用配置文件生成证书
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout key.pem -out cert.pem -config openssl.cnf
```

---

## 证书链和 PKI

### 证书链结构

```
根 CA 证书 (Root CA)
    ↓ 签名
中间 CA 证书 (Intermediate CA)
    ↓ 签名
服务器证书 (End-Entity Certificate)
```

### 构建证书链

```bash
# 合并证书链（终端证书 + 中间 CA）
cat server.crt intermediate.crt > chain.pem

# 完整证书链（终端证书 + 中间 CA + 根 CA）
cat server.crt intermediate.crt root.crt > fullchain.pem

# 验证完整链
openssl verify -CAfile root.crt -untrusted intermediate.crt server.crt
```

### 提取证书链信息

```bash
# 从服务器获取完整证书链
openssl s_client -connect example.com:443 -showcerts </dev/null 2>/dev/null | \
  awk '/BEGIN CERTIFICATE/,/END CERTIFICATE/{print}' > chain.pem

# 拆分证书链
awk '/BEGIN CERTIFICATE/{n++}{print > "cert" n ".pem"}' chain.pem
```

---

## [[Kubernetes|Kubernetes]] 证书操作

### 查看集群证书

```bash
# 查看 API Server 证书信息
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -text -noout

# 查看 CA 证书
openssl x509 -in /etc/kubernetes/pki/ca.crt -text -noout

# 查看 etcd 证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -text -noout

# 检查证书过期时间（kubeadm）
kubeadm certs check-expiration

# 自动更新所有证书
kubeadm certs renew all

# 更新特定证书
kubeadm certs renew apiserver
kubeadm certs renew apiserver-kubelet-client
kubeadm certs renew front-proxy-client
kubeadm certs renew etcd-server
```

### [[cert-manager|cert-manager]] 操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看证书状态
kubectl get certificate -A
kubectl describe certificate <name> -n <namespace>

# 查看证书详情（从 Secret）
kubectl get secret <tls-secret> -o jsonpath='{.data.tls\\.crt}' | \
  base64 -d | openssl x509 -text -noout

# 手动触发续期
kubectl cert-manager renew <certificate-name> -n <namespace>

# 强制续期（添加注解）
kubectl annotate certificate <name> cert-manager.io/force-renewal="true"

# 查看 ACME 挑战状态
kubectl get challenge -A
kubectl get order -A

# 查看 CertificateRequest
kubectl get certificaterequest -A
```
---

## 证书监控脚本

### 检查证书过期时间

```bash
#!/bin/bash
# check_cert_expiry.sh - 检查证书过期时间

CERT_FILE="$1"
DAYS_THRESHOLD="${2:-30}"

if [ -z "$CERT_FILE" ]; then
    echo "Usage: $0 <cert-file> [days-threshold]"
    exit 1
fi

# 获取过期日期
EXPIRY_DATE=$(openssl x509 -in "$CERT_FILE" -noout -enddate | cut -d= -f2)
EXPIRY_EPOCH=$(date -d "$EXPIRY_DATE" +%s)
CURRENT_EPOCH=$(date +%s)

# 计算剩余天数
DAYS_REMAINING=$(( (EXPIRY_EPOCH - CURRENT_EPOCH) / 86400 ))

echo "证书: $CERT_FILE"
echo "过期日期: $EXPIRY_DATE"
echo "剩余天数: $DAYS_REMAINING"

if [ $DAYS_REMAINING -lt $DAYS_THRESHOLD ]; then
    echo "⚠️  警告: 证书将在 $DAYS_REMAINING 天内过期!"
    exit 1
else
    echo "✓ 证书状态正常"
    exit 0
fi
```

### 监控多个域名证书

```bash
#!/bin/bash
# check_domains_expiry.sh - 批量检查域名证书

DOMAINS=(
    "example.com:443"
    "api.example.com:443"
    "grafana.example.com:443"
)

for domain in "${DOMAINS[@]}"; do
    host=$(echo $domain | cut -d: -f1)
    port=$(echo $domain | cut -d: -f2)
    
    echo "检查: $host:$port"
    
    expiry=$(echo | openssl s_client -connect $host:$port -servername $host 2>/dev/null | \
        openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2)
    
    if [ -n "$expiry" ]; then
        expiry_epoch=$(date -d "$expiry" +%s)
        current_epoch=$(date +%s)
        days=$(( (expiry_epoch - current_epoch) / 86400 ))
        
        if [ $days -lt 30 ]; then
            echo "  ⚠️  $days 天后过期"
        else
            echo "  ✓ $days 天后过期"
        fi
    else
        echo "  ✗ 无法获取证书信息"
    fi
done
```

---

## TLS 配置最佳实践

### 密码套件配置

```nginx
# Nginx 强加密配置
ssl_protocols TLSv1.2 TLSv1.3;
ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384;
ssl_prefer_server_ciphers off;
ssl_session_cache shared:SSL:10m;
ssl_session_timeout 1d;
ssl_session_tickets off;
```

```yaml
# Ingress NGINX TLS 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  annotations:
    nginx.ingress.kubernetes.io/ssl-protocols: "TLSv1.2 TLSv1.3"
    nginx.ingress.kubernetes.io/ssl-ciphers: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256"
    nginx.ingress.kubernetes.io/ssl-prefer-server-ciphers: "true"
```

### 证书有效期建议

| 场景 | 建议有效期 | 续期窗口 |
|:---|:---|:---|
| Let's Encrypt 证书 | 90 天 | 30 天 |
| 内部 CA 证书 | 1 年 | 30-60 天 |
| 根 CA 证书 | 10 年 | 1-2 年 |
| 中间 CA 证书 | 5 年 | 6 个月-1 年 |

### 密钥算法选择

| 算法 | 密钥大小 | 适用场景 |
|:---|:---|:---|
| ECDSA (P-256) | 256 位 | 推荐，性能好，密钥小 |
| ECDSA (P-384) | 384 位 | 高安全性要求 |
| RSA | 2048 位 | 兼容性要求 |
| RSA | 4096 位 | 高安全性 + 兼容性 |
| Ed25519 | 256 位 | 现代算法，高性能 |

---

## 常见错误排查

### 证书验证失败

```bash
# 错误: unable to get local issuer certificate
# 解决: 指定完整的 CA 证书链
openssl verify -CAfile root.crt -untrusted intermediate.crt cert.pem

# 错误: certificate has expired
# 检查过期时间
openssl x509 -in cert.pem -noout -dates

# 错误: hostname mismatch
# 检查 SAN 或 CN
openssl x509 -in cert.pem -noout -ext subjectAltName
openssl x509 -in cert.pem -noout -subject
```

### TLS 连接问题

```bash
# 测试 TLS 握手详情
openssl s_client -connect example.com:443 -debug

# 检查支持的密码套件
openssl s_client -connect example.com:443 -tls1_3 2>&1 | grep "Cipher"

# 详细调试输出
openssl s_client -connect example.com:443 -msg -debug

# 检查证书链完整性
openssl s_client -connect example.com:443 -showcerts 2>/dev/null | \
  awk '/BEGIN/,/END/' | openssl crl2pkcs7 -nocrl -certfile /dev/stdin | \
  openssl pkcs7 -print_certs -noout
```

### cert-manager 问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Challenge 失败 - 检查 Ingress 配置
kubectl describe challenge <name>

# Order 失败 - 检查 ACME 账户
kubectl describe order <name>

# 证书不 Ready - 检查 Issuer
kubectl describe issuer <name>
kubectl describe clusterissuer <name>

# 查看 cert-manager 日志
kubectl logs -n cert-manager deployment/cert-manager
kubectl logs -n cert-manager deployment/cert-manager-webhook
kubectl logs -n cert-manager deployment/cert-manager-cainjector
```
---

## 相关文档

- [安全/10-certificate-management.md](../../08-%E5%AE%89%E5%85%A8/06-%E5%90%88%E8%A7%84%E5%AE%A1%E8%AE%A1/10-certificate-management.md) - 完整证书管理指南
- [man/man8/cert-manager.8](../man/man8/cert-manager.8) - cert-manager manpage
- [故障诊断/FTA故障树/list/certificate-fta.md](../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/06-FTA%E6%95%85%E9%9A%9C%E6%A0%91/list/certificate-fta.md) - 证书故障树分析

## Related

- index/observability-index|Observabilityty 可观测性知识图谱索引|Observability 可观测性知识图谱索引]]]]
- [[21-生态参考/03-领域索引/cert-index.md|[[Certificate / TLS 证书知识图谱索引|Certificate / TLS 证书知识图谱索引]]]]

```

<!-- risk-assessed -->
