---
title: Daily Check Quiz
description: A. 容器配置错误
summary: A. 容器配置错误
category: skills
tags:
- k8s
- troubleshooting
- skill
- pdb
- gpu
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Daily Check Quiz 是什么
- 如何 Daily Check Quiz
trigger_keywords:
- Daily
- Check
- Quiz
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Daily Check Quiz

### Day 1（周一）

**题目**：一个 Pod 处于 `CrashLoopBackOff` 状态，已知容器退出码是 137。以下哪个是最可能的原因？

A. 容器配置错误
B. 内存不足导致 OOM Kill
C. 镜像拉取失败
D. 网络不通
E. 磁盘空间不足

**涉及知识点**：
- Pod 生命周期
- OOM Kill
- 退出码含义

---

### Day 2（周二）

**题目**：你执行 `kubectl get [[Pods|pods]]` 发现某个 Pod 处于 `Pending` 状态超过 5 分钟。执行 `kubectl describe pod` 看到事件 "0/3 nodes are available: 1 Insufficient cpu, 2 node(s) had taint". 以下哪个操作最适合作为第一步？

A. 删除 Pod 重新创建
B. 增加节点资源
C. 检查节点污点和 Pod 容忍配置
D. 重启集群
E. 升级集群版本

**涉及知识点**：
- Pod 调度
- 污点和容忍
- 资源不足

---

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[skills/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[gpu-fta]] — GPU 异常故障树分析
- [[skills/ts-workloads.md|ts-workloads]] — 工作负载故障排查
- [[pdb-fta]] — PDB 异常故障树分析
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
