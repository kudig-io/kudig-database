---
title: 应用排障 Runbook
description: 生产级应用故障速查：CrashLoopBackOff、OOMKilled、ImagePullBackOff、Ingress 5xx 与高频重启排障流程
summary: 生产级应用故障速查：CrashLoopBackOff、OOMKilled、ImagePullBackOff、Ingress 5xx 与高频重启排障流程，含诊断决策树与修复命令。
category: application-patterns
tags:
- troubleshooting
- crashloopbackoff
- oom
- imagepull
- ingress
- runbook
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 应用开发者
estimated_read_time: 20min
intent_queries:
- Pod CrashLoopBackOff 怎么排查
- OOMKilled 怎么修复
- ImagePullBackOff 原因
trigger_keywords:
- CrashLoopBackOff
- OOMKilled
- ImagePullBackOff
- 排障
- 5xx
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
> 本文档包含诊断命令。排查命令标注 🟢（只读）；涉及修复的命令标注 🟡/🔴 并说明风险。

# 应用排障 Runbook

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 排障 Runbook

本文覆盖生产环境最高频的 5 类应用故障：CrashLoopBackOff、OOMKilled、ImagePullBackOff、Ingress 5xx、高频重启。每个场景提供诊断决策树、验证命令和修复步骤，目标是 5 分钟内定位根因。

---

## 1. CrashLoopBackOff

### 1.1 含义与根因分类

CrashLoopBackOff 表示 Pod 反复崩溃重启，kubelet 按指数退避延迟下次重启。根因分为四类：

| 类别 | 典型根因 | 快速验证 |
|---|---|---|
| **启动崩溃** | 配置错误、依赖缺失、端口冲突 | `kubectl logs <pod> --previous` |
| **探针失败** | livenessProbe 太激进、端口/路径错 | `kubectl describe pod` 看 Last State |
| **资源不足** | OOMKilled、CPU throttling 严重 | `kubectl describe pod` 看 Reason |
| **依赖不可用** | DB 连不上、配置中心超时 | `kubectl logs` 看错误堆栈 |

### 1.2 诊断决策树

```
Pod CrashLoopBackOff
  │
  ├─ kubectl logs <pod> --previous    ← 看崩溃前最后的日志
  │   ├─ 有明确错误 (NullPointer/连接超时) → 应用/配置问题
  │   └─ 无日志 → 进程未输出就崩溃
  │
  ├─ kubectl describe pod <pod>       ← 看 Last State + Events
  │   ├─ Reason: OOMKilled → 见 §2 OOMKilled
  │   ├─ Reason: Error / Completed → 进程主动退出，检查 exit code
  │   └─ Liveness probe failed → 探针问题
  │
  └─ kubectl get events --field-selector involvedObject.name=<pod>
      └─ 看 FailedScheduling / FailedMount 等事件
```

### 1.3 修复步骤

```bash
# 🟢 Step 1: 看崩溃前日志（关键！）
kubectl logs <pod> --previous --tail=50

# 🟢 Step 2: 查看退出状态
kubectl describe pod <pod> | grep -A5 "Last State"

# 🟢 Step 3: 检查 Events
kubectl get events --field-selector involvedObject.name=<pod> --sort-by=.lastTimestamp

# 🟡 Step 4: 临时关掉 livenessProbe 观察是否还崩（确认是否探针问题）
kubectl patch deploy <app> --type='json' \
  -p='[{"op":"remove","path":"/spec/template/spec/containers/0/livenessProbe"}]'
# ⚠️ 仅用于诊断，确认后恢复

# 🟡 Step 5: 回滚到上一版本（如果崩溃从新版本开始）
kubectl rollout undo deploy/<app>
```

---

## 2. OOMKilled

### 2.1 两类 OOM

| 类型 | 含义 | 容器 Reason | 修复方向 |
|---|---|---|---|
| **容器 OOM** | 进程超过容器 memory limit | `OOMKilled` | 提升 limits 或修复内存泄漏 |
| **节点 OOM** | 节点内存耗尽，内核杀进程 | `Evicted` | 扩容节点或收紧 requests |

### 2.2 诊断与修复

```bash
# 🟢 确认是否 OOMKilled
kubectl describe pod <pod> | grep -A3 "Last State"
# 期望看到: Reason: OOMKilled, Exit Code: 137

# 🟢 查看容器内存 limit
kubectl get pod <pod> -o jsonpath='{.spec.containers[0].resources.limits.memory}'

# 🟢 对比实际使用量
kubectl top pod <pod> --containers

# 🟡 修复: 提升内存 limit（需确认不是泄漏）
kubectl set resources deploy <app> --limits=memory=2Gi --requests=memory=1Gi

# 🟢 判断是泄漏还是配置不足
# 监控内存趋势: 如果持续单调上升 → 内存泄漏（需修代码）
#                如果快速稳定在某值 → 配置不足（提 limit 即可）
```

> ⚠️ **生产陷阱**: Exit Code 137 也可能是 livenessProbe 脚本被 OOM 杀死，而非主进程。检查 `dmesg` 或节点日志确认被杀的进程 PID。

---

## 3. ImagePullBackOff / ErrImagePull

### 3.1 根因速查表

