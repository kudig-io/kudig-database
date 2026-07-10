---
title: 节点证书轮换 — kubelet 证书自动续期源码分析
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- apiserver
- kubelet
- controller-manager
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点证书轮换 — kubelet 证书自动续期源码分析 是什么
- 如何 节点证书轮换 — kubelet 证书自动续期源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点证书轮换
- kubelet
- 证书自动续期源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 节点证书轮换源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- apiserver
- kubelet
- controller-manager
- rbac
last_updated: 2026-05-18
difficulty: expert
reading_level: expert
audience:
- Kubernetes 安全工程师
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- kubelet certificate auto rotation source code
- TLS Bootstrap node registration flow
- CSR certificate signing request kubelet
- kubelet certificate renewal mechanism
- Bootstrap Token node join
trigger_keywords:
- certificate
- CSR
- TLS Bootstrap
- certificate rotation
- kubelet certificate
- Bootstrap Token
- csrapproving
- csrsigning
- rotateCertificates
- serverTLSBootstrap
- kubelet.conf
- bootstrap-kubelet.conf
related_domains:
- 集群基础
- 安全
related_topics:
- node-create/02-registration
- cluster-create/03-certs
- cluster-create/06-join
- cluster-create/12-join-advanced
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

# 节点证书轮换 — kubelet 证书自动续期源码分析

## 概述

在 Kubernetes 集群中，每个 kubelet 都需要持有有效的客户端证书才能与 API Server 进行安全通信。证书是有有效期的，过期后 kubelet 将无法连接 API Server，导致节点上的 Pod 无法被管理、日志无法采集、状态无法上报。因此，证书的自动轮换机制对集群稳定性至关重要。

kubelet 的证书管理分为两个阶段：**Bootstrap 阶段**和**正式证书阶段**。在 Bootstrap 阶段，kubelet 使用 Bootstrap Token 向 API Server 发起 CSR（Certificate Signing Request），获取正式的客户端证书。在正式证书阶段，kubelet 会监控证书的有效期，在证书即将过期时自动发起新的 CSR 来续期证书。

这个自动轮换机制从 Kubernetes v1.19 起默认启用，是生产环境中 kubelet 证书管理的标准方案。本文档从源码层面深入分析 kubelet 证书轮换的完整流程、CSR 审批机制、常见问题及解决方案。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 证书管理 | `pkg/kubelet/certificate/` | 证书存储、轮换、CSR 创建 |
| kubelet 主入口 | `pkg/kubelet/kubelet.go` | 证书管理器初始化 |
| CSR 审批控制器 | `pkg/controller/certificates/approval/` | 自动审批 CSR |
| CSR 签发控制器 | `pkg/controller/certificates/` | 证书签发 |
| kubeadm bootstrap | `cmd/kubeadm/app/phases/kubelet/` | Bootstrap Token 逻辑 |
| 证书工具 | `staging/src/k8s.io/client-go/util/cert/` | 证书解析工具 |

---

## 一、kubelet 证书管理架构

### 1.1 两个 kubeconfig 文件

kubelet 在证书管理过程中使用两个不同的 kubeconfig 文件：

```bash
# 1. bootstrap-kubelet.conf (首次启动用)
#    包含 Bootstrap Token，用于向 API Server 发起 CSR
#    路径: /etc/kubernetes/bootstrap-kubelet.conf
#    Token 格式: <token-id>.<token-secret>
#    有效期: 默认 24 小时

# 2. kubelet.conf (正式证书)
#    包含签发后的客户端证书，用于正常的 API Server 通信
#    路径: /etc/kubernetes/kubelet.conf
#    证书路径: /var/lib/kubelet/pki/kubelet-client-current.pem
```

### 1.2 证书文件结构

```bash
# kubelet 证书目录
/var/lib/kubelet/pki/
├── kubelet-client-2024-01-01-00-00-00.pem   # 签发的客户端证书（含时间戳）
├── kubelet-client-2024-01-02-00-00-00.pem   # 轮换后的新证书
├── kubelet-client-current.pem               → 软链接到最新证书
├── kubelet.crt                              # 服务端证书（自签名或 CSR 签发）
└── kubelet.key                              # 服务端私钥
```

### 1.3 证书管理器工作流程

```
kubelet 启动
    │
    ├── 检查 /etc/kubernetes/kubelet.conf 是否存在
    │       │
    │       ├── 存在 → 使用正式证书连接 API Server
    │       │
    │       └── 不存在 → 进入 Bootstrap 流程
    │               │
    │               ├── 读取 bootstrap-kubelet.conf
    │               ├── 使用 Bootstrap Token 向 API Server 发起 CSR
    │               ├── 等待 CSR 被 approve 和 sign
    │               ├── 将签发的证书写入 /var/lib/kubelet/pki/
    │               ├── 创建 kubelet.conf（指向签发的证书）
    │               └── 使用正式证书连接 API Server
    │
    └── 启动证书轮换协程
            │
            ├── 定期检查证书剩余有效期
            │   (默认: 剩余有效期 < 80% 时触发轮换)
            │
            ├── 发起新的 CSR
            │
            ├── 等待 CSR 被 approve 和 sign
            │
            ├── 更新证书文件和软链接
            │
            └── 重新加载证书（无需重启 kubelet）
```

