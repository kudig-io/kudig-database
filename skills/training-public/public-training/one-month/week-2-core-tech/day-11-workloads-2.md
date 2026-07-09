---
title: 'Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA'
description: '# Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA'
summary: 'kubectl get pod qos-guaranteed -o jsonpath='{.status.qosClass}''
category: learning
tags:
- k8s
- training
- hands-on
- hpa
- vpa
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA 是什么'
- '如何 Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA'
trigger_keywords:
- Day
- '11:'
- 工作负载
- Pod
- 生命周期
- 资源管理
- HPA
- learn
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY11
title: Day 11 - 工作负载 - Pod 生命周期 + 资源管理 + HPA
topic: [[entities/kubernetes.md|kubernetes]]
type: hands-on-guide
tags: [pod, lifecycle, probe, resources, qos, hpa, vpa, autoscaling, hands-on, week-2]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Pod 生命周期怎么理解"
  - "Liveness/Readiness 探针怎么配"
  - "resources requests/limits 怎么设置"
  - "QoS 等级是什么"
  - "HPA 怎么配置"
trigger_keywords:
  - Pod Lifecycle
  - Init Container
  - PostStart
  - PreStop
  - LivenessProbe
  - ReadinessProbe
  - StartupProbe
  - resources
  - requests
  - limits
  - QoS
  - Guaranteed
  - Burstable
  - BestEffort
  - HPA
  - HorizontalPodAutoscaler
  - autoscaling
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - 工作负载
  - 故障诊断
related_topics:
  - workloads
  - pod
  - hpa
  - resources
related:
  - 生产运维/topic-learn/public-training/one-month/week-2-core-tech/day-10-workloads-1.md
  - 工作负载/11-pod-lifecycle-events.md
---
```

> **学习时间**: 4-5 小时 | **主题**: Pod 高级配置与自动扩缩容

---

## 今日目标

- [ ] 掌握 Pod 生命周期和 Hook 机制
- [ ] 理解资源管理 (requests/limits) 和 QoS
- [ ] 配置 HPA 实现自动扩缩容

---

## 理论学习 (2h)

### 必读文档

1. **Pod 生命周期事件**
   - 文件: `../../工作负载/11-pod-lifecycle-events.md`
   - 重点: Init Container、PostStart/PreStop、探针

2. **资源管理**
   - 文件: `../../工作负载/23-resource-management.md`
   - 重点: requests/limits、QoS 等级

3. **HPA/VPA 自动扩缩容**
   - 文件: `../../工作负载/21-hpa-vpa-autoscaling.md`
   - 重点: 基于 CPU/Memory 的自动扩缩

---

## 实践任务 (2.5h)

### 任务 1: 探针配置实践 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建带三种探针的 Pod
cat > probe-demo.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: probe-demo
spec:
  containers:
  - name: nginx
    image: nginx:alpine
    ports:
    - containerPort: 80
    # 存活探针: 检测容器是否存活
    livenessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 5
      periodSeconds: 10
      failureThreshold: 3
    # 就绪探针: 检测容器是否可以接收流量
    readinessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 3
      periodSeconds: 5
    # 启动探针: 检测容器是否启动完成
    startupProbe:
      httpGet:
        path: /
        port: 80
      failureThreshold: 30
      periodSeconds: 2
EOF

kubectl apply -f probe-demo.yaml

# 观察探针状态
kubectl describe pod probe-demo | grep -A10 "Liveness|Readiness|Startup"

# 模拟探针失败
kubectl exec probe-demo -- rm /usr/share/nginx/html/index.html

# 观察 Pod 重启
kubectl get pod probe-demo -w

# 清理
kubectl delete pod probe-demo
```
### 任务 2: 生命周期 Hook 实践 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建带 Hook 的 Pod
cat > lifecycle-demo.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: lifecycle-demo
spec:
  containers:
  - name: app
    image: nginx:alpine
    lifecycle:
      postStart:
        exec:
          command: ["/bin/sh", "-c", "echo 'PostStart hook executed' >> /var/log/lifecycle.log"]
      preStop:
        exec:
          command: ["/bin/sh", "-c", "echo 'PreStop hook executed' >> /var/log/lifecycle.log && sleep 10"]
EOF

kubectl apply -f lifecycle-demo.yaml

# 验证 PostStart
kubectl exec lifecycle-demo -- cat /var/log/lifecycle.log

# 删除 Pod 观察 PreStop
kubectl delete pod lifecycle-demo

