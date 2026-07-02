---
title: 03 - Terway 使用指南 (Usage Guide)
description: '# 03 - Terway 使用指南 (Usage Guide)'
summary: '在 ACK 控制台创建集群时，网络插件选择 **Terway** 即自动完成安装。Terway 以 [[DaemonSet|DaemonSet]] 形态运行在每个节点上，并在 kube-system 命名空间中创建对应的 ConfigMap 和 RBAC 资源。'
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- apiserver
- istio
- cilium
- flannel
- calico
- coredns
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 10min
intent_queries:
- Terway 使用指南 (Usage Guide) 是什么
- 如何 Terway 使用指南 (Usage Guide)
trigger_keywords:
- Terway
- 使用指南
- Usage
- Guide
- terway
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- iac-basics
- ebpf-basics
- cilium-basics
- cni-basics
- mysql-basics
- gpu-scheduling-basics
---



# 03 - Terway 使用指南 (Usage Guide)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

## 1. 安装与初始化

### 1.1 ACK 托管集群自动安装

在 ACK 控制台创建集群时，网络插件选择 **Terway** 即自动完成安装。Terway 以 [[DaemonSet|DaemonSet]] 形态运行在每个节点上，并在 kube-system 命名空间中创建对应的 ConfigMap 和 RBAC 资源。

验证安装状态：

```bash
kubectl get ds -n kube-system terway-eniip -o wide
kubectl get pods -n kube-system -l app=terway -o wide
kubectl get clusterrole terway -o yaml
kubectl get clusterrolebinding terway -o yaml
```

确认所有节点上的 Terway Pod 均为 Running 且 Ready：

```bash
kubectl get pods -n kube-system -l app=terway --all-namespaces
kubectl get pods -n kube-system -l app=terway -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.containerStatuses[0].ready}{"\n"}{end}'
```

### 1.2 查看当前配置

```bash
kubectl get configmap -n kube-system eni-config -o yaml
kubectl get configmap -n kube-system eni-config -o jsonpath='{.data.eni_conf}' | jq .
```

关键配置字段说明：

| 字段 | 说明 | 示例值 |
|:---|:---|:---|
| `network_type` | 网络模式 | `ENIIP` / `ENI` / `VPC` / `IPVlan` |
| `vswitches` | Pod 使用的 vSwitch，支持多可用区 | `{"cn-hangzhou-b": ["vsw-xxx"]}` |
| `security_group` | Pod 默认安全组 | `sg-2ze...` |
| `eni_cap` | 每 ENI 分配的辅助 IP 数 | `2` - `10` |
| `service_cidr` | ClusterIP [[Service|Service]] CIDR | `10.96.0.0/12` |
| `max_pool_size` | 本地 IP 池最大容量 | `25` |
| `min_pool_size` | 本地 IP 池最小保留数 | `5` |

### 1.3 查看 Terway 版本

```bash
kubectl get ds -n kube-system terway-eniip -o jsonpath='{.spec.template.spec.containers[0].image}'
```

---

## 2. 网络模式配置

### 2.1 ENIIP 模式（推荐）

ENIIP 模式通过 ENI 辅助 IP 为 Pod 分配 VPC IP，兼顾性能与密度，是 ACK 托管集群的默认推荐模式。

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "ENIIP",
      "max_pool_size": 25,
      "min_pool_size": 5,
      "credential_path": "/var/addon/token",
      "vswitches": {
        "cn-hangzhou-b": ["vsw-bp1axxxx", "vsw-bp1bxxxx"],
        "cn-hangzhou-h": ["vsw-bp2axxxx"]
      },
      "security_group": "sg-2zexxxxx",
      "service_cidr": "10.96.0.0/12",
      "eni_cap": 5
    }
  10-terway.conf: |
    {
      "name": "terway",
      "cniVersion": "0.4.0",
      "type": "terway"
    }
