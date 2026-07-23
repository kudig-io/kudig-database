---
title: CNI 插件开发实战指南
summary: 用 containernetworking/cni skel 库从零开发一个 Kubernetes CNI 插件，覆盖 ADD/DEL/CHECK/IPAM 全流程。
category: 网络
tags:
- cni
- plugin-development
- go
- skel
- ipam
- networking
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: expert
reading_level: expert
audience:
- 网络开发工程师
- CNI 维护者
- 架构师
estimated_read_time: 30min
intent_queries:
- 如何开发 CNI 插件
- CNI skel 库怎么用
- CNI ADD DEL CHECK 命令
- CNI IPAM 子插件
trigger_keywords:
- CNI 插件开发
- skel
- ADD/DEL/CHECK
- IPAM
- conflist
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标节点与集群是否正确；是否具备足够的权限（节点 root / sudo）；是否已在非生产环境（kind / minikube / 专用节点）验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断、节点网络不可达）、🟡 中风险（会修改节点网络栈或集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> 此外，自研 CNI 插件以 root 身份运行，且会被 kubelet 在每个 Pod 创建/销毁时调用——一个有 bug 的插件足以让整个节点上的 Pod 无法启动或网络瘫痪。任何开发版本的插件都必须先在隔离的测试节点上跑通，再考虑进入预发环境。

# CNI 插件开发实战指南 (CNI Plugin Development Guide)

> **适用版本**: Kubernetes v1.28+（CNI spec 1.0.0+） | **难度**: 专家 | **最后更新**: 2026-07
> **依赖库**: `github.com/containernetworking/cni` v1.1.x、`github.com/containernetworking/plugins`、`github.com/vishvananda/netlink`

---

## 概述

CNI（Container Network Interface）插件本质上是一个**可执行二进制**。kubelet 在创建或销毁 Pod sandbox 时，通过 CRI（containerd / CRI-O）以约定的环境变量和 stdin JSON 配置去 `exec` 这个二进制，由它完成 veth pair 创建、IP 分配、路由配置、iptables 规则写入等数据面工作。这套机制与"插件是一个常驻 DaemonSet"完全不同——它是一次性的、同步的、进程级的 RPC。

本文是一篇**源码级的实战指南**，目标是用 containernetworking/cni 官方提供的 `skel` 库，从零写出一个最小可用、可编译、可部署的 bridge-style CNI 插件（下文称 `mycni`）。读完后你应当能够：

- 说清 kubelet → CRI → CNI plugin → IPAM 的完整调用链与时序；
- 用 `skel` 库写出符合 1.0+ 规范的 ADD/DEL/CHECK/VERSION 四个操作；
- 通过 `delegate` 调用 IPAM 子插件完成 IP 分配；
- 编译二进制、写 `conflist`、部署到 `/opt/cni/bin` 并在 kind 集群上验证。

本文**不是**一篇 CNI 规范教程（规范与配置见 [[网络/K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构基础]]），也**不是**一篇 CNI 选型指南（选型见 [[网络/网络基础/10-cni-plugin-comparison-selection.md|CNI 插件选型]]）。它假设你已经读过那两篇，了解 CNI 的位置与主流实现，现在想知道"如果我自己写一个，代码长什么样"。

> **定位说明**：本指南的示例插件是教学骨架，仅用于理解 CNI 插件的工作原理。生产环境请使用成熟项目（Calico / Cilium / Terway），不建议自研——理由见文末"生产实践"一节。

---

## CNI 插件调用模型

理解 CNI 插件开发的第一步，是彻底搞清楚**谁在什么时候、以什么方式调用你的二进制**。这决定了你接收到什么参数、返回什么结果、失败时如何处理。

### 调用链总览

一个 Pod 从调度到网络就绪，CNI 的位置如下：

```
┌──────────────┐   1. CreatePod (API)      ┌──────────────────┐
│  kube-sched  │ ─────────────────────────▶ │  kube-apiserver  │
└──────────────┘                            └────────┬─────────┘
                                                     │ 2. watch
                                                     ▼
                                            ┌──────────────────┐
                                            │     kubelet      │  (每个 Node)
                                            └────────┬─────────┘
                                                     │ 3. RunPodSandbox (gRPC/CRI)
                                                     ▼
                                            ┌──────────────────┐
                                            │  containerd cri  │  (或 CRI-O)
                                            │   plugin         │
                                            └────────┬─────────┘
                                                     │ 4. 读取 /etc/cni/net.d/*.conflist
                                                     │    定位默认网络与插件链
                                                     ▼
                                            ┌──────────────────┐
                                            │  CNI Shim (libcni)│
                                            │  - 解析配置       │
                                            │  - 按顺序调用插件 │
                                            └────────┬─────────┘
                                                     │ 5. exec /opt/cni/bin/<plugin>
                                                     │    + 环境变量 + stdin JSON
                                                     ▼
                                            ┌──────────────────┐
                                            │  mycni (你的二进制)│
                                            │  - 解析 stdin     │
                                            │  - 调用 IPAM 子插件│ (exec)
                                            │  - 创建 veth/路由 │
                                            │  - 返回 Result    │
                                            └──────────────────┘
```

关键点：**kubelet 本身并不直接 exec CNI 二进制**。它通过 CRI gRPC 接口调用 containerd（或 CRI-O）的 `RunPodSandbox` / `StopPodSandbox`，由容器运行时内部的 CRI 实现去读 `/etc/cni/net.d/` 下的配置，再通过 `libcni`（github.com/containernetworking/cni/pkg/libcni）按 chained plugin 顺序逐个 exec 插件二进制。所以从开发视角看，"你的二进制是被 libcni exec 的"。

### ADD 调用时序

下图是一个 Pod sandbox 创建时 CNI ADD 的完整时序（DELETE/CHECK 类似，只是命令不同）：

```
kubelet          containerd(cri)       libcni            mycni           host-local(IPAM)
  │                   │                   │                 │                  │
  │ RunPodSandbox     │                   │                 │                  │
  │──────────────────▶│                   │                 │                  │
  │                   │ read conflist     │                 │                  │
  │                   │ build CmdArgs      │                 │                  │
  │                   │ AddNetworkList()  │                 │                  │
  │                   │──────────────────▶│                 │                  │
  │                   │                   │ exec ADD        │                  │
  │                   │                   │ env: CNI_*      │                  │
  │                   │                   │ stdin: config   │                  │
  │                   │                   │────────────────▶│                  │
  │                   │                   │                 │ exec IPAM ADD    │
  │                   │                   │                 │─────────────────▶│
  │                   │                   │                 │  IP result JSON  │
  │                   │                   │                 │◀─────────────────│
  │                   │                   │                 │ create veth      │
  │                   │                   │                 │ move to netns    │
  │                   │                   │                 │ assign addr/route│
  │                   │                   │  Result JSON    │                  │
  │                   │                   │◀────────────────│                  │
  │                   │  result           │                 │                  │
  │                   │◀──────────────────│                 │                  │
  │ sandbox ready     │                   │                 │                  │
  │◀──────────────────│                   │                 │                  │
```

### 实际的调用方式

CRI 实现最终调用你的二进制时，等价于在 shell 执行了这样一个命令：

```bash
# 🟢 低风险：这是 kubelet/cri 调用你插件时的等价命令，了解参数即可
CNI_COMMAND=ADD \
CNI_CONTAINERID=bd5fafb5c2e0a2d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8 \
CNI_NETNS=/var/run/netns/cni-bd5fafb5-c2e0-2d4e-5f6a-7b8c9d0e1f2a \
CNI_IFNAME=eth0 \
CNI_PATH=/opt/cni/bin \
CNI_ARGS='IgnoreUnknown=true;K8S_POD_NAMESPACE=default;K8S_POD_NAME=nginx;K8S_POD_INFRA_CONTAINERID=bd5fafb5c2e0a2d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8' \
  /opt/cni/bin/mycni < /etc/cni/net.d/00-mycni.conflist-first-fragment.json
```

注意三个要点：

