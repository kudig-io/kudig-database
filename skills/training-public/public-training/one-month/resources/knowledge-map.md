---
title: 知识图谱模板
description: 使用此模板记录你的学习成果，构建个人知识图谱。每完成一个模块的学习，在对应区域用自己的语言总结核心概念、记录仍需加强的领域，并画出你理解的架构图。
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- flannel
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- beginner-devops
- developer
- platform-engineer
estimated_read_time: 5min
intent_queries:
- k8s 知识图谱怎么画
- kubernetes 知识体系全景图
- 学习笔记模板 知识梳理
- 个人知识库构建方法
trigger_keywords:
- 知识图谱
- 学习笔记
- 知识体系
- 模板
- 架构图
- 概念总结
- 学习路径
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- cni-basics
- etcd-basics
- logging-basics
related_domains:
- domain-01-cluster-fundamentals
- domain-02-workloads-applications
- domain-03-networking-traffic
- domain-05-security-compliance
- domain-06-observability
related_topics:
- domain-11-production-operations/topic-learn/public-training/one-month/resources/reading-sequence
- domain-11-production-operations/topic-learn/public-training/one-month/resources/commands-cheatsheet
created: "2026-05-23"
---

# 知识图谱模板

使用此模板记录你的学习成果，构建个人知识图谱。每完成一个模块的学习，在对应区域用自己的语言总结核心概念、记录仍需加强的领域，并画出你理解的架构图。

---

## Week 1: 地基建设期

### Docker

**核心概念:**
- [ ] Docker Engine 架构（Client-Server 模型、daemon、[[containerd|containerd]]）
- [ ] 镜像 vs 容器（镜像 = 只读模板，容器 = 运行实例）
- [ ] Union Filesystem（分层存储、Copy-on-Write）
- [ ] 网络模式 (bridge/host/overlay/none)
- [ ] 存储 (Volume/Bind Mount/tmpfs)

**我的理解:**
```
Docker 是容器化平台，核心是 Linux Namespace（隔离）和 Cgroup（资源限制）。
镜像通过分层存储实现高效复用，容器运行时共享宿主机内核。

关键命令速查:
docker build -t app:v1 .         # 构建镜像
docker run -d -p 80:80 nginx     # 运行容器
docker exec -it <id> sh          # 进入容器
docker logs <id>                 # 查看日志
docker system prune -a           # 清理资源
```

**还需加强:**
```
(记录需要进一步学习的领域)
```

---

### Linux

**核心概念:**
- [ ] namespace (7种类型: PID/Net/Mnt/IPC/UTS/User/Cgroup)
- [ ] cgroup (资源限制: CPU/Memory/IO/PID)
- [ ] 进程管理 (ps/top/systemd)
- [ ] 网络配置 (ip/iptables/tc)
- [ ] 存储管理 (LVM/disk/partition)

**我的理解:**
```
Linux Namespace 是容器隔离的基础技术:
- PID Namespace: 进程 ID 隔离
- Network Namespace: 网络栈隔离
- Mount Namespace: 文件系统挂载隔离
- User Namespace: 用户 ID 隔离

Cgroup 用于资源限制:
- cpu.cfs_quota_us: CPU 时间限制
- memory.limit_in_bytes: 内存使用上限
- 这些是 Kubernetes resources.limits 的底层实现

关键命令:
ip netns list                        # 列出网络命名空间
lsns -p <pid>                        # 查看进程的命名空间
cat /proc/<pid>/cgroup               # 查看 cgroup 信息
```

**还需加强:**
```
```

---

### K8s 架构

**核心概念:**
- [ ] 控制平面: [[etcd|etcd]], API Server, Scheduler, Controller Manager
- [ ] 数据平面: [[kubelet|kubelet]], kube-proxy, Container Runtime
- [ ] 声明式管理（Desired State vs Actual State）
- [ ] 控制器模式（Reconcile Loop）

