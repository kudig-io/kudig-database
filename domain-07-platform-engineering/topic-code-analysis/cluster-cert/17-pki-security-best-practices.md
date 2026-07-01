---
title: Kubernetes PKI 安全最佳实践 (topic-code-analysis)
description: 'title: Kubernetes PKI 安全最佳实践'
summary: 'title: Kubernetes PKI 安全最佳实践'
category: general
tags:
- reference
- security
- best-practice
- etcd
- apiserver
- kubelet
- prometheus
- job
- rbac
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes PKI 安全最佳实践 是什么
- 如何 Kubernetes PKI 安全最佳实践
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- Kubernetes
- PKI
- 安全最佳实践
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
- etcd-basics
- tls-basics
---



title: Kubernetes PKI 安全最佳实践
description: '# Kubernetes PKI 安全最佳实践'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- prometheus
- job
- rbac
- webhook
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- Kubernetes 管理员
- 合规管理员
estimated_read_time: 5min
intent_queries:
- Kubernetes PKI 安全加固 私钥保护 证书监控
- Kubernetes CA 私钥离线存储 HSM KMS
- 证书有效期配置 轮换策略 Prometheus 告警
- CIS Benchmark Kubernetes 证书相关检查项
- 最小权限原则 CA 证书 Organization 策略
trigger_keywords:
- PKI 安全
- 私钥保护
- KMS
- HSM
- 证书监控
- Prometheus 告警
- CIS Benchmark
- 最小权限
- CA 离线存储
- 证书轮换
related_domains:
- domain-01-cluster-fundamentals
- domain-05-security-compliance
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/ca-generation
- cluster-cert/cert-rotation
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

# Kubernetes PKI 安全最佳实践

## 概述

Kubernetes 集群的 PKI 安全不仅关乎证书是否正确配置，更涉及私钥保护、证书监控、最小权限原则等多个维度。本文档基于生产环境安全规范，提供系统性的 PKI 安全加固指南。

---

## 一、私钥保护

### 1.1 文件系统权限

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `chmod/chown -R`：递归改权限，误操作破坏系统文件访问

```bash
# 当前权限检查
ls -la /etc/kubernetes/pki/

# 标准权限设置
chmod 644 /etc/kubernetes/pki/*.crt      # 证书：所有人可读
chmod 600 /etc/kubernetes/pki/*.key      # 私钥：仅 root 可读写
chmod 600 /etc/kubernetes/pki/*.pub      # SA 公钥：所有人可读即可，但建议 644
chmod 600 /etc/kubernetes/pki/etcd/*.key # etcd 私钥

# 递归设置 PKI 目录权限
find /etc/kubernetes/pki -type f -name "*.crt" -exec chmod 644 {} \;
find /etc/kubernetes/pki -type f -name "*.key" -exec chmod 600 {} \;
chown -R root:root /etc/kubernetes/pki
```

**为什么私钥必须是 600**：
- 任何能够读取私钥的用户都可以冒充该组件身份
- 例如：读取 `ca.key` 即可签发任意证书，绕过所有认证

### 1.2 私钥存储加固

| 方案 | 安全级别 | 实现方式 | 适用场景 |
|-----|---------|---------|---------|
| 本地文件系统 (默认) | 中 | `/etc/kubernetes/pki/*.key` | 开发/测试环境 |
| 文件系统 + 加密 | 中高 | LUKS 加密磁盘存储 PKI | 中小型生产环境 |
| HSM (硬件安全模块) | 高 | Thales Luna, AWS CloudHSM | 金融/高安全要求 |
| 云 KMS | 高 | AWS KMS, GCP KMS, Azure Key Vault | 云原生部署 |
| HashiCorp Vault | 高 | Vault PKI 引擎动态签发 | 企业级自动化 |

### 1.3 使用 KMS 保护 CA 私钥

```yaml
# 使用 HashiCorp Vault 作为外部 CA
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
# 将 Vault 配置为外部 CA，kubeadm 不生成 CA
# 需要预先将 Vault 签发的证书放入 /etc/kubernetes/pki/
```

**Vault PKI 集成逻辑**：
```
kubeadm init → 检测外部 CA (ca.crt 存在, ca.key 不存在)
           → 生成 CSR
           → 提交 Vault 签名
           → Vault 使用受保护的 CA 私钥签名
           → 将签发后的证书写入 /etc/kubernetes/pki/
```

---

## 二、证书有效期与轮换策略

### 2.1 推荐有效期配置

| 证书类型 | kubeadm 默认 | 生产推荐 | 理由 |
|---------|------------|---------|------|
| CA 证书 | 10 年 | 5-10 年 | CA 轮换成本极高，不宜过短 |
| 服务端/客户端证书 | 1 年 | 90 天 - 1 年 | 平衡安全与运维负担 |
| kubelet 证书 | 1 年 (CSR) | 默认即可 | 自动轮换，无需干预 |
| Webhook 证书 | 不定 | 90 天 | cert-manager 自动管理 |
| SA 签名密钥 | 无过期 | 每年手动轮换 | 无自动轮换机制 |

