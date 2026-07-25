---
title: Cilium (entities)
description: Cilium — Kubernetes 生产运维知识库
summary: Cilium — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- ebpf
- cni
- networking
- security
- cilium
- envoy
- kafka
- networkpolicy
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium 是什么
- 如何 Cilium
trigger_keywords:
- Cilium
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cilium

Cilium is an eBPF-based networking, security, and observability platform for Kubernetes, graduated from CNCF in 2023.

## Key Facts

- **Status**: CNCF graduated (2023)
- **Technology**: eBPF programs in Linux kernel
- **Kernel Requirement**: Linux 5.10+ (5.15+ for BTF, 6.1+ for advanced features)
- **Components**: Cilium Agent, Cilium Operator, CNI Plugin, Hubble Relay

## Capabilities

| Capability | Description |
|------------|-------------|
| CNI | Pod networking, IPAM, Kubernetes [[Service|Service]] routing |
| NetworkPolicy | L3/L4 policies + L7 HTTP/gRPC/Kafka policies |
| Service Mesh | Sidecar-less mesh via eBPF + optional Envoy for L7 |
| Load Balancing | eBPF-based kube-proxy replacement (Maglev, ECMP) |
| Encryption | WireGuard or IPSec for Pod-to-Pod encryption |
| Observability | Hubble for L3/L4/L7 flow visualization |

## Hubble Integration

Hubble provides network flow observability:
- **Hubble Relay**: Aggregates flow data from all Cilium agents
- **Hubble CLI**: Command-line flow analysis
- **Hubble UI**: Web-based service dependency map

## kube-proxy Replacement

Cilium can replace kube-proxy entirely using eBPF for Service load balancing. Benefits: higher throughput, lower latency, no iptables/IPVS rules to manage.

## 安装与配置

```bash
# 🟢 使用 cilium CLI 安装
cilium install --version 1.15.0 \
  --set kubeProxyReplacement=true \
  --set k8sServiceHost=10.0.1.1 \
  --set k8sServicePort=6443

# 🟢 验证安装
cilium status
cilium connectivity test

# 🟢 查看 Cilium 配置
kubectl get configmap cilium-config -n kube-system -o yaml
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Cilium 状态
cilium status
kubectl get pods -n kube-system -l k8s-app=cilium

# 🟢 查看网络策略
cilium policy get
kubectl get ciliumnetworkpolicy -A

# 🟢 查看端点 (Pod 网络)
cilium endpoint list

# 🟢 查看 Service 负载均衡
cilium service list

# 🟢 查看连接跟踪
cilium bpf ct list global

# 🟢 监控网络流量
cilium monitor --type trace
cilium monitor --type drop

# 🟢 Hubble 流量观察
hubble observe --namespace default
hubble observe --type drop
hubble observe --from-namespace team-a --to-namespace team-b

# 🟢 连通性测试
cilium connectivity test
cilium connectivity test --namespace default

# 🟢 查看 eBPF 程序
cilium bpf policy get <endpoint-id>
```

### CiliumNetworkPolicy 示例

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-policy
  namespace: default
spec:
  endpointSelector:
    matchLabels:
      app: api
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
          path: /api/.*
        - method: POST
          path: /api/orders
  egress:
  - toEndpoints:
    - matchLabels:
        app: database
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
  - toCIDR:
    - 0.0.0.0/0
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Pod 网络不通 | eBPF 程序未加载 | 检查内核版本、Cilium 日志 |
| DNS 解析失败 | CoreDNS 策略阻断 | 检查 egress DNS 规则 |
| Service 不可达 | eBPF LB 异常 | `cilium service list` |
| 策略不生效 | 标签不匹配 | `cilium policy trace` |
| 性能下降 | eBPF map 满 | 检查 map 大小 |
| Hubble 无数据 | Relay 异常 | 检查 Hubble 组件 |

### 排查流程

```
1. Cilium 状态检查
   cilium status
       │
2. 连通性测试
   cilium connectivity test
       │
3. 流量监控
   cilium monitor --type drop
       │
4. 策略跟踪
   cilium policy trace --src-namespace default --src-pod <pod> --dst-namespace default --dst-pod <pod>
       │
5. Hubble 观察
   hubble observe --pod <pod-name>
```

## Cilium vs 其他 CNI

| 特性 | Cilium | Calico | Flannel |
|------|--------|--------|--------|
| 数据平面 | eBPF | iptables/eBPF | VXLAN |
| L7 策略 | 支持 | 不支持 | 不支持 |
| kube-proxy 替代 | 完整 | 部分 | 不支持 |
| 可观测性 | Hubble | 基本 | 无 |
| 加密 | WireGuard/IPSec | IPSec | 无 |
| 性能 | 极高 | 高 | 中 |
| 复杂度 | 中-高 | 中 | 低 |

## 生产案例

### 案例1：eBPF 程序未加载

**症状：** Pod 创建后无网络

**根因：** 内核版本 < 5.10，不支持 BTF

**解决：** 升级内核到 5.15+

### 案例2：DNS 被策略阻断

**症状：** 部署 CiliumNetworkPolicy 后 DNS 失败

**根因：** egress 策略未允许 UDP:53 到 CoreDNS

**解决：** 添加 DNS egress 规则

## 检查清单

- [ ] 理解 Cilium eBPF 架构
- [ ] 掌握 CiliumNetworkPolicy 编写
- [ ] 能使用 Hubble 观察流量
- [ ] 掌握故障排查流程
- [ ] 理解 kube-proxy 替代模式
- [ ] 了解 Cilium vs Calico vs Flannel

## Related
- [[22-概念/11-交叉分析/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[22-概念/03-网络/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[22-概念/03-网络/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[22-概念/03-网络/service-mesh-architecture.md|Service Mesh Architecture]]
- [[23-实体/06-安全/tetragon.md|Tetragon]]
- Hubble

- [[22-概念/11-交叉分析/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- 18-kubernetes-ebpf-cilium-deep-practice
- 03-cilium-cni-architecture
- 99-cilium-ebpf-network-guide
- 05-cilium-service-mesh
- 04-cilium-network-policy
- [[19-故障诊断/06-FTA故障树/list/cilium-fta.md|cilium-fta]]
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-networking-ecosystem.md|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]] — Cross-reference
- [[22-概念/11-交叉分析/eBPF × 运行时安全.md|eBPF x 运行时安全]] — Cross-reference
- [[22-概念/12-研究/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[22-概念/03-网络/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[22-概念/01-核心架构/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Cross-reference
- [[22-概念/03-网络/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]] — Cross-reference
- [[26-技能/05-网络/networkpolicy/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[26-技能/05-网络/networkpolicy/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[21-生态参考/03-领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
