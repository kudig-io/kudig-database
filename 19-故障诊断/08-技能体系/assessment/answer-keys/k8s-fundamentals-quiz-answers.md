---
title: K8s 基础知识考核 - 答案解析
description: '# K8s 基础知识考核 - 答案解析'
summary: 'kubectl get pods -n <namespace> | grep CrashLoopBackOff'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- scheduler
- envoy
- containerd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- K8s 基础知识考核 - 答案解析 是什么
- 如何 K8s 基础知识考核 - 答案解析
trigger_keywords:
- K8s
- 基础知识考核
- 答案解析
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
- mysql-basics
- gpu-scheduling-basics
- policy-basics
skill_id: SKILL-K8S_FUNDAMENTALS_QUIZ_ANSWERS-001
skill_name: K8s 基础知识考核 - 答案解析
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s8s 基础知识考核|K8s 基础知识考核]] - 答案解析

> **版本**: K8s 1.28-1.33 | **仅内部使用**

---

## 一、选择题答案

| 题号 | 答案 | 解析 |
|------|------|------|
| 1 | D | Docker 是容器运行时，不是 K8s 核心组件。K8s 支持 [[containerd|containerd]]/cri-o 等多种运行时。 |
| 2 | D | `Pending` 状态不可能是 "Pod 已经 Running"。Pod 不会在 Running 状态时又回到 Pending。 |
| 3 | A | Pod 创建后会经过 Pending → ContainerCreating（镜像拉取/容器创建）→ Running。 |
| 4 | C | ClusterIP 是集群内部的虚拟 IP，仅在集群内可达。 |
| 5 | C | `kubectl top [[Pods|pods]]` 查看资源使用，前提是 metrics-server 正常运行。 |
| 6 | B | PDB（PodDisruptionBudget）可以限制同时不可用的 Pod 数量，实现维护期间的高可用。 |
| 7 | D | 修改 Deployment 的探针配置会应用到新创建的 Pod，Deployment 是声明式管理的推荐方式。 |
| 8 | A | `nodeSelector` 用于将 Pod 调度到特定标签的节点。GPU 节点通常有 `nvidia.com/gpu=true` 标签。 |
| 9 | A | 集群证书位于 `/etc/kubernetes/pki` 目录，包括 apiserver、etcd、kubelet 等证书。 |
| 10 | E | `curl -sk https://localhost:6443/healthz` 可以在控制平面节点检查；`kubectl cluster-info` 也可以检查。 |
| 11 | C | etcd 默认监听 2379（客户端通信）和 2380（节点间通信）。6443 是 API Server 端口。 |
| 12 | D | 在容器内可以使用 `ps aux`；在节点上可以使用 `crictl ps` 查看容器进程。 |
| 13 | A | Role 作用于单个 namespace，ClusterRole 作用于整个集群。 |
| 14 | B | 扩缩容后新 Pod 的调度仍然由 kube-scheduler 决定，基于资源情况和调度策略。 |
| 15 | A | ReadWriteOnce 表示单节点读写。多节点读写是 ReadWriteMany。 |
| 16 | B | Init Container 在主容器启动前执行，常用于等待依赖服务就绪或执行初始化脚本。 |
| 17 | E | `kubectl describe pod` 可以看到 Pod 的 Events；`kubectl get events` 可以看到所有事件。 |
| 18 | C | kube-proxy 负责将 Service 的流量代理到后端 Endpoints（Pod）。 |
| 19 | B | 当节点资源不足时，kubelet 会按照 QoS 优先级驱逐 Pod，BestEffort 优先级最低。 |
| 20 | A | `--dry-run` 是预览 YAML 不实际执行的标准参数。`--dry-run=client` 是本地预览。 |

**总分**: 40 分

---

## 二、简答题答案

### 1. Pod 生命周期（10 分）

**参考答案**：

```
Pod 创建后经历以下阶段：
1. Pending
   - API Server 接收请求并创建 Pod 对象
   - Scheduler 计算最优节点
   - 如无可用节点，保持 Pending

2. Initializing（如有 Init Container）
   - Init Container 按顺序执行
   - 全部成功后才进入主容器阶段

3. ContainerCreating
   - 镜像拉取（ImagePullBackOff 如失败）
   - 容器创建（CreateContainerError 如失败）
   - 配置网络（Pod IP 分配）

4. Running
   - 所有容器启动
   - 探针检查通过（liveness/readiness）
   - Pod 可以接收流量

5. Succeeded（一次性任务正常终止）
   - 所有容器成功退出

6. Failed（不可恢复的错误）
   - 容器异常退出（非重启）
   - 调度失败

7. Unknown
   - 节点不可达
   - kubelet 无法上报状态
```

