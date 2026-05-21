---
title: kubelet 证书与 CSR 机制源码分析
description: 'description: ''## 概述'''
category: general
tags:
- reference
- apiserver
- kubelet
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- kubelet 证书与 CSR 机制源码分析 是什么
- 如何 kubelet 证书与 CSR 机制源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- kubelet
- 证书与
- CSR
- 机制源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
---

title: kubelet 证书与 CSR 机制源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- apiserver
- kubelet
- rbac
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 管理员
- 节点运维人员
- 集群运维人员
estimated_read_time: 5min
intent_queries:
- Kubernetes kubelet 引导证书 Bootstrap Token CSR 自动签发
- kubelet 客户端证书轮换 rotation 机制源码
- kubelet 服务端证书 serverTLSBootstrap metrics API
- CSR CertificateSigningRequest kubelet 证书管理器
- kubelet 双证书管理器 ClientCertificateManager ServerCertificateManager
trigger_keywords:
- kubelet
- Bootstrap Token
- CSR
- CertificateSigningRequest
- 证书轮换
- rotateCertificates
- serverTLSBootstrap
- kubelet-client-current.pem
- kubelet-server-current.pem
- 自动签发
related_domains:
- domain-01-cluster-fundamentals
- domain-4-nodes
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/join-cert-flow
- cluster-cert/cert-rotation
- cluster-cert/apiserver-cert-flags
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

# kubelet 证书与 CSR 机制源码分析

## 概述

kubelet 证书管理是 Kubernetes 集群证书体系中最复杂的部分。与其他控制面组件不同，kubelet 采用 **引导证书（Bootstrap Token）+ CSR（Certificate Signing Request）自动签发** 机制，使节点能够自动加入集群并管理自身证书。

---

## 源码路径

- **kubelet 证书管理器**: `pkg/kubelet/certificate/kubelet.go`
- **kubelet 证书轮换**: `pkg/kubelet/certificate/rotation.go`
- **CSR 控制器**: `pkg/controller/certificates/signer/signer.go`
- **kubeadm 引导**: `cmd/kubeadm/app/phases/kubelet/config.go`
- **Bootstrap Token**: `cmd/kubeadm/app/phases/bootstraptoken/node/tls.go`

---

## kubelet 证书类型

kubelet 需要两类证书：

| 证书类型 | 用途 | 签发方式 |
|---------|------|---------|
| **客户端证书** | kubelet -> API Server 的身份认证 | Bootstrap / CSR |
| **服务端证书** | kubelet 提供 metrics/logs API | Bootstrap / CSR |

---

## kubelet 双证书管理器架构

```go
// pkg/kubelet/kubelet.go
func NewMainKubelet(...) (*Kubelet, error) {
    // 1. 客户端证书管理器（连接 API Server）
    clientCertificateManager, err := certificate.NewKubeletClientCertificateManager(...)
    
    // 2. 服务端证书管理器（提供 kubelet 10250 HTTPS）
    serverCertificateManager, err := certificate.NewKubeletServerCertificateManager(...)
    
    // 3. 将服务端证书管理器注入 HTTP Server
    k.serverTLSConfig.GetCertificate = serverCertificateManager.GetCertificate
}
```

**两个独立的 Manager**：

| Manager | 负责 | 存储路径 | 默认启用 |
|---------|------|---------|---------|
| `ClientCertificateManager` | 客户端证书轮换 | `/var/lib/kubelet/pki/kubelet-client-current.pem` | 是 |
| `ServerCertificateManager` | 服务端证书轮换 | `/var/lib/kubelet/pki/kubelet-server-current.pem` | 否（需 `serverTLSBootstrap: true`） |

**设计原因**：
- 客户端证书用于 kubelet ** outbound ** 连接（→ API Server）
- 服务端证书用于 kubelet ** inbound ** 连接（→ metrics/exec/logs）
- 两者独立轮换，互不影响
- 服务端证书需要额外批准 CSR，因此默认关闭

---

## 引导证书机制（Bootstrap）

### 1. 引导 kubeconfig

新节点加入集群时，首先使用引导 kubeconfig，其中包含一个 Bootstrap Token：

```go
// cmd/kubeadm/app/phases/kubelet/config.go
func WriteKubeletBootstrapConfigFile(bootstrapKubeConfigFile string, ... ) error {
    // 生成包含 Bootstrap Token 的 kubeconfig
    bootstrapConfig := &clientcmdapi.Config{
        Clusters: map[string]*clientcmdapi.Cluster{
            "kubernetes": {
                Server:                   apiServerEndpoint,
                CertificateAuthorityData: caCertData,
            },
        },
        AuthInfos: map[string]*clientcmdapi.AuthInfo{
            "kubelet-bootstrap": {
                Token: bootstrapToken,
            },
        },
    }
    return clientcmd.WriteToFile(*bootstrapConfig, bootstrapKubeConfigFile)
}
```

