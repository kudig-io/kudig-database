# 143 - Terway 高级指南 (Terway Advanced Guide)

> **适用版本**: 阿里云 ACK v1.25 - v1.32 | **Terway 版本**: v1.5+ | **最后更新**: 2026-01

---

## 1. Terway 架构概览

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      阿里云 VPC 网络                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                    VPC (172.16.0.0/12)                          │   │
│   │  ┌───────────────────────────────────────────────────────────┐  │   │
│   │  │            交换机 (vSwitch) 172.16.1.0/24                  │  │   │
│   │  │                                                           │  │   │
│   │  │   ┌─────────────────────────────────────────────────┐    │  │   │
│   │  │   │              ECS Node 1                          │    │  │   │
│   │  │   │                                                  │    │  │   │
│   │  │   │   ┌─────────┐  ┌─────────┐  ┌─────────┐        │    │  │   │
│   │  │   │   │  Pod A  │  │  Pod B  │  │  Pod C  │        │    │  │   │
│   │  │   │   │ VPC IP  │  │ VPC IP  │  │ VPC IP  │        │    │  │   │
│   │  │   │   │172.16.  │  │172.16.  │  │172.16.  │        │    │  │   │
│   │  │   │   │ 1.10    │  │ 1.11    │  │ 1.12    │        │    │  │   │
│   │  │   │   └────┬────┘  └────┬────┘  └────┬────┘        │    │  │   │
│   │  │   │        │            │            │              │    │  │   │
│   │  │   │        └────────────┼────────────┘              │    │  │   │
│   │  │   │                     │                           │    │  │   │
│   │  │   │              ┌──────▼──────┐                    │    │  │   │
│   │  │   │              │    ENI      │  弹性网卡          │    │  │   │
│   │  │   │              │ (多IP模式)  │                    │    │  │   │
│   │  │   │              └──────┬──────┘                    │    │  │   │
│   │  │   │                     │                           │    │  │   │
│   │  │   │              ┌──────▼──────┐                    │    │  │   │
│   │  │   │              │   eth0      │  主网卡            │    │  │   │
│   │  │   │              │ 172.16.1.5  │                    │    │  │   │
│   │  │   │              └─────────────┘                    │    │  │   │
│   │  │   └──────────────────────────────────────────────────┘    │  │   │
│   │  └───────────────────────────────────────────────────────────┘  │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Terway 网络模式对比

| 模式 | Pod IP 来源 | 性能 | 容量 | 适用场景 |
|:---|:---|:---:|:---|:---|
| **VPC** | VPC 路由 | 高 | 受路由条目限制 | 小规模集群 |
| **ENI** | 独占 ENI | 最高 | 受 ENI 配额限制 | 高性能/隔离需求 |
| **ENIIP** | ENI 辅助 IP | 高 | 高密度 | **推荐**，大规模 |
| **ENIIP-Trunking** | ENI Trunk | 高 | 最高密度 | 超大规模 |

### 各模式 Pod 容量计算

| ECS 规格 | ENI 数量 | 单 ENI IP 数 | VPC 模式 | ENIIP 模式 | Trunk 模式 |
|:---|:---:|:---:|:---:|:---:|:---:|
| ecs.g7.large | 3 | 6 | 路由限制 | (3-1)*6=12 | 更高 |
| ecs.g7.xlarge | 4 | 10 | 路由限制 | (4-1)*10=30 | 更高 |
| ecs.g7.2xlarge | 4 | 15 | 路由限制 | (4-1)*15=45 | 更高 |
| ecs.g7.4xlarge | 8 | 20 | 路由限制 | (8-1)*20=140 | 更高 |

---

## 3. ENIIP 模式详解

### 3.1 ENIIP 工作原理

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         ENIIP 模式原理                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ECS Node                                                               │
│   ┌───────────────────────────────────────────────────────────────┐     │
│   │                                                               │     │
│   │   eth0 (主网卡)                                               │     │
│   │   IP: 172.16.1.5                                              │     │
│   │   用于：Node 通信、控制面                                     │     │
│   │                                                               │     │
│   │   eth1 (ENI-1)              eth2 (ENI-2)                      │     │
│   │   主 IP: 172.16.1.100       主 IP: 172.16.1.200               │     │
│   │   ┌─────────────────┐       ┌─────────────────┐               │     │
│   │   │ 辅助 IP 池:     │       │ 辅助 IP 池:     │               │     │
│   │   │ 172.16.1.101    │──┐    │ 172.16.1.201    │──┐            │     │
│   │   │ 172.16.1.102    │  │    │ 172.16.1.202    │  │            │     │
│   │   │ 172.16.1.103    │  │    │ 172.16.1.203    │  │            │     │
│   │   │ ...             │  │    │ ...             │  │            │     │
│   │   └─────────────────┘  │    └─────────────────┘  │            │     │
│   │                        │                         │            │     │
│   │   ┌────────────────────┼─────────────────────────┘            │     │
│   │   │                    │                                      │     │
│   │   ▼                    ▼                                      │     │
│   │   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐         │     │
│   │   │  Pod A  │  │  Pod B  │  │  Pod C  │  │  Pod D  │         │     │
│   │   │172.16.  │  │172.16.  │  │172.16.  │  │172.16.  │         │     │
│   │   │ 1.101   │  │ 1.102   │  │ 1.201   │  │ 1.202   │         │     │
│   │   └─────────┘  └─────────┘  └─────────┘  └─────────┘         │     │
│   │                                                               │     │
│   └───────────────────────────────────────────────────────────────┘     │
│                                                                          │
│   特点:                                                                  │
│   - Pod 直接使用 VPC IP，无封装开销                                      │
│   - 与 VPC 内其他资源直接通信                                            │
│   - 支持安全组、NetworkPolicy                                            │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 ENIIP 配置

