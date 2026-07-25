---
title: 03 - 准入控制器(Webhook)配置与实现
description: '# 03 - 准入控制器(Webhook)配置与实现'
summary: 'requiredDuringSchedulingIgnoredDuringExecution:'
category: extensions
tags:
- k8s
- extensions
- crd
- operator
- webhook
- etcd
- controller-manager
- prometheus
- grafana
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 开发工程师
- 架构师
estimated_read_time: 5min
intent_queries:
- 准入控制器(Webhook)配置与实现 是什么
- 如何 准入控制器(Webhook)配置与实现
- Kubernetes 10 extensions 最佳实践
trigger_keywords:
- 准入控制器
- Webhook
- 配置与实现
- extensions
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- mysql-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../平台工程/
  label: '相关知识域: 平台工程'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 03 - 准入控制器(Webhook)配置与实现

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/docs/reference/access-authn-authz/admission-controllers/](https://kubernetes.io/docs/reference/access-authn-authz/admission-controllers/)

<!-- chunk: 准入控制器架构与原理 -->
## 准入控制器架构与原理

### 准入控制流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Kubernetes API Server                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────┐    ┌─────────┐ │
│  │  认证阶段    │───▶│   授权阶段    │───▶│   准入控制阶段   │───▶│ 持久化  │ │
│  │ (AuthN)     │    │  (AuthZ)     │    │  (Admission)    │    │ (etcd)  │ │
│  └─────────────┘    └──────────────┘    └─────────────────┘    └─────────┘ │
│                                    │                                        │
│                                    ▼                                        │
│                    ┌─────────────────────────────┐                          │
│                    │    准入控制器链              │                          │
│                    │                             │                          │
│                    │  ┌──────────────────────┐   │                          │
│                    │  │  内置准入控制器      │   │                          │
│                    │  │  (Mutating)          │   │                          │
│                    │  └──────────────────────┘   │                          │
│                    │              │               │                          │
│                    │              ▼               │                          │
│                    │  ┌──────────────────────┐   │                          │
│                    │  │  自定义Webhook       │   │                          │
│                    │  │  (Mutating/Validating)│  │                          │
│                    │  └──────────────────────┘   │                          │
│                    └─────────────────────────────┘                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 准入控制器类型对比

| 类型 | 执行时机 | 修改能力 | 验证能力 | 典型用途 |
|------|----------|----------|----------|----------|
| **Mutating** | 创建/更新前 | ✅ 可修改对象 | ✅ 可验证 | 注入sidecar、设置默认值 |
| **Validating** | 创建/更新前 | ❌ 只读 | ✅ 可验证 | 策略验证、安全检查 |
| **内置控制器** | 固定顺序 | 依控制器而定 | 依控制器而定 | 基础验证 |

<!-- chunk: 企业级Webhook最佳实践 -->
## 企业级Webhook最佳实践

### 1. 高可用部署架构

```yaml
# webhook-ha-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: admission-webhook
  namespace: kube-system
spec:
  replicas: 3
  selector:
    matchLabels:
      app: admission-webhook
  template:
    metadata:
      labels:
        app: admission-webhook
    spec:
      serviceAccountName: admission-webhook
      containers:
      - name: webhook-server
        image: mysql-webhook:v1.0.0
        args:
        - --tls-cert-file=/etc/webhook/certs/tls.crt
        - --tls-private-key-file=/etc/webhook/certs/tls.key
        - --port=8443
        ports:
        - containerPort: 8443
          name: webhook
        readinessProbe:
          httpGet:
            path: /readyz
            port: 8443
            scheme: HTTPS
          initialDelaySeconds: 5
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8443
            scheme: HTTPS
          initialDelaySeconds: 15
          periodSeconds: 20
        resources:
          limits:
            cpu: 100m
            memory: 128Mi
          requests:
            cpu: 50m
            memory: 64Mi
        volumeMounts:
        - name: certs
          mountPath: /etc/webhook/certs
          readOnly: true
      volumes:
      - name: certs
        secret:
          secretName: admission-webhook-certs
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - admission-webhook
            topologyKey: kubernetes.io/hostname
```

### 2. 安全加固配置

```yaml
# security-hardening.yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: admission-webhook
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: admission-webhook-role
rules:
# 最小权限原则
- apiGroups: ["admissionregistration.k8s.io"]
  resources: ["mutatingwebhookconfigurations", "validatingwebhookconfigurations"]
  verbs: ["get", "list", "watch", "update", "patch"]
- apiGroups: [""]
  resources: ["namespaces", "pods", "services"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["database.example.com"]
  resources: ["mysqlclusters"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: admission-webhook-rolebinding
subjects:
- kind: ServiceAccount
  name: admission-webhook
  namespace: kube-system
roleRef:
  kind: ClusterRole
  name: admission-webhook-role
  apiGroup: rbac.authorization.k8s.io
```

```go
// security-context.go - 容器安全上下文
func getSecureContainerSpec() corev1.Container {
    return corev1.Container{
        SecurityContext: &corev1.SecurityContext{
            AllowPrivilegeEscalation: pointer.BoolPtr(false),
            ReadOnlyRootFilesystem:   pointer.BoolPtr(true),
            RunAsNonRoot:             pointer.BoolPtr(true),
            RunAsUser:                pointer.Int64Ptr(1000),
            Capabilities: &corev1.Capabilities{
                Drop: []corev1.Capability{"ALL"},
            },
        },
        // 只读挂载证书
        VolumeMounts: []corev1.VolumeMount{
            {
                Name:      "certs",
                MountPath: "/etc/webhook/certs",
                ReadOnly:  true,
            },
            {
                Name:      "tmp",
                MountPath: "/tmp",
            },
        },
    }
}
```

### 3. 证书管理与自动轮换

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

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
#!/bin/bash
# cert-manager.sh - Webhook证书管理脚本

WEBHOOK_NAME="mysql-admission-webhook"
NAMESPACE="kube-system"
SECRET_NAME="admission-webhook-certs"
SERVICE_NAME="admission-webhook-service"

# 生成证书
generate_certificates() {
    echo "🔐 生成Webhook证书..."
    
    # 创建临时目录
    TEMP_DIR=$(mktemp -d)
    cd ${TEMP_DIR}
    
    # 生成CA证书
    openssl genrsa -out ca.key 2048
    openssl req -x509 -new -nodes -key ca.key -days 365 -out ca.crt -subj "/CN=admission-webhook-ca"
    
    # 生成服务端证书
    openssl genrsa -out tls.key 2048
    openssl req -new -key tls.key -out tls.csr -subj "/CN=${SERVICE_NAME}.${NAMESPACE}.svc"
    
    # 创建证书配置
    cat > csr.conf <<EOF
[req]
default_bits = 2048
prompt = no
default_md = sha256
req_extensions = req_ext
distinguished_name = dn

[dn]
CN = ${SERVICE_NAME}.${NAMESPACE}.svc

[req_ext]
subjectAltName = @alt_names

[alt_names]
DNS.1 = ${SERVICE_NAME}
DNS.2 = ${SERVICE_NAME}.${NAMESPACE}
DNS.3 = ${SERVICE_NAME}.${NAMESPACE}.svc
DNS.4 = ${SERVICE_NAME}.${NAMESPACE}.svc.cluster.local
EOF

    # 签发证书
    openssl x509 -req -in tls.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out tls.crt -days 365 -extensions req_ext -extfile csr.conf
    
    # 创建Kubernetes Secret
    kubectl create secret generic ${SECRET_NAME} \
        --from-file=tls.crt=./tls.crt \
        --from-file=tls.key=./tls.key \
        --from-file=ca.crt=./ca.crt \
        -n ${NAMESPACE} --dry-run=client -o yaml | kubectl apply -f -
    
    # 清理临时文件
    cd - && rm -rf ${TEMP_DIR}  # ⚠️ 删除系统/数据文件
    
    echo "✅ 证书生成完成"
}

# 配置Webhook
configure_webhook() {
    echo "⚙️  配置Webhook..."
    
    # 获取CA Bundle
    CA_BUNDLE=$(kubectl get secret ${SECRET_NAME} -n ${NAMESPACE} -o jsonpath='{.data.ca\.crt}' | base64 -d | tr -d '\n')
    
    # 更新MutatingWebhookConfiguration
    cat <<EOF | kubectl apply -f -
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: ${WEBHOOK_NAME}-mutating
webhooks:
- name: mysql-mutating.example.com
  clientConfig:
    service:
      name: ${SERVICE_NAME}
      namespace: ${NAMESPACE}
      path: "/mutate-database-example-com-v1beta1-mysqlcluster"
      port: 443
    caBundle: ${CA_BUNDLE}
  rules:
  - operations: ["CREATE", "UPDATE"]
    apiGroups: ["database.example.com"]
    apiVersions: ["v1beta1"]
    resources: ["mysqlclusters"]
  failurePolicy: Fail
  sideEffects: None
  admissionReviewVersions: ["v1", "v1beta1"]
EOF

    # 更新ValidatingWebhookConfiguration
    cat <<EOF | kubectl apply -f -
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: ${WEBHOOK_NAME}-validating
webhooks:
- name: mysql-validating.example.com
  clientConfig:
    service:
      name: ${SERVICE_NAME}
      namespace: ${NAMESPACE}
      path: "/validate-database-example-com-v1beta1-mysqlcluster"
      port: 443
    caBundle: ${CA_BUNDLE}
  rules:
  - operations: ["CREATE", "UPDATE"]
    apiGroups: ["database.example.com"]
    apiVersions: ["v1beta1"]
    resources: ["mysqlclusters"]
  failurePolicy: Fail
  sideEffects: None
  admissionReviewVersions: ["v1", "v1beta1"]
EOF

    echo "✅ Webhook配置完成"
}

# 证书轮换
rotate_certificates() {
    echo "🔄 执行证书轮换..."
    
    # 备份当前证书
    kubectl get secret ${SECRET_NAME} -n ${NAMESPACE} -o yaml > backup-${SECRET_NAME}-$(date +%Y%m%d-%H%M%S).yaml
    
    # 生成新证书
    generate_certificates
    
    # 重启Webhook Pod以加载新证书
    kubectl rollout restart deployment/admission-webhook -n ${NAMESPACE}
    
    # 等待Pod就绪
    kubectl rollout status deployment/admission-webhook -n ${NAMESPACE} --timeout=300s
    
    echo "✅ 证书轮换完成"
}

# 验证证书有效性
validate_certificates() {
    echo "🔍 验证证书有效性..."
    
    # 检查证书是否存在
    if ! kubectl get secret ${SECRET_NAME} -n ${NAMESPACE} >/dev/null 2>&1; then
        echo "❌ 证书Secret不存在"
        return 1
    fi
    
    # 检查Webhook配置
    if ! kubectl get mutatingwebhookconfiguration ${WEBHOOK_NAME}-mutating >/dev/null 2>&1; then
        echo "❌ MutatingWebhookConfiguration不存在"
        return 1
    fi
    
    if ! kubectl get validatingwebhookconfiguration ${WEBHOOK_NAME}-validating >/dev/null 2>&1; then
        echo "❌ ValidatingWebhookConfiguration不存在"
        return 1
    fi
    
    # 检查Pod状态
    unhealthy_pods=$(kubectl get pods -n ${NAMESPACE} -l app=admission-webhook --no-headers | grep -v Running | wc -l)
    if [ ${unhealthy_pods} -gt 0 ]; then
        echo "❌ 发现${unhealthy_pods}个非Running状态的Pod"
        return 1
    fi
    
    echo "✅ 证书验证通过"
    return 0
}

# 主函数
main() {
    case "${1:-install}" in
        "install")
            generate_certificates
            configure_webhook
            validate_certificates
            ;;
        "rotate")
            rotate_certificates
            validate_certificates
            ;;
        "validate")
            validate_certificates
            ;;
        *)
            echo "使用方法: $0 {install|rotate|validate}"
            exit 1
            ;;
    esac
}

