# 13 - 安全、准入控制与 RBAC 事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录证书管理、ServiceAccount、准入控制、RBAC 和安全相关的所有事件。**

---

## 📋 事件分类总览

| 分类 | 事件数量 | 频率分布 | 主要场景 |
|------|---------|---------|---------|
| **Certificate Controller** | 3 | 低频-罕见 | 证书请求审批、签发 |
| **ServiceAccount Controller** | 2 | 罕见 | SA Token 管理 |
| **Token Controller** | 2 | 低频-罕见 | Token Secret 清理 |
| **ClusterRole Aggregation** | 1 | 低频 | 聚合角色更新 |
| **Admission Webhook** | 3 | 中频-低频 | 准入控制策略 |
| **Pod Security** | 3 | 中频-低频 | Pod 安全标准 |
| **总计** | **14** | 覆盖生产安全全场景 | K8s v1.25+ |

---

## 🔐 准入控制流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                    Kubernetes API Request                        │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
                   ┌──────────────────┐
                   │  Authentication  │
                   │   (认证层)        │
                   └────────┬─────────┘
                            │
                            ▼
                   ┌──────────────────┐
                   │  Authorization   │
                   │   (RBAC/ABAC)    │
                   └────────┬─────────┘
                            │
                            ▼
              ┌─────────────────────────┐
              │  Mutating Admission     │◄───── MutatingAdmissionWebhookError
              │  (变更准入控制)          │
              │  - Add sidecar          │
              │  - Inject secrets       │
              │  - Modify resources     │
              └────────┬────────────────┘
                       │
                       ▼
              ┌─────────────────────────┐
              │  Object Schema          │
              │  Validation (模式验证)   │
              └────────┬────────────────┘
                       │
                       ▼
              ┌─────────────────────────┐
              │  Validating Admission   │◄───── FailedAdmission
              │  (验证准入控制)          │◄───── ValidatingAdmissionPolicyViolation
              │  - Security policies    │◄───── PodSecurityViolation
              │  - Resource quotas      │
              │  - Custom validation    │
              └────────┬────────────────┘
                       │
                       ▼
              ┌─────────────────────────┐
              │  Persist to etcd        │
              │   (持久化存储)           │
              └─────────────────────────┘
```

**准入控制关键点**:
- **Mutating Webhooks**: 先执行,可修改对象
- **Validating Webhooks**: 后执行,只能拒绝或通过
- **Pod Security Admission**: 内置的 PSS 验证
- **Admission Policy (v1.30 GA)**: CEL 表达式策略

---

## 📊 事件汇总表

| # | 事件名称 | 类型 | 组件 | 版本 | 频率 | 核心场景 |
|---|---------|------|------|------|------|---------|
| 1 | `CertificateRequestApproved` | Normal | CertificateController | v1.4+ | 低频 | 证书请求批准 |
| 2 | `CertificateRequestDenied` | Warning | CertificateController | v1.4+ | 罕见 | 证书请求拒绝 |
| 3 | `CertificateRequestFailed` | Warning | CertificateController | v1.4+ | 罕见 | 证书签发失败 |
| 4 | `FailedCreate` | Warning | ServiceAccountController | v1.0+ | 罕见 | SA Token 创建失败 |
| 5 | `InvalidServiceAccount` | Warning | ServiceAccountController | v1.0+ | 罕见 | 无效 ServiceAccount |
| 6 | `DeletedTokenSecret` | Normal | TokenController | v1.6+ | 低频 | Token Secret 清理 |
| 7 | `FailedToDeleteTokenSecret` | Warning | TokenController | v1.6+ | 罕见 | Token 删除失败 |
| 8 | `ClusterRoleUpdated` | Normal | ClusterRoleAggregation | v1.9+ | 低频 | 聚合角色更新 |
| 9 | `FailedAdmission` | Warning | AdmissionWebhook | v1.9+ | 中频 | 准入策略拒绝 |
| 10 | `ValidatingAdmissionPolicyViolation` | Warning | ValidatingAdmissionPolicy | v1.30 GA | 中频 | CEL 策略违规 |
| 11 | `MutatingAdmissionWebhookError` | Warning | MutatingAdmissionWebhook | v1.9+ | 低频 | 变更 Webhook 失败 |
| 12 | `PodSecurityViolation` | Warning | PodSecurity | v1.25+ | 中频 | Pod 安全标准违规 |
| 13 | `FailedValidation` (Deprecated) | Warning | PodSecurityPolicy | v1.0-v1.25 | N/A | PSP 验证失败 (已弃用) |
| 14 | `PodSecurityExemption` | Normal | PodSecurity | v1.25+ | 低频 | Pod 安全豁免 |

---

## 🔖 1. Certificate Controller Events

### 1.1 CertificateRequestApproved

**事件详情**:
- **类型**: `Normal`
- **组件**: `CertificateController`
- **版本**: Kubernetes v1.4+
- **频率**: 低频 (自动化证书管理场景)
- **对象**: `CertificateSigningRequest`

**触发场景**:
```yaml
# 场景 1: 节点 kubelet 证书请求批准
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: node-csr-xyz
spec:
  request: <base64-encoded-CSR>
  signerName: kubernetes.io/kubelet-serving
  usages:
  - digital signature
  - key encipherment
  - server auth
---
# 自动批准后产生事件
Normal  CertificateRequestApproved  Approved by kubernetes.io/kubelet-serving
```

**事件示例**:
```
LAST SEEN   TYPE     REASON                        OBJECT                MESSAGE
2m          Normal   CertificateRequestApproved    certificatesigningrequest/node-csr-abc   Certificate request approved by kubernetes.io/kubelet-serving
30s         Normal   CertificateRequestApproved    certificatesigningrequest/user-cert-123   Approved by admin user
```

**常见原因**:
1. **自动批准**: Kubelet 证书自动更新
2. **手动批准**: 管理员批准用户证书请求
3. **策略批准**: 自定义 Approver 批准

**排查步骤**:
```bash
# 1. 查看 CSR 状态
kubectl get csr
kubectl describe csr <csr-name>

# 2. 查看批准历史
kubectl get csr <csr-name> -o jsonpath='{.status.conditions[*]}'

