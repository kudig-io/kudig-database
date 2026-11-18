---
title: Nacos (entities)
description: '## 概述'
summary: 'Nacos 是阿里巴巴开源的服务发现与配置管理平台，为微服务提供动态服务注册、配置管理和流量管理能力。'
category: entities
tags:
- k8s
- microservices
- service-discovery
- configuration
- nacos
- spring-cloud
- dubbo
- java
tier: supporting
created: '2026-11-17'
last_updated: 2026-11
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- Nacos 是什么
- Nacos 架构与注册配置原理
- 如何在 Kubernetes 上部署 Nacos
- Nacos 服务注册不上如何排查
trigger_keywords:
- Nacos
- 服务注册
- 配置中心
- spring.cloud.nacos
prerequisites:
- kubectl-basics
- microservices-basics
- spring-boot-basics
- yaml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Nacos

> **项目**: 阿里巴巴开源（Apache License 2.0） | **类别**: 服务注册与配置管理 | **主要语言**: Java

## 概述

Nacos（Dynamic Naming and Configuration Service）由阿里巴巴于 2018 年开源，是微服务架构中的基础中间件，提供两大核心能力：**服务注册与发现**（Naming 模块）和**动态配置管理**（Config 模块）。在国内微服务生态中，Nacos 是 Spring Cloud Alibaba 与 Dubbo 体系的默认注册中心与配置中心；在云原生场景中，Nacos 也常作为 Kubernetes 之外的跨集群/跨语言服务发现层，自 3.0 起提供 MCP Registry，支持 AI Agent 场景的服务与工具注册。Nacos 设计目标是"易于构建云原生应用和微服务平台"，一个集群同时承担注册中心与配置中心两个角色，减少组件数量。

## 核心能力

- **服务注册与发现**: 临时实例（心跳上报）与持久实例（主动查询）两种模式，支持命名空间与分组隔离
- **动态配置管理**: 配置以 DataId + Group 为键管理，客户端长连接订阅变更，秒级推送热更新
- **流量管理（不推荐新增使用）**: 基于权重的路由与简单流量管理能力（推荐由网关/Mesh 承担）
- **元数据管理**: 服务与实例元数据（labels）可用于灰度路由与路由规则匹配
- **多语言客户端**: Java（spring-cloud-alibaba、dubbo）、Go、Python、Node.js、C++ 等
- **鉴权与安全**: 控制台与 OpenAPI 鉴权（username/password + accessToken），2.x 起支持鉴权开关
- **运维友好**: OpenAPI 丰富，控制台可视化管理配置与服务列表

## 架构

Nacos 服务端由三个模块构成：

- **Naming 模块**: 管理服务实例表。集群内使用 **Distro 协议**（AP，各节点分片持有数据并相互同步），客户端注册的临时实例数据不要求强一致
- **Config 模块**: 管理配置数据。使用 **Raft 协议**（CP，Nacos 自研 Java Raft 实现）保证配置写入一致性，客户端本地磁盘存有快照（snapshot）兜底
- **控制台（Console）**: 3.0 起控制台与服务端分离部署、前后端分离，可独立升级与扩缩容

客户端与服务端的关键连接（Nacos 2.x 起）：

- **gRPC 长连接**：客户端与服务端建立 gRPC 连接进行注册、心跳与配置订阅，端口规则为 **主端口 + 1000**（默认 8848 → gRPC 端口 9848，客户端 SDK 端口 9849）
- **HTTP 兼容**：1.x 风格的 HTTP OpenAPI 仍然保留，控制台与存量脚本可继续使用

```text
Client (SDK) ──gRPC(9848)──> Naming 集群 (Distro/AP)
      │  └──gRPC(9848)──> Config 集群 (Raft/CP)
      │
Client 本地快照 (snapshot) <── 配置拉取兜底
```

## 关键机制

- **临时实例心跳**: 客户端默认每 5s 上报心跳，服务端 15s 未收到标记不健康，30s 未收到剔除实例
- **配置变更推送**: 配置发布后通过 gRPC 流向订阅了该 DataId 的所有客户端推送，客户端收到后触发 `Refresh` 事件（Spring Cloud 中配合 `@RefreshScope` 重建 Bean）
- **命名隔离三层**: Namespace（环境/集群隔离，默认 public）→ Group（业务线/应用组）→ Service/DataId（服务/配置）
- **容灾**: 客户端配置本地快照，服务端不可用时可从快照读取；注册数据在 Distro 协议下节点故障可恢复
- **持久化**: 配置数据默认内嵌 Derby（单机），生产必须切换外置 **MySQL**； Naming 中持久实例同样入库

## Kubernetes 部署

使用官方 Helm Chart 部署单机模式（试点）或集群模式（生产）：

```bash
# 🟢 添加仓库并查看（只读操作）
helm repo add nacos https://nacos.io.github.io/helm-charts/
helm repo update

# 🟡 安装集群模式 Nacos（创建 nacos 命名空间与 3 节点 StatefulSet）
helm install nacos nacos/nacos -n nacos --create-namespace \
  --set mysql.enabled=true,mysql.auth.rootPassword=ChangeMe,mysql.auth.database=nacos
```

