---
title: K8S Fundamentals Quiz
description: '### 7. 当容器探针配置错误导致应用无法启动时，应如何修复？'
summary: '### 7. 当容器探针配置错误导致应用无法启动时，应如何修复？'
category: skills
tags:
- k8s
- troubleshooting
- skill
- kubelet
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8S Fundamentals Quiz 是什么
- 如何 K8S Fundamentals Quiz
trigger_keywords:
- K8S
- Fundamentals
- Quiz
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8S Fundamentals Quiz

### 7. 当容器探针配置错误导致应用无法启动时，应如何修复？

A. 删除 Pod 让 Deployment 重建
B. 修改 Pod 的探针配置并重新 apply
C. 重启 [[kubelet|kubelet]]
D. 修改 Deployment 的探针配置
E. 创建新的 Deployment

### 2. 在生产环境中，发现多个 Pod 出现 `CrashLoopBackOff`，请列出你的排查步骤和可能原因。



## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[skills/assessment-troubleshooting-lab-exam.md|assessment-troubleshooting-lab-exam]] — Troubleshootingbleshooting Lab Exam]]
- [[skills/skill-20-networkpolicy-connectivity.md|skill-20-networkpolicy-connectivity]] — NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
- [[deployment]] — Deployment
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