---

## 二、源码分析：证书管理器

### 2.1 证书管理器初始化

```go
// pkg/kubelet/certificate/certificate_manager.go
func NewManager(
    kubeClient clientset.Interface,
    certDir string,
    bootstrapCertData *tls.Certificate,
    store CertStore,
    template *x509.CertificateRequest,
    forceCertRotation func() bool,
) (*Manager, error) {
    // 初始化证书管理器:
    // 1. 设置证书存储目录 (certDir = /var/lib/kubelet/pki/)
    // 2. 加载现有证书（如果存在）
    // 3. 设置 CSR 模板
    // 4. 启动证书轮换协程
}
```

### 2.2 证书轮换触发条件

```go
// pkg/kubelet/certificate/certificate_manager.go
func (m *Manager) certSatisfiesTemplate() bool {
    // 检查当前证书是否满足要求:
    // 1. 证书是否存在
    // 2. 证书是否过期
    // 3. 证书剩余有效期是否 > 20% (即剩余 < 80% 时触发轮换)
    // 4. 证书的 SAN 和 Organization 是否匹配
}
```

### 2.3 CSR 创建

```go
// pkg/kubelet/certificate/certificate_manager.go
func (m *Manager) createCertificateSigningRequest(csrPEM []byte) error {
    csr := &certificatesv1.CertificateSigningRequest{
        ObjectMeta: metav1.ObjectMeta{
            GenerateName: "csr-",  // 自动生成名称
            Labels: map[string]string{
                "kubernetes.io/kubelet-serving": "",  // 标记为 kubelet serving CSR
            },
        },
        Spec: certificatesv1.CertificateSigningRequestSpec{
            Request:    csrPEM,                // PEM 编码的 CSR
            SignerName: "kubernetes.io/kube-apiserver-client-kubelet",  // 签发者
            Usages: []certificatesv1.KeyUsage{
                certificatesv1.UsageDigitalSignature,
                certificatesv1.UsageKeyEncipherment,
                certificatesv1.UsageClientAuth,  // 客户端认证用途
            },
            ExpirationSeconds: pointer.Int32(86400 * 365),  // 1 年有效期
        },
    }
    _, err := m.kubeClient.CertificatesV1().CertificateSigningRequests().Create(
        context.TODO(), csr, metav1.CreateOptions{})
    return err
}
```

---

## 三、CSR 审批机制

### 3.1 自动审批控制器

Kubernetes 内置了 `csrapproving` 控制器，自动审批 kubelet 发起的 CSR：

```go
// pkg/controller/certificates/approval/sarapproval.go
// csrapproving 控制器审批条件:
// 1. CSR 的 SignerName 必须是 kubernetes.io/kube-apiserver-client-kubelet
// 2. CSR 的 Subject.Organization 必须包含 system:nodes
// 3. CSR 的 Subject.CommonName 必须以 system:node: 开头
// 4. 请求者必须是 system:node:<name> 或有 Node Bootstrap 权限
```

### 3.2 CSR 签发控制器

审批通过后，`csrsigning` 控制器使用 CA 私钥签发证书：

```go
// pkg/controller/certificates/signer/signer.go
// csrsigning 控制器:
// 1. 读取审批通过的 CSR
// 2. 使用 kubernetes CA 私钥签发证书
// 3. 将签发的证书写入 CSR.Status.Certificate
// 4. kubelet 从 CSR 中获取签发的证书
```

### 3.3 手动审批 CSR

当自动审批失败时，可以手动审批：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 CSR
kubectl get csr
# NAME        AGE   SIGNERNAME                                    REQUESTOR          REQUESTEDDURATION   CONDITION
# csr-abc12   1m    kubernetes.io/kube-apiserver-client-kubelet   system:node:node1  365d                Pending

# 查看 CSR 详情
kubectl describe csr csr-abc12

# 审批 CSR
kubectl certificate approve csr-abc12

# 拒绝 CSR
kubectl certificate deny csr-abc12

# 查看 CSR 中的请求内容
kubectl get csr csr-abc12 -o jsonpath='{.spec.request}' | base64 -d | openssl req -text -noout
```
---

## 四、证书配置

### 4.1 kubelet 证书轮换配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
rotateCertificates: true    # 启用证书自动轮换（默认 true）
serverTLSBootstrap: true    # 启用服务端证书 Bootstrap（通过 CSR 签发服务端证书）
```

### 4.2 API Server 证书审批 RBAC

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# csrapproving 控制器需要的 ClusterRole（kubeadm 自动创建）
kubectl get clusterrole system:certificates.k8s.io:certificatesigningrequests:nodeclient
kubectl get clusterrole system:certificates.k8s.io:certificatesigningrequests:selfnodeclient

