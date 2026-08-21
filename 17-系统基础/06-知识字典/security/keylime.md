---
title: Keylime 远程证明
description: Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验...
summary: Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验...
category: dictionary
tags:
- k8s
- glossary
- security
- attestation
- tpm
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Keylime 远程证明 是什么
- Keylime 详解
trigger_keywords:
- Keylime 远程证明
- Keylime
- dictionary
prerequisites:
- kubernetes
---



# Keylime 远程证明（Keylime）

## 概述

Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验证远程系统的完整性和可信状态。

## 核心概念/原理

- **远程证明**：验证远程系统的启动和运行状态
- **TPM 基础**：利用 TPM 2.0 硬件信任根
- **CNCF Sandbox**：MITRE 主导
- **Linux 专注**：为 Linux 系统设计

## 关键机制或特性

- Agent（被测系统）+ Verifier（验证者）架构
- TPM Quote 采集和验证
- IMA（Integrity Measurement Architecture）日志
- 可信启动链验证
- 密钥分发和绑定
- 证书管理
- REST API 和 CLI

## 使用场景与最佳实践

- 服务器启动完整性验证
- 边缘设备的信任验证
- 合规要求的系统完整性监控
- 零信任架构的硬件信任根
- 机密计算的远程证明

## 架构深度解析

### Keylime 远程证明架构

