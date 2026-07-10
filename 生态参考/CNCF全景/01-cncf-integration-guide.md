---
title: CNCF 集成实践指南
description: CNCF 项目组合使用指南，涵盖监控、安全、网络、存储等场景的最佳实践集成方案
summary: CNCF 项目组合使用指南，涵盖监控、安全、网络、存储等场景的最佳实践集成方案
category: cncf-landscape
tags:
- k8s
- cncf
- integration
- observability
- security
- networking
- storage
- apiserver
- prometheus
- grafana
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- SRE
- DevOps
estimated_read_time: 10min
intent_queries:
- CNCF 项目集成
- 云原生技术栈组合
- CNCF 最佳实践
trigger_keywords:
- CNCF
- 集成
- 实践
- 组合
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- kafka-basics
- tls-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNCF 集成实践指南

> **适用版本**: [[Kubernetes|Kubernetes]] v1.28+ | **最后更新**: 2026-05

---

<!-- chunk: 1. 概述 -->## 1. 概述

## 1.1 集成原则

| 原则 | 说明 |
|:-----|:-----|
| **松耦合** | 各项目独立运行，通过标准接口通信 |
| **可观测** | 统一的可观测性栈 |
| **安全内建** | 安全性贯穿各层 |
| **云原生** | 符合云原生设计理念 |

## 1.2 典型技术栈

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                        │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐          │
│  │  Helm   │  │ Argo CD │  │ KEDA    │  │ Dapr    │          │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘          │
├─────────────────────────────────────────────────────────────────┤
│                      运行时 (Runtime)                           │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                        │
│  │containerd│  │  CRI-O  │  │ CoreDNS │                        │
│  └─────────┘  └─────────┘  └─────────┘                        │
├─────────────────────────────────────────────────────────────────┤
│                      编排层 (Orchestration)                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐          │
│  │Kubernetes│  │  Keda   │  │ Volcano │  │ Karmada │          │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘          │
├─────────────────────────────────────────────────────────────────┤
│                      网络层 (Networking)                        │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐          │
│  │ Cilium  │  │  Istio  │  │ Linkerd │  │ CoreDNS │          │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘          │
├─────────────────────────────────────────────────────────────────┤
│                      存储层 (Storage)                           │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                        │
│  │  Rook   │  │ Longhorn│  │ CubeFS  │                        │
│  └─────────┘  └─────────┘  └─────────┘                        │
├─────────────────────────────────────────────────────────────────┤
│                      可观测性 (Observability)                    │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐          │
│  │Prometheus│  │  Jaeger │  │ Fluentd │  │OpenTelemetry│        │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘          │
├─────────────────────────────────────────────────────────────────┤
│                      安全层 (Security)                          │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐          │
│  │ Falco   │  │  OPA    │  │cert-mgr │  │ SPIFFE  │          │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘          │
└─────────────────────────────────────────────────────────────────┘
```
---

<!-- chunk: 2. 监控与可观测性集成 -->## 2. 监控与可观测性集成

## 2.1 Prometheus + Grafana + Alertmanager

**架构图**：
```
┌─────────────────────────────────────────────────────────────────┐
│                    Prometheus Stack                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐      │
│  │ Prometheus  │────▶│ Alertmanager │────▶│   Email    │      │
│  │  (采集)     │     │   (告警)    │     │   Slack    │      │
│  └─────────────┘     └─────────────┘     └─────────────┘      │
│         │                                       │                │
│         ▼                                       ▼                │
│  ┌─────────────┐                         ┌─────────────┐      │
│  │  Grafana   │                         │   PagerDuty │      │
│  │  (可视化)  │                         │   (值班)    │      │
│  └─────────────┘                         └─────────────┘      │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

**集成配置**：

```yaml
# Prometheus Operator CRD
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: k8s-prometheus
  namespace: monitoring
spec:
  replicas: 2
  retention: 15d
  alerting:
    alertmanagers:
      - namespace: monitoring
        name: alertmanager-main
  serviceMonitorSelector:
    matchLabels:
      team: k8s
```