main "$@"
```
### 4. 性能优化与监控

```go
// metrics.go - Webhook性能监控
package metrics

import (
    "github.com/prometheus/client_golang/prometheus"
    "sigs.k8s.io/controller-runtime/pkg/metrics"
)

var (
    // Webhook请求计数
    WebhookRequestTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "admission_webhook_requests_total",
            Help: "Total number of admission webhook requests",
        },
        []string{"webhook", "operation", "result"},
    )
    
    // Webhook请求延迟
    WebhookRequestDuration = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name:    "admission_webhook_request_duration_seconds",
            Help:    "Duration of admission webhook requests",
            Buckets: prometheus.ExponentialBuckets(0.001, 2, 15),
        },
        []string{"webhook", "operation"},
    )
    
    // Webhook拒绝率
    WebhookRejectionRate = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "admission_webhook_rejection_rate",
            Help: "Rate of rejected admission requests",
        },
        []string{"webhook"},
    )
)

func init() {
    metrics.Registry.MustRegister(
        WebhookRequestTotal,
        WebhookRequestDuration,
        WebhookRejectionRate,
    )
}

// 在Webhook处理器中使用
func recordMetrics(webhookName, operation string, duration float64, allowed bool) {
    result := "allowed"
    if !allowed {
        result = "rejected"
    }
    
    WebhookRequestTotal.WithLabelValues(webhookName, operation, result).Inc()
    WebhookRequestDuration.WithLabelValues(webhookName, operation).Observe(duration)
    
    // 计算拒绝率
    totalRequests := prometheus.MustNewConstMetric(
        WebhookRequestTotal.Desc(), prometheus.CounterValue, 1,
        webhookName, operation, "",
    )
    
    rejectedRequests := prometheus.MustNewConstMetric(
        WebhookRequestTotal.Desc(), prometheus.CounterValue, 0,
        webhookName, operation, "rejected",
    )
    
    if totalRequests > 0 {
        rejectionRate := float64(rejectedRequests) / float64(totalRequests)
        WebhookRejectionRate.WithLabelValues(webhookName).Set(rejectionRate)
    }
}
```

### 5. 问题排除与调试

```yaml
# debugging-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: admission-webhook-debug-config
  namespace: kube-system
