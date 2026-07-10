---
title: '安全机制: ServiceAccount Token 与 Audit [cluster-create]'
description: 'title: ''安全机制: ServiceAccount Token 与 Audit'''
summary: 'title: ''安全机制: ServiceAccount Token 与 Audit'''
category: general
tags:
- reference
- security
- etcd
- apiserver
- kubelet
- scheduler
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- '安全机制: ServiceAccount Token 与 Audit 是什么'
- '如何 安全机制: ServiceAccount Token 与 Audit'
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- '安全机制:'
- ServiceAccount
- Token
- Audit
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




title: '安全机制: ServiceAccount Token 与 Audit'
description: '# 安全机制: ServiceAccount Token 与 Audit'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- scheduler
- rbac
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- DevOps工程师
- 安全工程师
- Kubernetes管理员
estimated_read_time: 5min
intent_queries:
- Kubernetes ServiceAccount token TokenRequest volumeProjection
- Kubernetes BoundServiceAccountTokenVolumeProjection
- Kubernetes API server audit log configuration
- Kubernetes encryption at rest secrets provider
- Kubernetes NodeRestriction admission plugin
trigger_keywords:
- ServiceAccount
- token
- TokenRequest
- BoundServiceAccountTokenVolume
- audit
- encryption
- NodeRestriction
- RBAC
- RBAC
- sa.key
- sa.pub
- api server
- admission
related_domains:
- domain-2-security
- 故障诊断
related_topics:
- ServiceAccount
- RBAC
- API Server
- audit
- encryption
- admission
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

# 安全机制: ServiceAccount Token 与 Audit

## 源码路径

`cmd/kubeadm/app/phases/certs/` (SA 密钥)
`pkg/kube-apiserver/options/` (audit 配置)
`pkg/auth/` (token 机制)

---

## ServiceAccount Token 演进

### 传统模式 (K8s < 1.20)

```yaml
# Pod 自动挂载 ServiceAccount token
# 存储为 Secret，永久有效
volumes:
- name: token
  secret:
    secretName: my-sa-token
```

**问题**: Token 永久有效，一旦泄露无法撤销。

---

### TokenRequest + VolumeProjection (K8s >= 1.20, 默认开启)

```yaml
# Pod spec 启用 Token Request
spec:
  serviceAccountName: my-sa
  volumes:
  - name: kube-api-access
    projected:
      sources:
      - serviceAccountToken:
          audience: api           # Token 接收者
          expirationSeconds: 3600 # 1小时后自动过期
          path: token
      - configMap:
          name: kube-root-ca.crt
          items:
          - key: ca.crt
            path: ca.crt
      - downwardAPI:
          items:
          - path: namespace
            fieldRef:
              fieldPath: metadata.namespace
```

---

## BoundServiceAccountTokenVolumeProjection 特性门控

```go
// 从 K8s 1.22 起，-- BoundServiceAccountTokenVolumeProjection=true 默认开启
// 传统 ServiceAccount token 已废弃 (K8s 1.24 移除)
```

**SA token 不再是永久的**，而是通过 TokenRequest API 签发有期限的 JWT。

---

## kubeadm 中的 SA 签名密钥

```bash
# kubeadm init 生成 Service Account 签名密钥对:
# /etc/kubernetes/pki/sa.pub (公钥，API Server 用于验证 Token 签名)
/etc/kubernetes/pki/sa.key (私钥，API Server 用于签发 Token)

# API Server 启动参数:
--service-account-signing-key-file=/etc/kubernetes/pki/sa.key
--service-account-issuer=api
--service-account-api-audiences=api
```

---

## TokenRequest API

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动创建有期限的 ServiceAccount Token
kubectl create token <serviceaccount-name> --duration=1h

# 查看 Token
kubectl create token default --duration=24h
# 输出: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...

