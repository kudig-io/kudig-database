---
title: 'Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet'
description: '- "Deployment 滚动更新怎么配置"'
summary: '- "Deployment 滚动更新怎么配置"'
category: learning
tags:
- k8s
- training
- hands-on
- hpa
- vpa
- statefulset
- daemonset
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
- 'Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet 是什么'
- '如何 Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet'
trigger_keywords:
- Day
- '10:'
- 工作负载
- Deployment
- StatefulSet
- DaemonSet
- learn
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-ml-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 10: 工作负载 - Deployment + [[StatefulSet|StatefulSet]] + [[DaemonSet|DaemonSet]]

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY10
title: Day 10 - 工作负载 - Deployment + StatefulSet + DaemonSet
topic: kubernetes
type: hands-on-guide
tags: [deployment, statefulset, daemonset, replicaset, rolling-update, rollback, hands-on, week-2]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "Deployment 滚动更新怎么配置"
  - "StatefulSet 和 Deployment 区别"
  - "DaemonSet 什么场景用"
  - "maxSurge/maxUnavailable 怎么设置"
trigger_keywords:
  - Deployment
  - StatefulSet
  - DaemonSet
  - ReplicaSet
  - RollingUpdate
  - maxSurge
  - maxUnavailable
  - Rollback
  - revisionHistoryLimit
  - Headless Service
  - volumeClaimTemplate
  - topologyKey
  - nodeSelector
reading_level: beginner
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
  - deployment
  - statefulset
  - daemonset
related:
  - 生产运维/topic-learn/public-training/one-month/week-2-core-tech/day-11-workloads-2.md
  - 工作负载/02-deployment-production-patterns.md
---
```

> **学习时间**: 4-5 小时 | **主题**: K8s 核心工作负载类型

---

## 今日目标

- [ ] 掌握 Deployment 的滚动更新和回滚机制
- [ ] 理解 StatefulSet 的有序部署和稳定标识
- [ ] 掌握 DaemonSet 的使用场景和配置

---

## 理论学习 (2h)

### 必读文档

1. **Deployment 生产模式**
   - 文件: `../../工作负载/02-deployment-production-patterns.md`
   - 重点: 滚动更新、回滚、蓝绿、金丝雀

2. **StatefulSet 高级操作**
   - 文件: `../../工作负载/03-statefulset-advanced-operations.md`
   - 重点: 有状态应用、稳定网络标识、有序部署

3. **DaemonSet 管理**
   - 文件: `../../工作负载/04-daemonset-management.md`
   - 重点: 每节点一个 Pod、日志采集、监控代理

---

## 实践任务 (2.5h)

### 任务 1: Deployment 滚动更新深入 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Deployment
cat > rolling-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rolling-demo
spec:
  replicas: 4
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1        # 最多多创建 1 个 Pod
      maxUnavailable: 1  # 最多 1 个不可用
  selector:
    matchLabels:
      app: rolling-demo
  template:
    metadata:
      labels:
        app: rolling-demo
    spec:
      containers:
      - name: nginx
        image: nginx:1.20
        ports:
        - containerPort: 80
EOF

kubectl apply -f rolling-deployment.yaml

# 观察滚动更新过程
kubectl set image deployment/rolling-demo nginx=nginx:1.25
kubectl rollout status deployment/rolling-demo

# 实时观察 Pod 变化
kubectl get pods -l app=rolling-demo -w

# 查看 ReplicaSet 历史
kubectl get replicaset -l app=rolling-demo

# 查看更新历史
kubectl rollout history deployment/rolling-demo

# 回滚到上一版本
kubectl rollout undo deployment/rolling-demo

# 回滚到指定版本
kubectl rollout undo deployment/rolling-demo --to-revision=1
```
### 任务 2: StatefulSet 实践 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 创建 Headless Service (StatefulSet 必需)
cat > statefulset-demo.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: nginx-headless
spec:
  clusterIP: None
  selector:
    app: nginx-sts
  ports:
  - port: 80
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: nginx-sts
spec:
  serviceName: "nginx-headless"
  replicas: 3
  selector:
    matchLabels:
      app: nginx-sts
  template:
    metadata:
      labels:
        app: nginx-sts
    spec:
      containers:
      - name: nginx
        image: nginx:alpine
        ports:
        - containerPort: 80
        volumeMounts:
        - name: data
          mountPath: /usr/share/nginx/html
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Mi
EOF