data:
  log-level: "debug"
  enable-profiling: "true"
  request-logging: "true"
  audit-logging: "true"
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# webhook-debug.sh - Webhook故障诊断脚本

WEBHOOK_NAME="mysql-admission-webhook"
NAMESPACE="kube-system"

diagnose_webhook_issues() {
    echo "=== Webhook诊断报告 ==="
    
    # 1. 检查Webhook配置
    echo "1. 检查Webhook配置:"
    kubectl get mutatingwebhookconfiguration ${WEBHOOK_NAME}-mutating -o wide
    kubectl get validatingwebhookconfiguration ${WEBHOOK_NAME}-validating -o wide
    
    # 2. 检查证书状态
    echo "2. 检查证书状态:"
    kubectl get secret admission-webhook-certs -n ${NAMESPACE} -o yaml
    
    # 3. 检查Pod状态
    echo "3. 检查Pod状态:"
    kubectl get pods -n ${NAMESPACE} -l app=admission-webhook -o wide
    
    # 4. 检查服务状态
    echo "4. 检查服务状态:"
    kubectl get service admission-webhook-service -n ${NAMESPACE}
    
    # 5. 查看日志
    echo "5. 查看最近日志:"
    kubectl logs -n ${NAMESPACE} -l app=admission-webhook --tail=100
    
    # 6. 检查事件
    echo "6. 检查相关事件:"
    kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=admission-webhook-service
    
    echo "=== 诊断完成 ==="
}

test_webhook_functionality() {
    echo "=== Webhook功能测试 ==="
    
    # 创建测试资源
    cat <<EOF | kubectl apply -f -
apiVersion: database.example.com/v1beta1
kind: MySQLCluster
metadata:
  name: test-cluster
  namespace: default
spec:
  replicas: 1
  version: "8.0"
  storage:
    size: "10Gi"
EOF

    # 检查资源是否被正确处理
    kubectl get mysqlcluster test-cluster -o yaml
    
    # 清理测试资源
    kubectl delete mysqlcluster test-cluster
    
    echo "=== 功能测试完成 ==="
}

# 执行完整诊断
perform_complete_diagnostics() {
    diagnose_webhook_issues
    test_webhook_functionality
    
    echo "建议检查点:"
    echo "1. 证书是否有效且未过期"
    echo "2. Webhook服务是否可达"
    echo "3. RBAC权限是否正确配置"
    echo "4. 网络策略是否允许通信"
    echo "5. 资源配额是否充足"
}

perform_complete_diagnostics
```
### 6. 生产环境部署清单

```markdown
# Webhook生产环境部署清单

