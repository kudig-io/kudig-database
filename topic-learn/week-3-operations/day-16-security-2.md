# Day 16: 安全体系 - Pod 安全 + 密钥管理

> **学习时间**: 4-5 小时 | **主题**: Pod 安全标准与 Secret 管理

---

## 今日目标

- [ ] 理解 Pod Security Standards (Restricted/Baseline/Privileged)
- [ ] 掌握 Secret 管理最佳实践
- [ ] 了解 Kyverno/OPA 策略引擎

---

## 理论学习 (2h)

### 必读文档

1. **Pod 安全标准**
   - 文件: `../../domain-7-security/06-pod-security-standards.md`
   - 重点: Restricted/Baseline/Privileged 三个级别

2. **Secret 管理工具**
   - 文件: `../../domain-7-security/11-secret-management-tools.md`
   - 重点: Secret 最佳实践，外部 Secret 管理

3. **策略引擎**
   - 文件: `../../domain-7-security/14-policy-engines-opa-kyverno.md`
   - 重点: Kyverno 策略配置

---

## 实践任务 (2.5h)

### 任务 1: Pod SecurityContext 配置 (45min)

```bash
# 非 root 用户运行
cat > secure-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
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
        drop:
        - ALL
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

kubectl apply -f secure-pod.yaml
kubectl exec secure-pod -- id
```

### 任务 2: Secret 管理 (45min)

```bash
# 创建 Secret
kubectl create secret generic app-secret \
  --from-literal=username=admin \
  --from-literal=password=secretpass

# 查看 Secret (Base64 编码)
kubectl get secret app-secret -o yaml

# 在 Pod 中使用 Secret
cat > secret-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: secret-test
spec:
  containers:
  - name: app
    image: nginx:alpine
    env:
    - name: DB_USER
      valueFrom:
        secretKeyRef:
          name: app-secret
          key: username
    - name: DB_PASS
      valueFrom:
        secretKeyRef:
          name: app-secret
          key: password
    volumeMounts:
    - name: secret-volume
      mountPath: /etc/secrets
      readOnly: true
  volumes:
  - name: secret-volume
    secret:
      secretName: app-secret
EOF

kubectl apply -f secret-pod.yaml
kubectl exec secret-test -- env | grep DB_
kubectl exec secret-test -- cat /etc/secrets/username
```

### 任务 3: Kyverno 策略实践 (45min)

```bash
# 安装 Kyverno
kubectl create -f https://github.com/kyverno/kyverno/releases/download/v1.10.0/install.yaml

# 等待就绪
kubectl wait --namespace kyverno --for=condition=ready pod -l app.kubernetes.io/name=kyverno --timeout=300s

# 创建策略: 禁止使用 latest 标签
cat > disallow-latest.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
spec:
  validationFailureAction: Enforce
  rules:
  - name: disallow-latest
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "Using 'latest' tag is not allowed."
      pattern:
        spec:
          containers:
          - image: "!*:latest"
EOF

kubectl apply -f disallow-latest.yaml

# 测试策略
kubectl run test --image=nginx:latest  # 应该被拒绝
kubectl run test --image=nginx:1.25    # 应该成功
```

---

## 费曼复述 (0.5h)

1. **Pod Security Standards 的三个级别分别有什么限制？**
2. **为什么容器应该以非 root 用户运行？**
3. **Secret 和 ConfigMap 的区别是什么？**

---

## 今日检验

- [ ] 能够配置 Pod SecurityContext
- [ ] 能够创建和使用 Secret
- [ ] 能够使用 Kyverno 实现策略即代码
