---
title: SOPS（Secrets OPerationS）
description: SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key
  Vault、ag...
summary: SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key Vault、ag...
category: dictionary
tags:
- k8s
- glossary
- security
- secrets
- encryption
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SOPS（Secrets OPerationS） 是什么
- SOPS 详解
trigger_keywords:
- SOPS（Secrets OPerationS）
- SOPS
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SOPS（Secrets OPerationS）（SOPS）

## 概述

SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key Vault、age 或 PGP 作为密钥后端，实现 GitOps 友好的密钥管理。

## 核心概念/原理

- **文件级加密**：对值（value）加密，保留键（key）和结构不变，便于 diff 和 review
- **多密钥后端**：同时支持 AWS KMS、GCP KMS、Azure Key Vault、age、PGP
- **审计与权限**：通过 .sops.yaml 配置加密规则（creation rules），按路径匹配密钥
- **GitOps 集成**：加密后的文件可安全提交到 Git，配合 External Secrets 或 Sealed Secrets 使用

## 关键机制或特性

- 支持加密/解密/原地编辑（in-place edit）操作
- `sops --encrypt --in-place secrets.yaml` 加密文件
- `sops --decrypt secrets.yaml` 解密到标准输出
- 支持 SOPS + age 轻量方案，无需云 KMS
- 与 External Secrets Operator 配合实现自动注入

## 使用场景与最佳实践

- GitOps 仓库中的 Secret/ConfigMap 加密存储
- CI/CD pipeline 中的敏感配置管理
- 多环境（dev/staging/prod）密钥分离
- 合规要求下的密钥轮转与审计

## 架构深度解析

### SOPS 加密工作流