**评分标准**：
- 完整描述各阶段（6 分）
- 触发条件和转换条件（2 分）
- 异常状态说明（2 分）

---

### 2. CrashLoopBackOff 排查（10 分）

**参考答案**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 排查步骤：

# 1. 确认 Pod 状态
kubectl get pods -n <namespace> | grep CrashLoopBackOff

# 2. 查看重启次数和退出码
kubectl describe pod <pod-name> | grep -E "Restart Count|Exit Code|Last State"

# 3. 查看容器日志（关键！）
kubectl logs <pod-name> --previous
# 或多容器场景
kubectl logs <pod-name> -c <container-name> --previous

# 4. 可能原因：

# a) 应用配置错误
# - 环境变量缺失
# - 配置文件路径错误
# - 连接字符串错误
kubectl exec -it <pod-name> -- env | grep -E "DATABASE|API|HOST"

# b) 资源不足（OOMKilled）
# - 退出码 137（SIGKILL）
# - 内存 limit 过低
kubectl top pods -n <namespace>

# c) 依赖服务不可达
# - 应用启动需要连接数据库/缓存
# - 连接超时
kubectl exec -it <pod-name> -- nc -zv <service> <port>

# d) 镜像问题
# - 镜像不存在
# - 镜像拉取超时
kubectl describe pod | grep -A5 "ImagePull"

# e) 权限问题
# - Secret/ConfigMap 访问被拒绝
# - RBAC 限制
```
**评分标准**：
- 正确使用 kubectl 命令（4 分）
- 列出所有可能原因（4 分）
- 排查逻辑清晰（2 分）

---

### 3. 有状态 MySQL 部署（10 分）

**参考答案**：

```yaml
# 需要使用以下 K8s 资源：

# 1. StatefulSet（而非 Deployment）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql-headless  # 配合 Headless Service
  replicas: 1
  selector:
    matchLabels:
      app: mysql
  template:
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: password
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:  # PVC 模板
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: standard
        resources:
          requests:
            storage: 20Gi

# 2. Headless Service（稳定网络标识）
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
spec:
  clusterIP: None  # Headless
  selector:
    app: mysql
  ports:
    - port: 3306

# 3. Secret（敏感信息）
apiVersion: v1
kind: Secret
metadata:
  name: mysql-secret
type: Opaque
stringData:
  password: changeme123

# 4. PVC（持久化存储）
# 通过 volumeClaimTemplates 自动创建

# 5. ConfigMap（可选，非敏感配置）
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-config
data:
  my.cnf: |
    [mysqld]
    max_connections=200
