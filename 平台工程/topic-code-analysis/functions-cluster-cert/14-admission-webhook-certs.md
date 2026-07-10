---
title: Admission Webhook 证书体系 (topic-code-analysis)
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- apiserver
- kubelet
- istio
- opa
- operator
- webhook
- kserve
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Admission Webhook 证书体系 是什么
- 如何 Admission Webhook 证书体系
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- Admission
- Webhook
- 证书体系
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- service-mesh-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Admission Webhook 证书体系
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- apiserver
- kubelet
- istio
- opa
- operator
- webhook
- kserve
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 管理员
- 安全工程师
- 应用开发者
estimated_read_time: 5min
intent_queries:
- Kubernetes Admission Webhook 证书 caBundle 配置
- Webhook 证书体系 cert-manager cainjector 自动更新
- ValidatingWebhookConfiguration caBundle
- Webhook 证书故障排查 x509 unknown authority
- ValidatingAdmissionPolicy 无证书优势
trigger_keywords:
- Webhook
- caBundle
- cert-manager
- cainjector
- ValidatingWebhook
- MutatingWebhook
- failurePolicy
- certificate
- webhook-server-cert
- 证书轮换
related_domains:
- 集群基础
- 安全
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/ca-generation
- cluster-cert/apiserver-cert-flags
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Admission Webhook 证书体系

## 概述

Admission Webhook 是 Kubernetes 扩展准入控制的核心机制。与集群内部组件（如 API Server、kubelet）的证书由 kubeadm 统一管理不同，Webhook 服务端的证书通常由外部系统（如 cert-manager、自签脚本）管理，并通过 Webhook 配置的 `caBundle` 字段告知 API Server 如何验证。本文档分析 Webhook 证书的完整生命周期。

---

## 架构对比：集群证书 vs Webhook 证书

| 维度 | 集群组件证书 | Webhook 证书 |
|-----|-----------|-------------|
| 签发者 | kubeadm / kubernetes-ca | cert-manager / 自签 / 外部 CA |
| 存储位置 | `/etc/kubernetes/pki/` | Webhook Pod 的 Secret/Volume |
| API Server 验证方式 | `--client-ca-file` | Webhook 配置中的 `caBundle` |
| 轮换方式 | kubeadm renew / kubelet CSR | cert-manager / 手动更新 Secret |
| 有效期 | 1 年（默认） | 通常 90 天（cert-manager 默认） |

---

## Webhook 配置的证书字段

### ValidatingWebhookConfiguration / MutatingWebhookConfiguration

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: pod-security-policy
webhooks:
- name: validate-pod.example.com
  clientConfig:
    # 方式 1: 通过 Service 连接 Webhook
    service:
      name: webhook-service
      namespace: webhook-ns
      path: "/validate"
      port: 443
    # 方式 2: 直接通过 URL 连接（需确保网络可达）
    # url: "https://webhook.example.com:443/validate"
    
    # API Server 使用此 CA 验证 Webhook 的服务端证书
    caBundle: LS0tLS1CRUdJTi...  # ← Base64 编码的 CA 证书
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    operations: ["CREATE", "UPDATE"]
    resources: ["pods"]
  admissionReviewVersions: ["v1", "v1beta1"]
  sideEffects: None
  failurePolicy: Fail  # Ignore 或 Fail
```

**关键字段**：
- `clientConfig.caBundle` — API Server 验证 Webhook TLS 服务端证书时使用的 CA
- `clientConfig.service` / `clientConfig.url` — Webhook 服务端地址
- `failurePolicy` — Webhook 不可用时（包括证书验证失败）的处理策略

---

## API Server 对 Webhook 证书的验证逻辑

```go
// staging/src/k8s.io/apiserver/pkg/admission/plugin/webhook/config/client.go
func (cm *ClientManager) HookClient(cc *v1.WebhookClientConfig) (*rest.RESTClient, error) {
    // 1. 从 caBundle 构建 CA 证书池
    caBundle := cc.CABundle
    if len(caBundle) == 0 {
        return nil, errors.New("caBundle is empty")
    }
    
    caCertPool := x509.NewCertPool()
    caCertPool.AppendCertsFromPEM(caBundle)
    
    // 2. 创建 TLS 配置
    tlsConfig := &tls.Config{
        RootCAs:    caCertPool,
        ServerName: cc.Service.Name + "." + cc.Service.Namespace + ".svc",
    }
    
    // 3. 使用此 TLS 配置连接 Webhook
    transport := &http.Transport{
        TLSClientConfig: tlsConfig,
    }
    
    return rest.RESTClientFor(&rest.Config{
        Host:      "https://" + cc.Service.Name + "." + cc.Service.Namespace + ".svc:" + strconv.Itoa(int(cc.Service.Port)),
        Transport: transport,
    })
}
```

**验证规则**：
1. API Server 使用 `caBundle` 中的 CA 验证 Webhook 服务端证书
2. 证书必须包含匹配 `service.name.namespace.svc` 的 SAN
3. 如果 `failurePolicy: Fail`，证书验证失败会拒绝请求
4. 如果 `failurePolicy: Ignore`，证书验证失败会跳过该 Webhook

---

## Webhook 服务端证书的部署模式

### 模式 1: cert-manager 自动管理（推荐）

```yaml
# cert-manager Certificate 资源
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: webhook-cert
  namespace: webhook-ns
