---
title: in-toto 供应链安全
description: in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构...
summary: in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构...
category: dictionary
tags:
- k8s
- glossary
- security
- supply-chain
- verification
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- in-toto 供应链安全 是什么
- in-toto 详解
trigger_keywords:
- in-toto 供应链安全
- in-toto
- dictionary
prerequisites:
- kubernetes
---



# in-toto 供应链安全（in-toto）

## 概述

in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构建和分发过程中未被篡改。

## 核心概念/原理

- **完整性框架**：定义供应链步骤（Steps）和检查（Inspections）的完整布局
- **元数据记录**：每个步骤的输入/输出哈希、命令、执行者签名
- **验证链**：从源代码到最终制品的端到端验证
- **CNCF 孵化**：与 TUF/Sigstore 构成供应链安全三件套

## 关键机制或特性

- Layout 定义：供应链步骤序列和验证规则
- Link 元数据：每个步骤的材料（materials）和产品（products）
- 函数签名验证（Functionary verification）
- 子布局（Sublayouts）支持嵌套供应链
- ITE-5/ITE-6 规范标准化
- `in-toto-run` / `in-toto-verify` CLI 工具

## 使用场景与最佳实践

- CI/CD Pipeline 的完整性验证
- 软件发布流程的审计追踪
- 第三方依赖的来源验证
- SLSA 合规的供应链证明
- 与 Sigstore/TUF 集成的综合安全方案

## 架构深度解析

### in-toto 供应链验证模型

