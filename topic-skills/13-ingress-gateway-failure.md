---
title: Ingress/Gateway 路由故障诊断与修复 / Ingress & Gateway Routing Failure Diagnosis & Remediation
description: '## 1. 概述'
category: network
tags:
- k8s
- skills
- sop
- runbook
- prometheus
- istio
- envoy
- helm
- ingress
- gateway
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- Ingress/Gateway 路由故障诊断与修复 / Ingress & Gateway Routing Failure Diagnosis & Remediation 是什么
- 如何 Ingress/Gateway 路由故障诊断与修复 / Ingress & Gateway Routing Failure Diagnosis & Remediation
trigger_keywords:
- ingress 404
- ingress 502
- ingress 503
- gateway not ready
- httproute not accepted
- tls termination failed
- ingress controller crash
- nginx reload failed
- upstream timeout
- backend unhealthy
- certificate mismatch
- cross-namespace route denied
- 路由失败
- 网关故障
- 证书不匹配
- 后端不可达
- Ingress 无法访问
- Gateway 不工作
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2
skill_metadata:
  skill_id: SKILL-13
  category: network
  subcategory: ingress-gateway
  severity: P1
  time_to_diagnosis_minutes: 15
  time_to_remediation_minutes: 20
  escalation_required: false
  control_plane_impact: false
agent_notes:
  decision_tree_entry: "kubectl get pods -n ingress-nginx -o wide 检查 Ingress Controller 状态"
  critical_commands:
    - "kubectl get pods -n ingress-nginx -l app=ingress-nginx -o wide"
    - "kubectl describe ingress <name> -n <ns>"
    - "kubectl logs -n ingress-nginx -l app=ingress-nginx --tail=100"
    - "kubectl get events -A --field-selector involvedObject.kind=Ingress --sort-by='.lastTimestamp'"
  danger_operations:
    - action: "kubectl delete pod -n ingress-nginx -l app=ingress-nginx --force"
      risk: "强制删除会导致 Ingress Controller 重启，可能短暂中断入口流量"
      requires_confirmation: true
---

<!-- condition: kubectl get pods -n ingress-nginx -o jsonpath='{range .items[?(@.status.phase!="Running")]} {.metadata.name}{"\n"}{end}' 显示 Ingress Controller 异常 -->

# Ingress/Gateway 路由故障诊断与修复 / Ingress & Gateway Routing Failure Diagnosis & Remediation

---

## 1. 概述

Ingress 和 Gateway API 是 Kubernetes 集群中**南北向流量**的核心入口点。当 Ingress Controller 或 Gateway 发生故障时，所有通过该入口的外部请求都将受到影响，可能导致用户无法访问应用服务、API 调用失败、TLS 连接中断等严重后果。在微服务架构中，Ingress/Gateway 故障的**爆炸半径**往往覆盖多个服务，是高优先级的生产事件。

### 典型触发场景

1. **HTTP 错误响应**: 外部请求返回 404/502/503 等 HTTP 错误，表明路由规则、后端服务或 Ingress Controller 本身存在问题
2. **TLS/SSL 故障**: 证书过期、证书不匹配、TLS 握手失败，导致 HTTPS 请求无法建立安全连接
3. **Ingress Controller 异常**: Nginx/Traefik/ALB/Envoy Gateway Controller Pod 崩溃、OOM、配置重载失败
4. **Gateway API 绑定失败**: HTTPRoute/GRPCRoute/TLSRoute 未被 Gateway 接受，或跨命名空间权限缺失
5. **后端不可达**: Service 无 Ready Endpoints、后端 Pod 健康检查失败、upstream 连接超时

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `ingresses` (networking.k8s.io), `gateways` (gateway.networking.k8s.io), `httproutes` (gateway.networking.k8s.io), `services`, `endpoints`, `endpointslices`, `secrets`, `pods`, `events` 的 `get/list/watch`
  - 修复权限: `ingresses`, `gateways`, `httproutes`, `services` 的 `patch/update`
  - 验证命令: `kubectl auth can-i list ingresses`
- **网络访问**: 能够从集群外部访问 Ingress/Gateway 的 LoadBalancer IP 或 NodePort
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `curl`
  - `openssl` >= 1.1.1
  - `jq` >= 1.6（可选但推荐）
- **监控系统**: Prometheus + Ingress Controller 指标（用于 trigger_metrics 匹配）

> ⚠️ **重要**: 本 Skill 覆盖多种 Ingress Controller（Nginx Ingress、Traefik、ALB Ingress、Envoy Gateway）以及 Gateway API 资源。不同控制器的诊断命令和日志格式有所差异，诊断时需确认实际使用的控制器类型。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | 外部请求返回 404 Not Found / External request returns 404 | `curl -v -H "Host: DOMAIN" http://INGRESS_IP/path` 返回 404 | 0.90 | 后端应用本身返回 404（非 Ingress 配置问题）；请求的 Host 或 Path 拼写错误 |
| SP-02 | 外部请求返回 502 Bad Gateway / External request returns 502 | `curl -v -H "Host: DOMAIN" http://INGRESS_IP/path` 返回 502 | 0.85 | 后端 Pod 正在重启中（短暂 502）；后端应用启动慢但即将就绪 |
| SP-03 | 外部请求返回 503 Service Unavailable / External request returns 503 | `curl -v -H "Host: DOMAIN" http://INGRESS_IP/path` 返回 503 | 0.85 | Ingress Controller 正在滚动更新；后端 Deployment 正在缩容 |
| SP-04 | TLS 握手失败 / TLS handshake failure (ERR_SSL_PROTOCOL_ERROR) | `curl -vk https://DOMAIN` 或 `openssl s_client -connect DOMAIN:443` 显示握手错误 | 0.90 | 客户端 TLS 版本不兼容；网络防火墙拦截 443 端口 |
| SP-05 | Ingress Controller Pod OOM/CrashLoop / Ingress Controller Pod in CrashLoopBackOff or OOMKilled | `kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller` 显示非 Running 状态 | 0.95 | 初次部署时的启动延迟；滚动更新期间的旧 Pod 终止 |
| SP-06 | Ingress 注解配置不生效 / Ingress annotation not taking effect | 配置了特定注解但行为未改变（如 rate-limiting、rewrite） | 0.70 | 注解语法正确但值设置不当；需要重启 Ingress Controller 才生效 |
| SP-07 | Gateway 状态非 Ready/Programmed / Gateway status not Ready or Programmed | `kubectl get gateway -A` 显示 STATUS 非 True 或 Conditions 异常 | 0.90 | Gateway 刚创建，正在等待 Controller 处理 |
| SP-08 | HTTPRoute parentRef 未被接受 / HTTPRoute parentRef not accepted | `kubectl get httproute NAME -o yaml` 的 status.parents 显示 Accepted=False | 0.90 | Route 刚创建，Controller 尚未同步 |
| SP-09 | 间歇性 upstream timeout / Intermittent upstream timeout | Ingress Controller 日志中出现 `upstream timed out` 或响应时间突然增加 | 0.75 | 后端应用处理慢（应用性能问题）；网络波动 |
| SP-10 | HTTPS 重定向循环 / HTTPS redirect loop (ERR_TOO_MANY_REDIRECTS) | 浏览器或 curl 报告重定向次数过多 | 0.85 | CDN 或云 LB 已配置 HTTPS 重定向，与 Ingress 配置冲突 |
| SP-11 | WebSocket 连接无法建立 / WebSocket connection fails | `wscat -c wss://DOMAIN/ws` 连接失败或立即断开 | 0.80 | 后端服务不支持 WebSocket；连接超时设置过短 |
| SP-12 | gRPC 请求路由失败 / gRPC request routing failure | gRPC 客户端报错 `UNAVAILABLE` 或 `UNIMPLEMENTED` | 0.80 | gRPC 服务本身异常；HTTP/2 未正确配置 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "访问网站返回 502 错误，请排查"
- "域名无法访问，显示 404"
- "SSL 证书报错，网站打不开"
- "Ingress 配置修改后不生效"
- "Gateway 创建了但状态一直是 Not Ready"
- "HTTPRoute 没有被 Gateway 接受"
- "网站响应很慢，经常超时"
- "HTTPS 访问一直在跳转"
- "WebSocket 连接断开"
- "gRPC 服务调用失败"

