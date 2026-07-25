---
title: 证书轮换机制源码分析 (topic-code-analysis)
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 证书轮换机制源码分析 是什么
- 如何 证书轮换机制源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 证书轮换机制源码分析
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 证书轮换机制源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 管理员
- 集群运维人员
- 安全工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 证书轮换 kubeadm certs renew 机制
- CA 证书轮换高风险操作 完整指南
- kubelet 自动轮换阈值 20% 72小时
- kubeadm upgrade 证书行为 过期中断
- 证书过期应急恢复 时间回拨
trigger_keywords:
- kubeadm certs renew
- 证书轮换
- CA 轮换
- 过期恢复
- check-expiration
- kubelet rotation
- kubeadm upgrade
- 证书过期
- 应急恢复
- 时间回拨
related_domains:
- 集群基础
- 安全
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/ca-generation
- cluster-cert/kubelet-cert
- cluster-cert/openssl-cookbook
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

# 证书轮换机制源码分析

## 概述

Kubernetes 集群证书具有默认 1 年的有效期（CA 10 年），需要定期轮换以保障集群安全。Kubernetes 提供两种轮换机制：
1. **kubeadm 手动/自动轮换** — 控制面证书
2. **kubelet 自动轮换** — 节点证书（基于 CSR）

---

## 源码路径

- **kubeadm 轮换命令**: `cmd/kubeadm/app/cmd/phases/certs/renew.go`
- **kubeadm 轮换实现**: `cmd/kubeadm/app/phases/certs/renew.go`
- **kubelet 轮换**: `pkg/kubelet/certificate/rotation.go`
- **证书有效期检查**: `cmd/kubeadm/app/phases/certs/certs.go`

---

## kubeadm 证书轮换

### 1. 轮换命令入口

```go
// cmd/kubeadm/app/cmd/phases/certs/renew.go
func newCmdRenewAll() *cobra.Command {
    return &cobra.Command{
        Use:   "all",
        Short: "Renew all available certificates",
        RunE: func(cmd *cobra.Command, args []string) error {
            // 轮换所有证书
            return renewCerts(renewAllCerts)
        },
    }
}

func newCmdRenewApiserver() *cobra.Command {
    return &cobra.Command{
        Use:   "apiserver",
        Short: "Renew the certificate for serving the Kubernetes API",
        RunE: func(cmd *cobra.Command, args []string) error {
            return renewCerts([]*certs.KubeadmCert{certs.KubeadmCertApiserver})
        },
    }
}
```

**支持的轮换目标**：

| 命令 | 证书 |
|-----|------|
| `kubeadm certs renew all` | 所有证书 |
| `kubeadm certs renew apiserver` | API Server 服务端证书 |
| `kubeadm certs renew apiserver-kubelet-client` | API Server -> kubelet 客户端证书 |
| `kubeadm certs renew apiserver-etcd-client` | API Server -> etcd 客户端证书 |
| `kubeadm certs renew front-proxy-client` | Front Proxy 客户端证书 |
| `kubeadm certs renew etcd-server` | etcd 服务端证书 |
| `kubeadm certs renew etcd-peer` | etcd Peer 证书 |
| `kubeadm certs renew etcd-healthcheck-client` | etcd 健康检查客户端证书 |
| `kubeadm certs renew admin.conf` | 管理员 kubeconfig |
| `kubeadm certs renew controller-manager.conf` | Controller Manager kubeconfig |
| `kubeadm certs renew scheduler.conf` | Scheduler kubeconfig |

### 2. 轮换核心逻辑

