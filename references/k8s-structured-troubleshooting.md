---
title: 结构化排障方法论：配置优先、全组件排障指南
description: '## 配置优先原则'
category: reference
tags:
- k8s
- troubleshooting
- structured-troubleshooting
- configuration-first
- diagnostic
- etcd
- kubelet
- scheduler
- coredns
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 结构化排障方法论：配置优先、全组件排障指南 是什么
- 如何 结构化排障方法论：配置优先、全组件排障指南
- 结构化排障方法论：配置优先、全组件排障指南 故障排查
- 结构化排障方法论：配置优先、全组件排障指南 排障步骤
trigger_keywords:
- 结构化排障方法论：配置优先
- 全组件排障指南
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# 结构化排障方法论

## 配置优先原则

经验统计：**80% 的 K8s 故障源于配置错误**，而非代码 Bug。

排障顺序：
1. **配置检查**：YAML 清单是否正确、资源限制是否合理
2. **事件检查**：`kubectl describe` 查看 Events
3. **日志检查**：`kubectl logs` 查看容器日志
4. **指标检查**：CPU/Memory/Network 是否异常
5. **深度排查**：系统调用追踪、网络抓包

## 全组件排障清单

### 控制平面
- etcd 健康状态、磁盘空间、Leader 选举
- API Server 请求延迟、错误率
- Scheduler 调度失败原因
- KCM 控制器日志

### 节点
- kubelet 状态、证书过期
- 容器运行时（containerd/CRI-O）状态
- 资源压力（Memory/Disk/PID）

### 网络
- CoreDNS 解析延迟/失败
- Service Endpoint 一致性
- CNI 插件状态
- NetworkPolicy 冲突

### 存储
- PV/PVC 绑定状态
- CSI 驱动日志
- 挂载点权限

### 工作负载
- Pod 状态（Pending/CrashLoopBackOff/ImagePullBackOff）
- Init Container 失败
- 探针配置错误
- 资源请求超过节点容量

---

> 来源：.zread/wiki/drafts/15-jie-gou-hua-gu-zhang-pai-cha-*.md

## Related

- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
