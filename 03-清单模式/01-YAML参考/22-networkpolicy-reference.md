---
title: 22 - NetworkPolicy YAML 配置参考
description: 'title: 22 - NetworkPolicy YAML 配置参考'
summary: 'title: 22 - NetworkPolicy YAML 配置参考'
category: general
tags:
- yaml
- reference
- networking
- prometheus
- istio
- cilium
- flannel
- calico
- coredns
- mysql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 45min
intent_queries:
- 22-networkpolicy-reference的完整参考
- 22-networkpolicy-reference的所有命令/字段
- 22-networkpolicy-reference速查手册
trigger_keywords:
- NetworkPolicy
- YAML
- 配置参考
- manifests
- patterns
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- kafka-basics
- mysql-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 22 - [[NetworkPolicy|NetworkPolicy]] YAML 配置参考
description: '# 22 - NetworkPolicy YAML 配置参考'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- [[Prometheus|prometheus]]
- [[Istio|istio]]
- [[Cilium|cilium]]
- flannel
- calico
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- NetworkPolicy YAML 配置参考 是什么
- 如何 NetworkPolicy YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- NetworkPolicy
- YAML
- 配置参考
- yaml
- manifests
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/networkpolicy-fta.md
  label: '故障树: networkpolicy'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 22 - NetworkPolicy YAML 配置参考

<!-- chunk: 概述 -->## 概述

NetworkPolicy 是 Kubernetes 中用于控制 Pod 网络流量的资源对象。它定义了 Pod 之间以及 Pod 与外部网络端点之间的通信规则,实现**零信任网络**和**微分段**安全策略。NetworkPolicy 的实现依赖于 CNI 插件 (如 Calico、Cilium、Weave Net),不同的 CNI 插件对 NetworkPolicy 的支持程度和实现方式有所差异。

**适用版本**: Kubernetes v1.25 - v1.32  
**更新时间**: 2026-02

---

<!-- chunk: 1. NetworkPolicy 基础配置 -->## 1. NetworkPolicy 基础配置

## 1.1 基本 NetworkPolicy 结构

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  # NetworkPolicy 名称
  name: test-network-policy
  # NetworkPolicy 所在的 namespace
  # NetworkPolicy 只能选择同一 namespace 内的 Pods
  namespace: default
  labels:
    app: myapp
  annotations:
    description: "示例网络策略"

spec:
  # podSelector 选择此策略应用的 Pods
  # 空的 podSelector {} 表示选择 namespace 内的所有 Pods
  podSelector:
    matchLabels:
      app: myapp
      tier: backend
  
  # policyTypes 定义策略类型
  # Ingress: 入站流量规则
  # Egress: 出站流量规则
  # 如果不指定,默认为 ["Ingress"]
  # 如果指定了 egress 规则,会自动添加 "Egress"
  policyTypes:
    - Ingress
    - Egress
  
  # ingress 定义入站流量规则列表
  # 如果 policyTypes 包含 "Ingress" 但 ingress 为空,则拒绝所有入站流量
  ingress:
  # 每个规则定义一组允许的源和端口
  - from:
    # from 是一个选择器列表,使用 OR 逻辑 (满足任一条件即允许)
    # 但同一个 from 项内的多个选择器使用 AND 逻辑
    
    # 源 1: 来自同一 namespace 且带有特定标签的 Pods
    - podSelector:
        matchLabels:
          app: frontend
    
    # 源 2: 来自带有特定标签的 namespaces 中的所有 Pods
    - namespaceSelector:
        matchLabels:
          name: production
    
    # 源 3: 来自特定 IP 地址块
    - ipBlock:
        # 允许的 CIDR 范围
        cidr: 172.17.0.0/16
        # 排除的 CIDR 范围 (可选)
        except:
          - 172.17.1.0/24
    
    # ports 定义允许的端口列表
    # 如果不指定 ports,则允许所有端口
    ports:
    # 端口 1: TCP 8080
    - protocol: TCP
      port: 8080
    
    # 端口 2: TCP 端口范围 8000-9000 (v1.25+)
    - protocol: TCP
      port: 8000
      endPort: 9000
  
  # egress 定义出站流量规则列表
  # 如果 policyTypes 包含 "Egress" 但 egress 为空,则拒绝所有出站流量
  egress:
  # 每个规则定义一组允许的目标和端口
  - to:
    # to 是一个选择器列表,使用 OR 逻辑
    
    # 目标 1: 到同一 namespace 且带有特定标签的 Pods
    - podSelector:
        matchLabels:
          app: database
    
    # 目标 2: 到特定 IP 地址块 (通常用于外部服务)
    - ipBlock:
        cidr: 10.0.0.0/8
    
    # ports 定义允许的端口列表
    ports:
    # 端口 1: TCP 3306 (MySQL)
    - protocol: TCP
      port: 3306
    
    # 端口 2: UDP 53 (DNS)
    - protocol: UDP
      port: 53
```

---

## 1.2 默认拒绝所有入站流量

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
  annotations:
    description: "默认拒绝所有入站流量 (白名单模式)"

spec:
  # 选择 namespace 内的所有 Pods
  podSelector: {}
  
  # 指定策略类型为 Ingress
  policyTypes:
    - Ingress
  
  # ingress 规则为空 (或不指定 ingress 字段)
  # 表示拒绝所有入站流量
  # ingress: []
```

