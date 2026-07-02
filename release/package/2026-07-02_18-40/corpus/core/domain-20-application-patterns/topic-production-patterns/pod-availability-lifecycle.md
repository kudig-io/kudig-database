---
title: Pod 可用性与生命周期生产模式
description: 生产级 Pod 可用性保障：探针设计、PDB、优雅终止、就绪门控与滚动更新策略
summary: 生产级 Pod 可用性保障：探针设计、PDB、优雅终止、就绪门控与滚动更新策略，含排障速查与可落地清单。
category: application-patterns
tags:
- pod
- availability
- probes
- pdb
- lifecycle
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 18min
intent_queries:
- Pod 可用性生产模式是什么
- 如何设计生产级探针与 PDB
trigger_keywords:
- 探针
- PDB
- 优雅终止
- 滚动更新
- 可用性
prerequisites:
- kubectl-basics
- pod-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod 可用性与生命周期生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

Pod 是 Kubernetes 中最小的可调度单元，其可用性直接决定应用 SLO。本文涵盖生产环境中保障 Pod 高可用的核心模式：探针设计、PodDisruptionBudget、优雅终止、就绪门控与滚动更新策略。错误的探针配置或缺失 PDB 是生产事故的高频根因——前者导致流量打到未就绪实例，后者导致节点维护期间意外中断。

---

## 1. 生产探针设计矩阵

### 1.1 四种探针的职责边界

| 探针 | 作用 | 失败后果 | 生产建议 |
|---|---|---|---|
| **startupProbe** | 判定容器是否完成初始化 | 阻断 liveness/readiness 执行 | 慢启动应用（JVM/Python）必配，`failureThreshold × periodSeconds` ≥ 最长启动时间 |
| **livenessProbe** | 判定容器是否需要重启 | Pod 被 kill 重启 | 只检测"死锁/死循环"，**不要**检测外部依赖（DB/缓存），否则引发级联重启 |
| **readinessProbe** | 判定 Pod 是否可接收流量 | 从 Endpoints 摘除 | 检测"是否准备好服务请求"，可检测本地依赖连通性 |
| **readinessGates** | 外部条件控制就绪 | Pod NotReady | 用于自定义负载均衡器、外部健康检查集成 |

> ⚠️ **最常见的生产事故**: livenessProbe 检查数据库连通性。当 DB 抖动时，所有 Pod 同时被 kill 引发雪崩。liveness 只应检测进程自身健康。

### 1.2 生产探针模板（HTTP 服务）

```yaml
containers:
  - name: app
    startupProbe:          # 保护慢启动，成功后退出
      httpGet:
        path: /healthz/startup
        port: 8080
      failureThreshold: 30 # 30 × 10s = 5 分钟启动窗口
      periodSeconds: 10
    livenessProbe:         # 仅检测进程死活
      httpGet:
        path: /healthz/live
        port: 8080
      periodSeconds: 10
      failureThreshold: 3  # 连续 3 次失败(30s)才重启
      timeoutSeconds: 3
    readinessProbe:        # 检测是否可服务
      httpGet:
        path: /healthz/ready
        port: 8080
      periodSeconds: 5
      failureThreshold: 2  # 10s 摘除，快速止损
      timeoutSeconds: 2
```

### 1.3 探针端点设计规范

健康检查端点应分层实现，避免单端点承担多重职责：

| 端点 | 检查内容 | 返回 200 条件 |
|---|---|---|
| `/healthz/live` | 进程存活（event loop 心跳） | 进程未死锁 |
| `/healthz/ready` | 本地缓存预热 + 配置加载完成 | 可处理请求 |
| `/healthz/startup` | 依赖初始化（DB 迁移、索引构建） | 初始化完成 |

> 🟢 低风险。验证探针状态: `kubectl describe pod <pod> | grep -A5 Liveness`

---

## 2. PodDisruptionBudget (PDB) 生产实践

### 2.1 为什么必须有 PDB

没有 PDB 时，`kubectl drain`（节点维护）和集群自动缩容会**同时驱逐所有 Pod**，导致服务完全中断。PDB 强制驱逐器保留最小可用副本数。

### 2.2 PDB 配置模板

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-server-pdb
  namespace: production
spec:
  minAvailable: 2           # 或使用 maxUnavailable: 1
  selector:
    matchLabels:
      app: api-server
```

### 2.3 minAvailable vs maxUnavailable 决策

| 策略 | 适用场景 | 示例 |
|---|---|---|
| `minAvailable: N` | 固定副本数部署 | 3 副本 → minAvailable: 2，允许 1 个被驱逐 |
| `maxUnavailable: M` | 弹性伸缩部署（HPA） | HPA 10-50 副本 → maxUnavailable: 25%（按比例） |

> ⚠️ **常见陷阱**: 单副本 Deployment 配 PDB `minAvailable: 1` 会**永久阻塞 drain**。单副本服务应使用 `maxUnavailable: 1` 并接受短暂中断，或改用多副本。

### 2.4 PDB 审计 🟢

```bash
# 查看所有 PDB 状态
kubectl get pdb -A -o wide