<!-- chunk: 🔧 部署前检查 -->
## 🔧 部署前检查
- [ ] Kubernetes版本兼容性验证 (v1.25+)
- [ ] 网络连通性测试
- [ ] RBAC权限配置验证
- [ ] 证书有效期检查 (>90天)
- [ ] 资源配额评估

<!-- chunk: 🚀 部署步骤 -->
## 🚀 部署步骤
1. [ ] 生成并部署证书
2. [ ] 部署Webhook服务
3. [ ] 配置MutatingWebhook
4. [ ] 配置ValidatingWebhook
5. [ ] 验证部署结果

<!-- chunk: 📊 监控配置 -->
## 📊 监控配置
- [ ] Prometheus指标集成
- [ ] Grafana仪表板配置
- [ ] 告警规则设置
- [ ] 日志收集配置

<!-- chunk: 🔒 安全配置 -->
## 🔒 安全配置
- [ ] 网络策略实施
- [ ] 最小权限RBAC
- [ ] 证书自动轮换
- [ ] 审计日志启用

<!-- chunk: 🔄 运维流程 -->
## 🔄 运维流程
- [ ] 定期证书轮换
- [ ] 性能监控告警
- [ ] 故障诊断预案
- [ ] 版本升级流程
```

### 1. 项目结构初始化

```bash
# 创建webhook项目
mkdir mysql-webhook && cd mysql-webhook
go mod init github.com/example/mysql-webhook

# 初始化kubebuilder项目
kubebuilder init --domain example.com --repo github.com/example/mysql-webhook

# 创建webhook
kubebuilder create webhook --group database --version v1beta1 --kind MySQLCluster --programmatic-validation --defaulting
```

### 2. Webhook服务器实现

```go
// main.go
package main

import (
    "crypto/tls"
    "flag"
    "fmt"
    "net/http"
    "os"
    "time"

    "github.com/go-logr/logr"
    admissionv1 "k8s.io/api/admission/v1"
    admissionv1beta1 "k8s.io/api/admission/v1beta1"
    corev1 "k8s.io/api/core/v1"
    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/apimachinery/pkg/runtime/serializer"
    "k8s.io/client-go/kubernetes"
    "k8s.io/client-go/rest"
    "k8s.io/client-go/tools/clientcmd"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/certwatcher"
    "sigs.k8s.io/controller-runtime/pkg/log/zap"
    "sigs.k8s.io/controller-runtime/pkg/webhook"
    "sigs.k8s.io/controller-runtime/pkg/webhook/admission"

    databasev1beta1 "github.com/example/mysql-webhook/api/v1beta1"
    "github.com/example/mysql-webhook/webhooks"
)

var (
    scheme = runtime.NewScheme()
    codecs = serializer.NewCodecFactory(scheme)
)

func init() {
    _ = databasev1beta1.AddToScheme(scheme)
    _ = admissionv1.AddToScheme(scheme)
    _ = admissionv1beta1.AddToScheme(scheme)
    _ = corev1.AddToScheme(scheme)
}

func main() {
    var metricsAddr string
    var enableLeaderElection bool
    var probeAddr string
    var certDir string
    var port int
    var tlsMinVersion string

    flag.StringVar(&metricsAddr, "metrics-bind-address", ":8080", "The address the metric endpoint binds to.")
    flag.StringVar(&probeAddr, "health-probe-bind-address", ":8081", "The address the probe endpoint binds to.")
    flag.BoolVar(&enableLeaderElection, "leader-elect", false,
        "Enable leader election for controller manager. "+
            "Enabling this will ensure there is only one active controller manager.")
    flag.StringVar(&certDir, "cert-dir", "/tmp/k8s-webhook-server/serving-certs", "The directory that contains the server key and certificate.")
    flag.IntVar(&port, "port", 9443, "The port that the webhook server serves at.")
    flag.StringVar(&tlsMinVersion, "tls-min-version", "1.2", "Minimum TLS version")

    opts := zap.Options{
        Development: true,
    }
    opts.BindFlags(flag.CommandLine)
    flag.Parse()

    ctrl.SetLogger(zap.New(zap.UseFlagOptions(&opts)))

    // Setup manager
    mgr, err := ctrl.NewManager(ctrl.GetConfigOrDie(), ctrl.Options{
        Scheme:                 scheme,
        MetricsBindAddress:     metricsAddr,
        Port:                   port,
        HealthProbeBindAddress: probeAddr,
        LeaderElection:         enableLeaderElection,
        LeaderElectionID:       "mysql-webhook.example.com",
    })
    if err != nil {
        setupLog.Error(err, "unable to start manager")
        os.Exit(1)
    }

    // Setup webhook server
    hookServer := mgr.GetWebhookServer()
    hookServer.CertDir = certDir
    hookServer.Port = port

    // Register webhooks
    if err = (&databasev1beta1.MySQLCluster{}).SetupWebhookWithManager(mgr); err != nil {
        setupLog.Error(err, "unable to create webhook", "webhook", "MySQLCluster")
        os.Exit(1)
    }

    // Add health checks
    if err := mgr.AddHealthzCheck("healthz", healthz.Ping); err != nil {
        setupLog.Error(err, "unable to set up health check")
        os.Exit(1)
    }
    if err := mgr.AddReadyzCheck("readyz", healthz.Ping); err != nil {
        setupLog.Error(err, "unable to set up ready check")
        os.Exit(1)
    }

    setupLog.Info("starting manager")
    if err := mgr.Start(ctrl.SetupSignalHandler()); err != nil {
        setupLog.Error(err, "problem running manager")
        os.Exit(1)
    }
}

var setupLog = ctrl.Log.WithName("setup")
```

### 3. 默认值注入Webhook (Mutating)

```go
// webhooks/mysqlcluster_mutating.go
package webhooks