```

关键参数详解：

| 参数 | 默认值 | 说明 |
|:---|:---:|:---|
| `max_pool_size` | 25 | 单节点 IP 池最大容量，按节点 Pod 密度调整 |
| `min_pool_size` | 5 | 单节点 IP 池最小保留数，影响冷启动速度 |
| `credential_path` | `/var/addon/token` | RAM 角色凭证挂载路径，托管集群自动管理 |
| `vswitches` | - | 多可用区 vSwitch 映射，key 为可用区 ID |
| `security_group` | - | Pod 默认安全组，ENIIP 模式下所有 Pod 共享 |
| `service_cidr` | - | ClusterIP Service 网段，用于 kube-proxy 规则 |
| `eni_cap` | 取决于实例规格 | 单 ENI 分配的辅助 IP 数量上限 |

### 2.2 IPVlan 模式

IPVlan 模式使用内核 IPVlan L2 接口替代 veth pair，实现极致网络性能。

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "IPVlan",
      "vswitches": {
        "cn-hangzhou-b": ["vsw-bp1axxxx"]
      },
      "security_group": "sg-2zexxxxx",
      "ip_type": "ipvlan"
    }
```

前置要求：

| 要求 | 说明 |
|:---|:---|
| 内核版本 | Linux 4.19+ （推荐 5.10+） |
| Terway 版本 | v1.3+ |
| ENI 多 IP | 已开启 ENI 辅助 IP 分配 |
| IPVlan 内核模块 | `lsmod | grep ipvlan` 确认已加载 |

```bash
modprobe ipvlan
lsmod | grep ipvlan
```

### 2.3 VPC 模式

VPC 路由模式通过 VPC 路由表将 Pod CIDR 指向节点 IP，类似 Flannel host-gw。

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "VPC",
      "vswitches": {
        "cn-hangzhou-b": ["vsw-bp1axxxx"]
      },
      "security_group": "sg-2zexxxxx"
    }
```

适用场景：小规模集群、需要与存量网络方案兼容的场景。受 VPC 路由条目配额限制（默认 48 条），不建议大规模使用。

### 2.4 模式切换注意事项

| 风险项 | 说明 |
|:---|:---|
| 切换需重建所有 Pod | 网络模式变更后，存量 Pod 网络配置不兼容，必须删除重建 |
| IP 地址变化 | 切换后 Pod IP 重新分配，固定 IP 配置需重新检查 |
| 建议维护窗口操作 | 生产环境切换应在维护窗口内进行 |
| ENIIP -> IPVlan | 需确认内核版本和 IPVlan 模块可用 |
| ENIIP -> ENI | 节点 Pod 密度大幅降低，需评估容量 |

切换操作步骤：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl edit configmap eni-config -n kube-system
kubectl rollout restart ds terway-eniip -n kube-system
kubectl rollout restart ds kube-proxy -n kube-system
kubectl delete pods -A --all  # ⚠️ 批量删除，波及面大
```

### 2.5 ENIIP-Trunking 模式配置

ENIIP-Trunking 模式通过 Trunk ENI 的 VLAN 子接口复用 ENI，单节点 Pod 密度可达 **500+**，适用于超大规模集群和 Serverless 场景。

**ConfigMap 配置:**

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "ENIIP",
      "enable_eni_trunking": true,
      "vswitches": {
        "cn-hangzhou-b": ["vsw-bp1axxxx"]
      },
      "security_group": "sg-2zexxxxx",
      "max_pool_size": 50,
      "min_pool_size": 10
    }
```

**支持的实例规格族:**

| 规格族 | 代表规格 | 说明 |
|:---|:---|:---|
| 第七代 (g7, c7, r7) | ecs.g7.2xlarge+ | 原生支持 Trunk ENI |
| 第六代增强 (g6e, c6e, r6e) | ecs.g6e.xlarge+ | 增强型网卡，支持 Trunk |
| GPU 规格 (gn7i, gn6e) | ecs.gn7i-c16g1.4xlarge+ | AI/ML 场景 |

> 不支持的规格族将回退至普通 ENIIP 模式，不会报错。

**前置要求:**

| 要求 | 说明 |
|:---|:---|
| 内核版本 | Linux 4.19+ (推荐 5.10+) |
| Terway 版本 | v1.3+ |
| ECS 实例规格 | 必须支持 Trunk ENI (第七代/第六代增强) |
| VLAN ID 范围 | 2-4094，Terway 自动分配 |

**验证步骤:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 确认 Trunk ENI 已启用
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show eni
# 输出中应包含 "trunk" 标识的 ENI

# 2. 检查 NodeNetworking CRD 是否包含 Trunk 配置
kubectl get nodenetworking <node-name> -o yaml | grep -A 5 trunk

# 3. 确认 VLAN 子接口已创建
kubectl exec -n kube-system <terway-pod> -c terway -- ip link show type ipvlan
```

