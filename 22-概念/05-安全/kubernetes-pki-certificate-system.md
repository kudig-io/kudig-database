---
title: Kubernetes PKI 证书体系
description: '## 概述'
summary: 'Kubernetes 集群的认证与授权体系高度依赖 PKI（公钥基础设施）。一个标准的 kubeadm 部署包含超过 14 组证书/密钥对，涵盖所有控制面组件的身份认证。整个 PKI 体系由三组独立的 CA 构成，每组 CA 服务于不同的安全域。'
category: concepts
tags:
- k8s
- pki
- certificate
- ca
- tls
- kubeadm
- x509
- csr
- etcd
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
- Kubernetes PKI 证书体系 是什么
- 如何 Kubernetes PKI 证书体系
trigger_keywords:
- Kubernetes
- PKI
- 证书体系
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes PKI 证书体系

## 概述

Kubernetes 集群的认证与授权体系高度依赖 PKI（公钥基础设施）。一个标准的 kubeadm 部署包含超过 14 组证书/密钥对，涵盖所有控制面组件的身份认证。整个 PKI 体系由三组独立的 CA 构成，每组 CA 服务于不同的安全域。

## 三组 CA 架构

### kubernetes-ca（集群根 CA）

作为整个 Kubernetes 控制面的信任根，签发以下证书：

| 证书 | 用途 |
|------|------|
| `apiserver.crt` | API Server 服务端证书 |
| `apiserver-kubelet-client.crt` | API Server 连接 [[kubelet|kubelet]] 的客户端证书 |
| `admin.conf` | 管理员 kubeconfig（嵌入 client certificate） |
| `controller-manager.conf` | Controller Manager kubeconfig |
| `scheduler.conf` | Scheduler kubeconfig |

### etcd-ca（etcd 独立 CA）

etcd 作为独立分布式存储，拥有自己的 PKI 体系：

| 证书 | 用途 |
|------|------|
| `etcd/server.crt` | etcd 服务端证书 |
| `etcd/peer.crt` | etcd Peer 证书（集群间通信） |
| `etcd/healthcheck-client.crt` | etcd 健康检查客户端证书 |
| `apiserver-etcd-client.crt` | API Server 连接 etcd 的客户端证书 |

### front-proxy-ca（API 聚合层 CA）

隔离 API 聚合层（Aggregation Layer）的信任链：

| 证书 | 用途 |
|------|------|
| `front-proxy-client.crt` | API 聚合层客户端证书（用于 metrics-server 等） |

### ServiceAccount 密钥对

| 密钥 | 用途 |
|------|------|
| `sa.key` | Controller Manager 签名 ServiceAccount Token |
| `sa.pub` | API Server 验证 ServiceAccount Token |

## 证书路径与有效期

```
/etc/kubernetes/pki/
├── ca.crt / ca.key              # Kubernetes CA，默认 10 年
├── apiserver.crt / .key          # API Server 服务端证书，默认 1 年
├── apiserver-kubelet-client.crt  # API Server -> kubelet，默认 1 年
├── apiserver-etcd-client.crt     # API Server -> etcd，默认 1 年
├── front-proxy-ca.crt / .key     # Front Proxy CA，默认 10 年
├── front-proxy-client.crt / .key # Front Proxy 客户端，默认 1 年
├── sa.pub / sa.key               # ServiceAccount 密钥对
└── etcd/
    ├── ca.crt / ca.key           # etcd CA，默认 10 年
    ├── server.crt / .key         # etcd 服务端，默认 1 年
    ├── peer.crt / .key           # etcd Peer，默认 1 年
    └── healthcheck-client.crt/.key # 健康检查，默认 1 年
```

## 证书生成流程

`kubeadm init` 的 `certs` 阶段按以下顺序生成证书：

1. 生成三组自签名 CA（kubernetes-ca、etcd-ca、front-proxy-ca）
2. 使用对应 CA 签发终端实体证书
3. 生成 ServiceAccount 密钥对（RSA，非证书）
4. 所有证书写入 `/etc/kubernetes/pki/`

