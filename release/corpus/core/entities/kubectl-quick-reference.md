---
title: Kubectl Quick Reference
description: Kubectl Quick Reference — Kubernetes 生产运维知识库
summary: Kubectl Quick Reference — Kubernetes 生产运维知识库
category: references
tags:
- kubectl
- cli
- reference
- troubleshooting
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubectl Quick Reference 是什么
- 如何 Kubectl Quick Reference
trigger_keywords:
- Kubectl
- Quick
- Reference
prerequisites:
- kubectl-basics
- etcd-basics
---



# Kubectl Quick Reference

## Cluster Diagnostics

```bash
# Cluster overview
kubectl cluster-info
kubectl get componentstatuses

# Node status
kubectl get nodes -o wide
kubectl describe node <node-name>
kubectl top nodes

# API resource discovery
kubectl api-resources
kubectl api-versions
```

## Pod Troubleshooting

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# Find problematic pods
kubectl get pods --all-namespaces | grep -v Running
kubectl get pods --field-selector status.phase!=Running

# Detailed pod info
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous    # Previous container
kubectl logs <pod> -n <ns> -c <container> # Specific container

# Event analysis
kubectl get events --sort-by='.lastTimestamp' -n <ns>
kubectl get events --field-selector type=Warning

# Exec into pod
kubectl exec -it <pod> -n <ns> -- /bin/sh
kubectl debug -it <pod> -n <ns> --image=busybox --target=<container>
```

## Resource Management

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# Get resources with labels
kubectl get <resource> -L app,version,env

# Sort by resource usage
kubectl top pods --sort-by=cpu
kubectl top pods --sort-by=memory

# JSONPath queries
kubectl get pods -o jsonpath='{.items[*].spec.nodeName}'
kubectl get svc -o jsonpath='{.items[?(@.spec.type=="LoadBalancer")].metadata.name}'

# Bulk operations
kubectl delete pods --field-selector status.phase=Failed -n <ns>
kubectl scale deployment <name> --replicas=3 -n <ns>
```

## Network Debugging

```bash
# Check endpoints
kubectl get endpoints <svc> -n <ns>
kubectl get endpointslices -n <ns>

# Service DNS resolution
kubectl run -it --rm dns-test --image=busybox --restart=Never -- nslookup <svc>.<ns>.svc.cluster.local

# Port forwarding
kubectl port-forward svc/<name> 8080:80 -n <ns>
kubectl port-forward pod/<name> 8080:80 -n <ns>
```

## Configuration and Secrets

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# View raw manifests
kubectl get <resource> <name> -o yaml
kubectl get cm <name> -o jsonpath='{.data}'

# Decode secrets
kubectl get secret <name> -o jsonpath='{.data.<key>}' | base64 -d

# Dry-run validation
kubectl apply -f manifest.yaml --dry-run=server --validate=true
```

## Etcd Operations

```bash
# Check etcd health
ETCDCTL_API=3 etcdctl endpoint health --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key

# Backup etcd
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-snapshot.db --endpoints=https://127.0.0.1:2379 --cacert=... --cert=... --key=...
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[skills/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[skills/troubleshoot-node-issues.md|Troubleshoot Node Issues]]
- [[entities/kube-apiserver.md|kube-apiserver]]
- [[etcd|etcd]]

- [[domain-07-platform-engineering/topic-code-analysis/node-create/08-troubleshooting.md|08-troubleshooting]]