spec:
  secretName: webhook-server-cert
  issuerRef:
    name: selfsigned-issuer
    kind: ClusterIssuer
  dnsNames:
    - webhook-service.webhook-ns.svc
    - webhook-service.webhook-ns.svc.cluster.local
  usages:
    - digital signature
    - key encipherment
    - server auth
---
# Webhook Deployment 挂载证书
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webhook-server
  namespace: webhook-ns
spec:
  template:
    spec:
      containers:
      - name: webhook
        volumeMounts:
        - name: cert
          mountPath: /tmp/k8s-webhook-server/serving-certs
          readOnly: true
      volumes:
      - name: cert
        secret:
          secretName: webhook-server-cert
```

**cert-manager 的自动化**：
- 自动生成 RSA 2048 私钥和服务端证书
- 证书有效期默认 90 天，到期前 30 天自动续期
- 自动更新 Secret，Webhook Pod 通过 Volume 自动获取新证书
- **但不会自动更新 `caBundle`** — 需要额外配置（见下文）

### 模式 2: Operator 框架的证书注入

```go
// controller-runtime 的证书注入
import "sigs.k8s.io/controller-runtime/pkg/webhook"

// 自动将 Secret 中的 ca.crt 注入到 ValidatingWebhookConfiguration 的 caBundle
mgr.GetWebhookServer().Register("/validate", &webhook.Admission{Handler: validator})
```

**controller-runtime 的证书管理**：
- 自动生成自签 CA 和服务器证书
- 将 CA 证书自动写入 Webhook 配置的 `caBundle`
- 证书轮换时自动更新 `caBundle`
- 开发测试环境最便捷的方式

### 模式 3: 手动管理（生产不推荐）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 生成 CA
openssl genrsa -out webhook-ca.key 2048
openssl req -x509 -new -nodes -key webhook-ca.key -subj "/CN=webhook-ca" -days 3650 -out webhook-ca.crt

# 2. 生成 Webhook 服务端证书
openssl genrsa -out webhook-server.key 2048
openssl req -new -key webhook-server.key -subj "/CN=webhook-service.webhook-ns.svc" -out webhook-server.csr
openssl x509 -req -in webhook-server.csr -CA webhook-ca.crt -CAkey webhook-ca.key -CAcreateserial -out webhook-server.crt -days 365

# 3. 创建 Secret
kubectl create secret tls webhook-server-cert \
  --cert=webhook-server.crt \
  --key=webhook-server.key \
  -n webhook-ns

# 4. 更新 Webhook 配置的 caBundle
CA_BUNDLE=$(base64 -w0 webhook-ca.crt)
kubectl patch validatingwebhookconfiguration pod-security-policy \
  --type='json' -p='[{"op": "replace", "path": "/webhooks/0/clientConfig/caBundle", "value":"'${CA_BUNDLE}'"}]'
```
---

## caBundle 的自动更新问题

### 问题：cert-manager 更新 Secret 但不更新 caBundle

```
时间线:
T+0  : cert-manager 签发证书，写入 Secret
       Webhook 配置 caBundle 设置为当前 CA
       ✓ 正常工作

T+60d: cert-manager 轮换证书（同一 CA）
       Secret 中的 tls.crt 更新
       Webhook Pod 自动获取新证书
       ✓ 仍然正常工作（同一 CA）

T+365d: CA 证书即将过期，cert-manager 轮换 CA
        Secret 中的 ca.crt 变更
        Webhook 配置 caBundle 仍是旧 CA
        ✗ API Server 无法验证 Webhook 证书 → 请求被拒
```

### 解决方案 1: cert-manager 的 cainjector

```yaml
# 使用 cert-manager 的 cainjector 自动注入 caBundle
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: my-webhook
  annotations:
    cert-manager.io/inject-ca-from: webhook-ns/webhook-cert
webhooks:
- name: validate.example.com
  clientConfig:
    service:
      name: webhook-service
      namespace: webhook-ns
    # caBundle 留空，由 cainjector 自动填充
    caBundle: ""
```

**原理**：
- cert-manager 的 `cainjector` 组件监控带 `cert-manager.io/inject-ca-from` 注解的资源
- 自动从指定的 Certificate 中提取 CA，写入 `caBundle`
- CA 轮换时自动更新

### 解决方案 2: Operator 自动同步