```
┌──────────────────────────────────────────────────────────────┐
│  软件供应链步骤（Steps）                                       │
│   │                                                          │
│   ├─ ① 编写源代码（开发人员）→ link 元数据                    │
│   ├─ ② 构建产物（CI 系统）→ link 元数据                      │
│   ├─ ③ 代码审查（审查者）→ link 元数据                       │
│   ├─ ④ 打包发布（发布者）→ link 元数据                       │
│   │                                                          │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Layout（布局文件，由 Root 签名）                          │  │
│  │ ├─ 定义每个步骤的 名称/责任人/命令/产物                  │  │
│  │ ├─ 定义 Inspection（对最终产物的自动检查）               │  │
│  │ └─ 定义公钥（每个步骤责任人的验证密钥）                  │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 验证                           │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ in-toto-verify：                                        │  │
│  │ ├─ 校验 layout 签名与步骤公钥                           │  │
│  │ ├─ 校验每个 link 的签名与命令/产物哈希                  │  │
│  │ └─ 执行 Inspection 并汇总通过/失败                      │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（in-toto/in-toto）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| Layout 校验 | in_toto/verifylib.py | 验证布局、链路与检查项 |
| 元数据模型 | in_toto/models/layout.py | Layout/Link 数据模型与签名 |
| 运行记录 | in_toto/runlib.py | 命令包装生成 link 元数据 |
| 密钥管理 | in_toto/keylib.py | 密钥加载与验签 |
| CLI 入口 | in_toto/in_toto_run.py | `in-toto-run` 命令实现 |

### 流程步骤

1. Root 密钥签发 Layout，声明供应链步骤（责任人、命令、产物）。
2. 每个步骤执行时用 `in-toto-run` 包装，生成带签名与命令/产物哈希的 link 文件。
3. 最终产物与全部 link 元数据一并发布到仓库（或 OCI registry）。
4. 验证方运行 `in-toto-verify --layout root.layout`，按序校验签名与哈希。
5. 校验通过表示"每一步由声明的责任人按声明命令执行"，供应链未被篡改。

## 生产案例

### 案例 1：CI 构建机被植入恶意代码（2023 年开源供应链事件复盘）

| 时间 | 事件 |
|---|---|
| T+0 | 攻击者拿到 CI 构建机权限，向构建脚本注入后门并重新构建发布 |
| T+30min | 用户侧 in-toto 验证报错：`link signature verification failed` |
| T+1h | 比对 link 元数据发现构建步骤签名密钥已更换（攻击者替换了 CI 密钥） |
| T+4h | 吊销被窃密钥，从干净基线重建构建机，发布新 link 元数据 |

- **根因**：构建机密钥未做硬件保护（明文私钥），且无密钥轮换审计。
- **修复命令**（验证 + 吊销）：
```bash
# 🟢 验证供应链元数据完整性
in-toto-verify --layout root.layout --layout-key root.pub
# 🔴 吊销泄露密钥（更新 Root 布局中的公钥并重新签发）
in-toto-sign --key root --layout root.layout --output new-root.layout
```

### 案例 2：布局策略过宽导致验证形同虚设

- **现象**：安全团队发现部分产物绕过 in-toto 校验直接上线。
- **诊断**：Layout 中 `expected_materials/products` 字段留空、`allow_bypass` 未禁用，发布步骤可用任意命令通过验证。
- **修复**：严格声明每个步骤的命令白名单与产物清单，禁用 `allow_bypass`，将 `in-toto-verify` 接入 CI 门禁（gate）强制阻塞未验证产物。

## 对比评测

| 维度 | in-toto | Sigstore（cosign） | SLSA（框架） |
|---|---|---|---|
| 定位 | 供应链步骤完整性验证 | 制品签名/验证 | 供应链安全等级标准 |
| 粒度 | 全链路步骤级 | 单制品级 | 级别定义（L1-L3） |
| 防篡改 | Layout+Link 链式校验 | 签名+透明度日志 | 依赖实现 |
| 集成成本 | 需改造 CI 步骤 | 较低（OCI 原生） | 无强制实现 |

- **选型建议**：需要"谁在何时做了什么"的可审计链路选 in-toto；仅需制品签名选 Sigstore；两者可组合（in-toto 元数据装入 cosign bundle）满足 SLSA L3。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 验证报签名失败 | 步骤密钥轮换未同步 | `in-toto-verify --verbose` 定位失败步骤 |
| 产物哈希不匹配 | 构建过程非确定性 | 检查构建工具版本/时间戳，用 `--keep-logs` 比对 |
| Layout 拒绝 | 公钥过期/不匹配 | `in-toto-keyinfo` 列出步骤公钥指纹 |
| 链路缺 link | 步骤未用 in-toto-run 包装 | `find . -name '*.link'` 核对产物 |
| Inspection 失败 | 策略脚本错误 | 单独运行 inspection 命令调试 |

## 生产部署清单

- [ ] 根密钥离线冷存储，多签名人（threshold 签名）控制 Layout 变更
- [ ] 每个 CI 步骤用 in-toto-run 包装并记录命令/产物/环境
- [ ] Layout 中禁用 allow_bypass，严格声明命令白名单与产物清单
- [ ] 验证接入 CI 门禁（gate），未通过验证的产物禁止发布
- [ ] 建立密钥轮换与吊销 SOP，轮换后立即重新签发 Layout

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 步骤密钥泄露或被吊销 | 立即吊销密钥、重新签发 Layout，暂停发布直至链路恢复验证 |
| P1 | 构建链重构（换 CI/构建机） | 更新 Layout 责任人密钥与命令，灰度验证新旧链路并行 |
| P2 | in-toto 版本升级 | 先验证旧 Layout 兼容性，按项目分批迁移 |

## 面试要点

> 以下 Q&A 覆盖 in-toto 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：in-toto 如何保证供应链中"谁做了什么"不可抵赖？**
   A：in-toto 用两层信任：Layout（由 Root 离线密钥签发）声明供应链的每个步骤、责任人公钥与命令；每个步骤执行时生成的 link 元数据携带该步骤私钥签名和命令/产物哈希。验证时逐条校验 layout 签名、link 签名与哈希一致性，任何一步被篡改或伪造都无法通过，从而实现全链路审计与防抵赖。

2. **Q：in-toto 与 Sigstore 的分工与配合是什么？**
   A：Sigstore 解决"制品是谁签的"（单制品签名 + Rekor 透明度日志 + Fulcio 证书），in-toto 解决"制品是怎么来的"（全链路步骤元数据）。配合方式：构建时用 in-toto 生成 link，发布时用 cosign 把 link 元数据打包进制品签名 bundle，验证方同时校验签名与供应链链路，这也是 SLSA 高等级要求的实现路径。

3. **Q：实施 in-toto 最常见的失败模式是什么？**
   A：① Layout 策略过宽（命令/产物未严格声明、allow_bypass 未禁用）导致验证无意义；② 密钥管理薄弱（CI 明文私钥、无轮换）导致攻击者可伪造 link；③ 只验证不门禁（验证结果不阻塞发布）。对策：离线根密钥 + threshold 签名、严格布局策略、验证接入 CI gate 强制生效。

## 运维要点

- 密钥体系：根密钥离线冷存，步骤密钥独立签发；所有密钥纳入统一 PKI 管理。
- 元数据分发：link 与 layout 随制品发布到 OCI registry（cosign bundle），验证方可离线校验。
- 轮换节奏：步骤密钥按季度轮换，轮换窗口内新旧公钥并存完成过渡。
- 审计闭环：in-toto-verify 结果作为发布审批附件，留存至少 1 年。
- 告警：验证失败率、密钥到期时间、Layout 变更次数纳入监控。

## 参考链接

- https://in-toto.io/
- https://github.com/in-toto/in-toto

## Related

- [[17-系统基础/06-知识字典/security/notary-project.md|Notary Project]]
- [[17-系统基础/06-知识字典/security/ratify.md|Ratify]]
- [[17-系统基础/06-知识字典/security/supply-chain-security.md|供应链安全]]
