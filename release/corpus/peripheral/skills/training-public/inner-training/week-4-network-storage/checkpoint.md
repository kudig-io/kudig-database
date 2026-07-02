---
title: 'Week 4 自测: 网络与存储'
description: '# Week 4 自测: 网络与存储'
summary: 'Week 4 是整个培训的收官阶段，涵盖了 [[Kubernetes|Kubernetes]] 网络和存储两大核心基础设施主题。网络和存储是支撑业务应用运行的关键基础能力，理解 Service/Ingress 的路由机制、CNI 插件的工作原理、以及 PV/PVC 的生命周期管理，是运维工程师独立排障和架构设计的基础。'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- statefulset
- ingress
- networkpolicy
- rag
- cilium
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 4 自测: 网络与存储 是什么'
- '如何 Week 4 自测: 网络与存储'
trigger_keywords:
- Week
- '自测:'
- 网络与存储
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- ebpf-basics
- cilium-basics
- cni-basics
- tls-basics
---



# Week 4 自测: 网络与存储

```yaml
---
title: Week 4 自测: 网络与存储
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes网络存储自测"
  - "Week4测试题"
  - "Service Ingress自测"
  - "PV PVC测试"
trigger_keywords:
  - "自测"
  - "Week4"
  - "网络"
  - "存储"
  - "Service"
  - "Ingress"
  - "PV"
  - "PVC"
  - "Terway"
  - "Flannel"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 60min
related_domains:
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
id: WEEK4-CHECKPOINT
topic: training
type: checkpoint
tags: [week-4, checkpoint, self-test, networking, storage, k8s, k8s-1.28-1.33]
---
```

> **满分**: 50 分 | **建议用时**: 60 分钟

---

## 概述

Week 4 是整个培训的收官阶段，涵盖了 [[Kubernetes|Kubernetes]] 网络和存储两大核心基础设施主题。网络和存储是支撑业务应用运行的关键基础能力，理解 Service/Ingress 的路由机制、CNI 插件的工作原理、以及 PV/PVC 的生命周期管理，是运维工程师独立排障和架构设计的基础。

本自测检验你对网络和存储两个领域的掌握程度，包含概念理解、命令实操和场景分析三个部分。请独立完成，不查阅参考资料。

**自测目标**：
- 检验 Service/Ingress/CNI 网络概念的掌握程度
- 验证 PV/PVC/StorageClass 存储管理的实操能力
- 评估网络和存储故障排查的综合能力

---

## 一、概念理解 (5 题, 每题 2 分, 共 10 分)

### 1. ClusterIP、NodePort、LoadBalancer 三种 Service 类型的区别是什么？在 ACK 中 LoadBalancer 类型会自动创建什么资源？

> 你的回答:

**参考答案**:

| Service 类型 | 访问方式 | 端口范围 | ACK 自动创建 | 适用场景 |
|-------------|---------|---------|-------------|---------|
| **ClusterIP** | 集群内部访问 | 虚拟 IP | 无 | 微服务间通信 |
| **NodePort** | 通过节点 IP:Port 访问 | 30000-32767 | 无 | 测试/临时暴露 |
| **LoadBalancer** | 通过外部 LB 访问 | LB 公网/内网 IP | SLB (Server Load Balancer) | 生产环境对外暴露 |

在 ACK 中，创建 `type: LoadBalancer` 的 Service 会自动创建一个阿里云 SLB 实例。可以通过 annotation 控制创建内网还是公网 SLB：

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: intranet
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: slb.s2.medium
spec:
  type: LoadBalancer
  selector:
    app: my-app
  ports:
  - port: 80
    targetPort: 8080
