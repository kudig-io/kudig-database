---
title: 07 - 可观测性与安全迁移 [migration]
description: 'title: 07 - 可观测性与安全迁移'
summary: 'title: 07 - 可观测性与安全迁移'
category: general
tags:
- migration
- upgrade
- observability
- security
- kubelet
- prometheus
- grafana
- jaeger
- helm
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 可观测性与安全迁移 是什么
- 如何 可观测性与安全迁移
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 可观测性与安全迁移
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- tls-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 07 - 可观测性与安全迁移
description: '# 07 - 可观测性与安全迁移'
category: migration
tags:
- k8s
- migration
- modernization
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- grafana
- [[Jaeger|jaeger]]
- [[Helm|helm]]
- docker
- elasticsearch
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 可观测性与安全迁移 是什么
- 如何 可观测性与安全迁移
trigger_keywords:
- 可观测性与安全迁移
- migration
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

# 07 - 可观测性与安全迁移

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: Prometheus, Grafana, EFK, SLS, ARMS, RBAC, NetworkPolicy, 证书, 安全基线

---

<!-- chunk: 目录 -->## 目录

1. [监控体系迁移](#1-监控体系迁移)
2. [日志体系迁移](#2-日志体系迁移)
3. [链路追踪迁移](#3-链路追踪迁移)
4. [告警规则迁移](#4-告警规则迁移)
5. [Grafana Dashboard 迁移](#5-grafana-dashboard-迁移)
6. [RBAC 与权限迁移](#6-rbac-与权限迁移)
7. [证书与 TLS 迁移](#7-证书与-tls-迁移)
8. [安全基线建立](#8-安全基线建立)

---

<!-- chunk: 1. 监控体系迁移 -->## 1. 监控体系迁移

## 1.1 方案选择

| 方案 | 说明 | 适用场景 | 成本 |
|------|------|---------|------|
| **自建 Prometheus → ACK 自建 Prometheus** | 在 ACK 重新部署 kube-prometheus-stack | 保持一致性，自主可控 | 云盘存储费 |
| **自建 Prometheus → ARMS Prometheus** | 使用阿里云托管 Prometheus | 免运维，与 ACK 深度集成 | ARMS 服务费 |
| **混合方案** | ACK 自建 Prometheus + ARMS 基础指标 | 灵活，可渐进迁移 | 中等 |

## 1.2 自建 Prometheus 部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在 ACK 部署 kube-prometheus-stack（推荐 Helm 方式）
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 创建 values 文件
cat > prom-values.yaml <<EOF
prometheus:
  prometheusSpec:
    retention: 30d
    storageSpec:
      volumeClaimTemplate:
        spec:
          storageClassName: alicloud-disk-essd
          resources:
            requests:
              storage: 200Gi
    nodeSelector:
      node-role: system
    tolerations:
    - key: CriticalAddonsOnly
      operator: Exists
      effect: NoSchedule
    # 迁移自建集群的额外抓取配置
    additionalScrapeConfigs: []

grafana:
  persistence:
    enabled: true
    storageClassName: alicloud-disk-essd
    size: 20Gi
  adminPassword: "<secure-password>"
  nodeSelector:
    node-role: system
  tolerations:
  - key: CriticalAddonsOnly
    operator: Exists
    effect: NoSchedule

alertmanager:
  alertmanagerSpec:
    storage:
      volumeClaimTemplate:
        spec:
          storageClassName: alicloud-disk-essd
          resources:
            requests:
              storage: 10Gi
EOF

helm install kube-prometheus-stack prometheus-community/kube-prometheus-stack \
  -n monitoring --create-namespace \
  -f prom-values.yaml

# 验证
kubectl get pods -n monitoring
kubectl get svc -n monitoring
```
## 1.3 自定义指标抓取配置迁移

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出源集群的 Prometheus 额外抓取配置
kubectl --context=source-cluster get secret -n monitoring \
  prometheus-kube-prometheus-stack-prometheus \
  -o jsonpath='{.data.additional-scrape-configs\.yaml}' | base64 -d > src-scrape-configs.yaml

# 检查并适配（主要是 endpoint 地址变化）
cat src-scrape-configs.yaml
# 将其合并到 ACK Prometheus 的 additionalScrapeConfigs

# 导出 ServiceMonitor 资源
kubectl --context=source-cluster get servicemonitors -A -o yaml | kubectl neat > src-servicemonitors.yaml

# 应用到 ACK
kubectl --context=ack-cluster apply -f src-servicemonitors.yaml
```
## 1.4 PrometheusRule 迁移

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出自定义告警规则
kubectl --context=source-cluster get prometheusrules -A -o yaml | kubectl neat > src-prometheus-rules.yaml

# 应用到 ACK
kubectl --context=ack-cluster apply -f src-prometheus-rules.yaml

# 验证规则已加载
kubectl --context=ack-cluster port-forward -n monitoring svc/kube-prometheus-stack-prometheus 9090:9090 &
curl -s http://localhost:9090/api/v1/rules | jq '.data.groups | length'
```
---

<!-- chunk: 2. 日志体系迁移 -->## 2. 日志体系迁移

## 2.1 方案选择

| 方案 | 说明 | 适用场景 |
|------|------|---------|
| **EFK → ACK EFK** | 在 ACK 重新部署 EFK Stack | 保持一致性 |
| **EFK → SLS** | 迁移到阿里云日志服务 | 免运维，强大查询 |
| **Loki → ACK Loki** | 在 ACK 重新部署 Loki | 轻量级，与 Grafana 集成 |

## 2.2 使用 SLS（推荐）

```yaml
# ACK 默认已安装 logtail-ds
# 创建日志采集配置

# stdout 日志采集
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: app-stdout
  namespace: kube-system
spec:
  project: "k8s-log-${CLUSTER_ID}"
  logstore: "app-stdout"
  logtailConfig:
    inputType: plugin
    configName: app-stdout
    inputDetail:
      plugin:
        inputs:
        - type: service_docker_stdout
          detail:
            IncludeLabel:
              io.kubernetes.pod.namespace: "production|staging"
            Stdout: true
            Stderr: true
---
# 文件日志采集
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: app-file-log
  namespace: kube-system
spec:
  project: "k8s-log-${CLUSTER_ID}"
  logstore: "app-file-log"
  logtailConfig:
    inputType: file
    configName: app-file-log
    inputDetail:
      logPath: /var/log/app
      filePattern: "*.log"
      dockerFile: true
      dockerIncludeLabel:
        io.kubernetes.pod.namespace: "production"
```

## 2.3 EFK Stack 迁移

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 如果保持 EFK Stack，在 ACK 部署

# Elasticsearch (使用阿里云 ES 或自建)
# 参考 06-stateful-services-migration.md ES 迁移部分

# Fluentd / Fluent Bit
helm install fluent-bit fluent/fluent-bit \
  -n logging --create-namespace \
  --set config.outputs="[OUTPUT]\n    Name es\n    Match *\n    Host <es-endpoint>\n    Port 9200\n    Index k8s-logs\n    Type _doc"

# Kibana
helm install kibana elastic/kibana \
  -n logging \
  --set elasticsearchHosts="http://<es-endpoint>:9200" \
  --set persistence.enabled=true \
  --set persistence.storageClass=alicloud-disk-essd
```
---

<!-- chunk: 3. 链路追踪迁移 -->## 3. 链路追踪迁移

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 方案 A: 使用阿里云 ARMS（推荐）
# ACK 安装 ARMS Agent
# 控制台: ACK → 运维管理 → 应用实时监控服务 ARMS → 安装

# 方案 B: 自建 Jaeger
helm install jaeger jaegertracing/jaeger \
  -n tracing --create-namespace \
  --set storage.type=elasticsearch \
  --set storage.elasticsearch.host=<es-endpoint> \
  --set collector.service.type=ClusterIP

# 应用需确保 OpenTelemetry/Jaeger SDK 配置指向新的 Collector 地址
# 更新应用环境变量:
# OTEL_EXPORTER_OTLP_ENDPOINT=http://jaeger-collector.tracing:4317
# 或
# JAEGER_AGENT_HOST=jaeger-agent.tracing
```
---

<!-- chunk: 4. 告警规则迁移 -->## 4. 告警规则迁移

## 4.1 Alertmanager 配置迁移

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出源集群 Alertmanager 配置
kubectl --context=source-cluster get secret -n monitoring \
  alertmanager-kube-prometheus-stack-alertmanager \
  -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d > src-alertmanager.yaml

# 适配后应用到 ACK
# 主要修改: webhook URL、钉钉/企微机器人地址等
kubectl --context=ack-cluster create secret generic alertmanager-config \
  -n monitoring \
  --from-file=alertmanager.yaml=src-alertmanager.yaml
```
## 4.2 告警通道配置

```yaml
# alertmanager.yaml 示例（适配阿里云环境）
global:
  resolve_timeout: 5m

route:
  group_by: ['alertname', 'namespace']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
  receiver: 'default'
  routes:
  - matchers:
    - severity="critical"
    receiver: critical-channel
  - matchers:
    - severity="warning"
    receiver: warning-channel
receivers:
- name: 'default'
  webhook_configs:
  - url: 'https://oapi.dingtalk.com/robot/send?access_token=<token>'
    send_resolved: true
- name: 'critical-channel'
  webhook_configs:
  - url: 'https://oapi.dingtalk.com/robot/send?access_token=<critical-token>'
    send_resolved: true
- name: 'warning-channel'
  webhook_configs:
  - url: 'https://oapi.dingtalk.com/robot/send?access_token=<warning-token>'
    send_resolved: true
```

---

<!-- chunk: 5. Grafana Dashboard 迁移 -->## 5. Grafana Dashboard 迁移

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 方式 1: 使用 Grafana API 导出/导入
# 导出所有 Dashboard
SOURCE_GRAFANA="http://grafana.source-cluster:3000"
ACK_GRAFANA="http://grafana.ack-cluster:3000"
API_KEY_SRC="<source-api-key>"
API_KEY_ACK="<ack-api-key>"

# 获取所有 Dashboard UID
curl -s -H "Authorization: Bearer $API_KEY_SRC" \
  "$SOURCE_GRAFANA/api/search?type=dash-db" | jq -r '.[].uid' > dashboard-uids.txt

# 批量导出
mkdir -p grafana-dashboards
while read uid; do
  curl -s -H "Authorization: Bearer $API_KEY_SRC" \
    "$SOURCE_GRAFANA/api/dashboards/uid/$uid" | jq '.dashboard' > "grafana-dashboards/$uid.json"
  echo "导出: $uid"
done < dashboard-uids.txt

# 批量导入到 ACK Grafana
for f in grafana-dashboards/*.json; do
  payload=$(jq '{dashboard: ., overwrite: true, folderId: 0}' "$f")
  curl -s -X POST -H "Authorization: Bearer $API_KEY_ACK" \
    -H "Content-Type: application/json" \
    -d "$payload" \
    "$ACK_GRAFANA/api/dashboards/db"
  echo "导入: $f"
done

# 方式 2: 使用 ConfigMap (GitOps 友好)
# 将 Dashboard JSON 存为 ConfigMap，Grafana sidecar 自动加载
kubectl --context=ack-cluster create configmap grafana-dashboard-apps \
  -n monitoring \
  --from-file=grafana-dashboards/ \
  -o yaml --dry-run=client | \
  yq eval '.metadata.labels.grafana_dashboard = "1"' - | \
  kubectl --context=ack-cluster apply -f -
```
---

<!-- chunk: 6. RBAC 与权限迁移 -->## 6. RBAC 与权限迁移

## 6.1 RBAC 迁移清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出并迁移自定义 RBAC（已在 03 文档中覆盖）
# 此处补充 ACK 特有的 RAM-RBAC 集成

# ACK 支持将 RAM 用户/角色映射为 K8s RBAC 主体
# 通过 ACK 控制台: 集群 → 安全管理 → 授权管理

# 或通过 YAML 配置
# 将 RAM 用户映射为 cluster-admin
kubectl --context=ack-cluster apply -f - <<EOF
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: ram-user-admin
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: User
  name: "<ram-user-id>"    # RAM 用户 UID
EOF
```
## 6.2 Pod Security Standards

```yaml
# ACK 1.25+ 使用 Pod Security Standards (PSS) 替代 PSP
# 在命名空间级别配置安全策略

# 为 production namespace 启用 restricted 策略
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

---

<!-- chunk: 7. 证书与 TLS 迁移 -->## 7. 证书与 TLS 迁移

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 导出自建集群的 TLS Secret
kubectl --context=source-cluster get secrets -A \
  -o json | jq '.items[] | select(.type == "kubernetes.io/tls")' > tls-secrets.json

# 2. 逐个导入 ACK
cat tls-secrets.json | jq -c '.' | while read secret; do
  name=$(echo $secret | jq -r '.metadata.name')
  ns=$(echo $secret | jq -r '.metadata.namespace')
  echo $secret | jq 'del(.metadata.uid,.metadata.resourceVersion,.metadata.creationTimestamp,.metadata.managedFields)' | \
    kubectl --context=ack-cluster apply -f -
  echo "导入 TLS: $ns/$name"
done

# 3. 推荐: 在 ACK 部署 cert-manager 自动管理
# 参考 05-network-migration-traffic-cutover.md cert-manager 部分
```
---

<!-- chunk: 8. 安全基线建立 -->## 8. 安全基线建立

## 8.1 ACK 安全巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 ACK 安全巡检
# 控制台: ACK → 安全管理 → 安全巡检

# 手动执行安全扫描
# 使用 kube-bench 检查 CIS 基准
kubectl --context=ack-cluster apply -f - <<EOF
apiVersion: batch/v1
kind: Job
metadata:
  name: kube-bench
  namespace: default
spec:
  template:
    spec:
      hostPID: true
      containers:
      - name: kube-bench
        image: aquasec/kube-bench:latest
        command: ["kube-bench", "run", "--targets", "node"]
        volumeMounts:
        - name: var-lib-kubelet
          mountPath: /var/lib/kubelet
          readOnly: true
        - name: etc-kubernetes
          mountPath: /etc/kubernetes
          readOnly: true
      volumes:
      - name: var-lib-kubelet
        hostPath:
          path: /var/lib/kubelet
      - name: etc-kubernetes
        hostPath:
          path: /etc/kubernetes
      restartPolicy: Never
EOF

# 查看安全扫描结果
kubectl logs job/kube-bench
```
## 8.2 安全迁移检查清单

- [ ] 监控体系已部署（Prometheus/ARMS）
- [ ] 核心指标采集正常（CPU/内存/网络/磁盘）
- [ ] 自定义 ServiceMonitor 已迁移
- [ ] PrometheusRule 告警规则已迁移
- [ ] Alertmanager 通知通道已配置并测试
- [ ] Grafana Dashboard 已导入
- [ ] 日志采集已配置（SLS/EFK）
- [ ] 链路追踪已部署（ARMS/Jaeger）
- [ ] RBAC 权限已迁移
- [ ] Pod Security Standards 已配置
- [ ] TLS 证书已迁移
- [ ] NetworkPolicy 已迁移
- [ ] 安全巡检已通过

---

**上一步**: ← [06-有状态服务迁移](./06-stateful-services-migration.md)
**下一步**: → [08-验收、切换与旧集群退役](./08-validation-cutover-decommission.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-migration MOC
- [[发布变更/topic-migration/README.md|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[发布变更/topic-migration/01-migration-assessment-planning.md|01 - 迁移评估与规划]]
- [[发布变更/topic-migration/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]]
- [[发布变更/topic-migration/03-application-workload-migration.md|03 - 应用工作负载迁移]]
- [[发布变更/topic-migration/04-storage-data-migration.md|04 - 存储与数据迁移]]
- [[发布变更/topic-migration/05-network-migration-traffic-cutover.md|05 - 网络迁移与流量切换]]
- [[发布变更/topic-migration/06-stateful-services-migration.md|06 - 有状态服务迁移]]
- [[发布变更/topic-migration/08-validation-cutover-decommission.md|08 - 验收、切换与旧集群退役]]
- [[发布变更/topic-migration/09-migration-toolchain.md|09 - 迁移工具链参考]]
- [[发布变更/topic-migration/10-real-world-case-study.md|10 - 生产迁移实战案例]]

## See Also

- 05-network-migration-traffic-cutover
- 06-stateful-services-migration
- 08-validation-cutover-decommission
- 09-migration-toolchain

```

<!-- risk-assessed -->
