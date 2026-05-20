---
title: Inclavare Containers
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Inclavare Containers 是什么
- 如何 Inclavare Containers
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Inclavare
- Containers
- cncf
- landscape
---

# Inclavare Containers

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://inclavare-containers.io/ |
| **GitHub** | https://github.com/inclavare-containers/inclavare-containers |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, Rust, C |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Inclavare Containers 是一个基于硬件可信执行环境 (TEE) 的机密容器项目。它利用 Intel SGX、ARM TrustZone 等硬件安全技术，在隔离的 Enclave 中运行容器工作负载，保护数据和代码的机密性和完整性。即使宿主机操作系统或 Hypervisor 被攻破，Enclave 内的数据也不会泄露。Inclavare Containers 兼容 OCI 标准，可与 Kubernetes 无缝集成。

### 核心特性

- **硬件级机密计算**: 基于 Intel SGX、AMD SEV、ARM TrustZone 等 TEE 技术
- **OCI 兼容**: 完全兼容 OCI 运行时标准，可替换 runc 使用
- **Kubernetes 集成**: 通过 Containerd 和 CRI 无缝集成 Kubernetes
- **远程证明**: 支持远程证明 (Remote Attestation)，验证 Enclave 的可信性
- **LibOS 支持**: 集成 Occlum、Gramine 等 LibOS，简化 Enclave 应用开发
- **加密内存**: 所有内存数据自动加密，防止物理攻击和内存窥探

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Kubernetes Cluster                   │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │              Containerd                        │    │
│  │  ┌─────────────────────────────────────────┐  │    │
│  │  │           Shim (rune-shim-v2)           │  │    │
│  │  └────────────────────┬────────────────────┘  │    │
│  └───────────────────────┼───────────────────────┘    │
│                          │                            │
│  ┌───────────────────────▼───────────────────────┐    │
│  │              Rune (OCI Runtime)                │    │
│  │  ┌────────────────────────────────────────┐   │    │
│  │  │        Enclave Runtime (LibOS)          │   │    │
│  │  │  ┌──────────┐  ┌──────────┐            │   │    │
│  │  │  │ Occlum   │  │ Gramine  │  ...       │   │    │
│  │  │  └──────────┘  └──────────┘            │   │    │
│  │  └────────────────────┬───────────────────┘   │    │
│  └───────────────────────┼───────────────────────┘    │
│                          │                            │
│  ┌───────────────────────▼───────────────────────┐    │
│  │         Hardware TEE (Intel SGX / AMD SEV)     │    │
│  │  ┌──────────────────────────────────────────┐ │    │
│  │  │              Enclave                      │ │    │
│  │  │  ┌──────────────────────────────────┐   │ │    │
│  │  │  │   Application (加密内存运行)      │   │ │    │
│  │  │  │   + LibOS (系统调用模拟)          │   │ │    │
│  │  │  └──────────────────────────────────┘   │ │    │
│  │  │         Encrypted Memory (EPC)           │ │    │
│  │  └──────────────────────────────────────────┘ │    │
│  └───────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 环境要求

```bash
# 检查 SGX 支持
cpuid | grep -i sgx

# 安装 SGX 驱动和 SDK (Intel 平台)
# 参考: https://github.com/intel/linux-sgx
```

### 安装 Inclavare Containers

```bash
# 安装 rune (Enclave-aware OCI runtime)
wget https://github.com/inclavare-containers/inclavare-containers/releases/download/v0.7.0/rune-0.7.0-linux-amd64.tar.gz
tar -xzf rune-0.7.0-linux-amd64.tar.gz
sudo mv rune /usr/local/bin/

# 安装 shim
wget https://github.com/inclavare-containers/inclavare-containers/releases/download/v0.7.0/containerd-shim-rune-v2-0.7.0-linux-amd64.tar.gz
tar -xzf containerd-shim-rune-v2-0.7.0-linux-amd64.tar.gz
sudo mv containerd-shim-rune-v2 /usr/local/bin/
```

### 配置 Containerd

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.rune]
  runtime_type = "io.containerd.rune.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.rune.options]
    BinaryName = "/usr/local/bin/rune"
```

### 构建 Enclave 应用 (使用 Occlum)

```bash
# 安装 Occlum
curl -fsSL https://occlum.io/install.sh | sudo bash

# 初始化 Occlum 实例
mkdir my-enclave-app && cd my-enclave-app
occlum init

# 复制应用到 Occlum 镜像
cp /path/to/my-app image/bin/
occlum build

# 运行 Enclave 应用
occlum run /bin/my-app
```

### 创建机密容器镜像

```dockerfile
# Dockerfile.enclave
FROM occlum/occlum:latest
COPY my-app /root/my-app
RUN occlum init && \
    cp /root/my-app image/bin/ && \
    occlum build
CMD ["occlum", "run", "/bin/my-app"]
```

### 在 Kubernetes 中运行

```yaml
# enclave-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: enclave-app
spec:
  runtimeClassName: rune
  containers:
    - name: app
      image: my-enclave-app:latest
      resources:
        limits:
          sgx.intel.com/epc: "10Mi"
          sgx.intel.com/enclave: "1"
```

---

## 高级功能

### 远程证明 (Remote Attestation)

```go
// 验证 Enclave 报告
import "github.com/inclavare-containers/rats-tls"

func VerifyEnclave(quote []byte) error {
    // 验证 SGX Quote
    report, err := rats.VerifyQuote(quote)
    if err != nil {
        return err
    }
    
    // 检查 MRENCLAVE (Enclave 身份)
    expectedMREnclave := "..."
    if report.MREnclave != expectedMREnclave {
        return errors.New("MRENCLAVE mismatch")
    }
    
    return nil
}
```

### RATS-TLS 安全通信

```yaml
# 配置 RATS-TLS 实现 Enclave 间安全通信
apiVersion: v1
kind: Pod
metadata:
  name: rats-tls-server
spec:
  runtimeClassName: rune
  containers:
    - name: server
      image: rats-tls-server:latest
      env:
        - name: RATS_TLS_MODE
          value: "mutual"
        - name: RATS_TLS_ATTESTER
          value: "sgx_ecdsa"
```

### 多种 LibOS 支持

```yaml
# 使用 Gramine (原 Graphene)
apiVersion: v1
kind: Pod
metadata:
  name: gramine-app
spec:
  runtimeClassName: rune
  containers:
    - name: app
      image: gramine-app:latest
      env:
        - name: ENCLAVE_RUNTIME
          value: "gramine"
```

---

## 与其他方案对比

| 特性 | Inclavare | Kata CC | Enarx | Veraison |
|:---|:---|:---|:---|:---|
| TEE 支持 | SGX/SEV/TZ | SEV/TDX | SGX/SEV/ARM | 通用证明 |
| OCI 兼容 | 完整 | 完整 | 有限 | N/A |
| LibOS | Occlum/Gramine | 无 | Enarx Keep | N/A |
| Kubernetes | 原生支持 | 原生支持 | 有限 | N/A |
| 远程证明 | RATS-TLS | 有限 | 内置 | 专注证明 |

---

## 最佳实践

1. **EPC 内存规划**: SGX EPC 内存有限（通常 128-256 MB），合理规划应用内存使用
2. **最小化 TCB**: 减少 Enclave 内的代码量，降低可信计算基（TCB）复杂度
3. **远程证明**: 生产环境中始终启用远程证明，验证 Enclave 的真实性
4. **密钥管理**: 使用远程证明后的安全通道获取密钥，不要硬编码密钥
5. **性能调优**: 减少 Enclave 进出（ECALL/OCALL）次数，降低上下文切换开销

---

## 参考资源

- [Inclavare Containers 官方文档](https://inclavare-containers.io/docs/)
- [Inclavare Containers GitHub](https://github.com/inclavare-containers/inclavare-containers)
- [Occlum LibOS](https://github.com/occlum/occlum)
- [Intel SGX 开发指南](https://software.intel.com/content/www/us/en/develop/topics/software-guard-extensions.html)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
