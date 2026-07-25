---
title: Automation
description: 自动化脚本目录索引
summary: 自动化脚本目录索引 — Kubernetes 运维自动化脚本集合
category: index
tags:
- index
- automation
- k8s
- operations
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
---

# Automation

> 自动化运维脚本集合，覆盖集群健康检查、GPU 监控、安全审计、资源清理和存储分析。

## 脚本清单

| # | 脚本 | 语言 | 用途 | 风险等级 |
|:---:|:---|:---:|:---|:---:|
| 1 | [[31-脚本/automation/k8s-health-check\|k8s-health-check]] | Bash | 综合集群健康检查 (节点/Pod/事件/资源/证书) | 🟢 只读 |
| 2 | [[31-脚本/automation/gpu-utilization-report\|gpu-utilization-report]] | Bash+Python | GPU 利用率报告 | 🟢 只读 |
| 3 | [[31-脚本/automation/network-policy-audit\|network-policy-audit]] | Bash | NetworkPolicy 覆盖审计 | 🟢 只读 |
| 4 | [[31-脚本/automation/resource-cleanup\|resource-cleanup]] | Bash | 僵尸资源清理 (已完成 Job/孤立 PVC/无用 ConfigMap) | 🟡 中风险 |
| 5 | [[31-脚本/automation/certificate-expiry-monitor\|certificate-expiry-monitor]] | Bash | TLS 证书到期监控 | 🟢 只读 |
| 6 | [[31-脚本/automation/storage-audit\|storage-audit]] | Bash | PV/PVC 使用审计与优化建议 | 🟢 只读 |

## 使用指南

### 前置条件

- `kubectl` 已配置集群访问权限 (`~/.kube/config`)
- 具有 `cluster-reader` 或同等只读 RBAC 权限
- 脚本 4 (`resource-cleanup`) 需要 `edit` 权限

### 运行方式

```bash
# 直接执行单个脚本
bash 脚本/automation/k8s-health-check.sh

# 设置 cron 定期执行
# 每天早上 8 点执行健康检查
0 8 * * * /path/to/k8s-health-check.sh >> /var/log/k8s-health.log 2>&1

# 证书到期监控 — 每周一次
0 9 * * 1 /path/to/certificate-expiry-monitor.sh >> /var/log/cert-expiry.log 2>&1
```

### 输出格式

所有只读脚本 (🟢) 默认输出人类可读的终端报告。可通过 `--json` 或 `--output json` 参数输出 JSON 格式，便于集成到监控系统 (如 Alertmanager webhook)。

## Related

- [[31-脚本/prompts/incident-diagnosis|事件诊断 Prompt]] — 配合健康检查脚本做根因分析
- [[31-脚本/prompts/capacity-review|容量规划 Prompt]] — 配合 GPU 报告做右 Sizing
- [[31-脚本/prompts/security-audit|安全审计 Prompt]] — 配合 NetworkPolicy 审计做合规检查
- [[31-脚本/templates/runbook-template|Runbook 模板]] — 运维手册编写

<!-- risk-assessed -->