**效果**:
- `production` namespace 内的所有 Pods **拒绝所有入站流量**
- 需要额外的 NetworkPolicy 显式允许特定流量
- 这是**零信任网络**的基础策略

---

## 1.3 默认拒绝所有出站流量

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
  annotations:
    description: "默认拒绝所有出站流量"

spec:
  # 选择 namespace 内的所有 Pods
  podSelector: {}
  
  # 指定策略类型为 Egress
  policyTypes:
    - Egress
  
  # egress 规则为空
  # 表示拒绝所有出站流量
  # egress: []
```

**效果**:
- `production` namespace 内的所有 Pods **拒绝所有出站流量**
- 包括访问 kube-dns (DNS 解析会失败)
- 通常需要配合允许 DNS 流量的策略使用

---

## 1.4 默认拒绝所有流量 (入站 + 出站)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
  annotations:
    description: "默认拒绝所有入站和出站流量 (完全隔离)"

spec:
  # 选择 namespace 内的所有 Pods
  podSelector: {}
  
  # 同时指定 Ingress 和 Egress
  policyTypes:
    - Ingress
    - Egress
  
  # 两个规则都为空,拒绝所有流量
```

**效果**:
- `production` namespace 内的所有 Pods **完全隔离**
- 无法接收任何入站流量
- 无法发送任何出站流量 (包括 DNS)

---

## 1.5 默认允许所有入站流量

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-all-ingress
  namespace: development
  annotations:
    description: "允许所有入站流量 (开发环境)"

spec:
  # 选择 namespace 内的所有 Pods
  podSelector: {}
  
  policyTypes:
    - Ingress
  
  ingress:
  # 空的 from 列表表示允许所有源
  - {}
```

**效果**:
- `development` namespace 内的所有 Pods 允许来自任何源的入站流量
- 适用于开发和测试环境

---

## 1.6 默认允许所有出站流量

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-all-egress
  namespace: development
  annotations:
    description: "允许所有出站流量 (开发环境)"

spec:
  # 选择 namespace 内的所有 Pods
  podSelector: {}
  
  policyTypes:
    - Egress
  
  egress:
  # 空的 to 列表表示允许所有目标
  - {}
```

**效果**:
- `development` namespace 内的所有 Pods 允许到任何目标的出站流量

---

<!-- chunk: 2. from/to 选择器详解 -->## 2. from/to 选择器详解

## 2.1 podSelector (同一 namespace)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-frontend
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: backend
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    # 只选择同一 namespace 内的 Pods
    - podSelector:
        matchLabels:
          app: frontend
          tier: web
    
    ports:
    - protocol: TCP
      port: 8080
```

**行为**:
- 允许来自 `default` namespace 内带有 `app=frontend` 和 `tier=web` 标签的 Pods
- 到 `default` namespace 内带有 `app=backend` 标签的 Pods
- 端口 TCP 8080

---

## 2.2 namespaceSelector (跨 namespace)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-monitoring
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    # 选择带有特定标签的 namespaces 中的所有 Pods
    - namespaceSelector:
        matchLabels:
          name: monitoring
    
    ports:
    - protocol: TCP
      port: 9090  # Prometheus metrics
```

**行为**:
- 允许来自带有 `name=monitoring` 标签的 namespace 内的**所有 Pods**
- 到 `production` namespace 内带有 `app=backend` 标签的 Pods
- 端口 TCP 9090

**注意**: 确保 namespace 有正确的标签:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 给 namespace 添加标签
kubectl label namespace monitoring name=monitoring
```
---

## 2.3 podSelector + namespaceSelector (AND 逻辑)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-specific-pod-in-namespace
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    # 同一个 from 项内的多个选择器使用 AND 逻辑
    # 必须同时满足: namespace 有标签 AND Pod 有标签
    - podSelector:
        matchLabels:
          app: prometheus
      namespaceSelector:
        matchLabels:
          name: monitoring
    
    ports:
    - protocol: TCP
      port: 9090
```

**行为**:
- 允许来自带有 `name=monitoring` 标签的 namespace 内
- **且** 带有 `app=prometheus` 标签的 Pods
- 到 `production` namespace 内带有 `app=backend` 标签的 Pods
- 端口 TCP 9090

**AND vs OR 逻辑示意**:

```yaml
# OR 逻辑: 满足任一条件即允许
ingress:
- from:
  # 条件 1: namespace=monitoring 的所有 Pods
  - namespaceSelector:
      matchLabels:
        name: monitoring
  # OR
  # 条件 2: 同一 namespace 内 app=frontend 的 Pods
  - podSelector:
      matchLabels:
        app: frontend

---
# AND 逻辑: 必须同时满足所有条件
ingress:
- from:
  # 条件: namespace=monitoring AND app=prometheus
  - podSelector:
      matchLabels:
        app: prometheus
    namespaceSelector:
      matchLabels:
        name: monitoring
```

---