# 检查是否有 PDB 阻塞了驱逐（DisruptionsAllowed=0 需关注）
kubectl get pdb -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,ALLOWED:.status.disruptionsAllowed,EXPECTED:.status.expectedPods,CURRENT:.status.currentHealthy
```

---

## 3. 优雅终止与 preStop 钩子

### 3.1 终止序列与陷阱

Pod 终止时 Kubernetes 执行以下序列，每一步都可能成为可用性缺口：

```
1. Pod 标记 Terminating → 从 Service Endpoints 摘除（异步，kube-proxy/iptables 更新有延迟）
2. preStop 钩子执行（同步阻塞，最长 terminationGracePeriodSeconds）
3. SIGTERM 发送给主进程
4. 等待 terminationGracePeriodSeconds（默认 30s）或进程退出
5. SIGKILL 强制终止
```

> ⚠️ **最常见事故**: Endpoints 摘除与 SIGTERM 并发，SIGTERM 后 Pod 立即停止接收新连接但仍可能在 iptables 规则更新前收到流量 → 502/连接拒绝。**解法**: preStop 钩子 sleep，给 kube-proxy 足够时间更新规则。

### 3.2 生产终止模板

```yaml
spec:
  terminationGracePeriodSeconds: 60   # 给足清理时间
  containers:
    - name: app
      lifecycle:
        preStop:
          exec:
            command: ["/bin/sh", "-c", "sleep 15 && curl -X POST http://localhost:8080/shutdown"]
```

`sleep 15` 的作用：在 SIGTERM 之前等待 15 秒，确保 Endpoints 摘除和 iptables/ipvs 规则同步完成，避免流量打到正在关闭的 Pod。

### 3.3 零停机滚动更新组合拳

```yaml
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxUnavailable: 0      # 不主动减少可用副本
    maxSurge: 1            # 临时多创建 1 个副本用于切换
```

配合 readinessProbe + preStop sleep 可实现零连接中断的滚动更新。`maxUnavailable: 0` 保证旧 Pod 摘除前新 Pod 已就绪。

---

## 4. 就绪门控 (Readiness Gates)

当标准探针不足以表达就绪条件时（如外部负载均衡器健康检查、自定义准入逻辑），使用 readinessGates：

```yaml
readinessGates:
  - conditionType: "feature-flags-loaded"      # 自定义条件
  - conditionType: "mesh-sidecar-injected"
```

Pod 的 `status.conditions` 中对应 `conditionType` 必须为 `True` 才算 Ready。外部控制器（如 service mesh、自定义 operator）负责写入这些条件。

---

## 5. 生产检查清单

| # | 检查项 | 验证命令 | 合格标准 |
|---|---|---|---|
| 1 | 所有面向流量的 Deployment ≥ 2 副本 | `kubectl get deploy -A` | READY 列无 1/1 的核心服务 |
| 2 | 配置了 readinessProbe | `kubectl get deploy -A -o yaml \| grep readinessProbe` | 所有核心服务命中 |
| 3 | livenessProbe 未检测外部依赖 | 审查探针端点实现 | `/live` 仅检查进程自身 |
| 4 | 慢启动应用配 startupProbe | `kubectl get deploy -A -o yaml \| grep startupProbe` | JVM/Python 服务命中 |
| 5 | 配置了 PDB | `kubectl get pdb -A` | 核心服务均有 PDB 且 DisruptionsAllowed ≥ 1 |
| 6 | 配置 preStop sleep | `kubectl get deploy -o yaml \| grep preStop` | 面向流量的服务命中 |
| 7 | terminationGracePeriodSeconds ≥ 应用清理时间 | 审查配置 | ≥ 30s，长连接服务 ≥ 60s |
| 8 | 滚动更新 maxUnavailable=0 | `kubectl get deploy -o yaml \| grep maxUnavailable` | 核心无状态服务命中 |

---

## 6. 排障速查

| 症状 | 可能根因 | 诊断命令 | 修复 |
|---|---|---|---|
| Pod 频繁重启 (CrashLoopBackOff) | livenessProbe 太激进 / 启动慢 | `kubectl describe pod` 看 Last State + `kubectl logs --previous` | 加 startupProbe 或放宽 liveness 阈值 |
| 滚动更新卡住 | readinessProbe 失败 / maxUnavailable=0 + 新 Pod 不就绪 | `kubectl get rs` 看 DESIRED vs CURRENT | 修复新版本就绪问题或临时设 maxUnavailable=1 |
| drain 卡住 | PDB 阻塞 (DisruptionsAllowed=0) | `kubectl get pdb` | 临时调整 PDB 或等待副本数恢复 |
| 更新期间 502/连接拒绝 | 未配 preStop sleep / Endpoints 摘除延迟 | 检查 preStop 配置 + 终止日志 | 加 `preStop: sleep 15` |
| Pod Ready 但无流量 | readinessGates 未满足 / selector 不匹配 | `kubectl get pod -o wide` + `kubectl get endpoints` | 检查 conditionType 和 label selector |

---

## 7. 跨域协作

- **Pod 调度与拓扑分布**: 见 [[topic-production-patterns/scheduling-topology-patterns|调度与拓扑分布模式]]
- **资源 QoS 与 right-sizing**: 见 [[topic-production-patterns/resource-qos-rightsizing|资源 QoS 与 Right-sizing]]
- **状态ful 应用备份恢复**: 见 [[topic-production-patterns/stateful-app-patterns|状态ful 应用生产模式]]
- **应用级排障 Runbook**: 见 [[topic-production-patterns/application-runbooks|应用排障 Runbook]]


<!-- risk-assessed -->
