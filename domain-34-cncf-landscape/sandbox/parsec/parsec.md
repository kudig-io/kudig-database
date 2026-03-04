# Parsec (Platform AbstRaction for SECurity)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://parallaxsecond.github.io/parsec-book/ |
| **GitHub** | https://github.com/parallaxsecond/parsec |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Parsec 是一个平台安全抽象层，为应用程序提供统一的 API 来访问底层硬件安全模块（HSM）、可信平台模块（TPM）和其他加密硬件。它通过 IPC 机制（Unix Domain Socket）对外提供统一的加密操作接口，屏蔽了不同安全硬件的差异，使应用无需关心底层使用的是 TPM 2.0、PKCS#11 HSM 还是 Arm TrustZone。

### 核心特性

- **统一 API**: 为所有安全硬件提供单一的加密操作接口
- **多后端支持**: TPM 2.0、PKCS#11、Mbed Crypto、CryptoAuthLib (Arm)、Trusted Services
- **多语言 SDK**: Rust、Go、C、Python、Java 客户端库
- **零信任身份**: 为每个应用提供基于硬件的身份认证
- **密钥管理**: 硬件绑定的密钥生成、存储和使用
- **最小权限**: 每个客户端只能访问自己的密钥

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│              应用层                                │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐    │
│  │ App A  │ │ App B  │ │ App C  │ │ App D  │    │
│  │(Rust)  │ │ (Go)   │ │(Python)│ │ (Java) │    │
│  └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘    │
│      │          │          │          │           │
│  ┌───▼──────────▼──────────▼──────────▼───┐      │
│  │        Parsec Client SDK               │      │
│  │        (统一加密 API)                   │      │
│  └───────────────┬────────────────────────┘      │
└──────────────────┼───────────────────────────────┘
                   │ IPC (Unix Socket)
┌──────────────────▼───────────────────────────────┐
│              Parsec Service (守护进程)              │
│                                                    │
│  ┌────────────────────────────────────────────┐   │
│  │           Core Engine                       │   │
│  │  身份认证 │ 密钥管理 │ 审计日志            │   │
│  └──────────────────┬─────────────────────────┘   │
│                     │                              │
│  ┌──────────────────▼─────────────────────────┐   │
│  │           Provider (后端适配器)              │   │
│  │  ┌──────┐ ┌────────┐ ┌──────────────────┐ │   │
│  │  │TPM   │ │PKCS#11 │ │Mbed Crypto       │ │   │
│  │  │2.0   │ │(HSM)   │ │(软件实现)         │ │   │
│  │  └──┬───┘ └───┬────┘ └────────┬─────────┘ │   │
│  └─────┼─────────┼───────────────┼────────────┘   │
└────────┼─────────┼───────────────┼─────────────────┘
    ┌────▼───┐ ┌───▼────┐ ┌───────▼────────┐
    │ TPM    │ │  HSM   │ │ Software Crypto│
    │ 芯片   │ │ 设备   │ │               │
    └────────┘ └────────┘ └────────────────┘
```

---

## 快速开始

### 安装 Parsec 服务

```bash
# 从源码构建
git clone https://github.com/parallaxsecond/parsec.git
cd parsec
cargo build --release --features "tpm-provider,pkcs11-provider"

# 或使用预编译二进制
curl -LO https://github.com/parallaxsecond/parsec/releases/latest/download/parsec
chmod +x parsec
```

### 配置

```toml
# /etc/parsec/config.toml
[core_settings]
log_level = "info"

[listener]
listener_type = "DomainSocket"
socket_path = "/run/parsec/parsec.sock"
timeout = 200

# TPM 2.0 后端
[[provider]]
provider_type = "Tpm"
key_info_manager_type = "OnDisk"
key_info_manager_path = "/var/lib/parsec/tpm-mappings"
tcti = "device:/dev/tpmrm0"
owner_hierarchy_auth = ""

# PKCS#11 HSM 后端
[[provider]]
provider_type = "Pkcs11"
key_info_manager_type = "OnDisk"
key_info_manager_path = "/var/lib/parsec/pkcs11-mappings"
library_path = "/usr/lib/softhsm/libsofthsm2.so"
slot_number = 0
user_pin = "1234"
```

### 客户端使用 (Rust)

```rust
use parsec_client::core::interface::requests::Opcode;
use parsec_client::BasicClient;

fn main() {
    let client = BasicClient::new(None).unwrap();

    // 生成 RSA 密钥对 (密钥存储在硬件中)
    client.psa_generate_key("my-signing-key", Default::default()).unwrap();

    // 使用硬件密钥签名
    let data = b"Hello, secure world!";
    let signature = client.psa_sign_hash(
        "my-signing-key",
        data,
        Default::default(),
    ).unwrap();

    // 验证签名
    let valid = client.psa_verify_hash(
        "my-signing-key",
        data,
        &signature,
        Default::default(),
    ).is_ok();
    println!("Signature valid: {}", valid);
}
```

### 客户端使用 (Go)

```go
package main

import "github.com/parallaxsecond/parsec-client-go/parsec"

func main() {
    client, _ := parsec.CreateConfiguredClient("my-app")

    // 生成密钥
    client.PsaGenerateKey("my-key", parsec.DefaultKeyAttributes())

    // 签名
    signature, _ := client.PsaSignHash("my-key", []byte("data"), parsec.DefaultSignAlgorithm())

    // 加密
    ciphertext, _ := client.PsaAsymmetricEncrypt("my-key", []byte("secret"), parsec.DefaultAsymEncryptAlgorithm())
}
```

---

## 与其他方案对比

| 特性 | Parsec | PKCS#11 直接调用 | TPM2-TSS | HashiCorp Vault |
|:---|:---|:---|:---|:---|
| 硬件抽象 | 统一 API | 单一标准 | 单一标准 | 软件 KMS |
| 多后端 | TPM/HSM/软件 | HSM 仅 | TPM 仅 | 软件 |
| 语言支持 | 多语言 SDK | C 为主 | C 为主 | REST API |
| 部署方式 | 系统守护进程 | 库链接 | 库链接 | 服务端 |
| 密钥隔离 | 每应用隔离 | 依赖配置 | 全局 | 策略控制 |

---

## 最佳实践

1. **硬件优先**: 生产环境优先使用 TPM 或 HSM 后端，开发环境可用 Mbed Crypto
2. **密钥命名**: 使用有意义的密钥名称，包含应用和用途信息
3. **权限控制**: 通过 Unix Socket 权限和 SELinux 策略控制 Parsec 访问
4. **备份策略**: HSM 后端的密钥需要配合 HSM 自身的备份机制
5. **监控**: 监控 Parsec 服务的可用性和操作延迟

---

## 参考资源

- [Parsec 官方文档](https://parallaxsecond.github.io/parsec-book/)
- [Parsec GitHub](https://github.com/parallaxsecond/parsec)
- [Parsec Client SDKs](https://github.com/parallaxsecond)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
