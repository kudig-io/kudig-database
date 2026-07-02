---
title: 'Day 27: 存储卷挂载'
description: '# Day 27: 存储卷挂载'
summary: 'kubectl create configmap app-config --from-literal=APP_ENV=production --from-literal=LOG_LEVEL=info'
category: learning
tags:
- k8s
- training
- hands-on
- statefulset
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
- 'Day 27: 存储卷挂载 是什么'
- '如何 Day 27: 存储卷挂载'
trigger_keywords:
- Day
- '27:'
- 存储卷挂载
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 27: 存储卷挂载

```yaml
---
title: Day 27: 存储卷挂载
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes存储挂载"
  - "Volume挂载"
  - "PVC挂载"
  - "ConfigMap Secret挂载"
  - "StatefulSet存储"
trigger_keywords:
  - "存储挂载"
  - "Volume"
  - "PVC"
  - "ConfigMap"
  - "Secret"
  - "subPath"
  - "emptyDir"
  - "hostPath"
  - "StatefulSet"
  - "volumeMounts"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/checkpoint
  - domain-04-storage-data/02-pv-architecture-fundamentals
id: WEEK4-DAY27
topic: training
type: hands-on
tags: [week-4, day-27, storage, volume, mount, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 存储挂载方式与最佳实践

---

## 今日目标

- [ ] 掌握 Volume、PVC、ConfigMap、Secret 等多种挂载方式
- [ ] 能为 Deployment/StatefulSet 配置持久化存储
- [ ] 了解 subPath、readOnly 等挂载选项
- [ ] 掌握存储卷的扩容操作

---

## 理论学习 (2h)

### 必读文档

1. **K8S Volume 类型**
   - 文件: `../../../domain-04-storage-data/02-pv-architecture-fundamentals.md`
   - 重点: emptyDir、hostPath、PVC、configMap、secret

2. **ACK 存储最佳实践**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/245-ack-ebs-storage.md`
   - 重点: 云盘扩容、NAS 子目录挂载

### 阅读要点

- **emptyDir**: Pod 生命周期内的临时存储，Pod 删除即丢失
- **hostPath**: 挂载节点本地目录，存在安全风险
- **PVC**: 持久化存储的标准方式，推荐使用
- **configMap / secret**: 将配置和密钥挂载为文件
- **subPath**: 将 Volume 的子目录挂载到容器特定路径
- **readOnly**: 只读挂载，防止容器误写
- **volumeClaimTemplates**: StatefulSet 专用，每个副本自动创建独立 PVC

---

## 实践任务 (2.5h)

### 任务 1: 多种 Volume 挂载 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 ConfigMap 和 Secret
kubectl create configmap app-config --from-literal=APP_ENV=production --from-literal=LOG_LEVEL=info
kubectl create secret generic app-secret --from-literal=DB_PASSWORD=mypassword123

# 创建带多种挂载的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: mount-demo
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    volumeMounts:
    - name: tmp-data
      mountPath: /tmp/data
    - name: config-vol
      mountPath: /etc/app/config
      readOnly: true
    - name: secret-vol
      mountPath: /etc/app/secrets
      readOnly: true
    - name: single-config
      mountPath: /etc/nginx/conf.d/custom.conf
      subPath: custom.conf
  volumes:
  - name: tmp-data
    emptyDir:
      sizeLimit: 100Mi
  - name: config-vol
    configMap:
      name: app-config
  - name: secret-vol
    secret:
      secretName: app-secret
  - name: single-config
    configMap:
      name: app-config
      items:
      - key: APP_ENV
        path: custom.conf
EOF

