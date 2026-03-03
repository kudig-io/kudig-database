# in-toto

> **成熟度**: Graduated | **加入时间**: 2019-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://in-toto.io |
| **GitHub** | https://github.com/in-toto/in-toto |
| **文档** | https://in-toto.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Python, Go |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
in-toto 是软件供应链安全框架，用于保护软件从开发到部署的完整性，防止供应链攻击。

### 核心定位
in-toto 通过加密签名验证软件供应链的每个步骤，确保最终产物未被篡改，是 SLSA 框架的核心实现之一。

### 发展历程
- **2016**: NYU 开始研发
- **2017**: 论文发表和开源
- **2019-08**: 加入 CNCF 沙箱项目
- **2023-03**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **布局定义**: 定义预期的软件供应链流程
- **链接元数据**: 记录每个构建步骤的输入输出
- **签名验证**: 加密签名保护元数据完整性
- **策略执行**: 验证实际流程符合预期布局
- **SBOM 集成**: 与软件物料清单集成
- **多语言支持**: Python、Go、Java、Rust 实现

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                    软件供应链流程                            │
│                                                             │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │  开发   │───►│  构建   │───►│  测试   │───►│  发布   │  │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
│       │              │              │              │        │
│       ▼              ▼              ▼              ▼        │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │  Link   │    │  Link   │    │  Link   │    │  Link   │  │
│  │ (签名)  │    │ (签名)  │    │ (签名)  │    │ (签名)  │  │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                       验证流程                               │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                     Layout                              ││
│  │  (定义预期步骤、签名者、检查规则)                        ││
│  └─────────────────────────────────────────────────────────┘│
│                              │                              │
│                              ▼                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                   Verification                          ││
│  │  • 验证所有步骤是否执行                                  ││
│  │  • 验证签名者身份                                        ││
│  │  • 验证材料和产品哈希                                    ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心概念
| 概念 | 功能 | 说明 |
|:---|:---|:---|
| Layout | 供应链定义 | 定义预期的步骤和规则 |
| Step | 构建步骤 | 供应链中的一个操作 |
| Link | 步骤记录 | 记录步骤的输入输出 |
| Functionary | 执行者 | 被授权执行步骤的角色 |
| Inspection | 检查规则 | 验证时执行的额外检查 |

### 工作原理
1. **定义 Layout**: 项目所有者定义供应链预期流程
2. **收集 Link**: 每个步骤执行者生成签名的 Link 元数据
3. **分发制品**: 将软件制品与元数据一起分发
4. **验证**: 终端用户验证 Link 符合 Layout 定义
5. **拒绝/接受**: 验证失败则拒绝使用软件

---

## 使用场景

### 典型应用
- **软件分发**: 验证开源软件完整性
- **CI/CD 流水线**: 保护构建流程
- **容器镜像**: 验证镜像构建过程
- **固件更新**: 保护嵌入式系统更新
- **合规审计**: 提供供应链证据

### 适用条件
- 需要保护软件供应链安全
- 需要符合 SLSA 等合规要求
- 需要防止供应链攻击
- 多团队协作的软件项目

### 不适用场景
- 极简单的单人项目
- 不需要安全验证的内部工具

---

## 快速开始

### 安装部署
```bash
# Python 安装
pip install in-toto

# Go 安装
go install github.com/in-toto/in-toto-golang/cmd/in-toto@latest

# Docker
docker pull in-toto/in-toto
```

### 基础配置
```python
# layout.py - 创建供应链布局
from in_toto.models.layout import Layout, Step
from in_toto.models.metadata import Metablock

layout = Layout()
layout.expires = "2025-12-31T00:00:00Z"

# 定义构建步骤
step = Step(name="build")
step.expected_materials = [["MATCH", "*.go", "WITH", "PRODUCTS", "FROM", "clone"]]
step.expected_products = [["CREATE", "app"]]
step.pubkeys = ["alice-key-id"]
step.expected_command = ["go", "build"]

layout.steps = [step]
```

### 验证测试
```bash
# 生成 Link 元数据
in-toto-run --step-name build --key alice.pem -- go build

# 验证供应链
in-toto-verify --layout layout.json --layout-key owner.pub

# 检查验证结果
echo $?  # 0 表示验证通过
```

---

## 最佳实践

### 生产环境建议
- 使用硬件安全模块存储签名密钥
- 配置多个 functionary 分权
- 与 CI/CD 系统集成
- 定期轮换密钥

### 安全建议
- 保护 Layout 签名密钥
- 使用阈值签名
- 审计 Link 元数据
- 监控验证失败事件

### 集成建议
- 集成到 CI/CD 流水线
- 与 Sigstore/cosign 配合使用
- 生成 SBOM 一同分发
- 配置自动化验证

---

## 生态集成

### 相关 CNCF 项目
- **TUF**: 安全更新框架
- **Sigstore**: 签名和透明日志
- **SPIFFE/SPIRE**: 工作负载身份

### 相关标准
- **SLSA**: 供应链安全等级
- **SBOM**: 软件物料清单
- **Sigstore**: 无密钥签名

### 常见集成方案
- in-toto + GitHub Actions
- in-toto + Tekton Chains
- in-toto + cosign

---

## 社区与支持

### 社区资源
- Slack: #in-toto in cloud-native.slack.com
- 邮件列表: in-toto-dev@googlegroups.com
- GitHub Discussions

### 贡献指南
访问 https://github.com/in-toto/in-toto/blob/develop/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://in-toto.io/docs)
- [GitHub Repo](https://github.com/in-toto/in-toto)
- [CNCF 项目页面](https://www.cncf.io/projects/in-toto/)
- [in-toto 规范](https://github.com/in-toto/docs/blob/master/in-toto-spec.md)
- [SLSA 框架](https://slsa.dev/)

---

**维护者**: Kudig Team | **许可证**: MIT