```

---

### 2. Ingress 和 Service LoadBalancer 在功能上有什么区别？什么情况下应该使用 Ingress？

> 你的回答:

**参考答案**:

| 维度 | Service LoadBalancer | Ingress |
|------|---------------------|---------|
| **L4/L7** | L4（TCP/UDP） | L7（HTTP/HTTPS） |
| **域名路由** | 不支持 | 支持（基于 Host/Path） |
| **TLS 终止** | 不支持 | 支持 |
| **LB 数量** | 每个 Service 一个 SLB | 多个 Service 共享一个 SLB |
| **成本** | 高（多个 SLB 实例） | 低（一个 Ingress Controller） |

**使用 Ingress 的场景**：
- 多个 HTTP 服务需要通过同一个 IP 暴露
- 需要基于域名或路径的路由
- 需要 TLS 证书管理
- 需要灰度发布/金丝雀发布

---

### 3. Terway ENIIP 模式和 Flannel VxLAN 模式的核心区别是什么？各自的优缺点？

> 你的回答:

**参考答案**:

| 维度 | Terway (ENIIP) | Flannel (VxLAN) |
|------|----------------|-----------------|
| **Pod IP 来源** | VPC 真实 IP (ENI 辅助 IP) | 虚拟 CIDR (overlay) |
| **网络性能** | 高（直接 VPC 网络） | 中（VxLAN 封装开销） |
| **Pod 与 VPC 互通** | 原生互通 | 需要额外配置 |
| **IP 地址管理** | 依赖 VPC vSwitch | 独立 Pod CIDR |
| **网络策略** | 原生支持 | 需要额外组件 |
| **适用场景** | 生产环境推荐 | 开发测试、简单场景 |

---

### 4. PV 的 Retain 和 Delete 回收策略有什么区别？生产环境推荐哪种？为什么？

> 你的回答:

**参考答案**:

| 回收策略 | 行为 | 数据安全 | 适用场景 |
|----------|------|---------|---------|
| **Retain** | PVC 删除后 PV 保留，数据不删除 | 高 | 生产环境 |
| **Delete** | PVC 删除后 PV 和云盘自动删除 | 低 | 临时数据、测试环境 |
| **Recycle** (已废弃) | 执行 rm -rf 后重新可用 | 低 | 不推荐 |

**生产环境推荐 Retain**，原因：
1. 防止误删 PVC 导致数据永久丢失
2. 保留数据用于审计和恢复
3. Retain 的 PV 可以手动回收和重新绑定

---

### 5. [[StatefulSet|StatefulSet]] 的 volumeClaimTemplates 和 Deployment 使用 PVC 有什么区别？

> 你的回答:

**参考答案**:

| 维度 | StatefulSet (volumeClaimTemplates) | Deployment (PVC) |
|------|-----------------------------------|-----------------|
| **PVC 创建** | 每个 Pod 自动创建独立 PVC | 手动创建，所有 Pod 共享 |
| **PVC 命名** | 固定命名: `<pvc-name>-<pod-name>-<ordinal>` | 自定义命名 |
| **数据隔离** | 每个 Pod 独立存储 | 共享存储 |
| **Pod 重建** | 重新绑定同一个 PVC | 可能绑定不同 PVC |
| **适用场景** | 数据库、ZooKeeper 等有状态应用 | Web 服务等无状态应用 |

---

## 二、命令实操 (5 题, 每题 2 分, 共 10 分)

### 1. 写出创建 LoadBalancer Service 并指定为内网 SLB 的 YAML:

**参考答案**:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: internal-lb-service
  namespace: default
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: intranet
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: slb.s2.medium
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: paybytraffic
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app: web-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: https
    port: 443
    targetPort: 8443
    protocol: TCP
```

**Annotation 参数说明**：

| Annotation | 说明 | 可选值 |
|------------|------|--------|
| `address-type` | 内网/公网 | intranet, internet |
| `spec` | SLB 规格 | slb.s1.small ~ slb.s3.2xlarge |
| `charge-type` | 计费方式 | paybytraffic, paybybandwidth |
| `vswitch-id` | 指定 vSwitch | vsw-xxx |

---

### 2. 写出创建基于域名路由的 Ingress 规则 YAML:

**参考答案**:

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-app-ingress
  namespace: default
  annotations:
    kubernetes.io/ingress.class: nginx
    [[cert-manager|cert-manager]].io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
spec:
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls-secret
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

---

### 3. 写出查看节点 Pod CIDR 分配的命令:

**参考答案**:

```bash
# 方法1: 查看节点的 PodCIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# 预期输出:
# node-worker-1    10.0.1.0/24
# node-worker-2    10.0.2.0/24

# 方法2: 查看节点详情中的 PodCIDR
kubectl get nodes -o custom-columns='NAME:.metadata.name,POD_CIDR:.spec.podCIDR'

# 预期输出:
# NAME            POD_CIDR
# node-worker-1   10.0.1.0/24
# node-worker-2   10.0.2.0/24

# 方法3: 在 Terway 集群中查看 vSwitch 分配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations["flannel\.alpha\.coreos\.com/public-ip"]}{"\n"}{end}'
```

---

### 4. 写出动态创建 20Gi 云盘 PVC 的 YAML:

**参考答案**:

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
  namespace: default
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 20Gi
```

**验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -f pvc.yaml
kubectl get pvc data-pvc

# 预期输出:
# NAME       STATUS   VOLUME                                     CAPACITY   ACCESS MODES   STORAGECLASS        AGE
# data-pvc   Bound    pvc-abc123-def456-ghi789                   20Gi       RWO            alicloud-disk-ssd   10s
```

---

### 5. 写出扩容 PVC 到 40Gi 的命令:

**参考答案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# Step 1: 检查 StorageClass 是否允许扩容
kubectl get storageclass alicloud-disk-ssd -o yaml | grep allowVolumeExpansion

# 预期输出:
# allowVolumeExpansion: true

