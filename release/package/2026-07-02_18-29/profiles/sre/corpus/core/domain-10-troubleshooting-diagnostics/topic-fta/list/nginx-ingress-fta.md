---
title: nginx-ingress-controller 异常故障树分析 (skills)
description: '### 故障排查命令速查'
summary: '### 故障排查命令速查'
category: skills
tags:
- k8s
- fta
- troubleshooting
- apiserver
- scheduler
- prometheus
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- nginx-ingress-controller 异常故障树分析 是什么
- 如何 nginx-ingress-controller 异常故障树分析
trigger_keywords:
- nginx-ingress-controller
- 异常故障树分析
prerequisites:
- kubectl-basics
- prometheus-basics
fta_id: FTA-NGINX_INGRESS-001
component: Nginx Ingress
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[nginx-ingress-fta|nginx-ingress-controller 异常故障树分析]]

### 故障排查命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 nginx-ingress 状态
kubectl get pods -n ingress-nginx

# 2. 查看 ingress-nginx 日志
kubectl logs -n ingress-nginx <pod> --tail=200 -f

# 3. 检查 Ingress 列表
kubectl get ingress -A

# 4. 检查 Endpoints
kubectl get endpoints -n <namespace>

# 5. 测试后端连通性 (在 nginx Pod 内)
kubectl exec -it ingress-nginx/<pod> -n ingress-nginx -- \
  curl -v http://<service>:<port>/health

# 6. 检查证书
kubectl get secret -n <namespace>
kubectl describe secret <tls-secret>

# 7. 检查 IngressClass
kubectl get ingressclass nginx
kubectl describe ingressclass nginx

# 8. 查看 nginx 配置
kubectl exec -it ingress-nginx/<pod> -n ingress-nginx -- \
  cat /etc/nginx/nginx.conf

# 9. 测试配置重载
kubectl exec -it ingress-nginx/<pod> -n ingress-nginx -- \
  nginx -t

# 10. 检查 Prometheus 指标
curl localhost:10254/metrics | grep nginx

# 11. 查看详细的 access log
kubectl exec -it ingress-nginx/<pod> -n ingress-nginx -- \
  tail -f /var/log/nginx/access.log
```
---

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[skills/ts-node-components.md|ts-node-components]] — 节点组件故障排查
- [[apiserver-fta]] — [[apiserver-fta|[[API Server 异常故障树分析|API Server 异常故障树分析]]]]
- [[scheduler-fta]] — Scheduler 异常故障树分析
- [[skills/assessment-k8s-fundamentals-quiz-answers.md|assessment-k8s-fundamentals-quiz-answers]] — K8S Fundamentals Quiz Answers
- [[prometheus]] — Prometheus
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