# 观察优雅终止过程 (terminationGracePeriodSeconds 默认 30s)
kubectl get pod lifecycle-demo -w
```
### 任务 3: Init Container 实践 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建带 Init Container 的 Pod
cat > init-container-demo.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: init-demo
spec:
  initContainers:
  - name: init-wait-db
    image: busybox
    command: ['sh', '-c', 'echo "Waiting for DB..." && sleep 5 && echo "DB Ready!"']
  - name: init-config
    image: busybox
    command: ['sh', '-c', 'echo "Config loaded" > /shared/config.txt']
    volumeMounts:
    - name: shared-data
      mountPath: /shared
  containers:
  - name: app
    image: nginx:alpine
    volumeMounts:
    - name: shared-data
      mountPath: /app/config
  volumes:
  - name: shared-data
    emptyDir: {}
EOF

kubectl apply -f init-container-demo.yaml

# 观察 Init Container 按顺序执行
kubectl get pod init-demo -w

# 查看 Init Container 日志
kubectl logs init-demo -c init-wait-db
kubectl logs init-demo -c init-config

# 验证配置文件
kubectl exec init-demo -- cat /app/config/config.txt

# 清理
kubectl delete pod init-demo
```
### 任务 4: 资源管理和 QoS (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Guaranteed QoS (requests = limits)
cat > qos-guaranteed.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: qos-guaranteed
spec:
  containers:
  - name: app
    image: nginx:alpine
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 100m
        memory: 128Mi
EOF

# Burstable QoS (requests < limits)
cat > qos-burstable.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: qos-burstable
spec:
  containers:
  - name: app
    image: nginx:alpine
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 200m
        memory: 256Mi
EOF

# BestEffort QoS (无 requests/limits)
cat > qos-besteffort.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: qos-besteffort
spec:
  containers:
  - name: app
    image: nginx:alpine
EOF

kubectl apply -f qos-guaranteed.yaml
kubectl apply -f qos-burstable.yaml
kubectl apply -f qos-besteffort.yaml

# 查看 QoS 等级
kubectl get pod qos-guaranteed -o jsonpath='{.status.qosClass}'
kubectl get pod qos-burstable -o jsonpath='{.status.qosClass}'
kubectl get pod qos-besteffort -o jsonpath='{.status.qosClass}'

# 清理
kubectl delete pod qos-guaranteed qos-burstable qos-besteffort
```
### 任务 5: HPA 自动扩缩容 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确保 metrics-server 已安装
kubectl top nodes

# 创建 Deployment
cat > hpa-demo.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: hpa-demo
spec:
  replicas: 1
  selector:
    matchLabels:
      app: hpa-demo
  template:
    metadata:
      labels:
        app: hpa-demo
    spec:
      containers:
      - name: app
        image: k8s.gcr.io/hpa-example
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 100m
          limits:
            cpu: 500m
---
apiVersion: v1
kind: Service
metadata:
  name: hpa-demo
spec:
  selector:
    app: hpa-demo
  ports:
  - port: 80
EOF

kubectl apply -f hpa-demo.yaml

# 创建 HPA
kubectl autoscale deployment hpa-demo --cpu-percent=50 --min=1 --max=10

# 或使用 YAML
cat > hpa.yaml << 'EOF'
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: hpa-demo
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hpa-demo
  minReplicas: 1
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 50
EOF

# 查看 HPA 状态
kubectl get hpa hpa-demo

# 生成负载
kubectl run -i --tty load-generator --rm --image=busybox --restart=Never -- /bin/sh -c "while true; do wget -q -O- http://hpa-demo; done"

# 在另一个终端观察扩容
kubectl get hpa hpa-demo -w
kubectl get pods -l app=hpa-demo -w

# 停止负载后观察缩容 (默认 5 分钟)

# 清理
kubectl delete deployment hpa-demo
kubectl delete svc hpa-demo
kubectl delete hpa hpa-demo
```
---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Liveness、Readiness、Startup 三种探针的区别和使用场景？**
   - Liveness: 检测容器是否需要重启
   - Readiness: 检测是否可以接收流量
   - Startup: 慢启动容器的保护

2. **QoS 的三个等级是什么？节点资源紧张时谁先被驱逐？**
   - Guaranteed > Burstable > BestEffort
   - BestEffort 最先被驱逐

3. **HPA 基于什么指标进行扩缩容？**
   - CPU 使用率
   - 内存使用率
   - 自定义指标

---

## 今日检验

- [ ] 能够配置三种探针并理解其作用
- [ ] 能够使用 Init Container 做初始化
- [ ] 理解 QoS 等级和资源管理
- [ ] 能够配置 HPA 实现自动扩缩容

---

## 核心配置模板

```yaml
# 资源配置模板
resources:
  requests:
    cpu: 100m      # 0.1 CPU
    memory: 128Mi
  limits:
    cpu: 500m      # 0.5 CPU
    memory: 512Mi

# 探针配置模板
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 10
  periodSeconds: 10
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /ready
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
```

---

## 明日预告

Day 12 将进入 K8s 网络栈，学习 CNI、Service、DNS 的工作原理。


<!-- risk-assessed -->