import (
    "context"
    "fmt"

    "k8s.io/apimachinery/pkg/runtime"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/webhook"
    "sigs.k8s.io/controller-runtime/pkg/webhook/admission"

    databasev1beta1 "github.com/example/mysql-webhook/api/v1beta1"
)

// MySQLClusterMutator handles mutating webhooks for MySQLCluster
type MySQLClusterMutator struct {
    decoder *admission.Decoder
}

//+kubebuilder:webhook:path=/mutate-database-example-com-v1beta1-mysqlcluster,mutating=true,failurePolicy=fail,sideEffects=None,groups=database.example.com,resources=mysqlclusters,verbs=create;update,versions=v1beta1,name=mmysqlcluster.kb.io,admissionReviewVersions={v1,v1beta1}

func (m *MySQLClusterMutator) SetupWebhookWithManager(mgr ctrl.Manager) error {
    return ctrl.NewWebhookManagedBy(mgr).
        For(&databasev1beta1.MySQLCluster{}).
        WithDefaulter(m).
        Complete()
}

// Default implements admission.CustomDefaulter
func (m *MySQLClusterMutator) Default(ctx context.Context, obj runtime.Object) error {
    cluster, ok := obj.(*databasev1beta1.MySQLCluster)
    if !ok {
        return fmt.Errorf("expected a MySQLCluster but got a %T", obj)
    }

    log := ctrl.LoggerFrom(ctx)
    log.Info("Defaulting MySQLCluster", "name", cluster.Name, "namespace", cluster.Namespace)

    // Set default replicas
    if cluster.Spec.Replicas <= 0 {
        cluster.Spec.Replicas = 1
        log.Info("Set default replicas", "replicas", cluster.Spec.Replicas)
    }

    // Set default version
    if cluster.Spec.Version == "" {
        cluster.Spec.Version = "8.0"
        log.Info("Set default version", "version", cluster.Spec.Version)
    }

    // Set default storage class
    if cluster.Spec.Storage.Class == "" {
        cluster.Spec.Storage.Class = "fast-ssd"
        log.Info("Set default storage class", "class", cluster.Spec.Storage.Class)
    }

    // Inject labels
    if cluster.Labels == nil {
        cluster.Labels = make(map[string]string)
    }
    cluster.Labels["managed-by"] = "mysql-operator"
    cluster.Labels["mysql-version"] = cluster.Spec.Version

    // Inject annotations
    if cluster.Annotations == nil {
        cluster.Annotations = make(map[string]string)
    }
    cluster.Annotations["mysql-operator.example.com/created-at"] = "2024-01-01T00:00:00Z"

    return nil
}
```

### 4. 验证Webhook (Validating)

```go
// webhooks/mysqlcluster_validating.go
package webhooks

import (
    "context"
    "fmt"
    "regexp"

    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/apimachinery/pkg/util/validation/field"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/webhook"
    "sigs.k8s.io/controller-runtime/pkg/webhook/admission"

    databasev1beta1 "github.com/example/mysql-webhook/api/v1beta1"
)

// MySQLClusterValidator handles validating webhooks for MySQLCluster
type MySQLClusterValidator struct {
    decoder *admission.Decoder
}

//+kubebuilder:webhook:path=/validate-database-example-com-v1beta1-mysqlcluster,mutating=false,failurePolicy=fail,sideEffects=None,groups=database.example.com,resources=mysqlclusters,verbs=create;update,versions=v1beta1,name=vmysqlcluster.kb.io,admissionReviewVersions={v1,v1beta1}

func (v *MySQLClusterValidator) SetupWebhookWithManager(mgr ctrl.Manager) error {
    return ctrl.NewWebhookManagedBy(mgr).
        For(&databasev1beta1.MySQLCluster{}).
        WithValidator(v).
        Complete()
}

// ValidateCreate implements admission.CustomValidator
func (v *MySQLClusterValidator) ValidateCreate(ctx context.Context, obj runtime.Object) (admission.Warnings, error) {
    cluster, ok := obj.(*databasev1beta1.MySQLCluster)
    if !ok {
        return nil, fmt.Errorf("expected a MySQLCluster but got a %T", obj)
    }

    log := ctrl.LoggerFrom(ctx)
    log.Info("Validating MySQLCluster creation", "name", cluster.Name, "namespace", cluster.Namespace)

    var allErrs field.ErrorList

    // Validate name
    if len(cluster.Name) > 63 {
        allErrs = append(allErrs, field.Invalid(
            field.NewPath("metadata").Child("name"),
            cluster.Name,
            "name must be no more than 63 characters"))
    }

    // Validate replicas
    if cluster.Spec.Replicas <= 0 {
        allErrs = append(allErrs, field.Invalid(
            field.NewPath("spec").Child("replicas"),
            cluster.Spec.Replicas,
            "replicas must be greater than 0"))
    }

    if cluster.Spec.Replicas > 10 {
        allErrs = append(allErrs, field.Invalid(
            field.NewPath("spec").Child("replicas"),
            cluster.Spec.Replicas,
            "replicas must be no more than 10"))
    }

    // Validate version
    validVersions := map[string]bool{
        "5.7": true,
        "8.0": true,
    }
    if !validVersions[cluster.Spec.Version] {
        allErrs = append(allErrs, field.NotSupported(
            field.NewPath("spec").Child("version"),
            cluster.Spec.Version,
            []string{"5.7", "8.0"}))
    }

    // Validate storage size format
    sizeRegex := regexp.MustCompile(`^[0-9]+Gi$`)
    if !sizeRegex.MatchString(cluster.Spec.Storage.Size) {
        allErrs = append(allErrs, field.Invalid(
            field.NewPath("spec").Child("storage").Child("size"),
            cluster.Spec.Storage.Size,
            "storage size must be in format like '100Gi'"))
    }

    // Validate backup configuration
    if cluster.Spec.Backup != nil && cluster.Spec.Backup.Enabled {
        if cluster.Spec.Backup.Schedule == "" {
            allErrs = append(allErrs, field.Required(
                field.NewPath("spec").Child("backup").Child("schedule"),
                "backup schedule is required when backup is enabled"))
        }
    }

    if len(allErrs) == 0 {
        return nil, nil
    }

    return nil, allErrs.ToAggregate()
}

