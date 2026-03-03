# Day 20: Pod 容器组进阶

> **学习时间**: 4-5 小时 | **主题**: Pod 调度、探针与资源配置

---

## 今日目标

- [ ] 掌握 Pod 调度策略 (nodeSelector / affinity / tolerations)
- [ ] 理解健康探针 (liveness / readiness / startup)
- [ ] 能够配置 Pod 资源请求与限制
- [ ] 了解 ACK 集群中 Pod 调度的特殊考量

---

## 理论学习 (2h)

### 必读文档

1. **Pod 调度策略**
   - 文件: `../../../domain-09-workload/05-pod-scheduling.md`
   - 重点: nodeSelector、nodeAffinity、podAffinity/podAntiAffinity

2. **健康检查探针**
   - 文件: `../../../domain-09-workload/06-pod-probes.md`
   - 重点: 三种探针类型、检测方式、参数配置

3. **ACK 调度优化**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/250-ack-workload.md`
   - 重点: ACK 中调度器扩展与拓扑感知调度

### 阅读要点

- **nodeSelector**: 最简单的节点选择方式，按标签匹配
- **nodeAffinity**: 更灵活，支持 `In`/`NotIn`/`Exists` 等操作符
- **podAffinity/podAntiAffinity**: 基于已运行 Pod 的标签做调度决策
- **Taint + Toleration**: 节点排斥机制，与 Day 16 结合理解
- **三种探针**: livenessProbe (存活)、readinessProbe (就绪)、startupProbe (启动)
- **资源管理**: requests (调度依据) vs limits (运行上限)

---

## 实践任务 (2.5h)

### 任务 1: nodeSelector 与 Affinity 调度 (40min)

```bash
# 给节点打标签
kubectl label nodes <node-name> workload=web

# 使用 nodeSelector 调度
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: selector-demo
spec:
  nodeSelector:
    workload: web
  containers:
  - name: nginx
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
EOF

# 使用 nodeAffinity 调度 (软约束)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: affinity-demo
spec:
  affinity:
    nodeAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 80
        preference:
          matchExpressions:
          - key: workload
            operator: In
            values: ["web", "api"]
  containers:
  - name: nginx
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
EOF

# 验证调度结果
kubectl get pod selector-demo affinity-demo -o wide
```

### 任务 2: Pod 反亲和性 (高可用部署) (30min)

```bash
# 创建 Deployment 并配置反亲和性 (Pod 分散到不同节点)
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ha-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: ha-app
  template:
    metadata:
      labels:
        app: ha-app
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values: ["ha-app"]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: nginx
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
EOF

# 查看 Pod 分布
kubectl get pod -l app=ha-app -o wide
```

### 任务 3: 健康探针配置 (40min)

```bash
# 创建带三种探针的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: probe-demo
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    ports:
    - containerPort: 80
    # 启动探针: 容器启动期间检测
    startupProbe:
      httpGet:
        path: /
        port: 80
      failureThreshold: 30
      periodSeconds: 2
    # 存活探针: 容器运行期间检测
    livenessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 0
      periodSeconds: 10
      failureThreshold: 3
    # 就绪探针: 决定是否接收流量
    readinessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 0
      periodSeconds: 5
      failureThreshold: 3
EOF

# 观察探针检测结果
kubectl describe pod probe-demo | grep -A 5 "Liveness\|Readiness\|Startup"

# 模拟探针失败: 删掉 index.html 触发 readiness 失败
kubectl exec probe-demo -- rm /usr/share/nginx/html/index.html
kubectl get pod probe-demo -w   # 观察 READY 变化
```

### 任务 4: 资源请求与限制 (30min)

```bash
# 创建带资源配置的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: resource-demo
spec:
  containers:
  - name: stress
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sh', '-c', 'while true; do echo running; sleep 10; done']
    resources:
      requests:
        cpu: 100m
        memory: 64Mi
      limits:
        cpu: 500m
        memory: 128Mi
EOF

# 查看 Pod 资源分配
kubectl describe pod resource-demo | grep -A 5 "Requests\|Limits"

# 查看节点资源占用情况
kubectl describe node <node-name> | grep -A 10 "Allocated resources"

# 清理
kubectl delete pod selector-demo affinity-demo probe-demo resource-demo
kubectl delete deploy ha-app
```

---

## 费曼复述 (0.5h)

1. **nodeAffinity 中 required 和 preferred 的区别是什么？调度失败分别会怎样？**
2. **livenessProbe 和 readinessProbe 失败后的行为有什么不同？**
3. **resources.requests 和 resources.limits 分别在什么阶段起作用？**

---

## 今日检验

- [ ] 能使用 nodeSelector 和 affinity 控制 Pod 调度
- [ ] 能配置 podAntiAffinity 实现高可用部署
- [ ] 能配置三种健康探针并理解其作用
- [ ] 能合理设置 Pod 的资源 requests 和 limits

---

## 核心概念总结

| 调度方式 | 粒度 | 硬/软 | 适用场景 |
|----------|------|-------|---------|
| nodeSelector | 节点标签 | 硬约束 | 简单节点选择 |
| nodeAffinity | 节点标签 (表达式) | 硬/软 | 灵活节点调度 |
| podAffinity | Pod 标签 | 硬/软 | 就近部署 |
| podAntiAffinity | Pod 标签 | 硬/软 | 分散部署 (HA) |
| toleration | 节点 taint | 允许调度 | 调度到特殊节点 |

| 探针类型 | 作用 | 失败动作 |
|----------|------|---------|
| startupProbe | 检测容器是否已启动 | 杀死容器重启 |
| livenessProbe | 检测容器是否存活 | 杀死容器重启 |
| readinessProbe | 检测容器是否就绪 | 从 Service 摘除 |

---

## 明日预告

Day 21 将学习 K8S 核心组件的状态检查与故障处理。