```go
// Operator 在 Reconcile 循环中同步 caBundle
func (r *WebhookReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 1. 从管理的 Secret 读取 CA
    caCert := secret.Data["ca.crt"]
    
    // 2. 读取 Webhook 配置
    webhookConfig := &admissionregistrationv1.ValidatingWebhookConfiguration{}
    r.Get(ctx, types.NamespacedName{Name: "my-webhook"}, webhookConfig)
    
    // 3. 如果 caBundle 不匹配，更新
    if !bytes.Equal(webhookConfig.Webhooks[0].ClientConfig.CABundle, caCert) {
        webhookConfig.Webhooks[0].ClientConfig.CABundle = caCert
        r.Update(ctx, webhookConfig)
    }
    
    return ctrl.Result{RequeueAfter: 1 * time.Hour}, nil
}
```

---

## Webhook 证书故障排查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Webhook 配置的 caBundle
kubectl get validatingwebhookconfiguration -o yaml | grep -A2 caBundle

# 2. 解码并查看 caBundle 内容
kubectl get validatingwebhookconfiguration my-webhook -o jsonpath='{.webhooks[0].clientConfig.caBundle}' | base64 -d | openssl x509 -noout -text

# 3. 检查 Webhook Secret 中的证书
kubectl get secret webhook-server-cert -n webhook-ns -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -text
kubectl get secret webhook-server-cert -n webhook-ns -o jsonpath='{.data.ca\.crt}' | base64 -d | openssl x509 -noout -text

# 4. 从 API Server 角度测试 Webhook 连接
# 进入 API Server 容器或使用相同网络命名空间
curl -v --cacert <(kubectl get validatingwebhookconfiguration my-webhook -o jsonpath='{.webhooks[0].clientConfig.caBundle}' | base64 -d) \
  https://webhook-service.webhook-ns.svc:443/validate

# 5. 查看 API Server 日志中的 Webhook 错误
kubectl logs -n kube-system kube-apiserver-<node> | grep -i "webhook|x509"

# 6. 临时将 failurePolicy 改为 Ignore 绕过
kubectl patch validatingwebhookconfiguration my-webhook \
  --type='json' -p='[{"op": "replace", "path": "/webhooks/0/failurePolicy", "value":"Ignore"}]'
```
---

## Webhook 证书常见问题

| 现象 | 根因 | 解决 |
|-----|------|------|
| `failed to call webhook: Post "...": x509: certificate signed by unknown authority` | caBundle 与 Webhook 服务端证书不匹配 | 更新 caBundle 为正确的 CA |
| `failed to call webhook: Post "...": x509: certificate is valid for X, not Y` | Webhook 证书 SAN 不包含 Service DNS 名 | 重新签发包含正确 SAN 的证书 |
| `failed to call webhook: timeout` | Webhook Pod 不可达或证书握手超时 | 检查 Pod 状态和网络连通性 |
| `Internal error occurred: failed calling webhook...` | Webhook 服务端证书过期 | 续期或重新签发证书 |
| 资源创建被静默拒绝 | `failurePolicy: Ignore` 且 Webhook 证书错误 | 改为 `Fail` 定位问题，修复证书 |

---

## ValidatingAdmissionPolicy 与 Webhook 的证书差异

Kubernetes v1.30+ 引入 **ValidatingAdmissionPolicy**（内置 CEL 表达式验证），与 Admission Webhook 有本质区别：

| 特性 | ValidatingAdmissionPolicy | Admission Webhook |
|-----|--------------------------|-------------------|
| 执行位置 | API Server 内部 | 外部 HTTP 服务 |
| 证书需求 | **无** | 需要 TLS 服务端证书 |
| 网络依赖 | 无 | 需要网络可达 Webhook 服务 |
| 延迟 | 低（本地执行） | 高（HTTP 调用） |
| 配置 | `ValidatingAdmissionPolicy` + `ValidatingAdmissionPolicyBinding` | `ValidatingWebhookConfiguration` |

**关键结论**：
- 迁移到 ValidatingAdmissionPolicy 可以**完全消除 Webhook 证书管理的负担**
- 但复杂验证逻辑仍需要 Webhook

---

## 与集群证书体系的交叉点

虽然 Webhook 证书独立于集群 PKI，但在以下场景会产生交叉：

1. **Pod Security Admission** — 内置准入控制器，不使用 Webhook，不依赖外部证书
2. **OPA/Gatekeeper** — 作为 ValidatingWebhook 部署，其证书完全独立于集群证书
3. **Istio / Service Mesh** — Sidecar 可能会 mTLS 终止 Webhook 流量，需要额外配置
4. **自定义 CA 与集群 CA 的关系** — 强烈建议 Webhook 使用独立的 CA，不要用 kubernetes-ca 签发 Webhook 证书（避免 CA 轮换影响 Webhook）

## Related

- [[reference|#reference Hub]] — tag hub

- [[系统基础/topic-cheat-sheet/go.md|go]]
- [[系统基础/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cert-manager.md|cert-manager]]
- [[entities/kserve.md|kserve]]


<!-- risk-assessed -->
