# The Update Framework (TUF)

> **成熟度**: Graduated | **加入时间**: 2017-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://theupdateframework.io |
| **GitHub** | https://github.com/theupdateframework/specification |
| **文档** | https://theupdateframework.io/overview |
| **许可证** | Apache-2.0/MIT |
| **主要语言** | Python, Go, Rust |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
TUF (The Update Framework) 是一个保护软件更新系统安全的框架规范。它通过角色分离、密钥管理、阈值签名等机制，确保软件分发过程免受各种攻击，即使部分基础设施被攻破也能保护最终用户。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2010 | 由纽约大学研究团队开发 |
| 2017-10 | 成为 CNCF 首批 Sandbox 项目 |
| 2019-12 | 晋升为 CNCF Incubating |
| 2021-06 | 晋升为 CNCF Graduated |

### 核心定位
TUF 是软件供应链安全的基础框架，被 Docker Content Trust、PyPI、Sigstore、Uptane (汽车) 等项目广泛采用，是保护软件更新系统的行业标准。

---

## 安全威胁模型

### 防范的攻击类型

```
┌─────────────────────────────────────────────────────────────────┐
│                    TUF 防范的攻击类型                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. 任意软件攻击 (Arbitrary Software Attack)                    │
│     攻击者替换合法软件为恶意软件                                 │
│     ► TUF: 所有文件需要有效签名                                  │
│                                                                  │
│  2. 回滚攻击 (Rollback Attack)                                   │
│     攻击者提供旧版本 (可能有已知漏洞)                            │
│     ► TUF: 版本号和时间戳验证                                    │
│                                                                  │
│  3. 冻结攻击 (Freeze Attack)                                     │
│     攻击者阻止客户端获取最新更新                                 │
│     ► TUF: 元数据过期机制                                        │
│                                                                  │
│  4. 混合攻击 (Mix-and-Match Attack)                              │
│     攻击者混合不同版本的软件组件                                 │
│     ► TUF: Snapshot 元数据确保一致性                             │
│                                                                  │
│  5. 无限循环攻击 (Endless Data Attack)                           │
│     攻击者发送无限数据耗尽资源                                   │
│     ► TUF: 预先声明文件大小                                      │
│                                                                  │
│  6. 密钥泄露恢复 (Key Compromise Recovery)                       │
│     某个签名密钥泄露后的恢复能力                                 │
│     ► TUF: 角色分离 + 阈值签名 + 密钥轮换                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 架构设计

### 角色和元数据结构

```
┌─────────────────────────────────────────────────────────────────┐
│                    TUF 元数据层次结构                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│                    ┌─────────────┐                              │
│                    │    ROOT     │  离线存储                     │
│                    │  (根角色)   │  最高信任锚点                 │
│                    └──────┬──────┘                              │
│                           │                                      │
│            ┌──────────────┼──────────────┐                      │
│            ▼              ▼              ▼                      │
│     ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│     │  TIMESTAMP  │ │  SNAPSHOT   │ │   TARGETS   │            │
│     │  (时间戳)   │ │  (快照)     │ │   (目标)    │            │
│     │             │ │             │ │             │            │
│     │ • 最新版本  │ │ • 所有元数据│ │ • 文件列表  │            │
│     │ • 防冻结    │ │   的版本    │ │ • 哈希值    │            │
│     │ • 短期有效  │ │ • 防混合    │ │ • 委托      │            │
│     └─────────────┘ └─────────────┘ └──────┬──────┘            │
│                                            │                    │
│                                   ┌────────┴────────┐          │
│                                   ▼                 ▼          │
│                           ┌─────────────┐   ┌─────────────┐    │
│                           │ Delegated   │   │ Delegated   │    │
│                           │ Targets A   │   │ Targets B   │    │
│                           │ (委托目标A) │   │ (委托目标B) │    │
│                           └─────────────┘   └─────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 角色职责

