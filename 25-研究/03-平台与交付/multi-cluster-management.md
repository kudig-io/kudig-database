---
title: 多集群 Kubernetes 管理技术选型研究
summary: 深入研究多集群 Kubernetes 管理的三大模式（Hub-Spoke、Federation、Service Mesh Multi-Cluster），对比 Kubefed、Argo CD ApplicationSet、Cluster API、Cilium ClusterMesh 等方案。
category: research
tags:
- research
- multi-cluster
- cluster-api
- argocd
- cilium
- federation
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# 多集群 Kubernetes 管理技术选型研究

## 研究背景

随着企业 Kubernetes 采用规模增长，单集群架构的局限性日益凸显：

- **集群规模上限**：单集群推荐上限 5000 节点，超大规模场景需要多集群
- **故障爆炸半径**：单集群故障影响全部业务，多集群可隔离故障域
- **多地域/多可用区**：全球化部署需要多地域集群
- **环境隔离**：开发/测试/生产环境物理隔离要求
- **合规与数据驻留**：不同国家/地区数据不得跨境，需要本地集群

多集群管理涉及集群生命周期管理、应用分发、网络连通、安全策略同步等多个维度。

## 核心问题

1. 多集群管理的三大模式（Hub-Spoke 集中管理、Federation 联邦、Mesh 互联）各自适合什么场景？
2. Cluster API、Argo CD ApplicationSet、Karmada 在集群生命周期和应用分发方面的差异是什么？
3. 跨集群服务发现和连通（Cilium ClusterMesh、Istio Multi-Cluster）如何实现？
4. 多集群场景下的安全策略、RBAC、证书管理如何统一？

## 调研发现

### 发现一：多集群管理三大模式

```
模式一：Hub-Spoke（集中管理）
  ┌──────────┐
  │ Hub集群   │ ← 管理面
  └──┬──┬──┬─┘
     │  │  │
  ┌──┴┐┌┴┐┌┴──┐
  │集群1││集群2││集群3│ ← 被管集群
  └────┘└───┘└────┘

  特点：中心化管理配置下发，集群间无直接通信
  工具：Argo CD ApplicationSet, Cluster API, Flux
  场景：CI/CD 多环境部署、集中安全策略管理

模式二：Federation（联邦）
  ┌───────────────────────────┐
  │  联邦控制面（Federation API）│
  └──┬──────┬──────┬─────────┘
     │      │      │
  ┌──┴─┐ ┌──┴─┐ ┌──┴───┐
  │集群1│ │集群2│ │集群3│  ← 成员集群
  └────┘ └────┘ └──────┘

  特点：统一 API 声明，自动分发到成员集群，支持调度策略
  工具：Karmada, Kubefed (已弃维)
  场景：跨集群负载调度、资源全局调度

模式三：Service Mesh Multi-Cluster（网格互联）
  ┌──────┐       ┌──────┐       ┌──────┐
  │集群1  │←─────→│集群2  │←─────→│集群3  │
  └──────┘       └──────┘       └──────┘

  特点：集群间服务级互通，透明跨集群调用
  工具：Istio Multi-Cluster, Cilium ClusterMesh, Linkerd
  场景：跨地域服务发现、多活容灾
```

### 发现二：集群生命周期管理对比

| 维度 | Cluster API | Karmada | Kubefed |
|------|------------|---------|---------|
| **定位** | 集群生命周期管理（创建/升级/销毁） | 多集群应用分发+调度 | 多集群资源联邦（已停维） |
| **工作方式** | 基础设施声明式创建集群 | 控制面聚合多个集群 | API 转发到成员集群 |
| **集群创建** | ✅ 支持（AWS/GCP/Azure/vSphere） | ❌ 不创建集群 | ❌ 不创建集群 |
| **集群升级** | ✅ 自动化滚动升级 | ❌ | ❌ |
| **应用分发** | ❌（需配合 ArgoCD 等） | ✅ PropagationPolicy | ✅ Federated* CRD |
| **调度策略** | ❌ | ✅ 权重/亲和/容忍 | ✅ 基础 |
| **社区活跃** | ⬤⬤⬤⬤⬤（CNCF Incubating） | ⬤⬤⬤⬤（CNCF Incubating） | ⬤（已停维） |
| **生产推荐** | ✅ 生命周期管理首选 | ✅ 应用分发推荐 | ❌ 不推荐新项目 |

### 发现三：应用分发方案对比

