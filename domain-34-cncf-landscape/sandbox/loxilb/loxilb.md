# LoxiLB

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.loxilb.io/ |
| **GitHub** | https://github.com/loxilb-io/loxilb |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, C |
| **CNCF 状态** | Sandbox |

---

## 项目概述

LoxiLB 是一个基于 eBPF 的云原生负载均衡器，专注于为 Kubernetes 提供高性能的 L4 负载均衡服务。它可以作为 Kubernetes 的 Service LoadBalancer、Ingress 控制器或独立的负载均衡网关运行，利用 eBPF/XDP 技术在内核数据面实现线速转发，支持 BGP、ECMP、DSR（Direct Server Return）等高级网络特性，特别适合电信 5G、边缘计算等对性能要求严格的场景。

### 核心特性

- **eBPF/XDP 数据面**: 内核级负载均衡，极低延迟和高吞吐
- **Kubernetes LoadBalancer**: 为裸金属/边缘 K8s 提供 Service type LoadBalancer
- **BGP 集成**: 内置 BGP 支持，与网络基础设施无缝集成
- **DSR 模式**: Direct Server Return 减少回程流量
- **多协议**: TCP、UDP、SCTP、QUIC 负载均衡
- **5G/电信**: 支持 SRv6、GTP 隧道等电信网络协议
- **高可用**: 内置 Active-Standby HA 和连接状态同步

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│                  LoxiLB                            │
│                                                    │
│  ┌────────────────────────────────────────────┐   │
│  │           用户态 (Go)                       │   │
│  │  ┌──────┐ ┌─────┐ ┌──────┐ ┌───────────┐ │   │
│  │  │ API  │ │ BGP │ │ HA   │ │ K8s CCM   │ │   │
│  │  │Server│ │Agent│ │Mgr   │ │ Provider  │ │   │
│  │  └──┬───┘ └──┬──┘ └──┬───┘ └─────┬─────┘ │   │
│  └─────┼────────┼───────┼───────────┼────────┘   │
│        │        │       │           │              │
│  ┌─────▼────────▼───────▼───────────▼────────┐   │
│  │           eBPF 数据面 (C/XDP)              │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────┐ │   │
│  │  │ L4 LB   │ │ NAT/    │ │ Conntrack   │ │   │
│  │  │(TC/XDP) │ │ DSR     │ │ State Sync  │ │   │
│  │  └─────────┘ └─────────┘ └─────────────┘ │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────┐ │   │
│  │  │ SCTP LB │ │ GTP/    │ │ SRv6        │ │   │
│  │  │         │ │ VXLAN   │ │             │ │   │
│  │  └─────────┘ └─────────┘ └─────────────┘ │   │
│  └────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────┘
```

---

## 快速开始

### Kubernetes 部署

```bash
# 安装 LoxiLB 作为 K8s LoadBalancer
kubectl apply -f https://raw.githubusercontent.com/loxilb-io/kube-loxilb/main/manifest/kube-loxilb.yaml

# 部署 LoxiLB 节点
kubectl apply -f https://raw.githubusercontent.com/loxilb-io/loxilb/main/manifest/loxilb.yaml
```

### 创建 LoadBalancer Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
  annotations:
    loxilb.io/lbmode: "fullnat"        # fullnat / onearm / dsr
    loxilb.io/liveness: "yes"
    loxilb.io/liveness-timeout: "5"
spec:
  type: LoadBalancer
  loadBalancerClass: loxilb.io/loxilb
  selector:
    app: my-app
  ports:
    - port: 80
      targetPort: 8080
      protocol: TCP
```

### 独立模式 API

```bash
# 创建负载均衡规则
curl -X POST http://loxilb:11111/netlox/v1/config/loadbalancer \
  -H "Content-Type: application/json" \
  -d '{
    "serviceArguments": {
      "externalIP": "10.10.10.1",
      "port": 80,
      "protocol": "tcp",
      "sel": 0
    },
    "endpoints": [
      {"endpointIP": "192.168.1.10", "weight": 1, "targetPort": 8080},
      {"endpointIP": "192.168.1.11", "weight": 1, "targetPort": 8080}
    ]
  }'
```

---

## 与其他方案对比

| 特性 | LoxiLB | MetalLB | kube-vip | IPVS |
|:---|:---|:---|:---|:---|
| 数据面 | eBPF/XDP | 用户态 | 用户态 | 内核 IPVS |
| BGP | 内置 | 内置 | 内置 | 需外部 |
| DSR | 支持 | 不支持 | 不支持 | 支持 |
| 5G/电信协议 | SCTP/GTP/SRv6 | 不支持 | 不支持 | SCTP |
| HA | 内置 | VIP 模式 | VIP | 需 Keepalived |
| 性能 | 极高 (XDP) | 中 | 中 | 高 |

---

## 最佳实践

1. **部署模式**: 裸金属集群推荐外部模式（独立 LB 节点），小集群可用 in-cluster 模式
2. **BGP 配置**: 与上游路由器建立 BGP 邻居，实现 VIP 的自动广播
3. **DSR 模式**: 高流量服务启用 DSR 减少回程带宽消耗
4. **健康检查**: 启用端点健康检查，自动剔除故障后端
5. **监控**: 监控 eBPF map 的连接数和流量统计

---

## 参考资源

- [LoxiLB 官方文档](https://docs.loxilb.io/)
- [LoxiLB GitHub](https://github.com/loxilb-io/loxilb)
- [kube-loxilb](https://github.com/loxilb-io/kube-loxilb)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
