---
title: CKA 认证备考完全指南
description: 面向小白和在职人员的 CKA（Certified Kubernetes Administrator）备考指南，包含考纲映射、4 周冲刺计划、高频考点、模拟题、考试技巧与注意事项
summary: 面向小白和在职人员的 CKA（Certified Kubernetes Administrator）备考指南，包含考纲映射、4 周冲刺计划、高频考点、模拟题、考试技巧与注意事项
category: learning
tags:
- CKA
- certification
- exam
- beginner
- admin
- etcd
- apiserver
- kubelet
- scheduler
- flannel
tier: supporting
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 计划考 CKA 的初学者
- 在职备考者
- 培训管理者
estimated_read_time: 25min
intent_queries:
- CKA 怎么备考
- CKA 考试技巧
- Kubernetes 认证
- CKA 模拟题
trigger_keywords:
- CKA
- 认证
- 备考
- 考试
- certificate
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CKA 认证备考完全指南

> **考试全称**: Certified [[Kubernetes|Kubernetes]] Administrator (CKA)  
> **主办方**: Linux Foundation + CNCF  
> **考试费用**: $395（约 ¥2800，偶有促销）  
> **考试形式**: 线上远程监考，2 小时，17 道实操题  
> **通过标准**: 满分 100 分，66 分及格  
> **有效期**: 3 年  
> **重考政策**: 首次未通过赠送 1 次重考机会  
> **官方链接**: https://training.linuxfoundation.org/certification/certified-kubernetes-administrator-cka/

---

## 一、CKA 值不值得考？

### 适合考 CKA 的人

| 人群 | 原因 |
|------|------|
| K8s 初学者 | 以考促学，系统性覆盖核心知识点 |
| 运维/开发工程师 | 简历加分，证明实操能力 |
| 想转型云原生 | 敲门砖，面试时有话可说 |
| 团队技术负责人 | 统一团队 K8s 语言，降低沟通成本 |

### CKA vs CKAD vs CKS

| 认证 | 全称 | 侧重点 | 难度 | 适合 |
|------|------|--------|------|------|
| **CKA** | Kubernetes 管理员 | 集群管理、运维、排障 | ⭐⭐⭐ | **大多数人首选** |
| CKAD | Kubernetes 应用开发者 | 应用设计、部署、配置 | ⭐⭐⭐ | 开发者 |
| CKS | Kubernetes 安全专家 | 集群安全、加固、合规 | ⭐⭐⭐⭐ | 有 CKA 基础后考 |

**建议顺序**: CKA → CKAD/CKS（二选一或都考）

---

## 二、考纲与知识库映射

CKA 官方考纲 7 大领域，以及本知识库对应的复习资料：

| 考纲领域 | 权重 | 核心考点 | 本库对应文档 |
|---------|------|---------|-------------|
| 集群架构、安装与配置 | 25% | kubeadm 安装、升级、高可用、[[etcd|etcd]] 备份恢复 | [domain-01/06-upgrade-paths](../../集群基础/升级路径/) |
| 工作负载与调度 | 15% | Pod、Deployment、DaemonSet、Job、调度规则 | [fundamentals/02-15](../fundamentals/) |
| 服务与网络 | 20% | Service、Ingress、NetworkPolicy、CoreDNS、CNI | [domain-03/00-core-k8s-networking](../../网络/K8s网络核心/) |
| 存储 | 10% | PV、PVC、StorageClass、Volume 模式 | [fundamentals/08-pv-pvc-basics.md](../fundamentals/08-pv-pvc-basics.md) |
| 故障排查 | 30% | 节点问题、Pod 排障、网络排障、组件排障 | [故障诊断](../../故障诊断/) |

> 📌 **考试真相**: 故障排查占 30%，是最大头。也是工作中最值钱的技能。

---

## 三、4 周冲刺计划

### 前提条件

- 已完成 K8s 核心概念学习（或同时学习）
- 拥有本地实验环境（[kind/minikube](02-local-lab-environment.md) 即可）
- 每天至少 2 小时专注时间

### Week 1: 集群架构与安装（25%）

**目标**: 能手搓一个 kubeadm 集群，能升级、能备份 etcd