### 2.2 证书监控告警

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-alerts
spec:
  groups:
  - name: certificates
    rules:
    - alert: KubernetesCertificateExpiringSoon
      expr: |
        (
          apiserver_client_certificate_expiration_seconds_count{job="apiserver"} > 0
          and
          apiserver_client_certificate_expiration_seconds{job="apiserver"} < 86400 * 30
        )
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Kubernetes certificate expiring in less than 30 days"
    
    - alert: KubernetesCertificateExpired
      expr: |
        apiserver_client_certificate_expiration_seconds{job="apiserver"} < 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Kubernetes certificate has expired"
```

### 2.3 证书轮换的变更管理

```
┌─────────────────────────────────────────────────────────────┐
│                    证书轮换变更管理流程                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. 轮换前 7 天                                              │
│     └─ 创建变更单，通知相关团队                               │
│                                                              │
│  2. 轮换前 1 天                                              │
│     └─ 备份 /etc/kubernetes/pki                              │
│     └─ 验证备份可恢复                                         │
│                                                              │
│  3. 维护窗口执行                                             │
│     └─ kubeadm certs renew <target>                          │
│     └─ 验证新证书有效期                                       │
│     └─ 验证证书链完整                                         │
│                                                              │
│  4. 组件重启                                                 │
│     └─ 滚动重启控制面组件                                     │
│     └─ 监控组件启动日志                                       │
│                                                              │
│  5. 功能验证                                                 │
│     └─ kubectl get nodes                                     │
│     └─ kubectl get pods -n kube-system                       │
│     └─ 测试核心功能（创建 Pod、访问 Service）                │
│                                                              │
│  6. 监控观察（轮换后 1 小时）                                │
│     └─ 观察 API Server 错误率                                │
│     └─ 观察节点 NotReady 事件                                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、最小权限原则

### 3.1 证书中的 Organization 策略

**反模式**：
```go
// 不要给非管理员组件 system:masters
Config: certutil.Config{
    CommonName:   "my-app",
    Organization: []string{"system:masters"},  // ← 危险！
}
```

**正确做法**：
```go
// 为组件创建专用的 RBAC 角色
Config: certutil.Config{
    CommonName:   "my-app",
    Organization: []string{"my-app-group"},
}

// 然后绑定最小权限 ClusterRole
// subjects:
// - kind: Group
//   name: my-app-group
// roleRef:
//   kind: ClusterRole
//   name: my-app-minimal-role
```

### 3.2 API Server 聚合层的权限隔离

```bash
# 限制 --requestheader-allowed-names 为严格白名单
--requestheader-allowed-names=front-proxy-client

# 禁止通配符或空值
# 错误的配置：
# --requestheader-allowed-names=""  # 允许任何 front-proxy CA 签发的证书
```

---

## 四、CA 安全

### 4.1 CA 私钥的离线存储

**生产环境建议**：
1. **初始部署后，将 CA 私钥离线存储**
   ```bash
   # 部署完成后，将 ca.key 加密并移出集群节点
   gpg --symmetric --cipher-algo AES256 /etc/kubernetes/pki/ca.key
   # 将 ca.key.gpg 存储到安全的离线位置（如密码管理器、HSM）
   # 从节点删除 ca.key
   shred -u /etc/kubernetes/pki/ca.key
   ```

2. **使用外部 CA 模式**
   ```bash
   # 部署时只放置 ca.crt，不放置 ca.key
   # kubeadm 会自动进入外部 CA 模式
   # 后续证书更新需要通过外部 PKI 系统
   ```

### 4.2 CA 轮换的灾难恢复预案

```bash
#!/bin/bash
# CA 轮换应急预案

# Phase 1: 评估影响
echo "=== CA Rotation Impact Assessment ==="
NODES=$(kubectl get nodes -o name | wc -l)
PODS=$(kubectl get pods -A --field-selector status.phase=Running | wc -l)
echo "Nodes: $NODES, Running Pods: $PODS"

# Phase 2: 完整备份
BACKUP_DIR="/backup/k8s-pki-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$BACKUP_DIR"
cp -r /etc/kubernetes/pki "$BACKUP_DIR/"
cp /etc/kubernetes/*.conf "$BACKUP_DIR/"
ETCDCTL_API=3 etcdctl snapshot save "$BACKUP_DIR/etcd.snapshot" \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# Phase 3: 维护窗口声明
cat <<EOF
WARNING: Cluster CA rotation in progress
- All control plane components will be restarted
- API will be briefly unavailable
- Worker nodes may show NotReady temporarily
EOF

# Phase 4: 执行轮换（详见 06-cert-rotation.md）
# ...

# Phase 5: 自动验证
kubectl get nodes || { echo "CRITICAL: Node communication failed"; exit 1; }
kubectl get pods -n kube-system || { echo "CRITICAL: API Server error"; exit 1; }
```

---

## 五、证书审计与合规

### 5.1 证书清单自动化

