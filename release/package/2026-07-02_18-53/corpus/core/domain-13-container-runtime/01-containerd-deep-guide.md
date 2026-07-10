---
title: containerd 深度指南
summary: containerd 深度指南：containerd 是 Kubernetes 1.24+ 的标准容器运行时。理解其架构和工作原理，对于排查镜像拉取失败、容器启动异常等问题至关重要。
category: domain-13
tags:
- domain-13
- containerd
- 容器运行时
- CRI
- 镜像管理
- RuntimeClass
- visibility/public
tier: core
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd 深度指南

## 概述

containerd 是 Kubernetes 1.24+ 的标准容器运行时。理解其架构和工作原理，对于排查镜像拉取失败、容器启动异常等问题至关重要。

## containerd 架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
Client (ctr/kubectl)
    ↓ CRI / containerd API
containerd (守护进程)
    ↓
containerd-shim (每个容器一个)
    ↓
runc (OCI 运行时)
    ↓
Linux Namespace + Cgroups
```
### 核心组件

| 组件 | 职责 | 对应进程 |
|---|---|---|
| containerd | 镜像管理、容器生命周期管理 | `containerd` |
| containerd-shim | 隔离容器与 containerd，允许 containerd 重启 | `containerd-shim-runc-v2` |
| runc | 创建和运行 OCI 标准容器 | `runc` |
| snapshotter | 管理镜像层和容器文件系统 | 插件实现 |

> containerd-shim 的设计使 containerd 守护进程可以升级或重启，而不影响已运行的容器。

## Docker vs containerd

| 特性 | Docker | containerd |
|---|---|---|
| 定位 | 完整容器平台（CLI + Daemon） | 轻量级运行时 |
| CRI 支持 | 需 dockershim 桥接 | 原生支持 |
| K8s 支持 | 1.24 后移除 dockershim | 推荐标准运行时 |
| 镜像管理 | docker image / docker pull | ctr image / crictl image |

Kubernetes 1.24 正式移除 dockershim，containerd 成为唯一推荐运行时。

## containerd 镜像管理

### 常用命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ctr -n k8s.io images list      # 查看镜像列表
crictl pull nginx:latest       # 拉取镜像
crictl inspecti nginx:latest   # 查看镜像详情
```
### 命名空间

containerd 使用命名空间隔离镜像：
- `k8s.io`：Kubernetes 默认命名空间
- `moby`：Docker 使用的命名空间（如同时安装）

### Snapshotter（快照器）

负责管理容器文件系统的分层结构：

| Snapshotter | 适用场景 | 特点 |
|---|---|---|
| overlayfs | 默认 | 性能好 |
| stargz | 延迟拉取 | 镜像按需加载，启动快 |
| nydus | 龙蜥加速 | 与 Dragonfly 配合大规模分发 |

## 运行时类（RuntimeClass）

RuntimeClass 允许在同一个集群中使用不同的容器运行时：

### gVisor

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
```

- 用户态内核，提供额外隔离
- 适用于不受信任的工作负载
- 有一定性能开销

### Kata Containers

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
```

- 轻量级虚拟机，每个 Pod 一个 VM
- 比 gVisor 性能更好
- 需要硬件虚拟化支持

## 镜像拉取优化

### 镜像缓存

- 本地缓存：containerd 默认会缓存已拉取的镜像层
- 节点预热：大规模扩容前，提前在节点上拉取镜像
- DaemonSet 预拉取：使用 DaemonSet 在节点加入时预拉取常用镜像

### 私有镜像仓库认证

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: regcred
type: kubernetes.io/dockerconfigjson
data:
  .dockerconfigjson: <base64-encoded>
```

- 将 `imagePullSecrets` 绑定到 ServiceAccount，避免每个 Pod 配置
- 定期刷新仓库 Token，避免认证过期

### 并行拉取

- Kubelet 默认最多 5 个并发拉取
- 大规模节点启动时配合镜像预热
- 使用本地 Harbor/Nexus 缓存加速

## 远程顾问诊断要点

### ImagePullBackOff 排查流程

1. **确认镜像存在**：`crictl images | grep <image-name>`
2. **检查认证**：`kubectl get pod my-pod -o yaml | grep imagePullSecrets`
3. **检查网络**：`curl -v https://registry.example.com/v2/`
4. **查看日志**：`journalctl -u containerd -f`

### 常见错误对照表

| 错误信息 | 根因 | 解决方案 |
|---|---|---|
| `not found` | 镜像不存在或标签错误 | 确认镜像仓库地址和标签 |
| `unauthorized` | 认证失败 | 检查 imagePullSecrets 和仓库凭证 |
| `timeout` | 网络不通或仓库慢 | 检查网络连通性，配置镜像加速 |
| `ImagePullBackOff` | 多次重试失败 | 排查后删除 Pod 重新触发 |
| `InvalidImageName` | 镜像名称格式错误 | 检查镜像名是否包含非法字符 |

> 远程顾问应要求客户提供 `crictl` 命令的输出和 containerd 日志，而非仅凭 `kubectl describe` 的摘要信息做判断。

## 相关链接

- [[domain-10-troubleshooting-diagnostics/技能体系/skill-set/k8s-image-pull/SKILL.md|image-pull-troubleshooting]] — 镜像拉取问题排查
- [[domain-10-troubleshooting-diagnostics/技能体系/skill-set/k8s-image-pull/SKILL.md|k8s-image-pull]] — K8s 镜像拉取机制
- [[domain-13-container-runtime/01-containerd-deep-guide.md|container-runtime-security]] — 容器运行时安全
- [[domain-13-container-runtime/01-containerd-deep-guide.md|docker-migration-containerd]] — Docker 迁移至 containerd 指南

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
