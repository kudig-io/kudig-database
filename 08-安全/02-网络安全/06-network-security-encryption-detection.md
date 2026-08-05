---
title: Kubernetes Network Security — Encryption, Segmentation, and Threat Detection
description: K8s 网络安全 — 传输加密（mTLS/WireGuard）、微分段策略、威胁检测（Falco/Tetragon）、DNS 安全、出口控制
summary: Kubernetes 网络安全的纵深防御实践，涵盖加密、分段、检测与响应
category: practice
tags:
- network-security
- encryption
- segmentation
- threat-detection
- defense-in-depth
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: security
---
# Kubernetes 网络安全纵深防御

> 构建加密、分段、检测、响应的多层网络安全体系。

## 纵深防御模型

```
┌─────────────────────────────────────────────────────────┐
│  L1: 边界安全（Ingress/Gateway + WAF + DDoS）           │
├─────────────────────────────────────────────────────────┤
│  L2: 传输加密（mTLS / WireGuard / TLS）                 │
├─────────────────────────────────────────────────────────┤
│  L3: 微分段（NetworkPolicy + Cilium L7）                │
├─────────────────────────────────────────────────────────┤
│  L4: 运行时检测（Falco / Tetragon / eBPF）              │
├─────────────────────────────────────────────────────────┤
│  L5: 出口控制（Egress Gateway + DNS 过滤）              │
└─────────────────────────────────────────────────────────┘
```

## 传输加密

### Cilium WireGuard（节点间加密）

```yaml
# Cilium Helm values — 启用 WireGuard
cilium:
  encryption:
    enabled: true
    type: wireguard
    wireguard:
      persistentKeepalive: "25s"
  # 或 IPsec
  # encryption:
  #   enabled: true
  #   type: ipsec
  #   ipsec:
  #     keyFile: keys/ipsec-keys
```

### Istio mTLS（Pod 间加密）

```yaml
# 全网格 STRICT mTLS
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
---
# 特定服务例外（如 legacy 不支持 mTLS）
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: legacy-permissive
  namespace: legacy
spec:
  selector:
    matchLabels:
      app: legacy-app
  mtls:
    mode: PERMISSIVE
```

### 加密方案对比

| 方案 | 层次 | 性能开销 | 复杂度 | 适用 |
|------|------|----------|--------|------|
| WireGuard (Cilium) | L3 节点间 | < 2% | 低 | 全集群加密 |
| IPsec (Cilium) | L3 节点间 | 3-5% | 中 | 合规要求 |
| Istio mTLS | L4/L7 Pod 间 | 5-10% | 中 | 零信任 |
| Linkerd mTLS | L4 Pod 间 | 3-5% | 低 | 轻量加密 |
| TLS 证书 (cert-manager) | 应用层 | 最小 | 低 | 特定服务 |

## 微分段策略

### 默认拒绝 + 白名单

```yaml
# 所有命名空间默认拒绝
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]
---
# 允许 DNS
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Egress"]
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
---
# 服务间白名单
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-to-db
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: postgres
  policyTypes: ["Ingress"]
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: api-server
        - podSelector:
            matchLabels:
              app: worker
      ports:
        - protocol: TCP
          port: 5432
```

### Cilium L7 策略（HTTP 级）

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-l7-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api-server
  ingress:
    - fromEndpoints:
        - matchLabels:
            app: frontend
      toPorts:
        - ports:
            - port: "8080"
              protocol: TCP
          rules:
            http:
              - method: GET
                path: "/api/v1/.*"
              - method: POST
                path: "/api/v1/orders"
              - method: PUT
                path: "/api/v1/orders/[0-9]+"
  egress:
    - toEndpoints:
        - matchLabels:
            app: postgres
      toPorts:
        - ports:
            - port: "5432"
              protocol: TCP
    - toFQDNs:
        - matchName: "api.stripe.com"
        - matchPattern: "*.s3.amazonaws.com"
      toPorts:
        - ports:
            - port: "443"
              protocol: TCP
```

## 运行时威胁检测

### Falco 规则

```yaml
# falco-rules.yaml — 自定义检测规则
- rule: Shell Spawned in Container
  desc: 检测容器内启动 shell（可能是入侵）
  condition: >
    spawned_process and container and
    proc.name in (bash, sh, zsh, ash) and
    not proc.pname in (kubectl, docker, containerd)
  output: >
    Shell spawned in container
    (user=%user.name container=%container.name shell=%proc.name
    parent=%proc.pname cmdline=%proc.cmdline)
  priority: WARNING
  tags: [container, shell, mitre_execution]

