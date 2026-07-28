---
title: 第14课：StatefulSet - 有状态应用管理 [10-advanced-workloads]
description: '# 第14课：StatefulSet - 有状态应用管理'
summary: '你需要在 [[23-实体/02-K8s核心组件/kubernetes.md|k8s]] 上部署 MySQL 数据库集群：'
category: k8s-lecturer
tags:
- k8s
- training
- lecturer
- redis
- mysql
- postgresql
- kafka
- pdb
- statefulset
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第14课：StatefulSet - 有状态应用管理 是什么
- 如何 第14课：StatefulSet - 有状态应用管理
trigger_keywords:
- 第14课：StatefulSet
- 有状态应用管理
- k8s
- lecturer
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-ml-basics
- kafka-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第14课：[[statefulset|StatefulSet]] - 有状态应用管理

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 25 分钟

---

## 学习目标

1. 理解有状态应用与无状态应用的区别
2. 掌握 StatefulSet 的概念和使用场景
3. 了解 StatefulSet 的稳定网络标识
4. 学会部署和管理有状态应用

---

## 1. 问题引入

### 1.1 无状态 vs 有状态

```
【无状态应用】

Deployment 管理的是无状态应用：
• Web 服务、API 服务
• 多个副本完全等价
• 可以随意删除、创建、扩缩容
• 不关心具体哪个节点运行

类比：餐厅的服务员
• 任何服务员都可以服务任何顾客
• 离职（删除）一个，再招一个（创建）就行
• 不需要记住之前的服务记录

【有状态应用】

数据库、消息队列、缓存等：
• 每个实例有自己的身份和数据
• 实例 A 的数据和实例 B 的不一样
• 删除实例可能会丢失数据
• 必须保持稳定的网络标识

类比：医院的医生
• 每个医生有自己的病人档案
• 张医生不能换成李医生（病人不认）
• 医生离职需要交接病人档案
```

### 1.2 问题场景

```
【场景】

你需要在 [[23-实体/02-K8s核心组件/kubernetes.md|k8s]] 上部署 MySQL 数据库集群：
• 1 个主库（写）
• 2 个从库（读）

问题：
• 如果用 Deployment，主库 Pod 挂了，新创建的 Pod IP 变了
• 应用配置里写死了主库的 IP，突然变了怎么办？
• 从库的复制关系怎么维持？
• 如果某个 Pod 需要持久化存储，Pod 漂移到新节点，数据还在吗？

【解决方案】

StatefulSet！

StatefulSet 专为有状态应用设计：
• 稳定的网络标识（Pod 名称固定）
• 稳定的存储（PVC 绑定到 Pod）
• 有序的部署和扩缩容
• 有序的删除和终止
```

---

## 2. StatefulSet 详解

### 2.1 基本配置

```
【YAML 示例】

apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql          # Headless Service 的名称
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
          name: mysql
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: password
  volumeClaimTemplates:       # PVC 模板
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: "standard"
      resources:
        requests:
          storage: 10Gi
```

### 2.2 核心概念

```
【稳定网络标识】

StatefulSet 创建的 Pod 有固定的名称：
mysql-0  ← 主库
mysql-1  ← 从库 1
mysql-2  ← 从库 2

Pod 名称不会因为重启而改变。
即使 Pod 被删除重建，名称仍然是 mysql-0、mysql-1、mysql-2。

【稳定存储】

每个 Pod 有自己独立的 PVC：
data-mysql-0  ← mysql-0 的存储
data-mysql-1  ← mysql-1 的存储
data-mysql-2  ← mysql-2 的存储

即使 Pod 漂移到其他节点，存储仍然绑定到该 Pod。

【有序的特性】

部署顺序：mysql-0 → mysql-1 → mysql-2
扩缩容顺序：mysql-2 → mysql-1 → mysql-0
删除顺序：mysql-2 → mysql-1 → mysql-0

每个 Pod 必须Ready 后，才会继续下一个。
```

---

## 3. Headless Service

### 3.1 什么是 Headless Service

```
【普通 Service】

ClusterIP Service：
• K8s 分配一个固定的 ClusterIP
• 通过 ClusterIP 访问后端 Pod
• K8s 自动做负载均衡

Headless Service：
• 不分配 ClusterIP
• 直接返回后端 Pod 的 DNS 记录
• 每个 Pod 有自己的 DNS 名称

【YAML 对比】

普通 Service：
apiVersion: v1
kind: Service
metadata:
  name: mysql
spec:
  type: ClusterIP
  selector:
    app: mysql
  ports:
  - port: 3306
    targetPort: 3306

Headless Service：
apiVersion: v1
kind: Service
metadata:
  name: mysql
spec:
  type: ClusterIP
  clusterIP: None          # 关键：设置为 None
  selector:
    app: mysql
  ports:
  - port: 3306
    targetPort: 3306
```