**架构图:**
```
                    ┌─────────────────────────────────────────┐
                    │              控制平面                     │
                    │  ┌─────────┐  ┌──────────┐  ┌────────┐ │
                    │  │API Server│  │Scheduler │  │Ctrl Mgr│ │
                    │  └────┬────┘  └──────────┘  └────────┘ │
                    │       │                                  │
                    │  ┌────▼────┐                             │
                    │  │  etcd   │  ← 唯一数据存储              │
                    │  └─────────┘                             │
                    └──────────┬──────────────────────────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
        ┌─────▼─────┐   ┌─────▼─────┐   ┌─────▼─────┐
        │  Node 1    │   │  Node 2    │   │  Node 3    │
        │ ┌────────┐ │   │ ┌────────┐ │   │ ┌────────┐ │
        │ │kubelet │ │   │ │kubelet │ │   │ │kubelet │ │
        │ │kube-proxy│ │   │ │kube-proxy│ │   │ │kube-proxy│ │
        │ │Runtime │ │   │ │Runtime │ │   │ │Runtime │ │
        │ └────────┘ │   │ └────────┘ │   │ └────────┘ │
        │  Pod Pod   │   │  Pod Pod   │   │  Pod Pod   │
        └────────────┘   └────────────┘   └────────────┘
```

**我的理解:**
```
K8s 采用声明式管理：用户声明期望状态，控制器通过 Reconcile Loop 持续将实际状态调整到期望状态。
API Server 是所有操作的入口，etcd 是唯一的数据存储。
kubelet 负责节点上的 Pod 生命周期管理，kube-proxy 负责服务发现和负载均衡。

核心工作流程:
1. kubectl apply → API Server → etcd (写入期望状态)
2. Controller Manager 检测到变化 → 创建 ReplicaSet → 创建 Pod
3. Scheduler 检测到未调度的 Pod → 选择合适节点 → 绑定
4. kubelet 检测到绑定的 Pod → 拉取镜像 → 启动容器
```

---

## Week 2: 核心技术构建期

### 控制平面

**核心概念:**
- [ ] etcd Raft 协议（Leader 选举、日志复制、多数派确认）
- [ ] API Server 请求链（认证 → 授权 → 准入控制 → etcd）
- [ ] Scheduler Filter/Score（节点过滤 + 打分排序）
- [ ] Controller Reconcile 循环（期望状态 vs 实际状态）

**我的理解:**
```
控制平面四大组件协作:
- etcd: 存储 (Raft 协议保证一致性)
- API Server: 网关 (认证/授权/准入)
- Scheduler: 调度 (Filter + Score 两阶段)
- Controller Manager: 控制循环 (Deployment/ReplicaSet/Node Controller)

关键排障命令:
kubectl get cs                          # 组件状态
etcdctl endpoint health                 # etcd 健康
kubectl logs -n kube-system -l component=kube-apiserver  # API Server 日志
```

---

### 工作负载

**核心概念:**
- [ ] Deployment 滚动更新（RollingUpdate strategy）
- [ ] StatefulSet 有序部署（有序索引、稳定网络标识）
- [ ] DaemonSet 每节点运行（日志采集、监控 Agent）
- [ ] Pod 生命周期和探针（livenessProbe/readinessProbe/startupProbe）
- [ ] 资源管理 (QoS: Guaranteed/Burstable/BestEffort)
- [ ] HPA 自动扩缩容（基于 CPU/内存/自定义指标）

**对比总结:**
| 类型 | 特点 | 使用场景 | 存储支持 |
|------|------|----------|---------|
| Deployment | 无状态、滚动更新 | Web 服务、API | 共享 PVC |
| StatefulSet | 有状态、有序部署 | 数据库、ZooKeeper | volumeClaimTemplates |
| DaemonSet | 每节点一个 | 日志采集、监控 | hostPath |

**HPA 配置示例:**
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

---

### 网络

**核心概念:**
- [ ] K8s 网络模型（每个 Pod 有独立 IP、Pod 间直接通信）
- [ ] CNI 插件机制（Terway/Flannel/Calico）
- [ ] Service 四种类型（ClusterIP/NodePort/LoadBalancer/ExternalName）
- [ ] CoreDNS 服务发现（Service → ClusterIP → DNS A 记录）
- [ ] Ingress 路由（基于域名/路径的 L7 路由）
- [ ] NetworkPolicy（Pod 级网络访问控制）

**网络流程图:**
```
外部用户 → DNS 解析 → Ingress Controller → Service → Pod
              │                                      │
              ▼                                      ▼
         app.example.com                    10.0.1.100:8080
              │                                      │
         Ingress 规则                          Endpoints 列表
         host: app.example.com               - 10.0.1.100:8080
         path: / → web-svc                   - 10.0.1.101:8080
              │                               - 10.0.1.102:8080
              ▼
         Service (ClusterIP: 10.96.1.100)
         → kube-proxy (iptables/ipvs)
         → Pod (负载均衡)
```

---

### 存储

**核心概念:**
- [ ] PV/PVC 绑定（静态供应 vs 动态供应）
- [ ] StorageClass 动态供应（自动创建 PV）
- [ ] 访问模式 (RWO/ROX/RWX)
- [ ] Reclaim Policy (Retain/Delete/Recycle)

**存储流程:**
```
PVC 创建 → StorageClass 匹配 → CSI 插件 → 云盘创建 → PV 绑定 → Pod 挂载
```

---

## Week 3: 运维作战能力期

### 安全

**核心概念:**
- [ ] RBAC 四种资源（Role/ClusterRole/RoleBinding/ClusterRoleBinding）
- [ ] ServiceAccount（Pod 身份标识）
- [ ] Pod Security Standards（Privileged/Baseline/Restricted）
- [ ] Secret 管理（Base64 编码、etcd 加密、外部 Secret）

**RBAC 权限设计:**
```
用户 → RoleBinding → Role (namespace 级别)
用户 → ClusterRoleBinding → ClusterRole (集群级别)
用户 → RoleBinding → ClusterRole (限制在 namespace)
```

---

### 可观测性

**核心概念:**
- [ ] Metrics/Logs/Traces 三支柱
- [ ] Prometheus 数据模型（Counter/Gauge/Histogram/Summary）
- [ ] PromQL 查询（rate/sum/histogram_quantile）
- [ ] Alertmanager 路由（分组/抑制/静默）
- [ ] Loki 日志查询（LogQL）

**监控架构图:**
```
Pod → Prometheus (采集+存储) → Grafana (可视化)
           │
           └→ PrometheusRule (告警规则) → Alertmanager (路由) → 钉钉/企微
```

---

### 故障排查

**核心概念:**
- [ ] FTA 故障树分析（自顶向下、逻辑门分解）
- [ ] FEBM 取证循证（证据收集→假设生成→验证确认）
- [ ] 结构化排障流程（分层排查: DNS → 网络 → 存储 → 控制面）

**常见问题速查:**
| 现象 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pending | 资源不足/调度约束 | describe pod 看 Events |
| CrashLoopBackOff | 启动失败/探针失败 | logs --previous 看日志 |
| OOMKilled | 内存限制过低/泄漏 | top pod 看内存趋势 |
| ImagePullBackOff | 镜像不存在/凭证错误 | describe pod 看 Events |

---

## Week 4: 企业级进阶期

### GitOps

**核心概念:**
- [ ] GitOps 原则（声明式/版本控制/自动拉取/持续协调）
- [ ] ArgoCD 工作流（Application/SyncPolicy/Health Check）
- [ ] 多环境管理（base + overlays / Kustomize）
- [ ] Kustomize/Helm（配置管理/包管理）

---

### 生产运维

**核心概念:**
- [ ] SLO/SLI 体系（可用性 99.9%/错误率 < 0.1%/P99 < 500ms）
- [ ] 变更管理（标准/正常/紧急/重大变更）
- [ ] 事故响应（P1-P4 分级、IC 角色制度、MTTD/MTTR）
- [ ] 容量规划（增长率预测、冗余系数、安全水位）

---

## 总结

### 最有价值的知识点

1. 
2. 
3. 

### 仍需深入的领域

1. 
2. 
3. 

### 下一步学习计划

1. 
2. 
3. 

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