// ValidateUpdate implements admission.CustomValidator
func (v *MySQLClusterValidator) ValidateUpdate(ctx context.Context, oldObj, newObj runtime.Object) (admission.Warnings, error) {
    oldCluster, ok := oldObj.(*databasev1beta1.MySQLCluster)
    if !ok {
        return nil, fmt.Errorf("expected a MySQLCluster but got a %T", oldObj)
    }

    newCluster, ok := newObj.(*databasev1beta1.MySQLCluster)
    if !ok {
        return nil, fmt.Errorf("expected a MySQLCluster but got a %T", newObj)
    }

    log := ctrl.LoggerFrom(ctx)
    log.Info("Validating MySQLCluster update", "name", newCluster.Name, "namespace", newCluster.Namespace)

    var allErrs field.ErrorList

    // Immutable fields validation
    if oldCluster.Spec.Version != newCluster.Spec.Version {
        allErrs = append(allErrs, field.Forbidden(
            field.NewPath("spec").Child("version"),
            "version is immutable"))
    }

    // Storage size can only increase
    oldSize := parseStorageSize(oldCluster.Spec.Storage.Size)
    newSize := parseStorageSize(newCluster.Spec.Storage.Size)
    if newSize < oldSize {
        allErrs = append(allErrs, field.Forbidden(
            field.NewPath("spec").Child("storage").Child("size"),
            "storage size cannot be decreased"))
    }

    if len(allErrs) == 0 {
        return nil, nil
    }

    return nil, allErrs.ToAggregate()
}

// ValidateDelete implements admission.CustomValidator
func (v *MySQLClusterValidator) ValidateDelete(ctx context.Context, obj runtime.Object) (admission.Warnings, error) {
    cluster, ok := obj.(*databasev1beta1.MySQLCluster)
    if !ok {
        return nil, fmt.Errorf("expected a MySQLCluster but got a %T", obj)
    }

    log := ctrl.LoggerFrom(ctx)
    log.Info("Validating MySQLCluster deletion", "name", cluster.Name, "namespace", cluster.Namespace)

    // Add deletion validation logic here if needed
    // For example: check if cluster has active connections before deletion

    return nil, nil
}

// Helper function to parse storage size
func parseStorageSize(size string) int64 {
    // Simple implementation - in production, use proper parsing
    var num int64
    fmt.Sscanf(size, "%dGi", &num)
    return num
}
```

### 5. Webhook配置清单

```yaml
# config/webhook/manifests.yaml
---
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  creationTimestamp: null
  name: mutating-webhook-configuration
webhooks:
- admissionReviewVersions:
  - v1
  - v1beta1
  clientConfig:
    service:
      name: webhook-service
      namespace: system
      path: /mutate-database-example-com-v1beta1-mysqlcluster
  failurePolicy: Fail
  name: mmysqlcluster.kb.io
  rules:
  - apiGroups:
    - database.example.com
    apiVersions:
    - v1beta1
    operations:
    - CREATE
    - UPDATE
    resources:
    - mysqlclusters
  sideEffects: None
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  creationTimestamp: null
  name: validating-webhook-configuration
webhooks:
- admissionReviewVersions:
  - v1
  - v1beta1
  clientConfig:
    service:
      name: webhook-service
      namespace: system
      path: /validate-database-example-com-v1beta1-mysqlcluster
  failurePolicy: Fail
  name: vmysqlcluster.kb.io
  rules:
  - apiGroups:
    - database.example.com
    apiVersions:
    - v1beta1
    operations:
    - CREATE
    - UPDATE
    - DELETE
    resources:
    - mysqlclusters
  sideEffects: None
---
apiVersion: v1
kind: Service
metadata:
  name: webhook-service
  namespace: system
spec:
  ports:
  - port: 443
    protocol: TCP
    targetPort: 9443
  selector:
    control-plane: controller-manager
```

<!-- chunk: 证书管理与部署 -->
## 证书管理与部署

### 1. Cert-Manager集成

```yaml
# config/certmanager/certificate.yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: serving-cert
  namespace: system
spec:
  dnsNames:
  - webhook-service.system.svc
  - webhook-service.system.svc.cluster.local
  issuerRef:
    kind: Issuer
    name: selfsigned-issuer
  secretName: webhook-server-cert
---
apiVersion: cert-manager.io/v1
kind: Issuer
metadata:
  name: selfsigned-issuer
  namespace: system
spec:
  selfSigned: {}
