---
title: Troubleshooting Lab Exam
description: '- 需要完整记录排查步骤、命令、修复和验证过程'
summary: '- 需要完整记录排查步骤、命令、修复和验证过程'
category: skills
tags:
- k8s
- troubleshooting
- skill
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Troubleshooting Lab Exam 是什么
- 如何 Troubleshooting Lab Exam
- Troubleshooting Lab Exam 故障排查
- Troubleshooting Lab Exam 排障步骤
trigger_keywords:
- Troubleshooting
- Lab
- Exam
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Troubleshooting Lab Exam

### 考核说明

- 每个场景满分 100 分，总分 200 分
- 场景随机抽取，考核过程中可查阅文档
- 需要完整记录排查步骤、命令、修复和验证过程
- 最终需要能复现问题修复

---

### 已知信息

- 命名空间：`production`
- Pod 名称：`payment-api-7d9f8b5c6-x2kqm`
- 镜像：`payment-[[Service|service]]:v1.2`
- 资源请求：`cpu: 2, memory: 4Gi`

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 评分标准

| 场景 | 分值 | 评分要点 |
|---|---|---|
| 问题识别 | 20 | 能否快速定位故障类型 |
| 诊断过程 | 30 | 命令使用正确性、排查思路 |
| 修复方案 | 30 | 修复命令正确、风险意识 |
| 预防措施 | 20 | 监控告警、流程改进建议 |

## 实验环境准备

```bash
# 🟢 创建故障场景
kubectl apply -f broken-deployment.yaml  # CrashLoopBackOff
kubectl apply -f broken-service.yaml     # Service 无法访问
kubectl apply -f broken-networkpolicy.yaml  # 网络不通
# 🟢 验证故障
kubectl get pods -o wide
kubectl get events --sort-by='.lastTimestamp'
```

## 面试要点

1. **Q：故障排查的第一步是什么？**
   A：观察现象(kubectl get pods/events)→确定影响范围→形成假设→验证假设→修复→确认。

2. **Q：如何快速定位 Pod 启动失败原因？**
   A：kubectl describe pod(事件)→kubectl logs(容器日志)→kubectl logs --previous(上次崩溃日志)→检查资源/配置/依赖。

3. **Q：网络不通的排查顺序？**
   A：Pod内(curl localhost)→同节点Pod→跨节点Pod→Service→DNS→Ingress→外部。每层用 tcpdump 验证。

## Related

- [[技能/集群运维/cluster-upgrade/reference/skill-reference-version-matrix.md|skill-reference-version-matrix]] — Version Matrix
- [[技能/安全/rbac/诊断排障/ts-security-auth.md|ts-security-auth]] — 安全认证故障排查
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[技能/节点/node/skill-notready/skill-k8s-node-notready-USAGE-GUIDE.md|skill-k8s-node-notready-USAGE-GUIDE]] — Usage Guide
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
