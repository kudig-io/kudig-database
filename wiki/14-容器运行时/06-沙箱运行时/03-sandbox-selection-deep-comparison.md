---
title: 沙箱选型深度对比：分层架构、进程模型与横向对标
description: runc/gVisor/Kata/Firecracker/CubeSandbox 全景对比：容器与沙箱的本质区别、四层隔离模型、各选型宿主进程清单、性能对标与选型决策
summary: 沙箱选型深度对比：容器与沙箱的区别、四层隔离模型、执行面选型横向大表、宿主进程级 X 光对照、选型决策树
category: container-runtime
tags:
- sandbox
- gvisor
- firecracker
- kata
- runc
- containerd
- runtime
- isolation
- agent-sandbox
- cubesandbox
tier: core
created: '2026-10-14'
last_updated: 2026-10
difficulty: advanced
audience:
- SRE
- 平台工程师
- 架构师
- AI 工程师
estimated_read_time: 15min
---

> **生产环境安全提示**
>
> 本文档以选型分析为主，不含高危操作命令。引用的性能数据来自库内语料与官方 README，落地前请以自有环境实测为准。

# 沙箱选型深度对比：分层架构、进程模型与横向对标

## 概述

选沙箱不是选「最安全」，是选「你的威胁模型需要第几层墙」。本文把市面上主流方案放到同一张坐标系里：先分清容器与沙箱的区别，再按隔离层次建立全景，然后用进程级视角看清每个选型在宿主机上到底多了什么，最后给出横向对标大表与决策树。

相关文档：[[wiki/14-容器运行时/06-沙箱运行时/01-gvisor-sandbox-production.md]]、[[wiki/14-容器运行时/06-沙箱运行时/02-firecracker-microvm-guide.md]]、[[wiki/14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container.md]]、[[wiki/14-容器运行时/03-containerd-CRI-O/06-gvisor-sandbox-runtime.md]]、[[wiki/15-AI基础设施/03-Agent运行时/12-agent-sandbox-isolation.md]]、[[wiki/15-AI基础设施/01-基础设施/37-agent-sandbox-security.md]]

## 1. 容器和沙箱是什么关系

很多人把两者混着说，其实一个是**跑法**，一个是**目的**。

### 1.1 容器的本质

容器是内核几项隔离机制的打包：**namespaces**（看到的不一样）+ **cgroups**（用多少我限着）+ rootfs（各自一套文件系统）。关键：**容器没有自己的内核**，所有容器进程直接调用宿主机那颗真内核——常被叫「合租」，墙只是一层 namespace。

而且**隔离不是容器的本职**。Docker 的卖点是打包分发（「一次构建到处跑」），隔离是顺手捎带的。它的威胁模型是「防误事」，不是「防坏人」。

### 1.2 沙箱的本质

沙箱是**目的性定义**：为了安全执行**不可信代码**（来路不明的依赖、用户上传的脚本、LLM 现场生成的代码）专门围出来的隔离区。防的是**恶意逃逸**，不是误操作。

| | 容器（runc） | 沙箱 |
|---|---|---|
| 目的 | 打包分发，环境一致 | 安全执行不可信代码 |
| 防谁 | 防误事、防越界 | 防恶意逃逸 |
| 内核 | 直接用宿主真内核 | 挡一层（Sentry）或整台 VM |

两句话定位：

- **容器是实现沙箱的一种原料**——「Docker 容器 + 非 root + seccomp + AppArmor」就是拿容器凑出来的第 1 层洋葱。
- **沙箱不一定是容器**——Firecracker 里跑的是微虚拟机，E2B/CubeSandbox 是独立服务，跟 containerd 都没关系。

## 2. 隔离的四层坐标（全景）

语料中的隔离手段分布在四个层次，像洋葱一层层裹：

