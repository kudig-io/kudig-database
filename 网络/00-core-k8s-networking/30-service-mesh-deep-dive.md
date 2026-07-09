---
title: 02 - Service Mesh 深度解析与生产实践
description: '# 02 - Service Mesh 深度解析与生产实践'
summary: 'iptables -t nat -L PREROUTING -n -v --line-numbers'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- istio
- envoy
- cilium
- helm
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Service Mesh 深度解析与生产实践 是什么
- 如何 Service Mesh 深度解析与生产实践
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- Service
- Mesh
- 深度解析与生产实践
- networking
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- service-mesh-basics
- cilium-basics
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
  path: ../故障诊断/topic-fta/list/service-fta.md
  label: '故障树: service'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 02 - [[Service|Service]]Service Mesh）|Service Mesh]] 深度解析与生产实践

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 专家级

---

<!-- chunk: 目录 -->
## 目录

1. [Service Mesh 核心架构](#1-service-mesh-核心架构)
2. [[entities/istio.md|Istio]] 深度实践 (Sidecar & Ambient)](#2-istio-深度实践-sidecar--ambient)
3. [[entities/linkerd.md|Linkerd]] 生产部署](#3-linkerd-生产部署)
4. [Cilium Service Mesh](#4-cilium-service-mesh)
5. [多集群服务网格](#5-多集群服务网格)
6. [性能优化与调优](#6-性能优化与调优)
7. [安全加固实践](#7-安全加固实践)
8. [监控与可观测性](#8-监控与可观测性)
9. [故障排查与运维](#9-故障排查与运维)

---

<!-- chunk: 1. Service Mesh 核心架构 -->
## 1. Service Mesh 核心架构

### 1.1 Sidecar vs Ambient 模式

#### Sidecar 模式 (传统)
- **原理**: 每个 Pod 注入一个代理容器 (Envoy)。
- **优势**: 细粒度控制、协议感知能力强。
- **缺点**: 资源开销大 (每个 Pod +~50MB)、升级需重启应用。

#### Ambient 模式 (未来趋势)
- **原理**: 分层架构，ztunnel (L4 转发) + Waypoint Proxy (L7 治理)。
- **优势**: 零应用干预、显著降低 CPU/内存开销 (可达 70%+)、独立升级。
- **适用**: 大规模集群、关注资源成本的场景。

### 1.2 流量拦截机制 (Sidecar 模式)

```yaml
# Sidecar 注入配置示例 (Istio 1.24+)
apiVersion: v1
kind: ConfigMap
metadata:
  name: sidecar-injector-config
  namespace: istio-system
data:
  config: |
    policy: enabled
    templates:
      initContainers:
      - name: istio-init
        image: docker.io/istio/proxyv2:1.24.0
        args:
        - istio-iptables
        - -p
        - "15001"
        - -z
        - "15006"
        - -u
        - "1337"
        - -m
        - REDIRECT
        - -i
        - "*"
        - -x
        - ""
        - -b
        - "*"
        - -d
        - "15090,15021,15020"
      containers:
      - name: istio-proxy
        image: docker.io/istio/proxyv2:1.24.0
```,old_str:
        ports:
        - containerPort: 15090
          protocol: TCP
          name: http-envoy-prom
        args:
        - proxy
        - sidecar
        env:
        - name: JWT_POLICY
          value: third-party-jwt
        - name: PILOT_CERT_PROVIDER
          value: istiod
        readinessProbe:
          httpGet:
            path: /healthz/ready
            port: 15021
          initialDelaySeconds: 1
          periodSeconds: 2
          timeoutSeconds: 3
```

### 1.2 流量拦截机制

```bash
#!/bin/bash
# iptables 规则分析脚本

echo "=== Service Mesh iptables 规则分析 ==="

# 查看 Envoy 相关规则
iptables-save | grep -A 10 -B 10 ISTIO

# 分析流量劫持规则
echo -e "\n=== 流量劫持规则 ==="
iptables -t nat -L PREROUTING -n -v --line-numbers

# 查看服务端口重定向
echo -e "\n=== 服务端口重定向 ==="
iptables -t nat -L ISTIO_INBOUND -n -v

# 查看出站流量规则
echo -e "\n=== 出站流量规则 ==="
iptables -t nat -L ISTIO_OUTPUT -n -v
```

### 1.3 数据平面组件

```yaml
# Envoy 配置模板
apiVersion: v1
kind: ConfigMap
metadata:
  name: envoy-config-template
  namespace: istio-system
data:
  bootstrap.yaml: |
    admin:
      access_log_path: /dev/null
      address:
        socket_address:
          address: 127.0.0.1
          port_value: 15000
    
    static_resources:
      listeners:
      - name: virtualInbound
        address:
          socket_address:
            address: 0.0.0.0
            port_value: 15006
        filter_chains:
        - filters:
          - name: envoy.filters.network.http_connection_manager
            typed_config:
              "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
              stat_prefix: inbound_http
              route_config:
                name: local_route
                virtual_hosts:
                - name: local_service
                  domains: ["*"]
                  routes:
                  - matchers:
                    - prefix="/"
                    - route=""
                    - cluster="inbound|9080|http|productpage.default.svc.cluster.local"

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```

---

<!-- chunk: 2. Istio 深度实践 (Sidecar & Ambient) -->
## 2. Istio 深度实践 (Sidecar & Ambient)

### 2.1 部署方式对比

| 部署方式 | 推荐工具 | 适用版本 | 说明 |
|----------|----------|----------|------|
| **Helm** | `helm upgrade --install` | v1.20+ | **生产首选**，符合 GitOps 流程 |
| **istioctl** | `istioctl install` | 调试/测试 | 简单快速，但不易版本控制 |
| **Operator** | `IstioOperator` | 传统 | 正在淡出，官方推荐转向 Helm |

### 2.2 Ambient Mesh 快速启用 (v1.24+)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Ambient Profile
istioctl install --set profile=ambient --skip-confirmation

# 标记命名空间使用 Ambient 模式
kubectl label namespace default istio.io/dataplane-mode=ambient

# 验证 ztunnel 状态
kubectl get pods -n istio-system -l app=ztunnel
```
### 2.3 生产级 Sidecar 部署配置 (Helm values)

```yaml
# istio-proxy 资源限制与优化
global:
  proxy:
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 2000m
        memory: 1Gi
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 15"] # 等待连接排空

pilot:
  autoscaleMin: 3
  replicaCount: 3
  resources:
    requests:
      cpu: 1000m
      memory: 2Gi
```,old_str:

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 MOC
- [[网络/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 28-coredns-troubleshooting-optimization
- 29-egress-traffic-management
- 31-multi-cluster-federation
- 32-multi-cluster-networking


<!-- risk-assessed -->