**English ticket descriptions**:
- "Website returns 502 Bad Gateway"
- "Ingress not routing traffic, getting 404"
- "TLS certificate error, site unreachable"
- "Ingress annotations not working"
- "Gateway stuck in NotReady state"
- "HTTPRoute not being accepted by Gateway"
- "Intermittent timeouts on API calls"
- "HTTPS redirect loop"
- "WebSocket connections failing"
- "gRPC calls returning UNAVAILABLE"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 后端 Pod 处于 CrashLoopBackOff 导致的 502 | SKILL-POD-001 | Pod 自身问题，非 Ingress 配置问题。先修复 Pod |
| Service 配置错误（selector 不匹配） | SKILL-NET-001 | Service 层面问题，虽然表现为 502/503 |
| DNS 解析失败（域名无法解析到 Ingress IP） | SKILL-NET-002 | DNS 问题，不在 Ingress 控制范围 |
| 云 LB 健康检查失败 | 云厂商文档 | 云厂商 LoadBalancer 层面问题 |
| 集群 CNI 网络故障导致 Pod 间无法通信 | SKILL-NET-001 | 底层网络故障，影响范围超出 Ingress |
| Ingress Controller 证书 Secret 问题但节点 NotReady | SKILL-NODE-001 | 节点故障是更根本的问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断故障爆炸半径：

**Step T1**: 确认 Ingress/Gateway 资源状态 (15s)
```bash
# 检查所有 Ingress 资源状态
kubectl get ingress -A -o wide

# 检查所有 Gateway 资源状态
kubectl get gateway -A 2>/dev/null || echo "Gateway API not installed"

# 检查 HTTPRoute 状态
kubectl get httproute -A 2>/dev/null || echo "No HTTPRoute resources"
```
> **判断规则**:
> - Ingress ADDRESS 列为空 → Ingress Controller 可能未运行或未分配 IP
> - Gateway STATUS 非 True/Programmed → Gateway 未就绪
> - 多个 Ingress/Gateway 受影响 → **P0/P1**（影响多个服务）
> - 仅单个 Ingress 受影响 → **P2**（待 T2 进一步确认）

**Step T2**: 外部连通性快速测试 (30s)
```bash
# 获取 Ingress/Gateway 外部 IP
INGRESS_IP=$(kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
# 或
INGRESS_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}')

# 快速测试 HTTP 响应
curl -s -o /dev/null -w "%{http_code}" -H "Host: <domain>" http://${INGRESS_IP}/
# 测试 HTTPS
curl -sk -o /dev/null -w "%{http_code}" https://<domain>/
```
> **判断规则**:
> - HTTP 200/2xx → 服务可用，可能是特定路径问题
> - HTTP 404 → 路由规则问题，检查 Ingress/HTTPRoute 配置
> - HTTP 502/503 → 后端不可达或 Controller 问题
> - HTTP 000 或连接失败 → 网络层面故障或 Controller 完全不可用

**Step T3**: Ingress Controller 状态检查 (30s)
```bash
# Nginx Ingress Controller
kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller

# 或 Traefik
kubectl get pods -n traefik -l app.kubernetes.io/name=traefik

# 或 AWS ALB Controller
kubectl get pods -n kube-system -l app.kubernetes.io/name=aws-load-balancer-controller

# 或 Envoy Gateway
kubectl get pods -n envoy-gateway-system -l control-plane=envoy-gateway
```
> **判断规则**:
> - Pod 状态为 Running 且 READY → Controller 运行正常
> - Pod 状态为 CrashLoopBackOff/OOMKilled → **P0**（Controller 完全不可用）
> - 多个 replica 但部分不健康 → 服务可能降级但仍可用

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| Ingress Controller 完全不可用（所有 replica down） **或** 生产域名全部 502/503 | **P0** | 所有通过该 Ingress Controller 的流量中断 | 立即响应，15min 内恢复或降级方案 |
| 多个关键服务的 Ingress 故障 **或** Gateway 状态非 Ready | **P1** | 部分服务不可用，影响多个业务线 | 15min 内响应，30min 内修复 |
| 单个非关键服务的 Ingress 故障 **或** 单个 HTTPRoute 未被接受 | **P2** | 单个服务受影响，影响范围有限 | 30min 内响应，2h 内修复 |
| TLS 证书即将过期 **或** 注解配置优化需求 | **P3** | 预防性维护，当前服务正常 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **全站故障**: 所有通过 Ingress 暴露的服务都返回 5xx 错误
- **Ingress Controller 无法恢复**: Controller Pod 反复 CrashLoop 超过 5 分钟
- **安全事件**: 发现异常证书、疑似证书泄露、或 TLS 配置被篡改
- **云 LB 故障**: 云厂商 LoadBalancer 状态异常（需联系云厂商支持）
- **数据面完全中断**: 所有请求超时，无任何 HTTP 响应

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 Ingress/Gateway 资源状态信息，无需登录 Controller Pod。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取 Ingress 资源详细信息
- **命令**:
  ```bash
  kubectl get ingress -A -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含 NAMESPACE, NAME, CLASS, HOSTS, ADDRESS, PORTS, AGE
- **判断规则**:
  - ADDRESS 列为空 → 可能是 IngressClass 未配置或 Controller 未运行（RC-004）
  - PORTS 列缺少 443 → TLS 可能未正确配置（RC-003）
  - CLASS 列为空或不匹配 → IngressClass 问题（RC-004）
- **版本差异**: 无

**Step D1.2**: 检查目标 Ingress 详细配置
- **命令**:
  ```bash
  kubectl describe ingress <ingress-name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: 详细配置包含 Rules、TLS、Annotations、Events
- **判断规则**:
  - Rules 中 Backend 显示 `<error: endpoints "xxx" not found>` → 后端 Service/Endpoints 问题（RC-001）
  - TLS 配置中 Secret 显示 `<error: secrets "xxx" not found>` → TLS Secret 缺失（RC-003）
  - Events 中出现 `Sync` 错误 → 配置同步失败（RC-005）
  - Events 中出现 `AddedOrUpdated` → 配置已被 Controller 处理
- **版本差异**: 无