**注意事项:**
- VLAN tag 由 Terway 自动管理，无需手动配置
- Trunk ENI 上的安全组规则应用于所有子接口流量
- 单个 Trunk ENI 最多支持 4094 个 VLAN，实际受 ECS 规格限制
- 与 IPVlan 模式互斥，不可同时启用

---

## 3. NetworkPolicy 使用

### 3.1 实现方式对比

| 特性 | iptables 模式 | Cilium eBPF 模式 |
|:---|:---|:---|
| 实现原理 | iptables 规则匹配 | eBPF 程序挂载到内核 |
| 性能 | 规则数量多时下降明显 | 恒定高性能 |
| 规则更新延迟 | 毫秒级 | 微秒级 |
| 可观测性 | 需查 iptables 规则 | 支持 Hubble 可视化 |
| 依赖要求 | 无额外依赖 | 内核 4.19+，需部署 Cilium Agent |
| L7 策略 | 不支持 | 支持 HTTP/gRPC 等七层规则 |
| 推荐场景 | 规则数量少、简单隔离 | 大规模、复杂策略、高性能需求 |

### 3.2 启用 Cilium eBPF 模式

通过 terway-config ConfigMap 启用 Cilium eBPF：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-config
  namespace: kube-system
data:
  cilium_config: |
    {
      "enable": true,
      "agent_image": "registry.cn-hangzhou.aliyuncs.com/acs/cilium:v1.14.0",
      "ebpf_dp": true
    }
```

启用后确认：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl get pods -n kube-system -l k8s-app=cilium -o wide
kubectl exec -n kube-system -c cilium-agent $(kubectl get pods -n kube-system -l k8s-app=cilium -o jsonpath='{.items[0].metadata.name}') -- cilium status
```

### 3.3 禁止所有入口流量

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
```

### 3.4 允许特定标签 Pod 访问指定端口

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
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
    - port: 8080
      protocol: TCP
    - port: 8443
      protocol: TCP
```

### 3.5 出口规则（含 DNS 放行）

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-egress-with-dns
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    - podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - port: 53
      protocol: UDP
    - port: 53
      protocol: TCP
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - port: 5432
      protocol: TCP
```

### 3.6 已知兼容性问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| ENI 模式 + Calico 无法阻断同节点 Pod 流量 | ENI 模式下同节点 Pod 流量不经宿主机网络栈 | 升级 Terway v1.4+ 且 Calico v3.24+ |
| NetworkPolicy 对 NodePort Service 不生效 | NodePort 流量经 DNAT 后策略匹配方向变化 | 使用外部 SLB 替代 NodePort |
| eBPF 模式与 kube-proxy iptables 不兼容 | 两者在 hook 点冲突 | eBPF 模式下使用 kube-proxy replacement 或完全关闭 |

---

## 4. 固定 IP (Fixed IP)

固定 IP 功能确保 Pod 重建后 IP 地址保持不变，适用于数据库、中间件等对 IP 有依赖的场景。

### 4.1 PodNetworking CRD 配置

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: fixed-ip-production
spec:
  allocationType:
    type: Fixed
    releaseStrategy: TTL
    releaseAfter: "5m"
  selector:
    podSelector:
      matchLabels:
        app: database
    namespaceSelector:
      matchLabels:
        name: production
```

allocationType 字段说明：

| 字段 | 值 | 说明 |
|:---|:---|:---|
| `type` | `Fixed` | 固定 IP 分配类型 |
| `releaseStrategy` | `TTL` | Pod 删除后 IP 保留策略 |
| `releaseAfter` | `5m` / `1h` / `24h` | IP 保留时长，超时后自动释放 |

