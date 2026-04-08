# 速查卡 (Cheat Sheet)

> 覆盖 Kubernetes 生产运维全场景的快速参考卡片集

## 概述

本目录包含 **9 张** 精心编写的技术速查卡，面向生产环境运维工程师和开发者，提供命令、语法、配置的快速参考。每张速查卡均经过生产验证，包含真实场景示例。

## 速查卡索引

| # | 速查卡 | 内容覆盖 | 适用版本 | 大小 |
|:---:|:---|:---|:---|:---:|
| 1 | [Kubernetes 速查卡](./k8s.md) | kubectl 命令、集群管理、Pod 操作、网络、存储、RBAC、排障 | v1.25-v1.32 | 37KB |
| 2 | [Linux 速查卡](./linux.md) | 系统管理、进程、网络、存储、安全、Shell 脚本 | RHEL 7-9, Ubuntu 20-24 | 44KB |
| 3 | [Go 语言速查卡](./go.md) | 语法、并发、网络、数据库、测试、性能优化 | Go 1.20-1.22 | 49KB |
| 4 | [Docker/Containerd 速查卡](./docker.md) | 容器生命周期、镜像管理、网络、存储、Compose、ctr | Docker 20.10+, containerd 1.6+ | 11KB |
| 5 | [PromQL 速查卡](./promql.md) | 指标查询、聚合函数、Kubernetes 监控、告警规则 | Prometheus 2.40+ | 11KB |
| 6 | [网络诊断速查卡](./networking.md) | DNS 诊断、TCP 调试、HTTP 测试、抓包分析、K8s 网络 | TCP/IP | 14KB |
| 7 | [Git 速查表](./git.md) | 日常操作、分支管理、撤销操作、故障排查 | Git 2.30+ | 12KB |
| 8 | [SQL 速查表](./sql.md) | 查询语法、表操作、索引优化、数据库管理 | MySQL 8.0, PostgreSQL 14 | 20KB |
| 9 | [TLS/PKI 速查卡](./tls-pki.md) | 证书格式、OpenSSL 命令、证书链、K8s 证书管理、监控脚本 | x509, TLS 1.2/1.3 | 11KB |

## 使用场景

### 日常运维速查
```bash
# 快速查找 kubectl 命令
open topic-cheat-sheet/k8s.md

# Linux 性能排查命令
open topic-cheat-sheet/linux.md
```

### 导入 AI 知识库
- **NotebookLM**: 导入整个目录作为速查参考源
- **IMA / 豆包**: 适合日常术语和命令查询
- **RAG 应用**: 作为快速检索层，配合 domain-* 深度内容

### 打印/离线使用
每张速查卡设计为可独立使用的完整参考文档，适合打印或导出 PDF。

## 与其他模块的关系

| 速查卡 | 深度知识来源 | 故障排查 |
|:---|:---|:---|
| k8s.md | domain-1 ~ domain-12 | domain-12, topic-fta |
| linux.md | domain-14 | domain-12/35 |
| docker.md | domain-13 | domain-12/08 |
| promql.md | domain-8, domain-20 | domain-12/30 |
| networking.md | domain-5, domain-15 | domain-12/25-26 |
| tls-pki.md | domain-7 | domain-12/13 |
| git.md | domain-23 | - |
| sql.md | domain-28 | - |
| go.md | domain-2 (源码阅读) | - |

## 贡献指南

新增速查卡请遵循以下规范：
- 文件名使用小写连字符格式（如 `helm.md`）
- 每个条目包含：命令/语法 + 简要说明 + 示例
- 标注适用版本范围
- 优先收录生产环境高频操作