| 层 | 手段 | 选手 | 特点 |
|---|---|---|---|
| 1. 进程层 | 非 root、seccomp、AppArmor、cgroup | runc、nsjail | 便宜，和内核只隔一层膜 |
| 2. 用户态内核层 | 拦截系统调用，自研假内核 | **gVisor** | 不用 KVM，中间地带 |
| 3. 虚拟机层 | 硬件虚拟化，独立 guest 内核 | **Kata、Firecracker、CubeSandbox** | 最厚的墙 |
| 4. 托管服务层 | 隔离外包给云 | E2B、Modal | 省心，代价是数据出网 |

补充路线（语料 `wiki/16-专项技术/02-WebAssembly`）：**语言级沙箱——Wasm**。它在指令集层面设卡：线性内存每次访存都过验证，WASI 零权限原则默认啥也不给，没有系统调用入口。已可通过 containerd-wasm-shim / SpinKube 接入 K8s RuntimeClass。用兼容性（只能跑可编译成 Wasm 的语言）换密度。

37 号文档将 Agent 沙箱归纳为四种架构模式，第 12 节给出推荐选型对照。

## 3. 分层澄清：谁和谁才是竞品

横向对比前必须先分层，否则会犯迷糊：

```
用户敲 docker run
   ↓
dockerd        ← 管家（解析命令、管镜像）
   ↓
containerd     ← 大管家（镜像分发、生命周期；CRI 时代 K8s 直连它）
   ↓ 通过 OCI spec 调用
真正开跑的地方 ← runc / gVisor(runsc) / Firecracker   【三选一，同层竞品】
CubeSandbox    ← 另一桌席：自带控制面+数据面的整套沙箱服务
```

- **runc、gVisor、Firecracker 是同一层的三个「司机」**；
- **dockerd/containerd 是「车管所」**——换司机不换车管所。这正是库内三篇沙箱文档都出现「containerd 接入 + RuntimeClass」的原因：上层不变，下层换跑法。
- **CubeSandbox 连锅端**：不经过 dockerd/containerd，自带控制面（模板、快照、凭证代理）+ 数据面（微 VM）。

## 4. 启动流程：gVisor 容器在宿主机上发生了什么

「先启动容器，再启动 gVisor」的说法方向对但顺序反了——**gVisor 就是那个容器的「内核」，一开始就得在**：

```
ECS（宿主机，真内核 Linux）
├─ dockerd ──→ containerd
│                 │ ① 收到起容器请求，查配置发现 runtime = runsc
│                 ↓
│              ② 执行 runsc（gVisor 启动器，拉完手就退）
│                 ↓
│              ③ 宿主机上拉起三个关键进程：
│                 ┌─────────────────────────────┐
│                 │ Sentry   ← Go 写的「假内核」   │
│                 │ Gofer    ← 代管文件系统的门房  │
│                 │ 你的应用  ← 真正的业务进程      │
│                 └─────────────────────────────┘
│                 应用的所有系统调用都发给 Sentry，
│                 Sentry 过滤后再决定要不要碰真内核
```

三个要点：

1. 「启动 gVisor 容器」= containerd 调 `runsc` 而非 `runc`。宿主机上**没有多出一台 VM**，只是多了几个普通进程。
2. 应用进程生下来就被放在 Sentry 后面，从第一个系统调用起就是在跟 Sentry 说话，**从没见过真内核**。
3. 对照 Firecracker：那才是「先起一台微 VM 再往里塞容器」——宿主会多出一个 KVM 虚拟机进程。gVisor 省掉了真开机开销，启动 ~50ms。

验证方法（01 号语料）：进 gVisor 容器看 `/proc`，「不再是真实宿主内核视图」——那是 Sentry 伪造的假 `/proc`。

## 5. 进程级 X 光：各选型下宿主机的进程清单

### 5.1 标准容器（dockerd + containerd + runc）

```
systemd
├─ dockerd                    ← 仅 docker CLI 环境有；K8s 通常没有
├─ containerd                 ← 常驻
├─ containerd-shim            ← 每容器一个，兜底托管
└─ 你的应用进程 (PID 1)        ← 直接跑在宿主内核上（原生进程，零额外开销）
（runc 不常驻！只在创建容器那几秒存在，起完就退）
```

