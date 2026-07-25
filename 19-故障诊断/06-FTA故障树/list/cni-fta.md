---
title: CNI 异常故障树分析 (skills)
description: '- **范围**：CNI 插件二进制、CNI 配置、IPAM 地址分配、网络策略、节点路由、云厂商网络集成、kubelet/CRI 调用链。'
summary: '- **范围**：CNI 插件二进制、CNI 配置、IPAM 地址分配、网络策略、节点路由、云厂商网络集成、kubelet/CRI 调用链。'
category: skills
tags:
- k8s
- fta
- troubleshooting
- cni
- network
- calico
- cilium
- flannel
- terway
- ipam
- networkpolicy
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNI 异常故障树分析 是什么
- 如何 CNI 异常故障树分析
trigger_keywords:
- CNI
- 异常故障树分析
prerequisites:
- kubectl-basics
- cni-basics
- networking-basics
fta_id: FTA-CNI-001
component: CNI
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




# CNI 异常故障树分析

<!-- condition: Pod 处于 ContainerCreating、同节点/跨节点 Pod 不通、Service 后端 Pod 不通、CNI 插件 Pod CrashLoopBackOff -->

## 适用范围与说明
- **目标**：覆盖 CNI 插件异常导致 Pod 网络创建失败、Pod 间通信异常、网络策略误拦截等关键成因与路径。
- **范围**：CNI 插件二进制、CNI 配置、IPAM 地址分配、网络策略、节点路由、云厂商网络集成、kubelet/CRI 调用链。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: CNI 异常导致 Pod 网络创建失败/通信异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> PLUGIN[CNI 插件异常]
  OR0 --> CONFIG[CNI 配置异常]
  OR0 --> IPAM[IPAM 地址分配异常]
  OR0 --> ROUTE[节点路由异常]
  OR0 --> POLICY[网络策略异常]
  OR0 --> CLOUD[云厂商网络集成异常]
  OR0 --> CHAIN[kubelet/CRI/CNI 调用链异常]

  %% CNI 插件异常分支
  PLUGIN_OR{{OR}}
  PLUGIN --> PLUGIN_OR
  PLUGIN_OR --> PLUGIN1[插件 Pod 未运行]
  PLUGIN_OR --> PLUGIN2[插件二进制缺失/损坏]
  PLUGIN_OR --> PLUGIN3[插件版本不兼容]

  PLUGIN1_OR{{OR}}
  PLUGIN1 --> PLUGIN1_OR
  PLUGIN1_OR --> PLUGIN1A[Calico/Cilium/Flannel Pod OOM]
  PLUGIN1_OR --> PLUGIN1B[节点污点导致未调度]
  PLUGIN1_OR --> PLUGIN1C[镜像拉取失败]

  %% CNI 配置异常分支
  CONFIG_OR{{OR}}
  CONFIG --> CONFIG_OR
  CONFIG_OR --> CONFIG1[/etc/cni/net.d 配置缺失]
  CONFIG_OR --> CONFIG2[CNI 配置文件语法错误]
  CONFIG_OR --> CONFIG3[多个 CNI 配置冲突]

  %% IPAM 地址分配异常分支
  IPAM_OR{{OR}}
  IPAM --> IPAM_OR
  IPAM_OR --> IPAM1[IP 池耗尽]
  IPAM_OR --> IPAM2[IP 地址泄漏]
  IPAM_OR --> IPAM3[IPAM 数据库不可达]

  IPAM2_AND{{AND}}
  IPAM2 --> IPAM2_AND
  IPAM2_AND --> IPAM2A[Pod 删除但 CNI DEL 未执行]
  IPAM2_AND --> IPAM2B[CNI 插件未释放地址]

  %% 节点路由异常分支
  ROUTE_OR{{OR}}
  ROUTE --> ROUTE_OR
  ROUTE_OR --> ROUTE1[节点无到达 Pod CIDR 的路由]
  ROUTE_OR --> ROUTE2[BGP/隧道未建立]
  ROUTE_OR --> ROUTE3[云路由表未同步]

  %% 网络策略异常分支
  POLICY_OR{{OR}}
  POLICY --> POLICY_OR
  POLICY_OR --> POLICY1[NetworkPolicy 误拦截]
  POLICY_OR --> POLICY2[CNI 策略引擎异常]
  POLICY_OR --> POLICY3[eBPF 程序加载失败]

  %% 云厂商网络集成异常分支
  CLOUD_OR{{OR}}
  CLOUD --> CLOUD_OR
  CLOUD_OR --> CLOUD1[VPC CIDR 与 Pod CIDR 冲突]
  CLOUD_OR --> CLOUD2[安全组/ACL 阻断]
  CLOUD_OR --> CLOUD3[云网络插件 Pod 异常]

  %% 调用链异常分支
  CHAIN_OR{{OR}}
  CHAIN --> CHAIN_OR
  CHAIN_OR --> CHAIN1[kubelet 未调用 CNI ADD]
  CHAIN_OR --> CHAIN2[CRI sandbox 创建失败]
  CHAIN_OR --> CHAIN3[CNI 插件执行超时]
```

---

## 生产级观测与证据

- **CNI 关键日志关键字**：`CNI plugin failed`、`IPAM allocation failed`、`network is unreachable`、`no route to host`、`NetworkPolicy denied`、`BPF program load failed`。
- **关键指标**：CNI 插件 Pod 状态、IP 池使用率、BGP 会话状态、节点路由表条目数、网络策略命中数。
- **关键命令**：
  ```bash
  ls -la /etc/cni/net.d/
  cat /etc/cni/net.d/10-*.conflist
  ip route
  ip addr show cni0
  calicoctl node status
  cilium status
  ```

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[23-实体/02-K8s核心组件/cni.md|CNI]] — CNI
- [[19-故障诊断/04-高级排障/structural-03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]]
- [[19-故障诊断/06-FTA故障树/list/service-fta.md|Service 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/kube-proxy-fta.md|kube-proxy 异常故障树分析]]


<!-- risk-assessed -->