## 2.4 ipBlock (IP 地址块)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-external
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    # 允许来自特定 IP 地址块的流量
    - ipBlock:
        # 允许 10.0.0.0/8 网段
        cidr: 10.0.0.0/8
        # 排除 10.1.0.0/16 子网
        except:
          - 10.1.0.0/16
          - 10.2.0.0/16
    
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
```

**行为**:
- 允许来自 `10.0.0.0/8` 网段的流量
- **但排除** `10.1.0.0/16` 和 `10.2.0.0/16` 子网
- 到 `default` namespace 内带有 `app=web` 标签的 Pods
- 端口 TCP 80 和 443

**注意**:
- `ipBlock` 通常用于允许外部流量或限制对外访问
- Pod IP 通常不匹配 `ipBlock` (除非 Pod 网络与 Node 网络相同)
- LoadBalancer 或 NodePort Service 的源 IP 可能被 SNAT,需要配置 `externalTrafficPolicy: Local`

---

## 2.5 混合选择器 (OR 逻辑)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-multiple-sources
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: database
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    # 源 1: 同一 namespace 的 backend Pods
    - podSelector:
        matchLabels:
          app: backend
    
    # OR
    # 源 2: monitoring namespace 的所有 Pods
    - namespaceSelector:
        matchLabels:
          name: monitoring
    
    # OR
    # 源 3: 管理员 VPN 网段
    - ipBlock:
        cidr: 192.168.100.0/24
    
    ports:
    - protocol: TCP
      port: 3306  # MySQL
```

**行为**:
- 允许来自以下**任一**源的流量:
  1. `production` namespace 内 `app=backend` 的 Pods
  2. `monitoring` namespace 内的所有 Pods
  3. `192.168.100.0/24` IP 段 (VPN)
- 到 `production` namespace 内 `app=database` 的 Pods
- 端口 TCP 3306

---

<!-- chunk: 3. 端口规则详解 -->## 3. 端口规则详解

## 3.1 单个端口

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-specific-port
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    
    ports:
    # 允许 TCP 8080 端口
    - protocol: TCP
      port: 8080
```

---

## 3.2 多个端口

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-multiple-ports
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    - podSelector: {}
    
    ports:
    # HTTP
    - protocol: TCP
      port: 80
    
    # HTTPS
    - protocol: TCP
      port: 443
    
    # Metrics
    - protocol: TCP
      port: 9090
    
    # Health check
    - protocol: TCP
      port: 8081
```

---

## 3.3 端口范围 (v1.25+)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-port-range
  namespace: default
  annotations:
    description: "允许端口范围 (v1.25+)"
spec:
  podSelector:
    matchLabels:
      app: media-server
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: client
    
    ports:
    # 端口范围: 8000-9000
    # endPort 是 v1.25 引入的特性
    - protocol: TCP
      port: 8000
      endPort: 9000
    
    # 等价于允许 8000, 8001, 8002, ..., 9000
```

**注意**:
- `endPort` 必须 >= `port`
- `endPort` 是包含的 (inclusive)
- 并非所有 CNI 插件都支持 `endPort` (Calico 3.26+, Cilium 1.13+)

---

## 3.4 命名端口

```yaml
---
# Pod 定义命名端口
apiVersion: v1
kind: Pod
metadata:
  name: web-pod
  namespace: default
  labels:
    app: web
spec:
  containers:
  - name: nginx
    image: nginx:1.25
    ports:
    # 命名端口: http-port
    - name: http-port
      containerPort: 8080
      protocol: TCP
    # 命名端口: metrics-port
    - name: metrics-port
      containerPort: 9090
      protocol: TCP

---
# NetworkPolicy 引用命名端口
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-named-ports
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    - podSelector: {}
    
    ports:
    # 引用命名端口 http-port
    # 会自动解析为 8080
    - protocol: TCP
      port: http-port
    
    # 引用命名端口 metrics-port
    # 会自动解析为 9090
    - protocol: TCP
      port: metrics-port
```

**优势**:
- 端口号变化时只需修改 Pod 定义,NetworkPolicy 无需改动
- 提高可读性

---

## 3.5 不指定端口 (允许所有端口)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-all-ports
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    
    # 不指定 ports 字段
    # 表示允许所有端口和协议
```

**行为**:
- 允许来自 `app=frontend` 的 Pods 访问 `app=web` 的 Pods
- 所有端口和协议 (TCP, UDP, SCTP)

---

## 3.6 协议类型

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-different-protocols
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: media-server
  
  policyTypes:
    - Ingress
  
  ingress:
  - from:
    - podSelector: {}
    
    ports:
    # TCP
    - protocol: TCP
      port: 8080
    
    # UDP
    - protocol: UDP
      port: 5000
    
    # SCTP (Stream Control Transmission Protocol)
    # 需要 CNI 插件支持
    - protocol: SCTP
      port: 9000
```

**支持的协议**:
- `TCP`: 默认,最常用
- `UDP`: 用于 DNS、QUIC 等
- `SCTP`: 流控制传输协议,较少使用 (需要 CNI 支持)

---

<!-- chunk: 4. 常见场景配置 -->## 4. 常见场景配置

## 4.1 允许 DNS 解析

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
  annotations:
    description: "允许访问 kube-dns (CoreDNS)"
spec:
  podSelector: {}  # 应用于 namespace 内所有 Pods
  
  policyTypes:
    - Egress
  
  egress:
  # 允许访问 kube-system namespace 的 kube-dns
  - to:
    - namespaceSelector:
        matchLabels:
          # kube-system namespace 通常有 kubernetes.io/metadata.name 标签
          kubernetes.io/metadata.name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    
    ports:
    # DNS 使用 UDP 53 和 TCP 53
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

**注意**:
- DNS 是大部分应用的基础依赖
- 如果使用 `default-deny-egress`,必须配合此策略
- 确保 `kube-system` namespace 有正确的标签:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# v1.22+ 自动添加
kubectl get namespace kube-system --show-labels
# kubernetes.io/metadata.name=kube-system

# 如果缺少,手动添加
kubectl label namespace kube-system kubernetes.io/metadata.name=kube-system
```
---

