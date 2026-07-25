---
title: Ingress 控制器 Pod 异常导致业务访问 404/502
description: 专有云 ACK 集群 Nginx Ingress Controller Pod 异常重启、配置重载失败，导致外部流量访问业务域名时出现 404
  与 502 的工单闭环样本。
summary: 专有云 ACK 集群 Nginx Ingress Controller Pod 异常重启、配置重载失败，导致外部流量访问业务域名时出现 404 与
  502 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- ingress
- nginx-ingress
- '404'
- '502'
- p0
- network
tier: supporting
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:45:00+08:00'
incident_id: INC-2026-ACK-046
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-04
affected_namespace: kube-system
ticket_type: 网络故障 / Ingress 故障
skill_ref:
- '[[domain-10-troubleshooting-diagnostics/技能体系/skill-set/k8s-ingress-gateway/SKILL.md|Ingress
  网关诊断 Skill]]'
- '[[domain-03-networking-traffic/K8s网络核心/25-ingress-monitoring-troubleshooting.md|Ingress
  监控与排障]]'
- '[[domain-02-workloads-applications/核心工作负载/11-pod-lifecycle-events.md|Pod
  生命周期事件]]'
fta_ref:
- 'FTA: Ingress 返回 404/502'
last_updated: 2026-06-26 16:45:00+08:00
duplicate_of: TC-2026-021
status: duplicate
duplication_reason: 与 TC-2026-021 主题重复，内容角度相似，降低 RAG 权重
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Ingress 控制器 Pod 异常导致业务访问 404/502 如何处理
trigger_keywords:
- Ingress
prerequisites:
- kubectl-basics
- k8s-networking
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[concepts/ingress.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户通过云监控告警发现，生产环境 ACK 集群 `ack-zyy-prod-04` 中部署在 `kube-system` 命名空间的 Nginx Ingress Controller Pod 频繁重启，外部用户访问业务域名 `api.order.example.com` 与 `shop.example.com` 时大量返回 **404 Not Found** 与 **502 Bad Gateway**。客户描述如下：

> “今天下午两点左右开始，手机端和 H5 页面很多接口报 502，刷新一下又好了，但过一会儿又不行。我们看了 SLB 状态是正常的，后端指向 ACK Ingress 的 NodePort。kubectl 看 ingress-nginx 的 Pod 有重启，describe 看到 Liveness 探针失败。集群是专有云 ACK，麻烦紧急处理一下。”

受影响命名空间包括 `order-service`、`shop-service`、`payment-service`。当前正值大促预热期，流量较平日上涨约 3 倍，部分用户下单链路出现中断。

## 分类与优先级判定

- **工单类型**：网络故障 / Ingress 控制器故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境集群入口流量异常，直接影响外部用户访问，业务可用性受损。
2. Ingress Controller Pod 频繁重启，属于集群入口层单点故障风险，影响面覆盖多个业务命名空间。
3. 处于大促预热期，流量高位运行，符合“服务不可用”标准，需在 15 分钟内止血。

## 诊断步骤

按“先看控制器状态，再查配置一致性，最后看后端健康度”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 Ingress Controller Pod 状态与重启次数
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o wide
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[0].restartCount}{"\n"}{end}'

# 2. 查看 Pod 事件与探针失败原因
kubectl describe pod -n kube-system -l app.kubernetes.io/name=ingress-nginx | grep -A 30 Events
kubectl get events -n kube-system --field-selector reason=Unhealthy --sort-by='.lastTimestamp' | head -30

# 3. 采集 Ingress Controller 日志，定位 404/502 触发点
kubectl logs -n kube-system -l app.kubernetes.io/name=ingress-nginx --tail=500 | grep -E "404|502|error|upstream|reload" | tail -50

# 4. 检查 Nginx 配置重载是否失败
kubectl logs -n kube-system -l app.kubernetes.io/name=ingress-nginx --tail=200 | grep -i "reload|template|fail|warn"

# 5. 核对 Ingress 资源与后端 Service/Endpoint 映射
kubectl get ingress -A
kubectl get svc -n order-service order-api -o wide
kubectl get endpoints -n order-service order-api

# 6. 检查 Ingress 配置中是否存在路径冲突或注解错误
kubectl get ingress -n order-service order-api-ingress -o yaml | grep -A 20 annotations
kubectl get ingress -n shop-service shop-web-ingress -o yaml | grep -A 20 annotations

