---
title: FTA-FEBM 联合诊断最佳实践
description: 'title: FTA-FEBM 联合诊断最佳实践'
category: febm
tags:
- febm
- troubleshooting
- kubelet
- istio
- envoy
- hpa
- agent
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 25min
intent_queries:
- FTA-FEBM 联合诊断最佳实践 是什么
- 如何 FTA-FEBM 联合诊断最佳实践
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- FTA-FEBM 联合诊断最佳实践 故障排查
- FTA-FEBM 联合诊断最佳实践 排障步骤
trigger_keywords:
- FTA-FEBM
- 联合诊断最佳实践
- troubleshooting
- diagnostics
- febm
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
---

title: FTA-FEBM 联合诊断最佳实践
description: '# FTA-FEBM 联合诊断最佳实践'
category: febm
tags:
- k8s
- forensics
- evidence-based
- methodology
- kubelet
- istio
- envoy
- hpa
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 5min
intent_queries:
- FTA-FEBM 联合诊断最佳实践 是什么
- 如何 FTA-FEBM 联合诊断最佳实践
trigger_keywords:
- FTA-FEBM
- 联合诊断最佳实践
- febm
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# FTA-FEBM 联合诊断最佳实践

> **版本**: v1.0
> **适用场景**: 复杂故障诊断、未知故障模式分析、生产复盘
> **更新日期**: 2026-05-18

---

<!-- chunk: 一、核心概念 -->## 一、核心概念

#<!-- chunk: 1.1 FTA vs FEBM 方法论对比 -->## 1.1 FTA vs FEBM 方法论对比

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FTA (演绎法) vs FEBM (归纳法)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  FTA (Fault Tree Analysis)                                                │
│  ═══════════════════════════════                                         │
│                                                                             │
│  思维模式: "系统可能在哪里出问题？"                                          │
│  起点:     顶事件 (系统级故障)                                             │
│  方向:     自上而下，分解到根因                                             │
│  方法:     假设 → 验证                                                      │
│  知识来源: 预定义的故障树结构                                               │
│  适用:     已知故障模式、架构评审、风险评估                                   │
│                                                                             │
│  示例:                                                                   │
│    TE-2: 应用服务不可用                                                     │
│    └── IE-2.1: Pod运行异常                                                 │
│        └── BE-2.3: OOMKilled                                               │
│            → 已知路径，直接验证                                              │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  FEBM (Forensic Evidence-Based Methodology)                                │
│  ════════════════════════════════════════════════════                      │
│                                                                             │
│  思维模式: "系统实际发生了什么？"                                           │
│  起点:     证据 (日志/指标/事件)                                           │
│  方向:     自下而上，推理到根因                                             │
│  方法:     证据 → 假设 → 验证                                              │
│  知识来源: 实际故障中收集的证据                                             │
│  适用:     未知故障、安全事件、动态环境、事后复盘                             │
│                                                                             │
│  示例:                                                                   │
│    证据: Pod OOMKilled, JVM heap 1.2Gi, limit 1Gi                           │
│    时间线: 16:32:15 内存突增 → 16:32:20 OOM → 16:32:25 Pod重启             │
│    根因: OrderCache.loadAll 内存泄漏 (不在预设 FTA 中)                     │
│    → 发现新故障模式，更新 FTA                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 1.2 何时使用何种方法 -->## 1.2 何时使用何种方法

| 场景 | 推荐方法 | 原因 |
|:---|:---:|:---|
| **已知故障模式** | FTA | 快速匹配，效率高 |
| **常见故障** (Pod OOM、证书过期、网络不通) | FTA | 已有成熟故障树 |
| **新故障/未知故障** | FEBM | 从证据推理，不依赖预设 |
| **多因素复杂故障** | FTA + FEBM 联合 | FTA 提供假设，FEBM 验证证据 |
| **安全事件/取证** | FEBM | 强调证据链完整性 |
| **故障复盘** | FEBM | 时间线重建，因果追溯 |
| **架构评审/风险评估** | FTA | 演绎分析，覆盖已知场景 |
| **快速恢复优先** | FTA | 直接匹配已知路径 |
| **深度分析优先** | FEBM | 探索未知，挖掘根因 |