## 4.2 三层应用架构 (Frontend → Backend → Database)

```yaml
---
# Namespace 标签 (用于跨 namespace 访问)
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    name: production

---
# 1. Frontend: 接收外部流量,访问 Backend
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: frontend
  
  policyTypes:
    - Ingress
    - Egress
  
  ingress:
  # 允许来自 Ingress Controller 的流量
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
      podSelector:
        matchLabels:
          app.kubernetes.io/name: ingress-nginx
    
    ports:
    - protocol: TCP
      port: 80
  
  egress:
  # 允许访问 Backend
  - to:
    - podSelector:
        matchLabels:
          tier: backend
    
    ports:
    - protocol: TCP
      port: 8080
  
  # 允许 DNS
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
# 2. Backend: 接收 Frontend 流量,访问 Database
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: backend
  
  policyTypes:
    - Ingress
    - Egress
  
  ingress:
  # 只允许来自 Frontend 的流量
  - from:
    - podSelector:
        matchLabels:
          tier: frontend
    
    ports:
    - protocol: TCP
      port: 8080
  
  egress:
  # 允许访问 Database
  - to:
    - podSelector:
        matchLabels:
          tier: database
    
    ports:
    - protocol: TCP
      port: 3306  # MySQL
  
  # 允许访问外部 API (示例: payment gateway)
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
        except:
          # 排除私有 IP 段
          - 10.0.0.0/8
          - 172.16.0.0/12
          - 192.168.0.0/16
    
    ports:
    - protocol: TCP
      port: 443
  
  # 允许 DNS
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

---
# 3. Database: 只接收 Backend 流量,无出站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: database-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: database
  
  policyTypes:
    - Ingress
    - Egress
  
  ingress:
  # 只允许来自 Backend 的流量
  - from:
    - podSelector:
        matchLabels:
          tier: backend
    
    ports:
    - protocol: TCP
      port: 3306
  
  egress:
  # 允许 DNS (用于健康检查等)
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
  
  # Database 通常不需要其他出站流量
  # 如果需要备份到 S3,可以添加 ipBlock 规则
```

---

## 4.3 允许来自 Ingress Controller 的流量

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-ingress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  # 方法 1: 使用 namespaceSelector + podSelector
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
      podSelector:
        matchLabels:
          app.kubernetes.io/name: ingress-nginx
    
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443

---
# 方法 2: 使用 ipBlock (如果 Ingress Controller 使用 hostNetwork)
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-ingress-ip
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web
  
  policyTypes:
    - Ingress
  
  ingress:
  # 允许来自 Node 网络的流量 (Ingress Controller 使用 hostNetwork)
  - from:
    - ipBlock:
        cidr: 192.168.0.0/16  # Node 网络 CIDR
    
    ports:
    - protocol: TCP
      port: 80
```

**注意**:
- 方法 1 更精确,但要求 Ingress Controller 不使用 `hostNetwork`
- 方法 2 适用于 Ingress Controller 使用 `hostNetwork` 的情况
- 确保 `ingress-nginx` namespace 有正确的标签

---

## 4.4 限制出站流量 (只允许特定域名)

**问题**: NetworkPolicy 不直接支持域名,只能使用 IP 地址。

**解决方案 1**: 使用 Calico 的 GlobalNetworkPolicy (支持域名)

```yaml
# Calico GlobalNetworkPolicy (Calico 特有)
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: allow-specific-domains
spec:
  selector: app == 'backend'
  types:
    - Egress
  egress:
  # 允许访问特定域名
  - action: Allow
    destination:
      domains:
        - "api.example.com"
        - "*.googleapis.com"
    protocol: TCP
    destination:
      ports:
        - 443
```

**解决方案 2**: 使用 Cilium 的 CiliumNetworkPolicy (支持域名)

```yaml
# Cilium CiliumNetworkPolicy (Cilium 特有)
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: allow-specific-domains
  namespace: default
spec:
  endpointSelector:
    matchLabels:
      app: backend
  egress:
  # 允许访问特定域名
  - toFQDNs:
    - matchName: "api.example.com"
    - matchPattern: "*.googleapis.com"
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
```

**解决方案 3**: 使用标准 NetworkPolicy + 外部 DNS 解析

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-specific-ips
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: backend
  
  policyTypes:
    - Egress
  
  egress:
  # 允许访问特定 IP (手动解析域名)
  - to:
    - ipBlock:
        # api.example.com 的 IP
        cidr: 203.0.113.0/32
    
    ports:
    - protocol: TCP
      port: 443
  
  # 允许 DNS
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
```

**限制**:
- 方案 3 需要定期更新 IP 地址 (域名 IP 可能变化)
- 建议使用 Calico 或 Cilium 的扩展功能

---

## 4.5 PCI-DSS 合规: 信用卡处理环境隔离

