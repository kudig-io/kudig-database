---
title: 容器技术本体深挖：内核隔离 · 运行时链路 · 资源管控 · 镜像工程
description: 容器技术专家视角的系统级深挖 — namespace 八种视图、cgroup v1/v2、OverlayFS 分层与 whiteout、docker run 完整链路（dockerd→containerd→shim→runc）、exec/setns、PID 1 语义、CFS quota throttling、内存账本三层、procfs 宿主机视图、镜像瘦身与构建缓存
summary: 面试与实战双用的容器本体总纲 — 共享内核下隔离三大支柱（namespace 视图/cgroup 限额/能力削权）、OverlayFS copy-up 与 whiteout 不回收原理、shim 解耦生命周期的设计动机、CFS quota 多线程毛刺的数字推演、cgroup 内存三层账本对照、free/top 显示宿主机数值的根因与 lxcfs/JVM 容器感知解法、镜像瘦身五板斧与 layer 缓存失效链
category: container-runtime
tags:
- container-internals
- namespace
- cgroup
- overlayfs
- runc
- containerd
- cri
- image-optimization
- interview
tier: supporting
created: '2026-10-20'
last_updated: '2026-10-20'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 容器工程师
- 架构师
estimated_read_time: 25min
---

# 容器技术本体深挖：内核隔离 · 运行时链路 · 资源管控 · 镜像工程

> 容器技术专家面试的**底料总纲**：从"容器是什么"下探到内核机制与运行时工程。
> 面试卡版（快问快答）见 [[36-报告/容器技术专家面试快问快答-沉淀.md|容器技术专家面试快问快答沉淀]]；
> K8s 视角的姊妹篇见 [[36-报告/k8s-高级面试快问快答-沉淀.md|K8s 高级面试快问快答沉淀]]（本文引用其题目时写作 `K8s·Q18` 形式）。

## 0. 一条主线

```
镜像（分层 tar + manifest）
  → 快照（OverlayFS lowerdir/upperdir）
    → 运行时（runc：namespace + cgroup + rootfs + exec）
      → 进程（PID 1 语义、信号、僵尸）
        → 资源（CFS quota / memory.max / PSI）
          → 可观测（procfs 全局视图 vs cgroup 视图）
```

所有面试题都是这条链上的某个截面：**"看得见什么"是 namespace，"用得了多少"是 cgroup，"从哪来"是 OverlayFS，"谁拉起来的"是 runc/containerd/shim，"为什么抖/为什么死"是资源管控，"监控为什么不对"是 procfs 视图。**

---

## 1. 内核隔离：namespace 与 cgroup

### 1.1 容器 vs 虚拟机

| 维度 | 容器 | 虚拟机 |
|------|------|--------|
| 内核 | 共享宿主机一个内核 | 每个 VM 独立内核 |
| 启动 | fork 进程级，毫秒 | 引导内核，分钟 |
| 镜像 | MB 级、分层共享 | GB 级、整盘 |
| 密度 | 单机数百 | 单机数十 |
| 隔离性 | 弱（内核 panic 全体陪葬，内核漏洞全量暴露） | 强（hypervisor 边界，内核漏洞不穿透） |

判断线：多租户强隔离 / 异构 OS → VM；高密度微服务 → 容器；既要容器体验又要 VM 隔离 → 沙箱容器（Kata = 每 Pod 一个 microVM；gVisor = 用户态内核拦截 syscall）。沙箱运行时深度对比见 [[14-容器运行时/06-沙箱运行时/03-sandbox-selection-deep-comparison.md|沙箱选型深度对比]]。

### 1.2 namespace：八种视图

| namespace | 隔离内容 | 备注 |
|-----------|----------|------|
| pid | 进程号空间 | 容器内从 1 重编 |
| net | 网络栈：接口/路由/端口/iptables | Pod 共享网络的基础 |
| mnt | 挂载点 / rootfs | pivot_root 的前提 |
| uts | hostname / domainname | |
| ipc | System V / POSIX IPC | |
| user | UID/GID 映射 | **rootless 容器的基石** |
| cgroup | /sys/fs/cgroup 的根视图 | 4.6+ |
| time | boottime/monotonic 偏移 | 5.6+ |

