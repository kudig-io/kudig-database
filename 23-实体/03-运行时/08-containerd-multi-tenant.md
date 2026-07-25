---
title: containerd 多租户
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 08-containerd-multi-tenant
- prometheus
- grafana
- containerd
- networkpolicy
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 多租户 是什么
- 如何 containerd 多租户
trigger_keywords:
- containerd
- 多租户
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 多租户

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd 多租户实践是关于在共享的 containerd 运行时上安全地运行多个租户工作负载的方法论。通过 Namespace 隔离、运行时类（RuntimeClass）、命名空间配额、镜像隔离和节点级安全策略，实现多租户环境下 containerd 的安全运维。该实践涵盖容器运行时隔离（runc vs. Kata Containers）、镜像仓库隔离、CRI 代理（如 CRI-O Shim）以及容器运行时安全加固等关键技术。

## Key Features（核心能力）

- **Containerd Namespace 隔离**：通过 containerd namespace 机制实现镜像和容器元数据隔离
- **RuntimeClass 支持**：通过 RuntimeClass 为不同租户分配不同的容器运行时（runc/Kata/gVisor）
- **镜像仓库策略**：通过镜像签名验证和准入策略限制可拉取的镜像来源
- **资源限制**：通过 CRI 和 cgroups 实现容器级别的 CPU/内存/IO 限制
- **审计日志**：记录容器创建、启动、销毁等操作，支持租户级行为审计
- **安全上下文**：强制非 root 用户运行、只读根文件系统等安全策略

## 架构与工作原理

多租户隔离通过多层机制实现：Kubernetes Namespace 提供逻辑隔离；RuntimeClass 通过 kubelet 为不同租户的 Pod 分配不同的底层运行时（如安全敏感租户使用 Kata Containers）；containerd 的 namespace 机制隔离镜像和容器元数据；CRI proxy 可在 kubelet 和 containerd 之间增加一层策略执行。节点上通过 AppArmor/SELinux/seccomp profile 进一步限制容器行为。

## K8s 集成

在 K8s 中，多租户隔离通过 Pod Security Admission、NetworkPolicy、RBAC、ResourceQuota、LimitRange 等机制实现。containerd 层面的多租户实践补充了这些 API 级别控制，通过 RuntimeClass 指定低层运行时，通过节点级安全策略限制容器行为。CRI 镜像策略可限制镜像仓库来源，防止租户运行不受信任的镜像。

## 生产用例

- **共享集群多租户**：多个团队共享同一 K8s 集群但需要工作负载隔离
- **SaaS 平台**：为客户提供隔离的容器运行环境
- **安全合规环境**：通过 Kata Containers 或 gVisor 提供硬件级隔离
- **开发测试平台**：为不同项目提供隔离但共享基础设施的容器环境

## 安装与配置

### RuntimeClass 配置

```bash
# 🟢 查看 containerd namespaces
ctr namespace list

# 🟢 创建隔离 namespace
ctr namespace create tenant-a
ctr namespace create tenant-b
```

```yaml
# Kata Containers RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata
scheduling:
  nodeSelector:
    runtime.kubernetes.io/kata: "true"
---
# gVisor RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
scheduling:
  nodeSelector:
    runtime.kubernetes.io/gvisor: "true"
---
# 标准 runc RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: runc
handler: runc
```

### containerd 多运行时配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

  # 标准 runc 运行时
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true

  # Kata Containers 运行时
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata.options]
      ConfigPath = "/opt/kata/share/defaults/kata-containers/configuration.toml"

  # gVisor 运行时
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
    runtime_type = "io.containerd.runsc.v1"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options]
      TypeUrl = "io.containerd.runsc.v1"
      ConfigPath = "/etc/containerd/runsc.toml"
```

### 租户隔离策略

```yaml
# 租户命名空间资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-a-quota
  namespace: tenant-a
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "50"
    services: "10"
    persistentvolumeclaims: "10"
---
# LimitRange 默认限制
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-a-limits
  namespace: tenant-a
spec:
  limits:
  - default:
      cpu: "1"
      memory: 1Gi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "4"
      memory: 8Gi
    type: Container
---
# 租户网络隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-a
spec:
  podSelector: {}
  policyTypes: [Ingress, Egress]
  ingress:
  - from:
    - podSelector: {}  # 仅允许同命名空间
  egress:
  - to:
    - podSelector: {}
  - to:  # 允许 DNS
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
```

### 镜像策略 (Kyverno)

```yaml
# 限制租户只能从指定仓库拉取镜像
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-tenant-images
spec:
  validationFailureAction: Enforce
  rules:
  - name: validate-image-registry
    match:
      any:
      - resources:
          kinds: ["Pod"]
          namespaces: ["tenant-a", "tenant-b"]
    validate:
      message: "仅允许从 registry.example.com 拉取镜像"
      pattern:
        spec:
          containers:
          - image: "registry.example.com/*"
