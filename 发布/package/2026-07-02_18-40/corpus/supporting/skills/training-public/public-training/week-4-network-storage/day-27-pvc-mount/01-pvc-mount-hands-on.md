---
title: 'Day 27: 存储卷挂载实操'
description: '# Day 27: 存储卷挂载实操'
summary: 'kubectl get pod <pod-name> -o jsonpath='{.spec.volumes[*].mountPropagation}''
category: learning
tags:
- k8s
- training
- hands-on
- scheduler
- opa
- mysql
- statefulset
- rag
- cilium
- flannel
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 27: 存储卷挂载实操 是什么'
- '如何 Day 27: 存储卷挂载实操'
trigger_keywords:
- Day
- '27:'
- 存储卷挂载实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- ebpf-basics
- cilium-basics
- cni-basics
- mysql-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 27: 存储卷挂载实操

> **日期**: Week 4 Day 6 | **主题**: 存储挂载方式与最佳实践 | **版本**: [[entities/kubernetes.md|[[Kubernetes 生产环境速查卡|k8s]]]] 1.28-1.33

---

## 1. 存储挂载类型

### 1.1 挂载类型对比

| 类型 | 说明 | 用途 |
|------|------|------|
| `emptyDir` | 临时存储，Pod 删除后丢失 | 临时缓存/共享内存 |
| `hostPath` | 宿主机目录 | 开发/单节点 |
| `persistentVolumeClaim` | PVC 持久化存储 | 生产环境有状态应用 |
| `configMap` | 配置文件挂载 | 应用配置 |
| `secret` | 密钥挂载 | 敏感信息 |
| `projected` | 多种资源投影 | 服务账号令牌等 |

---

## 2. emptyDir 实践

### 2.1 基本使用

```yaml
apiVersion: v1
kind of Pod
metadata:
  name: app-with-tmp
spec:
  containers:
    - name: app
      image: app:v1
      volumeMounts:
        - name: tmp-storage
          mountPath: /tmp
  volumes:
    - name: tmp-storage
      emptyDir:
        sizeLimit: 1Gi
        medium: Memory  # 使用内存存储（高性能）
```

### 2.2 共享存储（多容器）

```yaml
apiVersion: v1
kind of Pod
metadata:
  name: sidecar-pod
spec:
  containers:
    - name: main
      image: app:v1
      volumeMounts:
        - name: shared-data
          mountPath: /data
    - name: sidecar
      image: sidecar:v1
      volumeMounts:
        - name: shared-data
          mountPath: /output
  volumes:
    - name: shared-data
      emptyDir: {}
```

---

## 3. 挂载最佳实践

### 3.1 安全挂载

```yaml
# 只读挂载
volumeMounts:
  - name: config
    mountPath: /etc/config
    readOnly: true

# 使用 subPath（避免卷变更影响其他容器）
volumeMounts:
  - name: config
    mountPath: /etc/app/config
    subPath: config
```

### 3.2 挂载传播

```yaml
# 配置挂载传播（hostPath 场景）
volumes:
  - name: host-data
    hostPath:
      path: /data
      type: DirectoryOrCreate
    mountPropagation: HostToContainer  # 允许容器向主机传播挂载
```

### 3.3 SubPath 注意事项

```yaml
# 使用 subPath 避免覆盖目录
volumeMounts:
  - name: app-config
    mountPath: /etc/app
    subPath: config  # 只挂载 config 子目录
```

---

## 4. 配置与密钥挂载

### 4.1 ConfigMap 挂载

```yaml
# 创建 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  config.json: |
    {"log_level": "info", "db_host": "localhost"}
  nginx.conf: |
    server { listen 80; }
---
# 挂载为文件
apiVersion: v1
kind of Pod
metadata:
  name: app
spec:
  containers:
    - name: app
      image: app:v1
      volumeMounts:
        - name: config
          mountPath: /etc/config
  volumes:
    - name: config
      configMap:
        name: app-config
        items:
          - key: config.json
            path: config.json  # 挂载为指定文件名
```

### 4.2 Secret 挂载

```yaml
# 创建 Secret
apiVersion: v1
kind: Secret
metadata:
  name: db-credentials
type: Opaque
stringData:
  username: admin
  password: changeme
---
# 挂载为环境变量
apiVersion: v1
kind of Pod
metadata:
  name: app
spec:
  containers:
    - name: app
      image: app:v1
      env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: username
        - name: DB_PASS
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: password
```

---

## 5. [[StatefulSet|StatefulSet]] 存储管理

### 5.1 有状态应用存储

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  selector:
    matchLabels:
      app: mysql
  serviceName: mysql
  replicas: 3
  template:
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: standard
        resources:
          requests:
            storage: 20Gi
