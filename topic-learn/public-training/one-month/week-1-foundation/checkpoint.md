# Week 1 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. Docker 容器和虚拟机的本质区别是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- 容器共享宿主机内核，VM 有独立内核
- 容器是进程级隔离，VM 是硬件级隔离
- 容器启动秒级，VM 启动分钟级
- 容器资源占用小，VM 资源占用大

---

### 2. Linux namespace 有哪几种类型？各自隔离什么资源？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- pid: 进程 ID
- net: 网络栈
- mnt: 文件系统挂载点
- uts: 主机名
- ipc: 进程间通信
- user: 用户/组 ID
- cgroup: cgroup 根目录

---

### 3. cgroup 可以限制哪些资源？在 K8s 中如何体现？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- CPU、内存、IO、网络带宽
- K8s: resources.requests 和 resources.limits
- QoS 等级: Guaranteed, Burstable, BestEffort

---

### 4. K8s 的 etcd、API Server、Scheduler、Controller Manager 各做什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- etcd: 分布式键值存储，存储所有集群状态
- API Server: 集群网关，认证授权准入，所有组件通信中心
- Scheduler: 为 Pod 选择合适的节点
- Controller Manager: 运行各种控制器，维护期望状态

---

### 5. 某 Pod 一直处于 Pending 状态，你的排查步骤是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
1. `kubectl describe pod <name>` 查看 Events
2. 检查是否有足够的资源 (CPU/Memory)
3. 检查 nodeSelector/affinity 是否匹配
4. 检查 taints/tolerations
5. 检查 PVC 是否绑定
6. 检查 Scheduler 是否正常

---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. `kubectl rollout undo deployment/nginx` 做什么？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** 回滚到上一个版本的 Deployment

---

### 7. `kubectl port-forward pod/nginx 8080:80` 做什么？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** 将本地 8080 端口转发到 Pod 的 80 端口

---

### 8. `kubectl top node` 做什么？需要什么前置条件？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** 显示节点资源使用情况，需要安装 metrics-server

---

### 9. 如何查看 Pod 的实时日志？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl logs -f <pod-name>`

---

### 10. 如何进入一个正在运行的 Pod 执行命令？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl exec -it <pod-name> -- /bin/sh`

---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 当你执行 `kubectl apply -f deployment.yaml` 时，K8s 内部发生了什么？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
1. kubectl 发送请求到 API Server
2. API Server 认证、授权、准入控制
3. API Server 将资源存储到 etcd
4. Deployment Controller 发现新资源，创建 ReplicaSet
5. ReplicaSet Controller 创建 Pod
6. Scheduler 为 Pod 分配节点
7. kubelet 在节点上创建容器

---

### 12. 解释 Docker 镜像分层原理，以及为什么分层是有益的？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
- Union Filesystem 将多个层叠加
- 底层是只读的，顶层是可写的
- 好处: 层复用、构建效率、存储节省

---

### 13. 为什么 K8s 节点需要开启 `net.ipv4.ip_forward`？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
- Pod 网络需要跨节点通信
- 节点需要作为路由器转发流量
- 不开启会导致 Pod 间网络不通

---

### 14. Service 是如何将流量转发到 Pod 的？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
- Service 使用 Label Selector 选择 Pod
- kube-proxy 配置 iptables/IPVS 规则
- 流量通过 DNAT 转发到后端 Pod
- Endpoints 记录 Pod IP 列表

---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 20 |
| 命令实操 | __ | 10 |
| 场景分析 | __ | 20 |
| **总分** | __ | **50** |

### 评估标准

- **45-50 分**: 优秀，完全掌握本周内容
- **35-44 分**: 良好，基本掌握，部分细节需加强
- **25-34 分**: 及格，核心概念理解，需要复习
- **< 25 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

记录自测中暴露的薄弱点，下周重点复习:

```
1. 


2. 


3. 

```

---

## 下周计划调整

基于自测结果，调整下周学习重点:

```
需要加强的领域:


下周额外复习:


```
