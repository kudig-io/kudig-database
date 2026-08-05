---
title: 第八课：健康检查 - Probe 详解
description: '# 第八课：健康检查 - Probe 详解'
summary: '2. 掌握 LivenessProbe、ReadinessProbe、StartupProbe 的区别'
category: skills
tags:
- k8s
- learn
- fundamentals
- redis
- mysql
- job
- cronjob
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 第八课：健康检查 - Probe 详解 是什么
- 如何 第八课：健康检查 - Probe 详解
trigger_keywords:
- 第八课：健康检查
- Probe
- 详解
prerequisites:
- kubectl-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第八课：健康检查 - Probe 详解

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解健康检查的概念和作用
2. 掌握 LivenessProbe、ReadinessProbe、StartupProbe 的区别
3. 学会配置健康检查参数
4. 学会排查健康检查失败的问题

---

## 1. 问题引入

### 1.1 问题场景

```
【场景一：应用启动中】

你的应用刚启动，需要加载大量数据到内存。
这时候如果 K8s 就开始发送流量过来，应用可能直接崩溃。

场景二：应用死循环

你的应用进入了死循环，已经完全卡死。
但 K8s 还认为 Pod 是 Running，继续往它发流量。

场景三：依赖不可用

你的应用依赖数据库，但数据库暂时不可用。
这时候如果继续往应用发流量，用户会看到错误页面。

问题：K8s 如何知道应用是否真的"健康"？
```

### 1.2 类比说明

```
【医生类比】

LivenessProbe = 检查心跳
ReadinessProbe = 检查能否上班
StartupProbe = 检查是否已起床

【具体场景】

早上起床（StartupProbe）：
• 你还在睡觉（应用启动中）→ 还没起床，不检查
• 你醒了但还没穿好衣服 → StartupProbe 通过，Start 阶段结束
• 如果超过 2 分钟还没起床（failureThreshold × periodSeconds），宣布死亡

心跳检测（LivenessProbe）：
• 心跳正常 → 继续工作
• 心跳停止 → 宣布死亡，重启 Pod

上班能力检测（ReadinessProbe）：
• 穿好衣服、吃完早餐 → 可以上班了（Ready）
• 还在上厕所 → 暂时不能接收工作（从 Service 中摘除）
• 等准备好了 → 重新加入工作（Ready）

【K8s 场景对应】

StartupProbe 失败 → 不执行其他 Probe
LivenessProbe 失败 → 重启容器
ReadinessProbe 失败 → 从 Service 摘除
```

---

## 2. 三种 Probe 详解

### 2.1 LivenessProbe（存活探针）

```
【作用】

检查应用是否活着。
如果失败，K8s 会重启容器。

【适用场景】

• 应用进入死循环
• 应用完全卡死
• 应用主动崩溃

【YAML 示例】

apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  containers:
  - name: my-container
    image: my-app:1.0
    livenessProbe:
      httpGet:
        path: /health
        port: 8080
      initialDelaySeconds: 30    # 启动后 30 秒开始检查
      periodSeconds: 10          # 每 10 秒检查一次
      timeoutSeconds: 5           # 超时 5 秒算失败
      failureThreshold: 3         # 连续 3 次失败才重启
```

### 2.2 ReadinessProbe（就绪探针）

```
【作用】

检查应用是否准备好接收流量。
如果失败，K8s 会从 Service 中摘除该 Pod，不再发送流量。

【适用场景】

• 应用启动中，还不能接收流量
• 应用正在加载数据，还不能服务
• 应用依赖的数据库不可用

【YAML 示例】

apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  containers:
  - name: my-container
    image: my-app:1.0
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5     # 启动后 5 秒开始检查
      periodSeconds: 5           # 每 5 秒检查一次
      failureThreshold: 3         # 连续 3 次失败才摘除
```

### 2.3 StartupProbe（启动探针）

```
【作用】

检查应用是否启动完成。
在 StartupProbe 通过之前，其他 Probe 不会执行。

【适用场景】

• 应用启动时间较长（超过 10 秒）
• 应用启动需要加载大量数据
• 应用依赖服务较多

【YAML 示例】

apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  containers:
  - name: my-container
    image: my-app:1.0
    startupProbe:
      httpGet:
        path: /started
        port: 8080
      failureThreshold: 30        # 最多等 30 * 5 = 150 秒
      periodSeconds: 5           # 每 5 秒检查一次
```

### 2.4 三种 Probe 对比