**Step D1.3**: 检查 IngressClass 配置
- **命令**:
  ```bash
  kubectl get ingressclass
  kubectl get ingressclass <class-name> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: IngressClass 列表及详细配置
- **判断规则**:
  - 无 IngressClass 资源 → 需要创建或使用注解方式指定（RC-004）
  - `is-default-class: "true"` 注解存在 → 默认 IngressClass 已设置
  - controller 字段与实际 Controller 不匹配 → IngressClass 配置错误（RC-004）
- **版本差异**:
  - **[v1.28+]**: IngressClass 为 GA，推荐使用 `spec.controller` 替代 `kubernetes.io/ingress.class` 注解

**Step D1.4**: 检查 Gateway 资源状态
- **命令**:
  ```bash
  # 检查 Gateway 资源
  kubectl get gateway -A -o wide

  # 详细状态
  kubectl get gateway <gateway-name> -n <namespace> -o yaml | grep -A 30 "status:"
  ```
- **超时**: 10s
- **预期输出模式**: Gateway 列表及状态
- **判断规则**:
  - status.conditions 中 `Accepted=True` 且 `Programmed=True` → Gateway 正常
  - `Accepted=False` → Gateway 配置被拒绝（RC-007）
  - `Programmed=False` → 数据面未就绪
  - 无 status 字段 → Gateway Controller 未处理此资源
- **版本差异**:
  - **[v1.28]**: Gateway API v1beta1 GA
  - **[v1.30+]**: Gateway API v1 GA，新增更多 Condition 类型
  - **[v1.31+]**: GRPCRoute GA

**Step D1.5**: 检查 HTTPRoute 状态
- **命令**:
  ```bash
  # 列出 HTTPRoute
  kubectl get httproute -A

  # 检查详细状态
  kubectl get httproute <route-name> -n <namespace> -o yaml | grep -A 50 "status:"
  ```
- **超时**: 10s
- **预期输出模式**: HTTPRoute 及其绑定状态
- **判断规则**:
  - `parents[].conditions` 中 `Accepted=True` → 路由已被 Gateway 接受
  - `Accepted=False` 且 reason 为 `NoMatchingParent` → Gateway selector 不匹配（RC-007）
  - `Accepted=False` 且 reason 为 `RefNotPermitted` → 跨命名空间权限缺失（RC-009）
  - `ResolvedRefs=False` → 后端 Service 引用失败（RC-001）
- **版本差异**:
  - **[v1.30+]**: HTTPRoute v1 GA

**Step D1.6**: 外部连通性深度测试
- **命令**:
  ```bash
  # 获取 Ingress 外部 IP
  INGRESS_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}' 2>/dev/null || \
               kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')

  # HTTP 测试
  curl -v -H "Host: <domain>" http://${INGRESS_IP}/<path> 2>&1 | head -50

  # HTTPS 测试（含证书信息）
  curl -vk https://<domain>/<path> 2>&1 | grep -E "SSL|certificate|HTTP|< "

  # 检查 TLS 证书详情
  echo | openssl s_client -servername <domain> -connect <domain>:443 2>/dev/null | openssl x509 -noout -subject -dates -issuer
  ```
- **超时**: 30s
- **预期输出模式**: HTTP 响应头和状态码、TLS 证书信息
- **判断规则**:
  - HTTP 404 且 `server: nginx/xxx` → Nginx 返回的 404，路由规则问题（RC-002）
  - HTTP 502 且 `upstream` 错误 → 后端连接失败（RC-001）
  - HTTP 503 且 `no server is available` → 后端无可用 Endpoints（RC-001）
  - TLS 证书 subject/SAN 不包含目标域名 → 证书不匹配（RC-003）
  - 证书 `notAfter` 已过期 → 证书过期（RC-003）
- **版本差异**: 无

---

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入检查 Ingress Controller 内部状态、配置和后端连接情况。
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 Ingress Controller 日志
- **命令**:
  ```bash
  # Nginx Ingress Controller
  kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=100

  # 过滤错误日志
  kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=500 | grep -iE "error|warn|fail|timeout|connect"

  # Traefik
  kubectl logs -n traefik deploy/traefik --tail=100

  # Envoy Gateway
  kubectl logs -n envoy-gateway-system deploy/envoy-gateway --tail=100
  ```
- **超时**: 15s
- **预期输出模式**: Controller 日志条目
- **判断规则**:
  - `no upstream host` / `no resolver` → 后端 Service 解析失败（RC-001）
  - `SSL_do_handshake() failed` → TLS 握手失败（RC-003）
  - `upstream timed out` → 后端响应超时（RC-006）
  - `connect() failed` / `Connection refused` → 后端连接被拒绝（RC-001）
  - `certificate verification failed` → 后端 HTTPS 证书验证失败（RC-011）
  - `configuration reload failed` → Nginx 配置重载失败（RC-005）
  - `worker process exited on signal 9` → OOM 被杀（RC-008）
- **版本差异**: 日志格式因 Controller 类型和版本不同

**Step D2.2**: 验证 Nginx 配置（Nginx Ingress 特定）
- **命令**:
  ```bash
  # 导出当前 Nginx 配置
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -T 2>/dev/null | grep -E "server_name|location|upstream|proxy_pass" | head -100

  # 验证配置语法
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -t

  # 查看特定 server block
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -T 2>/dev/null | grep -A 30 "server_name <domain>"
  ```
- **超时**: 15s
- **预期输出模式**: Nginx 配置片段
- **判断规则**:
  - `server_name` 未包含目标域名 → Ingress Host 未生效（RC-002）
  - `location` 路径不匹配 → Path 规则配置错误（RC-002）
  - `upstream` 中无后端地址 → Service Endpoints 为空（RC-001）
  - `nginx -t` 返回非 0 → 配置语法错误（RC-005）
- **版本差异**: 无

**Step D2.3**: 检查后端 Service/Endpoints
- **命令**:
  ```bash
  # 检查 Service
  kubectl get svc <backend-service> -n <namespace> -o wide

  # 检查 Endpoints
  kubectl get endpoints <backend-service> -n <namespace>

  # 详细 Endpoints 信息
  kubectl get endpoints <backend-service> -n <namespace> -o yaml

  # 检查 EndpointSlice（v1.21+ 默认）
  kubectl get endpointslice -n <namespace> -l kubernetes.io/service-name=<backend-service>
  ```
- **超时**: 10s
- **预期输出模式**: Service 和 Endpoints 信息
- **判断规则**:
  - Endpoints 显示 `<none>` → 无 Ready Pod（RC-001）
  - Endpoints 数量少于预期 → 部分 Pod 不健康
  - Service selector 与 Pod labels 不匹配 → Service 配置错误（RC-001）
  - Service port 与 Ingress backend port 不匹配 → 端口配置错误（RC-002）
- **版本差异**: 无

**Step D2.4**: TLS Secret 内容验证
- **命令**:
  ```bash
  # 检查 Secret 是否存在
  kubectl get secret <tls-secret> -n <namespace>

  # 验证证书内容
  kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -subject -dates -issuer

  # 检查证书 SAN
  kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -text | grep -A 1 "Subject Alternative Name"

  # 验证证书与私钥匹配
  CERT_MD5=$(kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -modulus | md5sum)
  KEY_MD5=$(kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.key}' | base64 -d | openssl rsa -noout -modulus 2>/dev/null | md5sum)
  echo "Cert: $CERT_MD5"
  echo "Key:  $KEY_MD5"
  # 两者应相同
  ```
- **超时**: 10s
- **预期输出模式**: 证书详细信息
- **判断规则**:
  - Secret 不存在 → 需要创建 TLS Secret（RC-003）
  - 证书 `notAfter` 早于当前时间 → 证书已过期（RC-003）
  - 证书 SAN 不包含目标域名 → 证书不匹配（RC-003）
  - 证书与私钥 MD5 不匹配 → 证书/密钥不配对（RC-003）
- **版本差异**: 无

**Step D2.5**: Ingress 注解解析
- **命令**:
  ```bash
  # 获取所有注解
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations}' | jq .

  # 检查关键注解
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations.nginx\.ingress\.kubernetes\.io/rewrite-target}'
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations.nginx\.ingress\.kubernetes\.io/backend-protocol}'
  ```
- **超时**: 10s
- **预期输出模式**: 注解键值对
- **判断规则**:
  - `backend-protocol: HTTPS` 但后端是 HTTP → 协议不匹配（RC-011）
  - `backend-protocol: GRPC` 但后端是 HTTP → gRPC 协议不匹配（RC-011）
  - `rewrite-target` 配置错误 → 路径重写问题（RC-002）
  - `ssl-redirect: "true"` 但无 TLS 配置 → 重定向循环（RC-010）
- **版本差异**: 注解前缀因 Controller 不同而异

**Step D2.6**: Upstream 连接分析
- **命令**:
  ```bash
  # Nginx stub_status（如果启用）
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s http://localhost:10246/nginx_status

  # 检查 upstream 连接数
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s http://localhost:10254/metrics | grep -E "nginx_ingress_controller_upstream|nginx_ingress_controller_requests"
  ```
- **超时**: 10s
- **预期输出模式**: 连接统计
- **判断规则**:
  - `upstream_connect_fail` 持续增加 → 后端连接失败
  - `upstream_latency_seconds` P99 过高 → 后端响应慢（RC-006）
  - `requests{status="5xx"}` 增加 → 服务端错误
- **版本差异**: 无

**Step D2.7**: Gateway API ReferenceGrant 检查
- **命令**:
  ```bash
  # 列出所有 ReferenceGrant
  kubectl get referencegrant -A

  # 检查特定命名空间的 ReferenceGrant
  kubectl get referencegrant -n <backend-namespace> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: ReferenceGrant 配置
