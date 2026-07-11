---
title: Kubernetes
summary: Kubernetes 是一个开源的容器编排平台，用于自动化容器化应用的部署、扩展和管理。
category: concepts
tags:
- core-concept
- k8s
- visibility/public
tier: core
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Kubernetes

## 概述

Kubernetes（简称 K8s）是一个开源的容器编排平台，用于自动化容器化应用的部署、扩展和管理。它最初由 Google 基于内部运行的 Borg 系统设计经验开源，现由 Cloud Native Computing Foundation（CNCF）维护，是云原生领域事实上的标准编排系统。Kubernetes 屏蔽底层基础设施差异，提供统一的声明式 API 来管理无状态、有状态、批处理和守护进程等多种工作负载。

## 架构与工作原理

Kubernetes 采用经典的 Master-Worker 架构，分为**控制平面（Control Plane）**和**数据平面（Data Plane / Worker Nodes）**两部分。

```
┌──────────────────────── 控制平面 ────────────────────────┐
│  kube-apiserver  ←→  etcd                                 │
│  kube-scheduler  |  kube-controller-manager               │
│  cloud-controller-manager                                 │
└───────────────────────────────────────────────────────────┘
            │ gRPC / REST API（Watch & List）
            ▼
┌─────────────────────── 工作节点 ──────────────────────────┐
│  kubelet  →  CRI (containerd/CRI-O)  →  Pod               │
│  kube-proxy  →  iptables / IPVS                           │
└───────────────────────────────────────────────────────────┘
```

**控制平面组件**：
- **kube-apiserver**：集群统一入口，所有组件通过 REST API 与之交互，负责认证、授权、准入控制。
- **etcd**：强一致性的分布式键值存储，保存集群全部状态数据。
- **kube-scheduler**：根据资源请求、亲和性、污点容忍等策略，将未调度的 Pod 分配到合适的节点。
- **kube-controller-manager**：运行 Deployment、ReplicaSet、Node、Endpoint 等内置控制循环。
- **cloud-controller-manager**：与云厂商 API 交互，管理负载均衡器、存储卷、路由等。

**工作节点组件**：
- **kubelet**：节点代理，向 API Server 汇报状态，并通过 CRI 接口管理容器生命周期。
- **kube-proxy**：维护节点上的网络规则，实现 Service 的负载均衡。
- **容器运行时**：containerd 或 CRI-O，负责实际拉取镜像并启动容器。

声明式工作流：用户提交 YAML 清单 → apiserver 写入 etcd → scheduler 完成调度 → kubelet 感知到新 Pod → 调用 CRI 启动容器 → controller 持续对比期望状态与实际状态并驱动收敛（reconciliation loop）。

## 关键组件与特性

| 特性 | 说明 |
|------|------|
| 声明式 API | 用户描述"期望状态"，系统自动驱动实际状态收敛 |
| 自愈能力 | Pod 崩溃、节点宕机后自动重新调度 |
| 水平伸缩 | 通过 HPA/VPA 实现自动弹性 |
| 服务发现与负载均衡 | Service + Endpoints 提供稳定访问入口 |
| 滚动更新与回滚 | Deployment 原生支持无损发布 |
| 存储编排 | PV/PVC/StorageClass 动态供给 |
| Secret/ConfigMap | 配置与敏感信息分离管理 |

## 配置示例

一个典型的 Deployment + Service 声明式清单：

```yaml
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp
  namespace: production
  labels:
    app: webapp
spec:
  replicas: 3
  selector:
    matchLabels:
      app: webapp
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    metadata:
      labels:
        app: webapp
    spec:
      containers:
      - name: webapp
        image: registry.example.com/webapp:v1.2.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 250m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        readinessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
---
apiVersion: v1
kind: Service
metadata:
  name: webapp
  namespace: production
spec:
  selector:
    app: webapp
  ports:
  - port: 80
    targetPort: 8080
  type: ClusterIP
```

## 常用操作与命令

```bash
# 集群信息
kubectl cluster-info
kubectl get nodes -o wide

# 工作负载管理
kubectl apply -f deployment.yaml
kubectl scale deployment webapp --replicas=5
kubectl rollout status deployment/webapp
kubectl rollout undo deployment/webapp

# 排查与调试
kubectl describe pod <pod-name>
kubectl logs <pod-name> -c <container> --tail=200 -f
kubectl exec -it <pod-name> -- /bin/sh
kubectl get events --sort-by='.lastTimestamp'

# 集群运维
kubectl cordon <node>      # 标记节点不可调度
kubectl drain <node>       # 驱散节点上的 Pod
kubectl uncordon <node>    # 恢复调度
```

## 最佳实践

1. **一切皆声明式**：使用 GitOps（Argo CD / Flux）管理清单，避免手工 `kubectl edit`。
2. **设置资源 requests/limits**：未设 requests 的 Pod 可能导致调度失败或节点过载。
3. **命名空间隔离**：按团队/环境划分 Namespace，配合 ResourceQuota 和 LimitRange。
4. **健康探针必备**：livenessProbe 与 readinessProbe 缺一不可，前者触发重启，后者控制流量。
5. **版本跟随 LTS**：保持集群在最近三个次要版本内，每次只跨一个版本升级。
6. **RBAC 最小权限**：避免使用 cluster-admin，为每个服务账户配置精确的 Role/RoleBinding。

## 常见陷阱

- **Pod 处于 Pending**：通常是资源不足、调度策略冲突或 PVC 未绑定，用 `kubectl describe pod` 查看 Events。
- **CrashLoopBackOff**：容器反复崩溃，检查应用日志、启动命令、资源 limit 过低或依赖服务不可达。
- **Service 无 Endpoints**：selector 与 Pod 标签不匹配，或 readinessProbe 持续失败。
- **ImagePullBackOff**：镜像仓库鉴权失败（缺 imagePullSecrets）或镜像 tag 不存在。
- **升级后 API 弃用**：v1.22 移除 batch/v1beta1 CronJob 等，需提前迁移清单。

## 相关概念

- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[概念/pods.md|Pod]] — 最小调度单元
- [[概念/service.md|Service]] — 服务发现与负载均衡
- [[概念/deployments.md|Deployment]] — 无状态工作负载
- [[概念/container-runtime.md|Container Runtime]] — 容器运行时
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