**引导 kubeconfig 路径**: `/etc/kubernetes/bootstrap-kubelet.conf`

**Bootstrap Token 有效期**：
- 默认有效期：**24 小时**
- 可通过 `--ttl` 参数自定义：`kubeadm token create --ttl 48h`
- 永不过期：`kubeadm token create --ttl 0`

### 2. Bootstrap Token 的 RBAC 权限

```yaml
# kubeadm 自动创建的 ClusterRoleBinding
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: kubeadm:node-autoapprove-bootstrap
roleRef:
  kind: ClusterRole
  name: system:certificates.k8s.io:certificatesigningrequests:nodeclient
  apiGroup: rbac.authorization.k8s.io
subjects:
- kind: Group
  name: system:bootstrappers:kubeadm:default-node-token
  apiGroup: rbac.authorization.k8s.io
```

**权限说明**：
- `system:bootstrappers:*` 组的节点可以自动创建并获批 CSR
- 首次 CSR 批准后，节点获得正式客户端证书

---

## CSR 自动签发流程

### 1. kubelet 首次启动时申请证书

```go
// pkg/kubelet/certificate/kubelet.go
func NewKubeletCertificateManager(...) (certificate.Manager, error) {
    // 1. 检查是否存在已签发的证书
    // 2. 如果不存在，使用引导 kubeconfig 创建 CSR
    // 3. 等待 CSR 被批准并下发证书
    // 4. 将签发的证书写入 /var/lib/kubelet/pki/
}
```

### 2. kubelet 创建 CSR

```go
// pkg/kubelet/certificate/kubelet.go
func (m *manager) rotateCerts() (bool, error) {
    // 生成新的私钥
    privateKey, err := m.keyGenerator()
    
    // 构造 CSR
    csrPEM, privateKeyPEM, err := m.generateCSR(privateKey)
    
    // 提交 CSR 到 API Server
    reqName, reqUID, err := m.submitCSR(csrPEM)
    
    // 等待 CSR 被批准
    // 轮询 CSR 状态，直到证书被签发
}
```

### 3. CSR 资源结构

```yaml
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: csr-abc123
spec:
  request: <base64-encoded-csr>
  signerName: kubernetes.io/kube-apiserver-client-kubelet
  usages:
    - digital signature
    - key encipherment
    - client auth
  username: system:bootstrap:abc123  # 创建者身份
  groups:
    - system:bootstrappers
    - system:authenticated
status:
  conditions:
    - type: Approved
      status: "True"
      reason: AutoApproved
      message: Auto approving kubelet client certificate
  certificate: <base64-encoded-cert>
```

### 4. CSR 审批控制器

```go
// pkg/controller/certificates/signer/signer.go
type CSRSigningController struct {
    signerName string
    // ...
}

func (c *CSRSigningController) sync(ctx context.Context, csr *certificates.CertificateSigningRequest) error {
    // 1. 检查 CSR 的 signerName
    if csr.Spec.SignerName != c.signerName {
        return nil
    }
    
    // 2. 验证 CSR 内容（防止恶意 CSR）
    if err := c.validator(csr); err != nil {
        return err
    }
    
    // 3. 使用 CA 签发证书
    cert, err := c.signer.Sign(csr.Spec.Request)
    
    // 4. 将证书写入 CSR status
    csr.Status.Certificate = cert
    csr.Status.Conditions = append(csr.Status.Conditions, 
        certificates.CertificateSigningRequestCondition{
            Type:   certificates.CertificateApproved,
            Status: v1.ConditionTrue,
        })
    
    return c.client.UpdateApproval(ctx, csr)
}
```

---

## kubelet 证书轮换（Rotation）

### 1. 轮换触发条件

```go
// pkg/kubelet/certificate/rotation.go
func (m *manager) shouldRotate() bool {
    cert := m.currentCertificate()
    
    // 计算证书剩余有效期比例
    totalDuration := cert.NotAfter.Sub(cert.NotBefore)
    remaining := cert.NotAfter.Sub(time.Now())
    remainingRatio := remaining / totalDuration
    
    // 当剩余有效期 < 阈值时触发轮换
    // 默认阈值: 20% (即证书已使用 80% 时开始轮换)
    if remainingRatio < m.rotationThreshold {
        return true
    }
    
    // 或者证书即将过期（< 72 小时）
    if remaining < 72 * time.Hour {
        return true
    }
    
    return false
}
```

**默认轮换阈值**：
- 触发阈值：证书有效期的 **20%** 剩余时间
- 对于 1 年有效期证书：到期前约 **73 天** 开始轮换
- 紧急阈值：到期前 **72 小时** 强制轮换

