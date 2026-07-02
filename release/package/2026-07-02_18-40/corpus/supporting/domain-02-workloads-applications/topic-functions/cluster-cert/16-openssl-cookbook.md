---
title: OpenSSL 证书操作速查手册 (topic-code-analysis)
description: 'description: 本文档汇总 Kubernetes 集群证书运维中最常用的 OpenSSL 命令，覆盖查看、验证、生成、转换、调试全场景。'
summary: 'description: 本文档汇总 Kubernetes 集群证书运维中最常用的 OpenSSL 命令，覆盖查看、验证、生成、转换、调试全场景。'
category: general
tags:
- reference
- etcd
- apiserver
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenSSL 证书操作速查手册 是什么
- 如何 OpenSSL 证书操作速查手册
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- OpenSSL
- 证书操作速查手册
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: OpenSSL 证书操作速查手册
description: 本文档汇总 Kubernetes 集群证书运维中最常用的 OpenSSL 命令，覆盖查看、验证、生成、转换、调试全场景。
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
last_updated: '2026-05-18'
difficulty: intermediate
reading_level: intermediate
audience:
- Kubernetes 管理员
- 集群运维人员
- DevOps 工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 证书 OpenSSL 操作命令速查
- OpenSSL 查看验证生成转换证书
- openssl x509 verify certificate chain
- OpenSSL CSR 操作 Kubernetes CSR
- TLS 握手调试 s_client
trigger_keywords:
- openssl
- x509
- verify
- certificate
- CSR
- TLS
- s_client
- 证书验证
- 证书生成
- 格式转换
related_domains:
- domain-01-cluster-fundamentals
- domain-05-security-compliance
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/cert-format-encoding
- cluster-cert/apiserver-cert
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

# OpenSSL 证书操作速查手册

本文档汇总 Kubernetes 集群证书运维中最常用的 OpenSSL 命令，覆盖查看、验证、生成、转换、调试全场景。

---

## 一、查看证书信息

### 1.1 基础信息

```bash
# 查看证书完整信息（文本格式）
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -text

# 只查看过期时间
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -enddate
# 输出: notAfter=Jan 15 08:30:00 2026 GMT

# 只查看生效时间
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -startdate

# 查看 Subject（主体）
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -subject
# 输出: subject=CN = kube-apiserver

# 查看 Issuer（签发者）
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -issuer
# 输出: issuer=CN = kubernetes-ca

# 查看序列号
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -serial

# 查看指纹
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -fingerprint -sha256
```

### 1.2 查看扩展字段

```bash
# 查看 SAN（Subject Alternative Name）
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext subjectAltName

# 查看 Key Usage
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext keyUsage

# 查看 Extended Key Usage
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext extendedKeyUsage

# 查看 Basic Constraints
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -ext basicConstraints
```

### 1.3 批量检查

```bash
# 批量检查所有证书过期时间
for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
  echo "=== $(basename $cert) ==="
  openssl x509 -in "$cert" -noout -dates -subject 2>/dev/null
  echo ""
done

# 彩色输出：过期 < 30 天标红，< 90 天标黄
for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
  enddate=$(openssl x509 -in "$cert" -noout -enddate 2>/dev/null | cut -d= -f2)
  [ -z "$enddate" ] && continue
  epoch=$(date -d "$enddate" +%s 2>/dev/null || date -j -f "%b %d %H:%M:%S %Y %Z" "$enddate" +%s 2>/dev/null)
  now=$(date +%s)
  days=$(( (epoch - now) / 86400 ))
  if [ $days -lt 0 ]; then
    echo -e "\033[31m[EXPIRED]\033[0m $(basename $cert): $enddate"
  elif [ $days -lt 30 ]; then
    echo -e "\033[33m[WARNING]\033[0m $(basename $cert): ${days}d left"
  else
    echo -e "\033[32m[OK]\033[0m $(basename $cert): ${days}d left"
  fi
done
```

