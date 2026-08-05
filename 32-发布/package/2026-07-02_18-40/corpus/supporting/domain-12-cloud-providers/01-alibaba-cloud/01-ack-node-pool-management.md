---
title: ACK节点池管理与弹性伸缩策略
description: 'ACK节点池深度管理：节点池创建运维、ESS/ASK弹性伸缩、Serverless ECI节点池、GPU节点池配置、节点池升级策略'
summary: 'ACK节点池深度管理：节点池创建运维、ESS/ASK弹性伸缩、Serverless ECI节点池、GPU节点池配置、节点池升级策略'
category: cloud-providers
tags:
- cloud
- k8s
- alibaba-ack
- node-pool
- autoscaling
- gpu
- serverless
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
- ACK节点池管理 是什么
- 如何配置ACK弹性伸缩
- ACK Serverless节点池如何使用
- ACK GPU节点池配置方法
trigger_keywords:
- ACK
- 节点池
- 弹性伸缩
- ECI
- ASK
- GPU
- 节点升级
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


# ACK节点池管理与弹性伸缩策略

## 1. 节点池架构概览

ACK (Alibaba Cloud Container Service for Kubernetes) 的节点池机制允许将节点按业务特征分组管理：

```
ACK 集群
├── 节点池 A: 通用计算 (c7.xlarge)
│   ├── Node-1  (在线服务)
│   ├── Node-2  (在线服务)
│   └── 弹性伸缩: 2 ~ 20 台
│
├── 节点池 B: GPU 计算 (ecs.gn7i-c12g1.3xlarge)
│   ├── GPU-1  (AI 训练)
│   └── GPU-2  (AI 推理)
│
├── 节点池 C: 竞价实例 (低优先级批处理)
│   ├── Spot-1
│   └── Spot-2
│
└── 节点池 D: Serverless (ECI 弹性容器实例)
    └── 按需创建，无需管理节点
```

**节点池核心能力**：
- 独立配置：实例规格、系统盘、网络、标签、污点
- 独立伸缩：每个节点池独立的伸缩策略
- 独立升级：按节点池滚动升级 K8s 版本
- 调度隔离：通过标签和污点控制 Pod 调度目标

## 2. 节点池创建与管理

### 2.1 创建节点池 (控制台)

```
ACK 控制台 → 集群详情 → 节点管理 → 节点池 → 创建节点池

关键配置项:
├── 基本信息
│   ├── 节点池名称: online-service-pool
│   └── 节点数量: 3
│
├── 实例规格
│   ├── 实例规格族: 计算型 c7 / 通用型 g7 / 内存型 r7
│   ├── 实例规格: ecs.c7.xlarge (4C8G)
│   └── 竞价实例: 关闭 (生产在线服务)
│
├── 存储配置
│   ├── 系统盘: ESSD PL1 40Gi
│   └── 数据盘: ESSD PL1 200Gi
│
├── 网络配置
│   ├── VPC: vpc-xxx
│   ├── 交换机: vsw-zone-a, vsw-zone-b (多可用区)
│   └── 安全组: sg-xxx
│
├── 节点配置
│   ├── 操作系统: Alibaba Cloud Linux 3
│   ├── 登录凭证: 密钥对 (推荐) / 密码
│   └── 节点标签: env=prod, tier=online
│
└── 弹性伸缩
    ├── 开启自动伸缩: 是
    ├── 最小实例数: 2
    └── 最大实例数: 20
```

### 2.2 通过 Terraform 创建

