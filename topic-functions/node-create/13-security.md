# 节点安全

## 源码路径

`pkg/kubelet/server/`
`pkg/kube-apiserver/authorizer/`

---

## Node Authorization

```bash
# API Server 启动参数
--authorization-mode=Node,RBAC

# Node Authorizer 允许 kubelet:
# - 读写自己的 Node 对象
# - 读写自己节点上的 Pod
# - 读写自己节点上的 Secret (通过 Pod 的 ServiceAccount)
# - 读写自己节点上的 ConfigMap
```

---

## NodeRestriction Admission

```go
// API Server 启动参数
--enable-admission-plugins=NodeRestriction

// NodeRestriction 限制 kubelet:
```

---

## kubelet 安全配置

```yaml
# /var/lib/kubelet/config.yaml
authentication:
  anonymous:
    enabled: false      # 禁止匿名访问
  webhook:
    enabled: true       # 通过 API Server 认证
    cacheTTL: 2h0m0s

authorization:
  mode: Webhook        # 通过 API Server 授权
```

---

## Pod 安全

```yaml
# PodSecurity (1.22+ 替代 PSP)
apiVersion: v1
kind: Namespace
metadata:
  name: demo
  labels:
    pod-security.kubernetes.io/enforce: privileged
    # privileged / baseline / restricted
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| kubelet 认证失败 | 证书过期 | 续期证书 |
| 未授权访问 | RBAC 配置错误 | 配置正确的 RoleBinding |