- **判断规则**:
  - 无 ReferenceGrant 但 HTTPRoute 引用其他命名空间的 Service → 跨命名空间权限缺失（RC-009）
  - ReferenceGrant 的 `from` 不包含 HTTPRoute 所在命名空间 → 权限不足（RC-009）
- **版本差异**:
  - **[v1.28+]**: ReferenceGrant v1beta1 GA

**Step D2.8**: DNS 解析验证
- **命令**:
  ```bash
  # 外部 DNS 解析
  nslookup <domain>
  dig <domain> +short

  # 验证 DNS 指向 Ingress IP
  EXPECTED_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
  ACTUAL_IP=$(dig +short <domain> | tail -1)
  echo "Expected: $EXPECTED_IP"
  echo "Actual: $ACTUAL_IP"
  ```
- **超时**: 10s
- **预期输出模式**: DNS 解析结果
- **判断规则**:
  - DNS 解析失败 → DNS 配置问题（超出本 Skill 范围）
  - DNS 解析结果与 Ingress IP 不匹配 → DNS 记录过期或错误配置
- **版本差异**: 无

---

### Phase 3: 高级诊断（低风险，可能需审批）

> ⚠️ 以下步骤涉及更深入的检查和可能的写入操作。在 L1-advisory 模式下，Agent 应**提出建议并等待人工确认**后执行。
> **预计耗时**: 5-10 分钟

**Step D3.1**: 不同 Ingress Controller 的特异性诊断

**Nginx Ingress Controller**:
```bash
# 检查 ConfigMap 配置
kubectl get configmap -n ingress-nginx ingress-nginx-controller -o yaml

# 检查 IngressClass 参数
kubectl get ingressclass nginx -o yaml

# 检查 admission webhook
kubectl get validatingwebhookconfigurations | grep ingress
```

**Traefik**:
```bash
# 检查 IngressRoute（Traefik CRD）
kubectl get ingressroute -A

# Traefik 仪表板（如果启用）
kubectl port-forward -n traefik deploy/traefik 9000:9000
# 然后访问 http://localhost:9000/dashboard/
```

**AWS ALB Ingress Controller**:
```bash
# 检查 ALB 状态
kubectl get ing <ingress-name> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress[0].hostname}'

# 检查 ALB Controller 日志
kubectl logs -n kube-system deploy/aws-load-balancer-controller --tail=100

# 检查 TargetGroupBinding
kubectl get targetgroupbinding -A
```

**Envoy Gateway**:
```bash
# 检查 EnvoyProxy 配置
kubectl get envoyproxy -A

# 检查 Envoy 代理 Pod
kubectl get pods -n envoy-gateway-system -l gateway.envoyproxy.io/owning-gateway-name=<gateway-name>

# Envoy admin 接口
kubectl port-forward -n envoy-gateway-system deploy/<envoy-deploy> 19000:19000
# 访问 http://localhost:19000/
```
- **超时**: 30s
- **风险级别**: 🟢 低（只读操作，port-forward 除外）
- **判断规则**: 根据 Controller 类型进行特定诊断

**Step D3.2**: Rate Limiting / WAF 规则排查
- **命令**:
  ```bash
  # 检查 Nginx rate limiting 配置
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -T | grep -E "limit_req|limit_conn"

  # 检查 ModSecurity（如果启用）
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o jsonpath='{.data.enable-modsecurity}'

  # 检查 Nginx 日志中的限流信息
  kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=500 | grep -i "limiting\|rejected\|blocked"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低
- **判断规则**:
  - 日志中出现 `limiting requests` → 触发了限流规则
  - ModSecurity 阻止请求 → WAF 规则可能过于严格

**Step D3.3**: Session Affinity / Sticky Session 配置
- **命令**:
  ```bash
  # 检查 Service session affinity
  kubectl get svc <backend-service> -n <namespace> -o jsonpath='{.spec.sessionAffinity}'

  # 检查 Ingress 会话亲和性注解
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations.nginx\.ingress\.kubernetes\.io/affinity}'
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations.nginx\.ingress\.kubernetes\.io/session-cookie-name}'
  ```
- **超时**: 10s
- **风险级别**: 🟢 低
- **判断规则**:
  - Session affinity 配置但 cookie 未设置 → 会话保持可能不工作
  - 多 replica 场景未配置 affinity 但应用需要 → 可能导致会话丢失

**Step D3.4**: HTTP/2 和 gRPC 特殊配置
- **命令**:
  ```bash
  # 检查 HTTP/2 是否启用
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o jsonpath='{.data.use-http2}'

  # 检查 gRPC 后端协议
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations.nginx\.ingress\.kubernetes\.io/backend-protocol}'

  # 测试 HTTP/2 连接
  curl -v --http2 https://<domain>/ 2>&1 | grep -E "ALPN|HTTP/2"

  # 使用 grpcurl 测试 gRPC（如果安装）
  grpcurl -v <domain>:443 list
  ```
- **超时**: 15s
- **风险级别**: 🟢 低
- **判断规则**:
  - gRPC 请求失败但 HTTP 正常 → 需要设置 `backend-protocol: GRPC`（RC-011）
  - HTTP/2 连接失败 → 检查 `use-http2` 配置
  - ALPN 协商失败 → TLS 配置可能不支持 HTTP/2

**Step D3.5**: 负载均衡算法与权重配置
- **命令**:
  ```bash
  # 检查 Nginx 负载均衡配置
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o jsonpath='{.data.load-balance}'

  # 检查 upstream 权重
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -T | grep -A 10 "upstream.*{"

  # HTTPRoute 后端权重
  kubectl get httproute <route-name> -n <namespace> -o jsonpath='{.spec.rules[*].backendRefs}'
  ```
- **超时**: 10s
- **风险级别**: 🟢 低
- **判断规则**:
  - 权重配置不均匀可能导致流量倾斜
  - 负载均衡算法不当可能导致热点问题

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 | 风险等级 |
|--------|------|------|---------|---------|---------|
| RC-001 | **后端 Service 无 Ready Endpoints** — Service selector 不匹配、后端 Pod 全部不健康或正在重启，导致 Ingress 无法路由到任何后端 | ~20% | D2.3 显示 Endpoints 为空；D1.2 Backend 显示 `<error: endpoints>`；D2.1 日志包含 `no upstream host` | ingress-fta: BE-no-endpoints | 🟡 |
| RC-002 | **Ingress Host/Path 规则配置错误** — Ingress 的 host、path 或 pathType 配置不正确，导致请求无法匹配到正确的后端 | ~15% | D1.6 返回 404；D2.2 Nginx 配置中未包含目标 `server_name` 或 `location`；D1.2 Rules 配置与请求不匹配 | ingress-fta: BE-rule-mismatch | 🟢 |
| RC-003 | **TLS Secret 不存在或证书不匹配** — Ingress 引用的 TLS Secret 缺失、证书已过期、或证书 SAN 不包含目标域名 | ~12% | D2.4 Secret 不存在或证书过期/不匹配；D1.6 TLS 握手失败；D1.2 TLS 配置错误 | ingress-fta: BE-tls-invalid | 🟡 |
| RC-004 | **IngressClass 未指定或不匹配** — Ingress 未指定 IngressClass 或指定的 IngressClass 与实际 Controller 不匹配，导致 Ingress 未被处理 | ~10% | D1.1 ADDRESS 为空；D1.3 IngressClass 不存在或 controller 不匹配；D1.2 无任何 Events | ingress-fta: BE-class-mismatch | 🟢 |
| RC-005 | **Ingress Controller 配置重载失败** — Nginx/Traefik 配置文件语法错误或无效配置导致 reload 失败，新配置未生效 | ~8% | D2.1 日志包含 `configuration reload failed`；D2.2 `nginx -t` 失败；控制器指标显示 `config_last_reload_successful=0` | ingress-fta: BE-reload-fail | 🟡 |
| RC-006 | **Upstream 超时配置不当** — 后端响应时间超过 Ingress Controller 的 proxy_read_timeout 配置，导致请求被提前终止 | ~7% | D2.1 日志包含 `upstream timed out`；D2.6 upstream_latency 指标异常；D1.6 返回 504 | ingress-fta: BE-timeout | 🟢 |
| RC-007 | **Gateway API 绑定/权限失败** — Gateway 配置被拒绝、HTTPRoute 无法绑定到 Gateway、或 Gateway 控制器未就绪 | ~6% | D1.4 Gateway status 非 Ready；D1.5 HTTPRoute Accepted=False；Gateway 日志显示错误 | ingress-fta: BE-gateway-bind-fail | 🟡 |
| RC-008 | **Ingress Controller OOM** — Controller Pod 因内存不足被 OOM Killed，通常由于配置过多、流量激增或内存泄漏 | ~5% | D2.1 日志显示 `worker process exited on signal 9`；Pod 状态 OOMKilled；Controller 反复重启 | ingress-fta: BE-controller-oom | 🔴 |
| RC-009 | **跨命名空间 ReferenceGrant 缺失** — HTTPRoute 引用其他命名空间的 Service/Secret 但缺少 ReferenceGrant 授权 | ~5% | D1.5 HTTPRoute 状态 `RefNotPermitted`；D2.7 无对应 ReferenceGrant | ingress-fta: BE-ref-denied | 🟢 |
| RC-010 | **HTTP → HTTPS 重定向配置冲突** — Ingress 配置了 HTTPS 重定向但前端 LB 也配置了重定向，导致重定向循环 | ~4% | D1.6 客户端报告 `ERR_TOO_MANY_REDIRECTS`；D2.5 `ssl-redirect=true` 与外部 LB 冲突 | ingress-fta: BE-redirect-loop | 🟢 |
| RC-011 | **后端协议不匹配（HTTP vs HTTPS vs gRPC）** — Ingress 与后端之间的协议配置不正确，如后端是 HTTPS 但 Ingress 使用 HTTP 连接 | ~4% | D2.1 日志包含 SSL/TLS 错误或 gRPC 错误；D2.5 `backend-protocol` 配置与实际不符；gRPC 请求失败 | ingress-fta: BE-protocol-mismatch | 🟡 |
| RC-012 | **云厂商 LB 健康检查配置不兼容** — 云 LoadBalancer 的健康检查路径、端口或协议与 Ingress Controller 不兼容，导致后端被标记为不健康 | ~4% | 云控制台显示 LB 后端不健康；D2.1 日志正常但外部无法访问；健康检查端点返回非 200 | ingress-fta: BE-cloud-lb-health | 🟡 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 修正 Ingress Host/Path 规则
- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 确认当前 Ingress 配置
  kubectl get ingress <ingress-name> -n <namespace> -o yaml | grep -A 20 "rules:"
  
  # 确认期望的 Host 和 Path
  echo "Expected Host: <domain>"
  echo "Expected Path: <path>"
  ```