```yaml
# Alertmanager 配置
apiVersion: monitoring.coreos.com/v1
kind: Alertmanager
metadata:
  name: alertmanager-main
  namespace: monitoring
spec:
  replicas: 3
  config:
    route:
      group_by: ['job']
      group_wait: 30s
      group_interval: 5m
      receiver: 'default'
      routes:
        - matchers:
          - severity="critical"
          receiver: critical
    receivers:
      - name: 'default'
        webhook_configs:
          - url: 'http://alertmanager-notifier:9099/alerts'
      - name: 'critical'
        slack_configs:
          - api_url: 'https://hooks.slack.com/services/xxx'
            channel: '#alerts-critical'
```

## 2.2 OpenTelemetry Collector 集成

**架构**：
```
┌─────────────────────────────────────────────────────────────────┐
│                  OpenTelemetry Collector                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Receivers                             │    │
│  │  OTLP │ Jaeger │ Zipkin │ Prometheus │ Fluentd │       │    │
│  └─────────────────────────────────────────────────────────┘    │
│                            │                                     │
│                            ▼                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Processors                            │    │
│  │  batch │ memory_limiter │ k8sattributes │ filter │     │    │
│  └─────────────────────────────────────────────────────────┘    │
│                            │                                     │
│                            ▼                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Exporters                             │    │
│  │  otlp │ prometheus │ jaeger │ clickhouse │ elasticsearch │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

**部署配置**：

```yaml
apiVersion: opentelemetry.io/v1alpha1
kind: OpenTelemetryCollector
metadata:
  name: otel-collector
  namespace: observability
spec:
  mode: daemonset
  config: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
      prometheus:
        config:
          scrape_configs:
            - job_name: 'kubernetes-pods'
              kubernetes_pod_configs:
                - port: 9090
    processors:
      batch:
        timeout: 10s
      memory_limiter:
        limit_mib: 512
    exporters:
      otlp:
        endpoint: "jaeger-collector:4317"
      prometheus:
        endpoint: "0.0.0.0:8889"
    service:
      pipelines:
        traces:
          receivers: [otlp]
          processors: [batch, memory_limiter]
          exporters: [otlp]
        metrics:
          receivers: [prometheus]
          processors: [batch, memory_limiter]
          exporters: [prometheus]
```

## 2.3 链路追踪集成 (Jaeger + OpenTelemetry)

```yaml
# Jaeger Operator CRD
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: jaeger-production
  namespace: observability
spec:
  strategy: production
  collector:
    replicas: 3
    max_replicas: 10
  query:
    replicas: 2
  storage:
    type: elasticsearch
    elasticsearch:
      nodeCount: 3
      redundancyPolicy: SingleRedundancy
```

---

<!-- chunk: 3. 网络与服务网格集成 -->## 3. 网络与服务网格集成

## 3.1 Cilium + Hubble 集成

**架构**：
```
┌─────────────────────────────────────────────────────────────────┐
│                    Cilium + Hubble                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Kubernetes Cluster                    │    │
│  │                                                           │    │
│  │  ┌───────────┐      ┌───────────┐      ┌───────────┐   │    │
│  │  │  Pod A    │─────▶│  Pod B    │─────▶│  Pod C    │   │    │
│  │  └───────────┘      └───────────┘      └───────────┘   │    │
│  │         │                  │                  │         │    │
│  │         └──────────────────┼──────────────────┘         │    │
│  │                            │                            │    │
│  │                            ▼                            │    │
│  │                   ┌───────────────┐                     │    │
│  │                   │  Cilium Agent │                     │    │
│  │                   │   (eBPF)     │                     │    │
│  │                   └───────┬───────┘                     │    │
│  │                           │                             │    │
│  │                           ▼                             │    │
│  │                   ┌───────────────┐                     │    │
│  │                   │    Hubble    │                      │    │
│  │                   │  (流量可视化) │                      │    │
│  │                   └───────────────┘                     │    │
│  └─────────────────────────────────────────────────────────┘    │
│                            │                                     │
│                            ▼                                     │
│                   ┌───────────────┐                             │
│                   │   Grafana    │                              │
│                   │  (Hubble UI) │                              │
│                   └───────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
```

**Helm 安装**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add cilium https://helm.cilium.io/

helm install cilium cilium/cilium \
  --namespace kube-system \
  --set hubble.enabled=true \
  --set hubble.relay.enabled=true \
  --set hubble.ui.enabled=true \
  --set prometheus.enabled=true \
  --set operator.prometheus.enabled=true
```
## 3.2 Istio + Kiali + Prometheus 集成