# 如果为 false，需要先修改 StorageClass:
kubectl patch storageclass alicloud-disk-ssd -p '{"allowVolumeExpansion":true}'

# Step 2: 扩容 PVC
kubectl patch pvc data-pvc -p '{"spec":{"resources":{"requests":{"storage":"40Gi"}}}}'

# 预期输出:
# persistentvolumeclaim/data-pvc patched

# Step 3: 查看扩容状态
kubectl get pvc data-pvc

# 预期输出:
# NAME       STATUS   VOLUME                   CAPACITY   ACCESS MODES   STORAGECLASS        AGE
# data-pvc   Bound    pvc-abc123-def456-ghi    40Gi       RWO            alicloud-disk-ssd   5m

# Step 4: 验证 Pod 内文件系统已扩展
kubectl exec <pod-name> -- df -h /data
```

---

## 三、场景分析 (4 题, 每题 5 分, 共 20 分)

### 场景 1: Service 无法访问

**现象**: 创建了 ClusterIP Service，但从其他 Pod 访问时连接超时。

**参考答案 - 完整排查流程**:

```bash
# Step 1: 检查 Service selector 是否匹配 Pod 标签
kubectl describe svc <service-name> -n <ns> | grep Selector
kubectl get pods -n <ns> --show-labels
# 对比两边的标签是否一致

# Step 2: 检查 Endpoints 是否有后端 Pod
kubectl get endpoints <service-name> -n <ns>

# 预期输出 (正常):
# NAME          ENDPOINTS                           AGE
# my-service    10.0.1.100:8080,10.0.1.101:8080     5d

# 预期输出 (异常):
# NAME          ENDPOINTS       AGE
# my-service    <none>          5d

# Step 3: 检查 Pod 是否 Running 且 readinessProbe 通过
kubectl get pods -n <ns> -l <selector>
# READY 列应该显示 1/1 或 n/n

# Step 4: 检查 kube-proxy 是否正常
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50

# Step 5: 检查 NetworkPolicy 是否阻止了流量
kubectl get networkpolicy -n <ns>

# Step 6: 测试 Service 连通性
kubectl run test --image=busybox:1.36 --rm -it --restart=Never -- wget -qO- http://<service-name>:<port>
```

---

### 场景 2: Ingress 路由不生效

**参考答案 - 完整排查流程**:

```bash
# Step 1: 确认 Ingress Controller 正常运行
kubectl get pods -n ingress-nginx
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50

# Step 2: 检查 IngressClass
kubectl get ingressclass
kubectl get ingress <ingress-name> -n <ns> -o yaml | grep ingressClassName

# Step 3: 检查 Host 和 Path 配置
kubectl describe ingress <ingress-name> -n <ns>
# 确认 Rules 中的 Host 和 Path 与请求匹配

# Step 4: 检查后端 Service 是否正常
kubectl get svc <backend-service> -n <ns>
kubectl get endpoints <backend-service> -n <ns>

# Step 5: 查看 Ingress Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=100 | grep <host>

# Step 6: 测试路由
curl -H "Host: app.example.com" http://<ingress-ip>/api
```

---

### 场景 3: Pod IP 分配失败 (Terway)

**参考答案 - 完整排查流程**:

```bash
# Step 1: 查看 Pod Events
kubectl describe pod <pod-name> -n <ns>
# 关注 Events 中的错误信息

# Step 2: 检查 Pod vSwitch 可用 IP 数量
aliyun vpc DescribeVSwitchAttributes --VSwitchId <vswitch-id> | jq '.AvailableIpAddressCount'
# 如果为 0 或很少，需要扩展 CIDR 或添加新 vSwitch

# Step 3: 检查节点 ENI 配额
aliyun ecs DescribeInstanceTypes --InstanceTypeFamily <family> | jq '.InstanceTypes[0].EniQuantity'
# 节点的 ENI 数量有上限

# Step 4: 查看 Terway Pod 日志
kubectl logs -n kube-system -l app=terway --tail=100 | grep -i "error|fail"

# Step 5: 检查安全组规则
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId <sg-id>

# Step 6: 解决方案
# 方案A: 扩展 Pod vSwitch CIDR
# 方案B: 在节点池中添加新的 vSwitch
# 方案C: 使用 Terway 的 vPC 模式减少 ENI 消耗
```

---

### 场景 4: PVC 一直处于 Pending

**参考答案 - 完整排查流程**:

```bash
# Step 1: 查看 PVC Events
kubectl describe pvc <pvc-name> -n <ns>

# 预期输出 (常见错误):
# Events:
#   Warning  ProvisioningFailed  storageclass.storage.k8s.io "alicloud-disk-ssd" not found
#   Warning  ProvisioningFailed  no volume plugin matched

# Step 2: 检查 StorageClass 是否存在
kubectl get storageclass
kubectl describe storageclass alicloud-disk-ssd

