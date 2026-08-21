---
title: 容器网络接口
description: CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使
  Kubernet...
summary: CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使 Kubernet...
category: dictionary
tags:
- k8s
- glossary
- cni
- networking
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器网络接口 是什么
- CNI (Container Network Interface) 详解
trigger_keywords:
- 容器网络接口
- CNI (Container Network Interface)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器网络接口

> **英文名**: CNI (Container Network Interface)

## 概述

CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使 Kubernetes 能够使用各种网络插件实现 Pod 间通信。

## 核心概念/原理

### 核心概念

- **CNI 插件**：实现 CNI 规范的网络软件，如 Calico、Cilium、Flannel、Weave 等。
- **CNI 配置**：通过 JSON 配置文件定义网络拓扑和 IP 分配策略。
- **CNI 执行流程**：
  1. kubelet 通过 CRI 创建容器。
  2. 容器运行时调用 CNI 插件。
  3. CNI 插件配置网络接口和路由。

### 主流 CNI 插件

| 插件 | 数据面 | 网络策略 | 特点 |
|------|--------|---------|------|
| Calico | BGP/VXLAN | 支持 | 纯三层路由，性能优秀 |
| Cilium | eBPF | 支持 | 内核旁路，高性能 |
| Flannel | VXLAN | 不支持 | 简单轻量 |
| Weave | mesh | 支持 | 加密通信 |

## 关键机制或特性

- CNI 由 CNCF 维护，是容器网络的事实标准。
- CNI 配置文件位于 `/etc/cni/net.d/` 目录。
- 一个节点可以有多个 CNI 配置，按文件名排序选择。

## 使用场景与最佳实践

- 生产环境推荐 Calico 或 Cilium，功能完整且性能优秀。
- 大规模集群考虑 Cilium 的 eBPF 数据面获得更好性能。
- 确保 CNI 版本与 Kubernetes 版本兼容。
- 监控 CNI 的 IP 分配情况和网络延迟。

## 架构深度解析

### CNI 调用链路

