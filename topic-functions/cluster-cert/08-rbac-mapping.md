# 证书身份到 RBAC 的映射关系

## 概述

Kubernetes 不维护独立的"用户数据库"，而是通过 TLS 客户端证书中的 **Subject** 字段来标识调用者身份。API Server 从证书中提取 `CommonName`（用户名）和 `Organization`（用户组），再交由 RBAC 授权系统进行权限判定。理解这一映射机制是排查认证与授权问题的关键。

---

## 源码路径

- **X509 认证插件**: `staging/src/k8s.io/apiserver/pkg/authentication/request/x509/x509.go`
- **RBAC 鉴权**: `plugin/pkg/auth/authorizer/rbac/`
- **kubeadm 证书配置**: `cmd/kubeadm/app/phases/certs/certs.go`

---

## API Server 的 X509 认证流程

### 1. 认证插件提取用户身份

```go
// staging/src/k8s.io/apiserver/pkg/authentication/request/x509/x509.go
func (a *Authenticator) AuthenticateRequest(req *http.Request) (*authenticator.Response, bool, error) {
    // 1. 从 TLS 连接中提取客户端证书
    if req.TLS == nil || len(req.TLS.PeerCertificates) == 0 {
        return nil, false, nil
    }
    clientCertificate := req.TLS.PeerCertificates[0]
    
    // 2. 使用 --client-ca-file 验证证书链
    verifyingOptions := x509.VerifyOptions{
        Roots:     a.caBundle,
        KeyUsages: []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    }
    chains, err := clientCertificate.Verify(verifyingOptions)
    if err != nil {
        return nil, false, err
    }
    
    // 3. 提取用户名和用户组
    user := &user.DefaultInfo{
        Name:   clientCertificate.Subject.CommonName,
        Groups: clientCertificate.Subject.Organization,
    }
    
    // 4. 添加内置组
    user.Groups = append(user.Groups, "system:authenticated")
    
    return &authenticator.Response{User: user}, true, nil
}
```

**提取规则**：

| 证书字段 | Kubernetes 身份 | 说明 |
|---------|----------------|------|
| `Subject.CommonName` | `user.Name` | 用户名 |
| `Subject.Organization[]` | `user.Groups` | 用户所属组 |

---

## kubeadm 证书中的身份设计

### 1. 管理员证书 (admin.conf)

