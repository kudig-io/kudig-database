---
title: 'Day 7: K8S 集群证书'
description: '**学习时间**: 4-5 小时 | **主题**: 理解集群证书管理与更新机制'
summary: '**学习时间**: 4-5 小时 | **主题**: 理解集群证书管理与更新机制'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- opa
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 7: K8S 集群证书 是什么'
- '如何 Day 7: K8S 集群证书'
trigger_keywords:
- Day
- '7:'
- K8S
- 集群证书
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- etcd-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 7: K8S 集群证书
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|Kubernetes]] certificate system CA API Server [[etcd|etcd]]
  - ACK kubeconfig certificate renewal
  - Kubernetes certificate expiration troubleshooting
  - [[kubelet|kubelet]] TLS Bootstrap
  - Certificate renewal certrenew API
trigger_keywords:
  - certificate
  - CA
  - kubeconfig
  - kubelet
  - TLS
  - etcd
  - API Server
  - certificate renewal
  - certrenew
  - x509
reading_level: intermediate
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-05-security-compliance
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - certificate-management
  - security-architecture
  - certificate-troubleshooting
---

# Day 7: K8S 集群证书

> **学习时间**: 4-5 小时 | **主题**: 理解集群证书管理与更新机制

---

## 概述

本文深入讲解 Kubernetes 集群的证书体系，包括证书类型、有效期管理、轮换机制和故障排查。证书是 K8s 集群安全的基石——所有组件间的通信都依赖 TLS 证书进行加密和认证。理解证书体系对于排查 `x509: certificate has expired` 等常见错误至关重要。在 ACK 托管版中，管控面证书由阿里云自动管理，但用户仍需关注 kubeconfig 客户端证书的有效期。

### 学习目标

- 理解 K8S 集群中的完整证书体系（CA、API Server、etcd、kubelet、kubeconfig）
- 掌握 ACK 集群证书的管理方式（托管版 vs 专有版）
- 能够检查证书过期时间和状态
- 了解证书轮换和 kubeconfig 更新流程
- 掌握证书过期问题的应急排查和恢复方法

---

## 核心概念详解

### K8S 证书体系架构

Kubernetes 集群使用 PKI（Public Key Infrastructure）体系来保障组件间通信安全。整个证书体系以 CA（Certificate Authority）根证书为信任锚点，签发多个组件证书。

**CA 根证书** 是整个信任链的起点。K8s 集群通常有多个 CA：用于签发 API Server 相关证书的 `ca.crt`、用于签发 etcd 证书的 `etcd-ca.crt`、用于签发前端代理证书的 `front-proxy-ca.crt` 等。CA 证书的有效期通常为 10 年，是集群中最长寿命的证书。CA 证书过期意味着集群不可恢复（除非有备份），因此在专有版中必须做好 CA 证书的备份和监控。

**API Server 证书**（`tls.crt` / `tls.key`）用于 API Server 的服务端认证。当 kubectl 或其他客户端通过 HTTPS 连接 API Server 时，API Server 使用此证书证明自己的身份。证书中包含 SAN（Subject Alternative Name），列出了所有合法的访问地址——包括 API Server 的 IP、域名、Service 名称等。如果客户端访问的地址不在 SAN 列表中，会报 `x509: certificate relies on legacy Common Name field` 或 `certificate is valid for ... not for ...` 错误。

**etcd 证书** 包括服务器证书（用于 etcd 成员间通信）和客户端证书（用于 API Server 连接 etcd）。etcd 证书独立于 API Server 证书体系，有自己的 CA。etcd 对证书过期非常敏感——如果 etcd 服务器证书过期，etcd 集群会立即不可用，导致整个 K8s 集群无法写入任何状态。

**kubelet 证书** 用于 kubelet 与 API Server 之间的双向认证。kubelet 既可以作为客户端（向 API Server 汇报状态）也需要作为服务端（API Server 通过 kubelet API 执行 exec、logs 等操作）。kubelet 证书支持自动轮换（Server TLS Bootstrap），当证书即将过期时，kubelet 会自动向 API Server 申请新证书。

