---
title: Reliability Engineering
description: 可靠性工程知识域 — SRE 实践、混沌工程、灾难恢复、容量规划、事后复盘、性能测试、GameDay
summary: 可靠性工程知识域入口，涵盖 SLO/SLI/Error Budget、Chaos Mesh/Litmus 混沌工程、备份恢复、DR Playbook、GameDay 演练
category: domain
tags:
- sre
- slo
- disaster-recovery
- chaos-engineering
- postmortem
- capacity-planning
- gameday
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- SRE
- 所有工程师
estimated_read_time: 5min
---
# 可靠性 Reliability

> 备份恢复、灾难恢复、容量规划、混沌工程、事后复盘、SRE 实践与性能测试。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[wiki/12-可靠性/06-SRE实践/index.md\|SRE实践/]] | SRE 方法论 | SLO/SLI/Error Budget/Toil 消除/On-Call |
| [[wiki/12-可靠性/04-混沌工程/index.md\|混沌工程/]] | 混沌实验 | Chaos Mesh/Litmus/实验设计/GameDay |
| [[wiki/12-可靠性/02-灾难恢复/index.md\|灾难恢复/]] | DR | DR Playbook/RTO-RPO/切换演练 |
| [[wiki/12-可靠性/01-备份恢复/index.md\|备份恢复/]] | 备份 | Velero/etcd 快照/跨集群备份 |
| [[wiki/12-可靠性/03-容量规划/index.md\|容量规划/]] | 容量 | 资源预测/右调优/成本平衡 |
| [[wiki/12-可靠性/05-事后复盘/index.md\|事后复盘/]] | 复盘 | Blameless Postmortem/模板/改进跟踪 |
| [[wiki/12-可靠性/07-性能测试/index.md\|性能测试/]] | 压测 | 负载测试/基准测试/性能回归 |

## 跨域导航

- [[wiki/15-AI基础设施/README.md|AI基础设施]]
- [[wiki/16-专项技术/README.md|专项技术]]
- [[wiki/18-云厂商/README.md|云厂商]]
- [[wiki/11-发布变更/README.md|发布变更]]
- [[wiki/09-可观测性/README.md|可观测性]]
- [[wiki/06-存储/README.md|存储]]
- [[wiki/08-安全/README.md|安全]]
- [[wiki/14-容器运行时/README.md|容器运行时]]
- [[wiki/02-工作负载/README.md|工作负载]]
- [[wiki/10-平台工程/README.md|平台工程]]
- [[wiki/04-应用模式/README.md|应用模式]]
- [[wiki/19-故障诊断/README.md|故障诊断]]
- [[wiki/07-数据库中间件/README.md|数据库中间件]]
- [[wiki/03-清单模式/README.md|清单模式]]
- [[wiki/13-生产运维/README.md|生产运维]]
- [[wiki/21-生态参考/README.md|生态参考]]
- [[wiki/17-系统基础/README.md|系统基础]]
- [[wiki/05-网络/README.md|网络]]
- [[wiki/01-集群基础/README.md|集群基础]]

## Related

- [[wiki/02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[wiki/02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[wiki/02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[wiki/02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[wiki/09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[wiki/12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[wiki/19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[skills/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[wiki/26-标签/06-AI与专项/research|research]]
