---
title: CRI-O (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- cri-o
- prometheus
- grafana
- containerd
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CRI-O 是什么
- 如何 CRI-O
trigger_keywords:
- CRI-O
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CRI-O

> **CNCF 状态**: Graduated (Kubernetes 子项目) | **类别**: Container Runtime | **主要语言**: Go

## 概述

CRI-O 是 Kubernetes 原生的轻量级容器运行时，专为 Kubernetes 设计，实现 CRI（Container Runtime Interface）规范。与 containerd 相比，CRI-O 更专注于 Kubernetes 场景，不包含非 K8s 功能（如 Docker 兼容层）。它由 Red Hat 主导开发，是 OpenShift 的默认容器运行时。CRI-O 支持 OCI 镜像格式，底层使用 runc 或 crun 作为低层运行时。

## 核心特性

- **Kubernetes 原生**: 仅实现 CRI 接口，无额外抽象层，更轻量
- **OCI 兼容**: 完全支持 OCI 镜像和运行时规范
- **多运行时支持**: runc（默认）、crun（C 语言实现，更快）、Kata Containers（硬件虚拟化）
- **镜像管理**: 支持多 Registry、镜像拉取策略、签名验证
- **安全特性**: SELinux、AppArmor、Seccomp、User Namespace 支持
- **资源管理**: cgroup v1/v2 支持，CPU/内存/IO 限制

## 架构

CRI-O 采用分层架构。最上层是 CRI gRPC 服务（ImageService + RuntimeService），接收 kubelet 的容器生命周期请求。中间层是存储（containers/storage）和镜像（containers/image）库，管理镜像拉取、解压和层管理。底层通过 OCI Runtime（runc/crun）创建和运行容器进程。CRI-O 使用 conmon 监控容器进程，管理日志和 OOM 通知。网络通过 CNI 插件配置，存储通过 CSI 挂载。

## Kubernetes 集成

CRI-O 作为 kubelet 的容器运行时后端。kubelet 通过 Unix Socket（/var/run/crio/crio.sock）与 CRI-O 通信。配置通过 /etc/crio/crio.conf 和 /etc/crio/crio.conf.d/ 管理。kubelet 配置 `--container-runtime-endpoint=unix:///var/run/crio/crio.sock` 即可使用 CRI-O。CRI-O 与 Kubernetes 版本严格对应（如 CRI-O 1.29 对应 K8s 1.29）。

## 生产使用场景

1. **OpenShift 集群**: Red Hat OpenShift 的默认容器运行时
2. **安全敏感环境**: 配合 Kata Containers 提供硬件级隔离
3. **轻量级节点**: 无 Docker 依赖，减少攻击面和资源占用
4. **边缘计算**: 资源受限环境下的轻量运行时选择

## 安装与配置

```bash
# 安装 CRI-O (以 Ubuntu 为例)
export VERSION=1.29
curl -fsSL https://pkgs.k8s.io/addons:/cri-o:/prerelease:/main/deb/Release.key | gpg --dearmor -o /etc/apt/keyrings/cri-o-apt-keyring.gpg
echo "deb [signed-by=/etc/apt/keyrings/cri-o-apt-keyring.gpg] https://pkgs.k8s.io/addons:/cri-o:/prerelease:/main/deb/ /" > /etc/apt/sources.list.d/cri-o.list
apt-get update && apt-get install -y cri-o

# 启动 CRI-O
systemctl enable crio --now

# 配置 kubelet 使用 CRI-O
# /var/lib/kubelet/config.yaml:
# containerRuntimeEndpoint: unix:///var/run/crio/crio.sock
```

```toml
# /etc/crio/crio.conf 关键配置
[crio.runtime]
runtime_path = ""
runtime_type = "oci"
default_runtime = "runc"

[crio.runtime.runtimes.runc]
runtime_path = ""
runtime_type = "oci"
runtime_root = "/run/runc"

[crio.runtime.runtimes.crun]
runtime_path = "/usr/bin/crun"
runtime_type = "oci"

[crio.image]
registries = ["docker.io", "quay.io", "registry.k8s.io"]
pause_image = "registry.k8s.io/pause:3.9"

[crio.network]
network_dir = "/etc/cni/net.d/"
plugin_dirs = ["/opt/cni/bin/"]
```

## 运维操作

```bash
# 🟢 查看 CRI-O 状态
systemctl status crio
crio-status info

# 🟢 查看运行容器和 Pod
crictl ps
crictl pods
crictl images

# 🟢 查看容器日志和详细信息
crictl logs <container-id>
crictl inspect <container-id>
crictl stats

# 🟡 重启 CRI-O（影响所有容器）
systemctl restart crio

# 🟡 清理未使用镜像
crictl rmi --prune

# 🔴 停止 CRI-O（所有容器停止）
systemctl stop crio
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| kubelet 无法连接 CRI-O | Socket 不存在/服务未启动 | `systemctl status crio` | 启动 CRI-O 服务 |
| 容器启动失败 | OCI Runtime 错误/cgroup 问题 | `crictl inspect <id> \| jq .status` | 检查 runc/crun 和 cgroup 配置 |
| 镜像拉取失败 | Registry 不可达/认证失败 | `crictl pull <image> --debug` | 检查网络和 Registry 凭据 |
| CRI-O 启动失败 | 配置文件语法错误 | `journalctl -u crio --no-pager` | 检查 crio.conf 语法 |
| 容器 OOM | 内存限制过低/泄漏 | `crictl inspect <id> \| jq .info.runtimeSpec.linux.resources` | 调整内存限制或排查泄漏 |

```
排查流程：
├─ CRI-O 服务异常
│  ├─ systemctl status crio 检查服务状态
│  ├─ journalctl -u crio 查看日志
│  └─ crio-status info 检查内部状态
├─ 容器运行问题
│  ├─ crictl ps -a 查看容器状态
│  ├─ crictl logs 查看容器日志
│  └─ crictl inspect 检查配置
└─ 镜像问题
   ├─ crictl images 查看本地镜像
   ├─ 检查 Registry 连接和认证
   └─ 检查磁盘空间
```

## 生产案例

### 案例 1：从 Docker 迁移到 CRI-O

- **场景**: 企业 K8s 集群需要从 dockershim 迁移（K8s 1.24 移除 dockershim）
- **排查**: 评估 CRI-O vs containerd，选择 CRI-O 因为与 OpenShift 一致
- **方案**: 滚动替换节点运行时，先 cordon + drain，再安装 CRI-O + 重新加入
- **效果**: 节点资源占用减少 15%，攻击面缩小（无 Docker daemon）

### 案例 2：Kata Containers 硬件隔离

- **场景**: 多租户集群需要更强容器隔离，防止容器逃逸
- **排查**: 评估 gVisor vs Kata，选择 Kata 因为兼容性更好
- **方案**: CRI-O 配置 Kata RuntimeClass，敏感工作负载使用硬件虚拟化隔离
- **效果**: 容器逃逸风险归零，性能开销 <5%

## 替代方案对比

| 维度 | CRI-O | containerd | Docker (Moby) | Kata+crio |
|------|-------|------------|---------------|------------|
| 专注度 | 仅 K8s | 通用 | 通用 | 安全隔离 |
| 资源占用 | 最低 | 低 | 高 | 中 |
| 安全隔离 | 标准 | 标准 | 标准 | 硬件级 |
| 生态支持 | OpenShift | 最广泛 | 最广泛 | 特殊场景 |
| 适用场景 | K8s 专用 | 通用容器 | 开发环境 | 多租户 |

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[22-概念/15-运行时与系统/container-runtime-comparison.md|container-runtime-comparison]]
- [[22-概念/04-存储/storage-model.md|storage-model]]

## Related

- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubescape]] — Kubescape
- [[cedar]] — Cedar
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cri-o
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.28
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.29
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.22
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.32
- RELEASE-NOTES-1.26
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.27
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.23
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.33
- RELEASE-NOTES-1.24
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.34
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.30
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.21
- RELEASE-NOTES-1.31
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.25
- RELEASE-NOTES-1.11
- [[37-归档/release-notes/core-deps/cri-o/RELEASE-NOTES-1.35.md|RELEASE-NOTES-1.35]]
- troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[22-概念/01-核心架构/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[22-概念/15-运行时与系统/docker-architecture.md|Docker Architecture and Container Runtime]] — Cross-reference
- [[22-概念/08-可靠性与运维/node-lifecycle-management.md|节点生命周期管理]] — Cross-reference
- [[26-技能/03-节点/node/诊断排障/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[23-实体/15-参考与索引/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[23-实体/02-K8s核心组件/container-runtime.md|Container Runtime]] — Cross-reference
- [[21-生态参考/03-领域索引/node-index.md|Node 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