1. **环境变量传运行时上下文**（命令、容器 ID、netns 路径、接口名），**stdin 传网络配置**（cniVersion、name、type、ipam 等）。两者缺一不可。
2. `CNI_IFNAME` 几乎总是 `eth0`；但在 Multus 多网卡场景下会出现 `net1`、`net2`。你的插件不能写死 `eth0`。
3. stdin 是**单段配置**。`conflist`（多插件链）会被 libcni 拆成多个 `NetworkConfig`，逐个传给对应插件——每个插件收到的 stdin 只有自己那一段（外加上一段的 prevResult）。

### 调用语义约定

| 约定 | 说明 |
|:---|:---|
| **幂等性** | ADD 必须幂等——同一 `(containerID, ifname)` 重复调用应不报错或安全重建。DEL 同理，对已删除的对象再 DEL 应返回成功。 |
| **不持有状态** | 插件二进制是无状态的，每次调用都是独立进程。状态（如已分配 IP）应持久化到 IPAM 的本地存储（host-local 用 `/var/lib/cni/`）。 |
| **同步返回** | 调用是阻塞的，插件必须在合理时间内（通常 < 2s）返回 JSON Result 或错误。超时会导致 Pod 卡在 ContainerCreating。 |
| **stdout 只输出 Result** | 任何日志都必须写 stderr，stdout 只能是标准 Result JSON，否则 libcni 解析失败。 |

---

## skel 库与插件骨架

`skel`（github.com/containernetworking/cni/pkg/skel）是 CNI 官方提供的骨架库，它替你处理了：解析环境变量、读 stdin、按 cniVersion 校验、把错误格式化成规范要求的 JSON、调用对应的处理函数。你只需要实现几个回调函数。

### 核心类型

```go
// PluginMainFuncs 定义了插件需要实现的回调集合
type PluginMainFuncs struct {
    VersionAll version.All  // 支持的 CNI 版本列表
    CmdAdd     func(args *CmdArgs) error   // ADD 处理函数
    CmdCheck   func(args *CmdArgs) error   // CHECK 处理函数（可选）
    CmdDel     func(args *CmdArgs) error   // DEL 处理函数
    About      *About        // 1.1+ 插件元信息（可选）
}

// CmdArgs 是 skel 解析后传给你的参数
type CmdArgs struct {
    ContainerID string            // 来自 CNI_CONTAINERID
    Netns       string            // 来自 CNI_NETNS（netns 路径）
    IfName      string            // 来自 CNI_IFNAME（如 eth0）
    Args        string            // 来自 CNI_ARGS（key=value;key=value）
    Path        string            // 来自 CNI_PATH（如 /opt/cni/bin）
    StdinData   []byte            // 来自 stdin 的原始 JSON 配置
}
```

`CmdArgs` 的字段已经把环境变量映射好了，你通常不需要自己去读 `os.Getenv`。`StdinData` 是原始字节，需要你 `json.Unmarshal` 成自己的配置结构体。

### 最简 main.go 骨架

```go
package main

import (
	"fmt"

	"github.com/containernetworking/cni/pkg/skel"
	"github.com/containernetworking/cni/pkg/version"
)

func main() {
	skel.PluginMainFuncs{
		VersionAll: version.All,
		CmdAdd:     cmdAdd,
		CmdCheck:   cmdCheck,
		CmdDel:     cmdDel,
	}.PluginMain("mycni", "0.1.0", version.All)
	// PluginMain 内部会：
	//  1. 读 CNI_COMMAND 环境变量
	//  2. 按 VERSION/ADD/CHECK/DEL 分发到对应函数
	//  3. 把返回的 error 按 CNI 规范格式化成 JSON 输出到 stdout
}

func cmdAdd(args *skel.CmdArgs) error {
	return fmt.Errorf("not implemented")
}

func cmdDel(args *skel.CmdArgs) error {
	return fmt.Errorf("not implemented")
}

func cmdCheck(args *skel.CmdArgs) error {
	return fmt.Errorf("not implemented")
}
```

`skel.PluginMain(...)` 是入口。它的三个参数分别是插件名、版本字符串、支持的 CNI spec 版本列表。当 `CNI_COMMAND=VERSION` 时，skel 自动用 `VersionAll` 输出版本信息，你无需手写 VERSION 处理逻辑。

> **VERSION 命令的处理**：CNI 规范要求 `CNI_COMMAND=VERSION` 时插件输出支持的版本列表。`skel` 库已经自动处理，只要你传入了正确的 `version.All`，就不需要实现单独的回调。

---

## CNI 环境变量与 stdin 配置

### 环境变量（运行时传入）

libcni 在 exec 你的二进制时，会设置如下环境变量。`skel` 会校验必填项，缺失时报错。

| 环境变量 | 必填 | 示例值 | 含义 |
|:---|:---:|:---|:---|
| `CNI_COMMAND` | ✅ | `ADD` / `DEL` / `CHECK` / `VERSION` | 要执行的操作 |
| `CNI_CONTAINERID` | ✅* | `bd5fafb5...` | 容器（sandbox）ID；VERSION 命令可省 |
| `CNI_NETNS` | ADD/CHECK | `/var/run/netns/cni-xxx` | 容器 network namespace 路径；DEL 可为空 |
| `CNI_IFNAME` | ADD/CHECK | `eth0` | 容器内接口名 |
| `CNI_PATH` | ✅ | `/opt/cni/bin` | CNI 二进制搜索路径（用于 delegate 调用 IPAM） |
| `CNI_ARGS` | 否 | `K8S_POD_NAMESPACE=default;K8S_POD_NAME=nginx` | 附加参数（Kubernetes 传入 Pod 元信息） |
| `CNI_NETNS_OVERRIDE` | 否 | `1` | 允许覆盖已有 netns（很少用） |

> Kubernetes 通过 CRI 传入的 `CNI_ARGS` 含有 `K8S_POD_NAMESPACE`、`K8S_POD_NAME`、`K8S_POD_INFRA_CONTAINERID` 等字段，可用于在插件内部做基于 Pod 名的策略。但不要依赖这些字段做正确性判断——非 K8s 运行时（例如直接用 podman + CNI）不会传。

### stdin JSON 配置（网络配置）

stdin 是一段 JSON，描述"这个网络长什么样、用哪个 IPAM"。基本结构如下：

```json
{
  "cniVersion": "1.0.0",
  "name": "mycni-net",
  "type": "mycni",
  "bridge": "mycni0",
  "mtu": 1450,
  "isDefaultGateway": true,
  "ipMasq": true,
  "ipam": {
    "type": "host-local",
    "ranges": [
      [ { "subnet": "10.244.0.0/24" } ]
    ],
    "routes": [ { "dst": "0.0.0.0/0" } ],
    "dataDir": "/var/lib/cni/networks"
  },
  "dns": {
    "nameservers": [ "10.96.0.10" ],
    "search": [ "default.svc.cluster.local", "svc.cluster.local" ]
  }
}
```

对应到 Go 配置结构体：

```go
import (
	"github.com/containernetworking/cni/pkg/types"
	invoke "github.com/containernetworking/cni/pkg/invoke"
)

// NetConf 是插件自己的配置（内嵌 types.NetConf 拿到 cniVersion/name/type/args）
type NetConf struct {
	types.NetConf
	Bridge          string `json:"bridge"`           // 网桥名，如 mycni0
	MTU             int    `json:"mtu"`              // 接口 MTU
	IsDefaultGateway bool   `json:"isDefaultGateway"` // 是否作为默认网关
	IPMasq          bool   `json:"ipMasq"`           // 是否做 SNAT
	IPAM            IPAMConfig `json:"ipam"`         // IPAM 子配置
	DNS             types.DNS `json:"dns,omitempty"` // DNS 配置
}

// IPAMConfig 实际上只是一个标记，告诉主插件用哪个 delegate
// 真正的 IPAM 字段（ranges/routes）由 host-local 自己解析
type IPAMConfig struct {
	Type string `json:"type"`
}

// loadConfig 解析 stdin
func loadConfig(stdin []byte) (*NetConf, error) {
	conf := &NetConf{}
	if err := json.Unmarshal(stdin, conf); err != nil {
		return nil, fmt.Errorf("failed to parse config: %w", err)
	}
	if conf.Type != "mycni" {
		return nil, fmt.Errorf("config type %q not mycni", conf.Type)
	}
	return conf, nil
}
```

