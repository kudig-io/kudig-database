---
title: Secret 管理 × 存储模型
description: '# Secret 管理 × 存储模型'
summary: '传统的 Secret 卷（tmpfs）在容器重启时保持，但节点重启时丢失。CSI 加密卷提供了持久化的加密存储，可以用于：'
category: synthesis
tags:
- k8s
- secrets
- storage
- encryption
- etcd
- csi
- kubelet
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Secret 管理 × 存储模型 是什么
- 如何 Secret 管理 × 存储模型
trigger_keywords:
- Secret
- 管理
- 存储模型
prerequisites:
- kubectl-basics
- etcd-basics
relationships:
- target: '[[实体/etcd.md]]'
  type: uses
- target: '[[实体/kubelet.md]]'
  type: uses
- target: '[[系统基础/知识字典/configuration/secrets.md]]'
  type: uses
- target: '[[实体/deployment.md]]'
  type: uses
- target: '[[系统基础/速查卡/k8s.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Secret 管理 × 存储模型


## 连接点

[[系统基础/知识字典/configuration/secrets.md|secrets]]-management]] 覆盖密钥的安全存储，[[概念/storage-model.md|storage model]] 覆盖 PV/PVC/StorageClass 的三层抽象。两者的交叉点是 **Secret 的物理存储路径**：[[系统基础/速查卡/k8s.md|K8s]] Secret 从 API Server 写入 [[实体/etcd.md|etcd]]，从 etcd 同步到 [[实体/kubelet.md|kubelet]]，从 kubelet 挂载为 tmpfs 到容器。这条路径上的每个存储层都有不同的安全特性和失效模式。

## 共现场景

- **etcd 静态加密**：Secret 在 etcd 中以加密形式存储（通过 EncryptionConfiguration）。但如果加密密钥泄露，所有 Secret 都可被解密——这是第一层防线
- **kubelet 缓存**：kubelet 将 Secret 缓存在节点本地内存中。节点被入侵时，攻击者可以从 kubelet 的内存中提取 Secret
- **tmpfs 挂载**：Secret 卷以 tmpfs 形式挂载到容器，不写入节点磁盘。但容器逃逸后，攻击者可以访问同一 Pod 的所有卷
- **CSI 加密卷**：某些 CSI 驱动支持加密卷（如 AWS EBS encryption），可以将 Secret 存储在加密卷中而非 tmpfs。这提供了额外的存储层加密，但增加了挂载延迟

## 交叉洞察

**核心洞察：Secret 存储是一个"多层瑞士奶酪"模型——每层都有不同的孔洞，只有多层叠加才能覆盖所有盲区。**

```
API Server → etcd（静态加密）→ kubelet 内存 → 节点 tmpfs → 容器内存
     [层1]      [层2]            [层3]          [层4]        [层5]
```

| 存储层 | 保护机制 | 失效模式 | 攻击场景 |
|--------|---------|---------|---------|
| **etcd** | AES-CBC/ASE-GCM 静态加密 | 加密密钥泄露 | 攻击者获取 etcd 备份 + 加密密钥 |
| **kubelet 缓存** | 节点级内存保护 | 节点被 root 入侵 | 攻击者读取 kubelet 进程内存 |
| **tmpfs 卷** | 不写入磁盘 | 容器逃逸 | 攻击者从容器内读取 /var/run/secrets |
| **容器内存** | 进程隔离 | 进程调试 | 攻击者通过 /proc/<pid>/environ 读取环境变量 Secret |

**每层失效后的降级策略：**
- etcd 加密失效 → kubelet 缓存成为唯一防线（节点级保护）
- 节点被入侵 → tmpfs 不写入磁盘（但内存中的 Secret 仍可读取）
- 容器逃逸 → 环境变量 Secret 仍暴露（因为环境变量不在卷中）

**CSI 加密卷作为 Secret 存储的替代方案：**
传统的 Secret 卷（tmpfs）在容器重启时保持，但节点重启时丢失。CSI 加密卷提供了持久化的加密存储，可以用于：
- 大型证书链（超过 1MB Secret 大小限制）
- 跨 Pod 共享的加密配置
- 需要审计日志的 Secret 访问（某些 CSI 驱动支持访问审计）

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **加密性能** | etcd 静态加密增加 5-10% 的写入延迟。在高频 Secret 更新场景（如 Vault 动态凭证），这种延迟可能累积为显著的性能问题 |
| **密钥管理递归** | etcd 加密需要加密密钥，这个密钥本身也是 Secret——它存储在哪里？KMS 集成（AWS KMS、GCP KMS）解决了这个问题，但引入了云厂商依赖 |
| **CSI 加密卷的可用性** | CSI 加密卷需要云厂商 KMS 支持，本地集群或裸金属环境无法使用。这导致混合云部署中 Secret 存储策略的不一致 |

## 开放问题

- **Secret 大小限制的突破**：K8s Secret 限制为 1MB（etcd 限制）。大型证书链或 CA 捆绑包需要拆分为多个 Secret。CSI 加密卷可以突破此限制，但缺乏标准化的集成模式
- **跨节点 Secret 同步的一致性**：当 Secret 更新后，各节点 kubelet 的同步时间是异步的。在滚动更新期间，同一 [[实体/deployment.md|Deployment]] 的不同 Pod 可能使用不同版本的 Secret。如何确保 Secret 更新的一致性？


## 相关

- [[概念/secrets-management.md|secrets-management]]
- [[概念/storage-model.md|storage-model]]
- [[实体/vault.md|vault]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## See Also

- [[概念/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]
- [[故障诊断/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]
- [[概念/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[归档/consolidation/consolidation-2026-05-21.md|consolidation-2026-05-21]]


<!-- risk-assessed -->
