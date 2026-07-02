---
title: 容器运行时docker daemon僵死导致Pod无法创建
summary: 容器运行时docker daemon僵死导致Pod无法创建：新部署的Pod长期处于ContainerCreating状态，已有Pod不受影响。问题仅出现在特定3个节点上。
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-03-08'
skill: 01-node-notready
severity: P1
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器运行时docker daemon僵死导致Pod无法创建

**日期**: 2026-03-08  
**关联Skill**: [[01-node-notready]]  
**严重级别**: P1

## 场景描述
新部署的Pod长期处于ContainerCreating状态，已有Pod不受影响。问题仅出现在特定3个节点上。

## 时间线
14:00 新部署版本，发现部分Pod卡在ContainerCreating
14:15 kubectl describe pod显示Failed to create container: rpc error: code = Unknown desc = Error response from daemon
14:20 检查节点docker状态：systemctl status docker显示active但docker ps无响应
14:30 检查docker daemon日志：大量goroutine泄漏错误
14:45 确认docker daemon进入僵死状态（进程存在但无响应）
15:00 重启docker daemon：systemctl restart docker
15:05 Pod恢复正常创建
15:30 分析根因：某应用频繁创建/删除容器，触发docker daemon已知bug

## 根因分析
特定应用频繁创建临时容器进行健康检查，触发docker daemon的goroutine泄漏bug（docker 20.10.x已知问题），导致daemon僵死。

## 影响评估
3个节点上约50个新Pod无法调度，滚动更新中断。

## 教训与预防
1. 考虑从docker迁移至containerd（containerd更稳定）
2. 避免频繁创建/删除容器的应用模式
3. 监控docker daemon响应时间，设置告警
## Related

- [[concepts/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[concepts/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]


<!-- risk-assessed -->
