---
title: 04 - Higress 云原生 API 网关企业级实践
description: '# 04 - Higress 云原生 API 网关企业级实践'
summary: '4. [Kubernetes 集群部署](#4-kubernetes-集群部署)'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- etcd
- scheduler
- prometheus
- istio
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- Higress 云原生 API 网关企业级实践 是什么
- 如何 Higress 云原生 API 网关企业级实践
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Higress
- 云原生
- API
- 网关企业级实践
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- etcd-basics
- redis-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/higress-fta.md
  label: '故障树: higress'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 04 - Higress 云原生 API 网关企业级实践

> **文档版本**: v2.0 | **适用版本**: Higress v1.x - v2.x, [[kubernetes|Kubernetes]] 1.25+ | **更新日期**: 2026-03-04 | **关键词**: Higress, [[envoy|Envoy]], Istiod, Wasm, AI Gateway, Gateway API, xDS, 阿里云, McpBridge

<!-- chunk: 目录 -->## 目录

1. [Higress 项目概述](#1-higress-项目概述)
2. [核心架构与原理](#2-核心架构与原理)
3. [Mac 快速 Demo（5 分钟上手）](#3-mac-快速-demo5-分钟上手)
4. [Kubernetes 集群部署](#4-kubernetes-集群部署)
5. [路由配置详解](#5-路由配置详解)
6. [服务发现与注册中心对接](#6-服务发现与注册中心对接)
7. [插件生态](#7-插件生态)
8. [Wasm 插件开发实战](#8-wasm-插件开发实战)
9. [AI 网关能力](#9-ai-网关能力)
10. [Gateway API 集成](#10-gateway-api-集成)
11. [可观测性](#11-可观测性)
12. [生产环境调优](#12-生产环境调优)
13. [与 [[istio|Istio]] 协同](#13-与-istio-协同)
14. [常见故障排查](#14-常见故障排查)
15. [与竞品横向对比](#15-与竞品横向对比)

---

<!-- chunk: 1. Higress 项目概述 -->## 1. Higress 项目概述

Higress 是阿里巴巴开源的云原生 API 网关，基于 Istio 和 Envoy 构建，2022 年开源，2023 年进入 CNCF Sandbox。其命名源自 "High" + "[[ingress|Ingress]]"，寓意高性能的入口网关。

## 核心定位

| 定位 | 说明 |
|------|------|
| **云原生 API 网关** | 面向 Kubernetes 环境的南北向流量管理，替代传统 Nginx Ingress |
| **AI 网关** | 原生支持 LLM 代理路由、Token 级限流、多模型 Fallback、语义缓存 |
| **Ingress 控制器** | 同时兼容 Kubernetes Ingress、Gateway API、自有 CRD 三种配置方式 |
| **微服务网关** | 通过 McpBridge 对接 Nacos/Consul/Eureka 等注册中心，覆盖 Spring Cloud 等传统微服务架构 |

## 核心特点

- **基于 Envoy + Istiod**: 复用 Envoy 高性能数据平面和 Istio 成熟的 xDS 配置下发链路，经过阿里内部大规模验证
- **Wasm 插件一等支持**: 插件以 OCI 镜像分发，热加载无需重启网关，支持 Go/Rust/C++ 等多语言
- **AI 网关原生集成**: 非后加功能，而是核心能力，支持 OpenAI/通义千问/Claude 等主流 LLM 协议
- **兼容性极强**: 高度兼容 nginx-ingress 注解，迁移成本最低
- **阿里云生态**: MSE 提供全托管商业版，与 ACK/ASM 深度集成

## 发展历程

| 时间 | 里程碑 |
|------|--------|
| 2022-10 | 阿里巴巴开源 Higress（源自内部 3 年+ API 网关实践） |
| 2023-05 | 发布 v1.0，支持 Ingress + 自定义 CRD |
| 2023-10 | 进入 CNCF Sandbox |
| 2024-06 | 发布 AI 网关能力（ai-proxy / ai-token-ratelimit） |
| 2025-03 | Gateway API v1.1 Extended 一致性通过 |
| 2025-12 | v2.0 发布，架构重构，独立控制面 |

---

<!-- chunk: 2. 核心架构与原理 -->## 2. 核心架构与原理

## 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Higress 架构总览                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │                      控制平面                                │     │
│  │  ┌──────────────────────┐  ┌──────────────────────┐        │     │
│  │  │  Higress Controller  │  │  Istiod (定制版)       │        │     │
│  │  │  ┌────────────────┐  │  │  ┌────────────────┐  │        │     │
│  │  │  │ Ingress 转换器  │  │  │  │ ServiceEntry   │  │        │     │
│  │  │  │ CRD 控制器     │  │  │  │ 生成器          │  │        │     │
│  │  │  │ Gateway API    │  │  │  │ xDS Server     │  │        │     │
│  │  │  │ 适配器          │  │  │  │ 证书签发器      │  │        │     │
│  │  │  │ McpBridge      │  │  │  │ 配置验证器      │  │        │     │
│  │  │  │ 注册中心同步    │  │  │  │                │  │        │     │
│  │  │  └────────────────┘  │  │  └────────────────┘  │        │     │
│  │  └──────────┬───────────┘  └──────────┬───────────┘        │     │
│  │             │ Istio API (内部模型)    │ xDS (gRPC)         │     │
│  └─────────────│─────────────────────────│────────────────────┘     │
│                │                         │                          │
│  ┌─────────────▼─────────────────────────▼────────────────────┐     │
│  │                      数据平面                                │     │
│  │  ┌──────────────────────────────────────────────────┐      │     │
│  │  │                  Envoy Proxy                      │      │     │
│  │  │                                                  │      │     │
│  │  │   Listener ──► Filter Chain ──► Router ──► Cluster│      │     │
│  │  │   (端口监听)    (请求处理链)     (路由匹配) (上游集群) │      │     │
│  │  │                                                  │      │     │
│  │  │   Filter Chain 包含:                              │      │     │
│  │  │   ├─ Wasm Filter (自定义插件)                      │      │     │
│  │  │   ├─ RBAC Filter (鉴权)                           │      │     │
│  │  │   ├─ RateLimit Filter (限流)                      │      │     │
│  │  │   ├─ CORS Filter (跨域)                           │      │     │
│  │  │   └─ Router Filter (路由转发)                      │      │     │
│  │  └──────────────────────────────────────────────────┘      │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                     │
│  ┌──────────────────────┐                                           │
│  │  Higress Console     │  Web 管理控制台（可选）                      │
│  │  ├─ 路由管理         │                                           │
│  │  ├─ 插件配置         │                                           │
│  │  ├─ 证书管理         │                                           │
│  │  ├─ 服务发现         │                                           │
│  │  └─ AI 网关配置      │                                           │
│  └──────────────────────┘                                           │
└─────────────────────────────────────────────────────────────────────┘
```

## 2.2 核心组件职责

| 组件 | 技术栈 | 职责 | 原理说明 |
|------|--------|------|---------|
| **Higress Controller** | Go | 监听 K8s API Server，将 Ingress/CRD/Gateway API 资源转换为 Istio 内部模型（VirtualService/DestinationRule 等） | Watch-Convert-Push 模式，增量同步 |
| **Istiod (定制版)** | Go | 基于 Istio Pilot 定制的控制平面，将 Istio 内部模型编译为 xDS 配置并通过 gRPC 双向流推送给 Envoy | 聚合 LDS/RDS/CDS/EDS 四类 xDS 资源 |
| **Envoy Proxy** | C++ | 高性能数据平面，处理实际流量请求。基于 Filter Chain 架构，每个请求经过一系列 Filter 处理 | 零拷贝转发、连接池复用、异步非阻塞 |
| **Higress Console** | Java/React | 可选的 Web 管理控制台，提供图形化配置能力 | 通过 K8s API 读写 CRD |

## 2.3 xDS 配置下发原理

```
配置变更流程:

  用户创建 Ingress/HTTPRoute
        │
        ▼
  K8s API Server (etcd 存储)
        │
        ▼
  Higress Controller (Watch 事件)
        │  转换为 Istio 内部模型
        ▼
  Istiod (配置聚合 + 编译)
        │  通过 xDS gRPC 双向流推送
        ▼
  Envoy Proxy (增量更新生效)
        │  无需重启，毫秒级生效
        ▼
  流量按新规则转发

xDS 资源类型:
  ├─ LDS (Listener Discovery Service)    → 监听端口和 Filter Chain 配置
  ├─ RDS (Route Discovery Service)       → 路由规则（域名、路径匹配）
  ├─ CDS (Cluster Discovery Service)     → 上游集群（后端服务）
  ├─ EDS (Endpoint Discovery Service)    → 具体 Pod IP 列表
  └─ SDS (Secret Discovery Service)      → TLS 证书和密钥
```

## 2.4 与 nginx-ingress 架构对比

| 维度 | nginx-ingress | Higress |
|------|--------------|---------|
| **配置更新** | 修改 nginx.conf → reload（有抖动） | xDS 推送（零抖动、无连接中断） |
| **扩展方式** | Lua/Annotation/配置片段 | Wasm 插件（安全沙箱、热加载） |
| **服务发现** | 仅 K8s Endpoints | K8s + Nacos/Consul/Eureka/DNS |
| **连接管理** | Reload 断连 | 配置热更新不断连 |
| **性能模型** | 多 Worker 进程 | 多 Worker 线程（共享内存更高效） |

---

<!-- chunk: 3. Mac 快速 Demo（5 分钟上手） -->## 3. Mac 快速 Demo（5 分钟上手）

> 目标：在 Mac 上零 K8s 依赖，快速体验 Higress 核心功能。

## 方式一：Docker All-in-One（最快）

> **⚠️ 镜像仓库说明**: Higress all-in-one 镜像**仅托管在阿里云中国区镜像仓库** (`higress-registry.cn-hangzhou.cr.aliyuncs.com`)，Docker Hub 上不存在该镜像。海外网络环境拉取时可能遇到 EOF 错误，重试几次通常可解决。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 前提：已安装 Docker Desktop for Mac
# 确认 Docker 运行中
docker info

# 创建工作目录（用于持久化配置，官方推荐）
mkdir -p ~/higress && cd ~/higress

# 一键启动 Higress（All-in-One 模式，内置 Nacos）
# -v ${PWD}:/data 挂载数据卷，持久化网关配置
docker run -d --rm --name higress-ai -v ${PWD}:/data \
  -p 8001:8001 -p 8080:8080 -p 8443:8443 \
  higress-registry.cn-hangzhou.cr.aliyuncs.com/higress/all-in-one:latest

# 等待约 30 秒启动完成，检查状态
docker logs -f higress-ai
# 看到各组件启动完成的日志后，按 Ctrl+C 退出日志跟踪
```
**拉取失败排查（常见于海外网络）：**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 错误示例 1: EOF（网络中断，重试即可）
# Error: failed to fetch anonymous token: ... EOF

# 错误示例 2: pull access denied（镜像名错误）
# 注意：镜像不在 Docker Hub，不要使用 higress/all-in-one

# 备选方案：使用官方安装脚本（自带重试机制）
curl -fsSL https://higress.io/standalone/get-higress.sh | bash -s -- -a
```
**验证安装成功（四步确认）：**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1：确认容器运行中
docker ps --filter name=higress-ai
# 确认 STATUS 列显示 Up，三个端口 (8001, 8080, 8443) 均已映射

# 步骤 2：测试 HTTP 网关端口
curl -s -o /dev/null -w "HTTP Status: %{http_code}\n" http://localhost:8080
# 预期输出: HTTP Status: 200
# 返回 Higress 欢迎页面 "Thanks for using Higress!"

# 步骤 3：测试 HTTPS 网关端口
curl -sk -o /dev/null -w "HTTP Status: %{http_code}\n" https://localhost:8443
# 预期输出: HTTP Status: 200
# TLS 1.3 握手成功，自签证书 CN=higress-gateway

# 步骤 4：访问管理控制台
# 浏览器打开: http://localhost:8001
# ⚠️ 首次访问需初始化管理员账号（设置用户名和密码），非默认 admin/admin
```
**实测验证输出参考：**

```bash
$ curl -v http://localhost:8080
# 关键响应信息：
# < HTTP/1.1 200 OK
# < content-type: text/html;charset=ISO-8859-1
# < server: istio-envoy          ← 确认 Envoy 数据平面正常
# <html>
#   <h1>Thanks for using Higress!</h1>
#   <p>Higress is successfully installed and is functioning properly.</p>
# </html>

$ curl -vk https://localhost:8443
# 关键响应信息：
# * SSL connection using TLSv1.3  ← TLS 1.3 握手成功
# * Server certificate: CN=higress-gateway
# < HTTP/1.1 200 OK
# < server: istio-envoy
```

## 方式二：Docker Compose（带后端服务完整 Demo）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 demo 目录
mkdir -p ~/higress-demo && cd ~/higress-demo

# 创建 docker-compose.yaml
cat > docker-compose.yaml << 'EOF'
version: '3.8'
services:
  # Higress 网关
  higress:
    image: higress-registry.cn-hangzhou.cr.aliyuncs.com/higress/all-in-one:latest
    volumes:
      - ./higress-data:/data    # 持久化配置
    ports:
      - "8001:8001"   # Console
      - "8080:8080"   # HTTP
      - "8443:8443"   # HTTPS
    networks:
      - higress-net

  # 后端服务 1（httpbin，用于测试）
  httpbin:
    image: kennethreitz/httpbin
    networks:
      - higress-net

  # 后端服务 2（echo server）
  echo:
    image: ealen/echo-server:latest
    environment:
      - PORT=3000
    networks:
      - higress-net

networks:
  higress-net:
    driver: bridge
EOF

# 启动所有服务
docker compose up -d

# 等待所有服务就绪
docker compose ps
```
**通过 Console 配置路由：**

1. 浏览器打开 `http://localhost:8001`，使用 admin/admin 登录
2. 进入「服务来源」→ 添加固定地址服务：`httpbin:80`（服务名：httpbin）
3. 进入「路由配置」→ 新建路由：
   - 域名：`demo.example.com`
   - 路径前缀：`/`
   - 目标服务：httpbin

```bash
# 通过 Higress 访问 httpbin
curl -H "Host: demo.example.com" http://localhost:8080/get

# 预期返回 httpbin 的 JSON 响应:
# {
#   "args": {},
#   "headers": {
#     "Host": "demo.example.com",
#     ...
#   },
#   "url": "http://demo.example.com/get"
# }

# 测试请求头传递
curl -H "Host: demo.example.com" \
     -H "X-Custom: hello-higress" \
     http://localhost:8080/headers

# 测试 POST 请求
curl -X POST -H "Host: demo.example.com" \
     -H "Content-Type: application/json" \
     -d '{"msg":"hello"}' \
     http://localhost:8080/post
```

## 方式三：Kind + Helm（Mac 上的完整 K8s 体验）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 前提：安装 kind 和 helm
brew install kind helm kubectl

# 创建 Kind 集群
kind create cluster --name higress-demo --config - << 'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  extraPortMappings:
  - containerPort: 80
    hostPort: 80
    protocol: TCP
  - containerPort: 443
    hostPort: 443
    protocol: TCP
EOF

# 安装 Higress
helm repo add higress https://higress.io/helm-charts
helm repo update

helm install higress higress/higress \
  -n higress-system --create-namespace \
  --set global.local=true \
  --set higress-core.gateway.replicas=1 \
  --set higress-core.controller.replicas=1

# 等待 Pod 就绪（约 2-3 分钟）
kubectl wait --for=condition=ready pod -l app=higress-gateway \
  -n higress-system --timeout=300s

# 部署测试应用
kubectl create deployment httpbin --image=kennethreitz/httpbin --port=80
kubectl expose deployment httpbin --port=80

# 创建 Ingress 路由
kubectl apply -f - << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: httpbin-demo
spec:
  ingressClassName: higress
  rules:
  - host: demo.localdev.me
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: httpbin
            port:
              number: 80
EOF

# 测试路由（localdev.me 自动解析到 127.0.0.1）
curl http://demo.localdev.me/get

# 清理
kind delete cluster --name higress-demo
```
---

<!-- chunk: 4. Kubernetes 集群部署 -->## 4. Kubernetes 集群部署

## Helm 安装（生产推荐）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm 仓库
helm repo add higress https://higress.io/helm-charts
helm repo update

# 安装 Higress（生产参数）
helm install higress higress/higress \
  -n higress-system \
  --create-namespace \
  --set global.local=false \
  --set higress-core.gateway.replicas=2 \
  --set higress-core.gateway.resources.requests.cpu=2 \
  --set higress-core.gateway.resources.requests.memory=2Gi \
  --set higress-core.gateway.resources.limits.cpu=4 \
  --set higress-core.gateway.resources.limits.memory=4Gi

# 安装 Higress Console（可选但推荐）
helm install higress-console higress/higress-console \
  -n higress-system
```
## 验证安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Pod 状态
kubectl get pods -n higress-system
# 预期输出:
# higress-controller-xxx    1/1  Running
# higress-gateway-xxx       1/1  Running
# higress-gateway-xxx       1/1  Running

# 检查 Gateway 服务
kubectl get svc -n higress-system higress-gateway

# 获取 Gateway 外部 IP
export GATEWAY_IP=$(kubectl get svc higress-gateway -n higress-system \
  -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
echo "Gateway IP: $GATEWAY_IP"

# 检查 xDS 连接状态
kubectl exec -n higress-system deploy/higress-gateway -- \
  curl -s localhost:15000/clusters | head -20
```
---

<!-- chunk: 5. 路由配置详解 -->## 5. 路由配置详解

## 方式一：Kubernetes Ingress（最简单）

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo
  annotations:
    # Higress 特有注解
    higress.io/exact-match-header-x-env: gray     # 精确匹配请求头
    higress.io/upstream-vhost: "internal.svc"      # 覆盖上游 Host 头
    # 兼容 nginx-ingress 注解
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
spec:
  ingressClassName: higress
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: demo-service
            port:
              number: 8080
```

## 方式二：Gateway API（标准化推荐）

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: demo-route
spec:
  parentRefs:
  - name: higress-gateway
    namespace: higress-system
  hostnames:
  - "demo.example.com"
  rules:
  # 规则 1：/api/v1 → backend-v1
  - matches:
    - path:
        type: PathPrefix
        value: /api/v1
    backendRefs:
    - name: backend-v1
      port: 8080
  # 规则 2：/api/v2 → backend-v2（带请求头匹配）
  - matches:
    - path:
        type: PathPrefix
        value: /api/v2
      headers:
      - name: x-api-version
        value: "2"
    backendRefs:
    - name: backend-v2
      port: 8080
```

## 金丝雀发布（权重路由）

```yaml
# Ingress 注解方式
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-canary
  annotations:
    higress.io/canary: "true"
    higress.io/canary-weight: "20"           # 20% 流量到 v2
    higress.io/canary-header: x-canary       # 或通过 Header 匹配
    higress.io/canary-header-value: "true"
spec:
  ingressClassName: higress
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: demo-service-v2
            port:
              number: 8080
```

```yaml
# Gateway API 方式（更标准）
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: canary-route
spec:
  parentRefs:
  - name: higress-gateway
    namespace: higress-system
  hostnames: ["demo.example.com"]
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: demo-service-v1
      port: 8080
      weight: 80         # 80% 到 v1
    - name: demo-service-v2
      port: 8080
      weight: 20         # 20% 到 v2
```

---

<!-- chunk: 6. 服务发现与注册中心对接 -->## 6. 服务发现与注册中心对接

Higress 的核心差异化能力之一是通过 **McpBridge** CRD 对接传统微服务注册中心，让 Nacos/Consul/Eureka 注册的服务无需 K8s Service 即可被网关路由。

## 原理

```
McpBridge CRD → Higress Controller → 轮询注册中心 API
                                          │
                                          ▼
                                    生成 ServiceEntry (Istio CRD)
                                          │
                                          ▼
                                    Istiod 编译 xDS (CDS/EDS)
                                          │
                                          ▼
                                    Envoy 直接路由到注册中心的实例 IP
```

## Nacos 对接

```yaml
apiVersion: networking.higress.io/v1
kind: McpBridge
metadata:
  name: nacos-bridge
  namespace: higress-system
spec:
  registries:
  - name: nacos-prod
    type: nacos2          # nacos2 协议（推荐），也支持 nacos
    domain: nacos.example.com
    port: 8848
    nacosNamespaceId: ""  # 空字符串表示 public 命名空间
    nacosGroups:
    - DEFAULT_GROUP
    - PROD_GROUP
```

## Consul 对接

```yaml
apiVersion: networking.higress.io/v1
kind: McpBridge
metadata:
  name: consul-bridge
  namespace: higress-system
spec:
  registries:
  - name: consul-prod
    type: consul
    domain: consul.example.com
    port: 8500
    consulDatacenter: dc1
```

---

<!-- chunk: 7. 插件生态 -->## 7. 插件生态

## 内置插件分类

| 类别 | 内置插件 | 说明 |
|------|---------|------|
| **认证鉴权** | key-auth, jwt-auth, hmac-auth, basic-auth, oidc | 支持消费者(Consumer)概念，可按路由/域名粒度配置 |
| **流量管控** | key-rate-limit, request-block, bot-detect | 支持本地限流和 Redis 分布式限流 |
| **安全防护** | waf, cors, ip-restriction, csrf | WAF 基于 ModSecurity 规则引擎 |
| **协议转换** | transformer, request-validation, response-rewrite | 请求/响应 Header、Body 改写 |
| **可观测性** | prometheus, request-log, skywalking | 自定义指标和日志格式 |
| **AI 网关** | ai-proxy, ai-token-ratelimit, ai-cache, ai-prompt-template, ai-statistics | LLM 专属插件集 |

## 插件配置示例（WasmPlugin CRD）

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: custom-auth
  namespace: higress-system
spec:
  url: oci://registry.example.com/higress/custom-auth:v1.0
  phase: AUTHN             # 执行阶段：AUTHN → AUTHZ → STATS
  priority: 100            # 同阶段内的执行顺序（数字越大越先执行）
  matchRules:              # 作用范围
  - ingress:
    - demo                 # 只对名为 demo 的 Ingress 生效
    config:                # 插件参数
      allowList:
      - "api-key-001"
      - "api-key-002"
---
# 全局生效示例
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: global-cors
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/cors:latest
  defaultConfig:           # 无 matchRules 时全局生效
    allow_origins:
    - "https://app.example.com"
    allow_methods: "GET,POST,PUT,DELETE,OPTIONS"
    max_age: 3600
```

## 插件执行阶段

```
请求流入 → AUTHN(认证) → AUTHZ(鉴权) → STATS(统计) → Router(转发) → 响应返回
                │              │             │
                │              │             └─ prometheus, request-log
                │              └─ ip-restriction, key-rate-limit, waf
                └─ jwt-auth, key-auth, basic-auth, oidc
```

---

<!-- chunk: 8. Wasm 插件开发实战 -->## 8. Wasm 插件开发实战

> 详细参考：[10-Wasm 插件生态与开发实践](./10-wasm-plugin-ecosystem.md)

## Go（TinyGo）开发示例

```go
package main

import (
    "github.com/higress-group/proxy-wasm-go-sdk/proxywasm"
    "github.com/higress-group/proxy-wasm-go-sdk/proxywasm/types"
    "github.com/tidwall/gjson"
)

type pluginContext struct {
    proxywasm.DefaultPluginContext
    headerName  string
    headerValue string
}

type httpContext struct {
    proxywasm.DefaultHttpContext
    contextID uint32
    p         *pluginContext
}

func main() {
    proxywasm.SetVMContext(&pluginContext{})
}

// 解析插件配置
func (p *pluginContext) OnPluginStart(pluginConfigurationSize int) types.OnPluginStartStatus {
    data, err := proxywasm.GetPluginConfiguration()
    if err != nil {
        proxywasm.LogErrorf("failed to get config: %v", err)
        return types.OnPluginStartStatusFailed
    }
    p.headerName = gjson.GetBytes(data, "header_name").String()
    p.headerValue = gjson.GetBytes(data, "header_value").String()
    return types.OnPluginStartStatusOK
}

func (p *pluginContext) NewHttpContext(contextID uint32) types.HttpContext {
    return &httpContext{contextID: contextID, p: p}
}

// 处理请求头
func (ctx *httpContext) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
    proxywasm.AddHttpRequestHeader(ctx.p.headerName, ctx.p.headerValue)
    proxywasm.LogInfof("added header %s: %s", ctx.p.headerName, ctx.p.headerValue)
    return types.ActionContinue
}
```

## 编译与发布

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 TinyGo（Mac）
brew install tinygo

# 编译为 Wasm
tinygo build -o plugin.wasm -scheduler=none -target=wasi ./main.go

# 构建 OCI 镜像并推送
docker buildx build --platform wasi/wasm -t registry.example.com/my-plugin:v1 .
docker push registry.example.com/my-plugin:v1
```
---

<!-- chunk: 9. AI 网关能力 -->## 9. AI 网关能力

Higress 是目前开源社区中 AI 网关能力最完整的产品之一，核心功能包括：

## 功能矩阵

| 能力 | 说明 | 对应插件 |
|------|------|---------|
| **LLM 代理路由** | 统一入口代理 OpenAI/通义/Claude 等 | ai-proxy |
| **Token 级限流** | 按 Token 消耗量而非请求数限流 | ai-token-ratelimit |
| **多模型 Fallback** | 主模型不可用时自动切换备用模型 | ai-proxy (fallbackConfig) |
| **语义缓存** | 基于语义相似度缓存 LLM 响应，降低成本 | ai-cache |
| **Prompt 模板** | 预定义 Prompt 模板，统一管理 | ai-prompt-template |
| **AI 可观测性** | Token 用量、延迟、成本统计 | ai-statistics |
| **模型映射** | 将外部模型名映射为内部实际模型 | ai-proxy (modelMapping) |

## AI 代理路由配置

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-proxy
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-proxy:latest
  matchRules:
  - ingress:
    - ai-route
    config:
      provider:
        type: openai              # 支持: openai, qwen, azure, claude, moonshot 等
        apiTokens:
        - "${OPENAI_API_KEY}"     # 从 K8s Secret 注入
        modelMapping:
          "gpt-4": "gpt-4-turbo"  # 模型名映射
          "*": "gpt-3.5-turbo"    # 默认回退
```

## Token 级限流

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-token-ratelimit
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-token-ratelimit:latest
  matchRules:
  - ingress:
    - ai-route
    config:
      rule_name: "default"
      rule_items:
      - limit_by_per_ip: true
        limit_keys:
        - key: "tokens_per_minute"
          token_per_minute: 10000
        - key: "tokens_per_day"
          token_per_day: 100000
```

## 多模型 Fallback

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-proxy-fallback
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-proxy:latest
  matchRules:
  - ingress:
    - ai-route
    config:
      provider:
        type: openai
        apiTokens:
        - "${OPENAI_API_KEY}"
      fallbackConfig:
        enabled: true
        fallbackProvider:
          type: dashscope         # 阿里云通义千问
          apiTokens:
          - "${DASHSCOPE_API_KEY}"
          modelMapping:
            "*": "qwen-max"
```

## Mac 上体验 AI 网关 Demo

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 Docker All-in-One
docker run -d --name higress-ai \
  -p 8001:8001 -p 8080:8080 \
  higress-registry.cn-hangzhou.cr.aliyuncs.com/higress/all-in-one:latest

# 1. 打开 Console: http://localhost:8001
# 2. 配置 AI 路由：
#    - 添加服务来源：固定地址 api.openai.com:443（HTTPS）
#    - 添加路由：/v1/* → api.openai.com
#    - 启用 ai-proxy 插件，填入 API Key
# 3. 测试：
curl http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-3.5-turbo",
    "messages": [{"role": "user", "content": "Hello!"}]
  }'
```
---

<!-- chunk: 10. Gateway API 集成 -->## 10. Gateway API 集成

Higress 支持 Gateway API v1.1 Extended 一致性级别，兼容 HTTPRoute、GRPCRoute、TLSRoute。

```yaml
# 创建 GatewayClass
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: higress
spec:
  controllerName: higress.io/gateway-controller

---
# 创建 Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: higress-gw
  namespace: higress-system
spec:
  gatewayClassName: higress
  listeners:
  - name: http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: All
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      mode: Terminate
      certificateRefs:
      - name: tls-cert
    allowedRoutes:
      namespaces:
        from: All

---
# HTTPRoute 示例
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: advanced-route
spec:
  parentRefs:
  - name: higress-gw
    namespace: higress-system
  hostnames: ["app.example.com"]
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
      method: GET
    filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Gateway
          value: higress
    backendRefs:
    - name: api-service
      port: 8080
```

---

<!-- chunk: 11. 可观测性 -->## 11. 可观测性

## Prometheus 指标

```yaml
# Higress 默认暴露核心 Envoy 指标
# istio_requests_total                    - 请求总数 (Counter)
# istio_request_duration_milliseconds     - 请求延迟分布 (Histogram)
# istio_request_bytes_total               - 请求字节数 (Counter)
# istio_response_bytes_total              - 响应字节数 (Counter)
# envoy_cluster_upstream_cx_active        - 活跃上游连接数 (Gauge)
# envoy_cluster_upstream_rq_retry         - 重试次数 (Counter)

# 手动获取指标（调试）
kubectl port-forward svc/higress-gateway 15020:15020 -n higress-system
curl http://localhost:15020/stats/prometheus
```

## ServiceMonitor

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: higress-gateway
  namespace: higress-system
spec:
  selector:
    matchLabels:
      app: higress-gateway
  endpoints:
  - port: http-envoy-prom
    path: /stats/prometheus
    interval: 15s
```

## 访问日志配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: higress-config
  namespace: higress-system
data:
  higress: |
    accessLog:
      enable: true
      format: |
        {"ts":"%START_TIME%","method":"%REQ(:METHOD)%","path":"%REQ(X-ENVOY-ORIGINAL-PATH?:PATH)%","code":"%RESPONSE_CODE%","duration":"%DURATION%","upstream":"%UPSTREAM_HOST%","req_id":"%REQ(X-REQUEST-ID)%","ua":"%REQ(USER-AGENT)%"}
    tracing:
      enable: true
      sampling: 10.0
      timeout: 500
```

> 详细参考：[12-API 网关可观测性](./12-api-gateway-observability.md)

---

<!-- chunk: 12. 生产环境调优 -->## 12. 生产环境调优

## 资源配置建议

| 规模 | Gateway CPU | Gateway Memory | 副本数 |
|------|------------|---------------|--------|
| 小型（< 1K QPS） | 1 core | 1Gi | 2 |
| 中型（1K-10K QPS） | 2-4 core | 2-4Gi | 2-4 |
| 大型（> 10K QPS） | 4-8 core | 4-8Gi | 4-10 |

## HPA 配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: higress-gateway
  namespace: higress-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: higress-gateway
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
```

## Envoy 性能调优

```yaml
# 关键调优参数（通过 Higress 配置或 EnvoyFilter）
concurrency: 4                    # Worker 线程数，建议等于 CPU 核数
connection_idle_timeout: 3600s    # 连接空闲超时
per_connection_buffer_limit: 32KB # 每连接缓冲区
stream_idle_timeout: 300s         # HTTP/2 流空闲超时

# 上游连接池（DestinationRule 或 Higress 注解）
# max_connections: 1024            # 最大连接数
# max_requests_per_connection: 100 # 每连接最大请求数
# connect_timeout: 5s              # 连接超时
```

## PodDisruptionBudget

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: higress-gateway-pdb
  namespace: higress-system
spec:
  minAvailable: 1
  selector:
    matchLabels:
      app: higress-gateway
```

---

<!-- chunk: 13. 与 Istio 协同 -->## 13. 与 Istio 协同

Higress 基于 Istiod 构建控制平面，可与 Istio 服务网格无缝协同：

```
┌────────────────────────────────────────────────────┐
│               Higress + Istio 协同架构               │
│                                                    │
│  外部流量 → Higress Gateway (南北向)                  │
│                    │                               │
│                    ▼                               │
│              K8s Service                           │
│                    │                               │
│                    ▼                               │
│         Istio Sidecar (东西向)                      │
│                    │                               │
│                    ▼                               │
│              Backend Pod                           │
│                                                    │
│  共享 Istiod 控制平面:                               │
│  - 统一服务发现（共享 ServiceEntry）                  │
│  - 统一证书管理（同一根 CA）                          │
│  - 统一配置下发 (xDS)                               │
│  - 统一可观测性（Trace ID 透传）                     │
└────────────────────────────────────────────────────┘
```

**复用已有 Istiod：** 如果集群中已部署 Istio，Higress 可以复用现有 Istiod 实例，通过 Helm values 配置：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm install higress higress/higress \
  -n higress-system --create-namespace \
  --set global.local=false \
  --set global.enableIstioAPI=true \
  --set higress-core.pilot.replicaCount=0  # 不部署内置 Istiod，复用已有的
```
---

<!-- chunk: 14. 常见故障排查 -->## 14. 常见故障排查

## 排查工具箱

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看 Gateway 配置（xDS dump）
kubectl exec -n higress-system deploy/higress-gateway -- \
  curl -s localhost:15000/config_dump | python3 -m json.tool | less

# 2. 查看路由信息
kubectl exec -n higress-system deploy/higress-gateway -- \
  curl -s localhost:15000/config_dump?resource=dynamic_route_configs

# 3. 查看上游集群
kubectl exec -n higress-system deploy/higress-gateway -- \
  curl -s localhost:15000/clusters

# 4. 查看访问日志
kubectl logs -n higress-system -l app=higress-gateway -f

# 5. 查看控制面日志
kubectl logs -n higress-system -l app=higress-controller -f
```
## 常见问题

| 症状 | 可能原因 | 排查方法 |
|------|---------|---------|
| 路由 404 | Ingress 未被识别 | 检查 `ingressClassName: higress`；检查 Controller 日志 |
| 503 Service Unavailable | 上游服务不可达 | `curl localhost:15000/clusters` 查看 Endpoint 是否为空 |
| 路由不生效 | xDS 推送失败 | 检查 Istiod 日志；确认 CRD 语法正确 |
| 插件不生效 | WasmPlugin 配置错误 | 检查 `matchRules` 的 ingress 名称是否匹配；查看 Envoy 日志 |
| TLS 证书错误 | Secret 未同步 | 确认 Secret 与 Gateway 在同一命名空间 |
| 性能下降 | Worker 线程不足 | 检查 `concurrency` 配置是否匹配 CPU 核数 |

---

<!-- chunk: 15. 与竞品横向对比 -->## 15. 与竞品横向对比

| 维度 | **Higress** | APISIX | Kong | Envoy Gateway | Traefik |
|------|------------|--------|------|---------------|---------|
| **数据平面** | Envoy (C++) | OpenResty (Nginx+Lua) | Nginx+Lua | Envoy (C++) | Go 原生 |
| **控制平面** | Istiod 定制版 | Admin API + etcd | Admin API / KIC | EG Controller | 内置 |
| **配置存储** | K8s CRD | etcd | PostgreSQL/DB-less | K8s CRD | Provider |
| **配置热更新** | xDS（零抖动） | etcd watch | DB 轮询 | xDS | Provider |
| **Wasm 插件** | 一等支持 | 支持 | 支持 | 支持 | 不支持 |
| **AI 网关** | 原生内置 | 插件 | AI Gateway | 无 | 无 |
| **注册中心** | Nacos/Consul/Eureka | etcd/Consul/Nacos | 无 | 无 | Consul/etcd |
| **GUI 控制台** | Higress Console | Dashboard | Kong Manager(EE) | 无 | Dashboard |
| **Istio 集成** | 原生融合 | 无 | 无 | 无 | 无 |
| **CNCF 状态** | Sandbox | ASF 顶级项目 | 无 | 孵化级 | 无 |
| **国内适用** | 极好（阿里生态） | 很好（API7） | 一般 | 一般 | 一般 |

> 详细选型参考：[03-API 网关选型指南](./03-api-gateway-selection-guide.md)

---

<!-- chunk: 参考资料 -->## 参考资料

- [Higress 官方文档](https://higress.io/docs/overview/what-is-higress)
- [Higress GitHub](https://github.com/alibaba/higress)
- [Higress 插件市场](https://higress.io/plugin)
- [Domain-34: CNCF Landscape](../生态参考)
- [10-Wasm 插件生态](./10-wasm-plugin-ecosystem.md)
- [12-API 网关可观测性](./12-api-gateway-observability.md)
- [03-API 网关选型指南](./03-api-gateway-selection-guide.md)
- [09-传统 Ingress 迁移指南](./09-nginx-ingress-migration-guide.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[05-网络/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践

## See Also

- 02-kubernetes-gateway-api-deep-dive
- 03-api-gateway-selection-guide
- 05-apisix-enterprise-gateway
- 06-kong-enterprise-gateway

## Related

- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]

```

<!-- risk-assessed -->
