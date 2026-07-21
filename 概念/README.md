---
title: Concepts & Synthesis
description: 概念知识域 — 跨域综合分析、概念交叉知识、案例研究、生产故障分析
summary: 概念知识域入口，涵盖 K8s 核心概念交叉分析、apiserver/etcd 关联知识、安全/可观测性/发布综合文档、生产案例研究
category: domain
tags:
- concepts
- synthesis
- cross-cutting
- case-studies
tier: core
created: '2026-05-24'
last_updated: '2026-07-21'
difficulty: advanced
audience:
- 所有工程师
- 架构师
- SRE
estimated_read_time: 15min
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 概念 Concepts & Synthesis

> 跨领域综合分析页面，连接多个概念和域的交叉知识。

## 核心综合分析

- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]]
- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- [[概念/CRD × 可观测性.md|CRD × 可观测性]]
- [[概念/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]]
- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]
- [[概念/Deployment-×-Ingress.md|Deployment-×-Ingress]]
- [[概念/Deployment-×-NetworkPolicy.md|Deployment-×-NetworkPolicy]]
- [[概念/Deployment-×-PV.md|Deployment-×-PV]]
- [[概念/Deployment-×-PVC.md|Deployment-×-PVC]]
- [[概念/Deployment-×-RBAC.md|Deployment-×-RBAC]]
- [[概念/Deployment-×-Service.md|Deployment-×-Service]]
- [[概念/GitOps x 平台工程.md|GitOps x 平台工程]]
- [[概念/IaC x 多集群管理.md|IaC x 多集群管理]]
- [[概念/K8s 故障分布与 MTTR 基准.md|K8s 故障分布与 MTTR 基准]]
- [[概念/Kubernetes Fault Distribution and MTTR.md|Kubernetes Fault Distribution and MTTR]]
- [[概念/MOC.md|MOC]]
- [[概念/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]]
- [[概念/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]
- [[概念/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]]
- [[概念/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]
- [[概念/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]
- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]]
- [[概念/StatefulSet-×-Ingress.md|StatefulSet-×-Ingress]]
- [[概念/StatefulSet-×-NetworkPolicy.md|StatefulSet-×-NetworkPolicy]]
- [[概念/StatefulSet-×-PV.md|StatefulSet-×-PV]]
- [[概念/StatefulSet-×-PVC.md|StatefulSet-×-PVC]]
- [[概念/StatefulSet-×-RBAC.md|StatefulSet-×-RBAC]]
- [[概念/StatefulSet-×-Service.md|StatefulSet-×-Service]]
- [[概念/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[概念/ai-agent-ops-patterns.md|ai-agent-ops-patterns]]
- [[概念/ai-ml-observability.md|ai-ml-observability]]
- [[概念/apiserver-×-Deployment.md|apiserver-×-Deployment]]
- [[概念/apiserver-×-GitOps.md|apiserver-×-GitOps]]
- [[概念/apiserver-×-Grafana.md|apiserver-×-Grafana]]
- [[概念/apiserver-×-IaC.md|apiserver-×-IaC]]
- [[概念/apiserver-×-Ingress.md|apiserver-×-Ingress]]
- [[概念/apiserver-×-NetworkPolicy.md|apiserver-×-NetworkPolicy]]
- [[概念/apiserver-×-PV.md|apiserver-×-PV]]
- [[概念/apiserver-×-PVC.md|apiserver-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/apiserver-×-Prometheus.md|apiserver-×-Prometheus]]
- [[概念/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[概念/apiserver-×-Service.md|apiserver-×-Service]]
- [[概念/apiserver-×-StatefulSet.md|apiserver-×-StatefulSet]]
- [[概念/apiserver-×-备份.md|apiserver-×-备份]]
- [[概念/apiserver-×-滚动更新.md|apiserver-×-滚动更新]]
- [[概念/apiserver-×-灾难恢复.md|apiserver-×-灾难恢复]]
- [[概念/apiserver-×-节点诊断.md|apiserver-×-节点诊断]]
- [[概念/apiserver-×-蓝绿发布.md|apiserver-×-蓝绿发布]]
- [[概念/backstage-platform-catalog.md|backstage-platform-catalog]]
- [[概念/chaos-drill-integration.md|chaos-drill-integration]]
- [[概念/chaos-engineering-observability.md|chaos-engineering-observability]]
- [[概念/consolidation-2026-05-21.md|consolidation-2026-05-21]]
- [[概念/consolidation-2026-05-23.md|consolidation-2026-05-23]]
- [[概念/cost-optimization-multi-cluster.md|cost-optimization-multi-cluster]]
- [[概念/cross-cloud-migration-playbook.md|cross-cloud-migration-playbook]]
- [[概念/data-protection-k8s.md|data-protection-k8s]]
- [[概念/eBPF x 运行时安全.md|eBPF x 运行时安全]]
- [[概念/edge-cloud-continuum.md|edge-cloud-continuum]]
- [[概念/etcd x 高可用模式.md|etcd x 高可用模式]]
- [[概念/etcd × Operator 模式.md|etcd × Operator 模式]]
- [[概念/etcd × 可观测性.md|etcd × 可观测性]]
- [[概念/etcd-×-Deployment.md|etcd-×-Deployment]]
- [[概念/etcd-×-GitOps.md|etcd-×-GitOps]]
- [[概念/etcd-×-Grafana.md|etcd-×-Grafana]]
- [[概念/etcd-×-IaC.md|etcd-×-IaC]]
- [[概念/etcd-×-Ingress.md|etcd-×-Ingress]]
- [[概念/etcd-×-NetworkPolicy.md|etcd-×-NetworkPolicy]]
- [[概念/etcd-×-PV.md|etcd-×-PV]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/etcd-×-Pod诊断.md|etcd-×-Pod诊断]]
- [[概念/etcd-×-Prometheus.md|etcd-×-Prometheus]]
- [[概念/etcd-×-RBAC.md|etcd-×-RBAC]]
- [[概念/etcd-×-Service.md|etcd-×-Service]]
- [[概念/etcd-×-StatefulSet.md|etcd-×-StatefulSet]]
- [[概念/etcd-×-备份.md|etcd-×-备份]]
- [[概念/etcd-×-滚动更新.md|etcd-×-滚动更新]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]
- [[概念/etcd-×-节点诊断.md|etcd-×-节点诊断]]
- [[概念/etcd-×-蓝绿发布.md|etcd-×-蓝绿发布]]
- [[概念/finops-resource-governance.md|finops-resource-governance]]
- [[概念/gitops-release-gate.md|gitops-release-gate]]
- [[概念/gitops-sre-release-gate.md|gitops-sre-release-gate]]
- [[概念/gpu-scheduling-ai-workloads.md|gpu-scheduling-ai-workloads]]
- [[概念/kubeadm-cluster-operations.md|kubeadm-cluster-operations]]
- [[概念/multi-cluster-observability-federation.md|multi-cluster-observability-federation]]
- [[概念/multi-cluster-security.md|multi-cluster-security]]
- [[概念/observability-finops.md|observability-finops]]
- [[概念/platform-engineering-sre.md|platform-engineering-sre]]
- [[概念/security-observability-correlation.md|security-observability-correlation]]
- [[概念/service-mesh-security-governance.md|service-mesh-security-governance]]
- [[概念/service-mesh-zero-trust-security.md|service-mesh-zero-trust-security]]
- [[概念/slo-monitoring-integration.md|slo-monitoring-integration]]
- [[概念/velero-disaster-recovery.md|velero-disaster-recovery]]
- [[概念/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]]
- [[概念/声明式 API × 控制器模式.md|声明式 API × 控制器模式]]
- [[概念/控制器模式 × Deployment.md|控制器模式 × Deployment]]
- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]]
- [[概念/控制器模式 × 可观测性.md|控制器模式 × 可观测性]]
- [[概念/服务网格 x 零信任安全.md|服务网格 x 零信任安全]]
- [[概念/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]]

