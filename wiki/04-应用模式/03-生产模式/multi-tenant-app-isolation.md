---
title: "多租户应用隔离模式"
description: "生产级多租户隔离：Namespace 隔离、ResourceQuota、NetworkPolicy、RBAC 与共享服务 vs 独占架构设计"
summary: "覆盖 Kubernetes 多租户应用隔离的完整实践，包括 Namespace 级隔离策略、ResourceQuota/LimitRange 资源管控、NetworkPolicy 网络隔离、RBAC 权限模型、共享服务与独占资源的架构权衡，以及租户自助服务平台设计。"
category: 应用模式
tags:
- patterns
- multi-tenant
- isolation
- namespace
- networkpolicy
- rbac
- resourcequota
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 应用开发者
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- "K8s 多租户隔离方案怎么设计"
- "Namespace 隔离和 NetworkPolicy 如何配合"
- "多租户 ResourceQuota 和 RBAC 最佳实践"
trigger_keywords:
- 多租户
- 隔离
- Namespace
- NetworkPolicy
- ResourceQuota
- RBAC
prerequisites:
- kubectl-basics
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

# 多租户应用隔离模式

> **适用范围**: Kubernetes v1.28–v1.32 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

## 概述

多租户（Multi-tenancy）是 Kubernetes 平台化运营的核心挑战。当多个团队、多个业务线、甚至多个外部客户共享同一集群时，隔离是安全与稳定的基石。没有隔离的多租户集群，一个团队的资源滥用可以拖垮所有其他团队；一个误配置的 NetworkPolicy 可以让租户 A 访问租户 B 的数据库；一个过宽的 RBAC 权限可以让开发者删除生产 Deployment。

本文覆盖 Kubernetes 多租户隔离的四个维度：逻辑隔离（Namespace）、资源隔离（ResourceQuota/LimitRange）、网络隔离（NetworkPolicy）、权限隔离（RBAC），并讨论共享服务与独占资源的架构权衡。相关内容可参见 [[application-security-hardening]]、[[resource-qos-rightsizing]]、[[scheduling-topology-patterns]]。

---

## 模式定义与适用场景

### 隔离模型对比

| 隔离级别 | 实现方式 | 隔离强度 | 资源效率 | 运维复杂度 | 适用场景 |
|---------|---------|---------|---------|-----------|---------|
| **Namespace 隔离** | Namespace + RBAC + Quota | 中 | 高 | 低 | 内部多团队 |
| **网络隔离** | NetworkPolicy + Service Mesh | 中高 | 高 | 中 | 安全敏感业务 |
| **节点隔离** | Taint/Toleration + 专用节点池 | 高 | 中 | 中 | 合规/性能敏感 |
| **集群隔离** | 独立集群 per 租户 | 极高 | 低 | 高 | 外部客户/强合规 |
| **vCluster** | 虚拟集群（嵌套控制平面） | 高 | 中高 | 中 | 平台即服务 |

### 租户类型与隔离需求

| 租户类型 | 信任级别 | 隔离需求 | 典型方案 |
|---------|---------|---------|---------|
| 内部开发团队 | 高 | Namespace + RBAC | 共享集群 |
| 内部业务线 | 中高 | Namespace + NetworkPolicy + Quota | 共享集群 + 节点池 |
| 合作伙伴 | 中 | 网络隔离 + 资源限制 | 专用节点池 |
| 外部客户（SaaS） | 低 | 集群级或 vCluster | 独立集群/vCluster |
| 监管合规（金融/医疗） | 极低 | 物理隔离 | 独立集群 + 加密 |

---

## 架构设计

### 多租户隔离分层模型