# 3. 查看签发的证书
kubectl get csr <csr-name> -o jsonpath='{.status.certificate}' | base64 -d | openssl x509 -text -noout
```

**解决方案**:
- ✅ **正常事件**: 证书请求正常批准,无需处理
- 📝 **审计建议**: 记录批准历史用于安全审计

---

### 1.2 CertificateRequestDenied

**事件详情**:
- **类型**: `Warning`
- **组件**: `CertificateController`
- **版本**: Kubernetes v1.4+
- **频率**: 罕见 (安全策略拒绝)
- **对象**: `CertificateSigningRequest`

**触发场景**:
```yaml
# 场景 1: 不符合签名策略的证书请求
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: invalid-csr
spec:
  request: <base64-encoded-CSR>
  signerName: kubernetes.io/kube-apiserver-client
  usages:
  - server auth  # 错误: client 证书不能用于 server auth
---
# 被拒绝后产生事件
Warning  CertificateRequestDenied  Denied: usage not allowed for signer
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                     OBJECT                MESSAGE
1m          Warning   CertificateRequestDenied   certificatesigningrequest/bad-csr   Denied by admin: security policy violation
5m          Warning   CertificateRequestDenied   certificatesigningrequest/user-123   Denied: subject does not match organization policy
```

**常见原因**:
1. **Usage 不匹配**: Usages 与 signerName 不兼容
2. **策略违规**: 不符合组织证书策略
3. **手动拒绝**: 管理员明确拒绝
4. **CN/SAN 异常**: Subject 或 SAN 不符合规范

**排查步骤**:
```bash
# 1. 查看拒绝原因
kubectl get csr <csr-name> -o jsonpath='{.status.conditions[?(@.type=="Denied")].message}'

# 2. 检查 CSR 内容
kubectl get csr <csr-name> -o jsonpath='{.spec.request}' | base64 -d | openssl req -text -noout

# 3. 验证 signerName 和 usages
kubectl get csr <csr-name> -o yaml
```

**解决方案**:
```bash
# 方案 1: 修正 usages (需删除重建)
kubectl delete csr <csr-name>
# 创建正确的 CSR

# 方案 2: 联系管理员重新审批
kubectl certificate approve <csr-name>  # 需要管理员权限

# 方案 3: 检查 signer 配置
kubectl get --raw /apis/certificates.k8s.io/v1/signers
```

---

### 1.3 CertificateRequestFailed

**事件详情**:
- **类型**: `Warning`
- **组件**: `CertificateController`
- **版本**: Kubernetes v1.4+
- **频率**: 罕见 (签名服务异常)
- **对象**: `CertificateSigningRequest`

**触发场景**:
```yaml
# 场景 1: CA 证书过期导致签发失败
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: cert-fail-example
spec:
  request: <base64-encoded-CSR>
  signerName: kubernetes.io/kube-apiserver-client
  usages:
  - client auth
---
# 签发失败产生事件
Warning  CertificateRequestFailed  Failed to sign certificate: CA certificate expired
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                      OBJECT                MESSAGE
30s         Warning   CertificateRequestFailed    certificatesigningrequest/node-xyz   Failed to sign certificate: signer not available
2m          Warning   CertificateRequestFailed    certificatesigningrequest/cert-123   Failed: CA key read error
```

**常见原因**:
1. **CA 不可用**: Signer 服务异常或未配置
2. **CA 证书过期**: CA 证书本身过期
3. **CA 密钥问题**: 无法读取 CA 私钥
4. **配置错误**: Controller Manager 签名配置错误

**排查步骤**:
```bash
# 1. 检查 Controller Manager 日志
kubectl logs -n kube-system kube-controller-manager-xxx | grep -i certificate

# 2. 验证 CA 证书
openssl x509 -in /etc/kubernetes/pki/ca.crt -text -noout | grep -A2 Validity

# 3. 检查 signer 配置
kubectl get --raw /apis/certificates.k8s.io/v1/signers

# 4. 查看 Controller Manager 启动参数
ps aux | grep kube-controller-manager | grep -o '\-\-cluster-signing.*'
```

**解决方案**:
```bash
# 方案 1: 重启 Controller Manager (CA 配置已修复)
kubectl delete pod -n kube-system -l component=kube-controller-manager

# 方案 2: 检查 CA 文件权限
ls -l /etc/kubernetes/pki/ca.*
# 确保 Controller Manager 有读取权限

# 方案 3: 更新 CA 证书 (极端情况)
# ⚠️ 需要集群维护窗口,影响所有证书签发
kubeadm certs renew all

# 方案 4: 使用外部 Signer
# 配置 external signer (如 cert-manager)
```

---

## 🔑 2. ServiceAccount Controller Events

### 2.1 FailedCreate (ServiceAccount Token)

**事件详情**:
- **类型**: `Warning`
- **组件**: `ServiceAccountController`
- **版本**: Kubernetes v1.0+
- **频率**: 罕见 (系统异常)
- **对象**: `ServiceAccount`

**触发场景**:
```yaml
# 场景 1: 为 ServiceAccount 创建 Token Secret 失败
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: prod
---
# Token Secret 创建失败产生事件
Warning  FailedCreate  Error creating token Secret for ServiceAccount: API server unreachable
```

**事件示例**:
```
LAST SEEN   TYPE      REASON         OBJECT                   MESSAGE
1m          Warning   FailedCreate   serviceaccount/app-sa    Error creating: secrets is forbidden: User "system:serviceaccount:kube-system:service-account-controller" cannot create resource "secrets"
30s         Warning   FailedCreate   serviceaccount/test-sa   Failed to create token secret: etcd timeout
```

**常见原因**:
1. **RBAC 权限不足**: Controller 缺少创建 Secret 权限
2. **API Server 异常**: API Server 不可达或超时
3. **etcd 问题**: etcd 存储异常
4. **ResourceQuota 限制**: Namespace 配额已满

**排查步骤**:
```bash
# 1. 检查 ServiceAccount 状态
kubectl get sa -A
kubectl describe sa <sa-name> -n <namespace>

# 2. 查看 ServiceAccount Controller 日志
kubectl logs -n kube-system kube-controller-manager-xxx | grep -i "service.*account"

# 3. 验证 RBAC 权限
kubectl auth can-i create secrets --as=system:serviceaccount:kube-system:service-account-controller

# 4. 检查 Namespace ResourceQuota
kubectl describe quota -n <namespace>
```

**解决方案**:
```bash
# 方案 1: 修复 RBAC (极少需要,应预检查)
kubectl get clusterrolebinding system:controller:service-account-controller -o yaml

# 方案 2: 清理无效 ServiceAccount 重建
kubectl delete sa <sa-name> -n <namespace>
kubectl create sa <sa-name> -n <namespace>

# 方案 3: 手动创建 Token Secret (v1.24+ 推荐用 TokenRequest API)
kubectl create token <sa-name> -n <namespace> --duration=8760h

