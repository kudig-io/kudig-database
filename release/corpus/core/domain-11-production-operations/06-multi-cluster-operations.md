---
title: 多集群与舰队运维指南
description: Kubernetes 多集群与舰队（Fleet）运维指南，覆盖集群注册表、舰队策略、Secret 同步、全局负载均衡、跨集群可观测性与灾难恢复。
summary: 多集群与舰队运维指南，覆盖集群注册表、舰队策略、Secret 同步、全局负载均衡、跨集群可观测性与 DR。
category: production-operations
tags:
- production
- best-practices
- playbook
- multi-cluster
- fleet
- cluster-registry
- secret-sync
- global-load-balancing
- cross-cluster-observability
- disaster-recovery
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 云架构师
estimated_read_time: 25min
intent_queries:
- 多集群运维指南是什么
- 如何管理 Kubernetes 舰队
- 多集群 Secret 同步 全局负载均衡 跨集群可观测性 最佳实践
trigger_keywords:
- 多集群
- 舰队
- fleet
- cluster registry
- secret sync
- global load balancing
- cross-cluster observability
prerequisites:
- kubectl-basics
- multi-cluster-basics
- gitops-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

# 多集群与舰队运维指南

> **适用范围**: 管理两个及以上 Kubernetes 生产集群的组织，涵盖多云、多地域、混合云场景。
> **目标读者**: SRE、平台工程师、云架构师。
> **最后更新**: 2026-07-01

本指南是 [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]] 的多集群专项 runbook，参考 [[_reports/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]] 中“Multi-Cluster / Fleet Management & GitOps at Scale”缺口，覆盖集群注册表、舰队策略、Secret 同步、全局负载均衡、跨集群可观测性与灾难恢复。

---

## 1. 适用场景与范围

- 建立统一的集群清单（Cluster Registry）与元数据管理。
- 通过舰队策略（Fleet Policy）下发基线配置、NetworkPolicy、Pod Security、ResourceQuota。
- 跨集群同步 Secret、ConfigMap、TLS 证书。
- 实现全局负载均衡与跨集群服务发现（MCS、Gateway API、GSLB）。
- 统一监控、日志、trace 与告警。
- 跨集群灾备与故障切换。

---

## 2. 前置条件与工具

```bash
# 推荐工具
kubectl version --client
helm version
argocd version --client        # Argo CD ApplicationSet
clusteradm version             # OCM (Open Cluster Management)
karmadactl version             # Karmada
subctl version                 # Submariner 跨集群网络
```

- 各集群网络互联互通（VPN、专线、Submariner、Cilium Cluster Mesh）。
- 统一的身份认证与 RBAC 体系。
- 共享的对象存储或日志后端。

---

## 3. 核心概念/架构

```
                ┌─────────────────┐
                │   全局控制平面    │
                │ (OCM/Karmada/    │
                │  Argo CD Hub)    │
                └────────┬────────┘
                         │
      ┌──────────────────┼──────────────────┐
      ▼                  ▼                  ▼
  Cluster A         Cluster B          Cluster C
  (Region 1)        (Region 2)         (DR Region)
```

- **Cluster Registry**: 统一记录集群元数据、标签、访问端点、责任人。
- **Fleet Policy**: 通过 Policy 或 GitOps 在所有集群强制一致的安全、网络、配额基线。
- **Secret Sync**: 使用 External Secrets Operator、SOPS + Flux、或 OCM 策略同步 Secret。
- **Global Load Balancing**: 通过 DNS（Route 53/Cloudflare/GTM）或 Gateway API 将流量路由到健康集群。
- **Cross-cluster Observability**: Thanos / Cortex / VictoriaMetrics 聚合指标，Loki/Tempo 聚合日志与 trace。

---

## 4. 标准操作流程

### 4.1 集群注册表

```bash
# OCM Hub 注册集群
clusteradm init
clusteradm join --hub-token <token> --hub-apiserver <hub-api> --cluster-name prod-ap

# 查看托管集群
kubectl get managedclusters

# 为集群打标签（环境、区域、成本中心）
kubectl label managedcluster prod-ap region=ap-southeast-1 env=prod cost-center=platform
```

### 4.2 舰队策略下发（OCM Policy）

```bash
# 示例：在所有 prod 集群强制 Pod Security Admission
kubectl apply -f - <<EOF
apiVersion: policy.open-cluster-management.io/v1
kind: Policy
metadata:
  name: enforce-restricted-psa
  namespace: policies
spec:
  remediationAction: enforce
  disabled: false
  policy-templates:
  - objectDefinition:
      apiVersion: policy.open-cluster-management.io/v1
      kind: ConfigurationPolicy
      metadata:
        name: ns-psa-labels
      spec:
        object-templates:
        - complianceType: musthave
          objectDefinition:
            apiVersion: v1
            kind: Namespace
            metadata:
              name: production
              labels:
                pod-security.kubernetes.io/enforce: restricted
EOF

kubectl apply -f - <<EOF
apiVersion: apps.open-cluster-management.io/v1
kind: PlacementRule
metadata:
  name: prod-clusters
  namespace: policies
spec:
  clusterSelector:
    matchLabels:
      env: prod
EOF
```

### 4.3 Secret 同步

```bash
# External Secrets Operator 跨集群同步
helm upgrade --install external-secrets external-secrets/external-secrets -n external-secrets --create-namespace

# ClusterSecretStore 指向中心 Vault/云 KMS
kubectl apply -f - <<EOF
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: global-vault
spec:
  provider:
    vault:
      server: https://vault.internal
      path: secret
      auth:
        kubernetes:
          mountPath: kubernetes
          role: external-secrets
EOF
EOF
```

