---
title: Notary Project 容器签名
description: Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安...
summary: Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安...
category: dictionary
tags:
- k8s
- glossary
- security
- supply-chain
- signing
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Notary Project 容器签名 是什么
- Notary Project 详解
trigger_keywords:
- Notary Project 容器签名
- Notary Project
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Notary Project 容器签名（Notary Project）

## 概述

Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安全的基石组件。

## 核心概念/原理

- **OCI 签名**：为容器镜像和 OCI 制品附加数字签名
- **签名验证**：在拉取和部署时验证签名的完整性和来源
- **CNCF 孵化**：Docker/Microsoft/VMware 等联合推动
- **跨 Registry**：签名与制品分离存储，支持跨 Registry 传播

## 关键机制或特性

- `notation sign` 对 OCI 制品签名
- `notation verify` 验证签名
- 支持多种密钥后端（本地文件、Azure Key Vault、AWS KMS）
- Trust Store 和 Trust Policy 管理
- 签名存储在 OCI Registry 的 Referrers API
- 与 Kyverno/OPA Gatekeeper/Ratify 集成验证

## 使用场景与最佳实践

- CI/CD Pipeline 中的镜像签名和验证
- 生产部署前的镜像来源验证
- 合规要求下的软件供应链审计
- 多环境镜像复制时的完整性保障
- Kubernetes Admission 策略中的签名验证

## 架构深度解析

### Notation 签名验证架构

