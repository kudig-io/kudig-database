---
title: 14 - API 网关生产运维最佳实践
description: 'title: 14 - API 网关生产运维最佳实践'
category: general
tags:
- gateway
- api
- production
- etcd
- prometheus
- grafana
- envoy
- helm
- argocd
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- 14-api-gateway-production-operations生产环境怎么配置？
- 14-api-gateway-production-operations的生产级实践
- 生产环境中14-api-gateway-production-operations的注意事项
trigger_keywords:
- API
- 网关生产运维最佳实践
- networking
- traffic
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- redis-basics
- tls-basics
- backup-basics
created: "2026-05-23"
---

title: 14 - API 网关生产运维最佳实践
description: '# 14 - API 网关生产运维最佳实践'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- [[Envoy|envoy]]
- apisix
- higress
- [[etcd|etcd]]
- [[Prometheus|prometheus]]
- grafana
- [[Helm|helm]]
- argocd
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- API 网关生产运维最佳实践 是什么
- 如何 API 网关生产运维最佳实践
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- API
- 网关生产运维最佳实践
- cloud
- native
- api
- gateway
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

# 14 - API 网关生产运维最佳实践

> **文档版本**: v1.0 | **适用版本**: Kubernetes 1.27+ | **更新日期**: 2026-03-04 | **关键词**: HA, 滚动升级, GitOps, 证书, 灾备, 容量规划, 多租户, AI网关

<!-- chunk: 目录 -->## 目录