- **执行命令**:
  ```bash
  # 方法 1: 使用 kubectl patch
  kubectl patch ingress <ingress-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/rules/0/host", "value": "<correct-domain>"}
  ]'

  # 方法 2: 编辑 Ingress
  kubectl edit ingress <ingress-name> -n <namespace>
  # 修正 host 和 path 配置
  ```
- **后置验证**:
  ```bash
  # 验证配置已更新
  kubectl get ingress <ingress-name> -n <namespace> -o yaml | grep -A 10 "rules:"
  
  # 验证外部访问
  curl -v -H "Host: <domain>" http://<ingress-ip>/<path>
  # 预期: HTTP 200 或正确的业务响应
  ```
- **回滚命令**:
  ```bash
  kubectl patch ingress <ingress-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/rules/0/host", "value": "<original-domain>"}
  ]'
  ```

#### REM-002: 设置正确的 IngressClass
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  # 查看可用的 IngressClass
  kubectl get ingressclass
  
  # 确认目标 Controller
  kubectl get ingressclass <class-name> -o jsonpath='{.spec.controller}'
  ```
- **执行命令**:
  ```bash
  # 方法 1: 添加 ingressClassName 字段
  kubectl patch ingress <ingress-name> -n <namespace> --type='json' -p='[
    {"op": "add", "path": "/spec/ingressClassName", "value": "nginx"}
  ]'

  # 方法 2: 使用注解（旧版兼容）
  kubectl annotate ingress <ingress-name> -n <namespace> kubernetes.io/ingress.class=nginx --overwrite
  ```
- **后置验证**:
  ```bash
  # 确认 IngressClass 已设置
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.spec.ingressClassName}'
  
  # 确认 ADDRESS 已分配
  kubectl get ingress <ingress-name> -n <namespace> -o wide
  # 预期: ADDRESS 列有值
  
  # 检查 Events
  kubectl describe ingress <ingress-name> -n <namespace> | tail -10
  # 预期: 出现 "Sync" 或 "AddedOrUpdated" 事件
  ```
- **回滚命令**:
  ```bash
  kubectl patch ingress <ingress-name> -n <namespace> --type='json' -p='[
    {"op": "remove", "path": "/spec/ingressClassName"}
  ]'
  ```

#### REM-003: 创建/更新 TLS Secret
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 确认 Secret 是否存在
  kubectl get secret <tls-secret> -n <namespace>
  
  # 如果存在，检查证书有效期
  kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
  ```
- **执行命令**:
  ```bash
  # 创建新的 TLS Secret
  kubectl create secret tls <tls-secret> -n <namespace> \
    --cert=<path-to-cert.pem> \
    --key=<path-to-key.pem>

  # 或更新已存在的 Secret
  kubectl create secret tls <tls-secret> -n <namespace> \
    --cert=<path-to-cert.pem> \
    --key=<path-to-key.pem> \
    --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  # 验证 Secret 已创建/更新
  kubectl get secret <tls-secret> -n <namespace>
  
  # 验证证书内容
  kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -subject -dates
  
  # 验证 HTTPS 访问
  curl -vk https://<domain>/ 2>&1 | grep -E "SSL|certificate|HTTP"
  # 预期: TLS 握手成功
  ```
- **回滚命令**:
  ```bash
  # 删除新创建的 Secret
  kubectl delete secret <tls-secret> -n <namespace>
  
  # 或恢复旧的 Secret（如果有备份）
  kubectl apply -f <backup-secret.yaml>
  ```

#### REM-004: 调整 upstream 超时和重试配置
- **适用根因**: RC-006
- **前置检查**:
  ```bash
  # 查看当前超时配置
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations}' | jq . | grep -i timeout
  
  # 查看全局 ConfigMap 配置
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o jsonpath='{.data}' | jq . | grep -i timeout
  ```
- **执行命令**:
  ```bash
  # 为特定 Ingress 设置超时
  kubectl annotate ingress <ingress-name> -n <namespace> \
    nginx.ingress.kubernetes.io/proxy-connect-timeout="60" \
    nginx.ingress.kubernetes.io/proxy-send-timeout="120" \
    nginx.ingress.kubernetes.io/proxy-read-timeout="120" \
    --overwrite
  ```