kubectl apply -f statefulset-demo.yaml

# 观察有序创建 (0, 1, 2)
kubectl get pods -l app=nginx-sts -w

# 验证稳定的网络标识
kubectl exec nginx-sts-0 -- hostname
kubectl exec nginx-sts-1 -- hostname

# 验证 DNS 解析
kubectl run dns-test --image=busybox --rm -it --restart=Never -- nslookup nginx-sts-0.nginx-headless

# 删除 Pod 后观察重建 (保持相同名称)
kubectl delete pod nginx-sts-1
kubectl get pods -l app=nginx-sts -w

# 扩缩容 (有序)
kubectl scale statefulset nginx-sts --replicas=5
kubectl get pods -l app=nginx-sts -w

# 清理
kubectl delete statefulset nginx-sts
kubectl delete svc nginx-headless
kubectl delete pvc -l app=nginx-sts
```
### 任务 3: DaemonSet 实践 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 DaemonSet (模拟日志采集)
cat > daemonset-demo.yaml << 'EOF'
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: log-collector
spec:
  selector:
    matchLabels:
      app: log-collector
  template:
    metadata:
      labels:
        app: log-collector
    spec:
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
      containers:
      - name: [[fluentd|fluentd]]
        image: fluent/fluentd:v1.14-debian
        resources:
          limits:
            cpu: 100m
            memory: 200Mi
        volumeMounts:
        - name: varlog
          mountPath: /var/log
          readOnly: true
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
EOF

kubectl apply -f daemonset-demo.yaml

# 验证每个节点都运行了 Pod
kubectl get pods -l app=log-collector -o wide
kubectl get nodes
# Pod 数量 = 节点数量

# 查看 DaemonSet 状态
kubectl get daemonset log-collector

# 滚动更新 DaemonSet
kubectl set image daemonset/log-collector fluentd=fluent/fluentd:v1.15-debian
kubectl rollout status daemonset/log-collector

# 清理
kubectl delete daemonset log-collector
```
### 任务 4: 对比三种工作负载 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建对比实验
# 1. Deployment - 无状态
kubectl create deployment stateless-app --image=nginx:alpine --replicas=3

# 2. StatefulSet - 有状态 (使用上面的 YAML)

# 3. DaemonSet - 每节点 (使用上面的 YAML)

# 对比:
# - Deployment: Pod 名称随机 (stateless-app-xxx)
# - StatefulSet: Pod 名称有序 (nginx-sts-0, nginx-sts-1)
# - DaemonSet: 每节点一个 Pod

# 删除后重建:
# - Deployment: 新名称
# - StatefulSet: 相同名称
# - DaemonSet: 自动在所有节点运行

# 清理
kubectl delete deployment stateless-app
```
---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Deployment 的 maxSurge 和 maxUnavailable 如何影响滚动更新？**
   - maxSurge: 允许临时超过期望副本数
   - maxUnavailable: 允许不可用的最大数量
   - 两者控制更新的速度和平滑度

2. **StatefulSet 为什么需要 Headless Service？**
   - 为每个 Pod 提供稳定的 DNS 名称
   - 格式: `<pod-name>.<service-name>.<namespace>.svc.cluster.local`

3. **DaemonSet 的典型使用场景有哪些？**
   - 日志采集 (Fluentd, Filebeat)
   - 监控代理 (Node Exporter)
   - 网络插件 (CNI)
   - 存储插件 (CSI)

---

## 今日检验

- [ ] 能够配置 Deployment 的滚动更新策略
- [ ] 能够部署 StatefulSet 有状态应用
- [ ] 能够使用 DaemonSet 在每个节点运行 Pod
- [ ] 理解三种工作负载的区别和使用场景

---

## 工作负载对比

| 特性 | Deployment | StatefulSet | DaemonSet |
|------|------------|-------------|-----------|
| Pod 命名 | 随机后缀 | 有序编号 | 节点名后缀 |
| 扩缩容 | 并行 | 有序 | 自动跟随节点 |
| 存储 | 共享/无 | 独立 PVC | 通常用 hostPath |
| 网络 | 随机 IP | 稳定 DNS | 通常用 hostNetwork |
| 使用场景 | 无状态应用 | 数据库、缓存 | 节点代理 |

---

## 明日预告

Day 11 将学习 Pod 生命周期、资源管理和自动扩缩容 (HPA/VPA)。

## Related

- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
