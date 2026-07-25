---
title: nginx-ingress-controller 异常故障树分析
description: nginx-ingress-controller 异常故障树分析，覆盖 Ingress 配置、502/503 错误、TLS 证书、 upstream 超时等问题路径
category: fta
tags:
- fta
- troubleshooting
- nginx-ingress
- ingress
- gateway
- tls
- 502
- 503
- upstream
- rag
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 后端工程师
estimated_read_time: 5min
intent_queries:
- nginx-ingress 异常故障树分析 是什么
- nginx-ingress 502 错误 根因分析
- nginx-ingress 503 错误 故障树
trigger_keywords:
- nginx-ingress
- nginx
- 异常故障树分析
- fta
- 502
- 503
- Ingress
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
fta_id: FTA-NGINX_INGRESS-001
component: Nginx Ingress
severity: medium
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../网络/21-nginx-ingress-complete-guide.md
  label: '核心文档: 21-nginx-ingress-complete-guide'
- type: index
  path: ../../生态参考/topic-index/nginx-ingress-index.md
  label: '索引文档: nginx-ingress-index'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# nginx-ingress-controller 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 nginx-ingress-controller 在生产环境中的流量管理异常。
- **范围**：Ingress 配置、502/503 错误、TLS 证书、upstream 选择、重写规则、Prometheus 指标。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: nginx-ingress 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> ROUTE[Ingress 路由异常]
  OR0 --> UPSTREAM[Upstream 后端异常]
  OR0 --> TLS[TLS/证书异常]
  OR0 --> RESOURCE[资源限制异常]
  OR0 --> CONFIG[配置/参数异常]
  OR0 --> ANNOT[注解兼容性异常]

  %% 路由分支
  ROUTE_OR{{OR}}
  ROUTE --> ROUTE_OR
  ROUTE_OR --> ROUTE1[IngressClass 未指定/不存在]
  ROUTE_OR --> ROUTE2[路径匹配规则错误]
  ROUTE_OR --> ROUTE3[host 不匹配/SNI 问题]
  ROUTE_OR --> ROUTE4[rewrite-target 规则失效]

  %% Upstream 分支
  UPSTREAM_OR{{OR}}
  UPSTREAM --> UPSTREAM_OR
  UPSTREAM_OR --> UPSTREAM1[所有 Endpoints 不可用 (503)]
  UPSTREAM_OR --> UPSTREAM2[单 endpoint 问题导致 502]
  UPSTREAM_OR --> UPSTREAM3[连接超时/读写超时]
  UPSTREAM_OR --> UPSTREAM4[keepalive 连接池耗尽]

  %% TLS 分支
  TLS_OR{{OR}}
  TLS --> TLS_OR
  TLS_OR --> TLS1[证书过期]
  TLS_OR --> TLS2[证书与 host 不匹配]
  TLS_OR --> TLS3[secret 未挂载到 Pod]
  TLS_OR --> TLS4[HTTP/2 ALPN 配置错误]

  %% 资源分支
  RESOURCE_OR{{OR}}
  RESOURCE --> RESOURCE_OR
  RESOURCE_OR --> RESOURCE1[Worker 进程 CPU 100%]
  RESOURCE_OR --> RESOURCE2[内存耗尽 OOM]
  RESOURCE_OR --> RESOURCE3[连接数达到 ulimit]

  %% 配置分支
  CONFIG_OR{{OR}}
  CONFIG --> CONFIG_OR
  CONFIG_OR --> CONFIG1[annotation 格式错误]
  CONFIG_OR --> CONFIG2[ingress 正文过长超过 limit]
  CONFIG_OR --> CONFIG3[nginx.conf 模板冲突]

  %% 注解分支
  ANNOT_OR{{OR}}
  ANNOT --> ANNOT_OR
  ANNOT_OR --> ANNOT1[不支持的 nginx 注解]
  ANNOT_OR --> ANNOT2[注解值语法错误]

  style TE fill:#ff6b6b,stroke:#c92a2a,color:#fff
  style ROUTE fill:#fbbf24,stroke:#d97706,color:#000
  style UPSTREAM fill:#fbbf24,stroke:#d97706,color:#000
  style TLS fill:#fbbf24,stroke:#d97706,color:#000
  style RESOURCE fill:#fbbf24,stroke:#d97706,color:#000
```

---

## 常见问题场景

### 场景 1: 请求返回 502 Bad Gateway

**顶事件**: 客户端请求返回 502

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
诊断路径:
1. 检查 upstream Pod 状态
   kubectl get pods -n <namespace> -l app=<app-name>

2. 检查 Service Endpoints
   kubectl get endpoints <service-name> -n <namespace>

3. 检查 nginx-ingress Pod 日志
   kubectl logs -n ingress-nginx <pod> --tail=100 | grep 502

4. 测试后端连通性
   kubectl exec -it <nginx-pod> -n ingress-nginx -- curl -v http://<service>:<port>/

5. 检查网络策略
   kubectl get networkpolicy -n <namespace>
```
### 场景 2: 请求返回 503 Service Temporarily Unavailable

