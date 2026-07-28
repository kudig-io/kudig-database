---
title: 'Day 11: 风险点识别与防范'
description: '**学习时间**: 4-5 小时 | **主题**: 安全风险评估与最佳实践'
summary: '**学习时间**: 4-5 小时 | **主题**: 安全风险评估与最佳实践'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- docker
- ingress
- rbac
- networkpolicy
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 11: 风险点识别与防范 是什么'
- '如何 Day 11: 风险点识别与防范'
trigger_keywords:
- Day
- '11:'
- 风险点识别与防范
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 11: 风险点识别与防范
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] security risk assessment checklist
  - Pod Securityod Security Standards]] PSS configuration
  - Kubernetes security baseline hardening
  - [[networkpolicy|NetworkPolicy]] zero trust security
  - SecurityContext container hardening
trigger_keywords:
  - security risk
  - PSS
  - Pod Security Standards
  - Baseline
  - Restricted
  - NetworkPolicy
  - SecurityContext
  - privilege escalation
  - defense in depth
  - RBAC minimum privilege
reading_level: intermediate
audience:
  - ACK operators
  - Security engineers
  - SRE engineers
estimated_read_time: 45min
related_domains:
  - 安全
  - 故障诊断
  - 云厂商
related_topics:
  - pod-security-standards
  - rbac-configuration
  - network-policy
  - secret-management
---

# Day 11: 风险点识别与防范

> **学习时间**: 4-5 小时 | **主题**: 安全风险评估与最佳实践

---

## 概述

安全是 K8s 生产环境的底线。一个配置不当的集群可能面临容器逃逸、权限滥用、数据泄露等严重安全风险。今天你将学习如何系统化地识别 ACK 集群中的安全风险点，掌握 Pod Security Standards (PSS) 的配置方法，并通过实操完成一次完整的安全风险评估。

安全防护遵循纵深防御 (Defense in Depth) 原则，从基础设施层到应用层逐层加固:

```
网络层 (VPC/安全组/NetworkPolicy)
  ↓
集群层 (RBAC/审计日志/PSS)
  ↓
容器层 (SecurityContext/只读文件系统/非 root)
  ↓
应用层 (镜像扫描/Secret 加密/mTLS)
```

---

## 今日目标

- [ ] 掌握 ACK 集群常见安全风险点
- [ ] 了解安全基线与加固方法
- [ ] 能够进行集群安全风险评估
- [ ] 掌握 Pod Security Standards (PSS) 配置

---

## 核心概念

### 1. K8s 安全风险分类

| 风险类别 | 典型威胁 | 影响等级 | 防护措施 |
|----------|---------|---------|---------|
| 容器逃逸 | 特权容器、挂载 docker.sock | 高危 | PSS Baseline+ |
| 权限滥用 | default SA 权限过大、cluster-admin 泛滥 | 高危 | 最小权限 RBAC |
| 网络攻击 | Pod 间无隔离、公网暴露 | 中高危 | NetworkPolicy |
| 数据泄露 | Secret 明文存储、日志打印密码 | 中危 | etcd 加密、Vault |
| 供应链攻击 | 恶意镜像、基础镜像漏洞 | 中危 | ACR 安全扫描 |
| 拒绝服务 | 无资源限制、单 Pod 占满资源 | 中危 | ResourceQuota + LimitRange |

### 2. Pod Security Standards 三级模型

| PSS 级别 | 限制程度 | 禁止的操作 | 适用场景 |
|----------|---------|-----------|---------|
| Privileged | 无限制 | 无 | 系统组件 (CNI/CSI/日志采集) |
| Baseline | 基本安全 | 特权容器、hostPath、hostNetwork、hostPort、capabilities 添加 | 大多数业务应用 |
| Restricted | 最严格 | 以上 + 必须 runAsNonRoot、必须 drop ALL capabilities、必须 readOnlyRootFilesystem | 安全敏感应用 (金融/医疗) |

### 3. 安全审计检查项