常用操作：

```bash
ls -l /proc/<pid>/ns/                     # 八种 namespace，同 inode = 同 ns
readlink /proc/<pid>/ns/pid               # 对比两个进程是否同 ns
unshare --pid --fork --mount-proc bash    # 玩具实验：新建 pid+mnt ns
lsns                                      # 列出系统所有 ns 及其中进程
```

### 1.3 cgroup v1 vs v2

| | v1 | v2 |
|---|---|---|
| 层级 | 每个子系统（cpu/memory/blkio…）各一棵树 | 统一单棵树 |
| 进程位置 | 可挂任意节点 | 只挂叶子（no-internal-process 约束） |
| 账本 | cpu/memory 各记各的 | 统一账本（memory.stat 打通 io） |
| 压力信号 | 无 | PSI：`cpu.pressure` / `memory.pressure` / `io.pressure` |
| 典型路径 | `memory.limit_in_bytes` | `memory.max` |

关键文件对照（v2）：

```bash
cat /sys/fs/cgroup/cpu.max          # "quota period"，如 200000 100000 = 2 核
cat /sys/fs/cgroup/cpu.weight       # 对应 v1 的 cpu.shares，调度权重非硬限
cat /sys/fs/cgroup/memory.max       # 内存硬顶
cat /sys/fs/cgroup/memory.current   # 当前账本 = anon + file(cache) + kernel
cat /sys/fs/cgroup/memory.stat      # 分账明细
cat /sys/fs/cgroup/memory.events    # oom / oom_kill 计数器，监控直接告警它
cat /sys/fs/cgroup/cpu.stat         # nr_throttled / throttled_usec，毛刺第一证据
```

K8s：cgroup v2 支持自 1.25 GA；kubelet 与 containerd 的 cgroup driver 必须一致（systemd 或 cgroupfs，不一致导致资源统计错乱与驱逐异常，见 `K8s·Q15`）。

---

## 2. 镜像与存储：OverlayFS

### 2.1 分层模型

```
merge 视图（容器看到的 rootfs）
├── upperdir  容器可写层（每容器独立）
├── lowerdir  镜像只读层 L3
├── lowerdir  镜像只读层 L2
└── lowerdir  镜像只读层 L1（基底）
```

- **读**：自上而下逐层找，upper 命中即返回。
- **写**：目标在 lower 层 → 整文件 **copy-up** 到 upper 再改（改 1 字节也要整拷几百 MB 的文件）；新文件直接进 upper。
- **删**：lower 层文件在 upper 放 **whiteout**（字符设备 0:0）遮蔽——lower 层原文件还在，**所以容器里 rm 大文件不省磁盘、镜像不会变小**。
- overlay2 是事实标准：内核原生（4.0+）、页粒度 COW、最多约 128 层 lower。

### 2.2 容器内动手验证

```bash
# 容器内
echo hi > /usr/bin/testfile && rm /usr/bin/testfile
cat /proc/mounts | grep overlay    # 看到自己的 lowerdir/upperdir/workdir

# 宿主机（按上面 upperdir 路径）
ls <upperdir>/usr/bin/   # testfile 变成 c 0,0 的 whiteout 设备文件
```

containerd 侧快照存储在 `/var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/`，按内容寻址 sha256 组织；`ctr snapshots ls` 可看全量。

镜像构建侧的层优化（多阶段构建/缓存/瘦身五板斧）见第 5 节与 [[14-容器运行时/04-镜像构建/06-image-layer-optimization.md|镜像层优化]]。

---

## 3. 运行时链路：dockerd → containerd → shim → runc

### 3.1 `docker run` 完整链路

