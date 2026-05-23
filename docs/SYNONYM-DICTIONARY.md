---
title: KUDIG 同义词与别名词典
description: '| `controller-manager` | kube-controller-manager, KCM, 控制器管理器 |'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
- istio
- envoy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 同义词与别名词典 是什么
- 如何 KUDIG 同义词与别名词典
trigger_keywords:
- KUDIG
- 同义词与别名词典
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- iac-basics
- ebpf-basics
- cilium-basics
- etcd-basics
created: "2026-05-23"
---

---
title: KUDIG 同义词与别名词典
description: KUDIG 同义词与别名词典
category: docs
tags:
- k8s
- dictionary
- metadata
relationships:
- target: '[[docs/TAG-DICTIONARY|KUDIG 全局标签字典]]'
  type: related_to
- target: '[[docs/FRONTMATTER-SPEC|KUDIG Frontmatter 规范]]'
  type: related_to
- target: '[[docs/SCENARIO-TAXONOMY|KUDIG 场景分类体系]]'
  type: related_to
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- DevOps
estimated_read_time: 10min
last_updated: 2026-05
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---

# KUDIG 同义词与别名词典

> 创建时间: 2026-05-20
> 用途: 解决 Agent/RAG 检索的词汇不匹配问题
> 格式: 标准名 → [别名1, 别名2, ...]

---

## Kubernetes 核心组件

| 标准名 | 别名 |
|---|---|
| `api-server` | apiserver, kube-apiserver, API Server, API服务器 |
| `controller-manager` | kube-controller-manager, KCM, 控制器管理器 |
| `scheduler` | kube-scheduler, K8s调度器, 调度程序 |
| `etcd` | 键值存储, KV store, 分布式存储 |
| `kubelet` | 节点代理, node agent, kube-proxy (误用) |
| `kube-proxy` | 网络代理, kube-proxy, 服务代理 |
| `coredns` | CoreDNS, kube-dns, DNS服务器, 域名解析 |
| `containerd` | container runtime, 容器运行时, cri |
| `cri-o` | CRI-O, cri-o, 容器运行时接口 |
| `kube-apiserver` | api server, k8s api, 控制面API |

## 工作负载资源

| 标准名 | 别名 |
|---|---|
| `Pod` | pod, 容器组, 工作单元 |
| `Deployment` | deploy, 部署, 应用部署 |
| `StatefulSet` | sts, 有状态集, 有状态应用 |
| `DaemonSet` | ds, 守护进程集, 守护进程 |
| `ReplicaSet` | rs, 副本集, 副本控制器 |
| `Job` | 批处理任务, 一次性任务 |
| `CronJob` | cj, 定时任务, 计划任务 |
| `ConfigMap` | cm, 配置映射, 配置 |
| `Secret` | 密钥, 凭据, credentials |
| `Service` | svc, 服务, 服务发现 |
| `Ingress` | 入口, 网关入口 |
| `Namespace` | ns, 命名空间, 项目 |

## 存储资源

| 标准名 | 别名 |
|---|---|
| `PersistentVolume` | pv, 持久卷, 持久化存储 |
| `PersistentVolumeClaim` | pvc, 持久卷声明, 存储请求 |
| `StorageClass` | sc, 存储类, 动态存储 |
| `CSI` | Container Storage Interface, 存储接口 |

## 网络资源

| 标准名 | 别名 |
|---|---|
| `Service` | svc, ClusterIP, NodePort, LoadBalancer |
| `NetworkPolicy` | netpol, 网络策略, 网络隔离 |
| `Ingress` | 入口规则, 路由规则, 网关 |
| `CNI` | 容器网络接口, 网络插件 |
| `DNS` | 域名解析, 域名系统, name resolution |
| `Endpoints` | ep, 端点, 服务后端 |

## 安全资源

| 标准名 | 别名 |
|---|---|
| `RBAC` | Role-Based Access Control, 角色权限, 访问控制 |
| `ServiceAccount` | sa, 服务账号, SA |
| `NetworkPolicy` | 网络策略, 网络隔离 |
| `PodSecurityPolicy` | psp, pod 安全策略 |
| `PodSecurityAdmission` | psa, pod 安全准入 |
| `TLS` | SSL, 证书, 传输层安全 |
| `Certificate` | 证书, cert, 加密证书 |

## 自动伸缩

| 标准名 | 别名 |
|---|---|
| `HPA` | Horizontal Pod Autoscaler, 水平自动伸缩, 自动扩缩容 |
| `VPA` | Vertical Pod Autoscaler, 垂直自动伸缩 |
| `KEDA` | Kubernetes Event-driven Autoscaling, 事件驱动伸缩 |
| `Cluster Autoscaler` | CA, 集群自动伸缩, 节点伸缩 |

## 生态项目

| 标准名 | 别名 |
|---|---|
| `Helm` | 包管理器, chart, helm chart |
| `Istio` | 服务网格, istio mesh, sidecar |
| `Prometheus` | prom, 监控, 指标采集 |
| `Grafana` | 可视化, 仪表盘, grafana dashboard |
| `ArgoCD` | argo, argocd, GitOps |
| `Flux` | fluxcd, GitOps, 持续部署 |
| `Cilium` | ebpf网络, cilium cni |
| `Envoy` | 代理, envoy proxy, 侧车代理 |
| `Linkerd` | linkerd, 轻量级mesh |
| `Terraform` | tf, 基础设施即代码, iac |
| `Ansible` | 配置管理, 自动化运维 |

## 常见错误码

| 标准名 | 别名 |
|---|---|
| `CrashLoopBackOff` | CrashLoop, pod崩溃, 循环崩溃 |
| `ImagePullBackOff` | 镜像拉取失败, image pull error |
| `ErrImagePull` | 镜像错误, 拉取错误 |
| `Pending` | pod pending, 等待调度, 调度失败 |
| `OOMKilled` | OOM, 内存溢出, 内存不足, Out Of Memory |
| `ContainerCreating` | 容器创建中, 启动中 |
| `CreateContainerConfigError` | 配置错误, config error |
| `Evicted` | 被驱逐, pod驱逐, 节点压力 |

## 操作术语

| 标准名 | 别名 |
|---|---|
| `kubectl apply` | 应用配置, kubectl, apply |
| `kubectl get` | 查看, 获取, list |
| `kubectl describe` | 详情, 描述, describe |
| `kubectl logs` | 日志, log, 查看日志 |
| `kubectl exec` | 进入容器, 执行, exec |
| `kubectl port-forward` | 端口转发, 本地访问 |
| `kubectl rollout` | 滚动更新, rollout, 回滚 |
| `kubectl scale` | 扩缩容, 伸缩, scale |
| `kubectl delete` | 删除, 移除, delete |

---

## 使用方式

在文档 frontmatter 的 `aliases` 字段中添加相关别名:

```yaml
aliases:
  - "apiserver"
  - "kube-apiserver"
  - "API Server"
```

---

*本文档是同义词体系的权威定义，新增词条时应在此文件中注册。*

---

## Related

- [[references/KUDIG Tag Dictionary|KUDIG Tag Dictionary]]
- [[references/KUDIG Frontmatter Spec|KUDIG Frontmatter Spec]]
- [[references/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]]
- [[docs/TAG-DICTIONARY|KUDIG 全局标签字典]]