# 方案 4: 调整 ResourceQuota
kubectl edit quota <quota-name> -n <namespace>
```

**版本差异**:
- **v1.24+**: Bound Token 自动创建,不再默认生成 Secret
- **v1.24-**: 自动为每个 SA 创建 Secret Token

---

### 2.2 InvalidServiceAccount

**事件详情**:
- **类型**: `Warning`
- **组件**: Various (Kubelet, Admission)
- **版本**: Kubernetes v1.0+
- **频率**: 罕见 (配置错误)
- **对象**: `Pod`

**触发场景**:
```yaml
# 场景 1: Pod 引用不存在的 ServiceAccount
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
  namespace: prod
spec:
  serviceAccountName: non-existent-sa
  containers:
  - name: app
    image: nginx
---
# 创建时产生事件
Warning  InvalidServiceAccount  ServiceAccount "non-existent-sa" not found
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                  OBJECT           MESSAGE
1m          Warning   InvalidServiceAccount   pod/app-pod      ServiceAccount "app-sa" not found in namespace "prod"
30s         Warning   InvalidServiceAccount   pod/test-pod     ServiceAccount "default" is being deleted
```

**常见原因**:
1. **SA 不存在**: ServiceAccount 未创建
2. **Namespace 错误**: SA 在不同 Namespace
3. **SA 正在删除**: ServiceAccount 处于 Terminating 状态
4. **拼写错误**: serviceAccountName 字段拼写错误

**排查步骤**:
```bash
# 1. 检查 ServiceAccount 是否存在
kubectl get sa -n <namespace>

# 2. 查看 Pod 事件
kubectl describe pod <pod-name> -n <namespace>

# 3. 验证 Pod 配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.serviceAccountName}'

# 4. 检查 SA 状态
kubectl get sa <sa-name> -n <namespace> -o yaml
```

**解决方案**:
```bash
# 方案 1: 创建缺失的 ServiceAccount
kubectl create sa <sa-name> -n <namespace>

# 方案 2: 更新 Pod 使用正确的 SA
kubectl delete pod <pod-name> -n <namespace>
# 修改 YAML 后重建

# 方案 3: 使用默认 ServiceAccount
# 删除 serviceAccountName 字段,自动使用 default

# 方案 4: 恢复被删除的 SA
kubectl apply -f serviceaccount.yaml
```

---

## 🎫 3. Token Controller Events

### 3.1 DeletedTokenSecret

**事件详情**:
- **类型**: `Normal`
- **组件**: `TokenController`
- **版本**: Kubernetes v1.6+
- **频率**: 低频 (自动清理)
- **对象**: `ServiceAccount`

**触发场景**:
```yaml
# 场景 1: ServiceAccount 删除后清理 Token Secret
apiVersion: v1
kind: ServiceAccount
metadata:
  name: temp-sa
  namespace: test
---
# 删除 SA 后,Token Secret 自动清理
Normal  DeletedTokenSecret  Deleted token secret "temp-sa-token-xyz" for deleted ServiceAccount
```

**事件示例**:
```
LAST SEEN   TYPE    REASON              OBJECT                  MESSAGE
1m          Normal  DeletedTokenSecret  serviceaccount/app-sa   Deleted token secret "app-sa-token-abc" after ServiceAccount deletion
30s         Normal  DeletedTokenSecret  serviceaccount/test-sa  Cleaned up orphaned token secret "test-sa-token-old"
```

**常见原因**:
1. **SA 删除**: ServiceAccount 被删除后清理关联 Token
2. **孤儿清理**: 清理无效的孤儿 Token Secret
3. **Token 轮换**: 旧 Token 过期后清理
4. **自动维护**: TokenController 定期清理

**排查步骤**:
```bash
# 1. 查看 ServiceAccount Token 历史
kubectl get secrets -A | grep token

# 2. 检查 TokenController 日志
kubectl logs -n kube-system kube-controller-manager-xxx | grep -i "token"

# 3. 验证 SA 状态
kubectl get sa -A

# 4. 查看 Secret 清理历史
kubectl get events -A --field-selector reason=DeletedTokenSecret
```

**解决方案**:
- ✅ **正常事件**: Token Secret 自动清理,无需处理
- 📝 **审计建议**: 监控异常清理频率

---

### 3.2 FailedToDeleteTokenSecret

**事件详情**:
- **类型**: `Warning`
- **组件**: `TokenController`
- **版本**: Kubernetes v1.6+
- **频率**: 罕见 (系统异常)
- **对象**: `ServiceAccount`

**触发场景**:
```yaml
# 场景 1: Token Secret 删除失败 (RBAC 权限问题)
apiVersion: v1
kind: Secret
metadata:
  name: old-token-xyz
  namespace: prod
  annotations:
    kubernetes.io/service-account.name: deleted-sa
type: kubernetes.io/service-account-token
---
# 删除失败产生事件
Warning  FailedToDeleteTokenSecret  Failed to delete token secret: secrets "old-token-xyz" is forbidden
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                     OBJECT                  MESSAGE
1m          Warning   FailedToDeleteTokenSecret  serviceaccount/app-sa   Failed to delete token secret "app-sa-token-abc": API server timeout
30s         Warning   FailedToDeleteTokenSecret  serviceaccount/test-sa  Error deleting: secrets is forbidden
```

**常见原因**:
1. **RBAC 权限不足**: TokenController 缺少删除 Secret 权限
2. **API Server 异常**: API Server 超时或不可达
3. **Finalizer 阻塞**: Secret 存在 Finalizer 阻止删除
4. **etcd 问题**: etcd 存储异常

**排查步骤**:
```bash
# 1. 查看 Token Secret 状态
kubectl get secret <secret-name> -n <namespace> -o yaml

# 2. 检查 Finalizers
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.metadata.finalizers}'

# 3. 验证 TokenController 权限
kubectl auth can-i delete secrets --as=system:serviceaccount:kube-system:token-controller

# 4. 查看 Controller Manager 日志
kubectl logs -n kube-system kube-controller-manager-xxx | grep -i "failed.*delete.*token"
```

**解决方案**:
```bash
# 方案 1: 手动删除 Token Secret
kubectl delete secret <secret-name> -n <namespace>

# 方案 2: 移除 Finalizer (如果存在)
kubectl patch secret <secret-name> -n <namespace> -p '{"metadata":{"finalizers":null}}' --type=merge

# 方案 3: 重启 Controller Manager (系统异常)
kubectl delete pod -n kube-system -l component=kube-controller-manager

