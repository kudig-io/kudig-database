---
title: Version Matrix
description: '- 检查 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` 配置'
category: skills
tags:
- k8s
- troubleshooting
- skill
- kubelet
- job
- cronjob
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Version Matrix 是什么
- 如何 Version Matrix
trigger_keywords:
- Version
- Matrix
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Version Matrix

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl debug node/<name>` | 支持，使用 `--image` 指定调试镜像 | 同左 | 新增 `--profile` 参数（GA） | 同左 | 同左 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/healthz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/configz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl top node` (metrics-server) | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get lease -n kube-node-lease` | 支持（v1.17+ GA） | 同左 | 同左 | 同左 | 同左 |
| `crictl` 版本要求 | >=1.28 | >=1.29 | >=1.30 | >=1.31 | >=1.32 |

---

### 9.4 版本相关的诊断注意事项

#### [v1.28+]: GracefulNodeShutdown 默认启用

当节点正在关机时，[[kubelet|kubelet]] 会尝试优雅终止 Pod。在诊断时需注意区分计划关机和异常关机：

- 检查 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` 配置
- 日志中出现 `shutting down gracefully` 不一定是问题
- **诊断影响**: D2.2 中看到 `shutting down gracefully` 日志时，需确认是否为计划内操作

#### [v1.30+]: Node swap support (beta)

可能影响内存压力的判断：

- 如果 `NodeSwap` feature gate 启用且 `swapBehavior: LimitedSwap`，需同时检查 swap 使用情况
- `free -m` 输出中的 Swap 行不再是"异常"信号
- kubelet 的 `--fail-swap-on` 标志在启用 swap 时为 `false`
- **诊断影响**: D1.2 中 MemoryPressure 的计算可能包含 swap 使用量；D2.2 中 `swap is enabled` 日志属于正常信息；D2.5 中需结合 swap 配置判断内存压力

#### [v1.31+]: EventedPLEG 默认启用

- 传统 GenericPLEG 的 relist 操作频率降低，`PLEG is not healthy` 误报减少
- 但如果 EventedPLEG 本身异常，可能出现新的故障模式
- 诊断时需检查 `--feature-gates=EventedPLEG=true` 是否生效
- **诊断影响**: D2.6 中 PLEG 相关日志的解读需考虑 EventedPLEG 的行为差异；RC-008 的诊断逻辑需更新

#### [v1.32+]: nftables kube-proxy 模式 GA

- 使用 nftables 模式时，`iptables -L` 不再显示 kube-proxy 规则
- 需使用 `nft list ruleset` 检查规则
- **诊断影响**: D3.3 中检查 kube-proxy 规则的命令需根据模式调整：
  ```bash
  # iptables 模式（传统）
  iptables -t nat -L KUBE-SERVICES 2>/dev/null | head -5
  
  # ipvs 模式
  ipvsadm -Ln 2>/dev/null | head -10
  
  # nftables 模式（v1.32+ GA）
  nft list ruleset 2>/dev/null | grep -A5 "KUBE-SERVICES"
  ```

---

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[skills/troubleshoot-node-issues.md|troubleshoot-node-issues]] — [[skills/troubleshoot-node-issues.md|Troubleshoot Node Issues]]
- [[skills/FTA Diagnostic Execution Engine.md|[[FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]]] — FTA Diagnostic Execution Engine
- [[skills/skill-23-job-cronjob-failure.md|skill-23-job-cronjob-failure]] — Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation Remediation
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
