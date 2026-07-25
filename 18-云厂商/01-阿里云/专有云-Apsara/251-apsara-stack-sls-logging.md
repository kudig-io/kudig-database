---
title: 专有云（Apsara Stack）- SLS 日志服务
description: 专有云 SLS 三层架构、Logtail 采集、机房级审计、高性能查询与运维最佳实践
summary: 专有云（Apsara Stack）SLS 日志服务深度指南：三层架构解析、Logtail DaemonSet 采集配置、机房级日志审计与安全、高性能查询与仪表盘、运维最佳实践与排障。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- sls
- logging
- logtail
- daemonset
- audit
tier: core
sources:
- 阿里云专有云 SLS 运维手册
- Logtail 采集最佳实践
created: 2026-05-23
last_updated: 2026-07-23
relationships:
- target: '[[18-云厂商/01-阿里云/02-ACK集群运维.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/255-apsara-compliance-hardening.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 平台工程师
- 远程顾问
estimated_read_time: 16min
intent_queries:
- 专有云 SLS 怎么配置
- 专有云 Logtail 采集
- 专有云审计日志
- 专有云日志 Shard 配额
trigger_keywords:
- SLS
- 日志服务
- Logtail
- 审计
- Shard
prerequisites:
- alicloud-basics
- k8s-logging
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 专有云（Apsara Stack）- SLS 日志服务