# 方案 4: 检查 RBAC (极少需要)
kubectl get clusterrolebinding | grep token-controller
```

---

## 👥 4. ClusterRole Aggregation Events

### 4.1 ClusterRoleUpdated

**事件详情**:
- **类型**: `Normal`
- **组件**: `ClusterRoleAggregationController`
- **版本**: Kubernetes v1.9+
- **频率**: 低频 (角色聚合更新)
- **对象**: `ClusterRole`

**触发场景**:
```yaml
# 场景 1: 聚合 ClusterRole 自动更新
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitoring-admin
aggregationRule:
  clusterRoleSelectors:
  - matchLabels:
      rbac.example.com/aggregate-to-monitoring: "true"
rules: []  # 自动聚合填充
---
# 添加新的被聚合角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: prometheus-reader
  labels:
    rbac.example.com/aggregate-to-monitoring: "true"
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list"]
---
# 聚合后产生事件
Normal  ClusterRoleUpdated  Updated aggregated ClusterRole with new rules
```

**事件示例**:
```
LAST SEEN   TYPE    REASON              OBJECT                       MESSAGE
1m          Normal  ClusterRoleUpdated  clusterrole/admin            Updated ClusterRole aggregation from 45 to 47 rules
30s         Normal  ClusterRoleUpdated  clusterrole/edit             Aggregated new rules from custom-resource-editor
```

**常见原因**:
1. **添加聚合角色**: 新增符合 selector 的 ClusterRole
2. **更新聚合角色**: 修改被聚合角色的 rules
3. **删除聚合角色**: 删除被聚合角色后自动移除规则
4. **CRD 安装**: 安装 Operator 时添加聚合权限

**排查步骤**:
```bash
# 1. 查看聚合 ClusterRole
kubectl get clusterrole <role-name> -o yaml

# 2. 查看被聚合的角色
kubectl get clusterrole -l <aggregation-label>

# 3. 查看聚合后的完整规则
kubectl describe clusterrole <role-name>

# 4. 查看聚合事件历史
kubectl get events --all-namespaces --field-selector reason=ClusterRoleUpdated
```

**解决方案**:
- ✅ **正常事件**: ClusterRole 聚合正常工作
- 📝 **最佳实践**: 使用聚合管理复杂 RBAC

**聚合示例**:
```yaml
# 内置聚合角色示例
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: admin
aggregationRule:
  clusterRoleSelectors:
  - matchLabels:
      rbac.authorization.k8s.io/aggregate-to-admin: "true"
rules: []
---
# 自定义聚合角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: custom-resource-admin
  labels:
    rbac.authorization.k8s.io/aggregate-to-admin: "true"
rules:
- apiGroups: ["mycompany.com"]
  resources: ["myresources"]
  verbs: ["*"]
```

---

## 🚪 5. Admission Webhook Events

### 5.1 FailedAdmission

**事件详情**:
- **类型**: `Warning`
- **组件**: `AdmissionWebhook` (Validating/Mutating)
- **版本**: Kubernetes v1.9+
- **频率**: 中频 (策略拒绝常见)
- **对象**: `Pod`, `Deployment`, 等各类资源

**触发场景**:
```yaml
# 场景 1: Pod 被 ValidatingWebhook 拒绝
apiVersion: v1
kind: Pod
metadata:
  name: nginx-pod
  namespace: prod
spec:
  containers:
  - name: nginx
    image: nginx:latest  # 违反策略: 不允许使用 latest 标签
---
# 被拒绝后产生事件
Warning  FailedAdmission  Admission webhook "image-policy.company.com" denied: image tag "latest" is not allowed
```

**事件示例**:
```
LAST SEEN   TYPE      REASON          OBJECT              MESSAGE
1m          Warning   FailedAdmission pod/nginx-pod       Admission webhook "policy.example.com" denied the request: missing required label "owner"
30s         Warning   FailedAdmission deployment/app      Admission webhook "resource-quota.company.com" denied: CPU request exceeds limit
10s         Warning   FailedAdmission service/frontend    Validating webhook denied: LoadBalancer type not allowed in this namespace
```

**常见原因**:
1. **镜像策略**: 镜像标签或仓库不符合规范
2. **资源限制**: 资源请求超过策略限制
3. **标签缺失**: 缺少必需的标签或注解
4. **安全策略**: 违反安全策略(如特权容器)
5. **自定义策略**: 违反组织自定义策略

**排查步骤**:
```bash
# 1. 查看 Webhook 配置
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations
kubectl describe validatingwebhookconfiguration <webhook-name>

# 2. 查看拒绝详情
kubectl describe pod <pod-name> -n <namespace>
kubectl get events -n <namespace> --field-selector reason=FailedAdmission

# 3. 测试 Webhook 策略
kubectl apply --dry-run=server -f pod.yaml

# 4. 查看 Webhook 服务日志
kubectl logs -n <webhook-namespace> <webhook-pod>
```

**解决方案**:
```bash
# 方案 1: 修复资源配置符合策略
# 示例: 修改镜像标签
spec:
  containers:
  - name: app
    image: nginx:1.21.6  # 使用具体版本

# 方案 2: 添加必需的标签/注解
metadata:
  labels:
    owner: team-a
    environment: production

# 方案 3: 申请策略豁免 (如果 Webhook 支持)
metadata:
  annotations:
    policy.company.com/exempt: "true"
    policy.company.com/reason: "legacy-app"

# 方案 4: 临时禁用 Webhook (紧急情况)
kubectl delete validatingwebhookconfiguration <webhook-name>
# ⚠️ 仅用于紧急情况,需要重新启用

# 方案 5: 修改 Webhook failurePolicy
kubectl patch validatingwebhookconfiguration <webhook-name> \
  --type='json' -p='[{"op": "replace", "path": "/webhooks/0/failurePolicy", "value":"Ignore"}]'
```

**Webhook 调试技巧**:
```bash
# 查看 Webhook 超时配置
kubectl get validatingwebhookconfiguration <name> -o jsonpath='{.webhooks[0].timeoutSeconds}'

# 查看 Webhook 失败策略
kubectl get validatingwebhookconfiguration <name> -o jsonpath='{.webhooks[0].failurePolicy}'