### 4.2 StatefulSet 注解方式

StatefulSet 通过 annotation 声明使用固定 IP：

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      annotations:
        k8s.aliyun.com/pod-ip-fixed: "true"
        k8s.aliyun.com/pod-ip-retain-hour: "24"
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        ports:
        - containerPort: 3306
```

### 4.3 ReservedIP CRD 显式保留 IP

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: ReservedIP
metadata:
  name: reserve-db-ip
spec:
  ipType: IPv4
  ip:
    ipv4: "10.0.1.100"
  retentionDuration: "72h"
  reclaimPolicy: "Delete"
```

| 字段 | 说明 |
|:---|:---|
| `ipType` | IP 类型：`IPv4` / `IPv6` / `DualStack` |
| `retentionDuration` | 保留时长，`0` 表示永久保留 |
| `reclaimPolicy` | 回收策略：`Delete`（自动释放）/ `Retain`（保留记录） |

### 4.4 验证固定 IP

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}') -- terway-cli show | grep "fixed"
kubectl get podeni -A -o wide
kubectl get ipinstance -A -o wide
```

---

## 5. Pod 安全组

ENIIP 模式支持 Pod 级别独立安全组，通过 annotation 指定：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-sg
  annotations:
    k8s.aliyun.com/security-group: "sg-2zexxxxx,sg-2zeYYYYY"
spec:
  containers:
  - name: app
    image: nginx:latest
```

也可在 Deployment / StatefulSet 的 Pod template 中使用：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      annotations:
        k8s.aliyun.com/security-group: "sg-2zexxxxx"
      labels:
        app: backend
    spec:
      containers:
      - name: backend
        image: backend:latest
```

注意事项：
- 安全组必须属于与集群相同的 VPC
- 多个安全组以英文逗号分隔
- annotation 优先级高于 eni-config 中的全局 security_group
- 仅 ENIIP 模式支持，VPC 路由模式不支持

---

## 6. ENI 独占模式

ENI 独占模式为每个 Pod 分配一块独立的弹性网卡，实现完全的网络隔离和最高性能。

### 6.1 通过 Pod Annotation 启用

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: high-perf-pod
  annotations:
    k8s.aliyun.com/eni: "true"
spec:
  containers:
  - name: app
    image: nginx:latest
    resources:
      limits:
        aliyun/eni: "1"
      requests:
        aliyun/eni: "1"
```

### 6.2 典型使用场景

| 场景 | 说明 |
|:---|:---|
| 独立安全组 | 需要与节点及其他 Pod 使用不同安全组的业务 |
| 高性能网络 | 低延迟、高吞吐场景（如金融交易、游戏服） |
| 独立公网 EIP | 需要为单个 Pod 绑定独立弹性公网 IP |
| 网络合规隔离 | 监管要求网络完全物理隔离的业务 |

### 6.3 注意事项

- 单节点可分配的 ENI 数量受 ECS 实例规格限制，Pod 密度远低于 ENIIP 模式
- 必须在 resources 中声明 `aliyun/eni: "1"` 的 requests 和 limits
- ENI 独占模式的 Pod 网络性能接近物理机水平

### 6.4 Pod 弹性公网 IP (EIP)

通过 Annotation 为 Pod 自动分配弹性公网 IP，使 Pod 具备公网访问能力。

**Annotation:**

| Annotation | 值 | 说明 |
|:---|:---|:---|
| `k8s.aliyun.com/allocated-eip` | `"true"` | 自动为 Pod 分配 EIP |

**Pod YAML 示例:**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-eip
  annotations:
    k8s.aliyun.com/allocated-eip: "true"
spec:
  containers:
  - name: app
    image: nginx:latest
```

**Deployment YAML 示例:**

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: internet-facing-app
spec:
  replicas: 2
  selector:
    matchLabels:
      app: internet-facing
  template:
    metadata:
      annotations:
        k8s.aliyun.com/allocated-eip: "true"
      labels:
        app: internet-facing
    spec:
      containers:
      - name: app
        image: nginx:latest
```

**前置要求:**

