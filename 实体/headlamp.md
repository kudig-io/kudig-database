---
title: Headlamp (entities)
description: '## 概述'
summary: 'Headlamp 是一个现代化的 Kubernetes Web UI，提供直观的集群管理界面。它可以作为桌面应用、Web 应用或集群内应用运行，支持插件扩展系统，允许用户自定义功能。Headlamp 注重用户体验，提供清晰的资源视图和操作界面。'
category: entities
tags:
- k8s
- cncf
- platform
- headlamp
- prometheus
- grafana
- envoy
- flux
- ingress
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Headlamp 是什么
- 如何 Headlamp
trigger_keywords:
- Headlamp
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Headlamp

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: TypeScript, Go

## 概述

Headlamp 是由 Kinvolk（现 Microsoft）开发的现代化 Kubernetes Web UI，2021 年加入 CNCF Sandbox。它提供直观的集群管理界面，可作为桌面应用（Electron）、Web 应用或集群内应用运行。Headlamp 支持插件扩展系统，允许用户自定义功能。与 Lens 等工具不同，Headlamp 是完全开源的 Web 应用，注重轻量级和可扩展性。

## 核心特性

- **多平台部署**: 桌面应用（Electron）、集群内 Web 应用、本地 Web 服务
- **多集群管理**: 单一界面连接和管理多个 Kubernetes 集群
- **插件系统**: TypeScript/JavaScript 插件扩展 UI 和功能
- **实时更新**: 资源状态通过 WebSocket 实时刷新
- **RBAC 感知**: 基于用户 ServiceAccount 权限显示可用操作
- **YAML 编辑器**: 内置带语法高亮和校验的 YAML 编辑器

## 架构

Headlamp 采用前后端分离架构。前端使用 React + TypeScript 构建，提供资源浏览、详情查看和操作界面。后端使用 Go 实现，作为 Kubernetes API 的反向代理和认证中间件。在集群内部署模式中，后端以 Deployment 运行，使用 ServiceAccount 连接 API Server。桌面模式下，前端直接通过 kubeconfig 连接集群（无需后端）。插件系统允许在运行时动态加载 React 组件和菜单项，通过后端 API 注入插件功能。

## Kubernetes 集成

Headlamp 通过标准 Kubernetes API 交互。使用 ServiceAccount Token 或 OIDC Token 认证。RBAC 权限决定用户在 UI 中能看到的资源和能执行的操作——无权限的操作自动隐藏。支持所有标准 Kubernetes 资源类型和 CRD 的浏览和编辑。Ingress 模式暴露 Web UI 供团队共享使用。通过 ConfigMap 管理集群列表和插件配置。

## 生产使用场景

1. **运维 Dashboard**: 为 SRE 团队提供统一的集群管理界面
2. **开发自服务**: 开发者通过 Web UI 自助管理自己的命名空间资源
3. **多集群管理**: 统一界面管理开发/测试/生产多个集群
4. **自定义面板**: 通过插件集成内部工具和 CRD 管理界面

## 安装与配置

```bash
# Helm 部署到集群
helm repo add headlamp https://headlamp-k8s.github.io/headlamp/
helm install headlamp headlamp/headlamp -n headlamp --create-namespace
# 创建 ServiceAccount 和 Token
kubectl create serviceaccount headlamp-admin -n headlamp
kubectl create clusterrolebinding headlamp-admin --clusterrole=cluster-admin --serviceaccount=headlamp:headlamp-admin
kubectl create token headlamp-admin -n headlamp
# 访问 UI
kubectl port-forward svc/headlamp -n headlamp 8080:80
# 或桌面应用
brew install --cask headlamp
```

```yaml
# Ingress 暴露 Headlamp
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: headlamp
  namespace: headlamp
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  tls:
  - hosts:
    - headlamp.example.com
    secretName: headlamp-tls
  rules:
  - host: headlamp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: headlamp
            port:
              number: 80
```

