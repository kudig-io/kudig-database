---
title: 静态传输加密方案
description: '加密方案：etcd EncryptionConfiguration (KMS/AESCBC)、Secret 加密 provider、TLS 严格模式 (mTLS)、证书轮转自动化'
summary: 'etcd 加密、Secret provider、mTLS 与证书自动轮转'
category: security-compliance
tags:
- encryption
- etcd
- tls
- mtls
- kms
- certificate
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 加密方案是什么
- 如何配置 etcd EncryptionConfiguration
trigger_keywords:
- EncryptionConfiguration
- etcd 加密
- KMS
- mTLS
- 证书轮转
prerequisites:
- kubectl-basics
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 静态传输加密方案

## 概述

Kubernetes 安全涉及两个关键的加密维度：静态加密（Encryption at Rest）保护 etcd 中的数据，传输加密（Encryption in Transit）保护组件间通信。本文档涵盖 etcd EncryptionConfiguration、TLS 严格模式和证书轮转自动化。

## 1. etcd EncryptionConfiguration

### 1.1 加密 Provider 类型

| Provider | 安全性 | 性能 | 适用场景 |
|----------|-------|------|---------|
| `identity` | 无加密 | 最快 | 仅用于解密迁移 |
| `aescbc` | 对称加密 | 快 | 本地密钥管理 |
| `aesgcm` | 对称加密 (GCM) | 快 | 需要认证加密 |
| `secretbox` | XSalsa20-Poly1305 | 快 | 轻量级加密 |
| `kms` v1 | KMS 提供 | 中等 | 云 KMS 集成（已弃用） |
| `kms` v2 | KMS 提供 | 中等 | 推荐的 KMS 方式 |

### 1.2 AESCBC 配置

```yaml
# /etc/kubernetes/enc-config.yaml
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
        # 生成密钥：head -c 32 /dev/urandom | base64
        secret: <base64-encoded-32-byte-key>
  - identity: {}  # 用于读取未加密的旧数据
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 生成加密密钥
head -c 32 /dev/urandom | base64

# 启用加密
# 修改 kube-apiserver 配置
# --encryption-provider-config=/etc/kubernetes/enc-config.yaml

# 验证加密已生效
kubectl get secrets -A -o json | jq -r '.items[0].metadata.annotations["encryption.apiserver.kubernetes.io/identity"]'

# 加密所有现有 Secret（触发重新加密）
kubectl get secrets --all-namespaces -o json | kubectl replace -f -
```
### 1.3 KMS v2 配置（推荐）

```yaml
# /etc/kubernetes/enc-config-kms.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  providers:
  - kms:
      apiVersion: v2
      name: aws-kms
      endpoint: unix:///var/run/kms-provider.sock
      cachesize: 1000
      timeout: 3s
  - identity: {}
```

AWS KMS Plugin 部署：

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: aws-kms-provider
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: aws-kms-provider
  template:
    metadata:
      labels:
        app: aws-kms-provider
    spec:
      nodeSelector:
        node-role.kubernetes.io/control-plane: ""
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
      containers:
      - name: kms-provider
        image: public.ecr.aws/aws-encryption-provider/aws-encryption-provider:v0.0.15
        command:
        - /aws-encryption-provider
        - --key=<kms-key-arn>
        - --region=ap-northeast-1
        - --listen=/var/run/kms-provider.sock
        volumeMounts:
        - name: kms-socket
          mountPath: /var/run
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
      volumes:
      - name: kms-socket
        hostPath:
          path: /var/run
```

HashiCorp Vault KMS Plugin：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vault-kms-provider
  namespace: kube-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: vault-kms-provider
  template:
    metadata:
      labels:
        app: vault-kms-provider
    spec:
      containers:
      - name: kms-provider
        image: hashicorp/vault-k8s:latest
        command:
        - /bin/sh
        - -c
        - |
          vault server -config=/etc/vault/config.hcl
        env:
        - name: VAULT_ADDR
          value: "https://vault.example.com:8200"
        - name: VAULT_CACERT
          value: "/etc/vault/tls/ca.crt"
        volumeMounts:
        - name: vault-config
          mountPath: /etc/vault
        - name: kms-socket
          mountPath: /var/run
      volumes:
      - name: vault-config
        configMap:
          name: vault-kms-config
      - name: kms-socket
        hostPath:
          path: /var/run
```

### 1.4 加密 Provider 迁移

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# 从 aescbc 迁移到 KMS v2
set -euo pipefail

# Step 1: 添加新 provider（放在列表第一位）
cat > /tmp/enc-migration.yaml <<EOF
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  providers:
  - kms:
      apiVersion: v2
      name: aws-kms
      endpoint: unix:///var/run/kms-provider.sock
  - aescbc:
      keys:
      - name: key1
        secret: <old-key>
  - identity: {}
EOF

# Step 2: 重启 API Server
systemctl restart kubelet

# Step 3: 触发所有 Secret 重新加密
kubectl get secrets -A -o json | kubectl replace -f -

# Step 4: 验证所有 Secret 已使用新 provider
kubectl get secrets -A -o json | jq -r '.items[] |
  .metadata.annotations["encryption.apiserver.kubernetes.io/identity"]' | sort | uniq -c

