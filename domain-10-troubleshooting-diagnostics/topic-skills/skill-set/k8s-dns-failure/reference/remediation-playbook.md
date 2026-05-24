---
title: "dns failure Remediation Playbook"
category: remediation
skill_set: "k8s-dns-failure"
created: "2026-05-22"
updated: "2026-05-22"
tags: ["reference", "remediation", "playbook", "visibility/public"]
---

# DNS 解析问题修复手册

## 修复步骤

### 修复 1：重启 [[CoreDNS|CoreDNS]]

```bash
kubectl rollout restart deployment coredns -n kube-system
kubectl rollout status deployment coredns -n kube-system
```

### 修复 2：修正 CoreDNS ConfigMap

```bash
kubectl get configmap coredns -n kube-system -o yaml
# 检查 Corefile 配置，修正后：
kubectl apply -f coredns-configmap-fixed.yaml
kubectl rollout restart deployment coredns -n kube-system
```

### 修复 3：扩大 CoreDNS 资源

```bash
kubectl patch deployment coredns -n kube-system -p '{"spec":{"template":{"spec":{"containers":[{"name":"coredns","resources":{"limits":{"memory":"256Mi","cpu":"500m"}}}]}}}}'
```

### 修复 4：修正 Pod DNS 配置

```bash
kubectl patch pod <pod> --type merge -p '{"spec":{"dnsPolicy":"ClusterFirst","dnsConfig":{"nameservers":["10.96.0.10"],"searches":["default.svc.cluster.local","svc.cluster.local","cluster.local"]}}}'
```


## 参见

- [[remediation-playbook]] — remediation 领域核心页面