| 天数 | 学习内容 | 实践任务 |
|------|---------|---------|
| D1 | kubeadm 安装单节点集群 | 用 Vagrant/VM 或云服务器装一个 |
| D2 | kubeadm 高可用集群（stacked etcd） | 3 控制平面 + 2 工作节点 |
| D3 | 集群升级（小版本升级） | 1.29 → 1.30 升级实操 |
| D4 | etcd 备份与恢复 | `etcdctl snapshot save` + `restore` |
| D5 | 证书管理 | 查看证书有效期、手动轮换 |
| D6 | kubelet 配置 | 修改 kubelet 配置并重启 |
| D7 | Week 1 模拟测试 | 限时 30 分钟完成 5 道题 |

**核心命令记忆**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubeadm 初始化
kubeadm init --pod-network-cidr=10.244.0.0/16

# 加入节点
kubeadm join 192.168.1.10:6443 --token xxx --discovery-token-ca-cert-hash sha256:xxx

# etcd 备份
ETCDCTL_API=3 etcdctl snapshot save snapshot.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# etcd 恢复
ETCDCTL_API=3 etcdctl snapshot restore snapshot.db --data-dir=/var/lib/etcd-restored  # ⚠️ 覆盖 etcd 数据，集群状态回退
```
### Week 2: 工作负载与调度（15%）

**目标**: 熟练编写各种工作负载 YAML，理解调度机制

| 天数 | 学习内容 | 实践任务 |
|------|---------|---------|
| D8 | Pod 生命周期与配置 | 多容器 Pod、Init 容器、生命周期钩子 |
| D9 | Deployment 策略 | 滚动更新、回滚、暂停/恢复 |
| D10 | DaemonSet、Job、CronJob | 节点级服务、定时任务 |
| D11 | 资源限制与 QoS | requests/limits、Guaranteed/Burstable/BestEffort |
| D12 | 调度亲和性 | nodeSelector、亲和性、反亲和性、污点容忍 |
| D13 | 自定义调度器 / 优先级 | PriorityClass、抢占 |
| D14 | Week 2 模拟测试 | 限时 30 分钟完成 5 道题 |

**核心 YAML 模板**（必须能手写）:

```yaml
# Deployment + HPA
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
        resources:
          requests:
            memory: "128Mi"
            cpu: "100m"
          limits:
            memory: "256Mi"
            cpu: "200m"
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

### Week 3: 服务、网络与存储（30%）

**目标**: 理解 K8s 网络模型，能配置服务发现和存储

| 天数 | 学习内容 | 实践任务 |
|------|---------|---------|
| D15 | Service 类型与实现 | ClusterIP、NodePort、LoadBalancer、Headless |
| D16 | Ingress 与 IngressClass | 配置路径路由、TLS |
| D17 | NetworkPolicy | 限制 Pod 间通信 |
| D18 | CoreDNS 排查 | 修改 CoreDNS 配置、排查 DNS 解析失败 |
| D19 | PV / PVC / StorageClass | 静态/动态供应、访问模式 |
| D20 | Volume 类型 | emptyDir、hostPath、configMap、secret、downwardAPI |
| D21 | Week 3 模拟测试 | 限时 40 分钟完成 5 道题 |

**核心命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 快速暴露 Deployment
kubectl expose deployment web --port=80 --target-port=8080 --type=NodePort

# 测试 DNS
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup kubernetes.default

# 查看 StorageClass
kubectl get sc

# 创建 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
  storageClassName: standard
EOF
```
### Week 4: 故障排查 + 全真模拟（30%）

**目标**: 能在压力下快速定位并解决问题

| 天数 | 学习内容 | 实践任务 |
|------|---------|---------|
| D22 | Pod 排障流程 | Pending/CrashLoopBackOff/ImagePullBackOff |
| D23 | 节点排障 | NotReady、DiskPressure、MemoryPressure |
| D24 | 网络排障 | Service 不通、DNS 失败、NetworkPolicy 拦截 |
| D25 | 控制平面排障 | API Server、Scheduler、Controller Manager 问题 |
| D26 | 模拟考试 1 | Killer.sh 或本库模拟题，严格限时 2 小时 |
| D27 | 模拟考试 2 | 分析错题，针对性补强 |
| D28 | 模拟考试 3 + 预约真考 | 信心建立，预约考试 |

**故障排查决策树**:

```
# 🟢 低风险：只读/信息收集，通常无副作用
Pod 异常？
├── kubectl describe pod <name>    ← 先看 Events
├── kubectl logs <name>            ← 再看日志
├── kubectl get events --sort-by='.lastTimestamp'
│
├── STATUS: Pending?
│   ├── 资源不足？kubectl describe node
│   ├── 镜像拉不下？检查 imagePullSecrets / 网络
│   ├── 调度失败？kubectl get pods -o wide + 看 node 标签
│   └── PVC 未绑定？kubectl get pvc
│
├── STATUS: CrashLoopBackOff?
│   ├── 启动命令错误？kubectl logs --previous
│   ├── 健康检查失败？调整 livenessProbe
│   └── 资源限制太严？扩大 limits
│
└── STATUS: ImagePullBackOff?
    ├── 镜像名/标签错误？
    ├── 私有仓库未配置 imagePullSecrets？
    └── 网络不通（无法拉取外网镜像）？
