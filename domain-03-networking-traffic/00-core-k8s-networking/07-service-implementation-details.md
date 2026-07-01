---
title: 77 - Service实现机制
description: '## IPVS调度算法'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- scheduler
- cilium
- mysql
- statefulset
- ebpf
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Service实现机制 是什么
- 如何 Service实现机制
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- Service实现机制
- networking
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
- mysql-basics
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md
  label: '故障树: service'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
created: "2026-05-23"
---

# 77 - Service实现机制

<!-- chunk: Service类型对比 -->
## Service类型对比

| 类型 | 访问范围 | 负载均衡 | 外部访问 | 使用场景 |
|-----|---------|---------|---------|---------|
| ClusterIP | 集群内 | kube-proxy | ❌ | 内部服务 |
| NodePort | 节点端口 | kube-proxy | ✅ | 开发测试 |
| LoadBalancer | 云LB | 云厂商 | ✅ | 生产入口 |
| ExternalName | DNS别名 | ❌ | ✅ | 外部服务 |
| Headless | 无ClusterIP | ❌ | ❌ | [[StatefulSet|StatefulSet]] |

<!-- chunk: kube-proxy模式对比 -->
## kube-proxy模式对比

| 模式 | 实现 | 性能 | 功能 | 推荐场景 |
|-----|------|------|------|---------|
| iptables | iptables规则 | 中 | 基础 | 小规模 |
| IPVS | Linux IPVS | 高 | 丰富 | 大规模 |
| nftables | nftables规则 | 高 | 基础 | v1.29+ |
| eBPF | Cilium等 | 最高 | 最丰富 | 高性能 |

<!-- chunk: IPVS调度算法 -->
## IPVS调度算法

| 算法 | 参数 | 说明 | 适用场景 |
|-----|------|------|---------|
| rr | roundrobin | 轮询 | 默认 |
| lc | leastconn | 最少连接 | 长连接 |
| dh | destinationhash | 目标哈希 | 缓存亲和 |
| sh | sourcehash | 源地址哈希 | 会话保持 |
| sed | shortestexpecteddelay | 最短延迟 | 异构后端 |
| nq | neverqueue | 永不排队 | 低延迟 |

<!-- chunk: kube-proxy IPVS配置 -->
## kube-proxy IPVS配置

```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: ipvs
ipvs:
  scheduler: rr
  syncPeriod: 30s
  minSyncPeriod: 2s
  tcpTimeout: 0s
  tcpFinTimeout: 0s
  udpTimeout: 0s
  strictARP: true  # MetalLB需要
conntrack:
  maxPerCore: 32768
  min: 131072
  tcpEstablishedTimeout: 86400s
  tcpCloseWaitTimeout: 3600s
```

<!-- chunk: Service会话保持 -->
## Service会话保持

```yaml
apiVersion: v1
kind: Service
metadata:
  name: sticky-service
spec:
  selector:
    app: myapp
  ports:
  - port: 80
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时
```

<!-- chunk: LoadBalancer注解(ACK) -->
## LoadBalancer注解(ACK)

| 注解 | 值 | 说明 |
|-----|---|------|
| `service.beta.[[Kubernetes|kubernetes]].io/alibaba-cloud-loadbalancer-address-type` | internet/intranet | 公网/内网 |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec` | slb.s1.small等 | SLB规格 |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id` | lb-xxx | 复用已有SLB |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag` | on/off | 健康检查 |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler` | wrr/rr | 调度算法 |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth` | 100 | 带宽限制(Mbps) |

<!-- chunk: LoadBalancer配置示例 -->
## LoadBalancer配置示例

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-service
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留客户端IP
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
```

<!-- chunk: Headless Service -->
## Headless Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
spec:
  clusterIP: None  # Headless
  selector:
    app: mysql
  ports:
  - port: 3306
# DNS返回所有Pod IP
# mysql-0.mysql-headless.default.svc.cluster.local
# mysql-1.mysql-headless.default.svc.cluster.local
```

<!-- chunk: ExternalName Service -->
## ExternalName Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-db
spec:
  type: ExternalName
  externalName: db.external.example.com
# 访问external-db会解析到外部域名
```

<!-- chunk: Service监控指标 -->
## Service监控指标

| 指标 | 类型 | 说明 |
|-----|-----|------|
| `kube_service_info` | Gauge | Service信息 |
| `kube_service_spec_type` | Gauge | Service类型 |
| `kube_endpoint_address_available` | Gauge | 可用端点数 |
| `kube_endpoint_address_not_ready` | Gauge | 未就绪端点数 |

<!-- chunk: Service故障排查 -->
## Service故障排查

```bash
# 检查Service
kubectl get svc <name> -o wide

# 检查Endpoints
kubectl get endpoints <name>

# 检查kube-proxy规则
# iptables模式
iptables -t nat -L KUBE-SERVICES -n
# IPVS模式
ipvsadm -Ln

# 检查连通性
kubectl run test --image=busybox --rm -it -- wget -qO- <service-ip>
```

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | Service IP范围扩展 |
| v1.27 | Service拓扑感知改进 |
| v1.29 | nftables模式Alpha |
| v1.30 | TrafficDistribution字段 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic KUDIG Database — Global MOC
- [[domain-03-networking-traffic/README.md|Domain 03: Networking 网络]]
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

- 05-terway-advanced-guide
- 06-service-concepts-types
- 08-service-topology-aware
- 09-kube-proxy-modes-performance

## Related

- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