`types.NetConf` 内嵌结构会自动覆盖 `cniVersion`、`name`、`type`、`capabilities`、`ipMasq`(规范内置)、`dns` 等标准字段。你只需要在自己的结构体里加插件私有字段（如 `bridge`、`mtu`）。

---

## 四个必须实现的操作

CNI 规范定义了四个命令。其中 ADD、DEL、VERSION 是必须的，CHECK 自 1.0 起成为推荐实现（kubelet 在 `--cni-bin-dir` 的插件支持 CHECK 时会调用它做一致性检查）。

### 1. ADD（CNI_COMMAND=ADD）

ADD 是最核心的操作。它的职责是：**为一个容器接口配上网络**。典型步骤：

1. 解析 stdin 配置、校验 cniVersion；
2. （可选）调用 IPAM 子插件分配 IP，拿到 IP 信息；
3. 创建 host 侧网络设备（如 bridge / veth host 端）；
4. 创建 veth pair，把一端移入容器 netns；
5. 在容器接口上配置 IP 地址与路由（默认路由指向网桥）；
6. 在 host 侧接口配置（如把 veth host 端 attach 到 bridge）；
7. （可选）配置 SNAT/iptables；
8. 构造并返回 `types.Result`（含 IP、接口、路由、DNS）。

函数签名与关键逻辑骨架：

```go
import (
	"github.com/containernetworking/cni/pkg/types"
	current "github.com/containernetworking/cni/pkg/types/100"
)

func cmdAdd(args *skel.CmdArgs) error {
	// 1. 解析配置
	conf, err := loadConfig(args.StdinData)
	if err != nil {
		return err
	}

	// 2. 解析 prevResult（如果是 chained plugin，前一个插件会传入 prevResult）
	//    用 current.GetResult(conf.CNIVersion) 反序列化

	// 3. 调用 IPAM 分配 IP（详见下节）
	ipamResult, err := ipam.ExecAdd(conf.IPAM.Type, args.StdinData)
	if err != nil {
		return fmt.Errorf("ipam add failed: %w", err)
	}
	// ipamResult 是 types.Result，需要转成当前版本
	result, err := current.NewResultFromResult(ipamResult)
	if err != nil {
		return fmt.Errorf("convert ipam result: %w", err)
	}
	if len(result.IPs) == 0 {
		return fmt.Errorf("ipam returned no IPs")
	}

	// 4. 创建 bridge（如果不存在）
	br, err := ensureBridge(conf.Bridge, conf.MTU)
	if err != nil {
		return fmt.Errorf("ensure bridge: %w", err)
	}

	// 5. 创建 veth pair，移入 netns
	hostVeth, containerVeth, err := setupVethPair(args.Netns, args.IfName, conf.MTU, br)
	if err != nil {
		return fmt.Errorf("setup veth: %w", err)
	}

	// 6. 在容器接口上配置 IP 和路由
	if err := configureContainerNIC(args.Netns, args.IfName, result.IPs, conf); err != nil {
		return fmt.Errorf("configure container nic: %w", err)
	}

	// 7. 填充 Result 的 interfaces 字段（让 CHECK 能校验）
	result.Interfaces = []*current.Interface{
		{Name: conf.Bridge, Mac: br.Attrs().HardwareAddr.String()},
		{Name: hostVeth, Mac: "...", Sandbox: ""},
		{Name: args.IfName, Mac: "...", Sandbox: args.Netns},
	}

	// 8. （可选）配置 SNAT
	if conf.IPMasq {
		if err := setupIPMasq(conf.Bridge, result.IPs); err != nil {
			return fmt.Errorf("setup ipmasq: %w", err)
		}
	}

	// 9. 返回 Result（types.Result 必须输出到 stdout）
	types.WriteResult(result, conf.CNIVersion)
	return nil
}
```

返回的 Result 序列化后形如：

```json
{
  "cniVersion": "1.0.0",
  "interfaces": [
    {"name": "mycni0", "mac": "..." },
    {"name": "veth1234", "mac": "...", "sandbox": ""},
    {"name": "eth0", "mac": "...", "sandbox": "/var/run/netns/cni-xxx"}
  ],
  "ips": [
    {
      "interface": 2,
      "address": "10.244.1.5/24",
      "gateway": "10.244.1.1"
    }
  ],
  "routes": [
    { "dst": "0.0.0.0/0", "gw": "10.244.1.1" }
  ],
  "dns": {
    "nameservers": ["10.96.0.10"],
    "search": ["default.svc.cluster.local"]
  }
}
```

### 2. DEL（CNI_COMMAND=DEL）

DEL 负责清理 ADD 创建的所有资源。**幂等性是硬性要求**——对已经删除的资源再 DEL 必须返回成功，否则 Pod 卡在 Terminating。

```go
func cmdDel(args *skel.CmdArgs) error {
	conf, err := loadConfig(args.StdinData)
	if err != nil {
		return err
	}

	// 1. 先调用 IPAM 释放 IP（哪怕后面删 veth 失败也要尝试释放，避免 IP 泄漏）
	if err := ipam.ExecDel(conf.IPAM.Type, args.StdinData); err != nil {
		// 某些 IPAM（如 dhcp）在容器已删时可能找不到记录，按规范应容忍
		// 但 host-local 找不到记录是异常，需返回
		return fmt.Errorf("ipam del failed: %w", err)
	}

	// 2. 如果 netns 已经不存在（Pod 已被强删），veth 会随之消失，直接返回成功
	if args.Netns == "" {
		return nil
	}
	nsExists, _ := nsExists(args.Netns)
	if !nsExists {
		return nil
	}

	// 3. 进入 netns 删除容器侧接口
	if err := deleteContainerNIC(args.Netns, args.IfName); err != nil {
		// "link not found" 视为成功（幂等）
		if !isNotExist(err) {
			return fmt.Errorf("delete container nic: %w", err)
		}
	}

	// 4. 删除 SNAT 规则（如果配置过）
	if conf.IPMasq {
		teardownIPMasq(conf.Bridge)
	}

	// 5. （可选）如果网桥上没有接口了，删除网桥
	//    生产 CNI 通常不删 bridge（保留给下一个 Pod 复用）

	return nil
}
```

> **DEL 的顺序很重要**：先调 IPAM 释放 IP，再删 veth。如果顺序反了，IPAM 释放失败时 veth 已删，下次 ADD 可能复用同一个 IP 却找不到旧 veth。另外 `args.Netns` 在 DEL 时可能为空字符串（Pod 已被运行时清理），必须处理这种情况。

### 3. CHECK（CNI_COMMAND=CHECK，1.0+）

CHECK 用于校验"这个接口的配置和当前实际状态是否一致"，**不修改任何状态**。kubelet 会在 Pod 启动后调用它做健康检查。如果你的插件不实现 CHECK，`skel` 会自动返回一个"不支持 CHECK"的标准错误，kubelet 会容忍并回退到只调用 ADD。

```go
func cmdCheck(args *skel.CmdArgs) error {
	conf, err := loadConfig(args.StdinData)
	if err != nil {
		return err
	}

	// 解析 prevResult（CHECK 一定有 prevResult，它是 ADD 的返回值）
	prevResultRaw, err := prevResultFromStdin(args.StdinData)
	if err != nil {
		return err
	}
	prev, err := current.NewResultFromResult(prevResultRaw)
	if err != nil {
		return err
	}

	// 1. 校验容器接口是否存在、IP 是否匹配
	netns, err := ns.GetNS(args.Netns)
	if err != nil {
		return types.NewError(types.ErrInvalidEnvVars,
			"failed to open netns", err.Error())
	}
	defer netns.Close()

	err = netns.Do(func(_ ns.NetNS) error {
		link, err := netlink.LinkByName(args.IfName)
		if err != nil {
			return fmt.Errorf("interface %s not found: %w", args.IfName, err)
		}
		// 校验每个 IP 是否真的配在接口上
		addrs, _ := netlink.AddrList(link, netlink.FAMILY_V4)
		for _, ip := range prev.IPs {
			expected := *ip.Address.IP
			found := false
			for _, a := range addrs {
				if a.IP.Equal(expected) {
					found = true
					break
				}
			}
			if !found {
				return fmt.Errorf("IP %s missing on %s", expected, args.IfName)
			}
		}
		// 校验路由
		// 校验 MTU
		if link.Attrs().MTU != conf.MTU && conf.MTU != 0 {
			return fmt.Errorf("MTU mismatch: want %d got %d",
				conf.MTU, link.Attrs().MTU)
		}
		return nil
	})
	return err
}
```

