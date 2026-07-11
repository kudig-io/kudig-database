---
title: 多区域灾备架构
description: 多区域灾备架构模式：主备、双活、引导灯三种模式与 RTO/RPO 目标及流量切换方案
summary: Active-Passive / Active-Active / Pilot-Light 三模式对比 + 全局流量切换 + 数据同步策略
category: reliability
tags:
- slo
- sli
- reliability
- disaster-recovery
- multi-region
- architecture
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 多区域灾备架构

> **核心原则**：多区域灾备不是"再建一个集群放着"，而是**明确承诺一个可量化的 RTO/RPO，并围绕这个承诺设计数据同步、流量切换、回切的完整闭环**。没演练过的灾备架构 = 没有灾备。RTO/RPO 写进文档没用，写进定期 Game Day 验证才有用。

## 三种架构模式对比

```
模式1: Active-Passive（主备）
   Region-A(主,写读写)  ──异步复制──▶  Region-B(备,待命)
   流量100%→A   A挂→切B

模式2: Active-Active（双活）
   Region-A(写读写) ◀──双向复制──▶ Region-B(写读写)
   流量50/50   任一挂→另一扛全部

模式3: Pilot-Light（引导灯）
   Region-A(全量)  ──异步复制──▶  Region-B(最小核心运行)
   B平时只跑核心/数据热   A挂→B扩容接管
```

| 模式 | RTO | RPO | 成本 | 复杂度 | 适用 |
|------|-----|-----|------|--------|------|
| Active-Passive | 分钟–小时 | 秒 | 高（全量冷备） | 中 | 合规要求高 |
| Active-Active | 秒 | ~0 | 最高（双全量） | 高 | 全球用户、零容忍 |
| Pilot-Light | 分钟 | 秒 | 中（最小热备） | 中 | 性价比首选 |

## 数据同步策略（灾备成败核心）

```
┌─────────────┐                     ┌─────────────┐
│ Region-A    │   1. 异步流          │ Region-B    │
│  无状态服务  │ ◀──────────────────▶ │  无状态服务  │
│  有状态DB    │   2. DB 复制          │  有状态DB    │
│  缓存        │   3. 缓存重建(不复制)  │  缓存        │
└─────────────┘                     └─────────────┘
```

1. **应用层无状态**：会话放 Redis/外部存储，Pod 可在任一区域拉起。
2. **DB 异步复制**：主从复制（PostgreSQL streaming、MySQL binlog、MongoDB replica set）。RPO 取决于复制延迟，**必须监控复制延迟指标**。
3. **缓存不跨区复制**：成本高且无必要，灾备区缓存冷启动，用预热脚本填关键数据。

⚠️ **冲突处理**：Active-Active 下双写需应用层做 CRDT 或最后写入胜出（LWW），数据库原生双活支持有限。

## 全局流量切换

```yaml
# AWS Route 53 / Cloudflare / GSLB 配置（示意）
Type: Failover
Primary: Region-A (health check: GET /health, 10s interval)
Secondary: Region-B
FailoverPolicy:
  - Primary 连续 3 次健康检查失败 → 自动切 Secondary
  - DNS TTL: 60s（短的 TTL 才能快速切换）
```

🔴 **高危**：手动切流必须双人确认。误切流到未就绪的备区会造成全站不可用。

```bash
# 🔴 高危：手动 DNS 切换（生产事故级操作）
# 切换前 Checklist：
#   [ ] 备区健康检查通过
#   [ ] 复制延迟 < RPO 承诺
#   [ ] 备区缓存已预热
#   [ ] 通知客户与支持团队
aws route53 change-resource-record-sets \
  --hosted-zone-id Z123 \
  --change-batch '{"Changes":[{"Action":"UPSERT","ResourceRecordSet":{...Region-B...}}]}'
```

## 回切（Failback，最易出错）

切换到备区容易，**回切到主区**才是难点——备区在故障期间产生了新数据，主区需要先追上：

```
T+0   主区故障恢复
T+1   反向建立复制：备区(主) → 主区(从)
T+2   等待主区数据追平（监控 lag=0）
T+3   选窗口切回（低峰期）
T+4   DNS 切回主区
T+5   观察 30 分钟稳定
```

跳过数据追平就回切 = 数据丢失。回切必须有"复制延迟=0"的硬门控。

## Kubernetes 多区域实现

```yaml
# Karmada / Cluster API 管理多集群（示意）
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata: { name: api-spread }
spec:
  resourceSelectors: [{ apiVersion: apps/v1, kind: Deployment, name: api }]
  placement:
    clusterAffinity: { clusterNames: [region-a, region-b] }
    replicaScheduling:
      replicaSchedulingType: Divided
      replicaDivisionPreference: Weighted
      weightPreference:
        staticWeightList:
        - targetCluster: { clusterNames: [region-a] }
          weight: 1
        - targetCluster: { clusterNames: [region-b] }
          weight: 1
```

## RTO/RPO 承诺矩阵（参考）

| 服务等级 | RTO | RPO | 架构建议 |
|---------|-----|-----|---------|
| 核心（支付） | < 1 min | ~0 | Active-Active |
| 重要（下单） | < 15 min | < 1 min | Pilot-Light |
| 一般（浏览） | < 1 hour | < 5 min | Active-Passive |
| 内部工具 | < 4 hour | < 1 hour | 备份恢复 |

## 演练铁律

1. **每季度全链路切换演练**：从 DNS 切换到真实流量接管，不能只"看一眼备区在不在"。
2. **演练产出 = 工单**：每次演练发现的问题必须开工单，下季度验证修复。
3. **轮换演练区域**：这次切到 B，下次切到 A，避免"备区永远是备区"的隐性腐烂。
4. **故障注入演练**：见 [[可靠性/灾难恢复/03-enterprise-disaster-recovery-chaos-engineering.md]]，用混沌工程模拟区域级故障。

## 常见陷阱

1. **RPO 承诺 vs 复制延迟不监控**：承诺 RPO=1分钟，但复制延迟实际 5 分钟，事故时数据丢 5 倍。
2. **备区配置漂移**：主区天天改，备区半年没动，切过去发现配置不兼容。用 GitOps 统一管理两边配置。
3. **只测切换不测回切**：回切才是真正考验数据一致性的环节。
4. **DNS TTL 太长**：TTL=1小时意味着切换后 1 小时内仍有流量去旧区，等于没切。

## 相关

- [[可靠性/灾难恢复/02-dr-automation-playbook.md|02 dr automation playbook]]
- [[可靠性/灾难恢复/18-cross-region-disaster-recovery.md|18 cross region disaster recovery]]
- [[可靠性/灾难恢复/03-enterprise-disaster-recovery-chaos-engineering.md|03 enterprise dr chaos]]

<!-- risk-assessed -->