```
---

## 四、高频考点速查

### 考点 1：kubeadm 集群安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 所有节点
apt-get update && apt-get install -y apt-transport-https ca-certificates curl
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.30/deb/Release.key | gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.30/deb/ /' | tee /etc/apt/sources.list.d/kubernetes.list
apt-get update
apt-get install -y kubelet kubeadm kubectl

# 2. 控制平面节点
kubeadm init --pod-network-cidr=10.244.0.0/16
mkdir -p $HOME/.kube
cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# 3. 工作节点
kubeadm join <control-plane-ip>:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash>
```
### 考点 2：etcd 备份恢复

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 备份
ETCDCTL_API=3 etcdctl snapshot save /opt/snapshot-backup.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 恢复（必须停止 kube-apiserver 和 etcd）
ETCDCTL_API=3 etcdctl snapshot restore /opt/snapshot-backup.db \
  --data-dir=/var/lib/etcd-restored
# 修改 etcd manifest 指向新 data-dir，重启
```
### 考点 3：节点维护与驱逐

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
# 标记节点不可调度
kubectl cordon <node-name>

# 驱逐节点上所有 Pod（除 DaemonSet）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 恢复调度
kubectl uncordon <node-name>
```
### 考点 4：RBAC

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: default
  name: pod-reader
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "watch", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: default
subjects:
- kind: User
  name: jane
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
```

### 考点 5：NetworkPolicy

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-allow
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: api
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
```

---

## 五、考试技巧

### 考前准备

1. **双屏/单屏**: 考试只允许一个屏幕。如果平时用双屏，提前适应单屏。
2. **浏览器**: 用 Chrome，安装考试插件（PSI 或 Examity）。
3. **身份证明**: 护照或身份证（需与报名信息一致）。
4. **环境检查**: 考试前 24 小时运行系统检查工具。
5. **房间布置**: 桌面清空，四周无书籍/纸张，手机放另一个房间。

### 考试中

1. **第一分钟**: 配置 kubectl 自动补全
   ```bash
   source <(kubectl completion bash)
   alias k=kubectl
   complete -o default -F __start_kubectl k
   ```

2. **时间管理**: 17 题，120 分钟
   - 简单题（1-2%）: 每题 2-3 分钟
   - 中等题（4-6%）: 每题 5-8 分钟
   - 难题（8-13%）: 每题 10-15 分钟
   - **策略**: 先快速浏览所有题，标记难题，先做简单题拿分

