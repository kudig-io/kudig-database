---
title: Kubernetes 开源项目快速选型指南
description: '# Kubernetes 开源项目快速选型指南'
summary: '# Kubernetes 开源项目快速选型指南'
category: general
tags:
- k8s
- prometheus
- grafana
- jaeger
- istio
- envoy
- cilium
- flannel
- calico
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- Kubernetes 开源项目快速选型指南 是什么
- 如何 Kubernetes 开源项目快速选型指南
trigger_keywords:
- Kubernetes
- 开源项目快速选型指南
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- ebpf-basics
- cilium-basics
- cni-basics
- kafka-basics
- mysql-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- tracing-basics
- observability-basics
---



# Kubernetes 开源项目快速选型指南

> **适用版本**: 2026-04  
> **最后更新**: 2026-04-24  
> **用途**: 按场景快速定位项目与深度指南

---

## 使用说明

本指南汇总了 kudig-database 中 **26 篇深度实践指南** 和 **40 个 Domain 项目索引** 的核心选型结论，按**场景 → 决策 → 推荐**三层结构组织，帮助工程师在 30 秒内定位最适合的开源方案。

---

## 一、可观测性 (Observability)

### 1.1 监控 (Metrics)

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 中小集群 (<100 节点) | kube-prometheus-stack (Helm) | domain-06-observability/99-prometheus-enterprise-guide.md |
| 多集群联邦 | Thanos / Grafana Mimir | domain-06-observability/00-open-source-projects-index.md |
| 云原生替代方案 | VictoriaMetrics / SigNoz | domain-06-observability/00-open-source-projects-index.md |
| 企业级 SaaS | Datadog / Dynatrace | domain-06-observability/00-open-source-projects-index.md |

**决策**: Prometheus 是标配，多集群场景选 Thanos，追求极简选 VictoriaMetrics。

### 1.2 日志 (Logs)

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 成本敏感 | Grafana Loki | domain-06-observability/00-open-source-projects-index.md |
| 全文检索强 | ELK (OpenSearch) | domain-06-observability/00-open-source-projects-index.md |
| 云厂商集成 | Fluentd → CloudWatch/SPL | domain-06-observability/00-open-source-projects-index.md |
| Agent 选择 | Fluent Bit (轻量) / Grafana Alloy (新) | domain-06-observability/00-open-source-projects-index.md |

### 1.3 追踪 (Traces)

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| Grafana 生态 | Tempo + OpenTelemetry | domain-06-observability/99-distributed-tracing-guide.md |
| 全功能独立 | Jaeger + OpenTelemetry | domain-06-observability/99-distributed-tracing-guide.md |
| 快速启动 | Zipkin | domain-06-observability/99-distributed-tracing-guide.md |
| 统一协议 | OpenTelemetry Collector | domain-06-observability/99-distributed-tracing-guide.md |

**决策**: Grafana 用户直接选 Tempo，需要丰富 UI 选 Jaeger，Spring 生态选 Zipkin。

---

## 二、GitOps & CI/CD

### 2.1 GitOps 控制器

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 纯 GitOps，无 UI 依赖 | **Flux** | domain-08-release-change-management/99-flux-gitops-guide.md |
| 集中式多集群管理 | **Argo CD** | domain-08-release-change-management/99-argo-cd-gitops-guide.md |
| 需要镜像自动更新 | Flux (内置) > Argo CD + Image Updater | domain-08-release-change-management/99-flux-gitops-guide.md |
| 大规模 (>1000 apps) | Argo CD | domain-08-release-change-management/99-argo-cd-gitops-guide.md |
| 需要丰富 UI | Argo CD | domain-08-release-change-management/99-argo-cd-gitops-guide.md |

**决策**: 简单场景/镜像自动更新 → Flux；复杂多集群/企业级 → Argo CD。

### 2.2 CI/CD 流水线

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| K8s 原生 / 供应链安全 | **Tekton** | domain-08-release-change-management/99-tekton-cicd-guide.md |
| 快速启动 / GitHub 生态 | GitHub Actions | domain-08-release-change-management/00-open-source-projects-index.md |
| 传统企业 / 丰富插件 | Jenkins | domain-08-release-change-management/00-open-source-projects-index.md |
| 云原生替代 | GitLab CI / Woodpecker | domain-08-release-change-management/00-open-source-projects-index.md |

---

## 三、安全 (Security)

### 3.1 运行时安全

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| eBPF 深度监控 | **Falco** | domain-05-security-compliance/99-falco-runtime-security-guide.md |
| 轻量审计 | Tetragon | domain-05-security-compliance/00-open-source-projects-index.md |
| 商业级 | Sysdig / Aqua | domain-05-security-compliance/00-open-source-projects-index.md |

