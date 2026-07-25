---
title: LoxiLB [entities]
description: '## 概述'
summary: 'LoxiLB 是一个基于 eBPF 的云原生负载均衡器，专注于为 Kubernetes 提供高性能的 L4 负载均衡服务。'
category: entities
tags:
- k8s
- cncf
- networking
- loxilb
- cilium
- opa
- ingress
- crd
- operator
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LoxiLB 是什么
- 如何 LoxiLB
trigger_keywords:
- LoxiLB
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# LoxiLB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, C

## 概述

LoxiLB 是一个基于 eBPF 的云原生负载均衡器，由 LoxiLab 团队开发，2023 年加入 CNCF 沙箱。它专注于为 Kubernetes 提供高性能的 L4 负载均衡服务，可作为 Kubernetes 的 [[Service|Service]] LoadBalancer、[[Ingress|Ingress]] 控制器或独立负载均衡网关运行。LoxiLB 利用 eBPF/XDP 技术在内核数据面实现线速转发，支持 BGP、ECMP、DSR（Direct Server Return）等高级网络特性，在 10Gbps+ 吞吐场景下相比 kube-proxy/iptables 有数量级的性能提升。它还原生支持 IPv6、NAT46/64、防火墙和流量镜像等功能。

## 核心能力

- **eBPF/XDP 数据面**: 内核态线速转发，绕过传统 iptables 性能瓶颈
- **BGP/ECMP 路由**: 与上游路由器建立 BGP 邻居，实现 VIP 自动广播和多路径负载均衡
- **DSR 模式**: Direct Server Return，后端直接回包给客户端，减少 LB 节点带宽压力
- **多协议支持**: TCP、UDP、SCTP、HTTP、QUIC 负载均衡
- **健康检查**: 主动/被动健康检查，自动剔除不健康后端
- **NAT 和防火墙**: 内置 SNAT、DNAT、NAT46/64 和 ACL 规则能力

## 架构

LoxiLB 采用 eBPF 驱动的高性能架构：

- **LoxiLB Agent**: 用户态控制面，管理负载均衡规则、BGP 邻居和健康检查
- **eBPF 程序**: 挂载在 XDP/TC hook，在内核态处理数据包转发和负载均衡
- **eBPF Maps**: 内核态数据结构，存储 LB 规则、后端列表和连接状态
- **BGP 守护进程**: 与上游路由器交换路由信息，广播 VIP 可达性
- **kube-loxilb**: Kubernetes 集成组件，监听 Service 资源并创建 LoxiLB 规则

数据流：`客户端 → 路由器 (BGP) → LoxiLB 节点 (eBPF/XDP) → 后端 Pod → 客户端 (DSR)`

## K8s 集成

LoxiLB 通过 **kube-loxilb** 组件与 Kubernetes 集成。kube-loxilb 以 Deployment 方式运行在集群中，监听类型为 LoadBalancer 的 Service 资源，自动为其分配 External IP 并在 LoxiLB 中创建负载均衡规则。通过 `loxilb.io/rr-mode` 等 annotation 控制 BGP/DSR 行为。LoxiLB 可以作为集群内组件（in-cluster 模式）或集群外独立 LB 节点（external 模式）运行，适合裸金属集群和私有云场景。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中基于 iptables 的 kube-proxy 相比，eBPF 数据面消除了规则同步延迟。

## 生产场景

1. **裸金属集群 LB**: 无云厂商 LoadBalancer 时，为 Kubernetes Service 提供外部可达的 VIP
2. **高性能流量入口**: 5G UPF、电信级 VNF 等需要 10Gbps+ 吞吐的 L4 负载均衡场景
3. **多集群流量调度**: 通过 BGP Anycast 实现跨集群的流量调度和容灾
4. **DSR 高吞吐场景**: 视频流、大文件传输等回程流量大的场景，通过 DSR 降低 LB 节点压力

## 安装与配置

```bash
# 方式一：Helm 安装（集群内模式）
helm repo add loxilb https://loxilb-io.github.io/loxilb/
helm install loxilb loxilb/loxilb -n kube-system
helm install kube-loxilb loxilb/kube-loxilb -n kube-system

# 方式二：直接 YAML 部署
kubectl apply -f https://github.com/loxilb-io/loxilb/raw/main/manifest/loxilb.yaml
kubectl apply -f https://github.com/loxilb-io/kube-loxilb/raw/main/manifest/kube-loxilb.yaml

# 创建 LoadBalancer 类型 Service
kubectl expose deployment my-app --port=80 --type=LoadBalancer
```