**kubeconfig 客户端证书** 是用户使用 kubectl 连接集群时的身份凭证。kubeconfig 文件中的 `client-certificate-data` 字段包含了 Base64 编码的客户端证书。在 ACK 托管版中，kubeconfig 的有效期默认为 3 年（也可以通过 API 获取临时 kubeconfig）。

### 证书信任链

```
CA 根证书 (ca.crt, 10年有效期)
├── API Server 服务器证书 (tls.crt, 1年)
├── API Server 客户端证书 (apiserver-kubelet-client.crt, 1年)
├── Controller Manager 客户端证书 (cm.crt, 1年)
├── Scheduler 客户端证书 (scheduler.crt, 1年)
├── kubelet 服务器证书 (kubelet.crt, 1年, 自动轮换)
├── kubelet 客户端证书 (kubelet-client.crt, 1年, 自动轮换)
├── 前端代理证书 (front-proxy.crt, 1年)
└── 用户/管理员客户端证书 (admin.crt, 有效期各异)

etcd CA (etcd-ca.crt, 10年有效期)
├── etcd 服务器证书 (server.crt, 1年)
├── etcd 客户端证书 (client.crt, 1年)
├── etcd 对等证书 (peer.crt, 1年)
└── API Server etcd 客户端证书 (apiserver-etcd-client.crt, 1年)
```

### ACK 证书管理差异

| 证书类型 | ACK 托管版 | ACK 专有版 |
|----------|-----------|-----------|
| CA 根证书 | 阿里云管理，自动轮换 | 需要用户管理 |
| API Server 证书 | 阿里云管理 | 需要手动或脚本轮换 |
| etcd 证书 | 阿里云管理 | 需要手动管理 |
| kubelet 证书 | 自动轮换（默认开启） | 自动轮换（需配置） |
| kubeconfig | 用户通过 API 获取更新 | 用户自行管理 |
| Webhook 证书 | 用户管理 | 用户管理 |

### 证书过期的连锁影响

证书过期是 K8s 集群中最严重的问题之一，影响范围取决于过期的证书类型：

- **CA 证书过期**: 集群完全不可用，无法恢复（除非有 CA 私钥备份）
- **API Server 证书过期**: 所有 kubectl 命令失败，所有 API 调用失败
- **etcd 证书过期**: 集群无法写入状态，现有工作负载继续运行但无法管理
- **kubelet 证书过期**: API Server 无法与节点通信，无法执行 exec/logs，Pod 状态停止上报
- **kubeconfig 过期**: 用户无法使用 kubectl 连接集群，但集群内部运行不受影响

---

## 实战演练

### 任务 1: 检查集群证书状态 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 提取 kubeconfig 中的客户端证书并查看过期时间
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | \
  base64 -d | openssl x509 -text -noout | grep -A2 "Validity"
# 预期输出:
# Validity
#     Not Before: Jan 15 08:00:00 2024 GMT
#     Not After : Jan 15 08:00:00 2027 GMT

# 检查证书的 Subject 和 SAN
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | \
  base64 -d | openssl x509 -text -noout | grep -E "Subject:|DNS:|IP Address:"
# 预期输出:
# Subject: O=system:users, OU=..., CN=...
# DNS: ..., IP Address: ...

# 检查集群 CA 证书（通过 cluster-info ConfigMap）
kubectl get configmap -n kube-system cluster-info -o yaml

# 查看 kube-system 中的证书相关 Secret
kubectl get secrets -n kube-system | grep -i cert
# 预期输出:
# csi-disk-cert               Opaque   2      30d
# csi-nas-cert                Opaque   2      30d
# ...