```
┌─────────────────────────────────────────────────────────────┐
│                    平台管理层                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ 租户自助  │  │ 配额管理  │  │ 审批流程  │  │ 成本分摊  │   │
│  │ Portal   │  │ Service  │  │ Workflow │  │ FinOps   │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    隔离控制层                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ RBAC     │  │ Resource │  │ Network  │  │ Admission│   │
│  │ Policies │  │ Quotas   │  │ Policies │  │ Webhooks │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    租户工作空间                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ tenant-a     │  │ tenant-b     │  │ tenant-c     │      │
│  │ (电商团队)   │  │ (数据团队)   │  │ (AI 团队)    │      │
│  │              │  │              │  │              │      │
│  │ Deployments  │  │ Deployments  │  │ Deployments  │      │
│  │ Services     │  │ Services     │  │ Services     │      │
│  │ ConfigMaps   │  │ ConfigMaps   │  │ ConfigMaps   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                    共享服务层                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ 日志平台  │  │ 监控平台  │  │ 服务网格  │  │ CI/CD    │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 共享 vs 独占决策矩阵

| 组件 | 共享 | 独占 | 决策依据 |
|------|------|------|---------|
| 控制平面 | 共享 | 强隔离时独占 | 租户信任级别 |
| 节点 | 共享（默认） | 性能/合规时独占 | SLA 要求 |
| 存储 | 共享 StorageClass | 数据敏感时独占 PV | 合规要求 |
| 网络 | 共享 CNI + NetworkPolicy | 强隔离时独立 VPC | 安全等级 |
| 日志/监控 | 共享平台 + 租户标签 | 合规时独立实例 | 数据主权 |
| Ingress | 共享 Gateway | 高流量时独占 | 性能隔离 |
| 数据库 | 共享实例 + 逻辑隔离 | 高负载时独占 | 性能/合规 |

---

## K8s 实现

### Namespace + ResourceQuota + LimitRange

```yaml
# 🟡 中风险：创建租户 Namespace 和配额
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-ecommerce
  labels:
    kudig.io/tenant: "ecommerce-team"
    kudig.io/tier: "standard"
    # 启用 NetworkPolicy 默认拒绝
    network-policy: enabled
    # 启用 Pod Security Standards
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
  annotations:
    kudig.io/tenant-owner: "ecommerce-lead@example.com"
    kudig.io/cost-center: "CC-1001"
    kudig.io/created-by: "platform-automation"
---
# 资源配额：限制租户总资源使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-ecommerce-quota
  namespace: tenant-ecommerce
spec:
  hard:
    # 计算资源
    requests.cpu: "16"
    requests.memory: "32Gi"
    limits.cpu: "32"
    limits.memory: "64Gi"
    # 存储
    requests.storage: "100Gi"
    persistentvolumeclaims: "10"
    # 对象数量
    pods: "50"
    services: "20"
    secrets: "30"
    configmaps: "30"
    # 特定资源
    requests.nvidia.com/gpu: "0"  # 标准租户无 GPU
    services.loadbalancers: "2"
    services.nodeports: "0"  # 禁止 NodePort
  scopes:
    - NotTerminating  # 只限制非临时 Pod
---
# LimitRange：限制单个 Pod/Container 的资源范围
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-ecommerce-limits
  namespace: tenant-ecommerce
spec:
  limits:
    # 容器级限制
    - type: Container
      default:  # 默认 limits（未设置时自动注入）
        cpu: "1"
        memory: "1Gi"
      defaultRequest:  # 默认 requests
        cpu: "100m"
        memory: "128Mi"
      max:  # 单容器最大
        cpu: "8"
        memory: "16Gi"
      min:  # 单容器最小
        cpu: "10m"
        memory: "16Mi"
    # Pod 级限制
    - type: Pod
      max:
        cpu: "16"
        memory: "32Gi"
    # PVC 限制
    - type: PersistentVolumeClaim
      max:
        storage: "50Gi"
      min:
        storage: "1Gi"
```

### NetworkPolicy 网络隔离

```yaml
# 🟡 中风险：NetworkPolicy 配置不当可能阻断合法流量
# 默认拒绝所有入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: tenant-ecommerce
spec:
  podSelector: {}  # 匹配所有 Pod
  policyTypes:
    - Ingress
  # 无 ingress 规则 = 拒绝所有入站
---
# 默认拒绝所有出站流量（可选，更严格）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: tenant-ecommerce
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    # 允许 DNS 解析
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
---
# 允许同租户内部通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-tenant-internal
  namespace: tenant-ecommerce
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kudig.io/tenant: "ecommerce-team"
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kudig.io/tenant: "ecommerce-team"
---
# 允许访问共享服务（监控、日志）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-shared-services
  namespace: tenant-ecommerce
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    # Prometheus 抓取
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: monitoring
      ports:
        - protocol: TCP
          port: 9090
    # 日志收集
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: logging
      ports:
        - protocol: TCP
          port: 3100
    # 外部 HTTPS（API 调用）
    - to:
        - ipBlock:
            cidr: 0.0.0.0/0
            except:
              - 10.0.0.0/8      # 禁止访问内网其他租户
              - 172.16.0.0/12
              - 192.168.0.0/16
      ports:
        - protocol: TCP
          port: 443