---

## 二、验证证书关系

### 2.1 验证证书链

```bash
# 验证 apiserver.crt 是否由 ca.crt 签发
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt
# 输出: /etc/kubernetes/pki/apiserver.crt: OK

# 验证完整证书链（包含中间 CA）
openssl verify -CAfile ca.crt -untrusted intermediate.crt server.crt
```

### 2.2 验证证书与私钥匹配

```bash
# 方法 1: 比较 modulus（推荐）
openssl x509 -noout -modulus -in /etc/kubernetes/pki/apiserver.crt | openssl md5
openssl rsa -noout -modulus -in /etc/kubernetes/pki/apiserver.key | openssl md5
# 两行的 MD5 值必须相同

# 方法 2: 使用 nscert（同时验证证书和私钥文件）
# 无直接命令，需要组合检查

# 方法 3: 提取公钥对比
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -pubkey > cert.pub
openssl rsa -in /etc/kubernetes/pki/apiserver.key -pubout > key.pub
diff cert.pub key.pub
```

### 2.3 远程验证

```bash
# 检查 API Server 远程证书
echo | openssl s_client -connect 192.168.1.10:6443 -servername kubernetes 2>/dev/null | openssl x509 -noout -text

# 只查看远程证书的 SAN
echo | openssl s_client -connect 192.168.1.10:6443 2>/dev/null | openssl x509 -noout -ext subjectAltName

# 检查 etcd 远程证书
echo | openssl s_client -connect 127.0.0.1:2379 2>/dev/null | openssl x509 -noout -text

# 指定 CA 验证远程证书
echo | openssl s_client -connect 192.168.1.10:6443 -CAfile /etc/kubernetes/pki/ca.crt 2>/dev/null | grep "Verify return code"
# 输出应为: Verify return code: 0 (ok)
```

---

## 三、生成证书（手动签发）

### 3.1 生成 CA

```bash
# 1. 生成 CA 私钥
openssl genrsa -out ca.key 2048
chmod 600 ca.key

# 2. 生成自签名 CA 证书
openssl req -x509 -new -nodes \
  -key ca.key \
  -subj "/CN=kubernetes-ca" \
  -days 3650 \
  -sha256 \
  -out ca.crt
```

### 3.2 生成服务端证书（含 SAN）

```bash
# 1. 生成私钥
openssl genrsa -out apiserver.key 2048

# 2. 创建 CSR 配置文件（包含 SAN）
cat > apiserver-csr.conf <<EOF
[req]
distinguished_name = req_distinguished_name
req_extensions = v3_req
prompt = no

[req_distinguished_name]
CN = kube-apiserver
O = system:masters

[v3_req]
keyUsage = keyEncipherment, digitalSignature
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = kubernetes
DNS.2 = kubernetes.default
DNS.3 = kubernetes.default.svc
DNS.4 = kubernetes.default.svc.cluster.local
IP.1 = 10.96.0.1
IP.2 = 192.168.1.10
IP.3 = 127.0.0.1
EOF

# 3. 生成 CSR
openssl req -new -key apiserver.key -out apiserver.csr -config apiserver-csr.conf

# 4. 使用 CA 签发证书
openssl x509 -req -in apiserver.csr \
  -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out apiserver.crt -days 365 \
  -extensions v3_req -extfile apiserver-csr.conf
```

### 3.3 生成客户端证书

```bash
# 1. 生成私钥
openssl genrsa -out admin.key 2048

# 2. 生成 CSR
openssl req -new -key admin.key \
  -subj "/CN=kubernetes-admin/O=system:masters" \
  -out admin.csr

# 3. 使用 CA 签发
openssl x509 -req -in admin.csr \
  -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out admin.crt -days 365 \
  -extensions client_cert -extfile <(cat <<EOF
[client_cert]
extendedKeyUsage = clientAuth
EOF
)
```

---

## 四、格式转换

### 4.1 PEM ↔ DER

