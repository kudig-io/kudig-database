---
title: 专有云（Apsara Stack）- POP 平台运维（ASOP）
description: 专有云 POP 网关与 API 接入、ASOP 运维架构、多租户资源隔离配额、巡检监控与错误码处理
summary: 专有云（Apsara Stack）POP 平台运维（ASOP）深度指南：ASOP 运维架构、POP 网关与 API 接入规范、多租户资源隔离与配额管理、自动化巡检监控链路、常见 POP 错误码与处理。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- pop
- asop
- operations
- quota
- multi-tenant
tier: core
sources:
- 阿里云专有云 ASOP 运维手册
- POP 网关接入规范
created: 2026-05-23
last_updated: 2026-07-23
relationships:
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/005-apsara-tianji-aso-operations.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/002-apsara-stack-ess-scaling.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/apsara-stack-components.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 平台工程师
- 远程顾问
estimated_read_time: 16min
intent_queries:
- 专有云 POP 网关是什么
- 专有云 ASOP 怎么运维
- 专有云多租户配额怎么管
- 专有云 POP 错误码
trigger_keywords:
- POP
- ASOP
- 配额
- 多租户
- 错误码
prerequisites:
- alicloud-basics
- k8s-architecture
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

# 专有云（Apsara Stack）- POP 平台运维（ASOP）