| 检查项 | 命令 | 期望结果 |
|--------|------|---------|
| 特权容器 | `kubectl get pods -A -o json` 查询 privileged:true | 无业务 Pod 使用 |
| default SA 权限 | 检查 ClusterRoleBinding | default SA 无 cluster-admin |
| 网络策略覆盖 | `kubectl get networkpolicies -A` | 所有业务 NS 有策略 |
| Secret 加密 | 检查 etcd encryption config | 启用加密 |
| 公网暴露 | `kubectl get svc -A | grep LoadBalancer` | 仅必要服务暴露 |

---

## 理论学习 (2h)

### 必读文档

1. **Pod 安全标准**
   - 文件: `../../../安全/06-pod-security-standards.md`
   - 重点: Privileged/Baseline/Restricted 三个级别

2. **Secret 管理工具**
   - 文件: `../../../安全/11-secret-management-tools.md`
   - 重点: Secret 安全存储与轮换

---

## 实战演练 (2.5h)

### 任务 1: 风险点检查清单 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
echo "========== 安全风险检查报告 =========="
echo "检查时间: $(date)"
echo "集群版本: $(kubectl version --short 2>/dev/null | grep Server)"
echo ""

echo "--- 1. 检查 default ServiceAccount 权限 ---"
kubectl get clusterrolebindings -o json | jq '.items[] | select(.subjects[]?.name=="system:serviceaccount:default:default") | {name: .metadata.name, role: .roleRef.name}'

echo ""
echo "--- 2. 检查特权容器 ---"
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].securityContext.privileged==true) | {name: .metadata.name, namespace: .metadata.namespace}'

echo ""
echo "--- 3. 检查 Secret 类型 ---"
kubectl get secrets -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.type}{"\n"}{end}'

echo ""
echo "--- 4. 检查网络策略覆盖 ---"
kubectl get namespaces --no-headers | awk '{print $1}' | while read ns; do
  count=$(kubectl get networkpolicies -n "$ns" --no-headers 2>/dev/null | wc -l)
  echo "Namespace: $ns, NetworkPolicies: $count"
done

echo ""
echo "--- 5. 检查 Pod 安全上下文 ---"
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: runAsNonRoot={.spec.securityContext.runAsNonRoot}{"\n"}{end}'

echo ""
echo "--- 6. 检查集群公网暴露 ---"
kubectl get svc -A | grep -E 'LoadBalancer|NodePort'

echo ""
echo "--- 7. 检查 hostPath 挂载 ---"
kubectl get pods -A -o json | jq '.items[] | select(.spec.volumes[]?.hostPath != null) | {name: .metadata.name, namespace: .metadata.namespace}'

echo ""
echo "--- 8. 检查 hostNetwork 使用 ---"
kubectl get pods -A -o json | jq '.items[] | select(.spec.hostNetwork==true) | {name: .metadata.name, namespace: .metadata.namespace}'

echo ""
echo "========== 检查完毕 =========="
```
示例输出:

```
========== 安全风险检查报告 ==========
检查时间: Mon May 18 10:00:00 CST 2026
集群版本: Server Version: v1.28.9-aliyun.1

--- 1. 检查 default ServiceAccount 权限 ---
{
  "name": "default-admin-binding",
  "role": "cluster-admin"
}

--- 6. 检查集群公网暴露 ---
default       my-app-svc    LoadBalancer   10.96.0.100   47.102.xx.xx   80:31234/TCP
```

---

### 任务 2: Pod Security Standards 配置 (45min)

#### 2.1 为 Namespace 启用 PSS

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label namespace default pod-security.kubernetes.io/enforce=baseline
kubectl label namespace default pod-security.kubernetes.io/audit=baseline
kubectl label namespace default pod-security.kubernetes.io/warn=restricted

kubectl label namespace production pod-security.kubernetes.io/enforce=restricted
kubectl label namespace production pod-security.kubernetes.io/audit=restricted
kubectl label namespace production pod-security.kubernetes.io/warn=restricted

kubectl get namespaces --show-labels | grep pod-security
```
#### 2.2 测试: 创建特权 Pod (应该被拒绝)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > privileged-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: test-privileged
  namespace: default
spec:
  containers:
  - name: test
    image: nginx:alpine
    securityContext:
      privileged: true
EOF