```yaml
---
# Namespace: 信用卡处理环境 (CDE - Cardholder Data Environment)
apiVersion: v1
kind: Namespace
metadata:
  name: pci-cde
  labels:
    name: pci-cde
    compliance: pci-dss

---
# 默认拒绝所有流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: pci-cde
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress

---
# 允许支付网关访问信用卡处理服务
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-payment-gateway
  namespace: pci-cde
  annotations:
    compliance: "PCI-DSS Requirement 1.2.1"
    description: "只允许支付网关访问信用卡处理服务"
spec:
  podSelector:
    matchLabels:
      app: payment-processor
      compliance: pci-dss
  
  policyTypes:
    - Ingress
    - Egress
  
  ingress:
  # 只允许来自支付网关的流量
  - from:
    - podSelector:
        matchLabels:
          app: payment-gateway
    
    ports:
    - protocol: TCP
      port: 8443  # HTTPS
  
  egress:
  # 允许访问信用卡数据库
  - to:
    - podSelector:
        matchLabels:
          app: card-database
    
    ports:
    - protocol: TCP
      port: 5432  # PostgreSQL
  
  # 允许访问外部支付处理器 (Stripe, PayPal)
  - to:
    - ipBlock:
        # Stripe API IP 范围 (示例)
        cidr: 54.187.174.169/32
    
    ports:
    - protocol: TCP
      port: 443
  
  # 允许 DNS
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

---
# 信用卡数据库: 完全隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: card-database-policy
  namespace: pci-cde
  annotations:
    compliance: "PCI-DSS Requirement 1.2.1, 1.3.2"
    description: "信用卡数据库只允许来自支付处理器的流量"
spec:
  podSelector:
    matchLabels:
      app: card-database
      compliance: pci-dss
  
  policyTypes:
    - Ingress
    - Egress
  
  ingress:
  # 只允许来自支付处理器的流量
  - from:
    - podSelector:
        matchLabels:
          app: payment-processor
    
    ports:
    - protocol: TCP
      port: 5432
  
  egress:
  # 数据库不需要出站流量 (除了 DNS 健康检查)
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

---
# 审计日志: 允许日志收集器访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-log-collector
  namespace: pci-cde
  annotations:
    compliance: "PCI-DSS Requirement 10.5"
spec:
  podSelector:
    matchLabels:
      compliance: pci-dss
  
  policyTypes:
    - Egress
  
  egress:
  # 允许发送日志到日志收集器
  - to:
    - namespaceSelector:
        matchLabels:
          name: logging
      podSelector:
        matchLabels:
          app: fluentd
    
    ports:
    - protocol: TCP
      port: 24224  # Fluentd
```

---

<!-- chunk: 5. 内部原理: CNI 实现差异 -->## 5. 内部原理: CNI 实现差异

## 5.1 Calico 实现

**架构**:

```
┌─────────────────────────────────────────────────────────────┐
│  Kubernetes API Server                                       │
│  - NetworkPolicy 资源                                        │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  Calico Controller (calico-kube-controllers)                 │
│  - 监听 NetworkPolicy, Pod, Namespace 变化                   │
│  - 转换为 Calico NetworkPolicy 对象                          │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  Felix (calico-node daemonset)                               │
│  - 在每个节点上运行                                          │
│  - 将 NetworkPolicy 转换为 iptables 规则或 eBPF 程序          │
│  - 应用到本节点的 Pods                                       │
└─────────────────────────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  Linux Kernel (iptables 或 eBPF)                             │
│  - iptables 链: cali-INPUT, cali-OUTPUT, cali-FORWARD         │
│  - eBPF 程序: 附加到网络接口                                 │
└─────────────────────────────────────────────────────────────┘
```

**iptables 实现**:

```bash
# Calico 创建的 iptables 规则示例
iptables -t filter -L cali-INPUT -n -v

# 示例规则:
# Chain cali-INPUT (1 references)
#  pkts bytes target     prot opt in     out     source               destination
#     0     0 ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            /* cali:Cz_u1IQiXIMmKD4c */ ctstate RELATED,ESTABLISHED
#     0     0 DROP       all  --  *      *       0.0.0.0/0            0.0.0.0/0            /* cali:8s1h1TpxZzQpVVN_ */ ctstate INVALID
#     0     0 cali-wl-to-host  all  --  cali+  *       0.0.0.0/0            0.0.0.0/0            /* cali:lH3rsRQz4PB0U5RZ */
#     0     0 DROP       all  --  *      *       0.0.0.0/0            0.0.0.0/0            /* cali:pGQEoAM3g5LGK8W5 */
```

**eBPF 实现** (Calico v3.13+):

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启用 eBPF 模式
kubectl patch felixconfiguration default --type='merge' -p '{"spec":{"bpfEnabled":true}}'

# eBPF 程序附加到网络接口
bpftool prog show