3. **善用文档**: 考试允许打开 [kubernetes.io/docs](https://kubernetes.io/docs/)（一个标签页）
   - 不要背 YAML，要背"去哪里找"
   - 善用搜索：`site:kubernetes.io pv pvc`
   - 收藏常用页面：Pod Spec、Service、Deployment、NetworkPolicy

4. **切换集群**: 每道题可能要求操作不同的集群
   ```bash
   # 按题目要求执行
   kubectl config use-context cluster1
   ```

5. **复制粘贴**: 考试提供的终端支持复制粘贴，善用文档中的示例 YAML。

6. **验证**: 每道题做完都要验证
   ```bash
   kubectl get pods
   kubectl describe pod <name>
   kubectl logs <name>
   ```

7. **不会就过**: 某题卡了 10 分钟？标记跳过，最后回来做。不要因小失大。

### 常用速查

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl edit/patch`：修改运行中的资源

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
# 快速创建 Pod（调试）
kubectl run tmp --image=nginx:alpine --rm -it --restart=Never -- /bin/sh

# 快速暴露服务
kubectl expose pod tmp --port=80 --name=tmp-service --type=NodePort

# 编辑资源（比 apply 快）
kubectl edit deployment web

# 查看资源使用
kubectl top nodes
kubectl top pods

# 导出 pod yaml（参考用）
kubectl get pod web -o yaml > web.yaml

# 强制删除（卡住的 Pod）
kubectl delete pod <name> --force --grace-period=0  # ⚠️ 跳过优雅终止，可能丢数据
```
---

## 六、模拟练习题

### 模拟题 1（2%）

**题目**: 创建一个名为 `cka-pod` 的 Pod，使用镜像 `nginx:1.25`，暴露端口 80，并设置环境变量 `ENV=production`。

<details>
<summary>参考答案</summary>

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run cka-pod --image=nginx:1.25 --port=80 --env="ENV=production"
```
或写 YAML：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: cka-pod
spec:
  containers:
  - name: nginx
    image: nginx:1.25
    ports:
    - containerPort: 80
    env:
    - name: ENV
      value: "production"
EOF
```
</details>

### 模拟题 2（4%）

**题目**: 创建一个 Deployment `web`，镜像 `nginx:1.25`，3 个副本。然后将其滚动更新到 `nginx:1.26`，并记录历史。如果更新后出现问题，回滚到上一个版本。

<details>
<summary>参考答案</summary>

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Deployment
kubectl create deployment web --image=nginx:1.25 --replicas=3 --record

# 更新（--record 已废弃，改用 annotation）
kubectl set image deployment web nginx=nginx:1.26 --record
kubectl annotate deployment web kubernetes.io/change-cause="update to 1.26"

# 查看历史
kubectl rollout history deployment web

# 回滚
kubectl rollout undo deployment web

# 验证
kubectl get pods -l app=web -o jsonpath='{range .items[*]}{.spec.containers[0].image}{"\n"}{end}'
```
</details>

### 模拟题 3（7%）

**题目**: 集群中节点 `worker-1` 需要维护。请安全地将其上的所有工作负载迁移到其他节点，维护完成后恢复。要求 DaemonSet 的 Pod 不受影响。

<details>
<summary>参考答案</summary>

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
# 标记不可调度
kubectl cordon worker-1

# 驱逐工作负载（保留 DaemonSet）
kubectl drain worker-1 --ignore-daemonsets --delete-emptydir-data

# ... 维护操作 ...

# 恢复调度
kubectl uncordon worker-1
```
</details>

### 模拟题 4（8%）

**题目**: etcd 数据目录损坏。请使用 `/opt/snapshot.db` 快照恢复到 `/var/lib/etcd-restored`，并修改静态 Pod 配置使 etcd 使用新目录。

<details>
<summary>参考答案</summary>

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 停止 etcd 和 kube-apiserver（移动 manifest 文件）
mv /etc/kubernetes/manifests/etcd.yaml /tmp/
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/

# 2. 恢复 etcd
ETCDCTL_API=3 etcdctl snapshot restore /opt/snapshot.db --data-dir=/var/lib/etcd-restored  # ⚠️ 覆盖 etcd 数据，集群状态回退

# 3. 修改 etcd manifest 中的 hostPath.data.path 指向 /var/lib/etcd-restored
vim /tmp/etcd.yaml
# 修改：
#     - hostPath:
#         path: /var/lib/etcd-restored
#         type: DirectoryOrCreate

# 4. 恢复 manifest
mv /tmp/etcd.yaml /etc/kubernetes/manifests/
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/

# 5. 验证
kubectl get nodes
```
</details>

---

## 七、推荐资源

| 资源 | 链接 | 用途 |
|------|------|------|
| 官方课程 | Linux Foundation CKA | 系统学习 + 送 2 次模拟考 |
| Killer.sh | https://killer.sh/ | 最接近真实考试的模拟环境 |
| CKA 练习题 | https://github.com/alijahnas/CKA-practice-environment | 免费练习环境 |
| 本库故障排查 | [故障诊断](../../故障诊断/) | 系统性排障知识 |
| K8s 官方文档 | https://kubernetes.io/docs/ | 考试时唯一允许打开的网站 |

---

## 八、考后建议

通过 CKA 后：

1. **把证书加到简历和 LinkedIn** — 这是看得见的产出
2. **继续考 CKAD 或 CKS** — 形成证书组合
3. **参与社区** — CNCF Slack、K8s Meetup、开源贡献
4. **实战为王** — 证书是敲门砖，生产经验才是硬通货

---

**关联文档**:
- [[技能/training-public/00-beginner-learning-roadmap.md|00 beginner learning roadmap]] — 完整学习路线图
- ../../故障诊断/ — 故障排查深度文档
- [[02-local-lab-environment]] — 本地实验环境搭建

```

<!-- risk-assessed -->