---

<!-- chunk: 二、联合诊断架构 -->## 二、联合诊断架构

#<!-- chunk: 2.1 联合诊断流程图 -->## 2.1 联合诊断流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FTA + FEBM 联合诊断架构                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐                                                           │
│  │   故障发生   │                                                           │
│  └──────┬──────┘                                                           │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────────────────────────────────────────────────┐           │
│  │  Phase 1: FTA 快速匹配 (5 分钟内)                            │           │
│  │  ════════════════════════════════════════════                │           │
│  │                                                             │           │
│  │  1. 识别顶事件 (TE)                                          │           │
│  │     输入: 故障现象                                          │           │
│  │     输出: 候选 TE 列表 (如 TE-2 应用服务不可用)              │           │
│  │                                                             │           │
│  │  2. FTA 路径匹配                                            │           │
│  │     输入: TE → 遍历 IE → 遍历 BE                            │           │
│  │     输出: 候选根因路径 + 置信度                              │           │
│  │                                                             │           │
│  │  3. 证据收集验证                                            │           │
│  │     输入: 候选 BE                                            │           │
│  │     输出: 每个 BE 的可观测性证据                            │           │
│  │                                                             │           │
│  └─────────────────────────────────────────────────────────────┘           │
│                              │                                              │
│                              ▼                                              │
│         ┌────────────────────┼────────────────────┐                        │
│         │                    │                    │                        │
│         ▼                    ▼                    ▼                        │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐                │
│  │  置信度 > 85% │      │  置信度 50-85% │      │  置信度 < 50%  │                │
│  │  FTA 路径确认 │      │  需要更多验证 │      │  FTA 匹配失败  │                │
│  └──────┬──────┘      └──────┬──────┘      └──────┬──────┘                │
│         │                    │                    │                        │
│         ▼                    ▼                    ▼                        │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐                │
│  │  直接修复    │      │  Phase 2   │      │  Phase 3    │                │
│  │  执行 HA    │      │  FEBM 深度推理 │      │  FEBM 从头推理 │                │
│  └─────────────┘      └──────┬──────┘      └──────┬──────┘                │
│                              │                    │                        │
│                              │                    │                        │
│                              ▼                    ▼                        │
│                    ┌─────────────────────────────┐                       │
│                    │  证据链构建 + 时间线重建       │                       │
│                    │  因果推理 + 根因确认          │                       │
│                    └─────────────────────┬───────────┘                       │
│                                        │                                   │
│                                        ▼                                   │
│                              ┌─────────────────┐                           │
│                              │  根因确认       │                           │
│                              │  修复执行       │                           │
│                              │  FTA 更新(如需要)│                           │
│                              └─────────────────┘                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 2.2 联合诊断决策树 -->## 2.2 联合诊断决策树

```
故障发生
    │
    ├─► FTA 快速匹配
    │       │
    │       ├─► 找到匹配路径 + 置信度 > 85%?
    │       │       │
    │       │       ├─► 是 → 执行 FTA 修复 → 完成
    │       │       │
    │       │       └─► 否 → 继续
    │       │
    │       └─► 未找到匹配路径 → 转到 FEBM
    │
    ├─► 置信度评估 (50-85%)
    │       │
    │       ├─► 收集更多证据验证 FTA 假设
    │       │       │
    │       │       ├─► 证据确认假设 → 执行 FTA 修复 → 完成
    │       │       │
    │       │       └─► 证据否定假设 → FEBM 深度推理
    │       │
    │       └─► 证据不足以判断 → FEBM 深度推理
    │
    └─► 置信度 < 50% 或 FTA 完全不匹配
            │
            └─► FEBM 从头推理
                    │
                    ├─► 证据链完整 → 确认根因 → 修复 + 更新 FTA
                    │
                    └─► 证据链不完整 → 保留为待验证假设 → 人工介入
```

---

<!-- chunk: 三、实战案例 -->## 三、实战案例

#<!-- chunk: 3.1 案例 1：HPA 扩容后新型故障（FTA+FEBM 联合） -->## 3.1 案例 1：HPA 扩容后新型故障（FTA+FEBM 联合）