| 要求 | 说明 |
|:---|:---|
| RAM 角色 | 节点 ECS 实例角色需包含 `AliyunEIPFullAccess` 或以下 API 权限: AllocateEipAddress, AssociateEipAddress, UnassociateEipAddress, ReleaseEipAddress |
| Terway 版本 | v1.3+ |
| 网络模式 | ENI 独占模式或 ENIIP 模式 |
| EIP 配额 | 账号下 EIP 配额充足 (默认 20 个/地域) |

**验证命令:**

```bash
# 查看 Pod 获得的 EIP 地址
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations["k8s.aliyun.com/allocated-eip"]}'

# 查看所有带 EIP 的 Pod
kubectl get pods -A -o json | jq -r '.items[] | select(.metadata.annotations["k8s.aliyun.com/allocated-eip"] == "true") | "\(.metadata.name)\t\(.metadata.namespace)"'

# 从外部验证 EIP 可达
curl http://<eip-address>
```

**注意事项:**
- EIP 与 Pod 生命周期绑定，Pod 删除后 EIP 自动释放
- 计费模式默认为按量付费 (PayByTraffic)
- 不支持与 `k8s.aliyun.com/pod-ip-fixed: "true"` 同时使用 (固定 IP 场景建议使用 SLB)
- 安全组需放通 EIP 相关端口

---

## 7. 多 vSwitch / 多可用区

### 7.1 配置多可用区 vSwitch

```json
{
  "vswitches": {
    "cn-hangzhou-b": ["vsw-bp1aXXXXX", "vsw-bp1bXXXXX"],
    "cn-hangzhou-h": ["vsw-bp2aXXXXX"],
    "cn-hangzhou-i": ["vsw-bp3aXXXXX"]
  }
}
```

Terway 会根据节点所在可用区自动选择对应的 vSwitch。当该可用区的 vSwitch IP 资源不足时，可跨可用区分配（受 VPC 限制）。

### 7.2 vSwitch 选择策略

通过 `vswitch_selection_policy` 控制多 vSwitch 的选择逻辑：

| 策略 | 说明 | 适用场景 |
|:---|:---|:---|
| `ordered` | 按 vSwitch 列表顺序依次尝试 | 主备容灾，优先使用指定 vSwitch |
| `random` | 随机选择可用 vSwitch | IP 资源均匀分布，避免单个 vSwitch 耗尽 |

```json
{
  "vswitches": {
    "cn-hangzhou-b": ["vsw-bp1aXXXXX", "vsw-bp1bXXXXX"]
  },
  "vswitch_selection_policy": "ordered"
}
```

### 7.3 容量规划建议

- 每个 vSwitch 建议 /22 以上 CIDR（1024 个 IP），生产环境建议 /20 以上
- 多可用区部署时，每个可用区至少配置一个 vSwitch
- vSwitch 的 CIDR 不能与节点网段、Service CIDR 重叠

---

## 8. IPv6 双栈

### 8.1 启用 IPv6 双栈

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "ENIIP",
      "ip_type": "DualStack",
      "vswitches": {
        "cn-hangzhou-b": ["vsw-bp1aXXXXX"]
      },
      "security_group": "sg-2zexxxxx"
    }
```

### 8.2 验证双栈 Pod

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl get pods -o wide
kubectl exec <pod-name> -- ip addr show eth0
kubectl exec <pod-name> -- ip -6 route
```

双栈 Pod 将同时获得 IPv4 和 IPv6 地址。IPv6 地址可直接在 VPC 内通信，无需 NAT。

前置要求：
- VPC 已开启 IPv6 功能
- vSwitch 已分配 IPv6 CIDR
- 集群 API Server 版本 >= 1.21
- kube-apiserver 启用了 `--feature-gates=IPv6DualStack=true`（1.21 以下版本）

---

## 9. 常用 Annotation 速查表

### 9.1 Pod 级别 Annotation