```

### 2. 手动证书生成脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# generate-certs.sh

set -e

SERVICE_NAME="webhook-service"
SERVICE_NAMESPACE="mysql-operator-system"
SECRET_NAME="webhook-server-cert"
TMP_DIR="/tmp/k8s-webhook-server/serving-certs"

mkdir -p ${TMP_DIR}

echo "🔧 生成自签名证书..."

# Generate private key
openssl genrsa -out ${TMP_DIR}/tls.key 2048

# Generate certificate
cat > ${TMP_DIR}/csr.conf <<EOF
[req]
req_extensions = v3_req
distinguished_name = req_distinguished_name
[req_distinguished_name]
[ v3_req ]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names
[alt_names]
DNS.1 = ${SERVICE_NAME}.${SERVICE_NAMESPACE}.svc
DNS.2 = ${SERVICE_NAME}.${SERVICE_NAMESPACE}.svc.cluster.local
EOF

openssl req -new -key ${TMP_DIR}/tls.key -subj "/CN=${SERVICE_NAME}.${SERVICE_NAMESPACE}.svc" -out ${TMP_DIR}/server.csr -config ${TMP_DIR}/csr.conf

# Self sign the certificate
openssl x509 -req -days 365 -in ${TMP_DIR}/server.csr -signkey ${TMP_DIR}/tls.key -out ${TMP_DIR}/tls.crt -extensions v3_req -extfile ${TMP_DIR}/csr.conf

# Create Kubernetes secret
kubectl create secret tls ${SECRET_NAME} \
  --cert=${TMP_DIR}/tls.crt \
  --key=${TMP_DIR}/tls.key \
  --namespace=${SERVICE_NAMESPACE} \
  --dry-run=client -o yaml | kubectl apply -f -

echo "✅ 证书生成完成!"
echo "🔐 证书文件位置: ${TMP_DIR}"
echo "🔑 Secret名称: ${SECRET_NAME}"
```
### 3. 部署脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# deploy-webhook.sh

set -e

NAMESPACE="mysql-operator-system"
WEBHOOK_IMG="mysql-webhook:latest"

echo "🏗️  构建Webhook镜像..."
docker build -t ${WEBHOOK_IMG} .

echo "📦 部署Webhook..."
kubectl apply -f config/crd/
kubectl apply -f config/rbac/
kubectl apply -f config/webhook/

echo "🔐 生成证书..."
./generate-certs.sh

echo "🚀 部署Webhook服务器..."
kubectl set image deployment/mysql-webhook-controller-manager \
  manager=${WEBHOOK_IMG} -n ${NAMESPACE}

echo "⏱️  等待Webhook就绪..."
kubectl wait --for=condition=available deployment/mysql-webhook-controller-manager -n ${NAMESPACE} --timeout=300s

echo "🧪 测试Webhook..."
cat <<EOF | kubectl apply -f -
apiVersion: database.example.com/v1beta1
kind: MySQLCluster
metadata:
  name: test-webhook
spec:
  replicas: 1
  storage:
    size: "10Gi"
EOF

echo "🧹 清理测试资源..."
kubectl delete mysqlcluster test-webhook

echo "✅ Webhook部署完成!"
```
<!-- chunk: 高级Webhook特性 -->
## 高级Webhook特性

### 1. 对象变更审计

```go
// webhooks/audit.go
package webhooks

import (
    "context"
    "encoding/json"
    "time"

    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/client-go/kubernetes"
    corev1 "k8s.io/client-go/kubernetes/typed/core/v1"
)

type AuditLogger struct {
    clientset kubernetes.Interface
    namespace string
}

func NewAuditLogger(clientset kubernetes.Interface, namespace string) *AuditLogger {
    return &AuditLogger{
        clientset: clientset,
        namespace: namespace,
    }
}

func (a *AuditLogger) LogChange(ctx context.Context, operation string, oldObj, newObj runtime.Object) error {
    auditEvent := map[string]interface{}{
        "timestamp":   time.Now().UTC(),
        "operation":   operation,
        "user":        ctx.Value("user"), // From authentication
        "oldObject":   oldObj,
        "newObject":   newObj,
    }

    auditBytes, err := json.Marshal(auditEvent)
    if err != nil {
        return err
    }

    // Log to ConfigMap or external system
    configMapClient := a.clientset.CoreV1().ConfigMaps(a.namespace)
    configMap := &corev1.ConfigMap{
        ObjectMeta: metav1.ObjectMeta{
            Name: fmt.Sprintf("audit-%s", time.Now().Format("20060102-150405")),
        },
        Data: map[string]string{
            "audit.json": string(auditBytes),
        },
    }

    _, err = configMapClient.Create(ctx, configMap, metav1.CreateOptions{})
    return err
}
```

### 2. 复杂验证逻辑

```go
// webhooks/complex_validation.go
package webhooks

import (
    "context"
    "fmt"
    "strings"

    "k8s.io/apimachinery/pkg/util/validation/field"
    databasev1beta1 "github.com/example/mysql-webhook/api/v1beta1"
)

type ComplexValidator struct {
    // External dependencies
    securityScanner SecurityScanner
    costCalculator  CostCalculator
}

func (cv *ComplexValidator) ValidateClusterCreation(ctx context.Context, cluster *databasev1beta1.MySQLCluster) field.ErrorList {
    var allErrs field.ErrorList

    // Security validation
    if err := cv.validateSecurityCompliance(cluster); err != nil {
        allErrs = append(allErrs, err)
    }

    // Cost validation
    if err := cv.validateCostLimits(cluster); err != nil {
        allErrs = append(allErrs, err)
    }

    // Resource quota validation
    if err := cv.validateResourceQuotas(cluster); err != nil {
        allErrs = append(allErrs, err)
    }

    // Naming convention validation
    if err := cv.validateNamingConvention(cluster); err != nil {
        allErrs = append(allErrs, err)
    }

    return allErrs
}

func (cv *ComplexValidator) validateSecurityCompliance(cluster *databasev1beta1.MySQLCluster) *field.Error {
    // Check if security scanner detects vulnerabilities
    issues := cv.securityScanner.Scan(cluster)
    if len(issues) > 0 {
        return field.Forbidden(
            field.NewPath("spec"),
            fmt.Sprintf("security issues detected: %s", strings.Join(issues, ", ")))
    }
    return nil
}

