---
title: 'Week 2 Checkpoint: 自测检验'
description: '- 综合设计'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- mysql
- hpa
- statefulset
- ingress
- networkpolicy
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 2 Checkpoint: 自测检验 是什么'
- '如何 Week 2 Checkpoint: 自测检验'
trigger_keywords:
- Week
- 'Checkpoint:'
- 自测检验
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- mysql-basics
---

# Week 2 Checkpoint: 自测检验

```yaml
---
id: LEARN-ONE-MONTH-W2-CHECKPOINT
title: Week 2 Checkpoint - 自测检验
topic: kubernetes
type: checkpoint
tags: [checkpoint, self-test, week-2, deployment, statefulset, service, ingress, pvc, hpa, networkpolicy]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "K8s Week 2 自测题"
  - "Deployment 滚动更新题"
  - "StatefulSet vs Deployment 区别"
  - "Service 转发原理"
  - "PV/PVC 动态供应流程"
trigger_keywords:
  - 自测
  - checkpoint
  - 概念理解
  - 命令实操
  - 场景分析
  - 综合设计
  - 评分标准
  - 薄弱点
  - 知识点速查
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 90min
related_domains:
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - workloads
  - networking
  - storage
  - troubleshooting
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/[[domain-07-platform-engineering/topic-code-analysis/deployment-create/README|README]].md
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-8-control-plane-1.md
---
```

> 完成本周学习后，请独立完成以下自测题。

---

## 概述

本测验覆盖 Week 2 全部核心知识点，包括 Deployment/StatefulSet 管理、Service/Ingress 网络配置、PV/PVC 存储管理、HPA 自动伸缩和 NetworkPolicy 安全策略。测验分为四个部分，总计 80 分。答题时间限制 90 分钟。

---

## 一、概念理解 (每题 3 分，共 30 分)

### 1. Deployment 的 maxSurge 和 maxUnavailable 如何影响滚动更新行为？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```yaml
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
```

| 参数 | 含义 | 计算方式 | 示例 (replicas=4) |
|------|------|---------|------------------|
| maxSurge | 允许临时超过期望副本数的数量/比例 | 绝对值 或 百分比 | 1 → 最多 5 个 Pod |
| maxUnavailable | 允许不可用的最大数量/比例 | 绝对值 或 百分比 | 0 → 始终保持 4 个可用 |

常见配置组合:

| 组合 | maxSurge | maxUnavailable | 更新速度 | 可用性 |
|------|----------|---------------|---------|--------|
| 零宕机 | 1 | 0 | 慢 | 始终满副本 |
| 快速 | 25% | 25% | 快 | 中间态可能不可用 |
| 保守 | 1 | 1 | 中等 | 平衡 |

验证命令:

```bash
kubectl rollout status deployment/nginx
kubectl get pods -l app=nginx -w
```

---

### 2. StatefulSet 为什么不能像 Deployment一样随意调度？headless Service 的作用是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

StatefulSet vs Deployment 关键差异:

| 特性 | Deployment | StatefulSet |
|------|-----------|-------------|
| Pod 名称 | 随机后缀 (app-a1b2c) | 有序固定 (app-0, app-1, app-2) |
| DNS 记录 | 无独立 DNS | 每个 Pod 有独立 DNS |
| 启动顺序 | 并行启动 | 有序启动 (0 → 1 → 2) |
| 滚动更新 | 同时更新 | 倒序更新 (2 → 1 → 0) |
| 存储 | 共享或无 | 每个 Pod 独立 PVC |
| 网络标识 | 不稳定 | 稳定可预测 |

Headless Service 配置:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
spec:
  clusterIP: None
  selector:
    app: mysql
  ports:
  - port: 3306
```

DNS 记录格式:

```
<pod-name>.<service-name>.<namespace>.svc.cluster.local

mysql-0.mysql-headless.default.svc.cluster.local  → Pod IP
mysql-1.mysql-headless.default.svc.cluster.local  → Pod IP
mysql-2.mysql-headless.default.svc.cluster.local  → Pod IP
```

---

### 3. ClusterIP Service 的流量是如何通过 iptables/IPVS 转发到 Pod 的？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

iptables 模式流量路径:

```
Client (10.244.1.5) → Service ClusterIP (10.96.0.100:80)
  │
  ├─ iptables PREROUTING → KUBE-SERVICES chain
  │   ├─ 匹配目标 IP:Port → KUBE-SVC-XXX chain
  │   │   ├─ 概率 33% → KUBE-SEP-POD1 (DNAT → 10.244.1.10:80)
  │   │   ├─ 概率 33% → KUBE-SEP-POD2 (DNAT → 10.244.2.10:80)
  │   │   └─ 概率 34% → KUBE-SEP-POD3 (DNAT → 10.244.3.10:80)
  │   └─ 无匹配 → RETURN
  │
  └─ 回程: conntrack 表自动反向 DNAT
```

