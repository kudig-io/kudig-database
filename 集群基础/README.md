---
title: Cluster Fundamentals
description: 集群基础知识域 — K8s 架构总览、设计原则、控制平面、API 版本、kubectl、升级路径、性能调优
summary: 集群基础知识域入口，涵盖控制平面组件、etcd、调度器、API Server、集群升级策略、性能基准与调优
category: domain
tags:
- kubernetes
- architecture
- control-plane
- etcd
- scheduler
- upgrade
- performance
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- 所有工程师
- 架构师
- SRE
estimated_read_time: 5min
---
# 集群基础 Cluster Fundamentals

> Kubernetes 集群架构总览、设计原则、控制平面、API 版本、kubectl 与升级路径。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[集群基础/架构总览/README.md\|架构总览/]] | 架构 | Master/Worker、组件交互、高可用拓扑 |
| [[集群基础/控制平面/README.md\|控制平面/]] | 控制平面 | API Server/etcd/Scheduler/Controller Manager |
| [[集群基础/设计原则/README.md\|设计原则/]] | 原则 | 声明式、不可变基础设施、水平扩展、自愈 |
| [[集群基础/API版本/README.md\|API版本/]] | API | Alpha/Beta/GA、版本协商、废弃策略 |
| [[集群基础/kubectl/README.md\|kubectl/]] | kubectl | 常用命令、输出格式、插件、调试技巧 |
| [[集群基础/升级路径/README.md\|升级路径/]] | 升级 | 滚动升级、版本偏差策略、升级检查清单 |
| [[集群基础/性能调优/README.md\|性能调优/]] | 性能 | API Server 调优、etcd 性能、调度器延迟 |

## 跨域导航

- [[AI基础设施/README.md|AI基础设施]]
- [[专项技术/README.md|专项技术]]
- [[云厂商/README.md|云厂商]]
- [[发布变更/README.md|发布变更]]
- [[可观测性/README.md|可观测性]]
- [[可靠性/README.md|可靠性]]
- [[存储/README.md|存储]]
- [[安全/README.md|安全]]
- [[容器运行时/README.md|容器运行时]]
- [[工作负载/README.md|工作负载]]
- [[平台工程/README.md|平台工程]]
- [[应用模式/README.md|应用模式]]
- [[故障诊断/README.md|故障诊断]]
- [[数据库中间件/README.md|数据库中间件]]
- [[清单模式/README.md|清单模式]]
- [[生产运维/README.md|生产运维]]
- [[生态参考/README.md|生态参考]]
- [[系统基础/README.md|系统基础]]
- [[网络/README.md|网络]]
