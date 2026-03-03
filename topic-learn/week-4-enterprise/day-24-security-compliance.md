# Day 24: 云原生安全 + 合规

> **学习时间**: 4-5 小时 | **主题**: 安全加固与合规检查

---

## 今日目标

- [ ] 了解 Kyverno 企业策略管理
- [ ] 了解 Vault Secret 管理
- [ ] 理解零信任安全架构

---

## 理论学习 (2h)

### 必读文档

1. **Kyverno 企业策略管理**
   - 文件: `../../domain-25-cloud-native-security/04-kyverno-enterprise-policy-management.md`

2. **Vault 企业 Secret 管理**
   - 文件: `../../domain-25-cloud-native-security/05-vault-enterprise-secrets-management.md`

3. **零信任安全架构**
   - 文件: `../../domain-18-production-operations/07-zero-trust-security-architecture.md`

---

## 实践任务 (2.5h)

### 任务 1: Kyverno 高级策略 (1h)

```bash
# 策略 1: 强制资源限制
cat > require-limits.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-limits
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-limits
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "CPU and memory limits are required"
      pattern:
        spec:
          containers:
          - resources:
              limits:
                memory: "?*"
                cpu: "?*"
EOF

kubectl apply -f require-limits.yaml

# 策略 2: 自动添加标签
cat > add-labels.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-labels
spec:
  rules:
  - name: add-team-label
    match:
      any:
      - resources:
          kinds:
          - Pod
    mutate:
      patchStrategicMerge:
        metadata:
          labels:
            managed-by: kyverno
EOF

kubectl apply -f add-labels.yaml
```

### 任务 2: Secret 管理最佳实践 (1h)

```bash
# 使用 External Secrets Operator 或 Vault
# 示例: Sealed Secrets

# 安装 Sealed Secrets Controller
kubectl apply -f https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.0/controller.yaml

# 安装 kubeseal CLI
# brew install kubeseal

# 创建加密的 Secret
kubectl create secret generic my-secret --dry-run=client -o yaml \
  --from-literal=password=supersecret | kubeseal > sealed-secret.yaml

# 应用加密的 Secret
kubectl apply -f sealed-secret.yaml
```

### 任务 3: 安全审计 (30min)

```bash
# 检查 RBAC 权限
kubectl auth can-i --list --as=system:serviceaccount:default:default

# 检查特权容器
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.containers[*].securityContext.privileged}{"\n"}{end}' | grep true

# 检查 hostNetwork
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.hostNetwork}{"\n"}{end}' | grep true
```

---

## 费曼复述 (0.5h)

1. **Kyverno 和 OPA Gatekeeper 的区别？**
2. **如何在 K8s 中安全管理 Secret？**
3. **零信任架构的核心原则是什么？**

---

## 今日检验

- [ ] 能够编写 Kyverno 策略
- [ ] 了解 Secret 管理最佳实践
- [ ] 理解云原生安全体系