### Service Annotation 配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app-lb
  annotations:
    loxilb.io/lbmode: "dsr"          # DSR 模式
    loxilb.io/bgp: "true"            # 启用 BGP 广播
    loxilb.io/rr-mode: "ecmp"        # ECMP 多路径
    loxilb.io/probetype: "http"      # HTTP 健康检查
    loxilb.io/probeport: "8080"      # 健康检查端口
    loxilb.io/probepath: "/health"   # 健康检查路径
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
  selector:
    app: my-app
```

## 运维操作

```bash
# 🟢 查看 LoxiLB 状态
kubectl get pods -n kube-system -l app=loxilb
kubectl get pods -n kube-system -l app=kube-loxilb

# 🟢 查看 LB 规则
kubectl exec -n kube-system loxilb-0 -- loxicmd get lb

# 🟢 查看 BGP 邻居状态
kubectl exec -n kube-system loxilb-0 -- loxicmd get bgp

# 🟢 查看健康检查状态
kubectl exec -n kube-system loxilb-0 -- loxicmd get ep

# 🟢 查看 NAT 规则
kubectl exec -n kube-system loxilb-0 -- loxicmd get nat

# 🟡 手动添加 LB 规则
kubectl exec -n kube-system loxilb-0 -- loxicmd create lb 10.0.0.100 --tcp=80:8080 --endpoints=10.244.1.10:1,10.244.2.10:1

# 🟢 查看 eBPF 程序状态
kubectl exec -n kube-system loxilb-0 -- bpftool prog list

# 🟢 查看 Service External IP
kubectl get svc -l loxilb.io/managed=true
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| External IP 未分配 | kube-loxilb 异常 | `kubectl logs -n kube-system -l app=kube-loxilb` | 检查 kube-loxilb Pod 状态 |
| 流量不通 | eBPF 规则未加载 | `loxicmd get lb` | 检查 LB 规则和后端状态 |
| BGP 邻居未建立 | 网络/AS 配置错误 | `loxicmd get bgp` | 检查 BGP 配置和网络连通性 |
| 健康检查失败 | 后端服务异常 | `loxicmd get ep` | 检查后端 Pod 健康状态 |
| DSR 回包异常 | 路由配置问题 | 检查后端节点路由 | 配置正确的回程路由 |
| 性能下降 | eBPF Map 溢出 | `bpftool map list` | 调整 Map 容量配置 |

## 生产案例

### 案例1: 裸金属集群 LoadBalancer 替代

**场景**: 私有云裸金属 K8s 集群无云 LB，需要外部可达的 VIP  
**方案**: LoxiLB + BGP 广播 VIP，上游路由器自动学习路由  
**效果**: 替代硬件 F5，吐量提升 3倍，成本降低 80%  

### 案例2: 5G UPF 高性能负载均衡

**场景**: 电信 5G UPF 需要 40Gbps L4 负载均衡  
**方案**: LoxiLB eBPF/XDP + DSR + ECMP 多路径  
**效果**: 单节点 40Gbps 线速转发，延迟 < 10μs  

## 对比

| 特性 | LoxiLB | MetalLB | kube-vip | Cilium LB |
|------|--------|---------|----------|----------|
| 数据面 | eBPF/XDP | iptables/IPVS | iptables/arpping | eBPF |
| 性能 | 极高（线速） | 中 | 中 | 高 |
| BGP | ✅ | ✅ | ⚠️ 有限 | ❌ |
| DSR | ✅ | ❌ | ❌ | ⚠️ 有限 |
| ECMP | ✅ | ❌ | ❌ | ❌ |
| NAT46/64 | ✅ | ❌ | ❌ | ❌ |
| 适用场景 | 高性能/电信 | 通用裸金属 | 简单 VIP | Cilium 用户 |

## 架构定位

在 CNCF 生态中，LoxiLB 属于 **Networking** 类别，为云原生应用提供基于 eBPF 的高性能 L4 负载均衡能力。

## 检查清单

- [ ] 生产环境配置 BGP 实现 VIP 高可用
- [ ] 配置主动健康检查自动剔除不健康后端
- [ ] 高吞吐场景使用 DSR 模式
- [ ] 监控 eBPF Map 使用率
- [ ] 配置多 LoxiLB 节点实现 LB 高可用
- [ ] 测试故障切换时间符合 SLA

## 参考链接

- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/04-网络/cilium.md|Cilium]]

## Related

- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[serverless-devs]] — Serverless Devs
- [[sermant]] — Sermant
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