### 3.2 DNS 记录

```
【Headless Service 的 DNS】

对于 Headless Service "mysql"：
• mysql-0.mysql.default.svc.cluster.local
• mysql-1.mysql.default.svc.cluster.local
• mysql-2.mysql.default.svc.cluster.local

应用可以直接访问特定的 Pod：
• 写操作 → mysql-0.mysql.default.svc.cluster.local
• 读操作 → mysql-1.mysql.default.svc.cluster.local

【StatefulSet + Headless Service】

serviceName: mysql
必须与 Headless Service 的名称匹配！

StatefulSet 根据 serviceName 找到对应的 Headless Service，
从而知道 Pod 的 DNS 名称规则。
```

---

## 4. 部署顺序与管理

### 4.1 部署顺序

```
# 🟢 低风险：只读/信息收集，通常无副作用
【部署过程】

StatefulSet 创建 Pod 时是严格有序的：

1. 创建 mysql-0
   - 等待 mysql-0 Ready
   - 然后进入下一步

2. 创建 mysql-1
   - 等待 mysql-1 Ready
   - 然后进入下一步

3. 创建 mysql-2
   - 等待 mysql-2 Ready

【查看状态】

kubectl get statefulset
kubectl get pods -l app=mysql

输出：
NAME    READY   AGE
mysql   3/3     10m

NAME      READY   STATUS
mysql-0  1/1     Running
mysql-1  1/1     Running
mysql-2  1/1     Running
```
### 4.2 扩缩容

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【扩容】

kubectl scale statefulset mysql --replicas=4

扩容是增加 Pod，按顺序创建：
mysql-3

【缩容】

kubectl scale statefulset mysql --replicas=2

缩容是删除 Pod，按倒序删除：
mysql-3 → mysql-2

注意：缩容时如果有 PVC，数据可能会丢失！
建议先备份数据再缩容。

【⚠️ 高危命令：强制删除卡住的 Pod】

如果某个 Pod 卡住无法删除：
```bash
# ⚠️ 危险！可能导致数据丢失
kubectl delete pod mysql-2 -n <namespace> --grace-period=0 --force
```
⚠️ 风险提示：
• 可能导致 StatefulSet 数据丢失
• Pod 的 PVC 可能被回收
• 应该先尝试正常删除

StatefulSet 会检测到 Pod 丢失并重新创建。
```

---

## 5. 更新策略

### 5.1 更新类型

```
【RollingUpdate（默认）】

StatefulSet 默认使用 RollingUpdate：
• 按倒序更新 Pod（mysql-2 → mysql-1 → mysql-0）
• 必须等待当前 Pod Ready 后才更新下一个

【Partition】

可以配置 partition 实现金丝雀发布：
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 1

partition: 1 表示：
• mysql-2 和 mysql-1 会更新（索引 >= 1）
• mysql-0 不会更新（索引 < 1）

用于新版本验证，先更新部分节点。

【OnDelete】

spec:
  updateStrategy:
    type: OnDelete

只会在你手动删除 Pod 时才更新。
不会自动滚动更新。
```

---

## 6. 常见问题

### 6.1 Pod 一直处于 Pending

```
【原因】

PVC 无法绑定（存储不足、StorageClass 不存在等）。

【排查步骤】

1. 检查 PVC 状态
   kubectl get pvc

2. 查看 PVC 详情
   kubectl describe pvc data-mysql-0

3. 检查 StorageClass
   kubectl get storageclass

4. 如果是云存储，检查 CSI driver
   kubectl get pods -n kube-system | grep csi
```
# 🟢 低风险：只读/信息收集，通常无副作用
### 6.2 StatefulSet 无法更新

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【排查步骤】

1. 检查更新策略
   kubectl describe statefulset mysql | grep -A5 "Update Strategy"

2. 检查 Pod 状态
   kubectl get pods -l app=mysql

3. 查看具体 Pod 的问题
   kubectl describe pod mysql-0

4. 如果某个 Pod 卡住，删除它
   kubectl delete pod mysql-0

   StatefulSet 会自动重新创建。
```

### 6.3 如何持久化数据

