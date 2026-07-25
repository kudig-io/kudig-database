---
title: Terway 使用指南
description: '# Terway 使用指南'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- istio
- cilium
- networkpolicy
- crd
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 使用指南 是什么
- 如何 Terway 使用指南
trigger_keywords:
- Terway
- 使用指南
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 使用指南

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 03 - Terway 使用指南 (Usage Guide)

## 技术细节

### 基本使用

#### 创建使用 Terway 的 Pod

Terway 作为默认 CNI，Pod 创建时自动分配 VPC IP：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx-basic
  namespace: default
spec:
  containers:
    - name: nginx
      image: nginx:latest
      ports:
        - containerPort: 80
```

```bash
# 🟢 低风险：创建 Pod
kubectl apply -f nginx-basic.yaml

# 🟢 低风险：查看 Pod IP
kubectl get pod nginx-basic -o wide

# 🟢 低风险：查看 Pod 网络详情
kubectl exec -it nginx-basic -- ip addr
kubectl exec -it nginx-basic -- ip route
```

### Pod 网络注解

Terway 支持通过注解控制 Pod 网络配置：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx-annotated
  annotations:
    # 指定 vSwitch
    k8s.aliyun.com/vswitch: vsw-bp1234567890abcdef
    # 指定安全组
    k8s.aliyun.com/security-group: sg-bp1234567890abcdef
    # 指定 ENI 类型 (Secondary/Trunk)
    k8s.aliyun.com/eni-type: Secondary
    # 固定 IP (StatefulSet 场景)
    k8s.aliyun.com/fixed-ip: "true"
    # 指定 PodNetworking
    k8s.aliyun.com/pod-networking: high-performance-network
spec:
  containers:
    - name: nginx
      image: nginx:latest
```

#### 注解说明

| 注解 | 说明 | 示例值 |
|-----|------|-------|
| `k8s.aliyun.com/vswitch` | 指定 vSwitch ID | `vsw-bp123...` |
| `k8s.aliyun.com/security-group` | 指定安全组 ID | `sg-bp123...` |
| `k8s.aliyun.com/eni-type` | ENI 类型 | `Secondary`/`Trunk` |
| `k8s.aliyun.com/fixed-ip` | 固定 IP | `"true"` |
| `k8s.aliyun.com/pod-networking` | PodNetworking 名称 | `high-perf` |
| `k8s.aliyun.com/ip-stack` | IP 栈 | `ipv4`/`ipv6`/`dual` |

### 固定 IP 使用场景

StatefulSet 工作负载（如数据库）需要稳定的网络标识：

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql
  replicas: 3
  template:
    metadata:
      annotations:
        # 启用固定 IP
        k8s.aliyun.com/fixed-ip: "true"
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          env:
            - name: MYSQL_ROOT_PASSWORD
              value: password
---
apiVersion: v1
kind: Service
metadata:
  name: mysql
spec:
  clusterIP: None
  selector:
    app: mysql
```

```bash
# 🟢 低风险：验证固定 IP
# 删除 Pod 后重建，IP 应保持不变
kubectl delete pod mysql-0
kubectl get pod mysql-0 -o wide  # IP 应与之前相同
```

### NetworkPolicy 配置

Terway 支持 Kubernetes NetworkPolicy，通过安全组实现：

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-all-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend
      ports:
        - protocol: TCP
          port: 8080
```

```bash
# 🟡 中风险：应用 NetworkPolicy
kubectl apply -f networkpolicy.yaml

# 🟢 低风险：查看 NetworkPolicy
kubectl get networkpolicy -n production

# 🟢 低风险：测试连通性
kubectl exec -it frontend-pod -- curl -s http://backend:8080  # 应成功
kubectl exec -it other-pod -- curl -s http://backend:8080      # 应失败
```

### Service 集成

#### ClusterIP Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx-clusterip
spec:
  type: ClusterIP
  selector:
    app: nginx
  ports:
    - port: 80
      targetPort: 80
```

#### LoadBalancer Service (SLB)

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx-lb
  annotations:
    # 指定 SLB 规格
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: slb.s1.small
    # 使用内网 SLB
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: intranet
    # 指定 vSwitch
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-vswitch-id: vsw-bp1234567890abcdef
spec:
  type: LoadBalancer
  selector:
    app: nginx
  ports:
    - port: 80
      targetPort: 80
```

```bash
# 🟢 低风险：查看 Service 外部 IP
kubectl get svc nginx-lb -o wide

# 🟢 低风险：测试访问
curl http://<EXTERNAL-IP>
```

### 多网卡配置 (Multus)

```yaml
# 创建 NetworkAttachmentDefinition
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: terway-secondary
  namespace: default
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "terway-secondary",
      "type": "terway",
      "eniType": "Secondary",
      "vSwitch": "vsw-bp1234567890abcdef",
      "securityGroup": "sg-bp1234567890abcdef"
    }
---
# Pod 使用多网卡
apiVersion: v1
kind: Pod
metadata:
  name: multi-nic-pod
  annotations:
    k8s.v1.cni.cncf.io/networks: terway-secondary
spec:
  containers:
    - name: app
      image: nicolaka/netshoot
      command: ["sleep", "infinity"]
```

```bash
# 🟢 低风险：验证多网卡
kubectl exec -it multi-nic-pod -- ip addr
# 应看到 eth0 (主网卡) 和 net1 (附加网卡)
```

### IPv6 与双栈

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ipv6-pod
  annotations:
    k8s.aliyun.com/ip-stack: dual  # ipv4/ipv6/dual
spec:
  containers:
    - name: nginx
      image: nginx:latest
```

```bash
# 🟢 低风险：验证 IPv6
kubectl exec -it ipv6-pod -- ip -6 addr
kubectl exec -it ipv6-pod -- ping6 -c 3 <ipv6-address>
```

### 常见使用场景

| 场景 | 配置要点 | 示例 |
|-----|---------|------|
| **Web 应用** | 默认配置 + LoadBalancer | Nginx/Node.js |
| **数据库** | 固定 IP + StatefulSet | MySQL/PostgreSQL |
| **微服务** | NetworkPolicy 隔离 | Spring Cloud |
| **大数据** | IPVlan 模式 + 高带宽 | Spark/Flink |
| **多租户** | 独立安全组 + vSwitch | SaaS 平台 |

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[istio]]
- [[cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]

## Related

- [[bfe]] — BFE
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-USAGE-GUIDE.md|skill-k8s-node-notready-USAGE-GUIDE]] — Usage Guide
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[40-terway-product-overview]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 42-terway-usage-guide

<!-- risk-assessed -->
