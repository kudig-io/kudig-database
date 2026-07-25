---
title: Terway 运维手册
description: '# Terway 运维手册'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- prometheus
- grafana
- cilium
- networkpolicy
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 运维手册 是什么
- 如何 Terway 运维手册
trigger_keywords:
- Terway
- 运维手册
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway 运维手册

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

Terway 是阿里云 ACK 集群的默认 CNI 插件。本页是 Terway 的日常运维参考手册，涵盖监控指标、告警配置、版本升级、节点维护和常见运维操作。Terway 运维的核心是确保每个节点的 ENI 和辅助 IP 正常分配，Pod 网络保持连通。

Terway 的运维职责包括：**IPAM 管理**（确保节点有足够的辅助 IP 供 Pod 使用）、**ENI 生命周期管理**（节点的 ENI 创建、附加和释放）、**NetworkPolicy 引擎维护**（iptables/eBPF 规则的编译和加载）、**监控告警**（网络指标采集和异常告警）。

## 运维操作

- **IPAM 监控**：监控每节点的辅助 IP 分配率，避免 IP 耗尽
- **ENI 管理**：节点的 ENI 附加状态、安全组绑定
- **版本升级**：Terway 版本升级的滚动更新策略
- **节点维护**：cordon/drain 节点时的网络清理
- **监控告警**：Terway 关键指标（ENI 分配、IP 使用率、策略匹配延迟）
- **日志收集**：terway-daemon 日志和 CNI 操作日志

## Architecture

Terway 运维涉及三层组件：**节点级**（terway-daemon、CNI binary、ENI 设备）、**集群级**（terway-controller、Felix/PolicyEngine）和**云平台级**（阿里云 ENI API、安全组、VPC 路由）。运维操作需要理解三层的依赖关系——例如 ENI API 限流会影响 IPAM 分配，进而导致 Pod 创建失败。

## K8s 集成

Terway 作为 DaemonSet 运行在 kube-system 命名空间。运维操作通过 kubectl 和 `terway-cli` 工具完成。升级通过更新 terway DaemonSet 镜像版本实现。Prometheus ServiceMonitor 配置指标采集。节点 drain 时，Terway 自动清理该节点上 Pod 的网络配置并释放辅助 IP。

## 生产部署要点

- **IP 容量告警**：设置辅助 IP 使用率 >80% 告警，提前扩容
- **ENI API 限流**：阿里云 ENI API 有 QPS 限制，大规模集群需要分批操作
- **版本兼容性**：Terway 升级前验证与 ACK 版本的兼容性
- **日志保留**：terway-daemon 日志保留至少 7 天用于故障排查

## 生产场景

1. **日常 IPAM 巡检**：每日检查各节点辅助 IP 使用率，识别即将耗尽的节点
2. **Terway 版本升级**：滚动升级 Terway 到新版本，确保网络无中断
3. **节点维护**：cordon/drain 节点时清理网络资源，维护后恢复
4. **安全组批量更新**：批量更新 Pod 安全组规则，验证网络隔离

## 操作命令

```bash
# 🟢 查看 Terway DaemonSet 状态
kubectl get ds -n kube-system terway
kubectl rollout status ds/terway -n kube-system

# 🟢 查看节点 IP 分配情况
kubectl get node <node-name> -o jsonpath='{.metadata.annotations}' | jq 'to_entries[] | select(.key | startswith("network"))'

# 🟢 查看 Terway 日志
kubectl logs -n kube-system terway-daemon-<node-name> --tail=50
kubectl logs -n kube-system terway-daemon-<node-name> --previous

# 🟡 升级 Terway 版本（滚动更新）
kubectl set image ds/terway terway=registry.cn-hangzhou.aliyuncs.com/acs/terway:v1.2.3 -n kube-system
kubectl rollout status ds/terway -n kube-system

# 🟢 在节点上运行 terway-cli 诊断
terway-cli show eni          # 显示 ENI 状态
terway-cli show ip           # 显示 IP 分配
terway-cli show policy       # 显示 NetworkPolicy 规则

# 🟢 检查 NetworkPolicy 统计
kubectl exec -n kube-system terway-daemon-<node> -- terway-cli show policy-stats
```

## 对比

| 运维维度 | Terway | Cilium | Calico |
|----------|--------|--------|--------|
| 诊断工具 | terway-cli | cilium-cli | calicoctl |
| IPAM 管理 | ENI 辅助 IP | 可配置 | 可配置 |
| 监控指标 | Prometheus | Prometheus/Hubble | Prometheus |
| 升级方式 | DaemonSet | DaemonSet | DaemonSet |

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]

## Related

- [[antrea]] — Antrea
- [[40-terway-product-overview]] — Terway 产品概览
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 44-terway-operations-manual

<!-- risk-assessed -->
