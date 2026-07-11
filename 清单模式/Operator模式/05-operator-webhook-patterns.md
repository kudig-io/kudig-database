---
title: Operator Webhook 模式
description: Validating/Mutating Admission Webhook 设计模式与证书管理
summary: ValidatingWebhook 和 MutatingWebhook 的设计模式、转换 Webhook、证书自动轮换及生产部署
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- webhook
- admission
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- 开发工程师
estimated_read_time: 12min
intent_queries:
- Admission Webhook 如何实现
- MutatingWebhook 配置
- ValidatingWebhook 设计
trigger_keywords:
- webhook
- admission
- validating
- mutating
- cert-manager
prerequisites:
- operator-basics
- tls-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator Webhook 模式

## 1. Webhook 类型对比

| 类型 | 作用 | 示例 |
|------|------|------|
| **MutatingWebhook** | 修改请求（可多链调用） | 注入默认值、sidecar 注入 |
| **ValidatingWebhook** | 验证请求（只读） | 业务规则验证 |
| **ConversionWebhook** | CRD 版本转换 | v1beta1 ↔ v1 |

## 2. ValidatingWebhookConfiguration

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: webapp-validator
  annotations:
    cert-manager.io/inject-ca-from: webapp-system/webapp-serving-cert
webhooks:
  - name: vwebapp.kb.io
    rules:
      - apiGroups: ["platform.example.com"]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["webapps"]
        scope: Namespaced
    failurePolicy: Fail        # Webhook 不可用时拒绝请求
    sideEffects: None          # 声明无副作用
    admissionReviewVersions: ["v1"]
    clientConfig:
      service:
        namespace: webapp-system
        name: webapp-webhook-service
        path: "/validate-platform-example-com-v1-webapp"
        port: 443
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values: ["kube-system", "webapp-system"]  # 排除系统命名空间
    timeoutSeconds: 10  # 超时后按 failurePolicy 处理
```

## 3. MutatingWebhookConfiguration

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: webapp-mutator
  annotations:
    cert-manager.io/inject-ca-from: webapp-system/webapp-serving-cert
webhooks:
  - name: mwebapp.kb.io
    rules:
      - apiGroups: ["platform.example.com"]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["webapps"]
    failurePolicy: Fail
    sideEffects: NoneOnDryRun
    admissionReviewVersions: ["v1"]
    reinvocationPolicy: IfNeeded  # 后续 webhook 修改对象时可再次调用
    clientConfig:
      service:
        namespace: webapp-system
        name: webapp-webhook-service
        path: "/mutate-platform-example-com-v1-webapp"
```

## 4. 证书管理（cert-manager 集成）

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: webapp-serving-cert
  namespace: webapp-system
spec:
  dnsNames:
    - webapp-webhook-service.webapp-system.svc
    - webapp-webhook-service.webapp-system.svc.cluster.local
  issuerRef:
    kind: Issuer
    name: webapp-selfsigned-issuer
  secretName: webhook-server-cert
---
apiVersion: cert-manager.io/v1
kind: Issuer
metadata:
  name: webapp-selfsigned-issuer
  namespace: webapp-system
spec:
  selfSigned: {}
```

## 5. Webhook 服务部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webhook-server
  namespace: webapp-system
spec:
  replicas: 2  # 多副本保证高可用
  selector:
    matchLabels:
      app: webhook-server
  template:
    metadata:
      labels:
        app: webhook-server
    spec:
      containers:
        - name: webhook
          image: registry.example.com/webapp-operator:v1.0.0
          args:
            - --webhook-port=9443
            - --webhook-cert-dir=/tmp/k8s-webhook-server/serving-certs
          ports:
            - containerPort: 9443
              name: webhook-server
              protocol: TCP
          volumeMounts:
            - mountPath: /tmp/k8s-webhook-server/serving-certs
              name: cert
              readOnly: true
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8081
            initialDelaySeconds: 5
      volumes:
        - name: cert
          secret:
            defaultMode: 420
            secretName: webhook-server-cert
---
apiVersion: v1
kind: Service
metadata:
  name: webapp-webhook-service
  namespace: webapp-system
spec:
  ports:
    - port: 443
      targetPort: webhook-server
  selector:
    app: webhook-server
```

## 6. Mutating Webhook Handler 示例

```go
func (w *WebappMutator) Mutate(ctx context.Context, req admission.Request) admission.Response {
    var webapp platformv1.WebApp
    if err := w.decoder.Decode(req, &webapp); err != nil {
        return admission.Errored(http.StatusBadRequest, err)
    }

    // 设置默认值
    if webapp.Spec.Replicas == 0 {
        webapp.Spec.Replicas = 1
    }
    if webapp.Spec.Strategy == "" {
        webapp.Spec.Strategy = "RollingUpdate"
    }

    // 序列化修改后的对象
    marshaled, err := json.Marshal(webapp)
    if err != nil {
        return admission.Errored(http.StatusInternalServerError, err)
    }
    return admission.PatchResponseFromRaw(req.Object.Raw, marshaled)
}
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| `failurePolicy: Fail` | 生产环境确保验证生效（但需确保 Webhook 可用） |
| `sideEffects: None` | 声明无副作用，优化 DryRun 性能 |
| `timeoutSeconds: 10` | 合理超时，避免请求堆积 |
| `namespaceSelector` | 排除系统命名空间，避免死锁 |
| 多副本部署 | 避免 Webhook 单点故障 |
| 证书自动轮换 | 使用 cert-manager 自动管理 |

## 8. 避免 Webhook 死锁

> ⚠️ **关键陷阱**：如果 Webhook 自身部署依赖的资源也被同一 Webhook 拦截，会导致循环死锁。

解决方案：为 Webhook 自身的 Namespace 添加标签排除（如 `webhookignore: "true"`），并在 `namespaceSelector` 中排除。

## Related

- [[清单模式/Operator模式/01-operator-cr-design-patterns|CRD 设计模式]]
- [[清单模式/YAML参考/24-admission-webhook-configuration|Admission Webhook 参考]]

## See Also

- [Kubernetes Admission Webhook 文档](https://kubernetes.io/docs/reference/access-authn-authz/extensible-admission-controllers/)
- [cert-manager 文档](https://cert-manager.io/docs/)

<!-- risk-assessed -->