# 7. 通过 ACK 控制台查看 SLB 后端组健康状态
ack-cli cluster inspect ack-zyy-prod-04 --module ingress

# 8. 进入 Controller Pod 检查 Nginx 运行时 upstream 状态
kubectl exec -n kube-system -it $(kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].metadata.name}') -- nginx -T 2>/dev/null | grep -A 5 "upstream order-api" | head -30

# 9. 检查 Controller 资源使用是否触发了 OOM
kubectl top pod -n kube-system -l app.kubernetes.io/name=ingress-nginx
```
## 根因分析

通过日志与现场状态确认，Nginx Ingress Controller 的 `nginx-ingress-controller` 容器持续触发 **OOMKilled**，并伴随配置模板渲染异常：

```
[error] 102#102: *12345 upstream prematurely closed connection while reading response header from upstream
[warn] 88#88: unable to create worker process: Cannot allocate memory
[error] 45#45: template: nginx.tmpl: execute failed: error calling eq: incompatible types for comparison
```

根本原因为以下三点叠加：

1. **内存 Limit 配置偏低**：当前 Ingress Controller 的 Deployment 为 `memory: 1Gi`，大促期间连接数与 Ingress 对象数量激增，Nginx worker 进程内存占用超过 Limit，触发 OOM 并被 kubelet 反复重启。
2. **存在非法 Ingress 注解**：`shop-service/shop-web-ingress` 中使用了 `nginx.ingress.kubernetes.io/configuration-snippet` 注入自定义 Lua 片段，片段中存在语法错误，导致 Nginx 在配置重载时部分失败，部分 worker 仍持有旧配置，引发 404。
3. **后端 Pod 健康检查配置不一致**：`order-service/order-api` 的 Service 会话亲和性设置为 `ClientIP`，但业务应用 `/healthz` 探针未正确透传真实客户端 IP，导致 Ingress Controller 将部分请求路由到尚未 Ready 的 Pod，返回 502。

## 修复命令

**第一步：临时扩容 Ingress Controller 副本数，缓解入口压力**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl scale deployment ingress-nginx-controller -n kube-system --replicas=5
```
**第二步：调整 Ingress Controller 内存 Limit 至 4Gi，避免 OOM**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment ingress-nginx-controller -n kube-system --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "4Gi"},
  {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "2Gi"}
]'
```
**第三步：移除导致配置渲染失败的非法 Lua 注解**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl annotate ingress shop-web-ingress -n shop-service nginx.ingress.kubernetes.io/configuration-snippet-
```
若业务确实需要自定义片段，需先修正语法并重新应用：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: shop-web-ingress
  namespace: shop-service
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      proxy_set_header X-Real-IP $remote_addr;
      proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
spec:
  rules:
  - host: shop.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: shop-web
            port:
              number: 80
EOF
```
**第四步：修正 Service 会话亲和性与后端健康检查**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch svc order-api -n order-service --type='json' -p='[
  {"op": "replace", "path": "/spec/sessionAffinity", "value": "None"}
]'
```
同时要求业务方将 `/healthz` 探针从 TCP 改为 HTTP，并在 Deployment 中显式声明：

```yaml
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
    httpHeaders:
    - name: X-Forwarded-For
      value: "127.0.0.1"
readinessProbe:
  httpGet:
    path: /ready
    port: 8080
```

**第五步：强制重载 Ingress Controller 配置并确认无异常**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment ingress-nginx-controller -n kube-system
kubectl rollout status deployment ingress-nginx-controller -n kube-system --timeout=300s
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Ingress Controller Pod 全部 Running 且重启次数不再增长
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o wide
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[0].restartCount}{"\n"}{end}'

# 2. 验证 Nginx 配置语法与 upstream 状态
kubectl exec -n kube-system -it $(kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].metadata.name}') -- nginx -t
kubectl exec -n kube-system -it $(kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].metadata.name}') -- curl -s http://localhost:10246/nginx_status

# 3. 从外部探测业务域名
for i in {1..20}; do
  curl -s -o /dev/null -w "%{http_code}\n" https://api.order.example.com/healthz
  sleep 1
done

# 4. 检查业务 Pod 健康状态
kubectl get pod -n order-service -l app=order-api
kubectl get pod -n shop-service -l app=shop-web

# 5. 查看近 5 分钟 5xx 比例是否回落
kubectl logs -n kube-system -l app.kubernetes.io/name=ingress-nginx --since=5m | grep -c " 50[0-9] "

