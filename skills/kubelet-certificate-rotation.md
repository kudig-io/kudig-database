---
title: kubelet 证书轮换机制
description: '## 概述'
summary: '## 概述'
category: skills
tags:
- k8s
- kubelet
- certificate-rotation
- csr
- tls-bootstrap
- auto-renew
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 证书轮换机制 是什么
- 如何 kubelet 证书轮换机制
trigger_keywords:
- kubelet
- 证书轮换机制
prerequisites:
- kubectl-basics
---



# [[kubelet|kubelet]] 证书轮换机制

## 概述

每个 kubelet 都需要持有有效的客户端证书才能与 API Server 进行安全通信。证书过期后 kubelet 将无法连接 API Server，导致节点上的 Pod 无法被管理。kubelet 通过 TLS Bootstrap 和自动轮换机制确保证书持续有效，自 Kubernetes v1.19 起默认启用。

## 证书管理架构

### 两个 kubeconfig 文件

| 文件 | 路径 | 用途 |
|------|------|------|
| `bootstrap-kubelet.conf` | `/etc/kubernetes/bootstrap-kubelet.conf` | 包含 Bootstrap Token，用于首次 CSR |
| `kubelet.conf` | `/etc/kubernetes/kubelet.conf` | 包含正式证书，用于正常通信 |

### 证书文件结构

```
/var/lib/kubelet/pki/
├── kubelet-client-2024-01-01-00-00-00.pem   # 签发的客户端证书
├── kubelet-client-2024-01-02-00-00-00.pem   # 轮换后的新证书
├── kubelet-client-current.pem               → 软链接到最新证书
├── kubelet.crt                              # 服务端证书
└── kubelet.key                              # 服务端私钥
```

## 完整工作流程

```
kubelet 启动
  │
  ├── 检查 kubelet.conf 是否存在
  │   ├── 存在 → 使用正式证书连接
  │   └── 不存在 → 进入 Bootstrap 流程
  │       ├── 读取 bootstrap-kubelet.conf
  │       ├── 使用 Bootstrap Token 发起 CSR
  │       ├── 等待 CSR 被 approve 和 sign
  │       └── 将证书写入 /var/lib/kubelet/pki/
  │
  └── 启动证书轮换协程
      ├── 定期检查证书有效期
      ├── 剩余有效期 < 80% 时触发轮换
      ├── 发起新的 CSR
      ├── 更新证书文件和软链接
      └── 热加载证书（无需重启）
```

## CSR 审批机制

Kubernetes 内置两个控制器处理 kubelet CSR：

| 控制器 | 职责 |
|--------|------|
| `csrapproving` | 自动审批 kubelet 发起的 CSR |
| `csrsigning` | 使用 CA 私钥签发证书 |

### 自动审批条件

csrapproving 控制器自动审批的条件：

1. CSR 的 SignerName 必须是 `kubernetes.io/kube-apiserver-client-kubelet`
2. CSR 的 Subject.Organization 必须包含 `system:nodes`
3. CSR 的 Subject.CommonName 必须以 `system:node:` 开头
4. 请求者必须是 `system:node:<name>` 或有 Node Bootstrap 权限

### 手动审批

当自动审批失败时：

```bash
kubectl get csr
kubectl describe csr <csr-name>
kubectl certificate approve <csr-name>
kubectl get csr <csr-name> -o jsonpath='{.status.certificate}' | base64 -d
```

## 配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
rotateCertificates: true    # 启用证书自动轮换（默认 true）
serverTLSBootstrap: true    # 启用服务端证书 Bootstrap
```

## Bootstrap Token 管理

Bootstrap Token 默认 24 小时后过期：

```bash
# 查看 Token
kubeadm token list

# 创建新 Token
kubeadm token create

# 生成完整 join 命令
kubeadm token create --print-join-command
```

## 证书过期恢复

当 kubelet 证书过期且 Bootstrap Token 也过期时：

```bash
# 1. 创建新 Token
kubeadm token create

# 2. 删除过期证书
rm -f /var/lib/kubelet/pki/kubelet-client-*.pem
rm -f /etc/kubernetes/kubelet.conf

# 3. 重新 join
kubeadm join --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  <api-server>:6443
```

## 调试命令

```bash
# 查看当前证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 查看证书身份
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -subject
# subject=O = system:nodes, CN = system:node:node-1

# 查看证书轮换日志
journalctl -u kubelet | grep -i "certificate|csr|rotation"
```

## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `certificate has expired` | kubelet 客户端证书过期 | 检查 `rotateCertificates` 配置 |
| CSR 一直是 Pending | csrapproving 控制器未运行 | 手动 `kubectl certificate approve` |
| `x509: certificate signed by unknown authority` | CA 证书不匹配 | 更新节点上的 CA 证书 |
| `Unauthorized` | 证书 CN/O 不正确 | 确保 CN=`system:node:<name>`, O=`system:nodes` |
| kubelet 无法发起 CSR | Bootstrap Token 过期 | 创建新 Token |

## 相关技能

- [[concepts/kubernetes-pki-certificate-system.md|[[Kubernetes PKI 证书体系|Kubernetes PKI 证书体系]]]]
- [[skills/kubeadm-cluster-lifecycle.md|[[kubeadm 集群创建生命周期|kubeadm 集群创建生命周期]]]]
- [[skills/node-drain-and-maintenance.md|[[节点驱逐与维护|节点驱逐与维护]]]]
- [[entities/kubelet.md|kubelet]]

## Related

- [[skills/troubleshoot-node-issues.md|troubleshoot-node-issues]] — Troubleshoot Node Issues
- [[entities/kubelet.md|kubelet]] — kubelet
- [[entities/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-pki-certificate-system.md|kubernetes-pki-certificate-system]] — Kubernetes PKI 证书体系

- [[skills/kubeadm-cluster-lifecycle.md|kubeadm-cluster-lifecycle]]
- [[skills/node-drain-and-maintenance.md|node-drain-and-maintenance]]