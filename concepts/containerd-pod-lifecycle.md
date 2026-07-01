---
title: "containerd 容器生命周期与 Pod 管理"
category: synthesis
tags: [synthesis, containerd, pod, k8s]
sources: []
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

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

```bash
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
