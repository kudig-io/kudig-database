# 02 - Service Mesh 深度解析与生产实践

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 专家级

---

## 目录

1. [Service Mesh 核心架构](#1-service-mesh-核心架构)
2. [Istio 深度实践](#2-istio-深度实践)
3. [Linkerd 生产部署](#3-linkerd-生产部署)
4. [Cilium Service Mesh](#4-cilium-service-mesh)
5. [多集群服务网格](#5-多集群服务网格)
6. [性能优化与调优](#6-性能优化与调优)
7. [安全加固实践](#7-安全加固实践)
8. [监控与可观测性](#8-监控与可观测性)
9. [故障排查与运维](#9-故障排查与运维)

---

## 1. Service Mesh 核心架构

### 1.1 Sidecar 模式详解

```yaml
# Sidecar 注入配置示例
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
        image: docker.io/istio/proxyv2:1.20.0
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
        image: docker.io/istio/proxyv2:1.20.0
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

## 2. Istio 深度实践

### 2.1 生产级部署架构

```yaml
# Istio 生产部署配置
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: production-istio
  namespace: istio-system
spec:
  profile: demo
  components:
    pilot:
      k8s:
        resources:
          requests:
            cpu: 1000m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 2Gi
        replicaCount: 3
        nodeSelector:
          kubernetes.io/arch: amd64
        tolerations:
        - key: dedicated
          operator: Equal
          value: istio-control
          effect: NoSchedule
    
    ingressGateways:
    - name: istio-ingressgateway
      enabled: true
      k8s:
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 1000m
            memory: 1Gi
        replicaCount: 3
        service:
          ports:
          - name: status-port
            port: 15021
            targetPort: 15021
          - name: http2
            port: 80
            targetPort: 8080
          - name: https
            port: 443
            targetPort: 8443

  values:
    global:
      proxy:
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 2000m
            memory: 1Gi
      
    pilot:
      autoscaleEnabled: true
      autoscaleMin: 2
      autoscaleMax: 5
      
    gateways:
      istio-ingressgateway:
        autoscaleEnabled: true
        autoscaleMin: 2
        autoscaleMax: 5

    telemetry:
      enabled: true
      v2:
        enabled: true
        metadataExchange:
          wasmEnabled: false
        prometheus:
          enabled: true
          wasmEnabled: false
        stackdriver:
          enabled: false

    sidecarInjectorWebhook:
      enableNamespacesByDefault: false
      
    meshConfig:
      accessLogFile: /dev/stdout
      accessLogEncoding: JSON
      enableTracing: true
      outboundTrafficPolicy:
        mode: REGISTRY_ONLY
      defaultConfig:
        tracing:
          zipkin:
            address: zipkin.istio-system:9411
        proxyMetadata:
          ISTIO_META_DNS_CAPTURE: "true"
          ISTIO_META_DNS_AUTO_ALLOCATE: "true"