```
┌──────────────────────────────────────────────────────────────┐
│  Verifier（验证者）                                           │
│  ├─ 定义验证策略（TPM 度量基准、文件白名单）                  │
│  ├─ 发起远程证明挑战                                          │
│  └─ 判定 Agent 信任状态                                       │
│   │                                                          │
│   ▼  challenge/quote                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Agent（部署在被验证节点）                                 │  │
│  │ ├─ 定期采集 TPM PCR 值与文件度量                        │  │
│  │ ├─ 用 TPM 签名生成 quote（防伪造）                       │  │
│  │ └─ 上报度量数据与租约状态                                │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 度量                        │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Registrar（登记处）                                      │  │
│  │ ├─ 存储节点 TPM 公钥（EK/AK）与验证密钥                  │  │
│  │ └─ 供 Verifier 校验 quote 签名                          │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（keylime/keylime）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| Agent | keylime/agent/ | 度量采集与 quote 生成 |
| Verifier | keylime/verifier/ | 策略校验与信任判定 |
| Registrar | keylime/registrar/ | 密钥登记与校验 |
| 度量机制 | keylime/ima/ | IMA（完整性度量架构）集成 |
| TPM 交互 | keylime/tpm/ | TPM 2.0 quote/密钥操作 |

### 流程步骤

1. 节点安装 Keylime Agent 并首次向 Registrar 登记 TPM 密钥（EK/AK）。
2. 运维定义验证策略（基准 PCR 值、IMA 文件白名单、允许的度量）导入 Verifier。
3. Verifier 向 Agent 发起证明挑战，Agent 采集 PCR 值与 IMA 度量并生成 TPM 签名 quote。
4. Verifier 用 Registrar 登记的密钥验签，比对策略基准，判定节点可信/不可信。
5. 不可信时触发联动（隔离网络、吊销证书、告警），Agent 周期性上报保持持续信任。

## 生产案例

### 案例 1：内核模块被篡改触发自动隔离（2024 年零信任试点）

| 时间 | 事件 |
|---|---|
| T+0 | 某节点被植入后门内核模块（重启后驻留） |
| T+15min | Keylime 下一轮证明发现 PCR 值与基线不符，判定不可信 |
| T+20min | 联动 SDN 策略将节点从生产网络隔离，同时告警安全团队 |
| T+2h | 取证确认后重装系统并重新登记基线，恢复节点 |

- **根因**：节点长期未打补丁，被利用植入持久化后门；无启动完整性校验。
- **修复命令**（查看证明状态 + 恢复基线）：
```bash
# 🟢 查看节点信任状态与失败原因
keylime_tenant -c status -t <agent-uuid>
# 🔴 隔离后重装系统，重新登记并更新基准策略
keylime_tenant -c reg -t <agent-uuid> --file new-ek-cert.pem
```

### 案例 2：IMA 度量误报导致大量节点被误判

- **现象**：软件更新后数百节点同时被判不可信，业务大面积受影响。
- **诊断**：IMA 文件白名单未同步更新，更新后的二进制哈希偏离基线；批量更新未分批。
- **修复**：软件更新流程与 Keylime 基线更新绑定（先更新基线再更新节点）；白名单变更走审批 + 灰度，观察误报率后再全量。

## 对比评测

| 维度 | Keylime | TPM 直用 | 机密计算（CoCo） |
|---|---|---|---|
| 定位 | 运行时完整性证明 | 硬件信任根原语 | 计算机密性 |
| 部署复杂度 | 中（三组件） | 高（自研） | 中高 |
| 证明粒度 | 系统级（IMA/PCR） | 自实现 | 工作负载级 |
| 生态 | CNCF 项目 | 无 | Kata+CoCo 生态 |
| 适用 | 裸机/边缘/云主机 | 定制化 | 机密工作负载 |

- **选型建议**：需要系统级完整性持续证明选 Keylime；机密数据计算选 CoCo；两者可组合（CoCo 内用 Keylime 验证宿主机）。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 证明失败 | TPM 不可用/驱动缺失 | `ls /dev/tpm*`、`tpm2_getcap handles-persistent` |
| 验签失败 | 登记密钥不匹配 | `keylime_tenant -c reglist` 核对 AK |
| 误判不可信 | 基线未更新 | 比对失败 PCR/IMA 条目，更新白名单 |
| Agent 失联 | 网络/租约过期 | 检查 Agent 心跳与防火墙端口 9001 |
| 启动失败 | 配置证书错误 | `keylime_agent -c verify` 诊断 |

## 生产部署清单

- [ ] TPM 2.0 全节点启用（BIOS + 内核 tpm 驱动），确认 `tpm2_getcap` 可用
- [ ] Verifier/Registrar 高可用部署（多副本 + 数据库），证书纳入统一 PKI
- [ ] IMA 基线建立：从干净系统采集，纳入 GitOps 版本管理
- [ ] 联动机制定义：不可信时自动隔离/吊销/告警的响应矩阵
- [ ] 监控证明成功率、误报率、Agent 心跳并告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 证明大面积失败或误报（可信节点被隔离） | 暂停联动隔离，回退基线策略，逐节点排查 |
| P1 | 内核/软件批量更新 | 先更新基线白名单，分批灰度节点验证 |
| P2 | Keylime 版本升级 | 测试环境验证 Agent/Verifier 兼容性后滚动升级 |

## 面试要点

> 以下 Q&A 覆盖 Keylime 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Keylime 的远程证明与普通健康检查有何本质区别？**
   A：健康检查只验证"服务是否活着"（进程/端口），不验证系统是否被篡改；Keylime 通过 TPM 硬件信任根生成不可伪造的 quote（签名 PCR 度量），并持续采集 IMA 文件完整性，能证明"系统启动状态与运行时文件都符合基线"，即使 root 权限的恶意软件也无法伪造度量结果。

2. **Q：IMA（Integrity Measurement Architecture）在 Keylime 中的作用？**
   A：IMA 是内核机制，在文件被访问/执行时记录其哈希到度量列表，并聚合进 PCR。Keylime Agent 定期读取 IMA 度量并与 Verifier 的白名单比对，从而检测运行时文件篡改（二进制、库、配置）。它把"启动完整性"扩展到"运行时完整性"，是持续证明的关键。

3. **Q：Keylime 生产落地的主要坑有哪些？**
   A：① 基线管理：系统更新后忘记同步 IMA 白名单导致大规模误判（更新流程必须与基线更新绑定）；② TPM 兼容性：部分云虚机未暴露 TPM 2.0；③ 联动策略过激：误报直接隔离节点造成业务损失（分级响应：先告警再隔离）；④ 证书/密钥登记遗漏：新节点忘记登记导致无法证明。

## 运维要点

- 基线治理：IMA 白名单与系统镜像同步发布，变更走审批 + 灰度。
- 容量：Verifier 按节点数 × 证明频率规划资源；高并发证明可水平扩容。
- 证书：Verifier/Registrar/Agent 间 TLS 证书统一 PKI 管理，提前轮换。
- 响应矩阵：不可信分级处理（记录 → 告警 → 隔离 → 吊销），避免误报影响。
- 告警：证明失败率、误报率、Agent 心跳丢失、PCR 漂移。

## 参考链接

- https://keylime.dev/
- https://github.com/keylime/keylime

## Related

- [[17-系统基础/06-知识字典/security/confidential-containers.md|Confidential Containers]]
- [[17-系统基础/06-知识字典/security/parsec.md|PARSEC]]
- [[17-系统基础/06-知识字典/security/spire.md|SPIRE]]