### 3.2 策略管理

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| K8s 原生 / 快速落地 | **Kyverno** | domain-05-security-compliance/99-kyverno-policy-guide.md |
| 跨平台统一 / 复杂策略 | **OPA Gatekeeper** | domain-05-security-compliance/99-opa-gatekeeper-policy-guide.md |
| 轻量级 WASM 策略 | Kubewarden | domain-05-security-compliance/00-open-source-projects-index.md |

**决策**: 纯 K8s 场景优先 Kyverno (YAML 即可)；跨平台或多云选 OPA。

### 3.3 密钥管理

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 企业级 PKI / 动态凭证 | **Vault** | domain-05-security-compliance/99-vault-k8s-secrets-guide.md |
| K8s 原生 / 简单同步 | External Secrets Operator | domain-05-security-compliance/00-open-source-projects-index.md |
| 离线场景 | Sealed Secrets / SOPS | domain-05-security-compliance/00-open-source-projects-index.md |

### 3.4 证书管理

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 自动 TLS (必装) | **[[cert-manager|cert-manager]]** | domain-05-security-compliance/99-cert-manager-tls-guide.md |
| 内部 CA | cert-manager + 自建 CA | domain-05-security-compliance/99-cert-manager-tls-guide.md |
| 通配符证书 | cert-manager + DNS-01 | domain-05-security-compliance/99-cert-manager-tls-guide.md |

### 3.5 供应链安全

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 镜像签名验证 | **cosign (Sigstore)** | domain-05-security-compliance/99-slsa-supply-chain-security-guide.md |
| 构建证明 | **Tekton Chains** | domain-05-security-compliance/99-slsa-supply-chain-security-guide.md |
| SBOM 生成 | Syft / Trivy | domain-05-security-compliance/99-slsa-supply-chain-security-guide.md |
| 准入控制验证 | Kyverno / OPA + cosign | domain-05-security-compliance/99-slsa-supply-chain-security-guide.md |

---

## 四、网络 (Networking)

### 4.1 CNI

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 企业级网络 + 安全 | **Cilium** | domain-03-networking-traffic/99-cilium-ebpf-network-guide.md |
| 云厂商集成 | AWS VPC-CNI / Azure CNI | domain-03-networking-traffic/00-open-source-projects-index.md |
| 简单稳定 | Calico | domain-03-networking-traffic/00-open-source-projects-index.md |
| Windows 支持 | Calico / Flannel | domain-03-networking-traffic/00-open-source-projects-index.md |

### 4.2 服务网格

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 功能最全面 | **Istio** | domain-03-networking-traffic/99-istio-service-mesh-guide.md |
| 极简轻量 | **Linkerd** | domain-03-networking-traffic/99-linkerd-service-mesh-guide.md |
| Ambient (无 Sidecar) | Istio Ambient | domain-03-networking-traffic/99-istio-service-mesh-guide.md |
| 纯 Sidecar | Linkerd | domain-03-networking-traffic/99-linkerd-service-mesh-guide.md |

**决策**: 复杂流量管理 / Ambient → Istio；简单 mTLS + 可观测性 → Linkerd。

### 4.3 API Gateway

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| Gateway API 标准 | **Envoy Gateway** | domain-03-networking-traffic/99-envoy-gateway-enterprise-guide.md |
| 功能丰富 | Apache APISIX / Kong | domain-03-networking-traffic/00-open-source-projects-index.md |
| 阿里云原生 | Higress | domain-03-networking-traffic/00-open-source-projects-index.md |
| 传统迁移 | NGINX Ingress → Gateway API | domain-03-networking-traffic/00-open-source-projects-index.md |

---

## 五、存储 & 数据库

### 5.1 云原生数据库

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| PostgreSQL on K8s | **CloudNativePG** | domain-16-database-middleware/99-cloudnativepg-enterprise-guide.md |
| Kafka on K8s | Strimzi / Redpanda | domain-16-database-middleware/00-open-source-projects-index.md |
| MySQL on K8s | Vitess / Oracle MySQL Operator | domain-16-database-middleware/00-open-source-projects-index.md |
| MongoDB on K8s | Percona / MongoDB Community Operator | domain-16-database-middleware/00-open-source-projects-index.md |

### 5.2 分布式存储

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| Ceph on K8s | Rook | domain-04-storage-data/00-open-source-projects-index.md |
| 轻量块存储 | Longhorn | domain-04-storage-data/00-open-source-projects-index.md |
| 云厂商 | EBS / Azure Disk / CSI Driver | domain-04-storage-data/00-open-source-projects-index.md |

---

## 六、平台工程 & 开发者体验

