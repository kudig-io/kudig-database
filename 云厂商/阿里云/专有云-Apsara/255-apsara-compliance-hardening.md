---
title: 专有云（Apsara Stack）- 合规加固（等保/国密）
description: 专有云等保四级、国密 SM2/SM4 集成、KMS 密钥管理、审计日志对接与金融级 NetworkPolicy/RBAC 安全配置实战
summary: 专有云（Apsara Stack）面向等保四级、商用密码（国密 SM2/SM4）合规的安全加固实战：KMS 密钥管理、审计日志、金融级 NetworkPolicy、最小权限 RBAC 与镜像/准入安全。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- security
- compliance
- cryptography
- sm2
- sm4
- networkpolicy
- rbac
tier: core
sources:
- 等保 2.0 四级要求
- 商用密码应用安全性评估要求
- 阿里云专有云安全加固指南
created: 2026-07-23
last_updated: 2026-07-23
relationships:
- target: '[[云厂商/阿里云/02-ACK集群运维.md]]'
  type: related_to
- target: '[[云厂商/阿里云/apsara-stack-components.md]]'
  type: related_to
difficulty: advanced
audience:
- 安全工程师
- SRE
- 平台工程师
- 合规/审计人员
estimated_read_time: 18min
intent_queries:
- 专有云等保四级怎么加固
- 国密 SM2 SM4 怎么集成 K8s
- 专有云金融级 NetworkPolicy 配置
- 专有云审计日志对接
trigger_keywords:
- 等保
- 国密
- SM2
- SM4
- 合规
- 加固
- 审计
prerequisites:
- alicloud-basics
- k8s-security
- networkpolicy
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令与安全配置。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 专有云（Apsara Stack）- 合规加固（等保/国密）

