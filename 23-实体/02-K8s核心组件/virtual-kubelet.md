---
title: Virtual Kubelet [entities]
description: '## 概述'
summary: 'Virtual Kubelet 是一个开源框架，它模拟 Kubernetes kubelet，将自身注册为集群中的一个节点。但不同于真正的 kubelet 运行在物理/虚拟机上，Virtual Kubelet 将 Pod 调度到其他后端服务，'
category: entities
tags:
- k8s
- cncf
- runtime
- virtual-kubelet
- kubelet
- scheduler
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Virtual Kubelet 是什么
- 如何 Virtual Kubelet
trigger_keywords:
- Virtual
- Kubelet
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Virtual [[kubelet|Kubelet]]

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

Virtual Kubelet 是由 Microsoft 开源的开源框架，2019 年加入 CNCF Sandbox。它模拟 Kubernetes kubelet，将自身注册为集群中的一个节点，但不同于真正的 kubelet 运行在物理/虚拟机上，Virtual Kubelet 将 Pod 调度到其他后端服务，如 Azure Container Instances（ACI）、AWS Fargate、HashiCorp Nomad 等无服务器容器平台。它使 Kubernetes 集群能够弹性扩展到云端无服务器基础设施，无需管理底层节点。

## 核心特性

- **虚拟节点**: 在 K8s 集群中注册为一个 Node，对调度器透明
- **多 Provider 后端**: 支持 ACI、Fargate、Nomad、ECS、OpenStack Zun 等
- **Provider 接口**: 定义标准接口，可自定义实现新的后端
- **弹性扩展**: 无需预置节点即可运行突发工作负载
- **标准 Pod API**: 兼容 Kubernetes Pod 生命周期管理
- **成本优化**: 仅在实际运行 Pod 时计费

## 架构

Virtual Kubelet 核心是一个实现 Kubernetes kubelet 接口的进程。它通过 Node API 将自身注册为集群节点，配置特定的 Taint 以避免普通 Pod 被调度到此节点。当 Pod 被调度到虚拟节点时，Virtual Kubelet 的 Provider 接口将 Pod 定义转换为后端服务（如 ACI）的容器实例并启动。Pod 状态通过 watch 机制持续同步回 Kubernetes。Provider 接口定义了 CreatePod、DeletePod、GetPod、GetPodStatus 等核心方法，每个 Provider 负责具体的后端对接。

## Kubernetes 集成

Virtual Kubelet 通过 Kubernetes Node API 注册为集群节点，具有特定的 Taint（`virtual-kubelet.io/provider`）。Pod 通过 Toleration 显式调度到虚拟节点。它实现了 Pod 生命周期管理（创建、删除、状态查询），但网络、存储和 Secret 等方面可能因 Provider 而异。Provider 负责将 Kubernetes Pod Spec 映射到底层平台的容器实例，包括环境变量、卷挂载、端口映射等配置。

## 生产使用场景

1. **突发流量处理**: 在流量高峰期将溢出的 Pod 调度到 ACI/Fargate，无需扩容节点
2. **CI/CD 作业**: 将构建任务调度到无服务器平台，节省集群资源
3. **混合调度**: 关键服务运行在自管节点，非关键任务运行在虚拟节点
4. **边缘扩展**: 在边缘集群中使用 Virtual Kubelet 连接云端无服务器后端

## 安装与配置

```bash
# Azure ACI Provider
helm repo add virtual-kubelet https://virtual-kubelet.github.io
helm install virtual-kubelet virtual-kubelet/virtual-kubelet \
  --set provider=azure \
  --set env.azureSubscriptionId=<id> \
  --set env.azureTenantId=<tenant> \
  --set env.azureClientId=<client> \
  --set env.azureClientKey=<secret>

# AWS Fargate Provider (EKS 原生支持，无需单独安装)
# 通过 EKS Fargate Profile 配置

# 通用 CLI 方式
vkubelet --provider azure \
  --nodeName virtual-node-aci \
  --nodename virtual-node-aci \
  --kubeconfig ~/.kube/config
```

```yaml
# Pod 调度到虚拟节点的 Toleration + NodeSelector
apiVersion: v1
kind: Pod
metadata:
  name: burst-pod
  labels:
    app: burst-worker
spec:
  nodeSelector:
    kubernetes.io/role: agent
    type: virtual-kubelet
  tolerations:
    - key: virtual-kubelet.io/provider
      operator: Exists
      effect: NoSchedule
  containers:
    - name: worker
      image: myapp/worker:v1.0
      resources:
        requests:
          cpu: "1"
          memory: 2Gi
```

```yaml
# HPA 配合 Virtual Kubelet 实现弹性扩展
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 3
  maxReplicas: 100
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
  behavior:
    scaleUp:
      policies:
        - type: Pods
          value: 20
          periodSeconds: 60
```

## 运维操作