## 案例研究

- [[概念/case-studies/2026-01-08-kubelet证书过期导致全节点notready.md|2026-01-08-kubelet证书过期导致全节点notready]]
- [[概念/case-studies/2026-01-15-node-notready-pod-eviction.md|2026-01-15-node-notready-pod-eviction]]
- [[概念/case-studies/2026-01-22-coredns-discovery-failure.md|2026-01-22-coredns-discovery-failure]]
- [[概念/case-studies/2026-02-05-etcd-inconsistency-503.md|2026-02-05-etcd-inconsistency-503]]
- [[概念/case-studies/2026-02-12-coredns-hpa配置错误导致dns雪崩.md|2026-02-12-coredns-hpa配置错误导致dns雪崩]]
- [[概念/case-studies/2026-02-18-hpa-thrashing.md|2026-02-18-hpa-thrashing]]
- [[概念/case-studies/2026-03-02-certificate-expiry-kubelet.md|2026-03-02-certificate-expiry-kubelet]]
- [[概念/case-studies/2026-03-08-容器运行时docker-daemon僵死导致pod无法创建.md|2026-03-08-容器运行时docker-daemon僵死导致pod无法创建]]
- [[概念/case-studies/2026-03-15-oomkilled-java-restart.md|2026-03-15-oomkilled-java-restart]]
- [[概念/case-studies/2026-03-28-networkpolicy-misconfig.md|2026-03-28-networkpolicy-misconfig]]
- [[概念/case-studies/2026-04-05-storageclass动态供应失败导致所有有状态应用中断.md|2026-04-05-storageclass动态供应失败导致所有有状态应用中断]]
- [[概念/case-studies/2026-04-10-ingress-502-bad-gateway.md|2026-04-10-ingress-502-bad-gateway]]
- [[概念/case-studies/2026-04-22-pvc-unbound-statefulset.md|2026-04-22-pvc-unbound-statefulset]]
- [[概念/case-studies/2026-05-01-imagepullbackoff-registry-auth.md|2026-05-01-imagepullbackoff-registry-auth]]
- [[概念/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]
- [[概念/case-studies/2026-05-15-configmap-no-rolling-update.md|2026-05-15-configmap-no-rolling-update]]
- [[概念/case-studies/2026-05-28-daemonset-affinity-miss.md|2026-05-28-daemonset-affinity-miss]]
- [[概念/case-studies/2026-06-01-helm-release历史过多导致configmap超限.md|2026-06-01-helm-release历史过多导致configmap超限]]
- [[概念/case-studies/2026-06-10-cronjob-concurrency-backlog.md|2026-06-10-cronjob-concurrency-backlog]]
- [[概念/case-studies/2026-06-20-节点时区不一致导致cronjob调度错乱.md|2026-06-20-节点时区不一致导致cronjob调度错乱]]
- [[概念/case-studies/2026-06-25-resourcequota-exceeded.md|2026-06-25-resourcequota-exceeded]]
- [[概念/case-studies/2026-07-08-prometheus-high-cardinality-oom.md|2026-07-08-prometheus-high-cardinality-oom]]
- [[概念/case-studies/2026-07-15--admission-webhook超时导致所有api操作失败.md|2026-07-15--admission-webhook超时导致所有api操作失败]]
- [[概念/case-studies/2026-07-20-velero-backup-failure.md|2026-07-20-velero-backup-failure]]
- [[概念/case-studies/2026-08-05-istio-mtls-strict.md|2026-08-05-istio-mtls-strict]]
- [[概念/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[概念/case-studies/2026-08-18-cluster-autoscaler-scale-down-delay.md|2026-08-18-cluster-autoscaler-scale-down-delay]]
- [[概念/case-studies/2026-08-25-init-container失败导致deployment滚动更新卡死.md|2026-08-25-init-container失败导致deployment滚动更新卡死]]
- [[概念/case-studies/2026-09-01-gpu-memory-leak.md|2026-09-01-gpu-memory-leak]]
- [[概念/case-studies/2026-09-05-污点容忍度配置错误导致pod无法调度到专用节点.md|2026-09-05-污点容忍度配置错误导致pod无法调度到专用节点]]
- [[概念/case-studies/2026-09-15-multicluster-network-partition.md|2026-09-15-multicluster-network-partition]]
- [[概念/case-studies/2026-09-20-服务端口名不一致导致istio-mtls握手失败.md|2026-09-20-服务端口名不一致导致istio-mtls握手失败]]
- [[概念/case-studies/2026-10-05-节点内核参数不一致导致sysctl配置冲突.md|2026-10-05-节点内核参数不一致导致sysctl配置冲突]]
- [[概念/case-studies/2026-10-15-pod-disruption-budget阻止节点维护排空.md|2026-10-15-pod-disruption-budget阻止节点维护排空]]
- [[概念/case-studies/2026-10-25-secret未更新导致rolling-update新旧版本配置不一致.md|2026-10-25-secret未更新导致rolling-update新旧版本配置不一致]]
- [[概念/case-studies/README.md|README]]

## 相关链接

- [[MOC]] — 主索引
- [[_insights]] — 仓库洞察

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
