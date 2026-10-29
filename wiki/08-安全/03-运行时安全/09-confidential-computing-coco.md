---
title: 机密计算与 Confidential Containers 落地指南
description: 机密计算威胁模型、TEE 硬件对比（TDX/SEV-SNP/CCA/H100 CC-mode）、Kata Containers 与 CoCo 架构（peer-pod/attestation-agent/KBS）、K8s RuntimeClass 集成、CoCo operator 部署实操、加密镜像、性能开销与落地路线图
summary: 在不可信宿主机上运行加密内存工作负载 — 覆盖 TEE 威胁模型与四大硬件方案对比、Kata+CoCo 组件架构与远程证明流程、RuntimeClass/加密镜像/KBS 注解集成、CoCo operator v0.17.0 生产部署实操、故障排查、性能基准讨论与分阶段落地路线图
category: cloud-native-security
tags:
- k8s
- security
- kata-containers
- confidential-containers
- tee
- sev-snp
- tdx
- attestation
- encryption
- containerd
- production
tier: supporting
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: expert
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- 机密计算与 Confidential Containers 落地指南 是什么
- 如何在 Kubernetes 上部署机密容器
- confidential containers 部署与远程证明怎么做
- TDX SEV-SNP CCA 机密计算如何选型
trigger_keywords:
- 机密计算
- Confidential Containers
- CoCo
- TEE
- 可信执行环境
- SEV-SNP
- TDX
- 远程证明
- attestation
- KBS
- Kata Containers
- 机密 AI
prerequisites:
- kubectl-basics
- security-basics
- container-runtime-basics
- containerd-basics
- kvm-basics
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# 机密计算与 Confidential Containers 落地指南

> **适用版本**: CoCo operator v0.17.0 / Kata Containers 4.2.0（2026-10-14 经 GitHub Releases 核对）
> **硬件前提**: Intel TDX（Xeon 4 代起）/ AMD SEV-SNP（EPYC 7003/9004 系起）/ ARM CCA / NVIDIA H100 CC-mode
> **难度**: expert — 涉及硬件固件、虚拟化层与 K8s 三层协同

---

## 目录