| Annotation | 值 | 说明 |
|:---|:---|:---|
| `k8s.aliyun.com/eni` | `"true"` | 启用 ENI 独占模式 |
| `k8s.aliyun.com/pod-ip-fixed` | `"true"` | 启用固定 IP |
| `k8s.aliyun.com/pod-ip-retain-hour` | `"24"` | 固定 IP 保留时长（小时） |
| `k8s.aliyun.com/security-group` | `"sg-xxx,sg-yyy"` | 指定 Pod 安全组 |
| `k8s.aliyun.com/vswitch-ids` | `"vsw-xxx,vsw-yyy"` | 指定 Pod 使用的 vSwitch |
| `k8s.aliyun.com/ignore-insecure` | `"true"` | 跳过安全组校验（调试用） |
| `k8s.aliyun.com/allocated-ipv4` | `"true"` | 强制分配 IPv4 地址 |
| `k8s.aliyun.com/allocated-ipv6` | `"true"` | 强制分配 IPv6 地址 |
| `k8s.aliyun.com/allocated-eni` | `"true"` | 强制分配 ENI |
| `k8s.aliyun.com/allocated-eip` | `"true"` | 为 Pod 分配弹性公网 IP |

### 9.2 节点级别 Annotation

| Annotation | 值 | 说明 |
|:---|:---|:---|
| `k8s.aliyun.com/allocated-eniips` | `"true"` | 在节点上预分配 ENI 辅助 IP |
| `k8s.aliyun.com/enipool-ip` | `"10.0.1.100,10.0.1.101"` | 指定节点 IP 池中的 IP 地址 |
| `k8s.aliyun.com/node-network-policy` | `"true"` | 在节点上启用 NetworkPolicy |
| `k8s.aliyun.com/eni-max` | `"5"` | 节点最大 ENI 数量限制 |
| `k8s.aliyun.com/ip-max` | `"50"` | 节点最大辅助 IP 数量限制 |

---

## 10. CRD 操作速查

### 10.1 PodENI

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl get podeni -A
kubectl get podeni <name> -n <namespace> -o yaml
kubectl describe podeni <name> -n <namespace>
kubectl delete podeni <name> -n <namespace>
kubectl get podeni -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podName}{"\t"}{.status.status}{"\n"}{end}'
```

### 10.2 NodeNetworking

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl get nodenetworking
kubectl get nodenetworking <node-name> -o yaml
kubectl describe nodenetworking <node-name>
kubectl patch nodenetworking <node-name> --type merge -p '{"spec":{"eniConfig":{"maxENI":5}}}'
```

### 10.3 PodNetworking

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl get podnetworking
kubectl get podnetworking <name> -o yaml
kubectl apply -f podnetworking.yaml
kubectl delete podnetworking <name>
```

### 10.4 ReservedIP

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl get reservedip
kubectl get reservedip <name> -o yaml
kubectl apply -f reservedip.yaml
kubectl delete reservedip <name>
```

### 10.5 IPInstance

```bash
kubectl get ipinstance -A
kubectl get ipinstance -A -o wide
kubectl get ipinstance <name> -n <namespace> -o yaml
kubectl get ipinstance -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.ip}{"\t"}{.status.podName}{"\n"}{end}'
```

---

## 11. 容量规划参考

### 11.1 集群规模与 CIDR 建议

| 集群规模 | 节点数 | 推荐 vSwitch CIDR | Pod CIDR | Service CIDR | 说明 |
|:---|:---:|:---|:---|:---|:---|
| 小型 | < 50 | /22 (1024 IP) | 10.0.0.0/16 | 10.96.0.0/16 | 开发测试、小团队 |
| 中型 | 50 - 200 | /20 (4096 IP) | 10.0.0.0/14 | 10.96.0.0/13 | 生产环境标准规模 |
| 大型 | 200 - 500 | /18 (16384 IP) | 10.0.0.0/12 | 10.96.0.0/12 | 核心业务、多团队 |
| 超大型 | > 500 | /16 (65536 IP) | 10.0.0.0/10 | 10.96.0.0/10 | 多可用区、多集群 |

### 11.2 单节点 Pod 容量计算

ENIIP 模式下，单节点最大 Pod 数（Terway 管理）计算公式：

```
最大 Pod 数 = (ENI 配额 - 1) * 单 ENI 辅助 IP 数
```

