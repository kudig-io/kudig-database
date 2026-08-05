---
title: 'Day 14: K8S 集群配额 & License'
description: '- "ACK配额管理"'
summary: '在多团队共享集群的场景下，资源配额管理是保障公平性和稳定性的关键机制。今天你将学习 K8s 原生的 ResourceQuota 和 LimitRange 机制，理解 ACK 集群级别的配额限制，并掌握多团队资源配额方案的设计方法。'
category: learning
tags:
- k8s
- training
- hands-on
- statefulset
- daemonset
- job
- cronjob
- rbac
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
- 'Day 14: K8S 集群配额 & License 是什么'
- '如何 Day 14: K8S 集群配额 & License'
trigger_keywords:
- Day
- '14:'
- K8S
- 集群配额
- License
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 14: K8S 集群配额 & License

```yaml
---
title: Day 14: 集群配额与License
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes资源配额"
  - "ResourceQuota"
  - "LimitRange"
  - "QoS等级"
  - "ACK配额管理"
trigger_keywords:
  - "ResourceQuota"
  - "LimitRange"
  - "配额"
  - "QoS"
  - "资源限制"
  - "容器资源"
  - "requests"
  - "limits"
  - "集群配额"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - 工作负载
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring/checkpoint
  - 工作负载/23-resource-management
  - 故障诊断/24-quota-limitrange-troubleshooting
id: WEEK2-DAY14
topic: training
type: hands-on
tags: [week-2, day-14, quota, resource, limitrange, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 资源配额管理与许可证相关

---

## 概述

在多团队共享集群的场景下，资源配额管理是保障公平性和稳定性的关键机制。今天你将学习 K8s 原生的 ResourceQuota 和 LimitRange 机制，理解 ACK 集群级别的配额限制，并掌握多团队资源配额方案的设计方法。

---

## 今日目标

- [ ] 掌握 ResourceQuota 和 LimitRange 配置
- [ ] 理解 ACK 集群配额限制
- [ ] 了解 License 与集群规模的关系
- [ ] 能够设计合理的资源配额方案

---

## 核心概念

### 1. 资源配额三层模型

```
层级 1: 集群级配额 (ACK 限制)
  ├── 节点数量上限 (托管版默认 500)
  ├── Pod 总数上限 (与 CIDR 相关)
  ├── Service 总数上限 (与 Service CIDR 相关)
  └── 每节点最大 Pod 数 (默认 110)

层级 2: Namespace 级配额 (ResourceQuota)
  ├── CPU/Memory 请求和限制总量
  ├── Pod/Service/PVC 数量上限
  └── ConfigMap/Secret 数量上限

层级 3: 容器级限制 (LimitRange)
  ├── 默认 CPU/Memory 请求和限制
  ├── 单容器 CPU/Memory 最小/最大值
  └── 单 Pod CPU/Memory 最大值
```

### 2. QoS 等级

| QoS 等级 | 条件 | 驱逐优先级 | cgroup 设置 |
|----------|------|-----------|------------|
| Guaranteed | requests == limits (所有容器) | 最低 (最后被驱逐) | cpuShares=1024*cores |
| Burstable | 至少设置了 requests 或 limits | 中等 | cpuShares=1024*(requests/1) |
| BestEffort | 未设置 requests 和 limits | 最高 (最先被驱逐) | cpuShares=2 |

### 3. ResourceQuota vs LimitRange

| 特性 | ResourceQuota | LimitRange |
|------|--------------|------------|
| 作用范围 | Namespace 总量 | 单个 Pod/Container |
| 限制对象 | 资源总量 | 默认值/最小值/最大值 |
| 强制方式 | 创建时校验 | 自动注入/拒绝超限 |
| 配置位置 | Namespace 级 | Namespace 级 |

---

## 理论学习 (2h)

### 必读文档

1. **资源管理**
   - 文件: `../../../工作负载/23-resource-management.md`
   - 重点: requests/limits、QoS 等级

2. **配额排障**
   - 文件: `../../../故障诊断/24-quota-limitrange-troubleshooting.md`
   - 重点: 配额相关的常见问题

---

## 实战演练 (2.5h)

### 任务 1: ResourceQuota 配置 (45min)

#### 1.1 创建 ResourceQuota

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace quota-test

cat > resource-quota.yaml << 'EOF'
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: quota-test
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    pods: "20"
    services: "10"
    persistentvolumeclaims: "5"
    configmaps: "20"
    secrets: "20"
    replicationcontrollers: "5"
    resourcequotas: "1"
    count/deployments.apps: "5"
    count/statefulsets.apps: "3"
    count/jobs.batch: "10"
    count/cronjobs.batch: "5"
EOF

kubectl apply -f resource-quota.yaml
```
#### 1.2 查看配额使用情况

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe resourcequota team-quota -n quota-test
```
示例输出:

```
Name:            team-quota
Namespace:       quota-test
Resource         Used   Hard
--------         ----   ----
configmaps       0      20
limits.cpu       0      8
limits.memory    0      16Gi
pods             0      20
requests.cpu     0      4
requests.memory  0      8Gi
secrets          0      20
services         0      10
```

#### 1.3 测试配额限制

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Deployment 消耗资源
cat > quota-test-deploy.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: quota-test-app
  namespace: quota-test
spec:
  replicas: 3
  selector:
    matchLabels:
      app: test
  template:
    metadata:
      labels:
        app: test
    spec:
      containers:
      - name: nginx
        image: nginx:alpine
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "1"
            memory: 1Gi
EOF

kubectl apply -f quota-test-deploy.yaml

# 再次查看配额
kubectl describe resourcequota team-quota -n quota-test
# requests.cpu: 1500m / 4
# requests.memory: 1536Mi / 8Gi
# pods: 3 / 20
```
---

