---
title: ACK/ACR/K8S 内部培训大纲
description: '- "K8s运维培训"'
summary: '- "K8s运维培训"'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- prometheus
- grafana
- flannel
- coredns
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ACK/ACR/K8S 内部培训大纲 是什么
- 如何 ACK/ACR/K8S 内部培训大纲
trigger_keywords:
- ACK
- ACR
- K8S
- 内部培训大纲
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- gpu-scheduling-basics
---



# ACK/ACR/K8S 内部培训大纲

```yaml
---
title: ACK/ACR/K8S 内部培训大纲
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "ACK培训课程"
  - "阿里云Kubernetes培训"
  - "四周学习路径"
  - "内部培训体系"
  - "K8s运维培训"
trigger_keywords:
  - "ACK培训"
  - "ACR培训"
  - "阿里云容器"
  - "Kubernetes培训"
  - "四周计划"
  - "内部培训"
  - "集群生命周期"
  - "安全认证"
reading_level: intermediate
audience:
  - 内部运维工程师
  - 技术支持人员
  - SRE工程师
estimated_read_time: 20min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-05-security-compliance
  - domain-12-cloud-providers
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/inner-one-month-training
  - domain-11-production-operations/topic-learn/inner-training/week-1-ack-acr-lifecycle
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring
id: INNER-TRAINING-INDEX-001
topic: training
type: training-plan
tags: [training, inner-training, ack, acr, k8s, month-1, k8s-1.28-1.33]
---
```

## 概述

本培训大纲为内部运维工程师和技术支持人员设计，覆盖 ACK（阿里云容器服务）、ACR（阿里云容器镜像服务）和 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 三大技术栈，通过 28 天的系统性学习，从基础概念到生产运维，逐步建立完整的云原生运维能力。

培训采用"每日主题学习 + 实操练习"的模式，每天 4-5 小时的学习时间，包含理论阅读（1.5-2h）、实践操作（2-2.5h）和费曼复述（0.5h）三个环节。每周结束有一个自测检验（checkpoint.md）和一个实践项目，确保学习效果可量化验证。

**培训目标**: 完成培训后，能够独立处理 ACK/ACR/K8S 相关的日常运维工单，具备集群管理、安全配置、故障排查和性能优化的基本能力。

- **培训周期**: 28 天（一个月）
- **培训形式**: 每日主题学习 + 实操练习
- **目标受众**: 内部运维工程师、技术支持人员

---

## Week 1: ACK/ACR 基础与集群生命周期 (Day 1-7)

> 目标：掌握 ACK/ACR 服务基础与集群全生命周期管理

| Day | 主题 | 学习目标 |
|:---:|------|---------|
| Day 1 | ACK ACR 管控 SR | 了解 ACK/ACR 服务架构与管控层基本概念 |
| Day 2 | ACK SDK & API | 掌握 ACK SDK 使用与 API 调用方式 |
| Day 3 | ACK ACR 控制台 & 功能 | 熟悉控制台界面与核心功能操作 |
| Day 4 | K8S 新建集群 | 掌握集群创建流程与配置选项 |
| Day 5 | K8S 集群删除 | 理解集群删除流程与注意事项 |
| Day 6 | K8S 集群升级 | 掌握集群版本升级策略与操作步骤 |
| Day 7 | K8S 集群证书 | 理解集群证书管理与更新机制 |

**本周产出**: 能够独立完成集群创建、升级、删除全流程操作

### Day 1-7 核心知识点

```bash
# Day 1: ACK/ACR 服务架构
# - ACK 三种形态: 托管版 / 专有版 / Serverless
# - ACR 两个版本: 个人版 / 企业版
# - 管控层组件: cluster-manager / meta-service / addons

# Day 2: ACK SDK & API
aliyun cs GET /api/v1/clusters                          # 集群列表
aliyun cs POST /api/v1/clusters --body @cluster.json    # 创建集群
aliyun cs GET /clusters/<id>                            # 集群详情
aliyun cs PUT /clusters/<id> --body @upgrade.json       # 升级集群
aliyun cs DELETE /clusters/<id>                         # 删除集群

# Day 3: 控制台功能
# - 集群管理 / 节点池 / 应用管理 / 运维管理
# - ACR: 镜像仓库 / 安全扫描 / 构建配置

# Day 4: 集群创建
# 关键配置: VPC/vSwitch/安全组/实例规格/K8s版本/CNI插件/存储插件
# 预计时间: 10-15 分钟

# Day 5: 集群删除
# 注意事项: 备份数据/清理 LB/释放云盘/检查依赖资源

# Day 6: 集群升级
# 升级策略: 先升级 Master 再升级 Node
# 前置检查: 组件兼容性/资源余量/备份 etcd

# Day 7: 证书管理
# 证书类型: CA/Server/Client/Kubelet
# 更新方式: 自动轮换 (kubelet) / 手动更新 (集群证书)
```

---

## Week 2: 安全认证与监控运维 (Day 8-14)

> 目标：建立集群安全体系与监控运维能力

| Day | 主题 | 学习目标 |
|:---:|------|---------|
| Day 8 | K8S 集群 RBAC | RBAC 权限模型与配置实践 |
| Day 9 | RAM 账号管理 | RAM 账号与 K8S 集成方案 |
| Day 10 | ACK ACR K8S 漏洞 | 常见漏洞类型与防护措施 |
| Day 11 | 风险点识别与防范 | 安全风险评估与最佳实践 |
| Day 12 | K8S 集群审计 | 审计日志配置与分析方法 |
| Day 13 | K8S 集群监控 | 监控体系搭建与告警配置 |
| Day 14 | K8S 集群配额 & License | 资源配额管理与许可证相关 |

**本周产出**: 能够配置集群 RBAC 权限、识别安全风险、搭建基础监控

### Day 8-14 核心知识点

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# Day 8: RBAC 配置
kubectl create role developer --verb=get,list,watch --resource=pods,deployments
kubectl create rolebinding dev-binding --role=developer --user=dev@company.com -n dev
kubectl auth can-i create deployments --as=dev@company.com -n dev

# Day 9: RAM 集成
# 两层权限模型: RAM (云平台) + RBAC (集群内)
aliyun cs POST /clusters/<id>/permissions --body @rbac.json
# RAM 子账号 → ACK 角色 → K8S RBAC 绑定

# Day 10: 漏洞管理
trivy image --severity HIGH,CRITICAL nginx:latest    # 镜像扫描
kubectl get pods -A -o jsonpath='...' | grep true    # 检查特权容器
# CVE 跟踪: 评估 → 缓解 → 修复 → 验证

# Day 11: 安全最佳实践
# - Pod Security Standards: Privileged/Baseline/Restricted
# - NetworkPolicy: 限制 Pod 间网络访问
# - Secret 加密: etcd EncryptionConfiguration

# Day 12: 审计日志
# ACK 审计 → SLS (日志服务)
# 查询: verb:delete AND objectRef.resource:deployments
# 告警: ClusterRoleBinding 创建、Secret 访问、403 错误

# Day 13: 监控体系
# Prometheus + Grafana + Alertmanager
# 关键指标: CPU/Memory/Disk/网络/Pod 状态/etcd 延迟

# Day 14: 配额管理
kubectl describe resourcequota -n <ns>     # 查看配额
kubectl describe limitrange -n <ns>        # 查看限制范围
# License: ACK Pro 版功能授权
```

---

## Week 3: 节点与工作负载管理 (Day 15-21)

> 目标：精通节点管理与工作负载运维

| Day | 主题 | 学习目标 |
|:---:|------|---------|
| Day 15 | Node 节点基础 | 节点概念、状态与管理操作 |
| Day 16 | Node 节点进阶 | 节点维护、标签与调度约束 |
| Day 17 | 节点池基础 | 节点池概念与创建配置 |
| Day 18 | 节点池进阶 | 节点池扩缩容与生命周期管理 |
| Day 19 | Pod 容器组基础 | Pod 生命周期与基本操作 |
| Day 20 | Pod 容器组进阶 | Pod 调度、探针与资源配置 |
| Day 21 | K8S 组件运维 | 核心组件状态检查与故障处理 |

**本周产出**: 能够管理节点池、排查 Pod 问题、维护 K8S 核心组件

### Day 15-21 核心知识点

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# Day 15-16: 节点管理
kubectl get nodes -o wide                        # 节点列表
kubectl label node <node> env=production         # 添加标签
kubectl cordon <node>                            # 禁止调度
kubectl drain <node> --ignore-daemonsets         # 驱逐Pod
kubectl uncordon <node>                          # 恢复调度
kubectl taint nodes <node> dedicated=gpu:NoSchedule  # 添加污点

# Day 17-18: 节点池管理
aliyun cs GET /clusters/<id>/nodepools           # 节点池列表
aliyun cs PUT /clusters/<id>/nodepools/<np-id>   # 修改节点池
# Cluster Autoscaler: 自动扩缩容配置
# 托管节点池: 自动修复/升级

# Day 19-20: Pod 管理
kubectl get pods -A -o wide                      # Pod 列表
kubectl logs <pod> --previous                    # 上次崩溃日志
kubectl describe pod <pod>                       # Pod 详情
kubectl exec -it <pod> -- sh                     # 进入容器
# 探针: livenessProbe/readinessProbe/startupProbe
# 资源: requests/limits/QoS

# Day 21: 组件运维
kubectl get cs                                   # 组件状态
kubectl get pods -n kube-system                  # 系统Pod
# CoreDNS/kube-proxy/CSI/CNI 组件检查与排障
```