- **后置验证**:
  ```bash
  # 验证注解已设置
  kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations}' | jq . | grep timeout
  
  # 测试请求
  time curl -v -H "Host: <domain>" http://<ingress-ip>/<path>
  # 预期: 请求在新的超时时间内完成或正确超时
  ```
- **回滚命令**:
  ```bash
  kubectl annotate ingress <ingress-name> -n <namespace> \
    nginx.ingress.kubernetes.io/proxy-connect-timeout- \
    nginx.ingress.kubernetes.io/proxy-send-timeout- \
    nginx.ingress.kubernetes.io/proxy-read-timeout-
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-005: 修复 Ingress Controller 配置（ConfigMap）
- **适用根因**: RC-005, RC-006
- **影响说明**: 修改 Ingress Controller 的全局 ConfigMap 会影响所有通过该 Controller 的 Ingress 资源。修改后 Controller 会自动 reload 配置。
- **审批提示**: "建议修改 Ingress Controller 的全局配置（ConfigMap）。此修改会影响所有通过该 Controller 路由的服务。是否批准？"
- **前置检查**:
  ```bash
  # 备份当前 ConfigMap
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o yaml > /tmp/ingress-configmap-backup.yaml
  
  # 查看当前配置
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o jsonpath='{.data}'
  ```
- **执行命令**:
  ```bash
  # 编辑 ConfigMap
  kubectl edit configmap -n ingress-nginx ingress-nginx-controller
  
  # 或使用 patch
  kubectl patch configmap -n ingress-nginx ingress-nginx-controller --type='merge' -p='{"data":{"proxy-read-timeout":"120","proxy-connect-timeout":"60"}}'
  ```
- **后置验证**:
  ```bash
  # 验证配置已更新
  kubectl get configmap -n ingress-nginx ingress-nginx-controller -o jsonpath='{.data}'
  
  # 检查 Controller 是否成功 reload
  kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=20 | grep -i reload
  # 预期: "successfully reloaded configuration"
  
  # 检查 reload 指标
  kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s localhost:10254/metrics | grep config_last_reload_successful
  # 预期: 值为 1
  ```
- **回滚命令**:
  ```bash
  kubectl apply -f /tmp/ingress-configmap-backup.yaml
  ```

#### REM-006: 创建 ReferenceGrant 跨命名空间权限
- **适用根因**: RC-009
- **影响说明**: 创建 ReferenceGrant 允许其他命名空间的 HTTPRoute 引用本命名空间的 Service 或 Secret。这是一个安全相关的配置变更。
- **审批提示**: "建议在命名空间 `<namespace>` 中创建 ReferenceGrant，允许命名空间 `<from-namespace>` 的 HTTPRoute 引用本命名空间的资源。是否批准？"
- **前置检查**:
  ```bash
  # 确认 HTTPRoute 所在命名空间
  kubectl get httproute <route-name> -n <route-namespace>
  
  # 确认目标 Service 所在命名空间
  kubectl get svc <backend-service> -n <backend-namespace>
  
  # 检查是否已存在 ReferenceGrant
  kubectl get referencegrant -n <backend-namespace>
  ```
- **执行命令**:
  ```bash
  cat <<EOF | kubectl apply -f -
  # Valid for v1.28+ (v1beta1); v1 API available since v1.32+
  apiVersion: gateway.networking.k8s.io/v1beta1
  kind: ReferenceGrant
  metadata:
    name: allow-httproute-from-<route-namespace>
    namespace: <backend-namespace>
  spec:
    from:
      - group: gateway.networking.k8s.io
        kind: HTTPRoute
        namespace: <route-namespace>
    to:
      - group: ""
        kind: Service
  EOF
  ```
- **后置验证**:
  ```bash
  # 验证 ReferenceGrant 已创建
  kubectl get referencegrant -n <backend-namespace>
  
  # 验证 HTTPRoute 状态
  kubectl get httproute <route-name> -n <route-namespace> -o yaml | grep -A 10 "status:"
  # 预期: Accepted=True, ResolvedRefs=True
  ```
- **回滚命令**:
  ```bash
  kubectl delete referencegrant allow-httproute-from-<route-namespace> -n <backend-namespace>
  ```

#### REM-007: 修复 Gateway API 路由绑定
- **适用根因**: RC-007
- **影响说明**: 修改 Gateway 或 HTTPRoute 配置以修复绑定问题。可能影响现有路由。
- **审批提示**: "建议修改 Gateway/HTTPRoute 配置以修复绑定问题。是否批准？"
- **前置检查**:
  ```bash
  # 检查 Gateway 配置
  kubectl get gateway <gateway-name> -n <namespace> -o yaml
  
  # 检查 HTTPRoute 的 parentRefs
  kubectl get httproute <route-name> -n <namespace> -o jsonpath='{.spec.parentRefs}'
  
  # 检查 Gateway listeners
  kubectl get gateway <gateway-name> -n <namespace> -o jsonpath='{.spec.listeners}'
  ```
- **执行命令**:
  ```bash
  # 修正 HTTPRoute 的 parentRef
  kubectl patch httproute <route-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/parentRefs/0/name", "value": "<correct-gateway-name>"},
    {"op": "replace", "path": "/spec/parentRefs/0/namespace", "value": "<correct-gateway-namespace>"}
  ]'
  
  # 或编辑 HTTPRoute
  kubectl edit httproute <route-name> -n <namespace>
  ```
- **后置验证**:
  ```bash
  # 验证 HTTPRoute 状态
  kubectl get httproute <route-name> -n <namespace> -o yaml | grep -A 15 "status:"
  # 预期: Accepted=True
  
  # 测试路由
  curl -v http://<gateway-ip>/<path>
  ```
- **回滚命令**:
  ```bash
  # 恢复原始 parentRef
  kubectl patch httproute <route-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/parentRefs/0/name", "value": "<original-gateway-name>"}
  ]'
  ```

#### REM-008: 调整 Ingress Controller 资源限制
- **适用根因**: RC-008
- **影响说明**: 增加 Ingress Controller 的内存/CPU 限制。可能需要重启 Controller Pod。
- **审批提示**: "建议增加 Ingress Controller 的资源限制（当前内存: `<current>`, 建议: `<new>`）。Controller Pod 将重启。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前资源配置
  kubectl get deploy -n ingress-nginx ingress-nginx-controller -o jsonpath='{.spec.template.spec.containers[0].resources}'
  
  # 查看当前资源使用
  kubectl top pod -n ingress-nginx -l app.kubernetes.io/component=controller
  ```
- **执行命令**:
  ```bash
  # 增加内存限制
  kubectl patch deploy -n ingress-nginx ingress-nginx-controller --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "512Mi"},
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "256Mi"}
  ]'
  ```
- **后置验证**:
  ```bash
  # 等待新 Pod 启动
  kubectl rollout status deploy -n ingress-nginx ingress-nginx-controller --timeout=300s
  
  # 验证资源配置
  kubectl get deploy -n ingress-nginx ingress-nginx-controller -o jsonpath='{.spec.template.spec.containers[0].resources}'
  
  # 验证 Pod 状态
  kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller
  # 预期: Running 状态
  ```
