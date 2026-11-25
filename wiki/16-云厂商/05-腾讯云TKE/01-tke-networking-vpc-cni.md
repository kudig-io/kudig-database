---
title: TKE 网络与 VPC-CNI 深度解析
description: 'VPC-CNI 模式、GlobalRouter 模式、独立网卡方案、CLB/ALB 集成、NAT 网关配置全面指南'
summary: 'VPC-CNI 模式、GlobalRouter 模式、独立网卡方案、CLB/ALB 集成、NAT 网关配置全面指南'
category: cloud-providers
tags:
- cloud
- k8s
- tke
- tencent
- networking
- vpc-cni
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- TKE VPC-CNI 模式是什么
- 如何配置 TKE 网络
- GlobalRouter 与 VPC-CNI 区别
trigger_keywords:
- VPC-CNI
- GlobalRouter
- CLB
- ALB
- NAT 网关
- 独立网卡
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# TKE 网络与 VPC-CNI 深度解析

## 1. TKE 网络模式概览

| 模式 | Pod IP 来源 | 性能 | IP 限制 | 适用场景 |
|------|------------|------|---------|---------|
| **VPC-CNI** | VPC 子网 IP | 最佳 | 受子网限制 | 性能敏感、需要 Pod 被 VPC 直接访问 |
| **GlobalRouter** | 独立 CIDR | 良 | 无限制 | 大规模集群、IP 资源有限 |
| **独立网卡** | 弹性网卡 | 最佳 | 受子网限制 | 高性能、需独立网卡隔离 |

## 2. VPC-CNI 模式

### 2.1 原理

VPC-CNI 模式下，Pod 直接分配 VPC 子网 IP，无需 NAT。每个节点绑定多个弹性网卡（ENI），Pod IP 来自 ENI 的辅助 IP。

```
VPC-CNI 数据路径：

Pod (10.0.1.15) → veth → ENI (10.0.1.x) → VPC 路由 → 目标
                              ↑
                    节点主网卡 + 辅助网卡
                    每个 ENI 可分配 N 个辅助 IP
```

### 2.2 创建集群（VPC-CNI）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 Terraform
resource "tencentcloud_kubernetes_cluster" "vpc_cni" {
  cluster_name = "tke-prod-vpc-cni"
  cluster_desc = "Production TKE with VPC-CNI"
  cluster_type = "MANAGED_CLUSTER"
  vpc_id       = tencentcloud_vpc.main.id
  service_cidr = "172.16.0.0/18"
  cluster_internet = false
  cluster_intranet = true
  cluster_intranet_subnet_id = tencentcloud_subnet.mgmt.id

  # 网络模式：VPC-CNI
  network_type = "VPC-CNI"

  # 节点配置
  worker_config {
    count               = 3
    availability_zone   = "ap-guangzhou-3"
    instance_type       = "S5.2XLARGE16"
    subnet_id           = tencentcloud_subnet.node.id
    system_disk_type    = "CLOUD_SSD"
    system_disk_size    = 100
    security_group_ids  = [tencentcloud_security_group.k8s.id]
  }

  # Pod 网络配置
  eni_subnet_ids = [
    tencentcloud_subnet.pod-a.id,
    tencentcloud_subnet.pod-b.id,
    tencentcloud_subnet.pod-c.id
  ]
}
```
### 2.3 IP 规划

```
# 🟢 低风险：只读/信息收集，通常无副作用
VPC 地址空间规划（VPC: 10.0.0.0/16）：

节点子网：
  node-subnet-a: 10.0.0.0/20   → 4,094 节点 IP（AZ-a）
  node-subnet-b: 10.0.16.0/20  → 4,094 节点 IP（AZ-b）
  node-subnet-c: 10.0.32.0/20  → 4,094 节点 IP（AZ-c）

Pod 子网（ENI 辅助 IP）：
  pod-subnet-a:  10.0.64.0/18  → 16,382 Pod IP（AZ-a）
  pod-subnet-b:  10.0.128.0/18 → 16,382 Pod IP（AZ-b）
  pod-subnet-c:  10.0.192.0/18 → 16,382 Pod IP（AZ-c）

Service CIDR：
  172.16.0.0/18  → 16,382 Service IP

IP 消耗计算：
  每节点 max-pods = 64
  节点数量 = 100
  总 Pod IP = 6,400
  预留 50% 缓冲 → /18 子网足够
