---
title: "配置管理与 Feature Flag 模式"
description: "生产级配置管理：ConfigMap/Secret 治理、外部配置中心集成、Feature Flag 平台与渐进式功能发布实践"
summary: "覆盖 Kubernetes 配置管理的完整实践，包括 ConfigMap/Secret 生命周期管理、外部配置中心（Vault/Nacos）集成、Feature Flag 平台设计、渐进式功能发布策略和配置变更的安全审计。"
category: 应用模式
tags:
- patterns
- configmap
- secret
- feature-flags
- configuration
- vault
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 应用开发者
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- "K8s ConfigMap Secret 生产管理最佳实践"
- "Feature Flag 平台怎么设计和集成"
- "外部配置中心 Vault 如何与 K8s 集成"
trigger_keywords:
- ConfigMap
- Secret
- Feature Flag
- 配置管理
- Vault
- 渐进式发布
prerequisites:
- kubectl-basics
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

# 配置管理与 Feature Flag 模式

> **适用范围**: Kubernetes v1.28–v1.32 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

## 概述

配置是应用行为的"隐形代码"。一个错误的 ConfigMap 变更等同于一次未经测试的发布，一个泄露的 Secret 可能导致全面安全事件。在 Kubernetes 环境中，配置管理面临独特挑战：ConfigMap 更新后 Pod 不会自动重启、Secret 默认仅 Base64 编码而非加密、配置变更缺乏审计追踪、多环境配置一致性难以保证。

Feature Flag（特性开关）则是连接配置管理与渐进式发布的桥梁——它让功能发布从"全有或全无"变为"渐进可控"，支持按用户群、按比例、按地域逐步放量。本文覆盖配置管理的完整生命周期和 Feature Flag 平台设计。相关内容可参见 [[progressive-delivery-patterns]]、[[release-change-management-patterns]]、[[application-security-hardening]]。

---

## 模式定义与适用场景

### 配置分类与管理策略

| 配置类型 | 变更频率 | 敏感度 | 管理方式 | 热更新 | 示例 |
|---------|---------|--------|---------|--------|------|
| **静态配置** | 极低 | 低 | 镜像内/ConfigMap | 需重启 | 应用端口、日志格式 |
| **动态配置** | 中 | 低 | ConfigMap + 热加载 | 支持 | 限流阈值、功能开关 |
| **敏感配置** | 低 | 高 | Secret/Vault | 需重启或热加载 | 数据库密码、API Key |
| **环境配置** | 极低 | 中 | Kustomize Overlay | 需重新部署 | 区域、集群标识 |
| **Feature Flag** | 高 | 低 | 专用平台 | 实时 | 功能开关、A/B 实验 |

### Feature Flag 使用场景

| 场景 | Flag 类型 | 生命周期 | 示例 |
|------|----------|---------|------|
| 渐进式发布 | Release Flag | 短期（天-周） | 新支付渠道 5%→100% |
| A/B 测试 | Experiment Flag | 中期（周-月） | 新推荐算法对比 |
| 运维开关 | Ops Flag | 长期 | 降级开关、限流开关 |
| 权限控制 | Permission Flag | 长期 | VIP 功能、Beta 功能 |
| 代码清理 | Cleanup Flag | 极短期 | 旧代码路径移除 |

---

## 架构设计

### 配置管理分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    配置源 (Source of Truth)                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Git Repo │  │  Vault   │  │  Nacos/  │  │ Feature  │   │
│  │(Kustomize)│  │(Secrets) │  │  Apollo  │  │ Flag Svc │   │
│  └─────┬────┘  └─────┬────┘  └─────┬────┘  └─────┬────┘   │
├────────┼──────────────┼─────────────┼─────────────┼─────────┤
│        ▼              ▼             ▼             ▼         │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           配置分发层                                  │    │
│  │  Argo CD / External Secrets Operator / SDK Poll      │    │
│  └──────────────────────┬──────────────────────────────┘    │
│                         ▼                                   │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           Kubernetes 集群                             │    │
│  │  ConfigMap / Secret / Env / Volume Mount              │    │
│  └──────────────────────┬──────────────────────────────┘    │
│                         ▼                                   │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           应用层                                      │    │
│  │  配置热加载 / Feature Flag SDK / 变更监听              │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### Feature Flag 评估流程