---

## Week 4: 网络与存储 (Day 22-28)

> 目标：掌握集群网络架构与存储管理

| Day | 主题 | 学习目标 |
|:---:|------|---------|
| Day 22 | Service 基础 | Service 类型与配置实践 |
| Day 23 | [[Ingress|Ingress]] | Ingress 路由规则与控制器配置 |
| Day 24 | Terway 网络 | Terway CNI 架构与配置 |
| Day 25 | Flannel 网络 | Flannel 网络模型与故障排查 |
| Day 26 | 存储卷创建 & 删除 | PV/PVC 创建与生命周期管理 |
| Day 27 | 存储卷挂载 | 存储挂载方式与最佳实践 |
| Day 28 | 综合复习与实践 | 全流程实操与问题答疑 |

**本周产出**: 能够配置 Service/Ingress、排查网络问题、管理存储卷

### Day 22-28 核心知识点

```bash
# Day 22-23: Service & Ingress
kubectl expose deployment <name> --port=80 --type=LoadBalancer  # 创建Service
kubectl get svc,endpoints                                        # 验证
# Service 类型: ClusterIP/NodePort/LoadBalancer
# Ingress: 域名路由/TLS终止/灰度发布

# Day 24-25: CNI 网络
# Terway (ENIIP): VPC 真实 IP，高性能，支持 NetworkPolicy
# Flannel (VxLAN): Overlay 网络，简单易用
# 排障: DNS → kube-proxy → CNI → 节点网络

# Day 26-27: 存储
kubectl get pv,pvc                               # 存储资源
kubectl get storageclass                         # 存储类
# 动态供应: PVC → StorageClass → CSI → 云盘
# 扩容: patch PVC storage size
# 快照: VolumeSnapshot

# Day 28: 综合复习
# 全流程实操: 创建集群 → 部署应用 → 配置网络 → 挂载存储 → 监控告警
```

---

## 培训主题索引

| 类别 | 包含主题 | 对应 Domain |
|------|---------|-------------|
| ACK/ACR 服务 | 管控 SR、SDK & API、控制台 & 功能 | Domain 17 |
| 集群生命周期 | 新建集群、集群删除、集群升级、集群证书 | Domain 17 |
| 安全认证 | RBAC、RAM 账号、漏洞 & 风险点 | Domain 7 |
| 监控运维 | 集群审计、集群监控、配额 & License | Domain 7/8 |
| 节点管理 | Node 节点、节点池 | Domain 9 |
| 工作负载 | Pod 容器组、K8S 组件运维 | Domain 4 |
| 网络 | Service、Ingress、Terway、Flannel | Domain 5 |
| 存储 | 存储卷创建 & 删除、存储卷挂载 | Domain 6 |

---

## 学习方法论

### 1. 费曼学习法 (每日)
每天学完后用自己的语言复述核心概念，检测理解漏洞。

### 2. 间隔重复 (每周)
每周一回顾上周关键概念，每周五复习本周 10 个核心术语。

### 3. 实践优先
理论 <= 1.5h，实践 >= 2.5h。动手复现是最有效的学习方式。

### 4. 项目驱动
每周完成一个实践项目，将知识串联应用。

---

## 实践项目清单

| # | 项目名称 | 周 | 详情 |
|---|----------|---|------|
| P1 | ACK 集群全生命周期管理 | Week 1 | [p1-ack-cluster-lifecycle.md](./projects/p1-ack-cluster-lifecycle.md) |
| P2 | 安全认证与监控体系搭建 | Week 2 | [p2-security-monitoring-setup.md](./projects/p2-security-monitoring-setup.md) |
| P3 | 节点与工作负载运维实战 | Week 3 | [p3-node-workload-management.md](./projects/p3-node-workload-management.md) |
| P4 | 网络与存储综合实践 | Week 4 | [p4-network-storage-practice.md](./projects/p4-network-storage-practice.md) |
| P5 | 毕业综合实践项目 | Week 4 | [p5-graduation-project.md](./projects/p5-graduation-project.md) |

---

## 延伸阅读

- [ACK 产品文档](https://help.aliyun.com/product/85222.html)
- [ACR 产品文档](https://help.aliyun.com/product/60716.html)
- [Kubernetes 官方文档](https://kubernetes.io/docs/home/)
- [阿里云容器服务最佳实践](https://help.aliyun.com/document_detail/2627792.html)
