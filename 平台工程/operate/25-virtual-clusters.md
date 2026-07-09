---
title: 55 - 虚拟集群与多租户
description: '| 模式 | 隔离级别 | 资源效率 | 管理复杂度 | 适用场景 |'
summary: '| 模式 | 隔离级别 | 资源效率 | 管理复杂度 | 适用场景 |'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- etcd
- apiserver
- scheduler
- controller-manager
- helm
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 虚拟集群与多租户 是什么
- 如何 虚拟集群与多租户
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- 虚拟集群与多租户
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
- etcd-basics
- policy-basics
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
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../专项技术/
  label: '相关知识域: 专项技术'
- type: domain
  path: ../故障诊断/
  label: '相关知识域: 故障诊断'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 55 - 虚拟集群与多租户

<!-- chunk: 多租户隔离模式 -->
## 多租户隔离模式

| 模式 | 隔离级别 | 资源效率 | 管理复杂度 | 适用场景 |
|-----|---------|---------|-----------|---------|
| 命名空间 | 软隔离 | 高 | 低 | 团队隔离 |
| 虚拟集群 | 强隔离 | 中 | 中 | 多租户平台 |
| 物理集群 | 完全隔离 | 低 | 高 | 安全敏感 |

<!-- chunk: 虚拟集群工具对比 -->
## 虚拟集群工具对比

| 工具 | 架构 | API兼容性 | 成熟度 | 社区 |
|-----|------|---------|-------|------|
| vCluster | 嵌入式控制平面 | 完全 | ⭐⭐⭐⭐⭐ | 活跃 |
| Kamaji | 外部控制平面 | 完全 | ⭐⭐⭐⭐ | 活跃 |
| Cluster API | 独立集群 | 完全 | ⭐⭐⭐⭐⭐ | CNCF |
| Hierarchical Namespaces | 命名空间层次 | 部分 | ⭐⭐⭐ | K8s SIG |

<!-- chunk: vCluster架构 -->
## vCluster架构

| 组件 | 位置 | 功能 |
|-----|------|------|
| syncer | 虚拟集群Pod | 资源同步 |
| kube-apiserver | 虚拟集群Pod | API服务器 |
| [[etcd|etcd]]/SQLite | 虚拟集群Pod | 数据存储 |
| kube-controller-manager | 虚拟集群Pod | 控制器管理 |
| kube-scheduler | 可选 | 调度器 |

<!-- chunk: vCluster安装 -->
## vCluster安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装vCluster CLI
curl -L -o vcluster "https://github.com/loft-sh/vcluster/releases/latest/download/vcluster-linux-amd64"
chmod +x vcluster
sudo mv vcluster /usr/local/bin

# 创建虚拟集群
vcluster create my-vcluster -n host-namespace

# 连接虚拟集群
vcluster connect my-vcluster -n host-namespace

# 使用Helm安装
helm upgrade --install my-vcluster vcluster \
  --repo https://charts.loft.sh \
  --namespace host-namespace \
  --create-namespace
```
<!-- chunk: vCluster配置 -->
## vCluster配置

```yaml
# values.yaml
sync:
  # 同步的资源类型
  pods:
    enabled: true
  services:
    enabled: true
  configmaps:
    enabled: true
  secrets:
    enabled: true
  persistentvolumeclaims:
    enabled: true
  ingresses:
    enabled: true
  
# 控制平面配置
controlPlane:
  distro:
    k8s:
      enabled: true
  statefulSet:
    resources:
      limits:
        cpu: "1"
        memory: 2Gi
      requests:
        cpu: 200m
        memory: 256Mi
    persistence:
      size: 5Gi

# 同步选项
sync:
  toHost:
    pods:
      enabled: true
    services:
      enabled: true
  fromHost:
    nodes:
      enabled: true
    
# 隔离配置
isolation:
  enabled: true
  resourceQuota:
    enabled: true
  limitRange:
    enabled: true
  networkPolicy:
    enabled: true
```

<!-- chunk: Hierarchical Namespaces (HNC) -->
## Hierarchical Namespaces (HNC)

```yaml
# 安装HNC
kubectl apply -f https://github.com/kubernetes-sigs/hierarchical-namespaces/releases/latest/download/default.yaml

# 创建父命名空间
apiVersion: v1
kind: Namespace
metadata:
  name: org-team-a

# 创建子命名空间
apiVersion: hnc.x-k8s.io/v1alpha2
kind: SubnamespaceAnchor
metadata:
  name: dev
  namespace: org-team-a
---
apiVersion: hnc.x-k8s.io/v1alpha2
kind: SubnamespaceAnchor
metadata:
  name: staging
  namespace: org-team-a
```

<!-- chunk: HNC资源继承 -->
## HNC资源继承

```yaml
# 在父命名空间创建资源(自动传播到子命名空间)
apiVersion: v1
kind: ConfigMap
metadata:
  name: team-config
  namespace: org-team-a
  labels:
    hnc.x-k8s.io/inherited-from: org-team-a
data:
  team: team-a
  
# 配置传播规则
apiVersion: hnc.x-k8s.io/v1alpha2
kind: HNCConfiguration
metadata:
  name: config
spec:
  resources:
  - resource: secrets
    mode: Propagate  # Propagate/Remove/Ignore
  - resource: roles
    mode: Propagate
  - resource: rolebindings
    mode: Propagate
  - resource: networkpolicies
    mode: Propagate
```

<!-- chunk: 多租户RBAC策略 -->
## 多租户RBAC策略

```yaml
# 租户管理员角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: tenant-admin
rules:
- apiGroups: [""]
  resources: ["namespaces"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets"]
  verbs: ["*"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets"]
  verbs: ["*"]
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies", "ingresses"]
  verbs: ["*"]
---
# 租户RoleBinding
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-admin-binding
  namespace: tenant-a
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: tenant-admin
subjects:
- kind: Group
  name: tenant-a-admins
  apiGroup: rbac.authorization.k8s.io
```

<!-- chunk: 租户隔离NetworkPolicy -->
## 租户隔离NetworkPolicy

```yaml
# 默认拒绝所有流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: tenant-a
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
---
# 允许同命名空间通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: tenant-a
spec:
  podSelector: {}
  ingress:
  - from:
    - podSelector: {}
  egress:
  - to:
    - podSelector: {}
```

<!-- chunk: 租户ResourceQuota -->
## 租户ResourceQuota

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-quota
  namespace: tenant-a
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "100"
    services: "20"
    secrets: "100"
    configmaps: "100"
    persistentvolumeclaims: "20"
    requests.storage: 100Gi
```

<!-- chunk: ACK多租户方案 -->
## ACK多租户方案

| 功能 | 说明 |
|-----|------|
| ACK One | 多集群统一管理 |
| 弹性配额 | 租户间资源共享 |
| 命名空间配额 | 资源限制 |
| 网络隔离 | Terway NetworkPolicy |
| 日志隔离 | SLS日志隔离 |

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | PSA替代PSP实现租户安全 |
| v1.27 | 资源配额改进 |
| v1.28 | CEL准入策略增强 |
| v1.30 | ValidatingAdmissionPolicy GA |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 平台工程 KUDIG Database — Global MOC
- [[平台工程/README.md|[[Platform Ops Domain (平台运维领域)|Platform Ops Domain (平台运维领域)]]]]
- Domain-9 平台运维 — 开源项目索引
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## See Also

- 23-cli-enhancement-tools
- 24-addons-extensions
- 26-kubectl-plugin-ecosystem
- 99-java-k8s-client-operator-guide

## Related

- [[生态参考/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]

```

<!-- risk-assessed -->