```bash
# 🟢 检查虚拟节点状态
kubectl get nodes -l type=virtual-kubelet
kubectl describe node virtual-node-aci

# 🟢 检查运行在虚拟节点上的 Pod
kubectl get pods -A --field-selector spec.nodeName=virtual-node-aci

# 🟢 检查 Virtual Kubelet Pod 状态
kubectl get pods -n kube-system -l app=virtual-kubelet
kubectl logs -n kube-system -l app=virtual-kubelet --tail=50

# 🟢 检查节点 Taint 和 Condition
kubectl get node virtual-node-aci -o jsonpath='{.spec.taints}'
kubectl get node virtual-node-aci -o jsonpath='{.status.conditions}'

# 🟡 删除虚拟节点上的 Pod (触发重新调度)
kubectl delete pod <pod-name> -n <ns>

# 🔴 删除虚拟节点 (所有 Pod 将被终止)
kubectl delete node virtual-node-aci
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending 无法调度到虚拟节点 | 缺少 Toleration/NodeSelector | `kubectl describe pod` | 添加正确的 toleration |
| 虚拟节点 NotReady | Provider 连接失败 | `kubectl describe node` | 检查云凭证/网络 |
| Pod 创建后长时间 Unknown | 后端服务响应慢 | 检查 Provider 日志 | 检查云服务商配额 |
| Pod 日志无法获取 | Provider 不支持 logs API | `kubectl logs <pod>` | 使用云平台控制台查看 |
| 存储卷挂载失败 | 后端不支持 PVC | `kubectl describe pod` | 使用 emptyDir 或云平台存储 |
| 虚拟节点消失 | VK Pod 崩溃 | `kubectl get pods -n kube-system` | 重启 VK Deployment |

### 排查流程

```
虚拟节点异常
├── 节点不存在
│   ├── 检查 VK Deployment 状态
│   ├── 检查 VK Pod 日志
│   └── 检查 RBAC 权限 (nodes/create)
├── 节点 NotReady
│   ├── 检查云服务商凭证有效性
│   ├── 检查网络连通性 (API endpoint)
│   └── 检查云服务商配额限制
└── Pod 调度失败
    ├── 检查 Toleration 配置
    ├── 检查 NodeSelector/NodeAffinity
    ├── 检查资源请求是否超出 Provider 支持
    └── 检查 Pod Spec 兼容性 (hostPath 等不支持)
```

## 生产案例

### 案例 1: 电商大促流量突发弹性扩展

- **场景**: 电商大促期间流量增长 10 倍，自管节点无法快速扩容
- **排查**: HPA 触发扩容但节点资源不足，Pod Pending
- **方案**: 配置 Virtual Kubelet (ACI) 作为溢出节点；HPA 扩容的 Pod 通过 toleration 调度到 ACI；大促结束后自动缩容
- **效果**: 扩容时间从 10 分钟(节点)降至 30 秒(ACI)，成本降低 60%

### 案例 2: CI/CD 构建任务隔离

- **场景**: Jenkins Agent Pod 占用大量集群资源，影响业务服务
- **排查**: 构建高峰期节点 CPU 使用率 >90%，业务 Pod 被驱逐
- **方案**: 将 CI/CD 命名空间的 Pod 通过 PriorityClass + Toleration 调度到 Fargate；业务 Pod 保留在自管节点
- **效果**: 业务服务稳定性提升，构建任务不受节点资源限制

## Provider 接口

```go
// Provider 核心接口定义
type PodLifecycleHandler interface {
    CreatePod(ctx context.Context, pod *v1.Pod) error
    UpdatePod(ctx context.Context, pod *v1.Pod) error
    DeletePod(ctx context.Context, pod *v1.Pod) error
    GetPod(ctx context.Context, namespace, name string) (*v1.Pod, error)
    GetPodStatus(ctx context.Context, namespace, name string) (*v1.PodStatus, error)
    GetPods(ctx context.Context) ([]*v1.Pod, error)
}

type PodHandler interface {
    PodLifecycleHandler
    GetContainerLogs(ctx context.Context, namespace, podName, containerName string, opts ContainerLogOpts) (io.ReadCloser, error)
    RunInContainer(ctx context.Context, namespace, podName, containerName string, cmd []string, attach AttachIO) error
}
```

## 对比与替代方案

| 维度 | Virtual Kubelet | KEDA + Jobs | Karpenter | ACK ECI |
|------|----------------|-------------|-----------|----------|
| 扩展粒度 | Pod 级 | Pod 级 | 节点级 | Pod 级 |
| 启动速度 | ~30s | ~10s | ~2min | ~20s |
| 成本模型 | 按 Pod 计费 | 按 Pod 计费 | 按节点计费 | 按 Pod 计费 |
| K8s 兼容性 | 部分 API | 完整 | 完整 | 部分 API |
| 存储支持 | 有限 | 完整 | 完整 | 有限 |
| 网络支持 | 有限 | 完整 | 完整 | VPC 集成 |
| 适用场景 | 突发溢出 | 事件驱动 | 节点自动供给 | 阿里云 |

## 检查清单

- [ ] 云服务商凭证有效且权限充足
- [ ] 虚拟节点状态 Ready
- [ ] Pod Toleration 和 NodeSelector 配置正确
- [ ] 资源请求在 Provider 支持范围内
- [ ] 不使用 hostPath/hostNetwork 等不支持的特性
- [ ] 监控覆盖虚拟节点 Pod 状态
- [ ] 成本告警配置 (避免无限制扩展)
- [ ] 网络连通性验证 (Pod 可访问集群内 Service)
- [ ] 日志收集方案确认 (Provider 可能不支持 kubectl logs)

## 参考链接

- [[operator-pattern]]
- [[pod-lifecycle]]
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]]
- [[23-实体/02-K8s核心组件/kube-scheduler.md|kube-scheduler]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[openfeature]] — OpenFeature
- [[k3s]] — k3s 轻量级 Kubernetes
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference

<!-- risk-assessed -->