### 4.4 全局负载均衡

```bash
# 使用 Gateway API + 健康检查实现多集群入口
kubectl apply -f - <<EOF
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: global-gw
  namespace: ingress
spec:
  gatewayClassName: istio
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    hostname: api.example.com
    tls:
      mode: Terminate
      certificateRefs:
      - name: api-tls
EOF
```

生产常用方案：

- **DNS GSLB**: Route 53 / Cloudflare / 阿里云 GTM，按健康检查与地理位置路由。
- **ServiceExport/ServiceImport**: Kubernetes MCS API，实现跨集群服务发现。
- **Istio Multi-Primary / Cilium Cluster Mesh**: 服务网格级跨集群流量管理。

### 4.5 跨集群可观测性

```bash
# Thanos Query 联邦多个集群 Prometheus
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-query-global
  namespace: monitoring
spec:
  template:
    spec:
      containers:
      - name: thanos-query
        image: quay.io/thanos/thanos:v0.36.0
        args:
        - query
        - --store=dnssrv+thanos-sidecar.monitoring.svc.cluster.local:10901
        - --store=dnssrv+thanos-sidecar.monitoring.svc.cluster.local:10901
EOF
```

- 指标：Thanos / Cortex / VictoriaMetrics 远程写入中心对象存储。
- 日志：Fluent Bit / Promtail 将日志发送到共享 Loki / SLS。
- Trace：OpenTelemetry Collector 将 trace 汇聚到统一 Tempo / Jaeger。

### 4.6 灾难恢复与切换

```bash
# 演练：将 DNS 从主集群切换到灾备集群
aws route53 change-resource-record-sets \
  --hosted-zone-id ZXXX \
  --change-batch file://failover-to-dr.json

# 验证灾备集群应用状态
kubectl --context=dr-prod get pods -A
kubectl --context=dr-prod get svc -A
```

---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|---|---|---|
| 集群注册状态 | `kubectl get managedclusters` | Ready，标签完整 |
| 策略合规 | `kubectl get policyreport -A` | 无 NonCompliant |
| Secret 同步 | `kubectl get externalsecrets -A` | Ready，Secret 已创建 |
| 全局入口健康 | `dig api.example.com` + `curl -I https://api.example.com/health` | 返回健康集群 IP |
| 跨集群指标 | Thanos Query StoreAPI 状态 | 所有集群 sidecar 可达 |
| 灾备切换演练 | 模拟 DNS 切换后业务可用 | RTO/RPO 达标 |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| 新集群未纳入舰队 | join token 过期、网络不通 | `kubectl get managedcluster <name>` | 重新 join、检查防火墙 |
| 策略未下发 | PlacementRule 标签不匹配 | `kubectl describe placementrule prod-clusters` | 修正 clusterSelector |
| Secret 同步失败 | ClusterSecretStore 认证失败 | `kubectl describe externalsecret` | 检查 Vault role、SA token |
| 跨集群服务发现失败 | MCS 控制器未部署、网络策略拦截 | `kubectl get serviceexport -A` | 部署 MCS、放行跨集群流量 |
| 全局 DNS 路由异常 | 健康检查失败、TTL 过长 | `dig +trace api.example.com` | 修复后端健康检查、降低 TTL |
| 指标聚合缺失 | Thanos sidecar 未运行 | `kubectl get pods -n monitoring -l app=thanos-sidecar` | 重启 sidecar、检查 StoreAPI |

---

## 7. 风险与注意事项

- **控制平面单点**: Hub 集群故障会导致策略与同步中断，Hub 自身必须高可用并跨 AZ/Region。
- **网络延迟与带宽**: 跨集群 Secret 同步、metrics 远程写入会占用带宽，需评估专线容量。
- **版本一致性**: 各集群 Kubernetes 版本、CRD、策略引擎版本差异会导致策略下发失败。
- **Secret 扩散风险**: 跨集群同步 Secret 会增加泄露面，必须使用最小权限与加密传输。
- **DNS TTL 与缓存**: 灾备切换时过长的 TTL 会拖慢流量切换，建议生产 TTL ≤ 60s。
- **数据一致性**: 跨集群有状态服务需独立设计复制/同步机制，不能依赖舰队层。

---

## 8. 相关 Runbook / 推荐阅读

### 同域核心文档

- [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[domain-11-production-operations/03-on-call-playbook.md|值班手册与告警响应规范]]
- [[domain-11-production-operations/02-change-management-guide.md|变更管理指南]]

### 跨域参考

- [[_reports/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]]
- [[domain-12-cloud-providers/08-multi-cloud/00-multi-cloud-hybrid-deployment-strategy.md|多云混合部署策略]]
- [[domain-12-cloud-providers/08-multi-cloud/08-multicloud-federation-karmada.md|多集群联邦 Karmada]]
- [[domain-12-cloud-providers/08-multi-cloud/09-multicloud-network-interconnect.md|多云网络互联]]
- [[domain-12-cloud-providers/08-multi-cloud/10-multicloud-disaster-recovery.md|多云灾难恢复]]
- [[domain-07-platform-engineering/99-production-readiness-operations-guide.md|平台工程生产就绪运维指南]]
- [[domain-06-observability/README.md|可观测性域]]
- [[domain-05-security-compliance/README.md|安全合规域]]

---

*本指南应根据集群规模增长、选择的舰队技术栈与灾备演练结果持续迭代。建议每半年 review 一次全局架构图与 RTO/RPO。*
