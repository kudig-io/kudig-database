---
title: Domain-40 云原生 API 网关 — 开源项目索引
description: '| **Gateway API** | K8s 流量管理新标准 | K8s SIG | v1.2.0 | - | Apache-2.0
  |'
summary: '| **Gateway API** | K8s 流量管理新标准 | K8s SIG | v1.2.0 | - | Apache-2.0 |'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- istio
- cilium
- ingress
- gateway
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-40 云原生 API 网关 — 开源项目索引 是什么
- 如何 Domain-40 云原生 API 网关 — 开源项目索引
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Domain-40
- 云原生
- API
- 网关
- 开源项目索引
- cloud
- native
- api
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-40 云原生 API 网关 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Envoy** | L7 代理与网关标准 | CNCF Graduated | v1.33.0 | 25k+ | Apache-2.0 |
| **Ingress NGINX** | K8s Ingress 控制器 | K8s SIG | v1.12.0 | 17k+ | Apache-2.0 |
| **Emissary-Ingress** | API 网关 | CNCF Incubating | v3.10.0 | 4.5k+ | Apache-2.0 |
| **Contour** | Envoy Ingress | CNCF Incubating | v1.30.0 | 3.5k+ | Apache-2.0 |
| **Envoy Gateway** | 官方 Envoy K8s 网关 | Envoy | v1.3.0 | 2k+ | Apache-2.0 |
| **Kong Gateway** | API 网关 | Kong | v3.9.0 | 41k+ | Apache-2.0 |
| **Kuma** | Envoy 服务网格 + 网关 | Kong | v2.10.0 | 3k+ | Apache-2.0 |
| **Traefik** | 云原生反向代理 | Traefik Labs | v3.3.0 | 52k+ | MIT |
| **HAProxy Ingress** | HAProxy K8s 集成 | 社区 | v0.14.0 | 2k+ | Apache-2.0 |
| **Apache APISIX** | 动态 API 网关 | Apache | v3.11.0 | 14k+ | Apache-2.0 |
| **Tyk** | 开源 API 网关 | Tyk | v5.7.0 | 9k+ | MPL-2.0 |
| **Zuul** | Netflix API 网关 | Netflix | v2.6.0 | 13k+ | Apache-2.0 |
| **Spring Cloud Gateway** | Spring 生态网关 | VMware | v4.3.0 | 6k+ | Apache-2.0 |
| **BFE** | 百度开源网关 | 百度 | v1.8.0 | 6k+ | Apache-2.0 |
| **Gateway API** | K8s 流量管理新标准 | K8s SIG | v1.2.0 | - | Apache-2.0 |
| **Istio Gateway** | 服务网格网关 | CNCF Graduated | v1.29.0 | 36k+ | Apache-2.0 |
| **Cilium Gateway** | eBPF 网关 | CNCF Graduated | v1.17.0 | 21k+ | Apache-2.0 |
| **Solo Gloo** | Envoy 网关 | Solo.io | v1.18.0 | 4k+ | Apache-2.0 |
| **Solo Gloo Mesh** | Istio 多集群管理 | Solo.io | v2.7.0 | 1k+ | Apache-2.0 |
| **AWS App Mesh** | AWS 托管服务网格 | AWS | - | - | 商业 |

---

## 参考链接

- [Gateway API 文档](https://gateway-api.sigs.k8s.io/)
- [Envoy 文档](https://www.envoyproxy.io/docs/)
- [Ingress NGINX 文档](https://kubernetes.github.io/ingress-nginx/)
- [Apache APISIX](https://apisix.apache.org/)
- [Traefik 文档](https://doc.traefik.io/traefik/)

---

## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[05-网络/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践


<!-- risk-assessed -->