# 示例输出:
# 123: cgroup_skb  name cali_to_host_ep  tag abc123...
# 124: cgroup_skb  name cali_from_host_ep  tag def456...
```
**Calico 特性**:
- ✅ 支持标准 NetworkPolicy
- ✅ 支持 `ipBlock`, `podSelector`, `namespaceSelector`
- ✅ 支持 `endPort` (v3.26+)
- ✅ 扩展: GlobalNetworkPolicy (集群级别策略)
- ✅ 扩展: NetworkSet (IP 集合复用)
- ✅ 扩展: 域名过滤 (DNS policy)
- ⚡ 性能: eBPF 模式性能优于 iptables 模式

---

## 5.2 Cilium 实现

**架构**:

```
┌─────────────────────────────────────────────────────────────┐
│  Kubernetes API Server                                       │
│  - NetworkPolicy 资源                                        │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  Cilium Operator                                             │
│  - 监听 NetworkPolicy, Pod, Namespace, Service 变化          │
│  - 转换为 Cilium 内部策略                                    │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  Cilium Agent (cilium daemonset)                             │
│  - 在每个节点上运行                                          │
│  - 使用 eBPF 实现网络策略                                    │
│  - 身份管理: 每个 Pod 分配唯一的 Security Identity          │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  Linux Kernel (eBPF)                                         │
│  - eBPF 程序附加到网络接口 (TC hook, XDP)                    │
│  - eBPF Map: 存储策略、身份、连接跟踪                        │
└─────────────────────────────────────────────────────────────┘
```

**eBPF 实现**:

```bash
# 查看 Cilium eBPF 程序
cilium bpf policy list

# 示例输出:
# POLICY       IDENTITY   LABELS
# Ingress      12345      k8s:app=frontend
# Egress       12345      k8s:app=frontend

# 查看 Cilium Identity
cilium identity list

# 示例输出:
# IDENTITY   LABELS
# 1          reserved:host
# 2          reserved:world
# 12345      k8s:app=frontend k8s:io.kubernetes.pod.namespace=default
```

**Cilium 特性**:
- ✅ 支持标准 NetworkPolicy
- ✅ 支持 `ipBlock`, `podSelector`, `namespaceSelector`
- ✅ 支持 `endPort` (v1.13+)
- ✅ 原生 eBPF 实现 (无 iptables 开销)
- ✅ 扩展: CiliumNetworkPolicy (L7 策略, HTTP/Kafka/gRPC)
- ✅ 扩展: FQDN 过滤 (域名策略)
- ✅ 扩展: Service Mesh (取代 Istio sidecar)
- ⚡ 性能: 原生 eBPF,性能优异
- 🔍 可观测性: Hubble (网络流量可视化)

---

## 5.3 其他 CNI 插件对比

| CNI 插件 | NetworkPolicy 支持 | 实现方式 | endPort 支持 | 扩展功能 | 性能 |
|----------|-------------------|----------|--------------|----------|------|
| **Calico** | ✅ 完整支持 | iptables 或 eBPF | ✅ v3.26+ | GlobalNetworkPolicy, DNS policy | ⭐⭐⭐⭐ |
| **Cilium** | ✅ 完整支持 | eBPF (原生) | ✅ v1.13+ | L7 policy, FQDN, Service Mesh | ⭐⭐⭐⭐⭐ |
| **Weave Net** | ✅ 支持 | iptables | ❌ | 加密通信 | ⭐⭐⭐ |
| **Flannel** | ❌ 不支持 | N/A | ❌ | 简单易用 | ⭐⭐⭐ |
| **Canal** (Flannel+Calico) | ✅ 支持 (Calico) | iptables | ✅ | Calico 策略 | ⭐⭐⭐⭐ |
| **Antrea** | ✅ 完整支持 | OVS + eBPF | ✅ v1.4+ | Traceflow, 多集群 | ⭐⭐⭐⭐ |
| **AWS VPC CNI** | ⚠️ 需要配合 Calico | N/A | ❌ | ENI 直通 | ⭐⭐⭐ |
| **Azure CNI** | ⚠️ 需要配合 Calico/Cilium | N/A | ❌ | Azure VNET 集成 | ⭐⭐⭐ |

---

<!-- chunk: 6. 生产案例 -->## 6. 生产案例

## 6.1 案例 1: 零信任网络 (Zero Trust)

**目标**: 所有流量默认拒绝,显式允许必要的通信。

```yaml
---
# Namespace
apiVersion: v1
kind: Namespace
metadata:
  name: zerotrust
  labels:
    name: zerotrust

---
# 步骤 1: 默认拒绝所有流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: zerotrust
  annotations:
    description: "零信任: 默认拒绝所有流量"
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress

---
# 步骤 2: 允许 DNS
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: zerotrust
spec:
  podSelector: {}
  policyTypes:
    - Egress
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
# 步骤 3: Frontend → Backend
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-to-backend
  namespace: zerotrust
spec:
  podSelector:
    matchLabels:
      tier: backend
  policyTypes:
    - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: frontend
    ports:
    - protocol: TCP
      port: 8080

---
# 步骤 4: Backend → Database
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-to-database
  namespace: zerotrust
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes:
    - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 5432

---
# 步骤 5: Frontend Egress (访问 Backend + 外部 API)
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-egress
  namespace: zerotrust
spec:
  podSelector:
    matchLabels:
      tier: frontend
  policyTypes:
    - Egress
  egress:
  # 访问 Backend
  - to:
    - podSelector:
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 8080
  
  # 访问外部 HTTPS
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
        except:
          - 10.0.0.0/8
          - 172.16.0.0/12
          - 192.168.0.0/16
    ports:
    - protocol: TCP
      port: 443

---
# 步骤 6: Backend Egress (访问 Database)
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-egress
  namespace: zerotrust
spec:
  podSelector:
    matchLabels:
      tier: backend
  policyTypes:
    - Egress
  egress:
  # 访问 Database
  - to:
    - podSelector:
        matchLabels:
          tier: database
    ports:
    - protocol: TCP
      port: 5432