```go
// cmd/kubeadm/app/phases/certs/renew.go
func RenewCert(cfg *kubeadmapi.InitConfiguration, cert *certs.KubeadmCert) error {
    // 1. 加载 CA 证书和私钥
    caCert, caKey, err := loadCA(cfg.CertificatesDir, cert.CAName)
    if err != nil {
        return err
    }

    // 2. 生成新私钥
    newKey, err := pkiutil.NewPrivateKey()
    if err != nil {
        return err
    }

    // 3. 构造证书配置（继承原有配置）
    certConfig := cert.GetConfig(cfg)

    // 4. 使用 CA 签发新证书
    newCert, err := pkiutil.NewCertAndKey(caCert, caKey, certConfig, newKey)
    if err != nil {
        return err
    }

    // 5. 原子写入新证书和密钥
    // 注意：这里使用 0644/0600 权限
    if err := pkiutil.WriteCertAndKey(cfg.CertificatesDir, cert.BaseName, newCert, newKey); err != nil {
        return err
    }

    // 6. 更新 kubeconfig 中的证书（如果是 kubeconfig 类型）
    if cert.IsKubeConfig() {
        return updateKubeConfigCert(cfg, cert, newCert, newKey)
    }

    return nil
}
```

**关键设计**：
- 轮换时**保持 CA 不变**，只更新终端实体证书
- 生成**新私钥**（而非复用旧私钥），增强前向安全性
- 继承原有证书配置（SAN、Organization 等）

### 3. 证书过期检查

```go
// cmd/kubeadm/app/phases/certs/certs.go
func CheckCertificateExpiration(config *kubeadmapi.InitConfiguration) error {
    for _, cert := range certs.KubeadmCerts {
        certPath := filepath.Join(config.CertificatesDir, cert.BaseName+".crt")
        
        certData, err := os.ReadFile(certPath)
        if err != nil {
            continue
        }
        
        cert, err := x509.ParseCertificate(certData)
        if err != nil {
            continue
        }
        
        remaining := cert.NotAfter.Sub(time.Now())
        if remaining < 30*24*time.Hour {
            // 剩余有效期 < 30 天，告警
            fmt.Printf("WARNING: %s expires in %d days\n", certPath, remaining/(24*time.Hour))
        }
    }
    return nil
}
```

**检查命令**：
```bash
$ kubeadm certs check-expiration

CERTIFICATE                EXPIRES                  RESIDUAL TIME   EXTERNALLY MANAGED
admin.conf                 Jan 15, 2026 08:30 UTC   364d            no
apiserver                  Jan 15, 2026 08:30 UTC   364d            no
apiserver-etcd-client      Jan 15, 2026 08:30 UTC   364d            no
apiserver-kubelet-client   Jan 15, 2026 08:30 UTC   364d            no
controller-manager.conf    Jan 15, 2026 08:30 UTC   364d            no
etcd-healthcheck-client    Jan 15, 2026 08:30 UTC   364d            no
etcd-peer                  Jan 15, 2026 08:30 UTC   364d            no
etcd-server                Jan 15, 2026 08:30 UTC   364d            no
front-proxy-client         Jan 15, 2026 08:30 UTC   364d            no
scheduler.conf             Jan 15, 2026 08:30 UTC   364d            no

CERTIFICATE AUTHORITY   EXPIRES                  RESIDUAL TIME   EXTERNALLY MANAGED
ca                      Jan 10, 2035 08:30 UTC   9y              no
etcd-ca                 Jan 10, 2035 08:30 UTC   9y              no
front-proxy-ca          Jan 10, 2035 08:30 UTC   9y              no
```

---

## CA 证书轮换（高风险操作）

### CA 轮换的挑战

CA 证书轮换需要**同时更新所有由该 CA 签发的证书**，否则会导致信任链断裂。

```
# 🟢 低风险：只读/信息收集，通常无副作用
旧 CA (ca.crt.old)          新 CA (ca.crt.new)
     │                            │
     ├── apiserver.crt (旧)       ├── apiserver.crt (新)
     ├── kubelet-client.crt (旧)  ├── kubelet-client.crt (新)
     └── ...                      └── ...

// 必须确保所有组件同时切换到新证书
// 如果 API Server 使用新证书，但 kubectl 使用旧 CA，会导致 TLS 失败
```
### kubeadm 的 CA 轮换支持