### CA 创建核心逻辑

```
NewCertificateAuthority:
  1. 生成 RSA 2048 位私钥（或 ECDSA P-256）
  2. 构造 X.509 证书模板:
     - Subject: CN=kubernetes, O=kubernetes
     - KeyUsage: DigitalSignature, KeyEncipherment, CertSign
     - BasicConstraints: CA=true
     - Validity: 10 年
  3. 使用私钥自签名
  4. 返回证书和私钥
```

## 三组 CA 独立性的设计意图

| CA | 设计意图 |
|----|---------|
| **kubernetes-ca** | 整个控制面的信任根，与 etcd CA 分离允许独立轮换 |
| **etcd-ca** | etcd 拥有独立 PKI，支持外部 etcd 集群场景 |
| **front-proxy-ca** | 隔离聚合层信任链，避免聚合层证书问题影响核心控制面 |

## 信任链验证

```
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl → 使用 ca.crt 验证 API Server 证书 → 建立 TLS 连接
       → 使用客户端证书（admin.conf）认证 → API Server 验证

API Server → 使用 etcd/ca.crt 验证 etcd 证书 → 建立 TLS 连接
          → 使用 etcd 客户端证书认证 → etcd 验证

API Server → 使用 front-proxy-ca 验证前端代理请求
          → 使用 kubernetes-ca 验证 kubelet 客户端证书
```
## kubelet 证书（CSR 动态签发）

kubelet 客户端证书不由 kubeadm 直接生成，而是通过 CSR 机制动态签发：

1. 首次启动使用 Bootstrap Token 发起 CSR
2. CSR 被 csrapproving 控制器自动审批
3. 签发的证书写入 `/var/lib/kubelet/pki/`
4. kubelet 使用正式证书连接 API Server

## 证书轮换

控制面证书通过 `kubeadm certs renew` 手动轮换，kubelet 证书自动轮换：

| 轮换方式 | 适用证书 | 说明 |
|---------|---------|------|
| `kubeadm certs renew all` | 控制面证书 | 保持 CA 不变，重新签发终端证书 |
| kubelet 自动轮换 | kubelet 客户端证书 | 剩余有效期 < 80% 时触发 CSR |

### 证书过期应急恢复

如果证书已过期且集群不可用：

```bash
# 临时回拨系统时间到证书有效期内
sudo date -s "2025-01-14 08:00:00"

# 执行证书轮换
sudo kubeadm certs renew all
sudo kubeadm init phase kubeconfig all

# 恢复正确时间
sudo ntpdate -u pool.ntp.org
```

## 外部 CA 模式

当 `/etc/kubernetes/pki/ca.crt` 存在但 `ca.key` 不存在时，kubeadm 进入外部 CA 模式，无法自行签发证书。适用于：

- 企业已有内部 PKI / AD CS
- CA 私钥不允许离开 HSM
- 需要安全团队审批后签发证书

## 安全最佳实践

- 私钥文件权限设为 600（`chmod 600 *.key`）
- 定期执行 `kubeadm certs check-expiration` 检查有效期
- 使用 Prometheus 监控证书过期：`apiserver_client_certificate_expiration_seconds`
- 维护窗口内主动轮换，不要等到过期
- 备份整个 PKI 目录：`tar czf k8s-pki-backup.tar.gz /etc/kubernetes/pki/`

## 相关概念

- [[26-技能/07-安全/certificate/kubelet-certificate-rotation.md|[[26-技能/07-安全/certificate/kubelet-certificate-rotation|kubelet 证书轮换机制]]]]
- [[22-概念/05-安全/security-defense-depth.md|安全纵深防御]]
- [[22-概念/05-安全/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]]
- [[26-技能/02-控制面/etcd/backup-restore-etcd.md|备份和恢复 etcd]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle.md|[[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]]]

## Related

- [[22-概念/05-安全/secrets-management.md|secrets-management]] — [[secrets|Secrets]]ts Management|Secrets Management]]
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
