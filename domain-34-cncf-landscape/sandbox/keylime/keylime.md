---
title: Keylime
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Keylime 是什么
- 如何 Keylime
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Keylime
- cncf
- landscape
---

# Keylime

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://keylime.dev/ |
| **GitHub** | https://github.com/keylime/keylime |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust, Python |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Keylime 是一个基于 TPM (Trusted Platform Module) 的远程引导完整性验证和运行时完整性监控系统。它利用硬件 TPM 芯片提供加密度量，持续验证节点的引导过程和运行时状态是否被篡改，适用于零信任安全架构中的节点信任验证。

### 核心特性

- **TPM 远程证明**: 利用 TPM 2.0 验证节点引导完整性
- **运行时 IMA 监控**: 基于 Linux IMA (Integrity Measurement Architecture) 的运行时文件完整性监控
- **密钥分发**: 验证通过后安全分发密钥和配置
- **持续验证**: 周期性重新验证节点状态，检测运行时篡改
- **Revocation 机制**: 检测到异常时自动触发撤销动作
- **Kubernetes 集成**: Keylime Operator 实现 K8s 节点信任验证

---

## 架构设计

```
┌────────────────────────────────────────────┐
│           Keylime Verifier                  │
│  (Continuously verifies agent attestation)  │
└──────────────┬─────────────────────────────┘
               │
┌──────────────┴─────────────────────────────┐
│           Keylime Registrar                 │
│  (Manages agent registration & EK certs)    │
└──────────────┬─────────────────────────────┘
               │
    ┌──────────┼──────────┐
    ▼          ▼          ▼
┌────────┐ ┌────────┐ ┌────────┐
│ Agent  │ │ Agent  │ │ Agent  │
│ + TPM  │ │ + TPM  │ │ + TPM  │
│ Node 1 │ │ Node 2 │ │ Node 3 │
└────────┘ └────────┘ └────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Keylime 服务端
pip install keylime

# 或使用容器
docker run -d --name keylime-verifier \
  -v /var/lib/keylime:/var/lib/keylime \
  quay.io/keylime/keylime_verifier:latest

# 安装 Keylime Agent（在被验证节点上）
# 需要 TPM 2.0 硬件
cargo install keylime_agent
# 或
pip install keylime-agent
```

### 注册和验证节点

```bash
# 在 Agent 节点启动 Agent
keylime_agent

# 在 Verifier 端添加 Agent 进行验证
keylime_tenant -v 127.0.0.1 \
  -t <agent-ip> \
  -u <agent-uuid> \
  --verify \
  --allowlist /path/to/allowlist.txt \
  --exclude /path/to/exclude.txt

# 查看验证状态
keylime_tenant -v 127.0.0.1 -t <agent-ip> -u <agent-uuid> --status
```

### IMA 策略

```
# allowlist.txt - 允许的文件 hash 列表
# hash algorithm : file hash : file path
sha256:abc123... /usr/bin/bash
sha256:def456... /usr/lib/systemd/systemd
sha256:789ghi... /etc/keylime.conf
```

---

## Kubernetes 集成

```yaml
# Keylime Operator 部署
apiVersion: attestation.keylime.dev/v1alpha1
kind: KeylimeAgent
metadata:
  name: node-attestation
spec:
  verifierURL: "https://keylime-verifier:8881"
  registrarURL: "https://keylime-registrar:8891"
  nodeSelector:
    tpm: "true"
  ima:
    enabled: true
    allowlist: keylime-allowlist
  revocation:
    enabled: true
    actions:
      - type: cordon  # 验证失败时 cordon 节点
```

---

## 最佳实践

1. **TPM 确认**: 确保节点配备 TPM 2.0 芯片并在 BIOS 中启用
2. **IMA 启用**: 配置 Linux IMA 策略实现运行时完整性监控
3. **允许列表**: 维护准确的文件 hash 允许列表，定期更新
4. **撤销操作**: 配置验证失败时的自动响应（K8s cordon/drain）
5. **密钥管理**: 利用 Keylime 的安全密钥分发替代手动密钥部署

---

## 参考资源

- [Keylime 官方文档](https://keylime.dev/docs/)
- [Keylime GitHub](https://github.com/keylime/keylime)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
