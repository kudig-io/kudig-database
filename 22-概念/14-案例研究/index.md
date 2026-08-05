---
title: Case Studies
description: Case Studies 目录索引
summary: Case Studies 目录索引
category: index
tags:
- index
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Case Studies

> 本页为 `concepts/case-studies` 目录的自动索引。

## 概览

- [[22-概念/14-案例研究/README.md|Readme]]

## 文档

- [[22-概念/14-案例研究/2026-01-08-kubelet证书过期导致全节点notready.md|01 08 Kubelet证书过期导致全节点Notready]]
- [[22-概念/14-案例研究/2026-01-15-node-notready-pod-eviction.md|01 15 Node Notready Pod Eviction]]
- [[22-概念/14-案例研究/2026-01-22-coredns-discovery-failure.md|01 22 Coredns Discovery Failure]]
- [[22-概念/14-案例研究/2026-02-05-etcd-inconsistency-503.md|02 05 Etcd Inconsistency 503]]
- [[22-概念/14-案例研究/2026-02-12-coredns-hpa配置错误导致dns雪崩.md|02 12 Coredns Hpa配置错误导致Dns雪崩]]
- [[22-概念/14-案例研究/2026-02-18-hpa-thrashing.md|02 18 Hpa Thrashing]]
- [[22-概念/14-案例研究/2026-03-02-certificate-expiry-kubelet.md|03 02 Certificate Expiry Kubelet]]
- [[22-概念/14-案例研究/2026-03-08-容器运行时docker-daemon僵死导致pod无法创建.md|03 08 容器运行时Docker Daemon僵死导致Pod无法创建]]
- [[22-概念/14-案例研究/2026-03-15-oomkilled-java-restart.md|03 15 Oomkilled Java Restart]]
- [[22-概念/14-案例研究/2026-03-28-networkpolicy-misconfig.md|03 28 Networkpolicy Misconfig]]
- [[22-概念/14-案例研究/2026-04-05-storageclass动态供应失败导致所有有状态应用中断.md|04 05 Storageclass动态供应失败导致所有有状态应用中断]]
- [[22-概念/14-案例研究/2026-04-10-ingress-502-bad-gateway.md|04 10 Ingress 502 Bad Gateway]]
- [[22-概念/14-案例研究/2026-04-22-pvc-unbound-statefulset.md|04 22 Pvc Unbound Statefulset]]
- [[22-概念/14-案例研究/2026-05-01-imagepullbackoff-registry-auth.md|05 01 Imagepullbackoff Registry Auth]]
- [[22-概念/14-案例研究/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|05 10 Networkpolicy默认拒绝导致Ci Cd流水线全中断]]
- [[22-概念/14-案例研究/2026-05-15-configmap-no-rolling-update.md|05 15 Configmap No Rolling Update]]
- [[22-概念/14-案例研究/2026-05-28-daemonset-affinity-miss.md|05 28 Daemonset Affinity Miss]]
- [[22-概念/14-案例研究/2026-06-01-helm-release历史过多导致configmap超限.md|06 01 Helm Release历史过多导致Configmap超限]]
- [[22-概念/14-案例研究/2026-06-10-cronjob-concurrency-backlog.md|06 10 Cronjob Concurrency Backlog]]
- [[22-概念/14-案例研究/2026-06-20-节点时区不一致导致cronjob调度错乱.md|06 20 节点时区不一致导致Cronjob调度错乱]]
- [[22-概念/14-案例研究/2026-06-25-resourcequota-exceeded.md|06 25 Resourcequota Exceeded]]
- [[22-概念/14-案例研究/2026-07-08-prometheus-high-cardinality-oom.md|07 08 Prometheus High Cardinality Oom]]
- [[22-概念/14-案例研究/2026-07-15--admission-webhook超时导致所有api操作失败.md|07 15  Admission Webhook超时导致所有Api操作失败]]
- [[22-概念/14-案例研究/2026-07-20-velero-backup-failure.md|07 20 Velero Backup Failure]]
- [[22-概念/14-案例研究/2026-08-05-istio-mtls-strict.md|08 05 Istio Mtls Strict]]
- [[22-概念/14-案例研究/2026-08-10-容器内存限制过严导致java应用频繁oom.md|08 10 容器内存限制过严导致Java应用频繁Oom]]
- [[22-概念/14-案例研究/2026-08-18-cluster-autoscaler-scale-down-delay.md|08 18 Cluster Autoscaler Scale Down Delay]]
- [[22-概念/14-案例研究/2026-08-25-init-container失败导致deployment滚动更新卡死.md|08 25 Init Container失败导致Deployment滚动更新卡死]]
- [[22-概念/14-案例研究/2026-09-01-gpu-memory-leak.md|09 01 Gpu Memory Leak]]
- [[22-概念/14-案例研究/2026-09-05-污点容忍度配置错误导致pod无法调度到专用节点.md|09 05 污点容忍度配置错误导致Pod无法调度到专用节点]]
- [[22-概念/14-案例研究/2026-09-15-multicluster-network-partition.md|09 15 Multicluster Network Partition]]
- [[22-概念/14-案例研究/2026-09-20-服务端口名不一致导致istio-mtls握手失败.md|09 20 服务端口名不一致导致Istio Mtls握手失败]]
- [[22-概念/14-案例研究/2026-10-05-节点内核参数不一致导致sysctl配置冲突.md|10 05 节点内核参数不一致导致Sysctl配置冲突]]
- [[22-概念/14-案例研究/2026-10-15-pod-disruption-budget阻止节点维护排空.md|10 15 Pod Disruption Budget阻止节点维护排空]]
- [[22-概念/14-案例研究/2026-10-25-secret未更新导致rolling-update新旧版本配置不一致.md|10 25 Secret未更新导致Rolling Update新旧版本配置不一致]]



<!-- risk-assessed -->