# Token 包含:
# - iss: api (API Server 签发者)
# - sub: system:serviceaccount:namespace:serviceaccountname
# - aud: api (接收者)
# - exp: 过期时间
# - iat: 签发时间
```
---

## API Server Audit 配置

kubeadm 支持配置 Audit 审计日志:

```yaml
# audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 不记录只读请求
  - level: None
    resources:
    - users: ["system:kube-proxy"]
    verbs: ["get"]
  # 记录 Pod 变更
  - level: Metadata
    resources:
    - group: ""
      resources: ["pods"]
  # 记录敏感操作 (Secrets, ConfigMaps)
  - level: RequestResponse
    resources:
    - group: "" # core API group
      resources: ["secrets", "configmaps"]
      namespaces: ["kube-system"]
  # 默认记录所有请求
  - level: Metadata
    omitStages:
    - RequestReceived
```

```bash
# kubeadm init 时指定 audit 策略
kubeadm init --audit-policy-file=/path/to/audit-policy.yaml

# 或在 InitConfiguration 中配置:
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
spec:
  apiServer:
    extraArgs:
      audit-policy-file: /etc/kubernetes/audit-policy.yaml
      audit-log-path: /var/log/kubernetes/audit.log
      audit-log-maxage: "30"
      audit-log-maxbackup: "10"
      audit-log-maxsize: "100"
```

---

## API Server 加密配置 (Encryption at Rest)

敏感数据 (Secret) 在 etcd 中加密存储:

```yaml
# encryption-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
    - secrets
    - configmaps
    providers:
    - aescbc:
        keys:
        - name: key1
          secret: <base64-encoded-32-byte-key>
    - identity: {}  # 明文存储 (默认)
```

```bash
# kubeadm init 时指定加密配置
kubeadm init --encryption-provider-config=/etc/kubernetes/encryption-config.yaml
```

---

## NodeRestriction 插件详解

```go
// cmd/kubeadm/app/phases/controlplane/manifests.go
// API Server 启动参数:
--enable-admission-plugins=NodeRestriction
```

NodeRestriction 限制 kubelet 的操作:

```go
// kubelet 只能:
// ✅ 创建/更新自己节点的 Node 对象
// ✅ 创建/更新自己节点的 Pod
// ✅ 设置 node.kubernetes.io/* 注解
// ✅ 设置 kubelet.kubernetes.io/* 注解
// ✅ 设置 node.beta.kubernetes.io/* 注解

// kubelet 不能:
// ❌ 创建/修改其他节点的 Node
// ❌ 创建/修改 kube-system 命名空间的 Pod (除非是 self)
```

---

## RBAC 权限链

```
                    ┌─────────────────────────────────────┐
                    │         API Server                   │
                    │  authorization: RBAC                │
                    └─────────────────────────────────────┘
                                       │
          ┌────────────────────────────┼────────────────────────────┐
          ↓                            ↓                            ↓
  ┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
  │  admin.conf     │       │  kubelet.conf   │       │  CM/Scheduler   │
  │  system:masters │       │  system:nodes   │       │  system:kube-*  │
  └─────────────────┘       └─────────────────┘       └─────────────────┘
          ↓                            ↓                            ↓
  ┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
  │ cluster-admin   │       │ system:node      │       │ system:kube-*   │
  │ (所有权限)      │       │ (节点相关权限)   │       │ (组件相关权限)   │
  └─────────────────┘       └─────────────────┘       └─────────────────┘
```

---

## 网络安全: API Server 访问控制

```bash
# API Server 启动参数控制访问:
--anonymous-auth=false       # 禁用匿名访问

--enable-admission-plugins=NodeRestriction,PodSecurityPolicy
--encryption-provider-config # 加密存储
--audit-policy-file          # 审计日志

```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| `token isn't valid` | Token 过期 | 重新 `kubectl create token` |
| `invalid api token` | Token 签名不对 | 检查 sa.pub/sa.key 是否匹配 |
| `node not found` | kubelet 未完成注册 | 检查 Bootstrap Token 和 CSR 状态 |
| `service account token is not being mounted` | 未启用 TokenRequest | Pod spec 需显式配置 projected volume |
| `audit log permission denied` | 审计日志目录无权限 | 确保 /var/log/kubernetes 可写 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[log|log]]
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[系统基础/知识字典/configuration/secrets.md|secrets]]

```

<!-- risk-assessed -->
