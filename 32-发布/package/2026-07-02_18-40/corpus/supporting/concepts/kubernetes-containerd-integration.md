---
title: Kubernetes 与 containerd 集成深度分析
summary: Kubernetes 与 containerd 集成深度分析：Kubernetes 通过 CRI（Container Runtime Interface）与
  containerd 通信。kubelet 作为节点代理，调用 containerd 的 CRI 插件来管理容器生命周期。
category: synthesis
tags:
- synthesis
- k8s
- containerd
- cluster
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 与 containerd 集成深度分析

> 本文综合分析 Kubernetes 与 containerd 的集成架构、运维要点和最佳实践。

## 核心关系

Kubernetes 通过 CRI（Container Runtime Interface）与 containerd 通信。kubelet 作为节点代理，调用 containerd 的 CRI 插件来管理容器生命周期。

## 架构层次

```
kubelet → CRI → containerd → runc → Linux kernel
```

- [[kubelet]]: 节点代理，发起容器操作请求
- [[containerd]]: 容器运行时，管理镜像和容器
- CRI: 标准化接口，解耦 kubelet 与具体运行时

## 运维要点

1. **版本兼容性**: Kubernetes 版本与 containerd 版本有严格的兼容矩阵
2. **镜像管理**: containerd 使用 `ctr` 和 `crictl` 两种命令行工具
3. **日志路径**: 容器日志位于 `/var/log/pods/`，由 kubelet 管理
4. **资源限制**: 通过 CRI 传递 cgroup 配置到 containerd

## 常见问题

- 容器启动失败: 检查 containerd 日志 (`journalctl -u containerd`)
- 镜像拉取超时: 配置镜像加速器和超时参数
- 节点 NotReady: 排查 containerd 进程健康状态

## 相关页面

- [[kubernetes]] — 集群整体架构
- [[containerd]] — containerd 详细文档
- [[pod-lifecycle]] — Pod 生命周期管理
- [[kubelet]] — 节点代理


<!-- risk-assessed -->