# 检查 API Server 证书（通过 OpenSSL）
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}')
echo "API Server: ${APISERVER}"
echo | openssl s_client -connect ${APISERVER#https://} 2>/dev/null | \
  openssl x509 -noout -dates -subject -issuer
# 预期输出:
# notBefore=Jan 15 08:00:00 2024 GMT
# notAfter=Jan 15 08:00:00 2025 GMT
# subject=CN = kube-apiserver
# issuer=CN = kubernetes

# 检查 Webhook 配置中的证书
kubectl get validatingwebhookconfigurations -o yaml | grep -A2 caBundle
kubectl get mutatingwebhookconfigurations -o yaml | grep -A2 caBundle
```
### 任务 2: kubeconfig 管理 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 通过 ACK API 获取新的 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config
# 预期输出: 完整的 kubeconfig YAML 内容

# 2. 获取临时 kubeconfig（有效期较短，适合临时调试）
aliyun cs GET /k8s/<cluster_id>/user_config \
  --TemporaryDurationMinutes 480
# 输出的 kubeconfig 在 8 小时后失效

# 3. 获取私网 kubeconfig（通过内网访问，适合从 VPC 内部连接）
aliyun cs GET /k8s/<cluster_id>/user_config \
  --PrivateIpAddress true
# 输出的 kubeconfig 使用内网端点

# 4. 将 kubeconfig 保存到文件
aliyun cs GET /k8s/<cluster_id>/user_config > ~/.kube/config-new
export KUBECONFIG=~/.kube/config-new
kubectl cluster-info
# 预期输出:
# Kubernetes control plane is running at https://xxx.cn-hangzhou.alicontainer.com:6443

# 5. 撤销已颁发的所有 kubeconfig（紧急安全措施）
aliyun cs POST /clusters/<cluster_id>/ravelokens/revoke
# 执行后所有旧的 kubeconfig 将失效，需要重新获取

# 6. 管理多集群 kubeconfig
# 查看当前所有 context
kubectl config get-contexts
# 预期输出:
# CURRENT   NAME            CLUSTER         AUTHINFO        NAMESPACE
# *         cluster-1       cluster-1       user-1
#           cluster-2       cluster-2       user-2

# 切换 context
kubectl config use-context cluster-2

# 合并多个 kubeconfig 文件
KUBECONFIG=~/.kube/config:~/cluster-2.yaml kubectl config view --flatten > merged-config.yaml

# 设置默认命名空间
kubectl config set-context --current --namespace=default

# 查看 kubeconfig 中的集群列表
kubectl config get-clusters
```
### 任务 3: 证书轮换操作 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ACK 托管版证书轮换（通过 API）
# 触发管控面证书轮换
aliyun cs POST /clusters/<cluster_id>/certrenew

# 查看轮换进度
aliyun cs GET /clusters/<cluster_id>/logs
# 预期输出包含轮换操作的日志记录

# 轮换完成后，需要重新获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config > ~/.kube/config

# 验证新证书
kubectl cluster-info
kubectl get nodes
# 预期输出: 正常返回节点列表

# 验证新证书的有效期
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | \
  base64 -d | openssl x509 -noout -dates
# 预期输出: 更新的 Not After 日期

# 专有版 kubelet 证书轮换检查
kubectl get csr
# 预期输出（如果开启了自动轮换）:
# NAME        AGE   SIGNER                        REQUESTOR          REQUESTED DURATION   CONDITION
# csr-abc12   5m    kubernetes.io/kube-apiserver-client-kubelet   system:node:node-1   365d                Approved,Issued

# 手动批准 CSR（如果未自动批准）
kubectl certificate approve <csr-name>
```
### 任务 4: 证书过期场景模拟与排查 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 场景: kubectl 连接失败，疑似证书过期

# 排查步骤 1: 检查错误信息
kubectl get nodes 2>&1
# 常见错误输出:
# Unable to connect to the server: x509: certificate has expired or is not yet valid: current time 2025-01-20T10:00:00Z is after 2025-01-15T08:00:00Z

# 排查步骤 2: 检查系统时间是否正确
date
# 如果系统时间不对（如 NTP 未同步），可能导致证书验证失败

# 排查步骤 3: 检查 kubeconfig 证书有效期
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | \
  base64 -d | openssl x509 -noout -dates

# 排查步骤 4: 如果 kubeconfig 过期，重新获取
aliyun cs GET /k8s/<cluster_id>/user_config > ~/.kube/config

# 排查步骤 5: 验证修复
kubectl cluster-info
kubectl get nodes
kubectl get pods -n kube-system

# 场景: 某个 Webhook 的 caBundle 证书过期
# 错误信息: Internal error occurred: failed calling webhook "xxx": Post "https://...": x509: certificate signed by unknown authority
# 排查: 检查 Webhook 配置中的 caBundle
kubectl get validatingwebhookconfiguration <name> -o yaml | grep caBundle | head -1 | awk '{print $2}' | base64 -d | openssl x509 -noout -dates
```
---

## 配置示例

### kubeconfig 文件完整结构

```yaml
apiVersion: v1
kind: Config
preferences: {}
clusters:
- cluster:
    certificate-authority-data: LS0tLS1CRUdJTi...  # CA 根证书（Base64）
    server: https://xxx.cn-hangzhou.alicontainer.com:6443
  name: cluster-ack
contexts:
- context:
    cluster: cluster-ack
    user: user-ack
    namespace: default
  name: context-ack
current-context: context-ack
users:
- name: user-ack
  user:
    client-certificate-data: LS0tLS1CRUdJTi...  # 客户端证书（Base64）
    client-key-data: LS0tLS1CRUdJTi...          # 客户端私钥（Base64）
    token: ""                                     # 或使用 Token 认证
```

### 自定义 Webhook 证书 Secret

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: webhook-cert
  namespace: kube-system
type: Opaque
data:
  ca.crt: LS0tLS1CRUdJTi...
  tls.crt: LS0tLS1CRUdJTi...
  tls.key: LS0tLS1CRUdJTi...
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: my-webhook
webhooks:
- name: validate.example.com
  clientConfig:
    caBundle: LS0tLS1CRUdJTi...
    service:
      name: webhook-service
      namespace: kube-system
      path: /validate
      port: 443
  admissionReviewVersions: ["v1"]
  sideEffects: None
  failurePolicy: Fail
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    operations: ["CREATE", "UPDATE"]
    resources: ["pods"]
```

### 证书过期检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# check-certs.sh - 检查 K8s 相关证书过期时间

echo "=========================================="
echo "  K8s 证书过期检查报告"
echo "  检查时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="

echo ""
echo "=== 1. kubeconfig 客户端证书 ==="
CERT_DATA=$(kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' 2>/dev/null)
if [ -n "$CERT_DATA" ]; then
    echo "$CERT_DATA" | base64 -d 2>/dev/null | openssl x509 -noout -subject -issuer -dates 2>/dev/null
    EXPIRY=$(echo "$CERT_DATA" | base64 -d 2>/dev/null | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2)
    if [ -n "$EXPIRY" ]; then
        EXPIRY_EPOCH=$(date -j -f "%b %d %T %Y %Z" "$EXPIRY" "+%s" 2>/dev/null || date -d "$EXPIRY" "+%s" 2>/dev/null)
        NOW_EPOCH=$(date "+%s")
        DAYS_LEFT=$(( (EXPIRY_EPOCH - NOW_EPOCH) / 86400 ))
        echo "剩余天数: ${DAYS_LEFT} 天"
        if [ "$DAYS_LEFT" -lt 30 ]; then
            echo "⚠️  警告: 证书将在 ${DAYS_LEFT} 天后过期！"
        fi
    fi
else
    echo "未找到客户端证书数据"
fi

echo ""
echo "=== 2. API Server 服务器证书 ==="
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}' 2>/dev/null)
if [ -n "$APISERVER" ]; then
    echo "API Server: ${APISERVER}"
    echo | openssl s_client -connect "${APISERVER#https://}" -servername "${APISERVER#https://}" 2>/dev/null | \
        openssl x509 -noout -subject -issuer -dates 2>/dev/null
fi

echo ""
echo "=== 3. Webhook 证书检查 ==="
kubectl get validatingwebhookconfigurations -o name 2>/dev/null | while read wh; do
    echo "--- $wh ---"
    kubectl get "$wh" -o jsonpath='{.webhooks[0].clientConfig.caBundle}' 2>/dev/null | \
        base64 -d 2>/dev/null | openssl x509 -noout -dates 2>/dev/null || echo "无 caBundle 或解析失败"
done

echo ""
echo "=========================================="
echo "  检查完成"
echo "=========================================="
```
---

## 常见问题

### Q1: kubeconfig 过期了但无法访问 ACK API 怎么办？

如果你丢失了阿里云 CLI 的访问权限，可以通过阿里云控制台获取 kubeconfig：登录控制台 → 容器服务 → 集群列表 → 点击集群名称 → 连接信息 → 复制 kubeconfig。如果是 RAM 用户，需要确保有 `cs:GetUserConfig` 权限。

### Q2: 证书轮换会导致服务中断吗？

ACK 托管版的管控面证书轮换是滚动进行的，不会导致服务中断。kubelet 证书轮换也是自动完成的，不影响正在运行的 Pod。但 kubeconfig 客户端证书更新需要用户手动获取新的 kubeconfig 文件，在更新之前 kubectl 命令会失败。

### Q3: 如何监控证书即将过期？

推荐使用 `[[cert-manager|cert-manager]]` 或自定义脚本定期检查证书过期时间。可以在 Prometheus 中配置告警规则，当证书剩余有效期少于 30 天时触发告警。ACK 托管版的管控面证书由阿里云自动监控和轮换，用户无需关注。但用户管理的 Webhook 证书、自定义证书需要自行监控。

### Q4: Webhook 的 caBundle 证书过期后怎么更新？

先更新 Webhook 使用的 Secret 中的证书，然后更新 ValidatingWebhookConfiguration 或 MutatingWebhookConfiguration 中的 caBundle 字段。如果使用 `cert-manager` 管理证书，可以配置自动轮换并使用 `ca Injector` 自动注入 caBundle。

### Q5: 如何判断是证书问题还是网络问题？

如果错误信息中包含 `x509`、`certificate`、`tls` 等关键词，通常是证书问题。如果错误信息是 `connection refused`、`timeout`、`no route to host`，通常是网络问题。如果错误信息是 `Unauthorized` 或 `Forbidden`，通常是认证授权问题而非证书问题。可以用 `curl -v https://<apiserver>:6443/healthz` 来查看详细的 TLS 握手信息。

### Q6: 专有版集群如何手动轮换所有证书？

使用 `kubeadm` 工具可以手动轮换证书：`kubeadm certs renew all`。执行后需要重启控制平面组件使其加载新证书。对于 etcd 证书，需要单独处理。kubelet 证书可以通过删除 `/var/lib/kubelet/pki/` 目录下的证书文件并重启 kubelet 来触发重新签发。

---

## 要点总结

| 证书类型 | 用途 | 有效期 | 管理方式 | 过期影响 |
|----------|------|--------|---------|---------|
| CA 根证书 | 签发所有组件证书 | 10 年 | 阿里云管理（托管版） | 集群不可恢复 |
| API Server 证书 | API 服务端认证 | 1 年 | 自动轮换（托管版） | 所有 API 调用失败 |
| etcd 证书 | etcd 通信加密 | 1 年 | 自动轮换（托管版） | 集群无法写入状态 |
| kubeconfig | 用户访问凭证 | 3 年 | 手动获取更新 | 用户无法连接集群 |
| kubelet 证书 | 节点身份认证 | 1 年 | 自动轮换 | 节点失联 |
| Webhook 证书 | 准入控制 | 自定义 | 用户管理 | Webhook 调用失败 |

---

## 延伸阅读

- [证书管理详解](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-05-security-compliance/06-compliance/07-certificate-management.md)
- [安全架构总览](32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-01-cluster-fundamentals/01-architecture-overview/08-security-architecture.md)
- [证书排障指南](32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/05-certificate-troubleshooting.md)
- [ACK 集群管理](../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md)
- [kubelet TLS Bootstrap](../../domain-05-security-compliance/02-kubelet-tls-bootstrap.md)

## Related

- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