本文档面向在客户数据中心运维 [[18-云厂商/01-阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE，系统梳理专有云 SLS（日志服务）的三层架构、Logtail 采集方案、机房级日志审计与安全、高性能查询与仪表盘及运维最佳实践。

> **环境**: Apsara Stack 企业版/敏捷版

---

## 1. 专有云 SLS 架构深度解析

专有云 SLS 采用三层架构设计，确保在私有化部署环境下的大吞吐量与稳定性。

### 1.1 三层架构

```
┌─────────────────────────────────────┐
│  采集端：Logtail / SDK               │
│  （DaemonSet 运行于 ACK 节点）        │
└────────────────┬────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│  接入层：LVS + Nginx                 │
│  （负载均衡 + 鉴权）                  │
└────────────────┬────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│  逻辑层：LogServer                   │
│  （API 解析、读写分发，水平扩容）       │
└──────┬──────────────────────┬───────┘
       ↓                      ↓
┌──────────────┐      ┌──────────────┐
│  存储层       │      │  索引层       │
│  Pangu/KV    │      │  LogIndex    │
│  （盘古）     │      │  倒排索引     │
└──────────────┘      └──────────────┘
```

| 组件 | 专有云特点 |
|:---|:---|
| **LogServer** | 负责 API 解析、读写分发，支持水平扩容 |
| **LogIndex** | 负责倒排索引构建，支持秒级 GB 级搜索 |
| **存储底座** | 基于盘古（Pangu）分布式文件系统 |
| **接入层** | LVS + Nginx，内网 VIP 部署 |

> **关键认知**：SLS 的存储底座是盘古。当 SLS 查询/写入全局异常时，需联动 [[18-云厂商/01-阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md|盘古存储排障]]。

---

## 2. 日志采集方案（Logtail）

### 2.1 Logtail DaemonSet 部署（ACK）

在专有云 ACK 环境中，Logtail 以 **DaemonSet** 形式运行，每个节点一个 Pod 采集容器与节点日志。

```yaml
# 采集配置示例（AliyunLogConfig CRD）
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: apsara-stdout-log
  namespace: kube-system
spec:
  project: apsara-stack-project        # 专有云 SLS Project
  logstore: app-stdout
  shardCount: 2                        # 按流量规划 Shard
  lifeCycle: 7                         # 专有云通常存储 7-15 天
  logtailConfig:
    inputType: plugin
    configName: app-stdout
    inputDetail:
      plugin:
        inputs:
        - type: service_docker_stdout   # 容器标准输出
          detail:
            Stdout: true
            Stderr: true
            IncludeLabel:
              app: business-app         # 按标签过滤采集
```

### 2.2 文件日志采集

```yaml
# 采集容器内文件日志（如应用日志文件）
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: app-file-log
  namespace: kube-system
spec:
  project: apsara-stack-project
  logstore: app-file
  shardCount: 4
  lifeCycle: 30
  logtailConfig:
    inputType: file
    configName: app-file
    inputDetail:
      logType: common_reglog
      logPath: /data/logs              # 容器挂载路径
      filePattern: "*.log"
      regex: '^(?P<time>\d{4}-\d{2}-\d{2}.*) (?P<level>\w+) (?P<msg>.*)$'
      timeFormat: '%Y-%m-%d %H:%M:%S'
```

### 2.3 关键配置优化

| 配置项 | 专有云建议 | 说明 |
|--------|-----------|------|
| **Shard 规划** | 单 Shard 支撑 ~5MB/s 写入 | 按总流量预分配：Shard 数 = 总流量 / 5MB/s |
| **接入地址（Endpoint）** | 专有云 POP 内网 VIP | 必须用专有云提供的地址，非公有云 |
| **生命周期** | 7-15 天（热数据） | 长期数据归档至 OSS |
| **Logtail 版本** | 与 ACK 插件版本兼容 | 专有云更新慢，注意兼容性 |
| **资源限制** | Logtail 内存 limit 512Mi-1Gi | 防止 OOM 影响节点 |

---

## 3. 机房级日志审计与安全

专有云环境对安全性有极高要求，SLS 是审计的核心载体。

### 3.1 审计三层集成

| 审计层 | 内容 | 投递目标 |
|--------|------|----------|
| **ASOP 审计** | 平台管理员操作日志 | SLS（ASOP 审计 Logstore） |
| **ACK 审计** | kube-apiserver 所有请求 | SLS（audit Logstore） |
| **RDS/OSS 审计** | 数据库/对象存储操作日志 | 定期同步至 SLS 合规检查 |

### 3.2 审计日志留存（等保要求）

> **合规要点**：等保四级要求审计日志留存 ≥180 天。SLS 热数据生命周期通常 7-15 天，超期数据需归档至 OSS 长期保存。

```yaml
# SLS 投递 OSS 归档配置（SLS 控制台/POP）
# 当 Logstore 数据超过生命周期时，自动投递至 OSS 冷存储
OSSArchive:
  OSSBucket: "audit-archive-apsara"
  PathFormat: "%Y/%m/%d/%H"
  CompressType: "snappy"
  RetentionDays: 365              # 满足等保 ≥180 天
```

### 3.3 SIEM 集成

专有云支持将 SLS 日志转发至第三方 SIEM（安全态势感知）：

| 方式 | 说明 |
|------|------|
| SLS → 消费组 → SIEM | 通过消费组实时消费日志转发 |
| SLS Webhook 告警 | 告警转发至 SIEM |
| 定期导出 | 批量导出审计日志至 SIEM |

---

## 4. 高性能查询与仪表盘

### 4.1 查询优化技巧

| 技巧 | 说明 | 示例 |
|:---|:---|:---|
| **前缀匹配** | 加快长字符串匹配 | `request_id: abc123*` |
| **字段索引优先** | 高频查询字段开字段索引，降低存储 | 只对 `status`、`app` 等开索引 |
| **Shard 均衡** | Shard 倾斜时重新哈希 | 观察 Shard 写入分布 |
| **时间范围收敛** | 查询时缩小时间窗 | 避免全量扫描 |
| **全文 vs 字段索引** | 全文索引适合模糊，字段索引适合精确 | 按查询模式选择 |

### 4.2 仪表盘（SQL 统计可视化）

专有云 SLS 控制台支持直接将 SQL 统计结果可视化：

```sql
-- HTTP 状态码分布（实时监控）
* | select status, count(1) as cnt group by status order by cnt desc

-- Top10 慢请求
* | select request_uri, avg(latency_ms) as avg_lat
    group by request_uri order by avg_lat desc limit 10

-- 错误率趋势（按分钟）
* | select date_format(__time__, '%H:%i') as t,
    sum(case when status >= 500 then 1 else 0 end) * 100.0 / count(1) as err_pct
    group by t order by t
```

---

## 5. 运维最佳实践

1. **租户配额管理**：SLS 资源在专有云受限于 `Quota`，需在 ASOP 提前分配存储容量与 Shard 数
2. **机房级容灾**：双机房部署时确认日志采集负载均衡策略，避免单点
3. **Logtail 版本**：专有云 Logtail 更新较慢，需确认与 ACK 插件版本兼容
4. **Shard 容量规划**：按峰值流量 × 1.5 倍冗余规划 Shard 数，避免 `ShardReadQuotaExceed`
5. **索引成本**：字段索引按需开启，全文索引存储成本高
6. **审计完整性**：定期验证审计日志完整性（无丢失），满足合规

---

## 6. 常见故障排查

### 6.1 日志采集延迟/丢失

| 原因 | 排查 | 处理 |
|------|------|------|
| Logtail OOM | `kubectl logs logtail`、节点内存 | 调大 Logtail 内存 limit |
| Shard 配额满 | `ShardReadQuotaExceed` | 扩 Shard 数 |
| 采集配置错误 | AliyunLogConfig 状态、Label 过滤 | 修正配置与标签 |
| 网络不通 | Logtail → SLS Endpoint 连通性 | 检查 POP 网络与 Endpoint 配置 |
| 盘古异常 | SLS 全局写入失败 | 联动盘古排障（§1） |

```bash
# 🟢 低风险：只读
# Logtail DaemonSet 状态
kubectl get pods -n kube-system | grep logtail
# Logtail 日志（采集错误）
kubectl logs -n kube-system -l app=logtail --tail=100
# AliyunLogConfig 采集状态
kubectl get aliyunlogconfig -A
```

### 6.2 查询无数据/慢

| 原因 | 排查 | 处理 |
|------|------|------|
| 索引未开 | Logstore 是否开索引 | 开启字段/全文索引 |
| 时间范围错 | 查询时间窗 | 收敛时间范围 |
| Shard 倾斜 | Shard 写入分布 | 重新哈希均衡 |
| LogIndex 异常 | 索引层健康 | 联系日志团队/TAM |

---

## 相关文档

- [[18-云厂商/01-阿里云/02-ACK集群运维.md|02 ACK集群运维]]
- [[18-云厂商/01-阿里云/专有云-Apsara/252-apsara-stack-pop-operations.md|252 POP 平台运维]]
- [[18-云厂商/01-阿里云/专有云-Apsara/255-apsara-compliance-hardening.md|255 合规加固]]
- [[18-云厂商/01-阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md|256 盘古存储排障]]
- [[18-云厂商/01-阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]

## Related

- [[23-实体/02-K8s核心组件/coredns.md|CoreDNS]]
- [[17-系统基础/05-速查卡/sql.md|sql]]

<!-- risk-assessed -->