```
┌──────────────────────────────────────────────────────────────┐
│  kubelet（Pod 创建流程）                                       │
│  1. 调用 CRI（containerd/CRI-O）创建 sandbox 容器             │
│  2. CRI 运行时解析 Pod 的 annotations 与 CNI 配置             │
│  3. 按文件名顺序读取 /etc/cni/net.d/*.conf                    │
│  4. 执行 CNI 插件二进制（ADD 操作）                           │
│     ├─ 分配 IP（IPAM 插件：host-local / calico-ipam）         │
│     ├─ 创建 veth 对 / 配置隧道接口                            │
│     └─ 配置路由与 sysctl 参数                                 │
│  5. 将结果写回 sandbox 的网络命名空间                         │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（containernetworking/cni）

| 模块 | 路径 | 职责 |
|------|------|------|
| libcni | `libcni/api.go` | 提供 `AddNetworkList`/`DelNetworkList` 高层 API，按列表顺序调用插件 |
| skel | `pkg/skel/skel.go` | 插件骨架：解析 stdin 的 CNI 命令参数并分发到 cmdAdd/cmdDel |
| types | `pkg/types/types.go` | 定义 `NetworkConfig`、`RuntimeConf`、`Result` 等核心类型 |
| invoke | `pkg/invoke/raw_exec.go` | 通过 exec 执行插件二进制，注入 CNI_COMMAND/CNI_CONTAINERID 等环境变量 |
| plugins | `plugins/ipam/host-local` | 参考 IPAM 实现：基于文件锁的 IP 分配与释放 |

### 流程步骤

1. kubelet 通过 CRI 创建 sandbox，`sandboxConfig` 携带 `RuntimeHandler` 对应的 CNI 配置目录。
2. containerd 的 `cri` 插件读取 `--cni-conf-dir`（默认 `/etc/cni/net.d`）下的配置文件。
3. libcni 按文件名顺序（`10-xxx`、`99-xxx`）执行插件，后加载的配置优先。
4. 插件执行 ADD 操作，返回 `CNIResult`（接口名、IP、路由、DNS）。
5. 失败时执行 DEL 清理，避免 IP 泄漏；kubelet 重试并上报事件。

## 生产案例

### 案例 1：CNI 升级后 Pod 长时间处于 ContainerCreating

| 时间 | 事件 |
|------|------|
| 10:00 | 运维将 Calico 从 v3.24 升级到 v3.27，DaemonSet 滚动完成 |
| 10:05 | 新建 Pod 全部卡在 ContainerCreating |
| 10:08 | `kubectl describe pod` 显示 `network plugin not ready: cni config uninitialized` |
| 10:12 | 检查发现 `/etc/cni/net.d/` 残留旧版 `10-calico.conflist` 与新版配置共存 |
| 10:20 | 删除旧配置并重启节点 kubelet 后恢复 |

**根因**：升级脚本未清理旧 CNI 配置，kubelet 按文件名顺序加载到不一致的配置组合。
**修复命令**：
```bash
# 查看节点上 CNI 配置 🟢 只读
ls -la /etc/cni/net.d/
# 校验配置可用性（在节点上执行）🟢 只读
cat /etc/cni/net.d/*.conflist | jq .
# 备份并移除旧配置后重启 kubelet 🟡 中风险
sudo mv /etc/cni/net.d/10-calico.conflist.bak /etc/cni/net.d/10-calico.conflist.bak.old
sudo systemctl restart kubelet
```

### 案例 2：IPAM 池耗尽导致 Pod 创建失败

**现象**：集群新增 50 个 Pod 后，后续 Pod 全部报 `failed to allocate for range 10.244.0.0/24: no IP addresses available in range`。
**诊断**：`kubectl get ippool -o wide` 显示池使用率 100%；`calicoctl ipam show` 确认大量已释放但未回收的 IP（孤儿 IP 来自被删除的 StatefulSet）。
**修复**：扩容 IP 池网段；清理孤儿 IP；为 DaemonSet/StatefulSet 规划独立 IP 池，避免大 Pod 抢占小池。

## 对比评测

| 维度 | Calico | Cilium | Flannel |
|------|--------|--------|---------|
| 数据面 | BGP/VXLAN（iptables/eBPF） | eBPF（TC/XDP） | VXLAN |
| 网络策略 | 支持（iptables/eBPF） | 支持（eBPF 原生） | 不支持 |
| 大规模性能 | 好 | 极好（eBPF 直通） | 一般 |
| 功能扩展 | NetworkPolicy、IPPool | L7 策略、可观测性、服务网格 | 仅三层连通 |
| 运维复杂度 | 中 | 高（内核要求高） | 低 |

**选型建议**：生产首选 Cilium（性能与可观测性），对内核版本有限制或团队熟悉 iptables 时选 Calico，小规模实验选 Flannel。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| Pod 卡 ContainerCreating | `kubectl describe pod <name>`；节点上 `journalctl -u kubelet -f` | CNI 插件崩溃、配置错误、IPAM 池耗尽 |
| Pod 有 IP 但不通 | `kubectl exec <pod> -- ping <peer>`；`ip route` | 路由缺失、防火墙规则、VXLAN 隧道故障 |
| 节点重启后 Pod IP 全变 | 检查 IPAM 是否持久化 | host-local 无状态、etcd 中 IPAM 数据损坏 |
| 删除 Pod 后 IP 不释放 | `calicoctl ipam show --show-blocks` | DEL 调用失败、Pod 强制删除 |

## 生产部署清单

- [ ] CNI 版本与 Kubernetes 版本矩阵已核对（kubelet 支持 CNI 1.0+）
- [ ] IPAM 池规划完成（预留 NodeIP 段、Service CIDR 不重叠）
- [ ] 网络策略默认拒绝已测试（default-deny Namespace 验证）
- [ ] 隧道/VXLAN 端口在安全组放行（如 4789/8472）
- [ ] 升级回滚方案已演练（保留旧版本配置备份）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 当前 CNI 有已知 CVE 或与 K8s 版本不兼容 | 立即规划升级窗口，先升级小集群验证 |
| P0 | IPAM 池使用率 > 80% 且扩容受限 | 扩容网段或迁移 IPPool 方案 |
| P1 | 需要 NetworkPolicy 但当前插件不支持 | 评估切换 Calico/Cilium，双 CNI 灰度 |
| P1 | 集群规模 > 500 节点仍用 iptables 数据面 | 评估 eBPF 数据面，压测后切换 |
| P2 | 现有插件功能满足需求 | 跟随 LTS 版本节奏，每 2-3 个 minor 升级一次 |

## 面试要点

> 以下 Q&A 覆盖 CNI 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：CNI 规范中 ADD/DEL/CHECK 三个操作的职责是什么？**
   A：ADD 在容器创建时配置网络（建接口、分配 IP、写路由）；DEL 在容器销毁时清理网络资源（避免 IP 泄漏与路由残留）；CHECK 用于容器运行期校验网络配置是否与期望一致（CNI 0.4.0+，kubelet 周期性调用）。实现时三个操作必须幂等，因为 kubelet 会重试失败的 ADD。

2. **Q：为什么 kubelet 不直接调用 CNI 插件而是通过 CRI 运行时？**
   A：kubelet 只负责编排生命周期，具体容器运行时（containerd/CRI-O）通过 CRI 的 `PodSandbox` 接口创建网络命名空间，再由运行时内部加载 CNI 配置并执行插件。这样解耦了 kubelet 与具体网络插件，且不同运行时可以复用同一套 CNI 插件体系；CNI 插件的调用环境（命名空间、网络栈）由运行时提供，插件本身只需操作 stdin/stdout 协议。

3. **Q：生产集群中 CNI 插件选择要考虑哪些关键因素？**
   A：① 数据面性能：eBPF（Cilium）在大规模高吞吐场景优于 iptables；② 网络策略能力：Flannel 不支持策略，合规要求必须选 Calico/Cilium；③ 内核兼容性：eBPF 要求 5.x+ 内核，老内核只能用 iptables 数据面；④ 与云平台集成：云厂商 ENI 方案（如阿里云 Terway、AWS VPC CNI）在云上延迟更优；⑤ 运维成本：隧道模式（VXLAN）调试复杂但跨子网简单，BGP 模式需管理 AS 与路由表。

## 参考链接

- [CNI (Container Network Interface) - Official Documentation](https://www.cni.dev/)

## Related

[[23-实体/04-网络/cilium.md|Cilium]] | [[23-实体/02-K8s核心组件/cni-plugins.md|CNI Plugins]]


<!-- risk-assessed -->
