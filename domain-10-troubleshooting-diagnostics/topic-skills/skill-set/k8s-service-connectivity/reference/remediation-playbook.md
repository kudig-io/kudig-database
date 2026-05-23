---
title: "service connectivity Remediation Playbook"
category: remediation
skill_set: "k8s-service-connectivity"
created: "2026-05-22"
updated: "2026-05-22"
---

# [[Service|Service]] 连通性问题修复手册

## 修复步骤

### 修复 1：修正 Selector 标签

```bash
# 查看当前 selector
kubectl get svc <svc> -o jsonpath='{.spec.selector}'

# 查看后端 Pod 标签
kubectl get pods -l app=<correct-app> --show-labels

# 修正 Service selector
kubectl patch svc <svc> -p '{"spec":{"selector":{"app":"<correct-label>"}}}'
```

### 修复 2：重启 kube-proxy

```bash
kubectl rollout restart daemonset kube-proxy -n kube-system
```

### 修复 3：删除并重建 Service

```bash
kubectl get svc <svc> -o yaml > svc-backup.yaml
kubectl delete svc <svc>
kubectl apply -f svc-backup.yaml
```
