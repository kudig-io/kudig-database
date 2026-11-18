---
id: INC-20250612-GCP-SERVICECONTROL
title: Google Cloud 全球中断（Service Control 空指针崩溃循环）
company: Google Cloud
company_type: cloud-native
domain: cloud-infrastructure
date: 2025-06-12
duration_minutes: 600
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [null-pointer, feature-flag-gap, error-handling, global-replication, crash-loop, quota-policy]
status: published
last_updated: 2026-07-29
sources:
  - https://status.cloud.google.com/incidents/ow5i3PPK96RduMcb1SsW
---

# Google Cloud 全球中断（2025-06-12）：一个空字段引爆全球 API 网关

## 摘要

2025 年 6 月 12 日 10:45 PDT 起，Google Cloud 发生**全球性**重大中断：承担所有 API 请求准入（鉴权、配额、策略检查）的核心组件 **Service Control** 在全球各区域陷入崩溃循环，**50 多个 GCP 服务**（IAM、BigQuery、Cloud Storage、GKE、Vertex AI 等）在全球范围返回 503，Cloudflare（依赖 GCS 的 Workers KV）、Spotify、Discord 等大量互联网服务连带故障。根因链条极具教科书性：5 月 29 日部署的**配额策略新功能代码路径缺少错误处理，且未挂特性开关（feature flag）**；6 月 12 日一条**含空白字段的配额策略数据**被写入并**秒级复制到全球所有区域的 Spanner 数据表**——每个区域的 Service Control 读到该数据后触发**空指针崩溃**，全球同时倒下。红队 2 分钟内触达、10 分钟定位根因、25 分钟启用"红色按钮"（kill switch）止血，大部分区域 2 小时 40 分钟内恢复；但 us-central1 因**恢复风暴（herd effect）压垮底层 Spanner**，拖到约 10 小时才完全恢复。Google 发布了罕见直白的道歉与复盘，承诺全面整改：强制特性开关、模块化防御性编程、全球元数据传播加入验证与渐进放量、错峰退避。

## 影响评估（CRE 视角）

- **影响面**：全球所有 GCP 区域 50+ 服务 API 报 503；下游波及 Cloudflare、Spotify、Discord 等全球互联网服务
- **影响时长**：主要影响约 3 小时，us-central1 完全恢复约 10 小时（10:45 PDT – 20:49 PDT 事件关闭）
- **次生影响**：Cloudflare 同日发布独立复盘（Workers KV 对 GCS 的隐式依赖暴露）；行业再次审视"云上云"依赖链
- **对外沟通评估**：优秀——复盘含逐分钟时间线、明确根因（无错误处理、无特性开关）与整改承诺，措辞罕见地直接认责
- **定级依据**：全球性平台级中断并大规模外溢，SEV-1

## 时间线（太平洋时间，2025-06-12）

| 时间 | 事件 | 证据 |
|---|---|---|
| 05-29 | 含缺陷的配额策略新代码路径随发布上线——无错误处理、无特性开关，从未在生产被真实数据触发 | E1 |
| 06-12 10:45 | 含空白字段的配额策略数据写入，经 Spanner 秒级全球复制 | E1 |
| 10:47-10:51 | 全球各区域 Service Control 读到该数据触发空指针，进入崩溃循环，API 大面积 503 | E1 |
| 10:47 | SRE 红队 2 分钟内介入 | E1 |
| ~10:55 | 定位根因（10 分钟） | E1 |
| ~11:10 | 启用"红色按钮"禁用故障代码路径，开始全球止血（25 分钟） | E1 |
| ~13:25 | 大部分区域恢复（约 2h40m） | E1 |
| 下午-晚间 | us-central1 恢复流量风暴压垮 Spanner，需限流+错峰重启，恢复被拉长 | E1 |
| 20:49 | 全部区域恢复，事件关闭（约 10 小时） | E1 |

**关键时间指标**：TTD = 2min / 根因定位 = 10min / TTM = 25min（红色按钮）/ TTR ≈ 2h40m（多数区域）～10h（us-central1）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Service Control 是 GCP API 管理的准入层：每个 API 请求都要经它做鉴权、配额与策略检查，按区域部署但共享**全球复制的 Spanner 策略数据表**——策略需全球秒级一致，这是配额管理的产品需求，也构成了全球同时失效的传播通道。

### 因素三分