本文档面向在客户数据中心运维 [[云厂商/阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的安全工程师与 SRE，系统梳理等保四级、商用密码（国密 SM2/SM4）合规要求下的 K8s 安全加固实战：KMS 密钥管理、审计日志对接、金融级 NetworkPolicy、最小权限 RBAC、镜像与准入安全。

> **关键认知**：专有云合规加固的核心是「密码国产化 + 全链路审计 + 纵深防御」。国密集成依赖客户 HSM/内部 KMS（非阿里云公有云 KMS），审计需对接客户 SIEM，网络隔离需配合客户防火墙。

---

## 1. 合规要求映射

### 1.1 等保四级 → K8s 加固项

| 等保要求域 | K8s/专有云加固项 | 对应章节 |
|-----------|------------------|----------|
| 安全通信网络 | TLS 全链路加密、NetworkPolicy 分区隔离 | §2、§3 |
| 安全区域边界 | 默认拒绝 NetworkPolicy、Ingress 网关 WAF | §3 |
| 安全计算环境 | 国密加密（Secret/云盘）、最小权限 RBAC、镜像签名 | §4、§5、§6 |
| 安全管理中心 | 集中审计、统一身份（RAM/AD 联邦）、监控告警 | §7、§8 |

### 1.2 国密（商用密码）合规

| 场景 | 国密算法 | 实现方式 |
|------|----------|----------|
| 传输加密 | SM2（证书/密钥交换）、SM4（对称加密） | 国密 TLS（mTLS）、国密 SSL |
| 存储加密 | SM4 | KMS BYOK 加密云盘、Secret 加密 |
| 签名验签 | SM2 | 镜像签名（Cosign 国密版）、API 签名 |
| 证书签发 | SM2 | 国密 CA（对接客户 CA） |

---

## 2. 传输加密与国密 TLS

### 2.1 国密 TLS（mTLS）配置要点

专有云中，管控面、etcd、业务间通信应采用国密 TLS。国密证书通常由客户内部 CA 签发（对接客户 PKI）。

| 通信链路 | 加密 | 配置位置 |
|----------|------|----------|
| kube-apiserver ↔ etcd | 国密 TLS | apiserver/etcd 静态 Pod 参数 |
| kube-apiserver ↔ kubelet | 国密 TLS | apiserver flag + kubelet cert |
| Pod ↔ Pod（业务） | mTLS（国密） | Service Mesh / 应用层 |
| Ingress ↔ 后端 | 国密 HTTPS | Ingress TLS + 后端 Service |

> **远程顾问注意**：国密 TLS 需确认 kube-apiserver/etcd 编译时是否启用国密 provider（GmSSL/Tongsuo 等）。专有云版本通常已支持，但需客户确认证书链与 CA。

### 2.2 证书过期巡检

```bash
# 🟢 低风险：只读/信息收集
# 检查管控面证书有效期（自管 Master）
kubeadm certs check-expiration
# 检查 etcd 证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates
# 检查 Ingress/TLS Secret 证书（业务侧）
kubectl get secret -A -o json | jq -r '.items[] | select(.type=="kubernetes.io/tls") | "\(.metadata.namespace)/\(.metadata.name): \(.data."tls.crt" | @base64d | openssl x509 -noout -enddate 2>/dev/null)"'
```

---

## 3. 金融级 NetworkPolicy

### 3.1 分区隔离架构

金融/政企典型网络分区（三区/四区模型）：

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│  接入区       │ → │  应用区       │ → │  数据区       │
│ (Ingress/    │    │ (微服务)      │    │ (DB/中间件)   │
│  WAF/网关)   │    │              │    │              │
└──────────────┘    └──────────────┘    └──────────────┘
       ↑                   ↑                   ↑
   命名空间: ingress   命名空间: app       命名空间: data
   NetworkPolicy       NetworkPolicy       NetworkPolicy
```

### 3.2 默认拒绝 + 按需放行

```yaml
# 默认拒绝所有入站/出站（每个业务命名空间基线）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: app
spec:
  podSelector: {}
  policyTypes: [Ingress, Egress]
  ingress: []
  egress: []
---
# 放行 DNS（CoreDNS）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: app
spec:
  podSelector: {}
  policyTypes: [Egress]
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
---
# 核心交易：仅允许来自网关命名空间的入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: core-transaction-ingress
  namespace: app
spec:
  podSelector:
    matchLabels:
      app: core-banking
  policyTypes: [Ingress]
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress
      podSelector:
        matchLabels:
          tier: gateway
    ports:
    - protocol: TCP
      port: 8443   # 国密 HTTPS
```

> **Terway 前提**：NetworkPolicy 依赖 CNI 支持。专有云推荐 Terway ENIIP 模式（支持 NetworkPolicy）；Flannel 不支持 NetworkPolicy，需额外方案（如 Calico Felix）。

### 3.3 出站数据库严格限制

```yaml
# 应用区仅允许访问数据区指定 DB 端口
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-db-egress
  namespace: app
spec:
  podSelector:
    matchLabels:
      app: core-banking
  policyTypes: [Egress]
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: data
    ports:
    - protocol: TCP
      port: 3306   # MySQL
    - protocol: TCP
      port: 1521   # Oracle
```

---

## 4. 国密 KMS 与 Secret/云盘加密

### 4.1 KMS BYOK 加密云盘

专有云云盘（ESSD）支持通过 KMS 加密，KMS 后端对接客户 HSM/内部 KMS，使用国密 SM4。

| 配置项 | 说明 |
|--------|------|
| KMS 实例 | 客户内部 KMS（对接 HSM），非阿里云公有云 KMS |
| 密钥算法 | SM4（对称），SM2（非对称） |
| BYOK | Bring Your Own Key，客户导入主密钥 |
| StorageClass | `encrypted: "true"`，`kmsKeyId` 指定 |

```yaml
# 加密云盘 StorageClass（专有云 CSI）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd-encrypted
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  encrypted: "true"                 # 启用加密
  kmsKeyId: "kms-xxxx-sm4"          # 国密 SM4 密钥 ID
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
```

### 4.2 Secret 加密（EncryptionProvider）

```yaml
# kube-apiserver 静态配置：Secret 加密（片段）
# /etc/kubernetes/encryption-config.yaml（专有云 kms-provider）
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources: [secrets, configmaps]
  providers:
  - kms:
      apiVersion: v2
      name: alicloud-kms-provider
      endpoint: unix:///var/run/kmsprovider/socket.sock   # kms-plugin
      timeouts:
        key-id: 3s
  - identity: {}   # 兜底
```

> **验证**：创建 Secret 后，检查 etcd 中存储是否为密文（`etcdctl get` 看到非明文 base64）。

```bash
# 🟢 低风险：只读
# 确认 kms-plugin 运行状态
kubectl get pods -n kube-system | grep kms
# 确认 Secret 在 etcd 中已加密（驻场 etcd 节点）
ETCDCTL_API=3 etcdctl get /registry/secrets/default/test-secret \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key | head -c 50
# 输出非明文（k8s:enc:kms:v2:...）即已加密
```

---

## 5. 最小权限 RBAC

### 5.1 RBAC 设计原则

| 原则 | 实践 |
|------|------|
| 命名空间隔离 | 业务 SA 仅在所属 namespace 授 Role（非 ClusterRole） |
| 最小权限 | 仅授予业务必需的 verbs/resources |
| 分离读权限 | 审计/监控用只读 Role，与操作 Role 分离 |
| 定期审计 | 定期 `kubectl auth can-i --list` 复核 |

```yaml
# 金融级最小权限 Role 示例
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: app
  name: financial-app-role
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/exec"]
  verbs: []                          # 默认禁止 exec（合规要求）
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch", "patch"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["create", "get", "list", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  namespace: app
  name: financial-app-rb
subjects:
- kind: ServiceAccount
  name: financial-app-sa
roleRef:
  kind: Role
  name: financial-app-role
  apiGroup: rbac.authorization.k8s.io
```

```bash
# 🟢 低风险：只读
# 复核 SA 的实际权限
kubectl auth can-i --list --as=system:serviceaccount:app:financial-app-sa -n app
```

---

## 6. 镜像与准入安全

| 安全项 | 实现 |
|--------|------|
| 私有镜像仓库 | 内部 Harbor，禁止拉取公网镜像 |
| 镜像扫描 | Harbor 集成 Trivy/Clair，阻断高危镜像 |
| 镜像签名 | Cosign 国密签名，准入验证 |
| 准入控制 | OPA Gatekeeper / Kyverno 策略（禁止特权、强制标签等） |
| imagePullSecrets | ServiceAccount 自动注入仓库凭证 |

```yaml
# Kyverno 策略：禁止特权容器
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged-containers
spec:
  validationFailureAction: Enforce
  rules:
  - name: privileged-containers
    match:
      resources:
        kinds: [Pod]
    validate:
      message: "特权容器被禁止（等保合规）"
      pattern:
        spec:
          containers:
          - securityContext:
              privileged: "false"
```

---

## 7. 审计日志对接

### 7.1 审计三层

| 审计层 | 内容 | 对接 |
|--------|------|------|
| ASOP 审计 | 平台管理员操作 | ASO → SLS/SIEM |
| ACK 审计 | kube-apiserver 所有请求 | audit-policy → SLS/SIEM |
| 底座审计 | ActionTrail | ActionTrail → SLS/SIEM |

### 7.2 kube-apiserver 审计策略

```yaml
# kube-apiserver 审计策略（片段，等保要求记录关键操作）
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
- level: RequestResponse            # 记录请求与响应
  verbs: ["create", "update", "patch", "delete"]
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]
- level: Request
  verbs: ["get", "list", "watch"]
  resources:
  - group: ""
    resources: ["secrets"]
- level: Metadata                   # 记录元数据（不含 body）
  resources:
  - group: "*"
```

> 审计日志投递到专有云 SLS（见 [[云厂商/阿里云/专有云-Apsara/251-apsara-stack-sls-logging.md|251 SLS 日志服务]]），并转发至客户 SIEM 做长期留存（等保要求 ≥180 天）。

---

## 8. 合规自检清单

- [ ] **传输加密**：管控面/etcd/Ingress 全链路 TLS（国密优先）
- [ ] **存储加密**：云盘 StorageClass `encrypted: true`（SM4）；Secret 在 etcd 加密
- [ ] **网络隔离**：业务命名空间默认拒绝 NetworkPolicy；分区按需放行
- [ ] **身份认证**：RAM/AD 联邦；RRSA Pod 级授权；最小权限 RBAC
- [ ] **镜像安全**：私有 Harbor；扫描阻断高危；签名验证；准入策略
- [ ] **审计日志**：三层审计启用；投递 SLS/SIEM；留存 ≥180 天
- [ ] **密钥管理**：KMS BYOK；密钥定期轮换；HSM 对接
- [ ] **证书巡检**：定期检查证书过期；自动续期或窗口续期
- [ ] **定期渗透**：定期安全评估与漏洞修复

---

## 9. 何时联系安全团队 / TAM

| 场景 | 处理方 |
|------|--------|
| AK 泄漏/根账号异常 | TAM + 安全团队 |
| RAM/RRSA 根证书或密钥问题 | 阿里云安全团队 |
| KMS/HSM 对接异常 | 客户安全团队 + TAM |
| 重大漏洞修复 | TAM 评估补丁 + 驻场执行 |

---

## 相关文档

- [[云厂商/阿里云/02-ACK集群运维.md|02 ACK集群运维]]
- [[云厂商/阿里云/专有云-Apsara/251-apsara-stack-sls-logging.md|251 SLS 日志服务]]
- [[云厂商/阿里云/专有云-Apsara/252-apsara-stack-pop-operations.md|252 POP 平台运维]]
- [[云厂商/阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]
- [[云厂商/阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]

## Related

- [[系统基础/知识字典/security/pod-security-policies.md|Pod 安全策略]]
- [[系统基础/知识字典/networking/networkpolicy.md|NetworkPolicy]]
- [[系统基础/知识字典/configuration/secrets.md|Secrets]]

<!-- risk-assessed -->