```

**验证零信任**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试: Frontend → Backend (应该成功)
kubectl exec -it frontend-pod -n zerotrust -- curl http://backend-service:8080
# 200 OK

# 测试: Frontend → Database (应该失败)
kubectl exec -it frontend-pod -n zerotrust -- curl http://database-service:5432
# Timeout (blocked by NetworkPolicy)

# 测试: Backend → Database (应该成功)
kubectl exec -it backend-pod -n zerotrust -- psql -h database-service -p 5432
# Connected
```
---

## 6.2 案例 2: 微分段 (Microsegmentation)

**目标**: 在同一 namespace 内对 Pods 进行细粒度隔离。

```yaml
---
# Namespace: 电商平台
apiVersion: v1
kind: Namespace
metadata:
  name: ecommerce
  labels:
    name: ecommerce

---
# 微服务 1: 用户服务
# 接收: API Gateway, 访问: User Database
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: user-service-policy
  namespace: ecommerce
spec:
  podSelector:
    matchLabels:
      app: user-service
  policyTypes:
    - Ingress
    - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: user-db
    ports:
    - protocol: TCP
      port: 5432
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

---
# 微服务 2: 产品服务
# 接收: API Gateway, 访问: Product Database
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: product-service-policy
  namespace: ecommerce
spec:
  podSelector:
    matchLabels:
      app: product-service
  policyTypes:
    - Ingress
    - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: product-db
    ports:
    - protocol: TCP
      port: 3306
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

---
# 微服务 3: 订单服务
# 接收: API Gateway, 访问: Order Database + User Service + Product Service
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: order-service-policy
  namespace: ecommerce
spec:
  podSelector:
    matchLabels:
      app: order-service
  policyTypes:
    - Ingress
    - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
  egress:
  # 访问 Order Database
  - to:
    - podSelector:
        matchLabels:
          app: order-db
    ports:
    - protocol: TCP
      port: 5432
  # 访问 User Service (获取用户信息)
  - to:
    - podSelector:
        matchLabels:
          app: user-service
    ports:
    - protocol: TCP
      port: 8080
  # 访问 Product Service (获取产品信息)
  - to:
    - podSelector:
        matchLabels:
          app: product-service
    ports:
    - protocol: TCP
      port: 8080
  # DNS
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

---
# 数据库隔离: User Database 只允许 User Service 访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: user-db-policy
  namespace: ecommerce
spec:
  podSelector:
    matchLabels:
      app: user-db
  policyTypes:
    - Ingress
    - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: user-service
    ports:
    - protocol: TCP
      port: 5432
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
```

**微分段效果**:
- User Service 只能访问 User Database
- Product Service 只能访问 Product Database
- Order Service 可以访问 Order Database + User Service + Product Service
- 数据库之间完全隔离
- 防止横向移动 (Lateral Movement)

---

## 6.3 案例 3: 控制 DNS 出站流量

**目标**: 限制 Pods 只能解析特定域名 (使用 Cilium FQDN 策略)。

```yaml
# Cilium CiliumNetworkPolicy
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: allow-specific-domains
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend
  
  egress:
  # 规则 1: 允许解析和访问特定域名
  - toFQDNs:
    # 精确匹配
    - matchName: "api.stripe.com"
    - matchName: "api.twilio.com"
    
    # 通配符匹配
    - matchPattern: "*.googleapis.com"
    - matchPattern: "*.amazonaws.com"
    
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
  
  # 规则 2: 允许访问 kube-dns
  - toEndpoints:
    - matchLabels:
        k8s:io.kubernetes.pod.namespace: kube-system
        k8s:k8s-app: kube-dns
    toPorts:
    - ports:
      - port: "53"
        protocol: UDP
      - port: "53"
        protocol: TCP
  
  # 规则 3: 允许访问同一 namespace 的服务
  - toEndpoints:
    - matchLabels:
        k8s:io.kubernetes.pod.namespace: production
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
```

**Cilium FQDN 策略工作原理**:

```
┌─────────────────────────────────────────────────────────────┐
│  1. Pod 发起 DNS 查询                                        │
│     curl https://api.stripe.com                              │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  2. Cilium 拦截 DNS 查询 (eBPF)                              │
│     - 解析 DNS 响应,获取 IP 地址                             │
│     - 将 FQDN → IP 映射存储在 eBPF Map                       │
│     - 动态更新 NetworkPolicy 规则                            │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  3. Pod 发起 HTTPS 连接                                      │
│     curl https://54.187.174.169 (api.stripe.com IP)         │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  4. Cilium 检查 FQDN 策略 (eBPF)                             │
│     - 目标 IP 54.187.174.169 对应 api.stripe.com             │
│     - api.stripe.com 在允许列表 → 允许连接                   │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│  5. 连接成功                                                 │
└─────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 7. 常见问题排查 -->## 7. 常见问题排查

## 7.1 NetworkPolicy 不生效

**排查步骤**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 CNI 插件是否支持 NetworkPolicy
kubectl get pods -n kube-system | grep -E "calico|cilium|weave"

# 如果使用 Flannel (不支持 NetworkPolicy)
kubectl get pods -n kube-system | grep flannel

# 2. 检查 NetworkPolicy 资源
kubectl get networkpolicies -A

# 3. 检查 Pod 标签是否匹配
kubectl get pods --show-labels -n production

# 4. 检查 Namespace 标签
kubectl get namespaces --show-labels

# 5. 验证 Pod 是否被策略选中
kubectl describe networkpolicy <policy-name> -n <namespace>

# 6. 测试网络连通性
kubectl exec -it <source-pod> -n <namespace> -- curl <target-service>:8080
```
## 7.2 Calico 特定问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Calico 状态
kubectl get pods -n kube-system -l k8s-app=calico-node

