---
title: Multi-Cluster Management & Federation
description: 多集群管理与联邦 — 集群拓扑设计、跨集群服务发现、流量调度、配置同步、集群生命周期管理
summary: 企业级多集群 Kubernetes 管理实践，涵盖架构模式、工具选型、运维治理
category: practice
tags:
- multi-cluster
- federation
- cluster-mesh
- fleet-management
- gitops
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: cloud
---
# 多集群管理与联邦

> 企业级多集群 Kubernetes 的架构设计、流量调度与统一治理。

## 多集群架构模式

| 模式 | 说明 | 适用 |
|------|------|------|
| 主从联邦 | 中心集群管理多个成员集群 | 配置分发 |
| 对等网格 | 集群间直接互联 | 服务网格 |
| Hub-Spoke | 管理中心 + 工作集群 | 企业标准 |
| 区域部署 | 每区域独立集群 | 低延迟/合规 |
| 混合云 | 公有云 + 私有云集群 | 弹性/合规 |

## 多集群管理工具对比

| 工具 | 能力 | 维护方 | 适用 |
|------|------|--------|------|
| Rancher | 全生命周期管理 | SUSE | 企业统一管理 |
| Cluster API | 集群即代码 | CNCF | 自动化集群供给 |
| Fleet (Rancher) | 大规模 GitOps | SUSE | 配置分发 |
| ArgoCD App-of-Apps | 多集群 GitOps | CNCF | 应用部署 |
| Cilium ClusterMesh | 跨集群网络 | Isovalent | 服务互联 |
| Submariner | 跨集群网络 | CNCF | 网络打通 |
| Karmada | 多集群调度 | CNCF | 工作负载分发 |
| Admiralty | 多集群调度 | 社区 | Pod 级调度 |

## Cluster API — 集群即代码

### 管理集群部署

```bash
# 初始化管理集群
clusterctl init --infrastructure aws

# 或使用 Docker（开发环境）
clusterctl init --infrastructure docker
```

### 工作集群定义

```yaml
# workload-cluster.yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: prod-us-east
  namespace: clusters
spec:
  clusterNetwork:
    pods:
      cidrBlocks: ["10.244.0.0/16"]
    services:
      cidrBlocks: ["10.96.0.0/12"]
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta2
    kind: AWSCluster
    name: prod-us-east
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: prod-us-east-control-plane
---
apiVersion: controlplane.cluster.x-k8s.io/v1beta1
kind: KubeadmControlPlane
metadata:
  name: prod-us-east-control-plane
  namespace: clusters
spec:
  replicas: 3
  version: v1.30.2
  machineTemplate:
    infrastructureRef:
      apiVersion: infrastructure.cluster.x-k8s.io/v1beta2
      kind: AWSMachineTemplate
      name: prod-us-east-cp
  kubeadmConfigSpec:
    clusterConfiguration:
      apiServer:
        extraArgs:
          enable-admission-plugins: NodeRestriction,PodSecurity
---
apiVersion: cluster.x-k8s.io/v1beta1
kind: MachineDeployment
metadata:
  name: prod-us-east-workers
  namespace: clusters
spec:
  clusterName: prod-us-east
  replicas: 5
  selector:
    matchLabels:
      cluster: prod-us-east
  template:
    spec:
      clusterName: prod-us-east
      version: v1.30.2
      bootstrap:
        configRef:
          apiVersion: bootstrap.cluster.x-k8s.io/v1beta1
          kind: KubeadmConfigTemplate
          name: prod-us-east-workers
      infrastructureRef:
        apiVersion: infrastructure.cluster.x-k8s.io/v1beta2
        kind: AWSMachineTemplate
        name: prod-us-east-worker
```

## ArgoCD 多集群部署

### 注册集群

```bash
# 添加集群到 ArgoCD
argocd cluster add prod-us-east --name prod-us-east
argocd cluster add prod-eu-west --name prod-eu-west
argocd cluster add staging --name staging
```

### App-of-Apps 多集群分发

```yaml
# root-app.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: platform-apps
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/org/platform-config
    path: apps/
    targetRevision: main
  destination:
    server: https://kubernetes.default.svc
    namespace: argocd
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
---
# apps/order-service.yaml — 部署到所有集群
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: order-service
  namespace: argocd
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            environment: production
  template:
    metadata:
      name: 'order-service-{{name}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/order-service
        path: deploy/overlays/production
        targetRevision: main
      destination:
        server: '{{server}}'
        namespace: production
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

## Cilium ClusterMesh — 跨集群服务发现

```yaml
# 启用 ClusterMesh
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  cluster-name: us-east-1
  cluster-id: "1"
  enable-clustermesh: "true"
  clustermesh-apiserver-etcd-service-account-name: clustermesh-apiserver
---
# 连接远程集群
apiVersion: v1
kind: Secret
metadata:
  name: clustermesh-remote-eu-west
  namespace: kube-system
  labels:
    io.cilium/clustermesh: "true"
type: Opaque
stringData:
  key: <base64-encoded-key>
  cert: <base64-encoded-cert>
  config: |
    endpoints:
      - https://eu-west-mesh:2379
    trusted-ca-file: /var/lib/cilium/clustermesh/etcd-ca
```

```yaml
# 跨集群服务（Global Service）
apiVersion: v1
kind: Service
metadata:
  name: api-gateway
  namespace: production
  annotations:
    service.cilium.io/global: "true"
spec:
  selector:
    app: api-gateway
  ports:
    - port: 80
      targetPort: 8080
```

## Karmada 多集群调度

```yaml
# 工作负载跨集群分发
apiVersion: work.karmada.io/v1alpha2
kind: PropagationPolicy
metadata:
  name: nginx-propagation
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
  placement:
    clusterAffinity:
      labelSelector:
        matchLabels:
          region: us-east
    spreadConstraints:
      - spreadByField:
          field: cluster
    replicaScheduling:
      replicaSchedulingType: Duplicated
  failover:
    application:
      decisionConditions:
        tolerationSeconds: 300
```

## 多集群运维治理

### 集群生命周期管理

| 阶段 | 活动 | 自动化 |
|------|------|--------|
| 供给 | Cluster API 创建集群 | GitOps 触发 |
| 配置 | 基线组件安装 | Helm/Kustomize |
| 升级 | 滚动升级控制平面+节点 | Cluster API |
| 监控 | 统一指标收集 | Thanos/Grafana |
| 退役 | 数据迁移+集群销毁 | Runbook |

### 统一可观测性

```
各集群 Prometheus → Thanos Sidecar → Thanos Receive → Grafana
                                         ↓
                                    长期存储 (S3)
```

## 最佳实践

1. **集群命名规范**：`{env}-{region}-{purpose}`（如 prod-us-east-apps）
2. **版本一致性**：所有生产集群保持相同 K8s 版本（N-1 策略）
3. **GitOps 单一来源**：所有集群配置来自同一 Git 仓库
4. **渐进式升级**：先 staging → 再 1 个 prod → 最后全部
5. **灾难恢复**：每个集群独立可恢复，不依赖联邦
6. **网络规划**：集群间 CIDR 不重叠，预留扩展空间

## Related

- [[云厂商/index.md|云厂商总索引]]
- [[发布变更/index.md|发布变更]]
- [[网络/服务网格/index.md|Service Mesh]]