```
### 2.4 Pod 子网管理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 ENI 分配情况
kubectl get nodes -o custom-columns=NAME:.metadata.name,INTERNAL-IP:.status.addresses[0].address

# 查看 Pod IP 分配
kubectl get pods -A -o wide

# 扩展 Pod 子网
# 在控制台或 API 中为集群添加新的 ENI 子网
# TKE 会自动将新子网用于后续 Pod 分配
```
## 3. GlobalRouter 模式

### 3.1 原理

GlobalRouter 使用独立的 Pod CIDR（与 VPC 不重叠），通过节点上的路由实现 Pod 间通信。Pod IP 不消耗 VPC 子网 IP。

```
GlobalRouter 数据路径：

Pod (172.20.1.5) → veth → 节点路由表 → Node IP → VPC 路由 → 目标节点 → Pod
                                    ↑
                          通过 iptables/IPVS NAT
```

### 3.2 创建集群（GlobalRouter）

```bash
resource "tencentcloud_kubernetes_cluster" "global_router" {
  cluster_name = "tke-prod-gr"
  cluster_type = "MANAGED_CLUSTER"
  vpc_id       = tencentcloud_vpc.main.id
  service_cidr = "172.16.0.0/18"
  cluster_internet = false

  # 网络模式：GlobalRouter
  network_type = "GR"

  # Pod CIDR（独立于 VPC）
  cluster_pod_cidr     = "172.20.0.0/16"
  cluster_service_cidr = "172.16.0.0/18"

  worker_config {
    count             = 3
    availability_zone = "ap-guangzhou-3"
    instance_type     = "S5.2XLARGE16"
    subnet_id         = tencentcloud_subnet.node.id
  }
}
```

### 3.3 VPC-CNI vs GlobalRouter 选择

```
决策树：

Pod IP 需要被 VPC 内其他服务直接访问？
  ├── 是 → VPC-CNI
  │         Pod IP 直接是 VPC IP
  │         数据库、Redis 等可直接通过 Pod IP 访问
  │
  └── 否 → 节点规模 > 500 或 Pod > 10,000？
            ├── 是 → GlobalRouter
            │         独立 CIDR，无子网 IP 限制
            │
            └── 否 → 均可
                      VPC-CNI 性能更优
                      GlobalRouter IP 管理更简单
```

## 4. 独立网卡方案

### 4.1 概述

独立网卡方案为特定 Pod 分配独立的弹性网卡，实现网络隔离和更高性能。

```yaml
# 使用独立网卡的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: high-perf-app
  annotations:
    # 指定使用独立网卡
    k8s.v1.cni.cncf.io/networks: |
      [{
        "name": "tke-eni",
        "interface": "eth1",
        "ips": ["10.0.64.100"]
      }]
spec:
  containers:
  - name: app
    image: high-perf:v1
    resources:
      requests:
        cpu: "4"
        memory: 8Gi
```

### 4.2 多网卡配置

```yaml
# NetworkAttachmentDefinition
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: tke-eni
  namespace: production
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "type": "tke-eni",
      "eniName": "eth1",
      "subnetId": "subnet-xxxxxxxx",
      "securityGroupIds": ["sg-xxxxxxxx"]
    }
```

## 5. CLB 集成

### 5.1 Service 使用 CLB

```yaml
apiVersion: v1
kind: Service
metadata:
  name: api-service
  namespace: production
  annotations:
    # 指定已有 CLB
    service.kubernetes.io/tke-existed-lbid: "lb-xxxxxxxx"
    # 监听器配置
    service.kubernetes.io/tke-service-config: |
      {"listeners": [{"protocol": "TCP", "port": 80, "targetPort": 8080}]}
spec:
  type: LoadBalancer
  # 不指定则自动创建
  # loadBalancerIP: 1.2.3.4
  ports:
  - port: 80
    targetPort: 8080
    protocol: TCP
  selector:
    app: api

---
# 使用已有 CLB（共享模式）
apiVersion: v1
kind: Service
metadata:
  name: api-v2
  annotations:
    # 多个 Service 共享同一个 CLB
    service.kubernetes.io/tke-existed-lbid: "lb-xxxxxxxx"
spec:
  type: LoadBalancer
  ports:
  - port: 8080
    targetPort: 8080
```