# 查看 Webhook 匹配规则
kubectl get validatingwebhookconfiguration <name> -o jsonpath='{.webhooks[0].rules}'
```

---

### 5.2 ValidatingAdmissionPolicyViolation

**事件详情**:
- **类型**: `Warning`
- **组件**: `ValidatingAdmissionPolicy`
- **版本**: Kubernetes v1.26 Beta → v1.30 GA
- **频率**: 中频 (CEL 策略验证)
- **对象**: 所有支持的资源类型

**触发场景**:
```yaml
# 场景 1: CEL 策略验证失败
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-labels
spec:
  matchConstraints:
    resourceRules:
    - apiGroups: ["apps"]
      apiVersions: ["v1"]
      resources: ["deployments"]
      operations: ["CREATE", "UPDATE"]
  validations:
  - expression: "has(object.metadata.labels.owner)"
    message: "Deployment must have 'owner' label"
---
# 创建不符合策略的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
  # 缺少 owner 标签
spec:
  replicas: 1
  ...
---
# 产生策略违规事件
Warning  ValidatingAdmissionPolicyViolation  Validation failed: Deployment must have 'owner' label
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                               OBJECT                MESSAGE
1m          Warning   ValidatingAdmissionPolicyViolation   deployment/app        Validation expression failed: has(object.metadata.labels.owner)
30s         Warning   ValidatingAdmissionPolicyViolation   pod/nginx             Policy "resource-limits" violation: container memory limit exceeds 2Gi
10s         Warning   ValidatingAdmissionPolicyViolation   service/api           Expression evaluation error: invalid port range
```

**常见原因**:
1. **CEL 表达式失败**: 资源不满足 CEL 验证表达式
2. **必需字段缺失**: 缺少策略要求的字段或标签
3. **值不合规**: 字段值不符合策略范围
4. **表达式错误**: CEL 表达式本身有误

**排查步骤**:
```bash
# 1. 查看 ValidatingAdmissionPolicy 配置
kubectl get validatingadmissionpolicies
kubectl describe validatingadmissionpolicy <policy-name>

# 2. 查看关联的 PolicyBinding
kubectl get validatingadmissionpolicybindings
kubectl describe validatingadmissionpolicybinding <binding-name>

# 3. 查看违规事件详情
kubectl get events -A --field-selector reason=ValidatingAdmissionPolicyViolation

# 4. 测试 CEL 表达式 (使用 kubectl-validate-cel 插件)
kubectl validate-cel --expression="has(object.metadata.labels.owner)" --object=deployment.yaml
```

**解决方案**:
```bash
# 方案 1: 修复资源配置满足策略
# 示例: 添加缺失的标签
metadata:
  labels:
    owner: team-a
    cost-center: engineering

# 方案 2: 调整 ValidatingAdmissionPolicy 表达式
kubectl edit validatingadmissionpolicy <policy-name>
# 修改 validations.expression 字段

# 方案 3: 修改 PolicyBinding 排除特定 Namespace
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: require-labels-binding
spec:
  policyName: require-labels
  validationActions: ["Deny"]
  matchResources:
    namespaceSelector:
      matchExpressions:
      - key: policy.company.com/enforce
        operator: NotIn
        values: ["false"]

# 方案 4: 设置 validationActions 为 Warn (审计模式)
spec:
  validationActions: ["Warn"]  # 不阻止,仅警告
```

**CEL 策略示例**:
```yaml
---
# 示例 1: 限制容器资源
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: container-limits
spec:
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      resources: ["pods"]
      operations: ["CREATE", "UPDATE"]
  validations:
  - expression: |
      object.spec.containers.all(c, 
        has(c.resources.limits.memory) && 
        c.resources.limits.memory.endsWith('Gi') &&
        int(c.resources.limits.memory.replace('Gi','')) <= 4
      )
    message: "Container memory limit must be set and ≤ 4Gi"
---
# 示例 2: 强制使用特定镜像仓库
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: image-registry
spec:
  matchConstraints:
    resourceRules:
    - apiGroups: ["apps"]
      resources: ["deployments"]
      operations: ["CREATE", "UPDATE"]
  validations:
  - expression: |
      object.spec.template.spec.containers.all(c,
        c.image.startsWith('registry.company.com/')
      )
    message: "Images must be from registry.company.com"
```

---

### 5.3 MutatingAdmissionWebhookError

**事件详情**:
- **类型**: `Warning`
- **组件**: `MutatingAdmissionWebhook`
- **版本**: Kubernetes v1.9+
- **频率**: 低频 (Webhook 服务异常)
- **对象**: 各类资源

**触发场景**:
```yaml
# 场景 1: Mutating Webhook 服务不可达
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: sidecar-injector
webhooks:
- name: sidecar.company.com
  clientConfig:
    service:
      name: sidecar-injector
      namespace: istio-system
      path: /inject
  failurePolicy: Fail  # 失败时拒绝请求
---
# Webhook 服务异常导致注入失败
Warning  MutatingAdmissionWebhookError  Failed calling webhook: connection refused
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                         OBJECT          MESSAGE
1m          Warning   MutatingAdmissionWebhookError  pod/app-xyz     Failed calling webhook "sidecar.company.com": Post https://sidecar-injector.istio-system:443: dial tcp 10.96.1.100:443: connect: connection refused
30s         Warning   MutatingAdmissionWebhookError  deployment/app  Webhook "mutate.example.com" timeout after 10s
10s         Warning   MutatingAdmissionWebhookError  pod/test        Webhook returned invalid JSON patch
```

**常见原因**:
1. **Webhook 服务不可达**: Webhook Pod 未运行或 Service 异常
2. **超时**: Webhook 处理超过配置的超时时间
3. **TLS 证书问题**: 证书过期或不匹配
4. **响应格式错误**: Webhook 返回无效的 JSON Patch
5. **Webhook Panic**: Webhook 代码崩溃

**排查步骤**:
```bash
# 1. 检查 Webhook 服务状态
kubectl get mutatingwebhookconfigurations
kubectl describe mutatingwebhookconfiguration <webhook-name>

# 2. 检查 Webhook Pod 运行状态
kubectl get pods -n <webhook-namespace>
kubectl logs -n <webhook-namespace> <webhook-pod>

# 3. 测试 Webhook 服务连通性
kubectl run test -it --rm --image=curlimages/curl -- \
  curl -k https://<webhook-service>.<namespace>:443/health

# 4. 检查 Webhook 证书
kubectl get secret -n <webhook-namespace> <webhook-cert-secret> -o jsonpath='{.data.tls\.crt}' | \
  base64 -d | openssl x509 -text -noout | grep -A2 Validity

# 5. 查看 API Server 日志
journalctl -u kubelet | grep -i "webhook"
```

**解决方案**:
```bash
# 方案 1: 重启 Webhook Pod
kubectl rollout restart deployment/<webhook-deployment> -n <webhook-namespace>

