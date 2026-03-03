# Virtual Kubelet

> **成熟度**: Sandbox | **加入时间**: 2019-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://virtual-kubelet.io |
| **GitHub** | https://github.com/virtual-kubelet/virtual-kubelet |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Scheduling & Orchestration |
| **适用场景** | Kubernetes 节点虚拟化 |

---

## 项目概述

Virtual Kubelet 是一个开源框架，它模拟 Kubernetes kubelet，将自身注册为集群中的一个节点。但不同于真正的 kubelet 运行在物理/虚拟机上，Virtual Kubelet 将 Pod 调度到其他后端服务，如 Azure Container Instances (ACI)、AWS Fargate、HashiCorp Nomad 等无服务器容器平台。

---

## 核心特性

- **虚拟节点**: 在 K8s 中注册虚拟节点
- **多后端**: ACI、Fargate、Nomad、OpenStack
- **无限扩展**: 无需管理底层节点基础设施
- **标准 API**: 兼容 Kubernetes Pod API
- **弹性伸缩**: 实现真正的无服务器容器
- **Provider 接口**: 可扩展的 Provider 插件架构

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                Virtual Kubelet Architecture                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Kubernetes Control Plane                      │   │
│  │  ┌─────────────┐  ┌─────────────────────────────────┐   │   │
│  │  │ API Server  │  │          Scheduler              │   │   │
│  │  └──────┬──────┘  └──────────────┬──────────────────┘   │   │
│  └─────────┼────────────────────────┼──────────────────────┘   │
│            │                        │                           │
│    ┌───────▼────────┐       ┌───────▼────────┐                 │
│    │  Real Nodes    │       │ Virtual Kubelet │                 │
│    │  ┌──────────┐  │       │  (registers as  │                 │
│    │  │ kubelet  │  │       │   a node)       │                 │
│    │  │ Pod A    │  │       │  ┌────────────┐ │                 │
│    │  │ Pod B    │  │       │  │  Provider  │ │                 │
│    │  └──────────┘  │       │  │  Interface │ │                 │
│    └────────────────┘       │  └─────┬──────┘ │                 │
│                             └────────┼────────┘                 │
│                                      │                          │
│  ┌───────────────────────────────────▼───────────────────────┐ │
│  │                   Provider Backends                        │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐ │ │
│  │  │  Azure   │ │  AWS     │ │ HashiCorp│ │  Custom      │ │ │
│  │  │  ACI     │ │  Fargate │ │  Nomad   │ │  Provider    │ │ │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────────┘ │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 (Azure ACI Provider)

```bash
# Helm 安装 ACI Provider
helm install virtual-kubelet \
  --set provider=azure \
  --set providers.azure.masterUri=https://k8s-api-server:6443 \
  oci://mcr.microsoft.com/aks/virtual-kubelet/virtual-kubelet

# 验证虚拟节点
kubectl get nodes
# NAME                  STATUS   ROLES    AGE
# worker-1              Ready    <none>   30d
# virtual-kubelet-aci   Ready    agent    1m
```

### 调度 Pod 到虚拟节点

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: burst-workload
spec:
  nodeSelector:
    kubernetes.io/role: agent
    type: virtual-kubelet
  tolerations:
    - key: virtual-kubelet.io/provider
      operator: Exists
  containers:
    - name: app
      image: nginx:latest
      resources:
        requests:
          cpu: "1"
          memory: "1Gi"
```

---

## Provider 接口

```go
// Provider 核心接口
type Provider interface {
    CreatePod(ctx context.Context, pod *v1.Pod) error
    UpdatePod(ctx context.Context, pod *v1.Pod) error
    DeletePod(ctx context.Context, pod *v1.Pod) error
    GetPod(ctx context.Context, namespace, name string) (*v1.Pod, error)
    GetPodStatus(ctx context.Context, namespace, name string) (*v1.PodStatus, error)
    GetPods(ctx context.Context) ([]*v1.Pod, error)
}
```

---

## 使用场景

| 场景 | 说明 |
|:---|:---|
| **弹性突发** | 高峰期将工作负载溢出到无服务器平台 |
| **CI/CD** | 批量构建任务无需预留节点 |
| **混合云** | 连接多云容器服务 |
| **IoT/边缘** | 管理远端设备上的容器 |

---

## 最佳实践

1. **Taint/Toleration**: 使用 taint 控制调度到虚拟节点
2. **资源限制**: 设置合理的资源请求
3. **网络规划**: 注意虚拟节点的网络连通性
4. **持久化**: 虚拟节点通常不支持本地存储

---

## 参考资源

- [官方文档](https://virtual-kubelet.io)
- [GitHub Repo](https://github.com/virtual-kubelet/virtual-kubelet)
- [Azure ACI Provider](https://github.com/virtual-kubelet/azure-aci)
- [Provider 开发指南](https://virtual-kubelet.io/docs/creating-a-provider/)

---

**维护者**: Kudig Team | **许可证**: MIT