# Step 5: 移除旧 provider
cat > /tmp/enc-final.yaml <<EOF
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  providers:
  - kms:
      apiVersion: v2
      name: aws-kms
      endpoint: unix:///var/run/kms-provider.sock
  - identity: {}
EOF

# Step 6: 重启 API Server 并验证
systemctl restart kubelet
```
## 2. TLS 严格模式（mTLS）

### 2.1 组件间 mTLS 配置

```yaml
# kube-apiserver TLS 配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  extraArgs:
    tls-min-version: VersionTLS12
    tls-cipher-suites: TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
    client-ca-file: /etc/kubernetes/pki/ca.crt
    tls-cert-file: /etc/kubernetes/pki/apiserver.crt
    tls-private-key-file: /etc/kubernetes/pki/apiserver.key
    kubelet-client-certificate: /etc/kubernetes/pki/apiserver-kubelet-client.crt
    kubelet-client-key: /etc/kubernetes/pki/apiserver-kubelet-client.key
    etcd-cafile: /etc/kubernetes/pki/etcd/ca.crt
    etcd-certfile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    etcd-keyfile: /etc/kubernetes/pki/apiserver-etcd-client.key
```

### 2.2 etcd mTLS 配置

```yaml
# etcd 静态 Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    command:
    - etcd
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --client-cert-auth=true
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --peer-client-cert-auth=true
    - --cipher-suites=TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
```

### 2.3 Service Mesh mTLS

```yaml
# Istio 严格 mTLS 策略
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
---
# 命名空间级别 mTLS
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: production-mtls
  namespace: production
spec:
  mtls:
    mode: STRICT
---
# 目标规则
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: production-mtls
  namespace: production
spec:
  host: "*.production.svc.cluster.local"
  trafficPolicy:
    tls:
      mode: ISTIO_MUTUAL
```

## 3. 证书轮转自动化

### 3.1 cert-manager 配置

```yaml
# 安装 cert-manager
# kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.14.0/cert-manager.yaml

# 集群 CA 证书
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: kubernetes-ca
spec:
  ca:
    secretName: kubernetes-ca-key-pair
---
# 应用证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: my-app-tls
  namespace: production
spec:
  secretName: my-app-tls
  issuerRef:
    name: kubernetes-ca
    kind: ClusterIssuer
  duration: 2160h    # 90 天
  renewBefore: 360h  # 提前 15 天续期
  dnsNames:
  - my-app.production.svc.cluster.local
  - my-app.example.com
  privateKey:
    algorithm: ECDSA
    size: 256
```

### 3.2 kubelet 证书轮转

```yaml
# kubelet 配置启用证书轮转
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
rotateCertificates: true
serverTLSBootstrap: true
```

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 检查 kubelet 证书状态
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 手动触发证书轮转
# kubelet 会在证书过期前自动轮转
# 可以通过删除证书文件强制轮转
rm /var/lib/kubelet/pki/kubelet-client-current.pem
systemctl restart kubelet
```
### 3.3 API Server 证书轮转

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# API Server 证书轮转脚本（kubeadm 集群）
set -euo pipefail

# 检查证书过期时间
kubeadm certs check-expiration

# 续期所有证书
kubeadm certs renew all

# 重启控制平面组件
systemctl restart kubelet

# 验证新证书
kubeadm certs check-expiration

# 更新 kubeconfig
cp /etc/kubernetes/admin.conf ~/.kube/config
```
## 4. 安全最佳实践

### 4.1 加密配置检查清单

```
静态加密检查清单：

□ 启用 etcd EncryptionConfiguration
□ 使用 KMS v2 作为首选加密 provider
□ 定期轮转加密密钥
□ 配置加密 provider 迁移流程
□ 验证所有 Secret 已加密

传输加密检查清单：

□ 配置 TLS 最低版本（TLS 1.2+）
□ 限制 TLS 密码套件
□ 启用 etcd mTLS
□ 启用 kubelet 客户端证书认证
□ 使用 Service Mesh 实现应用层 mTLS
□ 配置 cert-manager 自动证书管理
□ 启用 kubelet 证书自动轮转
□ 定期检查证书过期时间
□ 配置证书过期告警
```

### 4.2 监控配置

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-expiry
spec:
  groups:
  - name: certificate.rules
    rules:
    - alert: CertificateExpiringSoon
      expr: |
        (certmanager_certificate_expiration_timestamp_seconds - time()) / 86400 < 30
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Certificate expiring in less than 30 days"
        description: "Certificate {{ $labels.name }} in {{ $labels.namespace }} expires in {{ $value }} days"

    - alert: etcdEncryptionError
      expr: |
        apiserver_storage_encryption_duration_seconds_count{transformation_type="encrypt"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "etcd encryption not functioning"
```

## Related

- [[domain-05-security-compliance/06-compliance/01-kubernetes-audit-logging-configuration|审计日志配置]]
- [[domain-05-security-compliance/01-identity-access/01-rbac-best-practices|RBAC 最佳实践]]

## See Also

- [EncryptionConfiguration 文档](https://kubernetes.io/docs/tasks/administer-cluster/encrypt-data/)
- [cert-manager 文档](https://cert-manager.io/docs/)


<!-- risk-assessed -->