- **回滚命令**:
  ```bash
  kubectl rollout undo deploy -n ingress-nginx ingress-nginx-controller
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-009: Ingress Controller 滚动重启
- **适用根因**: RC-005, RC-008
- **影响说明**: 重启 Ingress Controller 会导致**短暂的流量中断**（取决于副本数和滚动更新策略）。所有通过该 Controller 的请求可能在重启期间受影响。
- **操作步骤**:
  1. **确认当前副本数和更新策略**:
     ```bash
     kubectl get deploy -n ingress-nginx ingress-nginx-controller -o jsonpath='{.spec.replicas}'
     kubectl get deploy -n ingress-nginx ingress-nginx-controller -o jsonpath='{.spec.strategy}'
     ```
  2. **确保有足够副本以支持滚动重启**（推荐至少 2 个副本）:
     ```bash
     # 如果只有 1 个副本，考虑先扩容
     kubectl scale deploy -n ingress-nginx ingress-nginx-controller --replicas=2
     ```
  3. **执行滚动重启**:
     ```bash
     kubectl rollout restart deploy -n ingress-nginx ingress-nginx-controller
     ```
  4. **监控重启进度**:
     ```bash
     kubectl rollout status deploy -n ingress-nginx ingress-nginx-controller --timeout=300s
     ```
  5. **验证服务恢复**:
     ```bash
     kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller
     curl -v http://<ingress-ip>/healthz
     ```
- **安全检查**:
  - 确保至少有 2 个 replica 以支持滚动重启
  - 在低流量时段执行（如凌晨）
  - 通知相关团队即将执行重启
- **回滚方案**:
  ```bash
  kubectl rollout undo deploy -n ingress-nginx ingress-nginx-controller
  ```

#### REM-010: 切换备用 Ingress Controller
- **适用根因**: RC-008（反复发生且无法修复）
- **影响说明**: 将流量切换到备用 Ingress Controller（如果存在）。这需要修改 Ingress 的 IngressClass 或 DNS 记录。
- **操作步骤**:
  1. **确认备用 Controller 存在且健康**:
     ```bash
     # 查看所有 IngressClass
     kubectl get ingressclass
     
     # 确认备用 Controller 状态
     kubectl get pods -n <backup-ingress-namespace> -l <backup-controller-label>
     ```
  2. **修改 Ingress 使用备用 IngressClass**:
     ```bash
     # 批量更新受影响的 Ingress
     for ing in $(kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
       ns=$(echo $ing | cut -d/ -f1)
       name=$(echo $ing | cut -d/ -f2)
       kubectl patch ingress $name -n $ns --type='json' -p='[{"op": "replace", "path": "/spec/ingressClassName", "value": "<backup-class>"}]'
     done
     ```
  3. **或修改 DNS 指向备用 Controller 的 IP**:
     ```bash
     # 获取备用 Controller 的外部 IP
     BACKUP_IP=$(kubectl get svc -n <backup-namespace> <backup-svc> -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
     echo "Update DNS to point to: $BACKUP_IP"
     # 在 DNS 管理控制台更新记录
     ```
  4. **验证切换成功**:
     ```bash
     curl -v http://<domain>/
     ```
- **安全检查**:
  - 确认备用 Controller 配置与主 Controller 一致
  - 确认备用 Controller 有足够容量
- **回滚方案**:
  ```bash
  # 将 IngressClass 改回原来的
  kubectl patch ingress <ingress-name> -n <namespace> --type='json' -p='[{"op": "replace", "path": "/spec/ingressClassName", "value": "<original-class>"}]'
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: Ingress Controller 完全重新部署
- **适用根因**: RC-005, RC-008（其他修复方法均失败）
- **审批要求**: 需要高级 SRE + Team Lead 审批
- **数据备份**: 备份所有 Ingress、ConfigMap、Secret 资源
- **操作步骤**:
  1. **备份所有相关资源**:
     ```bash
     # 备份 Ingress 资源
     kubectl get ingress -A -o yaml > /tmp/all-ingress-backup.yaml
     
     # 备份 ConfigMap
     kubectl get configmap -n ingress-nginx -o yaml > /tmp/ingress-configmap-backup.yaml
     
     # 备份 Secrets（TLS 证书）
     kubectl get secrets -A -l type=kubernetes.io/tls -o yaml > /tmp/tls-secrets-backup.yaml
     ```
  2. **删除现有 Controller**:
     ```bash
     # 使用 Helm
     helm uninstall ingress-nginx -n ingress-nginx
     
     # 或使用 kubectl
     kubectl delete deploy -n ingress-nginx ingress-nginx-controller
     kubectl delete svc -n ingress-nginx ingress-nginx-controller
     ```
  3. **重新部署 Controller**:
     ```bash
     # 使用 Helm
     helm install ingress-nginx ingress-nginx/ingress-nginx -n ingress-nginx \
       -f <custom-values.yaml>
     
     # 等待部署完成
     kubectl rollout status deploy -n ingress-nginx ingress-nginx-controller --timeout=300s
     ```
  4. **验证 Ingress 资源重新同步**:
     ```bash
     kubectl get ingress -A -o wide
     # 预期: 所有 Ingress 都有 ADDRESS
     
     # 测试关键服务
     curl -v http://<domain>/
     ```
- **回滚方案**:
  ```bash
  # 恢复备份
  kubectl apply -f /tmp/all-ingress-backup.yaml
  kubectl apply -f /tmp/ingress-configmap-backup.yaml
  kubectl apply -f /tmp/tls-secrets-backup.yaml
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

```bash
# V1: 确认 Ingress/Gateway 状态正常
kubectl get ingress <ingress-name> -n <namespace> -o wide
# 预期: ADDRESS 列有值

kubectl get gateway <gateway-name> -n <namespace> -o jsonpath='{.status.conditions[?(@.type=="Programmed")].status}'
# 预期: True

# V2: 确认 HTTPRoute 状态正常
kubectl get httproute <route-name> -n <namespace> -o jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'
# 预期: True

# V3: 确认外部请求正常
curl -s -o /dev/null -w "%{http_code}" -H "Host: <domain>" http://<ingress-ip>/<path>
# 预期: 200 或期望的业务状态码

# V4: 确认 TLS 证书有效（如果适用）
echo | openssl s_client -servername <domain> -connect <domain>:443 2>/dev/null | openssl x509 -noout -dates
# 预期: notAfter 在未来

# V5: 确认 Controller 配置 reload 成功
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s localhost:10254/metrics | grep config_last_reload_successful
# 预期: nginx_ingress_controller_config_last_reload_successful 1

# V6: 确认无新的错误日志
kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=20 --since=2m | grep -iE "error|fail"
# 预期: 无错误日志
```

### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| HTTP 5xx 错误率 | `nginx_ingress_controller_requests{status=~"5.."}` | 下降至 0 或极低 | 持续 >1% 的请求返回 5xx |
| Upstream 延迟 | `nginx_ingress_controller_upstream_latency_seconds` | P99 < 预期 SLA | P99 > 5s |
| 配置 reload 状态 | `nginx_ingress_controller_config_last_reload_successful` | 保持为 1 | 值变为 0 |
| Controller Pod 状态 | `kubectl get pods -n ingress-nginx` | Running, Ready | CrashLoopBackOff, OOMKilled |
| 活跃连接数 | `nginx_ingress_controller_nginx_process_connections` | 恢复正常水平 | 突然降为 0 或异常高 |
| Gateway 状态 | `kubectl get gateway -A` | Ready/Programmed | NotReady |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：

- [ ] Ingress ADDRESS 列有正确的 IP/Hostname
- [ ] Gateway 状态为 Ready/Programmed
- [ ] HTTPRoute 状态为 Accepted=True
- [ ] 外部 HTTP/HTTPS 请求返回期望状态码
- [ ] TLS 证书有效且与域名匹配
- [ ] Ingress Controller Pod 状态为 Running 且无重启
- [ ] 无新的 5xx 错误日志
- [ ] 上游延迟恢复正常
- [ ] 根因已明确记录

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| HTTP 错误率 | Prometheus 告警 `IngressHighErrorRate` | 持续 | 如果错误率再次上升 → 重新进入诊断流程 |
| 证书有效期 | cert-manager 或手动检查 | 每日 | 有效期 <7 天 → 预防性更新证书 |
| Controller 重启 | `kubectl get pods -n ingress-nginx` | 每 4 小时 | 24h 内重启 >2 次 → 排查根因 |
| 配置同步 | `kubectl describe ingress` Events | 每 4 小时 | Sync 失败 → 检查配置 |
| 后端健康 | Endpoints 数量 | 每小时 | Endpoints 减少 → 检查后端 Pod |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多服务受影响） | 诊断过程中故障扩散 |
| **未知根因** | 完成所有诊断步骤但无法匹配任何已知根因 | 所有 RC 均已排除 |
| **安全事件** | 发现证书泄露迹象、异常配置变更 | 任何阶段发现安全异常 |
| **Controller 无法恢复** | Controller Pod 反复 CrashLoop >5min | REM-009 执行后仍无法恢复 |

### 8.2 升级消息模板

```
【{severity}】Ingress/Gateway 路由故障 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 故障概述: {domain} 通过 {ingress_name} 路由的请求返回 {http_status}，持续 {duration}
- 影响范围:
  - 受影响域名: {affected_domains}
  - 受影响服务: {affected_services}
  - 错误率: {error_rate}%
- 已完成诊断:
  - Phase 1 资源状态检查: {phase1_summary}
  - Phase 2 深度检查: {phase2_summary}
  - Phase 3 高级诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-NET-003 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.5）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
4. **关键资源快照**:
   ```bash
   # Ingress 描述
   kubectl describe ingress <ingress-name> -n <namespace> > ingress-describe.txt
   # Gateway 状态
   kubectl get gateway <gateway-name> -n <namespace> -o yaml > gateway-status.yaml
   # HTTPRoute 状态
   kubectl get httproute <route-name> -n <namespace> -o yaml > httproute-status.yaml
   # Controller 日志
   kubectl logs -n ingress-nginx deploy/ingress-nginx-controller --tail=500 > controller-logs.txt
   # TLS 证书信息
   openssl s_client -connect <domain>:443 2>/dev/null | openssl x509 -noout -text > cert-info.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Ingress API | networking.k8s.io/v1 (GA) | v1 | v1 | v1 | v1 |
| Gateway API | v1beta1 (GA) | v1beta1 | v1 (GA) | v1 | v1 |
| HTTPRoute | v1beta1 | v1beta1 | v1 (GA) | v1 | v1 |
| GRPCRoute | v1alpha2 | v1alpha2 | v1alpha2 | v1 (GA) | v1 |
| TLSRoute | v1alpha2 | v1alpha2 | v1alpha2 | v1alpha2 | v1beta1 |
| ReferenceGrant | v1beta1 (GA) | v1beta1 | v1beta1 | v1beta1 | v1 |
| IngressClass | v1 (GA) | v1 | v1 | v1 | v1 |
| Backend TLS Policy | N/A | alpha | alpha | beta | beta |
| Gateway API Mesh | N/A | N/A | experimental | experimental | alpha |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get gateway` | 需安装 Gateway API CRDs | 同左 | 同左，推荐 v1 API | 同左 | 同左 |
| `kubectl get httproute` | v1beta1 | v1beta1 | v1 GA | v1 | v1 |
| `kubectl get grpcroute` | v1alpha2 | v1alpha2 | v1alpha2 | v1 GA | v1 |
| `kubectl get ingressclass` | v1 (GA) | 同左 | 同左 | 同左 | 同左 |
| EndpointSlice 默认 | 是 | 是 | 是 | 是 | 是 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Ingress | networking.k8s.io/v1 | v1 | v1 | v1 | v1 |
| IngressClass | networking.k8s.io/v1 | v1 | v1 | v1 | v1 |
| Gateway | gateway.networking.k8s.io/v1beta1 | v1beta1 | v1 | v1 | v1 |
| HTTPRoute | gateway.networking.k8s.io/v1beta1 | v1beta1 | v1 | v1 | v1 |
| GRPCRoute | gateway.networking.k8s.io/v1alpha2 | v1alpha2 | v1alpha2 | v1 | v1 |
| ReferenceGrant | gateway.networking.k8s.io/v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1 |

### 9.4 版本相关的诊断注意事项

- **[v1.28]**: Gateway API v1beta1 GA。Ingress Class 参数支持。注意旧版 `kubernetes.io/ingress.class` 注解仍被支持但不推荐。

- **[v1.29]**: 改进的 Ingress 状态报告。Gateway API 引入更多 Condition 类型用于细粒度状态诊断。

- **[v1.30]**: Gateway API v1 GA。HTTPRoute 升级为 v1。诊断时注意 API 版本变更：
  - 使用 `gateway.networking.k8s.io/v1` 替代 `v1beta1`
  - `kubectl get httproute -o yaml` 输出格式可能略有变化

- **[v1.31]**: GRPCRoute GA。gRPC 流量管理更成熟：
  - 诊断 gRPC 问题时检查 GRPCRoute 而非 HTTPRoute
  - 新增 `GRPCMethodMatch` 支持更精细的路由

- **[v1.32]**: TLSRoute 升级为 v1beta1。Backend TLS Policy beta：
  - 后端 TLS 配置更灵活
  - 诊断后端 HTTPS 问题时检查 BackendTLSPolicy 资源

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **后端 Pod 问题误判为 Ingress 配置问题** | 请求返回 502，初步判断为 Ingress 规则错误 | 后端 Pod 正在重启或不健康，Service Endpoints 为空或不稳定 | 先执行 D2.3 检查 Endpoints 状态。502 首先检查后端可用性，再检查 Ingress 配置 |
| **云 LB 问题误判为 Controller 故障** | 外部完全无法访问，Controller 日志正常 | 云厂商 LoadBalancer 健康检查失败或安全组配置问题 | 在 D1.6 外部测试失败但 Controller 日志正常时，检查云控制台 LB 状态 |
| **DNS 缓存导致的"故障"** | 修改后访问仍返回旧响应 | DNS 记录 TTL 未过期，客户端使用缓存的旧 IP | 使用 `curl -H "Host: domain" http://ingress-ip` 直接测试，绕过 DNS |
| **CDN/WAF 层问题误判为 Ingress 问题** | HTTPS 请求失败，Ingress 看似正常 | CDN 或 WAF 层的 SSL 证书或规则问题 | 确认请求路径：客户端 → CDN → LB → Ingress → Pod。逐层排查 |
| **pathType 配置不当** | Path 看似正确但匹配不到 | `pathType: Exact` 但请求带 trailing slash，或反之 | D2.2 检查 Nginx 实际生成的 location 配置，确认 pathType 与实际请求匹配 |
| **跨命名空间引用被拒绝** | HTTPRoute 状态 Accepted=False，误认为 Gateway 配置问题 | 缺少 ReferenceGrant 授权跨命名空间引用 | D1.5 检查 HTTPRoute 状态的具体 Reason，`RefNotPermitted` 明确指向权限问题 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Ingress 故障排查深度指南 | `domain-12-troubleshooting/15-ingress-troubleshooting.md` | 超出本 Skill 覆盖范围的深度排查 |
| Kubernetes 网络模型 | `domain-5-networking/` | 理解 Service、Endpoints、网络策略 |
| Gateway API 详解 | `domain-40-cloud-native-api-gateway/` | Gateway API 概念和最佳实践 |
| TLS/证书管理 | `SKILL-SEC-001` | 证书过期和 TLS 配置问题 |
| Service 网络排障 | `SKILL-NET-001` | Service 层面的网络问题 |
| Pod 故障诊断 | `SKILL-POD-001` | 后端 Pod 问题导致的 502/503 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，支持 Nginx/Traefik/ALB/Envoy Gateway，包含 12 个根因、11 个修复操作 | 基于生产环境 Ingress 故障分析，建立标准化诊断流程 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **Service Mesh Ingress**: Istio Gateway、Linkerd 等 Service Mesh 的 Ingress 诊断
2. **多集群 Ingress**: 跨集群路由和全局负载均衡的故障诊断
3. **边缘场景**: KubeEdge 等边缘计算场景的 Ingress 特殊性
4. **自定义 Ingress Controller**: 非主流 Controller 的诊断差异
5. **API Gateway 高级功能**: 认证、授权、限流、熔断等高级功能的故障诊断
