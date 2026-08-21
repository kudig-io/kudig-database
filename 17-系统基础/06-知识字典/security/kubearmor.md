---
title: KubeArmor 运行时安全
description: KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security
  Modules）为 ...
summary: KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security Modules）为
  ...
category: dictionary
tags:
- k8s
- glossary
- security
- runtime
- ebpf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeArmor 运行时安全 是什么
- KubeArmor 详解
trigger_keywords:
- KubeArmor 运行时安全
- KubeArmor
- dictionary
prerequisites:
- kubernetes
---



# KubeArmor 运行时安全（KubeArmor）

## 概述

KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security Modules）为 Kubernetes 提供运行时安全策略，限制容器的文件/网络/进程行为。

## 核心概念/原理

- **eBPF + LSM**：在内核层拦截容器的系统调用
- **运行时策略**：限制容器可访问的文件/网络/进程
- **CNCF Sandbox**：Accuknox 主导
- **可视化**：提供安全事件的可视化和告警

## 关键机制或特性

- KubeArmorPolicy CRD 定义安全策略
- 文件访问控制（读写/执行限制）
- 网络访问控制（出站/入站限制）
- 进程执行控制（允许/拒绝列表）
- AppArmor/SELinux/BPF-LSM 后端
- 安全事件日志和告警
- KubeArmor VM（非 K8s 环境支持）

## 使用场景与最佳实践

- 容器运行时的安全加固
- 最小权限原则的强制执行
- 合规要求下的运行时安全策略
- 零信任架构中的工作负载保护
- 安全审计和合规报告

## 架构深度解析

### KubeArmor 运行时防护架构

