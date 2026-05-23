---
title: ACK/ACR/K8S 内部培训知识图谱
description: '├── Week 2: 安全认证与监控运维'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- grafana
- flannel
- coredns
- ingress
- rbac
- networkpolicy
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- All kudig-database users
- ACK learners
- New joiners
estimated_read_time: 5min
intent_queries:
- Kubernetes knowledge system architecture
- ACK/ACR learning path knowledge graph
- Kubernetes core concepts relationship
- Inner training curriculum knowledge map
- Kubernetes week by week learning
trigger_keywords:
- knowledge graph
- knowledge map
- concept relationship
- curriculum
- ACK
- ACR
- Kubernetes
- week
- core concept
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
related_domains:
- domain-12-cloud-providers
- domain-01-cluster-fundamentals
related_topics:
- reading-sequence
- commands-cheatsheet
created: "2026-05-23"
---

# ACK/ACR/K8S 内部培训知识图谱

> 按周组织的核心知识体系，用于系统回顾和查漏补缺

---

## 总览

```
ACK/ACR/K8S 内部培训
├── Week 1: ACK/ACR 基础与集群生命周期
├── Week 2: 安全认证与监控运维
├── Week 3: 节点与工作负载管理
└── Week 4: 网络与存储
```

---

## Week 1: ACK/ACR 基础与集群生命周期

```
ACK/ACR 管控
├── ACK 服务架构
│   ├── 托管版 (ManagedKubernetes)
│   ├── 专有版 (DedicatedKubernetes)
│   └── Serverless (ASK)
├── ACR 镜像服务
│   ├── 个人版 (免费)
│   └── 企业版 (ACR EE)
└── 管控层 SR
    ├── API Server 入口
    └── 区域与可用区

SDK & API
├── OpenAPI (ROA 风格)
│   ├── RESTful 路径设计
│   └── 签名认证
├── aliyun CLI
│   ├── 安装与配置
│   └── cs 子命令
└── Python SDK
    ├── alibabacloud-cs20151215
    └── Client 初始化

控制台操作
├── 集群管理入口
├── 节点池管理
├── 工作负载视图
└── 日志与监控入口

集群生命周期
├── 创建
│   ├── VPC/vSwitch 规划
│   ├── CIDR 设计 (Pod/Service/VPC)
│   ├── CNI 选择 (Terway/Flannel)
│   └── Addon 配置
├── 删除
│   ├── 资源清理顺序
│   ├── SLB/EIP 关联资源
│   └── retain_all_resources 选项
├── 升级
│   ├── 管控面升级
│   ├── 节点替换升级
│   └── 版本兼容性检查
└── 证书管理
    ├── 集群 CA 证书
    ├── kubeconfig 证书
    └── 证书轮转
```

---

## Week 2: 安全认证与监控运维

```
RBAC 权限模型
├── Role / ClusterRole
│   ├── apiGroups
│   ├── resources
│   └── verbs
├── RoleBinding / ClusterRoleBinding
├── ServiceAccount
└── 内置 ClusterRole
    ├── cluster-admin
    ├── admin
    ├── edit
    └── view

RAM 账号集成
├── RAM 用户 → K8S 权限映射
├── grant_permissions API
├── 双层权限模型
│   ├── RAM 层: 云资源访问
│   └── RBAC 层: K8S 资源访问
└── kubeconfig 生成

安全防护
├── 漏洞管理
│   ├── 运行时漏洞
│   ├── 镜像漏洞
│   └── K8S CVE
├── 风险防范
│   ├── Pod Security Standards (PSS)
│   ├── 特权容器风险
│   └── 安全基线检查
└── 审计日志
    ├── 开启审计
    ├── SLS 投递
    └── 审计查询语法

监控运维
├── Prometheus/ARMS
│   ├── 指标采集
│   ├── PromQL 查询
│   ├── Grafana Dashboard
│   └── 告警规则 (PrometheusRule)
└── 配额管理
    ├── ResourceQuota
    │   ├── 计算资源配额
    │   ├── 对象数量配额
    │   └── 存储配额
    └── LimitRange
        ├── 默认值
        └── 最大/最小限制
```