### 6.1 内部开发者平台 (IDP)

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 开源 IDP | **Backstage** | domain-07-platform-engineering/99-backstage-idp-guide.md |
| 平台编排 | **Crossplane** | domain-08-release-change-management/99-crossplane-platform-guide.md |
| 商业 IDP | Port / Humanitec | domain-07-platform-engineering/00-open-source-projects-index.md |

### 6.2 开发者工具链

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 终端管理 | **k9s** | domain-15-specialized-tech/99-kubernetes-developer-toolchain-guide.md |
| Web UI | **Headlamp** (Lens 开源替代) | domain-15-specialized-tech/99-kubernetes-developer-toolchain-guide.md |
| 日志聚合 | **stern** | domain-15-specialized-tech/99-kubernetes-developer-toolchain-guide.md |
| 本地联调 | **mirrord** / Telepresence | domain-15-specialized-tech/99-kubernetes-developer-toolchain-guide.md |

---

## 七、基础设施管理

### 7.1 IaC

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| K8s 原生平台工程 | **Crossplane** | domain-08-release-change-management/99-crossplane-platform-guide.md |
| 多云 Terraform | OpenTofu / Terraform | domain-08-release-change-management/00-open-source-projects-index.md |
| 编程式 IaC | Pulumi | domain-08-release-change-management/00-open-source-projects-index.md |

### 7.2 自动扩展

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 节点自动扩展 | **Karpenter** | domain-11-production-operations/99-karpenter-node-autoscaling-guide.md |
| 事件驱动 Pod 扩缩 | **KEDA** | domain-11-production-operations/99-keda-event-driven-autoscaling-guide.md |
| 标准 HPA | 原生 HPA v2 | domain-11-production-operations/00-open-source-projects-index.md |

### 7.3 成本优化

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| K8s 成本可视化 | **OpenCost** (CNCF) / Kubecost | domain-11-production-operations/99-finops-cost-optimization-guide.md |
| IaC 成本预估 | **Infracost** | domain-11-production-operations/99-finops-cost-optimization-guide.md |
| Spot 优化 | Karpenter + Spot | domain-11-production-operations/99-karpenter-node-autoscaling-guide.md |

---

## 八、AI / ML 基础设施

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| MLOps 平台 | **Kubeflow** | domain-14-ai-ml-infra/99-kubeflow-ai-platform-guide.md |
| 分布式训练 | KubeRay + Volcano | domain-14-ai-ml-infra/00-open-source-projects-index.md |
| GPU 调度 | NVIDIA GPU Operator + Kueue | domain-14-ai-ml-infra/00-open-source-projects-index.md |
| 模型推理 | KServe | domain-14-ai-ml-infra/00-open-source-projects-index.md |

---

## 九、灾备 & 业务连续性

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| 集群资源备份 | **Velero** | domain-09-reliability-engineering/99-velero-backup-recovery-guide.md |
| 应用级备份 | Velero + CSI 快照 | domain-09-reliability-engineering/99-velero-backup-recovery-guide.md |
| 跨集群迁移 | Velero + 对象存储 | domain-09-reliability-engineering/99-velero-backup-recovery-guide.md |

---

## 十、前沿技术

| 场景 | 推荐方案 | 指南位置 |
|:---|:---|:---|
| Wasm 运行时 | **WasmEdge** | domain-15-specialized-tech/99-wasmedge-cloud-native-guide.md |
| Wasm 微服务 | **Spin** (Fermyon) | domain-15-specialized-tech/99-wasmedge-cloud-native-guide.md |
| 边缘 Serverless | Spin + KEDA | domain-15-specialized-tech/99-wasmedge-cloud-native-guide.md |

---

## 按角色快速索引

### SRE / 平台工程师
1. **必学**: Prometheus + Grafana + Loki + Tempo (可观测性栈)
2. **必学**: cert-manager + Falco + Kyverno (安全基线)
3. **必学**: Velero + Karpenter (运维基础)
4. **进阶**: Istio / Cilium / Crossplane

### 应用开发者
1. **必学**: kubectl + k9s + stern (日常工具)
2. **必学**: mirrord / Telepresence (本地开发)
3. **进阶**: Argo CD / Flux (GitOps)

### 安全工程师
1. **必学**: Falco + Kyverno + OPA (策略与监控)
2. **必学**: Vault + cert-manager (密钥与证书)
3. **进阶**: cosign + Tekton Chains + SLSA (供应链)

### FinOps / 成本管理
1. **必学**: OpenCost / Kubecost (成本可视化)
2. **必学**: Infracost (IaC 预估)
3. **进阶**: Karpenter + Spot (成本优化)

---

## 参考

- 全景项目列表: [OPEN-SOURCE-ECOSYSTEM.md](./OPEN-SOURCE-ECOSYSTEM.md)
- 各 Domain 详细索引: `domain-*/00-open-source-projects-index.md`
- 独立深度指南: `domain-*/99-*-guide.md`