```yaml
# ACK 集群创建时选择 Terway ENIIP 模式
# 或通过 ConfigMap 配置

apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "max_pool_size": 25,
      "min_pool_size": 10,
      "credential_path": "/var/addon/token-config",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx1"],
        "cn-hangzhou-i": ["vsw-xxx2"]
      },
      "security_groups": ["sg-xxx"],
      "service_cidr": "172.21.0.0/20"
    }
```

---

## 4. ENI 独占模式

### 4.1 使用场景

- 需要独立安全组的 Pod
- 高性能低延迟需求
- Pod 需要独立公网 EIP

### 4.2 配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: high-performance-pod
  annotations:
    k8s.aliyun.com/eni: "true"  # 使用独占 ENI
spec:
  containers:
    - name: app
      image: nginx
      resources:
        limits:
          aliyun/eni: "1"  # 请求 1 个 ENI
```

---

## 5. 固定 IP 配置 (StatefulSet)

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  annotations:
    k8s.aliyun.com/pod-ip-fixed: "true"  # 启用固定 IP
spec:
  serviceName: mysql
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          ports:
            - containerPort: 3306
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 100Gi
```

---

## 6. NetworkPolicy 支持

### 6.1 Terway + Cilium

Terway 支持两种 NetworkPolicy 实现:

| 实现 | 性能 | 功能 |
|:---|:---|:---|
| **iptables** | 中 | 基础 NetworkPolicy |
| **Cilium eBPF** | 高 | 高级策略、可观测性 |

### 6.2 启用 Cilium eBPF

```yaml
# 创建集群时选择 Terway + Cilium

# 或更新现有集群
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-config
  namespace: kube-system
data:
  terway_config: |
    {
      "network_policy": "cilium"
    }
```

### 6.3 NetworkPolicy 示例

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-allow
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              env: production
        - podSelector:
            matchLabels:
              role: frontend
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - podSelector:
            matchLabels:
              app: database
      ports:
        - protocol: TCP
          port: 3306
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
```

---

## 7. 故障排查

### 7.1 诊断命令

```bash
# 查看 Terway 组件状态
kubectl get pods -n kube-system -l app=terway

# 查看 Terway 日志
kubectl logs -n kube-system -l app=terway -c terway --tail=100

# 查看节点 ENI 信息
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}{"\n"}{end}'

# 查看 Pod IP 分配
kubectl get pods -o wide -A | grep <node-name>

# 检查 ENI 状态
aliyun ecs DescribeNetworkInterfaces --RegionId cn-hangzhou --InstanceId <instance-id>

# Terway IPAM 状态
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show
```

### 7.2 常见问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| Pod Pending (IP不足) | ENI/IP 配额耗尽 | 增加节点或检查配额 |
| Pod 网络不通 | 安全组规则 | 检查安全组配置 |
| 跨节点不通 | vSwitch 路由 | 检查 VPC 路由表 |
| ENI 创建失败 | 权限不足 | 检查 RAM 策略 |

---

## 8. 性能优化

### 8.1 eBPF 加速

```yaml
# 启用 Cilium eBPF 替代 kube-proxy
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  kube-proxy-replacement: "strict"
  enable-bpf-masquerade: "true"
  enable-bandwidth-manager: "true"
```

### 8.2 ENI 预热

```yaml
# 配置 ENI 池预热
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "max_pool_size": 30,
      "min_pool_size": 15,
      "hot_plug": true
    }
```

---

## 9. 最佳实践

| 类别 | 建议 |
|:---|:---|
| **网络模式** | 生产环境使用 ENIIP 模式 |
| **容量规划** | 根据 ECS 规格计算 Pod 容量 |
| **安全组** | Pod 级别安全组隔离关键应用 |
| **固定 IP** | StatefulSet 启用固定 IP |
| **NetworkPolicy** | 使用 Cilium eBPF 提升性能 |
| **监控** | 监控 ENI 配额使用率 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
