---
title: Parsec
description: '## 概述'
summary: 'Parsec 是一个平台安全抽象层，为应用程序提供统一的 API 来访问底层硬件安全模块（HSM）、可信平台模块（TPM）和其他加密硬件。它通过 IPC 机制（Unix Domain Socket）对外提供统一的加密操作接口，屏蔽了不同安全硬件的差异，使应用无需关心底层使用的是 TPM 2.0、PKCS#11 HSM 还是 Arm TrustZone。'
category: entities
tags:
- k8s
- cncf
- security
- parsec
- argocd
- ingress
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Parsec 是什么
- 如何 Parsec
trigger_keywords:
- Parsec
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Parsec

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust

## 概述

Parsec（Platform AbstRaction for SECurity）是由 Arm 公司发起、Cloud Native Computing Foundation 托管的开源平台安全抽象层项目，2020 年进入 CNCF Sandbox。它为应用程序提供**统一的 API** 来访问底层硬件安全模块——包括 HSM（硬件安全模块）、TPM（可信平台模块）、Arm TrustZone 和软件加密库（如 Mbed Crypto）。

Parsec 的核心价值在于**硬件抽象**。传统应用直接使用 PKCS#11 或 TPM 2.0 API 时，代码与特定硬件绑定，更换硬件需要修改应用。Parsec 通过 Unix Domain Socket 提供统一的加密操作接口（PSA Crypto API），应用只需对接 Parsec，底层硬件切换（从 TPM 到 HSM）对应用透明。Parsec 使用 Rust 编写，内存安全性优于 C 实现。

## Key Features

- **统一安全 API**：PSA Crypto API 标准接口，屏蔽底层硬件差异
- **多后端支持**：TPM 2.0、PKCS#11 HSM、Arm TrustZone（OP-TEE）、Mbed Crypto、CryptoAuthLib
- **密钥隔离**：每个应用/容器的密钥独立管理，通过身份验证隔离
- **Unix Socket IPC**：通过 Unix Domain Socket 提供服务，零网络开销
- **Rust 安全实现**：内存安全语言，避免 C 语言常见的缓冲区溢出
- **K8s/DaemonSet 部署**：可作为节点级 DaemonSet 在 K8s 中运行

## Architecture

Parsec 采用 **Client-Server 架构**。**Parsec Daemon**（parsec）运行在宿主机或容器中，监听 Unix Domain Socket（`/run/parsec/parsec.sock`）。**Parsec Client**（应用通过 libparsec_client 库）通过 socket 发送加密操作请求。Daemon 内部由 **Core Service**（路由请求到后端）、**Provider Manager**（管理多个硬件后端）和 **Authenticator**（验证客户端身份）组成。每个 Provider（TPM/PKCS#11/TrustZone）封装对应硬件的 SDK。

## K8s 集成

Parsec 以 **DaemonSet** 形式运行在每个节点上。容器中的 Pod 通过挂载 `/run/parsec/parsec.sock` 访问宿主机的 Parsec 服务。使用 Kubernetes ServiceAccount 或 SPIFFE SVID 作为身份验证来源。也支持通过 Device Plugin 暴露安全硬件资源（如 TPM）到 Pod。

## 生产部署要点

- **硬件优先**：生产环境优先使用 TPM 或 HSM 后端，开发环境可用 Mbed Crypto
- **密钥命名**：使用有意义的密钥名称，包含应用和用途信息
- **权限控制**：通过 Unix Socket 权限和 SELinux 策略控制 Parsec 访问
- **备份策略**：HSM 后端的密钥需要配合 HSM 自身的备份机制
- **监控**：监控 Parsec 服务的可用性和操作延迟

## 生产场景

1. **密钥安全存储**：应用的 TLS 私钥存储在 TPM/HSM 中，而非文件系统
2. **容器签名**：CI/CD 系统使用 HSM 中的密钥对镜像进行 Cosign 签名
3. **IoT 设备认证**：边缘设备的身份密钥存储在 TrustZone 中
4. **数据库加密**：数据库透明加密（TDE）的主密钥存储在 HSM 中

## 安装与配置

```bash
# Ubuntu/Debian 安装
sudo apt install parsec

# 或从源码构建（Rust）
cargo install parsec-tool

# 启动 Parsec
sudo systemctl start parsec
sudo systemctl status parsec
```

### 配置文件 (TPM 后端)