```
┌──────────────────────────────────────────────────────────────┐
│  开发/运维（本地或 CI）                                       │
│   │  sops --encrypt secrets.yaml（KMS/age/PGP 加密）          │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ GitOps 仓库（明文 Git，密文文件）                        │  │
│  │ ├─ secrets.yaml：值加密，结构/键名可见                   │  │
│  │ └─ .sops.yaml：加密规则（路径匹配 + 密钥配置）           │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 解密                          │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 消费方                                                    │  │
│  │ ├─ Flux/Kustomize：sops 解密插件 → Secret 对象          │  │
│  │ ├─ External Secrets：sops 加密内容解密注入               │  │
│  │ └─ CI：sops --decrypt 生成临时明文（不留存）             │  │
│  └─────────────────────────────────────────────────────────┘  │
│                                                              │
│  密钥源：AWS KMS / GCP KMS / Azure KV / age / PGP / HashiCorp │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（getsops/sops）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| CLI | cmd/sops/ | 加密/解密/编辑命令 |
| 格式支持 | pkg/sops/ | YAML/JSON/ENV/INI 格式处理 |
| 密钥管理 | pkg/keyservice/ | KMS/age/PGP 密钥后端 |
| 树处理 | pkg/decrypt/ | 密文树解析与递归解密 |
| 规则 | pkg/config/ | .sops.yaml 规则引擎 |

### 流程步骤

1. 配置 `.sops.yaml` 声明加密规则（文件路径匹配、KMS/age 密钥、加密字段）。
2. 开发者运行 `sops --encrypt secrets.yaml`，值字段按规则加密（结构保留）。
3. 加密文件提交 Git（可安全审阅 diff：键名可见、值不可见）。
4. GitOps 控制器（Flux Kustomize sops 插件）或 External Secrets 拉取并解密。
5. 解密结果注入为 K8s Secret，明文不落盘（内存/临时文件即时清理）。

## 生产案例

### 案例 1：KMS 密钥轮换导致集群解密失败（2023 年 GitOps 事故）

| 时间 | 事件 |
|---|---|
| T+0 | 云团队按例轮换 KMS 主密钥，未通知 GitOps 平台组 |
| T+30min | Flux 解密全部失败，Secret 未更新，依赖 Secret 的服务启动报错 |
| T+2h | 定位为 SOPS 加密引用旧 KMS key ARN，新文件加密用新 key |
| T+4h | 用旧 key 解密存量文件后用新 key 重新加密提交，恢复 |

- **根因**：KMS 密钥轮换未联动 SOPS 重加密；多团队无变更通知机制。
- **修复命令**（重加密）：
```bash
# 🟢 查看文件加密引用的 key 信息
sops --config .sops.yaml -d secrets.yaml | head
# 🔴 使用新 key 重新加密（全部环境文件）
sops updatekeys secrets.yaml && sops --encrypt --in-place secrets.yaml
```

### 案例 2：age 密钥泄露引发的紧急轮换

- **现象**：开发机 age 私钥疑似泄露（备份文件被拖走）。
- **诊断**：全仓使用单 age 密钥，泄露即全量暴露；无密钥分级。
- **修复**：生成新 age 密钥对，全部文件 `sops updatekeys` 重加密；旧私钥吊销；环境级密钥分离（dev/staging/prod 各自密钥）。

## 对比评测

| 维度 | SOPS | Sealed Secrets | Vault（外部） |
|---|---|---|---|
| 加密位置 | 文件级（Git 内） | K8s Secret 级 | 外部存储 |
| 密钥管理 | KMS/age/PGP | 集群内密钥 | Vault 统一 |
| GitOps 集成 | Flux/Argo 原生 | 控制器 | 需同步器 |
| 审计 | Git 历史 | 无 | 完整审计 |
| 适用 | 配置文件加密 | 集群内 Secret | 运行时动态密钥 |

- **选型建议**：GitOps 文件加密选 SOPS；仅 K8s Secret 场景选 Sealed Secrets；需要动态密钥/集中审计选 Vault。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 解密失败 | 密钥不存在/权限不足 | `sops -d file` 看错误，核对 KMS 权限 |
| 部分字段未加密 | .sops.yaml 规则缺失 | `sops --config .sops.yaml -e file` 检查 |
| CI 解密超时 | KMS 限流 | 检查 KMS 配额与重试策略 |
| 密钥轮换后失效 | 未 updatekeys | 检查文件 metadata 中 key 列表 |
| 明文泄露 | 加密遗漏/日志输出 | 扫描仓库明文密钥（gitleaks） |

## 生产部署清单

- [ ] .sops.yaml 规则覆盖全部敏感文件，CI 校验"无未加密敏感文件"
- [ ] 密钥分级：环境级密钥分离，KMS 权限最小化
- [ ] 密钥轮换 SOP：轮换后全量 updatekeys 重加密，纳入变更日历
- [ ] 明文检测流水线（gitleaks/trufflehog）阻止明文密钥入库
- [ ] 监控解密失败率、密钥到期时间、仓库密钥扫描结果

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 密钥泄露或解密大面积失败 | 立即轮换密钥 + 全量重加密，暂停依赖 Secret 的发布 |
| P1 | KMS/age 主密钥轮换 | 联动重加密计划，灰度环境验证后全量 |
| P2 | SOPS 版本升级 | 测试环境验证文件格式兼容性后分批升级 |

## 面试要点

> 以下 Q&A 覆盖 SOPS 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：SOPS 加密与普通 GPG 加密文件有什么核心区别？**
   A：SOPS 是结构化文件的"值加密"：保留 YAML/JSON 的键名与结构，只加密值字段，支持按路径规则选择字段；同时支持 KMS/age/PGP 多种密钥后端和多 key 混用（不同文件不同密钥），并能 `updatekeys` 批量重加密，天然适配 GitOps（diff 可见、审阅友好）。

2. **Q：SOPS 在 GitOps 中如何与 Flux/External Secrets 配合？**
   A：Flux 通过 Kustomize 的 SOPS 解密插件（或 flux sops 集成）在 apply 时解密生成 Secret；External Secrets 则把 SOPS 加密内容作为数据源解密注入。共同点：明文只存在于控制器内存/临时文件，Git 仓库只存密文，密钥由 KMS/age 统一管理，实现"密钥与配置分离"。

3. **Q：SOPS 密钥轮换的正确姿势？**
   A：① 先确认新密钥可用（KMS 权限/age 公钥）；② 对全部环境文件执行 `sops updatekeys`（把新 key 加入文件 metadata）并重新加密；③ 分批应用到各环境，验证解密成功；④ 最后吊销/归档旧密钥；⑤ 全过程纳入变更管理，防止"轮换即事故"。多环境务必分级密钥，避免单密钥全暴露。

## 运维要点

- 密钥治理：环境级密钥分离，KMS 权限最小化 + 访问审计。
- 轮换节奏：KMS 主密钥季度轮换，联动 updatekeys 重加密。
- 明文防控：CI 集成 gitleaks 扫描，阻止明文密钥入库。
- 排障入口：先看密钥存在性/权限 → 文件 metadata key 列表 → KMS 配额。
- 告警：解密失败率、密钥到期、KMS 限流、明文扫描命中。

## 参考链接

- https://github.com/getsops/sops
- https://fluxcd.io/flux/guides/mozilla-sops/

## Related

- [[17-系统基础/06-知识字典/security/external-secrets.md|External Secrets]]
- [[17-系统基础/06-知识字典/security/vault.md|Vault]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA]]


<!-- risk-assessed -->