**顶事件**: 客户端请求返回 503，所有后端不可用

```
# 🟢 低风险：只读/信息收集，通常无副作用
诊断路径:
1. 检查所有 backend Pod 是否 Running
   kubectl get pods -n <namespace> -o wide

2. 检查 Pod 是否在启动中
   kubectl get events -n <namespace> | grep -i liveness

3. 检查 Service selector 是否正确
   kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.selector}'

4. 检查 endpoints 是否有 IP
   kubectl describe endpoints <service> -n <namespace>

5. 检查 Pod 是否被驱逐 (Evicted)
   kubectl get pods -n <namespace> | grep Evicted
```
### 场景 3: IngressClass 不生效

**顶事件**: 创建了 Ingress 但未被 nginx-ingress 处理

```
# 🟢 低风险：只读/信息收集，通常无副作用
诊断路径:
1. 检查 IngressClass 配置
   kubectl get ingressclass
   kubectl describe ingressclass nginx

2. 检查 nginx-ingress Controller 是否运行
   kubectl get pods -n ingress-nginx

3. 确认 Ingress 的 spec.ingressClassName 与 IngressClass 匹配
   kubectl get ingress <name> -n <namespace> -o jsonpath='{.spec.ingressClassName}'

4. 检查 annotation 是否正确
   - kubernetes.io/ingress.class: nginx (旧方式)
   - spec.ingressClassName: nginx (新方式)
```
### 场景 4: TLS 证书问题

**顶事件**: HTTPS 请求返回 400 或握手失败

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
诊断路径:
1. 检查 Secret 是否存在且正确
   kubectl get secret <secret-name> -n <namespace>

2. 检查证书有效期
   kubectl exec -it <nginx-pod> -n ingress-nginx -- \
     openssl x509 -in /etc/nginx/secrets/<secret-name>/tls.crt -noout -dates

3. 检查证书与 host 是否匹配
   kubectl exec -it <nginx-pod> -n ingress-nginx -- \
     openssl s_client -connect localhost:443 -servername <host>

4. 检查 secret 是否挂载到 Pod
   kubectl describe pod <nginx-pod> -n ingress-nginx | grep -A5 "Mounts"
```
### 场景 5: 路径重写规则不生效

**顶事件**: 配置了 rewrite-target 但请求仍然 404

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
诊断路径:
1. 检查 annotation 是否正确配置
   nginx.ingress.kubernetes.io/rewrite-target: /$2

2. 确认 path 使用正则
   path: /foo(/|$)(.*)

3. 检查 nginx-ingress 日志中的 rewrite 处理
   kubectl logs -n ingress-nginx <pod> | grep rewrite

4. 验证 nginx.conf 中的 location 块
   kubectl exec -it <nginx-pod> -n ingress-nginx -- cat /etc/nginx/nginx.conf
```
---

## 故障排查命令速查

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

## Prometheus 关键指标

| 指标 | 说明 | 告警阈值 |
|:---|:---|:---|
| `nginx_ingress_controller_requests` | 总请求数 | - |
| `nginx_ingress_controller_annotations` | annotation 处理数 | - |
| `nginx_ingress_controller_success` | 成功处理数 | - |
| `nginx_ingress_controller_error` | 错误数 | >0 |
| `nginx_ingress_controller_connection_errors` | 连接错误 | >10 |
| `backend_conn_failure` | 后端连接失败 | >0 |

---

## 与 Higress 迁移问题

| 问题 | 原因 | 迁移注意 |
|:---|:---|:---|
| 注解不兼容 | nginx-ingress 特有注解在 Higress 中不支持 | 使用标准 Ingress 注解 |
| upstream 超时不同 | 默认超时值不同 | 迁移前检查并统一配置 |
| 灰度方式不同 | nginx canary vs Higress canary | 使用 Higress 原生方案 |

---

## 相关文档

- [nginx-ingress 完全指南](../../../05-%E7%BD%91%E7%BB%9C/01-K8s%E7%BD%91%E7%BB%9C%E6%A0%B8%E5%BF%83/21-nginx-ingress-complete-guide.md)
- [nginx-ingress 迁移指南](../../../05-%E7%BD%91%E7%BB%9C/04-API%E7%BD%91%E5%85%B3/09-nginx-ingress-migration-guide.md)
- [Ingress 故障排查](../../04-%E9%AB%98%E7%BA%A7%E6%8E%92%E9%9A%9C/structural-03-networking/03-service-ingress-troubleshooting.md)
- [nginx-ingress 全局索引](../../../21-%E7%94%9F%E6%80%81%E5%8F%82%E8%80%83/03-%E9%A2%86%E5%9F%9F%E7%B4%A2%E5%BC%95/nginx-ingress-index.md)

## Related

- [[21-生态参考/03-领域索引/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