### 5.2 gVisor（runsc）

```
systemd
├─ dockerd / containerd / containerd-shim     ← 上层不变
├─ runsc (bootstrap)    ← 启动器，拉完就退
├─ Sentry               ← 「假内核」，常驻
├─ Gofer                ← 文件访问门房，常驻
└─ 你的应用进程          ← 仍是宿主普通进程，但 syscall 全部递交 Sentry
```

比方案 5.1 **多两个长驻进程（Sentry、Gofer）**，没有 VM。

### 5.3 Firecracker（firecracker-containerd）

```
systemd
├─ containerd / containerd-shim-runc-v2
├─ firecracker            ← VMM 主进程（虚拟设备、vsock、网络）
├─ [KVM]                  ← 内核线程，虚拟机的「物理机」
│   └─ Guest 内核 (Linux)
│       └─ 容器 init
│           └─ 你的应用    ← 宿主 ps 数不到它
```

宿主上多出 **firecracker 进程 + 一整个 guest 内核**。应用在宿主进程表里**隐身**——从宿主看它是一台小电脑，从它往外看是一整台 VM。

### 5.4 Kata Containers

```
systemd
├─ containerd / containerd-shim-kata
├─ kata-runtime           ← Kata 管理面（比 firecracker 多这层）
├─ qemu（或 cloud-hypervisor / firecracker）
│   └─ Guest 内核
│       └─ 你的应用（隐身，同上）
```

结构同 Firecracker，但多了 kata-runtime 这层容器语义管理——这正是「Kata 换 Firecracker 后端仍比裸 Firecracker 慢一档」的开销来源。

### 5.5 CubeSandbox

```
systemd
├─ cube 控制面/数据面组件    ← 模板、快照(CubeCoW)、凭证代理
├─ RustVMM 进程            ← 每沙箱一个微 VM（KVM 路线）
│   └─ Guest → Agent 代码（隐身）
（不经过 dockerd/containerd）
```

### 5.6 对照总表

| 选型 | 宿主常驻（除管理面） | 应用在宿主 ps 里？ | 内核数 |
|---|---|---|---|
| runc | containerd-shim | ✅ 可见 | 1（真） |
| gVisor | shim + **Sentry + Gofer** | ✅ 可见（但 syscall 被接管） | 1 真 + 1 假（进程态） |
| Firecracker | firecracker | ❌ 隐藏在 guest | 1 真 + 1 真（VM 里） |
| Kata | kata-runtime + qemu/CL/FC | ❌ 隐藏在 guest | 1 真 + 1 真 |
| CubeSandbox | cube 组件 + RustVMM | ❌ 隐藏在 guest | 1 真 + 1 真 |

> **进程表就是隔离强度的 X 光片**：runc 应用直接踩真内核；gVisor 中间隔了个 Sentry 进程；VM 系中间隔的是一整颗 guest 内核 + hypervisor。

## 6. 隔离原理对决：三条路线的修墙思路

三家要防的都是「不可信代码碰到真内核」，思路截然不同：

- **gVisor——不修墙，换个假前台。** 系统调用全由 Sentry 接盘翻译过滤，真内核攻击面从几百个 syscall 入口缩到一条窄缝。代价是所有调用多绕一圈，性能损耗 10-30%。
- **Kata——每人一间独立房间。** 每个 Pod 一台轻量 VM，有自己的独立内核。打穿容器脚下也只是 VM 内核。硬件虚拟化（VMX）是最硬的隔离边界，代价是「装修」：qemu 启动 ~1s、内存 ~50-130MB。
- **Firecracker——Kata 的表亲，极简主义。** 同样 KVM 微 VM，但砍掉大部分设备模拟——内存 ~5MB、启动 <125ms，**VM 级隔离 + 近容器级速度**，高密度 Serverless 的立身之本。

## 7. 横向对标大表

### 7.1 核心指标（隔离 × 性能 × 成本）