```
┌──────────────────────────────────────────────────────────────┐
│  CI/CD 流水线                                                 │
│   │  notation sign（签名镜像，密钥来自 KMS/本地）              │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ OCI Registry（支持 Referrers API）                       │  │
│  │ ├─ 镜像 manifest                                        │  │
│  │ └─ 签名 artifact（subject 指向镜像，含签名+证书）         │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 验证                          │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 验证方                                                    │  │
│  │ ├─ notation verify：本地验证签名链                      │  │
│  │ ├─ Trust Store：证书信任库（CA/自签）                    │  │
│  │ └─ Trust Policy：验证规则（是否要求签名/证书指纹）        │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 准入集成                      │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ K8s Admission：Kyverno / Gatekeeper / Ratify             │  │
│  │ └─ 未签名/签名无效镜像拒绝创建                          │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（notaryproject/notation）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| CLI | cmd/notation/ | sign/verify/list 命令 |
| 签名库 | notation-go/ | 签名生成与验证核心 |
| 证书管理 | notation-go/verifier/ | Trust Store/Policy 校验 |
| Registry 交互 | notation-go/registry/ | Referrers API 操作 |
| 插件 | internal/plugin/ | KMS 签名插件接口 |

### 流程步骤

1. CI 构建镜像后调用 `notation sign`，用签名插件（KMS/HSM）对 manifest 摘要签名。
2. 签名以 OCI artifact 推送到 registry，通过 subject 字段关联镜像。
3. 部署侧配置 Trust Store（信任证书）与 Trust Policy（验证规则）。
4. `notation verify` 或 K8s 准入控制器校验签名链：证书在信任库内 + 签名匹配镜像 digest。
5. 验证通过才允许部署/拉取，失败则拒绝并记录审计。

## 生产案例

### 案例 1：证书过期导致大规模签名验证失败（2023 年供应链门禁事件）

| 时间 | 事件 |
|---|---|
| T+0 | 签名证书到期前 1 天，CI 流水线批量签发失败 |
| T+1h | 存量验证未受影响（签名已生成），但新镜像无法签发，发布阻塞 |
| T+3h | 轮换证书并更新 Trust Store 白名单，流水线恢复 |
| T+1d | 补加证书到期监控（提前 30 天预警）与自动轮换 |

- **根因**：签名证书无到期监控与自动轮换；Trust Store 更新流程滞后。
- **修复命令**（轮换 + 验证）：
```bash
# 🔴 用新证书重新签名镜像
notation sign --key <new-key-name> registry.example.com/app:v1.2.3
# 🟢 验证签名链（含证书链校验）
notation verify registry.example.com/app:v1.2.3
```

### 案例 2：多环境信任策略不一致导致测试镜像误入生产

- **现象**：生产准入发现测试环境签名的镜像通过验证。
- **诊断**：测试与生产共用 Trust Store，无环境隔离的签名标识；Trust Policy 未区分环境。
- **修复**：按环境拆分 Trust Store 与签名密钥（环境标签进证书/签名）；Trust Policy 增加环境声明校验；生产仅接受生产签名的镜像。

## 对比评测

| 维度 | Notation（Notary Project） | cosign（Sigstore） | Ratify |
|---|---|---|---|
| 定位 | 签名规范 + CLI | 签名工具链 | 验证编排框架 |
| 信任模型 | X.509 PKI（Trust Store） | 透明日志 + 证书 | 插件化（可接两者） |
| K8s 集成 | 通过策略引擎 | 通过策略引擎 | 原生验证器 |
| 密钥后端 | KMS/HSM 插件 | KMS/HSM/密钥对 | 依赖验证器 |
| 生态 | CNCF（notaryproject） | Sigstore 生态 | CNCF 孵化 |

- **选型建议**：已有 PKI 体系选 Notation；Keyless 与透明日志需求选 cosign；需要统一验证框架选 Ratify（可同时配置 notation/cosign 验证器）。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 签名失败 | 密钥/插件配置错误 | `notation key list`、查看插件日志 |
| 验证失败 | 证书不在 Trust Store | `notation cert show` 核对指纹 |
| 策略拒绝 | Trust Policy 过严 | `notation policy show` 调整规则 |
| 找不到签名 | registry 不支持 Referrers | 检查 registry 版本与 artifact 类型 |
| 证书链断裂 | 中间 CA 缺失 | `notation cert inspect` 检查链 |

## 生产部署清单

- [ ] 签名密钥 KMS/HSM 托管，证书自动轮换 + 到期监控（提前 30 天）
- [ ] Trust Store 按环境隔离，Trust Policy 声明式 GitOps 管理
- [ ] CI 签名步骤纳入流水线门禁，失败即阻断发布
- [ ] K8s 准入集成（Kyverno/Gatekeeper/Ratify）强制验证
- [ ] 监控签名覆盖率、验证失败率、证书到期时间并告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 证书泄露/签名体系失效 | 立即吊销证书 + 轮换密钥，重签全部受影响镜像 |
| P1 | 信任体系迁移（Notation↔cosign 或 PKI 变更） | 双信任并存过渡期，灰度验证后切换 |
| P2 | Notation/插件版本升级 | 测试环境验证签名格式兼容性后滚动升级 |

## 面试要点

> 以下 Q&A 覆盖 Notary Project 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Notation 的信任模型与 Sigstore 有何区别？**
   A：Notation 采用传统 X.509 PKI：验证方维护 Trust Store（信任的根/中间证书），签名证书必须在信任链内且策略匹配（指纹/日期/环境）；Sigstore 采用 Keyless 模式（Fulcio 短期证书 + Rekor 透明日志）。前者适合已有企业 PKI 的受控环境，后者适合开放生态的供应链场景，两者可被 Ratify 统一编排。

2. **Q：Notation 如何实现"签名随镜像走"？**
   A：签名以 OCI artifact（如 `application/vnd.cncf.notary.signature`）推送到 registry，通过 manifest 的 subject 字段指向被签名镜像，registry 用 Referrers API 枚举关联。签名内容包含镜像 digest、签名者证书与时间戳，验证方按 subject 找到签名并校验 digest 一致性，镜像复制/迁移后签名仍可验证。

3. **Q：Notation 生产落地的关键风险与对策？**
   A：① 证书到期（无监控）→ 自动轮换 + 30 天预警；② 多环境信任串扰 → Trust Store/策略按环境隔离；③ 存量未签名镜像 → 灰度（先审计后强制）+ 分批补签；④ 签名覆盖率失控 → 覆盖率指标纳入发布门禁；⑤ 验证失败应急 → 一键回退审计模式，避免误拒阻塞发布。

## 运维要点

- 证书管理：签名证书 KMS 签发 + 自动轮换，纳入 PKI 到期监控。
- 信任治理：Trust Store/Policy 声明式管理，环境隔离，变更走审批。
- 覆盖监控：镜像签名覆盖率作为发布门禁指标，持续追踪。
- 排障入口：notation verify 本地复现 → 证书链 → registry referrers。
- 告警：签名失败率、验证失败率、证书到期、覆盖率下降。

## 参考链接

- https://notaryproject.dev/
- https://github.com/notaryproject/notation

## Related

- [[17-系统基础/06-知识字典/security/ratify.md|Ratify]]
- [[17-系统基础/06-知识字典/security/in-toto.md|in-toto]]
- [[17-系统基础/06-知识字典/security/trivy.md|Trivy]]


<!-- risk-assessed -->