常见实例规格参考：

> 以下为 ENIIP 模式参考值，完整容量速查表见 [01-product.md 第 7 节](./01-product.md#7-ecs-实例规格-eni-限制速查)。

| 实例规格 | ENI 配额 | 单 ENI IP 数 | 最大 Terway Pod 数 |
|:---|:---:|:---:|:---:|
| ecs.g7.large | 3 | 6 | 12 |
| ecs.g7.xlarge | 4 | 10 | 30 |
| ecs.g7.2xlarge | 6 | 15 | 75 |
| ecs.g7.4xlarge | 8 | 30 | 210 |
| ecs.g7.8xlarge | 16 | 30 | 450 |

> 注：ENI 配额减 1 是因为至少保留一块 ENI 用于节点自身网络。

### 11.3 IP 池参数调优

```json
{
  "max_pool_size": 25,
  "min_pool_size": 5
}
```

- `max_pool_size`：建议设置为节点预期 Pod 数的 1.2 倍
- `min_pool_size`：建议设置为 `max_pool_size` 的 20%，保证冷启动速度
- 超大规格实例（如 64C256G）可适当增大 `max_pool_size` 至 60-100

---

## 12. Pod 带宽限制

Terway v1.5+ 支持通过 Annotation 对 Pod 进行出口/入口带宽限速，基于 eBPF EDT (Earliest Departure Time) 或 TC (Traffic Control) 实现。

**Annotation:**

| Annotation | 值格式 | 说明 |
|:---|:---|:---|
| `k8s.aliyun.com/egress-bandwidth` | `"10M"`, `"1G"` 等 | Pod 出口带宽限制 |
| `k8s.aliyun.com/ingress-bandwidth` | `"10M"`, `"1G"` 等 | Pod 入口带宽限制 |

**支持的值格式:** 数字 + 单位，单位支持 `K` (Kbps), `M` (Mbps), `G` (Gbps)。例如: `"10M"` = 10 Mbps, `"1G"` = 1 Gbps。

**Pod YAML 示例:**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: bandwidth-limited-pod
  annotations:
    k8s.aliyun.com/egress-bandwidth: "10M"
    k8s.aliyun.com/ingress-bandwidth: "20M"
spec:
  containers:
  - name: app
    image: nginx:latest
```

**Deployment YAML 示例:**

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rate-limited-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: rate-limited
  template:
    metadata:
      annotations:
        k8s.aliyun.com/egress-bandwidth: "100M"
      labels:
        app: rate-limited
    spec:
      containers:
      - name: app
        image: nginx:latest
```

**前置要求:**

| 要求 | 说明 |
|:---|:---|
| Terway 版本 | v1.5+ |
| 数据面 | eBPF/EDT (推荐) 或 TC 集成 |
| 内核版本 | 5.10+ (eBPF EDT 模式) |

**验证带宽限速:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 检查 TC 规则是否已挂载
kubectl exec -n kube-system <terway-pod> -- tc qdisc show dev eth0

# 使用 iperf3 测试实际带宽
kubectl run iperf3-server --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- -s
kubectl run iperf3-client --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- \
  -c <server-ip> -t 10 -P 1
# 观察吞吐量是否被限制在指定范围内

# 检查 Pod 的 bandwidth annotation 是否生效
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations}' | jq .
```

---

## 11. 多集群 Terway 网络方案

### 11.1 多集群网络互通方案

| 方案 | 适用场景 | 延迟 | 复杂度 |
|:---|:---|:---:|:---:|
| CEN (云企业网) | 同账号/跨账号 VPC 互通 | 低 | 中 |
| VPN 网关 | 混合云/跨云 | 中 | 高 |
| PrivateLink | 单向服务暴露 | 低 | 低 |
| Service Mesh (Istio MultiCluster) | 服务级互通 | 低 | 高 |

### 11.2 CEN 多集群 Pod 互通

**前置条件：**

- 每个 ACK 集群使用不同的 Pod vSwitch CIDR（不重叠）
- CEN 实例连接所有 VPC
- 路由表传播已启用

**配置清单：**

1. 为每个集群规划不重叠的 Pod CIDR
2. 创建 CEN 实例并挂载 VPC
3. 配置路由表传播 Pod CIDR
4. 验证跨集群 Pod 连通性

```bash
aliyun cen CreateCen --Name terway-multi-cluster
aliyun cen AttachCenChildInstance --CenId cen-xxx --ChildInstanceId vpc-xxx1 --ChildInstanceType VPC
aliyun cen AttachCenChildInstance --CenId cen-xxx --ChildInstanceId vpc-xxx2 --ChildInstanceType VPC
aliyun cen PublishRouteEntries --CenId cen-xxx --ChildInstanceId vpc-xxx1 --ChildInstanceType VPC --RouteTableId rtb-xxx --DestinationCidrBlock 10.244.0.0/16
```

**验证：**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n default test-pod-cluster1 -- ping <pod-ip-in-cluster2>
```

