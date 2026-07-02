---
title: service connectivity Remediation Playbook
summary: service connectivity Remediation Playbook：kubectl get svc <svc> -o jsonpath='{.spec.selector}'
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-service-connectivity
last_updated: 2026-05-22
---



# [[Service|Service]] 连通性问题修复手册

## 修复步骤

### 修复 1：修正 Selector 标签

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 查看当前 selector
kubectl get svc <svc> -o jsonpath='{.spec.selector}'

# 查看后端 Pod 标签
kubectl get pods -l app=<correct-app> --show-labels

# 修正 Service selector
kubectl patch svc <svc> -p '{"spec":{"selector":{"app":"<correct-label>"}}}'
```

### 修复 2：重启 kube-proxy

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart daemonset kube-proxy -n kube-system
```

### 修复 3：删除并重建 Service

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl get svc <svc> -o yaml > svc-backup.yaml
kubectl delete svc <svc>
kubectl apply -f svc-backup.yaml
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub
