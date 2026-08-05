---
title: "多集群工作负载分发"
description: "多集群工作负载分发：Karmada/OCM、ApplicationSet、权重路由、故障转移与一致性保障"
summary: "面向平台工程师与 SRE 的多集群工作负载分发完整指南，覆盖 Karmada、Argo CD ApplicationSet、权重路由、跨集群故障转移与一致性策略。"
category: 工作负载
tags:
- multicluster
- karmada
- argocd
- ocm
- federation
- failover
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 架构师
estimated_read_time: 20min
intent_queries:
- "如何将工作负载分发到多个 Kubernetes 集群"
- "Karmada 与 OCM 如何选择"
- "多集群故障转移如何实现"
trigger_keywords:
- multicluster
- karmada
- ocm
- applicationset
- failover
- federation
prerequisites:
- kubectl-basics
- argocd-basics
- cluster-federation
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

# 多集群工作负载分发

> **适用版本**: Karmada 1.9+ / Argo CD 2.10+ / Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

单个 Kubernetes 集群在规模、可用性和地域覆盖能力上都存在天然的上限。从规模角度看，社区建议单集群节点数不超过 5000，Pod 数不超过 15 万，超过这个规模控制平面的性能会显著下降。从可用性角度看，单集群意味着单一故障域——一旦该集群的控制平面或底层基础设施出现区域性故障，所有运行其上的服务都会受到影响。从地域覆盖角度看，要为全球用户提供低延迟访问，就需要在多个地域部署服务，而单集群很难跨越如此大的地理范围。

正是这些限制，使得多集群架构成为大型平台的必然选择。而多集群工作负载分发，作为多集群架构的核心能力，解决的是"一份应用定义，如何按策略部署到多个集群，并在某个集群故障时自动将负载转移到其他集群"这一核心问题。

本文覆盖主流分发方案（Karmada、OCM、Argo CD ApplicationSet），详解权重路由、故障转移、一致性保障三大核心能力，并给出生产级配置。需要特别强调的是，多集群之间的网络打通是工作负载分发的前提条件，如果集群间网络不通，分发再多的工作负载也无法正常通信，这部分内容需要结合 [[05-网络/01-K8s网络核心/51-multicluster-network-federation.md|多集群网络联邦]] 一起阅读。

---

## 核心概念

### 1. 多集群分发的核心问题

多集群工作负载分发看似简单——不就是把 YAML 应用到多个集群吗？但实际上涉及一系列复杂的决策和协调问题。

| 问题 | 说明 |
|------|------|
| 集群选择 | 按标签/区域/资源选择目标集群 |
| 副本分配 | 如何在各集群间分配副本数 |
| 差异管理 | 各集群配置差异（镜像仓库、域名）如何处理 |
| 故障转移 | 集群故障时如何自动迁移负载 |
| 一致性 | 如何保证各集群状态与期望一致 |
| 流量调度 | 用户流量如何路由到正确集群 |

集群选择需要考虑地域、资源余量、合规要求等多维因素。副本分配可以是静态权重，也可以根据各集群的资源使用情况动态调整。差异管理是一个极易被低估的难题——不同集群可能使用不同的镜像仓库地址、不同的域名后缀、不同的资源配额，如何在保持单一应用定义的同时处理这些差异，是分发系统必须解决的问题。

### 2. 主流方案对比

| 方案 | 模式 | 优势 | 劣势 | 适用场景 |
|------|------|------|------|---------|
| **Karmada** | 中心化控制平面 | 功能完整、调度策略丰富、故障转移强 | 引入额外控制平面 | 大规模多集群、强分发需求 |
| **OCM**（Open Cluster Management） | 去中心化、Agent | 轻量、CNCF 标准、可扩展 | 需自行编排上层能力 | 集群注册管理、合规 |
| **Argo CD ApplicationSet** | GitOps 生成器 | 与 GitOps 无缝、声明式 | 故障转移能力弱 | GitOps 驱动的多集群部署 |
| **Cluster API** | 集群生命周期 | 专注集群供给 | 不负责工作负载分发 | 集群创建管理 |
| **Submariner/Liqo** | 网络层 | 跨集群网络打通 | 不分发工作负载 | 网络联邦（见网络篇） |

Karmada 是目前功能最完整的多集群分发系统，它继承了 Kubernetes Federation v2 的设计理念并大幅增强，提供了丰富的调度策略（静态权重、动态权重、地域亲和）、完整的故障转移机制和差异覆盖能力。但它的代价是引入了一个独立的控制平面（包含独立的 etcd 和 apiserver），增加了运维复杂度。

Argo CD ApplicationSet 走的是完全不同的路线——它不引入额外的控制平面，而是作为 Argo CD 的扩展，通过生成器（Generator）模式从集群列表、Git 目录、矩阵等来源批量生成 Application 资源，实现 GitOps 驱动的多集群部署。它的优势是与现有 GitOps 流程无缝集成，劣势是故障转移能力较弱，需要配合其他机制实现。

### 3. 分发模式

从架构角度看，多集群分发主要有三种模式。Push 模式由中心控制平面主动将资源推送到各成员集群，Karmada 和 ApplicationSet 都采用这种模式，优点是控制力强、状态可见性好，缺点是中心控制平面成为单点。Pull 模式由成员集群上的 Agent 主动拉取并应用资源，OCM 和 Argo CD 的 pull 模式属于此类，优点是去中心化、成员集群自治，缺点是状态同步有延迟。混合模式结合两者，注册用 pull、分发用 push，兼顾了安全性和控制力。

---

## 生产部署/实现

### 1. Karmada 部署与集群注册 🔴

Karmada 的部署涉及建立一个独立的控制平面，这是一个影响整个多集群架构的高风险操作。

```bash
# 🔴 高风险：部署独立控制平面，影响多集群架构
# 安装 Karmada 控制平面（独立 etcd + apiserver）
kubectl apply -f https://github.com/karmada-io/karmada/releases/download/v1.9.0/karmada-crds.yaml
helm repo add karmada https://karmada-io.github.io/charts
helm install karmada-control-plane karmada/karmada \
  --namespace karmada-system --create-namespace --wait

# 注册成员集群（push 模式）
kubectl karmada join cluster-beijing \
  --kubeconfig=/root/.kube/beijing.config \
  --karmada-context=karmada-apiserver

kubectl karmada join cluster-shanghai \
  --kubeconfig=/root/.kube/shanghai.config \
  --karmada-context=karmada-apiserver
```

验证集群注册：

```bash
# 🟢 低风险：只读
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config get clusters
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config get clusters -o wide
```

Karmada 控制平面默认部署在宿主集群中，包含独立的 etcd、apiserver、controller-manager 和 scheduler。成员集群通过 karmada-agent（pull 模式）或 karmada-controller（push 模式）与控制平面通信。注册成功后，每个成员集群会作为一个 Cluster 资源出现在 Karmada 控制平面中，其健康状态、资源余量等信息会被持续上报。

### 2. Karmada PropagationPolicy（权重分发 + 故障转移） 🟡

PropagationPolicy 是 Karmada 的核心资源，它定义了哪些资源应该被分发、分发到哪些集群、如何分配副本，以及故障时如何处理。

```yaml
# 🟡 中风险：分发策略决定负载分布
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: web-app-propagation
  namespace: production
spec:
  resourceSelectors:
  - apiVersion: apps/v1
    kind: Deployment
    name: web-app
  placement:
    clusterAffinity:
      labelSelector:
        matchLabels:
          region: cn
    clusterTolerations:
    - key: cluster.karmada.io/unreachable
      operator: Exists
      effect: NoExecute
      tolerationSeconds: 60        # 集群不可达 60s 后驱逐
    replicaScheduling:
      replicaDivisionPreference: Weighted
      replicaSchedulingType: Divided
      weightPreference:
        staticWeightList:
        - targetCluster:
            labelSelector:
              matchLabels:
                zone: beijing
          weight: 2                 # 北京 2 份
        - targetCluster:
            labelSelector:
              matchLabels:
                zone: shanghai
          weight: 1                 # 上海 1 份
  failover:
    application:
      decisionConditions:
        tolerationSeconds: 60
      purgeMode: Gracefully
  propagationPolicy:
    activationPreference: AfterClusterUnreachable
```

这个策略配置实现了几个关键能力。clusterAffinity 将分发范围限定在 region=cn 的集群。replicaScheduling 采用加权分配，北京集群获得 2 份副本，上海获得 1 份，总副本数按 2:1 的比例分配。failover 配置了故障转移：当某个集群不可达超过 60 秒后，该集群上的副本会被优雅地驱逐（purgeMode: Gracefully），并重新调度到健康集群。这种机制确保了即使一个集群完全故障，服务也能自动恢复。

### 3. Argo CD ApplicationSet（GitOps 多集群） 🟡

对于已经采用 Argo CD 作为 GitOps 工具的团队，ApplicationSet 是实现多集群部署最自然的选择。

```yaml
# 🟡 中风险：批量生成 Application，影响多集群部署
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: web-app-multicluster
  namespace: argocd
spec:
  generators:
  - clusters:
      selector:
        matchLabels:
          env: production
      values:
        replicaCount: "3"
  template:
    metadata:
      name: 'web-app-{{name}}'
    spec:
      project: default
      source:
        repoURL: https://git.example.com/platform.git
        targetRevision: main
        path: apps/web-app
        helm:
          parameters:
          - name: replicaCount
            value: '{{values.replicaCount}}'
          - name: image.repository
            value: '{{metadata.labels.registry}}/web-app'
      destination:
        server: '{{server}}'
        namespace: production
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
        - CreateNamespace=true
```

ApplicationSet 的 clusters 生成器会自动发现所有匹配标签的集群，并为每个集群生成一个独立的 Application。模板中的占位符（如 {{name}}、{{server}}、{{metadata.labels.registry}}）会被替换为每个集群的实际值，从而实现差异化配置。syncPolicy 中的 selfHeal 确保任何手动修改都会被自动纠正，维持 Git 作为唯一真相源。

### 4. 跨集群权重路由（配合全局负载均衡） 🟡

工作负载分发只是多集群架构的一半，另一半是流量调度——用户的请求如何被路由到正确的集群。

```yaml
# 🟡 中风险：流量权重影响用户访问路径
# Karmada 结合全局流量调度（如基于 DNS 或 Service Mesh）
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: web-app-global
spec:
  hosts:
  - web-app.global.example.com
  location: MESH_EXTERNAL
  resolution: DNS
---
# 通过全局 LB（如 AWS Global Accelerator / Cloudflare LB）按地域权重路由
# 北京 70% / 上海 30%，故障时自动摘除
```

分发和流量调度必须配套设计。如果只做了工作负载分发而没有配置流量调度，那么当某个集群故障时，虽然负载已经转移到其他集群，但用户的流量仍然会被路由到故障集群，导致服务不可用。全局负载均衡器（如 AWS Global Accelerator、Cloudflare Load Balancing、或基于 DNS 的 GSLB）需要与健康检查配合，在检测到集群故障时自动将流量切换到健康集群。

---

## 运维操作

### 1. 查看分发状态 🟢

```bash
# 🟢 低风险：只读
# Karmada 资源绑定情况
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config \
  get resourcebinding -n production web-app-deployment -o yaml

# 各集群实际副本
for ctx in beijing shanghai; do
  echo "=== $ctx ==="
  kubectl --context=$ctx -n production get deploy web-app
done
```

ResourceBinding 是 Karmada 的内部资源，它记录了某个资源被分发到了哪些集群、每个集群分配了多少副本、当前的同步状态如何。通过检查 ResourceBinding，可以清楚地了解分发的实际执行情况。

### 2. 手动故障转移演练 🔴

```bash
# 🔴 高风险：模拟集群故障，验证故障转移
# 1. 标记集群不可达（演练用）
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config \
  taint clusters cluster-beijing cluster.karmada.io/unreachable=:NoExecute

# 2. 观察负载是否转移到上海
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config \
  get resourcebinding -n production web-app-deployment -o yaml | grep -A10 spec.clusters

# 3. 演练后恢复
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config \
  taint clusters cluster-beijing cluster.karmada.io/unreachable-
```

故障转移演练是验证多集群架构有效性的关键手段。通过手动给集群打上 unreachable 污点，可以模拟集群故障场景，观察工作负载是否按预期转移到健康集群。演练应该在非生产环境定期进行，确保在真实故障发生时，故障转移机制能够正常工作。

### 3. 差异化配置管理 🟢

```yaml
# 🟢 低风险：使用 Karmada OverridePolicy 处理集群差异
apiVersion: policy.karmada.io/v1alpha1
kind: OverridePolicy
metadata:
  name: web-app-override
  namespace: production
spec:
  targetCluster:
    labelSelector:
      matchLabels:
        zone: shanghai
  overriders:
    plaintext:
    - path: /spec/template/spec/containers/0/image
      operator: replace
      value: registry-shanghai.example.com/web-app:v1.0
```

OverridePolicy 是 Karmada 处理集群差异的核心机制。它允许针对特定集群或集群组，对分发的资源进行字段级别的覆盖。在这个例子中，上海集群使用不同的镜像仓库地址，通过 OverridePolicy 在分发时自动替换镜像路径，而无需维护多份 YAML。这种"单一来源、差异覆盖"的模式是多集群配置管理的最佳实践。

---

## 故障排查

### 症状 1：资源未分发到目标集群

```bash
# 🟢 低风险
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config \
  describe propagationpolicy web-app-propagation
kubectl --kubeconfig=/etc/karmada/karmada-apiserver.config \
  get events -n production
```

根因可能是 clusterAffinity 的标签选择器不匹配任何集群、成员集群的 Agent 离线无法接收分发、或者 RBAC 权限不足无法在目标集群创建资源。处置方法是检查集群标签是否与 affinity 匹配、确认 karmada-agent 运行正常、检查 ServiceAccount 的权限配置。

### 症状 2：故障转移未触发

根因可能是 tolerationSeconds 设置过长导致等待时间超预期、failover 配置未正确启用、或者集群状态未被正确上报。处置方法是调整 toleration 时间、确认 failover.application 配置完整、检查集群心跳上报是否正常。

### 症状 3：各集群配置漂移

根因是有人手动修改了成员集群中的资源，或者 selfHeal 未启用导致偏差未被纠正。处置方法是启用 Argo CD 的 selfHeal、使用 Karmada 的 ResourceRegistry 强制对齐，并通过 RBAC 禁止直接操作成员集群中的受管资源。

### 症状 4：副本分配不均

根因是 weight 配置错误或目标集群资源不足无法调度分配的副本。处置方法是检查 weightPreference 配置、确认成员集群有足够的资源余量。

### 排查决策树

```
分发异常
├── 未分发?       → affinity/agent/RBAC
├── 转移失败?     → toleration/failover 配置
├── 配置漂移?     → selfHeal/禁止手动改
└── 分配不均?     → weight/资源不足
```

---

## 最佳实践

第一，选型上，需要强分发和故障转移能力选 Karmada，纯 GitOps 驱动选 ApplicationSet，侧重集群注册管理选 OCM。第二，网络先行，跨集群分发前必须先打通集群间网络，见 [[05-网络/01-K8s网络核心/51-multicluster-network-federation.md|多集群网络联邦]]。第三，故障转移必须配置 tolerationSeconds 和 failover 策略，并定期演练验证。第四，差异管理用 OverridePolicy 或 Helm values 处理，禁止 fork 多份 YAML。第五，一致性保障启用 selfHeal，确保单一控制平面为唯一真相源。第六，流量调度必须与分发配套，否则故障转移无法真正生效。第七，建立跨集群统一可观测视图。第八，渐进推进，先双集群验证再扩展到多区域。

```yaml
# 🟢 低风险：集群健康监控告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: multicluster-alerts
spec:
  groups:
  - name: cluster
    rules:
    - alert: MemberClusterUnreachable
      expr: karmada_cluster_ready_status{state="Ready"} == 0
      for: 2m
      labels:
        severity: critical
```

---

## Related

- [[05-网络/01-K8s网络核心/51-multicluster-network-federation.md|多集群网络联邦]]
- [[05-网络/01-K8s网络核心/33-multi-cluster-federation.md|多集群联邦]]
- [[05-网络/01-K8s网络核心/34-multi-cluster-networking.md|多集群网络]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management.md|GPU 工作负载管理]]
- [[02-工作负载/00-总览/01-kubernetes-deployment-patterns-architecture.md|Kubernetes 部署模式架构]]
- [[12-可靠性/02-灾难恢复/index|02-灾难恢复]]
