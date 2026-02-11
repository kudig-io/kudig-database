# Kubernetes Workload 全栈进阶培训 (从入门到专家)

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 全栈技术实战指南
> **目标受众**: 开发者、运维初学者、SRE 专家
> **核心原则**: 掌握声明式编排、实现应用高可用稳定性防护

---

## 🔰 第一阶段：快速入门与核心概念

### 1.1 什么是 Workload (负载)？
*   **概念**: 负载是在 Kubernetes 上运行的应用程序。
*   **核心类型**:
    *   **Deployment**: 运行无状态应用（如 Web 服务）。支持滚动更新。
    *   **StatefulSet**: 运行有状态应用（如 MySQL, Redis）。有固定标识。
    *   **DaemonSet**: 在每个节点运行一个副本（如日志采集器）。
    *   **Job/CronJob**: 运行离线任务或定时任务。

### 1.2 Deployment 简单示例
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-deploy
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
        ports:
        - containerPort: 80
```

---

## 📘 第二阶段：核心架构与深度原理 (Deep Dive)

### 2.1 滚动更新原理
*   **ReplicaSet**: Deployment 并不直接管理 Pod，而是通过管理 ReplicaSet 来实现版本控制。
*   **策略参数**: `maxSurge` (允许超出副本数) 和 `maxUnavailable` (允许不可用副本数)。

### 2.2 状态机与控制器循环
*   **Informer**: 持续监听预期状态与实际状态的差异并自动修正。

---

## ⚡ 第三阶段：生产部署与稳定性防御

### 2.1 资源 QOS 隔离
*   **Request vs Limit**: 
    *   Request 决定调度。
    *   Limit 决定上限（超出会导致 Throttle 或 OOM）。
*   **Guaranteed 级别**: Request == Limit，最稳定的运行级别。

### 2.2 亲和性与反亲和性
*   **必须分散**: 通过 `podAntiAffinity` 确保 Pod 不会挤在同一个节点上。

---

## 📈 第四阶段：监控告警与弹性伸缩 (Performance)

### 3.1 弹性伸缩 (HPA)
*   基于 CPU/Memory 自动增减副本数。
*   **SRE 建议**: 核心业务必须配置 HPA，且配合 `behavior` 调整缩容窗口。

### 3.2 探针 (Probe)
*   **Liveness**: 活没活（死掉就重启）。
*   **Readiness**: 能不能接流量（不通过就切断流量）。
*   **Startup**: 解决大应用启动慢的杀手。

---

## 🛠️ 第五阶段：排障实战与 SRE 运维

### 4.1 典型故障诊断
*   **CrashLoopBackOff**: 检查日志 `kubectl logs`。
*   **Pending**: 检查调度 `kubectl describe pod`。
*   **OOMKilled**: 内存 Limit 设置过小。

### 🏆 SRE 运维红线
*   *红线 1: 生产环境严禁配置 `limits.cpu < requests.cpu`。*
*   *红线 2: 核心业务必须配置 `ReadinessProbe`。*
*   *红线 3: 所有 Pod 必须配置资源配额。*
