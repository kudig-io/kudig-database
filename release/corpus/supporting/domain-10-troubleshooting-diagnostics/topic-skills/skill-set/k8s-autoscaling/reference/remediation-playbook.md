---
title: Autoscaling Failure Remediation Playbook
summary: Autoscaling Failure Remediation Playbook：kubectl get pods -n kube-system
  -l k8s-app=metrics-server kubectl logs -n kube-system -l k8s-app=metrics-server
  --tail=50
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-autoscaling
last_updated: 2026-05-22
---



# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-AUTO-001 v1.0 — Autoscaling Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 修复 metrics-server](#rem-001)
    - [REM-002 修正 HPA 配置](#rem-002)
    - [REM-005 调整配额和节点池](#rem-005)
  - [🟡 中风险](#-中风险)
    - [REM-003 修复 VPA](#rem-003)
    - [REM-004 修复 Cluster Autoscaler](#rem-004)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 配置调整 | 可建议自动执行 |
| 中风险 | 🟡 | 组件重启或配置变更 | 建议操作并等待人工审批 |

## 修复操作

### 🟢 低风险

#### REM-001: 修复 metrics-server

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl get pods -n kube-system -l k8s-app=metrics-server
  kubectl logs -n kube-system -l k8s-app=metrics-server --tail=50
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案 A: 如果 metrics-server 被删除，重新安装
  kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

  # 方案 B: 如果证书问题，添加 --kubelet-insecure-tls（测试环境）
  kubectl patch deployment metrics-server -n kube-system --type='json' -p='
  [{"op": "add", "path": "/spec/template/spec/containers/0/args/-", "value": "--kubelet-insecure-tls"}]'

  # 方案 C: 重启 metrics-server
  kubectl rollout restart deployment metrics-server -n kube-system
  ```
- **后置验证**:
  ```bash
  kubectl top nodes
  kubectl top pods -n <namespace>
  ```

#### REM-002: 修正 HPA 配置

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl get hpa <name> -n <namespace> -o yaml
  # 检查 metrics 类型、target 值、scaleTargetRef
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 修正 scaleTargetRef
  kubectl patch hpa <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/scaleTargetRef", "value":
    {"apiVersion":"apps/v1","kind":"Deployment","name":"<correct-deployment>"}}]'

  # 方案 B: 调整 target 值
  kubectl patch hpa <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/targetCPUUtilizationPercentage", "value": 70}]'

  # 方案 C: 添加 metrics 配置（如果为空）
  kubectl patch hpa <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/metrics", "value":
    [{"type":"Resource","resource":{"name":"cpu","target":{"type":"Utilization","averageUtilization":70}}}]}]'
  ```
- **后置验证**:
  ```bash
  kubectl get hpa <name> -n <namespace>
  # 预期: TARGETS 列显示具体数值而非 <unknown>
  ```

#### REM-005: 调整配额和节点池

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl describe resourcequota -n <namespace>
  kubectl get nodes
  # 检查节点池最大/最小节点数
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 增加 ResourceQuota
  kubectl patch resourcequota <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/hard/pods", "value": "100"}]'

  # 方案 B: 调整节点池容量（云环境）
  # 通过云控制台或 terraform 调整节点池 min/max 节点数
  ```
- **后置验证**:
  ```bash
  kubectl get hpa <name> -n <namespace>
  # 预期: HPA 可以正常扩缩容
  ```

### 🟡 中风险

#### REM-003: 修复 VPA

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get pods -n kube-system | grep vpa
  kubectl logs -n kube-system -l app=vpa-admission-controller --tail=30
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 重启 VPA 组件
  kubectl rollout restart deployment vpa-admission-controller -n kube-system
  kubectl rollout restart deployment vpa-recommender -n kube-system
  kubectl rollout restart deployment vpa-updater -n kube-system
  ```
- **后置验证**:
  ```bash
  kubectl get vpa <name> -n <namespace> -o jsonpath='{.status.recommendation}'
  # 预期: 有推荐值
  ```

#### REM-004: 修复 Cluster Autoscaler

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl logs -n kube-system -l app=cluster-autoscaler --tail=50
  kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案 A: 修复云提供商权限
  # 确认 Cluster Autoscaler 使用的 ServiceAccount 有正确的云权限

  # 方案 B: 调整 CA 配置
  kubectl patch deployment cluster-autoscaler -n kube-system --type='json' -p='
  [{"op": "replace", "path": "/spec/template/spec/containers/0/command", "value":
    ["./cluster-autoscaler",
     "--cloud-provider=<provider>",
     "--namespace=kube-system",
     "--node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/<cluster-name>"]}]'

  # 方案 C: 重启 CA
  kubectl rollout restart deployment cluster-autoscaler -n kube-system
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n kube-system | grep cluster-autoscaler
  kubectl logs -n kube-system -l app=cluster-autoscaler --tail=20
  ```

## 验证确认

### 即时验证

```bash
# V1: metrics-server 正常
kubectl top nodes

# V2: HPA 有数值
kubectl get hpa <name> -n <namespace>
# 预期: TARGETS 为百分比数值

# V3: CA 无错误
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=20

# V4: 扩容测试（可选）
# 对服务施加负载，观察 HPA 是否扩容
kubectl get hpa <name> -n <namespace> -w
```

### 解决确认标准

- [ ] metrics-server Running 且 Metrics API 可用
- [ ] HPA TARGETS 显示具体数值
- [ ] HPA 可以在负载增加时扩容
- [ ] HPA 可以在负载降低时缩容
- [ ] Cluster Autoscaler Running 且无错误日志
- [ ] （如使用 VPA）VPA recommendation 不为空

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| CA 导致节点异常扩缩 | 可能造成成本失控或资源不足 |
| metrics-server 数据异常 | 指标数据不正确导致误扩缩 |

### 升级消息模板

```
【{severity}】Autoscaling Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {component} 自动扩缩容异常
- 影响范围: 
  - 受影响服务: {affected_services}
  - 扩容能力: {scaling_status}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-AUTO-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```


## 参见

- [[remediation-playbook]] — reference 领域核心页面

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub
