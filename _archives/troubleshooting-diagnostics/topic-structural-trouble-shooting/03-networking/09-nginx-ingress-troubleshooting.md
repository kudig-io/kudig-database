---
title: nginx-ingress-controller 故障排查指南
description: nginx-ingress-controller 故障排查指南，覆盖 Ingress 配置、502/503 错误、TLS 证书、upstream 超时等问题场景
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- nginx
- nginx-ingress
- ingress
- gateway
- tls
- 502
- 503
- upstream
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 后端工程师
estimated_read_time: 10min
intent_queries:
- nginx-ingress 故障排查 是什么
- nginx-ingress 502 错误 排查
- nginx-ingress 503 错误 排查
- nginx-ingress TLS 证书问题 排查
trigger_keywords:
- nginx-ingress
- nginx
- Ingress
- 故障排查
- 502
- 503
- TLS
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# nginx-ingress-controller 故障排查指南

> **适用版本**: nginx-ingress v1.9+ | **最后更新**: 2026-05 | **难度**: 高级

---

## 目录

1. [10 分钟快速诊断](#10-分钟快速诊断)
2. [架构与核心组件](#架构与核心组件)
3. [问题场景与排查步骤](#问题场景与排查步骤)
4. [配置参考](#配置参考)

---

## 10 分钟快速诊断

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 nginx-ingress 状态
kubectl get pods -n ingress-nginx

# 2. 查看 ingress-nginx 日志
kubectl logs -n ingress-nginx <pod> --tail=100 -f

# 3. 检查 Ingress 列表
kubectl get ingress -A

# 4. 检查 Endpoints
kubectl get endpoints -n <namespace>

# 5. 测试后端连通性
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  curl -v http://<service>:<port>/health

# 6. 检查 IngressClass
kubectl get ingressclass nginx
kubectl describe ingressclass nginx

# 7. 检查证书
kubectl get secret -n <namespace>

# 8. 测试配置重载
kubectl exec -it <nginx-pod> -n ingress-nginx -- nginx -t
```
---

## 架构与核心组件

```
┌─────────────────────────────────────────────────────────────┐
│              nginx-ingress-controller 架构                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Kubernetes 资源:                                           │
│  ├── Ingress (路由规则)                                     │
│  ├── IngressClass (控制器标识)                              │
│  ├── Secret (TLS 证书)                                     │
│  └── ConfigMap (nginx 配置)                                 │
│                                                             │
│  nginx-ingress Pod:                                         │
│  ├── nginx-ingress controller (同步 Ingress → nginx.conf)  │
│  └── nginx (反向代理 + Lua 扩展)                            │
│                                                             │
│  nginx 配置层级:                                            │
│  ├── nginx.conf (主配置)                                    │
│  ├── ingress-controller.conf (controller 生成)              │
│  └── ingress.conf (每个 Ingress 生成)                       │
│                                                             │
│  端口:                                                       │
│  ├── 80 (HTTP)                                              │
│  ├── 443 (HTTPS)                                            │
│  └── 10254 (Prometheus 指标 / 健康检查)                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 问题场景与排查步骤

### 场景 1: 请求返回 502 Bad Gateway

**现象**: 客户端请求返回 502

**排查步骤**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 upstream Pod 状态
kubectl get pods -n <namespace> -o wide

# Step 2: 检查 Service Endpoints
kubectl get endpoints <service-name> -n <namespace>

# Step 3: 查看 nginx-ingress 日志
kubectl logs -n ingress-nginx <pod> --tail=100 | grep 502

# Step 4: 测试后端连通性
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  curl -v http://<service>:<port>/

# Step 5: 检查网络策略
kubectl get networkpolicy -n <namespace>

# Step 6: 检查 Pod 健康状态
kubectl describe pod <upstream-pod> -n <namespace> | grep -E "Conditions|Events"

# Step 7: 检查 upstream 配置
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  cat /etc/nginx/conf.d/<ingress-name>.conf | grep -A10 "upstream"
```
**常见原因**:
- 所有 Endpoints 不可用（Pod 未运行、健康检查失败）
- 网络策略阻止访问
- upstream 超时设置过短

---

### 场景 2: 请求返回 503 Service Temporarily Unavailable

**现象**: 客户端请求返回 503，所有后端不可用

**排查步骤**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查所有 backend Pod 是否 Running
kubectl get pods -n <namespace> -o wide

# Step 2: 检查 Pod 启动状态
kubectl get events -n <namespace> | grep -i liveness

# Step 3: 检查 Service selector 是否正确
kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.selector}'

# Step 4: 检查 endpoints 是否有 IP
kubectl describe endpoints <service> -n <namespace>

# Step 5: 检查 Pod 是否被驱逐 (Evicted)
kubectl get pods -n <namespace> | grep Evicted

# Step 6: 检查 Pod 是否在 Pending (资源不足)
kubectl describe pod <pod> -n <namespace> | grep -E "Conditions|Pending"
```
**常见原因**:
- 所有 backend Pod 未运行
- Service selector 配置错误
- Pod 被调度到无法满足的资源节点

---

### 场景 3: IngressClass 不生效

**现象**: 创建了 Ingress 但未被 nginx-ingress 处理

**排查步骤**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查 IngressClass 配置
kubectl get ingressclass
kubectl describe ingressclass nginx

# Step 2: 检查 nginx-ingress Controller 是否运行
kubectl get pods -n ingress-nginx

# Step 3: 确认 Ingress 的 spec.ingressClassName 与 IngressClass 匹配
kubectl get ingress <name> -n <namespace> -o jsonpath='{.spec.ingressClassName}'

# Step 4: 检查 annotation 是否正确
# 旧方式 (已废弃): kubernetes.io/ingress.class: nginx
# 新方式: spec.ingressClassName: nginx

# Step 5: 查看 controller 日志
kubectl logs -n ingress-nginx <pod> -n ingress-nginx | grep -i "class"

# Step 6: 检查 controller 是否指定了 --ingress-class 参数
kubectl get deployment -n ingress-nginx -o yaml | grep -A5 "args"
```
**常见原因**:
- IngressClass 未创建
- spec.ingressClassName 与 IngressClass name 不匹配
- Controller 未正确配置 --ingress-class

---

### 场景 4: TLS 证书问题

**现象**: HTTPS 请求返回 400 或握手失败

**排查步骤**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 Secret 是否存在且正确
kubectl get secret <secret-name> -n <namespace>

# Step 2: 检查证书有效期
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  openssl x509 -in /etc/nginx/secrets/<secret-name>/tls.crt -noout -dates

# Step 3: 检查证书与 host 是否匹配
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  openssl s_client -connect localhost:443 -servername <host> -showcerts

# Step 4: 检查 secret 挂载到 Pod
kubectl describe pod <nginx-pod> -n ingress-nginx | grep -A5 "Mounts"

# Step 5: 测试 TLS 连接
curl -v --insecure https://<ingress-ip> -H "Host: <host>"

# Step 6: 检查 TLS protocol 版本
# nginx-ingress 默认仅支持 TLS 1.2+
openssl s_client -connect <ingress-ip>:443 -tls1_2
```
**常见原因**:
- 证书过期
- 证书与 host 不匹配
- Secret 未正确创建（缺少 tls.crt 或 tls.key）
- 客户端仅支持 TLS 1.3 但 server 配置不兼容

---

### 场景 5: 路径重写规则不生效

**现象**: 配置了 rewrite-target 但请求仍然 404

**排查步骤**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 annotation 是否正确配置
kubectl describe ingress <name> | grep -i rewrite

# Step 2: 确认 path 使用正则
# annotation: nginx.ingress.kubernetes.io/rewrite-target: /$2
# path: /foo(/|$)(.*)

# Step 3: 查看实际 nginx.conf 中的 location 块
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  cat /etc/nginx/conf.d/<ingress-name>.conf | grep -A20 "location"

# Step 4: 检查 nginx-ingress 日志中的 rewrite 处理
kubectl logs -n ingress-nginx <pod> | grep rewrite

# Step 5: 测试实际重写效果
curl -v http://<ingress-ip>/foo/bar -H "Host: <host>"
# 观察是否重写到 /bar
```
**常见原因**:
- path 未使用正则表达式匹配
- rewrite-target 格式错误
- annotation 与 path 不匹配

---

### 场景 6: 502/503 混合问题 (长连接/Keepalive)

**现象**: 偶发性 502/503，高并发时更频繁

**排查步骤**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 upstream keepalive 配置
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  cat /etc/nginx/conf.d/<ingress-name>.conf | grep -i keepalive

# Step 2: 检查 upstream keepalive connections
# annotation: nginx.ingress.kubernetes.io/upstream-keepalive

# Step 3: 检查 nginx  worker 连接数
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  cat /etc/nginx/nginx.conf | grep worker_connections

# Step 4: 检查 upstream 失败重试配置
# annotation: nginx.ingress.kubernetes.io/proxy-next-upstream-tries

# Step 5: 增加 upstream keepalive 超时
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  curl -X POST "http://localhost:8090/configuration/backend-keepalive-timeout"
```
**常见原因**:
- upstream keepalive connections 耗尽
- 长连接超时设置过短
- worker connections 不足

---

### 场景 7: Prometheus 指标不显示

**现象**: Prometheus 无法抓取 nginx-ingress 指标

**排查步骤**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 metrics 端点
curl http://<nginx-pod>:10254/metrics

# Step 2: 检查 ServiceMonitor 或 PodMonitor
kubectl get servicemonitor -A
kubectl get podmonitor -A

# Step 3: 检查 prometheus 抓取配置
kubectl exec -it <nginx-pod> -n ingress-nginx -- \
  curl localhost:10254/metrics | head -20

# Step 4: 检查 RBAC 权限
kubectl auth can-i get pods --as=system:serviceaccount:prometheus:prometheus

# Step 5: 检查 nginx-ingress 配置
kubectl get configmap -n ingress-nginx ingress-controller-configuration -o yaml
```
**常见原因**:
- metrics 端口未开启
- RBAC 权限不足
- Prometheus 抓取间隔过长

---

## 配置参考

### Ingress 配置示例

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "300"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "300"
    nginx.ingress.kubernetes.io/upstream-keepalive-timeout: "60"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-svc
            port:
              number: 8080
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
```

### 全局配置 ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-controller-configuration
  namespace: ingress-nginx
data:
  proxy-body-size: "50m"
  proxy-read-timeout: "300"
  proxy-send-timeout: "300"
  use-forwarded-headers: "true"
  compute-full-forwarded-for: "true"
  enable-underscores-in-headers: "true"
  large-client-header-buffers: "4 16k"
```

---

## 快速诊断检查清单

| 检查项 | 命令 | 预期结果 |
|:---|:---|:---|
| Pod 状态 | `kubectl get pods -n ingress-nginx` | Running |
| 日志无 Error | `kubectl logs -n ingress-nginx <pod>` | 无 Error |
| Ingress 存在 | `kubectl get ingress -A` | 规则正常 |
| Endpoints | `kubectl get endpoints -A` | 有 IP 列表 |
| 后端连通性 | `curl http://<svc>:<port>/health` | 200 OK |
| TLS 证书 | `openssl s_client -connect <ip>:443` | 证书有效 |
| metrics 端点 | `curl localhost:10254/metrics` | 有指标数据 |

---

## 与 Higress 迁移问题对照

| 问题 | nginx-ingress | Higress 迁移注意 |
|:---|:---|:---|
| 注解方式 | `nginx.ingress.kubernetes.io/*` | 使用标准 Ingress 注解 |
| upstream 超时 | 默认 60s | 需检查 `higress.io/backend-timeout` |
| 灰度发布 | `canary-weight` annotation | 使用 `higress.io/canary-weight` |
| Wasm 插件 | 不支持 | Higress 原生支持 |

---

## 相关文档

- [nginx-ingress 完全指南](./domain-03-networking-traffic/21-nginx-ingress-complete-guide.md)
- [nginx-ingress 迁移指南](./domain-03-networking-traffic/09-nginx-ingress-migration-guide.md)
- [nginx-ingress FTA 故障树](./domain-10-troubleshooting-diagnostics/topic-fta/list/nginx-[[domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta|ingress-fta]].md)
- [nginx-ingress 全局索引](./domain-19-landscape-references/topic-index/nginx-ingress-index.md)
- [Ingress 通用故障排查](./03-service-ingress-troubleshooting.md)

## Related

- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
