---
title: Certificates（PKI 证书与要求）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- rbac
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Certificates（PKI 证书与要求） 是什么
- 如何 Certificates（PKI 证书与要求）
trigger_keywords:
- Certificates
- PKI
- 证书与要求
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Certificates（PKI 证书与要求）

## 概述

[[Kubernetes|Kubernetes]] 集群的所有组件之间都通过 **TLS（传输层安全协议）** 进行通信，因此需要一套完整的 **PKI（公钥基础设施）证书体系** 来完成双向身份验证与加密传输。如果你使用 `kubeadm` 安装集群，这些证书会自动生成；但在手动部署或需要更高安全性的场景下，运维人员需要自行创建并管理证书。

> **注意**：原概念页面 `/docs/concepts/cluster-administration/certificates/` 已迁移，当前内容主要基于官方最佳实践文档 `/docs/setup/best-practices/certificates/` 进行总结。

## 核心概念/原理

### 1. 服务端证书（Server Certificates）

用于向客户端证明服务器身份：

- **API Server**：集群入口点，所有 `kubectl` 和组件都通过其 REST API 访问。
- **[[系统基础/知识字典/fundamentals/etcd.md|etcd]] Server**：Kubernetes 的后端数据库，需要 TLS 保护键值存储访问。
- **[[kubelet|Kubelet]]**：每个节点上的代理，提供 Pod/容器管理接口。
- **Front-Proxy（可选）**：用于 API Server 的聚合层（Aggregation Layer），[[系统基础/知识字典/fundamentals/the-kubernetes-api.md|扩展 Kubernetes API]]PI|Kubernetes API]]。

### 2. 客户端证书（Client Certificates）

用于客户端向服务端证明自己的身份：

- **Kubelet 客户端证书**：每个 kubelet 用此证书向 API Server 认证。
- **API Server 的 etcd 客户端证书**：API Server 访问 etcd 时使用。
- **Controller Manager / Scheduler 客户端证书**：与控制平面安全通信。
- **kube-proxy 客户端证书**：每个节点上的网络代理向 API Server 认证。
- **管理员客户端证书（可选）**：集群运维人员通过 `kubectl` 访问集群。

### 3. Kubelet 的服务端与客户端证书

API Server 与 Kubelet 通信时，可采用两种模式：

- **共享证书**：复用 API Server 已有的服务端证书（`apiserver.crt/key`）作为 kubelet 的客户端证书。
- **独立证书**：生成专门的 `kubelet-client.crt/key`，与 API Server 主证书分离，提升安全性。

### 4. etcd 的 mTLS

etcd 集群内部节点之间以及对客户端（主要是 API Server）都采用 **双向 TLS（mutual TLS）** 进行认证，防止未授权的数据访问。

## 关键机制或特性

### 证书存储路径

使用 `kubeadm` 时，默认证书存放在：

```
/etc/kubernetes/pki/
```

用户账户相关的 `kubeconfig` 文件则存放在：

```
/etc/kubernetes/
```

### 手动配置证书的两种模式

#### 模式 A：单根 CA（Single Root CA）

由管理员创建一个根 CA，然后创建多个中间 CA，后续证书生成可委托给 Kubernetes 自身。需要准备以下 CA：

| 路径 | 默认 CN | 用途 |
|------|---------|------|
| `ca.crt/key` | `kubernetes-ca` | Kubernetes 通用 CA |
| `etcd/ca.crt/key` | `etcd-ca` | etcd 相关功能 |
| `front-proxy-ca.crt/key` | `kubernetes-front-proxy-ca` | 前端代理（Aggregation Layer） |

此外，还需要服务账户密钥对：`sa.key` 和 `sa.pub`。

#### 模式 B：全部自行生成（All Certificates）

如果你不希望将 CA 私钥复制到集群节点上，可以自行生成所有终端实体证书。常见需求证书包括：

| 默认 CN | 签发 CA | 类型 | hosts (SAN) |
|---------|---------|------|-------------|
| `kube-etcd` | `etcd-ca` | server, client | 主机名、Host_IP、localhost、127.0.0.1 |
| `kube-etcd-peer` | `etcd-ca` | server, client | 同上 |
| `kube-etcd-healthcheck-client` | `etcd-ca` | client | - |
| `kube-apiserver-etcd-client` | `etcd-ca` | client | - |
| `kube-apiserver` | `kubernetes-ca` | server | 主机名、Host_IP、advertise_IP |
| `kube-apiserver-kubelet-client` | `kubernetes-ca` | client | - |
| `front-proxy-client` | `kubernetes-front-proxy-ca` | client | - |

### 用户账户证书

需要手动为以下组件/用户生成 `kubeconfig` 文件：

| 文件名 | 默认 CN | 所属组织 (O) |
|--------|---------|--------------|
| `admin.conf` | `kubernetes-admin` | `kubeadm:cluster-admins` 或 `system:masters` |
| `super-admin.conf` | `kubernetes-super-admin` | `system:masters` |
| `kubelet.conf` | `system:node:<nodeName>` | `system:nodes` |
| `controller-manager.conf` | `system:kube-controller-manager` | - |
| `scheduler.conf` | `system:kube-scheduler` | - |

