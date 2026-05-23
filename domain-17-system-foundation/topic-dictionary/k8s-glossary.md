---
title: K8s 中英术语表（Glossary）
description: '## 1. 架构与组件（Architecture & Components）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 中英术语表（Glossary） 是什么
- 如何 K8s 中英术语表（Glossary）
trigger_keywords:
- K8s
- 中英术语表
- Glossary
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- logging-basics
created: "2026-05-23"
---

# K8s 中英术语表（Glossary）

> **适用对象**: SRE/Ops 工程师新人培训 | **版本**: K8s 1.28-1.33

---

## 1. 架构与组件（Architecture & Components）

### 控制平面（Control Plane）

| 英文 | 中文 | 说明 |
|------|------|------|
| Control Plane | 控制平面 | 负责整个集群的管理和控制 |
| kube-apiserver | API Server | K8s 集群的入口，REST API 提供者 |
| kube-scheduler | 调度器 | 将 Pod 分配到合适的节点 |
| kube-controller-manager | 控制器管理器 | 运行各种控制器（Deployment、[[ReplicaSet|ReplicaSet]] 等） |
| cloud-controller-manager | 云控制器管理器 | 与云厂商 API 交互，管理节点/负载均衡/路由 |
| [[etcd|etcd]] | etcd | 分布式键值存储，保存集群所有数据 |

### 节点组件（Node Components）

| 英文 | 中文 | 说明 |
|------|------|------|
| Node | 节点 | K8s 集群中的 worker 机器 |
| [[kubelet|kubelet]] | kubelet | 节点上的代理，负责管理 Pod 和容器 |
| kube-proxy | kube-proxy | 节点上的网络代理，处理 Service 流量 |
| Container Runtime | 容器运行时 | 负责运行容器（containerd/Docker/CRI-O） |

### 工作节点（Worker Node）

| 英文 | 中文 | 说明 |
|------|------|------|
| Worker Node | 工作节点 | 运行用户工作负载的节点 |
| Master Node | 主节点 | 运行控制平面组件的节点（通常高可用部署） |
| Control Plane Node | 控制平面节点 | 同 Master Node |

---

## 2. 核心资源对象（Core Resources）

### 工作负载（Workloads）

| 英文 | 中文 | 说明 |
|------|------|------|
| Pod | Pod | K8s 的最小调度单元，包含一个或多个容器 |
| Deployment | Deployment | 管理无状态应用的工作负载控制器 |
| StatefulSet | 有状态副本集 | 管理有状态应用的工作负载控制器 |
| DaemonSet | 守护进程集 | 确保每个节点都运行一个 Pod 副本 |
| Job | 任务 | 创建一次性执行的 Pod |
| CronJob | 定时任务 | 按时间计划创建 Pod |
| ReplicaSet | 副本集 | 确保指定数量的 Pod 副本运行（通常由 Deployment 管理） |

### 服务发现与网络（Service Discovery & Networking）

| 英文 | 中文 | 说明 |
|------|------|------|
| Service | 服务 | 为 Pod 提供稳定的访问入口（负载均衡） |
| Ingress | 入口 | 管理集群外部 HTTP/HTTPS 访问 |
| Endpoints | 端点 | Service 后端 Pod 的 IP 和端口组合 |
| Ingress Controller | 入口控制器 | 实现 Ingress 规则的组件（如 Nginx Ingress Controller） |
| NetworkPolicy | 网络策略 | 控制 Pod 之间/外部的网络流量 |
| CNI (Container Network Interface) | 容器网络接口 | 容器网络插件的标准接口 |
| DNS | 域名服务 | 集群内部的域名解析服务（CoreDNS） |

### 存储（Storage）

| 英文 | 中文 | 说明 |
|------|------|------|
| PersistentVolume (PV) | 持久化卷 | 集群级别的存储资源 |
| PersistentVolumeClaim (PVC) | 持久化卷声明 | Pod 申请存储资源的请求 |
| StorageClass | 存储类 | 存储资源的分类，用于动态制备 |
| Volume | 卷 | 容器挂载的存储 |
| emptyDir | 空目录卷 | 临时存储，Pod 删除后丢失 |
| hostPath | 主机路径卷 | 映射宿主机文件系统路径 |
| CSI (Container Storage Interface) | 容器存储接口 | 存储插件的标准接口 |

