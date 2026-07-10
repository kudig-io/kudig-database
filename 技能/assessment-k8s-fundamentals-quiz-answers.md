---
title: K8S Fundamentals Quiz Answers
description: '### 2. CrashLoopBackOff 排查（10 分）'
summary: '### 2. CrashLoopBackOff 排查（10 分）'
category: skills
tags:
- k8s
- troubleshooting
- skill
- apiserver
- scheduler
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8S Fundamentals Quiz Answers 是什么
- 如何 K8S Fundamentals Quiz Answers
trigger_keywords:
- K8S
- Fundamentals
- Quiz
- Answers
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8S8s 基础知识考核|K8S Fundamentals Quiz]] Answers

### 2. CrashLoopBackOff 排查（10 分）

**参考答案**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 排查步骤：

# 1. 确认 Pod 状态
kubectl get pods -n <namespace> | grep CrashLoopBackOff

# 2. 查看重启次数和退出码
kubectl describe pod <pod-name> | grep -E "Restart Count|Exit Code|Last State"

# 3. 查看容器日志（关键！）
kubectl logs <pod-name> --previous
# 或多容器场景
kubectl logs <pod-name> -c <container-name> --previous

# 4. 可能原因：

# a) 应用配置错误
# - 环境变量缺失
# - 配置文件路径错误
# - 连接字符串错误
kubectl exec -it <pod-name> -- env | grep -E "DATABASE|API|HOST"

# b) 资源不足（OOMKilled）
# - 退出码 137（SIGKILL）
# - 内存 limit 过低
kubectl top pods -n <namespace>

# c) 依赖服务不可达
# - 应用启动需要连接数据库/缓存
# - 连接超时
kubectl exec -it <pod-name> -- nc -zv <service> <port>

# d) 镜像问题
# - 镜像不存在
# - 镜像拉取超时
kubectl describe pod | grep -A5 "ImagePull"

# e) 权限问题
# - Secret/ConfigMap 访问被拒绝
# - RBAC 限制
```
**评分标准**：
- 正确使用 kubectl 命令（4 分）
- 列出所有可能原因（4 分）
- 排查逻辑清晰（2 分）

---

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[技能/ts-node-components.md|ts-node-components]] — 节点组件故障排查
- [[apiserver-fta]] — API Server 异常故障树分析
- [[scheduler-fta]] — Scheduler 异常故障树分析
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/assessment-k8s-fundamentals-quiz.md|assessment-k8s-fundamentals-quiz]] — K8S Fundamentals Quiz


<!-- risk-assessed -->
