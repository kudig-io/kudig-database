---
title: Networking & Traffic
description: 网络知识域 — K8s 网络核心、CNI 对比、Service Mesh、API Gateway、eBPF、Terway
summary: 网络知识域入口，涵盖 Pod 网络模型、CNI 插件对比、Istio/Envoy 服务网格、Ingress/Gateway API、eBPF 网络编程、Terway 阿里云网络
category: domain
tags:
- networking
- cni
- service-mesh
- istio
- cilium
- api-gateway
- ebpf
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- 所有工程师
- 网络工程师
- SRE
estimated_read_time: 10min
---
# 网络 Networking

> K8s 网络核心、基础协议、服务网格、API 网关、eBPF 与 Terway。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[网络/K8s网络核心/README.md\|K8s网络核心/]] | Pod 网络 | CNI 规范、Service/DNS、NetworkPolicy |
| [[网络/网络基础/README.md\|网络基础/]] | 协议 | TCP/IP、负载均衡、DNS 原理 |
| [[网络/服务网格/README.md\|服务网格/]] | Mesh | Istio/Envoy/Linkerd、流量管理、mTLS |
| [[网络/API网关/README.md\|API网关/]] | 网关 | Ingress Controller、Gateway API、Kong/APISIX |
| [[网络/eBPF/README.md\|eBPF/]] | eBPF | Cilium、XDP、网络可观测、性能优化 |
| [[网络/Terway/README.md\|Terway/]] | Terway | 阿里云 ENI、Trunk、固定 IP |
| [[网络/附件/README.md\|附件/]] | 附件 | 网络拓扑图、配置模板、参考数据 |

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
- [[集群基础/README.md|集群基础]]
