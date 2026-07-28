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
tier: peripheral
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

## 生产案例

### 案例 1: Nginx Ingress Controller reload 失败导致配置不更新

| 时间 | 事件 |
|------|------|
| 15:00 | 新增 Ingress 规则不生效，访问 404 |
| 15:05 | `kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx` 显示 "nginx: configuration file test failed" |
| 15:08 | 某 Ingress annotation 包含非法字符导致 nginx.conf 语法错误 |
| 15:12 | 🟡 修复问题 annotation，Controller 自动 reload |

**根因**: `nginx.ingress.kubernetes.io/configuration-snippet` 中注入了非法 nginx 指令。

### 案例 2: Ingress Controller 连接数耗尽导致 502

**现象**: 高并发时大量 502，`kubectl top pod` 显示 CPU 正常。

**诊断**: `kubectl exec ingress-pod -- cat /proc/sys/net/core/somaxconn` 值过低

**修复**: 🟡 调整 worker-connections 和 upstream keepalive 配置

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Ingress Controller 全部不可用 | 检查 Pod 状态 + 快速重启 |
| P1 | 配置不更新 | 检查 nginx.conf 语法 |
| P2 | 性能优化 | 调整 worker/keepalive 参数 |

## 面试要点

1. **Q: Nginx Ingress Controller 的配置更新机制？**
   A: Controller watch Ingress/ConfigMap/Secret 变更 → 生成新 nginx.conf → `nginx -t` 测试 → 成功则 reload(发送 HUP 信号)，失败则保留旧配置。避免无效配置影响流量。

2. **Q: Nginx Ingress 与 Traefik/Envoy Gateway 的对比？**
   A: Nginx: 成熟稳定、annotation 丰富、性能优秀；Traefik: 自动服务发现、原生 Let's Encrypt；Envoy Gateway: Gateway API 原生、xDS 动态配置、可扩展性强。

3. **Q: 如何优化 Nginx Ingress 性能？**
   A: ① 增加 worker-processes(= CPU 核数) ② 调高 worker-connections ③ 启用 upstream keepalive ④ 配置 proxy-buffer-size ⑤ 使用 ConfigMap 全局调优 ⑥ HPA 基于连接数扩容。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|[[19-故障诊断/06-FTA故障树/fta-execution-engine|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[26-技能/03-节点/node/诊断排障/ts-node-components.md|ts-node-components]] — 节点组件故障排查
- [[apiserver-fta]] — [[apiserver-fta|[[19-故障诊断/06-FTA故障树/list/apiserver-fta|API Server 异常故障树分析]]]]
- [[scheduler-fta]] — Scheduler 异常故障树分析
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz-answers.md|assessment-k8s-fundamentals-quiz-answers]] — K8S Fundamentals Quiz Answers
- [[prometheus]] — Prometheus
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
