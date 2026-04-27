# Kyverno K8s 原生策略管理实践指南

> **适用版本**: Kyverno v1.14.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、Kyverno vs OPA Gatekeeper 选型](#一kyverno-vs-opa-gatekeeper-选型)
- [二、Helm 部署](#二helm-部署)
- [三、核心策略类型](#三核心策略类型)
- [四、安全策略示例库](#四安全策略示例库)
- [五、镜像验证与供应链安全](#五镜像验证与供应链安全)
- [六、策略报告与合规审计](#六策略报告与合规审计)
- [七、与 CI/CD 集成](#七与-cicd-集成)
- [八、性能与大规模部署](#八性能与大规模部署)

---

## 一、Kyverno vs OPA Gatekeeper 选型

| 维度 | Kyverno | OPA Gatekeeper |
|:---|:---|:---|
| 策略语言 | YAML (K8s 原生) | Rego (专用 DSL) |
| 学习曲线 | 低 | 高 |
| 适用场景 | 纯 K8s 环境 | 多平台统一策略 |
| 变异能力 | 强 (JSON Patch / Strategic Merge) | 中等 |
| 镜像验证 | 原生支持 cosign/Notary | 需扩展 |
| 生成能力 | 自动生成资源 (NetworkPolicy/Quota/etc) | 有限 |
| 社区 | 快速增长 | 成熟庞大 |
| CNCF 状态 | Graduated | 非 CNCF (但 OPA Graduated) |

**选型建议**: K8s 专属团队选 Kyverno；跨平台统一治理选 OPA。

---

## 二、Helm 部署

```bash
helm repo add kyverno https://kyverno.github.io/kyverno/
helm repo update

# 生产部署 (高可用)
helm install kyverno kyverno/kyverno \
  --namespace kyverno \
  --create-namespace \
  --set admissionController.replicas=3 \
  --set backgroundController.replicas=2 \
  --set cleanupController.replicas=2 \
  --set reportsController.replicas=2 \
  --version 3.3.0
```

---

## 三、核心策略类型

### 3.1 Validate (验证)

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-labels
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-team-label
    match:
      resources:
        kinds:
        - Deployment
        - StatefulSet
        - DaemonSet
    validate:
      message: "所有工作负载必须包含 team 标签"
      pattern:
        metadata:
          labels:
            team: "?*"
```

### 3.2 Mutate (变异)

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-default-resources
spec:
  rules:
  - name: add-memory-limit
    match:
      resources:
        kinds:
        - Deployment
    mutate:
      patchStrategicMerge:
        spec:
          template:
            spec:
              containers:
              - (name): "*"
                resources:
                  limits:
                    memory: "256Mi"
                  requests:
                    memory: "128Mi"
```

### 3.3 Generate (生成)

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-networkpolicy
spec:
  rules:
  - name: default-deny-ingress
    match:
      resources:
        kinds:
        - Namespace
    generate:
      kind: NetworkPolicy
      name: default-deny
      namespace: "{{request.object.metadata.name}}"
      synchronize: true
      data:
        spec:
          podSelector: {}
          policyTypes:
          - Ingress
```

### 3.4 VerifyImages (镜像验证)

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  rules:
  - name: verify-cosign-signature
    match:
      resources:
        kinds:
        - Pod
    verifyImages:
    - imageReferences:
      - "harbor.example.com/*"
      verifyDigest: true
      required: true
      attestors:
      - entries:
        - keys:
            publicKeys: |
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
```

---

## 四、安全策略示例库

### 4.1 禁止特权容器

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged
spec:
  validationFailureAction: Enforce
  rules:
  - name: privileged-containers
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "特权容器被禁止"
      pattern:
        spec:
          containers:
          - securityContext:
              privileged: "false"
```

### 4.2 禁止以 root 运行

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-run-as-non-root
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-run-as-non-root
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "容器必须以非 root 用户运行"
      pattern:
        spec:
          securityContext:
            runAsNonRoot: true
          containers:
          - securityContext:
              allowPrivilegeEscalation: false
              capabilities:
                drop:
                - ALL
              readOnlyRootFilesystem: true
              seccompProfile:
                type: RuntimeDefault
```

### 4.3 限制镜像来源

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-image-registries
spec:
  validationFailureAction: Enforce
  rules:
  - name: validate-registries
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "仅允许使用内部 Harbor 或官方镜像仓库"
      pattern:
        spec:
          containers:
          - image: "harbor.example.com/* | docker.io/library/* | gcr.io/company/*"
```

### 4.4 自动添加 Pod 安全标准

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: enforce-pod-security
spec:
  background: true
  validationFailureAction: Enforce
  rules:
  - name: restricted-profile
    match:
      resources:
        kinds:
        - Pod
    validate:
      podSecurity:
        level: restricted
        version: latest
```

---

## 五、镜像验证与供应链安全

### 5.1 cosign 密钥验证

```yaml
verifyImages:
- imageReferences:
  - "*"
  required: true
  mutateDigest: true
  verifyDigest: true
  attestors:
  - count: 1
    entries:
    - keys:
        publicKeys: |
          -----BEGIN PUBLIC KEY-----
          ...
          -----END PUBLIC KEY-----
```

### 5.2 Fulcio/Rekor 密钥less验证

```yaml
verifyImages:
- imageReferences:
  - "ghcr.io/my-org/*"
  required: true
  attestors:
  - count: 1
    entries:
    - keyless:
        issuer: "https://token.actions.githubusercontent.com"
        subject: "https://github.com/my-org/.github/workflows/*.yaml@refs/heads/main"
      rekor:
        url: https://rekor.sigstore.dev
```

---

## 六、策略报告与合规审计

### 6.1 PolicyReport

```bash
# 查看策略报告
kubectl get policyreport -A
kubectl get clusterpolicyreport

# 查看失败的资源
kubectl get clusterpolicyreport -o yaml | grep -A 5 "fail"
```

### 6.2 Policy Reporter UI

```bash
helm install policy-reporter policy-reporter/policy-reporter \
  --set ui.enabled=true \
  --set kyvernoPlugin.enabled=true
```

---

## 七、与 CI/CD 集成

### 7.1 CLI 预检

```bash
# 安装 kyverno CLI
brew install kyverno

# CI 中预检策略
kyverno test ./policies/
kyverno apply ./policies/ --resource ./manifests/deployment.yaml
```

### 7.2 GitHub Actions

```yaml
- name: Kyverno Policy Check
  uses: kyverno/action-install-cli@v0.2.0
- run: |
    kyverno apply ./policies/ --resource ./k8s-manifests/
```

---

## 八、性能与大规模部署

| 维度 | 建议 |
|:---|:---|
| Webhook 超时 | 默认 10s，大规模集群调至 30s |
| 规则数量 | 单 Policy 建议 < 50 条规则 |
| 背景扫描 | 大规模集群启用但调低频率 |
| 内存 | Admission Controller 2GB+ |
| 副本数 | 至少 3 副本保证高可用 |
| 排除命名空间 | kube-system, kyverno, monitoring |

---

## 参考链接

- [Kyverno 官方文档](https://kyverno.io/docs/)
- [Kyverno Policies 库](https://kyverno.io/policies/)
- [Kyverno Helm Chart](https://github.com/kyverno/kyverno/tree/main/charts/kyverno)
- [Policy Reporter](https://kyverno.github.io/policy-reporter/)