CHECK 的返回约定：发现不一致时返回 CNI 错误（types.NewError with code `ErrInvalidEnvVars` 或自定义），但**不要尝试修复**——修复是 kubelet 通过重新触发 ADD/DEL 来完成的（通过把 Pod 标记为需要重建）。

### 4. VERSION（CNI_COMMAND=VERSION）

VERSION 由 `skel` 自动处理，无需自己实现。它返回一个固定格式：

```json
{
  "cniVersion": "1.0.0",
  "supportedVersions": [ "0.1.0", "0.2.0", "0.3.0", "0.3.1", "0.4.0", "1.0.0", "1.1.0" ]
}
```

`supportedVersions` 来自你传给 `skel.PluginMain` 的 `version.All`。如果你想限制只支持某些版本，自定义一个 `[]*version.PluginInfo` 传入即可。

---

## IPAM 子插件

CNI 规范把"IP 地址管理"从主插件中拆出来，作为独立的二进制（即 IPAM plugin）。这样你可以自由组合：任何一个主插件可以搭配任何一个 IPAM。常见的 IPAM 有：

| IPAM | 类型 | 说明 |
|:---|:---|:---|
| `host-local` | 子网本地分配 | 从配置的 subnet 内分配 IP，记录在本地磁盘 `/var/lib/cni/networks/<name>/`。最常用。 |
| `dhcp` | DHCP 客户端 | 通过一个宿主 daemon 向 DHCP 服务器申请 IP。需要常驻 `dhcp daemon`。 |
| `static` | 静态分配 | 按配置直接给定 IP，不分配。用于调试或固定 IP 场景。 |
| Calico ipam | 集群级分配 | Calico 自带，基于 etcd 做集群级 IPAM，支持 IP 池与亲和。 |

### 主插件如何调用 IPAM

主插件通过 `github.com/containernetworking/cni/pkg/invoke` 的 delegate 模式调用 IPAM。流程是：

1. 把 stdin 的原始 JSON（包含 `ipam` 字段）原样传给 IPAM 二进制；
2. IPAM 解析 `ipam` 字段，分配 IP，返回 `types.Result`（只含 IPs，不含 interfaces）；
3. 主插件拿到 IPs，自己负责把 IP 配到接口上。

`pkg/invoke` 的 `delegate` 包封装了这个调用：

```go
import (
	"github.com/containernetworking/cni/pkg/invoke"
	"github.com/containernetworking/cni/pkg/types"
)

// ipamExec 是对 invoke.DelegateDelegate 的薄封装
var ipamExec invoke.IPAMDelegate = invoke.DelegateExecutor{}

func ipamAdd(exec invoke.IPAMDelegate, conf *NetConf, args *skel.CmdArgs) (types.Result, error) {
	// 把原始 stdin 透传给 IPAM（它只关心 ipam 字段）
	// exec 会自动用 CNI_PATH 找到对应二进制
	result, err := exec.ExecAdd(conf.IPAM.Type, args.StdinData)
	if err != nil {
		return nil, err
	}
	// 校验 IPAM 返回的版本是否与主插件兼容
	if result == nil {
		return nil, fmt.Errorf("ipam returned nil result")
	}
	return result, nil
}

func ipamDel(exec invoke.IPAMDelegate, conf *NetConf, args *skel.CmdArgs) error {
	return exec.ExecDel(conf.IPAM.Type, args.StdinData)
}

func ipamCheck(exec invoke.IPAMDelegate, conf *NetConf, args *skel.CmdArgs) error {
	return exec.ExecCheck(conf.IPAM.Type, args.StdinData)
}
```

> **调用 IPAM 等价命令**：`invoke.ExecAdd("host-local", stdin)` 实际上等价于执行 `CNI_COMMAND=ADD CNI_PATH=/opt/cni/bin /opt/cni/bin/host-local < stdin`。`CNI_PATH` 用来定位 IPAM 二进制路径——这就是为什么环境变量 `CNI_PATH` 必填。

### IPAM 返回的数据

host-local 在 ADD 成功后会返回类似下面的 JSON（仍是 CNI Result 格式，但只填 ips 字段）：

```json
{
  "cniVersion": "1.0.0",
  "ips": [
    {
      "address": "10.244.1.5/24",
      "gateway": "10.244.1.1"
    }
  ],
  "routes": [
    { "dst": "0.0.0.0/0", "gw": "10.244.1.1" }
  ]
}
```

主插件拿到这个 result 后，需要把 `ips[].interface` 字段填上——IPAM 不知道接口名，所以返回的 IP 没有关联 interface index。主插件在创建完 veth 后，把 ips 的 `interface` 指向容器侧接口在 `result.Interfaces` 中的下标（如上面 ADD 示例里的 `2`）。

### 自定义 IPAM

如果你需要自定义 IPAM（例如对接内部 IP 管理系统），写法和写主插件完全一样——也是一个独立的二进制，用 `skel`，实现 ADD/DEL/CHECK。区别是它的 ADD 只分配 IP、不碰任何网络设备。在 conflist 里把它的 `type` 设成你的二进制名即可。

---

## 完整 Go 示例：mycni bridge-style 插件

下面给出一个可编译、可部署的最小 bridge-style 插件完整源码。它实现：创建一个 Linux bridge `mycni0`，为每个 Pod 创建 veth pair，一端在 bridge 上、一端在容器 netns 里，配置 IP 和默认路由，并搭配 host-local IPAM。

### 项目结构

```
mycni/
├── go.mod
├── main.go         # 入口 + skel 注册
├── config.go       # 配置结构体与解析
├── add.go          # CmdAdd 实现
├── del.go          # CmdDel 实现
├── check.go        # CmdCheck 实现
└── netlink.go      # netlink 封装（bridge/veth/addr/route）
```

### go.mod

```go
module github.com/yourorg/mycni

go 1.22

require (
	github.com/containernetworking/cni v1.1.2
	github.com/containernetworking/plugins v1.4.0
	github.com/vishvananda/netlink v1.2.1-beta.2
	github.com/vishvananda/netns v0.0.4
)
```

### config.go

```go
package main

import (
	"encoding/json"
	"fmt"

	"github.com/containernetworking/cni/pkg/types"
)

// NetConf 是 mycni 的配置结构
type NetConf struct {
	types.NetConf             // 内嵌标准字段: cniVersion/name/type/args/dns
	Bridge          string    `json:"bridge"`                     // 网桥名，默认 mycni0
	MTU             int       `json:"mtu"`                        // veth MTU
	IsDefaultGateway bool     `json:"isDefaultGateway,omitempty"` // 是否作默认网关
	IPMasq          bool      `json:"ipMasq,omitempty"`           // 是否做 SNAT
	HairpinMode     bool      `json:"hairpinMode,omitempty"`      // hairpin 模式
	IPAM            IPAMConf  `json:"ipam"`                       // IPAM 配置（透传给子插件）
}

// IPAMConf 只需保留 type，其它字段原样透传
type IPAMConf struct {
	Type string `json:"type"`
}

// loadConfig 解析 stdin，做基本校验
func loadConfig(stdin []byte) (*NetConf, *types.Result, error) {
	conf := &NetConf{}
	if err := json.Unmarshal(stdin, conf); err != nil {
		return nil, nil, fmt.Errorf("failed to parse network config: %w", err)
	}
	if conf.RawPrevResult != nil {
		// 如果是 chained plugin，skel 会把前一个插件的 Result 原样放在 RawPrevResult
		// 这里返回 nil 让调用方按需处理
	}
	if conf.Type != "mycni" {
		return nil, nil, fmt.Errorf("config type must be 'mycni', got %q", conf.Type)
	}
	if conf.Bridge == "" {
		conf.Bridge = "mycni0"
	}
	if conf.MTU == 0 {
		conf.MTU = 1450
	}
	if conf.IPAM.Type == "" {
		return nil, nil, fmt.Errorf("ipam.type must be specified")
	}
	return conf, nil, nil
}
```

