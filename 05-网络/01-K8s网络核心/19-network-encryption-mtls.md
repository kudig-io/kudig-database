---
title: 83 - 网络加密与mTLS
description: '# 83 - 网络加密与mTLS'
summary: '# 83 - 网络加密与mTLS'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- istio
- cilium
- calico
- helm
- opa
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
- 网络加密与mTLS 是什么
- 如何 网络加密与mTLS
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- 网络加密与mTLS
- networking
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
- policy-basics
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
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 83 - 网络加密与mTLS

<!-- chunk: 网络加密方案 -->
## 网络加密方案

| 方案 | 层级 | 性能影响 | 管理复杂度 | 适用场景 |
|-----|------|---------|-----------|---------|
| WireGuard | L3 | 低(3-5%) | 低 | CNI加密 |
| IPsec | L3 | 中(5-10%) | 中 | 传统方案 |
| mTLS([[istio\|Istio]]) | L7 | 中(5-15%) | 中 | 服务网格 |
| Cilium加密 | L3/L4 | 低 | 低 | eBPF加密 |

<!-- chunk: Calico WireGuard加密 -->
## Calico WireGuard加密

```yaml
apiVersion: projectcalico.org/v3
kind: FelixConfiguration
metadata:
  name: default
spec:
  wireguardEnabled: true
  wireguardListeningPort: 51820
  wireguardMTU: 1400
  wireguardHostEncryptionEnabled: true  # 主机流量也加密
---
# 验证加密状态
# calicoctl get node <node-name> -o yaml | grep wireguard
```

<!-- chunk: Cilium加密配置 -->
## Cilium加密配置

```yaml
# Helm values
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-encryption
data:
  values.yaml: |
    encryption:
      enabled: true
      type: wireguard
      # 或使用IPsec
      # type: ipsec
      # ipsec:
      #   keyFile: /etc/ipsec.d/keys/ipsec.keys
    
    # WireGuard节点加密
    l7Proxy: false
    
    # 透明加密(不需要服务网格)
    encryption:
      nodeEncryption: true
```

<!-- chunk: Istio mTLS配置 -->
## Istio mTLS配置

```yaml
# 命名空间级别严格mTLS
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata:
  name: default
  namespace: production
spec:
  mtls:
    mode: STRICT  # STRICT/PERMISSIVE/DISABLE
---
# 工作负载级别配置
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata:
  name: workload-mtls
  namespace: production
spec:
  selector:
    matchLabels:
      app: sensitive-service
  mtls:
    mode: STRICT
  portLevelMtls:
    8080:
      mode: STRICT
    9090:
      mode: PERMISSIVE  # 监控端口允许明文
```

<!-- chunk: Istio证书轮换 -->
## Istio证书轮换

```yaml
# 配置证书轮换
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  meshConfig:
    certificates:
    - secretName: cacerts
      dnsNames:
      - istio-ca-secret
    defaultConfig:
      proxyMetadata:
        # 工作负载证书有效期(默认24h)
        SECRET_TTL: "24h"
        # 证书轮换检查间隔
        SECRET_GRACE_PERIOD_RATIO: "0.5"
```

<!-- chunk: 自定义CA配置 -->
## 自定义CA配置

```yaml
# 使用自定义CA
apiVersion: v1
kind: Secret
metadata:
  name: cacerts
  namespace: istio-system
type: Opaque
data:
  ca-cert.pem: <base64-encoded-cert>
  ca-key.pem: <base64-encoded-key>
  cert-chain.pem: <base64-encoded-chain>
  root-cert.pem: <base64-encoded-root>
```

<!-- chunk: SPIFFE身份验证 -->
## SPIFFE身份验证

```yaml
# Istio AuthorizationPolicy使用SPIFFE ID
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata:
  name: httpbin-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: httpbin
  action: ALLOW
  rules:
  - from:
    - source:
        principals:
        - "cluster.local/ns/production/sa/sleep"
        - "cluster.local/ns/production/sa/curl"
    to:
    - operation:
        methods: ["GET"]
        paths: ["/status/*"]
```

<!-- chunk: 加密验证命令 -->
## 加密验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 验证Calico WireGuard
calicoctl get node -o yaml | grep -i wireguard
wg show

# 验证Cilium加密
cilium status | grep Encryption
cilium encrypt status

# 验证Istio mTLS
istioctl x authz check <pod-name>
kubectl exec <pod> -c istio-proxy -- \
  openssl s_client -connect <service>:443 -showcerts

# 检查证书
istioctl proxy-config secret <pod-name>
```
<!-- chunk: 加密监控指标 -->
## 加密监控指标

| 指标 | 类型 | 说明 |
|-----|-----|------|
| `istio_tcp_sent_bytes_total` | Counter | TLS发送字节 |
| `istio_tcp_received_bytes_total` | Counter | TLS接收字节 |
| `cilium_encrypt_packets_total` | Counter | 加密包数 |
| `cilium_decrypt_packets_total` | Counter | 解密包数 |

<!-- chunk: 性能影响对比 -->
## 性能影响对比

| 方案 | CPU增加 | 延迟增加 | 吞吐量下降 |
|-----|--------|---------|-----------|
| WireGuard | 2-5% | 0.1-0.5ms | <5% |
| IPsec | 5-15% | 0.5-2ms | 5-15% |
| Istio mTLS | 5-10% | 1-5ms | 5-15% |
| Cilium WG | 2-5% | 0.1-0.5ms | <5% |

<!-- chunk: 零信任网络 -->
## 零信任网络

```yaml
# 默认拒绝所有流量
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  {}  # 空规则=拒绝所有
---
# 显式允许特定服务
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata:
  name: allow-frontend
  namespace: production
spec:
  selector:
    matchLabels:
      app: backend
  action: ALLOW
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/frontend"]
```

<!-- chunk: ACK加密方案 -->
## ACK加密方案

| 功能 | 说明 |
|-----|------|
| Terway加密 | VPC流量加密 |
| ASM mTLS | 托管mTLS |
| 专有网络 | VPC隔离 |
| SSL证书 | 证书托管 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[05-网络/README.md|[[37-归档/domain-indexes/network/README-from-domain-5|Domain 5: Networking 网络]]working]] 网络]]
- [[05-网络/01-K8s网络核心/00-network-in-nutshell.md|00 network in nutshell]]
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

- 16-networkpolicy-deep-practice
- 17-network-policy-advanced
- 19-ingress-fundamentals
- 20-ingress-controller-deep-dive

## Related

- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[21-生态参考/03-领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