- rule: Sensitive File Read in Container
  desc: 检测容器内读取敏感文件
  condition: >
    open_read and container and
    fd.name in (/etc/shadow, /etc/passwd, /root/.ssh/*, /var/run/secrets/kubernetes.io/*)
  output: >
    Sensitive file read (file=%fd.name container=%container.name)
  priority: CRITICAL
  tags: [container, filesystem, mitre_credential_access]

- rule: Outbound Connection to Crypto Miner
  desc: 检测到矿池连接
  condition: >
    outbound and container and
    (fd.sip in (mining_pool_ips) or fd.sport in (3333, 4444, 5555, 7777, 8888, 9999))
  output: >
    Possible crypto mining connection (dest=%fd.sip:%fd.sport container=%container.name)
  priority: CRITICAL
  tags: [container, network, mitre_impact]
```

### Tetragon（eBPF 原生）

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: security-monitoring
  namespace: kube-system
spec:
  kprobes:
    # 监控文件写入
    - call: "fd_install"
      syscall: false
      args:
        - index: 0
          type: "int"
        - index: 1
          type: "file"
      selectors:
        - matchNamespaces: ["production"]
          matchArgs:
            - index: 1
              operator: "In"
              values:
                - "/etc/passwd"
                - "/etc/shadow"
                - "/var/run/secrets/kubernetes.io/serviceaccount/token"
    # 监控网络连接
    - call: "tcp_connect"
      syscall: false
      args:
        - index: 0
          type: "sock"
      selectors:
        - matchNamespaces: ["production"]
```

## 出口控制

### Egress Gateway（Istio）

```yaml
apiVersion: networking.istio.io/v1alpha3
kind: Gateway
metadata:
  name: egress-gateway
  namespace: istio-system
spec:
  selector:
    istio: egressgateway
  servers:
    - port:
        number: 443
        name: https
        protocol: HTTPS
      hosts:
        - "*.example.com"
        - "api.stripe.com"
      tls:
        mode: PASSTHROUGH
---
# 强制所有出口流量经过 Egress Gateway
apiVersion: networking.istio.io/v1alpha3
kind: Sidecar
metadata:
  name: restrict-egress
  namespace: production
spec:
  outboundTrafficPolicy:
    mode: REGISTRY_ONLY
  egress:
    - hosts:
        - "./*"
        - "istio-system/egress-gateway"
```

### DNS 过滤（Cilium）

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: dns-filter
  namespace: production
spec:
  endpointSelector:
    matchLabels: {}
  egress:
    - toEndpoints:
        - matchLabels:
            k8s:io.kubernetes.pod.namespace: kube-system
            k8s-app: kube-dns
      toPorts:
        - ports:
            - port: "53"
              protocol: UDP
          rules:
            dns:
              - matchPattern: "*.internal.example.com"
              - matchPattern: "*.amazonaws.com"
              - matchName: "api.stripe.com"
              # 拒绝其他 DNS 查询（隐式）
```

## 安全事件响应

| 事件类型 | 检测 | 响应 | 恢复 |
|----------|------|------|------|
| 异常 Shell | Falco 告警 | 隔离 Pod + 取证 | 重建 Pod |
| 数据外泄 | 出口流量异常 | 阻断 NetworkPolicy | 审计影响范围 |
| 挖矿程序 | CPU 异常 + 矿池连接 | 删除 Pod + 扫描镜像 | 修复漏洞 |
| 凭证泄露 | 审计日志异常 | 轮换 SA Token | 审查权限 |
| 横向移动 | 异常 Pod 间通信 | 收紧 NetworkPolicy | 全面扫描 |

```bash
# 紧急隔离 Pod
kubectl label pod compromised-pod -n production quarantined=true --overwrite
# 配合 NetworkPolicy 隔离
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: quarantine
  namespace: production
spec:
  podSelector:
    matchLabels:
      quarantined: "true"
  policyTypes: ["Ingress", "Egress"]
EOF
```

## Related

- [[08-安全/02-网络安全/index.md|网络安全]]
- [[08-安全/07-零信任架构/03-zero-trust-network-mtls-microsegmentation.md|零信任网络]]
- [[05-网络/05-eBPF/index.md|eBPF]]
