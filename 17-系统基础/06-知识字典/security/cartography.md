---
title: Cartography 资产图谱
description: Cartography 是 Lyft 开源的安全资产图谱工具，自动收集和关联云基础设施的资产信息，以图数据库（Neo4j）可视化展示资产关系和安全态势。...
summary: Cartography 是 Lyft 开源的安全资产图谱工具，自动收集和关联云基础设施的资产信息，以图数据库（Neo4j）可视化展示资产关系和安全态势。...
category: dictionary
tags:
- k8s
- glossary
- security
- asset-management
- graph
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cartography 资产图谱 是什么
- Cartography 详解
trigger_keywords:
- Cartography 资产图谱
- Cartography
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cartography 资产图谱（Cartography）

## 概述

Cartography 是 Lyft 开源的安全资产图谱工具，自动收集和关联云基础设施的资产信息，以图数据库（Neo4j）可视化展示资产关系和安全态势。

## 核心概念/原理

- **资产图谱**：自动发现和关联云基础设施资产
- **Neo4j 可视化**：图数据库驱动的资产关系视图
- **Lyft 开源**：经过 Lyft 大规模生产验证
- **多云支持**：AWS/GCP/Azure/K8s 资产采集

## 关键机制或特性

- 自动化资产采集（Cron 调度）
- 多云资产关联（EC2→S3→IAM→VPC）
- Kubernetes 资产采集
- 安全分析查询（Cypher 查询语言）
- 自定义分析插件
- 差异检测（变更追踪）
- Grafana Dashboard 集成

## 使用场景与最佳实践

- 云基础设施的资产盘点
- 安全态势的可视化分析
- 资产关系的自动化发现
- 合规审计的资产报告
- 安全团队的攻击面分析

## 架构深度解析

### Cartography 资产图谱架构