```
请求到达
  │
  ▼
[提取上下文: user_id, region, tier, device]
  │
  ▼
[查询 Flag 配置（本地缓存 + 定期同步）]
  │
  ▼
[评估规则链]
  ├── 强制开启/关闭？ → 直接返回
  ├── 白名单命中？ → 返回 true
  ├── 百分比命中？ → hash(user_id) % 100 < percentage
  ├── 条件规则？ → region == "cn-north" AND tier == "premium"
  └── 默认值 → false
  │
  ▼
[返回 Flag 值 + 记录评估事件]
```

---

## K8s 实现

### 生产级 ConfigMap 管理

```yaml
# 🟡 中风险：ConfigMap 变更可能影响运行中的应用行为
apiVersion: v1
kind: ConfigMap
metadata:
  name: order-service-config
  namespace: production
  labels:
    app.kubernetes.io/name: order-service
    app.kubernetes.io/managed-by: argocd
    kudig.io/config-type: dynamic
  annotations:
    # 配置变更追踪
    kudig.io/change-id: "CHG-2026-0719-002"
    kudig.io/last-reviewed: "2026-07-19"
    # 触发 Pod 重启的注解（配合 Stakater Reloader）
    reloader.stakater.com/match: "true"
data:
  # 应用配置（YAML 格式）
  application.yaml: |
    server:
      port: 8080
      shutdown_grace_period: 30s
    
    order:
      max_items_per_order: 100
      payment_timeout: 30s
      auto_cancel_unpaid_minutes: 30
      
    rate_limit:
      enabled: true
      requests_per_second: 100
      burst: 200
      
    cache:
      ttl: 300s
      max_size: 10000
      
    feature_flags:
      new_checkout_flow: false
      express_delivery: true
      ai_recommendation: false
  # 日志配置
  logback.xml: |
    <configuration>
      <appender name="JSON" class="ch.qos.logback.core.ConsoleAppender">
        <encoder class="net.logstash.logback.encoder.LogstashEncoder"/>
      </appender>
      <root level="INFO">
        <appender-ref ref="JSON"/>
      </root>
    </configuration>
```

### External Secrets Operator（Vault 集成）

```yaml
# 🟡 中风险：Secret 同步配置影响应用凭证
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: order-service-secrets
  namespace: production
  labels:
    app.kubernetes.io/name: order-service
spec:
  refreshInterval: 1h  # 每小时同步一次
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: order-service-secrets
    creationPolicy: Owner
    template:
      metadata:
        labels:
          app.kubernetes.io/name: order-service
      data:
        # 模板化 Secret 数据
        DATABASE_URL: "postgres://{{ .db_user }}:{{ .db_password }}@db.production.svc:5432/orders"
        REDIS_URL: "redis://:{{ .redis_password }}@redis.production.svc:6379"
  data:
    - secretKey: db_user
      remoteRef:
        key: secret/data/production/order-service
        property: db_username
    - secretKey: db_password
      remoteRef:
        key: secret/data/production/order-service
        property: db_password
    - secretKey: redis_password
      remoteRef:
        key: secret/data/production/redis
        property: password
---
# Vault ClusterSecretStore
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-backend
spec:
  provider:
    vault:
      server: "https://vault.internal:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "external-secrets"
          serviceAccountRef:
            name: external-secrets
            namespace: external-secrets
```

### Feature Flag Service 部署