### 5.2 CLB 健康检查配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: api-service
  annotations:
    # 健康检查配置
    service.kubernetes.io/qcloud-loadbalancer-health-check-flag: "on"
    service.kubernetes.io/qcloud-loadbalancer-health-check-interval: "5"
    service.kubernetes.io/qcloud-loadbalancer-health-check-timeout: "2"
    service.kubernetes.io/qcloud-loadbalancer-health-check-health-num: "3"
    service.kubernetes.io/qcloud-loadbalancer-health-check-unhealth-num: "3"
    service.kubernetes.io/qcloud-loadbalancer-health-check-port: "8080"
    service.kubernetes.io/qcloud-loadbalancer-http-check-domain: "health.example.com"
    service.kubernetes.io/qcloud-loadbalancer-http-check-path: "/healthz"
    service.kubernetes.io/qcloud-loadbalancer-http-check-method: "GET"
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
```

## 6. ALB（Application Load Balancer）集成

```yaml
# ALB Ingress 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  annotations:
    # 使用 TKE ALB
    kubernetes.io/ingress.class: "alb"
    alb.ingress.kubernetes.io/listener-protocol: "HTTPS"
    alb.ingress.kubernetes.io/certificate-id: "cert-xxxxxxxx"
    alb.ingress.kubernetes.io/healthcheck-path: "/healthz"
    alb.ingress.kubernetes.io/load-balancer-id: "alb-xxxxxxxx"
    # 限流配置
    alb.ingress.kubernetes.io/limit: "1000"
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 80
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2-service
            port:
              number: 80
```

## 7. NAT 网关配置

### 7.1 集群出站 NAT

```hcl
# 创建 NAT 网关
resource "tencentcloud_nat_gateway" "tke" {
  name           = "nat-tke-prod"
  vpc_id         = tencentcloud_vpc.main.id
  bandwidth      = 1000
  max_concurrent = 10000000
  assigned_eip_set = [
    tencentcloud_eip.nat_1.public_ip,
    tencentcloud_eip.nat_2.public_ip
  ]
}

# 路由规则：节点子网通过 NAT 出站
resource "tencentcloud_route_table" "node" {
  name   = "rtb-node-prod"
  vpc_id = tencentcloud_vpc.main.id
}

resource "tencentcloud_route_table_entry" "nat" {
  route_table_id         = tencentcloud_route_table.node.id
  destination_cidr_block = "0.0.0.0/0"
  next_type              = "NAT"
  next_hub               = tencentcloud_nat_gateway.tke.id
}

# 关联子网
resource "tencentcloud_subnet_route_table_attachment" "node_a" {
  subnet_id      = tencentcloud_subnet.node_a.id
  route_table_id = tencentcloud_route_table.node.id
}
```

### 7.2 Pod 出站 SNAT

```yaml
# 使用 iptables 为 Pod 流量设置 SNAT
# 通常 TKE 自动管理，但可手动调整

# 查看 SNAT 规则
iptables -t nat -L POSTROUTING -n -v

# 自定义 SNAT（需在节点上操作）
# 通常不建议手动修改，使用 NAT 网关即可
```

## 8. DNS 配置

```yaml
# 自定义 CoreDNS
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-custom
  namespace: kube-system
data:
  corporate.server: |
    corporate.internal:53 {
        forward . 10.0.0.53 10.0.0.54
    }
  tencent.server: |
    cns.tencentyun.com:53 {
        forward . 169.254.0.3
    }
```

## 9. 网络策略

```yaml
# TKE 支持 Calico NetworkPolicy
apiVersion: projectcalico.org/v3
kind: NetworkPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  selector: all()
  types:
  - Ingress
  - Egress
  egress:
  - action: Allow
    destination:
      selector: k8s-app == "kube-dns"
    protocol: UDP
    destination:
      ports:
      - 53
```

## 10. 故障排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 CNI 插件状态
ls /opt/cni/bin/
cat /etc/cni/net.d/*.conflist

# 检查 Pod 网络连通性
kubectl exec -it debug-pod -- ping <pod-ip>
kubectl exec -it debug-pod -- nslookup kubernetes.default

# 检查 ENI 绑定情况（SSH 到节点）
ip link show | grep eth
ip addr show eth1

# CLB 连通性测试
curl -v http://<CLB-IP>:<port>/healthz

# 检查路由表
ip route show table all

# TKE 网络插件日志
kubectl logs -n kube-system -l k8s-app=tke-eni --tail=100
```
## Related

- [[02-tke-storage-cbs-cfs|TKE 存储：CBS/CFS/TurboFS]]
- [[04-tke-troubleshooting-playbook|TKE 故障排查手册]]

## See Also

- TKE 网络模式官方文档
- VPC-CNI 最佳实践


<!-- risk-assessed -->