```bash
#!/bin/bash
# Kubernetes PKI 资产清单脚本

OUTPUT="/var/log/k8s-pki-inventory-$(date +%Y%m%d).json"

echo "{" > "$OUTPUT"
echo '  "generated_at": "'$(date -Iseconds)'",' >> "$OUTPUT"
echo '  "certificates": [' >> "$OUTPUT"

first=true
for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
  [ -f "$cert" ] || continue
  
  subject=$(openssl x509 -in "$cert" -noout -subject 2>/dev/null | sed 's/subject=//')
  issuer=$(openssl x509 -in "$cert" -noout -issuer 2>/dev/null | sed 's/issuer=//')
  enddate=$(openssl x509 -in "$cert" -noout -enddate 2>/dev/null | cut -d= -f2)
  fingerprint=$(openssl x509 -in "$cert" -noout -fingerprint -sha256 2>/dev/null | cut -d= -f2)
  
  if [ "$first" = true ]; then
    first=false
  else
    echo "," >> "$OUTPUT"
  fi
  
  cat <<CERT >> "$OUTPUT"
  {
    "file": "$(basename $cert)",
    "path": "$cert",
    "subject": "$subject",
    "issuer": "$issuer",
    "expires": "$enddate",
    "sha256_fingerprint": "$fingerprint"
  }
CERT
done

echo "" >> "$OUTPUT"
echo '  ]' >> "$OUTPUT"
echo "}" >> "$OUTPUT"

echo "Inventory saved to $OUTPUT"
```

### 5.2 CIS Benchmark 证书相关检查项

| CIS 检查项 | 要求 | 验证命令 |
|-----------|------|---------|
| 1.2.29 | etcd 使用 TLS | `ps aux | grep etcd | grep -E "cert-file|key-file"` |
| 1.2.30 | API Server 使用 TLS | `ps aux | grep apiserver | grep tls-cert-file` |
| 1.2.31 | API Server 客户端 CA 配置 | `ps aux | grep apiserver | grep client-ca-file` |
| 1.2.32 | etcd CA 文件权限 | `stat -c %a /etc/kubernetes/pki/etcd/ca.crt` 应为 644 |
| 1.2.33 | etcd 私钥权限 | `stat -c %a /etc/kubernetes/pki/etcd/ca.key` 应为 600 |
| 4.1.9 | kubelet 客户端 CA | `cat /var/lib/kubelet/config.yaml | grep clientCAFile` |
| 4.1.10 | kubelet 证书轮换 | `cat /var/lib/kubelet/config.yaml | grep rotateCertificates` |

---

## 六、常见安全反模式

| 反模式 | 风险 | 正确做法 |
|-------|------|---------|
| 将 `ca.key` 提交到 Git | 私钥泄露，任何人可签发证书 | 使用 Git-Crypt / SOPS 加密，或外部 Secret 管理 |
| 证书有效期 > 2 年 | 泄露的证书长期有效 | 使用 90 天 - 1 年有效期 |
| 所有组件共用同一 CA | 单点问题，权限无法隔离 | 使用 kubernetes-ca / etcd-ca / front-proxy-ca 分离 |
| 忽略证书过期告警 | 集群突然不可用 | 配置 Prometheus 告警 + PagerDuty |
| 手动编辑证书文件 | 格式损坏、密钥不匹配 | 使用 kubeadm 或脚本自动化 |
| 没有证书备份 | 丢失后无法恢复 | 定期备份 /etc/kubernetes/pki 到加密存储 |
| Webhook 使用 kubernetes-ca | CA 轮换影响 Webhook | Webhook 使用独立的 CA |

---

## 七、供应链安全

### 7.1 证书签发审批流程

```
┌─────────────────────────────────────────────────────────────┐
│                证书签发审批流程 (企业级)                      │
├─────────────────────────────────────────────────────────────┤
│  1. 申请阶段                                                 │
│     - 组件/服务提交 CSR 或证书申请单                          │
│     - 指定 CommonName、Organization、有效期、SAN             │
│     - 关联到具体的 RBAC 权限申请                             │
│                                                              │
│  2. 安全审查                                                 │
│     - CN/O 命名规范检查                                      │
│     - Organization 权限范围评估                              │
│     - SAN 必要性确认                                         │
│     - 签名算法检查（拒绝 SHA-1、MD5）                        │
│                                                              │
│  3. 审批与签发                                               │
│     - 由安全团队审批                                         │
│     - 使用 HSM 或 KMS 保护的 CA 私钥签发                     │
│     - 签发记录审计日志                                       │
│                                                              │
│  4. 分发与配置                                               │
│     - 证书分发到目标组件                                      │
│     - 更新对应的 RBAC 绑定                                   │
│     - 监控证书使用情况                                        │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 入侵检测与证书异常监控

```yaml
# 异常证书行为监控指标
# 1. 新证书签发（除 kubeadm 正常轮换外）
# 2. 证书 Subject 异常变更
# 3. 来自异常 IP/区域的证书申请
# 4. CA 密钥访问日志异常

# 推荐告警规则 (PromQL)
- alert: CertificateSigningRateAnomaly
  expr: rate(apiserver_certificate_signing_requests_total[5m]) > 10
  for: 5m
  labels:
    severity: warning
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/sops.md|sops]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-19-landscape-references/topic-index/cert-index.md|Certificate / TLS 证书知识图谱索引]]

```