```yaml
# 🟡 中风险：Feature Flag 服务是全局依赖，需高可用
apiVersion: apps/v1
kind: Deployment
metadata:
  name: feature-flag-service
  namespace: platform-system
  labels:
    app.kubernetes.io/name: feature-flag-service
    app.kubernetes.io/part-of: platform
spec:
  replicas: 3  # 高可用：至少 3 副本
  selector:
    matchLabels:
      app.kubernetes.io/name: feature-flag-service
  template:
    metadata:
      labels:
        app.kubernetes.io/name: feature-flag-service
    spec:
      priorityClassName: system-cluster-critical
      containers:
        - name: flagd
          image: ghcr.io/open-feature/flagd:v0.11.0
          args:
            - start
            - --uri
            - file:///etc/feature-flags/flags.json
            - --port
            - "8013"
            - --metrics-port
            - "8014"
          ports:
            - containerPort: 8013
              name: grpc
            - containerPort: 8014
              name: metrics
          resources:
            requests:
              cpu: "100m"
              memory: "128Mi"
            limits:
              cpu: "500m"
              memory: "256Mi"
          volumeMounts:
            - name: flags-config
              mountPath: /etc/feature-flags
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8014
            periodSeconds: 10
          readinessProbe:
            grpc:
              port: 8013
            periodSeconds: 5
      volumes:
        - name: flags-config
          configMap:
            name: feature-flags-definitions
---
# PDB：保证 Flag 服务高可用
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: feature-flag-service-pdb
  namespace: platform-system
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app.kubernetes.io/name: feature-flag-service
```

---

## 生产配置示例

### Feature Flag 定义文件

```yaml
# 🟡 中风险：Flag 变更直接影响线上功能行为
apiVersion: v1
kind: ConfigMap
metadata:
  name: feature-flags-definitions
  namespace: platform-system
  labels:
    app.kubernetes.io/name: feature-flag-service
  annotations:
    kudig.io/flag-owner: "product-team"
data:
  flags.json: |
    {
      "flags": {
        "new-checkout-flow": {
          "state": "ENABLED",
          "variants": {
            "on": true,
            "off": false
          },
          "defaultVariant": "off",
          "targeting": {
            "if": [
              {"in": ["@user.email", ["vip@example.com", "beta@example.com"]]},
              "on",
              {
                "if": [
                  {"<": [{"var": "bucket"}, 15]},
                  "on",
                  "off"
                ]
              }
            ]
          }
        },
        "ai-recommendation": {
          "state": "ENABLED",
          "variants": {
            "v2-model": "model-v2",
            "v1-model": "model-v1",
            "disabled": "none"
          },
          "defaultVariant": "disabled",
          "targeting": {
            "if": [
              {"==": [{"var": "region"}, "cn-north"]},
              {
                "if": [
                  {"<": [{"var": "bucket"}, 30]},
                  "v2-model",
                  "v1-model"
                ]
              },
              "v1-model"
            ]
          }
        },
        "maintenance-mode": {
          "state": "DISABLED",
          "variants": {
            "on": true,
            "off": false
          },
          "defaultVariant": "off"
        }
      }
    }
```

### 配置热加载（Reloader）

```yaml
# 🟡 中风险：Reloader 会在 ConfigMap 变更时自动重启 Pod
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
  annotations:
    # ConfigMap 变更时自动滚动重启
    reloader.stakater.com/auto: "true"
    # 或指定特定 ConfigMap
    configmap.reloader.stakater.com/reload: "order-service-config"
    # Secret 变更时自动重启
    secret.reloader.stakater.com/reload: "order-service-secrets"
spec:
  template:
    spec:
      containers:
        - name: app
          image: registry.internal/order-service:v2.3.1
          envFrom:
            - configMapRef:
                name: order-service-env
          volumeMounts:
            - name: config
              mountPath: /etc/app/config
              readOnly: true
            - name: secrets
              mountPath: /etc/app/secrets
              readOnly: true
      volumes:
        - name: config
          configMap:
            name: order-service-config
        - name: secrets
          secret:
            secretName: order-service-secrets
            defaultMode: 0400  # 只读权限
```

---

## 运维要点

### 配置审计与合规

```bash
# 🟢 低风险：查看 ConfigMap 变更历史（通过 Argo CD）
argocd app get order-service --show-params
argocd app history order-service

# 🟢 低风险：检查 Secret 是否有明文暴露风险
kubectl get configmaps -n production -o yaml | grep -i "password\|secret\|token\|key"

# 🟢 低风险：查看谁修改了 ConfigMap（审计日志）
kubectl get events -n production --field-selector reason=ConfigMapUpdated

# 🔴 高风险：轮转 Secret（触发所有依赖 Pod 重启）
kubectl create secret generic order-service-secrets \
  --from-literal=db_password='new-rotated-password' \
  -n production --dry-run=client -o yaml | kubectl apply -f -

# 🟢 低风险：检查 External Secret 同步状态
kubectl get externalsecrets -n production
kubectl describe externalsecret order-service-secrets -n production
```