| 维度 | runc | nsjail | gVisor | Kata(QEMU) | Kata(Cloud HV) | Kata(FC后端) | Firecracker | Wasm | E2B/Modal |
|---|---|---|---|---|---|---|---|---|---|
| 隔离路线 | 进程层 | 进程层 | 用户态内核 | 轻量 VM | 轻量 VM | 轻量 VM | 微 VM | 语言级 | 服务边界 |
| 内核 | 共享宿主 | 共享宿主 | 假内核(Sentry) | 独立 VM | 独立 VM | 独立 VM | 独立 VM | 无内核概念 | 远端机器 |
| 隔离强度 | 弱 | 弱-中 | 中 | 强 | 强 | 强 | 强 | 中-强* | 强(边界在云) |
| 冷启动 | ~10-200ms | 毫秒级 | ~50-100ms | **~1.5s** | ~800ms | ~500ms | **<125ms** | 毫秒级 | 秒级(网络) |
| 内存开销 | ~0-10MB | 极低 | ~15MB | **~130MB** | ~80MB | ~50MB | **~5MB** | 极低 | 不占本地 |
| CPU 开销 | 基准 | 基准 | 10-30% | +5-10% | +3-5% | +2-3% | 低-中 | 极低 | 不占本地 |
| I/O 损耗 | 基准 | 基准 | 中(经Gofer) | virtiofs -5~15% | 同左 | 同左 | 中 | N/A | 网络决定 |
| 硬性依赖 | 无 | 无 | **不需 KVM** | KVM+QEMU | KVM | KVM | **仅 KVM** | Wasm 工具链 | 无 |
| 运维成本 | 低 | 低 | 中 | 高 | 高 | 高 | 中 | 中 | 零(付费) |
| 数据主权 | 本地 | 本地 | 本地 | 本地 | 本地 | 本地 | 本地 | 本地 | **出机器** |

\* Wasm 强在指令集层无系统调用入口，弱在只能跑可编译成 Wasm 的语言。

数据来源：01 号语料「隔离模型对比」、02 号语料「隔离层级对比」、05 号语料 §3「性能开销分析」（Kata 后端细分）。

Kata I/O 细分（05 号语料）：virtiofs 相对 runc 顺序读写 -5~10%、随机读 -10~15%、网络 -3~5%；9pfs 差得多（-30~60%），生产选 virtiofs。

### 7.2 执行面四选一（runc / gVisor / Firecracker / CubeSandbox）

| 维度 | runc | gVisor (runsc) | Firecracker | CubeSandbox |
|---|---|---|---|---|
| 是什么 | OCI 运行时 | OCI 运行时（沙箱型） | VMM 微虚拟机监视器 | Agent 沙箱**整套服务** |
| 出处 | Open Containers | Google | AWS | 腾讯云 2026 开源 |
| 隔离路线 | namespace/cgroup | 用户态内核 | KVM 硬件虚拟化 | RustVMM + KVM |
| 启动 | ~10-200ms | ~50-100ms | <125ms | **~60ms**（官方） |
| 内存开销 | ~0 | ~15MB | ~5MB | **≤5MB**（官方） |
| 硬性依赖 | 无 | **无 KVM 也行** | 必须 KVM | 必须 KVM |
| 兼容性 | 100%（标准本体） | 有不支持的 syscall | 镜像需打成 VM rootfs | E2B SDK 兼容 |
| 接入 containerd | 默认 runtime | 加 runsc handler + RuntimeClass | firecracker-containerd + RuntimeClass | **不走 containerd**，独立控制面（v0.6 起可部署进 K8s） |

CubeSandbox 官方数据：60ms 创建硬件隔离沙箱、内存 ≤5MB、兼容 E2B SDK；差异化不在跑得快，而在服务层能力——CubeCoW 快照/克隆/回滚、跨节点暂停恢复、凭证托管（Key 不进沙箱）、模板管理。与裸 Firecracker（<125ms/5MB）数字接近，本质是 RustVMM 系微 VM 的工程优化版 + 服务化包装。

### 7.3 选型决策树（语料原文取舍 + Agent 场景）