```toml
# /etc/parsec/config.toml
[core_settings]
listen_socket = "unix:/run/parsec/parsec.sock"
log_level = "info"

[provider]
[provider.tpm]
provider_type = "tpm"
tcti = "mssim:host=localhost,port=2321"

[provider.pkcs11]
provider_type = "pkcs11"
library_path = "/usr/lib/softhsm/libsofthsm2.so"
slot_number = 0
```

### K8s DaemonSet 部署

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: parsec
  namespace: parsec-system
spec:
  selector:
    matchLabels:
      app: parsec
  template:
    metadata:
      labels:
        app: parsec
    spec:
      hostNetwork: true
      containers:
      - name: parsec
        image: ghcr.io/parallaxsecond/parsec:latest
        securityContext:
          privileged: true
        volumeMounts:
        - name: parsec-socket
          mountPath: /run/parsec
        - name: tpm-device
          mountPath: /dev/tpm0
      volumes:
      - name: parsec-socket
        hostPath:
          path: /run/parsec
      - name: tpm-device
        hostPath:
          path: /dev/tpm0
```

```bash
# 使用 parsec-tool 测试
parsec-tool create-ecc-key --key-name mykey
parsec-tool sign --key-name mykey --algorithm ecdsa-sha256 "hello"
parsec-tool list-keys
```

## 运维操作

```bash
# 🟢 查看密钥列表
parsec-tool list-keys

# 🟢 查看服务状态
sudo systemctl status parsec

# 🟡 创建密钥
parsec-tool create-ecc-key --key-name app-signing-key

# 🟡 签名操作
parsec-tool sign --key-name app-signing-key --algorithm ecdsa-sha256 <data>

# 🔴 删除密钥（不可恢复）
parsec-tool destroy-key --key-name mykey
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 服务启动失败 | TPM 设备不可用 | `ls /dev/tpm*` | 检查 TPM 硬件/驱动 |
| 密钥创建失败 | 后端配置错误 | `journalctl -u parsec` | 检查 config.toml |
| Socket 连接失败 | 权限不足 | `ls -la /run/parsec/` | 调整 socket 权限 |
| K8s Pod CrashLoop | TPM 设备未挂载 | `kubectl describe pod` | 检查 hostPath 配置 |
| 签名验证失败 | 密钥不匹配 | `parsec-tool list-keys` | 确认使用正确密钥 |

```
排查流程:
├── 服务异常
│   ├── systemctl status parsec → 服务状态
│   ├── journalctl -u parsec → 详细日志
│   └── 检查 config.toml 配置
├── 硬件问题
│   ├── ls /dev/tpm* → 确认设备存在
│   ├── tpm2_getcap properties-fixed → TPM 状态
│   └── 检查内核模块加载
└── K8s 部署问题
    ├── kubectl logs parsec-pod → 容器日志
    ├── 确认 hostPath 挂载正确
    └── 检查 securityContext 权限
```

## 生产案例

### 案例 1: 容器镜像签名硬件根信任

- **场景**: 镜像签名密钥存储在软件中，存在泄露风险
- **方案**: 使用 Parsec + TPM 存储签名密钥；私钥永不离开硬件；通过 PSA API 进行签名操作
- **效果**: 密钥安全等级提升到硬件级，通过安全审计

### 案例 2: 边缘设备统一密钥管理

- **场景**: 1000+ 边缘设备需要唯一设备证书，密钥管理复杂
- **方案**: 每台设备部署 Parsec + TPM；设备证书私钥存储在 TPM 中；统一 PSA API 接口
- **效果**: 设备身份不可伪造，密钥管理自动化

## 对比

| 特性 | Parsec | PKCS#11 | TPM2-TSS | Vault Transit | 适用场景 |
|------|--------|---------|----------|--------------|----------|
| 硬件抽象 | ✅ 多后端 | ❌ HSM only | ❌ TPM only | ❌ 软件 | 统一接口 |
| API 统一性 | ✅ PSA API | ❌ C API | ❌ C API | ✅ REST | 开发效率 |
| K8s 部署 | ✅ DaemonSet | ⚠️ | ⚠️ | ✅ | 云原生 |
| 硬件安全 | ✅ TPM/HSM | ✅ HSM | ✅ TPM | ❌ | 安全等级 |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF | 生态 |

## 参考链接

- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[实体/vault.md|[[HashiCorp Vault|vault]]]]
- [[概念/secrets-management.md|secrets-management]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[实体/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[piraeus-datastore]] — Piraeus Datastore
- [[k8up]] — K8up
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- parsec
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
