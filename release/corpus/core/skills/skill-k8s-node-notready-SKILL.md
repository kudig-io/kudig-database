---
title: Skill
description: '| RC-006 | 节点与 apiserver 网络不通 | 中 | 网络修复(手动) | HIGH |'
summary: '| RC-006 | 节点与 apiserver 网络不通 | 中 | 网络修复(手动) | HIGH |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- apiserver
- kubelet
- containerd
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Skill 是什么
- 如何 Skill
trigger_keywords:
- Skill
prerequisites:
- kubectl-basics
---



# Skill

### 根因概览 (12 种)

| RC ID | 根因 | 概率 | 首选修复 | 风险 |
|-------|------|------|---------|------|
| RC-001 | [[kubelet|kubelet]] 进程崩溃或未运行 | 高 | REM-003 重启 kubelet | MEDIUM |
| RC-002 | 容器运行时(containerd)异常 | 高 | REM-004 重启 containerd | MEDIUM |
| RC-003 | 磁盘空间耗尽 (DiskPressure) | 高 | REM-002 清理磁盘 | LOW |
| RC-004 | 内存耗尽 (MemoryPressure) | 中 | REM-006 排空重启 | HIGH |
| RC-005 | PID 耗尽 (PIDPressure) | 中 | REM-003 重启 kubelet | MEDIUM |
| RC-006 | 节点与 apiserver 网络不通 | 中 | 网络修复(手动) | HIGH |
| RC-007 | kubelet 客户端证书过期 | 中 | REM-008 证书轮转 | HIGH |
| RC-008 | PLEG 不健康 | 中 | REM-004 重启 containerd | MEDIUM |
| RC-009 | 内核问题/硬件异常 | 低 | REM-007 替换节点 | HIGH |
| RC-010 | NTP 时间不同步 | 低 | 修复 NTP(手动) | MEDIUM |
| RC-011 | CNI 插件异常 | 中 | 重启 CNI Pod(手动) | MEDIUM |
| RC-012 | 节点被手动 cordon | 低 | REM-001 uncordon | LOW |

> 完整根因详情见 [reference/root-cause-catalog.md](./reference/root-cause-catalog.md)
> 完整修复步骤见 [reference/remediation-playbook.md](./reference/remediation-playbook.md)

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- observability.md|ts-monitoring-observability]] — 监控可观测性排查
- [[entities/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 29-agentscope-studio-skill-demo
- 48-openclaw-skill-mechanism
- SKILL