# 2. 查看 Calico 日志
kubectl logs -n kube-system -l k8s-app=calico-node

# 3. 检查 Felix 配置
kubectl get felixconfiguration default -o yaml

# 4. 检查 iptables 规则
# 在节点上执行
iptables-save | grep cali

# 5. 调试 NetworkPolicy
# 启用 Calico 调试日志
kubectl patch felixconfiguration default -p '{"spec":{"logSeverityScreen":"Debug"}}'
```
## 7.3 Cilium 特定问题

```bash
# 1. 检查 Cilium 状态
cilium status

# 2. 查看 Cilium 策略
cilium policy get

# 3. 查看 Cilium Identity
cilium identity list

# 4. 监控网络流量 (Hubble)
hubble observe --from-pod <pod-name> --to-pod <target-pod>

# 5. 检查 eBPF 程序
cilium bpf policy list
```

---

<!-- chunk: 8. 最佳实践 -->## 8. 最佳实践

## 8.1 策略设计原则

1. **默认拒绝 + 显式允许** (零信任):
   ```yaml
   # 第一步: 创建 default-deny-all
   # 第二步: 逐个添加允许规则
   ```

2. **最小权限原则**:
   - 只允许必要的流量
   - 明确指定端口 (避免允许所有端口)
   - 使用 `podSelector` 而非 namespace 级别策略

3. **分层策略**:
   ```
   Layer 1: default-deny-all (baseline)
   Layer 2: allow-dns (infrastructure)
   Layer 3: allow-monitoring (observability)
   Layer 4: app-specific policies (application)
   ```

4. **标签规范**:
   ```yaml
   # 使用一致的标签
   labels:
     app: myapp          # 应用名称
     tier: frontend      # 层级
     version: v1.0       # 版本
     team: platform      # 团队
   ```

5. **文档化**:
   ```yaml
   annotations:
     description: "允许 Frontend 访问 Backend"
     jira: "PROJ-1234"
     owner: "platform-team@example.com"
   ```

## 8.2 测试和验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 使用 kubectl auth can-i (RBAC)
kubectl auth can-i create networkpolicies -n production

# 2. 使用 kubectl exec 测试连通性
kubectl exec -it frontend-pod -n production -- curl http://backend-service:8080

# 3. 使用 netshoot 容器调试
kubectl run netshoot --rm -it --image=nicolaka/netshoot -n production -- bash
# 在容器内测试
curl http://backend-service:8080
nc -zv backend-service 8080

# 4. 使用 Cilium Hubble 可视化
hubble observe --from-pod frontend-pod --to-pod backend-pod

# 5. 使用 tcpdump 抓包
kubectl exec -it frontend-pod -n production -- tcpdump -i any port 8080
```
## 8.3 性能优化

1. **选择高性能 CNI**:
   - Cilium (eBPF 原生,推荐)
   - Calico eBPF 模式
   - 避免 iptables 模式 (大规模集群)

2. **减少策略数量**:
   - 合并相似的策略
   - 使用通配符标签选择器

3. **监控 CNI 性能**:
   ```bash
   # Calico: 查看 iptables 规则数量
   iptables-save | wc -l
   
   # Cilium: 查看 eBPF 程序性能
   cilium bpf metrics list
   ```

---

<!-- chunk: 9. 参考资料 -->## 9. 参考资料

- [Kubernetes NetworkPolicy 官方文档](https://kubernetes.io/docs/concepts/services-networking/network-policies/)
- [NetworkPolicy API Reference](https://kubernetes.io/docs/reference/kubernetes-api/policy-resources/network-policy-v1/)
- [Calico NetworkPolicy](https://docs.tigera.io/calico/latest/network-policy/)
- [Cilium NetworkPolicy](https://docs.cilium.io/en/stable/security/policy/)
- [Network Policy Editor](https://editor.networkpolicy.io/) - 可视化编辑器

---

**文档版本**: v1.0  
**最后更新**: 2026-02  
**维护者**: Kubernetes 中文社区  
**适用版本**: Kubernetes v1.25 - v1.32

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-32-yaml-manifests MOC
- [[03-清单模式/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- Domain-32 YAML 清单 — 开源项目索引
- 01 - YAML 语法基础与 Kubernetes 资源通用规范
- 02 - Namespace / ResourceQuota / LimitRange YAML 配置参考
- 03 - Pod 完整规格说明书
- 04 - Deployment / ReplicaSet YAML 配置参考
- 05 - StatefulSet YAML 配置参考
- 06 - DaemonSet YAML 配置参考
- 07 - Job / CronJob YAML 配置参考
- 08 - Service 全类型 YAML 配置参考
- 09 - Endpoints / EndpointSlice YAML 配置参考

## See Also

- 20-rbac-role-rolebinding
- 21-rbac-clusterrole-clusterrolebinding
- 23-pod-security-standards
- 24-admission-webhook-configuration

- [[10-平台工程/06-代码分析/cluster-create/19-cni-networking.md|19-cni-networking]]

## Related

- [[reference|#reference Hub]] — tag hub

- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
