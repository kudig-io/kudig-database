# Day 11: 风险点识别与防范

> **学习时间**: 4-5 小时 | **主题**: 安全风险评估与最佳实践

---

## 今日目标

- [ ] 掌握 ACK 集群常见安全风险点
- [ ] 了解安全基线与加固方法
- [ ] 能够进行集群安全风险评估
- [ ] 掌握 Pod Security Standards (PSS) 配置

---

## 理论学习 (2h)

### 必读文档

1. **Pod 安全标准**
   - 文件: `../../../domain-7-security/06-pod-security-standards.md`
   - 重点: Privileged/Baseline/Restricted 三个级别

2. **Secret 管理工具**
   - 文件: `../../../domain-7-security/11-secret-management-tools.md`
   - 重点: Secret 安全存储与轮换

---

## 实践任务 (2.5h)

### 任务 1: 风险点检查清单 (45min)

```bash
# 1. 检查 default ServiceAccount 权限
kubectl get clusterrolebindings -o json | jq '.items[] | select(.subjects[]?.name=="system:serviceaccount:default:default")'

# 2. 检查 Secret 是否明文存储
kubectl get secrets -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.type}{"\n"}{end}'

# 3. 检查网络策略覆盖
kubectl get networkpolicies -A

# 4. 检查 Pod 安全上下文
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: runAsNonRoot={.spec.securityContext.runAsNonRoot}{"\n"}{end}'

# 5. 检查集群公网暴露
kubectl get svc -A | grep -E 'LoadBalancer|NodePort'
```

### 任务 2: Pod Security Standards 配置 (45min)

```bash
# 为 Namespace 启用 PSS
kubectl label namespace default pod-security.kubernetes.io/enforce=baseline
kubectl label namespace default pod-security.kubernetes.io/warn=restricted

# 测试: 创建特权 Pod (应该被拒绝)
cat > privileged-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: test-privileged
spec:
  containers:
  - name: test
    image: nginx:alpine
    securityContext:
      privileged: true
EOF
kubectl apply -f privileged-pod.yaml  # 应该被拒绝

# 创建符合 baseline 的 Pod
cat > safe-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: test-safe
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
```

### 任务 3: 安全加固实践 (30min)

```bash
# 1. 限制 ServiceAccount token 自动挂载
kubectl patch serviceaccount default -p '{"automountServiceAccountToken": false}'

# 2. 创建安全的 Deployment 模板
cat > secure-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
spec:
  replicas: 2
  selector:
    matchLabels:
      app: secure-app
  template:
    metadata:
      labels:
        app: secure-app
    spec:
      automountServiceAccountToken: false
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 2000
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
EOF
kubectl apply -f secure-deployment.yaml
```

### 任务 4: 风险报告编写 (30min)

```
# 安全风险评估报告模板:

# 1. 集群基本信息
#    - 集群 ID、版本、节点数
#    - CNI 类型、集群类型

# 2. 风险发现
#    - 高危: 特权容器、公网暴露的 API Server
#    - 中危: 缺少网络策略、Secret 未加密
#    - 低危: 未配置 PSS、ServiceAccount token 自动挂载

# 3. 修复建议
#    - 优先级排序
#    - 具体操作步骤
#    - 预期影响评估

# 4. 持续改进
#    - 定期安全扫描计划
#    - 安全基线维护
```

---

## 费曼复述 (0.5h)

1. **ACK 集群中最常见的安全风险点有哪些？**
2. **PSS 的三个级别分别是什么？生产环境推荐哪个？**
3. **如何编写一个安全的 Deployment 模板？**

---

## 今日检验

- [ ] 能列出 ACK 集群常见安全风险点
- [ ] 能配置 Pod Security Standards
- [ ] 能创建安全加固的 Deployment
- [ ] 了解安全风险评估报告的编写方法

---

## 核心概念总结

| PSS 级别 | 限制程度 | 适用场景 |
|----------|---------|---------|
| Privileged | 无限制 | 系统组件 |
| Baseline | 基本安全 | 大多数业务应用 |
| Restricted | 最严格 | 安全敏感应用 |

---

## 明日预告

Day 12 将学习集群审计日志的配置与分析方法。