| 根因 | 验证方法 | 修复 |
|---|---|---|
| 镜像名/Tag 拼写错误 | 检查 image 字段 | 修正镜像名 |
| 私有仓库认证缺失 | `kubectl describe pod` 看 Events | 配置 imagePullSecrets |
| 镜像仓库不可达 | 节点 `curl/ping` 仓库 | 检查网络/DNS/防火墙 |
| 镜像过大拉取超时 | Events 有 `Failed to pull image` timeout | 预拉取或增大拉取超时 |
| 仓库 Rate Limit | Events 有 `toomanyrequests` | 使用镜像加速/本地缓存 |
| 节点磁盘满 | `kubectl describe node` 看 DiskPressure | 清理镜像/扩容节点盘 |

### 3.2 诊断命令

```bash
# 🟢 查看 Events 中的具体错误
kubectl describe pod <pod> | grep -A10 "Events:"

# 🟢 常见错误信息对照:
#   "rpc error: code = NotFound" → 镜像不存在
#   "Unauthorized/Failed to authorize" → 认证问题
#   "context deadline exceeded" → 网络超时
#   "no space left on device" → 节点磁盘满

# 🟢 检查 imagePullSecrets 是否配置
kubectl get pod <pod> -o jsonpath='{.spec.imagePullSecrets}'

# 🟡 在节点上手动测试拉取（验证网络/认证）
crictl pull <image>    # 需 SSH 到节点
```

### 3.3 私有仓库配置

```yaml
spec:
  imagePullSecrets:
    - name: registry-credentials   # kubectl create secret docker-registry
```

---

## 4. Ingress / Service 5xx 排障

### 4.1 5xx 分类与定位

| 状态码 | 含义 | 常见根因 | 定位 |
|---|---|---|---|
| **500** | Internal Server Error | 应用代码异常 | 看应用日志 |
| **502** | Bad Gateway | 后端 Pod 无响应/连接拒绝 | 检查 Endpoints + Pod 健康 |
| **503** | Service Unavailable | 无可用后端 Pod | Pod 全挂/Pending 或 Endpoints 为空 |
| **504** | Gateway Timeout | 后端响应超时 | 检查后端延迟/Ingress 超时配置 |

### 4.2 502/503 诊断流程（最常见）

```bash
# 🟢 Step 1: 检查 Endpoints 是否有后端
kubectl get endpoints <service>
# 期望: ENDPOINTS 列有 IP:Port。如果为 <none> → 无就绪 Pod

# 🟢 Step 2: 检查 Pod 是否 Ready
kubectl get pods -l app=<app> -o wide
# READY 列应为 x/x。0/1 → readinessProbe 失败

# 🟢 Step 3: 检查 Service selector 是否匹配 Pod label
kubectl get svc <service> -o yaml | grep -A5 selector
kubectl get pods --show-labels | grep <app>
# selector label 必须与 Pod label 完全匹配

# 🟢 Step 4: 检查 targetPort 是否正确
kubectl get svc <service> -o yaml | grep targetPort
# 应与容器实际监听端口一致

# 🟢 Step 5: 如果 Endpoints 正常，直接 curl Pod IP 验证
kubectl run debug --rm -it --image=curlimages/curl -- curl -v http://<pod-ip>:<port>
```

### 4.3 504 超时调优

```yaml
# Nginx Ingress 超时注解
metadata:
  annotations:
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
```

---

## 5. 高频重启排查

### 5.1 诊断命令

```bash
# 🟢 查看 Pod 重启次数
kubectl get pods | awk '{print $1, $4}'   #	RESTARTS 列

# 🟢 找出重启次数异常高的 Pod
kubectl get pods -A --sort-by=.status.containerStatuses[0].restartCount -o wide | tail

# 🟢 查看重启原因统计
kubectl get pod <pod> -o json | jq '.status.containerStatuses[] | {name, restartCount, lastState}'

# 🟢 节点层面: 检查是否有节点问题导致重启
kubectl get events -A --field-selector reason=BackOff,reason=Unhealthy --sort-by=.lastTimestamp | tail -20
```

### 5.2 重启根因频率（生产经验）

| 排名 | 根因 | 占比 | 快速识别 |
|---|---|---|---|
| 1 | OOMKilled | ~35% | Reason: OOMKilled |
| 2 | livenessProbe 太激进 | ~25% | 关掉 liveness 后稳定 |
| 3 | 应用启动失败（配置错） | ~20% | previous 日志有异常 |
| 4 | 节点资源压力/驱逐 | ~10% | Events 有 Evicted |
| 5 | 依赖超时级联 | ~10% | 日志有连接超时 |

---

## 6. 通用排障工具箱

```bash
# 🟢 临时调试容器（Pod 内缺工具时）
kubectl debug <pod> -it --image=busybox --target=<container>

# 🟢 临时 Pod 做网络连通性测试
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- /bin/sh

# 🟢 导出 Pod 完整状态用于离线分析
kubectl get pod <pod> -o yaml > pod-debug.yaml

# 🟢 查看节点上的容器日志（直接通过 runtime）
crictl logs <container-id>   # 需 SSH 到节点
```

---

## 7. 跨域协作

- **Pod 可用性与探针设计**: 见 [[04-应用模式/03-生产模式/pod-availability-lifecycle|Pod 可用性生产模式]]
- **资源 QoS 与 OOM**: 见 [[04-应用模式/03-生产模式/resource-qos-rightsizing|资源 QoS 与 Right-sizing]]
- **结构化排障方法论 (FTA)**: 见 `19-故障诊断/06-FTA故障树/`
- **网络连通性排障**: 见 `网络/99-production-readiness-operations-guide.md`
- **节点异常排障**: 见 `19-故障诊断/04-高级排障/structural-02-node-components/`


<!-- risk-assessed -->