# 方案 2: 修改 failurePolicy 为 Ignore (紧急情况)
kubectl patch mutatingwebhookconfiguration <webhook-name> \
  --type='json' -p='[{"op": "replace", "path": "/webhooks/0/failurePolicy", "value":"Ignore"}]'

# 方案 3: 增加超时时间
kubectl patch mutatingwebhookconfiguration <webhook-name> \
  --type='json' -p='[{"op": "replace", "path": "/webhooks/0/timeoutSeconds", "value":30}]'

# 方案 4: 更新 Webhook 证书
cert-manager renew <certificate-name> -n <webhook-namespace>
# 或手动更新证书 Secret

# 方案 5: 临时禁用 Webhook (极端情况)
kubectl delete mutatingwebhookconfiguration <webhook-name>
# ⚠️ 需要重新部署 Webhook

# 方案 6: 修复 Webhook 代码 (响应格式错误)
# 检查 Webhook 返回的 JSON Patch 格式
kubectl logs -n <webhook-namespace> <webhook-pod> | grep -i "patch"
```

**Webhook 最佳实践**:
```yaml
---
# 推荐配置
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: example-webhook
webhooks:
- name: example.company.com
  failurePolicy: Ignore  # 生产环境推荐 Ignore,除非强制要求
  timeoutSeconds: 10     # 合理的超时时间
  admissionReviewVersions: ["v1", "v1beta1"]
  reinvocationPolicy: IfNeeded
  matchPolicy: Equivalent
  clientConfig:
    service:
      name: webhook-service
      namespace: webhook-system
      path: /mutate
    caBundle: <base64-CA-cert>
  objectSelector:
    matchExpressions:
    - key: webhook.company.com/inject
      operator: In
      values: ["true"]
```

---

## 🔐 6. Pod Security Events

### 6.1 PodSecurityViolation

**事件详情**:
- **类型**: `Warning`
- **组件**: `PodSecurity` Admission
- **版本**: Kubernetes v1.25+ (取代 PSP)
- **频率**: 中频 (安全策略常见)
- **对象**: `Pod`

**触发场景**:
```yaml
# 场景 1: Namespace 强制 Baseline 模式,Pod 违规
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.32
---
# 创建违规 Pod (使用 hostPath)
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
  namespace: production
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: host-data
      mountPath: /data
  volumes:
  - name: host-data
    hostPath:
      path: /var/data  # 违反 Baseline: hostPath 不允许
---
# 产生安全违规事件
Warning  PodSecurityViolation  Pod violates PodSecurity "baseline": hostPath volumes are forbidden
```

**事件示例**:
```
LAST SEEN   TYPE      REASON                OBJECT          MESSAGE
1m          Warning   PodSecurityViolation  pod/app-pod     violates PodSecurity "baseline:v1.32": hostPath volumes are forbidden
30s         Warning   PodSecurityViolation  pod/nginx       violates PodSecurity "restricted:v1.32": runAsNonRoot != true
10s         Warning   PodSecurityViolation  pod/debug       violates PodSecurity "baseline:v1.32": privileged container not allowed
```

**常见原因**:
1. **Privileged 容器**: 在 Baseline/Restricted 模式下使用特权容器
2. **hostPath Volume**: Baseline/Restricted 禁止 hostPath
3. **hostNetwork**: Baseline/Restricted 禁止 hostNetwork
4. **runAsRoot**: Restricted 模式要求 runAsNonRoot=true
5. **Capabilities**: 添加不允许的 Linux Capabilities
6. **hostPort**: Baseline/Restricted 禁止 hostPort
7. **HostProcess**: 禁止 Windows HostProcess 容器

**Pod Security Standards 对比**:

| 策略 | Privileged | Baseline | Restricted |
|------|-----------|----------|------------|
| **级别** | 无限制 | 最小限制 | 高度限制 |
| **hostNetwork** | ✅ | ❌ | ❌ |
| **hostPID/IPC** | ✅ | ❌ | ❌ |
| **hostPath** | ✅ | ❌ | ❌ |
| **privileged** | ✅ | ❌ | ❌ |
| **Capabilities (add)** | ✅ | 部分 | 最小集 |
| **runAsNonRoot** | ⚠️ | ⚠️ | ✅ 强制 |
| **SELinux** | ⚠️ | ⚠️ | ✅ 强制 |
| **seccompProfile** | ⚠️ | ⚠️ | RuntimeDefault 或 Localhost |
| **Volume types** | 全部 | 不含 hostPath/hostPathType | 更严格 |

**排查步骤**:
```bash
# 1. 查看 Namespace Pod Security 配置
kubectl get namespace <namespace> -o jsonpath='{.metadata.labels}' | grep pod-security

# 2. 查看违规详情
kubectl describe pod <pod-name> -n <namespace>

# 3. 验证 Pod 安全上下文
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.securityContext}'
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].securityContext}'

# 4. 测试 Pod Security 合规性 (dry-run)
kubectl apply --dry-run=server -f pod.yaml -n <namespace>

# 5. 审计模式检查所有 Namespace
kubectl label namespace <namespace> \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted
```

**解决方案**:
```bash
# 方案 1: 修复 Pod 配置符合安全标准
# 示例 1: 移除 hostPath
spec:
  volumes:
  - name: data
    emptyDir: {}  # 使用 emptyDir 替代 hostPath

# 示例 2: 设置 runAsNonRoot (Restricted 模式)
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 1000
  containers:
  - name: app
    securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop: ["ALL"]
      seccompProfile:
        type: RuntimeDefault

# 方案 2: 调整 Namespace 安全级别 (谨慎)
kubectl label namespace <namespace> \
  pod-security.kubernetes.io/enforce=baseline \
  --overwrite

# 方案 3: 使用豁免 (特定 Pod)
# 在 admission configuration 中配置豁免
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: "baseline"
      enforce-version: "latest"
    exemptions:
      usernames: []
      runtimeClasses: []
      namespaces: ["kube-system"]

# 方案 4: 审计模式排查 (不阻止创建)
kubectl label namespace <namespace> \
  pod-security.kubernetes.io/enforce=privileged \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted
```

**安全加固示例**:
```yaml
---
# 符合 Restricted 标准的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
  namespace: production
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: nginx:1.21.6
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
    resources:
      limits:
        cpu: "1"
        memory: "1Gi"
      requests:
        cpu: "100m"
        memory: "128Mi"
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache/nginx
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
```

---

### 6.2 FailedValidation (PodSecurityPolicy - Deprecated)

**事件详情**:
- **类型**: `Warning`
- **组件**: `PodSecurityPolicy` Admission (已废弃)
- **版本**: Kubernetes v1.0 - v1.25 (v1.25 移除)
- **频率**: N/A (功能已移除)
- **对象**: `Pod`

**历史背景**:
- PodSecurityPolicy (PSP) 在 Kubernetes v1.25 中完全移除
- 替代方案: **Pod Security Admission (PSA)** (v1.25+)
- 迁移指南: [PSP to PSA Migration](https://kubernetes.io/docs/tasks/configure-pod-container/migrate-from-psp/)

**事件示例** (历史参考):
```
LAST SEEN   TYPE      REASON            OBJECT       MESSAGE
1m          Warning   FailedValidation  pod/app-pod  Pod does not match any PodSecurityPolicy
30s         Warning   FailedValidation  pod/nginx    Unable to validate against any PodSecurityPolicy: privileged container not allowed
```

**迁移建议**:
```bash
# 检查集群是否仍使用 PSP (v1.25+ 应为空)
kubectl get psp

# 迁移到 Pod Security Admission
kubectl label namespace <namespace> \
  pod-security.kubernetes.io/enforce=baseline \
  pod-security.kubernetes.io/enforce-version=v1.32
```

---

### 6.3 PodSecurityExemption

**事件详情**:
- **类型**: `Normal`
- **组件**: `PodSecurity` Admission
- **版本**: Kubernetes v1.25+
- **频率**: 低频 (豁免场景)
- **对象**: `Pod`

**触发场景**:
```yaml
# 场景 1: kube-system Namespace 默认豁免
apiVersion: v1
kind: Pod
metadata:
  name: kube-proxy-xyz
  namespace: kube-system  # 系统 Namespace 通常豁免
spec:
  hostNetwork: true  # 允许违反安全策略
  containers:
  - name: kube-proxy
    image: k8s.gcr.io/kube-proxy:v1.32.0
---
# 产生豁免事件
Normal  PodSecurityExemption  Pod exempted from PodSecurity: namespace "kube-system" is exempt
```

**事件示例**:
```
LAST SEEN   TYPE    REASON                OBJECT                MESSAGE
1m          Normal  PodSecurityExemption  pod/kube-proxy-abc    Exempted from PodSecurity: namespace "kube-system" is exempt
30s         Normal  PodSecurityExemption  pod/calico-node       Exempted: user "system:serviceaccount:kube-system:calico-node" is exempt
```

**豁免配置**:
```yaml
---
# Pod Security Admission Configuration
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: "baseline"
      enforce-version: "latest"
      audit: "restricted"
      audit-version: "latest"
      warn: "restricted"
      warn-version: "latest"
    exemptions:
      # 豁免特定 Namespace
      namespaces:
        - kube-system
        - kube-public
        - kube-node-lease
      # 豁免特定 RuntimeClass
      runtimeClasses:
        - kata-containers
      # 豁免特定用户 (谨慎使用)
      usernames:
        - "system:serviceaccount:monitoring:prometheus"
```

**排查步骤**:
```bash
# 1. 查看 Admission Configuration
kubectl get --raw /api/v1/namespaces/kube-system/configmaps/pod-security-configuration

# 2. 验证豁免配置
kubectl describe namespace <namespace>

# 3. 查看豁免事件
kubectl get events -A --field-selector reason=PodSecurityExemption
```

**解决方案**:
- ✅ **正常事件**: 豁免按预期工作
- 📝 **审计建议**: 定期审查豁免配置,最小化豁免范围

---

## 🔍 证书管理生命周期

```
┌──────────────────────────────────────────────────────────────┐
│                 Certificate Lifecycle                         │
└──────────────────────────────────────────────────────────────┘

1. Certificate Request Creation
   ├─ User/Service creates CSR
   │  kubectl create -f csr.yaml
   │
   ▼
2. Approval Process
   ├─ Automatic Approval (Kubelet CSR)
   │  ├─ kubernetes.io/kube-apiserver-client-kubelet
   │  ├─ kubernetes.io/kubelet-serving
   │  └─ Event: CertificateRequestApproved ✅
   │
   ├─ Manual Approval (Admin)
   │  kubectl certificate approve <csr-name>
   │  └─ Event: CertificateRequestApproved ✅
   │
   └─ Denial
      kubectl certificate deny <csr-name>
      └─ Event: CertificateRequestDenied ⚠️
   ▼
3. Certificate Signing
   ├─ Controller Manager signs CSR
   ├─ Success: Certificate issued
   │  └─ Certificate available in .status.certificate
   │
   └─ Failure
      └─ Event: CertificateRequestFailed ⚠️
   ▼
4. Certificate Usage
   ├─ Client uses certificate for authentication
   ├─ Certificate rotation (before expiry)
   │  └─ New CSR created (back to step 1)
   │
   └─ Certificate expiry
      └─ Client auth fails, new CSR required
```

**证书类型与用途**:

| Signer Name | 用途 | 自动批准 | 有效期 |
|-------------|------|---------|--------|
| `kubernetes.io/kube-apiserver-client` | Client 证书 | ❌ | 1年 (默认) |
| `kubernetes.io/kube-apiserver-client-kubelet` | Kubelet Client | ✅ | 1年 |
| `kubernetes.io/kubelet-serving` | Kubelet Server | ✅ | 1年 |
| `kubernetes.io/legacy-unknown` | 遗留 CSR | ❌ | 1年 |

---

## 🔧 RBAC 故障排查技巧

### 1. 权限验证

```bash
# 验证当前用户权限
kubectl auth can-i create pods
kubectl auth can-i '*' '*' --all-namespaces

# 验证 ServiceAccount 权限
kubectl auth can-i list secrets \
  --as=system:serviceaccount:default:my-sa \
  -n production

# 验证 Group 权限
kubectl auth can-i delete deployments \
  --as=user1 \
  --as-group=developers
```

### 2. Role/ClusterRole 分析

```bash
# 查看用户绑定的角色
kubectl get rolebindings,clusterrolebindings -A \
  -o json | jq '.items[] | select(.subjects[]?.name=="user1") | {name: .metadata.name, role: .roleRef.name}'

# 查看 ServiceAccount 绑定的角色
kubectl get rolebindings,clusterrolebindings -A \
  -o json | jq '.items[] | select(.subjects[]?.name=="my-sa") | {namespace: .metadata.namespace, role: .roleRef.name}'

