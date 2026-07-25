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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

> 完整根因详情见 [reference/root-cause-catalog.md](../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/08-%E6%8A%80%E8%83%BD%E4%BD%93%E7%B3%BB/skill-set/k8s-node-notready/reference/root-cause-catalog.md)
> 完整修复步骤见 [reference/remediation-playbook.md](./reference/remediation-playbook.md)

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## Node NotReady 技能详解

### 技能执行流程

```
1. 确认节点状态
   kubectl get node <node> -o wide
2. 检查节点条件
   kubectl describe node <node> | grep -A10 Conditions
3. 检查 kubelet 状态
   systemctl status kubelet
   journalctl -u kubelet -n 50
4. 检查网络连通性
   ping <node-ip>
   curl -k https://<node-ip>:10250/healthz
5. 检查资源压力
   kubectl describe node <node> | grep -A5 "Allocated resources"
```

### 常见根因及修复

| 根因 | 修复命令 | 风险 |
|---|---|---|
| kubelet 停止 | `systemctl restart kubelet` | 🟡 |
| 磁盘压力 | 清理磁盘/扩容 | 🟡 |
| 内存压力 | 驱逐 Pod/扩容 | 🟡 |
| 网络不通 | 检查 CNI/防火墙 | 🟡 |
| 证书过期 | `kubeadm certs renew all` | 🟡 |

## 面试要点

1. **Q：Node NotReady 的排查思路？**
   A：检查 kubelet→检查网络→检查资源→检查证书→检查 CNI。从简单到复杂逐层排查。

2. **Q：如何预防 Node NotReady？**
   A：监控节点资源、配置告警、定期证书轮转、CNI 健康检查、节点自动修复。

3. **Q：节点故障对 Pod 的影响？**
   A：默认 5 分钟后 Pod 被标记为 Terminating，控制器会在其他节点重建。有状态服务需特殊处理。

## Related

- observability.md|ts-monitoring-observability]] — 监控可观测性排查
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 29-agentscope-studio-skill-demo
- 48-openclaw-skill-mechanism
- SKILL

<!-- risk-assessed -->
