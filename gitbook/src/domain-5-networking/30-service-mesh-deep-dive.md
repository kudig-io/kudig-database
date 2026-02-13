# 02 - Service Mesh 深度解析与生产实践

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 专家级

---

## 目录

1. [Service Mesh 核心架构](#1-service-mesh-核心架构)
2. [Istio 深度实践 (Sidecar & Ambient)](#2-istio-深度实践-sidecar--ambient)
3. [Linkerd 生产部署](#3-linkerd-生产部署)
4. [Cilium Service Mesh](#4-cilium-service-mesh)
5. [多集群服务网格](#5-多集群服务网格)
6. [性能优化与调优](#6-性能优化与调优)
7. [安全加固实践](#7-安全加固实践)
8. [监控与可观测性](#8-监控与可观测性)
9. [故障排查与运维](#9-故障排查与运维)

---

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
                  - match:
                      prefix: "/"
                    route:
                      cluster: inbound|9080|http|productpage.default.svc.cluster.local
```

---

## 2. Istio 深度实践 (Sidecar & Ambient)

### 2.1 部署方式对比

| 部署方式 | 推荐工具 | 适用版本 | 说明 |
|----------|----------|----------|------|
| **Helm** | `helm upgrade --install` | v1.20+ | **生产首选**，符合 GitOps 流程 |
| **istioctl** | `istioctl install` | 调试/测试 | 简单快速，但不易版本控制 |
| **Operator** | `IstioOperator` | 传统 | 正在淡出，官方推荐转向 Helm |

### 2.2 Ambient Mesh 快速启用 (v1.24+)

```bash
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