| 角色 | 职责 | 密钥管理 |
|:---|:---|:---|
| **Root** | 信任锚点，定义其他角色的公钥 | 离线存储，多人持有 |
| **Timestamp** | 标记最新版本，防冻结攻击 | 在线，自动轮换 |
| **Snapshot** | 记录所有元数据版本 | 在线，定期轮换 |
| **Targets** | 列出可分发的文件及哈希 | 可委托，按需管理 |

---

## 核心机制

### 1. 阈值签名 (Threshold Signatures)

```json
{
  "signed": {
    "_type": "root",
    "spec_version": "1.0.0",
    "version": 2,
    "expires": "2025-01-01T00:00:00Z",
    "keys": {
      "key-id-1": {"keytype": "ed25519", "scheme": "ed25519", "keyval": {"public": "..."}},
      "key-id-2": {"keytype": "ed25519", "scheme": "ed25519", "keyval": {"public": "..."}},
      "key-id-3": {"keytype": "ed25519", "scheme": "ed25519", "keyval": {"public": "..."}}
    },
    "roles": {
      "root": {
        "keyids": ["key-id-1", "key-id-2", "key-id-3"],
        "threshold": 2  // 需要 3 个密钥中的 2 个签名
      },
      "targets": {
        "keyids": ["key-id-4"],
        "threshold": 1
      }
    }
  },
  "signatures": [
    {"keyid": "key-id-1", "sig": "..."},
    {"keyid": "key-id-2", "sig": "..."}
  ]
}
```

### 2. 元数据过期机制

```
时间线:
──────────────────────────────────────────────────────►
    │              │              │              │
    ▼              ▼              ▼              ▼
┌──────┐      ┌──────┐      ┌──────┐      ┌──────┐
│TS v1 │      │TS v2 │      │TS v3 │      │TS v4 │
│exp:  │      │exp:  │      │exp:  │      │exp:  │
│1天   │      │1天   │      │1天   │      │1天   │
└──────┘      └──────┘      └──────┘      └──────┘

Timestamp: 1 天过期 (短期)
Snapshot:  1 周过期 (中期)
Targets:   1 年过期 (长期)
Root:      1-2 年过期 (离线管理)
```

### 3. 安全更新流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    TUF 客户端更新流程                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Client                                Repository               │
│    │                                        │                    │
│    │  1. 获取 Root 元数据                   │                    │
│    │────────────────────────────────────────►│                    │
│    │        (验证签名，检查版本)            │                    │
│    │◄────────────────────────────────────────│                    │
│    │                                        │                    │
│    │  2. 获取 Timestamp                     │                    │
│    │────────────────────────────────────────►│                    │
│    │        (检查过期时间)                  │                    │
│    │◄────────────────────────────────────────│                    │
│    │                                        │                    │
│    │  3. 获取 Snapshot                      │                    │
│    │────────────────────────────────────────►│                    │
│    │        (验证哈希匹配 Timestamp)        │                    │
│    │◄────────────────────────────────────────│                    │
│    │                                        │                    │
│    │  4. 获取 Targets                       │                    │
│    │────────────────────────────────────────►│                    │
│    │        (验证哈希匹配 Snapshot)         │                    │
│    │◄────────────────────────────────────────│                    │
│    │                                        │                    │
│    │  5. 下载目标文件                       │                    │
│    │────────────────────────────────────────►│                    │
│    │        (验证哈希匹配 Targets)          │                    │
│    │◄────────────────────────────────────────│                    │
│    │                                        │                    │
│    ▼  6. 安装/使用文件                      │                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 实现示例

### python-tuf 使用

