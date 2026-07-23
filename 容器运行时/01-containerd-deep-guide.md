---
title: containerd 深度指南
summary: containerd 深度指南：containerd 是 Kubernetes 1.24+ 的标准容器运行时。理解其架构和工作原理，对于排查镜像拉取失败、容器启动异常等问题至关重要。
category: 容器运行时
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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| containerd 服务无法启动 | 配置文件语法错误 | `containerd config dump` | 校验 TOML 语法，恢复备份 |
| 容器无法创建 | shim 二进制缺失 | `ls /usr/bin/containerd-shim*` | 重新安装 containerd |
| 镜像拉取超时 | registry 不可达 | `crictl pull <image> -v` | 检查网络和 registry 配置 |
| 节点 NotReady | containerd 进程崩溃 | `systemctl status containerd` | 查看日志并重启服务 |
| 磁盘空间不足 | 镜像/容器未清理 | `du -sh /var/lib/containerd/` | 执行 GC 清理 |
| cgroup 错误 | 驱动不匹配 | `containerd config dump | grep -i cgroup` | 统一使用 systemd cgroup |
| 内存泄漏 | 已知 bug | `journalctl -u containerd` | 升级到已修复版本 |
| 插件加载失败 | 版本不兼容 | `journalctl -u containerd | grep plugin` | 检查插件版本兼容性 |

## containerd 架构详解

```text
containerd 架构层次：

containerd daemon
  ├── gRPC API (CRI + 原生)
  ├── 插件系统
  │    ├── CRI 插件 (K8s 集成)
  │    ├── Snapshotter (存储驱动)
  │    ├── Runtime (运行时管理)
  │    └── Diff (层差异计算)
  ├── Content Store (镜像层存储)
  ├── Metadata Store (BoltDB)
  └── Shim 管理
       └── containerd-shim-runc-v2 (per-container)
            └── runc (OCI runtime)
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 版本 | 使用稳定版（1.7.x/2.0.x） | 避免使用 RC 版本 |
| 配置 | 纳入配置管理工具 | Ansible/Salt 版本化 |
| 监控 | 监控进程资源 + CRI 延迟 | CPU/内存/FD/P99 |
| 日志 | info 级别 + logrotate | 避免磁盘占满 |
| 升级 | 滚动升级，先测试后生产 | 验证 shim 兼容性 |
| 备份 | 定期备份 config.toml | 便于回滚 |
| 安全 | 限制 containerd socket 权限 | 仅 root 可访问 |
| GC | 配置合理的镜像 GC 策略 | 与磁盘容量匹配 |

## 相关工具

| 工具 | 用途 | 使用方式 |
|------|------|----------|
| ctr | containerd 原生 CLI | `ctr images ls` |
| crictl | CRI 调试 | `crictl ps/pods/info` |
| nerdctl | Docker 兼容 CLI | `nerdctl run/build` |
| containerd config | 配置管理 | `containerd config dump/default` |
| journalctl | 日志查看 | `journalctl -u containerd -f` |
| systemctl | 服务管理 | `systemctl status containerd` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| containerd 和 Docker 的关系？ | Docker 底层使用 containerd，K8s 1.24+ 直接对接 containerd |
| 如何查看 containerd 版本？ | `containerd --version` |
| 如何重启不影响运行容器？ | containerd 重启不会杀容器（shim 独立） |
| 如何配置镜像加速？ | 配置 /etc/containerd/certs.d/ 目录 |
| 如何查看运行中的容器？ | `crictl ps` 或 `ctr -n k8s.io containers ls` |
| 如何清理无用镜像？ | `crictl rmi --prune` |
| containerd 2.0 有何变化？ | 新配置格式 v3，插件路径变化 |
| 如何启用多运行时？ | 配置多个 runtime_type（runc/kata/gvisor） |

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| 镜像拉取慢 | 配置 mirror | registry.mirrors 指向内网 |
| 并发拉取 | 调整并发度 | max_concurrent_downloads |
| 磁盘 I/O 高 | 独立数据盘 | /var/lib/containerd 挂载 SSD |
| 内存占用高 | 调整 GC | 配置合理的 image gc 阈值 |
| 启动慢 | 减少插件 | 禁用不需要的插件 |
| 日志过大 | 调整级别 | info + logrotate |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| containerd_process_cpu | CPU 使用率 | > 80% |
| containerd_process_memory | 内存使用 | > 1GB |
| containerd_process_fds | 文件描述符 | > 10000 |
| containerd_restart_count | 重启次数 | > 0 |
| cri_request_duration | CRI 调用延迟 | P99 > 5s |
| image_pull_duration | 镜像拉取耗时 | P99 > 120s |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| socket 权限 | 600，仅 root | 避免未授权访问 |
| 数据目录 | 独立挂载，权限 700 | 避免信息泄露 |
| 日志 | 不含敏感信息 | 避免记录 token |
| 插件 | 最小化启用 | 减小攻击面 |
| 升级 | 及时更新 | 修复已知漏洞 |
| 审计 | 记录关键操作 | 便于安全审计 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| Docker | containerd | 安装 containerd→配置 CRI→移除 dockershim |
| containerd 1.x | 2.0 | 升级二进制→调整配置 v3 |
| 单运行时 | 多运行时 | 配置多个 runtime_type |
| 无监控 | 有监控 | 接入 Prometheus metrics |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 版本 | `containerd --version` | 稳定版 |
| 服务 | `systemctl status containerd` | active |
| 配置 | `containerd config dump` | 无错误 |
| CRI | `crictl info` | 返回 JSON |
| 镜像 | `crictl pull <image>` | 成功 |
| 容器 | `crictl ps` | 正常 |
| 日志 | `journalctl -u containerd` | 无错误 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| containerd 1.0 | 2017 | 初始稳定版 |
| containerd 1.4 | 2020 | shim v2 默认 |
| containerd 1.7 | 2023 | CRI v1 强制 |
| containerd 2.0 | 2024 | 新插件架构，config v3 |

## 架构对比

```text
containerd 在 K8s 中的位置：

kubectl → API Server → kubelet
  └── CRI gRPC → containerd
       ├── CRI Plugin (K8s 集成)
       ├── Shim (per-container)
       │    └── runc/kata/runsc
       ├── Snapshotter (overlayfs)
       └── Content Store (镜像层)

与 Docker 对比：
Docker: kubelet → dockershim → dockerd → containerd → runc
containerd: kubelet → CRI → containerd → runc (更短)
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 小集群 | 默认 | 足够 |
| 大集群 | 独立数据盘 + SSD | 性能 |
| 高密度 | 监控 shim 进程 | 避免泄漏 |
| 多运行时 | 多个 runtime 配置 | 隔离 |

- [[故障诊断/技能体系/skill-set/k8s-image-pull/SKILL.md|image-pull-troubleshooting]] — 镜像拉取问题排查
- [[故障诊断/技能体系/skill-set/k8s-image-pull/SKILL.md|k8s-image-pull]] — K8s  镜像拉取机制
- [[容器运行时/01-containerd-deep-guide.md|container-runtime-security]] — 容器运行时安全
- [[容器运行时/01-containerd-deep-guide.md|docker-migration-containerd]] — Docker 迁移至 containerd 指南

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