# 验证挂载
kubectl exec mount-demo -- ls -la /tmp/data/
kubectl exec mount-demo -- cat /etc/app/config/APP_ENV
kubectl exec mount-demo -- cat /etc/app/secrets/DB_PASSWORD
kubectl exec mount-demo -- cat /etc/nginx/conf.d/custom.conf
```
### 任务 2: Deployment + PVC 持久化 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data-pvc
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 20Gi
EOF

# 创建使用 PVC 的 Deployment
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-with-storage
spec:
  replicas: 1
  selector:
    matchLabels:
      app: app-with-storage
  template:
    metadata:
      labels:
        app: app-with-storage
    spec:
      containers:
      - name: app
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        volumeMounts:
        - name: data
          mountPath: /usr/share/nginx/html
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: app-data-pvc
EOF

# 写入数据并验证持久化
kubectl exec deploy/app-with-storage -- sh -c 'echo "<h1>Persistent Data</h1>" > /usr/share/nginx/html/index.html'

# 重启 Pod 后数据仍在
kubectl delete pod -l app=app-with-storage
kubectl exec deploy/app-with-storage -- cat /usr/share/nginx/html/index.html
```
### 任务 3: StatefulSet + volumeClaimTemplates (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 StatefulSet (每个副本自动创建独立 PVC)
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: db-cluster
spec:
  serviceName: db-headless
  replicas: 3
  selector:
    matchLabels:
      app: db-cluster
  template:
    metadata:
      labels:
        app: db-cluster
    spec:
      containers:
      - name: db
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "Node: $(hostname)" > /data/identity.txt && sleep 3600']
        volumeMounts:
        - name: db-data
          mountPath: /data
  volumeClaimTemplates:
  - metadata:
      name: db-data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-ssd
      resources:
        requests:
          storage: 20Gi
---
apiVersion: v1
kind: Service
metadata:
  name: db-headless
spec:
  clusterIP: None
  selector:
    app: db-cluster
  ports:
  - port: 80
EOF

# 等待所有副本就绪
kubectl get pods -l app=db-cluster -w

# 查看每个副本的独立 PVC
kubectl get pvc | grep db-data

# 验证每个副本有独立数据
kubectl exec db-cluster-0 -- cat /data/identity.txt
kubectl exec db-cluster-1 -- cat /data/identity.txt
kubectl exec db-cluster-2 -- cat /data/identity.txt
```
### 任务 4: 云盘扩容 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源
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
# 查看 StorageClass 是否支持扩容
kubectl get sc alicloud-disk-ssd -o yaml | grep allowVolumeExpansion

# 扩容 PVC (仅增大，不可缩小)
kubectl patch pvc app-data-pvc -p '{"spec":{"resources":{"requests":{"storage":"40Gi"}}}}'

# 查看扩容状态
kubectl get pvc app-data-pvc
kubectl describe pvc app-data-pvc | grep -A 5 "Conditions"

# 验证扩容后容量
kubectl exec deploy/app-with-storage -- df -h /usr/share/nginx/html

# 清理
kubectl delete pod mount-demo
kubectl delete deploy app-with-storage
kubectl delete statefulset db-cluster
kubectl delete svc db-headless
kubectl delete pvc app-data-pvc
kubectl delete pvc -l app=db-cluster
kubectl delete configmap app-config
kubectl delete secret app-secret
```
---

## 费曼复述 (0.5h)

1. **emptyDir 和 PVC 的本质区别是什么？各自适用什么场景？**
2. **StatefulSet 的 volumeClaimTemplates 如何实现每副本独立存储？**
3. **subPath 挂载的作用是什么？不用 subPath 会有什么问题？**

---

## 今日检验

- [ ] 能配置 emptyDir / configMap / secret / PVC 等多种挂载
- [ ] 能为 Deployment 配置持久化存储
- [ ] 理解 StatefulSet volumeClaimTemplates 机制
- [ ] 了解云盘扩容操作

---

## 核心概念总结

| 挂载类型 | 生命周期 | 访问模式 | 适用场景 |
|---------|---------|---------|---------|
| emptyDir | Pod 级 | 容器间共享 | 临时缓存、sidecar 共享 |
| hostPath | 节点级 | 单节点 | 日志采集 (谨慎使用) |
| PVC (云盘) | 独立 | RWO | 数据库、单副本有状态应用 |
| PVC (NAS) | 独立 | RWX | 共享存储、多副本读写 |
| configMap | ConfigMap 级 | 只读 | 应用配置文件 |
| secret | Secret 级 | 只读 | 密码、证书、密钥 |

---

## 明日预告

Day 28 将进行 4 周培训的综合复习与实践。

## Related

- index/pvc-index|PVC 知识图谱索引]]


<!-- risk-assessed -->