```
docker CLI
  └─ REST over /var/run/docker.sock → dockerd          # 镜像/认证/网络/卷
       └─ gRPC → containerd                             # 常驻"容器管家"：镜像存储/快照/元数据（CNCF 毕业）
            └─ fork → containerd-shim-runc-v2            # 每容器一个，"贴身保姆"
                 └─ runc create/start                    # 一次性：
                      # 1. 建 namespace    2. 写 cgroup
                      # 3. 准备 rootfs + pivot_root
                      # 4. exec 用户进程后 runc 立即退出
```

**shim 常驻的意义（面试高频）**：
1. 容器进程的父进程是 shim 而非 dockerd——**dockerd/containerd 升级重启，运行中容器不死**（生命周期解耦）；
2. shim 收集 exit code——容器退出后 `137` 这类状态不丢；
3. shim 是 stdio 中转站——`kubectl logs` / `docker logs` / `exec` 都经它。

K8s 链路：kubelet → CRI → containerd → shim（1.24 起 dockershim 移除，不再经过 docker）。CRI 接口细节见 [[14-容器运行时/03-containerd-CRI-O/11-cri-interface-internals.md|CRI 接口内幕]]，运行时生命周期总览见 [[14-容器运行时/03-containerd-CRI-O/12-container-runtime-lifecycle.md|容器运行时生命周期]]，OCI 运行时对比见 [[14-容器运行时/03-containerd-CRI-O/04-oci-runtimes-comparison.md|OCI 运行时对比]]，控制面视角的运行时深挖见 [[01-集群基础/03-控制平面/21-container-runtime-deep-dive.md|容器运行时深挖]]。

```bash
ps -ef --forest | grep -A2 containerd-shim   # 进程树：containerd → shim → 应用
crictl ps                                     # 直连 containerd（跳过 docker）
ctr -n moby containers ls                     # dockerd 在 containerd 里的 namespace
runc spec && runc run mybox                   # 最小容器 = 一个 spec.json
```

### 3.2 `docker exec`：setns 而非"再起一个容器"

1. 找到容器主进程的宿主机 PID；
2. `open("/proc/<pid>/ns/{mnt,net,pid,ipc,uts}")` 拿 nsfd；
3. 依次 `setns()`（**user ns 必须最先**；pid ns 需 setns 后再 fork 才对子进程生效）；
4. fork-exec 目标命令，stdio 经 shim 管道接回终端。

证据：宿主机 `ps -ef` 能看到 exec 起的进程（宿主机 PID 空间有号）；容器内它 PID 从 1 空间重编。docker daemon 挂掉时的救命招：`nsenter -t <主进程PID> -m -n -p -i -u bash`。

exec 起的进程**父进程是 shim、与容器 PID 1 平级**——PID 1 不负责收它的尸；它自己产生的孤儿才会被内核 reparent 给 PID 1。

### 3.3 PID 1 的两个特权与两个坑

内核给 PID 1 的特殊语义：

1. **信号**：未注册 handler 的信号对普通进程走默认动作（SIGTERM=杀），对 PID 1 **直接丢弃**——内核假设 init 有自己的清理逻辑。shell 当 1 号位不转发 SIGTERM → `docker stop` 宽限期（默认 10s）耗尽被 SIGKILL，这就是"容器 stop 总要等满 10 秒"的经典根因。
2. **收养**：父进程先死的进程被 PID 1 收养，PID 1 必须 `wait()` 回收，不收就堆积僵尸（Z 状态）。Nginx master 当 1 号位不收 worker 僵尸是真实生产事故。

标准解法：

```dockerfile
# ① 入口脚本用 exec 让主进程顶替 shell 当 PID 1
ENTRYPOINT ["./entrypoint.sh"]     # 脚本内：exec java -jar app.jar
# ② 专业 init 当 1 号位
docker run --init ...              # 内置 tini：转发 SIGTERM + 循环 wait 收尸
```

K8s：`shareProcessNamespace: true` 后 Pod 级 PID 1 是 pause 容器，由它统一收尸（Pod 内互见进程）；pause 的完整作用见 `K8s·Q18`。

---

## 4. 资源管控与可观测

### 4.1 CPU：CFS quota 与 throttling 毛刺