### Feature Flag 生命周期管理

| 阶段 | 操作 | 负责人 | 验证 |
|------|------|--------|------|
| 创建 | 定义 Flag + 默认关闭 | 开发者 | Code Review |
| 灰度 | 逐步放量 5%→25%→50%→100% | 产品/SRE | 监控指标 |
| 全量 | 默认开启 | 产品 | 稳定运行 1 周 |
| 清理 | 移除 Flag 代码 + 删除定义 | 开发者 | 代码中无 Flag 引用 |
| 审计 | 每月清理过期/无用 Flag | 平台团队 | Flag 数量不增长 |

### 配置变更安全清单

```bash
# 🟢 低风险：变更前检查
# 1. 确认目标 Namespace
kubectl config view --minify -o jsonpath='{.contexts[0].context.namespace}'

# 2. 查看当前配置
kubectl get configmap order-service-config -n production -o yaml

# 3. 检查依赖此 ConfigMap 的 Pod 数量
kubectl get pods -n production -o json | \
  jq '.items[] | select(.spec.volumes[]?.configMap.name == "order-service-config") | .metadata.name'

# 🟡 中风险：执行变更
kubectl apply -f new-config.yaml

# 🟢 低风险：验证变更生效
kubectl rollout status deployment/order-service -n production
kubectl logs -n production -l app.kubernetes.io/name=order-service --tail=20
```

---

## 反模式

### 反模式 1：Secret 明文存储在 Git

```yaml
# ❌ 错误：密码直接写在 Manifest 中
apiVersion: v1
kind: Secret
metadata:
  name: db-credentials
data:
  password: bXlwYXNzd29yZA==  # 仅 Base64，非加密
```

**后果**：任何有仓库读权限的人都能解码密码，Git 历史永久保留。

**修正**：使用 External Secrets Operator + Vault，Git 中只存 ExternalSecret 引用。参见 [[application-security-hardening]]。

### 反模式 2：ConfigMap 变更不触发 Pod 更新

**后果**：修改了 ConfigMap 但 Pod 仍使用旧配置（Volume Mount 有延迟，Env 完全不更新），导致配置不一致。

**修正**：使用 Reloader 自动重启，或应用内实现文件 Watch 热加载。关键配置变更走 Deployment 滚动更新。

### 反模式 3：Feature Flag 永不清理

**后果**：代码中充斥数百个过期 Flag，条件分支复杂度爆炸，新人无法理解代码逻辑。

**修正**：Flag 创建时设置过期日期，每月审计清理，Release Flag 全量后 2 周内必须移除代码。

### 反模式 4：所有配置都放 ConfigMap

**后果**：ConfigMap 有 1MB 大小限制，大量配置导致 etcd 压力；频繁变更触发不必要的 Pod 重启。

**修正**：静态配置打入镜像，动态配置用 ConfigMap，敏感配置用 Secret/Vault，大文件用 PVC 或对象存储。

### 反模式 5：Feature Flag 服务单点

**后果**：Flag 服务宕机，所有依赖它的服务无法评估 Flag，可能全部走默认值（功能异常）。

**修正**：Flag SDK 本地缓存 + 降级到默认值 + Flag 服务多副本 PDB。参见 [[app-resilience-circuit-breaker]]。

---

## Related

- [[progressive-delivery-patterns]] — 渐进式交付生产模式
- [[release-change-management-patterns]] — 发布变更管理模式
- [[application-security-hardening]] — 应用安全加固
- [[config-management-feature-flags]] — 配置管理与 Feature Flag 模式
- [[api-design-versioning-patterns]] — API 设计与版本管理模式
- [[app-observability-patterns]] — 应用可观测性模式
