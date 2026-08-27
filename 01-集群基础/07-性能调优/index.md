---
title: Performance Tuning
description: 性能调优知识域 — 集群性能调优、网络性能优化、存储性能优化、API Server/etcd 调优
summary: 性能调优子目录索引，涵盖集群整体性能调优、网络延迟优化、存储 IOPS 优化、控制平面性能基准
category: subdomain
tags:
- performance
- tuning
- optimization
- benchmark
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 性能调优 Performance Tuning

> 集群、网络、存储全方位性能优化。

## 调优层次

| 层次 | 关键指标 | 调优方向 |
|------|----------|----------|
| 控制平面 | API 延迟、etcd 延迟 | API Server 参数、etcd 磁盘 |
| 网络 | Pod 延迟、带宽 | CNI 选择、MTU、网络策略 |
| 存储 | IOPS、延迟 | StorageClass、PV 类型 |
| 调度 | 调度延迟 | 调度器配置、抢占策略 |

## 文档索引

| 文件 | 内容 | 难度 |
|------|------|------|
| [[01-集群基础/07-性能调优/03-cluster-performance-tuning.md\|集群性能调优]] | 整体性能评估、参数调优 | advanced |
| [[01-集群基础/07-性能调优/04-network-performance-optimization.md\|网络性能优化]] | CNI 性能、MTU、网络策略 | advanced |
| [[01-集群基础/07-性能调优/05-storage-performance-optimization.md\|存储性能优化]] | IOPS、延迟、存储类型选择 | advanced |
| [[01-集群基础/07-性能调优/01-apiserver-etcd-performance-tuning.md\|API Server/etcd 调优]] | 控制平面性能深度调优 | advanced |

## 性能基准参考

| 场景 | 目标 | 测量方法 |
|------|------|----------|
| API 延迟 | P99 < 100ms | apiserver_request_duration_seconds |
| Pod 启动 | < 30s | kubelet_pod_start_duration_seconds |
| 网络延迟 | < 1ms (同节点) | ping/iperf |
| 存储 IOPS | > 3000 (SSD) | fio |

## Related

- [[01-集群基础/03-控制平面/index.md|控制平面]] — API Server/etcd
- [[05-网络/01-K8s网络核心/index.md|K8s 网络核心]] — CNI 性能
- [[09-可观测性/02-指标/index.md|指标监控]] — 性能指标采集