```go
// cmd/kubeadm/app/phases/certs/certs.go
var KubeadmCertAdmin = &KubeadmCert{
    Name:     "admin",
    Config: certutil.Config{
        CommonName:   "kubernetes-admin",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**映射结果**：
- 用户名：`kubernetes-admin`
- 用户组：`system:masters`、`system:authenticated`

**RBAC 意义**：
```yaml
# 集群内置的 ClusterRoleBinding
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-admin
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: system:masters        # ← admin 证书属于此组
```

`system:masters` 组被绑定到 `cluster-admin` ClusterRole，拥有集群的完全控制权限。**该绑定不可删除**，是 Kubernetes 的默认超级管理员机制。

### 2. Controller Manager 证书

```go
var KubeadmCertControllerManager = &KubeadmCert{
    Name:     "controller-manager",
    Config: certutil.Config{
        CommonName:   "system:kube-controller-manager",
        Organization: []string{"system:kube-controller-manager"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**映射结果**：
- 用户名：`system:kube-controller-manager`
- 用户组：`system:kube-controller-manager`、`system:authenticated`

**RBAC 设计**：
- `system:kube-controller-manager` 用户/组拥有内置的 RBAC 规则
- 用于 Node 生命周期管理、PV/PVC 绑定、EndpointSlice 等

### 3. Scheduler 证书

```go
var KubeadmCertScheduler = &KubeadmCert{
    Name:     "scheduler",
    Config: certutil.Config{
        CommonName:   "system:kube-scheduler",
        Organization: []string{"system:kube-scheduler"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

### 4. API Server -> kubelet 客户端证书

```go
var KubeadmCertApiserverKubeletClient = &KubeadmCert{
    Config: certutil.Config{
        CommonName:   "kube-apiserver-kubelet-client",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**关键设计**：
- API Server 使用此证书连接 kubelet 的 10250 端口
- 属于 `system:masters` 组，因此对 kubelet 拥有完全访问权限
- 这是 kubelet **只读端口关闭后**（v1.20+ 默认关闭 10255）的必要设计

### 5. kubelet Bootstrap 证书

```go
// kubelet 首次 CSR 获批后签发的证书
Subject: pkix.Name{
    CommonName:   "system:node:<nodename>",
    Organization: []string{"system:nodes"},
}
```

**映射结果**：
- 用户名：`system:node:node-1`
- 用户组：`system:nodes`、`system:authenticated`

**RBAC 规则**：
```yaml
# 内置 ClusterRoleBinding
subjects:
- kind: Group
  name: system:nodes
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: system:node          # 节点基本权限
```

---

## Front Proxy 的特殊映射

Front Proxy 体系使用 **RequestHeader 认证插件**，不同于 X509 直接认证：

```go
// API Server 启动参数
--requestheader-allowed-names=["front-proxy-client"]
--requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
--requestheader-username-headers=X-Remote-User
--requestheader-group-headers=X-Remote-Group
```

**工作流**：
```
客户端 ──► API Server ──► 扩展 API Server (metrics-server)
            │                  ▲
            │ 携带 X-Remote-User 头    │
            └──────────────────────────┘
            使用 front-proxy-client 证书
```

API Server 在连接扩展 API Server 时：
1. 使用 `front-proxy-client.crt` 进行 TLS 客户端认证
2. 将原始用户的身份信息放入 HTTP Header：`X-Remote-User`、`X-Remote-Group`
3. 扩展 API Server 使用 `front-proxy-ca.crt` 验证 API Server 身份
4. 扩展 API Server 信任 Header 中的用户信息（ impersonation 机制）

---

## 身份验证调试

```bash
# 1. 查看当前 kubectl 用户的证书身份
kubectl config view --raw -o jsonpath='{.users[?(@.name=="kubernetes-admin")].user.client-certificate-data}' | base64 -d | openssl x509 -noout -subject -issuer

# 输出:
# subject=CN = kubernetes-admin, O = system:masters
# issuer=CN = kubernetes-ca

# 2. 查看 API Server 中当前用户的身份
kubectl auth whoami

# 3. 模拟用户权限检查
kubectl auth can-i create pods --as=system:node:node-1
kubectl auth can-i '*' '*' --as-group=system:masters --as=kubernetes-admin

# 4. 查看证书的完整 Subject
openssl x509 -in /etc/kubernetes/pki/apiserver-kubelet-client.crt -noout -subject -ext subjectAltName

# 5. 查看 CSR 中请求的证书身份
kubectl get csr <csr-name> -o jsonpath='{.spec.username}'
kubectl get csr <csr-name> -o jsonpath='{.spec.groups}'
```

---

## 常见认证问题

| 问题 | 现象 | 根因 |
|-----|------|------|
| Organization 错误 | `User "xxx" cannot create resource` | 证书不属于预期的 RBAC 组 |
| CommonName 不匹配 | 无法通过节点鉴权 | kubelet 证书 CN 必须是 `system:node:<nodename>` |
| CA 不信任 | `unknown authority` | API Server 使用的 `--client-ca-file` 与证书签发 CA 不匹配 |
| 证书用途错误 | `certificate specifies incompatible key usage` | 服务端证书不能用于客户端认证，EKU 不匹配 |
| front-proxy 配置错误 | metrics-server 401 | `--requestheader-allowed-names` 未包含 front-proxy-client 的 CN |

---

## 自定义证书身份的设计原则

当手动签发证书或配置外部 CA 时：

1. **用户名**：使用 `CommonName`，建议格式 `system:<component>:<name>`
2. **用户组**：使用 `Organization`，对齐 RBAC 中已定义的 Group
3. **避免使用 `system:masters`**：非管理员不要放入此组，应创建细粒度的 RBAC
4. **节点证书 CN 严格约束**：必须是 `system:node:<nodename>`，否则 Node 鉴权器拒绝
