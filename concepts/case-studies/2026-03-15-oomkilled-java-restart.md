---
title: "[2026-03-15] [P1] OOMKilled 导致 Java 应用反复重启"
category: case-study
tags: [production, incident, workloads, java, memory, oom]
date: "2026-03-15"
severity: P1
mttr: "35min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-03-15] Java 应用 OOMKilled 循环重启，促销期间订单服务崩溃

## 工单信息
- **工单编号**: INC-2026-0315-006
- **发现时间**: 2026-03-15 08:00 UTC
- **恢复时间**: 2026-03-15 08:35 UTC
- **影响范围**: `prod-order` namespace，`order-api` Deployment（6 个 Pod 全部 OOMKilled）
- **业务影响**: 订单服务不可用 35 分钟，促销期间损失预估订单 12,000 笔

## 问题现象
08:00，促销活动开始，用户流量激增。`order-api` Pod 开始反复重启：
```bash
kubectl get pods -n prod-order -l app=order-api
# NAME                          READY   STATUS      RESTARTS   AGE
# order-api-7d9f4b8c5a-abc12   0/1     OOMKilled   5          12m
# order-api-7d9f4b8c5a-def34   0/1     OOMKilled   5          12m
# ...
```

用户反馈：下单按钮点击后页面刷新，无响应或报错 "系统繁忙"。

## 诊断过程

**08:05** — 检查 Pod 事件：
```bash
kubectl describe pod order-api-7d9f4b8c5a-abc12 -n prod-order
# ...
# Last State: Terminated
#   Reason:    OOMKilled
#   Exit Code: 137
# ...
# Events:
#   Warning  OOMKilling  5m    kubelet  Memory cgroup out of memory: Killed process 1234 (java) ...
```

**08:07** — 检查资源限制：
```bash
kubectl get deployment order-api -n prod-order -o jsonpath='{.spec.template.spec.containers[0].resources}' | jq .
# {
#   "limits": {
#     "cpu": "2",
#     "memory": "2Gi"
#   },
#   "requests": {
#     "cpu": "500m",
#     "memory": "1Gi"
#   }
# }
```

**08:10** — 检查 JVM 参数：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-order order-api-7d9f4b8c5a-abc12 -- ps -ef | grep java
# java -Xmx2048m -Xms2048m -XX:+UseG1GC -jar order-api.jar
```

**08:12** — 问题定位：
- Pod memory limit: 2Gi
- JVM `-Xmx2048m` (2Gi)
- JVM 堆内存已达容器上限，但 Java 应用还有 Metaspace、堆外内存、线程栈等额外内存开销
- 容器实际 RSS 超过 2Gi 时触发 OOMKiller

**08:14** — 查看历史内存趋势：
```bash
# Prometheus 查询
# container_memory_working_set_bytes{container="order-api", namespace="prod-order"}
# 趋势显示：促销流量涌入后，内存从 1.2Gi 迅速爬升至 2.1Gi，触发 OOMKilled
```

## 根因
Java 应用的 `-Xmx2048m` 与容器的 `memory limit: 2Gi` 相等，未预留 headroom 给：
- JVM 自身开销（GC 线程、JIT 编译器）
- 堆外内存（DirectBuffer、Native 内存）
- 线程栈
- 容器内其他进程（JVM 监控 agent、日志 sidecar）

促销期间流量激增，订单对象在堆中堆积，同时大量并发请求消耗堆外内存，总 RSS 超过 2Gi 即被 OOMKilled。

## 修复动作

**08:18** — 紧急提升 Pod memory limit 并调整 JVM：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-api
  namespace: prod-order
spec:
  template:
    spec:
      containers:
      - name: order-api
        resources:
          limits:
            cpu: "2"
            memory: "3Gi"
          requests:
            cpu: "500m"
            memory: "2Gi"
        env:
        - name: JAVA_OPTS
          value: "-Xmx1536m -Xms1536m -XX:+UseG1GC -XX:MaxRAMPercentage=75.0"
EOF
```

**08:22** — 验证 Pod 状态：
```bash
kubectl get pods -n prod-order -l app=order-api
# NAME                          READY   STATUS    RESTARTS   AGE
# order-api-7d9f4b8c5a-ghi56   1/1     Running   0          3m
# ...
```

**08:25** — 监控内存使用：
```bash
kubectl top pods -n prod-order -l app=order-api
# NAME                          CPU(cores)   MEMORY(bytes)
# order-api-7d9f4b8c5a-ghi56   850m         1.8Gi
# ...
```

**08:30** — HPA 扩容应对促销流量：
```bash
kubectl scale deployment order-api -n prod-order --replicas=12
# 同时调整 HPA maxReplicas: 20
```

## 验证
- 08:32 — `kubectl get pods` 显示全部 Running，无 OOMKilled
- 08:33 — 下单成功率恢复至 99.5%
- 08:35 — 订单接口 P99 延迟从 5.2s 恢复至 180ms

## 复盘
- **直接原因**: JVM `-Xmx=2Gi` 等于容器 `memory limit=2Gi` → 无 headroom → 促销流量下 RSS 超限 → OOMKilled
- **根本原因**: 开发团队在 02-28 的优化中将 `-Xmx` 从 1536m 提升至 2048m，但未同步提升容器 limit
- **改进措施**:
  1. **Java 容器内存公式**: `memory limit = JVM heap + 非堆内存 + 容器 headroom`。建议 `-Xmx` 不超过容器 limit 的 70%-75%
  2. 在 CI/CD 中添加校验：`if [ $XMX_MB -ge $LIMIT_MB ]; then exit 1; fi`
  3. 为 Java 应用配置 `XX:+AlwaysPreTouch` 和 `XX:+UseContainerSupport`（JDK 8u191+）
  4. 添加容器内存使用率告警：`container_memory_working_set_bytes / memory_limit > 0.85`
- **相关 Skill**: [[ts-workloads]]
- **相关 FTA**: [[pod-fta]]