```python
# 仓库端：创建 TUF 仓库
from tuf.api.metadata import (
    Metadata, Root, Snapshot, Targets, Timestamp,
    TargetFile, MetaFile
)
from securesystemslib.signer import CryptoSigner

# 1. 创建 Root 元数据
root = Root()
root.add_key(root_public_key, "root")
root.roles["root"].threshold = 2

# 2. 创建 Targets 元数据
targets = Targets()
target_file = TargetFile.from_file(
    "mypackage-1.0.0.tar.gz",
    "/path/to/mypackage-1.0.0.tar.gz"
)
targets.targets["mypackage-1.0.0.tar.gz"] = target_file

# 3. 创建 Snapshot
snapshot = Snapshot()
snapshot.meta["targets.json"] = MetaFile(version=targets.version)

# 4. 创建 Timestamp
timestamp = Timestamp()
timestamp.snapshot_meta = MetaFile(version=snapshot.version)

# 5. 签名并保存
for metadata, signer in [(root, root_signer), 
                          (targets, targets_signer),
                          (snapshot, snapshot_signer),
                          (timestamp, timestamp_signer)]:
    metadata_obj = Metadata(metadata)
    metadata_obj.sign(signer)
    metadata_obj.to_file(f"{metadata.type}.json")
```

```python
# 客户端：安全更新
from tuf.ngclient import Updater

# 创建更新器
updater = Updater(
    metadata_dir="./metadata/",
    metadata_base_url="https://repo.example.com/metadata/",
    target_base_url="https://repo.example.com/targets/",
    target_dir="./targets/"
)

# 刷新顶层元数据
updater.refresh()

# 下载目标文件 (自动验证)
info = updater.get_targetinfo("mypackage-1.0.0.tar.gz")
if info:
    path = updater.download_target(info)
    print(f"Downloaded and verified: {path}")
```

### go-tuf 使用

```go
package main

import (
    "github.com/theupdateframework/go-tuf/v2/metadata"
    "github.com/theupdateframework/go-tuf/v2/metadata/updater"
)

func main() {
    // 创建更新器配置
    cfg := &updater.Config{
        LocalMetadataDir:   "./metadata",
        RemoteMetadataURL:  "https://repo.example.com/metadata/",
        RemoteTargetsURL:   "https://repo.example.com/targets/",
        LocalTargetsDir:    "./targets",
    }
    
    // 创建更新器
    up, _ := updater.New(cfg)
    
    // 刷新元数据
    up.Refresh()
    
    // 下载目标
    targetInfo, _ := up.GetTargetInfo("mypackage-1.0.0.tar.gz")
    path, _ := up.DownloadTarget(targetInfo, "./targets/")
}
```

---

## 生态集成

| 项目 | 集成方式 |
|:---|:---|
| **Sigstore/cosign** | 基于 TUF 的容器签名验证 |
| **Docker Notary** | Docker Content Trust 使用 TUF |
| **PyPI (PEP 458)** | Python 包索引安全更新 |
| **Uptane** | 汽车软件更新标准 |
| **Datadog Agent** | 安全自动更新 |
| **ORAS** | OCI Registry As Storage |

---

## 使用场景

### 1. 容器镜像签名

```bash
# 使用 cosign (基于 TUF/Sigstore)
cosign sign --key cosign.key myregistry/myimage:latest

# 验证签名
cosign verify --key cosign.pub myregistry/myimage:latest
```

### 2. 软件包分发

```python
# PyPI TUF 集成
pip install --require-hashes -r requirements.txt
# pip 未来版本将集成 TUF 验证
```

### 3. 汽车 OTA 更新

```yaml
# Uptane (汽车 TUF 变体)
# Director Repository: 车辆特定更新
# Image Repository: 通用镜像仓库
# 支持增量更新、带宽优化
```

---

## 参考资源

- [TUF 规范](https://theupdateframework.github.io/specification/latest/)
- [官方文档](https://theupdateframework.io/overview)
- [GitHub - python-tuf](https://github.com/theupdateframework/python-tuf)
- [GitHub - go-tuf](https://github.com/theupdateframework/go-tuf)
- [CNCF 项目页面](https://www.cncf.io/projects/the-update-framework-tuf/)
- [Uptane 标准](https://uptane.github.io/)

---

**维护者**: Kudig Team | **许可证**: MIT
