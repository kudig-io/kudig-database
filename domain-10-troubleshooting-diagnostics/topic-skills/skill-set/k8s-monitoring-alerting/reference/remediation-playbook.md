---
title: Monitoring & Alerting Failure Remediation Playbook
summary: Monitoring & Alerting Failure Remediation Playbook：kubectl get servicemonitor
  <name> -n <namespace> -o yaml
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-monitoring-alerting
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-MON-001 v1.0 — Monitoring & Alerting Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-002 修正 ServiceMonitor 选择器](#rem-002)
    - [REM-003 修正 Grafana 数据源](#rem-003)
    - [REM-004 修正 Alertmanager 配置](#rem-004)
    - [REM-005 修正告警规则](#rem-005)
  - [🟡 中风险](#-中风险)
    - [REM-001 扩容/清理 [[Prometheus|Prometheus]] 存储](#rem-001)
    - [REM-006 调整网络策略](#rem-006)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 配置调整 | 可建议自动执行 |
| 中风险 | 🟡 | 存储/网络变更 | 建议操作并等待人工审批 |

## 修复操作

### 🟢 低风险

#### REM-002: 修正 ServiceMonitor 选择器

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl get servicemonitor <name> -n <namespace> -o yaml
  # 检查 spec.selector.matchLabels 是否与目标 Service 标签匹配
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修正选择器标签
  kubectl patch servicemonitor <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/selector/matchLabels", "value":
    {"app": "<correct-app-label>"}}]'

  # 修正 namespaceSelector
  kubectl patch servicemonitor <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/namespaceSelector", "value":
    {"matchNames":["<target-namespace>"]}}]'
  ```
- **后置验证**:
  ```bash
  # 在 Prometheus UI 中检查 target 是否变为 UP
  kubectl port-forward svc/prometheus -n monitoring 9090:9090
  # 访问 http://localhost:9090/targets
  ```

#### REM-003: 修正 Grafana 数据源

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get configmap -n <grafana-namespace> | grep datasource
  # 检查 datasource URL 和认证配置
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 更新 Prometheus datasource
  cat <<EOF | kubectl apply -f -
  apiVersion: 1
  datasources:
  - name: Prometheus
    type: prometheus
    url: http://prometheus:9090
    access: proxy
    isDefault: true
  EOF
  # 通过 Grafana sidecar 自动加载，或手动重启 Grafana
  kubectl rollout restart deployment grafana -n <grafana-namespace>
  ```
- **后置验证**:
  ```bash
  # 在 Grafana UI 中测试数据源连接
  ```

#### REM-004: 修正 Alertmanager 配置

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get secret alertmanager-<name> -n <namespace> -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 更新 Alertmanager 配置 Secret
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Secret
  metadata:
    name: alertmanager-<name>
    namespace: <namespace>
  stringData:
    alertmanager.yaml: |
      global:
        smtp_smarthost: 'localhost:587'
        smtp_from: 'alertmanager@example.com'
      route:
        receiver: 'default'
      receivers:
      - name: 'default'
        email_configs:
        - to: 'oncall@example.com'
  EOF
  ```
- **后置验证**:
  ```bash
  # 发送测试告警
  curl -X POST http://alertmanager:9093/api/v2/alerts -H "Content-Type: application/json" -d '[{"labels":{"alertname":"TestAlert","severity":"warning"}}]'
  ```

#### REM-005: 修正告警规则

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get prometheusrules <name> -n <namespace> -o yaml
  # 检查 PromQL 语法
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修正规则中的 PromQL
  kubectl patch prometheusrules <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/groups/0/rules/0/expr", "value": "up{job=\"kubernetes-pods\"} == 0"}]'
  ```
- **后置验证**:
  ```bash
  # 在 Prometheus UI 中检查规则状态
  # http://prometheus:9090/rules
  ```

### 🟡 中风险

#### REM-001: 扩容/清理 Prometheus 存储

- **适用根因**: RC-001
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get pvc -n <namespace> | grep prometheus
  kubectl exec <prometheus-pod> -n <namespace> -- df -h /prometheus
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 删除旧数据（缩短 retention）
  kubectl patch prometheus <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/retention", "value": "7d"}]'

  # 方案 B: 扩容 PVC（如果存储类支持）
  kubectl patch pvc prometheus-<name>-db-prometheus-<name>-0 -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/resources/requests/storage", "value": "100Gi"}]'

  # 方案 C: 增加 Prometheus 内存限制
  kubectl patch prometheus <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/resources", "value":
    {"limits":{"memory":"16Gi"},"requests":{"memory":"8Gi"}}}]'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get pods -n <namespace> | grep prometheus
  kubectl exec <prometheus-pod> -n <namespace> -- df -h /prometheus
  ```

#### REM-006: 调整网络策略

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  kubectl get networkpolicy -n <target-namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 允许 Prometheus 抓取目标 namespace
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-prometheus-scrape
    namespace: <target-namespace>
  spec:
    podSelector: {}
    policyTypes:
    - Ingress
    ingress:
    - from:
      - namespaceSelector:
          matchLabels:
            kubernetes.io/metadata.name: monitoring
      ports:
      - protocol: TCP
        port: 8080
      - protocol: TCP
        port: 9090
      - protocol: TCP
        port: 9100
  EOF
  ```
- **后置验证**:
  ```bash
  kubectl get networkpolicy -n <target-namespace>
  ```

## 验证确认

### 即时验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V1: Prometheus Running
kubectl get pods -n monitoring | grep prometheus

# V2: Grafana Running
kubectl get pods -n monitoring | grep grafana

# V3: Alertmanager Running
kubectl get pods -n monitoring | grep alertmanager

# V4: Active Targets > 0
kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/targets | jq '.data.activeTargets | length'

# V5: No rule errors
kubectl logs <prometheus-pod> -n monitoring --tail=20 | grep -i "rule evaluation"
```
### 解决确认标准

- [ ] Prometheus、Grafana、Alertmanager Pod Running
- [ ] Prometheus 有活跃的抓取目标
- [ ] Grafana 数据源连接正常
- [ ] 告警规则无评估错误
- [ ] Alertmanager 可以发送测试通知
- [ ] 关键告警面板有数据

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| Prometheus 数据损坏 | 需要数据恢复专家 |
| 监控基础设施被入侵 | 安全事件响应 |

### 升级消息模板

```
【{severity}】Monitoring & Alerting Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {component} 监控异常
- 影响范围: 
  - 受影响组件: {affected_components}
  - 监控覆盖: {monitoring_coverage}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-MON-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