本文档面向在客户数据中心运维 [[18-云厂商/01-阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE，系统梳理专有云统一运维中枢 **ASOP（Apsara Stack Operations Platform）**与 **POP（Platform Open API）网关**的运维架构、API 接入规范、多租户资源隔离与配额管理、巡检监控链路及常见错误码处理。

> **环境**: Apsara Stack 企业版/敏捷版

---

## 1. ASOP 运维架构概述

**ASOP** 是专有云的统一运维中枢。它不仅管理 ECS/SLB 等云产品，还负责底层物理资源与安全，是上层系统（如 ACK）与底层云产品交互的桥梁。

### 1.1 核心子系统

| 组件 | 核心职责 | 运维关注点 |
|:---|:---|:---|
| **Tianji（天基）** | 自动化部署与基线管理 | 集群部署进度、补丁分发、自愈 |
| **Tianmu（天目）** | 统一监控平台 | 物理机告警、容器健康度、指标采集 |
| **Kong（控）** | POP 接口网关 | 请求节流、API 安全认证、路由 |
| **Fuxi（伏羲）** | 分布式任务调度 | 离线任务、大规模作业调度、资源分配 |

> 详细的 ASO/天基操作流程见 [[18-云厂商/01-阿里云/专有云-Apsara/005-apsara-tianji-aso-operations.md|253 天基/ASO 运维流程]]。

### 1.2 ASOP 在架构中的位置

```
┌─────────────────────────────────────┐
│  ASOP 控制台（运维/运营总入口）         │
│  租户管理 | 资源配额 | 产品运维 | 告警  │
└────────────────┬────────────────────┘
                 │
         ┌───────┴────────┐
         ↓                ↓
┌──────────────┐  ┌──────────────┐
│  POP 网关     │  │  天基编排     │
│  （API 入口） │  │  （部署/自愈）│
└──────┬───────┘  └──────┬───────┘
       │                 │
       ↓                 ↓
┌─────────────────────────────────────┐
│  云产品：ECS/SLB/VPC/RDS/OSS/ACK ... │
└─────────────────────────────────────┘
```

---

## 2. POP 网关与 API 接入

POP（Platform Open API）是上层系统（如 ACK、CCI）与底层云产品交互的桥梁。

### 2.1 API 调用链（示例：创建 ECS）

```
ACK Console / CCI
       ↓ 调用 POP
POP Gateway（Kong 网关）
       ↓ 鉴权 + 路由
ECS Controller
       ↓
天基 / 伏羲（计算）
       ↓
ECS 实例创建
```

### 2.2 调用规范

| 项 | 说明 |
|----|------|
| **接入地址** | 通常为私网 VIP（如 `pop.apsarastack.com`） |
| **认证方式** | `AccessKey / AccessSecret`，需在 RAM 为子用户配置 |
| **SDK** | 专有云需用特定版本云产品 SDK（如 `aliyun-python-sdk-core`） |
| **签名** | 阿里云 POP 签名算法（HMAC-SHA1） |
| **Region** | 专有云 Region 标识（如 `cn-apsara-local`） |

```bash
# 🟢 低风险：只读
# 检查 POP 网关连通性
curl -v http://pop.apsarastack.com/ping
# aliyun CLI 调用示例（指定专有云 endpoint）
aliyun ecs DescribeInstances \
  --RegionId cn-apsara-local \
  --endpoint ecs.aliyuncs.com
# 查询特定租户配额
aliyun pop-cli GetQuota --OwnerId 12345
```

---

## 3. 多租户资源隔离与配额

专有云通过 **组织（Organization）** 和 **资源集（Resource Group）** 实现多租户能力。

### 3.1 配额管理（Quota）

| 配额类型 | 内容 | 常见瓶颈 |
|----------|------|----------|
| **计算配额** | CPU 核心数、内存容量、ECS 实例数 | ESS 扩容失败（`InsufficientCapacity`） |
| **存储配额** | ESSD 容量、OSS Bucket 数、NAS 容量 | PVC 创建失败 |
| **网络配额** | EIP 数量、SLB 实例数、安全组规则数 | Service LB 创建失败 |
| **日志配额** | SLS Project/Logstore/Shard 数 | 日志写入失败 |

> **运维要点**：ACK 扩容失败最常见的原因是底层配额不足，需在 ASOP「资源管理」模块手动扩充。

### 3.2 配额规划建议

| 业务规模 | ECS 配额 | 存储配额 | SLB 配额 |
|----------|----------|----------|----------|
| 小型（<20 节点） | 50 vCPU | 2TB | 10 |
| 中型（20-100 节点） | 300 vCPU | 10TB | 30 |
| 大型（>100 节点） | 1000+ vCPU | 50TB+ | 100+ |

> 建议配额预留 50% 冗余，应对弹性伸缩与突发需求。

---

## 4. 自动化巡检与监控链路

### 4.1 监控集成逻辑

| 层级 | 采集方 | 内容 |
|------|--------|------|
| 基础监控 | 天目（Tianmu）Agent | 物理服务器指标（CPU/内存/磁盘/温度） |
| 云产品监控 | POP 接口抓取 | SLB/RDS/ACK 健康状态 |
| 告警分发 | 天目告警 | Webhook、邮件、短信（需对接短信网关） |

### 4.2 巡检清单（ASOP 运维节点）

```bash
# 🟢 低风险：只读
# 1. POP 网关连通性
curl -v http://pop.apsarastack.com/ping
# 2. 租户配额使用率（CLI）
aliyun pop-cli GetQuota --OwnerId 12345
# 3. 天目告警大盘（控制台）：无未确认告警
# 4. 天基运维大盘：底座组件全绿
# 5. ASO 变更中心：近期变更无异常
```

### 4.3 告警分发配置

| 渠道 | 配置 | 适用 |
|------|------|------|
| Webhook | 自建告警系统 | 主要 |
| 邮件 | SMTP 网关 | 辅助 |
| 短信 | 对接客户短信网关 | 紧急 |

---

## 5. 常见 POP 错误码与处理

| 错误码 | 含义 | 建议行动 |
|:---|:---|:---|
| `QuotaExceeded` | 配额超出限制 | 在 ASOP 调优租户 Quota（计算/存储/网络） |
| `InvalidAccessKeyId` | AK 错误或已禁用 | 检查 RAM 控制台密钥状态；重新生成 |
| `ResourceBusy` | 资源正在变更中 | 等待上一个操作完成（如 ECS 正在停止） |
| `Forbidden.Unauthorized` | 权限不足 | 在 RAM 赋予对应角色权限 |
| `InternalError` | 内部错误（多为底座） | 查天基大盘；联系 TAM |
| `ServiceUnavailable` | 服务暂不可用 | 查对应云产品/底座健康；重试 |
| `Throttling` | 接口限流 | 降低调用频率；申请提升 QPS |
| `InvalidParameter` | 参数错误 | 核对 API 文档；专有云参数可能与公有云不同 |

---

## 6. 运维最佳实践

1. **配额前置规划**：业务上线前在 ASOP 预分配充足配额，留 50% 冗余
2. **AK 管理**：使用 RAM 子账号 + 最小权限；定期轮换 AK；禁用主账号 AK
3. **POP 限流**：高频调用注意 `Throttling`，合理设置重试与退避
4. **endpoint 区分**：专有云 endpoint 与公有云不同，SDK 配置务必使用专有云地址
5. **审计对接**：ASOP 操作审计投递 SLS/SIEM，满足合规留存
6. **定期巡检**：定期巡检配额水位、POP 网关健康、天目告警

---

## 7. 何时联系 TAM / 驻场

| 场景 | 处理方 |
|------|--------|
| POP 网关自身异常（全产品 API 失败） | TAM + 驻场 |
| 配额无法扩容（底层资源池不足） | TAM 评估物理扩容 |
| AK/根账号异常 | 阿里云安全团队 |
| 天基/ASOP 自身故障 | 驻场工程师 |

---

## 相关文档

- [[18-云厂商/01-阿里云/专有云-Apsara/005-apsara-tianji-aso-operations.md|253 天基/ASO 运维流程]]
- [[18-云厂商/01-阿里云/专有云-Apsara/002-apsara-stack-ess-scaling.md|250 ESS 弹性伸缩]]
- [[18-云厂商/01-阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]
- [[18-云厂商/01-阿里云/专有云-Apsara/01-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]

## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]]
- [[17-系统基础/05-速查卡/k8s.md|k8s]]

<!-- risk-assessed -->