### 任务 2: LimitRange 配置 (45min)

#### 2.1 创建 LimitRange

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > limit-range.yaml << 'EOF'
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: quota-test
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "2"
      memory: 4Gi
    min:
      cpu: 50m
      memory: 64Mi
    maxLimitRequestRatio:
      cpu: "4"
      memory: "4"
  - type: Pod
    max:
      cpu: "4"
      memory: 8Gi
  - type: PersistentVolumeClaim
    max:
      storage: 50Gi
    min:
      storage: 1Gi
EOF

kubectl apply -f limit-range.yaml

# 查看 LimitRange
kubectl describe limitrange default-limits -n quota-test
```
示例输出:

```
Name:       default-limits
Namespace:  quota-test
Type        Resource  Min   Max  Default Request  Default Limit  Max Limit/Request Ratio
----        --------  ---   ---  ---------------  -------------  -----------------------
Container   cpu       50m   2    100m             200m           4
Container   memory    64Mi  4Gi  128Mi            256Mi          4
Pod         cpu       -     4    -                -              -
Pod         memory    -     8Gi  -                -              -
PVC         storage   1Gi   50Gi -                -              -
```

#### 2.2 测试默认值注入

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建不带 resources 的 Pod
kubectl run test-limit --image=nginx:alpine -n quota-test

# 查看自动注入的资源限制
kubectl get pod test-limit -n quota-test -o yaml | grep -A 10 resources
# resources:
#   limits:
#     cpu: 200m
#     memory: 256Mi
#   requests:
#     cpu: 100m
#     memory: 128Mi
```
#### 2.3 测试超限拒绝

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建超出最大限制的 Pod
cat > over-limit-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: over-limit
  namespace: quota-test
spec:
  containers:
  - name: test
    image: nginx:alpine
    resources:
      requests:
        cpu: "3"
        memory: 8Gi
EOF

kubectl apply -f over-limit-pod.yaml
# Error from server (Forbidden): error when creating "over-limit-pod.yaml":
# pods "over-limit" is forbidden: [maximum cpu per Pod is 4, but request is 3
# maximum memory per Container is 4Gi, but request is 8Gi]
```
---

### 任务 3: ACK 集群配额检查 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
echo "========== ACK 集群配额检查 =========="

echo "--- 1. 节点 Pod 容量 ---"
kubectl get nodes -o custom-columns='NAME:.metadata.name,CAPACITY:.status.capacity.pods,ALLOCATABLE:.status.allocatable.pods'

echo ""
echo "--- 2. 当前 Pod 数量 ---"
kubectl get pods -A --no-headers | wc -l

echo ""
echo "--- 3. 节点资源使用 ---"
kubectl top nodes 2>/dev/null || echo "需要 metrics-server"

echo ""
echo "--- 4. Namespace 配额 ---"
kubectl get resourcequota -A

echo ""
echo "--- 5. LimitRange ---"
kubectl get limitrange -A

echo ""
echo "--- 6. Service CIDR 容量 ---"
kubectl cluster-info dump 2>/dev/null | grep service-cluster-ip-range || echo "通过 kubectl cluster-info dump 查看"

echo ""
echo "========== 检查完毕 =========="
```
ACK 集群级配额参考:

| 配额项 | 托管版默认 | 说明 |
|--------|----------|------|
| 节点数量 | 500 | 可申请提升 |
| 每节点 Pod 数 | 110 | Terway 模式与 ENI 数量相关 |
| Service 数量 | 与 Service CIDR 相关 | /16 可支持约 65000 |
| PVC 数量 | 无硬限制 | 受存储后端限制 |
| SecurityGroup 规则 | 500 | 可申请提升 |

---

### 任务 4: 多团队配额方案设计 (30min)

#### 4.1 场景: 3 个团队共享一个集群

```
集群总资源: 12C 48G (3 节点 ecs.g6.xlarge)
分配方案:
  - 开发团队: 4C 16G (quota-dev)
  - 测试团队: 4C 16G (quota-test)
  - 生产团队: 4C 16G (预留，quota-prod)
```

