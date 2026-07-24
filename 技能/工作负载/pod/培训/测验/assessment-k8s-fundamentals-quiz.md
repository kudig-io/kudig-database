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

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 评分标准

| 等级 | 分数 | 说明 |
|---|---|---|
| 优秀 | 90-100 | 完全掌握核心概念，能举一反三 |
| 良好 | 75-89 | 掌握主要概念，少量细节需加强 |
| 及格 | 60-74 | 基本理解，需进一步学习 |
| 不及格 | <60 | 需重新学习基础知识 |

## 答题建议

1. **时间分配**：每题平均 2 分钟，难题先跳过
2. **排除法**：先排除明显错误选项
3. **关键词**：注意 "always"、"never"、"must" 等绝对化表述
4. **实践联系**：结合实际运维经验理解概念

## 面试要点

1. **Q：Pod 和 Container 的关系？**
   A：Pod 是最小调度单位，包含一个或多个共享网络/存储的容器。同一 Pod 内容器共享 IP、端口空间、存储卷。

2. **Q：Deployment 和 ReplicaSet 的关系？**
   A：Deployment 管理 ReplicaSet，ReplicaSet 管理 Pod。Deployment 提供滚动更新、回滚等高级功能。

3. **Q：Service 的四种类型？**
   A：ClusterIP(集群内)、NodePort(节点端口)、LoadBalancer(外部负载均衡)、ExternalName(DNS CNAME)。

## Related

- [[技能/工作负载/pod/培训/测验/assessment-troubleshooting-lab-exam.md|assessment-troubleshooting-lab-exam]] — Troubleshootingbleshooting Lab Exam]]
- [[技能/网络/networkpolicy/skill-20-networkpolicy-connectivity.md|skill-20-networkpolicy-connectivity]] — NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
- [[deployment]] — Deployment
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
