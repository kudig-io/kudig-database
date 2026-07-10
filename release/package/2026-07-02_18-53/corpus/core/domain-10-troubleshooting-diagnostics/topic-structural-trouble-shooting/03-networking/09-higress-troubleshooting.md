---
title: Higress 网关故障排查指南 [topic-structural-trouble-shooting]
description: 'title: Higress 网关故障排查指南'
summary: 'title: Higress 网关故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- istio
- envoy
- flannel
- docker
- ingress
- gateway
- crd
- wasm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- Higress 网关故障排查指南 是什么
- 如何 Higress 网关故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Higress 网关故障排查指南 故障排查
- Higress 网关故障排查指南 排障步骤
trigger_keywords:
- Higress
- 网关故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Higress 网关故障排查指南
description: Higress 云原生 API 网关故障排查指南，覆盖路由配置、xDS 推送、服务发现、Wasm 插件、AI 网关等问题场景
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- higress
- [[Ingress|ingress]]
- gateway
- [[Envoy|envoy]]
- wasm
- nacos
- xds
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 后端工程师
estimated_read_time: 10min
intent_queries:
- Higress 网关故障排查 是什么
- Higress 路由配置问题 排查
- Higress xDS 推送失败 排查
- Higress Wasm 插件问题 排查
trigger_keywords:
- Higress
- 网关
- 故障排查
- Envoy
- Ingress
- Wasm
- Nacos
- xDS
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Higress 网关故障排查指南

> **适用版本**: Higress v1.x - v2.x | **最后更新**: 2026-05 | **难度**: 高级

---

## 目录