```hcl
resource "alicloud_cs_kubernetes_node_pool" "online_pool" {
  cluster_id   = alicloud_cs_managed_kubernetes.cluster.id
  name         = "online-service-pool"
  vswitch_ids  = [alicloud_vswitch.zone_a.id, alicloud_vswitch.zone_b.id]

  # 实例规格
  instance_types    = ["ecs.c7.xlarge", "ecs.c7.2xlarge"]
  desired_size      = 3
  password          = ""  # 使用密钥对

  # 存储
  system_disk_category = "cloud_essd"
  system_disk_size     = 40

  dynamic "data_disks" {
    for_each = [1]
    content {
      category = "cloud_essd"
      size     = 200
    }
  }

  # 标签与污点
  labels {
    key   = "env"
    value = "prod"
  }
  labels {
    key   = "tier"
    value = "online"
  }

  taints {
    key    = "dedicated"
    value  = "online"
    effect = "NoSchedule"
  }

  # 弹性伸缩
  scaling_config {
    min_size = 2
    max_size = 20
  }

  # 安全组
  security_group_ids = [alicloud_security_group.k8s.id]

  # 系统盘加密
  system_disk_encrypted = true
  kms_key_id            = alicloud_kms_key.disk_key.id
}
```

### 2.3 节点池标签与污点

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池配置
kubectl get nodes -l env=prod --show-labels

# 节点污点效果:
# NoSchedule: 不调度新 Pod (除非 Pod 有对应 toleration)
# PreferNoSchedule: 尽量不调度
# NoExecute: 驱逐已有 Pod

# Pod 配置 toleration
apiVersion: v1
kind: Pod
metadata:
  name: online-app
spec:
  tolerations:
    - key: "dedicated"
      operator: "Equal"
      value: "online"
      effect: "NoSchedule"
  nodeSelector:
    env: prod
    tier: online
  containers:
    - name: app
      image: my-app:v1
```
## 3. 弹性伸缩策略

### 3.1 节点自动伸缩 (Cluster Autoscaler)

ACK 使用 Cluster Autoscaler 实现节点级别的自动伸缩：

```
伸缩触发流程:
Pod Pending (资源不足)
    │
    ▼
Cluster Autoscaler 检测
    │
    ├── 评估哪个节点池可以满足
    ├── 检查节点池是否已达上限
    │
    ▼
调用 ESS (Auto Scaling) API
    │
    ├── 创建 ECS 实例
    ├── 加入集群 (节点纳管)
    └── Pod 调度到新节点
```

### 3.2 节点池伸缩配置

```yaml
# 通过 Annotation 配置节点池伸缩参数
apiVersion: v1
kind: Node
metadata:
  annotations:
    # 节点池 ID
    cluster-autoscaler.kubernetes.io/nodepool-id: "np-xxxxxxxxxx"
    # 最小/最大节点数
    cluster-autoscaler.kubernetes.io/min-size: "2"
    cluster-autoscaler.kubernetes.io/max-size: "20"
    # 伸缩冷却时间 (秒)
    cluster-autoscaler.kubernetes.io/scale-down-delay: "10m"
    # 未使用节点阈值 (CPU 利用率低于此值触发缩容)
    cluster-autoscaler.kubernetes.io/utilization-threshold: "0.5"
```

### 3.3 ESS 弹性伸缩策略

```bash
# 查看 ESS 伸缩组
aliyun ess DescribeScalingGroups --RegionId cn-hangzhou

# 创建伸缩规则
aliyun ess CreateScalingRule \
  --ScalingGroupId sg-xxxxxxxxxx \
  --ScalingRuleName scale-out-cpu-80 \
  --AdjustmentType QuantityChangeInCapacity \
  --AdjustmentValue 2 \
  --Cooldown 300

# 查看伸缩活动
aliyun ess DescribeScalingActivities \
  --ScalingGroupId sg-xxxxxxxxxx \
  --MaxResults 10
```

### 3.4 多维度伸缩策略

```
伸缩策略组合:
├── CPU 维度
│   ├── 节点 CPU 使用率 > 70% → 扩容 1 台
│   ├── 节点 CPU 使用率 < 30% 持续 10 分钟 → 缩容 1 台
│   └── 自定义指标: 业务 QPS / 队列深度
│
├── 内存维度
│   ├── 节点内存使用率 > 75% → 扩容 1 台
│   └── 配合 VPA 调整 Pod 内存 requests
│
├── 调度维度
│   ├── Pending Pod 持续 > 1 分钟 → 扩容
│   └── 节点利用率 < 50% 且可排空 → 缩容
│
└── 定时维度
    ├── 工作日 08:00 扩容至 10 台
    ├── 工作日 20:00 缩容至 3 台
    └── 周末保持最小规模 2 台