- **触发因素（Trigger）**：一条包含空白字段的配额策略数据写入并全球复制。
- **根本原因（Root Cause）**：5 月 29 日上线的新代码路径对空白字段**无错误处理**（空指针崩溃），且**未挂特性开关**——未经真实数据验证的代码直接暴露在全球生产路径上。
- **扩大因素（Aggravating Factors）**：
  1. 策略数据全球秒级复制，坏数据无金丝雀、无渐进放量，全球同时触发；
  2. Service Control 崩溃循环使止血只能靠预置的 kill switch；
  3. 恢复阶段未做错峰退避，us-central1 的重启风暴压垮底层 Spanner，二次事故拉长恢复；
  4. 下游服务（如 Cloudflare Workers KV）对 GCS 的隐式强依赖放大了互联网级影响。

- **减轻因素（Mitigating Factor）**：GCP 其他区域未受影响；Service Control 恢复后下游服务自动恢复。

### 5 Whys

```
现象：全球 50+ GCP 服务 API 503，最长区域 10 小时恢复
Why1 → 各区域 Service Control 崩溃循环，API 准入全部失败
Why2 → 读到含空白字段的配额策略数据触发空指针
Why3 → 新代码路径无错误处理，坏数据直接崩溃而非拒绝
Why4 → 该功能未挂特性开关、未经真实生产数据渐进验证
Why5 → 全球秒级复制的元数据通道没有"数据即变更"的防护（校验/金丝雀/放量）
        （系统性原因：数据面变更未享受代码变更同级的安全部署待遇）
```

## 解决过程

红队 2 分钟触达、10 分钟锁定根因、25 分钟按下预置的"红色按钮"全局禁用故障代码路径；多数区域 2 小时 40 分钟恢复。us-central1 因大量客户端与内部任务同时重连形成 herd effect，压垮 Spanner，团队通过限流、错峰重启与容量调配在约 10 小时后完全恢复。整改承诺：所有关键路径新功能强制特性开关并默认关闭、防御性编程审计、全球元数据传播增加校验与渐进放量、客户端与平台侧强制随机指数退避。

## 经验教训

1. **数据就是变更**：全球复制的元数据/策略/配置必须享受金丝雀、校验、渐进放量——"秒级全球一致"的产品需求与"渐进安全发布"必须同时满足（与 Cloudflare 2025-11、CrowdStrike 2024 同款教训）。
2. **特性开关是准入门槛而非可选项**：关键路径代码无 flag = 无法快速止血；本次 25 分钟 TTM 全靠预置 kill switch。
3. **无错误处理的代码路径 = 定时炸弹**：对不可信输入 crash-fast 是错误策略，准入层必须 fail-safe（拒绝单条坏数据而非进程崩溃）。
4. **恢复风暴是第二次事故**：大规模恢复必须内建错峰与退避，否则底层存储会被"恢复"打垮。
5. 依赖 GCP 的服务商学到：**云上服务的隐式依赖（如 Workers KV→GCS）需要显式化并配独立降级路径**。

## 预防与改进措施

- **预防（Prevent）**：关键路径强制特性开关；防御性编程与坏数据注入测试
- **减小爆炸半径（Contain）**：全球元数据传播的分区渐进放量与坏数据熔断
- **快速检测（Detect）**：准入层崩溃循环的全局聚合告警（本次 TTD 2 分钟已达标）
- **快速恢复（Recover）**：预置 kill switch；恢复流量错峰退避与底层存储保护性限流

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | TTD 2min/TTM 25min 展示了顶级应急能力，但架构层（全球数据通道无防护）一票否决了一切流程优秀 |
| CRE | 一次故障同时打击 GCP 客户与"客户的客户"（Cloudflare→其客户），云依赖链的信任传导成为 CRE 新课题 |
| FDE | 复盘精确到"5/29 部署、6/12 首次被真实数据触发"，展示了潜伏缺陷的激活时间线取证方法 |
| SA（客情危机） | 全球 50+ 服务中断，鉴权核心组件崩溃循环；Google 25 分钟 TTM 靠预置 kill switch 快速止血，但'鉴权=全局单点'的现实令企业客户对平台关键路径依赖加深警惕 |
| SA（技术危机） | 特性开关是准入门槛而非可选项——客户应关注平台级鉴权依赖并设计降级；关键路径代码无 flag=无法快速止血，客户对供应商的 kill switch 能力应有评估 |

## 参考资料

1. [Google Cloud Service Health Incident Report: ow5i3PPK96RduMcb1SsW（官方复盘）](https://status.cloud.google.com/incidents/ow5i3PPK96RduMcb1SsW) — E1
