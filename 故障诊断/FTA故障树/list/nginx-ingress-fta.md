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

## 生产案例

### 案例1: Nginx Ingress reload 失败导致配置不生效

**时间线**:
- 10:00 新增 Ingress 规则，但访问返回 404
- 10:05 检查 Ingress Controller 日志: `nginx: [emerg] invalid server name`
- 10:08 确认根因: 某个 Ingress 的 host 字段包含非法字符，导致整个 nginx reload 失败
- 10:12 修复非法 host 后 reload 成功，新规则生效

**根因链**:
```
Ingress host字段含非法字符 → nginx配置生成错误
→ reload失败 → 所有新Ingress规则不生效 → 404
```

**修复**:
```bash
# 🟢 检查 Ingress Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50 | grep -i "emerg\|error\|invalid"
# 🟢 查看当前生效的 nginx 配置
kubectl exec -n ingress-nginx ${POD} -- cat /etc/nginx/nginx.conf | grep server_name
# 🟡 修复非法 Ingress
kubectl patch ingress ${INGRESS} -n ${NS} -p '{"spec":{"rules":[{"host":"correct.example.com"}]}}'
```

### 案例2: Nginx Ingress 连接数耗尽

**现象**: 部分请求超时，nginx error.log 显示 `worker_connections are not enough`

**根因**: 默认 worker_connections 16384，高峰期连接数超过限制

**修复**:
```bash
# 🟡 调整 worker_connections
kubectl edit configmap ingress-nginx-controller -n ingress-nginx
# 添加:
# data:
#   max-worker-connections: "65536"
#   max-worker-open-files: "65536"
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: nginx-ingress-alerts
  rules:
  - alert: NginxIngressReloadFailed
    expr: nginx_ingress_controller_config_last_reload_successful == 0
    for: 5m
    labels:
      severity: critical
  - alert: NginxIngressHighLatency
    expr: histogram_quantile(0.99, rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) > 2
    for: 5m
    labels:
      severity: warning
  - alert: NginxIngressConnectionsHigh
    expr: nginx_ingress_controller_nginx_process_connections{state="active"} > 10000
    for: 5m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 配置变更验证 | CI 中验证 Ingress YAML 合法性 | P0 |
| 连接数规划 | 根据峰值流量调整 worker_connections | P0 |
| 多副本部署 | 至少 2 副本 + Pod 反亲和 | P0 |
| reload 监控 | 监控配置 reload 成功状态 | P1 |

## 面试要点

1. **Q: Nginx Ingress 配置不生效的排查步骤？**
   A: 检查 Controller 日志是否有 reload 错误 → 验证 Ingress 资源状态 → 确认 nginx.conf 是否包含新规则 → 检查后端 Service/Endpoint → 验证网络连通性

2. **Q: Nginx Ingress 性能优化方案？**
   A: 调整 worker_connections/worker_processes → 启用 keepalive → 配置 proxy_buffer → 使用 Lua 插件减少 reload → HPA 自动扩容

3. **Q: Nginx Ingress 与 Envoy 类网关的对比？**
   A: Nginx 配置变更需 reload(有短暂中断)；Envoy xDS 动态更新无中断；Nginx 生态成熟；Envoy 可观测性更强；大规模场景 Envoy 更优

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[技能/ts-node-components.md|ts-node-components]] — 节点组件故障排查
- [[apiserver-fta]] — [[apiserver-fta|[[API Server 异常故障树分析|API Server 异常故障树分析]]]]
- [[scheduler-fta]] — Scheduler 异常故障树分析
- [[技能/assessment-k8s-fundamentals-quiz-answers.md|assessment-k8s-fundamentals-quiz-answers]] — K8S Fundamentals Quiz Answers
- [[prometheus]] — Prometheus
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