```
| 类型 | 失败动作 | 执行时机 | 典型场景 |
|------|---------|---------|---------|
| StartupProbe | 阻止其他 Probe | 应用启动阶段 | 启动慢的应用 |
| LivenessProbe | 重启容器 | 应用运行中 | 应用死循环/卡死 |
| ReadinessProbe | 摘除流量 | 应用运行中 | 应用未就绪/依赖不可用 |

【重要规则】

1. StartupProbe 通过之前，LivenessProbe 和 ReadinessProbe 不会执行
2. StartupProbe 只能设置 failureThreshold，不能设置 successThreshold
3. LivenessProbe 和 ReadinessProbe 可以同时存在
```

---

## 3. 健康检查方式

### 3.1 HTTP 检测

```
【原理】

发送 HTTP GET 请求，返回 2xx 表示成功，非 2xx 表示失败。

【适用场景】

Web 应用、微服务、有 HTTP 端点的应用。

【示例】

livenessProbe:
  httpGet:
    path: /health
    port: 8080
    scheme: HTTP
    httpHeaders:
    - name: X-Custom-Header
      value: Awesome

【健康检查端点示例】

// Go
http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("OK"))
})

// Node.js
app.get('/health', (req, res) => {
    res.status(200).send('OK')
})
```

### 3.2 TCP 检测

```
【原理】

尝试建立 TCP 连接，能连接上表示成功。

【适用场景】

• MySQL、Redis 等数据库连接
• 非 HTTP 的应用
• 无法修改应用代码的情况

【示例】

readinessProbe:
  tcpSocket:
    port: 3306
    host: localhost

【场景】

对于 MySQL，可以检测 3306 端口是否可连接。
如果连接不上，说明 MySQL 还没准备好。
```

### 3.3 命令检测

```
【原理】

在容器内执行命令，退出码为 0 表示成功，非 0 表示失败。

【适用场景】

• 应用没有 HTTP 端点
• 需要自定义健康检查逻辑
• 检测文件是否存在等

【示例】

livenessProbe:
  exec:
    command:
    - cat
    - /tmp/healthy
  initialDelaySeconds: 30
  periodSeconds: 10

【应用端创建标记文件】

// 启动时创建
os.MkdirAll("/tmp", 0755)
os.WriteFile("/tmp/healthy", []byte("ok"), 0644)

// 应用内部定期检查
if !app.isHealthy() {
    os.Remove("/tmp/healthy")
}
```

---

## 4. 健康检查参数

### 4.1 参数说明

```
【参数速查】

initialDelaySeconds:   启动后多久开始检查（默认 0）
periodSeconds:        检查频率（默认 10 秒）
timeoutSeconds:       超时时间（默认 1 秒）
failureThreshold:     连续失败多少次认为失败（默认 3）
successThreshold:     连续成功多少次认为成功（默认 1）

【计算示例】

initialDelaySeconds: 30     → 应用启动后 30 秒开始检查
periodSeconds: 15           → 每 15 秒检查一次
timeoutSeconds: 3           → 3 秒内响应算成功
failureThreshold: 3        → 连续 3 次失败才触发动作

总时间：failureThreshold × periodSeconds = 3 × 15 = 45 秒
```

### 4.2 建议配置

```
【Web 应用建议配置】

livenessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 30    # 等待应用启动
  periodSeconds: 15
  timeoutSeconds: 3
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /ready
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
  failureThreshold: 3

startupProbe:
  httpGet:
    path: /started
    port: 8080
  failureThreshold: 30        # 允许最多 150 秒启动时间
  periodSeconds: 5

【Java Spring Boot 应用】

如果 Spring Boot 应用启动需要 30 秒：
initialDelaySeconds: 40       # 启动时间 + 10 秒 buffer
periodSeconds: 10
failureThreshold: 3
```

### 4.3 不同应用类型配置

```
【Node.js 应用（启动快）】

startupProbe:
  httpGet:
    path: /health
    port: 3000
  failureThreshold: 10
  periodSeconds: 5

livenessProbe:
  httpGet:
    path: /health
    port: 3000
  initialDelaySeconds: 10
  periodSeconds: 10
  failureThreshold: 3

【Java 应用（启动慢）】

startupProbe:
  httpGet:
    path: /actuator/health
    port: 8080
  failureThreshold: 60       # 60 × 5 = 300 秒启动时间
  periodSeconds: 5

livenessProbe:
  httpGet:
    path: /actuator/health
    port: 8080
  initialDelaySeconds: 60    # 等待 Spring 启动
  periodSeconds: 15
  failureThreshold: 3

【数据库应用】

readinessProbe:
  tcpSocket:
    port: 5432
  initialDelaySeconds: 10
  periodSeconds: 10
  failureThreshold: 3

livenessProbe:
  exec:
    command: ["pg_isready", "-U", "postgres"]
  initialDelaySeconds: 30
  periodSeconds: 10
  failureThreshold: 3
```