| 方案 | 工作模式 | GitOps 支持 | 多集群调度 | 差异化配置 | 推荐场景 |
|------|---------|------------|-----------|-----------|---------|
| **Argo CD ApplicationSet** | Hub-Spoke 推送 | ✅ 原生 | ✅ 按集群列表 | ✅ Kustomize/Helm overlays | GitOps 多集群部署首选 |
| **Flux Fleet** | Hub-Spoke 推送 | ✅ 原生 | ✅ 按集群列表 | ✅ Kustomize overlays | GitOps 多集群部署 |
| **Karmada** | Federation 拉取 | ⚠️ 需适配 | ✅ 高级调度 | ✅ OverridePolicy | 需要智能调度的场景 |

**Argo CD ApplicationSet 示例（GitOps 多集群）**：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-cluster-app
spec:
  generators:
  - clusters:
      selector:
        matchLabels:
          env: production    # 只部署到生产集群
  template:
    metadata:
      name: '{{name}}-web-app'
    spec:
      source:
        repoURL: https://github.com/org/gitops-repo
        path: apps/web-app/overlays/{{metadata.labels.env}}
        targetRevision: main
      destination:
        server: '{{server}}'
        namespace: production
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

### 发现四：跨集群网络连通

| 方案 | 连通层级 | 延迟开销 | 多租户 | 复杂度 | 推荐场景 |
|------|---------|---------|--------|--------|---------|
| **Cilium ClusterMesh** | L3/L4（Pod IP 级） | 极低（eBPF） | ✅ | 中 | 同 CNI 多集群首选 |
| **Istio Multi-Cluster** | L7（Service 级） | 中（Envoy） | ✅ | 高 | 需要跨集群 Mesh 策略 |
| **Submariner** | L3/L4（Pod/Service） | 低 | ⚠️ | 中 | 不同 CNI 集群互联 |
| **VPC Peering** | L2/L3 | 最低 | ❌ | 低 | 同云厂商同地域 |

**Cilium ClusterMesh 架构**：

```
集群 A                          集群 B
┌──────────────────┐           ┌──────────────────┐
│ Pod → Service    │           │ Service → Pod    │
│       ↓          │    eBPF   │       ↑          │
│  ClusterMesh     │←─────────→│  ClusterMesh     │
│  (etcd 同步)     │  隧道/VPC  │  (etcd 同步)     │
└──────────────────┘  Peer     └──────────────────┘

  → 集群 A 的 Pod 可以直接访问集群 B 的 Service IP
  → 自动跨集群负载均衡
  → NetworkPolicy 跨集群生效
  → 全局 Service（@global 后缀）支持跨集群发现
```

### 发现五：多集群安全统一

| 安全维度 | 方案 | 说明 |
|---------|------|------|
| **统一认证** | OIDC + 外部 IdP | 所有集群对接同一 OIDC Provider |
| **RBAC 统一** | Cluster API + Argo CD | Hub 集群管理所有集群的 RBAC 配置 |
| **证书管理** | cert-manager + Internal CA | 所有集群使用同一 CA 签发证书 |
| **策略即代码** | Kyverno/OPA Gatekeeper (Policy Hub) | Hub 集群定义策略，ApplicationSet 分发 |
| **审计日志** | Falco + 中央日志平台 | 所有集群发送审计日志到中心化 SIEM |

## 结论与建议

1. **分层选择多集群方案**：
   - 集群生命周期 → **Cluster API**
   - 应用分发 → **Argo CD ApplicationSet**（GitOps 首选）
   - 跨集群网络 → **Cilium ClusterMesh**（已用 Cilium）或 Istio（已用 Istio）
   - 智能调度 → **Karmada**（如需跨集群负载调度）
2. **Hub-Spoke 模式适合大多数企业**：比 Federation 简单，比 Mesh 多集群安全
3. **避免 Kubefed**：项目已停维，新项目应选 Karmada 或 Argo CD ApplicationSet
4. **安全策略必须集中管理**：通过 GitOps 分发统一的 Kyverno/OPA 策略到所有集群
5. **跨集群连通优先选 Cilium ClusterMesh**：eBPF 方案延迟最低，与 NetworkPolicy 无缝集成

## 参考资料

- Cluster API: https://cluster-api.sigs.k8s.io/
- Karmada: https://karmada.io/
- Cilium ClusterMesh: https://docs.cilium.io/en/stable/network/clustermesh/
- [[01-集群基础/index.md|集群基础目录]]
- [[18-云厂商/index.md|云厂商目录]]
- [[10-平台工程/index.md|平台工程目录]]

## Related

- [[24-综合/02-交付与GitOps/argocd-gitops.md|ArgoCD × GitOps]]
- [[25-研究/02-网络与安全/zero-trust-k8s-security|多集群安全治理]]