```

```yaml
# 定时伸缩规则
apiVersion: autoscaling.alibabacloud.com/v1beta1
kind: CronScaling
metadata:
  name: workday-scale
spec:
  # 工作日早上 8 点扩容
  - schedule: "0 8 * * 1-5"
    minSize: 10
    maxSize: 30
  # 工作日晚上 8 点缩容
  - schedule: "0 20 * * 1-5"
    minSize: 3
    maxSize: 10
  # 周末最小规模
  - schedule: "0 0 * * 6,0"
    minSize: 2
    maxSize: 5
```

## 4. Serverless 节点池 (ASK + ECI)

### 4.1 ASK ECI 架构

ASK (Alibaba Serverless Kubernetes) ECI 节点池无需管理 ECS 节点，Pod 直接运行在弹性容器实例上：

```
ACK 集群 + ASK ECI 节点池
├── ECS 节点池 (在线服务，固定资源)
│
└── ECI 节点池 (弹性工作负载)
    ├── 无需预购 ECS
    ├── 按 Pod 计费 (秒级)
    ├── 支持 GPU ECI
    └── 最大支持 1000 Pod / 节点池
```

### 4.2 创建 ECI 节点池

```yaml
# 控制台 → 节点池 → 创建节点池 → Serverless 节点池
# 或通过 API:
apiVersion: cs.aliyun.com/v1
kind: ECIProfile
metadata:
  name: eci-burst-pool
spec:
  # ECI 实例规格
  instanceType:
    - ecs.c7.large
    - ecs.g7.large
    # 多规格备选，提升创建成功率
  # 网络配置
  vswitchIds:
    - vsw-zone-a
    - vsw-zone-b
  securityGroupId: sg-xxx
  # 资源限制
  resourceGroupId: rg-xxx
  # 标签
  tags:
    - key: env
      value: burst
    - key: managed-by
      value: ack
```

### 4.3 ECI 与 ECS 混合调度

```yaml
# 优先使用 ECS 节点，资源不足时自动溢出到 ECI
apiVersion: apps/v1
kind: Deployment
metadata:
  name: burst-worker
spec:
  replicas: 10
  selector:
    matchLabels:
      app: worker
  template:
    metadata:
      labels:
        app: worker
      annotations:
        # ECI 弹性策略
        k8s.aliyun.com/eci-auto-strategy: "auto"
        # 最大 ECI 实例数
        k8s.aliyun.com/eci-max-count: "5"
    spec:
      containers:
        - name: worker
          image: my-worker:v1
          resources:
            requests:
              cpu: "1"
              memory: 2Gi
      # 优先调度到 ECS 节点
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              preference:
                matchExpressions:
                  - key: type
                    operator: NotIn
                    values:
                      - eci
```

### 4.4 ECI 计费与优化

```
ECI 计费模式:
├── 按量付费: 按秒计费，适合突发流量
├── 抢占式实例: 低至 1 折，适合可中断任务
└── 节省计划: 承诺消费额度，享折扣

优化建议:
├── 设置合理的资源 requests (避免过度分配)
├── 使用多规格实例提升创建成功率
├── 配合 spot 策略降低批处理成本
└── 设置自动缩容冷却时间避免频繁伸缩
```

## 5. GPU 节点池配置

### 5.1 GPU 节点池创建

```
关键配置:
├── 实例规格
│   ├── 推理: ecs.gn7i-c8g1.2xlarge (1x T4 16GB)
│   ├── 训练: ecs.gn7e-c16g1.4xlarge (1x A100 80GB)
│   └── 多卡: ecs.gn7e-16g1.16xlarge (8x A100 80GB)
│
├── 驱动与运行时
│   ├── 自动安装 GPU 驱动 (推荐)
│   ├── GPU 驱动版本: 535.x / 550.x
│   └── 容器运行时: containerd + nvidia-container-runtime
│
└── 调度标签
    ├── accelerator: nvidia-t4
    ├── gpu-memory: 16g
    └── gpu-count: "1"