- [一、概述与核心概念](#一概述与核心概念)
- [二、TEE 硬件技术对比](#二tee-硬件技术对比)
- [三、架构：Kata Containers 与 CoCo](#三架构kata-containers-与-coco)
- [四、Kubernetes 集成方式](#四kubernetes-集成方式)
- [五、生产部署实操](#五生产部署实操)
- [六、故障排查与诊断流程](#六故障排查与诊断流程)
- [七、最佳实践与反模式](#七最佳实践与反模式)
- [八、性能开销与监控](#八性能开销与监控)
- [九、典型用例与落地路线图](#九典型用例与落地路线图)
- [十、版本兼容性与升级注意](#十版本兼容性与升级注意)
- [十一、参考链接与延伸阅读](#十一参考链接与延伸阅读)

---

## 一、概述与核心概念

### 1.1 什么是机密计算

机密计算（Confidential Computing）指在**基于硬件的可信执行环境（TEE, Trusted Execution Environment）**中执行计算，使工作负载的代码与数据在运行期间保持机密性（内存加密）与完整性（度量与远程证明），即使**宿主机操作系统、Hypervisor 甚至拥有物理访问权的运维人员**也无法窥视其内存内容。

在 Kubernetes 语境下，落地机密计算的路径是把容器放进**硬件隔离的轻量虚拟机**中运行——这正是 Kata Containers 与 CNCF 沙箱项目 **Confidential Containers（CoCo）** 的结合点：Kata 提供硬件级隔离的 Pod VM，CoCo 提供机密化所需的镜像加密、远程证明与密钥分发组件。

### 1.2 威胁模型：host 不可信

传统容器安全（见 [[wiki/08-安全/03-运行时安全/04-runtime-security-defense.md|运行时防御]]）假设宿主机与集群管理员是可信方，防护对象是容器内进程逃逸。机密计算**反转了这一假设**：

| 威胁方 | 传统模型 | 机密计算模型 |
|--------|----------|--------------|
| 宿主机内核（被入侵） | 可信 | **不可信**（移出 TCB） |
| Hypervisor / VMM | 可信 | **不可信**（移出 TCB） |
| 云厂商运维/管理员 | 可信 | **不可信**（只能看到密文） |
| 物理攻击（内存冷启/探针） | 不防护 | **防护**（内存加密） |
| 同宿主机恶意租户 | 依赖隔离强度 | 防护（独立加密域） |
| 网络链路窃听 | 需 TLS | **仍需 TLS**（不因 TEE 而豁免） |
| 宿主机发起的 DoS/资源压制 | 不防护 | **不防护**（可用性仍由 host 掌控） |

**TCB（Trusted Computing Base）收缩**是核心收益：TCB 从「宿主内核 + hypervisor + 容器运行时」收缩为「CPU 固件（PSP/TEE firmware）+ guest 内核 + guest 内应用」。代价是新增信任根：必须信任 CPU 厂商固件与硬件根密钥体系。

### 1.3 机密计算的边界（不要神化）

- **机密性 ≠ 可用性**：宿主机仍可随时杀死/驱逐你的 Pod VM；
- **侧信道**：内存加密不消除所有侧信道（缓存计时、页面级访问模式等仍需应用层防护）；
- **I/O 边界**：磁盘与网络数据离开 TEE 后仍是明文，持久卷必须独立加密（见 [[wiki/08-安全/03-运行时安全/07-security-context-fields-reference.md|Security Context 字段参考]] 中的存储加密配合）；
- **证明是前提**：未做远程证明前，"运行在 TEE 里" 只是自我声明。

---

## 二、TEE 硬件技术对比

### 2.1 四大硬件方案总览

| 维度 | Intel TDX | AMD SEV-SNP | ARM CCA | NVIDIA H100 CC-mode |
|------|-----------|-------------|---------|---------------------|
| 全称 | Trust Domain Extensions | Secure Encrypted Virtualization - Secure Nested Paging | Confidential Compute Architecture | Hopper Confidential Computing |
| 隔离单元 | Trust Domain (TD) | 加密虚拟机 | Realm（领域） | GPU 与 CPU TEE 的组合通道 |
| 内存保护 | 多钥加密 + 完整性元数据表 | 每 VM 独立 ASID 加密密钥 + RMP 反篡改页表 | Granule Protection Table + Realm 管理监视器 (RMM) | 保护内存区域 + CPU↔GPU 加密隧道 |
| 度量/证明 | TDREPORT（Quote 需 SGX DCAP 服务签发） | ATTESTATION_REPORT（固件直接签发） | Realm Token + Platform Token | GPU 证明报告，与 CPU TEE 报告绑定 |
| 内核要求 | Linux ≥ 6.8（KVM TDX 支持） | Linux ≥ 6.8（KVM SNP 支持） | 依赖 Armv9-A RME 扩展 | 主机与 guest 驱动 + nvtrust 工具 |
| K8s 生态成熟度 | 高（CoCo/Kata 主力支持） | 高（CoCo/Kata 主力支持 + 云厂商 SEV 实例） | 追赶中（软件栈仍在完善） | 高（机密 AI 主流方案） |
| 典型硬件 | Xeon 4 代（Sapphire Rapids）及后续 | EPYC 7003/9004/9005 系 | Armv9-A RME 服务器 SoC | H100/H200 + SEV-SNP 或 TDX 平台 |

### 2.2 CPU TEE 关键机制

- **内存加密引擎**：TDX 用 AES-XTS 多钥架构（MKTME），每个 TD 独立密钥；SEV-SNP 用内存加密控制器为每 VM 分配 ASID 绑定的密钥。DRAM 中的内容对 PSP/固件以外均为密文。
- **完整性保护**：SEV-SNP 的 RMP（Reverse Map Table）阻止 hypervisor 篡改 guest 物理页映射（防重映射/重放）；TDX 通过 Secure-EPT 与完整性元数据表实现类似保证。这是 SNP/TDX 相对早期 SEV/SEV-ES 的关键分水岭——早期方案只有加密没有完整性。
- **度量链**：从固件度量 guest 内核镜像、initrd、内核命令行，形成 launch digest / TD measurement。远程证明时第三方（KBS）核对度量值是否匹配预期（防篡改启动）。

### 2.3 GPU 机密计算：H100 CC-mode（机密 AI）

机密 AI 推理的参考拓扑：**CPU TEE（SEV-SNP 或 TDX）运行应用与模型加载逻辑，GPU 处于 CC-mode，两者间经硬件加密通道传输**：

1. 管理员用 NVIDIA nvtrust 工具（`nvidia-confidential-compute`）将 GPU 切入 CC-mode（需重启生效）；CC-mode 下 **MIG 不可用**，且受信任内存（TOM）截断会小幅减少可用显存/BAR 空间。
2. GPU 驱动运行在 guest VM 内，CPU↔GPU 的命令流与数据经硬件加密隧道，宿主机与 PCIe 链路只见密文。
3. GPU 生成自己的证明报告并与 CPU TEE 度量绑定，模型权重解密密钥经 CPU TEE 的 KBS 链路下发——实现「模型在显存中始终密文/受保护、按需解密」。
4. H100 CC-mode 与 vLLM/TensorRT-LLM 等推理栈配合已是公有云机密推理服务的主流实现；对延迟敏感的小 batch 场景相对开销更明显（见[八、性能开销](#八性能开销与监控)）。

### 2.4 硬件选型要点

- 存量 x86 集群优先盘点 CPU 代际：SEV-SNP 需 EPYC 7003+ 且 BIOS 开启；TDX 需 Xeon 4 代+；两者都要求内核与 KVM 模块较新。
- ARM 服务器占比高的集群关注 CCA/RME 软件栈演进节奏，短期生产落地仍以 x86 + TDX/SNP 为主。
- 机密 AI 场景按「GPU 型号（H100/H200 及后续支持 CC 的卡）+ CPU TEE 平台」组合选型，并确认所用推理框架版本支持 guest 内驱动模式。
- 公有云优先选择厂商的机密虚拟机/机密容器托管形态（如 SNP/TDX 实例），可省去固件与内核运维。

---

## 三、架构：Kata Containers 与 CoCo

### 3.1 Kata Containers 架构

Kata 用轻量 VM 承载 Pod，容器进程运行在独立 guest 内核中（对比 gVisor 的用户态内核方案，见 [[wiki/08-安全/03-运行时安全/06-gvisor-container-sandbox.md|gVisor 沙箱]]）：

```text
kubelet → containerd (CRI)
             └─ containerd-shim-kata-v2        # shim 进程（宿主机侧，每个 Pod 一个）
                  └─ kata-runtime (VMM)         # QEMU / cloud-hypervisor / runtime-rs
                       └─ Pod VM (guest)
                            ├─ guest kernel（专用裁剪内核镜像）
                            ├─ kata-agent       # guest 内 ttrpc 服务，替代 guest 内的容器运行时
                            └─ container#1..N   # 普通 OCI 容器（runc 语义，共享 Pod VM）
```

要点：

- **一 Pod 一 VM**：Pod 内容器共享 guest 内核，Pod 间硬件级隔离；
- **kata-agent 是机密化枢纽**：它内置 **attestation-agent (AA)** 与 **KBC（Key Broker Client）**，负责在 guest 内完成证明与取回密钥；
- **镜像拉取与解压位置**：传统 kata-qemu 模式下镜像层由宿主机 shim 侧拉取并以 virtio-blk/9p 注入 guest——CoCo 通过加密镜像（见 3.6）保证宿主机只见密文层。

### 3.2 Confidential Containers（CoCo）项目

CoCo 是 CNCF 沙箱项目（`confidential-containers` GitHub 组织），由 Intel/AMD/Red Hat/IBM 等推动，目标是让 K8s 用户**以最小改动**（换 RuntimeClass + 加密镜像）获得机密 Pod：

- **coordinator / operator**：K8s operator（本文 v0.17.0），在节点安装机密化 Kata 运行时或部署 peer-pod 能力；
- **guest 组件**：attestation-agent、image-rs（加密镜像解密）、kata-agent；
- **trustee 仓库**：Key Broker Service（KBS）、Attestation Service（AS）、Reference Value Provider Service（RVPS）及 `kbs-client` CLI；
- **tests / demos 仓库**：一致性测试与场景示例。

### 3.3 关键组件：attestation-agent / KBC / KBS

| 组件 | 位置 | 职责 |
|------|------|------|
| attestation-agent (AA) | guest（kata-agent 内） | 证明编排：生成证据 → 提交 KBS 获取 token → 凭 token 取密钥/配置 |
| KBC | guest | Key Broker Client，与特定 KBS 协议实现（如 `cc_kbc`）对接 |
| KBS | 集群内/独立服务（trustee） | 鉴别TEE 证据 → 签发 token → 按策略下发资源（密钥、配置、镜像解密钥） |
| AS (Attestation Service) | trustee 内 | 校验证据（Quote/Report）与参考值比对，产出 Attestation Results |
| RVPS | trustee 内 | 存储与管理参考值（launch digest、TCB 版本下限等） |

### 3.4 远程证明与密钥分发流程

```text
1. Pod 调度到 kata 节点，Pod VM 启动（guest 内核/镜像/initrd 均被度量）
2. kata-agent 以 kernel_params 中的 aa_kbc_params（如 cc_kbc::https://kbs...）发起连接
3. AA 收集硬件证据：
   - SNP: 向固件请求 ATTESTATION_REPORT（含 launch digest、TCB 版本）
   - TDX: 读取 TDREPORT，经 Quote 生成服务签发 Quote
4. KBS → AS 校验签名/证书链/反重放 nonce，与 RVPS 中参考值比对
5. 校验通过 → KBS 返回 attestation token
6. AA 凭 token 请求资源（镜像解密钥、应用密钥），image-rs 解密镜像层，容器启动
7. （可选）应用经 AA API 获取业务密钥（密钥不出 KBS→TEE 之外的明文域）
```

**失败即拒绝启动**：证明不通过时镜像解密钥不下发，Pod 停留在拉取/解密阶段——这是「加密镜像 + KBS」模式的强制执行点。

### 3.5 peer-pod 模式

peer-pod（peer-pods）解决「存量虚拟机/云上节点内核不支持 TEE KVM」的场景：Pod VM 运行在**云上的独立 Pod 虚拟机实例**（libvirt/AWS/IBM Cloud 等远程 hypervisor），K8s 节点上仅运行轻量 shim：

- K8s 节点不必支持嵌套虚拟化或 TEE 固件，Pod VM 落在专门的可信节点池；
- `runtimeClassName: kata-remote`（operator 开启 `enablePeerPods` 后创建）；
- 注意：peer-pod 下镜像层仍在 K8s 节点侧拉取转换（受信节点池内完成），机密性依赖节点池与 KBS 证明链路设计，与节点内 TEE 模式的信任边界不同——选型时必须区分。

### 3.6 加密容器镜像

CoCo 采用 **ocicrypt（OCI 镜像层加密）** 方案，`containerd` 侧（宿主机 shim）与 guest 侧 `image-rs` 都支持解密：

- 用 RSA 公钥（JWE）加密镜像层并推送仓库，**宿主机与仓库只见密文**；
- 私钥存 KBS，仅证明通过后在 guest 内解密；
- 制作工具链：`ocicrypt`（keyprovider）+ `skopeo copy --encryption-key` 或 `ctr-enc`（见[五、生产部署实操](#五生产部署实操)）。

---

## 四、Kubernetes 集成方式

### 4.1 RuntimeClass

CoCo operator / kata-deploy 会创建多个 RuntimeClass（`kata-qemu`、`kata-clh`、`kata-runtime-rs`、peer-pod 的 `kata-remote` 等），Pod 通过 `runtimeClassName` 选用：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: tee-demo
spec:
  runtimeClassName: kata-qemu     # operator 安装的机密运行时
  containers:
  - name: app
    image: registry.example.com/app:v1.0-encrypted
```

生产上建议用 MutatingWebhook（operator 可选组件）按 namespace/label 自动注入 `runtimeClassName`，避免开发者手写遗漏。

### 4.2 加密镜像 + KBS URL 注解

guest 内 AA 需要知道 KBS 地址——通过 kata kernel_params 注解传递（operator 或手工注入）：

```yaml
metadata:
  annotations:
    io.katacontainers.config.agent.kernel_params: >-
      agent.aa_kbc_params=cc_kbc::https://kbs.kbs-operator-system.svc.cluster.local:8080
```

- `cc_kbc` 是当前推荐 KBC 实现（历史上还有 `online_sev_kbc` 等早期实现，逐步统一到 trustee/KBS 体系）；
- KBS 地址必须在 Pod VM 可达的网络内（NetworkPolicy 允许 kube-system/运行时命名空间 → KBS）；
- 高可用部署 KBS 多副本 + 证书轮换机制，KBS 不可达会导致所有新机密 Pod 无法启动。

### 4.3 调度与开销声明

- operator 会在支持节点打标签（如 `katacontainers.io/kata-runtime: kata-qemu`），配合 nodeSelector/taint 把机密负载调度到 TEE 节点池；
- 在自定义 RuntimeClass 中显式声明 `overhead`（内存/CPU）与 `scheduling`，避免调度器低估 Pod VM 成本（见[七、最佳实践](#七最佳实践与反模式)）。

---

## 五、生产部署实操

### 5.0 版本记录（2026-10-14 核对）

- CoCo operator：**v0.17.0**（operator releases）；Kata Containers：**4.2.0**（kata-containers releases），均经 `gh api repos/<org>/<repo>/releases/latest --jq .tag_name` 核对

### 5.1 前置条件检查（节点侧）

```bash
# 🟢 检查内核版本（SNP/TDX 的 KVM 支持需较新内核，官方支持矩阵见 Kata/CoCo 文档）
uname -r

# 🟢 AMD SEV-SNP 能力：内核日志 + kvm_amd 模块参数（内核 ≥6.8 提供 sev_snp 参数）
dmesg | grep -i "sev"
cat /sys/module/kvm_amd/parameters/sev_snp 2>/dev/null   # 1 = 已启用，0/不存在 = 未启用

# 🟢 Intel TDX 能力：内核日志 + kvm_intel 模块参数
dmesg | grep -i tdx
cat /sys/module/kvm_intel/parameters/tdx 2>/dev/null
```

若 `sev_snp`/`tdx` 为 0 或不存在：核对 CPU 代际、BIOS 中 SEV-SNP/TDX/SGX 开关、内核版本与 `kvm_amd`/`kvm_intel` 模块加载参数（模块参数的既有说明见上方命令注释）。固件设置变更属于 🔴 高风险操作（需重启节点，按节点池滚动执行）。

### 5.2 安装 CoCo operator（节点模式）

```bash
# 🟡 安装 operator（默认部署到 confidential-containers-system 命名空间）
kubectl apply -k "github.com/confidential-containers/operator/config?ref=v0.17.0"

# 🟢 观察 operator 就绪
kubectl -n confidential-containers-system get pods -w
```

创建 KataConfig 触发节点运行时安装：

```yaml
# kataconfig.yaml — 节点模式（在节点上安装机密化 kata 运行时）
apiVersion: confidentialcontainers.org/v1alpha1
kind: KataConfig
metadata:
  name: cluster-kataconfig
spec:
  enablePeerPods: false          # true 则安装 peer-pod（kata-remote）能力
```

```bash
# 🟡 应用 KataConfig（会在目标节点 DaemonSet 式安装 kata + 重启 containerd）
kubectl apply -f kataconfig.yaml

# 🟢 观察安装进度与状态
kubectl get kataconfig cluster-kataconfig -o yaml | grep -A5 status
kubectl get runtimeclass                       # 应看到 kata-qemu 等
kubectl get nodes -L katacontainers.io/kata-runtime
```

> ⚠️ KataConfig 安装会滚动重启节点上的 containerd（Pod 会被重新拉起），务必在非生产窗口验证，节点池逐台放量。

### 5.3 （可选）peer-pod 模式

```yaml
apiVersion: confidentialcontainers.org/v1alpha1
kind: KataConfig
metadata:
  name: cluster-kataconfig
spec:
  enablePeerPods: true
  peerPods: {}                   # 各云/裸机 remote hypervisor 的具体参数见 operator CRD 文档
```

peer-pod 还需按环境配置远程 Pod VM 的镜像/网络/实例限额（libvirt、云 API 凭据等），细节以所装 operator 版本的 `KataConfig` CRD schema 为准。

### 5.4 部署 KBS（trustee）

KBS 建议部署在**独立于业务工作负载的可信管理域**（独立集群或独立 namespace + 严格 NetworkPolicy/RA）：

```bash
# 🟡 以 kustomize 部署 trustee（overlay/参数以所 pin 版本仓库 README 为准）
# 参考：github.com/confidential-containers/trustee 的 config/release 目录
kubectl apply -k "github.com/confidential-containers/trustee/config/release?ref=<pin-tag>"

# 🟢 确认 KBS 服务（默认示例环境常为 kbs-operator-system 命名空间）
kubectl -n kbs-operator-system get pods,svc
```

部署后管理员需要：初始化 KBS 根证书/客户端证书（trustee 提供 `kbs-client` CLI 与示例脚本），并把「参考值」（预期 launch digest、TCB 版本下限）写入 RVPS。参考值的管理策略直接决定安全强度——建议纳入变更评审。

### 5.5 制作并分发加密镜像

```bash
# 🟢 生成镜像加密密钥对（本地无副作用）
openssl genrsa -out img_key.pem 2048
openssl rsa -in img_key.pem -pubout -out img_pub.pem

# 🟡 加密并推送镜像（skopeo 需启用 encryption 构建标签）
skopeo copy \
  --encryption-key jwe:img_pub.pem \
  docker://registry.example.com/shop/app:v1.0 \
  docker://registry.example.com/shop/app:v1.0-enc

# 🟡 将解密私钥写入 KBS（kbs-client 子参数以所装版本 --help 为准）
kbs-client --url https://kbs.example.com \
  config --auth-private-key admin.key \
  set-resource --repository default --type key --tag img-v1.0 --input-file img_key.pem
```

### 5.6 创建机密 Pod 并验证 TEE 状态

```yaml
# tee-demo-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: tee-demo
  annotations:
    io.katacontainers.config.agent.kernel_params: >-
      agent.aa_kbc_params=cc_kbc::https://kbs.kbs-operator-system.svc.cluster.local:8080
spec:
  runtimeClassName: kata-qemu
  containers:
  - name: app
    image: registry.example.com/shop/app:v1.0-enc
```

```bash
# 🟡 创建机密 Pod
kubectl apply -f tee-demo-pod.yaml

# 🟢 Pod 状态与事件（证明失败会卡在镜像拉取/创建阶段并体现于 events）
kubectl get pod tee-demo -w
kubectl describe pod tee-demo | tail -20

# 🟢 权威验证：KBS 侧证明日志（证据校验 + 资源下发记录）
kubectl -n kbs-operator-system logs deploy/kbs --tail=100 | grep -Ei "attest|resource|verify"

# 🟢 guest 内交叉验证（应用可暴露的只读信号）
kubectl exec -it tee-demo -- sh -c 'grep -oE "sev_snp|sev_es" /proc/cpuinfo | sort -u || true'
```

验证通过的判据链：① Pod Running 且镜像为 `-enc`（宿主机不可能有明文层）；② KBS 日志出现该 Pod VM 度量的通过记录与密钥下发记录；③ RVPS 参考值未被修改。三者齐备才可认定「运行于已证明的 TEE」。

---

## 六、故障排查与诊断流程

| 现象 | 高频根因 | 诊断命令（均 🟢 只读） |
|------|----------|------------------------|
| Pod 一直 Pending | RuntimeClass 不存在；TEE 节点未打标签/污点排斥 | `kubectl get runtimeclass`；`kubectl get nodes -L katacontainers.io/kata-runtime` |
| 卡 ContainerCreating，事件含 image pull/decrypt | KBS 不可达；`aa_kbc_params` 注解缺失或拼错；证明失败未获 token | `kubectl describe pod <p>`；`kubectl -n kbs-operator-system logs deploy/kbs` |
| KBS 日志 verify/reject | 参考值过期（内核/固件升级后度量变化）；TCB 版本下限策略过严 | 比对 RVPS 存储的 digest 与实际 launch digest；核对固件/内核近期变更 |
| RunPodSandbox failed: kata | 节点 TEE 未启用（sev_snp/tdx=0）；kata 运行时安装不完整 | 节点 `journalctl -u containerd --since -10m`；`dmesg \| grep -iE "sev\|tdx"` |
| Pod 内应用崩溃 | guest 内核缺该应用所需特性（如某些调试接口）；/proc 只读策略 | `kubectl logs <p>`；用非加密同镜像在 kata（非 TEE）模式对照复现 |
| 吞吐/延迟异常劣化 | 加密引擎带宽瓶颈、Pod VM 内存 overhead 被低估导致超卖 | 见[八、性能开销与监控](#八性能开销与监控)指标定位 |
| peer-pod 起不来 | remote hypervisor 凭据/配额/网络问题 | `kubectl -n confidential-containers-system logs <peer-pod-shim-pod>` |

通用排查顺序：`kubectl describe`（事件）→ 节点 containerd/kata 日志 → KBS/AS 日志 → 对照「证明链路 7 步」（3.4 节）逐步定位断点。

---

## 七、最佳实践与反模式

### 7.1 最佳实践

1. **RuntimeClass 显式声明 overhead**：为机密 RuntimeClass 配置 `overhead`（内存 256Mi+，按 Pod VM 实测）与 `scheduling.nodeSelector`，防止调度器超卖 TEE 节点。
2. **密钥只走 KBS**：业务密钥、镜像解密钥一律经 AA/KBS 分发，绝不放进 Secret/ConfigMap（它们在 etcd 与宿主机上明文可见）。
3. **参考值纳入变更管理**：内核/固件/agent 升级会改变度量值，升级流程必须包含「更新 RVPS 参考值 + 试点验证证明链路」步骤。
4. **分层验证**：CI 中固化「加密镜像构建 → 测试集群证明通过 → 部署」的门禁，防止有人悄悄换回明文镜像。
5. **TEE 节点池独立治理**：独立节点池 + 污点 + 最小化 daemonset（关闭非必要监控 agent 进入 guest 语义的干扰），配合运行时检测（见 [[wiki/08-安全/03-运行时安全/08-falco-runtime-security-guide.md|Falco 指南]]）覆盖宿主机侧异常。
6. **KBS 高可用与证书轮换**：KBS 是机密负载启动的硬依赖，至少双副本 + 提前演练证书过期轮换。

### 7.2 反模式

- ❌ **TEE 万能论**：不做 mTLS、不做卷加密，认为「进了 TEE 就安全」——网络与持久化数据仍明文离开 TEE。
- ❌ **KBS 与业务同域部署**：KBS 与被证明负载同 namespace/同节点池，信任边界自相矛盾。
- ❌ **加密镜像但参考值形同虚设**：RVPS 配置成「接受任何度量」，证明沦为走形式。
- ❌ **在机密 Pod 里挂 hostPath/debug 工具**：既破坏隔离也污染度量基线。
- ❌ **把 peer-pod 当节点内 TEE 用**：两者信任边界不同（3.5 节），安全评审必须区分。
- ❌ **忽视可用性面**：机密计算不防宿主 DoS，关键业务仍需多副本/多可用区设计。

---

## 八、性能开销与监控

### 8.1 开销构成

| 开销源 | 典型量级 | 说明 |
|--------|----------|------|
| Pod VM 启动 | 秒级（约 1–5s/每次启动） | 对比 runc 毫秒级；影响弹性扩容速度与冷启动 |
| 内存 overhead | 每 Pod VM 数百 MiB 起 | guest 内核 + agent 常驻；需计入 RuntimeClass overhead |
| 内存加密/解密 | CPU 型号相关，通常个位数百分比 | 内存带宽受限型负载（memcpy 密集）受影响更明显 |
| 证明/取钥 | 首次启动增加数百 ms–秒级 | 仅启动时发生，可缓存 token 减少重复证明 |
| GPU CC（H100） | 官方口径低个位数百分比；小 batch 延迟敏感场景更高 | TOM 截断减少可用显存；CC-mode 禁用 MIG |

### 8.2 基准讨论（需自测校准）

- 公开资料中 SEV-SNP/TDX 的纯 CPU 开销多在 **0–7%** 区间，但高度依赖负载形态：计算密集型接近无感，内存带宽密集与高频小 I/O 型放大；
- NVIDIA 发布的 H100 CC 推理基准称**低个位数百分比**开销，但该结论对应特定模型/batch 配置，LLM 推理建议用自身负载复测（TTFT/吞吐/显存占用三项）；
- **结论**：以自身业务做 A/B（同负载 runc vs kata-qemu-TEE vs kata 非 TEE）建立基线，引用任何外部数字只做量级参考。

### 8.3 监控指标

- **启动链路**：Pod 从创建到 Running 的 P95 分位数（分离出 kata 启动与证明取钥耗时）；KBS 请求时延与失败率；
- **证明健康度**：attestation 成功/失败计数、RVPS 拒绝次数（突增常意味着异常镜像或度量漂移）；
- **资源水位**：TEE 节点内存超卖率、Pod VM overhead 实际用量 vs 声明值；
- **GPU（机密 AI）**：推理 TTFT/吞吐对比基线、GPU 显存可用量（监控 TOM 截断影响）、CC-mode 状态巡检（防有人重启后模式丢失）。

---

## 九、典型用例与落地路线图

### 9.1 典型用例

| 用例 | 方案要点 |
|------|----------|
| 多方联合计算/数据不出域 | 各方数据进入各自 TEE Pod，仅交换密文/聚合结果；度量值作为准入凭据 |
| 机密 AI 推理 | H100 CC-mode + CPU TEE；模型权重密文分发，密钥经 KBS 证明后注入 |
| 密钥资产保护 | 把签名/发币/密钥派生服务放进 TEE Pod，宿主机管理员无法导出主密钥 |
| 多租户强隔离 | 不合格租户/第三方代码运行于机密 Pod，替代「每租户专用集群」的重成本方案（配合 [[wiki/08-安全/07-零信任架构/index.md|零信任架构]] 总体设计） |
| 合规驱动（数据驻留/审计） | 用远程证明报告作为「数据仅在受控环境处理」的技术证据（对接 [[wiki/08-安全/06-合规审计/index.md|合规审计]]） |

### 9.2 成熟度评估（2026-10 视角）

| 层面 | 成熟度 | 说明 |
|------|--------|------|
| 硬件（SNP/TDX） | 生产可用 | 云厂商机密 VM 商用化多年；自管需固件/内核运维能力 |
| GPU CC | 生产可用（机密 AI） | H100 起为主流方案；需注意 MIG 限制与驱动版本 |
| ARM CCA | 追赶中 | 软件栈与云形态在完善，暂不适合首批生产落地 |
| Kata Containers | 生产可用 | 4.x 稳定演进（当前 4.2.0） |
| CoCo operator | 早期（0.x，v1alpha1） | 功能完整但 API 与行为跨版本可能变化，升级需谨慎 |
| KBS/trustee | 可用但需自运营 | 高可用/密钥轮换/审计需自建流程 |

### 9.3 分阶段路线图

1. **PoC（1–2 月）**：单节点池 SNP/TDX + operator + KBS 单副本；跑通加密镜像与证明全链路；产出性能基线。
2. **试点（1 季度）**：选 1–2 个低风险业务（如内部签名服务）双跑；RuntimeClass 注入 webhook 化；KBS 双副本。
3. **生产（1 年内）**：机密节点池 taint 治理；参考值变更流程接入 CI；监控告警（证明失败率/启动时长）上线；故障演练（KBS 不可达降级预案）。
4. **平台化**：机密 Pod 作为平台自服务能力（namespace 级开关）；机密 AI 推理模板（H100 CC + vLLM）；跨集群 KBS 联邦与审计报表。

---

## 十、版本兼容性与升级注意

- **版本核对口径（2026-10-14）**：CoCo operator `v0.17.0`、Kata Containers `4.2.0`（GitHub Releases API）。写入任何文档/镜像 tag 时请以当日重跑命令为准。
- **operator 0.x 语义**：`KataConfig` 为 v1alpha1，跨小版本升级前必须阅读对应 release notes；升级顺序 = CRD → operator 镜像 → 重新下发 KataConfig → 节点滚动。
- **Kata 4.x 升级**：更换 kata 版本会更换 guest 内核/镜像组合 → **launch digest 改变** → 必须先更新 RVPS 参考值再放量，否则所有机密 Pod 证明失败（这是最经典的「升级后全挂」事故）。
- **内核/固件**：宿主内核升级可能改变 KVM TEE 行为；CPU 微码/固件升级可能抬升 TCB 版本要求，同步核对 KBS 中策略的最低 TCB 版本。
- **K8s 版本**：operator README 给出支持矩阵（1.2x–1.3x 区间随版本演进）；containerd 需支持 shim v2 与镜像加密构建标签。
- **trustee/KBS**：与 operator 的 `aa_kbc_params` 协议需版本匹配，两者建议同窗口升级并在预发验证证明链路。

---

## 十一、参考链接与延伸阅读

- CoCo operator：<https://github.com/confidential-containers/operator>（v0.17.0）
- Confidential Containers 官网/文档：<https://confidentialcontainers.org>
- trustee（KBS/AS/RVPS/kbs-client）：<https://github.com/confidential-containers/trustee>
- Kata Containers：<https://github.com/kata-containers/kata-containers>（4.2.0）/ 文档 <https://github.com/kata-containers/kata-containers/blob/main/docs/>
- Intel TDX：<https://www.intel.com/content/www/us/en/products/docs/accelerator-engines/intel-trust-domain-extensions.html>
- AMD SEV-SNP：<https://www.amd.com/en/products/processors/software-technologies/security/sev-snp.html>
- ARM CCA：<https://www.arm.com/technologies/confidential-compute-architecture>
- NVIDIA 机密计算/H100 CC-mode：<https://developer.nvidia.com/confidential-computing> / nvtrust <https://github.com/NVIDIA/nvtrust>
- OCI 镜像加密（ocicrypt）：<https://github.com/containers/ocicrypt>
- CNCF 机密计算 TAG 相关白皮书：<https://tag-security.cncf.io>

## Related

- [[wiki/08-安全/03-运行时安全/06-gvisor-container-sandbox.md|gVisor 容器沙箱]] — 用户态内核隔离方案对比
- [[wiki/08-安全/03-运行时安全/04-runtime-security-defense.md|运行时安全防御]] — 宿主机侧纵深防御
- [[wiki/08-安全/03-运行时安全/07-security-context-fields-reference.md|Security Context 字段参考]] — Pod 级安全基线
- [[wiki/08-安全/03-运行时安全/index.md|运行时安全索引]] — 本域导航
- [[wiki/08-安全/05-供应链/index.md|供应链安全]] — 镜像签名与分发信任链
- [[wiki/14-容器运行时/index.md|容器运行时]] — containerd/kata 运行时基础

<!-- risk-assessed -->