机制：`cpu.max = "<quota> <period>"`（默认 period 100ms）。窗口内额度用尽 → 该 cgroup 全部 runnable 线程被 **throttle 挂起**到下一窗口。

**"limit 不高但 P99 抖"的数字推演**（面试必会）：

```
cpu.max = "20000 100000"  （0.2 核）
Go 服务跑在 96 核宿主机，未设 automaxprocs → GOMAXPROCS=96
→ 96 个 P 各跑 ~0.2ms 就烧光 20ms 额度 → 每窗口剩余 ~80ms 全组挂起
→ CPU 利用率只有 5%，P99 却出现 100ms 台阶
```

缓解次序：
1. **并发度对齐 quota**（根治）：Go `import _ "go.uber.org/automaxprocs"`；Go 1.25 起 GOMAXPROCS 默认感知 cgroup quota；JVM `-XX:ActiveProcessorCount=N`。
2. 调小 period（如 20ms）让 throttle 粒度变细。
3. 低延迟场景 CPU 独占：kubelet CPU Manager static policy（cpuset 绑核，见 `K8s·Q40`）。

红线：CPU **利用率**再低也可能 throttle——判据是 `cpu.stat` 的 `nr_throttled`，不是 CPU%。

### 4.2 内存：三层账本

| 层 | 账本 | 命令 | 超限表现 |
|----|------|------|----------|
| 进程 | RSS | `ps -o rss` | 无 |
| cgroup | memory.current | `memory.current` + `memory.stat`（anon/file/kernel） | **OOMKilled（exit 137）** |
| 节点 | MemAvailable | `free -m`、`dmesg \| grep -i oom` | **Evicted** / 内核全局 OOM |

触发链：memory.current 撞 memory.max → 内核先 reclaim（踢 page cache）→ 踢不动（涨的是 anon）→ cgroup OOM killer 杀该 cgroup 内 oom_score_adj 最大者 → 137。

要点：**page cache 计入 memory limit**——纯读写大文件/写日志都能顶满 limit，RSS 却很低（`K8s·Q2` 深挖的根因）。监控直接告警 `memory.events` 的 `oom_kill` 计数。swap：cgroup v2 `memory.swap.max`；K8s 1.28+ 经 NodeSwap 特性 beta 支持节点 swap。

### 4.3 procfs 全局视图：free/top 为什么是"错"的

`/proc/meminfo`、`/proc/cpuinfo`、`/proc/stat` 由内核按**整机**信息生成，namespace 不隔离其内容——容器里看到的是宿主机总量。

受害链（JVM 经典）：老 JVM（8u131-）按物理内存默认 heap = 机器内存 1/4 → 128G 节点上默认 32G 堆，撞 4G limit 必死。解法：

1. JDK 8u191+/10+ `UseContainerSupport`（默认开）：按 cgroup quota 算核数与内存；
2. 显式配：`-Xmx` ≤ 0.5~0.75×limit 且给堆外（metaspace/direct buffer/线程栈）留份；`-XX:MaxRAMPercentage=75`；
3. 系统级：**lxcfs**——FUSE 文件系统把容器 cgroup 值回写成它自己的 /proc 视图（阿里云容器服务老版标配）。

Go 同理：`nproc` 返回宿主机核数，GOMAXPROCS 需 automaxprocs（Go 1.25 起默认感知）。

验证三连：容器内 `free -g` / `nproc` 对比宿主机；`java -XX:+PrintFlagsFinal -version | grep MaxHeapSize`；`mount | grep lxcfs`。

---

## 5. 镜像工程：瘦身与缓存

### 5.1 瘦身五板斧

1. **多阶段构建**：`FROM golang AS build` 编译 → `FROM gcr.io/distroless/static` 只 COPY 二进制——gcc/源码/包管理器全留在 build stage。
2. **小基底**：scratch（纯静态二进制）< distroless < alpine（musl：注意 glibc 兼容与 Go DNS re-resolve 坑）< slim。
3. **同层清理**：`RUN apt-get update && apt-get install -y ... && rm -rf /var/lib/apt/lists/*` 必须同一条 RUN——分开写，删除只是 whiteout，层还在（第 2 节原理的直接应用）。
4. **缓存顺序**：层按指令序失效，依赖清单前置：