kubectl apply -f privileged-pod.yaml
# Error from server (Forbidden): error when creating "privileged-pod.yaml":
# pods "test-privileged" is forbidden: violates PodSecurity "baseline:latest":
# privileged (container "test" must not set securityContext.privileged=true)
```
#### 2.3 测试: 创建符合 baseline 的 Pod

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > safe-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: test-safe
  namespace: default
spec:
  containers:
  - name: test
    image: nginx:alpine
    securityContext:
      runAsNonRoot: true
      runAsUser: 1000
      allowPrivilegeEscalation: false
EOF

kubectl apply -f safe-pod.yaml
# Warning: would violate PodSecurity "restricted:latest": ...
# (warn 级别不会阻止创建，仅打印警告)
```
#### 2.4 测试: 创建符合 restricted 的 Pod

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > restricted-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: test-restricted
  namespace: production
spec:
  securityContext:
    runAsNonRoot: true
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: test
    image: nginx:alpine
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      runAsUser: 1000
      capabilities:
        drop: ["ALL"]
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache/nginx
    - name: run
      mountPath: /var/run
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
  - name: run
    emptyDir: {}
EOF

kubectl apply -f restricted-pod.yaml
```
---

### 任务 3: 安全加固实践 (30min)

#### 3.1 限制 ServiceAccount Token 自动挂载

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch serviceaccount default -p '{"automountServiceAccountToken": false}'

kubectl get sa default -o yaml | grep automount
# automountServiceAccountToken: false
```
#### 3.2 创建安全的 Deployment 模板

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > secure-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
  labels:
    app: secure-app
spec:
  replicas: 2
  selector:
    matchLabels:
      app: secure-app
  template:
    metadata:
      labels:
        app: secure-app
      annotations:
        seccomp.security.alpha.kubernetes.io/pod: runtime/default
    spec:
      automountServiceAccountToken: false
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 2000
        seccompProfile:
          type: RuntimeDefault
      containers:
      - name: app
        image: nginx:alpine
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          capabilities:
            drop: ["ALL"]
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        volumeMounts:
        - name: tmp
          mountPath: /tmp
        - name: cache
          mountPath: /var/cache/nginx
        - name: run
          mountPath: /var/run
      volumes:
      - name: tmp
        emptyDir: {}
      - name: cache
        emptyDir: {}
      - name: run
        emptyDir: {}
EOF

kubectl apply -f secure-deployment.yaml
```
#### 3.3 配置 NetworkPolicy 限制流量

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > network-policy.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-all-ingress
  namespace: default
spec:
  podSelector:
    matchLabels: {}
  policyTypes:
  - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-api
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: secure-app
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 80
EOF

kubectl apply -f network-policy.yaml
```
---

### 任务 4: 风险报告编写 (30min)

```
# 安全风险评估报告模板

## 1. 集群基本信息
| 项目 | 值 |
|------|-----|
| 集群 ID | c-xxxxxxxxxxxxx |
| 集群版本 | v1.28.9-aliyun.1 |
| 集群类型 | ManagedKubernetes |
| 节点数量 | 5 |
| CNI 类型 | Terway |
| 创建时间 | 2024-01-15 |

## 2. 风险发现

### 高危
| 编号 | 风险描述 | 影响范围 | 修复建议 |
|------|---------|---------|---------|
| H-01 | default SA 绑定 cluster-admin | 整个集群 | 移除绑定，使用最小权限 |
| H-02 | 存在特权容器运行 | 容器逃逸风险 | 改用非特权模式 |
| H-03 | API Server 公网暴露且无 IP 白名单 | 集群被攻击 | 配置 IP 白名单 |

### 中危
| 编号 | 风险描述 | 影响范围 | 修复建议 |
|------|---------|---------|---------|
| M-01 | 业务 NS 未配置 NetworkPolicy | 横向移动 | 配置默认拒绝策略 |
| M-02 | Secret 仅 Base64 编码 | 数据泄露 | 启用 etcd 加密 |
| M-03 | 未启用 PSS | 不安全 Pod 可创建 | 启用 baseline 级别 |

### 低危
| 编号 | 风险描述 | 影响范围 | 修复建议 |
|------|---------|---------|---------|
| L-01 | SA Token 自动挂载 | 信息泄露 | 设置 automount=false |
| L-02 | 未配置审计日志 | 无法追溯 | 启用审计日志 |

## 3. 修复优先级

1. [P0] 移除 default SA 的 cluster-admin 权限
2. [P0] API Server 配置 IP 白名单
3. [P1] 为所有业务 NS 启用 PSS baseline
4. [P1] 配置 NetworkPolicy
5. [P2] 启用 etcd 加密
6. [P2] 启用审计日志

## 4. 持续改进
- 每月执行一次安全扫描
- 每季度更新安全基线
- 新 NS 自动打 PSS 标签
- CI/CD 集成镜像安全扫描
```

