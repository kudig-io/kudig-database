---
title: 02 - ACK 目标集群设计与搭建 [migration]
description: 'title: 02 - ACK 目标集群设计与搭建'
category: general
tags:
- migration
- upgrade
- etcd
- prometheus
- grafana
- cilium
- flannel
- calico
- coredns
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- ACK 目标集群设计与搭建 是什么
- 如何 ACK 目标集群设计与搭建
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- ACK
- 目标集群设计与搭建
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

title: 02 - ACK 目标集群设计与搭建
description: '# 02 - ACK 目标集群设计与搭建'
category: migration
tags:
- k8s
- migration
- modernization
- [[etcd|etcd]]
- [[Prometheus|prometheus]]
- grafana
- [[Cilium|cilium]]
- flannel
- calico
- [[CoreDNS|coredns]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- ACK 目标集群设计与搭建 是什么
- 如何 ACK 目标集群设计与搭建
trigger_keywords:
- ACK
- 目标集群设计与搭建
- migration
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

# 02 - ACK 目标集群设计与搭建

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: ACK, 集群设计, VPC, Terway, 节点池, Addon

---

<!-- chunk: 目录 -->## 目录

1. [集群类型选择](#1-集群类型选择)
2. [VPC 与网络规划](#2-vpc-与网络规划)
3. [节点池设计](#3-节点池设计)
4. [Addon 与组件配置](#4-addon-与组件配置)
5. [集群创建实操](#5-集群创建实操)
6. [基础设施验证](#6-基础设施验证)
7. [迁移前基线建立](#7-迁移前基线建立)

---

<!-- chunk: 1. 集群类型选择 -->## 1. 集群类型选择

#<!-- chunk: 1.1 ACK 版本对比 -->## 1.1 ACK 版本对比

| 维度 | ACK 标准托管版 | ACK Pro 版 | ACK Serverless |
|------|--------------|-----------|---------------|
| **控制面** | 阿里云全托管 | 阿里云全托管（增强 SLA） | 阿里云全托管 |
| **SLA** | 99.95% | 99.95%（赔偿承诺） | 99.9% |
| **调度增强** | 无 | Gang Scheduling、拓扑感知 | 不适用 |
| **安全增强** | 基础 | 镜像签名验证、审计增强 | 基础 |
| **节点管理** | 用户管理 ECS | 用户管理 ECS | 无需管理节点 |
| **成本** | 集群免费，按节点收费 | ¥1500/月/集群 + 节点 | 按 Pod 资源收费 |
| **适用场景** | 中小型业务 | 生产核心业务 | 突发/弹性负载 |
| **迁移推荐** | 测试环境 | **生产环境（推荐）** | 补充弹性能力 |

#<!-- chunk: 1.2 决策建议 -->## 1.2 决策建议

```
自建集群规模
    │
    ├── < 10 节点，非核心业务 → ACK 标准托管版
    │
    ├── 10-100 节点，生产核心 → ACK Pro 版（推荐）
    │
    └── > 100 节点，多租户     → ACK Pro 版 + ACK Serverless（混合）
```

---

<!-- chunk: 2. VPC 与网络规划 -->## 2. VPC 与网络规划

#<!-- chunk: 2.1 网络架构设计 -->## 2.1 网络架构设计

> **核心原则**: CIDR 不能与自建集群重叠（如需 VPN/专线互联），预留 3 倍以上扩展空间。

```
┌─────────────────────────────────────────────────────────────────┐
│                        VPC: 10.0.0.0/8                          │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  可用区 A (cn-hangzhou-h)                                  │   │
│  │  ├─ vSwitch-system: 10.0.0.0/20  (4094 IPs) → 系统节点池    │   │
│  │  ├─ vSwitch-app:    10.0.16.0/20 (4094 IPs) → 业务节点池    │   │
│  │  └─ vSwitch-pod-a:  10.0.32.0/19 (8190 IPs) → Pod (Terway) │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  可用区 B (cn-hangzhou-i)                                  │   │
│  │  ├─ vSwitch-system: 10.0.64.0/20  → 系统节点池              │   │
│  │  ├─ vSwitch-app:    10.0.80.0/20  → 业务节点池              │   │
│  │  └─ vSwitch-pod-b:  10.0.96.0/19  → Pod (Terway)          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  可用区 C (cn-hangzhou-j) — 灾备可用区                       │   │
│  │  ├─ vSwitch-app:    10.0.128.0/20 → 业务节点池              │   │
│  │  └─ vSwitch-pod-c:  10.0.144.0/19 → Pod (Terway)          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  Service CIDR: 172.21.0.0/16 (独立，不占 VPC 地址)               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 2.2 CIDR 规划规则 -->## 2.2 CIDR 规划规则

| 网段类型 | CIDR 范围 | 说明 | 规划建议 |
|---------|----------|------|---------|
| **VPC CIDR** | 10.0.0.0/8 | VPC 主网段 | 使用 /8 或 /12 预留最大空间 |
| **节点 vSwitch** | /20 每个可用区 | 节点 IP 分配 | 每 AZ 4094 个节点 IP |
| **Pod vSwitch** | /19 每个可用区 | Terway Pod IP | 每 AZ 8190 个 Pod IP |
| **Service CIDR** | 172.21.0.0/16 | K8s Service VIP | 65534 个 Service |

#<!-- chunk: 2.3 与自建集群网络互通 -->## 2.3 与自建集群网络互通

> 迁移期间，自建集群与 ACK 可能需要互访（如数据库同步、服务调用）。

```bash
# 方案一：CEN (云企业网) — 推荐用于 IDC 与阿里云 VPC 互联
# 1. 创建 CEN 实例
aliyun cbn CreateCen --Name "migration-cen"

# 2. 将 VPC 加入 CEN
aliyun cbn AttachCenChildInstance \
  --CenId <cen-id> \
  --ChildInstanceId <vpc-id> \
  --ChildInstanceType VPC \
  --ChildInstanceRegionId cn-hangzhou

# 方案二：VPN Gateway — 适用于通过公网加密互联
# 1. 创建 VPN 网关
aliyun vpc CreateVpnGateway \
  --RegionId cn-hangzhou \
  --VpcId <vpc-id> \
  --Bandwidth 100 \
  --InstanceChargeType PostPaid

# 2. 创建用户网关（指向 IDC 公网 IP）
aliyun vpc CreateCustomerGateway \
  --RegionId cn-hangzhou \
  --IpAddress <idc-public-ip> \
  --Name "idc-gw"

# 3. 创建 IPSec 连接
aliyun vpc CreateVpnConnection \
  --RegionId cn-hangzhou \
  --VpnGatewayId <vpn-gw-id> \
  --CustomerGatewayId <customer-gw-id> \
  --LocalSubnet "10.0.0.0/8" \
  --RemoteSubnet "192.168.0.0/16" \
  --IkeConfig '{"IkeVersion":"ikev2","IkeMode":"main","IkeEncAlg":"aes","IkeAuthAlg":"sha1","IkePfs":"group2","IkeLifetime":86400}' \
  --IpsecConfig '{"IpsecEncAlg":"aes","IpsecAuthAlg":"sha1","IpsecPfs":"group2","IpsecLifetime":86400}'

# 方案三：高速通道 (Express Connect) — 适用于高带宽低延迟需求
# 需通过控制台申请物理专线接入
```

#<!-- chunk: 2.4 VPC 创建实操 -->## 2.4 VPC 创建实操

```bash
# 创建 VPC
aliyun vpc CreateVpc \
  --RegionId cn-hangzhou \
  --CidrBlock "10.0.0.0/8" \
  --VpcName "ack-migration-vpc" \
  --Description "ACK migration target VPC"

# 记录 VPC ID
export VPC_ID=$(aliyun vpc DescribeVpcs --VpcName "ack-migration-vpc" \
  --output cols=VpcId --rows Vpcs.Vpc[] | tail -1)
echo "VPC ID: $VPC_ID"

# 创建 vSwitch（可用区 A — 系统节点）
aliyun vpc CreateVSwitch \
  --VpcId $VPC_ID \
  --ZoneId cn-hangzhou-h \
  --CidrBlock "10.0.0.0/20" \
  --VSwitchName "vsw-system-a"

# 创建 vSwitch（可用区 A — 业务节点）
aliyun vpc CreateVSwitch \
  --VpcId $VPC_ID \
  --ZoneId cn-hangzhou-h \
  --CidrBlock "10.0.16.0/20" \
  --VSwitchName "vsw-app-a"

# 创建 vSwitch（可用区 A — Pod Terway）
aliyun vpc CreateVSwitch \
  --VpcId $VPC_ID \
  --ZoneId cn-hangzhou-h \
  --CidrBlock "10.0.32.0/19" \
  --VSwitchName "vsw-pod-a"

# 创建 vSwitch（可用区 B — 同理）
aliyun vpc CreateVSwitch --VpcId $VPC_ID --ZoneId cn-hangzhou-i --CidrBlock "10.0.64.0/20" --VSwitchName "vsw-system-b"
aliyun vpc CreateVSwitch --VpcId $VPC_ID --ZoneId cn-hangzhou-i --CidrBlock "10.0.80.0/20" --VSwitchName "vsw-app-b"
aliyun vpc CreateVSwitch --VpcId $VPC_ID --ZoneId cn-hangzhou-i --CidrBlock "10.0.96.0/19" --VSwitchName "vsw-pod-b"

# 验证
aliyun vpc DescribeVSwitches --VpcId $VPC_ID --output cols=VSwitchId,VSwitchName,CidrBlock,ZoneId --rows VSwitches.VSwitch[]
```

---

<!-- chunk: 3. 节点池设计 -->## 3. 节点池设计

#<!-- chunk: 3.1 节点池规划 -->## 3.1 节点池规划

| 节点池 | 用途 | 实例规格 | 数量 | 标签 | 污点 |
|--------|------|---------|------|------|------|
| **system-pool** | 系统组件（监控/日志/Ingress） | ecs.g7.xlarge (4C16G) | 2-3 | `node-role=system` | `CriticalAddonsOnly=true:NoSchedule` |
| **app-pool** | 无状态业务应用 | ecs.g7.2xlarge (8C32G) | 3-10 | `node-role=app` | 无 |
| **stateful-pool** | 有状态服务（DB/缓存） | ecs.r7.2xlarge (8C64G) | 2-4 | `node-role=stateful` | `workload-type=stateful:NoSchedule` |
| **spot-pool** | 弹性/非核心任务 | ecs.g7.2xlarge | 0-5 (ASG) | `node-role=spot` | `spot=true:NoSchedule` |

#<!-- chunk: 3.2 节点规格选型 -->## 3.2 节点规格选型

```
自建集群节点规格 → ACK 节点规格映射

自建 4C8G  → ecs.g7.xlarge  (4C16G)   — 建议内存翻倍，Terway 需要更多内存
自建 8C16G → ecs.g7.2xlarge (8C32G)   — 通用计算型，适合大部分应用
自建 8C32G → ecs.r7.2xlarge (8C64G)   — 内存优化型，适合数据库/缓存
自建 16C32G → ecs.c7.4xlarge (16C32G) — 计算优化型，适合 CPU 密集任务
GPU 节点   → ecs.gn7i-c8g1.2xlarge   — NVIDIA A10，适合推理任务
```

#<!-- chunk: 3.3 自动伸缩配置 -->## 3.3 自动伸缩配置

```yaml
# 节点池自动伸缩配置（通过 ACK 控制台或 API 设置）
# 等效配置参考:
auto_scaling:
  enable: true
  min_instances: 3        # 最小节点数
  max_instances: 20       # 最大节点数
  type: "cpu"             # 扩缩策略: cpu / memory
  
  # 扩容策略
  scale_up:
    threshold: 70         # CPU 利用率 > 70% 触发扩容
    cooldown: 300         # 扩容冷却期 5 分钟
    
  # 缩容策略
  scale_down:
    threshold: 30         # CPU 利用率 < 30% 触发缩容
    cooldown: 600         # 缩容冷却期 10 分钟
    delay_after_add: 600  # 新节点加入后 10 分钟内不缩容
```

---

<!-- chunk: 4. Addon 与组件配置 -->## 4. Addon 与组件配置

#<!-- chunk: 4.1 核心 Addon 选型 -->## 4.1 核心 Addon 选型

| 类别 | Addon 名称 | 说明 | 迁移建议 |
|------|-----------|------|---------|
| **网络** | terway-eniip | Terway ENI 多 IP 模式 | **推荐** — 高性能、支持 NetworkPolicy |
| **网络** | flannel | Flannel VXLAN 覆盖网络 | 兼容性好，但性能不如 Terway |
| **存储** | csi-plugin + csi-provisioner | 阿里云 CSI 驱动 | **必装** — 云盘/NAS/OSS |
| **Ingress** | nginx-ingress-controller | Nginx Ingress | 与自建一致，迁移最平滑 |
| **Ingress** | alb-ingress-controller | ALB Ingress | 云原生方案，推荐新业务 |
| **监控** | arms-prometheus | ARMS Prometheus 采集 | 可选，也可自建 Prometheus |
| **日志** | logtail-ds | SLS 日志采集 | 可选，也可保留 EFK |
| **DNS** | coredns | 集群 DNS | **必装**，ACK 默认含 |
| **安全** | ack-security-inspector | ACK 安全巡检 | **推荐** |

#<!-- chunk: 4.2 CNI 选型决策 -->## 4.2 CNI 选型决策

```
自建集群 CNI
    │
    ├── Calico → Terway（推荐）
    │   ├─ NetworkPolicy 兼容性: Terway 原生支持 K8s NetworkPolicy
    │   ├─ 性能: Terway ENI 模式接近裸金属性能
    │   └─ 注意: CIDR 规划不同，需 Pod vSwitch
    │
    ├── Flannel → Flannel 或 Terway
    │   ├─ Flannel: 迁移最简单，CIDR 可复用
    │   └─ Terway: 性能更好，推荐长期方案
    │
    ├── Cilium → Terway
    │   ├─ eBPF 特性: Terway 也支持 eBPF datapath
    │   └─ NetworkPolicy: 需验证 CiliumNetworkPolicy 兼容
    │
    └── Weave → Terway（推荐）
        └─ Weave 特有功能需替代方案
```

---

<!-- chunk: 5. 集群创建实操 -->## 5. 集群创建实操

#<!-- chunk: 5.1 通过 API 创建 ACK Pro 集群 -->## 5.1 通过 API 创建 ACK Pro 集群

```bash
# 创建 ACK Pro 托管版集群
aliyun cs POST /clusters --body '{
  "name": "ack-migration-prod",
  "cluster_type": "ManagedKubernetes",
  "cluster_spec": "ack.pro.small",
  "kubernetes_version": "1.28.9-aliyun.1",
  "region_id": "cn-hangzhou",
  "vpcid": "'$VPC_ID'",
  "service_cidr": "172.21.0.0/16",
  "pod_vswitch_ids": ["<vsw-pod-a-id>", "<vsw-pod-b-id>"],
  "vswitch_ids": ["<vsw-system-a-id>", "<vsw-system-b-id>"],
  "num_of_nodes": 0,
  "endpoint_public_access": true,
  "snat_entry": true,
  "proxy_mode": "ipvs",
  "timezone": "Asia/Shanghai",
  "node_cidr_mask": "25",
  "deletion_protection": true,
  "addons": [
    {"name": "terway-eniip"},
    {"name": "csi-plugin"},
    {"name": "csi-provisioner"},
    {"name": "nginx-ingress-controller", "config": "{\"IngressSlbNetworkType\":\"internet\"}"},
    {"name": "arms-prometheus"},
    {"name": "logtail-ds"}
  ],
  "tags": [
    {"key": "Environment", "value": "production"},
    {"key": "Project", "value": "migration"},
    {"key": "ManagedBy", "value": "aliyun-cli"}
  ]
}'

# 记录集群 ID
export CLUSTER_ID="<返回的 cluster_id>"

# 等待集群创建完成（约 8-15 分钟）
watch -n 10 "aliyun cs GET /clusters/$CLUSTER_ID | jq '.state'"
# 预期: "running"

# 获取 kubeconfig
aliyun cs GET /k8s/$CLUSTER_ID/user_config | jq -r '.config' > ~/.kube/ack-migration.yaml
export KUBECONFIG=~/.kube/ack-migration.yaml

# 验证连接
kubectl cluster-info
kubectl get nodes  # 此时应为空（尚未创建节点池）
```

#<!-- chunk: 5.2 创建节点池 -->## 5.2 创建节点池

```bash
# 系统节点池
aliyun cs POST /clusters/$CLUSTER_ID/nodepools --body '{
  "nodepool_info": {
    "name": "system-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw-system-a-id>", "<vsw-system-b-id>"],
    "instance_types": ["ecs.g7.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "system_disk_performance_level": "PL1",
    "desired_size": 2,
    "multi_az_policy": "BALANCE",
    "tags": [{"key": "node-pool", "value": "system"}]
  },
  "kubernetes_config": {
    "labels": [
      {"key": "node-role", "value": "system"}
    ],
    "taints": [
      {"key": "CriticalAddonsOnly", "value": "true", "effect": "NoSchedule"}
    ],
    "runtime": "containerd",
    "runtime_version": "1.6.28"
  }
}'

# 业务节点池
aliyun cs POST /clusters/$CLUSTER_ID/nodepools --body '{
  "nodepool_info": {
    "name": "app-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw-app-a-id>", "<vsw-app-b-id>"],
    "instance_types": ["ecs.g7.2xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "system_disk_performance_level": "PL1",
    "desired_size": 3,
    "multi_az_policy": "BALANCE",
    "auto_scaling": {
      "enable": true,
      "min_instances": 3,
      "max_instances": 20,
      "type": "cpu",
      "eip_bandwidth": 0
    }
  },
  "kubernetes_config": {
    "labels": [
      {"key": "node-role", "value": "app"}
    ],
    "runtime": "containerd",
    "runtime_version": "1.6.28"
  }
}'

# 等待节点就绪
kubectl get nodes -w
# 预期: 所有节点 STATUS=Ready
```

---

<!-- chunk: 6. 基础设施验证 -->## 6. 基础设施验证

#<!-- chunk: 6.1 集群健康检查 -->## 6.1 集群健康检查

```bash
# 全面健康检查脚本
echo "=== 集群版本 ==="
kubectl version --short

echo "=== 节点状态 ==="
kubectl get nodes -o wide

echo "=== 系统 Pod 状态 ==="
kubectl get pods -n kube-system
# 预期: 所有 Pod 均为 Running/Completed

echo "=== CoreDNS 解析测试 ==="
kubectl run dns-test --rm -it --image=busybox:1.36 --restart=Never -- \
  nslookup kubernetes.default.svc.cluster.local
# 预期: 返回 Service IP (172.21.0.1)

echo "=== 网络连通性测试 ==="
kubectl run net-test --rm -it --image=busybox:1.36 --restart=Never -- \
  wget -qO- --timeout=5 http://www.aliyun.com
# 预期: 返回 HTML 内容（确认外网可达）

echo "=== CSI 存储测试 ==="
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: csi-test-pvc
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: alicloud-disk-essd
  resources:
    requests:
      storage: 20Gi
EOF

# 等待 PVC 绑定
kubectl wait --for=jsonpath='{.status.phase}'=Bound pvc/csi-test-pvc --timeout=120s
kubectl get pvc csi-test-pvc
# 预期: STATUS=Bound

# 清理
kubectl delete pvc csi-test-pvc

echo "=== Ingress Controller 测试 ==="
kubectl get svc -n kube-system | grep nginx-ingress
# 预期: 有 LoadBalancer 类型 Service，EXTERNAL-IP 已分配

echo "=== 健康检查完成 ==="
```

#<!-- chunk: 6.2 性能基线测试 -->## 6.2 性能基线测试

```bash
# 部署网络性能测试工具
kubectl apply -f https://raw.githubusercontent.com/InfraBuilder/k8s-bench-suite/master/netperf.yaml

# Pod 间网络延迟测试
kubectl run ping-test --rm -it --image=busybox:1.36 --restart=Never -- \
  sh -c "ping -c 20 <另一节点Pod-IP> | tail -1"
# 记录: 平均延迟应 < 1ms (同 AZ) / < 2ms (跨 AZ)

# 存储 IOPS 测试
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: Job
metadata:
  name: fio-test
spec:
  template:
    spec:
      containers:
      - name: fio
        image: ljishen/fio
        command: ["fio", "--name=randwrite", "--ioengine=libaio", "--direct=1",
                  "--bs=4k", "--size=1G", "--numjobs=4", "--runtime=30",
                  "--group_reporting", "--filename=/data/testfile"]
        volumeMounts:
        - name: data
          mountPath: /data
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: fio-test-pvc
      restartPolicy: Never
  backoffLimit: 0
EOF
# 查看结果
kubectl logs job/fio-test
# 记录: ESSD PL1 预期 IOPS > 50,000
```

---

<!-- chunk: 7. 迁移前基线建立 -->## 7. 迁移前基线建立

#<!-- chunk: 7.1 监控基线 -->## 7.1 监控基线

```bash
# 如果安装了 ARMS Prometheus，确认指标采集
kubectl get pods -n arms-prom -l app=arms-prometheus

# 部署标准监控 Stack（如不使用 ARMS）
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install kube-prometheus-stack prometheus-community/kube-prometheus-stack \
  -n monitoring --create-namespace \
  --set prometheus.prometheusSpec.retention=30d \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.storageClassName=alicloud-disk-essd \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
  --set grafana.persistence.enabled=true \
  --set grafana.persistence.storageClassName=alicloud-disk-essd \
  --set grafana.persistence.size=20Gi

# 记录 ACK 集群空载基线指标
# - 节点 CPU/内存利用率
# - API Server 请求延迟
# - etcd 磁盘 IOPS
# - 网络吞吐
```

#<!-- chunk: 7.2 日志采集配置 -->## 7.2 日志采集配置

```yaml
# 如果使用 SLS，配置 Logtail 采集规则
# 通过 ACK 控制台: 集群 → 日志中心 → 日志组件管理 → 配置采集规则

# 等效 AliyunLogConfig CRD:
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: stdout-log
  namespace: kube-system
spec:
  project: "k8s-log-<cluster-id>"
  logstore: "stdout-log"
  logtailConfig:
    inputType: "plugin"
    configName: "stdout-log"
    inputDetail:
      plugin:
        inputs:
        - type: "service_docker_stdout"
          detail:
            IncludeEnv:
              COLLECT_STDOUT_LOG: "true"
            Stdout: true
            Stderr: true
```

---

<!-- chunk: 检查清单 -->## 检查清单

#<!-- chunk: Phase 1 完成标准 -->## Phase 1 完成标准

- [ ] ACK 集群类型已选择（推荐 Pro 版）
- [ ] VPC 和 vSwitch 已创建，CIDR 无冲突
- [ ] 与自建集群网络互通已配置（VPN/CEN/专线）
- [ ] 节点池已创建，所有节点 Ready
- [ ] CoreDNS 解析正常
- [ ] CSI 存储驱动工作正常（PVC 可创建绑定）
- [ ] Ingress Controller 工作正常，外部 IP 已分配
- [ ] 监控基线已建立
- [ ] 日志采集已配置
- [ ] 网络性能基线已记录
- [ ] kubeconfig 已安全保存，团队成员可访问

---

**上一步**: ← [01-迁移评估与规划](./01-migration-assessment-planning.md)
**下一步**: → [03-应用工作负载迁移](./03-application-workload-migration.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-migration MOC
- [[domain-08-release-change-management/topic-migration/README|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[domain-08-release-change-management/topic-migration/01-migration-assessment-planning|01 - 迁移评估与规划]]
- [[domain-08-release-change-management/topic-migration/03-application-workload-migration|03 - 应用工作负载迁移]]
- [[domain-08-release-change-management/topic-migration/04-storage-data-migration|04 - 存储与数据迁移]]
- [[domain-08-release-change-management/topic-migration/05-network-migration-traffic-cutover|05 - 网络迁移与流量切换]]
- [[domain-08-release-change-management/topic-migration/06-stateful-services-migration|06 - 有状态服务迁移]]
- [[domain-08-release-change-management/topic-migration/07-observability-security-migration|07 - 可观测性与安全迁移]]
- [[domain-08-release-change-management/topic-migration/08-validation-cutover-decommission|08 - 验收、切换与旧集群退役]]
- [[domain-08-release-change-management/topic-migration/09-migration-toolchain|09 - 迁移工具链参考]]
- [[domain-08-release-change-management/topic-migration/10-real-world-case-study|10 - 生产迁移实战案例]]

## See Also

- storage
- 01-migration-assessment-planning
- 03-application-workload-migration
- 04-storage-data-migration

## Related

- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
