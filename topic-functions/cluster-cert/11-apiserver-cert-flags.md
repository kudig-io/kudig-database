# API Server 证书相关启动参数汇总

本文档汇总 kube-apiserver 所有与证书、TLS、认证相关的启动参数，作为生产环境配置和故障排查的速查手册。

---

## TLS 服务端证书

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--tls-cert-file` | `/etc/kubernetes/pki/apiserver.crt` | API Server 服务端证书 |
| `--tls-private-key-file` | `/etc/kubernetes/pki/apiserver.key` | API Server 服务端私钥 |
| `--tls-cipher-suites` | 系统默认 | TLS 密码套件列表 |
| `--tls-min-version` | `VersionTLS12` | 最低 TLS 版本 |

---

## 客户端 CA 验证（X509 认证）

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--client-ca-file` | `/etc/kubernetes/pki/ca.crt` | 验证客户端证书的 CA |
| `--anonymous-auth` | `true` | 允许匿名请求 |
| `--enable-bootstrap-token-auth` | `true` (kubeadm) | 启用 Bootstrap Token 认证 |

**验证逻辑**：
- 所有客户端证书必须由 `--client-ca-file` 指定的 CA 签发
- 从证书提取 `CN` 作为用户名，`O` 作为用户组

---

## Front Proxy（聚合层）

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--proxy-client-cert-file` | `/etc/kubernetes/pki/front-proxy-client.crt` | 连接扩展 API Server 的客户端证书 |
| `--proxy-client-key-file` | `/etc/kubernetes/pki/front-proxy-client.key` | 连接扩展 API Server 的客户端私钥 |
| `--requestheader-client-ca-file` | `/etc/kubernetes/pki/front-proxy-ca.crt` | 验证代理客户端的 CA |
| `--requestheader-allowed-names` | `front-proxy-client` | 允许的代理客户端 CN 白名单 |
| `--requestheader-username-headers` | `X-Remote-User` | 用户名 Header |
| `--requestheader-group-headers` | `X-Remote-Group` | 用户组 Header |
| `--requestheader-extra-headers-prefix` | `X-Remote-Extra-` | 额外属性 Header 前缀 |
| `--enable-aggregator-routing` | `false` | 直接路由到扩展 API Server Endpoint |

---

## etcd TLS 配置

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--etcd-cafile` | `/etc/kubernetes/pki/etcd/ca.crt` | 验证 etcd 服务端证书的 CA |
| `--etcd-certfile` | `/etc/kubernetes/pki/apiserver-etcd-client.crt` | 连接 etcd 的客户端证书 |
| `--etcd-keyfile` | `/etc/kubernetes/pki/apiserver-etcd-client.key` | 连接 etcd 的客户端私钥 |
| `--etcd-servers` | - | etcd 集群地址列表 |

---

## ServiceAccount Token 验证

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--service-account-key-file` | `/etc/kubernetes/pki/sa.pub` | 验证 SA JWT Token 的公钥 |
| `--service-account-issuer` | - | SA Token 的 issuer URL (v1.20+) |
| `--service-account-jwks-uri` | - | JWKS 公钥集地址 (v1.20+) |
| `--service-account-signing-key-file` | - | 签名 SA Token 的私钥 (如 API Server 直接签发) |

---

## kubelet 证书相关

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--kubelet-certificate-authority` | - | 验证 kubelet 服务端证书的 CA |
| `--kubelet-client-certificate` | `/etc/kubernetes/pki/apiserver-kubelet-client.crt` | 连接 kubelet 的客户端证书 |
| `--kubelet-client-key` | `/etc/kubernetes/pki/apiserver-kubelet-client.key` | 连接 kubelet 的客户端私钥 |

---

## 准入控制 Webhook 证书

| 参数 | 默认值 | 说明 |
|-----|-------|------|
| `--admission-control-config-file` | - | 准入控制器配置文件 |