---

## 费曼复述 (0.5h)

1. **ACK 集群中最常见的安全风险点有哪些？**
2. **PSS 的三个级别分别是什么？生产环境推荐哪个？**
3. **如何编写一个安全的 Deployment 模板？关键的安全字段有哪些？**
4. **NetworkPolicy 的默认行为是什么？如何实现零信任网络？**

---

## 今日检验

- [ ] 能列出 ACK 集群常见安全风险点
- [ ] 能配置 Pod Security Standards
- [ ] 能创建安全加固的 Deployment
- [ ] 了解安全风险评估报告的编写方法

---

## 配置参考

### PSS 标签速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# enforce: 违反时拒绝创建
kubectl label namespace <ns> pod-security.kubernetes.io/enforce=<level>

# audit: 违反时记录审计事件
kubectl label namespace <ns> pod-security.kubernetes.io/audit=<level>

# warn: 违反时打印警告
kubectl label namespace <ns> pod-security.kubernetes.io/warn=<level>

# 查看所有 NS 的 PSS 配置
kubectl get ns --show-labels | grep pod-security
```
### SecurityContext 完整配置参考

```yaml
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
```

---

## 常见问题

### Q1: PSS 和 PSP 有什么区别？


PSP (PodSecurityPolicy) 在 K8s 1.25 中已被移除，PSS (Pod Security Standards) 是其替代方案。PSS 通过 Namespace 标签实现，配置更简单，无需创建额外的 API 资源。

### Q2: 如何在不阻断业务的情况下启用 PSS？

建议使用 `warn` 模式先观察哪些 Pod 不合规，修复后再切换到 `enforce` 模式:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label namespace <ns> pod-security.kubernetes.io/warn=baseline
# 观察一段时间后
kubectl label namespace <ns> pod-security.kubernetes.io/enforce=baseline --overwrite
```
### Q3: readOnlyRootFilesystem 导致应用写入失败怎么办？

使用 emptyDir 卷挂载需要写入的目录:

```yaml
volumes:
- name: tmp
  emptyDir: {}
containers:
- volumeMounts:
  - name: tmp
    mountPath: /tmp
```

---

## 要点总结

| 安全层级 | 关键措施 | 优先级 |
|----------|---------|--------|
| 网络层 | NetworkPolicy、VPC 安全组 | P1 |
| 集群层 | RBAC 最小权限、审计日志 | P0 |
| Pod 层 | PSS Baseline/Restricted | P0 |
| 容器层 | 非 root、只读文件系统、drop capabilities | P1 |
| 应用层 | 镜像扫描、Secret 加密 | P2 |

---

## 明日预告

Day 12 将学习集群审计日志的配置与分析方法。

---

## 延伸阅读

- [Pod 安全标准](../../../%E5%AE%89%E5%85%A8/06-pod-security-standards.md)
- [Secret 管理工具](../../../../../../08-%E5%AE%89%E5%85%A8/01-%E8%BA%AB%E4%BB%BD%E4%B8%8E%E8%AE%BF%E9%97%AE/11-secret-management-tools.md)
- [RBAC 矩阵配置](../../../../../../08-%E5%AE%89%E5%85%A8/01-%E8%BA%AB%E4%BB%BD%E4%B8%8E%E8%AE%BF%E9%97%AE/07-rbac-matrix-configuration.md)
- [认证授权系统](../../../../../../08-%E5%AE%89%E5%85%A8/01-%E8%BA%AB%E4%BB%BD%E4%B8%8E%E8%AE%BF%E9%97%AE/01-authentication-authorization-system.md)
- [NetworkPolicy 实践](../../domain-6-networking/12-network-policy-practice.md)

```

<!-- risk-assessed -->
