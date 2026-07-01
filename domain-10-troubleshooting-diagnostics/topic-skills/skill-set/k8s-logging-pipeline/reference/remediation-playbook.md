---
title: "Logging Pipeline Failure Remediation Playbook"
category: remediation
skill_set: "k8s-logging-pipeline"
created: "2026-05-22"
updated: "2026-05-22"
last_updated: 2026-05-22
tags: ["reference", "remediation", "playbook", "visibility/public"]
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-LOG-001 v1.0 — Logging Pipeline Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-003 修正解析/过滤配置](#rem-003)
    - [REM-005 调整缓冲区](#rem-005)
  - [🟡 中风险](#-中风险)
    - [REM-001 重启/修复日志代理](#rem-001)
    - [REM-004 修复节点日志文件](#rem-004)
  - [🔴 高风险](#-高风险)
    - [REM-002 修复后端存储](#rem-002)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 配置调整 | 可建议自动执行 |
| 中风险 | 🟡 | 代理/节点操作 | 建议操作并等待人工审批 |
| 高风险 | 🔴 | 后端存储操作 | 仅提供操作指导，由人工执行 |

## 修复操作

### 🟢 低风险

#### REM-003: 修正解析/过滤配置

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get configmap -n <logging-namespace> | grep fluent
  # 检查 parser/filter 配置
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # Fluent Bit: 修正 parser 配置
  kubectl patch configmap fluent-bit-config -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/data/parsers.conf", "value":
    "[PARSER]\n    Name   json\n    Format json\n    Time_Key time\n    Time_Format %Y-%m-%dT%H:%M:%S.%L\n"}]'

  # 重启代理使配置生效
  kubectl rollout restart daemonset fluent-bit -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl logs -n <namespace> -l app=fluent-bit --tail=20
  # 预期: 无解析错误
  ```

#### REM-005: 调整缓冲区

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl logs -n <namespace> <fluent-pod> --tail=50 | grep -i "buffer\|drop\|retry"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # Fluent Bit: 增加缓冲和重试
  kubectl patch configmap fluent-bit-config -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/data/fluent-bit.conf", "value":
    "[OUTPUT]\n    Name  es\n    Match *\n    Host  elasticsearch\n    Port  9200\n    Retry_Limit 10\n    Buffer_Max_Size 10M\n"}]'

  kubectl rollout restart daemonset fluent-bit -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace>
  # 无 OOMKilled 或 CrashLoopBackOff
  ```

### 🟡 中风险

#### REM-001: 重启/修复日志代理

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl get pods -n <namespace> -l app=fluent-bit
  kubectl describe pod <fluent-pod> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案 A: 重启 DaemonSet
  kubectl rollout restart daemonset fluent-bit -n <namespace>

  # 方案 B: 删除异常 Pod 让其重建
  kubectl delete pod <fluent-pod> -n <namespace>

  # 方案 C: 检查并修复 RBAC
  kubectl auth can-i list pods --as=system:serviceaccount:<namespace>:fluent-bit -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l app=fluent-bit
  # 所有 Pod Running
  ```

#### REM-004: 修复节点日志文件

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  # 在节点上
  ls -la /var/log/containers/
  ls -la /var/log/pods/
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 修复符号链接（如果 containerd log 链接断裂）
  # 重启 containerd/kubelet 通常可以恢复
  systemctl restart containerd
  systemctl restart kubelet
  ```
- **后置验证**:
  ```bash
  ls -la /var/log/containers/
  # 预期: 有最新的容器日志文件
  ```

### 🔴 高风险

#### REM-002: 修复后端存储

- **适用根因**: RC-002
- **影响说明**: Elasticsearch 或 Loki 的后端修复可能涉及数据迁移和集群重建。
- **操作步骤**:
  1. **Elasticsearch red 状态**:
     ```bash
     # 查看哪些索引是 red
     curl -s http://elasticsearch:9200/_cluster/health?level=indices | jq '.indices | with_entries(select(.value.status == "red"))'

     # 如果有副本分片未分配，尝试重新路由
     curl -X POST http://elasticsearch:9200/_cluster/reroute?retry_failed=true

     # 如果索引已损坏，可能需要删除并重建
     curl -X DELETE http://elasticsearch:9200/<corrupted-index>
     ```
  2. **Loki ingester 问题**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     # 检查 ingester 内存和 WAL
     kubectl exec <loki-ingester> -n <namespace> -- wget -qO- http://localhost:3100/metrics | grep loki_ingester

     # 必要时重启 ingester
     kubectl rollout restart statefulset loki-ingester -n <namespace>
     ```
- **安全检查**:
  - 删除 ES 索引前确认数据可丢弃或已备份
  - Loki ingester 重启会丢失未刷新的内存数据
- **回滚方案**:
  - 从 ES snapshot 恢复
  - Loki 对象存储中的历史数据不受影响

## 验证确认

### 即时验证

```bash
# V1: 日志代理 Running
kubectl get pods -n <namespace> -l app=fluent-bit

# V2: 后端 Running
kubectl get pods -n <namespace> | grep elasticsearch

# V3: DaemonSet 完全调度
kubectl get ds -n <namespace>

# V4: 新日志出现
# 在 Kibana/Grafana 中查询最近 5 分钟的日志
```

### 解决确认标准

- [ ] 日志代理 Pod 全部 Running
- [ ] 日志后端（ES/Loki）集群健康
- [ ] DaemonSet 在所有节点调度成功
- [ ] 日志查询能返回最新日志（延迟 <5min）
- [ ] 无解析/过滤错误
- [ ] 日志量恢复至正常水平

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| 后端存储数据损坏 | 需要数据恢复 |
| 日志代理持续崩溃 | 可能是 Bug 或配置根本性错误 |

### 升级消息模板

```
【{severity}】Logging Pipeline Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: 日志管道 {component} 异常
- 影响范围: 
  - 日志收集: {collection_status}
  - 日志查询: {query_status}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-LOG-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
