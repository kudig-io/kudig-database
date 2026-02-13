# Kubernetes 架构与基础概念全栈培训

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 基础架构通识
> **目标受众**: 所有人（入门到专家）
> **核心原则**: 理解分布式系统设计哲学、掌握集群核心组件、构建云原生思维

---

## 🔰 第一阶段：什么是 Kubernetes？

### 1.1 核心价值
*   **不只是容器调度**: 它是分布式系统的操作系统（Cloud OS）。
*   **核心功能**: 自愈 (Self-healing)、弹性扩缩 (Scaling)、服务发现与负载均衡、滚动更新。
*   **设计哲学**: **声明式 API (Declarative API)**。你告诉它“我要 3 个副本”，它负责“对齐”到这个状态。

### 1.2 核心术语 (Beginner Checklist)
*   **Pod**: 最小调度单位，一个或多个容器的组合。
*   **Node**: 集群中的机器（物理机或虚拟机）。
*   **Namespace**: 逻辑隔离环境（类似文件夹）。
*   **Context**: 连接集群的“凭证”与环境定义。

---

## 📘 第二阶段：架构深度解析 (Control Plane & Data Plane)

### 2.1 控制平面 (Control Plane) - “大脑”
*   **kube-apiserver**: 唯一入口，资源操作的集散地。
*   **etcd**: 强一致性数据库，存储集群所有状态。
*   **kube-scheduler**: “打分员”，决定 Pod 该去哪个 Node。
*   **kube-controller-manager**: “监控员”，维持各种资源的状态对齐。

### 2.2 数据平面 (Data Plane) - “躯干”
*   **kubelet**: Node 上的代理，负责管理 Pod 的生命周期。
*   **kube-proxy**: 网络规则维护者。
*   **Container Runtime**: 容器运行时（如 containerd）。

---

## ⚡ 第三阶段：生产环境稳定性与高可用

### 2.1 控制平面高可用 (HA)
*   **etcd 仲裁**: 必须部署奇数个节点 (3, 5, 7) 以应对脑裂。
*   **多 Master 架构**: 通过 SLB 暴露 apiserver 实现负载均衡。

### 2.2 资源隔离策略
*   **ResourceQuota**: 限制 Namespace 的资源总量。
*   **LimitRange**: 限制单个 Pod 的资源上下限。

---

## 🛠️ 第四阶段：日常排障与 SRE 运维

### 3.1 生命周期全追踪
1. `kubectl apply` -> apiserver -> etcd。
2. scheduler 发现新 Pod -> 打分 -> 选 Node -> 写回 apiserver。
3. kubelet 监听到调度到本机的 Pod -> 调用 CNI 网络 -> 调用 CSI 存储 -> 启动容器。

### 🏆 SRE 运维红线
*   *红线 1: 生产环境 etcd 严禁部署在普通 HDD 磁盘上（必须 SSD）。*
*   *红线 2: 严禁在 Master 节点运行任何业务 Pod。*
*   *红线 3: 任何对 apiserver 的访问必须经过鉴权。*