### 11.3 多集群 Terway 配置对齐

跨集群需对齐的关键配置：

- NetworkPolicy 实现方式（iptables vs Cilium）
- ENI/IP 池大小设置
- GC 参数
- 安全组规则一致性
- DNS 解析（CoreDNS 跨集群转发）

### 11.4 注意事项

- Pod CIDR 绝对不能重叠
- 安全组需要放通其他集群的 Pod CIDR
- vSwitch 可用 IP 需要额外预留（CEN 路由传播）
- 建议使用 Terraform 管理多集群网络配置

> 相关参考：[domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md](../domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md)

---

## 13. 交叉引用

### 13.1 本专题其他文档

| 文档 | 说明 |
|:---|:---|
| [01-product.md](./01-product.md) | Terway 产品概览、版本历史、与 Flannel/Cilium 对比 |
| [02-architecture.md](./02-architecture.md) | 架构原理、ENI/ENIIP 数据面、IPAM 机制、CRD 模型 |
| [04-operations.md](./04-operations.md) | 运维手册、健康检查、GC 机制、升级策略、故障排查 |
| [05-testing.md](./05-testing.md) | 网络测试、连通性验证、NetworkPolicy 测试、ENI 配额验证 |
| [06-performance.md](./06-performance.md) | 性能调优、模式性能对比、内核调优、基准测试 |

### 13.2 Domain 知识库

| 文档 | 说明 |
|:---|:---|
| [domain-03-networking-traffic/05-terway-advanced-guide.md](../domain-03-networking-traffic/05-terway-advanced-guide.md) | Terway 高级指南（模式对比、ENIIP 详解、容量规划） |
| [domain-03-networking-traffic/37-terway-resources-crud-operations.md](../domain-03-networking-traffic/37-terway-resources-crud-operations.md) | CRD 完整 CRUD 操作 |
| [domain-03-networking-traffic/38-terway-gc-mechanism.md](../domain-03-networking-traffic/38-terway-gc-mechanism.md) | GC 垃圾回收机制详解 |
| [domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md](../domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md) | VPC 网络规划参考 |
| [domain-03-networking-traffic/16-networkpolicy-deep-practice.md](../domain-03-networking-traffic/16-networkpolicy-deep-practice.md) | NetworkPolicy 深度实践 |
| [domain-03-networking-traffic/02-cni-architecture-fundamentals.md](../domain-03-networking-traffic/02-cni-architecture-fundamentals.md) | CNI 架构基础 |
| [domain-03-networking-traffic/03-cni-plugins-comparison.md](../domain-03-networking-traffic/03-cni-plugins-comparison.md) | CNI 插件对比选型 |

### 13.3 Topic 专题

| 文档 | 说明 |
|:---|:---|
| [domain-11-production-operations/topic-presentations/kubernetes-terway-presentation.md](../domain-11-production-operations/topic-presentations/kubernetes-terway-presentation.md) | Terway 全栈进阶培训 |
| [domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md](../domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md) | Day 24: Terway CNI 入门 |
| [domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md) | 结构化故障排查 |
| [domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md](../domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md) | FTA 故障树 |

---

**Kusheet Project** | 作者: Allen Galler (allengaller@gmail.com)

## Related

- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