func (cv *ComplexValidator) validateCostLimits(cluster *databasev1beta1.MySQLCluster) *field.Error {
    estimatedCost := cv.costCalculator.Estimate(cluster)
    maxAllowedCost := 1000.0 // $1000/month limit
    
    if estimatedCost > maxAllowedCost {
        return field.Forbidden(
            field.NewPath("spec"),
            fmt.Sprintf("estimated monthly cost $%.2f exceeds limit $%.2f", 
                estimatedCost, maxAllowedCost))
    }
    return nil
}

func (cv *ComplexValidator) validateResourceQuotas(cluster *databasev1beta1.MySQLCluster) *field.Error {
    // Check namespace resource quotas
    // Implementation depends on your quota system
    return nil
}

func (cv *ComplexValidator) validateNamingConvention(cluster *databasev1beta1.MySQLCluster) *field.Error {
    // Enforce naming conventions
    validPrefixes := []string{"prod-", "staging-", "dev-"}
    hasValidPrefix := false
    
    for _, prefix := range validPrefixes {
        if strings.HasPrefix(cluster.Name, prefix) {
            hasValidPrefix = true
            break
        }
    }
    
    if !hasValidPrefix {
        return field.Invalid(
            field.NewPath("metadata").Child("name"),
            cluster.Name,
            fmt.Sprintf("name must start with one of: %s", strings.Join(validPrefixes, ", ")))
    }
    return nil
}
```

<!-- chunk: Webhook监控与问题排除 -->
## Webhook监控与问题排除

### 1. 监控指标

```go
// metrics/webhook_metrics.go
package metrics

import (
    "github.com/prometheus/client_golang/prometheus"
    "sigs.k8s.io/controller-runtime/pkg/metrics"
)

var (
    WebhookRequestTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "mysql_webhook_requests_total",
            Help: "Total number of webhook requests",
        },
        []string{"webhook", "operation", "result"},
    )
    
    WebhookRequestDuration = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name: "mysql_webhook_request_duration_seconds",
            Help: "Duration of webhook requests",
            Buckets: []float64{0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0},
        },
        []string{"webhook", "operation"},
    )
    
    WebhookValidationErrors = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "mysql_webhook_validation_errors_total",
            Help: "Total number of validation errors by type",
        },
        []string{"error_type", "field"},
    )
)

func init() {
    metrics.Registry.MustRegister(
        WebhookRequestTotal,
        WebhookRequestDuration,
        WebhookValidationErrors,
    )
}
```

### 2. 问题排除工具

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# webhook-debug.sh

NAMESPACE="mysql-operator-system"
WEBHOOK_POD=$(kubectl get pods -n ${NAMESPACE} -l control-plane=controller-manager -o name)

echo "=== Webhook Debug Information ==="

echo "1. Webhook Configurations:"
kubectl get mutatingwebhookconfigurations,validatingwebhookconfigurations -o wide

echo -e "\n2. Webhook Service Status:"
kubectl get service webhook-service -n ${NAMESPACE} -o wide

echo -e "\n3. Webhook Pod Status:"
kubectl get pods -n ${NAMESPACE} -l control-plane=controller-manager -o wide

echo -e "\n4. Webhook Logs:"
kubectl logs ${WEBHOOK_POD} -n ${NAMESPACE} --since=1h

echo -e "\n5. Certificate Status:"
kubectl get secret webhook-server-cert -n ${NAMESPACE} -o yaml

echo -e "\n6. Webhook Connection Test:"
kubectl run webhook-test --rm -i --tty --image=curlimages/curl:latest -- \
  curl -vk https://webhook-service.${NAMESPACE}.svc:443/healthz

echo -e "\n7. Recent Admission Reviews:"
kubectl get events -n ${NAMESPACE} --field-selector reason=AdmissionWebhook

echo "=== Debug Complete ==="
```
### 3. 性能测试

```bash
#!/bin/bash
# webhook-benchmark.sh

CONCURRENT_REQUESTS=10
TOTAL_REQUESTS=100
WEBHOOK_URL="https://webhook-service.mysql-operator-system.svc:443"

echo "🚀 开始Webhook性能测试..."

# Test mutating webhook
echo "🧪 测试Mutating Webhook..."
hey -n ${TOTAL_REQUESTS} -c ${CONCURRENT_REQUESTS} \
  -m POST \
  -H "Content-Type: application/json" \
  -d @test-mutating-request.json \
  ${WEBHOOK_URL}/mutate-database-example-com-v1beta1-mysqlcluster

# Test validating webhook
echo "🧪 测试Validating Webhook..."
hey -n ${TOTAL_REQUESTS} -c ${CONCURRENT_REQUESTS} \
  -m POST \
  -H "Content-Type: application/json" \
  -d @test-validating-request.json \
  ${WEBHOOK_URL}/validate-database-example-com-v1beta1-mysqlcluster

echo "✅ 性能测试完成!"
```

---
**Webhook开发原则**: 安全第一、性能优化、可观测性、故障恢复

---
**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 专项技术 KUDIG Database — Global MOC
- [[16-专项技术/README.md|Domain-10: Kubernetes 扩展生态]]
- index.md|Domain-10 扩展与自定义 — 开源项目索引]]
- CRD 自定义资源定义开发指南
- 02 - Operator开发模式与控制器实现
- Kubernetes API 聚合扩展机制详解
- 包管理与应用分发工具
- 47 - Helm Chart开发与管理
- 129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践
- CI/CD 管道
- 48 - GitOps工作流
- 103 - 容器镜像构建工具 (Container Image Build)

## See Also

- 01-crd-development-guide
- 02-operator-development-patterns
- 04-api-aggregation-extension
- 05-package-management-tools


<!-- risk-assessed -->
