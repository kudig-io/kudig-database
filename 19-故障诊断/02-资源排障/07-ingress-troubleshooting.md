---
title: 15 - Ingress 故障排查 (Ingress Troubleshooting)
description: 'title: Ingress 故障排查'
summary: 'title: Ingress 故障排查'
category: general
tags:
- k8s
- troubleshooting
- guide
- etcd
- apiserver
- prometheus
- calico
- hpa
- vpa
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 15-ingress-troubleshooting常见问题有哪些？
- 如何排查15-ingress-troubleshooting相关问题？
- 15-ingress-troubleshooting的故障处理方法
trigger_keywords:
- Ingress
- 故障排查
- Ingress
- Troubleshooting
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- cni-basics
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: [[ingress|Ingress]] 故障排查
description: '# 15 - Ingress 故障排查 (Ingress Troubleshooting)'
category: troubleshooting
tags:
- ingress
- nginx
- traefik
- tls
- certificate
- hostname
- ssl
- [[prometheus|prometheus]]
- [[statefulset|statefulset]]
- job
last_updated: 2026-02
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Ingress 404
- Ingress Controller 不工作
- TLS 证书错误
- 域名解析失败
- ingress not found
trigger_keywords:
- Ingress
- 故障排查
- troubleshooting
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/ingress-fta.md
  label: '故障树: ingress'
- type: skill
  path: ../故障诊断/topic-skills/13-ingress-gateway-failure.md
  label: '运维技能: 13-ingress-gateway-failure'
authors:
- name: KUDIG Team
  role: contributor
---

# 15 - Ingress 故障排查 (Ingress Troubleshooting)

---

<!-- chunk: 1. Ingress 故障诊断总览 (Ingress Diagnosis Overview) -->
## 1. Ingress 故障诊断总览 (Ingress Diagnosis Overview)

### 1.1 常见问题现象分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Ingress Controller 未运行** | Pod Pending/CrashLoopBackOff | 所有Ingress失效 | P0 - 紧急 |
| **路由规则不生效** | 404 Not Found | 特定服务不可访问 | P1 - 高 |
| **TLS证书问题** | SSL证书错误/过期 | HTTPS服务异常 | P1 - 高 |
| **负载均衡配置失败** | LoadBalancer Pending | 外部访问中断 | P1 - 高 |
| **后端服务连接失败** | 502 Bad Gateway | 服务调用失败 | P2 - 中 |
| **性能问题** | 响应慢/超时 | 用户体验差 | P2 - 中 |

### 1.2 Ingress 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Ingress 故障诊断架构                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       外部客户端访问                                   │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐                  │  │
│  │  │ Browser │  │ Mobile  │  │ API Client│  │ Monitor │                  │  │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘                  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    负载均衡器 (Cloud LB/Nginx)                        │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │  │
│  │  │   VIP:80    │  │   VIP:443   │  │ Health Check│                  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                 Ingress Controller (Pod副本)                         │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │  │
│  │  │ controller-1│  │ controller-2│  │ controller-3│                  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   配置加载    │    │   路由匹配    │    │   TLS终止    │                   │
│  │ (ConfigMap) │    │ (Rules)     │    │ (Certificates)│                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      后端服务转发                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │ Service A   │    │ Service B   │    │ Service C   │              │  │
│  │  │ (ClusterIP) │    │ (ClusterIP) │    │ (ClusterIP) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Pod终端节点                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A1    │    │   Pod-B1    │    │   Pod-C1    │              │  │
│  │  │   Pod-A2    │    │   Pod-B2    │    │   Pod-C2    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. Ingress Controller 故障排查 (Controller Troubleshooting) -->
## 2. Ingress Controller 故障排查 (Controller Troubleshooting)

### 2.1 排查流程图

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Ingress Controller 故障排查流程                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Ingress服务异常                                                          │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查Ingress Controller Pod状态                │                 │
│   │ kubectl get pods -n ingress-nginx                    │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── Pod未运行 ──▶ 检查Deployment/StatefulSet                      │
│          │                                                                  │
│          ├─── Pod CrashLoopBackOff ──▶ 查看日志                            │
│          │         └─▶ kubectl logs -n ingress-nginx <pod-name>            │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查LoadBalancer服务状态                      │                 │
│   │ kubectl get svc -n ingress-nginx                     │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── ExternalIP为空 ──▶ 检查云提供商配置                          │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查配置和权限                                │                 │
│   │ kubectl describe deploy -n ingress-nginx             │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```
### 2.2 详细诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. 基础状态检查 ==========
# 检查Ingress Controller部署状态
kubectl get deploy,sts -n ingress-nginx
kubectl get pods -n ingress-nginx -o wide

# 检查服务状态
kubectl get svc -n ingress-nginx
kubectl describe svc ingress-nginx-controller -n ingress-nginx

# 检查Ingress资源
kubectl get ingress --all-namespaces
kubectl describe ingress <ingress-name> -n <namespace>

# ========== 2. 日志检查 ==========
# Controller主日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=100

# 如果有多个副本，查看所有日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --all-containers=true

# 实时跟踪日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -f

# ========== 3. 配置检查 ==========
# 检查ConfigMap配置
kubectl get cm -n ingress-nginx
kubectl get cm nginx-configuration -n ingress-nginx -o yaml

# 检查RBAC权限
kubectl auth can-i list services --as=system:serviceaccount:ingress-nginx:nginx-ingress-serviceaccount
kubectl get role,rolebinding -n ingress-nginx

# ========== 4. 网络检查 ==========
# 检查端口监听
kubectl exec -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -- netstat -tlnp

# 检查负载均衡器后端
kubectl get endpoints -n ingress-nginx

# ========== 5. 性能指标 ==========
# 检查资源使用情况
kubectl top pods -n ingress-nginx

# 检查节点资源
kubectl describe nodes | grep -A5 "Allocated resources"
```
### 2.3 常见错误及解决方案