**故障现象**:
- 部分用户登录超时（不是全部用户）
- 持续时间约 3 分钟
- 影响范围：华东 1 区

**Phase 1: FTA 快速匹配**

```
输入: "部分用户登录超时"
匹配 TE: TE-2 应用服务不可用

FTA 路径遍历:
  TE-2 → IE-2.1 Pod运行异常 → BE-2.1 CrashLoopBackOff
  TE-2 → IE-2.1 Pod运行异常 → BE-2.3 OOMKilled
  TE-2 → IE-2.2 Service/Endpoint异常 → BE-2.5 无可用Endpoint

收集证据:
  - kubectl get pods -l app=auth-service -n production
    → 3 个 Pod Running，2 个 Restarting
  - kubectl top pod -n production
    → memory usage 正常，未超过 limit
  - kubectl get endpoints auth-service -n production
    → 3 endpoints，容量充足

FTA 置信度评估:
  - BE-2.1: 60% (Pod 在重启但不全是 CrashLoop)
  - BE-2.3: 30% (内存正常)
  - BE-2.5: 40% (有 endpoints 但时断时续)

结论: FTA 置信度不足，需要 FEBM 深度推理
```

**Phase 2: FEBM 深度推理**

```
收集证据（时间线重建）:
  16:32:15 - HPA 触发，auth-service 从 3 副本扩到 10 副本
  16:32:18 - 新 Pod 开始创建
  16:32:20 - 开始出现部分用户登录超时
  16:32:25 - 部分新 Pod 进入 Running但 readinessProbe 失败
  16:35:00 - 手动降低 HPA maxReplicas 到 5
  16:35:30 - 故障恢复

关键证据:
  - 应用日志: "connection pool exhausted"
  - 数据库连接池: max_connections=100
  - 连接数监控: 10 pods × 20 connections/pod = 200 > 100
  - HPA 配置: maxReplicas=20 (过大)

FEBM 根因推理:
  1. HPA 扩容太快，连接池配置未同步调整
  2. 连接池上限固定 100
  3. 10 个新 Pod 瞬间创建 200 个连接
  4. 数据库拒绝超过 100 的连接
  5. 部分请求失败

根因确认:
  - 直接原因: 连接池耗尽
  - 根本原因: HPA 扩容策略与连接池配置不匹配
```

**修复与更新**

```
修复动作:
  1. 立即: 降低 HPA maxReplicas 到 5
  2. 短期: 增加连接池 max_connections 到 200
  3. 长期: 配置 HPA 扩容时同步调整连接池

FTA 更新:
  - 新增 BE-2.3.3: HPA 扩容后连接池配置未同步
  - 更新 BE-2.3.2: 增加根因描述

FEBM 案例存档:
  - ID: FEBM-case-INC-2026-0318
  - 症状: 部分用户登录超时
  - 根因: HPA 扩容 + 连接池配置不匹配
  - 证据链: 完整
  - 建议: 更新 FTA + 配置协同机制
```

---

#<!-- chunk: 3.2 案例 2：ASM Istio 未知故障模式（FEBM 主导） -->## 3.2 案例 2：ASM Istio 未知故障模式（FEBM 主导）

**故障现象**:
- Service A 调用 Service B 出现偶发性超时
- 重试后通常成功
- 告警显示 "xDS 配置推送延迟"

**Phase 1: FTA 快速匹配**

```
匹配 TE: TE-10 ASM 服务网格故障

FTA 路径遍历:
  TE-10 → IE-10.1 数据面故障 → BE-10.1 Envoy 资源耗尽
  TE-10 → IE-10.2 控制面故障 → BE-10.3 Istiod 配置推送失败
  TE-10 → IE-10.3 流量管理故障 → BE-10.5 灰度发布异常

收集证据:
  - kubectl get pods -n istio-system -l app=istiod
    → istiod Running，1 restart
  - kubectl logs istiod-xxx -n istio-system --tail=100
    → "OOMKilled" 发现！
  - kubectl top pod istiod-xxx -n istio-system
    → memory 接近 limit

FTA 判断:
  - BE-10.3.1: xDS 配置推送失败 (Istiod OOM)
  - 但这只是表面原因
  - 需要深挖 "为什么 Istiod OOM"
```