> **注意**：`system:masters` 是超级用户组，可绕过 RBAC 授权层，应谨慎使用。

## 使用场景

1. **手动搭建集群**：不使用 kubeadm，而是二进制或自定义脚本部署时，需要完整了解证书需求。
2. **外部 CA 模式**：企业已有统一 CA 基础设施，希望复用现有根 CA，不将私钥暴露在 Kubernetes 节点上。
3. **高安全环境**：需要对证书进行轮换、审计，或将私钥存储在 HSM（硬件安全模块）中。
4. ** etcd 独立部署**：etcd 运行在独立机器上时，必须正确配置 peer 和 client 证书。

## 最佳实践/注意事项

- **最小权限原则**：为 `kube-apiserver-kubelet-client` 使用非 `system:masters` 的组（如 `kubeadm:cluster-admins`），避免过度授权。
- **正确配置 SANs**：为服务端证书添加所有可能的访问入口（主机名、IP、`kubernetes.default.svc.cluster.local` 等），否则客户端 TLS 校验会失败。
- **不要复制 CA 私钥**：如果可能，采用“外部 CA”模式，仅将 CA 公钥分发到节点，私钥保存在离线安全介质中。
- **定期轮换证书**：TLS 证书应设置合理的过期时间（如 1 年），并建立自动轮换机制。
- **使用标准工具**：官方文档推荐使用 `easyrsa`、`openssl` 或 `cfssl` 生成证书；生产环境可结合 `[[cert-manager|cert-manager]]` 或 `certificates.k8s.io` API 实现自动化管理。
- **备份 `sa.key`**：服务账户私钥一旦丢失，所有由它签发的 ServiceAccount Token 都将失效，务必做好备份。

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| API Server 启动失败，TLS 错误 | 证书过期或 SAN 不匹配 | `openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates -text` | 重新签发包含正确 SAN 的证书 |
| kubelet 无法注册到 API Server | 客户端证书无效或 CN/O 不匹配 | `openssl verify -CAfile ca.crt kubelet-client.crt` | 检查 CN 为 `system:node:<nodeName>`，O 为 `system:nodes` |
| etcd 集群不健康 | peer 证书不被信任 | `etcdctl endpoint health --cluster` | 确认 etcd CA 签发了所有 peer 和 client 证书 |
| kubectl 报 x509 证书错误 | kubeconfig 中 CA 与集群不匹配 | `kubectl config view --raw` | 更新 kubeconfig 中的 `certificate-authority-data` |
| 证书即将过期 | 未配置自动轮换 | `kubeadm certs check-expiration` | 运行 `kubeadm certs renew all` 并重启控制平面组件 |
| front-proxy 认证失败 | front-proxy CA 未正确配置 | `openssl x509 -in front-proxy-ca.crt -noout -subject` | 确认 `--requestheader-client-ca-file` 指向 front-proxy CA |
| ServiceAccount Token 失效 | sa.key 丢失或不匹配 | `kubectl get sa -A` | 从备份恢复 `sa.key`/`sa.pub` 并重启 API Server |

## 生产检查清单

- [ ] 所有 CA 私钥不存储在集群节点上（外部 CA 模式）
- [ ] 服务端证书包含所有必要的 SAN（IP + DNS）
- [ ] etcd 启用 mTLS（双向 TLS）
- [ ] 证书有效期不超过 1 年，配置自动轮换
- [ ] `kubeadm certs check-expiration` 纳入定期巡检
- [ ] `sa.key` / `sa.pub` 已安全备份
- [ ] 管理员账户使用独立证书，不复用 `system:masters` 组
- [ ] 证书轮换后所有控制平面组件已重启确认
- [ ] 生产环境使用 cert-manager 或 certificates.k8s.io API 管理证书

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查证书过期时间
kubeadm certs check-expiration

# 轮换所有证书
kubeadm certs renew all

# 查看证书详情（SAN、有效期、签发者）
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text

# 验证证书链
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# etcd 快照备份（含 TLS 参数）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 手动生成 CSR
openssl req -new -key server.key -out server.csr -subj "/CN=kube-apiserver"

# 使用 cfssl 签发证书
cfssl gencert -ca=ca.pem -ca-key=ca-key.pem -config=ca-config.json server-csr.json | cfssljson -bare server
```
## 交叉引用

- [PKI Certificates and Requirements - Kubernetes Best Practices](https://kubernetes.io/docs/setup/best-practices/certificates/)
- [Generating Certificates Manually](https://kubernetes.io/docs/tasks/administer-cluster/certificates/)
- [Certificate Management with kubeadm](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-certs/)
- [Managing TLS in a Cluster](https://kubernetes.io/docs/tasks/tls/managing-tls-in-a-cluster/)
- 相关主题：[Secrets](../configuration/secrets.md) · [API Priority and Fairness](../platform-engineering/api-priority-and-fairness.md)

## 参考链接

- [Certificates]()

## Related

- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]


<!-- risk-assessed -->