```

### 5.2 GPU 驱动与插件

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ACK 自动安装以下组件:
# - NVIDIA GPU 驱动
# - NVIDIA Container Toolkit
# - NVIDIA Device Plugin
# - GPU 共享组件 (可选)

# 验证 GPU 节点
kubectl get nodes -l accelerator=nvidia-t4
kubectl describe node <gpu-node> | grep -A 10 "Capacity:"
# nvidia.com/gpu: 1

# 验证 GPU Pod
kubectl run gpu-test --image=nvidia/cuda:12.2-base --rm -it \
  --limits='nvidia.com/gpu=1' -- nvidia-smi
```
### 5.3 GPU 调度策略

```yaml
# GPU 节点专用调度
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ai-inference
spec:
  replicas: 3
  selector:
    matchLabels:
      app: inference
  template:
    metadata:
      labels:
        app: inference
    spec:
      # 仅调度到 GPU 节点
      nodeSelector:
        accelerator: nvidia-t4
      # GPU 资源声明
      containers:
        - name: inference
          image: my-inference:v1
          resources:
            limits:
              nvidia.com/gpu: 1       # 使用 1 张 GPU
              cpu: "4"
              memory: 8Gi
            requests:
              nvidia.com/gpu: 1
              cpu: "2"
              memory: 4Gi
          # GPU 设备挂载
          env:
            - name: NVIDIA_VISIBLE_DEVICES
              value: "all"
```

### 5.4 GPU 共享 (MIG / 时间片)

```yaml
# GPU 共享方案 1: vGPU 时间片 (适合推理场景)
# 每张 GPU 可被多个 Pod 共享
apiVersion: v1
kind: Pod
metadata:
  name: small-inference
  annotations:
    # GPU 显存限制
    alibabacloud.com/gpu-memory: "4096"  # 4GB 显存
    # GPU 算力比例 (可选)
    alibabacloud.com/gpu-core-percentage: "50"
spec:
  containers:
    - name: inference
      image: my-inference:v1
      resources:
        limits:
          # 使用 gpu-memory annotation 替代 nvidia.com/gpu
          alibabacloud.com/gpu-memory: "4096"

# GPU 共享方案 2: NVIDIA MIG (A100/H100)
# 将一张 GPU 切分为多个独立实例
# 需要节点池开启 MIG 模式
# 每个 MIG 实例有独立的显存和算力
```

### 5.5 GPU 节点池成本优化

```
策略              适用场景                  节省幅度
──────────────────────────────────────────────────────
竞价实例          可中断的训练任务          60-90%
GPU 共享          推理场景 (小模型)         50-70%
自动伸缩          弹性推理服务              30-50%
混合精度训练      训练任务                  20-40% (时间)
模型量化          推理任务                  50-80% (显存)
```

## 6. 节点池升级策略

### 6.1 K8s 版本升级

```
升级流程:
1. 控制台 → 集群 → 升级 → 选择目标 K8s 版本
2. 控制面先升级 (API Server、etcd、Controller Manager、Scheduler)
3. 节点池逐个升级

节点池升级策略:
├── 自动升级: 跟随集群升级
├── 手动升级: 选择节点池单独升级
└── 维护窗口: 设置可维护时间段
```

### 6.2 节点池滚动升级

```yaml
# 节点升级参数 (控制台配置)
升级策略:
  # 最大不可用比例 (同时升级的节点数)
  maxUnavailable: "25%"
  # 升级顺序: 先创建新节点，再排空旧节点
  strategy: RollingUpdate
  # 是否使用蓝绿升级 (推荐)
  blueGreenStrategy:
    # 先创建新规格节点
    waitNodeReady: true
    # 排空旧节点超时时间
    drainTimeout: 30m
```

### 6.3 节点池蓝绿升级