### netlink.go（核心数据面操作）

```go
package main

import (
	"fmt"
	"net"
	"os"
	"syscall"

	"github.com/containernetworking/cni/pkg/ns"
	"github.com/containernetworking/plugins/pkg/utils/sysctl"
	current "github.com/containernetworking/cni/pkg/types/100"
	"github.com/vishvananda/netlink"
)

// ensureBridge 确保 host 侧网桥存在，返回 *netlink.Bridge
func ensureBridge(name string, mtu int) (*netlink.Bridge, error) {
	br := &netlink.Bridge{
		LinkAttrs: netlink.LinkAttrs{
			Name:   name,
			MTU:    mtu,
			TxQLen: -1,
		},
	}
	err := netlink.LinkAdd(br)
	if err != nil {
		if err != syscall.EEXIST {
			return nil, fmt.Errorf("create bridge %s: %w", name, err)
		}
		// 已存在，取出
		l, err := netlink.LinkByName(name)
		if err != nil {
			return nil, fmt.Errorf("get existing bridge %s: %w", name, err)
		}
		var ok bool
		br, ok = l.(*netlink.Bridge)
		if !ok {
			return nil, fmt.Errorf("%s exists but is not a bridge", name)
		}
	}

	// 关键：关闭网桥上 ip_forward 校验、设置 arp
	if _, err := sysctl.Sysctl("net/ipv4/conf/"+name+"/forwarding", "1"); err != nil {
		return nil, fmt.Errorf("enable forwarding on %s: %w", name, err)
	}

	if err := netlink.LinkSetUp(br); err != nil {
		return nil, fmt.Errorf("set bridge up: %w", err)
	}
	return br, nil
}

// setupVethPair 创建 veth pair，host 端 attach 到 bridge，container 端移入 netns
// 返回 hostVethName 和 containerVeth 的 MAC
func setupVethPair(netnsPath, ifName string, mtu int, br *netlink.Bridge) (string, string, error) {
	containerNS, err := ns.GetNS(netnsPath)
	if err != nil {
		return "", "", fmt.Errorf("open netns %s: %w", netnsPath, err)
	}
	defer containerNS.Close()

	hostVethName := generateVethName(ifName, 8)

	var containerMAC string
	err = containerNS.Do(func(_ ns.NetNS) error {
		// 1. 在容器 netns 内创建 veth pair，容器端名为 ifName
		hostVeth := &netlink.Veth{
			LinkAttrs: netlink.LinkAttrs{
				Name:  hostVethName,
				Flags: net.FlagUp,
				MTU:   mtu,
			},
			PeerName: ifName, // 容器端名
		}
		if err := netlink.LinkAdd(hostVeth); err != nil {
			return fmt.Errorf("create veth pair: %w", err)
		}

		// 取容器端 MAC
		cont, err := netlink.LinkByName(ifName)
		if err != nil {
			return fmt.Errorf("get container veth %s: %w", ifName, err)
		}
		containerMAC = cont.Attrs().HardwareAddr.String()

		// 2. host 端默认已经被移出 netns（veth 创建在哪个 netns，另一端就在另一个 netns）
		//    但上面我们是在容器 netns 内创建的，所以 host 端也在容器 netns 里
		//    需要把 host 端移回 host netns
		if err := netlink.LinkSetNsFd(hostVeth, int(getHostNetNSFd())); err != nil {
			return fmt.Errorf("move host veth to host ns: %w", err)
		}
		return nil
	})
	if err != nil {
		return "", "", err
	}

	// 3. 在 host netns 中把 host 端 attach 到 bridge 并 up
	hostVeth, err := netlink.LinkByName(hostVethName)
	if err != nil {
		return "", "", fmt.Errorf("get host veth %s: %w", hostVethName, err)
	}
	if err := netlink.LinkSetMaster(hostVeth, br); err != nil {
		return "", "", fmt.Errorf("attach host veth to bridge: %w", err)
	}
	if err := netlink.LinkSetUp(hostVeth); err != nil {
		return "", "", fmt.Errorf("set host veth up: %w", err)
	}
	return hostVethName, containerMAC, nil
}

// configureContainerNIC 进入 netns 给容器接口配 IP 和路由
func configureContainerNIC(netnsPath, ifName string, ips []*current.IPConfig, conf *NetConf) error {
	containerNS, err := ns.GetNS(netnsPath)
	if err != nil {
		return err
	}
	defer containerNS.Close()

	return containerNS.Do(func(_ ns.NetNS) error {
		link, err := netlink.LinkByName(ifName)
		if err != nil {
			return fmt.Errorf("get container link %s: %w", ifName, err)
		}
		if err := netlink.LinkSetUp(link); err != nil {
			return fmt.Errorf("set %s up: %w", ifName, err)
		}
		// 给每个 IP 配地址
		for _, ipc := range ips {
			addr := &netlink.Addr{
				IPNet: &ipc.Address,
				Label: "",
			}
			if err := netlink.AddrAdd(link, addr); err != nil {
				return fmt.Errorf("add addr %s to %s: %w", ipc.Address, ifName, err)
			}
		}
		// 默认路由（取第一个 IP 的 gateway）
		if len(ips) > 0 && ips[0].Gateway != nil {
			gw := ips[0].Gateway
			route := &netlink.Route{
				Dst: &net.IPNet{IP: net.IPv4zero, Mask: net.CIDRMask(0, 32)},
				Gw:  gw,
				// LinkIndex 让路由通过该接口
				LinkIndex: link.Attrs().Index,
			}
			if err := netlink.RouteAdd(route); err != nil {
				return fmt.Errorf("add default route via %s: %w", gw, err)
			}
		}
		return nil
	})
}

// deleteContainerNIC 进入 netns 删除容器接口
func deleteContainerNIC(netnsPath, ifName string) error {
	containerNS, err := ns.GetNS(netnsPath)
	if err != nil {
		return err
	}
	defer containerNS.Close()

	return containerNS.Do(func(_ ns.NetNS) error {
		link, err := netlink.LinkByName(ifName)
		if err != nil {
			if _, ok := err.(netlink.LinkNotFoundError); ok {
				return nil // 幂等
			}
			return err
		}
		return netlink.LinkDel(link)
	})
}

// getHostNetNSFd 返回当前进程（host）netns 的 fd
func getHostNetNSFd() uintptr {
	f, err := os.Open("/proc/self/ns/net")
	if err != nil {
		panic(err)
	}
	return f.Fd()
}

// generateVethName 生成 host 侧 veth 名，最多 15 字符
func generateVethName(prefix string, randLen int) string {
	// 简化：固定前缀 + 随机后缀，保证 < 15 字符
	return "veth" + randomHex(randLen)
}

func randomHex(n int) string {
	// 实现略：返回 n 位随机 hex 字符串
	return "abcd1234"[:n]
}
```

> **veth 命名长度限制**：Linux 接口名最多 15 字符。host 端 veth 名（如 `vethabcd1234`）务必控制长度，否则 `LinkAdd` 会失败。生产 CNI 通常用容器 ID 的前 N 位做后缀。

### add.go