---

## Week 3: 节点与工作负载管理

```
节点管理
├── 节点基础
│   ├── 节点状态 (Ready/NotReady)
│   ├── 节点信息 (capacity/allocatable)
│   └── 节点条件 (conditions)
├── 节点进阶
│   ├── Labels (标签)
│   ├── Taints & Tolerations (污点/容忍)
│   └── cordon / drain / uncordon
└── 节点池
    ├── 托管节点池 vs 自管理节点池
    ├── 创建与配置
    ├── 手动扩缩容
    ├── Cluster Autoscaler
    └── 节点池架构设计

工作负载 (Pod)
├── Pod 基础
│   ├── Pod 生命周期 (Phase)
│   ├── Container States
│   ├── Init Container
│   ├── Sidecar 模式
│   └── restartPolicy
├── Pod 进阶
│   ├── 调度策略
│   │   ├── nodeSelector
│   │   ├── nodeAffinity (required/preferred)
│   │   ├── podAffinity / podAntiAffinity
│   │   └── tolerations
│   ├── 健康探针
│   │   ├── startupProbe
│   │   ├── livenessProbe
│   │   └── readinessProbe
│   └── 资源管理
│       ├── requests (调度依据)
│       └── limits (运行上限)
└── K8S 组件运维
    ├── 管控面组件 (托管版由阿里云维护)
    ├── CoreDNS
    ├── kube-proxy
    ├── CNI 插件 (Terway/Flannel)
    ├── CSI 插件
    └── Addon 管理
```

---

## Week 4: 网络与存储

```
网络
├── Service
│   ├── ClusterIP (集群内)
│   ├── NodePort (节点端口)
│   ├── LoadBalancer (SLB 集成)
│   ├── Headless (DNS 直连)
│   └── ExternalName (CNAME)
├── Ingress
│   ├── 路由规则 (域名/路径)
│   ├── Nginx Ingress Controller
│   ├── ALB Ingress Controller
│   ├── TLS 终止
│   └── 灰度发布 (Canary)
├── Terway CNI
│   ├── VPC 模式
│   ├── ENI 模式
│   ├── ENIIP 模式 (推荐)
│   ├── NetworkPolicy 支持
│   └── Pod IP = VPC IP
└── Flannel CNI
    ├── VxLAN 封装
    ├── Pod CIDR 分配
    ├── 性能 (封装开销)
    └── 不支持 NetworkPolicy

存储
├── 概念
│   ├── PersistentVolume (PV)
│   ├── PersistentVolumeClaim (PVC)
│   ├── StorageClass (动态供给)
│   └── 访问模式 (RWO/ROX/RWX)
├── 阿里云存储类型
│   ├── 云盘 SSD (alicloud-disk-ssd) — RWO
│   ├── 云盘高效 (alicloud-disk-efficiency) — RWO
│   ├── NAS (alicloud-nas) — RWX
│   └── OSS (alicloud-oss) — ROX
├── 挂载方式
│   ├── PVC
│   ├── emptyDir
│   ├── hostPath
│   ├── configMap / secret
│   └── subPath
├── 回收策略
│   ├── Delete (删除底层存储)
│   └── Retain (保留底层存储)
└── 扩容
    ├── allowVolumeExpansion
    └── kubectl patch pvc
```

---

## 交叉知识关联

| 主题 A | 主题 B | 关联说明 |
|--------|--------|---------|
| RBAC | RAM | 双层权限模型 |
| 节点池 Taint | Pod Toleration | 调度配合 |
| Service | kube-proxy | 流量转发实现 |
| Ingress | Service | Ingress 后端指向 Service |
| Terway | VPC/vSwitch | Pod IP 来自 VPC |
| PVC | StorageClass | 动态供给链路 |
| Prometheus | Pod 探针 | 都是健康检测，层次不同 |
| ResourceQuota | LimitRange | 配额 + 默认值 |

## Related

- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
