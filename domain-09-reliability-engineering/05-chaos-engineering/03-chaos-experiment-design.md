---
title: 混沌实验设计与执行
description: '# 混沌实验设计与执行'
category: domain
tags:
- chaos-engineering
- experiment-design
- reliability
- redis
- mysql
- postgresql
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌实验设计与执行 是什么
- 如何 混沌实验设计与执行
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 混沌实验设计与执行
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- redis-basics
- mysql-basics
created: "2026-05-23"
---

# 混沌实验设计与执行

## 实验设计流程

```
1. 定义稳态假设
   → "正常情况下，服务延迟 P99 < 200ms，错误率 < 0.1%"

2. 识别变量（要注入的问题）
   → 网络延迟增加 100ms

3. 定义成功标准
   → P99 < 400ms，错误率 < 1%

4. 设定中止条件
   → 错误率 > 5% 或 P99 > 2s

5. 执行实验
   → 使用 Chaos Mesh 注入问题

6. 监控和记录
   → 实时观察指标变化

7. 分析结果
   → 是否验证/否定假设？

8. 改进和迭代
   → 修复发现的问题
```

## 8 类经典实验模板

### 1. 依赖超时实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: dependency-timeout
spec:
  action: delay
  mode: one
  selector:
    labelSelectors:
      app: order-service
  delay:
    latency: "500ms"
    correlation: "100"
    jitter: "0ms"
  target:
    selector:
      labelSelectors:
        app: payment-service
    mode: all
  duration: "5m"
```

### 2. Pod 级联问题实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-cascade-failure
spec:
  action: pod-kill
  mode: fixed-percent
  value: "30"
  selector:
    labelSelectors:
      app: api-gateway
  duration: "30s"
```

### 3. CPU 饱和实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: StressChaos
metadata:
  name: cpu-saturation
spec:
  mode: all
  selector:
    labelSelectors:
      app: data-processor
  stressors:
    cpu:
      workers: 4
      load: 80
  duration: "5m"
```

### 4. 数据库连接池耗尽实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: db-connection-exhaust
spec:
  action: loss
  mode: all
  selector:
    labelSelectors:
      app: order-service
  loss:
    loss: "100"
  target:
    selector:
      labelSelectors:
        app: postgresql
    mode: all
  duration: "2m"
```

### 5. 网络分区实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-partition
spec:
  action: partition
  mode: all
  selector:
    labelSelectors:
      app: service-a
  direction: to
  target:
    selector:
      labelSelectors:
        app: service-b
    mode: all
  duration: "5m"
```

### 6. 证书过期实验

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 模拟证书即将过期
kubectl create secret tls expired-cert \
  --cert=expired.crt \
  --key=expired.key \
  -n default --dry-run=client -o yaml | kubectl apply -f -
```

### 7. 配置错误实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: HTTPChaos
metadata:
  name: config-error-simulate
spec:
  mode: all
  selector:
    labelSelectors:
      app: config-service
  target: Request
  port: 8080
  path: /api/v1/config
  method: GET
  abort: true
  duration: "3m"
```

### 8. 级联依赖问题实验

```yaml
# 同时注入多个问题
apiVersion: chaos-mesh.org/v1alpha1
kind: Workflow
metadata:
  name: cascade-failure
spec:
  templates:
    - name: kill-cache
      templateType: Schedule
      deadline: 2m
      schedule:
        schedule: "*/1m"
        concurrencyPolicy: Allow
        type: PodChaos
        podChaos:
          action: pod-kill
          mode: one
          selector:
            labelSelectors:
              app: redis
    - name: delay-db
      templateType: Schedule
      deadline: 5m
      schedule:
        schedule: "*/1m"
        concurrencyPolicy: Allow
        type: NetworkChaos
        networkChaos:
          action: delay
          mode: all
          selector:
            labelSelectors:
              app: mysql
          delay:
            latency: "2s"
```

## 实验安全清单

```
执行前检查:
□ 实验范围已限制（命名空间/标签选择器）
□ 中止条件已设定（自动或手动）
□ 相关团队已通知
□ 监控和告警已就绪
□ 回滚方案已准备
□ 实验时间避开高峰期
```

## 相关

- [[domain-09-reliability-engineering/05-chaos-engineering/01-chaos-engineering-overview.md|01 chaos engineering overview]]
- deployment]]