```yaml
# Istio 安装配置
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: istio-production
  namespace: istio-system
spec:
  profile: default
  components:
    pilot:
      k8s:
        resources:
          requests:
            cpu: 500m
            memory: 2Gi
    ingressGateways:
      - name: istio-ingressgateway
        k8s:
          service:
            type: LoadBalancer
  values:
    prometheus:
      enabled: true
    kiali:
      enabled: true
```

```yaml
# Kiali 配置
apiVersion: kiali.io/v1alpha1
kind: Kiali
metadata:
  name: kiali
  namespace: istio-system
spec:
  auth:
    strategy: anonymous
  server:
    web_port: 20001
  external_services:
    prometheus:
      url: http://prometheus:9090
    grafana:
      url: http://grafana:3000
    tracing:
      in_cluster_url: http://jaeger-query:16685
```

## 3.3 Linkerd + Buoyant Cloud 集成

```yaml
# Linkerd 安装
curl -sL https://run.linkerd.io/install | sh

linkerd install | kubectl apply -f -

# 启用高可用
linkerd install --ha | kubectl apply -f -

# 安装 Linkerd Viz 扩展（监控）
linkerd viz install | kubectl apply -f -
```

---

<!-- chunk: 4. 安全集成 -->## 4. 安全集成

## 4.1 Falco + Prometheus + Alertmanager

```yaml
# Falco 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-config
  namespace: falco
data:
  falco.yaml: |
    json_output: true
    http_output:
      enabled: true
      url: http://falco-event-forwarder:8090/events

  # 自定义规则
  falco_rules.yaml: |
    - rule: Unexpected outbound connection
      desc: Detect unexpected outbound connections
      condition: >
        outbound and not ka.svc.name
      output: >
        Unexpected outbound connection
        (user=%user.name command=%proc.cmdline connection=%fd.name)
      priority: WARNING
```

```yaml
# PrometheusRule for Falco
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: falco-alerts
  namespace: falco
spec:
  groups:
    - name: falco
      rules:
        - alert: FalcoWarningEvent
          expr: rate(falco_events_total{priority="WARNING"}[5m]) > 0
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "Falco warning event detected"
```

## 4.2 cert-manager + Istio + External DNS

```yaml
# cert-manager ClusterIssuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-account-key
    solvers:
      - dns01:
          clouddns:
            project: my-project
            serviceAccountRef:
              name: clouddns-sa
        selector:
          dnsNames:
            - '*.example.com'
```

```yaml
# Istio Gateway with TLS
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: my-gateway
  namespace: istio-system
spec:
  selector:
    istio: ingressgateway
  servers:
    - port:
        number: 443
        name: https
        protocol: HTTPS
      tls:
        mode: SIMPLE
        credentialName: my-cert
      hosts:
        - "*.example.com"
```

## 4.3 OPA + Gatekeeper 策略集成

```yaml
# Gatekeeper 安装
helm repo add gatekeeper https://open-policy-agent.github.io/gatekeeper/charts
helm install gatekeeper gatekeeper/gatekeeper \
  --namespace gatekeeper-system \
  --create-namespace

# 拒绝特权容器策略
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPAllowedCapabilities
metadata:
  name: psp-allowed-capabilities
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
  parameters:
    allowedCapabilities:
      - NET_ADMIN
    requiredDropCapabilities:
      - ALL
```

---

<!-- chunk: 5. 存储集成 -->## 5. 存储集成

## 5.1 Rook + Ceph + Prometheus

```yaml
# Rook CephCluster
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: ceph/ceph:v17.2.6
  dataDirHostPath: /var/lib/rook
  mon:
    count: 3
    allowMultiplePerNode: false
  manager:
    count: 2
  storage:
    useAllNodes: true
    useAllDevices: true
    config:
      osdsPerDevice: "1"
  dashboard:
    enabled: true
  monitoring:
    enabled: true
    rulesNamespace: rook-ceph
```