# 检查 ClusterRoleBinding
kubectl get clusterrolebinding -o wide | grep certificate
```
---

## 五、证书查看与调试

### 5.1 查看 kubelet 客户端证书

```bash
# 查看当前使用的证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
# notBefore=Jan  1 00:00:00 2024 GMT
# notAfter=Jan  1 00:00:00 2025 GMT

# 查看证书详情
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -subject -issuer
# subject=O = system:nodes, CN = system:node:node-1
# issuer=O = kubernetes, CN = kubernetes

# 查看证书用途
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -ext extendedKeyUsage
```

### 5.2 对比 kubeconfig 中的证书

```bash
# 查看 kubelet.conf 中的证书
cat /etc/kubernetes/kubelet.conf | grep client-certificate-data | \
  awk '{print $2}' | base64 -d | openssl x509 -noout -dates

# 查看所有 kubelet 证书
ls -la /var/lib/kubelet/pki/
```

### 5.3 证书轮换日志

```bash
# 查看 kubelet 证书轮换日志
journalctl -u kubelet | grep -i "certificate|csr|rotation"

# 常见日志:
# "Rotating certificates: preparing CSR"
# "Rotating certificates: CSR created"
# "Rotating certificates: certificate signed"
# "Certificate rotation detected, new cert: /var/lib/kubelet/pki/kubelet-client-xxx.pem"
```

---

## 六、Bootstrap Token 过期处理

### 6.1 Token 有效期

```bash
# Bootstrap Token 默认 24 小时后过期
# 过期后 kubelet 无法发起新的 CSR

# 查看 Token 列表
kubeadm token list
# TOKEN                     TTL         EXPIRES                USAGES
# abcdef.0123456789abcdef   23h         2024-01-02T00:00:00Z  authentication,signing

# 创建新 Token
kubeadm token create
# 输出: xxxxxx.xxxxxxxxxxxxxxxx

# 创建永不过期的 Token（不推荐）
kubeadm token create --ttl 0

# 生成完整 join 命令
kubeadm token create --print-join-command
```

### 6.2 证书过期后的恢复

当 kubelet 证书过期且 Bootstrap Token 也过期时：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
# 步骤 1: 创建新的 Bootstrap Token
kubeadm token create

# 步骤 2: 获取 CA 证书 hash
openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
  openssl rsa -pubin -outform der 2>/dev/null | \
  openssl dgst -sha256 -hex | sed 's/^.* //'

# 步骤 3: 在节点上重新配置 kubelet
# 删除过期证书
rm -f /var/lib/kubelet/pki/kubelet-client-*.pem
rm -f /etc/kubernetes/kubelet.conf

# 重新生成 bootstrap-kubelet.conf
kubeadm join --token <new-token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  <api-server>:6443

# 或者直接使用 kubeadm 重置
kubeadm reset  # ⚠️ 清理节点所有 K8s 配置
kubeadm join ...
```

---

## 七、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| `certificate has expired or is not yet valid` | kubelet 客户端证书过期 | `openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates` | 检查 rotateCertificates 配置，手动续期 |
| CSR 一直是 Pending | csrapproving 控制器未运行或 RBAC 缺失 | `kubectl get csr; kubectl logs -n kube-system kube-controller-manager-xxx` | 检查 controller-manager 日志，手动 `kubectl certificate approve` |
| `x509: certificate signed by unknown authority` | CA 证书不匹配 | `diff /etc/kubernetes/pki/ca.crt /var/lib/kubelet/pki/ca.crt` | 更新节点上的 CA 证书 |
| `Unauthorized` | 证书 CN/O 不正确 | `openssl x509 -in <cert> -noout -subject` | 确保 CN=`system:node:<name>`, O=`system:nodes` |
| kubelet 无法发起 CSR | Bootstrap Token 过期 | `kubeadm token list` | 创建新 Token |
| 证书轮换后节点 NotReady | 新证书未生效 | `journalctl -u kubelet | grep rotation` | 重启 kubelet |

---

## 相关函数

| 函数 | 源码位置 | 说明 |
|------|---------|------|
| `NewManager` | `pkg/kubelet/certificate/certificate_manager.go` | 证书管理器初始化 |
| `certSatisfiesTemplate` | `pkg/kubelet/certificate/certificate_manager.go` | 检查证书是否需要轮换 |
| `createCertificateSigningRequest` | `pkg/kubelet/certificate/certificate_manager.go` | 创建 CSR |
| `rotateCert` | `pkg/kubelet/certificate/certificate_manager.go` | 执行证书轮换 |
| `certStore` | `pkg/kubelet/certificate/store.go` | 证书存储管理 |
| `autoApproveNodeClientCSR` | `pkg/controller/certificates/approval/sarapproval.go` | 自动审批逻辑 |
| `Sign` | `pkg/controller/certificates/signer/signer.go` | 证书签发 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[系统基础/topic-cheat-sheet/go.md|go]]
- [[系统基础/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[系统基础/topic-dictionary/operations/certificates.md|certificates]]
- [[平台工程/topic-code-analysis/node-create/02-registration.md|02-registration]]


<!-- risk-assessed -->
