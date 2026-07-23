---
title: 专有云 Apsara Stack
description: 阿里云专有云（Apsara Stack）深度运维知识索引 — 天基/ASO、升级补丁、合规加固、盘古存储、故障手册
summary: 阿里云专有云（Apsara Stack）目录索引，覆盖飞天底座组件、ESS/SLS/POP、天基/ASO 运维、升级补丁、合规加固（等保/国密）、盘古存储排障与专属故障手册。
category: index
tags:
- index
- alibaba-cloud
- apsara-stack
- private-cloud
tier: core
created: '2026-07-02'
last_updated: '2026-07-23'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 专有云 Apsara Stack

> 阿里云专有云（Apsara Stack）本地化部署的深度运维知识，定位政企/金融/运营商客户数据中心场景。专有云运维主干（架构/远程顾问/组件映射）见 [[云厂商/阿里云/index.md|阿里云根文档]]。

## 文档索引

| # | 文档 | 说明 |
|---|------|------|
| — | [[云厂商/阿里云/专有云-Apsara/alicloud-apsara-ack-overview.md\|专有版 ACK 概述]] | 专有版 ACK 金融级架构、控制平面高可用、安全加固与监控 |
| 250 | [[云厂商/阿里云/专有云-Apsara/250-apsara-stack-ess-scaling.md\|ESS 弹性伸缩]] | 架构差异、伸缩组配置、触发策略、ACK Cluster Autoscaler 集成 |
| 251 | [[云厂商/阿里云/专有云-Apsara/251-apsara-stack-sls-logging.md\|SLS 日志服务]] | 三层架构、Logtail 采集、机房级审计、高性能查询 |
| 252 | [[云厂商/阿里云/专有云-Apsara/252-apsara-stack-pop-operations.md\|POP 平台运维（ASOP）]] | POP 网关、多租户配额、巡检监控、错误码处理 |
| 253 | [[云厂商/阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md\|天基/ASO 运维流程]] | 部署编排、配置下发、巡检、变更中心、自愈、控制台路径速查 |
| 254 | [[云厂商/阿里云/专有云-Apsara/254-apsara-upgrade-patch-management.md\|升级与补丁管理]] | ACK 版本升级、飞天底座补丁、滚动策略、前置检查、回滚预案 |
| 255 | [[云厂商/阿里云/专有云-Apsara/255-apsara-compliance-hardening.md\|合规加固（等保/国密）]] | 等保四级、国密 SM2/SM4、KMS、审计、金融级 NetworkPolicy/RBAC |
| 256 | [[云厂商/阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md\|盘古存储排障]] | PVC Pending、IO 延迟、快照/扩容失败、盘古集群健康检查 |
| 99 | [[云厂商/阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md\|专有云故障手册]] | 组件异常→K8s症状→排查命令→升级路径 可执行 runbook |

## 概览

- [[云厂商/阿里云/README.md|阿里云根 README]]
- [[云厂商/阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]

<!-- risk-assessed -->
