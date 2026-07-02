---
title: dns failure Remediation Playbook
summary: dns failure Remediation Playbook：kubectl rollout restart deployment coredns
  -n kube-system kubectl rollout status deployment coredns -n kube-system
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-dns-failure
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DNS 解析问题修复手册

## 修复步骤

### 修复 1：重启 [[CoreDNS|CoreDNS]]

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment coredns -n kube-system
kubectl rollout status deployment coredns -n kube-system
```
### 修复 2：修正 CoreDNS ConfigMap

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get configmap coredns -n kube-system -o yaml
# 检查 Corefile 配置，修正后：
kubectl apply -f coredns-configmap-fixed.yaml
kubectl rollout restart deployment coredns -n kube-system
```
### 修复 3：扩大 CoreDNS 资源

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment coredns -n kube-system -p '{"spec":{"template":{"spec":{"containers":[{"name":"coredns","resources":{"limits":{"memory":"256Mi","cpu":"500m"}}}]}}}}'
```
### 修复 4：修正 Pod DNS 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch pod <pod> --type merge -p '{"spec":{"dnsPolicy":"ClusterFirst","dnsConfig":{"nameservers":["10.96.0.10"],"searches":["default.svc.cluster.local","svc.cluster.local","cluster.local"]}}}'
```
## 参见

- [[remediation-playbook]] — remediation 领域核心页面

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
