---
title: 共因故障
description: 共因故障（CCF）是由同一个根因导致的多个组件同时故障。共因故障会破坏冗余设计的有效性，是系统可靠性分析中需要特别关注的风险。...
summary: 共因故障（CCF）是由同一个根因导致的多个组件同时故障。共因故障会破坏冗余设计的有效性，是系统可靠性分析中需要特别关注的风险。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- commoncausefailure
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 共因故障 是什么
- Common Cause Failure (CCF) 详解
trigger_keywords:
- 共因故障
- Common Cause Failure (CCF)
- fta
prerequisites:
- troubleshooting-methodology
---



# 共因故障

> **英文名**: Common Cause Failure (CCF)

## 概述

共因故障（CCF）是由同一个根因导致的多个组件同时故障。共因故障会破坏冗余设计的有效性，是系统可靠性分析中需要特别关注的风险。

## 核心概念/原理

### 典型场景
- 同一机架的多个节点因交换机故障同时掉线。
- 同一容器镜像的 Bug 导致所有副本同时崩溃。
- 同一可用区的所有实例因区域级故障同时不可用。

## 关键机制或特性

共因故障是冗余系统失效的主要原因。通过多样化（不同厂商、不同版本、不同区域）可以降低共因故障风险。

## 使用场景与最佳实践

在 K8s 中，防止共因故障：多可用区部署、不同节点池使用不同实例类型、避免所有 Pod 使用同一镜像 tag。

## K8s 中的共因故障场景

| 共因 | 影响的冗余组件 | 后果 |
|------|--------------|------|
| 同 AZ 故障 | 同 AZ 的所有 Pod | 服务不可用 |
| 配置错误 | 所有副本 (同一 ConfigMap) | 全部崩溃 |
| 镜像缺陷 | 所有 Pod (同一镜像) | 全部故障 |
| 内核漏洞 | 同版本所有节点 | 节点崩溃 |
| 证书过期 | 同 CA 签发的所有证书 | API 不可用 |

## 防御共因故障的实践

```
1. 故障域分散
   └─ 多 AZ 部署 (topologySpreadConstraints)
   └─ 反亲和性 (podAntiAffinity)

2. 配置多样性
   └─ 金丝雀发布 (先更新部分副本)
   └─ 多版本镜像 (避免全部同一版本)

3. 独立更新
   └─ 滚动更新 (maxUnavailable=1)
   └─ 分批升级节点

4. 监控共因指标
   └─ 同批次 Pod 同时崩溃告警
   └─ 节点内核版本一致性检查
```

## 面试要点

1. **什么是共因故障？为什么危险？**
   - 多个冗余组件因同一原因同时故障
   - 使冗余设计失效，与门保护失效
   - 是可靠性分析中最危险的场景

2. **K8s 中如何防御共因故障？**
   - 多 AZ 部署（分散物理故障域）
   - 金丝雀发布（避免同时更新）
   - PDB（保证最小可用副本）

3. **如何识别潜在的共因故障？**
   - 检查冗余组件是否共享故障域
   - 分析配置/镜像/内核版本一致性
   - 审查更新策略是否分批执行

## 参考链接

- [Common Cause Failure (CCF)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
