---
title: cloud-controller-manager
description: cloud-controller-manager — Kubernetes 生产运维知识库
summary: cloud-controller-manager (CCM) 是 Kubernetes 与云提供商集成的控制平面组件，负责运行云特定的控制器（节点、路由、负载均衡等）。
category: entities
tags:
- k8s
- cloud-controller-manager
- ccm
- control-plane
- cloud-provider
- load-balancer
- route
- node
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cloud-controller-manager 是什么
- 如何 cloud-controller-manager
trigger_keywords:
- cloud-controller-manager
- ccm
prerequisites:
- kubectl-basics
- kubernetes-concepts
- cloud-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cloud-controller-manager

## Role

cloud-controller-manager (CCM) runs cloud-provider-specific control loops that were previously embedded in kube-controller-manager. It decouples Kubernetes core from cloud vendor implementations.

Cloud-specific controllers include:
- **Node Controller**: Adds cloud metadata (providerID, addresses, labels) to nodes; deletes cloud nodes when removed from cloud console
- **Route Controller**: Configures cloud network routes for Pod CIDR
- **Service Controller**: Provisions cloud load balancers for LoadBalancer Services
- **Volume Controller**: (legacy) Manages cloud volume attach/detach, now mostly superseded by CSI

## Architecture

```
Kubernetes Control Plane
    ├── kube-apiserver
    ├── kube-controller-manager (generic controllers)
    └── cloud-controller-manager (cloud-specific controllers)
            ↑↓
    Cloud Provider API (AWS/Azure/GCP/阿里云/腾讯云)
```

CCM uses the Cloud Provider Interface (CPI) to talk to cloud APIs. In-tree cloud providers have been removed since Kubernetes 1.29; out-of-tree CCM is mandatory.

## Key Configuration

| Parameter | Purpose | Example |
|-----------|---------|---------|
| `--cloud-provider` | External provider name | `external` |
| `--cloud-config` | Path to cloud config file | `/etc/kubernetes/cloud-config` |
| `--leader-elect` | Enable leader election | `true` |
| `--route-reconciliation-period` | Route sync interval | `10s` |
| `--configure-cloud-routes` | Whether to manage cloud routes | `true` |
| `--use-service-account-credentials` | Use per-controller SA tokens | `true` |

## 运维操作

```bash
# 🟢 查看 CCM Pod 状态
kubectl get pods -n kube-system -l component=cloud-controller-manager
# 或按名称过滤（不同厂商标签不同）
kubectl get pods -n kube-system | grep cloud-controller

# 🟢 查看 CCM 日志
kubectl logs -n kube-system -l component=cloud-controller-manager --tail=100

# 🟢 查看节点 providerID
kubectl get node <node> -o jsonpath='{.spec.providerID}'

# 🟢 查看 LoadBalancer Service 事件
kubectl get svc <svc> -o wide
kubectl describe svc <svc> | grep -i ingress\|event

# 🟢 检查云路由表
# AWS: 查看 VPC Route Tables
# Azure: az network route-table list
# 阿里云：登录 VPC 控制台查看路由表

# 🟡 验证 CCM 权限
kubectl auth can-i create nodes --as=system:serviceaccount:kube-system:cloud-controller-manager
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 节点无 providerID / 外部 IP | CCM 未运行 / 云凭证失效 | `kubectl get pods -n kube-system \| grep ccm` | 重启 CCM / 检查 cloud-config |
| LoadBalancer Service 无 EXTERNAL-IP | Service Controller 异常 / 云 API 限流 | `kubectl describe svc <svc>` | 检查 CCM 日志 / 云配额 |
| Pod 跨节点不通 | Route Controller 未创建路由 | 检查云路由表 | 启用 `--configure-cloud-routes` / 检查 CNI |
| 节点被误删 | Node Controller 与云实例状态不一致 | 检查云控制台实例状态 | 调整 `--node-status-update-frequency` |
| 证书/STS Token 过期 | 云凭证失效 | CCM 日志出现 auth error | 更新 cloud-config / IRSA/ACK-RAM 凭证 |

```bash
# 排查流程
# 1. 检查 CCM 是否运行
kubectl get pods -n kube-system -l component=cloud-controller-manager

# 2. 检查节点 cloud metadata
kubectl get node <node> -o yaml | grep -A5 providerID
kubectl get node <node> -o jsonpath='{.status.addresses}'

# 3. 检查 Service Controller 事件
kubectl describe svc <lb-svc> | grep -i event

# 4. 检查 CCM 日志
kubectl logs -n kube-system <ccm-pod> | grep -iE "error|fail|warn"

# 5. 检查 cloud-config
kubectl get cm -n kube-system | grep cloud
```

## 生产案例

### 案例1：CCM 权限不足导致 LoadBalancer 无法创建
- **场景**：`kubectl get svc` 显示 LoadBalancer Service 长期处于 `<pending>`
- **排查**：CCM 日志显示 `UnauthorizedOperation: You are not authorized to perform this operation`
- **方案**：为 CCM 绑定的云 IAM 角色添加创建 SLB/ELB/安全组的权限；在阿里云 ACK 中检查 `addons.aliyun.com/ram-role` 注解
- **效果**：Service 成功分配 EXTERNAL-IP

### 案例2：云路由未同步导致跨节点 Pod 不通
- **场景**：同一 Service 的后端 Pod 跨节点时间歇性不通
- **排查**：云路由表中缺少部分节点 Pod CIDR 的路由；CCM Pod 因 OOM 反复重启
- **方案**：提升 CCM 内存 limit；启用 `--configure-cloud-routes=true`；检查控制平面节点到云 API 的网络
- **效果**：路由表完整，跨节点 Pod 通信恢复

## 检查清单

- [ ] CCM 已部署并持有 Leader Lease
- [ ] 云凭证（cloud-config / IRSA / RAM Role）有效且权限充足
- [ ] 节点 providerID 已正确写入
- [ ] LoadBalancer Service 能成功分配云负载均衡
- [ ] 云路由表包含所有节点 Pod CIDR
- [ ] CCM 资源限制（CPU/Memory）符合控制平面规模
- [ ] CCM 版本与 Kubernetes 版本兼容

## Related

- [[实体/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[实体/kube-controller-manager.md|kube-controller-manager]] — kube-controller-manager
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[集群基础/控制平面/14-cloud-controller-manager-deep-dive.md|cloud-controller-manager 深度解析]]
- [[故障诊断/FTA故障树/list/cloud-provider-fta.md|Cloud Provider 异常故障树分析]]


<!-- risk-assessed -->
