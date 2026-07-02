---
title: K8s 多云架构术语参考
description: '# K8s 多云架构术语参考'
summary: '本页汇总了 **多云架构** 领域的 3 个 Kubernetes 术语定义与概念说明。'
category: references
tags:
- k8s
- dictionary
- multi-cloud
- etcd
- apiserver
- scheduler
- prometheus
- grafana
- istio
- calico
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 多云架构术语参考 是什么
- 如何 K8s 多云架构术语参考
trigger_keywords:
- K8s
- 多云架构术语参考
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- cni-basics
- etcd-basics
---



# K8s 多云架构术语参考

本页汇总了 **多云架构** 领域的 3 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[entities/k8s-cloud-provider-comparison.md|k8s-cloud-provider-comparison]] | [[alicloud-ack-overview]] | [[aws-eks-overview]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **边缘计算与轻量级 Kubernetes** | Edge Computing And K3S | **边缘计算（Edge Computing）** 将数据处理能力下沉到靠近数据源或终端用户的边缘节点，以降低延迟、减少带宽消耗并满足数据主权要求 |
| **10 - 多云混合云运维手册** | Multi Cloud Operations | title: 10 - 多云混合云运维手册
description: '# 10 - 多云混合云运维手册'
category: dictionary
ta... |
| **太空计算（Spaceborne Computing）** | Spaceborne Computing | **太空计算（Spaceborne Computing）** 是将边缘计算和人工智能能力部署到卫星、空间站和其他太空平台上的新兴领域 |

---

### 边缘计算与轻量级 Kubernetes

**边缘计算（Edge Computing）** 将数据处理能力下沉到靠近数据源或终端用户的边缘节点，以降低延迟、减少带宽消耗并满足数据主权要求。Kubernetes 正在从传统数据中心向工厂、零售门店、自动驾驶车辆和卫星等边缘场景扩展。**K3s、MicroK8s、k0s** 等轻量级 Kubernetes 发行版，以及 **WebAssembly** 运行时，正在推动这一趋势。2026 年，已有超过半数的企业在边缘生产环境中运行 Kubernetes 工作负载。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/multi-cloud/edge-computing-and-k3s.md`）*

---

### 10 - 多云混合云运维手册

title: 10 - 多云混合云运维手册
description: '# 10 - 多云混合云运维手册'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- scheduler
- prometheus
- grafana
- istio
- calico
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 多云混合云运维手册 是什么
- 如何 多云混合云运维手册
trigger_keywords:
- 多云混合云运维手册
- dictionary
title_en: Multi Cloud Operations
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/multi-cloud/multi-cloud-operations.md`）*

---

### 太空计算（Spaceborne Computing）

**太空计算（Spaceborne Computing）** 是将边缘计算和人工智能能力部署到卫星、空间站和其他太空平台上的新兴领域。随着低轨卫星（LEO, Low Earth Orbit）星座（如 Starlink、OneWeb、中国星网）的爆发式增长，以及在轨数据处理需求的激增，Kubernetes 和容器化技术正在进入太空。2026 年，NASA、ESA 以及多家商业航天公司已经开始在卫星上运行轻量级 Kubernetes 发行版（如 K3s），用于**星上 AI 推理、地球观测数据处理、自主导航和故障检测**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/multi-cloud/spaceborne-computing.md`）*

---

## 相关页面

- [[entities/k8s-cloud-provider-comparison.md|k8s-cloud-provider-comparison]]
- [[alicloud-ack-overview]]
- [[aws-eks-overview]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/multi-cloud/edge-computing-and-k3s.md`
- `domain-17-system-foundation/topic-dictionary/multi-cloud/multi-cloud-operations.md`
- `domain-17-system-foundation/topic-dictionary/multi-cloud/spaceborne-computing.md`

## Related

- [[k3s]] — k3s 轻量级 Kubernetes
- [[istio]] — Istio
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