```bash
# 插件安装
# 将插件放入 /plugins 目录或通过 ConfigMap 挂载
kubectl create configmap headlamp-plugins \
  --from-file=./my-plugin/ \
  -n headlamp
```

## 运维操作

```bash
# 🟢 查看 Headlamp 状态
kubectl get pods -n headlamp
kubectl logs -n headlamp -l app.kubernetes.io/name=headlamp --tail=50

# 🟢 检查集群连接
kubectl get configmap headlamp -n headlamp -o yaml

# 🟡 重启 Headlamp
kubectl rollout restart deployment/headlamp -n headlamp

# 🟡 更新 Token
kubectl create token headlamp-admin -n headlamp --duration=8760h

# 🔴 卸载 Headlamp
helm uninstall headlamp -n headlamp
kubectl delete namespace headlamp
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 无法登录 | Token 过期/无效 | 重新创建 Token | `kubectl create token` 生成新 Token |
| 资源不显示 | RBAC 权限不足 | 检查 ServiceAccount 权限 | 调整 ClusterRole 权限 |
| 插件加载失败 | 插件格式错误/版本不兼容 | 检查浏览器 Console | 检查插件 API 版本兼容性 |
| 集群连接失败 | kubeconfig 无效/网络不通 | 检查 Headlamp 日志 | 更新集群配置或检查网络 |
| WebSocket 断开 | Ingress 超时配置 | 检查 Nginx 代理配置 | 增加 proxy-read-timeout |

```
排查流程：
├─ 登录问题
│  ├─ 重新生成 ServiceAccount Token
│  ├─ 检查 RBAC 权限
│  └─ 检查 OIDC 配置（如使用）
├─ 资源显示问题
│  ├─ 检查用户权限是否足够
│  ├─ 确认集群连接正常
│  └─ 检查 CRD 是否已注册
└─ 插件问题
   ├─ 检查插件目录挂载
   └─ 查看浏览器 Console 错误
```

## 生产案例

### 案例 1：开发团队自助服务门户

- **场景**: 开发者频繁请求运维团队执行 kubectl 操作，运维成为瓶颈
- **排查**: 统计发现 80% 的请求是简单的查看日志、重启 Pod 等操作
- **方案**: 部署 Headlamp + RBAC 限制开发者只能操作自己的 Namespace
- **效果**: 运维工单减少 70%，开发者自助完成日常操作

### 案例 2：多集群统一管理界面

- **场景**: 5 个集群（dev/staging/prod×3）需要统一管理视图
- **排查**: 团队频繁切换 kubeconfig context，操作错误风险高
- **方案**: Headlamp 多集群配置，单一界面切换集群，RBAC 限制生产操作
- **效果**: 集群切换时间归零，误操作风险降低 90%

## 替代方案对比

| 维度 | Headlamp | Lens | KubeDash | Rancher Dashboard |
|------|----------|------|----------|-------------------|
| 开源 | ✅ Apache 2.0 | ⚠️ 部分 | ✅ | ✅ |
| 部署方式 | Web/桌面 | 桌面 | Web | Web |
| 插件系统 | ✅ TS/JS | ✅ | ❌ | ✅ |
| 多集群 | ✅ | ✅ | ❌ | ✅ |
| 适用场景 | 轻量 Web UI | 功能丰富 | 简单查看 | 企业级 |

## 架构定位

在 CNCF 生态中，Headlamp 属于 **Platform / UI** 类别，是 Kubernetes Web UI 的轻量级开源选择。它与 kubectl、Helm、FluxCD 等工具链配合使用。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[flux]]
- [[deployment]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[zot]] — zot
- [[openfga]] — OpenFGA
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- headlamp
- [[实体/opengitops.md|OpenGitOps]]
- [[实体/kubeclipper.md|KubeClipper]]
- [[实体/cozystack.md|Cozystack]]
- [[实体/kube-rs.md|kube-rs]]
- [[实体/kagent.md|Kagent]]
- [[实体/openchoreo.md|OpenChoreo]]
- [[实体/holmesgpt.md|HolmesGPT]]
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