1. [高可用部署架构](#1-高可用部署架构)
2. [滚动升级策略](#2-滚动升级策略)
3. [GitOps 配置管理](#3-gitops-配置管理)
4. [证书生命周期管理](#4-证书生命周期管理)
5. [灾备与恢复](#5-灾备与恢复)
6. [容量规划](#6-容量规划)
7. [多租户网关模式](#7-多租户网关模式)
8. [AI 网关生产模式](#8-ai-网关生产模式)
9. [运维巡检清单](#9-运维巡检清单)
10. [故障应急手册](#10-故障应急手册)

---

<!-- chunk: 1. 高可用部署架构 -->## 1. 高可用部署架构

#<!-- chunk: 1.1 多活 + 区域感知架构 -->## 1.1 多活 + 区域感知架构

```
┌────────────────────────────────────────────────────────────────────────┐
│                     生产级 API 网关高可用架构                             │
│                                                                        │
│  ┌──────── 可用区 A ────────┐  ┌──────── 可用区 B ────────┐             │
│  │                         │  │                         │             │
│  │  ┌─────────────────┐    │  │  ┌─────────────────┐    │             │
│  │  │  Gateway Pod 1  │    │  │  │  Gateway Pod 2  │    │             │
│  │  │  (Active)       │    │  │  │  (Active)       │    │             │
│  │  └────────┬────────┘    │  │  └────────┬────────┘    │             │
│  │           │              │  │           │              │             │
│  │  ┌─────────────────┐    │  │  ┌─────────────────┐    │             │
│  │  │  Gateway Pod 3  │    │  │  │  Gateway Pod 4  │    │             │
│  │  │  (Active)       │    │  │  │  (Active)       │    │             │
│  │  └────────┬────────┘    │  │  └────────┬────────┘    │             │
│  │           │              │  │           │              │             │
│  └───────────┼──────────────┘  └───────────┼──────────────┘             │
│              │                             │                            │
│              └──────────┬──────────────────┘                            │
│                         │                                               │
│              ┌──────────▼──────────┐                                    │
│              │  Kubernetes Service  │                                    │
│              │  (LoadBalancer/NLB)  │                                    │
│              └──────────┬──────────┘                                    │
│                         │                                               │
│              ┌──────────▼──────────┐                                    │
│              │   外部负载均衡器       │                                    │
│              │   (云厂商 NLB/SLB)   │                                    │
│              └──────────┬──────────┘                                    │
│                         │                                               │
│              ┌──────────▼──────────┐                                    │
│              │   DNS (GSLB/GeoDNS)  │                                    │
│              │   多集群故障转移       │                                    │
│              └─────────────────────┘                                    │
└────────────────────────────────────────────────────────────────────────┘

关键 HA 指标：
  - RTO（恢复时间目标）: < 30 秒（单 Pod 故障）
  - RPO（恢复点目标）:  0（无状态网关，配置存储在 etcd）
  - 可用性目标: 99.99%（4个九，允许年停机 < 52分钟）
```

#<!-- chunk: 1.2 多集群网关联邦架构 -->## 1.2 多集群网关联邦架构

```
┌──────────────────────────────────────────────────────────────────┐
│                    多集群网关联邦                                    │
│                                                                  │
│  ┌─────────────────────┐      ┌─────────────────────┐            │
│  │    集群 A（主区域）    │      │    集群 B（灾备区域）   │            │
│  │                     │      │                     │            │
│  │  ┌───────────────┐  │      │  ┌───────────────┐  │            │
│  │  │ 控制平面       │  │◄────►│  │ 控制平面       │  │            │
│  │  │ (etcd cluster)│  │ 同步  │  │ (etcd cluster)│  │            │
│  │  └───────────────┘  │      │  └───────────────┘  │            │
│  │         ▲           │      │         ▲           │            │
│  │         │ 配置推送   │      │         │ 配置推送   │            │
│  │  ┌───────────────┐  │      │  ┌───────────────┐  │            │
│  │  │ 数据平面 (x4)  │  │      │  │ 数据平面 (x2)  │  │            │
│  │  │ Envoy/NGINX   │  │      │  │ Envoy/NGINX   │  │            │
│  │  └───────────────┘  │      │  └───────────────┘  │            │
│  └─────────────────────┘      └─────────────────────┘            │
│            │ 80% 流量                   │ 20% 流量                  │
│            └─────────────┬─────────────┘                          │
│                          │                                         │
│               ┌──────────▼──────────┐                              │
│               │   全局流量管理        │                              │
│               │  (AWS Route53 /      │                              │
│               │   阿里云 GTM)         │                              │
│               └─────────────────────┘                              │
└──────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 1.3 区域感知路由配置 -->## 1.3 区域感知路由配置

```yaml
# 拓扑分布约束 - 确保跨区高可用
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: gateway-system
spec:
  replicas: 6    # 建议: 每可用区至少 2 副本
  template:
    spec:
      topologySpreadConstraints:
      # 跨可用区均匀分布
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: api-gateway
      # 跨节点分散（避免单节点故障影响过大）
      - maxSkew: 1
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: api-gateway
      # 必须使用反亲和，确保不同区的 Pod 不共用节点
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: api-gateway
            topologyKey: topology.kubernetes.io/zone
```

---

<!-- chunk: 2. 滚动升级策略 -->## 2. 滚动升级策略

#<!-- chunk: 2.1 零停机升级模式 -->## 2.1 零停机升级模式

```
滚动升级时序图:

T=0    [Pod1:v1] [Pod2:v1] [Pod3:v1] [Pod4:v1]  ← 所有 v1 运行
T=30s  [Pod1:v2] [Pod2:v1] [Pod3:v1] [Pod4:v1]  ← Pod1 升级完成
T=60s  [Pod1:v2] [Pod2:v2] [Pod3:v1] [Pod4:v1]  ← Pod2 升级完成
T=90s  [Pod1:v2] [Pod2:v2] [Pod3:v2] [Pod4:v1]  ← Pod3 升级完成
T=120s [Pod1:v2] [Pod2:v2] [Pod3:v2] [Pod4:v2]  ← 升级完成

关键保障机制:
  ① PDB 确保最少 3 个 Pod 可用
  ② preStop hook 等待连接排空（graceful drain）
  ③ readinessProbe 就绪才切流量
  ④ minReadySeconds 稳定后再升级下一个
```

#<!-- chunk: 2.2 PodDisruptionBudget 配置 -->## 2.2 PodDisruptionBudget 配置

```yaml
# 网关 PDB - 保障升级期间最低服务能力
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: gateway-pdb
  namespace: gateway-system
spec:
  # 最少保留 75% 副本（4副本时=3个）
  minAvailable: "75%"
  # 或者使用 maxUnavailable: 1
  selector:
    matchLabels:
      app: api-gateway
```

#<!-- chunk: 2.3 优雅排水配置 -->## 2.3 优雅排水配置

```yaml
# Deployment - 优雅关闭配置
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1          # 升级时最多 1 个不可用
      maxSurge: 1                # 允许超出 replicas 数量 1 个
  template:
    spec:
      # 终止宽限期（必须大于 preStop + 排水时间）
      terminationGracePeriodSeconds: 90

      containers:
      - name: gateway
        # 就绪探针（决定何时接受流量）
        readinessProbe:
          httpGet:
            path: /healthz/ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          successThreshold: 1
          failureThreshold: 3
        # 存活探针（决定何时重启）
        livenessProbe:
          httpGet:
            path: /healthz/live
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
          failureThreshold: 3

        # 优雅关闭钩子
        lifecycle:
          preStop:
            exec:
              command:
              - /bin/sh
              - -c
              - |
                # 1. 标记为不健康（停止接收新连接）
                echo "Draining connections..."
                # 2. 等待现有连接处理完毕
                sleep 30
                # 3. 通知 Envoy 开始排水
                curl -s -X POST localhost:9901/drain_listeners
                sleep 20
```

#<!-- chunk: 2.4 版本兼容性矩阵 -->## 2.4 版本兼容性矩阵

| 升级路径 | 控制平面兼容 | 配置 CRD 兼容 | 插件 API 兼容 | 推荐方式 |
|---------|-----------|-------------|-------------|---------|
| **Higress** 2.0 → 2.1 | ✅ 完全兼容 | ✅ 向后兼容 | ✅ 稳定 | 直接滚动升级 |
| **Higress** 1.x → 2.x | ⚠️ 需迁移 | ⚠️ CRD 版本变更 | ⚠️ 部分 API 变更 | 蓝绿升级 |
| **APISIX** 3.7 → 3.8 | ✅ 完全兼容 | ✅ 向后兼容 | ✅ 稳定 | 直接滚动升级 |
| **Kong** 3.5 → 3.6 | ✅ 完全兼容 | ✅ 向后兼容 | ✅ 稳定 | 直接滚动升级 |
| **Kong** 2.x → 3.x | ❌ 不兼容 | ❌ 重大变更 | ❌ 插件重写 | 蓝绿+全量测试 |
| **Envoy Gateway** 0.x → 1.x | ⚠️ API 变更 | ⚠️ v1alpha1→v1 | ✅ 稳定 | 金丝雀升级 |

---

<!-- chunk: 3. GitOps 配置管理 -->## 3. GitOps 配置管理

#<!-- chunk: 3.1 GitOps 工作流架构 -->## 3.1 GitOps 工作流架构

```
┌──────────────────────────────────────────────────────────────┐
│                  API 网关 GitOps 工作流                        │
│                                                              │
│  开发者                Git仓库              集群              │
│  ┌──────┐   PR/MR    ┌────────┐  ArgoCD   ┌────────────┐    │
│  │ Dev  │───────────▶│ gateway│──────────▶│ Gateway    │    │
│  │ team │            │ config │           │ Deployment │    │
│  └──────┘            │ repo   │           └────────────┘    │
│                      │        │                             │
│  ┌──────┐   Review   │ /helm  │  Helm     ┌────────────┐    │
│  │ SRE  │◄──────────│ /adc   │──────────▶│ Config     │    │
│  │ team │            │ /deck  │           │ CRD/Plugin │    │
│  └──────┘            └────────┘           └────────────┘    │
│      │                                                      │
│      │ 审批/合并                                              │
│      ▼                                                      │
│  ┌───────────────────────────────────────────────────┐      │
│  │              CI/CD Pipeline                        │      │
│  │                                                   │      │
│  │  Lint → Validate → Diff Preview → Apply → Verify │      │
│  └───────────────────────────────────────────────────┘      │
└──────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 3.2 Kong deck 配置管理 -->## 3.2 Kong deck 配置管理

```yaml
# deck.yaml - Kong 声明式配置示例
_format_version: "3.0"
_info:
  select_tags:
  - production
  - managed-by-gitops

services:
- name: user-service
  url: http://user-service.default.svc.cluster.local
  tags: [production]
  routes:
  - name: user-api
    paths:
    - /api/v1/users
    methods: [GET, POST, PUT, DELETE]
    strip_path: false
  plugins:
  - name: jwt
    config:
      secret_is_base64: false
      claims_to_verify: [exp, nbf]
  - name: rate-limiting-advanced
    config:
      limit: [1000]
      window_size: [60]
      strategy: redis
      redis:
        host: redis.cache.svc.cluster.local
        port: 6379
```

```bash
# CI/CD 中使用 deck 进行差异检查和应用
# 1. 差异预览（PR Review 阶段）
deck gateway diff \
  --state deck.yaml \
  --kong-addr https://kong-admin:8444 \
  --tls-server-name kong-admin \
  --output-format json | tee diff-report.json

# 2. 验证配置合法性
deck gateway validate --state deck.yaml

# 3. 应用配置（CD 阶段，需审批通过）
deck gateway sync \
  --state deck.yaml \
  --select-tag production \
  --kong-addr https://kong-admin:8444
```

#<!-- chunk: 3.3 APISIX ADC 配置管理 -->## 3.3 APISIX ADC 配置管理

```yaml
# apisix-config/routes.yaml - ADC 格式
routes:
  - id: user-api-route
    name: user-api
    uris:
      - /api/v1/users*
    methods:
      - GET
      - POST
    upstream_id: user-service
    plugin_config_id: standard-security

upstreams:
  - id: user-service
    name: user-service
    type: roundrobin
    nodes:
      - host: user-service.default.svc.cluster.local
        port: 80
        weight: 1
    health_check:
      passive:
        healthy:
          successes: 2
        unhealthy:
          http_failures: 3

plugin_configs:
  - id: standard-security
    desc: "标准安全插件链"
    plugins:
      jwt-auth:
        enable: true
      limit-count:
        count: 1000
        time_window: 60
        key: consumer_name
        policy: redis
        redis_host: redis.cache.svc.cluster.local
```

```bash
# ADC 工作流
# 验证配置
adc lint --backend apisix routes.yaml

# 差异对比
adc diff --backend apisix \
  --server http://apisix-admin:9180 \
  --token $ADMIN_KEY \
  routes.yaml

# 同步配置
adc sync --backend apisix \
  --server http://apisix-admin:9180 \
  --token $ADMIN_KEY \
  routes.yaml
```

#<!-- chunk: 3.4 ArgoCD + Helm 统一管理 -->## 3.4 ArgoCD + Helm 统一管理

```yaml
# argocd-gateway-app.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: api-gateway
  namespace: argocd
spec:
  project: infrastructure
  source:
    repoURL: https://github.com/your-org/gateway-config
    targetRevision: HEAD
    path: helm/gateway
    helm:
      valueFiles:
      - values-production.yaml
      parameters:
      - name: replicaCount
        value: "4"
      - name: image.tag
        value: "v2.1.0"
  destination:
    server: https://kubernetes.default.svc
    namespace: gateway-system
  syncPolicy:
    automated:
      prune: true        # 自动删除不在 Git 中的资源
      selfHeal: true     # 自动修复漂移的配置
    syncOptions:
    - CreateNamespace=true
    - ServerSideApply=true
    retry:
      limit: 3
      backoff:
        duration: 5s
        maxDuration: 3m
        factor: 2
```

---

<!-- chunk: 4. 证书生命周期管理 -->## 4. 证书生命周期管理

#<!-- chunk: 4.1 cert-manager 集成架构 -->## 4.1 cert-manager 集成架构

```
┌──────────────────────────────────────────────────────────────┐
│                  证书生命周期管理架构                            │
│                                                              │
│  ┌──────────────────┐    ┌──────────────────┐               │
│  │  Let's Encrypt   │    │  企业 CA          │               │
│  │  (ACME)          │    │  (Vault PKI)     │               │
│  └────────┬─────────┘    └────────┬─────────┘               │
│           │                       │                         │
│           └──────────┬────────────┘                         │
│                      │                                      │
│           ┌──────────▼──────────┐                           │
│           │    cert-manager     │                           │
│           │    (颁发+续期)       │                           │
│           └──────────┬──────────┘                           │
│                      │ 存储为 Secret                         │
│           ┌──────────▼──────────┐                           │
│           │   Kubernetes Secret  │                           │
│           │   (TLS cert/key)    │                           │
│           └──────────┬──────────┘                           │
│                      │ 挂载/引用                              │
│           ┌──────────▼──────────┐                           │
│           │    API 网关          │                           │
│           │  (Gateway/Ingress)  │                           │
│           └─────────────────────┘                           │
│                                                              │
│  监控: 证书到期告警（< 30 天）→ PagerDuty                        │
└──────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 4.2 ACME 自动化证书配置 -->## 4.2 ACME 自动化证书配置

```yaml
# 1. ClusterIssuer - Let's Encrypt 生产颁发者
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: ops@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-key
    solvers:
    # HTTP-01 挑战（适合有 Ingress 的场景）
    - http01:
        ingress:
          class: higress
    # DNS-01 挑战（适合通配符证书）
    - dns01:
        cloudDNS:
          project: my-gcp-project
          serviceAccountSecretRef:
            name: clouddns-dns01-solver-svc-acct
            key: key.json
      selector:
        dnsZones:
        - "*.example.com"

---
# 2. Certificate 资源
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-gateway-tls
  namespace: gateway-system
spec:
  secretName: api-gateway-tls-secret
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  # 提前 30 天自动续期
  renewBefore: 720h    # 30 天
  dnsNames:
  - api.example.com
  - "*.api.example.com"
  # 私钥算法
  privateKey:
    algorithm: ECDSA
    size: 256
  # 保留旧证书直到新证书就绪
  usages:
  - digital signature
  - key encipherment
```

#<!-- chunk: 4.3 证书到期监控 -->## 4.3 证书到期监控

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-expiry-alerts
  namespace: monitoring
spec:
  groups:
  - name: certificates
    rules:
    # 证书 30 天内到期告警
    - alert: CertificateExpiringIn30Days
      expr: |
        (x509_cert_expiry - time()) / 86400 < 30
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "证书即将到期"
        description: "证书 {{ $labels.subject_common_name }} 将在 {{ $value | printf \"%.0f\" }} 天后到期"
    # 证书 7 天内到期紧急告警
    - alert: CertificateExpiringIn7Days
      expr: |
        (x509_cert_expiry - time()) / 86400 < 7
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "【紧急】证书即将到期！"
        description: "证书 {{ $labels.subject_common_name }} 将在 {{ $value | printf \"%.0f\" }} 天后到期，请立即处理！"
```

---

<!-- chunk: 5. 灾备与恢复 -->## 5. 灾备与恢复

#<!-- chunk: 5.1 各产品备份策略对比 -->## 5.1 各产品备份策略对比

| 产品 | 配置存储 | 备份工具 | 备份频率 | 恢复命令 | RTO |
|------|---------|---------|---------|---------|-----|
| **Higress** | etcd (K8s CRD) | Velero | 每小时 | `velero restore create` | < 5min |
| **APISIX** | etcd (独立) | etcdctl snapshot | 每15分钟 | `etcdctl snapshot restore` | < 3min |
| **Kong** | PostgreSQL | pg_dump / Velero | 每小时 | `pg_restore` | < 10min |
| **Envoy GW** | etcd (K8s CRD) | Velero | 每小时 | `velero restore create` | < 5min |
| **Traefik** | Kubernetes 原生 | Velero | 每小时 | `velero restore create` | < 3min |

#<!-- chunk: 5.2 APISIX etcd 备份 -->## 5.2 APISIX etcd 备份

```bash
#!/bin/bash
# apisix-etcd-backup.sh - APISIX etcd 定期备份脚本

ETCD_ENDPOINTS="https://etcd-0:2379,https://etcd-1:2379,https://etcd-2:2379"
BACKUP_DIR="/backup/apisix-etcd"
BACKUP_RETENTION_DAYS=7
DATE=$(date +%Y%m%d-%H%M%S)

# 创建快照
echo "[${DATE}] 开始备份 APISIX etcd..."
ETCDCTL_API=3 etcdctl snapshot save \
  --endpoints=$ETCD_ENDPOINTS \
  --cacert=/etc/etcd/ca.crt \
  --cert=/etc/etcd/client.crt \
  --key=/etc/etcd/client.key \
  "${BACKUP_DIR}/snapshot-${DATE}.db"

# 验证快照完整性
ETCDCTL_API=3 etcdctl snapshot status \
  "${BACKUP_DIR}/snapshot-${DATE}.db" \
  --write-out=table

# 上传到对象存储（S3/OSS）
aws s3 cp "${BACKUP_DIR}/snapshot-${DATE}.db" \
  "s3://my-backup-bucket/apisix-etcd/snapshot-${DATE}.db" \
  --storage-class STANDARD_IA

# 清理过期备份
find $BACKUP_DIR -name "snapshot-*.db" \
  -mtime +$BACKUP_RETENTION_DAYS -delete

echo "[$(date +%Y%m%d-%H%M%S)] 备份完成: snapshot-${DATE}.db"
```

#<!-- chunk: 5.3 跨区域故障转移 -->## 5.3 跨区域故障转移

```
跨区域故障转移流程:

正常状态:
  DNS: api.example.com → Region-A (100% 流量)

故障检测:
  ┌──────────────────────────────────────────────┐
  │  监控系统发现 Region-A 健康检查失败 > 3次     │
  │  告警触发 → PagerDuty → On-call SRE          │
  └──────────────────────────────────────────────┘
                      ↓

自动切换（Route53 健康检查）:
  DNS: api.example.com → Region-B (100% 流量)
  切换时间: < 60 秒（TTL=30s）

手动验证:
  curl -v https://api.example.com/healthz
  → 确认流量已切换至 Region-B

Region-A 恢复后:
  1. 验证 Region-A 服务健康
  2. 逐步切回: 10% → 50% → 100%（使用加权路由）
```

```bash
# AWS Route53 故障转移脚本
#!/bin/bash
# dns-failover.sh

PRIMARY_REGION="ap-northeast-1"
SECONDARY_REGION="ap-southeast-1"
HOSTED_ZONE_ID="Z1234567890"
RECORD_NAME="api.example.com"

# 检查主区域健康状态
check_health() {
    local endpoint="$1"
    local status=$(curl -s -o /dev/null -w "%{http_code}" \
        --connect-timeout 5 \
        --max-time 10 \
        "$endpoint/healthz")
    echo $status
}

PRIMARY_STATUS=$(check_health "https://$PRIMARY_REGION-gateway.internal")

if [ "$PRIMARY_STATUS" != "200" ]; then
    echo "主区域不健康 (HTTP $PRIMARY_STATUS)，执行故障转移..."

    # 更新 DNS 权重，将流量切到备用区域
    aws route53 change-resource-record-sets \
        --hosted-zone-id $HOSTED_ZONE_ID \
        --change-batch '{
            "Changes": [{
                "Action": "UPSERT",
                "ResourceRecordSet": {
                    "Name": "'$RECORD_NAME'",
                    "Type": "A",
                    "SetIdentifier": "primary",
                    "Weight": 0,
                    "TTL": 30,
                    "ResourceRecords": [{"Value": "REGION_A_IP"}]
                }
            }]
        }'

    # 告警通知
    curl -X POST $SLACK_WEBHOOK_URL \
        -H 'Content-type: application/json' \
        --data '{"text":"⚠️ API Gateway 故障转移已执行！流量已切换到备用区域。"}'
fi
```

---

<!-- chunk: 6. 容量规划 -->## 6. 容量规划

#<!-- chunk: 6.1 流量增长估算模型 -->## 6.1 流量增长估算模型

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
流量增长模型：

  未来 N 月的峰值 QPS = 当前峰值 QPS × (1 + 月增长率)^N × 峰值倍数

参数参考：
  月增长率: 普通业务 5-15%，快速增长业务 20-40%
  峰值倍数: B2C 业务 3-5x，B2B 业务 1.5-2x

示例（当前峰值 50K QPS，月增长 15%，规划 12 个月）：
  未来峰值 = 50,000 × (1.15)^12 × 3
           = 50,000 × 5.35 × 3
           = 802,500 QPS

规划建议：
  ├─ 当前配置: 4 副本 × 8C = 32C（可处理 80K QPS）
  ├─ 6个月后: 8 副本 × 8C = 64C（可处理 160K QPS）
  └─ 12个月后: 16 副本 × 16C = 256C（可处理 800K QPS）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 6.2 HPA + VPA 协同配置 -->## 6.2 HPA + VPA 协同配置

```yaml
# HPA - 水平自动扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: gateway-hpa
  namespace: gateway-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-gateway
  minReplicas: 4     # 保证 HA 最低副本
  maxReplicas: 32    # 基于容量规划上限
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 65   # 65% 触发扩容，留余量应对突发
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 75
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0       # 立即扩容（应对流量突增）
      policies:
      - type: Percent
        value: 100        # 一次最多翻倍
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 600     # 缩容保守，稳定 10 分钟后缩
      policies:
      - type: Pods
        value: 1          # 每次最多缩 1 个
        periodSeconds: 120

---
# VPA - 垂直自动扩缩容（仅推荐模式）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: gateway-vpa
  namespace: gateway-system
spec:
  targetRef:
    apiVersion: "apps/v1"
    kind: Deployment
    name: api-gateway
  updatePolicy:
    updateMode: "Off"   # 仅推荐，不自动修改（网关不适合频繁重启）
  resourcePolicy:
    containerPolicies:
    - containerName: gateway
      minAllowed:
        cpu: "4"
        memory: 4Gi
      maxAllowed:
        cpu: "16"
        memory: 16Gi
```

---

<!-- chunk: 7. 多租户网关模式 -->## 7. 多租户网关模式

#<!-- chunk: 7.1 共享网关 vs 独立网关 -->## 7.1 共享网关 vs 独立网关

```
模式对比：

┌─────────────────────────────┬─────────────────────────────┐
│        共享网关模式           │        独立网关模式           │
├─────────────────────────────┼─────────────────────────────┤
│  ┌───────────────────────┐  │  ┌──────┐  ┌──────┐        │
│  │     共享网关 (x4)      │  │  │租户A │  │租户B │        │
│  │  ┌────┐┌────┐┌────┐  │  │  │网关  │  │网关  │        │
│  │  │租户│├租户│├租户│  │  │  │(x2) │  │(x2) │        │
│  │  │ A  ││ B  ││ C  │  │  │  └──────┘  └──────┘        │
│  │  └────┘└────┘└────┘  │  │                             │
│  └───────────────────────┘  │  完全隔离，独立生命周期          │
│                             │                             │
│  优点: 资源利用率高，运维统一  │  优点: 完全隔离，故障不互相影响   │
│  缺点: 故障影响范围大         │  缺点: 资源浪费，运维复杂度高    │
│  适用: 内部平台/中小租户       │  适用: 高付费/合规要求租户      │
└─────────────────────────────┴─────────────────────────────┘
```

#<!-- chunk: 7.2 Namespace 隔离配置 -->## 7.2 Namespace 隔离配置

```yaml
# 多租户 Gateway API 隔离方案
# 每个租户拥有独立 Namespace，共享 GatewayClass，各自管理 HTTPRoute

# 租户 A Namespace
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: tenant-a
    tier: standard

---
# 网关控制器 RBAC - 限制租户只能管理自己 Namespace 的路由
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: gateway-route-manager
  namespace: tenant-a
rules:
- apiGroups: ["gateway.networking.k8s.io"]
  resources: ["httproutes", "grpcroutes"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["gateway.networking.k8s.io"]
  resources: ["gateways"]
  verbs: ["get", "list", "watch"]   # 只读，不可修改共享网关

---
# 租户 A 的路由（绑定到共享网关）
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: tenant-a-api
  namespace: tenant-a
spec:
  parentRefs:
  - name: shared-gateway
    namespace: gateway-system
    sectionName: https
  hostnames:
  - "api-a.example.com"    # 租户专属域名
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /
    backendRefs:
    - name: tenant-a-service
      port: 80
```

#<!-- chunk: 7.3 资源配额与速率限制 -->## 7.3 资源配额与速率限制

```yaml
# 租户级别资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-a-quota
  namespace: tenant-a
spec:
  hard:
    # 限制路由数量
    count/httproutes.gateway.networking.k8s.io: "50"
    # 限制后端服务数量
    count/services: "20"

---
# 租户级别全局限流（APISIX 示例）
apiVersion: apisix.apache.org/v2
kind: ApisixPluginConfig
metadata:
  name: tenant-a-ratelimit
  namespace: tenant-a
spec:
  plugins:
  - name: limit-count
    enable: true
    config:
      # 租户 A 总配额: 每分钟 10 万次请求
      count: 100000
      time_window: 60
      key: "tenant-a"
      key_type: constant
      policy: redis
      redis_host: redis-cluster.cache.svc.cluster.local
      redis_prefix: "tenant_quota:"
      rejected_code: 429
      rejected_msg: "租户配额超限，请联系管理员升级套餐"
```

---

<!-- chunk: 8. AI 网关生产模式 -->## 8. AI 网关生产模式

#<!-- chunk: 8.1 LLM 上游故障转移 -->## 8.1 LLM 上游故障转移

```
AI 网关 LLM 路由策略:

┌─────────────────────────────────────────────────────────────────┐
│                   LLM 多上游故障转移架构                           │
│                                                                 │
│  客户端请求                                                      │
│       │                                                        │
│       ▼                                                        │
│  ┌────────────┐                                                │
│  │ AI 网关     │  ← Token 计数、成本追踪、语义缓存                │
│  └─────┬──────┘                                                │
│        │                                                       │
│   ┌────▼────┐    健康检查失败 → 自动切换                          │
│   │ 路由策略 │                                                   │
│   └────┬────┘                                                  │
│        │                                                       │
│   ┌────▼──────────────────────────────────────┐               │
│   │               LLM 上游池                   │               │
│   │                                           │               │
│   │  主: OpenAI GPT-4  (权重 70%, 优先级 1)    │               │
│   │  备: Azure OpenAI  (权重 20%, 优先级 2)    │               │
│   │  兜: 内部 LLaMA    (权重 10%, 优先级 3)    │               │
│   └───────────────────────────────────────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 8.2 Higress AI 网关配置 -->## 8.2 Higress AI 网关配置

```yaml
# AI 路由配置 - Higress
apiVersion: networking.higress.io/v1
kind: McpBridge
metadata:
  name: llm-providers
  namespace: higress-system
spec:
  registries:
  - name: openai-primary
    type: openai
    domain: api.openai.com
    port: 443
    protocol: https

---
# AI 网关插件配置
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-proxy
  namespace: higress-system
spec:
  pluginName: ai-proxy
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-proxy:1.0.0
  defaultConfig:
    provider:
      type: openai
      apiTokens:
      - "${OPENAI_API_KEY}"
    modelMapping:
      "gpt-4": "gpt-4-turbo-preview"
      "gpt-3.5": "gpt-3.5-turbo"
    # Token 预算管理
    tokenBudget:
      enabled: true
      dailyLimit: 10000000    # 每日 1000 万 Token 限制
      perUserLimit: 100000    # 每用户每日 10 万 Token
    # 语义缓存
    semanticCache:
      enabled: true
      similarity_threshold: 0.95
      ttl: 3600
    # 故障转移
    failover:
      enabled: true
      providers:
      - type: azure_openai
        endpoint: "https://my-azure.openai.azure.com"
        apiKey: "${AZURE_OPENAI_KEY}"
      - type: ollama
        endpoint: "http://ollama.ai-platform.svc.cluster.local:11434"
```

#<!-- chunk: 8.3 成本追踪与 Token 预算 -->## 8.3 成本追踪与 Token 预算

```yaml
# Prometheus 指标采集 - AI 网关成本追踪
# 关键指标
ai_gateway_token_usage_total{model, provider, user, endpoint}  # Token 累计用量
ai_gateway_request_cost_dollars{model, provider}               # 请求成本（美元）
ai_gateway_cache_hit_ratio                                      # 语义缓存命中率
ai_gateway_provider_latency_p99{provider}                      # 各提供商 P99 延迟
ai_gateway_failover_count_total{from_provider, to_provider}    # 故障转移次数

# Grafana Dashboard 告警
- Token 日用量超过预算 80%
- 单次请求 Token > 4000（异常请求检测）
- 语义缓存命中率 < 30%（缓存效率差）
- LLM P99 延迟 > 10s（上游性能劣化）
```

---

<!-- chunk: 9. 运维巡检清单 -->## 9. 运维巡检清单

#<!-- chunk: 9.1 日常巡检（每日） -->## 9.1 日常巡检（每日）

```
📋 API 网关每日巡检清单

【流量健康】
□ 检查 QPS 趋势，与昨日同期对比（波动 > 20% 需排查）
□ 检查错误率（5xx），目标 < 0.1%
□ 检查 P99 延迟，与基线对比（波动 > 50% 需排查）
□ 检查活跃连接数，是否接近 connection pool 上限

【系统资源】
□ 检查所有网关 Pod CPU 利用率（告警阈值 > 80%）
□ 检查所有网关 Pod 内存使用（告警阈值 > 85%）
□ 检查 etcd/数据库磁盘使用率（告警阈值 > 70%）
□ 检查日志磁盘使用率（告警阈值 > 80%）

【可用性】
□ 验证所有 Pod 状态为 Running，无 CrashLoop/Pending
□ 检查 Ready 比例（应等于 Desired）
□ 检查 PodDisruptionBudget 状态
□ 验证 HPA 当前副本数符合预期

【证书】
□ 检查 TLS 证书有效期（< 30 天告警）
□ 确认 cert-manager 无 Failed Certificate 对象

【告警】
□ 检查 Alertmanager 未处理告警
□ 确认昨日所有 P1/P2 告警已解决或有跟进
```

#<!-- chunk: 9.2 每周巡检 -->## 9.2 每周巡检

```
📋 API 网关每周巡检清单

【性能趋势】
□ 分析周 QPS 增长趋势，预测容量需求
□ 对比本周与上周 P99/P999 延迟，识别劣化趋势
□ 检查是否有长尾 API 响应时间异常（Top 10 慢接口）
□ 分析 HPA 扩缩容历史，评估 min/maxReplicas 是否合理

【配置审计】
□ 检查本周新增/修改的路由配置，确认无误
□ 审查 RBAC 变更记录
□ 验证 GitOps 同步状态（ArgoCD Sync Status = Synced）
□ 检查是否有漂移（ArgoCD OutOfSync）

【安全】
□ 检查 WAF 日志，分析攻击趋势
□ 查看频率超限日志，是否有异常客户端
□ 检查 TLS 证书链完整性
□ 扫描使用的镜像 CVE（trivy image scan）

【备份验证】
□ 验证本周自动备份是否成功（检查 S3/OSS 备份文件）
□ 执行一次备份恢复演练（恢复到测试环境验证）
```

#<!-- chunk: 9.3 每月巡检 -->## 9.3 每月巡检

```
📋 API 网关每月巡检清单

【容量规划】
□ 对比实际流量与容量规划，调整下季度规划
□ 评估是否需要扩容（节点/副本/资源规格）
□ 分析 Top 流量路由，优化资源分配

【版本管理】
□ 检查网关产品新版本，评估升级计划
□ 检查依赖组件版本（cert-manager、ingress-nginx 等）
□ 检查 Kubernetes 版本，规划升级路径

【容灾演练】
□ 执行一次单节点故障模拟（kubectl drain）
□ 验证自动故障转移是否正常（跨区）
□ 测试配置回滚流程（GitOps 回滚到上月版本）
□ 演练证书手动续期流程

【成本优化】
□ 分析云资源成本，识别优化点
□ 检查 Reserved Instance 覆盖率
□ 评估 AI 网关 Token 使用效率（缓存命中率）
□ 识别低流量路由，评估是否可合并或下线
```

---

<!-- chunk: 10. 故障应急手册 -->## 10. 故障应急手册

#<!-- chunk: 10.1 故障场景索引 -->## 10.1 故障场景索引

| 故障类型 | 影响范围 | 紧急程度 | 处理章节 |
|---------|---------|---------|---------|
| 网关全部 Pod 不可用 | 全量服务中断 | P0 | §10.2 |
| 网关 CPU 打满 / 响应超时 | 延迟飙升 | P1 | §10.3 |
| 证书过期 / HTTPS 故障 | HTTPS 流量中断 | P1 | §10.4 |
| 配置推送失败 / 配置不同步 | 新路由不生效 | P2 | §10.5 |
| 限流误杀正常流量 | 部分用户 429 | P2 | §10.6 |
| etcd 异常 / 控制平面故障 | 控制面不可用 | P1 | §10.7 |

#<!-- chunk: 10.2 网关全部 Pod 不可用（P0） -->## 10.2 网关全部 Pod 不可用（P0）

```bash
# ============================================
# P0 故障：网关全部不可用
# 目标：5 分钟内恢复服务
# ============================================

# Step 1: 确认故障范围（60秒内）
kubectl get pods -n gateway-system -o wide
kubectl get events -n gateway-system --sort-by='.lastTimestamp' | tail -20

# Step 2: 快速判断原因
# 情况A: Pod 全部 Pending（资源不足）
kubectl describe pod -n gateway-system | grep -A5 "Events:"
# → 紧急：kubectl scale deployment api-gateway --replicas=2（减少副本）
# → 或：kubectl taint nodes <node> key:NoSchedule- （解除污点）

# 情况B: Pod 全部 CrashLoopBackOff（应用崩溃）
kubectl logs -n gateway-system <pod-name> --previous
# → 紧急回滚到上个版本:
kubectl rollout undo deployment/api-gateway -n gateway-system
kubectl rollout status deployment/api-gateway -n gateway-system

# 情况C: Pod 全部 Running 但健康检查失败
kubectl exec -n gateway-system <pod-name> -- curl -s localhost:8080/healthz
# → 检查上游是否全部不可用
# → 临时关闭健康检查: 修改 readinessProbe failureThreshold=100

# Step 3: 验证恢复
kubectl get pods -n gateway-system   # 确认 READY
curl -v https://api.example.com/healthz   # 确认外部可访问

# Step 4: 触发告警升级（如 5 分钟内未恢复）
# 联系二线值班 / 网关产品负责人
```

#<!-- chunk: 10.3 网关高延迟 / CPU 打满（P1） -->## 10.3 网关高延迟 / CPU 打满（P1）

```bash
# ============================================
# P1 故障：网关延迟飙升 / CPU 使用率 > 95%
# ============================================

# Step 1: 快速定位流量异常
# 查看实时 QPS（是否有流量突增）
kubectl exec -n gateway-system <pod> -- \
  curl -s localhost:9901/stats | grep downstream_rq_total

# Step 2: 检查是否有 DDoS 或爬虫
kubectl logs -n gateway-system <pod> \
  --since=5m | awk '{print $1}' | sort | uniq -c | sort -rn | head -20

# Step 3: 临时限流（紧急措施）
# 对可疑 IP 或 User-Agent 添加全局限流
kubectl apply -f - <<EOF
apiVersion: apisix.apache.org/v2
kind: ApisixGlobalRule
metadata:
  name: emergency-ratelimit
  namespace: gateway-system
spec:
  plugins:
  - name: limit-req
    enable: true
    config:
      rate: 10000      # 全局每秒 1 万请求
      burst: 2000
      rejected_code: 503
EOF

# Step 4: 临时扩容（HPA 不够快时）
kubectl scale deployment api-gateway \
  -n gateway-system --replicas=16

# Step 5: 如确认为异常流量，启用 WAF 封锁
```

#<!-- chunk: 10.4 证书过期（P1） -->## 10.4 证书过期（P1）

```bash
# ============================================
# P1 故障：TLS 证书过期，HTTPS 无法访问
# ============================================

# Step 1: 确认证书状态
kubectl get certificate -n gateway-system
kubectl describe certificate api-gateway-tls -n gateway-system

# Step 2: 手动触发 cert-manager 续期
# 方式A: 删除 Secret，强制重新申请
kubectl delete secret api-gateway-tls-secret -n gateway-system
# cert-manager 会自动重新申请（等待 2-5 分钟）

# 方式B: 使用 cmctl 手动续期
kubectl cert-manager renew api-gateway-tls -n gateway-system

# Step 3: 监控续期进度
kubectl get certificate -n gateway-system -w

# Step 4: 临时降级（如 ACME 申请失败）
# 使用自签名证书临时恢复服务
openssl req -x509 -nodes -days 7 \
  -newkey rsa:2048 \
  -keyout /tmp/tls.key \
  -out /tmp/tls.crt \
  -subj "/CN=api.example.com"

kubectl create secret tls api-gateway-tls-secret \
  --cert=/tmp/tls.crt \
  --key=/tmp/tls.key \
  -n gateway-system \
  --dry-run=client -o yaml | kubectl apply -f -

# ⚠️ 通知用户：证书临时降级为自签名，浏览器会有安全警告
```

#<!-- chunk: 10.5 配置不同步（P2） -->## 10.5 配置不同步（P2）

```bash
# ============================================
# P2 故障：GitOps 同步失败 / 路由配置不生效
# ============================================

# Step 1: 检查 ArgoCD 同步状态
argocd app get api-gateway
argocd app diff api-gateway

# Step 2: 检查控制平面日志
kubectl logs -n higress-system \
  -l app=higress-controller \
  --since=30m | grep -i error

# Step 3: 验证 CRD 配置是否正确入库
kubectl get httproutes -A
kubectl describe httproute <route-name> -n <namespace>

# Step 4: 手动触发同步
argocd app sync api-gateway --force

# Step 5: 如仍不生效，检查数据平面是否收到配置
kubectl exec -n gateway-system <envoy-pod> -- \
  curl -s localhost:9901/config_dump | \
  python3 -m json.tool | grep -A5 "api.example.com"

# Step 6: 重启控制平面（最后手段）
kubectl rollout restart deployment/higress-controller \
  -n higress-system
```

---

<!-- chunk: 跨文档索引 -->## 跨文档索引

| 相关主题 | 文档路径 |
|---------|---------|
| 生产运维通用最佳实践 | `domain-11-production-operations/` |
| 容灾与备份策略 | `domain-30-disaster-recovery/` |
| 性能基准测试与调优 | `domain-03-networking-traffic/13-api-gateway-performance-benchmarks.md` |
| Higress 详细配置 | `domain-03-networking-traffic/04-higress-enterprise-gateway.md` |
| APISIX 生产配置 | `domain-03-networking-traffic/05-apisix-enterprise-gateway.md` |
| Kong 生产配置 | `domain-03-networking-traffic/06-kong-enterprise-gateway.md` |
| 网关产品选型 | `domain-03-networking-traffic/03-api-gateway-selection-guide.md` |

---

*文档维护: kudig.io 知识库团队 | 适用环境: 生产级 Kubernetes 1.27+ | 最后审核: 2026-03-04*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[domain-03-networking-traffic/README.md|Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移

## See Also

- 12-api-gateway-observability
- 13-api-gateway-performance-benchmarks
- 99-envoy-gateway-enterprise-guide
- 01-api-gateway-architecture-overview
