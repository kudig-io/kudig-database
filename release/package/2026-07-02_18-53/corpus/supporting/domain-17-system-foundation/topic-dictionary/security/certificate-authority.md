---
title: 证书颁发机构
description: Certificate Authority（CA，证书颁发机构）是负责签发和管理数字证书的受信任实体。在 Kubernetes 中，CA
  是集群 PKI（Pub...
summary: Certificate Authority（CA，证书颁发机构）是负责签发和管理数字证书的受信任实体。在 Kubernetes 中，CA 是集群
  PKI（Pub...
category: dictionary
tags:
- k8s
- glossary
- security
- certificate
- tls
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 证书颁发机构 是什么
- Certificate Authority (CA) 详解
trigger_keywords:
- 证书颁发机构
- Certificate Authority (CA)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 证书颁发机构

> **英文名**: Certificate Authority (CA)

## 概述

Certificate Authority（CA，证书颁发机构）是负责签发和管理数字证书的受信任实体。在 Kubernetes 中，CA 是集群 PKI（Public Key Infrastructure）的根，所有组件间的 TLS 通信都依赖 CA 签发的证书。

## 核心概念/原理

### Kubernetes PKI 结构

Kubernetes 集群的证书层次：

```
Root CA (ca.crt / ca.key)
├── API Server Certificate (apiserver.crt / apiserver.key)
├── API Server Kubelet Client (apiserver-kubelet-client.crt / .key)
├── Front Proxy CA (front-proxy-ca.crt / .key)
│   └── Front Proxy Client (front-proxy-client.crt / .key)
├── etcd CA (etcd/ca.crt / etcd/ca.key)
│   ├── etcd Server (etcd/server.crt / .key)
│   ├── etcd Peer (etcd/peer.crt / .key)
│   └── etcd Healthcheck Client (etcd/healthcheck-client.crt / .key)
└── ServiceAccount Key (sa.key / sa.pub)
```

### CA 文件位置（kubeadm 集群）

所有证书默认位于 `/etc/kubernetes/pki/` 目录。

## 关键机制或特性

- Kubernetes 使用自签名的 Root CA（非公共 CA）。
- CA 证书的有效期默认 10 年。
- 组件间通信通过验证对方的证书是否由同一个 CA 签发来建立信任。
- `--client-ca-file` 和 `--tls-cert-file` 等参数配置 API Server 的证书。

## 使用场景与最佳实践

- 保护 CA 私钥的安全（限制文件权限，不提交到 Git）。
- 定期检查证书过期时间（`kubeadm certs check-expiration`）。
- 使用 cert-manager 自动化证书轮转。
- 实施证书过期监控告警。
- CA 轮转时需要所有组件同步更新证书。

## 参考链接

- [Certificate Authority (CA) - Official Documentation](https://kubernetes.io/docs/tasks/tls/managing-tls-in-a-cluster/)

## Related

- [[domain-17-system-foundation/知识字典/security/rbac.md|Rbac]]
- [[domain-17-system-foundation/知识字典/security/role.md|Role]]
- [[domain-17-system-foundation/知识字典/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/知识字典/security/rolebinding.md|Rolebinding]]
- [[domain-17-system-foundation/知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