```dockerfile
COPY go.mod go.sum /app/
RUN go mod download        # 改代码不失效
COPY . .                   # 改代码只从这层起失效
RUN go build -o /app/server
```

5. **.dockerignore**：.git/测试数据/构建产物不进 build context（context 先 tar 上传给 daemon）。

数字感：典型 Go 服务 golang:1.22 基底 ≈1.2G → distroless + 静态二进制 ≈80M。Java 另有 jlink/jdeps 裁剪 JRE、AppCDS/CRaC 加速启动。

### 5.2 构建与分发的工程细节

- BuildKit 缓存挂载：`RUN --mount=type=cache,target=/root/.cache/go-build go build ...`——缓存出层不进镜像。
- 分析工具：`docker history <img>` 逐层看大小；`dive` 可视化逐层浪费。
- **registry 里的层删不掉**：层一旦被 push 或被下游镜像引用就永久存在；修法 = 改 Dockerfile 重建出新 digest + 清旧 tag + registry GC（Harbor garbage collect）。
- 分发加速（P2P/预热/mirror）见 [[14-容器运行时/02-镜像管理/09-image-distribution-acceleration.md|镜像分发加速]]；OCI 镜像规范与 artifact 见 [[14-容器运行时/02-镜像管理/10-oci-image-spec-artifacts.md|OCI 镜像规范]]。

---

## 6. 延伸追问题库（待沉淀）

| 追问 | 一句话提示 |
|------|-----------|
| rootless 容器怎么实现？ | user namespace 映射：容器内 UID 0 → 宿主机普通用户；无特权 daemon |
| 镜像 multi-arch 怎么工作？ | manifest list/index 按 os/arch 选 manifest，digest 寻址不可变 |
| 容器网络 veth/bridge 细节？ | veth pair 一端在容器 netns，一端挂宿主机网桥，出网走 NAT/路由 |
| seccomp/capabilities 削权链？ | 默认白名单 seccomp profile + drop 能力集； privileged = 全量回收 |
| overlay2 之外还有哪些存储驱动？ | btrfs/zfs 原生快照、vfs 兜底（无 COW 慢）；为什么淘汰 aufs/devicemapper |
| CRI-O 与 containerd 差异？ | 都实现 CRI；CRI-O 纯 K8s 专用守护进程，containerd 生态更广 |
| eBPF 改变了容器可观测什么？ | 不改应用、内核态采 syscall/网络事件；Falco/Tetragon/Pixie |

---

## 7. 互链

- 面试卡版：[[36-报告/容器技术专家面试快问快答-沉淀.md|容器技术专家面试快问快答沉淀（Q1–Q10）]]
- K8s 视角姊妹篇：[[36-报告/k8s-高级面试快问快答-沉淀.md|K8s 高级面试快问快答沉淀]]（`K8s·Q2` OOM 三层账本 / `K8s·Q15` cgroup driver / `K8s·Q18` pause 与 shim / `K8s·Q33` exit code / `K8s·Q40` CPU Manager）
- 相邻深挖：[[14-容器运行时/03-containerd-CRI-O/11-cri-interface-internals.md|CRI 接口内幕]] · [[14-容器运行时/03-containerd-CRI-O/04-oci-runtimes-comparison.md|OCI 运行时对比]] · [[14-容器运行时/03-containerd-CRI-O/12-container-runtime-lifecycle.md|运行时生命周期]] · [[14-容器运行时/06-沙箱运行时/03-sandbox-selection-deep-comparison.md|沙箱选型对比]] · [[14-容器运行时/04-镜像构建/06-image-layer-optimization.md|镜像层优化]] · [[19-故障诊断/02-资源排障/15-namespace-troubleshooting.md|namespace 排障]] · [[01-集群基础/03-控制平面/21-container-runtime-deep-dive.md|控制面视角运行时深挖]]