---
# 允许 Ingress Gateway 入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-ingress-gateway
  namespace: tenant-ecommerce
spec:
  podSelector:
    matchLabels:
      expose: "true"  # 只有标记 expose=true 的 Pod 接收外部流量
  policyTypes:
    - Ingress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: istio-system
          podSelector:
            matchLabels:
              istio: ingressgateway
      ports:
        - protocol: TCP
          port: 8080
```

### RBAC 租户权限模型

```yaml
# 🟡 中风险：RBAC 配置决定租户权限边界
# 租户管理员角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-admin
  namespace: tenant-ecommerce
rules:
  # 工作负载管理
  - apiGroups: ["apps"]
    resources: ["deployments", "statefulsets", "replicasets", "daemonsets"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  # Pod 操作
  - apiGroups: [""]
    resources: ["pods", "pods/log", "pods/exec"]
    verbs: ["get", "list", "watch", "create", "delete"]
  # 配置管理
  - apiGroups: [""]
    resources: ["configmaps", "secrets", "services", "persistentvolumeclaims"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  # 网络
  - apiGroups: ["networking.k8s.io"]
    resources: ["ingresses", "networkpolicies"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  # HPA
  - apiGroups: ["autoscaling"]
    resources: ["horizontalpodautoscalers"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  # 批处理
  - apiGroups: ["batch"]
    resources: ["jobs", "cronjobs"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  # 禁止：RBAC 修改、Namespace 删除、ResourceQuota 修改
---
# 租户开发者角色（只读 + 日志）
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-developer
  namespace: tenant-ecommerce
rules:
  - apiGroups: ["", "apps", "batch"]
    resources: ["pods", "pods/log", "deployments", "services", "configmaps", "jobs"]
    verbs: ["get", "list", "watch"]
  - apiGroups: [""]
    resources: ["pods/exec"]
    verbs: ["create"]  # 允许 exec 调试
  - apiGroups: [""]
    resources: ["events"]
    verbs: ["get", "list"]
---
# 绑定租户管理员
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: ecommerce-team-admin
  namespace: tenant-ecommerce
subjects:
  - kind: Group
    name: "ecommerce-team-leads"  # OIDC Group
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: tenant-admin
  apiGroup: rbac.authorization.k8s.io
```

---

## 生产配置示例

### Admission Webhook 强制隔离策略

```yaml
# 🟡 中风险：Admission Webhook 拦截不合规的资源创建
apiVersion: v1
kind: ConfigMap
metadata:
  name: tenant-isolation-policy
  namespace: platform-system
data:
  policy.yaml: |
    # 多租户准入策略
    policies:
      # 强制所有 Pod 设置 resources
      - name: require-resource-limits
        action: deny
        condition: "container.resources.limits == nil"
        message: "所有容器必须设置 resource limits"
      
      # 禁止使用 hostNetwork/hostPID
      - name: deny-host-namespaces
        action: deny
        condition: "pod.spec.hostNetwork == true || pod.spec.hostPID == true"
        message: "租户 Pod 禁止使用 hostNetwork/hostPID"
      
      # 禁止 privileged 容器
      - name: deny-privileged
        action: deny
        condition: "container.securityContext.privileged == true"
        message: "禁止特权容器"
      
      # 强制镜像来源
      - name: restrict-image-registry
        action: deny
        condition: "!container.image.startsWith('registry.internal/')"
        message: "只允许使用内部镜像仓库"
        exceptions:
          namespaces: ["platform-system", "kube-system"]
      
      # 强制标签
      - name: require-tenant-labels
        action: deny
        condition: "metadata.labels['kudig.io/tenant'] == nil"
        message: "所有资源必须包含租户标签"
```

### 租户成本追踪

```yaml
# 🟢 低风险：标签用于成本分摊
apiVersion: v1
kind: ConfigMap
metadata:
  name: cost-allocation-config
  namespace: platform-system
data:
  allocation.yaml: |
    # 成本分摊规则
    rules:
      # 按 Namespace 标签分摊
      dimension: namespace
      label_key: kudig.io/cost-center
      
      # 共享服务成本分摊
      shared_services:
        - name: monitoring
          allocation: proportional  # 按资源使用比例
        - name: logging
          allocation: per-namespace  # 按 Namespace 数量均摊
        - name: ingress-gateway
          allocation: per-request   # 按请求量分摊
      
      # 报告
      reporting:
        interval: daily
        format: csv
        destination: s3://finops-reports/
        include:
          - cpu_usage_hours
          - memory_usage_gib_hours
          - storage_gib_hours
          - network_egress_gb
          - gpu_usage_hours
```

---

## 运维要点

### 租户管理操作

```bash
# 🟢 低风险：查看租户资源使用情况
kubectl top pods -n tenant-ecommerce --sort-by=cpu
kubectl describe resourcequota -n tenant-ecommerce

# 🟢 低风险：检查 NetworkPolicy 是否生效
kubectl get networkpolicies -n tenant-ecommerce
kubectl exec -n tenant-ecommerce deploy/web-app -- \
  wget -qO- --timeout=3 http://tenant-data.svc:8080 2>&1 || echo "BLOCKED (expected)"

# 🟢 低风险：查看租户 RBAC 权限
kubectl auth can-i create deployments -n tenant-ecommerce --as=system:serviceaccount:tenant-ecommerce:developer
kubectl auth can-i delete namespaces -n tenant-ecommerce --as=system:serviceaccount:tenant-ecommerce:developer

# 🟡 中风险：调整租户配额
kubectl patch resourcequota tenant-ecommerce-quota -n tenant-ecommerce \
  --type merge -p '{"spec":{"hard":{"requests.cpu":"24","requests.memory":"48Gi"}}}'

# 🔴 高风险：删除租户 Namespace（所有资源将被删除）
kubectl delete namespace tenant-ecommerce
```

### 隔离验证测试

| 测试项 | 方法 | 预期结果 |
|--------|------|---------|
| 网络隔离 | 从 tenant-a Pod curl tenant-b Service | 连接超时/拒绝 |
| 资源限制 | 创建超限 Pod | Admission 拒绝 |
| RBAC | 租户开发者尝试 list secrets 其他 NS | 403 Forbidden |
| 存储隔离 | 尝试挂载其他租户 PVC | 绑定失败 |
| 进程隔离 | 容器内尝试访问宿主机进程 | 无权限 |
| 镜像限制 | 拉取外部镜像 | Admission 拒绝 |

### 租户自助服务

```bash
# 🟢 低风险：租户自助查看配额使用率
kubectl get resourcequota -n tenant-ecommerce -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.used}{"\n"}{end}'

# 🟢 低风险：租户查看自己的 HPA
kubectl get hpa -n tenant-ecommerce

# 🟡 中风险：租户自助扩容（在配额内）
kubectl scale deployment web-app -n tenant-ecommerce --replicas=5
```

---

## 反模式

### 反模式 1：所有租户共享 default Namespace

**后果**：无资源隔离、无网络隔离、无权限隔离，任何租户可以访问/删除其他租户资源。

**修正**：每个租户独立 Namespace + ResourceQuota + NetworkPolicy + RBAC。

### 反模式 2：NetworkPolicy 只配入站不配出站

**后果**：租户 Pod 可以自由访问集群内任何服务，包括其他租户和基础设施组件。

**修正**：默认拒绝所有出站，显式允许必要的出站目标（DNS、共享服务、外部 API）。

### 反模式 3：RBAC 使用 cluster-admin

**后果**：租户拥有集群级管理员权限，可以删除 Namespace、修改 RBAC、访问所有租户数据。

**修正**：最小权限原则，租户只获得 Namespace 级 Role，禁止集群级 ClusterRole 绑定。参见 [[application-security-hardening]]。

### 反模式 4：ResourceQuota 不设 LimitRange

**后果**：租户创建不设 limits 的 Pod，单 Pod 耗尽整个配额甚至节点资源。

**修正**：LimitRange 设置默认 limits 和最大值，确保单 Pod 不能 monopolize 资源。参见 [[resource-qos-rightsizing]]。

### 反模式 5：忽略 Pod Security Standards

**后果**：租户运行 privileged 容器、挂载宿主机路径、使用 hostNetwork，突破容器隔离。

**修正**：Namespace 标签启用 `pod-security.kubernetes.io/enforce: restricted`，禁止特权操作。

---

## Related

- [[application-security-hardening]] — 应用安全加固
- [[resource-qos-rightsizing]] — 资源 QoS 与 Right-sizing
- [[scheduling-topology-patterns]] — 调度拓扑与节点池设计
- [[config-management-feature-flags]] — 配置管理与 Feature Flag 模式
- [[cost-optimization-finops]] — 成本优化与 FinOps
- [[api-design-versioning-patterns]] — API 设计与版本管理模式