---

## 5. 在 Deployment 中配置

### 5.1 Deployment 完整示例

```
【YAML 示例】

apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: my-app
        image: my-app:1.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
          timeoutSeconds: 2
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 2
          failureThreshold: 3
        startupProbe:
          httpGet:
            path: /started
            port: 8080
          failureThreshold: 30
          periodSeconds: 5
```

---

## 6. 常见问题

### 6.1 LivenessProbe 失败导致 Pod 重启

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【问题】

Pod 一直处于 CrashLoopBackOff 或 Error 状态。

【原因】

LivenessProbe 连续失败达到阈值，K8s 不断重启容器。

【排查步骤】

1. 检查应用是否有健康检查端点
   kubectl describe pod <pod-name> | grep -A20 "Liveness"

2. 查看应用日志
   kubectl logs <pod-name> -n <namespace> --previous | tail -100

   如果日志显示健康检查端点返回非 200，很可能是：
   • 健康检查路径不对
   • 应用启动太慢，initialDelaySeconds 不够
   • 应用本身有问题

3. 测试健康检查端点
   kubectl exec -it <pod-name> -- curl http://localhost:8080/health

4. 调整 initialDelaySeconds
   如果应用启动时间较长，增加 initialDelaySeconds。
   规则：initialDelaySeconds > 应用实际启动时间
```
### 6.2 ReadinessProbe 失败导致无流量

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【问题】

Pod 是 Running，但 Service 没有流量过来。

【原因】

ReadinessProbe 失败，Pod 被从 Service 中摘除。

【排查步骤】

1. 检查 Pod 的 Conditions
   kubectl describe pod <pod-name> | grep -A10 "Conditions"

   看 Ready 是否为 False。

2. 测试就绪端点
   kubectl exec -it <pod-name> -- curl http://localhost:8080/ready

   返回 200 吗？

3. 检查依赖服务
   如果应用依赖数据库或 API，这些服务是否可用？
   可以在就绪检查中检查依赖服务的连通性。

4. 查看 Events
   kubectl describe pod <pod-name> | grep -A5 "Events"

5. 调整配置
   如果应用启动确实需要时间，增加 initialDelaySeconds。
   如果依赖服务恢复较慢，增加 failureThreshold。
```
### 6.3 StartupProbe 超时

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

Pod 一直处于 ContainerCreating 或 Initializing 状态。

【原因】

StartupProbe 失败次数达到 failureThreshold。

【排查步骤】

1. 检查 startupProbe 配置
   kubectl describe pod <pod-name> | grep -A10 "Startup"

2. 查看应用日志
   kubectl logs <pod-name> --tail=100

3. 增加 failureThreshold
   如果应用启动确实需要很长时间，增加 failureThreshold。

   计算：failureThreshold × periodSeconds = 最大启动时间

   例如：failureThreshold: 60, periodSeconds: 5
   → 最多等 60 × 5 = 300 秒 = 5 分钟

4. 检查应用启动日志
   kubectl describe pod <pod-name> | grep -A20 "Events"
```
### 6.4 健康检查端点返回错误

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【问题】

健康检查端点返回 500 或其他错误。

【排查步骤】

1. 测试端点
   kubectl exec -it <pod-name> -- curl -v http://localhost:8080/health

2. 查看应用日志
   kubectl logs <pod-name> --tail=100 | grep -i error

3. 检查应用代码
   确保健康检查端点正确实现：
   • 返回 200 表示健康
   • 返回非 200 表示不健康

4. 检查依赖
   健康检查端点可能依赖其他服务（数据库、缓存等）。
   确保这些依赖正常。

【健康检查端点最佳实践】

// 应该快速返回，不做复杂检查
func healthHandler(w http.ResponseWriter, r *http.Request) {
    // 只检查进程是否存活，不检查依赖
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("OK"))
}

// 就绪检查可以更复杂
func readyHandler(w http.ResponseWriter, r *http.Request) {
    if db.Ping() != nil {
        w.WriteHeader(http.StatusServiceUnavailable)
        return
    }
    w.WriteHeader(http.StatusOK)
}
```
---

## 7. 数字人 Q&A 场景

### 7.1 用户问：Pod 一直在重启怎么办？