1. [10 分钟快速诊断](#10-分钟快速诊断)
2. [架构与核心组件](#架构与核心组件)
3. [问题场景与排查步骤](#问题场景与排查步骤)
4. [配置参考](#配置参考)

---

## 10 分钟快速诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Higress 系统组件状态
kubectl get pods -n higress-system

# 2. 查看 Higress 网关日志
kubectl logs -n higress-system -l app=higress-gateway --tail=100 -f

# 3. 检查 Ingress 资源
kubectl get ingress -A
kubectl describe ingress <name> -n <namespace>

# 4. 检查 Endpoints
kubectl get endpoints <service> -n <namespace>

# 5. 测试路由
kubectl exec -it <test-pod> -- curl -H "Host: app.example.com" http://<higress-gateway>:80/

# 6. 检查 McpBridge 配置
kubectl get mcphbridge -A

# 7. 检查 WasmPlugin 配置
kubectl get wasmplugin -A

# 8. 查看 Envoy 配置
kubectl exec -it <higress-gateway-pod> -c envoy -- curl localhost:15000/config_dump
```
---

## 架构与核心组件

```
┌─────────────────────────────────────────────────────────────┐
│                    Higress 架构                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  配置层:                                                     │
│  ├── Higress Controller (CRD 控制器)                       │
│  ├── Ingress / GatewayClass / McpBridge CR                  │
│  └── Istiod (xDS 配置下发)                                  │
│                                                             │
│  数据面:                                                     │
│  └── Higress Gateway (Envoy based)                          │
│      ├── Listener (80/443)                                  │
│      ├── Route (路由规则)                                   │
│      ├── Cluster (Upstream)                                 │
│      └── Wasm Plugin (过滤器链)                              │
│                                                             │
│  服务发现:                                                   │
│  ├── Kubernetes Endpoint                                    │
│  ├── Nacos (阿里云注册中心)                                 │
│  └── Consul (可选)                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 问题场景与排查步骤

### 场景 1: Ingress 路由不生效

**现象**: 配置了 Ingress 但请求返回 404 或被拒绝

**排查步骤**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 Ingress 配置
kubectl get ingress <name> -n <namespace> -o yaml

# Step 2: 检查 IngressClass
kubectl get ingressclass
kubectl describe ingressclass higress

# Step 3: 检查 Higress Controller 状态
kubectl get pods -n higress-system -l app=higress-controller

# Step 4: 查看 Controller 日志
kubectl logs -n higress-system -l app=higress-controller --tail=200

# Step 5: 检查 Envoy Listener 配置
kubectl exec -it <higress-gateway-pod> -c envoy -- \
  curl localhost:15000/config_dump?resource=dynamic_listeners

# Step 6: 检查路由表
kubectl exec -it <higress-gateway-pod> -c envoy -- \
  curl localhost:15000/config_dump?resource=dynamic_routes
```
**常见原因**:
- IngressClass 未指定或不存在
- Host/path 匹配规则错误
- 证书 Secret 未正确挂载

---

### 场景 2: xDS 配置未推送

**现象**: Envoy 未收到路由配置，请求返回 404

**排查步骤**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 Istiod 状态
kubectl get pods -n istio-system

# Step 2: 检查 MCP Bridge 配置
kubectl get mcphbridge -A
kubectl describe mcphbridge <name> -n <namespace>

# Step 3: 检查 Endpoints 同步
kubectl get endpoints -n <namespace>

# Step 4: 查看 xDS 状态
kubectl exec -it <higress-gateway-pod> -c envoy -- \
  curl localhost:15000/clusters

# Step 5: 检查 CDS (Cluster Discovery Service)
curl localhost:15000/config_dump?resource=dynamic_clusters

# Step 6: 检查 EDS (Endpoint Discovery Service)
curl localhost:15000/config_dump?resource=dynamic_endpoints
```
**常见原因**:
- Istiod 未正常运行
- McpBridge 配置错误
- Endpoint 信息为空

---

### 场景 3: 服务发现失败 (Nacos)

**现象**: Nacos 注册的服务无法被路由

**排查步骤**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 McpBridge CR 状态
kubectl get mcphbridge -A -o yaml

# Step 2: 测试 Nacos 连接
kubectl exec -it <higress-gateway-pod> -- \
  curl nacos:8848/v1/ns/instance/list?serviceName=<svc>

# Step 3: 检查 Nacos 服务实例
kubectl exec -it <higress-gateway-pod> -- \
  curl -s "http://nacos:8848/nacos/v1/ns/instance/list?serviceName=<service>" | jq

# Step 4: 检查 K8s Endpoint 同步
kubectl get endpoints <service> -n <namespace> -o yaml

# Step 5: 检查服务标签匹配
# Ingress 的 host/path 是否与服务标签匹配
kubectl get svc <service> -n <namespace> --show-labels
```
**常见原因**:
- Nacos 连接配置错误
- 服务实例列表为空
- 标签选择器不匹配

---

### 场景 4: Wasm 插件加载失败

**现象**: 配置了 Wasm 插件但请求报错 500

**排查步骤**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查插件 OCI 镜像可访问性
crictl images | grep <plugin-image>

# Step 2: 检查插件配置
kubectl get wasmplugin -A
kubectl describe wasmplugin <name>

# Step 3: 查看 Envoy 日志
kubectl logs -n higress-system <pod> -c envoy | grep -i wasm

# Step 4: 检查插件超时配置
kubectl get wasmplugin <name> -o jsonpath='{.spec.config}' | jq

# Step 5: 测试插件加载
kubectl exec -it <higress-gateway-pod> -c envoy -- \
  curl localhost:15000/stats | grep wasm
```
**常见原因**:
- OCI 镜像无法下载
- 插件配置格式错误
- 插件执行超时

---

### 场景 5: TLS 证书问题

**现象**: HTTPS 请求返回 400 或握手失败

**排查步骤**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 检查 TLS Secret
kubectl get secret -n higress-system | grep -E "tls|cert"

# Step 2: 检查证书有效期
kubectl exec -it <higress-gateway-pod> -- \
  openssl x509 -in /etc/nginx/secrets/<secret-name>/tls.crt -noout -dates

# Step 3: 检查证书与 host 是否匹配
kubectl exec -it <higress-gateway-pod> -- \
  openssl s_client -connect localhost:443 -servername <host> -showcerts

# Step 4: 检查 secret 挂载
kubectl describe pod <higress-gateway-pod> -n higress-system | grep -A10 "Mounts"

# Step 5: 测试 TLS 连接
curl -v --insecure https://<higress-gateway>:443 -H "Host: <host>"
```
**常见原因**:
- 证书过期
- 证书与 host 不匹配
- Secret 未挂载到 Pod

---

### 场景 6: AI 网关能力问题 (LLM 代理)

**现象**: AI 网关路由到 LLM Provider 失败

**排查步骤**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查 AI Provider 连接
curl -X POST http://<higress-gateway>:80/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model":"gpt-4","messages":[{"role":"user","content":"test"}]}'

# Step 2: 检查 Token 限流配置
kubectl get globalrateLimit -A

# Step 3: 检查语义缓存配置
kubectl get semanticcache -A

# Step 4: 查看 LLM Provider 日志
kubectl logs -n higress-system <pod> | grep -i "openai|llm"

# Step 5: 检查 AI 路由规则
kubectl get aiportal -A
```
**常见原因**:
- LLM Provider API Key 配置错误
- Token 限流触发
- 网络连接问题

---

## 配置参考

### Ingress 配置示例

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    higress.io/backend-timeout: "300"
    higress.io/rate-limit-name: "app-rate-limit"
spec:
  ingressClassName: higress
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
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

### McpBridge 配置示例 (Nacos)

```yaml
apiVersion: networking.higress.io/v1
kind: McpBridge
metadata:
  name: nacos-bridge
  namespace: default
spec:
  registries:
  - name: nacos
    type: nacos
    endpoint: nacos:8848
    nacosGroups:
    - DEFAULT_GROUP
```

### WasmPlugin 配置示例

```yaml
apiVersion: networking.higress.io/v1
kind: WasmPlugin
metadata:
  name: my-wasm-plugin
  namespace: higress-system
spec:
  phase: GLOBAL
  priority: 100
  config:
    image: oci://registry/example/wasm-plugin:v1.0.0
    phase: REQUEST
    config:
      key: value

```

---

## 快速诊断检查清单

| 检查项 | 命令 | 预期结果 |
|:---|:---|:---|
| 系统组件 | `kubectl get pods -n higress-system` | 所有 Pod Running |
| Controller | `kubectl logs -n higress-system -l app=higress-controller` | 无 Error |
| 网关日志 | `kubectl logs -n higress-system -l app=higress-gateway` | 无异常 |
| Ingress | `kubectl get ingress -A` | 规则正常 |
| Endpoints | `kubectl get endpoints -A` | 有 IP 列表 |
| xDS | `kubectl exec -it <pod> -c envoy -- curl localhost:15000/clusters` | 有集群信息 |

---

## 相关文档

- [Higress 企业级网关实践](./domain-03-networking-traffic/04-higress-enterprise-gateway.md)
- Higress FTA 故障树](./domain-10-troubleshooting-diagnostics/topic-fta/list/higress-fta.md)
- [Higress 全局索引](./domain-19-landscape-references/领域索引/higress-index.md)
- [Ingress 通用故障排查](./[[domain-10-troubleshooting-diagnostics/高级排障/03-networking/03-service-ingress-troubleshooting.md|03-service-ingress-troubleshooting]].md)

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[index|index]]
- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/07-terway-troubleshooting.md|07-terway-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/08-flannel-troubleshooting.md|08-flannel-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/09-nginx-ingress-troubleshooting.md|09-nginx-ingress-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/01-cni-troubleshooting.md|01-cni-troubleshooting]]

```

<!-- risk-assessed -->
