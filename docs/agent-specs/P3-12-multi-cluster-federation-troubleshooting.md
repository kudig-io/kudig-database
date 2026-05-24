---
title: 多集群/联邦场景问题排查
description: '# 多集群/联邦场景问题排查'
category: general
tags:
- k8s
- cilium
- argocd
- flux
- gateway
- rbac
- networkpolicy
- crd
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 多集群/联邦场景问题排查 是什么
- 如何 多集群/联邦场景问题排查
- 多集群/联邦场景问题排查 问题排查
- 多集群/联邦场景问题排查 排障步骤
trigger_keywords:
- 多集群
- 联邦场景问题排查
prerequisites:
- kubectl-basics
- gitops-basics
- cilium-basics
created: "2026-05-23"
---

# 多集群/联邦场景问题排查

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 集群联邦 (Kubefed)、GitOps 多集群冲突、多集群网络打通的问题排查
> **关联**: domain-37-edge-computing, domain-08-release-change-management

---

## 1. 集群联邦 (Kubefed) 问题排查

### 1.1 Kubefed 控制平面异常

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| FederatedResource 无法创建 | `kubectl describe federatedtype` | Kubefed CRD 未安装 | `kubefed enable <type>` |
| 资源未同步到成员集群 | `kubectl -n kube-federation-system get pods` | 控制器未运行 | 检查控制器日志 |
| 成员集群无法注册 | `kubectl get kubefedclusters` | 网络不通/凭证问题 | 检查 kubeconfig |

```bash
# Kubefed 状态检查
kubectl get kubefedclusters -n kube-federation-system
kubectl get federatedtypeconfigs -A

# 查看控制器日志
kubectl logs -n kube-federation-system deployment/<controller> --tail=100

# 常见问题
# "Cluster not reachable" → 检查网络连通性和 kubeconfig
# " RBAC error" → 检查 member cluster 权限

# 重新注册成员集群
kubefed join <cluster-name> --cluster-context <context> --host-cluster-context <host>
```

### 1.2 资源同步问题

```bash
# 检查资源同步状态
kubectl describe federateddeployment <name> -n <ns>

# 查看 Sync Controller 状态
kubectl get pods -n kube-federation-system | grep sync

# 常见问题
# 资源未创建 → 检查 type configuration
# 资源创建但不匹配 → 检查 override 字段

# 手动触发同步
kubectl annotate federateddeployment <name> -n <ns> kubefed.io/sync-time=$(date +%s)
```

---

## 2. GitOps 多集群冲突

### 2.1 ArgoCD 多集群同步问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Application 一直 OutOfSync | `argocd app get <app>` | Git 仓库变更未同步 | `argocd app sync <app>` |
| Application Sync 失败 | `argocd app history <app>` | K8s 资源冲突 | `argocd app logs <app>` |
| 多集群部署不一致 | `argocd app diff <app>` | 不同步到所有集群 | 检查 destination 配置 |

```bash
# ArgoCD 诊断
argocd cluster list
argocd app list
argocd app get <app> -o json | jq '.status.sync'

# 查看同步历史
argocd app history <app>
argocd app logs <app> --Revision <rev>

# 常见问题
# "comparison failed" → Git 仓库内容与集群不一致
# "prune required" → 需要删除集群中的资源
# "timeout" → K8s API 响应慢

# 强制同步
argocd app sync <app> --force

# 检查 webhook 配置
argocd repo get <repo> --details | grep webhook
```

### 2.2 [[flux|Flux]] 多集群配置问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Flux 无法拉取 Git | `flux logs --namespace=flux-system` | Git credentials 问题 | 检查 GitRepository 凭证 |
| Image update 未触发 | `flux get images policy` | Image 仓库配置错误 | 检查 ImagePolicy |
| 资源冲突 | `flux reconcile kustomization <name>` | Kustomize 配置错误 | `flux logs` 查看详情 |

```bash
# Flux 诊断
flux get all -A
flux logs --namespace=flux-system --tail=100

# GitRepository 状态
flux get source git -A
flux verify secret gitrepository <name> -n <ns>

# Kustomization 状态
flux get kustomization -A
flux reconcile kustomization <name> -n <ns>

# 常见问题
# "authentication failed" → 更新 Git credentials Secret
# "manifest generation failed" → 检查 kustomization.yaml 配置
```

### 2.3 多集群配置冲突检测