```
# 🟢 低风险：只读/信息收集，通常无副作用
【回复】

"Pod 一直重启，很有可能是 LivenessProbe 失败导致的。

让我帮你排查一下：

第一步：查看 Pod 状态
kubectl get pods -n <namespace>

如果看到 CrashLoopBackOff，说明容器在不断崩溃重启。

第二步：查看 LivenessProbe 配置
kubectl describe pod <pod-name> | grep -A15 "Liveness"

第三步：查看日志
kubectl logs <pod-name> -n <namespace> --previous

如果日志显示健康检查端点返回非 200，很可能是：
1. 健康检查端点路径不对
2. 应用启动太慢，initialDelaySeconds 不够
3. 应用本身有问题

解决方案：
• 如果是路径问题，修改应用或配置
• 如果是启动慢，增加 initialDelaySeconds
• 如果是应用问题，修复应用逻辑

有其他问题吗？"
```
### 7.2 用户问：应用明明 Running 为什么没流量？

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【回复】

"好问题！Pod 是 Running，但没流量，通常是 ReadinessProbe 的问题。

K8s 的机制是这样的：
• Running = 容器进程在运行
• Ready = 应用准备好接收流量

ReadinessProbe 失败时，K8s 会把 Pod 从 Service 里摘除。

排查步骤：

第一步：检查 Pod 的 Ready 状态
kubectl describe pod <pod-name> | grep -A5 "Conditions"

如果 Ready 是 False，说明 ReadinessProbe 有问题。

第二步：测试就绪端点
kubectl exec -it <pod-name> -- curl http://localhost:8080/ready

返回 200 吗？

第三步：检查依赖
如果应用依赖数据库或 API，这些服务是否可用？

第四步：调整配置
如果应用启动确实需要时间，增加 initialDelaySeconds。

有其他问题吗？"
```
---

## 8. 快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【命令速查】

查看 Pod 健康检查配置：
kubectl describe pod <pod-name> | grep -E "Liveness|Readiness|Startup"

测试健康检查端点：
kubectl exec -it <pod-name> -- curl http://localhost:8080/health

查看 Pod 详细状态：
kubectl get pod <pod-name> -o yaml | grep -A10 "probe"

【Probe 故障排查三板斧】

1. 看配置：kubectl describe pod | grep Probe
2. 看日志：kubectl logs <pod> --previous
3. 测试端点：kubectl exec -it <pod> -- curl localhost:8080/health

【参数设置建议】

• initialDelaySeconds：设为应用实际启动时间的 1.5-2 倍
• periodSeconds：不要太短（增加负载），也不要太长（发现问题慢）
• failureThreshold：根据业务容忍度设置，通常 3-5 次
• timeoutSeconds：设为正常响应时间的 2-3 倍
```
---

## 9. 总结

```
【三种 Probe 对比】

| 类型 | 失败动作 | 使用场景 |
|------|---------|---------|
| StartupProbe | 阻止其他 Probe | 启动慢的应用 |
| LivenessProbe | 重启容器 | 应用死循环/卡死 |
| ReadinessProbe | 摘除流量 | 应用未就绪 |

【配置建议】

• initialDelaySeconds 要大于应用启动时间
• periodSeconds 不宜过短（会增加负载）
• failureThreshold 要合理（避免误判）
• 超时时间（timeoutSeconds）要设置合理

【下节课预告】

下节课我们会学习 Job 和 CronJob：
• 一次性任务和定时任务
• 如何配置并行执行
• 常见问题排查

有问题吗？"
```

---

**关联文档**:
- [../10-workloads/10-job-cronjob.md](32-发布/package/2026-07-02_18-29/corpus/peripheral/skills/training-lecturer/10-workloads/01-job-cronjob.md) — Job 和 CronJob
- [../../domain-10-troubleshooting-diagnostics/topic-skills/01-pod-crash-loop.md](../../domain-10-troubleshooting-diagnostics/技能体系/01-pod-crash-loop.md) — Pod 崩溃问题 [[SKILL|Skill]]
- [../../domain-10-troubleshooting-diagnostics/](../../domain-10-troubleshooting-diagnostics/) — 故障排查文档

## Related

- [[skills/learn-inner-training.md|learn-inner-training]] — [[Kubernetes|Kubernetes]]es 培训：Inner Training|Kubernetes 培训：Inner Training]]
- [[skills/learn-lecturer-persona.md|learn-lecturer-persona]] — K8S 讲师角色设定与场景规范
- [[skills/learn-03-oncall-handoff.md|learn-03-oncall-handoff]] — Day 3: 值班交接 SOP
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[deployment]] — Deployment


<!-- risk-assessed -->
