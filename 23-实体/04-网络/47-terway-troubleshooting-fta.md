---
title: Terway 故障排查
description: '# Terway 故障排查'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 故障排查 是什么
- 如何 Terway 故障排查
- Terway 故障排查 故障排查
- Terway 故障排查 排障步骤
trigger_keywords:
- Terway
- 故障排查
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
fta_id: FTA-47_TERWAY_TROUBLESHOOTING-001
component: 47 Terway Troubleshooting
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway 故障排查

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

本页是 Terway 网络故障的快速排查参考（FTA Troubleshooting Quick Reference），涵盖生产环境中最常见的 Terway 网络故障场景、诊断步骤和修复方案。Terway 故障排查的核心思路是分层定位：**Pod 网络配置层**（CNI 是否正确配置了网络命名空间）→ **节点网络层**（ENI/路由/安全组是否正常）→ **VPC 网络层**（阿里云 VPC 路由/安全组是否正确）。

常见的 Terway 故障症状包括：Pod 卡在 ContainerCreating、Pod 无网络连通性、NetworkPolicy 未生效、IP 地址分配失败。这些问题通常由 ENI IP 耗尽、安全组配置错误、terway-daemon 异常或阿里云 API 限流引起。

## 常见故障场景

- **Pod 卡在 ContainerCreating**：CNI 调用失败，通常是 IPAM 分配失败或 ENI API 超时
- **Pod 无网络**：网络配置成功但流量不通，通常是安全组或 VPC 路由问题
- **IP 耗尽**：节点辅助 IP 配额用完，新 Pod 无法分配 IP
- **NetworkPolicy 不生效**：策略引擎异常或规则编译失败
- **ENI API 限流**：阿里云 ENI API QPS 超限导致 IP 分配失败
- **terway-daemon OOM/Crash**：Daemon 容器异常退出

## 诊断流程

```
Pod 网络故障
    ↓
检查 Pod 事件（kubectl describe pod）
    ↓
检查 terway-daemon 日志（kubectl logs）
    ↓
节点上运行 terway-cli 诊断
    ↓
检查 ENI 和辅助 IP 状态
    ↓
检查安全组和 VPC 路由
    ↓
定位根因并修复
```

## Architecture

Terway 故障排查需要理解三层网络路径：**Pod 层**（Veth Pair、路由表、iptables/eBPF 规则）、**节点层**（ENI 设备、内核路由、conntrack 表）和**VPC 层**（VPC 路由表、安全组、NAT 网关）。故障可能发生在任何一层，排查时需要从 Pod 侧逐步向外定位。

## K8s 集成

Terway 故障通过 Kubernetes 事件和日志暴露。`kubectl describe pod` 显示 CNI 调用错误。terway-daemon 的日志在 kube-system 命名空间。`terway-cli` 在节点上提供详细的网络状态诊断。NetworkPolicy 的问题通过 `kubectl get networkpolicy` 和策略引擎统计诊断。

## 生产部署要点

- **快速诊断**：建立标准化的排查流程，减少平均恢复时间（MTTR）
- **告警覆盖**：对 IP 耗尽、terway-daemon 重启等关键故障配置告警
- **日志保留**：terway-daemon 日志保留足够长时间用于事后分析

## 生产场景

1. **Pod 创建失败**：新部署的 Pod 一直 ContainerCreating，CNI 报 IP 分配错误
2. **网络间歇不通**：某些 Pod 间通信时断时续，可能是 conntrack 表满
3. **NetworkPolicy 异常**：安全策略未按预期生效，流量未正确阻断
4. **节点 NotReady 后网络异常**：节点恢复后 Pod 网络不通

## 操作命令

```bash
# 🟢 步骤1：检查 Pod 事件和状态
kubectl describe pod <pod-name>
# 关注 Events 中的 "failed to setup network" 错误

# 🟢 步骤2：检查 terway-daemon 日志
kubectl logs -n kube-system terway-daemon-<node-name> --tail=100 | grep -i error
kubectl logs -n kube-system terway-daemon-<node-name> --previous

# 🟢 步骤3：在节点上运行诊断
terway-cli show eni           # ENI 状态
terway-cli show ip            # IP 分配状态
terway-cli check pod <pod-id> # 特定 Pod 网络诊断

# 🟢 步骤4：检查 IP 配额
# 查看节点的 ENI 和辅助 IP 限制
kubectl get node <node> -o jsonpath='{.metadata.annotations}' | jq | grep -i eni

# 🟡 步骤5：重启 terway-daemon（如卡死）
kubectl delete pod -n kube-system terway-daemon-<node-name>
# DaemonSet 会自动重建

# 🟢 步骤6：检查 NetworkPolicy
kubectl get networkpolicy -A
terway-cli show policy --node <node>
terway-cli show policy-stats --node <node>

# 🟢 步骤7：连通性测试
kubectl exec -it <debug-pod> -- ping <target-ip>
kubectl exec -it <debug-pod> -- curl -v http://<service>:8080
```

## 故障对照表

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|---------|---------|---------|
| ContainerCreating | IP 耗尽 | `terway-cli show ip` | 扩容节点或减少 Pod 密度 |
| 网络不通 | 安全组错误 | 检查 VPC 安全组 | 修正安全组规则 |
| 间歇断连 | conntrack 满 | `cat /proc/sys/net/netfilter/nf_conntrack_count` | 调大 conntrack_max |
| NP 不生效 | 引擎异常 | `terway-cli show policy-stats` | 重启 terway-daemon |

## 参考链接

- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[26-技能/05-网络/service/诊断排障/ts-networking.md|ts-networking]] — 网络故障排查
- [[k8gb]] — K8GB
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 47-terway-troubleshooting-fta

<!-- risk-assessed -->