```

### 租户 Pod 示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
  namespace: tenant-a
spec:
  runtimeClassName: kata-containers  # 使用 Kata 强隔离
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: registry.example.com/tenant-a/app:v1.0
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: "1"
        memory: 1Gi
```

## 运维操作

```bash
# 🟢 检查 RuntimeClass
kubectl get runtimeclass

# 🟢 检查租户资源使用
kubectl top pods -n tenant-a
kubectl describe resourcequota -n tenant-a

# 🟢 检查 containerd namespace 隔离
ctr -n k8s.io containers ls | head -10
ctr -n tenant-a containers ls 2>/dev/null

# 🟢 检查网络隔离
kubectl get networkpolicy -n tenant-a
kubectl exec -n tenant-a pod -- curl -s http://tenant-b-svc.tenant-b:80  # 应失败

# 🟢 检查安全上下文
kubectl get pods -n tenant-a -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.runtimeClassName}{"\n"}{end}'
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 无法使用 Kata | RuntimeClass 未创建/节点不支持 | `kubectl get runtimeclass`; `kubectl describe pod` | 创建 RuntimeClass/添加节点标签 |
| 租户间网络互通 | NetworkPolicy 缺失 | `kubectl get netpol -n <ns>` | 部署租户隔离策略 |
| 资源超限 | ResourceQuota 未配置 | `kubectl describe resourcequota` | 配置配额和 LimitRange |
| 镜像拉取被拒 | 镜像策略过严 | `kubectl get events` | 调整 Kyverno 策略 |
| Kata 启动慢 | VM 初始化开销 | `kubectl describe pod` 查看启动时间 | 预热 VM/调整配置 |

### 排查流程

```
多租户隔离异常
├── 租户间可互通？
│   ├── NetworkPolicy 存在？→ kubectl get netpol
│   ├── CNI 支持？→ 检查 Calico/Cilium
│   └── 策略正确？→ 检查 podSelector/namespaceSelector
├── 资源隔离失效？
│   ├── ResourceQuota 配置？→ kubectl describe quota
│   ├── LimitRange 存在？→ kubectl get limitrange
│   └── Pod 设置了 limits？→ kubectl get pod -o yaml
└── 运行时隔离失效？
    ├── RuntimeClass 正确？→ kubectl get runtimeclass
    ├── 节点支持？→ 检查节点标签
    └── containerd 配置？→ 检查 runtimes 配置
```

## 生产案例

### 案例1：SaaS 平台多租户强隔离

- **场景**：SaaS 平台为 100+ 客户提供独立环境，需防止容器逃逸和横向移动
- **方案**：每个客户一个 Namespace + Kata Containers RuntimeClass + default-deny NetworkPolicy + 镜像白名单
- **效果**：客户间 VM 级隔离，通过 SOC2 审计

### 案例2：开发平台资源共享

- **场景**：50 个开发团队共享 20 节点集群，需要公平资源分配
- **方案**：每团队一个 Namespace + ResourceQuota + LimitRange + PriorityClass；普通工作负载用 runc，安全测试用 gVisor
- **效果**：资源公平分配，无团队能独占集群

## 对比替代方案

| 方案 | 隔离级别 | 性能开销 | 适用场景 |
|------|----------|----------|----------|
| Namespace + RBAC + NetPol | 逻辑隔离 | 无 | 内部团队/信任环境 |
| + Kata Containers | VM级隔离 | ~5% | 多租户/不可信工作负载 |
| + gVisor | 用户态内核 | ~10-20% | 安全敏感/syscall过滤 |
| 独立集群 | 完全隔离 | 资源浪费 | 强合规/大客户 |
| vCluster | 虚拟集群 | 低 | 多团队共享控制平面 |

## 检查清单

- [ ] 每个租户有独立 Namespace
- [ ] RuntimeClass 已配置（runc/kata/gvisor）
- [ ] NetworkPolicy 默认拒绝已部署
- [ ] ResourceQuota 和 LimitRange 已配置
- [ ] 镜像拉取策略已限制
- [ ] Pod Security Admission 设置为 restricted
- [ ] RBAC 限制租户仅访问自己的 Namespace
- [ ] 审计日志已启用

## Related

- [[k0s]] — K0s
- [[kubeedge]] — KubeEdge
- [[telepresence]] — Telepresence
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 08-containerd-multi-tenant


<!-- risk-assessed -->
