---
title: Wasm 云原生运行时深度实践
description: Wasm 与容器的本质差异、containerd shim/SpinKube/wasmCloud 三种上 K8s 路径对比、WASI Preview 2 组件模型、KWASM 安装与 serverless 函数部署实操、性能与安全边界深度对比
summary: Wasm 云原生运行时深度实践 — 内存沙箱模型/毫秒级冷启动/Capability-based 权限与容器的本质差异、runwasi containerd-shim-wasmtime 与 SpinKube Spin Operator 与 wasmCloud lattice 三路径对比矩阵、WASI Preview 2（wasi-http/组件模型）生态现状、containerd 配置 shim + KWASM 安装器 + serverless 函数完整 YAML、冷启动性能量级对比、Wasm vs gVisor/Kata 安全边界与故障排查
category: webassembly-cloud-native
tags:
- k8s
- wasm
- webassembly
- wasi
- containerd
- spinkube
- wasmcloud
- serverless
- sandbox
- runtime
tier: supporting
created: '2026-10-15'
last_updated: '2026-10-15'
difficulty: advanced
audience:
- 平台工程师
- 架构师
- SRE
estimated_read_time: 18min
intent_queries:
- Wasm 和容器的本质区别是什么
- Wasm 上 Kubernetes 有哪几种路径，怎么选
- containerd 如何配置 wasm shim，RuntimeClass 怎么写
- KWASM 安装器怎么用
- WASI Preview 2 组件模型解决了什么问题
- Wasm 沙箱和 gVisor Kata 比安全性如何
- Wasm 冷启动比容器快多少
trigger_keywords:
- WebAssembly
- Wasm
- WASI
- runwasi
- containerd-shim-wasmtime
- SpinKube
- KWASM
- wasmCloud
- RuntimeClass
- 组件模型
prerequisites:
- kubectl-basics
- containerd-basics
- wasm-basics
k8s_versions:
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Wasm 云原生运行时深度实践

> **版本基准（2026-10 核对）**: runwasi `containerd-shim-wasmtime/v0.6.1`、SpinKube spin-operator `v0.6.1`、wasmCloud `v2.10.3`
> **前置阅读**: [[16-专项技术/02-WebAssembly/01-wasm-fundamentals-cloud-native.md|Wasm 云原生基础]]、[[16-专项技术/02-WebAssembly/02-containerd-wasm-shim.md|containerd Wasm Shim]]

---

## 📋 目录