```
【关键配置】

volumeClaimTemplates：
- metadata:
    name: data
  spec:
    accessModes: ["ReadWriteOnce"]
    storageClassName: "standard"
    resources:
      requests:
        storage: 10Gi

【数据安全建议】

1. 不要轻易缩容
   缩容会删除 Pod，对应的 PVC 可能被删除（取决于 reclaimPolicy）

2. 设置合理的 reclaimPolicy
   kubectl get pvc -o jsonpath='{.items[*].spec.persistentVolumeReclaimPolicy}'

3. 建议使用 ReadWriteOnce + 回收策略为 Retain

4. 定期备份重要数据
```

---

## 7. 数字人 Q&A 场景

### 7.1 用户问：StatefulSet 和 Deployment 有什么区别？

```
【回复】

"好问题！让我来对比一下：

【核心区别】

Deployment：
• 适合无状态应用
• 多个副本完全等价
• Pod 名称随机（如 web-5db4f9d7f-abcde）
• 存储不绑定到 Pod（删除后数据丢失）

StatefulSet：
• 适合有状态应用
• 每个 Pod 有固定的身份
• Pod 名称固定（mysql-0、mysql-1）
• 存储绑定到 Pod（删除后存储保留）

【生活类比】

Deployment = 酒店的自助餐
• 任何盘子都可以用
• 拿走一个，再拿一个一样的
• 不关心具体哪个盘子

StatefulSet = 医院的病房
• 每个病人有固定床位
• 床位和病人绑定
• 不能随意换床位

【使用场景】

用 Deployment：
• Web 应用、API 服务
• 不需要持久化数据

用 StatefulSet：
• 数据库（MySQL、PostgreSQL）
• 消息队列（Kafka、RabbitMQ）
• 缓存（Redis）
• 需要稳定网络标识的应用

有其他问题吗？"
```

### 7.2 用户问：StatefulSet 的 Pod 名称为什么是固定的？

```
【回复】

"这是 StatefulSet 的核心设计：稳定网络标识。

【为什么需要固定名称？】

比如 MySQL 主从集群：
• 主库 mysql-0 负责写操作
• 从库 mysql-1、mysql-2 负责读操作
• 应用配置里写的是：主库地址是 mysql-0.mysql.default.svc.cluster.local

如果 mysql-0 挂了，Deployment 会创建新 Pod，
但新 Pod 名称可能是随机的，配置就失效了。

StatefulSet 确保：
• mysql-0 永远是 mysql-0
• 即使 Pod 被删除重建
• 名称不会变

【PVC 绑定】

不仅名称固定，存储也绑定：
• data-mysql-0 绑定到 mysql-0
• 即使 Pod 漂移到其他节点
• 数据还在 data-mysql-0 里

这就像：
• 病人固定床位 A
• 床位 A 的病历柜也固定给病人 A 用
• 病人换床了，病历柜跟着换吗？ → StatefulSet 的答案是跟着

有其他问题吗？"
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
---

## 8. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【命令速查】

创建 StatefulSet：
kubectl apply -f statefulset.yaml

查看 StatefulSet：
kubectl get statefulset
kubectl describe statefulset <name>

查看 Pod：
kubectl get pods -l app=<label>

扩容：
kubectl scale statefulset <name> --replicas=3

更新镜像：
kubectl set image statefulset/<name> <container>=<image>

删除（会删除 Pod 但保留 PVC）：
kubectl delete statefulset <name>

【核心要点】

1. StatefulSet 用于有状态应用
2. Pod 有稳定的名称和网络标识
3. 每个 Pod 有独立的 PVC 存储
4. 部署、扩缩容、删除都是有序的
5. 需要配合 Headless Service 使用

【下节课预告】

下节课我们会学习调度与亲和性：
• 污点（Taints）和容忍（Tolerations）
• 节点亲和性（Node Affinity）
• Pod 亲和性（Pod Affinity）
• 如何控制 Pod 调度到特定节点

有问题吗？"
```

---

**关联文档**:
- [../10-advanced-workloads/10-daemonset-basics.md](../../../daemonset/%E5%9F%B9%E8%AE%AD/lecturer/10-daemonset-basics.md) — [[daemonset|DaemonSet]]
- [../../故障诊断/topic-skills/17-daemonset-pdb-failure.md](../../故障诊断/技能体系/17-daemonset-pdb-failure.md) — StatefulSet 问题 [[SKILL|Skill]]
- [../../工作负载/](../../工作负载/) — 工作负载文档

<!-- risk-assessed -->