```go
package main

import (
	"encoding/json"
	"fmt"

	"github.com/containernetworking/cni/pkg/invoke"
	"github.com/containernetworking/cni/pkg/skel"
	"github.com/containernetworking/cni/pkg/types"
	current "github.com/containernetworking/cni/pkg/types/100"
	"github.com/containernetworking/plugins/pkg/ipmasq"
)

func cmdAdd(args *skel.CmdArgs) error {
	conf, _, err := loadConfig(args.StdinData)
	if err != nil {
		return err
	}

	// 1. 调用 IPAM 分配 IP
	ipamResult, err := invoke.DelegateAdd(conf.IPAM.Type, args.StdinData, nil)
	if err != nil {
		return fmt.Errorf("ipam add: %w", err)
	}
	result, err := current.NewResultFromResult(ipamResult)
	if err != nil {
		return fmt.Errorf("convert ipam result: %w", err)
	}
	if len(result.IPs) == 0 {
		return types.NewError(types.ErrInvalidNetworkConfig, "no IP returned by IPAM", "")
	}

	// 2. 创建/确保 bridge
	br, err := ensureBridge(conf.Bridge, conf.MTU)
	if err != nil {
		return err
	}

	// 3. 创建 veth pair
	hostVeth, containerMAC, err := setupVethPair(args.Netns, args.IfName, conf.MTU, br)
	if err != nil {
		return err
	}

	// 4. 配置容器接口（IP + 路由）
	if err := configureContainerNIC(args.Netns, args.IfName, result.IPs, conf); err != nil {
		return err
	}

	// 5. 给每个 IP 标注它属于哪个 interface（容器侧）
	containerIfIndex := len(result.Interfaces)
	for i := range result.IPs {
		result.IPs[i].Interface = &containerIfIndex
	}

	// 6. 填充 interfaces 数组（顺序很重要，与 IP.Interface 索引对应）
	result.Interfaces = append(result.Interfaces, &current.Interface{
		Name: conf.Bridge,
		Mac:  br.Attrs().HardwareAddr.String(),
	})
	hostVethMAC := "" // 实际取 link.MAC
	result.Interfaces = append(result.Interfaces, &current.Interface{
		Name:    hostVeth,
		Mac:     hostVethMAC,
		Sandbox: "", // host netns
	})
	result.Interfaces = append(result.Interfaces, &current.Interface{
		Name:    args.IfName,
		Mac:     containerMAC,
		Sandbox: args.Netns,
	})

	// 7. SNAT（可选）
	if conf.IPMasq {
		chain := "MYCNI-" + conf.Name
		if err := ipmasq.Setup(conf.Name, args.ContainerID, args.IfName,
			result.IPs, chain, nil); err != nil {
			return fmt.Errorf("setup ipmasq: %w", err)
		}
	}

	// 8. 透传 DNS
	result.DNS = conf.DNS

	// 9. 输出 Result
	return types.PrintResult(result, conf.CNIVersion)
}

// prevResultFromStdin 用于 CHECK（解析 prevResult 字段）
func prevResultFromStdin(stdin []byte) (types.Result, error) {
	conf := struct {
		types.NetConf
	}{}
	if err := json.Unmarshal(stdin, &conf); err != nil {
		return nil, err
	}
	if conf.RawPrevResult == nil {
		return nil, fmt.Errorf("missing prevResult")
	}
	return types.NewResult(conf.RawPrevResult)
}
```

### del.go

```go
package main

import (
	"fmt"
	"os"

	"github.com/containernetworking/cni/pkg/invoke"
	"github.com/containernetworking/cni/pkg/ns"
	"github.com/containernetworking/cni/pkg/skel"
	"github.com/containernetworking/plugins/pkg/ipmasq"
	"github.com/vishvananda/netlink"
)

func cmdDel(args *skel.CmdArgs) error {
	conf, _, err := loadConfig(args.StdinData)
	if err != nil {
		// 配置解析失败也要尝试释放 IP（避免泄漏）
		// 但没有 conf 就不知道 IPAM type，只能放弃
		return err
	}

	// 1. 释放 IP（优先级最高）
	if err := invoke.DelegateDel(conf.IPAM.Type, args.StdinData, nil); err != nil {
		return fmt.Errorf("ipam del: %w", err)
	}

	// 2. 清理 SNAT
	if conf.IPMasq {
		chain := "MYCNI-" + conf.Name
		ipmasq.Teardown(conf.Name, args.ContainerID, args.IfName, chain)
	}

	// 3. 删除容器侧接口（netns 可能已不存在）
	if args.Netns == "" {
		return nil
	}
	_, statErr := os.Stat(args.Netns)
	if os.IsNotExist(statErr) {
		return nil
	}
	containerNS, err := ns.GetNS(args.Netns)
	if err != nil {
		// netns 已被清理，幂等返回成功
		return nil
	}
	defer containerNS.Close()

	err = containerNS.Do(func(_ ns.NetNS) error {
		link, err := netlink.LinkByName(args.IfName)
		if err != nil {
			// 接口已不存在，幂等返回
			return nil
		}
		return netlink.LinkDel(link)
	})
	return err
}
```

### check.go

```go
package main

import (
	"fmt"

	"github.com/containernetworking/cni/pkg/ns"
	"github.com/containernetworking/cni/pkg/skel"
	"github.com/containernetworking/cni/pkg/types"
	current "github.com/containernetworking/cni/pkg/types/100"
	"github.com/vishvananda/netlink"
)

func cmdCheck(args *skel.CmdArgs) error {
	conf, _, err := loadConfig(args.StdinData)
	if err != nil {
		return err
	}
	prev, err := prevResultFromStdin(args.StdinData)
	if err != nil {
		return err
	}
	result, err := current.NewResultFromResult(prev)
	if err != nil {
		return err
	}

	containerNS, err := ns.GetNS(args.Netns)
	if err != nil {
		return types.NewError(types.ErrInvalidEnvVars,
			"failed to open netns", err.Error())
	}
	defer containerNS.Close()

	return containerNS.Do(func(_ ns.NetNS) error {
		link, err := netlink.LinkByName(args.IfName)
		if err != nil {
			return fmt.Errorf("interface %s not found: %w", args.IfName, err)
		}
		if link.Attrs().MTU != conf.MTU && conf.MTU != 0 {
			return fmt.Errorf("MTU mismatch: want %d got %d",
				conf.MTU, link.Attrs().MTU)
		}
		// 校验 IP
		addrs, err := netlink.AddrList(link, netlink.FAMILY_ALL)
		if err != nil {
			return err
		}
		for _, wantIP := range result.IPs {
			found := false
			for _, a := range addrs {
				if a.IP.Equal(wantIP.Address.IP) {
					found = true
					break
				}
			}
			if !found {
				return fmt.Errorf("IP %s missing on %s", wantIP.Address, args.IfName)
			}
		}
		return nil
	})
}
```

### main.go

```go
package main

import (
	"github.com/containernetworking/cni/pkg/skel"
	"github.com/containernetworking/cni/pkg/version"
)

func main() {
	skel.PluginMainFuncs{
		VersionAll: version.All,
		CmdAdd:     cmdAdd,
		CmdCheck:   cmdCheck,
		CmdDel:     cmdDel,
	}.PluginMain("mycni", "0.1.0", version.All)
}
```