Webhook 服务端证书验证通过 Webhook 配置中的 `caBundle` 字段指定，不通过 API Server 启动参数。

---

## 参数验证检查清单

```bash
#!/bin/bash
# API Server 证书配置检查脚本

echo "=== API Server Certificate Flags ==="

# 获取 API Server 进程参数
ARGS=$(ps aux | grep kube-apiserver | grep -v grep | sed 's/.*kube-apiserver //')

# 检查关键证书文件存在
check_file() {
    local flag=$1
    local path=$(echo "$ARGS" | grep -oP "$flag=\K[^ ]+")
    if [ -z "$path" ]; then
        echo "[MISSING] $flag not set"
    elif [ -f "$path" ]; then
        echo "[OK] $flag -> $path"
        # 检查过期时间
        if [[ "$path" == *.crt ]]; then
            expiry=$(openssl x509 -in "$path" -noout -enddate 2>/dev/null | cut -d= -f2)
            echo "       Expires: $expiry"
        fi
    else
        echo "[ERROR] $flag -> $path (file not found)"
    fi
}

check_file "--tls-cert-file"
check_file "--tls-private-key-file"
check_file "--client-ca-file"
check_file "--etcd-cafile"
check_file "--etcd-certfile"
check_file "--etcd-keyfile"
check_file "--proxy-client-cert-file"
check_file "--proxy-client-key-file"
check_file "--requestheader-client-ca-file"
check_file "--service-account-key-file"
check_file "--kubelet-client-certificate"
check_file "--kubelet-client-key"

echo ""
echo "=== Front Proxy Allowed Names ==="
echo "$ARGS" | grep -oP "--requestheader-allowed-names=\K[^ ]+"

echo ""
echo "=== TLS Min Version ==="
echo "$ARGS" | grep -oP "--tls-min-version=\K[^ ]+"
```

---

## 多 CA 信任链验证关系

```
                    ┌──────────────────┐
                    │   API Server     │
                    │                  │
┌───────────────────┤  --client-ca     ├───────────────────┐
│                   │   -file          │                   │
│  kubectl/users    │  (kubernetes-ca) │                   │
│  使用 ca.crt      └──────────────────┘                   │
│  验证 API Server                                        │
│                                                         │
│                   ┌──────────────────┐                  │
│                   │   API Server     │                  │
│                   │                  │                  │
└───────────────────┤  --requestheader │                  │
                    │   -client-ca-file│                  │
                    │  (front-proxy-ca)│◄─────────────────┤
                    └──────────────────┘                  │
                         │                                │
                         │                                │
                    ┌────┴────┐                           │
                    │metrics- │                           │
                    │ server  │                           │
                    └─────────┘                           │
                                                          │
                   ┌──────────────────┐                   │
                   │   API Server     │                   │
                   │                  │                   │
                   │  --etcd-cafile   │                   │
                   │  (etcd-ca)       │◄──────────────────┘
                   └──────────────────┘
                         │
                   ┌─────┴─────┐
                   │   etcd    │
                   │  cluster  │
                   └───────────┘
```

---

## 常见配置错误

| 错误配置 | 现象 | 修复 |
|---------|------|------|
| `--client-ca-file` 指向错误的 CA | 所有 kubectl 命令返回 `Unauthorized` | 确认指向 `kubernetes-ca` |
| `--etcd-cafile` 指向 `ca.crt` | API Server 无法连接 etcd | 应指向 `etcd/ca.crt` |
| `--proxy-client-cert-file` 的 CN 不在白名单 | `subject with CN=xxx is not in the allowed list` | 添加 CN 到 `--requestheader-allowed-names` |
| `--service-account-key-file` 与 Controller Manager 不匹配 | Pod 无法访问 API | 确保使用同一密钥对 |
| `--tls-cert-file` SAN 缺失 | 部分节点/外部访问 TLS 失败 | 更新 certSANs 重新生成证书 |