```
┌──────────────────────────────────────────────────────────────┐
│  数据源（Intelligence Modules）                               │
│  ├─ AWS：EC2/IAM/S3/RDS 等资源                               │
│  ├─ GCP：GCE/IAM 等                                          │
│  ├─ Kubernetes：节点/Pod/Service/RBAC                        │
│  ├─ GitHub：仓库/成员/密钥                                   │
│  └─ Okta/Azure AD：用户/组/应用                              │
│   │  定期采集（cron/调度）                                    │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Cartography（Python 服务）                               │  │
│  │ ├─ 采集：调用云 API 拉取资产元数据                       │  │
│  │ ├─ 建模：资产/关系映射到图结构                           │  │
│  │ └─ 更新：增量 upsert + 差异检测（删除失效资产）          │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 写入                          │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Neo4j 图数据库                                           │  │
│  │ ├─ 节点：资产（EC2/User/Secret 等）                     │  │
│  │ └─ 边：关系（HAS/ATTACHED_TO/ADMINISTERED_BY 等）        │  │
│  └─────────────────────────────────────────────────────────┘  │
│                             │ 查询                          │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 消费方：Cypher 查询 / 安全分析 / Grafana / 自动化         │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（lyft/cartography）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| 采集框架 | cartography/intel/ | 各云平台 intelligence 模块 |
| 图谱建模 | cartography/models.py | 节点/边 schema 定义 |
| 同步逻辑 | cartography/sync.py | 采集编排与差异处理 |
| 客户端 | cartography/cli.py | CLI 入口与调度 |
| 查询示例 | examples/ | 安全分析 Cypher 模板 |

### 流程步骤

1. 配置云账号凭据与目标资源类型，Cartography 按调度周期执行采集。
2. 各 intelligence 模块调用云 API 获取资产与关系（如 EC2 的 IAM 角色、安全组）。
3. 数据按 schema 写入 Neo4j：资产为节点，关系为边，附带最近更新属性。
4. 差异检测：上次存在但本次缺失的资产被标记为失效（remove_stale）。
5. 安全团队用 Cypher 查询攻击路径（如"可被公网访问的数据库"）、合规报告与变更追踪。

## 生产案例

### 案例 1：利用图谱发现未授权 S3 桶暴露（2023 年安全团队实战）

| 时间 | 事件 |
|---|---|
| T+0 | 安全团队运行攻击路径查询，发现 3 个 S3 桶策略允许匿名读写 |
| T+30min | 通过图谱回溯桶所属账号、关联 IAM 角色与责任人 |
| T+2h | 修正桶策略并下线遗留密钥；查询固化为定期巡检任务 |
| T+1w | 将"敏感资产暴露"查询纳入 CI 门禁，新资源上线即检查 |

- **根因**：资产分散在多个账号，人工盘点遗漏；无攻击面持续评估。
- **修复命令**（图谱查询 + 修复）：
```bash
# 🟢 查询公网可访问的 S3 桶（Cypher）
MATCH (b:AWSBucket)-[:MEMBER_OF_AWS_ACCOUNT]->(a:AWSAccount)
WHERE b.policy_condition = 'public' RETURN b.name, a.id
# 🔴 修正桶策略（示例）
aws s3api put-bucket-policy --bucket <name> --policy file://private.json
```

### 案例 2：采集任务失败导致图谱数据过期

- **现象**：安全查询返回的数据与实际情况偏差数周（下线资产仍显示在线）。
- **诊断**：采集 Cron 因 API 限流静默失败，无告警；remove_stale 未配置清理窗口。
- **修复**：采集任务失败告警接入监控；配置 remove_stale 定期清理（如保留 7 天窗口）；采集状态数据新鲜度指标纳入 Grafana。

## 对比评测

| 维度 | Cartography | CloudSploit/ScoutSuite | 云厂商 Security Hub |
|---|---|---|---|
| 数据模型 | 图数据库（关系分析） | 检查项报告 | 合规发现列表 |
| 分析能力 | Cypher 自定义查询 | 预置检查 | 预置规则 |
| 跨云支持 | AWS/GCP/K8s/GitHub 等 | 多云检查 | 单云 |
| 扩展性 | 自定义模块 | 有限 | 无 |
| 适用 | 攻击面分析/图谱 | 合规扫描 | 云原生合规 |

- **选型建议**：需要资产关系/攻击路径分析选 Cartography；快速合规扫描选 ScoutSuite；云厂商生态内选 Security Hub 等托管服务。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 采集失败 | API 限流/凭据过期 | 查看采集日志，核对 IAM 权限 |
| 数据缺失 | 模块未启用/权限不足 | 检查 intel 配置与最小权限 |
| 图谱陈旧 | remove_stale 未配置 | 检查清理窗口参数 |
| 查询超时 | 图规模过大 | 优化 Cypher 索引与查询模式 |
| Neo4j 故障 | 磁盘/内存不足 | 检查 Neo4j 健康与存储容量 |

## 生产部署清单

- [ ] 云凭据最小权限 + 定期轮换，凭据存 KMS/Secret 管理
- [ ] 采集任务失败告警接入监控，数据新鲜度指标可视化
- [ ] remove_stale 清理窗口配置（如 7 天），避免幽灵资产
- [ ] Neo4j 高可用 + 定期备份，查询模式建立索引
- [ ] 关键安全查询固化为巡检任务，输出报告对接 SIEM

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 凭据泄露或采集数据被污染 | 立即轮换凭据，重建图谱并核对采集源 |
| P1 | 数据源 API 变更（云平台） | 升级对应 intelligence 模块，灰度采集验证 |
| P2 | Cartography 版本升级 | 测试环境验证 schema 兼容性后升级 |

## 面试要点

> 以下 Q&A 覆盖 Cartography 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Cartography 为什么选择图数据库（Neo4j）而非关系型数据库？**
   A：安全分析的核心是资产间关系（IAM 角色关联的权限、安全组关联的实例、公网暴露路径），图数据库以节点+边原生表达关系，多跳遍历（攻击路径）用 Cypher 一条查询完成；关系型库需要大量 JOIN，深度遍历性能差且查询表达复杂。图模型让"谁可以通过什么路径访问什么"这类问题可高效回答。

2. **Q：Cartography 如何保证图谱数据的新鲜度？**
   A：双机制：① 定期采集（intelligence 模块按调度拉取云 API 增量更新）；② 差异检测（remove_stale：上次存在本次缺失的资产标记失效并清理，避免已下线资产残留在图谱）。同时采集失败告警与数据新鲜度指标监控，防止"看起来新鲜实则过期"。

3. **Q：用 Cartography 做攻击面分析的一般方法？**
   A：① 建模资产全景（账号/资源/权限/暴露面）；② 编写攻击路径查询（如"公网可达 + 高权限角色 + 弱配置"组合）；③ 将高频查询固化为定期巡检与 CI 门禁；④ 结合变更检测（diff）发现新增暴露面；⑤ 输出可追溯的报告（资产→责任人→修复项），驱动修复闭环。

## 运维要点

- 凭据治理：采集凭据最小权限 + KMS 托管 + 季度轮换。
- 采集监控：任务成功率、数据新鲜度（距上次成功采集时长）纳入告警。
- 图谱健康：Neo4j 磁盘/内存水位、查询延迟监控，定期备份。
- 查询治理：常用 Cypher 模板化入库，禁止临时全库扫描。
- 审计：采集与查询动作记录，安全查询结果归档。

## 参考链接

- https://cartography-cncf.github.io/cartography/
- https://github.com/lyft/cartography

## Related

- [[17-系统基础/06-知识字典/security/kubescape.md|Kubescape]]
- [[17-系统基础/06-知识字典/security/trivy.md|Trivy]]
- [[17-系统基础/06-知识字典/operations/cloud-custodian.md|Cloud Custodian]]


<!-- risk-assessed -->
