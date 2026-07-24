---
title: 跨云迁移手册
description: '策略 1: 重新部署 (Rehost)'
summary: '策略 1: 重新部署 (Rehost)'
category: synthesis
tags:
- multi-cloud
- migration
- cloud-providers
- kubernetes
- strategy
- helm
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 跨云迁移手册 是什么
- 如何 跨云迁移手册
trigger_keywords:
- 跨云迁移手册
prerequisites:
- kubectl-basics
- helm-basics
- backup-basics
relationships:
- target: '[[实体/helm.md]]'
  type: uses
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 跨云迁移手册

## 概述

跨云迁移是将 Kubernetes 工作负载从一个云厂商（如 AWS EKS）迁移到另一个（如 GCP GKE 或阿里云 ACK）的系统工程。云原生架构的声明式特性使迁移在技术上可行，但数据迁移、网络互通、认证体系差异等带来了显著的工程复杂性。本手册提供从策略选择到执行的完整方法论。

## 迁移策略

### 三种核心策略

```
策略 1: 重新部署 (Rehost / Lift and Shift)
  → 直接在新云部署相同配置
  → 最简单，迁移速度最快
  → 无法利用新云的原生特性
  → 适合: 时间紧迫、云厂商锁定低的场景

策略 2: 重新平台化 (Replatform)
  → 保持应用架构不变，改用新云的托管服务
  → 如: EBS → GCP Persistent Disk，ELB → Cloud Load Balancing
  → 中等复杂度，可降低运维成本
  → 适合: 追求成本优化的场景

策略 3: 重构 (Refactor)
  → 利用新云特有服务重新设计
  → 如: 使用 GCP Cloud Spanner 替代自管 MySQL
  → 最复杂，迁移周期最长
  → 适合: 有明确技术债务需要偿还的场景
```

### 策略选择矩阵

| 因素 | Rehost | Replatform | Refactor |
|------|--------|------------|----------|
| 迁移速度 | 快（数天） | 中（数周） | 慢（数月） |
| 成本节省 | 低 | 中 | 高 |
| 风险 | 低 | 中 | 高 |
| 适用 | 紧急迁移 | 优化型迁移 | 架构升级 |

## 迁移检查清单

### 迁移前评估

```
□ 应用清单盘点
  → 梳理所有需要迁移的工作负载
  → 识别服务间依赖关系
  → 标记有状态服务和无状态服务

□ 云差异分析
  → 网络模型（VPC/CIDR/SecurityGroup）
  → 存储类型（EBS/PD/云盘）和性能差异
  → IAM/RBAC 模型差异
  → 可用区和 Region 映射
  → 成本对比（计算/存储/网络出口）

□ 配置抽取
  → ConfigMap / Secret 导出
  → 硬编码的云特定配置识别（如 endpoint、region）
  → 镜像仓库迁移计划
```

### 数据迁移计划

```
□ 有状态服务数据迁移
  → 数据库: 使用原生复制（如 MySQL binlog replication）
  → 文件存储: rsync / 云间数据传输服务
  → 对象存储: 跨云同步（如 AWS DataSync → GCP Storage Transfer）

□ 数据迁移窗口规划
  → 选择低峰期
  → 评估迁移时间（数据量 / 带宽）
  → 定义 cutover 时间点
```

### 网络与认证

```
□ 网络规划
  → VPC/CIDR 规划（避免冲突）
  → 防火墙/安全组规则迁移
  → 跨云 VPN 或专线建立（迁移期间双活需要）
  → DNS 切换计划（TTL 调整、流量权重）

□ 身份认证迁移
  → IAM 策略映射（AWS IAM → GCP IAM）
  → RBAC 角色迁移
  → ServiceAccount 令牌更新
  → OIDC/LDAP 集成验证
```

### 回滚与验证

```
□ 回滚方案
  → 定义回滚条件和触发标准
  → 确保旧环境在迁移完成前不销毁
  → DNS 回滚方案（降低 TTL 以支持快速回退）

□ 验证方案
  → 功能测试（API 测试、端到端测试）
  → 性能基准对比（迁移前后延迟/吞吐）
  → 监控和告警重建
  → 成本对比验证
```

## 工具链

| 任务 | 工具 | 说明 |
|------|------|------|
| 配置转换 | kustomize, [[实体/helm.md|helm]] | 多环境配置管理 |
| 数据迁移 | Velero, 数据库原生工具 | K8s 资源 + PV 数据备份恢复 |
| 网络测试 | iperf, curl, mtr | 带宽和延迟测试 |
| 验证 | k6, 自动化测试 | 负载测试和功能验证 |
| DNS 切换 | ExternalDNS, Route53 | DNS 记录管理 |
| 密钥迁移 | [[实体/external-secrets.md|External Secrets]] | 跨云密钥同步 |

### Velero 跨云迁移示例

```bash
# 🟢 低风险：备份操作
# 源集群：备份
velero backup create pre-migration \
  --include-namespaces production \
  --snapshot-volumes=true \
  --volume-snapshot-locations aws

# 将备份上传到跨云共享对象存储（如 S3 + GCS 互备）

# 🟡 中风险：恢复到新云
# 目标集群：恢复
velero backup-location create gcs-backup \
  --provider gcp \
  --bucket migration-backups \
  --config region=asia-east1

velero restore create --from-backup pre-migration \
  --namespace-mappings production:production
```

## 最佳实践

- **先迁移无状态服务，再迁移有状态服务**：无状态服务迁移风险低、速度快，可以先验证基础设施和网络连通性
- **保持双活窗口**：在 DNS 切换前，新旧环境并行运行，通过流量比例控制实现渐进迁移
- **降低 DNS TTL**：迁移前 24-48 小时降低 DNS TTL 到 60s，确保 DNS 切换能快速生效和回滚
- **验证数据一致性**：数据库迁移后使用校验工具（如 pt-table-checksum）确认数据完整性
- **保留旧环境至少 1 周**：切换后保持旧环境只读运行，确保完全确认无问题后再销毁

## 常见陷阱

- **忽略可用区映射差异**：AWS us-east-1a 和 GCP us-east1-a 不是同一个物理位置——跨云灾备需要基于 Region 而非 AZ 名称规划
- **网络出口成本被忽视**：迁移后如果数据访问仍指向旧云（如未更新的 endpoint），会产生高额跨云传输费用——需要完整更新所有 endpoint 配置
- **镜像仓库未迁移**：Pod 启动时从旧云镜像仓库拉取镜像，延迟高且可能因网络策略失败——迁移前需将镜像推送到新云仓库并更新配置

## 相关 Domain

- 云厂商/01-aws-eks/01-eks-migration-guide
- 云厂商/02-google-cloud-gke/01-gke-migration-guide

## 相关页面

- [[概念/data-protection-k8s.md|K8s 数据保护]] — Velero 备份恢复
- [[概念/multi-cluster-security.md|多集群安全]] — 跨云安全架构

## Related

- [[故障诊断/高级排障/08-cluster-operations/03-helm-troubleshooting.md|Helm 部署故障排查指南 [topic-structural-trouble-shooting]]]
- [[技能/集群运维/helm/helm-fta.md|Helm 发布异常故障树分析 (skills)]]


<!-- risk-assessed -->