### 配置与安全（Config & Security）

| 英文 | 中文 | 说明 |
|------|------|------|
| ConfigMap | 配置映射 | 存储非敏感的配置数据 |
| Secret | 密钥 | 存储敏感数据（密码/Token/证书） |
| ServiceAccount | 服务账号 | Pod 用于认证的身份 |
| Role | 角色 | 命名空间级别的权限定义 |
| ClusterRole | 集群角色 | 集群级别的权限定义 |
| RoleBinding | 角色绑定 | 将 Role/ClusterRole 绑定到主体 |
| ClusterRoleBinding | 集群角色绑定 | 将 ClusterRole 绑定到集群范围的主体 |
| RBAC (Role-Based Access Control) | 基于角色的访问控制 | K8s 的权限管理机制 |

---

## 3. 调度与资源（Scheduling & Resources）

| 英文 | 中文 | 说明 |
|------|------|------|
| Scheduler | 调度器 | 决定 Pod 应该调度到哪个节点 |
| nodeSelector | 节点选择器 | 将 Pod 调度到特定标签的节点 |
| Affinity | 亲和性 | 表达 Pod 对节点或其他 Pod 的偏好 |
| Anti-Affinity | 反亲和性 | 表达 Pod 不想与某些 Pod 调度到一起 |
| Taint | 污点 | 节点上的标记，表示节点不应该接受特定 Pod |
| Toleration | 容忍 | Pod 接受节点污点的能力 |
| Topology | 拓扑 | 表示节点的位置（可用区/区域等） |
| topologySpreadConstraints | 拓扑分布约束 | 控制 Pod 在拓扑域间的分布 |

### 资源模型（Resource Model）

| 英文 | 中文 | 说明 |
|------|------|------|
| Resource Request | 资源请求 | 容器需要多少资源（用于调度） |
| Resource Limit | 资源限制 | 容器最多使用多少资源（用于限制） |
| QoS (Quality of Service) | 服务质量 | Pod 的优先级分类（Guaranteed/Burstable/BestEffort） |
| LimitRange | 限制范围 | 限制命名空间中每个容器的资源使用 |
| ResourceQuota | 资源配额 | 限制命名空间的总资源使用 |

### 探针与健康检查（Probe & Health Check）

| 英文 | 中文 | 说明 |
|------|------|------|
| Liveness Probe | 存活探针 | 检测容器是否存活，失败会重启容器 |
| Readiness Probe | 就绪探针 | 检测容器是否就绪，失败会从 Service 移除 |
| Startup Probe | 启动探针 | 检测容器是否启动完成，启动期间禁用其他探针 |
| Probe | 探针 | 健康检查机制 |
| Graceful Shutdown | 优雅关闭 | 容器终止前允许清理操作 |

---

## 4. API 与工具（API & Tools）

### API 概念

| 英文 | 中文 | 说明 |
|------|------|------|
| API Group | API 组 | 相关 API 资源的分组（如 apps, batch） |
| API Version | API 版本 | API 的版本（如 v1, v1beta1） |
| Kind | 类型 | K8s 资源对象的类型（如 Pod, Deployment） |
| Manifest | 清单 | YAML/JSON 格式的 K8s 资源定义文件 |
| kubectl | kubectl | K8s 命令行工具 |

### 工具

| 英文 | 中文 | 说明 |
|------|------|------|
| kubeadm | kubeadm | 集群初始化和升级工具 |
| kubectx | kubectx | 快速切换集群上下文 |
| kubens | kubens | 快速切换命名空间 |
| k9s | k9s | 终端 UI 管理集群 |
| stern | stern | 日志实时跟踪工具 |
| etcdctl | etcdctl | etcd 命令行客户端 |

---

## 5. 运维概念（Operations）

| 英文 | 中文 | 说明 |
|------|------|------|
| Cordon | 封锁节点 | 阻止新 Pod 调度到该节点 |
| Drain | 驱逐 | 安全迁移节点上的 Pod |
| Uncordon | 解封节点 | 允许新 Pod 调度到该节点 |
| Rolling Update | 滚动更新 | 逐步更新 Pod 的策略 |
| Rollback | 回滚 | 将应用恢复到之前的版本 |
| Scale | 扩缩容 | 增加或减少 Pod 副本数 |
| Upgrade | 升级 | 升级集群或组件版本 |

### 监控与日志

| 英文 | 中文 | 说明 |
|------|------|------|
| Metrics Server | 指标服务器 | 收集集群资源使用指标 |
| Prometheus | Prometheus | 监控系统 |
| Grafana | Grafana | 可视化仪表盘 |
| Alertmanager | 告警管理器 | 处理和发送告警 |
| [[domain-19-landscape-references/01-cncf-landscape/graduated/fluentd/fluentd|Fluentd]]/Fluent Bit | 日志收集器 | 收集容器日志 |
| Kubernetes Events | K8s 事件 | 集群中发生的操作记录 |

---

## 6. 安全概念（Security）

| 英文 | 中文 | 说明 |
|------|------|------|
| Admission Controller | 准入控制器 | 请求拦截和处理的插件 |
| Webhook | Webhook | 外部服务回调机制 |
| NetworkPolicy | 网络策略 | 限制网络流量 |
| Pod Security Policy (PSP) | Pod 安全策略 | 控制 Pod 安全配置 |
| Pod Security Standards (PSS) | Pod 安全标准 | 新的安全标准（替代 PSP） |
| SecurityContext | 安全上下文 | 容器安全配置 |
| ServiceAccount Token | 服务账号令牌 | Pod 用于认证的 Token |
| Certificate | 证书 | TLS 证书 |
| Certificate Authority (CA) | 证书颁发机构 | 签发证书的机构 |

---

## 7. 网络概念（Networking）

| 英文 | 中文 | 说明 |
|------|------|------|
| ClusterIP | 集群 IP | Service 的内部虚拟 IP |
| NodePort | 节点端口 | 通过节点端口暴露服务 |
| LoadBalancer | 负载均衡器 | 通过外部负载均衡器暴露服务 |
| ExternalName | 外部名称 | 将 Service 映射到外部 DNS 名称 |
| DNS Resolution | DNS 解析 | 将域名转换为 IP 地址 |
| kube-dns / CoreDNS | 核心 DNS | K8s 集群 DNS 服务 |
| CNI (Container Network Interface) | 容器网络接口 | 容器网络插件标准 |
| VXLAN | VXLAN | 虚拟可扩展 LAN（隧道协议） |
| IPIP | IPIP | IP 隧道协议 |
| NAT (Network Address Translation) | 网络地址转换 | IP 地址转换 |

---

## 8. 常见缩写

| 缩写 | 全称 | 中文 |
|------|------|------|
| K8s | Kubernetes | 容器编排平台 |
| CNCF | Cloud Native Computing Foundation | 云原生计算基金会 |
| CRI | Container Runtime Interface | 容器运行时接口 |
| CSI | Container Storage Interface | 容器存储接口 |
| CNI | Container Network Interface | 容器网络接口 |
| RBAC | Role-Based Access Control | 基于角色的访问控制 |
| HPA | Horizontal Pod Autoscaler | 水平 Pod 自动扩缩容 |
| VPA | Vertical Pod Autoscaler | 垂直 Pod 自动扩缩容 |
| PDB | Pod Disruption Budget | Pod 中断预算 |
| SSA | Server-Side Apply | 服务器端应用 |

---

```yaml
---
id: K8S-GLOSSARY-001
topic: dictionary
type: glossary
tags: [glossary, terminology, dictionary, k8s, sre, ops-engineer, k8s-1.28-1.33]
intent_queries:
  - "K8s 术语表"
  - "中英对照"
  - "术语解释"
difficulty: beginner
target_roles: [sre, ops-engineer, developer]
related:
  - domain-11-production-operations/topic-learn/quick-start/01-day-one-checklist.md
  - domain-11-production-operations/topic-learn/quick-start/04-debug-tools-setup.md
---
```