- [一、概述：Wasm 与容器的本质差异](#一概述wasm-与容器的本质差异)
- [二、三种上 K8s 路径对比](#二三种上-k8s-路径对比)
- [三、WASI Preview 2 与组件模型](#三wasi-preview-2-与组件模型)
- [四、生产实操：shim 配置与函数部署](#四生产实操shim-配置与函数部署)
- [五、适用场景与选型](#五适用场景与选型)
- [六、性能参考与监控指标](#六性能参考与监控指标)
- [七、安全边界：Wasm vs gVisor/Kata](#七安全边界wasm-vsgvisorkata)
- [八、故障排查与最佳实践](#八故障排查与最佳实践)
- [九、版本兼容性与参考链接](#九版本兼容性与参考链接)

---

## 一、概述：Wasm 与容器的本质差异

容器与 Wasm 都自称"轻量可移植隔离单元"，但隔离模型处于不同层次，这决定了两者在云原生栈中的分工：

| 维度 | OCI 容器（runc） | Wasm 组件 |
|------|------------------|-----------|
| 隔离机制 | Linux namespace/cgroup（进程级，共享内核） | 线性内存沙箱（实例级，验证后字节码在运行时内执行） |
| 系统调用 | 直接 syscall 宿主内核 | 无 syscall，只能经 WASI 接口显式授予（capability-based） |
| 冷启动 | 秒级（含镜像拉取可达数十秒） | 毫秒级（模块 KB-MB，内存实例化即可执行） |
| 语言栈 | 依赖 OS/ABI（glibc 兼容问题） | 语言无关（Rust/Go/JS/Python 均可编译目标） |
| 生态边界 | 40 年 POSIX 生态，全量兼容 | 受 WASI 接口面限制，生态仍在补齐 |
| 资源开销 | 每容器 MB 级起步 | 每实例 KB-MB 级 |

**本质差异一句话**：容器隔离的是"进程能看到什么"（命名空间裁剪内核视图），Wasm 隔离的是"代码能做什么"（不授权接口就什么都没有）。后者权限模型是 capability-based——能力必须在实例化时显式传入，而非"先给 root 再做减法"。

### 1.1 三点工程推论

1. **冷启动敏感的密度场景**（serverless、插件、FaaS）是 Wasm 主场；需要完整 syscall 生态的应用（数据库、JVM 大单体）短期不适合。
2. **语言无关**让"同一沙箱跑多语言策略代码"成为可能——这是传统容器给不了的扩展性（见 [[16-专项技术/02-WebAssembly/07-wasm-plugin-system.md|Wasm 插件系统]]，本文以 K8s 运行时为主）。
3. Wasm **不是替代容器**，而是容器编排体系里新增的一类"workload 类型"——K8s 的一切（Service/HPA/滚动发布）对 Wasm 照常生效，这正是下文三条路径的共同前提。

---

## 二、三种上 K8s 路径对比

### 2.1 路径 A：containerd shim（runwasi）

思路：把 Wasm 运行时做成 containerd 的一个 runtime handler，Pod 的 OCI 镜像里只放一个 `.wasm` 入口文件；kubelet 照常走 CRI，containerd 按 RuntimeClass 分流给 wasm shim，**不启动任何 Linux 进程**，由 Wasmtime/WasmEdge 直接执行模块。

```text
kubelet → CRI → containerd
                      ├─ runc shim（普通容器）
                      └─ wasmtime shim（runwasi）：解 OCI 镜像 → 取 .wasm → 运行时实例化
```

代表实现：containerd/runwasi（Rust），提供 `containerd-shim-wasmtime` 等 shim。优点是**零侵入 K8s**：RuntimeClass + 普通 Pod 即可；代价是镜像仍走 OCI 分层（虽然层很小），调度粒度是 Pod。深入机制见 [[16-专项技术/02-WebAssembly/02-containerd-wasm-shim.md|containerd Wasm Shim]]。

### 2.2 路径 B：SpinKube（Spin Operator）

思路：以 Fermyon Spin 应用为一等公民。`spin registry push` 把 Spin 应用打成 OCI 工件（主体就是 wasm 模块 + 配置）；Spin Operator 提供 SpinApp CRD，由专用 executor（containerd-shim-spin）运行。镜像分发优化方向（社区称 shuffle）：wasm 是单文件小工件，拉取路径绕开传统容器多层解压，把"镜像下载"从百 MB 级降到个位 MB 级，且免分层缓存失效问题。框架全景见 [[16-专项技术/02-WebAssembly/03-spinkube-framework.md|SpinKube 框架]]。

### 2.3 路径 C：wasmCloud（actor/lattice）

思路：不止"上 K8s"，而是自带分布式运行时。应用单元是 component（早期称 actor），节点间通过 **lattice**（基于 NATS 的消息骨干）互联，提供声明式部署、能力提供者（provider）与分布式追踪。wasmCloud 部署在 K8s 上时由 operator 接管，但**没有 K8s 也能跑**（一组 VM/边缘设备 + NATS 即成网格）——适合"边缘到云一套模型"的场景。平台详解见 [[16-专项技术/02-WebAssembly/04-wasmcloud-platform.md|wasmCloud 平台]]。

### 2.4 对比矩阵

| 维度 | A: containerd shim（runwasi） | B: SpinKube | C: wasmCloud |
|------|------------------------------|-------------|--------------|
| 工作单元 | 普通 Pod（镜像含 .wasm） | SpinApp CR（OCI 工件） | Component + provider |
| 对 K8s 侵入 | 零（RuntimeClass） | 低（Operator + shim + KWASM） | 中（自带控制模型，K8s 只是宿主） |
| 镜像/分发 | OCI 分层（层小） | OCI 工件 + shuffle 优化，极小 | registry 或 OCI 均可，lattice 内分发 |
| 应用模型 | 任意 wasm（含 WASI Preview 1 兼容负载） | Spin trigger 体系（http/redis…） | actor 消息驱动 + capability |
| 依赖运行时 | Wasmtime（可换 WasmEdge shim） | Wasmtime（spin 内嵌） | wasmtime + NATS |
| 脱离 K8s | 否 | 否（Spin 可脱离，SpinKube 依赖 K8s） | 是（lattice 自组网） |
| 典型场景 | 平台统一运行时、灰度共存 | HTTP serverless/边缘函数 | 分布式边缘云、跨环境工作负载 |
| 版本（2026-10） | containerd-shim-wasmtime v0.6.1 | spin-operator v0.6.1 | wasmCloud v2.10.3 |

**速记**：只想"用 K8s 原生方式跑 Wasm"选 A；要"函数密度 + 极致镜像分发"选 B；要"K8s 之外还有一整套分布式模型"选 C。

---

## 三、WASI Preview 2 与组件模型

### 3.1 从 Preview 1 到 Preview 2 的范式变化

WASI Preview 1 是"类 POSIX 的系统调用子集"（wasi_snapshot_preview1），暴露了 fd/clock 等内存布局耦合接口。**Preview 2（即 WASI 0.2 小世界）重写为组件模型（Component Model）**：

| 变化 | Preview 1 | Preview 2 |
|------|-----------|-----------|
| 接口定义 | C 风格 exports | WIT（WebAssembly Interface Types）声明式接口 |
| 组合方式 | 单模块 | 组件间按 WIT 接口链接（componentize，语言无关互操作） |
| 网络能力 | fd 泛化（近似 socket） | `wasi:http`（incoming/outgoing handler）一等公民 |
| 生态影响 | 各语言绑定各自为政 | wit-bindgen 统一生成多语言绑定 |

**wasi-http 是关键增量**：一个 http handler 组件可以被任何兼容运行时（wasmtime/wasmCloud/Spin）以标准方式调用——这使"一次编译，多平台运行"首次在服务端成立。组件模型深入见 [[16-专项技术/02-WebAssembly/06-wasm-component-model.md|Wasm 组件模型]]。

### 3.2 现状与迁移注意

- 主流运行时与平台（Wasmtime、WasmEdge、Spin、wasmCloud）均已稳定支持 0.2；**0.3 方向是原生 async/await**（异步接口一等公民），生态在逐步跟进。
- 遗留 Preview 1 模块（大量存量教程代码）需经 `wasm-tools component new --adapt` 适配包装才能进 0.2 世界；新项目直接用 `cargo build --target wasip2`。
- 运行时之间对 wasi 接口世界的实现仍有差异（个别 world 未实现），跨平台部署前用 `wasm-tools print --component` 检查导入面。

---

## 四、生产实操：shim 配置与函数部署

### 4.1 路径 A 实操：containerd 配置 shim + 部署函数

**第一步：节点侧注册 shim**（每类节点一次，可用 KWASM 自动化，见 4.2）：

```toml
# /etc/containerd/config.toml 追加（🟡 修改后 systemctl restart containerd）
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.wasmtime]
  runtime_type = "io.containerd.wasmtime.v1"
```

**第二步：声明 RuntimeClass 与 serverless 函数 Pod**（完整可拷贝）：

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime
handler: wasmtime
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-fn-price
  namespace: faas
spec:
  replicas: 3
  selector:
    matchLabels: {app: wasm-fn-price}
  template:
    metadata:
      labels: {app: wasm-fn-price}
    spec:
      runtimeClassName: wasmtime      # 关键：分流到 wasm shim
      containers:
      - name: fn
        image: ghcr.io/containerd/runwasi/wasi-demo-app:latest   # runwasi 官方示例，产物为 .wasm
        command: ["wasi-demo-app"]    # wasm 模块入口（镜像内即模块，无 OS 进程）
        resources:
          requests: {cpu: 10m, memory: 16Mi}
          limits:   {cpu: 500m, memory: 64Mi}
        env:
        - name: FN_MODE
          value: "http"
        ports:
        - containerPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: wasm-fn-price
  namespace: faas
spec:
  selector: {app: wasm-fn-price}
  ports:
  - port: 80
    targetPort: 8080
```

### 4.2 KWASM：社区安装器自动化 shim

KWASM operator 把"每节点装 shim + 改 containerd 配置"自动化（支持 containerd/CRI-O，覆盖裸机、K3s、云托管节点的 OS 差异）：

```bash
# 🟢 低风险：安装 KWASM operator
helm repo add kwasm http://kwasm.sh/kwasm-operator/
helm install kwasm-operator kwasm/kwasm-operator \
  -n kwasm --create-namespace

# 🟡 中风险：给节点打注解，operator 自动注入 shim（DaemonSet 完成 per-node 安装）
kubectl annotate node --all kwasm.sh/kwasm-node=true

# 🟡 中风险：安装 spin-operator（路径 B；CRD + webhook）
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.6.1/spin-operator.crds.yaml
helm install spin-operator oci://ghcr.io/spinkube/helm-charts/spin-operator \
  -n spin-operator --create-namespace
```

### 4.3 路径 B 实操：SpinApp 部署

```yaml
apiVersion: core.spinoperator.dev/v1
kind: SpinApp
metadata:
  name: hello-spin
  namespace: faas
spec:
  image: "ghcr.io/spinkube/containerd-shim-spin/examples/spin-rust-hello:v0.13.1"  # 以 quickstart 镜像 tag 为准
  replicas: 2
  executor: containerd-shim-spin     # 由 SpinAppExecutor CRD 定义的 executor
  resources: {}
```

SpinApp 会转换为节点上的 shim 执行单元，HPA/Grafana/kubectl 体验与普通 Deployment 一致。更多 serverless 形态（含 Knative/边缘网关组合）见 [[16-专项技术/02-WebAssembly/09-wasm-serverless.md|Wasm Serverless]] 与 [[16-专项技术/02-WebAssembly/11-wasm-production-deployment.md|Wasm 生产部署]]。

### 4.4 验证

```bash
# 🟢 低风险：确认 Pod 以 wasm 运行（无普通容器进程语义）
kubectl -n faas get pods -o wide
kubectl -n faas logs deploy/wasm-fn-price --tail=20

# 🟢 低风险：冷启动体感（删 Pod 后重建到 Ready 的时间）
kubectl -n faas delete pod -l app=wasm-fn-price && time kubectl -n faas rollout status deploy/wasm-fn-price
```

---

## 五、适用场景与选型

### 5.1 适用

| 场景 | 为什么是 Wasm | 路径建议 |
|------|---------------|----------|
| 边缘函数/轻量 API | 毫秒冷启动 + 小工件适配边缘带宽 | B（SpinKube）或 A |
| 插件/策略扩展（网关 WasmPlugin、OPA 替代） | capability 沙箱跑不可信第三方代码 | A + 宿主集成 |
| SaaS 多租户自定义代码（用户上传脚本） | 每 KB 级内存即可隔离一个租户 | A/C，按密度选 |
| 事件驱动 serverless（偶发流量，缩到零常态） | 冷启动不打扰 P99 | B |
| 跨云-边一套运行时模型 | 无 OS ABI 依赖 | C（wasmCloud） |

### 5.2 不适用

- **需要完整 syscall 生态的传统应用**：数据库、依赖 eBPF/内核特性的网络方案、重型 JVM 单体——WASI 接口面覆盖不足，强行迁移等于重写。
- **长驻高吞吐计算密集服务**：Wasm 与原生有稳定但可感的性能折损（数值计算 5%-30% 量级视工作负载），长驻场景冷启动收益不再，原生容器更优。
- **依赖 fork/多进程模型的组件**：组件模型是单线程（0.3 前无原生 async）、无进程模型的。

---

## 六、性能参考与监控指标

### 6.1 冷启动量级对比

| 运行单元 | 冷启动量级 | 说明 |
|----------|-----------|------|
| Wasm 组件（Wasmtime/Spin） | **1-10ms** | 模块实例化即执行，无进程/命名空间创建 |
| runc 容器 | 50-300ms | 不含镜像拉取；大镜像全链路秒级到数十秒 |
| gVisor (runsc) | ~100ms+ | 用户态内核额外开销，syscall 密集型放大 |
| Firecracker microVM | ~125ms | 官方口径，硬件虚拟化启动 |
| Kata Containers | 100-300ms | 轻量 VM + agent 初始化 |

> 量级参考而非基准：真实数字受镜像大小、网络、runtime 版本影响。要点是 Wasm 比"容器+VM"两个层级各低约一个数量级，且内存占用（KB-MB/实例 vs 10s MB/容器 vs 100s MB/VM）决定了"单节点万级实例"的密度上限。Firecracker 细节见 [[14-容器运行时/06-沙箱运行时/02-firecracker-microvm-guide.md|Firecracker microVM]]。

### 6.2 监控指标

| 指标 | 来源 | 关注点 |
|------|------|--------|
| Pod/实例启动耗时 P99 | kube-state-metrics + 自定义 | 验证冷启动收益是否兑现 |
| 每 Pod 内存 RSS | cadvisor（shim 进程视角） | Wasm 内存限制需在 RuntimeClass/资源字段双重约束 |
| wasm 模块实例数 | SpinKute/wasmCloud 自带 metrics | 密度上限规划 |
| shim 崩溃/重启 | kubelet events | 沙箱逃逸兜底（见第七章）与 OOM 信号 |
| wasi 接口拒绝计数 | 运行时日志 | capability 配置缺口的信号 |

---

## 七、安全边界：Wasm vs gVisor/Kata

### 7.1 三种隔离模型

| 维度 | Wasm 沙箱 | gVisor（用户态内核） | Kata（轻量 VM） |
|------|-----------|----------------------|-----------------|
| 隔离层 | 运行时内字节码验证 + 线性内存边界 | 拦截 syscall 到用户态内核实现 | 硬件虚拟化，独立内核 |
| 攻击面 | WASI 接口面（显式授权） | Linux syscall 面（兼容优先，面大） | 独立 guest 内核（面=内核自身） |
| 逃逸后果 | 最坏到运行时进程（常配合 seccomp 收敛） | 逃逸到 runsc sentry 宿主进程 | 需再突破虚拟化层 |
| 兼容性 | 低（WASI only） | 高（近乎任意 Linux 应用） | 高（完整 OS） |
| 性能开销 | 最低 | syscall 密集场景 10-30% | 内存/IO 有虚拟化税 |

深入对比见 [[16-专项技术/02-WebAssembly/10-wasm-security-sandbox.md|Wasm 安全沙箱]]、[[14-容器运行时/06-沙箱运行时/01-gvisor-sandbox-production.md|gVisor 沙箱生产实践]]、[[14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container.md|Kata Containers 安全容器]]。

### 7.2 组合与边界原则

1. **Wasm 不豁免纵深防御**：跑不可信代码时仍建议 Wasm Pod + seccompProfile + 只读根文件系统 + 非特权 shim 进程。
2. **最强组合**：Kata（VM 边界）内跑 Wasm（接口边界），适合高价值数据旁路的租户代码，代价是牺牲冷启动优势。
3. **运行时本身是信任根**：Wasmtime/WasmEdge 的 CVE 是整个模型的单点，运行时升级要纳入与内核同级的紧急补丁流程。
4. **capability 审计**：WIT 接口即权限清单，CI 里用 `wasm-tools` 检查导入面，拒绝多余能力，等同容器世界的"最小权限镜像"。

---

## 八、故障排查与最佳实践

### 8.1 高频故障

**现象一：CRI 报错 "runtime handler not found / RunPodSandbox failed"**

```bash
# 🟢 低风险：RuntimeClass 与节点 shim 两侧核对
kubectl get runtimeclass wasmtime -o yaml     # handler 名
crictl info | grep -A3 wasmtime               # 节点 containerd 是否注册
# 常见根因：① 节点未跑 KWASM 注入（漏 annotate）② config.toml 段名拼写错
#           ③ RuntimeClass scheduler 未正确挑选带 shim 的节点（需 nodeSelector）
```

**现象二：wasm 模块导入不兼容（incompatible imports / unknown import）**

```bash
# 🟢 低风险：检查模块导入面 vs 运行时 world
wasm-tools print myapp.wasm | grep -A2 'import'
# 常见根因：① Preview 1 模块未 componentize 适配
#           ② 目标运行时未实现该 wasi 接口（换运行时或 world 版本）
#           ③ wasi-http 与 spin trigger 接口混用
```

| 现象 | 首查（🟢） | 高频根因 |
|------|-----------|----------|
| Pod Creating 卡住 | `kubectl describe pod` 看 events | 镜像入口非 .wasm，shim 解包失败 |
| 响应空/无监听 | `kubectl logs` | wasi-http handler 未导出标准入口 |
| OOMKilled 频发 | `kubectl top pod` | 线性内存增长（语言 GC 配置）叠加 limits 过紧 |
| shim 进程重启 | 节点 `journalctl -u containerd` | 运行时 CVE 修复版待升级 / 模块触发未捕获 trap |

### 8.2 最佳实践与反模式

最佳实践：RuntimeClass 配 nodeSelector 圈定"已装 shim 的节点池"；镜像用 digest 固定；capability 清单进 CI；冷启动收益场景才选 Wasm（见第五章）。

| 反模式 | 后果 | 纠正 |
|--------|------|------|
| 把所有服务改造成 Wasm 求统一 | syscall 依赖重写成本爆炸 | 只在适配场景引入 |
| 忽略 shim 进程本身的资源约束 | 单节点 shim OOM 连坐所有实例 | 给 shim 进程预留并监控 |
| 用 latest 镜像跑 wasm 工件 | 上游 tag 漂移引发运行时不兼容 | digest 锁定 + registry 扫描 |
| 只信沙箱不做纵深防御 | 运行时 CVE 即全租户失守 | seccomp + 最小 capability + 定期运行时升级 |

---

## 九、版本兼容性与参考链接

### 9.1 版本兼容性（2026-10 核对）

- runwasi：`containerd-shim-wasmtime/v0.6.1`（`gh api repos/containerd/runwasi/releases/latest`）；shim 版本与 containerd 1.7/2.x 兼容矩阵见仓库 release notes。
- SpinKube spin-operator：`v0.6.1`（`gh api repos/spinkube/spin-operator/releases/latest`）；CRD `core.spinoperator.dev/v1`，升级先 `kubectl diff` CRD。
- wasmCloud：`v2.10.3`（`gh api repos/wasmCloud/wasmCloud/releases/latest`）；2.x 起全面组件模型化，1.x actor 需迁移。
- WASI 0.2 为当前基线，0.3（原生 async）演进中——锁定运行时版本并跟踪其 world 支持表。

### 9.2 升级检查单

1. shim 升级属节点级变更：先灰度一个节点池，观察 CRI 错误率与冷启动回归。
2. wasm 工件与运行时一起升级（接口 world 变更会造成"模块能拉起但导入失败"）。
3. KWASM 注解漂移检查：新节点接入（ Autoscaler 扩容）时确认注解策略在节点模板里。

### 9.3 参考链接与延伸阅读

- runwasi：https://github.com/containerd/runwasi
- SpinKube：https://www.spinkube.dev ；KWASM：https://kwasm.sh
- wasmCloud：https://wasmcloud.com
- Wasmtime：https://wasmtime.dev ；WasmEdge：https://wasmedge.org（运行时对比见 [[16-专项技术/02-WebAssembly/05-wasmedge-runtime.md|WasmEdge 运行时]]）
- WASI 规范与组件模型：https://github.com/WebAssembly/WASI

## Related

- [[16-专项技术/02-WebAssembly/01-wasm-fundamentals-cloud-native.md|Wasm 云原生基础]]
- [[16-专项技术/02-WebAssembly/02-containerd-wasm-shim.md|containerd Wasm Shim]]
- [[16-专项技术/02-WebAssembly/03-spinkube-framework.md|SpinKube 框架]]
- [[16-专项技术/02-WebAssembly/04-wasmcloud-platform.md|wasmCloud 平台]]
- [[16-专项技术/02-WebAssembly/06-wasm-component-model.md|Wasm 组件模型]]
- [[16-专项技术/02-WebAssembly/09-wasm-serverless.md|Wasm Serverless]]
- [[16-专项技术/02-WebAssembly/10-wasm-security-sandbox.md|Wasm 安全沙箱]]
- [[16-专项技术/02-WebAssembly/11-wasm-production-deployment.md|Wasm 生产部署]]
- [[14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container.md|Kata Containers 安全容器]]
- [[14-容器运行时/06-沙箱运行时/01-gvisor-sandbox-production.md|gVisor 沙箱生产实践]]
- [[14-容器运行时/06-沙箱运行时/02-firecracker-microvm-guide.md|Firecracker microVM]]