**Phase 2: FEBM 深度推理**

```
证据收集:
  - 16:30:00 - 大量服务同时发布配置
  - 16:30:05 - Istiod 内存开始上涨
  - 16:30:30 - 内存达到 limit，OOMKilled
  - 16:30:31 - Kubelet 重启 Istiod
  - 16:30:45 - Istiod 恢复但配置缓存丢失
  - 16:30:50 - 开始重新推送 xDS，大量请求涌入
  - 16:31:00 - 推送延迟，Envoy 缓存过期，请求超时

根因链:
  1. 直接原因: Istiod OOM
  2. 触发原因: 大量并发配置推送
  3. 根本原因: Istiod 资源配置不足（memory limit 太低）
  4. 关联因素: 配置推送没有限流/背压机制

FEBM 发现的新故障模式:
  - Istiod 在大规模配置推送场景下缺少保护机制
  - 资源配置未考虑峰值场景
  - 缺少配置推送的限流/批处理机制
```

**修复与建议**

```
修复动作:
  1. 立即: 增加 Istiod memory limit 到 4Gi
  2. 短期: 配置 Istiod 资源 requests (预留足够)
  3. 长期: 与阿里云 ASM 团队沟通限流机制

FTA 更新:
  - BE-10.3.1 增加触发条件描述
  - 新增根因: "Istiod 资源配置不足"

建议:
  - 为 Istiod 添加 HPA
  - 阿里云 ASM 应增加配置推送的背压机制
  - 配置推送应支持批处理/压缩
```

---

#<!-- chunk: 3.3 案例 3：Terway ENI 复杂故障（FTA 主导 + FEBM 验证） -->## 3.3 案例 3：Terway ENI 复杂故障（FTA 主导 + FEBM 验证）

**故障现象**:
- 大规模 Pod 调度失败
- 错误: "failed to allocate pod IP: no available IP"
- 影响: 新部署完全失败

**Phase 1: FTA 快速匹配**

```
匹配 TE: TE-9 Terway 网络故障

FTA 路径遍历:
  TE-9 → IE-9.1 ENI 模式故障 → BE-9.1 ENI 多队列压力
  TE-9 → IE-9.1 ENI 模式故障 → BE-9.2 Pod IP 分配失败 → BE-9.2.1 VPC CIDR 子网容量耗尽

检查 VPC CIDR:
  - aliyun vpc DescribeVSwitchAttributes --VSwitchId {vswitch}
    → AvailableIpAddressCount: 50 (远低于正常)

检查 IP 泄漏:
  - kubectl exec terway-xxx -- terway-cli show | grep allocated
    → allocated: 450, running pods: 320
  - 泄漏 IP: 130 个

根因确认 (FTA):
  - BE-9.2.1: VPC CIDR 即将耗尽
  - BE-9.5.1: IP 泄漏导致提前耗尽
```

**Phase 2: FEBM 验证**

```
FEBM 证据:
  - 泄漏 IP 大部分是 3 天前被删除的 Pod
  - Terway GC 配置: gc_interval = 6h
  - 但实际 GC 没有执行（证据：日志中无 GC 记录）
  - 原因: GC 进程因某种原因被跳过

验证结论:
  - FTA 根因正确: IP 泄漏导致 CIDR 提前耗尽
  - FEBM 发现 GC 配置未生效的根本原因: GC 需要重启 Terway 才能触发

修复动作:
  1. 立即: kubectl exec terway-xxx -- terway-cli garbage-collect
  2. 短期: 检查 GC 配置，确保 6h 间隔生效
  3. 长期: 增加 IP 泄漏监控告警
```

---

<!-- chunk: 四、联合诊断检查清单 -->## 四、联合诊断检查清单

#<!-- chunk: 4.1 Phase 1: FTA 快速匹配检查清单 -->## 4.1 Phase 1: FTA 快速匹配检查清单