| 错误信息 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `CrashLoopBackOff` | 配置错误/权限不足 | 检查日志，修正配置，确保RBAC权限 |
| `ImagePullBackOff` | 镜像拉取失败 | 检查镜像仓库访问权限和网络 |
| `Pending` | 资源不足/污点限制 | 检查节点资源，添加容忍度 |
| `ExternalIP <pending>` | 云提供商配置问题 | 检查云账号权限和LoadBalancer配置 |
| `Readiness probe failed` | 健康检查端口错误 | 检查探针配置和端口监听 |

---

<!-- chunk: 3. 路由规则故障排查 (Routing Rules Troubleshooting) -->
## 3. 路由规则故障排查 (Routing Rules Troubleshooting)

### 3.1 路由不匹配问题

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. 检查Ingress规则 ==========
kubectl get ingress <ingress-name> -n <namespace> -o yaml

# 检查具体的路由规则
kubectl describe ingress <ingress-name> -n <namespace>

# ========== 2. 验证后端服务 ==========
# 检查Service是否存在
kubectl get svc <service-name> -n <namespace>

# 检查Endpoints
kubectl get endpoints <service-name> -n <namespace>

# 测试Service连通性
kubectl run debug --image=curlimages/curl -it --rm -- sh
# 在容器内执行
curl http://<service-name>.<namespace>.svc.cluster.local:<port>

# ========== 3. Controller配置检查 ==========
# 检查是否启用了特定注解
kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations}'

# 常见注解检查
# nginx.ingress.kubernetes.io/rewrite-target: /
# nginx.ingress.kubernetes.io/ssl-redirect: "true"
# kubernetes.io/ingress.class: nginx
```
### 3.2 路径匹配问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 路径类型检查 ==========
# Exact路径匹配
# path: /api
# pathType: Exact

# Prefix路径匹配  
# path: /api/
# pathType: Prefix

# ImplementationSpecific路径匹配
# path: /api(/|$)(.*)
# pathType: ImplementationSpecific

# ========== 调试技巧 ==========
# 使用临时Pod测试
kubectl run temp-debug --image=busybox -it --rm -- sh
# wget -O- http://ingress-controller-ip/path

# 检查生成的Nginx配置
kubectl exec -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -- cat /etc/nginx/nginx.conf
```
---

<!-- chunk: 4. TLS/SSL证书故障排查 (TLS Certificate Troubleshooting) -->
## 4. TLS/SSL证书故障排查 (TLS Certificate Troubleshooting)

### 4.1 证书管理检查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. Secret检查 ==========
# 检查TLS Secret是否存在
kubectl get secret <tls-secret-name> -n <namespace>

# 查看证书详情
kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout

# 检查证书有效期
kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates

# ========== 2. Ingress TLS配置检查 ==========
kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.spec.tls}'

# 检查TLS主机配置
kubectl describe ingress <ingress-name> -n <namespace>

# ========== 3. 证书更新验证 ==========
# 强制重新加载证书
kubectl delete pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# 检查证书是否生效
echo | openssl s_client -connect <hostname>:443 2>/dev/null | openssl x509 -noout -dates
```
### 4.2 Let's Encrypt证书问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== cert-manager检查 ==========
# 检查Certificate资源
kubectl get certificate --all-namespaces
kubectl describe certificate <cert-name> -n <namespace>

# 检查CertificateRequest
kubectl get certificaterequest --all-namespaces
kubectl describe certificaterequest <cr-name> -n <namespace>

# 检查Order状态
kubectl get order --all-namespaces
kubectl describe order <order-name> -n <namespace>

# ========== ACME挑战检查 ==========
# HTTP01挑战
kubectl get challenge --all-namespaces
kubectl describe challenge <challenge-name> -n <namespace>

# DNS01挑战检查
dig TXT _acme-challenge.<domain>
```
---

<!-- chunk: 5. 性能优化与监控 (Performance Optimization & Monitoring) -->
## 5. 性能优化与监控 (Performance Optimization & Monitoring)

### 5.1 性能指标监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 关键指标检查 ==========
# 请求率和错误率
kubectl exec -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -- curl -s http://localhost:10254/metrics | grep nginx_ingress_controller_requests

# 连接数统计
kubectl exec -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -- curl -s http://localhost:10254/metrics | grep nginx_ingress_controller_connections