iptables vs IPVS 对比:

| 特性 | iptables | IPVS |
|------|----------|------|
| 算法复杂度 | O(n) | O(1) |
| 负载均衡算法 | 随机概率 | rr/lc/wrr/sh/dh 等 |
| 规则规模 | 1000+ 规则时性能下降 | 支持万级规则 |
| 默认模式 | K8s 默认 | kube-proxy --mode=ipvs |

---

### 4. PV 的 Reclaim Policy 三种的区别和使用场景？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 策略 | 行为 | 数据处理 | 适用场景 |
|------|------|---------|---------|
| Retain | 保留 PV 和数据 | 手动清理 | 重要数据 (数据库、日志) |
| Delete | 自动删除 PV 和底层存储 | 自动删除 | 临时数据 (缓存、构建产物) |
| Recycle | 清空数据后重新可用 | rm -rf | **已废弃** (K8s 1.15+) |

StorageClass 关联:

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: retain-sc
provisioner: diskplugin.csi.alibabacloud.com
reclaimPolicy: Retain
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: delete-sc
provisioner: diskplugin.csi.alibabacloud.com
reclaimPolicy: Delete
```

---

### 5. 如果应用频繁 OOMKilled，resources.limits.memory 应该如何调整？依据是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

OOM 排查与调优步骤:

```bash
# Step 1: 确认 OOMKilled
kubectl describe pod <name> | grep -A 5 "Last State"
# Last State:     Terminated
#   Reason:       OOMKilled
#   Exit Code:    137

# Step 2: 查看实际内存使用趋势
kubectl top pod <name> --containers
# NAME           CPU(cores)   MEMORY(bytes)
# my-app-xxx     50m          480Mi    ← 接近 512Mi limit

# Step 3: 使用 Prometheus 查看内存趋势
# max_over_time(container_memory_working_set_bytes{pod="<name>"}[7d])

# Step 4: 调整资源
```

调整策略:

| 场景 | requests | limits | 说明 |
|------|---------|--------|------|
| 正常 | 正常使用量 | 峰值使用量 * 1.5 | 标准 Java/JVM 应用 |
| 内存泄漏 | 当前值 | 临时调高，排查泄漏 | 先临时缓解再修根因 |
| JVM 应用 | -Xms 值 | -Xmx 值 + 25% overhead | JVM 堆外内存 |
| Go/Rust | 正常使用量 | 峰值使用量 * 1.2 | 内存使用较稳定 |

---

### 6. HPA 的工作原理是什么？支持哪些指标？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

HPA 计算公式:

```
期望副本数 = ceil(当前指标值 / 目标指标值 * 当前副本数)
示例: 当前 3 副本，CPU 使用率 90%，目标 70%
期望 = ceil(90 / 70 * 3) = ceil(3.86) = 4
```

---

### 7. Ingress 和 Service 的关系是什么？何时使用 Ingress？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 特性 | Service | Ingress |
|------|---------|---------|
| OSI 层级 | L4 (TCP/UDP) | L7 (HTTP/HTTPS) |
| 路由能力 | 端口转发 | 域名/路径路由 |
| TLS 终止 | 不支持 | 支持 |
| 对外暴露 | ClusterIP/NodePort/LB | 通过 Ingress Controller |
| 配置复杂度 | 简单 | 较复杂 |

流量路径:

```
Client → Ingress Controller (Nginx/ALB)
  → Ingress Rule (域名/路径匹配)
    → Service (L4 负载均衡)
      → Pod
```

---

### 8. NetworkPolicy 的默认行为是什么？如何实现"只允许前端访问 API"？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-allow-frontend
  namespace: api
spec:
  podSelector:
    matchLabels:
      app: api
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: frontend
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
```

默认行为规则:

| 状态 | 行为 |
|------|------|
| 无 NetworkPolicy | 所有流量允许 |
| 有 Ingress Policy | 未明确允许的 Ingress 被拒绝 |
| 有 Egress Policy | 未明确允许的 Egress 被拒绝 |

---

### 9. PV/PVC 动态供应的完整流程

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**

```
1. 管理员创建 StorageClass
   └── 定义 Provisioner (如 alicloud-disk)
   
2. 用户创建 PVC
   └── 指定 storageClassName: my-sc
   
3. Provisioner (CSI) 监听 PVC
   └── 检测到 pending 的 PVC
   
4. Provisioner 调用云 API 创建存储卷
   └── 如: 创建阿里云云盘
   
5. Provisioner 自动创建 PV
   └── 关联到刚创建的存储卷
   
6. PVC 与 PV 自动绑定
   └── status.phase: Bound
   
7. Pod 使用 PVC
   └── volumes.pvc → mount 到容器
```

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 20Gi
```

---

### 10. K8s 中 Service 的 Endpoints 和 EndpointSlice 有什么区别？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| 数据结构 | 单个资源包含所有 IP | 分片存储，每个最多 100 IP |
| 扩展性 | 1000+ IP 时性能差 | 支持大规模集群 |
| 网络类型 | 仅 IPv4 | IPv4/IPv6 双栈 |
| 默认模式 | K8s < 1.21 | K8s >= 1.21 默认 |

```bash
kubectl get endpoints my-svc
kubectl get endpointslices -l kubernetes.io/service-name=my-svc
```

---

## 二、命令实操 (每题 2 分，共 16 分)

### 11. 如何查看 Deployment 的更新历史？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl rollout history deployment/<name>
kubectl rollout history deployment/<name> --revision=2
```