### 2. 轮换流程

```
┌─────────────────────────────────────────────────────────────┐
│                   kubelet 证书轮换流程                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. 监控证书有效期                                           │
│     └─ 每 10 分钟检查一次                                      │
│                                                              │
│  2. 触发条件满足？                                            │
│     └─ 剩余有效期 < 20% 或 < 72 小时                         │
│                                                              │
│  3. 生成新私钥                                               │
│     └─ /var/lib/kubelet/pki/kubelet-client-XXXX.pem         │
│                                                              │
│  4. 创建并提交 CSR                                           │
│     └─ POST /apis/certificates.k8s.io/v1/csrs               │
│                                                              │
│  5. 等待 CSR 审批                                            │
│     └─ 自动审批（AutoApproval）或手动审批                      │
│                                                              │
│  6. 获取新证书                                               │
│     └─ 从 CSR status.certificate 提取                        │
│                                                              │
│  7. 原子替换证书                                             │
│     └─ 更新符号链接 kubelet-client-current.pem               │
│                                                              │
│  8. 无需重启 kubelet                                         │
│     └─ 证书热加载                                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 3. 原子替换机制

```go
// pkg/kubelet/certificate/store.go
func (s *fileStore) Update(certData []byte) error {
    // 1. 写入临时文件
    tempFile := filepath.Join(s.certDirectory, fmt.Sprintf("%s.tmp", s.certFile))
    if err := os.WriteFile(tempFile, certData, 0600); err != nil {
        return err
    }
    
    // 2. 原子重命名到目标文件
    targetFile := filepath.Join(s.certDirectory, s.certFile)
    if err := os.Rename(tempFile, targetFile); err != nil {
        return err
    }
    
    // 3. 更新 current 符号链接
    // kubelet-client-current.pem -> kubelet-client-2024-01-15.pem
    return s.updateSymlink(targetFile)
}
```

---

## kubelet 证书路径

```
/var/lib/kubelet/pki/
├── kubelet-client-current.pem      # 符号链接 -> 当前客户端证书
├── kubelet-client-2024-01-01.pem   # 历史客户端证书 (轮换后保留)
├── kubelet-client-2024-06-01.pem   # 当前客户端证书
├── kubelet-server-current.pem      # 符号链接 -> 当前服务端证书
└── kubelet-server-2024-06-01.pem   # 当前服务端证书
```

---

## kubelet 服务端证书

服务端证书用于 kubelet 的 10250 端口（HTTPS metrics/logs/exec）：

```go
// kubelet 启动参数
--cert-dir=/var/lib/kubelet/pki
--rotate-certificates=true
--rotate-server-certificates=true
```

**服务端 CSR 的 signerName**：
```
kubernetes.io/kubelet-serving
```

**注意**：
- kubelet 服务端证书默认 **不自动审批**
- 需要手动批准或配置自动审批控制器：

```bash
# 手动批准服务端 CSR
kubectl get csr
kubectl certificate approve <csr-name>
```

---

## 生产环境配置建议

### 启用自动轮换

```yaml
# /var/lib/kubelet/config.yaml
rotateCertificates: true
serverTLSBootstrap: true
```

### 配置 CSR 自动审批（评估安全风险后）

```yaml
# 自动批准 kubelet 服务端证书
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: auto-approve-kubelet-server-certs
roleRef:
  kind: ClusterRole
  name: system:certificates.k8s.io:certificatesigningrequests:selfnodeclient
  apiGroup: rbac.authorization.k8s.io
subjects:
- kind: Group
  name: system:nodes
  apiGroup: rbac.authorization.k8s.io
```

**SignerName 对照表**：

| 证书类型 | SignerName | 自动审批 |
|---------|-----------|---------|
| kubelet 客户端证书 | `kubernetes.io/kube-apiserver-client-kubelet` | 是 (kubeadm 默认配置) |
| kubelet 服务端证书 | `kubernetes.io/kubelet-serving` | **否** (需手动或额外配置) |
| 普通客户端证书 | `kubernetes.io/kube-apiserver-client` | 否 |
| Legacy Unknown | `kubernetes.io/legacy-unknown` | 否 |

---

## 调试命令

```bash
# 查看 kubelet 当前证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -text

# 查看 kubelet 证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -enddate

# 查看待处理的 CSR
kubectl get csr

# 查看 CSR 详情
kubectl describe csr <csr-name>

# 查看 kubelet 证书管理日志
journalctl -u kubelet | grep -i "certificate\|csr\|rotation"

# 检查 kubelet 轮换配置
ps aux | grep kubelet | grep -E "rotate-certificates|rotate-server-certificates"
```

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/nodes.md|nodes]]