```bash
# PEM → DER
openssl x509 -in ca.crt -out ca.der -outform DER

# DER → PEM
openssl x509 -in ca.der -inform DER -out ca.pem

# PEM 私钥 → DER
openssl rsa -in apiserver.key -out apiserver.der -outform DER
```

### 4.2 提取/合并证书链

```bash
# 从证书链中提取单个证书
# （证书链文件包含多个 PEM 块时）

# 提取第一个证书（服务端证书）
openssl crl2pkcs7 -nocrl -certfile fullchain.crt | \
  openssl pkcs7 -print_certs -noout | \
  awk 'split_after==1{n++;split_after=0} /-----END CERTIFICATE-----/{split_after=1} {print > "cert" n ".crt"}'

# 简单方法：手动分割
# cert 0: 服务端证书
# cert 1: 中间 CA
# cert 2: 根 CA
```

---

## 五、kubeconfig 证书操作

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 从 kubeconfig 提取 CA 证书
kubectl config view --raw \
  -o jsonpath='{.clusters[0].cluster.certificate-authority-data}' | \
  base64 -d > extracted-ca.crt

# 从 kubeconfig 提取客户端证书
kubectl config view --raw \
  -o jsonpath='{.users[0].user.client-certificate-data}' | \
  base64 -d > extracted-client.crt

# 从 kubeconfig 提取客户端私钥
kubectl config view --raw \
  -o jsonpath='{.users[0].user.client-key-data}' | \
  base64 -d > extracted-client.key

# 查看提取的证书信息
openssl x509 -in extracted-client.crt -noout -text
```
---

## 六、CSR 操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Kubernetes CSR 中的请求内容
kubectl get csr <csr-name> -o jsonpath='{.spec.request}' | base64 -d | openssl req -noout -text

# 查看 CSR 的签名算法
kubectl get csr <csr-name> -o jsonpath='{.spec.request}' | base64 -d | openssl req -noout -text | grep "Signature Algorithm"

# 手动生成 CSR 并提交到 Kubernetes
cat <<EOF | kubectl apply -f -
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: my-csr
spec:
  request: $(cat my-request.csr | base64 | tr -d '\n')
  signerName: kubernetes.io/kube-apiserver-client
  usages:
    - client auth
EOF
```
---

## 七、调试 TLS 握手

```bash
# 详细调试 API Server TLS 握手
openssl s_client -connect 192.168.1.10:6443 \
  -CAfile /etc/kubernetes/pki/ca.crt \
  -cert extracted-client.crt \
  -key extracted-client.key \
  -state -debug

# 检查支持的 TLS 版本和密码套件
nmap --script ssl-enum-ciphers -p 6443 192.168.1.10

# 快速检查证书过期（批量）
for ip in 192.168.1.10 192.168.1.11 192.168.1.12; do
  echo -n "API Server $ip: "
  echo | openssl s_client -connect ${ip}:6443 2>/dev/null | openssl x509 -noout -enddate
done
```

---

## 八、常见错误速查

| 错误信息 | OpenSSL 诊断命令 | 常见原因 |
|---------|----------------|---------|
| `x509: certificate has expired` | `openssl x509 -noout -enddate` | 超过 NotAfter 时间 |
| `x509: certificate signed by unknown authority` | `openssl verify -CAfile` | CA 不匹配或证书链不完整 |
| `x509: certificate is valid for X, not Y` | `openssl x509 -noout -ext subjectAltName` | SAN 缺失 |
| `tls: private key does not match public key` | `openssl x509 -noout -modulus` vs `openssl rsa -noout -modulus` | 证书与私钥不配对 |
| `x509: cannot validate certificate for IP` | 检查 SAN 中的 IP 列表 | 访问 IP 不在 SAN 中 |
| `PEM routines:get_name:no start line` | 检查文件格式 | 文件是 DER 而非 PEM |

## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]


<!-- risk-assessed -->