# 查看角色详细权限
kubectl describe clusterrole <role-name>
kubectl describe role <role-name> -n <namespace>
```

### 3. 审计日志分析

```bash
# 查看 RBAC 拒绝日志 (需启用 audit log)
grep "Forbidden" /var/log/kubernetes/audit/audit.log | jq '.user, .verb, .objectRef.resource'

# 查看特定用户的操作历史
grep "user1" /var/log/kubernetes/audit/audit.log | jq '.verb, .objectRef'
```

### 4. 常见 RBAC 模式

```yaml
---
# 模式 1: Namespace Admin
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: namespace-admin
  namespace: production
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: admin  # 内置聚合角色
subjects:
- kind: User
  name: team-lead
  apiGroup: rbac.authorization.k8s.io
---
# 模式 2: 只读权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-viewer
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view  # 内置角色
subjects:
- kind: Group
  name: developers
  apiGroup: rbac.authorization.k8s.io
---
# 模式 3: 自定义应用权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-deployer
  namespace: production
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "create", "update", "patch"]
- apiGroups: [""]
  resources: ["services", "configmaps"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-deployer-binding
  namespace: production
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: app-deployer
subjects:
- kind: ServiceAccount
  name: deployer-sa
  namespace: ci-cd
```

---

## 📚 相关文档链接

### Domain-33: Kubernetes Events 全域事件大全
- [01 - Pod 生命周期事件](./01-pod-lifecycle-events.md)
- [02 - Workload 控制器事件](./02-workload-controller-events.md)
- [03 - Node 与 Kubelet 事件](./03-node-kubelet-events.md)
- [05 - 网络 CNI 事件](./05-network-cni-events.md)
- [10 - 资源配额与限制事件](./10-resource-quota-limit-events.md)
- [11 - 扩缩容与 HPA 事件](./11-scaling-hpa-events.md)

### Domain-5: 网络深度解析
- [30 - Service Mesh 深度解析](../domain-5-networking/30-service-mesh-deep-dive.md)
- [35 - Gateway API 概览](../domain-5-networking/35-gateway-api-overview.md)

### Topic-Structural-Troubleshooting
- [01 - API Server 故障排查](../topic-structural-trouble-shooting/01-control-plane/01-apiserver-troubleshooting.md)
- [05 - Webhook Admission 故障排查](../topic-structural-trouble-shooting/01-control-plane/05-webhook-admission-troubleshooting.md)

### Topic-Dictionary
- [05 - 概念速查手册](../topic-dictionary/05-concept-reference.md)

---

## 🎯 最佳实践总结

### 1. 证书管理最佳实践
- ✅ 启用证书自动轮换 (kubelet `--rotate-certificates`)
- ✅ 监控证书过期时间 (提前 30 天告警)
- ✅ 使用 cert-manager 自动化证书管理
- ✅ 审计所有证书批准和拒绝操作

### 2. ServiceAccount 管理
- ✅ 每个应用使用独立 ServiceAccount
- ✅ 遵循最小权限原则
- ✅ v1.24+ 使用 TokenRequest API (bound tokens)
- ✅ 定期审计 ServiceAccount 权限

### 3. 准入控制策略
- ✅ 使用 ValidatingAdmissionPolicy (v1.30+) 替代 Webhook
- ✅ 设置合理的 Webhook timeout (10-30s)
- ✅ 生产环境 Webhook 使用 `failurePolicy: Ignore`
- ✅ 实施渐进式策略 (Warn → Audit → Enforce)

### 4. Pod Security 加固
- ✅ 所有 Namespace 启用 Pod Security Admission
- ✅ 默认使用 `baseline` 模式,敏感 Namespace 用 `restricted`
- ✅ 最小化豁免范围 (仅系统组件)
- ✅ 定期审计安全违规事件

### 5. RBAC 管理
- ✅ 使用 ClusterRole 聚合管理复杂权限
- ✅ 优先使用内置角色 (admin/edit/view)
- ✅ 避免使用 `cluster-admin` (除了紧急情况)
- ✅ 定期审计 ClusterRoleBinding

### 6. 监控与告警
- ✅ 监控 `FailedAdmission` 频率
- ✅ 告警 `CertificateRequestFailed` 事件
- ✅ 审计 `PodSecurityViolation` 趋势
- ✅ 监控 Webhook 响应时间

---

## 📈 事件监控查询

### Prometheus 查询示例

```promql
# 准入控制拒绝率
rate(apiserver_admission_webhook_rejection_count[5m])

# Certificate Controller 错误
rate(certificate_controller_sync_errors_total[5m])

# Pod Security 违规计数
sum(rate(pod_security_evaluations_total{decision="deny"}[5m])) by (policy_level)

# Webhook 延迟
histogram_quantile(0.99, 
  rate(apiserver_admission_webhook_admission_duration_seconds_bucket[5m])
)

# ServiceAccount Token 创建失败
rate(serviceaccount_controller_token_secret_create_errors_total[5m])
```

### kubectl 事件查询

```bash
# 查看所有安全相关事件
kubectl get events -A --field-selector type=Warning | \
  grep -E "FailedAdmission|PodSecurityViolation|FailedValidation|CertificateRequest"

# 统计 PodSecurityViolation 事件
kubectl get events -A --field-selector reason=PodSecurityViolation \
  -o json | jq '.items | length'

# 查看最近 1 小时的准入失败
kubectl get events -A --field-selector reason=FailedAdmission \
  --sort-by='.lastTimestamp' | tail -20

# 按 Namespace 统计违规
kubectl get events -A --field-selector reason=PodSecurityViolation \
  -o json | jq '.items | group_by(.involvedObject.namespace) | map({namespace: .[0].involvedObject.namespace, count: length})'
```

---

## 📝 总结

本文档详细记录了 Kubernetes 安全、准入控制与 RBAC 相关的 **14 类核心事件**:

1. **Certificate Management (3)**: 证书请求审批、签发全流程
2. **ServiceAccount & Token (4)**: SA Token 管理与清理
3. **ClusterRole Aggregation (1)**: 角色聚合自动更新
4. **Admission Control (3)**: Webhook 和 CEL 策略验证
5. **Pod Security (3)**: Pod Security Admission 标准实施

**核心要点**:
- 🔐 **证书管理**: 自动化轮换,监控过期
- 🎫 **Token 管理**: v1.24+ 使用 Bound Tokens
- 🚪 **准入控制**: 优先使用 ValidatingAdmissionPolicy (CEL)
- 🔒 **Pod Security**: v1.25+ 使用 PSA 替代 PSP
- 👥 **RBAC**: 最小权限原则,审计绑定关系

掌握这些事件是生产环境安全运维的关键能力!

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 13/15
