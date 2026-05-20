---
title: Domain-10 扩展与自定义 — 开源项目索引
description: '| **Helm** | K8s 包管理器 | Graduated | v3.17.0 | 27k+ | Apache-2.0 |'
category: extensions
tags:
- k8s
- extensions
- crd
- operator
- webhook
- helm
- containerd
- harbor
- statefulset
- gpu
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 开发工程师
- 架构师
estimated_read_time: 5min
intent_queries:
- Domain-10 扩展与自定义 — 开源项目索引 是什么
- 如何 Domain-10 扩展与自定义 — 开源项目索引
- Kubernetes 10 extensions 最佳实践
trigger_keywords:
- Domain-10
- 扩展与自定义
- 开源项目索引
- extensions
cross_refs:
- type: domain
  path: ../domain-9-platform-ops/
  label: '相关知识域: domain-9-platform-ops'
---


# Domain-10 扩展与自定义 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Helm v3.17 / KubeVirt v1.5 / Backstage v1.36

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Helm (CNCF Graduated)](#二helm-cncf-graduated)
- [三、Operator 开发框架](#三operator-开发框架)
- [四、KubeVirt (CNCF Incubating)](#四kubevirt-cncf-incubating)
- [五、Backstage (CNCF Incubating)](#五backstage-cncf-incubating)
- [六、应用交付与高级工作负载](#六应用交付与高级工作负载)
- [七、CRD 扩展与 API 聚合](#七crd-扩展与-api-聚合)
- [八、版本兼容矩阵](#八版本兼容矩阵)
- [九、扩展生态选型](#九扩展生态选型)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Helm** | K8s 包管理器 | Graduated | v3.17.0 | 27k+ | Apache-2.0 |
| **Operator SDK** | Operator 开发框架 | K8s SIG | v1.39.0 | 7k+ | Apache-2.0 |
| **kubebuilder** | K8s API 构建框架 | K8s SIG | v4.5.0 | 8k+ | Apache-2.0 |
| **KubeVirt** | 虚拟机编排 | Incubating | v1.5.0 | 5.5k+ | Apache-2.0 |
| **Backstage** | 开发者门户 | Incubating | v1.36.0 | 28k+ | Apache-2.0 |
| **KubeVela** | OAM 应用交付 | Incubating | v1.10.0 | 6.5k+ | Apache-2.0 |
| **OpenKruise** | 高级工作负载 | Incubating | v1.8.0 | 4.5k+ | Apache-2.0 |
| **kro** | Kube Resource Orchestrator | AWS 开源 | v0.2.0 | 1k+ | Apache-2.0 |
| **kustomize** | 声明式配置定制 | K8s SIG | v5.6.0 | 11k+ | Apache-2.0 |
| **Buildpacks** | 云原生构建标准 | Incubating | v0.36.0 | 5k+ | Apache-2.0 |
| **Carvel** | VMware 应用打包工具集 | VMware | v0.55.0 | 1.5k+ | Apache-2.0 |
| **Helm Dashboard** | Helm UI 管理界面 | Komodor | v1.3.0 | 4k+ | Apache-2.0 |
| **Kubeapps** | K8s 应用仪表板 | VMware | v17.0.0 | 5k+ | Apache-2.0 |
| **DevSpace** | K8s 开发工作流 | Loft | v6.3.0 | 4k+ | Apache-2.0 |
| **Tilt** | 本地 K8s 开发 | Tilt.dev | v0.33.0 | 7k+ | Apache-2.0 |
| **Okteto** | 云端开发环境 | Okteto | v3.5.0 | 3k+ | Apache-2.0 |
| **DevPod** | 开源 Codespaces 替代 | Loft | v0.6.0 | 8k+ | MPL-2.0 |
| **mirrord** | 本地代码接入集群 | MetalBear | v3.0.0 | 5k+ | MIT |
| **telepresence** | 本地开发流量拦截 | Ambassador | v2.22.0 | 6k+ | Apache-2.0 |
| **Kratix** | K8s-native 平台框架 | Syntasso | v0.12.0 | 1k+ | Apache-2.0 |
| **Nitric** | 云原生开发框架 | Nitric | v1.0.0 | 2k+ | Apache-2.0 |
| **Score** | 工作负载规范 | Humanitec | v0.16.0 | 1k+ | Apache-2.0 |

---

## 二、Helm (CNCF Graduated)

### 2.1 核心特性

```yaml
# Helm 3 架构
- Client-only (无 Tiller，直接使用 K8s API)
- Chart: 打包的 K8s 应用模板
- Release: Chart 在集群中的实例
- Repository: Chart 分发仓库
- Library Charts: 可复用模板库
```

### 2.2 Helm 4 前瞻

- **启动时间**: 2024.11 KubeCon NA
- **预计发布**: 2025.11 KubeCon NA
- **主要改进方向**: 架构债务清理、性能优化、OCI 支持增强

### 2.3 OCI Registry 支持

```bash
# 将 Chart 推送到 OCI 兼容仓库 (Harbor/ACR/ECR)
helm push mychart-1.0.0.tgz oci://harbor.example.com/charts

# 从 OCI 安装
helm install myapp oci://harbor.example.com/charts/mychart --version 1.0.0
```

**GitHub**: https://github.com/helm/helm
**文档**: https://helm.sh/docs/

---

## 三、Operator 开发框架

### 3.1 Operator SDK

```yaml
# 支持工作流
- Go-based Operator (推荐生产使用)
- Ansible-based Operator (基础设施运维友好)
- Helm-based Operator (简单无状态应用)
```

**核心工具链**
- `operator-sdk init`: 初始化项目
- `operator-sdk create api`: 生成 CRD 与控制器骨架
- `make bundle`: 生成 OLM (Operator Lifecycle Manager) 包

**GitHub**: https://github.com/operator-framework/operator-sdk

### 3.2 kubebuilder

- K8s SIG 官方推荐的 API 构建框架
- 与 Controller-runtime 深度集成
- 代码生成、测试脚手架、API 验证

**GitHub**: https://github.com/kubernetes-sigs/kubebuilder

### 3.3 kro (Kube Resource Orchestrator)

- **来源**: AWS 于 KubeCon NA 2024 发布
- **目标**: 简化 CRD 与自定义 API 的使用
- **核心**: ResourceGraphDefinition (RGD) 声明式编排多资源
- **非 AWS 锁定**: 适用于任意 K8s 集群

```yaml
# RGD 示例
apiVersion: kro.run/v1alpha1
kind: ResourceGraphDefinition
metadata:
  name: webapp
spec:
  resources:
  - id: deployment
    template:
      apiVersion: apps/v1
      kind: Deployment
      ...
  - id: service
    template:
      apiVersion: v1
      kind: Service
      ...
```

**GitHub**: https://github.com/kro-run/kro

---

## 四、KubeVirt (CNCF Incubating)

### 4.1 虚拟机在 K8s 上运行

```yaml
# 核心能力
- 将 VM 作为 K8s Pod 运行
- 与 CNI/CNI 网络集成
- 与 CSI 存储集成
- Live Migration (热迁移)
- 云初始化 (cloud-init)
```

### 4.2 v1.4/v1.5 重大特性

- **网络热插拔 (Network Hotplug)**: 运行中 VM 添加/移除网卡
- **通用实例类型**: 预定义 VM 规格模板
- **NUMA 拓扑支持**: 性能敏感工作负载
- **GPU 分配 GA**: 直通 GPU 到虚拟机

```yaml
# VirtualMachine 示例
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: fedora-vm
spec:
  running: true
  template:
    spec:
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - name: containerdisk
            disk:
              bus: virtio
      volumes:
      - name: containerdisk
        containerDisk:
          image: kubevirt/fedora-cloud-container-disk-demo
```

**GitHub**: https://github.com/kubevirt/kubevirt
**文档**: https://kubevirt.io/user-guide/

---

## 五、Backstage (CNCF Incubating)

### 5.1 开发者门户平台

```yaml
# 核心功能
- Software Catalog: 服务、组件、资源统一目录
- Software Templates: 自助式服务创建 (Scaffolding)
- TechDocs: 文档即代码 (MkDocs 集成)
- 插件生态: 200+ 社区插件
- 搜索聚合、API 文档、成本洞察
```

### 5.2 2025-2026 增长数据

- 贡献量翻倍增长
- Spotify 开源，被 thousands of 组织采用
- 主要插件: Kubernetes, GitHub, PagerDuty, Argo CD, Cost Insights

**GitHub**: https://github.com/backstage/backstage
**文档**: https://backstage.io/docs/

---

## 六、应用交付与高级工作负载

### 6.1 KubeVela (CNCF Incubating)

- OAM (Open Application Model) 规范实现
- 平台工程师定义能力，开发者声明式使用
- 多集群应用交付
- 工作流引擎集成

**GitHub**: https://github.com/kubevela/kubevela

### 6.2 OpenKruise (CNCF Incubating)

- 阿里云开源，高级工作负载扩展
- **CloneSet**: 增强版 StatefulSet (原地升级、优先级打散)
- **Advanced StatefulSet**: 灰度发布、优先级策略
- **SidecarSet**: 独立管理 Sidecar 生命周期
- **PodUnavailableBudget**: Pod 级别不可用预算

**GitHub**: https://github.com/openkruise/kruise

---

## 七、CRD 扩展与 API 聚合

### 7.1 kustomize

- K8s 原生配置定制工具
- `kubectl apply -k` 内置支持
- base + overlay 模式
- 生成器与转换器 (ConfigMapGenerator, Patch)

### 7.2 API Aggregation Layer

- 扩展 K8s API Server
- metrics-server、custom metrics adapter 等实现基础

---

## 八、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Helm v3.17 | ✅ | ✅ | ✅ | Helm 4 开发中 |
| Operator SDK v1.39 | ✅ | ✅ | ✅ | controller-runtime v0.20 |
| kubebuilder v4.5 | ✅ | ✅ | ✅ | 推荐新项目使用 |
| KubeVirt v1.5 | ✅ | ✅ | ⚠️ 验证中 | 需容器运行时支持 |
| Backstage v1.36 | ✅ | ✅ | ✅ | 独立部署 |
| KubeVela v1.10 | ✅ | ✅ | ✅ | 多集群场景 |
| OpenKruise v1.8 | ✅ | ✅ | ✅ | 原地升级兼容 |
| kro v0.2 | ✅ | ✅ | ✅ | 早期版本 |

---

## 九、扩展生态选型

```
┌─────────────────────────────────────────────────────────────┐
│                  K8s 扩展技术选型指南                          │
└─────────────────────────────────────────────────────────────┘

应用打包与部署
  ├── Helm ──► 有状态/无状态应用的标准化打包
  ├── kustomize ──► 同构环境的多配置变体
  └── Carvel ──► VMware 生态的声明式应用生命周期

自定义资源与控制器
  ├── kubebuilder ──► Go 语言原生 CRD + Controller
  ├── Operator SDK ──► Ansible/Helm/Go 多语言 Operator
  └── kro ──► 无需编写代码的资源编排

高级工作负载
  ├── OpenKruise ──► 需要原地升级、Sidecar 独立管理
  └── KubeVirt ──► 传统 VM 上 K8s 的渐进式迁移

开发者体验
  ├── Backstage ──► 构建内部开发者平台 (IDP)
  └── KubeVela ──► 平台工程的多集群应用交付

构建与交付
  ├── Buildpacks ──► 源代码到镜像的自动化
  └── kaniko / ko ──► CI/CD 流水线内的安全构建
```

---

## 参考链接

- [Helm 官方文档](https://helm.sh/docs/)
- [Operator SDK 文档](https://sdk.operatorframework.io/)
- [kubebuilder 文档](https://book.kubebuilder.io/)
- [KubeVirt 用户指南](https://kubevirt.io/user-guide/)
- [Backstage 文档](https://backstage.io/docs/)
- [OpenKruise 文档](https://openkruise.io/docs/)
- [kro GitHub](https://github.com/kro-run/kro)