```

### 5.2 稳定网络标识

```yaml
# StatefulSet 提供稳定的主机名
# mysql-0.mysql-headless.default.svc.cluster.local
# mysql-1.mysql-headless.default.svc.cluster.local

# 配套 Headless Service
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
spec:
  clusterIP: None  # Headless
  selector:
    app: mysql
  ports:
    - name: mysql
      port: 3306
```

---

## 6. 挂载故障排查

### 6.1 常见错误

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. "MountVolume.Mount failed"
kubectl describe pod <pod-name> | grep -A15 "Events:"

# 2. 路径权限问题
# 检查容器内目录权限
kubectl exec -it <pod-name> -- ls -la /data

# 3. subPath 问题
# subPath 不支持动态扩展
# 检查 ConfigMap/Secret 更新是否生效
```
### 6.2 挂载传播问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查挂载传播配置
kubectl get pod <pod-name> -o jsonpath='{.spec.volumes[*].mountPropagation}'

# 常见问题：hostPath 挂载后容器内不可见
# 解决：确认 mountPropagation 设置为 HostToContainer 或 Bidirectional
```
### 6.3 ConfigMap/Secret 更新不生效

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 方式 1: 重启 Pod（推荐）
kubectl rollout restart deployment <deploy-name>

# 方式 2: 使用 subPath（会绕过热更新）
# subPath 挂载不会自动更新

# 方式 3: 监听挂载（推荐用于生产）
# 使用 dirsync 或 inotifywait 监听文件变化
```
---

## 7. 存储性能优化

### 7.1 存储类型选择

| 应用场景 | 推荐存储 |
|---------|---------|
| 高性能数据库 | SSD 云盘 (io1/gp3) |
| 日志/临时存储 | emptyDir (内存) |
| 共享文件存储 | NFS/CIFS |
| 大数据/HDFS | 本地 NVMe |

### 7.2 I/O 调度优化

```bash
# 在节点上查看当前 I/O scheduler
cat /sys/block/sda/queue/scheduler

# 对 SSD 使用 noopcheduler
echo noop > /sys/block/sda/queue/scheduler

# 在 StorageClass 中指定 volumeLifecyclePolicy
# 确保使用本地 SSD 的 Pod 使用 noop scheduler
```

---

## 8. 实战练习

**练习 1**: 创建带 emptyDir 的 Pod，验证多容器间共享数据

**练习 2**: 将 ConfigMap 挂载为配置文件，验证应用能读取配置

**练习 3**: 创建有状态 MySQL StatefulSet，验证数据持久化

**练习 4**: 模拟 ConfigMap 更新不生效问题，排查并解决

---

```yaml
---
id: LEARN-WEEK4-DAY27
title: Day 27 - 存储卷挂载实操
topic: network-storage
type: hands-on-guide
tags: [volume, mount, emptydir, configmap, secret, statefulset, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "存储挂载类型有哪些"
  - "emptyDir 怎么用"
  - "ConfigMap 怎么挂载"
  - "StatefulSet 存储怎么管理"
  - "subPath 挂载问题怎么解决"
trigger_keywords:
  - emptyDir
  - hostPath
  - configMap
  - secret
  - volumeMounts
  - subPath
  - mountPropagation
  - StatefulSet
  - 存储卷
  - PVC 挂载
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 40min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-04-storage-data
related_topics:
  - storage
  - pvc
  - statetefulset
  - configmap
related:
  - domain-11-production-operations/topic-learn/public-training/week-4-network-storage/day-26-pvc-create/01-pvc-create-hands-on.md
  - domain-10-troubleshooting-diagnostics/10-pv-pvc-troubleshooting.md
---
```

---

## 自测题 (Self-Check)

**1. ClusterIP 如何实现?**

<details><summary>答案</summary>

kube-proxy 通过 iptables/IPVS 将 ClusterIP DNAT 到后端 PodIP:TargetPort。

</details>

**2. [[Ingress|Ingress]] vs Gateway API?**

<details><summary>答案</summary>

Ingress 仅 HTTP, 需注解扩展; Gateway API 支持 HTTP/gRPC/TCP, 原生流量分割, 角色分离。

</details>

**3. StatefulSet 稳定网络标识原理?**

<details><summary>答案</summary>

Pod 名 <sts>-<ordinal> + Headless Service → DNS <pod>.<svc>.<ns>.svc.cluster.local。

</details>

**4. 如何选 CNI?**

<details><summary>答案</summary>

Calico (通用 BGP/VXLAN) / Cilium (eBPF 高性能) / Flannel (简单无 Policy)。生产推荐 Cilium 或 Calico。

</details>

**5. PVC 三种访问模式?**

<details><summary>答案</summary>

ReadWriteOnce (单节点 RW) / ReadOnlyMany (多节点 RO) / ReadWriteMany (多节点 RW)。

</details>



<!-- risk-assessed -->