```yaml
# StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: ceph.rook.io/block
parameters:
  clusterID: rook-ceph
  pool: replicapool
  csi.storage.k8s.io/fstype: xfs
reclaimPolicy: Retain
allowVolumeExpansion: true
```

## 5.2 Longhorn + Backup to S3

```yaml
# Longhorn 设置
apiVersion: longhorn.io/v1beta2
kind: Longhorn
metadata:
  name: longhorn
  namespace: longhorn-system
spec:
  backupTarget:
    concurrentBackupRestorePerNodeLimit: 3
    nutanix:
      fspType: nfs
  defaultSettings:
    defaultDataPath: /var/lib/longhorn
    backupCompressionMethod: gzip
    replicaReplenishmentWaitInterval: 600
    snapshotDataIntegrity: fast-check
  storageNetwork: storage-network
```

---

<!-- chunk: 6. [[系统基础/速查卡/gitops.md|GitOps]] 速查卡|GitOps]] 集成 -->## 6. GitOps 集成

## 6.1 Argo CD + Argo Rollouts + Flagger

```yaml
# Argo CD 安装
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# Argo Rollouts 安装
kubectl create namespace argo-rollouts
kubectl apply -n argo-rollouts -f https://github.com/argoproj/rollouts-manager/releases/latest/download/install.yaml

# Flagger 安装 (用于金丝雀发布)
helm repo add flagger https://flagger.app
helm install flagger flagger/flagger \
  --namespace=istio-system \
  --set meshProvider=istio
```

**金丝雀发布配置**：

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
  namespace: default
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  analysis:
    interval: 1m
    threshold: 5
    maxWeight: 50
    stepWeight: 10
    metrics:
      - name: request-success-rate
        thresholdRange:
          min: 99
        interval: 1m
      - name: request-duration
        thresholdRange:
          max: 500
        interval: 1m
```

## 6.2 Flux + Helm + Image Automation

```yaml
# Flux 安装
flux install \
  --namespace=flux-system \
  --components=source-controller,helm-controller,kustomize-controller

# HelmRepository
apiVersion: source.toolkit.fluxcd.io/v1beta2
kind: HelmRepository
metadata:
  name: bitnami
  namespace: flux-system
spec:
  interval: 1m
  url: https://charts.bitnami.com/bitnami

# HelmRelease
apiVersion: helm.toolkit.fluxcd.io/v2beta1
kind: HelmRelease
metadata:
  name: my-app
  namespace: default
spec:
  interval: 5m
  chart:
    spec:
      chart: my-app
      version: "1.0.0"
      sourceRef:
        kind: HelmRepository
        name: my-chart-repo
  values:
    replicaCount: 3
    image:
      repository: my-registry/my-app
      tag: latest
```

---

<!-- chunk: 7. 事件驱动与 Serverless -->## 7. 事件驱动与 Serverless

## 7.1 Knative + Kafka + Prometheus

```yaml
# Knative Eventing 安装
kubectl apply -f https://github.com/knative/eventing/releases/download/knative-v1.11.0/eventing-core.yaml
kubectl apply -f https://github.com/knative/eventing/releases/download/knative-v1.11.0/in-memory.yaml

# Strimzi Kafka 安装
helm install kafka strimzi/strimzi-kafka-operator \
  --namespace kafka \
  --create-namespace

apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: my-cluster
  namespace: kafka
spec:
  kafka:
    replicas: 3
    listeners:
      - name: plain
        port: 9092
        type: internal
        tls: false
      - name: tls
        port: 9093
        type: internal
        tls: true
    storage:
      type: jbod
      volumes:
        - id: 0
          type: persistent-claim
          size: 100Gi
  zookeeper:
    replicas: 3
  entityOperator:
    topicOperator: {}
    userOperator: {}
```

```yaml
# Knative Service 示例
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: my-service
  namespace: default
spec:
  template:
    metadata:
      annotations:
        autoscaling.knative.dev/metric: concurrency
        autoscaling.knative.dev/target: "100"
    spec:
      containerConcurrency: 10
      timeoutSeconds: 300
      containers:
        - image: my-registry/my-service:latest
          resources:
            requests:
              cpu: "100m"
              memory: "128Mi"
            limits:
              cpu: "1000m"
              memory: "512Mi"
          env:
            - name: KAFKA_BOOTSTRAP_SERVERS
              value: my-cluster-kafka-bootstrap.kafka:9092
