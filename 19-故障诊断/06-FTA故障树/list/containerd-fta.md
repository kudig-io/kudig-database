---
title: containerd 异常故障树分析 (skills)
description: '- **范围**：containerd 进程、CRI socket、镜像拉取与存储、containerd-shim、snapshotter、网络命名空间、资源与磁盘压力。'
summary: '- **范围**：containerd 进程、CRI socket、镜像拉取与存储、containerd-shim、snapshotter、网络命名空间、资源与磁盘压力。'
category: skills
tags:
- k8s
- fta
- troubleshooting
- containerd
- cri
- container-runtime
- image
- snapshotter
- shim
- disk
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 异常故障树分析 是什么
- 如何 containerd 异常故障树分析
trigger_keywords:
- containerd
- 异常故障树分析
prerequisites:
- kubectl-basics
- containerd-basics
- linux-basics
fta_id: FTA-CONTAINERD-001
component: containerd
severity: high
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'

---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd 异常故障树分析

<!-- condition: Pod 处于 ImagePullBackOff / ContainerCreating / ErrImagePull，或 crictl ps/info 超时、kubelet 报 PLEG 不健康 -->

## 适用范围与说明
- **目标**：覆盖 containerd 异常导致 Pod 启动失败、镜像拉取失败、运行时卡死等关键成因与路径。
- **范围**：containerd 进程、CRI socket、镜像拉取与存储、containerd-shim、snapshotter、网络命名空间、资源与磁盘压力。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: containerd 异常导致 Pod 启动失败/运行时卡死]
  OR0{{OR}}
  TE --> OR0

  OR0 --> PROC[containerd 进程异常]
  OR0 --> SOCK[CRI socket 异常]
  OR0 --> IMAGE[镜像拉取异常]
  OR0 --> STORE[镜像与存储异常]
  OR0 --> SHIM[containerd-shim 异常]
  OR0 --> CNI_NET[Pod 网络命名空间异常]
  OR0 --> RES[节点资源压力]

  %% containerd 进程异常分支
  PROC_OR{{OR}}
  PROC --> PROC_OR
  PROC_OR --> PROC1[服务未运行]
  PROC_OR --> PROC2[进程崩溃/OOM]
  PROC_OR --> PROC3[配置加载失败]

  PROC1_OR{{OR}}
  PROC1 --> PROC1_OR
  PROC1_OR --> PROC1A[systemd 未启动]
  PROC1_OR --> PROC1B[节点重启后未自启]

  PROC2_OR{{OR}}
  PROC2 --> PROC2_OR
  PROC2_OR --> PROC2A[内存 limit 过低]
  PROC2_OR --> PROC2B[异常 panic]

  PROC3_OR{{OR}}
  PROC3 --> PROC3_OR
  PROC3_OR --> PROC3A[/etc/containerd/config.toml 语法错误]
  PROC3_OR --> PROC3B[sandbox_image 不可达]

  %% CRI socket 异常分支
  SOCK_OR{{OR}}
  SOCK --> SOCK_OR
  SOCK_OR --> SOCK1[socket 文件缺失]
  SOCK_OR --> SOCK2[socket 权限错误]
  SOCK_OR --> SOCK3[CRI 响应超时]

  SOCK3_OR{{OR}}
  SOCK3 --> SOCK3_OR
  SOCK3_OR --> SOCK3A[containerd 主循环阻塞]
  SOCK3_OR --> SOCK3B[大量并发请求]

  %% 镜像拉取异常分支
  IMAGE_OR{{OR}}
  IMAGE --> IMAGE_OR
  IMAGE_OR --> IMAGE1[镜像不存在]
  IMAGE_OR --> IMAGE2[镜像仓库认证失败]
  IMAGE_OR --> IMAGE3[网络/镜像仓库不可达]
  IMAGE_OR --> IMAGE4[镜像仓库限流]

  IMAGE2_OR{{OR}}
  IMAGE2 --> IMAGE2_OR
  IMAGE2_OR --> IMAGE2A[imagePullSecrets 缺失]
  IMAGE2_OR --> IMAGE2B[registry mirror 配置错误]

  %% 镜像与存储异常分支
  STORE_OR{{OR}}
  STORE --> STORE_OR
  STORE_OR --> STORE1[磁盘空间不足]
  STORE_OR --> STORE2[inode 耗尽]
  STORE_OR --> STORE3[snapshotter 异常]
  STORE_OR --> STORE4[OverlayFS 挂载失败]

  STORE1_AND{{AND}}
  STORE1 --> STORE1_AND
  STORE1_AND --> STORE1A[镜像/层未清理]
  STORE1_AND --> STORE1B[容器日志占满磁盘]

  %% containerd-shim 异常分支
  SHIM_OR{{OR}}
  SHIM --> SHIM_OR
  SHIM_OR --> SHIM1[shim 进程泄漏]
  SHIM_OR --> SHIM2[shim 与 runc 通信失败]
  SHIM_OR --> SHIM3[容器退出后 shim 未回收]

  %% Pod 网络命名空间异常分支
  CNI_NET_OR{{OR}}
  CNI_NET --> CNI_NET_OR
  CNI_NET_OR --> CNI_NET1[CNI 插件调用失败]
  CNI_NET_OR --> CNI_NET2[pause 容器启动失败]
  CNI_NET_OR --> CNI_NET3[网络命名空间创建失败]

  %% 节点资源压力分支
  RES_OR{{OR}}
  RES --> RES_OR
  RES_OR --> RES1[PID 耗尽]
  RES_OR --> RES2[文件描述符耗尽]
  RES_OR --> RES3[磁盘 IO 极高]
```

---

## 生产级观测与证据

- **containerd 关键日志关键字**：`failed to pull and unpack image`、`CRI stream server error`、`failed to create shim task`、`snapshotter error`、`no space left on device`。
- **关键指标**：containerd 内存/CPU、镜像数量、snapshot 数量、磁盘使用率、inode 使用率。
- **关键命令**：
  ```bash
  systemctl status containerd
  journalctl -u containerd --since "10 min ago"
  crictl info
  crictl ps -a
  crictl images
  df -h /var/lib/containerd
  df -i /var/lib/containerd
  ```

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[23-实体/03-运行时/containerd.md|containerd]] — containerd
- [[19-故障诊断/04-高级排障/structural-02-node-components/03-container-runtime-troubleshooting.md|容器运行时故障排查指南]]
- [[14-容器运行时/03-containerd-CRI-O/02-containerd-production-operations.md|containerd 生产运维指南]]


<!-- risk-assessed -->