kubeadm **不直接支持 CA 轮换**，因为：
1. CA 轮换会导致集群短暂不可用
2. 需要同时更新所有节点上的 CA 信任
3. 需要重新签发所有证书

**手动 CA 轮换步骤**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 1. 备份
sudo cp -r /etc/kubernetes/pki /etc/kubernetes/pki.backup.$(date +%Y%m%d)

# 2. 生成新 CA (或使用原密钥延长有效期)
sudo openssl req -x509 -new -nodes \
  -key /etc/kubernetes/pki/ca.key \
  -subj "/CN=kubernetes-ca" \
  -days 3650 \
  -out /etc/kubernetes/pki/ca.crt

# 3. 使用新 CA 重新签发所有证书
# 注意：kubeadm certs renew 依赖现有 CA 密钥，这里新 CA 已替换，可以正常工作
sudo kubeadm certs renew all

# 4. 重新生成所有 kubeconfig（嵌入新证书）
sudo kubeadm init phase kubeconfig all --config /etc/kubernetes/kubeadm-config.yaml

# 5. 分发新 CA 到所有节点
# 必须同步 /etc/kubernetes/pki/ca.crt 到所有 master 和 worker 节点
# worker 节点需要 CA 来验证 API Server 证书

# 6. 重启所有控制面组件
sudo systemctl restart kubelet

# 7. 所有 worker 节点需要重新生成 bootstrap 或更新 kubelet.conf
# 如果 kubelet 客户端证书由 CSR 签发，删除旧证书触发重新申请
sudo rm /var/lib/kubelet/pki/kubelet-client-*
sudo systemctl restart kubelet
```
---

## 自动化轮换方案

### 方案 1: cron + kubeadm

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# /etc/cron.monthly/k8s-cert-renew
#!/bin/bash
# 每月检查证书，如剩余 < 60 天则轮换

EXPIRY=$(kubeadm certs check-expiration | grep apiserver | awk '{print $3}')
if "$EXPIRY" =~ ^[0-9]+d$; then
    DAYS=${EXPIRY%d}
    if [ "$DAYS" -lt 60 ]; then
        kubeadm certs renew all
        systemctl restart kubelet
    fi
fi
```
### 方案 2: 使用 kubeadm generate-csr（v1.29+ 外部 CA 场景）

当使用外部 CA 时，kubeadm 提供生成 CSR 而非直接签发证书的功能：

```bash
# 生成所有证书的 CSR 文件（不签发，不生成 .crt）
kubeadm certs generate-csr --kubeconfig-dir /etc/kubernetes

# 输出:
# /etc/kubernetes/pki/apiserver.csr
# /etc/kubernetes/pki/apiserver.key
# ...

# 将 CSR 提交给外部 CA 签名后，将证书放回对应目录
# 然后继续 kubeadm init 流程
```

**适用场景**：
- 企业已有内部 PKI / AD CS
- 需要安全团队审批后才能签发证书
- CA 私钥不允许离开 HSM

### 方案 3: 使用 cert-manager 管理自定义证书

对于集群外访问或自定义组件，可使用 cert-manager 自动管理证书：

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: apiserver-cert
  namespace: kube-system
spec:
  secretName: apiserver-tls
  issuerRef:
    name: kubernetes-ca-issuer
    kind: ClusterIssuer
  dnsNames:
    - kubernetes
    - kubernetes.default.svc
  ipAddresses:
    - 10.96.0.1
  duration: 2160h  # 90 天
  renewBefore: 360h  # 15 天前轮换
```

---

## kubeadm upgrade 时的证书行为

```
kubeadm upgrade 不会自动更新证书
                                       │
                   证书已过期？          │
                        │               │
           ┌────────────┴───────────┐   │
           ▼                        ▼   │
    [ 是 ] 升级中断           [ 否 ] 继续升级
    提示: kubeadm certs renew all    │
                                       │
