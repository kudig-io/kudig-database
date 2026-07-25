---
title: Dragonfly P2P 分发
description: Dragonfly 是阿里巴巴开源的 CNCF 孵化项目，基于 P2P（点对点）技术加速容器镜像和文件的分发，解决大规模集群中镜像拉取的性能瓶颈问题。...
summary: Dragonfly 是阿里巴巴开源的 CNCF 孵化项目，基于 P2P（点对点）技术加速容器镜像和文件的分发，解决大规模集群中镜像拉取的性能瓶颈问题。...
category: dictionary
tags:
- k8s
- glossary
- tooling
- distribution
- p2p
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dragonfly P2P 分发 是什么
- Dragonfly 详解
trigger_keywords:
- Dragonfly P2P 分发
- Dragonfly
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Dragonfly P2P 分发（Dragonfly）

## 概述

Dragonfly 是阿里巴巴开源的 CNCF 孵化项目，基于 P2P（点对点）技术加速容器镜像和文件的分发，解决大规模集群中镜像拉取的性能瓶颈问题。

## 核心概念/原理

- **P2P 加速**：节点间共享已下载的层，减少 Registry 压力
- **大规模验证**：阿里巴巴/蚂蚁集团生产环境使用
- **CNCF 孵化**：阿里开源
- **透明代理**：无需修改容器运行时配置

## 关键机制或特性

- DFDaemon 节点代理
- Scheduler 调度 P2P 节点
- Manager 集群管理
- 支持 Docker/Containerd/Podman/Nydus
- 预热（Preheating）机制
- 分片下载和断点续传
- 与 Harbor 集成

## 使用场景与最佳实践

- 大规模集群的镜像拉取加速
- CI/CD 构建产物的快速分发
- 跨区域的镜像同步加速
- Registry 带宽瓶颈的缓解
- K8s 扩容时的镜像分发优化

## 参考链接

- https://d7y.io/
- https://github.com/dragonflyoss/Dragonfly2

## Related

- [[17-系统基础/06-知识字典/tooling/distribution.md|Distribution]]
- [[17-系统基础/06-知识字典/tooling/harbor.md|Harbor]]
- [[17-系统基础/06-知识字典/fundamentals/containerd.md|containerd]]


<!-- risk-assessed -->