```

**各资源作用**：
- StatefulSet：管理有状态 Pod，提供稳定的网络标识（mysql-0.mysql-headless）和存储（PVC）
- Headless Service：提供稳定的网络主机名，让应用通过 DNS 发现 Pod
- Secret：安全存储数据库密码
- PVC：持久化存储，确保数据在 Pod 重启后不丢失

**评分标准**：
- StatefulSet 用于有状态应用（3 分）
- Headless Service 提供稳定网络标识（2 分）
- Secret 管理敏感信息（2 分）
- PVC 持久化存储（2 分）
- 整体配置合理（1 分）

---

### 4. Ingress 解释（10 分）

**参考答案**：

**Ingress 定义**：
- Ingress 是 K8s 的一种资源对象，定义了 HTTP/HTTPS 路由规则
- 通过 Ingress 将外部请求路由到集群内的 Service

**Ingress Controller 作用**：
- Ingress 只是一个规则定义，需要 Ingress Controller 来实际处理请求
- 常见的 Ingress Controller：Nginx、Traefik、[[envoy|Envoy]]、Cloud Load Balancer
- Controller 监听 Ingress 资源变化，将规则转换为具体的负载均衡配置

**返回 404 的可能原因**：

1. **Ingress Controller 未部署或未就绪**
   ```bash
   kubectl get pods -n ingress-nginx  # 检查是否有 Ingress Controller Pod
   ```

2. **Ingress 规则的 backend 配置错误**
   ```yaml
   # 检查 backend service 和 port 是否正确
   backend:
     service:
       name: web-svc
       port:
         number: 80  # 确认端口正确
   ```

3. **Ingress Class 未指定或不存在**
   ```bash
   # 检查是否有匹配的 IngressClass
   kubectl get ingressclass
   # 在 Ingress 中指定: spec.ingressClassName: nginx
   ```

4. **Service 没有 Endpoints（Pod 未运行）**
   ```bash
   kubectl get endpoints <backend-svc>  # 确认有 Endpoints
   ```

5. **路径匹配问题**
   ```yaml
   # pathType 必须是 Prefix / Exact / ImplementationSpecific
   # 路径 /api 不会匹配 /api/v1
   ```

6. **DNS 解析问题**
   ```bash
   # 确认 Ingress 绑定的域名正确解析到 Ingress Controller 的 IP
   nslookup api.example.com
   ```

**评分标准**：
- 正确解释 Ingress（2 分）
- 正确解释 Ingress Controller（2 分）
- 列出至少 3 个 404 原因（6 分）

---

## 三、实操题答案（20 分）

**参考答案**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 排查步骤 ==========

# 1. 确认问题（2 分）
# a) 确认 Pod 状态
kubectl get pods -n production -l app=web-backend
# 预期：所有 Pod Running（题目已告知 Running）

# b) 确认 Service 状态
kubectl get svc -n production -l app=web-backend
# 预期：Service 存在且类型正确

# 2. 收集信息（4 分）
# a) 查看 Ingress 配置（如有）
kubectl get ingress -n production
kubectl describe ingress web-backend -n production

# b) 查看 Service Endpoints（关键！）
kubectl get endpoints web-backend -n production
# 如果 Endpoints 为空，说明没有 Pod 匹配 Service selector

# c) 查看 Pod 和 Service 的 selector 匹配
kubectl get pods -n production -l app=web-backend --show-labels
kubectl describe svc web-backend -n production | grep -A5 Selector

# d) 检查 Ingress Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50

# 3. 定位根因（6 分）

# 情况 A：Endpoints 为空
# 根因：Service selector 与 Pod 标签不匹配
# 解决：检查并修正 Deployment 的 labels 或 Service 的 selector

# 情况 B：Ingress 配置错误
# 根因：Ingress backend 配置指向了错误的 Service 或端口
# 解决：修正 Ingress 的 backend 配置

# 情况 C：Ingress Controller 问题
# 根因：Ingress Controller 未正确路由
# 解决：重启 Ingress Controller 或检查配置

# 情况 D：Pod 就绪探针失败
# 根因：Pod Running 但 readinessProbe 失败，导致 Endpoints 为空
# 解决：检查 readinessProbe 配置和应用的 /health 端点

# 4. 修复并验证（8 分）

# 修复（以情况 A 为例）
# 1) 查看当前 Deployment 标签
kubectl get deployment web-backend -n production -o jsonpath='{.spec.selector.matchLabels}'

# 2) 修正 Deployment 标签（如果需要）
kubectl label deployment web-backend -n production app=web-backend --overwrite

# 3) 重启 Deployment 让标签生效
kubectl rollout restart deployment web-backend -n production

# 4) 验证 Endpoints
kubectl get endpoints web-backend -n production
# 预期：有 IP:Port 列表

# 5) 验证外部访问
curl -H "Host: api.example.com" http://<ingress-ip>/health

# 6) 验证 Ingress 状态
kubectl describe ingress web-backend -n production
# 预期：显示正确的 backend 配置
```
**评分标准**：
- 确认问题步骤完整（2 分）
- 收集信息命令正确（4 分）
- 定位根因方法合理（6 分）
- 修复和验证逻辑完整（8 分）

---

```yaml
---
id: ASSESSMENT-ANSWERS-001
topic: assessment
type: answer-key
tags: [assessment, answer-key, k8s-fundamentals, sre, ops-engineer, k8s-1.28-1.33]
intent_queries:
  - "K8s 考核答案"
  - "选择题答案解析"
  - "简答题答案"
difficulty: intermediate
target_roles: [sre, ops-engineer]
related:
  - 19-故障诊断/08-技能体系/assessment/k8s-fundamentals-quiz.md
  - 19-故障诊断/08-技能体系/assessment/troubleshooting-lab-exam.md
---
```
```

<!-- risk-assessed -->