```
蓝绿升级流程:
Phase 1: 创建绿色节点池
    ├── 复制蓝色节点池配置
    ├── 升级 K8s 版本 / 更新实例规格
    └── 等待新节点 Ready

Phase 2: 流量切换
    ├── 通过标签将工作负载调度到绿色节点池
    ├── 验证业务正常
    └── cordon 蓝色节点池 (不再调度新 Pod)

Phase 3: 排空蓝色节点
    ├── drain 蓝色节点池节点
    ├── Pod 迁移到绿色节点池
    └── 等待所有 Pod 迁移完成

Phase 4: 清理
    ├── 确认绿色节点池运行稳定
    ├── 删除蓝色节点池
    └── 将绿色节点池重命名为原名称
```

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 蓝绿升级 kubectl 操作

# 1. 标记蓝色节点为不可调度
kubectl cordon <blue-node-1> <blue-node-2>

# 2. 逐个排空蓝色节点
kubectl drain <blue-node-1> --ignore-daemonsets --delete-emptydir-data --timeout=30m

# 3. 验证绿色节点池状态
kubectl get nodes -l pool=green -o wide

# 4. 删除蓝色节点池 (通过控制台)
```
### 6.4 节点操作系统升级

```
操作系统升级策略:
├── 就地升级 (In-place)
│   ├── 适用: 小版本更新
│   ├── 优点: 不改变节点
│   └── 风险: 可能影响运行中的 Pod
│
└── 替换升级 (Replace)
    ├── 适用: 大版本升级 / 镜像变更
    ├── 优点: 干净的系统状态
    └── 流程: 创建新节点池 → 迁移工作负载 → 删除旧节点池
```

## 7. 节点池监控与运维

### 7.1 节点池健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池整体状态
kubectl get nodes --show-labels | grep node-pool-id

# 节点资源使用率
kubectl top nodes -l node-pool-id=np-xxxxxxxx

# 节点池事件
kubectl get events --field-selector reason=ScalingUp,involvedObject.kind=Node

# 伸缩活动日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100 | grep -i "scale"
```
### 7.2 节点池告警

```yaml
# 云监控告警规则
# 控制台 → 云监控 → 告警规则 → 创建

推荐告警:
├── 节点池节点数 = 最小值 (已到下限)
├── 节点池节点数 = 最大值 (已到上限)
├── 节点池 Pending Pod > 0 持续 5 分钟
├── 节点 NotReady 持续 3 分钟
├── 节点 CPU/内存 > 85% 持续 5 分钟
└── GPU 温度 > 80°C / GPU 利用率 < 10% (资源浪费)
```

### 7.3 常见问题排查

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 节点池扩容失败 | ECS 库存不足 / 配额用尽 | 切换可用区或申请配额 |
| 节点纳管超时 | 安全组未放行 | 放行 10250/443 端口 |
| Pod 未调度到目标池 | 标签/污点不匹配 | 检查 nodeSelector 和 toleration |
| GPU 驱动安装失败 | 内核版本不兼容 | 切换操作系统或手动安装驱动 |
| ECI 创建失败 | 子网 IP 不足 | 扩展子网 CIDR |
| 缩容不生效 | Pod 有 PDB 限制 | 调整 PodDisruptionBudget |
| 升级中断 | Pod 排空超时 | 增加 drain timeout 或强制排空 |

## 8. 最佳实践

1. **多可用区节点池**：交换机选择至少 2 个可用区，提高容灾能力
2. **实例规格多样化**：配置多个备选规格，提升扩容成功率
3. **合理设置 requests**：Pod requests 之和不超过节点 Capacity 的 80%
4. **GPU 共享**：推理场景使用 GPU 共享，提升利用率
5. **蓝绿升级**：生产环境节点升级使用蓝绿策略，降低风险
6. **竞价实例**：批处理任务使用竞价实例，成本降低 60-90%
7. **定时伸缩**：有明显峰谷的业务配置定时伸缩规则
8. **节点池隔离**：不同 SLA 等级的服务放在不同节点池

---

*本文档描述 ACK 节点池管理与弹性伸缩策略。具体参数以阿里云官方文档为准。*


<!-- risk-assessed -->
