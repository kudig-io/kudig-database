---
title: Konveyor (entities)
description: '## 概述'
summary: 'Konveyor 是一个应用现代化平台，帮助组织将传统应用（如 Java EE、Spring）迁移和重构到 Kubernetes 平台。它提供应用清单管理、依赖分析、迁移评估、自动化代码重构等能力。Konveyor 通过 AI 辅助分析识别迁移障碍，生成迁移路径建议，并提供 IDE 插件帮助开发者自动化完成代码变更。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- konveyor
- crd
- operator
- kserve
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Konveyor 是什么
- 如何 Konveyor
trigger_keywords:
- Konveyor
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Konveyor

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go, TypeScript

## 概述

Konveyor 是一个 CNCF 沙箱项目，由 Red Hat 主导，是一个应用现代化和迁移工具集。它帮助组织将传统应用（Java EE、虚拟机应用）迁移到 Kubernetes 和云原生架构。Konveyor 包含多个工具：Tackle（迁移项目管理）、Windup（代码分析）、Move2Kube（部署配置迁移）、Crane（K8s 集群间迁移）等。项目通过自动化分析和迁移建议，大幅降低应用现代化的工作量。

## Key Features（核心能力）

- **应用评估**：Tackle 提供应用现代化就绪度评估和迁移计划管理
- **代码分析**：Windup 分析应用源码，识别迁移到容器/K8s 的障碍和风险
- **Move2Kube**：自动将应用部署配置（如 docker-compose）转换为 K8s YAML
- **Crane**：K8s 集群间的资源和数据迁移工具
- **迁移路径建议**：基于分析结果推荐最佳迁移路径
- **多语言支持**：支持 Java、Python、Go、Node.js 等语言应用分析

## 架构与工作原理

Konveyor 是一个工具集而非单一系统：Tackle 提供项目管理 Web UI 和 API；Windup 通过静态代码分析识别迁移风险和依赖；Move2Kube 通过解析现有部署配置（如 docker-compose、Cloud Foundry manifest）生成 K8s 部署清单；Crane 通过 K8s API 迁移命名空间级别的资源。各工具可独立使用或通过 Tackle 统一管理。

## K8s 集成

Konveyor 本身可在 Kubernetes 上部署，通过 Operator 管理各组件。迁移工具通过 K8s API 连接到目标集群，执行资源迁移和配置转换。Move2Kube 生成的 K8s YAML 可直接 kubectl apply。Crane 支持跨集群的命名空间迁移，包括 PVC 数据迁移。

## 生产用例

- **应用现代化**：将传统 Java EE 应用迁移到 K8s 容器化架构
- **VM 到容器迁移**：将虚拟机应用容器化到 K8s
- **集群迁移**：跨 K8s 集群的资源和数据迁移
- **迁移评估**：评估应用组合的云原生就绪度

## 安装与配置

```bash
# 🟢 安装 Move2Kube CLI
pip3 install move2kube
# 或
curl -L https://github.com/konveyor/move2kube/releases/latest/download/move2kube-linux-amd64 -o move2kube
chmod +x move2kube && mv move2kube /usr/local/bin/

# 🟢 验证安装
move2kube version

# 🟢 安装 Tackle Operator
kubectl apply -f https://raw.githubusercontent.com/konveyor/tackle-operator/main/install/konveyor-operator.yaml

# 🟢 验证 Tackle 安装
kubectl get pods -n konveyor-tackle

# 🟢 访问 Tackle UI
kubectl port-forward svc/tackle-ui 8080:8080 -n konveyor-tackle
# 浏览器访问 http://localhost:8080
```

### Move2Kube 使用示例

```bash
# 🟢 从 docker-compose 生成 K8s YAML
move2kube translate -s ./docker-compose/ -o ./k8s-output/

# 🟢 交互式转换
move2kube translate -s ./source-app/ --interactive

# 🟢 从 Cloud Foundry manifest 转换
move2kube translate -s ./cf-manifest/ -o ./k8s-output/

# 🟢 查看生成的文件
ls -la ./k8s-output/
kubectl apply -f ./k8s-output/ --dry-run=client
```