| 场景 | 最优解 | 理由 |
|---|---|---|
| 不可信代码、最高安全 | **Firecracker** | 最强隔离+快启动+高密度，Agent 沙箱黄金标准 |
| 普通 ECS、无 KVM | **gVisor** | 唯一不依赖硬件虚拟化的中间路线 |
| 已有 QEMU 工具链、要完整 VM 能力 | **Kata** | 复用生态，hypervisor 后端可换 |
| 高密度 Serverless、毫秒级冷启 | **Firecracker** | <125ms + 5MB，独一份 |
| Bash 短命令、要秒起 | **nsjail + gVisor 双重隔离** | 37 号语料推荐表 |
| Python 代码执行、兼容性优先 | **gVisor 容器** | 兼容好、隔离强 |
| K8s 多租户平台、免自研 | **CubeSandbox / ACK 沙箱容器** | 产品化开箱即用 |
| 不想运维隔离设施 | **E2B / Modal** | 代价是数据出网 |
| 超高密度、语言可控 | **Wasm (WasmEdge/SpinKube)** | RuntimeClass 可接 K8s |

取舍一句话（02 号语料原文）：

- 要最强隔离 + 快启动 + 高密度 → Firecracker（需 KVM）
- 无 KVM（普通 ECS）+ 中等隔离 → gVisor
- 已有 QEMU 工具链 + 需完整 VM 能力 → Kata

## 8. 国内厂商开源格局

国内格局 = **深度参与国际开源 + 自研产品化**，直接开源整个沙箱运行时的不多：

| 厂商 | 项目 | 形态 | 路线 | 说明 |
|---|---|---|---|---|
| 蚂蚁 | Kata Containers | 开源上游 | 轻量 VM | Kata 核心贡献者，前身含国内 HyperHQ runV |
| 腾讯云 | **CubeSandbox** | 全栈开源 (Apache 2.0) | RustVMM + KVM | 2026-04 开源，CNCF Landscape 收录，兼容 E2B SDK |
| 阿里云 | ACK 沙箱容器 2.0 | 商业产品 | 自研轻量 VM | 2025 大版本，overhead 降 90%；runc/gVisor/Kata/自研按 RuntimeClass 切换 |
| 火山引擎 | AgentKit Sandbox | 商业产品 | Agent 专用 | 八大模块之一，未完整开源 |
| 百度 | — | — | — | 暂未查到有影响力的开源沙箱项目 |

## 9. 常见误区速查

| 误区 | 正解 |
|---|---|
| 「用了 Docker 就是用了沙箱」 | 容器防误事，沙箱防恶意逃逸；裸 runc 防不住内核漏洞逃逸 |
| 「先起容器，再起 gVisor」 | gVisor 是容器的「内核」，随容器一起诞生，Sentry 先于应用存在 |
| 「gVisor 是在容器外套一层 VM」 | 没有 VM，Sentry 是宿主上的用户态进程 |
| 「Kata 和 Firecracker 是纯竞品」 | Kata 可把 Firecracker 当 hypervisor 后端（05 号语料 §2.2） |
| 「换 gVisor 要换 dockerd/containerd」 | 只加 runsc handler + RuntimeClass，车管所不动 |
| 「隔离强度都能调到 VM 级」 | 共享内核的路线怎么调都到不了 VM 级，原理决定强度上限 |

## See Also

- [[wiki/14-容器运行时/06-沙箱运行时/01-gvisor-sandbox-production.md]]
- [[wiki/14-容器运行时/06-沙箱运行时/02-firecracker-microvm-guide.md]]
- [[wiki/14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container.md]]
- [[wiki/14-容器运行时/03-containerd-CRI-O/06-gvisor-sandbox-runtime.md]]
- [[wiki/15-AI基础设施/03-Agent运行时/12-agent-sandbox-isolation.md]]
- [[wiki/15-AI基础设施/01-基础设施/37-agent-sandbox-security.md]]
- [[wiki/16-专项技术/02-WebAssembly/10-wasm-security-sandbox.md]]
