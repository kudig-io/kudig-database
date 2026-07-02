---
title: containerd 容器生命周期与 Pod 管理
summary: containerd 容器生命周期与 Pod 管理：Created → Running → (Succeeded|Failed|Unknown)
  ↓ Paused → Running
category: synthesis
tags:
- synthesis
- containerd
- pod
- k8s
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd 容器生命周期与 Pod 管理

> containerd 如何实现 Kubernetes Pod 的容器生命周期管理。

## 生命周期映射

| Kubernetes 概念 | containerd 实现 |
|----------------|----------------|
| Pod Sandbox | pause 容器 |
| Init Container | 按序启动的容器 |
| Main Container | 主业务容器 |
| Container Probe | 由 kubelet 调用 |

## 容器状态转换

```
Created → Running → (Succeeded|Failed|Unknown)
         ↓
      Paused → Running
```

## 关键操作

1. **创建**: kubelet → CRI RunPodSandbox → containerd NewContainer
2. **启动**: kubelet → CRI StartContainer → containerd Start
3. **停止**: kubelet → CRI StopContainer → containerd Kill (SIGTERM → SIGKILL)
4. **删除**: kubelet → CRI RemoveContainer → containerd Delete

## 调试探参

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 containerd 管理的容器
crictl ps -a
# 查看 Pod sandbox
crictl pods
# 查看容器日志
crictl logs <container-id>
```
## 相关页面

- [[containerd]] — containerd 详细文档
- [[pod-lifecycle]] — Pod 生命周期
- [[kubelet]] — 节点代理


<!-- risk-assessed -->