# Step 3: 检查云盘配额
aliyun ecs DescribeAvailableResource --RegionId cn-hangzhou --DestinationResource=DataDisk

# Step 4: 检查可用区限制
# PVC 可能因可用区不匹配而无法绑定
kubectl get nodes -L topology.kubernetes.io/zone

# Step 5: 检查 CSI 插件状态
kubectl get pods -n kube-system -l app=csi-plugin
kubectl logs -n kube-system -l app=csi-plugin --tail=50

# Step 6: 检查 accessModes 兼容性
# 云盘只支持 ReadWriteOnce (RWO)
# 如果 PVC 请求 ReadWriteMany (RWX)，会永远 Pending
kubectl get pvc <pvc-name> -o jsonpath='{.spec.accessModes}'
```

---

## 四、评分统计

| 部分 | 满分 | 得分 |
|------|------|------|
| 概念理解 | 10 | |
| 命令实操 | 10 | |
| 场景分析 | 20 | |
| **自评加分** | 10 | |
| **合计** | **50** | |

**自评加分标准** (最高 10 分):
- 本周每日教案按时完成 +2
- 完成了 Service + Ingress 实操 +2
- 实践了 Terway 或 Flannel 排障 +3
- 完成了 PV/PVC 全流程操作 +3

**评估标准**：
- **45-50 分**: 优秀，具备网络和存储的独立运维能力
- **35-44 分**: 良好，能处理常见网络和存储问题
- **25-34 分**: 及格，需要加强实践操作
- **< 25 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

| 薄弱点 | 对应 Day | 补强计划 |
|--------|---------|---------|
| | | |
| | | |
| | | |

---

## 要点总结

- **三种 Service 类型**: ClusterIP（内部）→ NodePort（节点暴露）→ LoadBalancer（SLB）
- **Ingress** 是 L7 路由，支持域名/路径路由、TLS 终止，多 Service 共享一个 LB
- **Terway** 使用 VPC 真实 IP，性能优于 Flannel 的 VxLAN overlay
- **PV 回收策略**: 生产用 Retain，测试用 Delete
- **PVC 排查**: `describe pvc` 看 Events → StorageClass → CSI 插件 → 可用区
- **Service 排查**: selector 匹配 → Endpoints → readinessProbe → kube-proxy → NetworkPolicy

---

## 培训完成建议

恭喜完成 4 周培训！接下来建议:

1. **完成毕业项目**: P5: 毕业综合项目](../projects/p5-graduation-project.md)
2. **定期回顾**: 利用 [知识图谱](../resources/knowledge-map.md) 进行周期性复习
3. **持续实践**: 在实际工作中运用所学知识
4. **社区交流**: 参与团队知识分享，教是最好的学

---

## 延伸阅读

- [Kubernetes Service 文档](https://kubernetes.io/docs/concepts/services-networking/service/)
- [Ingress 文档](https://kubernetes.io/docs/concepts/services-networking/ingress/)
- [持久化存储](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)
- [ACK 网络方案](https://help.aliyun.com/document_detail/187464.html)

---

## 自测题 (Self-Check)

### Q1. Kubernetes Service 的 ClusterIP 是如何实现的?

<details>
<summary>查看答案</summary>

kube-proxy 通过 iptables 或 IPVS 规则将 ClusterIP:Port 的流量 DNAT 到后端 Pod 的 PodIP:TargetPort。

</details>

### Q2. Ingress 和 Gateway API 的区别?

<details>
<summary>查看答案</summary>

Ingress 仅支持 HTTP/HTTPS, 功能有限 (需注解扩展); Gateway API 支持 HTTP/gRPC/TCP/TLS/UDP, 原生流量分割, 角色分离 (GatewayClass→Gateway→Route)。

</details>

### Q3. StatefulSet 的 Pod 为什么有稳定的网络标识?

<details>
<summary>查看答案</summary>

StatefulSet 创建的 Pod 名称格式为 <statefulset-name>-<ordinal>, 配合 Headless Service 创建 DNS 记录 <pod-name>.<service-name>.<namespace>.svc.cluster.local。

</details>

### Q4. 如何选择 CNI 插件?

<details>
<summary>查看答案</summary>

Calico (通用, 支持 BGP/VXLAN, NetworkPolicy)、Cilium (eBPF, 高性能, 丰富 NetworkPolicy)、Flannel (简单, 仅 VXLAN, 无 NetworkPolicy)。生产推荐 Cilium 或 Calico。

</details>

### Q5. PVC 的三种访问模式?

<details>
<summary>查看答案</summary>

ReadWriteOnce (单节点读写)、ReadOnlyMany (多节点只读)、ReadWriteMany (多节点读写)。并非所有存储后端都支持全部模式。

</details>


## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
