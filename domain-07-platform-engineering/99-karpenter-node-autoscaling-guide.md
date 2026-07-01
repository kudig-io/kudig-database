---
title: Karpenter 节点自动扩展实践指南
description: '# Karpenter 节点自动扩展实践指南'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- prometheus
- cilium
- helm
- operator
- gpu
- nvidia
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Karpenter 节点自动扩展实践指南 是什么
- 如何 Karpenter 节点自动扩展实践指南
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- Karpenter
- 节点自动扩展实践指南
- production
- operations
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
- prometheus-basics
- cilium-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md
  label: '故障树: node'
created: "2026-05-23"
---

# Karpenter 节点自动扩展实践指南

> **适用版本**: Karpenter v1.3  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、Karpenter vs Cluster Autoscaler](#一karpenter-vs-cluster-autoscaler)
- [二、安装部署](#二安装部署)
- [三、NodePool 配置](#三nodepool-配置)
- [四、EC2NodeClass 云集成](#四ec2nodeclass-云集成)
- [五、多节点类型策略](#五多节点类型策略)
- [六、Spot 实例优化](#六spot-实例优化)
- [七、整合与漂移](#七整合与漂移)
- [八、成本优化最佳实践](#八成本优化最佳实践)
- [九、监控与告警](#九监控与告警)

---

<!-- chunk: 一、Karpenter vs Cluster Autoscaler -->## 一、Karpenter vs Cluster Autoscaler

```
Cluster Autoscaler              Karpenter
├── 按节点组扩容                ├── 按工作负载需求直接选型
├── 预设实例类型                ├── 动态选择最优实例类型
├── 扩容延迟 (分钟级)           ├── 扩容延迟 (秒级)
├── 需维护节点组配置            ├── 无需节点组
├── 云厂商适配有限              ├── 原生 AWS/Azure/GCP 集成
└── 基于 Pod request 判断       └── 基于 Pod + 调度约束判断
```

| 维度 | Cluster Autoscaler | Karpenter |
|:---|:---|:---|
| **架构** | 基于节点组 (ASG/MIG) | 无节点组，直接创建实例 |
| **扩容速度** | 2-5 分钟 (ASG 启动) | 30-60 秒 (直接创建) |
| **实例选择** | 节点组中预定义 | 动态计算最优实例 |
| **多实例类型** | 需多个节点组 | 单 NodePool 支持多种 |
| **Spot 支持** | 需单独节点组 | 原生混合模式 |
| **整合/漂移** | 不支持 | 自动节点替换优化 |
| **GPU 支持** | 需专用节点组 | NodePool 自动处理 |
| **学习曲线** | 低 | 中等 |
| **云厂商** | 全支持 | AWS (最成熟), Azure (预览), GCP (开发中) |

---

<!-- chunk: 二、安装部署 -->## 二、安装部署

## 2.1 AWS 前置准备

```bash
# 创建 IAM 角色 (OIDC 联邦)
export CLUSTER_NAME=production
export AWS_ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
export AWS_REGION=us-east-1

# 允许 Karpenter 创建 EC2 实例
cat > karpenter-policy.json <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "ec2:CreateLaunchTemplate",
        "ec2:CreateFleet",
        "ec2:RunInstances",
        "ec2:CreateTags",
        "iam:PassRole",
        "ec2:TerminateInstances",
        "ec2:DescribeInstances",
        "ec2:DescribeSecurityGroups",
        "ec2:DescribeSubnets",
        "ec2:DescribeInstanceTypes",
        "ec2:DescribeInstanceTypeOfferings",
        "ec2:DescribeAvailabilityZones",
        "ec2:DescribeSpotPriceHistory",
        "pricing:GetProducts"
      ],
      "Resource": "*"
    }
  ]
}
EOF

aws iam create-policy \
  --policy-name KarpenterControllerPolicy \
  --policy-document file://karpenter-policy.json
```

## 2.2 Helm 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
helm repo add karpenter https://charts.karpenter.sh
helm repo update

helm install karpenter oci://public.ecr.aws/karpenter/karpenter \
  --namespace karpenter \
  --create-namespace \
  --version 1.3.0 \
  --set serviceAccount.annotations."eks\.amazonaws\.com/role-arn"="arn:aws:iam::${AWS_ACCOUNT_ID}:role/KarpenterControllerRole" \
  --set settings.clusterName=${CLUSTER_NAME} \
  --set settings.interruptionQueue=${CLUSTER_NAME} \
  --set controller.resources.requests.cpu=100m \
  --set controller.resources.requests.memory=100Mi \
  --set controller.resources.limits.cpu=1 \
  --set controller.resources.limits.memory=1Gi
```

## 2.3 验证安装

```bash
kubectl get pods -n karpenter
kubectl logs -n karpenter -l app.kubernetes.io/name=karpenter
```

---

<!-- chunk: 三、NodePool 配置 -->## 三、NodePool 配置

## 3.1 基础 NodePool

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: default
spec:
  template:
    spec:
      requirements:
      # 实例类型偏好
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["m6i.large", "m6i.xlarge", "m6i.2xlarge", "m6g.large", "m6g.xlarge"]
      
      # 容量类型 (按需 + Spot)
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot", "on-demand"]
      
      # 架构
      - key: kubernetes.io/arch
        operator: In
        values: ["amd64", "arm64"]
      
      # 操作系统
      - key: kubernetes.io/os
        operator: In
        values: ["linux"]
      
      # 区域分布
      - key: topology.kubernetes.io/zone
        operator: In
        values: ["us-east-1a", "us-east-1b", "us-east-1c"]
      
      # 节点标签
      nodeClassRef:
        group: karpenter.k8s.aws
        kind: EC2NodeClass
        name: default
      
      # 污点 (可选)
      taints:
      - key: workload-type
        value: general
        effect: NoSchedule
      
      # 启动污点 (Karpenter 专用)
      startupTaints:
      - key: node.cilium.io/agent-not-ready
        value: "true"
        effect: NoExecute
  
  # 节点过期 (强制轮换)
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 1m
    expireAfter: 720h  # 30 天
    budgets:
    - nodes: "10%"
  
  # 资源限制
  limits:
    cpu: 1000
    memory: 1000Gi
  
  # 权重 (优先级)
  weight: 10
```

## 3.2 专用 GPU NodePool

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: gpu
spec:
  template:
    spec:
      requirements:
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["g5.xlarge", "g5.2xlarge", "p4d.24xlarge"]
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["on-demand"]  # GPU Spot 不稳定
      - key: nvidia.com/gpu.present
        operator: In
        values: ["true"]
      nodeClassRef:
        group: karpenter.k8s.aws
        kind: EC2NodeClass
        name: gpu
      taints:
      - key: nvidia.com/gpu
        value: "true"
        effect: NoSchedule
  limits:
    nvidia.com/gpu: 100
```

---

<!-- chunk: 四、EC2NodeClass 云集成 -->## 四、EC2NodeClass 云集成

```yaml
apiVersion: karpenter.k8s.aws/v1
kind: EC2NodeClass
metadata:
  name: default
spec:
  # AMI 选择
  amiFamily: AL2023  # AL2 | AL2023 | Bottlerocket | Ubuntu | Windows2019 | Windows2022
  
  # 子网选择
  subnetSelectorTerms:
  - tags:
      karpenter.sh/discovery: production
  
  # 安全组
  securityGroupSelectorTerms:
  - tags:
      karpenter.sh/discovery: production
  
  # IAM 角色
  role: KarpenterNodeRole-production
  
  # 块存储
  blockDeviceMappings:
  - deviceName: /dev/xvda
    ebs:
      volumeSize: 100Gi
      volumeType: gp3
      iops: 10000
      encrypted: true
      kmsKeyID: alias/aws/ebs
      deleteOnTermination: true
  
  # 元数据选项
  metadataOptions:
    httpEndpoint: enabled
    httpProtocolIPv6: disabled
    httpPutResponseHopLimit: 1
    httpTokens: required  # IMDSv2
  
  # 详细监控
  detailedMonitoring: true
  
  # 关联性
  associatePublicIPAddress: false
```

---

<!-- chunk: 五、多节点类型策略 -->## 五、多节点类型策略

## 5.1 工作负载匹配

```yaml
# 通用工作负载
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: general
spec:
  template:
    spec:
      requirements:
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["m6i.large", "m6i.xlarge", "m6i.2xlarge"]
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot", "on-demand"]
  weight: 20
---
# 内存密集型
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: memory-optimized
spec:
  template:
    spec:
      requirements:
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["r6i.xlarge", "r6i.2xlarge", "r6i.4xlarge"]
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["on-demand"]
  weight: 10
---
# 计算密集型
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: compute-optimized
spec:
  template:
    spec:
      requirements:
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["c6i.xlarge", "c6i.2xlarge", "c6i.4xlarge"]
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot"]
  weight: 10
```

## 5.2 Pod 节点选择

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: memory-app
spec:
  template:
    spec:
      nodeSelector:
        karpenter.sh/nodepool: memory-optimized
      containers:
      - name: app
        resources:
          requests:
            memory: "16Gi"
            cpu: "4"
```

---

<!-- chunk: 六、Spot 实例优化 -->## 六、Spot 实例优化

## 6.1 中断处理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
# 启用中断处理 (节点终止前优雅驱逐)
helm upgrade karpenter oci://public.ecr.aws/karpenter/karpenter \
  --namespace karpenter \
  --set settings.interruptionQueue=${CLUSTER_NAME}
```

## 6.2 Pod 优雅终止

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spot-workload
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 120  # 2 分钟优雅终止
      containers:
      - name: app
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 30 && curl -X POST localhost:8080/drain"]
```

## 6.3 Spot 占比控制

```yaml
spec:
  template:
    spec:
      requirements:
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot", "on-demand"]
  disruption:
    budgets:
    # 同时中断的节点不超过 10%
    - nodes: "10%"
```

---

<!-- chunk: 七、整合与漂移 -->## 七、整合与漂移

## 7.1 自动整合

```yaml
spec:
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 1m
```

| 策略 | 行为 | 适用场景 |
|:---|:---|:---|
| WhenEmpty | 仅删除空节点 | 保守策略 |
| WhenUnderutilized | 重新调度以合并 | 成本优化 |
| Never | 不自动整合 | 特殊场景 |

## 7.2 漂移检测

```
漂移触发条件:
1. AMI 更新
2. NodeClass 配置变更
3. 实例类型不再满足要求
4. 节点过期 (expireAfter)

Karpenter 行为:
1. 标记节点为待替换
2. 创建新节点
3. 驱逐 Pod 至新节点
4. 终止旧节点
```

---

<!-- chunk: 八、成本优化最佳实践 -->## 八、成本优化最佳实践

| 实践 | 节省效果 | 配置 |
|:---|:---|:---|
| Spot 实例 | 60-90% | capacity-type: spot |
| Graviton (ARM) | 20-40% | arch: arm64 |
| 自动整合 | 10-30% | consolidationPolicy: WhenUnderutilized |
| 节点过期轮换 | 避免老旧 | expireAfter: 720h |
|  rightsizing | 匹配工作负载 | 精确的资源 requests |

## 成本标签

```yaml
spec:
  tags:
    Environment: production
    Team: platform
    CostCenter: infrastructure
    ManagedBy: karpenter
```

---

<!-- chunk: 九、监控与告警 -->## 九、监控与告警

## 9.1 Prometheus Metrics

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: karpenter-metrics
  namespace: monitoring
spec:
  namespaceSelector:
    matchNames:
      - karpenter
  selector:
    matchLabels:
      app.kubernetes.io/name: karpenter
  endpoints:
  - port: http-metrics
    interval: 30s
```

## 9.2 关键告警

```yaml
- alert: KarpenterNodeLaunchFailed
  expr: rate(karpenter_cloudprovider_errors_total[5m]) > 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "Karpenter 节点启动失败"

- alert: KarpenterConsolidationBlocked
  expr: karpenter_nodes_consolidation_blocked > 0
  for: 15m
  labels:
    severity: warning
  annotations:
    summary: "节点整合被长期阻塞"

- alert: HighSpotInterruptionRate
  expr: rate(karpenter_nodeclaims_terminated[1h]) > 0.1
  for: 10m
  labels:
    severity: info
  annotations:
    summary: "Spot 实例中断率较高"

- alert: KarpenterNodePoolNearLimit
  expr: |
    karpenter_nodepool_usage / karpenter_nodepool_limit > 0.8
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "NodePool 资源接近上限"
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [Karpenter 官方文档](https://karpenter.sh/docs/)
- [Karpenter GitHub](https://github.com/aws/karpenter-provider-aws)
- [AWS Karpenter 最佳实践](https://aws.github.io/aws-eks-best-practices/karpenter/)
- [Cluster Autoscaler 对比](https://karpenter.sh/docs/concepts/nodepools/)
- [Karpenter 成本优化](https://karpenter.sh/docs/getting-started/getting-started-with-karpenter/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-11-production-operations KUDIG Database — Global MOC
- [[domain-11-production-operations/README.md|Domain 11: 生产环境运维最佳实践 ([[Production Operations|Production Operations]]ns Best Practices|Production Operations Best Practices]])]]
- Domain-18 生产运维 — 开源项目索引
- [[domain-01-cluster-fundamentals/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 09-软件物料清单

## See Also

- 99-finops-cost-optimization-guide
- 99-greenops-sustainable-computing-guide
- 99-keda-event-driven-autoscaling-guide
- 99-kubernetes-deployment-patterns-architecture

## Related

- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index.md|Node 知识图谱索引]]

```