```

## 7.2 KEDA + Kafka + HPA

```yaml
# KEDA 安装
helm repo add kedacore https://kedacore.github.io/charts
helm install keda kedacore/keda \
  --namespace keda \
  --create-namespace

# Kafka Trigger 配置
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: my-app-scaler
  namespace: default
spec:
  scaleTargetRef:
    name: my-app
  pollingInterval: 15
  cooldownPeriod: 300
  minReplicaCount: 3
  maxReplicaCount: 100
  triggers:
    - type: kafka
      metadata:
        bootstrapServers: my-cluster-kafka-bootstrap.kafka:9092
        consumerGroup: my-app-consumer
        topic: my-topic
        lagThreshold: "100"
```

---

<!-- chunk: 8. 多集群集成 -->## 8. 多集群集成

## 8.1 Karmada + Argo CD

```yaml
# Karmada 安装
helm repo add karmada https://rainbond.github.io/karmada-charts
helm install karmada karmada/karmada \
  --namespace karmada-system \
  --create-namespace

# 在 member 集群上安装 Karmada agent
helm install karmada-agent karmada/karmada-agent \
  --namespace karmada-system \
  --set karmaServer=https://karmada-apiserver:5443 \
  --set clusterName=member1 \
  --set clusterToken=<token>
```

```yaml
# 跨集群资源传播
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: my-app-propagation
  namespace: default
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
  placement:
    clusterAffinity:
      clusterNames:
        - member1
        - member2
    replicaScheduling:
      replicaSchedulingType: Duplicated
      replicaDivisionPreference: Weighted
      weight: 100
```

---

<!-- chunk: 9. 完整技术栈示例 -->## 9. 完整技术栈示例

## 9.1 生产环境推荐架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     Production Tech Stack                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                        GitOps Layer                       │  │
│  │                   Argo CD + Flux                          │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                      Service Mesh                         │  │
│  │              Cilium/Linkerd + Hubble                      │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                       API Gateway                          │  │
│  │                    Envoy + Contour                        │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    Application Layer                       │  │
│  │    Knative/KEDA + Dapr + Spring Cloud                    │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                       Data Layer                           │  │
│  │          TiKV/Vitess + Rook/CubeFS                       │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                     Observability                          │  │
│  │   Prometheus + Thanos + Grafana + Jaeger + Fluentd        │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                        Security                            │  │
│  │    Falco + OPA + cert-manager + SPIFFE/SPIRE              │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## 9.2 快速部署命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 完整技术栈一键部署

# 1. 安装基础组件
kubectl create namespace monitoring
kubectl create namespace istio-system
kubectl create namespace flux-system

# 2. 安装 Argo CD
kubectl apply -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 3. 安装 Prometheus Stack
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace

# 4. 安装 Cilium
helm repo add cilium https://helm.cilium.io/
helm install cilium cilium/cilium \
  --namespace kube-system \
  --set hubble.enabled=true \
  --set prometheus.enabled=true

# 5. 安装 Istio
istioctl install --set profile=default

# 6. 安装 cert-manager
helm repo add jetstack https://charts.jetstack.io
helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager \
  --create-namespace \
  --set installCRDs=true

# 7. 安装 Knative
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.11.0/serving-crds.yaml
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.11.0/serving-core.yaml

# 8. 安装 KEDA
helm repo add kedacore https://kedacore.github.io/charts
helm install keda kedacore/keda \
  --namespace keda \
  --create-namespace

echo "Installation complete!"
```
---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生态参考 KUDIG Database — Global MOC
- README.md|Domain-34: CNCF Landscape 开源项目]]
- Domain-34 CNCF Landscape — 开源项目索引
- CNCF 学习路径
- CNCF 项目选型指南
- CNCF 项目 FTA 索引

## See Also

- 03-cncf-selection-guide
- 04-cncf-fta-index
- 02-cncf-learning-paths
- 03-cncf-selection-guide


<!-- risk-assessed -->