```
┌──────────────────────────────────────────────────────────────┐
│  Kubernetes API Server                                       │
│   │  SecurityPolicy CRD（部署/更新）                          │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ KubeArmor Controller（Deployment）                      │  │
│  │ ├─ 监听 SecurityPolicy CRD                              │  │
│  │ └─ 下发策略到各节点 Agent                               │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ gRPC 策略下发                 │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ KubeArmor Enforcer（DaemonSet，每节点）                  │  │
│  │ ├─ LSM 后端：AppArmor / SELinux / BPF-LSM               │  │
│  │ ├─ 进程级策略执行（文件/网络/系统调用）                  │  │
│  │ └─ 事件采集（允许/阻止/审计）                           │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 事件流                        │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 可观测性出口（Falco/ELK/Prometheus/Kafka 等）            │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubearmor/KubeArmor）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| 控制器 | KubeArmorController/ | CRD 监听与策略分发 |
| 强制器 | KubeArmor/core/ | LSM/BPF 策略执行 |
| 事件引擎 | KubeArmor/core/ | 安全事件采集与导出 |
| 策略模型 | pkg/KubeArmorPolicy/ | 策略 CRD 定义 |
| CLI | KubeArmor/ | kubearmor-cli 调试工具 |

### 流程步骤

1. 用户创建 KubeArmorPolicy（选择器匹配 Pod，声明允许/禁止的文件、网络、syscall）。
2. Controller 监听策略变更，按节点/工作负载分组下发到对应 Agent。
3. Agent 将策略编译为 LSM/BPF 规则并生效（毫秒级）。
4. 工作负载运行时，违反策略的访问被阻止/审计（按 action 配置）。
5. 事件实时导出到 Falco/ELK/Prometheus，供告警与取证。

## 生产案例

### 案例 1：供应链投毒后门被策略阻断（2024 年运行时防护实战）

| 时间 | 事件 |
|---|---|
| T+0 | 某镜像被植入挖矿后门，启动后尝试连接外部矿池 |
| T+5min | KubeArmor 网络策略（默认拒绝出站白名单）阻断连接，事件告警 |
| T+30min | 安全团队定位恶意进程与镜像，隔离工作负载 |
| T+2h | 全量扫描 registry，下线受影响镜像并补签验证策略 |

- **根因**：镜像供应链被投毒；无出站网络白名单策略。
- **修复命令**（策略下发 + 事件排查）：
```bash
# 🔴 下发默认拒绝出站网络策略（白名单制）
kubectl apply -f network-policy-deny-all.yaml
# 🟢 查看被阻止的安全事件
kubectl -n kube-system logs ds/kubearmor | grep -i blocked
```

### 案例 2：策略误拦导致业务进程崩溃

- **现象**：部署文件系统白名单策略后，应用启动失败：`permission denied`。
- **诊断**：策略只允许 `/app` 读写，未放行 `/tmp`、日志目录；应用启动写日志被阻断。
- **修复**：策略审计模式（audit-only）先观察真实行为，生成允许清单后再 enforce；应用侧路径规范化后回归。

## 对比评测

| 维度 | KubeArmor | Falco | SELinux/AppArmor 原生 |
|---|---|---|---|
| 定位 | 策略强制执行 | 异常检测（告警） | 系统级强制 |
| 执行动作 | 阻止/审计 | 仅告警 | 阻止 |
| K8s 集成 | CRD 原生 | 事件流 | 无 |
| 性能开销 | 低（LSM） | 中（syscall 采样） | 低 |
| 适用 | 工作负载加固 | 威胁检测 | 系统加固 |

- **选型建议**：需要"阻止"能力的工作负载加固选 KubeArmor；威胁检测/取证选 Falco；两者可组合（KubeArmor 执行 + Falco 检测）。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 策略未生效 | 选择器不匹配/Agent 未同步 | `kubectl get ksp`、检查 Agent 日志 |
| 误拦应用 | 白名单缺失路径 | 审计模式观察后补允许规则 |
| 事件缺失 | 导出通道故障 | 检查事件导出配置与 Kafka/ELK 连通 |
| 性能下降 | BPF 规则过多 | 精简策略、分层（先审计后 enforce） |
| Agent 崩溃 | 内核/LSM 不兼容 | 检查内核版本与 LSM 后端选择 |

## 生产部署清单

- [ ] 确认节点内核支持 LSM/BPF 后端，Agent DaemonSet 全节点部署
- [ ] 策略从 audit-only 开始，观察 1-2 周真实行为后转 enforce
- [ ] 网络策略默认拒绝出站（白名单制），文件/syscall 最小允许
- [ ] 事件导出双通道（告警实时 + 归档取证），对接 SIEM
- [ ] 监控策略命中率、误拦率、Agent 健康与事件吞吐

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 策略误拦导致业务大面积中断 | 立即回退策略至 audit-only 或移除，恢复后重审 |
| P1 | 内核/LSM 升级 | 先灰度节点验证 Agent 兼容性，批次升级 |
| P2 | KubeArmor 版本升级 | 测试环境验证策略兼容性后滚动升级 |

## 面试要点

> 以下 Q&A 覆盖 KubeArmor 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：KubeArmor 与 Falco 的本质区别？**
   A：KubeArmor 是"执行者"：基于 LSM（AppArmor/SELinux/BPF）在系统调用层强制阻止违规行为（默认拒绝/白名单）；Falco 是"观察者"：通过内核事件流做异常检测并告警，本身不阻止。生产最佳实践是组合：KubeArmor 阻止已知违规面，Falco 发现未知威胁。

2. **Q：KubeArmor 的策略模型（SecurityPolicy）如何工作？**
   A：策略用 selector 匹配工作负载（namespace/labels），定义允许/禁止的文件路径、网络地址/端口、系统调用等行为，action 可为 Audit/Allow/Block。Controller 监听 CRD 并把策略编译下发到节点 Agent，Agent 经 LSM/BPF 强制执行，实现容器级最小权限。

3. **Q：KubeArmor 生产落地的关键步骤？**
   A：① 兼容性验证（内核/LSM 后端）；② 从 audit-only 采集真实行为基线；③ 白名单策略逐步收紧并 enforce；④ 事件双通道（告警 + 取证）；⑤ 建立误拦应急（一键回退 audit-only）与灰度发布机制，避免策略误伤业务。

## 运维要点

- 策略治理：全策略清单 + 负责人 + 审批，变更走 GitOps。
- 基线先行：新策略一律 audit-only 观察 ≥1 周，再 enforce。
- 性能：BPF 规则数量控制，高流量节点监控事件吞吐与 CPU 开销。
- 兼容性：内核升级前验证 Agent/LSM 兼容，灰度批次升级。
- 告警：阻断事件、误拦率、Agent 心跳、事件导出通道延迟。

## 参考链接

- https://kubearmor.io/
- https://github.com/kubearmor/KubeArmor

## Related

- [[17-系统基础/06-知识字典/security/falco.md|Falco]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA]]
- [[17-系统基础/06-知识字典/security/kyverno.md|Kyverno]]