---

### 12. 如何将 Deployment 回滚到指定版本？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl rollout undo deployment/<name> --to-revision=2
```

---

### 13. 如何查看 Service 对应的 Endpoints？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl get endpoints <service-name>
kubectl describe svc <service-name> | grep Endpoints
```

---

### 14. 如何测试 DNS 解析是否正常？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl run dns-test --image=busybox --rm -it -- nslookup <service-name>
kubectl run dns-test --image=busybox --rm -it -- nslookup <service-name>.<namespace>.svc.cluster.local
```

---

### 15. 如何查看 HPA 的当前状态和指标？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl get hpa <name>
kubectl describe hpa <name>
```

---

### 16. 如何查看 StorageClass 列表？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl get sc
kubectl get storageclass
```

---

### 17. 如何创建一个带有 nodeSelector 的 Pod？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl run nginx --image=nginx --overrides='{"spec":{"nodeSelector":{"disktype":"ssd"}}}'
```

---

### 18. 如何查看 Pod 的 QoS 等级？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

```bash
kubectl get pod <name> -o jsonpath='{.status.qosClass}'
```

---

## 三、场景分析 (每题 5 分，共 20 分)

### 19. 设计一个有状态应用 (如 MySQL) 的部署方案

**你的回答:**

```
(在此写下你的答案)





```

**参考要点:**

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql-headless
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        ports:
        - containerPort: 3306
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: password
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        livenessProbe:
          exec:
            command: ["mysqladmin", "ping", "-h", "localhost"]
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command: ["mysql", "-h", "127.0.0.1", "-e", "SELECT 1"]
          initialDelaySeconds: 10
          periodSeconds: 5
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: "2"
            memory: 4Gi
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-ssd
      resources:
        requests:
          storage: 100Gi
---
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
spec:
  clusterIP: None
  selector:
    app: mysql
  ports:
  - port: 3306
```

---

### 20. 解释 Ingress 和 Service 的关系及使用场景

**你的回答:**

```
(在此写下你的答案)





```

---

### 21. 如何设计一个 HPA 策略确保应用在流量高峰时自动扩容？

**你的回答:**

```
(在此写下你的答案)





```

**参考要点:**

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
  maxReplicas: 20
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 30
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: 1000
```

---

### 22. 描述 PV/PVC 动态供应的完整流程

**你的回答:**

```
(在此写下你的答案)





```

---

## 四、综合设计 (每题 7 分，共 14 分)

### 23. 设计一个完整的微服务部署方案 (前端 + API + 数据库)，要求高可用、自动伸缩、网络隔离。

**你的回答:**

```
(在此写下你的答案)





```

---

### 24. 设计一个 NetworkPolicy 方案实现零信任网络

**你的回答:**

```
(在此写下你的答案)





```

---

## 五、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 30 |
| 命令实操 | __ | 16 |
| 场景分析 | __ | 20 |
| 综合设计 | __ | 14 |
| **总分** | __ | **80** |

### 评估标准

- **72-80 分**: 优秀，完全掌握本周内容
- **56-71 分**: 良好，基本掌握，部分细节需加强
- **40-55 分**: 及格，核心概念理解，需要复习
- **< 40 分**: 不及格，建议重新学习本周内容

---

## 六、薄弱点记录

```
1.



2.



3.


```

---

## 七、知识点速查表

| 知识点 | 关键命令/概念 | 对应测验题 |
|--------|-------------|-----------|
| 滚动更新 | maxSurge/maxUnavailable 策略 | Q1 |
| StatefulSet | 有序 Pod/Headless Service/PVC | Q2 |
| Service 转发 | iptables DNAT / IPVS | Q3 |
| Reclaim Policy | Retain/Delete/Recycle | Q4 |
| OOM 排查 | describe pod/exit code 137 | Q5 |
| HPA | metrics/v2/计算公式 | Q6 |
| Ingress | L7 路由/TLS/域名匹配 | Q7 |
| NetworkPolicy | 默认允许/白名单模式 | Q8 |
| 动态供应 | StorageClass/Provisioner/PVC | Q9 |
| EndpointSlice | 分片/大规模集群 | Q10 |

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