```bash
# 检测资源差异
for cluster in cluster-1 cluster-2 cluster-3; do
  kubectl --context $cluster get deployment -A -o yaml > /tmp/$cluster-deploy.yaml
done
diff /tmp/cluster-1-deploy.yaml /tmp/cluster-2-deploy.yaml

# 检测配置漂移
argocd app diff <app> --cluster <cluster>

# 检测版本差异
for cluster in cluster-1 cluster-2; do
  kubectl --context $cluster get deployment <name> -n <ns> -o jsonpath='{.spec.template.spec.containers[0].image}'
done
```

---

## 3. 多集群网络打通

### 3.1 跨集群 Service 访问

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 无法跨集群访问 Service | `kubectl exec <pod> -- curl <svc>.<ns>.svc.cluster.local` | 跨集群 DNS 未配置 | 配置 Submariner 或 Cilium ClusterMesh |
| Service 无法导出 | `kubectl get exporting` | 导出策略未配置 | 配置 ServiceExport |

```bash
# Submariner 状态检查
submarinerctl status
submarinerctl clusters list

# 检查 ServiceExport
kubectl get serviceexports -A

# 常见问题
# "no route to host" → 检查 Submariner gateway 节点
# "cross-cluster DNS not working" → 检查 lighthouse-core-dns 配置

# 手动测试跨集群访问
kubectl exec -it <pod> -- nc -vz <svc>.<ns>.svc.cluster.local 443
```

### 3.2 跨集群网络连通性

```bash
# 检查网络路径
kubectl exec -it <pod> -- traceroute <target-svc>.<target-ns>.svc.cluster.local

# 检查 CNI 状态 (Cilium)
cilium status
cilium connectivity test

# 检查网络策略
kubectl get networkpolicy -A | grep -v "default-deny"

# 常见问题
# "packet loss" → 检查 CNI 插件和 MTU 设置
# "timeout" → 检查 firewall rules 和 CNI plugin status
```

---

## 4. 联邦学习/多租户场景

### 4.1 多集群资源配额问题

```bash
# 检查 ResourceQuota
kubectl get resourcequota -n <ns>
kubectl describe resourcequota -n <ns>

# 检查 LimitRange
kubectl get limitrange -n <ns>
kubectl describe limitrange -n <ns>

# 常见问题
# "exceeded quota" → 检查请求的资源量是否合理
# "limit not set" → 设置 LimitRange 自动注入默认值
```

### 4.2 集群联邦网络隔离

```bash
# 检查 NetworkPolicy 跨集群影响
kubectl get networkpolicy -A -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"'

# 多集群安全策略
# 使用 Cilium ClusterMesh 或 Submariner 时注意:
# - 跨集群流量需要明确配置
# - 默认 deny 策略可能阻止跨集群通信
```

---

## 5. 快速检查清单

### 多集群 on-call 速查

```bash
# 检查所有集群健康
for cluster in cluster-a cluster-b cluster-c; do
  echo "=== $cluster ==="
  kubectl --context $cluster get nodes --no-headers | grep -v Ready && echo "WARNING: Unhealthy nodes found"
done

# 检查 ArgoCD/Flux 同步状态
argocd app list 2>/dev/null || echo "ArgoCD not available"
flux get kustomization -A 2>/dev/null || echo "Flux not available"

# 检查跨集群连接
submarinerctl status 2>/dev/null || echo "Submariner not configured"

# 检查 Kubefed
kubectl get kubefedclusters -n kube-federation-system 2>/dev/null || echo "Kubefed not installed"
```

---

## 6. 升级条件

| 条件 | 操作 |
|------|------|
| 多个集群控制平面同时问题 | 立即升级 SRE + 多云团队 |
| 跨集群网络完全中断 | 立即升级网络团队 |
| GitOps 冲突导致服务中断 | 升级 GitOps 团队 |
| 联邦资源数据不一致 | 升级 SRE 团队 |

---

**关联文档**:
- [domain-08-release-change-management/](../domain-08-release-change-management/) — GitOps CI/CD
- [domain-15-specialized-tech/](../domain-15-specialized-tech/) — 边缘计算
- [domain-10-troubleshooting-diagnostics/topic-skills/](../domain-10-troubleshooting-diagnostics/topic-skills/) — 运维 Skill
- [P1-5: On-call 快速参考卡](./P1-5-oncall-quick-reference-card.md)