```
□ 识别顶事件 (TE)
  - 故障影响范围是什么？（单服务/多服务/集群级）
  - 严重程度是什么？（P0/P1/P2）

□ 遍历 FTA 路径
  - 列出所有可能的 TE → IE → BE 路径
  - 每个 BE 的可观测性证据是什么？

□ 证据收集
  - 收集候选 BE 的 metrics/logs/events
  - 评估每个 BE 的置信度

□ 决策
  - 置信度 > 85% → 执行 FTA 修复
  - 置信度 50-85% → 继续收集证据
  - 置信度 < 50% → 转到 FEBM
```

#<!-- chunk: 4.2 Phase 2: FEBM 深度推理检查清单 -->## 4.2 Phase 2: FEBM 深度推理检查清单

```
□ 时间线重建
  - 故障开始时间？
  - 关键事件时间点？
  - 故障持续时间？

□ 证据收集
  - metrics: 哪些指标异常？
  - logs: 应用/系统/组件日志
  - events: K8s Events
  - traces: 调用链追踪

□ 因果推理
  - 哪个事件是因，哪个是果？
  - 是否存在共因（Common Cause）？
  - 是否存在级联故障？

□ 根因确认
  - 直接原因？
  - 触发条件？
  - 根本原因？

□ FTA 更新
  - 是否发现新故障模式？
  - 是否需要更新现有 FTA？
  - 新路径的置信度是多少？
```

#<!-- chunk: 4.3 修复与验证检查清单 -->## 4.3 修复与验证检查清单

```
□ 修复执行
  - 修复动作的风险等级？
  - 是否需要人工审批？
  - 回滚方案是什么？

□ 验证
  - 故障是否消除？
  - 关键指标是否恢复正常？
  - 是否引入新问题？

□ 知识沉淀
  - 是否更新 FTA？
  - 是否创建 FEBM 案例？
  - 是否需要更新文档/SOP？
```

---

<!-- chunk: 五、方法论选择决策表 -->## 五、方法论选择决策表

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FTA vs FEBM 选择决策表                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  问题 1: 故障是已知的常见模式吗？                                          │
│  ├─ 是 → 问题 2                                                          │
│  └─ 否 → 使用 FEBM                                                        │
│                                                                             │
│  问题 2: 需要快速恢复还是深度分析？                                        │
│  ├─ 快速恢复 → 使用 FTA                                                   │
│  └─ 深度分析 → 使用 FEBM                                                  │
│                                                                             │
│  问题 3: 证据是否充分且明确？                                             │
│  ├─ 是 → 问题 4                                                          │
│  └─ 否 → 使用 FEBM                                                        │
│                                                                             │
│  问题 4: FTA 置信度是否 > 85%？                                          │
│  ├─ 是 → 执行 FTA 修复                                                    │
│  └─ 否 → 使用 FTA + FEBM 联合                                             │
│                                                                             │
│  ════════════════════════════════════════════════════════════════════════ │
│                                                                             │
│  快速判断:                                                                 │
│                                                                             │
│  场景                                    │ 推荐方法                        │
│  ───────────────────────────────────────┼─────────────────────────────── │
│  Pod OOM (配置了 limits)                │ FTA                            │
│  证书过期                               │ FTA                            │
│  网络不通 (常规)                        │ FTA                            │
│  DNS 解析失败                           │ FTA                            │
│  PVC 挂载失败                           │ FTA                            │
│  ───────────────────────────────────────┼─────────────────────────────── │
│  新故障/未知故障                        │ FEBM                           │
│  多因素复杂故障                         │ FTA + FEBM                     │
│  安全事件/取证                         │ FEBM                           │
│  故障复盘                              │ FEBM                           │
│  HPA 扩容后异常                        │ FTA + FEBM (联合)              │
│  服务网格流量异常                       │ FTA + FEBM (联合)              │
│  Terway/IPVLAN 特有故障                │ FTA + FEBM (联合)              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 六、工具与模板 -->## 六、工具与模板

#<!-- chunk: 6.1 FTA-FEBM 联合诊断记录模板 -->## 6.1 FTA-FEBM 联合诊断记录模板

```yaml
incident_record:
  incident_id: "INC-2026-XXXX"
  timestamp: "2026-05-18TXX:XX:XXZ"
  severity: "P0/P1/P2"
  duration: "X minutes"

  symptom: "故障现象描述"

  phase_1_fta:
    matched_te: "TE-X"
    candidate_paths:
      - path: "TE-X → IE-X.X → BE-X.X"
        confidence: 0.XX
        evidence: [...]
    conclusion: "FTA 置信度不足以确定根因"

  phase_2_febm:
    timeline:
      - time: "16:32:15"
        event: "描述"
        evidence: [...]
    root_cause:
      direct: "直接原因"
      trigger: "触发条件"
      ultimate: "根本原因"
    new_discovery: "是否发现新故障模式"

  fix_applied:
    immediate: "立即修复动作"
    short_term: "短期修复"
    long_term: "长期解决方案"

  fta_update_needed:
    - "需要更新的 FTA 节点"
    - "新路径描述"

  febm_case_created:
    - "FEBM 案例 ID"
    - "证据链存档位置"

  lessons_learned: "经验教训"
```

#<!-- chunk: 6.2 常用命令速查 -->## 6.2 常用命令速查

```bash
# FTA 快速匹配
kubectl get events --sort-by=.lastTimestamp | grep -E "Failed|Crash|OOM|Error"

# FEBM 证据收集
kubectl describe pod <pod> --namespace <ns>
kubectl logs <pod> --previous --tail=100
kubectl top pod <pod> --containers
kubectl get events -n <ns> --field-selector involvedObject.name=<pod>

# 时间线重建
kubectl get events --sort-by=.lastTimestamp -A | grep <pod>
kubectl logs <pod> --timestamps --tail=200

# 根因验证
kubectl exec <pod> -- <diagnostic-command>
```

---

<!-- chunk: 七、总结 -->## 七、总结

#<!-- chunk: 7.1 联合诊断优势 -->## 7.1 联合诊断优势

| 优势 | 说明 |
|:---|:---|
| **效率** | FTA 快速匹配已知路径，减少探索时间 |
| **准确性** | FEBM 深度验证，避免 FTA 误判 |
| **完整性** | 联合使用覆盖已知+未知故障 |
| **知识沉淀** | 发现新故障模式，持续更新 FTA |

#<!-- chunk: 7.2 最佳实践 -->## 7.2 最佳实践

```
1. 优先使用 FTA 进行快速匹配
2. 置信度不足时及时切换到 FEBM
3. 复杂故障使用联合诊断
4. 每次故障后更新 FTA 知识库
5. FEBM 案例沉淀用于未来快速匹配
```

#<!-- chunk: 7.3 适用场景总结 -->## 7.3 适用场景总结

```
FTA 主导:
  - 常见故障（Pod OOM、证书过期、网络不通）
  - 时间紧迫的恢复场景
  - 架构评审和风险评估

FEBM 主导:
  - 未知故障模式
  - 安全事件和取证
  - 深度分析和复盘

FTA + FEBM 联合:
  - 多因素复杂故障
  - HPA/自动扩缩容相关故障
  - 服务网格（ASM/Istio）故障
  - 云厂商特有组件（Terway/ACK-One）故障
  - 任何 FTA 置信度不足的场景
```

---

> **版本**: v1.0
> **维护团队**: SRE Team / Platform Team
> **下次更新**: 每次重大故障后补充新案例

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-febm/MOC.md|topic-febm MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/README.md|topic-febm: FEBM 法医鉴定循证方法论深度解析]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md|第一章：FEBM 方法论原理与理论基础]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md|第二章:FEBM 技术实现体系]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/03-febm-best-practices.md|第三章：FEBM 最佳实践]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/04-febm-agent-ticket-processing.md|第四章：FEBM 对云平台工单智能体托管的意义]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/05-febm-construction-methodology.md|第五章：FEBM 体系建设方法论]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/06-febm-future-evolution.md|第六章：未来演进方向]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/07-febm-appendix.md|第七章:附录]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/08-febm-production-quick-start.md|第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/febm-methodology-deep-dive.md|法医鉴定循证方法论（FEBM）深度解析]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-febm/08-febm-production-quick-start.md|08-febm-production-quick-start]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/febm-methodology-deep-dive.md|febm-methodology-deep-dive]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md|01-febm-theory-foundations]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md|02-febm-technical-implementation]]