生产要点：

- **至少 3 节点** StatefulSet，反亲和（podAntiAffinity）打散，PVC 使用云盘 StorageClass
- 配置 `MYSQL_SERVICE_*` 环境变量接入外置数据库，禁止使用内嵌 Derby 承载生产配置
- Service 需同时暴露 **8848（HTTP）与 9848（gRPC）**，只开 8848 会导致 2.x 客户端注册失败
- 就绪探针建议用 `GET /nacos/v1/console/health/readiness`（3.x）或 `GET /nacos/v1/console/health/liveness`，避免用首页 HTML 判断

## 运维操作

```bash
# 🟢 查看 Nacos Pod 状态（只读）
kubectl get pod -n nacos -o wide

# 🟢 查询某服务的健康实例列表（只读）
curl -s "http://nacos-nacos.nacos.svc:8848/nacos/v1/ns/instance/list?serviceName=my-service&namespaceId=prod"

# 🟢 查询配置内容（只读，需 accessToken）
curl -s "http://nacos-nacos.nacos.svc:8848/nacos/v1/cs/configs?dataId=app.yaml&group=DEFAULT_GROUP&tenant=prod"

# 🟡 发布/更新配置（修改集群状态，客户端会收到实时推送）
curl -s -X POST "http://nacos-nacos.nacos.svc:8848/nacos/v1/cs/configs" \
  -d "dataId=app.yaml&group=DEFAULT_GROUP&tenant=prod&content=key: value"

# 🟢 查看集群节点列表与选主状态（只读）
curl -s "http://nacos-nacos.nacos.svc:8848/nacos/v1/ns/operator/servers"
```

## 故障排查

- **客户端注册失败 / 连接超时**：优先检查 **9848 gRPC 端口**是否在 Service 与安全组中放通（最常见的 2.x 故障）；其次确认客户端与服务端 Nacos 大版本一致（2.x 客户端不能连 1.x 服务端）
- **服务注册上了但调用方看不到**：核对 namespaceId（`spring.cloud.nacos.discovery.namespace`）与 group 是否一致——跨 namespace 天然不可见
- **配置改了不生效**：① 客户端是否订阅了正确的 DataId+Group；② Spring Cloud 是否用 `@RefreshScope`/配置绑定类；③ 客户端本地快照陈旧时会在服务端恢复后自动重推，可重启客户端强制拉取
- **集群脑裂/写失败**：Config 模块 Raft 少数派无法写入，检查 3 节点间 8848/9848 互通与防火墙；`nacos-server` 日志中搜索 `raft` 相关报错
- **控制台 403**：鉴权开关 `nacos.core.auth.enabled=true` 后需带 accessToken，且客户端 `username/password` 必须配置

## 生产最佳实践

- 集群节点数 3 或 5，跨可用区部署，PVC 与 Pod 亲和性固定盘符
- 配置持久化统一用 MySQL（主备），并定期备份 `config_info` 表
- 开启鉴权，关闭对外暴露的 8848/9848 的公网访问，仅集群内网与 VPN 可达
- 命名空间按环境硬隔离（dev/test/prod），Group 按业务域划分，禁止所有配置挤在 DEFAULT_GROUP
- 监控核心指标：注册实例数、长连接数、config 发布 QPS、raft leader 状态、JVM GC；接入 Prometheus 抓取 `/nacos/actuator/prometheus`（2.x+）
- 客户端与服务端保持同大版本升级，先升服务端（向后兼容窗口）再滚动客户端

## 对比

| 维度 | Nacos | Consul | etcd | Kubernetes Service/CoreDNS |
|---|---|---|---|---|
| 定位 | 注册中心 + 配置中心 | 服务发现 + KV | 分布式 KV | 集群内 DNS/Endpoints |
| 一致性 | Naming AP(Distro) / Config CP(Raft) | CP(Raft) | CP(Raft) | watch Endpoints（最终一致） |
| 配置热更新 | 原生支持推送 | KV watch | watch | 依赖 ConfigMap 挂载/重启或反射器 |
| 中国生态 | Spring Cloud Alibaba / Dubbo 默认 | HashiCorp 生态 | 云原生通用 | K8s 原生 |
| 控制台 | 内置 | Consul UI（开源版有限） | 无 | kubectl/dashboard |

## 参考链接

- 官方文档：https://nacos.io/docs/latest/overview/
- GitHub：https://github.com/alibaba/nacos
- Helm Chart：https://nacos.io/docs/latest/deployment/

## Related

- [[wiki/23-实体/04-网络/dubbo.md|dubbo]]
- [[wiki/23-实体/10-平台与开发工具/spring-cloud-alibaba.md|spring-cloud-alibaba]]
- [[wiki/04-应用模式/02-行业架构/20-microservice-governance-architecture.md|微服务治理架构]]