```

**关键行为**：
- `kubeadm upgrade` **不自动轮换证书**
- 如果证书已过期，upgrade 会中断并提示先执行 `kubeadm certs renew all`
- 升级前应先检查证书有效期：`kubeadm certs check-expiration`
- 建议在升级流程中前置证书轮换步骤

**推荐升级流程**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 1. 检查证书
kubeadm certs check-expiration

# 2. 如有需要，先轮换证书
kubeadm certs renew all
systemctl restart kubelet

# 3. 执行升级
kubeadm upgrade apply v1.32.0

```
---

## 证书轮换后的重启要求

| 证书 | 是否需要重启组件 | 重启方式 |
|-----|--------------|---------|
| API Server 证书 | 是 | `systemctl restart kubelet` (静态 Pod) |
| etcd 证书 | 是 | 移动 manifest 文件触发重启 |
| Controller Manager | 是 | `systemctl restart kubelet` |
| Scheduler | 是 | `systemctl restart kubelet` |
| kubelet 证书 | **否** | 热加载 |
| kubeconfig | **否** | 下次连接自动使用新证书 |

---

## 证书过期后的应急恢复

如果证书已经过期且集群部分或全部不可用，按以下优先级恢复：

### 场景 1: 仅 API Server 证书过期，集群仍可部分访问

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 如果还能通过某个 master 节点的本地连接执行命令
sudo kubeadm certs renew apiserver
sudo kubeadm init phase kubeconfig admin --config /etc/kubernetes/kubeadm-config.yaml
sudo cp /etc/kubernetes/admin.conf ~/.kube/config
sudo systemctl restart kubelet
```
### 场景 2: 所有控制面证书过期，API 完全不可用

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 1. 在所有 master 节点上，将系统时间回拨到证书有效期内（临时措施）
sudo date -s "2025-01-14 08:00:00"  # 假设证书 2025-01-15 过期

# 2. 此时 API Server 可以启动，执行证书轮换
sudo kubeadm certs renew all
sudo kubeadm init phase kubeconfig all --config /etc/kubernetes/kubeadm-config.yaml
sudo systemctl restart kubelet

# 3. 恢复正确时间
sudo ntpdate -u pool.ntp.org
# 或重启 chronyd: sudo systemctl restart chronyd

# 4. 验证
kubectl get nodes
```
**⚠️ 警告**：回拨系统时间是高风险的临时措施，只应在完全无法访问的紧急情况下使用，且应尽快恢复正确时间。

### 场景 3: CA 证书过期

```bash
# CA 过期是最严重的场景，需要重建 CA 并重签所有证书
# 步骤详见上文 "CA 证书轮换" 章节
# 如无备份，可能需要重建集群
```

**预防建议**：
- 设置 Prometheus 告警：`apiserver_client_certificate_expiration_seconds < 86400 * 30`
- 每月执行一次 `kubeadm certs check-expiration`
- 维护窗口内主动轮换，不要等到过期

---

## 关键源码索引

| 功能 | 源码路径 |
|-----|---------|
| kubeadm renew 命令 | `cmd/kubeadm/app/cmd/phases/certs/renew.go` |
| 轮换实现 | `cmd/kubeadm/app/phases/certs/renew.go` |
| 过期检查 | `cmd/kubeadm/app/phases/certs/certs.go` |
| kubelet 自动轮换 | `pkg/kubelet/certificate/rotation.go` |
| 证书存储 | `pkg/kubelet/certificate/store.go` |

## Related

- [[reference|#reference Hub]] — tag hub

- [[17-系统基础/05-速查卡/go.md|go]]
- [[17-系统基础/05-速查卡/k8s.md|k8s]]
- [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]]
- [[23-实体/06-安全/cert-manager.md|cert-manager]]
- [[21-生态参考/03-领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]

```

<!-- risk-assessed -->