> **代码说明**：上面 ~350 行代码覆盖了一个 bridge-style CNI 的全部核心逻辑。为节省篇幅，部分辅助函数（`randomHex`、hostVethMAC 获取、SNAT 链细节）做了简化，真实实现可参考 [containernetworking/plugins](https://github.com/containernetworking/plugins) 仓库的 `plugins/main/bridge/` 与 `plugins/main/ptp/` 目录，那里的代码经过了大量生产验证。

---

## 编译与部署

### 编译

```bash
# 🟢 低风险：本地编译，不影响任何环境
cd ~/src/mycni
go build -o mycni ./...

# 静态编译（推荐，避免 glibc 版本不匹配）
CGO_ENABLED=0 go build -ldflags '-extldflags "-static"' -o mycni ./...
```

静态编译很重要：节点上的 Linux 发行版与你的开发机 glibc 版本可能不一致，动态链接会导致插件启动时报 `version `GLIBC_2.xx' not found`。CNI 插件二进制是 root 启动、每个 Pod 都要跑的，必须零依赖。

交叉编译（在 macOS 上编译 Linux 版本）：

```bash
# 🟢 低风险
CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -o mycni-linux-amd64 ./...
```

### 部署到节点

CNI 二进制默认放在 `/opt/cni/bin`，配置文件默认放在 `/etc/cni/net.d`。

```bash
# 🟡 中风险：在目标节点上替换/新增 CNI 二进制，错误二进制会导致节点 Pod 无法启动
# 拷贝二进制到节点
scp mycni-linux-amd64 root@node1:/opt/cni/bin/mycni
ssh root@node1 'chmod +x /opt/cni/bin/mycni'

# 在节点上验证二进制能响应 VERSION 命令
# 🟢 低风险
ssh root@node1 'CNI_COMMAND=VERSION CNI_PATH=/opt/cni/bin /opt/cni/bin/mycni'
# 期望输出: {"cniVersion":"1.0.0","supportedVersions":["0.1.0",...]}
```

> **不要覆盖现有的 CNI 二进制**。如果你在一个运行中的集群上替换 CNI（例如把 `calico` 换成 `mycni`），新的 Pod 会用新 CNI，但旧的 Pod 还在用旧 CNI 创建的接口，可能导致路由断裂。正确做法是新建节点、配置好新 CNI 后再迁移 Pod。

### 配置文件

`/etc/cni/net.d/` 下，libcni 按**文件名字典序**排序，取第一个 `.conflist` 作为默认网络。文件名通常以 `00-` 开头确保优先。

```bash
# 🟡 中风险：修改节点默认 CNI 配置
cat > /etc/cni/net.d/00-mycni.conflist <<'EOF'
{
  "cniVersion": "1.0.0",
  "name": "mycni-network",
  "plugins": [
    {
      "type": "mycni",
      "bridge": "mycni0",
      "mtu": 1450,
      "isDefaultGateway": true,
      "ipMasq": true,
      "ipam": {
        "type": "host-local",
        "ranges": [
          [ { "subnet": "10.244.0.0/24" } ]
        ],
        "routes": [ { "dst": "0.0.0.0/0" } ]
      }
    },
    {
      "type": "portmap",
      "capabilities": { "portMappings": true },
      "snat": true
    }
  ]
}
EOF
```

这是一个**chained plugin**配置：`plugins` 数组里第一个是主插件 `mycni`，第二个 `portmap` 用于处理 `hostPort`。libcni 会先调用 mycni 的 ADD，拿到 prevResult，再把它作为 stdin 传给 portmap 的 ADD，依次传递。

部署后**必须重启容器运行时**让 CRI 重新加载配置：

```bash
# 🔴 高风险：重启 containerd 会断开所有 Pod 的连接（Pod 不重启但容器短暂中断）
systemctl restart containerd

# 较温和的方式：发送 SIGHUP（部分版本支持热加载）
# 🟡 中风险
pkill -HUP containerd
```

### 在 kind / minikube 上测试

kind（Kubernetes IN Docker）是开发 CNI 最友好的环境——每个 node 是一个容器，可以 exec 进去任意改 `/opt/cni/bin`。

```bash
# 🟢 低风险：本地 kind 集群
kind create cluster --name cni-dev --image kindest/node:v1.30.0

# 把二进制拷进 kind 的 control-plane 节点
# 🟡 中风险：在 kind node 内替换 CNI 二进制
docker cp mycni-linux-amd64 cni-dev-control-plane:/opt/cni/bin/mycni
docker exec cni-dev-control-plane chmod +x /opt/cni/bin/mycni

# 替换默认 CNI 配置（kind 默认装的是 kindnetd）
# 🟡 中风险：替换后新建的 Pod 会用 mycni
docker cp 00-mycni.conflist cni-dev-control-plane:/etc/cni/net.d/00-mycni.conflist
docker exec cni-dev-control-plane sh -c 'rm -f /etc/cni/net.d/*kindnet*'

# 重启 kubelet 让它重新加载 CNI 配置
# 🟡 中风险
docker exec cni-dev-control-plane systemctl restart kubelet

# 创建测试 Pod
# 🟡 中风险
kubectl run nginx --image=nginx
kubectl get pod -o wide
```

---

## 调试技巧

### 1. 用 VERSION 命令快速验证二进制可用

```bash
# 🟢 低风险
CNI_COMMAND=VERSION CNI_PATH=/opt/cni/bin /opt/cni/bin/mycni
```

如果插件能正确响应 VERSION，说明二进制本身没问题（依赖库齐全、入口正确）。如果输出是规范错误 JSON，说明 skel 启动正常但调用有 bug。

### 2. 用 cnitool 手动调用插件

`cnitool`（github.com/containernetworking/cni 的 `cnitool` 子目录）可以在不启动 kubelet 的情况下手动调用 CNI，是最重要的调试工具。

```bash
# 🟢 低风险：在测试节点上
# 编译 cnitool
go install github.com/containernetworking/cni/cnitool@latest

# 创建一个测试 network namespace
# 🟡 中风险：在节点上创建 netns
ip netns add testns

# 准备配置文件
export NETCONFPATH=/tmp/cni
cat > /tmp/cni/00-mycni.conf <<'EOF'
{
  "cniVersion": "1.0.0",
  "name": "test",
  "type": "mycni",
  "bridge": "mycni0",
  "ipam": {
    "type": "host-local",
    "ranges": [[{"subnet": "10.244.0.0/24"}]]
  }
}
EOF

# 调用 ADD
# 🟡 中风险：会在节点上真的创建 veth 和配 IP
CNI_PATH=/opt/cni/bin cnitool add test /var/run/netns/testns

# 检查容器接口
# 🟢 低风险
ip -n testns addr
ip -n testns route

# 调用 DEL
# 🟡 中风险：清理创建的资源
CNI_PATH=/opt/cni/bin cnitool del test /var/run/netns/testns

# 清理
# 🟡 中风险
ip netns del testns
```

### 3. 检查 netns 与接口

Pod 的 netns 路径通常是 `/var/run/netns/cni-<container-id 前缀>-<ifname>`。在节点上：

```bash
# 🟢 低风险
# 列出所有 netns
ip netns list

# 看某个 netns 内的接口和 IP
ip -n <netns-name> addr
ip -n <netns-name> route

# 看网桥与挂载的 veth
bridge link
ip link show type bridge
ip link show type veth

# 看 host-local 已分配的 IP
ls /var/lib/cni/networks/<network-name>/
cat /var/lib/cni/networks/<network-name>/last_reserved_ip
```

### 4. 看 kubelet 与容器运行时日志

CNI 调用失败时，错误会出现在两个地方：

```bash
# 🟢 低风险
# kubelet 日志（Pod 卡在 ContainerCreating 时的第一现场）
journalctl -u kubelet --since "5 min ago" | grep -i cni
# 或
crictl logs --tail 50 $(crictl ps --name kubelet -q)

# containerd 日志（CNI 实际调用的发起方）
journalctl -u containerd --since "5 min ago" | grep -i cni
```

典型错误对照：

| 错误 | 含义 | 排查 |
|:---|:---|:---|
| `failed to find plugin "mycni" in path /opt/cni/bin` | 找不到二进制 | 确认二进制名与 conflist 的 `type` 一致，且在 `/opt/cni/bin` 下有执行权限 |
| `incompatible CNI versions` | cniVersion 不匹配 | 把 conflist 的 `cniVersion` 调成插件支持的版本（通常是 `1.0.0`） |
| `error unmarshaling JSON` | stdin 配置非法 | 用 `jq . < conflist` 校验 JSON 语法 |
| `networkPlugin mycni failed to set up pod: ...` | 插件 ADD 抛错 | 看插件 stderr（kubelet 日志里会带插件的错误消息） |

### 5. 给插件加调试日志

插件必须把日志写 **stderr**（stdout 只能是 Result）。开发时建议加一个 `DEBUG` 环境变量：

```go
import "log"

var debugEnabled = os.Getenv("MYCNI_DEBUG") != ""

func debugf(format string, a ...interface{}) {
	if debugEnabled {
		log.Printf("[mycni] "+format, a...)
	}
}

// 在 conflist 里通过 args 传 DEBUG（CNI_ARGS）或在 args 注释字段
```

然后在 kubelet 日志里 `journalctl -u kubelet | grep mycni` 就能看到详细调用轨迹。

### 6. strace 跟踪插件

```bash
# 🟡 中风险：strace 会拖慢 Pod 创建
# 找到 kubelet 进程
strace -f -e trace=execve -p $(pgrep kubelet) 2>&1 | grep mycni
# 当 Pod 被创建时会看到 execve 调用 mycni 的完整命令行
```

---

## 与现有 CNI 的对比 / 扩展点

### 生产 CNI 比示例复杂在哪

本指南的 `mycni` 只是一个**学习骨架**，它实现了"单节点 bridge + host-local IPAM"的最简模型。生产级 CNI 要复杂一个数量级：

| 维度 | mycni（示例） | Calico | Cilium |
|:---|:---|:---|:---|
| **跨节点通信** | 无（bridge 仅本机） | BGP / IP-in-IP / VXLAN | eBPF + VXLAN / Geneve |
| **策略** | 无 |iptables / eBPF, L3-L4 | eBPF, L3-L7（含 HTTP/Kafka） |
| **IPAM** | host-local 单机 | 集群级（etcd），IP 池 + 亲和 | 集群级（CRD） |
| **数据面** | Linux bridge | iptables / eBPF | eBPF（TC/XDP） |
| **可观测** | 无 | flow logs / Prometheus | Hubble（L7 flow） |
| **多集群** | 无 | 多集群 mesh | Cluster Mesh |
| **规模** | 单节点几十 Pod | 万级节点 | 万级节点 |

生产 CNI 都有自己的控制面（DaemonSet + CRD），负责把策略下发到每个节点的数据面。而 mycni 是一个纯数据面二进制，没有控制面——这也是为什么它不能跨节点。

### 基于成熟项目扩展

如果你真的需要自研能力（例如特殊的网络设备对接），优先考虑在成熟项目上扩展，而不是从零写：

- **Calico**：基于 `libcalico-go`，可以 fork 数据面或用 Felix 的 policy 引擎。
- **Cilium**：基于 eBPF，可以用 Cilium 的 CRD（CiliumNetworkPolicy）做自定义策略，或写自己的 eBPF 程序挂到 Cilium 的 hook 点。
- **plugins 仓库**：containernetworking/plugins 提供了 ptp、bridge、vlan、macvlan、ipvlan、host-device 等基础数据面实现，可以直接复用它们的 netlink 封装。

### Chained Plugin 扩展点

CNI 的 chaining 机制是天然扩展点。下面这些 chained plugin 可以叠加在任何主插件上：

| Chained Plugin | 作用 |
|:---|:---|
| `portmap` | 处理 `hostPort`，做 DNAT/SNAT |
| `bandwidth` | 用 TBF 实现入/出带宽限速（对应 Pod 的 `kubernetes.io/egress-bandwidth` 注解） |
| `firewall` | 基于 iptables 规则控制从 host 到 Pod 的流量 |
| `tuning` | 调整 sysctl（如 `net.ipv4.conf.all.rp_filter`）、MTU、MAC |
| `multus` | 多网卡支持，把多个 NetworkAttachmentDefinition 串成多个接口 |

实现自己的 chained plugin 和实现主插件没有区别——只要在 ADD 里读 prevResult、做自己的事、把 prevResult 原样或增强后输出即可。

---

## 生产实践

### 是否应该自研 CNI

**绝大多数情况：不应该**。自研 CNI 的隐性成本极高：

- 数据面正确性：bridge/vxlan/iptables 的边界条件极多（ARP、MTU、conntrack、rp_filter、ICMP redirect），没有大规模验证很难发现。
- 控制面：策略分发、IPAM 集群一致性、节点故障切换，每一项都是独立工程。
- 持续维护：内核升级、Kubernetes API 变化、新特性（dual-stack、IPv6-only、SR-IOV、eBPF）都需要跟进。
- 生态：监控、可观测、Service Mesh、Network Policy 工具默认只对接主流 CNI。

**应该自研的场景**（很窄）：
- 特殊硬件网络（SR-IOV、SmartNIC、DPDK），现有 CNI 不支持你的设备。
- 严格的私有协议网络（金融、电信专有数据链路）。
- 嵌入式/资源极受限环境，主流 CNI 太重。

即便如此，也应该**基于 plugins 仓库或 Calico/Cilium 改造**，而不是从零写。

### 如果必须自研的工程建议

1. **参考 containernetworking/plugins**：这是 CNCF 官方维护的"参考实现"，bridge/ptp/host-local 的代码都经过了多年生产验证。你的 mycni 应该直接复用 `plugins/pkg/ns`、`plugins/pkg/ip`、`plugins/pkg/ipam`、`plugins/pkg/utils` 这些包。
2. **netlink 批量操作**：使用 `netlink.Handle` 复用 socket，避免每个操作新建 netlink socket。涉及多个 link 操作时用 `netlink.Batch`。
3. **绝对不要 shell out**：不要在插件里调用 `iptables` / `ip` 命令行——慢且不可靠。用 `github.com/coreos/go-iptables/iptables` 或 `github.com/vishvananda/netlink` 的库调用。
4. **超时控制**：CNI 调用是同步阻塞的，插件里的每个外部调用（IPAM、netlink）都应有 context 超时。一个卡死的 CNI 会让整个节点 Pod 创建停滞。
5. **错误码规范**：返回错误时用 `types.NewError(code, msg, details)`，code 必须是规范定义的（如 `ErrInvalidNetworkConfig`、`ErrInternal`）。kubelet 会根据 code 决定是否重试。
6. **签名校验**：插件以 root 跑，是攻击面的关键节点。生产部署应校验二进制签名（cosign / GPG），并配置只读的 `/opt/cni/bin`。

### 测试策略

| 测试层 | 工具 | 覆盖内容 |
|:---|:---|:---|
| 单元测试 | Go `testing` + `netlink` fake | 配置解析、Result 构造 |
| 集成测试 | `cnitool` + 真实 netns | ADD/DEL/CHECK 端到端 |
| 集群测试 | kind / minikube | Pod 创建、跨节点通信、重启节点 |
| 压力测试 | 自定义脚本 | 并发 ADD/DEL、IP 泄漏检测 |
| 混沌测试 | chaos-mesh | 节点宕机、网络分区下 CNI 行为 |

集成测试模板（参考 containernetworking/plugins 的 `scripts/` 目录）：

```bash
# 🟢 低风险：在 CI 环境运行
#!/bin/bash
set -e
CNI_PATH=/opt/cni/bin
NETCONFPATH=/tmp/cni
ip netns add t1
trap 'ip netns del t1' EXIT

# ADD
cnitool add mycni /var/run/netns/t1
# 校验接口存在
ip -n t1 link show eth0
# CHECK
cnitool check mycni /var/run/netns/t1
# DEL
cnitool del mycni /var/run/netns/t1
# 校验接口已删
! ip -n t1 link show eth0
```

### 性能基线

一个写得正常的 bridge-style CNI，单次 ADD 应在 **20-50ms** 内完成（不含 IPAM）。如果超过 100ms，通常是：

- 重复创建/查询已有的 link（应缓存）；
- shell out 调用了 `ip`/`iptables`；
- netlink socket 没复用。

IPAM（host-local）单次 ADD 在 **1-5ms**（本地磁盘读写）。如果用 dhcp 或 etcd 类 IPAM，延迟会到几十 ms 量级，需要并发优化。

---

## 相关文档

- [[网络/K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构基础]] — CNI 规范、libcni、conflist 语法，本文的前置阅读。
- [[网络/K8s网络核心/03-cni-plugins-comparison.md|CNI 插件对比]] — Calico / Cilium / Flannel / Weave 架构与特性对比。
- [[网络/K8s网络核心/01-network-architecture-overview.md|K8s 网络架构总览]] — Kubernetes 网络模型整体视角。
- [[网络/网络基础/10-cni-plugin-comparison-selection.md|CNI 插件选型]] — 不同场景下选哪个 CNI 的决策矩阵。
- [[容器运行时/containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]] — kubelet 如何通过 CRI 调用容器运行时，进而触发 CNI。

### 外部参考

- **CNI SPEC**: https://github.com/containernetworking/cni/blob/main/SPEC.md
- **containernetworking/plugins**（参考实现）: https://github.com/containernetworking/plugins
- **containernetworking/cni skel 库文档**: https://pkg.go.dev/github.com/containernetworking/cni/pkg/skel
- **Linux bridge 文档**: https://wiki.linuxfoundation.org/networking/bridge
- **netlink 库**: https://github.com/vishvananda/netlink

---

> **总结一句话**：CNI 插件是一个被 kubelet→CRI→libcni 串联 exec 的 root 二进制，用 `skel` 库实现 ADD/DEL/CHECK 三个回调即可。本指南的 mycni 是理解这套机制的最小骨架——生产环境请用 Calico / Cilium，除非你有非常特殊的硬件或协议需求。

<!-- risk-assessed -->