```
## 回复客户话术

> 您好，经排查，本次业务访问 404/502 的根因是 **Nginx Ingress Controller 内存不足被 OOM 重启**，叠加一条 **Ingress 注解语法错误导致 Nginx 配置重载异常**，以及 **后端 Service 会话亲和性与健康检查配置不一致**。我们已完成以下处置：
>
> 1. 将 Ingress Controller 副本数临时扩容至 5，入口流量已分散；
> 2. 将 Controller 内存 Limit 从 1Gi 提升至 4Gi，避免 OOM 重启；
> 3. 移除并修正了非法 Lua 注解，Nginx 配置已成功重载；
> 4. 调整 `order-api` Service 的会话亲和性为 None，并建议业务方将 HTTP 探针透传真实 IP。
>
> 当前外部探测 `api.order.example.com` 与 `shop.example.com` 已稳定返回 200，5xx 比例已回落至正常水平。建议后续：
> - 为 Ingress Controller 配置基于连接数与内存的 HPA，参考 HPA 最佳实践；
> - 在 CI/CD 中增加 Ingress 注解语法校验，参考 [[domain-03-networking-traffic/K8s网络核心/25-ingress-monitoring-troubleshooting.md|Ingress 监控与排障]]；
> - 配置 Ingress Controller 内存使用率告警：`container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.8` 持续 3 分钟触发 P2 告警。
>
> 如有波动，请随时联系。

## 复盘与沉淀

本次故障集中体现了入口层“一处小配置错误 + 容量不足 + 后端健康检查不一致”叠加后的放大效应。Nginx Ingress Controller 作为集群流量入口，其稳定性高度依赖：

1. **资源容量预留**：Ingress Controller 的内存消耗与连接数、Ingress 对象数、location 块数量成正比。大促前应基于历史峰值预留 2~3 倍余量，并启用 HPA。在阿里云 ACK 专有云环境中，建议将 Ingress Controller 与业务工作负载部署在不同节点池，避免业务突发流量争抢入口控制器资源。
2. **配置变更管控**：自定义 snippet 与 Lua 注入是高风险操作，任何语法错误都会导致 Nginx reload 失败。建议在 GitOps 流程中引入 `ingress-nginx` 配置模板校验，使用 `kubectl apply --dry-run=server` 进行预检。对于频繁变更 Ingress 配置的业务，可考虑在测试集群先执行配置重载验证，观察 `nginx -t` 返回结果。
3. **后端健康检查一致性**：Service 的 `sessionAffinity` 与 Pod 探针需配套设计。若业务探针依赖真实 IP，应在 HTTP Header 中显式透传，避免 Ingress 将流量打到未 Ready Pod。同时建议将 Ingress Controller 的 `proxy-connect-timeout`、`proxy-read-timeout` 与业务实际响应时间对齐，防止后端处理稍慢即触发 502。
4. **可观测性覆盖**：Ingress 入口层应配置完整的访问日志、错误码分布、upstream 健康状态监控。通过 SLS 或 Prometheus 实时采集 `nginx_ingress_controller_requests` 指标，按 status class 聚合，可在 404/502 爆发前提前发现异常趋势。

建议将本案例沉淀为 Ingress 404/502 回复模板，并补充到 FTA 故障树 中。后续在变更窗口期对全量 Ingress 注解进行巡检，重点排查 `configuration-snippet`、`server-snippet`、`modsecurity-snippet`、`auth-snippet` 等高风险注解。对于使用了 `nginx.ingress.kubernetes.io/configuration-snippet` 的 Ingress，建议逐条人工复核语法，并在 CI 中集成 `ingress-nginx` 模板校验工具。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若调整资源后仍出现 OOM 或配置重载失败，需升级至 **ACK 网络组件支持团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-046`
  - 根因：Nginx Ingress Controller OOM + 非法 Ingress 注解 + 后端健康检查配置不一致
  - 影响集群：`ack-zyy-prod-04`
  - 影响命名空间：`order-service`、`shop-service`、`payment-service`
  - 临时修复：扩容副本、提升内存 Limit、移除非法注解、调整 Service 会话亲和性
  - 长期方案：启用 HPA、引入 Ingress 注解语法校验、统一后端探针规范
  - 待跟进：推动业务方修正 `/healthz` 探针配置，完成全量 Ingress 注解巡检

## Related

- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配

```

<!-- risk-assessed -->