#### 4.2 批量创建配额

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

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
# 创建团队 Namespace + ResourceQuota + LimitRange
for team in dev test prod; do
  # 创建 Namespace
  kubectl create namespace team-$team 2>/dev/null

  # 打标签
  kubectl label namespace team-$team team=$team --overwrite

  # 创建 ResourceQuota
  cat > team-${team}-quota.yaml << EOF
apiVersion: v1
kind: ResourceQuota
metadata:
  name: ${team}-quota
  namespace: team-${team}
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 16Gi
    limits.cpu: "4"
    limits.memory: 16Gi
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
    configmaps: "30"
    secrets: "30"
EOF

  kubectl apply -f team-${team}-quota.yaml

  # 创建 LimitRange
  cat > team-${team}-limits.yaml << EOF
apiVersion: v1
kind: LimitRange
metadata:
  name: ${team}-limits
  namespace: team-${team}
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "2"
      memory: 4Gi
    min:
      cpu: 50m
      memory: 64Mi
EOF

  kubectl apply -f team-${team}-limits.yaml
done

# 验证各团队配额
for team in dev test prod; do
  echo "=== team-$team ==="
  kubectl describe resourcequota -n team-$team | grep -E "Name:|Resource:|Used|Hard"
  echo ""
done

# 清理
# kubectl delete namespace quota-test team-dev team-test team-prod  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
---

## 费曼复述 (0.5h)

1. **ResourceQuota 和 LimitRange 的区别是什么？**
2. **为什么多团队场景必须配置资源配额？**
3. **ACK 集群有哪些级别的配额限制？**
4. **QoS 等级如何影响 Pod 驱逐顺序？**

---

## 今日检验

- [ ] 能创建 ResourceQuota 和 LimitRange
- [ ] 理解 requests 和 limits 的关系
- [ ] 了解 ACK 集群级配额限制
- [ ] 能设计多团队资源配额方案

---

## 配置参考

### ResourceQuota 完整配置

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: full-quota
  namespace: my-team
spec:
  hard:
    # 计算资源
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    # 对象数量
    pods: "20"
    services: "10"
    persistentvolumeclaims: "5"
    configmaps: "20"
    secrets: "20"
    replicationcontrollers: "5"
    # 工作负载数量
    count/deployments.apps: "5"
    count/statefulsets.apps: "3"
    count/daemonsets.apps: "2"
    count/jobs.batch: "10"
    count/cronjobs.batch: "5"
    # 存储
    requests.storage: 100Gi
    persistentvolumeclaims: "10"
```

### LimitRange 完整配置

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: full-limits
  namespace: my-team
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "2"
      memory: 4Gi
    min:
      cpu: 50m
      memory: 64Mi
  - type: Pod
    max:
      cpu: "4"
      memory: 8Gi
  - type: PersistentVolumeClaim
    max:
      storage: 50Gi
    min:
      storage: 1Gi
```

---

## 常见问题

### Q1: Pod 创建报 "exceeded quota" 怎么办？

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Namespace 配额使用
kubectl describe resourcequota -n <namespace>

# 常见原因:
# - CPU/Memory 配额已满
# - Pod 数量已达上限
# - PVC 数量已达上限
```
### Q2: LimitRange 的 default 和 defaultRequest 有什么区别？

- `default`: 容器未设置 limits 时自动注入的 limits 值
- `defaultRequest`: 容器未设置 requests 时自动注入的 requests 值

### Q3: 如何查看 Pod 的 QoS 等级？

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <name> -o jsonpath='{.status.qosClass}'
```
---

## 要点总结

| 资源 | 作用 | 生产建议 |
|------|------|---------|
| ResourceQuota | 限制 Namespace 资源总量 | 每个业务 Namespace 必配 |
| LimitRange | 限制单个 Pod/Container 资源 | 设置合理的默认值和上限 |
| QoS | 资源驱逐优先级 | Guaranteed > Burstable > BestEffort |

---

## 本周总结

恭喜完成 Week 2 的全部学习! 本周你应该已经掌握:
- RBAC 权限模型与配置
- RAM 账号与 ACK 集成
- 安全漏洞识别与风险防范
- 审计日志配置与分析
- 监控体系搭建与告警配置
- 资源配额管理

请完成 checkpoint.md](./checkpoint.md) 自测和 [P2 实操项目](../projects/p2-security-monitoring-setup.md)。

---

## 延伸阅读

- [资源管理](../../../../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/23-resource-management.md)
- [配额排障](../../../../../../19-故障诊断/02-资源排障/16-quota-limitrange-troubleshooting.md)
- [K8s Resource Quota 文档](https://[[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]].io/docs/concepts/policy/resource-quotas/)
- [K8s Limit Range 文档](https://kubernetes.io/docs/concepts/policy/limit-range/)


<!-- risk-assessed -->