### Crane 集群迁移示例

```bash
# 🟢 安装 Crane CLI
go install github.com/konveyor/crane/cmd/crane@latest

# 🟢 导出命名空间资源
crane export --namespace my-app --output ./export/

# 🟢 导入到目标集群
crane import --input ./export/ --target-context target-cluster

# 🟢 迁移 PVC 数据
crane migrate --namespace my-app --source-context src --target-context dst
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Tackle 组件
kubectl get pods -n konveyor-tackle

# 🟢 查看 Tackle 日志
kubectl logs -n konveyor-tackle -l app=tackle-ui --tail=50

# 🟢 查看分析任务
kubectl get tasks -n konveyor-tackle

# 🟢 Move2Kube 分析现有应用
move2kube collect -s ./my-app/ -o ./analysis/

# 🟢 查看分析报告
cat ./analysis/report.html

# 🟡 删除 Tackle
kubectl delete -f https://raw.githubusercontent.com/konveyor/tackle-operator/main/install/konveyor-operator.yaml
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Move2Kube 转换失败 | 源配置格式不支持 | 查看转换日志 | 检查源文件格式 |
| Tackle UI 无法访问 | Service 未暴露 | `kubectl get svc -n konveyor-tackle` | 配置 port-forward 或 Ingress |
| 分析结果不完整 | 代码解析失败 | 查看 Windup 日志 | 检查代码结构和依赖 |
| Crane 迁移失败 | 集群连接问题 | `crane status` | 检查 kubeconfig 和网络 |

### 排查流程

```
1. kubectl get pods -n konveyor-tackle → 确认组件状态
2. kubectl logs -l app=tackle-ui → 查看 UI 日志
3. 检查 Move2Kube 转换输出目录
4. 验证目标集群连接和权限
```

## 生产案例

### 案例1: Java EE 应用现代化
- **场景**: 100+ Java EE 应用运行在 WebLogic，需迁移到 K8s
- **方案**: Konveyor Tackle 评估 + Windup 分析 + Move2Kube 转换
- **效果**: 迁移工作量评估从数月缩短至数天，80% 应用可自动转换

### 案例2: 跨集群迁移
- **场景**: 从旧 K8s 集群迁移到新集群，包含 50+ 命名空间
- **方案**: Crane 自动化迁移资源和 PVC 数据
- **效果**: 迁移时间从数周缩短至数小时，零数据丢失

## 对比替代方案

| 维度 | Konveyor | 手动迁移 | AWS Migration Hub | Azure Migrate |
|------|----------|---------|------------------|---------------|
| 自动化分析 | 支持 | 无 | 支持 | 支持 |
| 代码转换 | 支持 | 手动 | 有限 | 有限 |
| 厂商中立 | 是 | N/A | 否 (AWS) | 否 (Azure) |
| 开源 | 是 | N/A | 否 | 否 |
| K8s 原生 | 支持 | N/A | 有限 | 有限 |

## 检查清单

- [ ] 应用组合已完成现代化评估
- [ ] Move2Kube 转换结果已验证
- [ ] 目标集群资源已准备
- [ ] 迁移计划包含回滚方案
- [ ] 关键应用经过充分测试
- [ ] 迁移后监控已配置

## Related

- [[network-service-mesh]] — [[23-实体/04-网络/network-service-mesh.md|Network Service Mesh (NSM)]]]Service Mesh）|Service Mesh]] (NSM)
- [[kserve]] — KServe
- [[meshery]] — Meshery
- [[knative]] — Knative
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- konveyor
- [[23-实体/08-交付与制品/shipwright.md|Shipwright]]
- [[23-实体/15-参考与索引/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference


<!-- risk-assessed -->