# 响应时间
kubectl exec -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -- curl -s http://localhost:10254/metrics | grep nginx_ingress_controller_request_duration

# ========== 资源优化配置 ==========
# 调整worker进程数
kubectl patch cm nginx-configuration -n ingress-nginx -p '{"data":{"worker-processes":"auto"}}'

# 调整连接限制
kubectl patch cm nginx-configuration -n ingress-nginx -p '{"data":{"max-worker-connections":"16384"}}'

# 启用gzip压缩
kubectl patch cm nginx-configuration -n ingress-nginx -p '{"data":{"enable-vts-status":"true","gzip-level":"6"}}'
```
### 5.2 负载测试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 压力测试工具 ==========
# 使用hey进行负载测试
hey -z 30s -c 50 http://<ingress-host>/path

# 使用wrk进行基准测试
wrk -t12 -c400 -d30s http://<ingress-host>/path

# 检查Ingress Controller资源使用
kubectl top pods -n ingress-nginx

# ========== 性能调优建议 ==========
# 调整副本数
kubectl scale deploy ingress-nginx-controller -n ingress-nginx --replicas=3

# 调整资源限制
kubectl patch deploy ingress-nginx-controller -n ingress-nginx -p '{"spec":{"template":{"spec":{"containers":[{"name":"controller","resources":{"requests":{"cpu":"100m","memory":"90Mi"},"limits":{"cpu":"1000m","memory":"1Gi"}}}]}}}}'
```
---

<!-- chunk: 6. 云提供商集成问题 (Cloud Provider Integration) -->
## 6. 云提供商集成问题 (Cloud Provider Integration)

### 6.1 AWS ELB问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== ELB健康检查 ==========
# 检查ELB目标组
aws elb describe-target-groups --load-balancer-arn <lb-arn>

# 检查健康检查配置
aws elb describe-target-health --target-group-arn <tg-arn>

# ========== 注解配置检查 ==========
# Service注解检查
kubectl describe svc ingress-nginx-controller -n ingress-nginx

# 关键注解:
# service.beta.kubernetes.io/aws-load-balancer-type: nlb
# service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
# service.beta.kubernetes.io/aws-load-balancer-ssl-cert: arn:aws:acm:region:account:certificate/cert-id
```
### 6.2 GCP LoadBalancer问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== GCP配置检查 ==========
# 检查ForwardingRule
gcloud compute forwarding-rules list

# 检查BackendService
gcloud compute backend-services list

# 检查HealthCheck
gcloud compute health-checks list

# ========== 注解配置 ==========
# 关键GCP注解:
# kubernetes.io/ingress.global-static-ip-name: <ip-name>
# networking.gke.io/load-balancer-type: "External"
# networking.gke.io/premium-tier: "true"
```
---

<!-- chunk: 7. 紧急处理流程 (Emergency Response Process) -->
## 7. 紧急处理流程 (Emergency Response Process)

### 7.1 快速恢复步骤

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. 立即诊断 ==========
# 快速检查所有关键组件
./ingress-health-check.sh

# ========== 2. 临时绕过方案 ==========
# 直接访问Service (如果NodePort可用)
kubectl get svc -n <namespace> | grep NodePort

# 使用端口转发临时访问
kubectl port-forward svc/<service-name> -n <namespace> 8080:80

# ========== 3. 回滚操作 ==========
# 回滚到之前的Ingress配置
kubectl rollout undo deploy ingress-nginx-controller -n ingress-nginx

# 恢复之前的ConfigMap
kubectl apply -f backup/nginx-configmap.yaml
```
### 7.2 预防措施

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 监控告警配置 ==========
# 配置关键指标告警
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress.rules
    rules:
    - alert: IngressControllerDown
      expr: up{job="ingress-nginx"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Ingress Controller is down"
        
    - alert: HighErrorRate
      expr: rate(nginx_ingress_controller_requests{status=~"5.."}[5m]) > 0.05
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High error rate in Ingress"
EOF

# ========== 定期维护脚本 ==========
# 证书自动续期检查脚本
#!/bin/bash
# check-certificates.sh
kubectl get secrets --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.type}{"\n"}{end}' | grep kubernetes.io/tls | while read namespace secret type; do
    echo "Checking certificate in $namespace/$secret"
    kubectl get secret $secret -n $namespace -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
done
```
---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 故障诊断 MOC
- [[19-故障诊断/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]]
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[19-故障诊断/01-核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[19-故障诊断/01-核心排障/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[19-故障诊断/02-资源排障/01-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[19-故障诊断/02-资源排障/05-certificate-troubleshooting.md|13-certificate-troubleshooting]]
- [[19-故障诊断/02-资源排障/06-pvc-storage-troubleshooting.md|14-pvc-storage-troubleshooting]]
- [[19-故障诊断/02-资源排障/08-networkpolicy-troubleshooting.md|16-networkpolicy-troubleshooting]]
- [[19-故障诊断/02-资源排障/09-hpa-vpa-troubleshooting.md|17-hpa-vpa-troubleshooting]]

## Related

- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
