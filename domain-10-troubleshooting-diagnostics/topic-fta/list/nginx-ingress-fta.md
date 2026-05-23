---
title: nginx-ingress-controller 异常故障树分析 (skills)
description: '### 故障排查命令速查'
category: skills
tags:
- k8s
- fta
- troubleshooting
- apiserver
- scheduler
- prometheus
- ingress
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
created: "2026-05-23"
---

# [[nginx-ingress-fta|nginx-ingress-controller 异常故障树分析]]

### 故障排查命令速查

```bash
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

- [[skills/FTA Methodology and Core Principles|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[skills/ts-node-components|ts-node-components]] — 节点组件故障排查
- [[apiserver-fta]] — [[apiserver-fta|[[API Server 异常故障树分析|API Server 异常故障树分析]]]]
- [[scheduler-fta]] — Scheduler 异常故障树分析
- [[skills/assessment-k8s-fundamentals-quiz-answers|assessment-k8s-fundamentals-quiz-answers]] — K8S Fundamentals Quiz Answers
- [[prometheus]] — Prometheus
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
