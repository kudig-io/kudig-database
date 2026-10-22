---
title: DPU 与智能网卡在云原生网络中的卸载实践
description: DPU/SmartNIC 卸载云原生网络深度指南 — CPU 网络瓶颈分析（每核 Gbps、context switch、overlay 封解包成本）、BlueField-3/Pensando/Intel IPU 三大家对比、四层卸载演进（checksum/TSO → OVS/RTE → eBPF/XDP → 完整 CNI 卸载）、Cilium+BlueField per-node 模式、OVS-DOCA、可观测性挑战与 PoC 落地清单
summary: DPU 智能网卡卸载实践 — 覆盖为什么 CPU 处理网络成瓶颈、NVIDIA BlueField-3/AMD Pensando/Intel IPU 规格与定位对比表、从传统 offload 到 eBPF/XDP 再到完整 K8s CNI 卸载的层级演进、host 与 DPU 的 dataplane 分工及运维含义、性能收益量级讨论、flow 可见性与 Prometheus 导出、固件升级与双系统排障等风险清单
category: network-fundamentals
tags:
- dpu
- smartnic
- offload
- network
- ebpf
- xdp
- cilium
- ovs
- kubernetes
- hardware
tier: supporting
created: '2026-10-15'
last_updated: '2026-10-15'
difficulty: advanced
audience:
- 网络工程师
- SRE
- 平台工程师
- 架构师
- 硬件选型工程师
estimated_read_time: 25min
intent_queries:
- DPU 是什么，和智能网卡什么关系
- 为什么 Kubernetes 网络需要 DPU 卸载
- BlueField-3、Pensando、Intel IPU 怎么选
- Cilium 如何跑在 BlueField DPU 上
- OVS-DOCA 硬件卸载怎么工作
- DPU 上网络流量怎么观测，怎么导出 Prometheus
- DPU 落地 PoC 应该验证哪些项
trigger_keywords:
- DPU
- SmartNIC
- 智能网卡
- 卸载
- offload
- BlueField
- Pensando
- IPU
- DOCA
- XDP
prerequisites:
- kubernetes-basics
- networking-basics
- cilium-basics
- ebpf-basics
- ovs-basics
k8s_versions:
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# DPU 与智能网卡在云原生网络中的卸载实践

> **适用对象**: Kubernetes 平台团队 / 网络硬件选型与运维工程师
> **难度**: 高级（涉及硬件、内核与 K8s 三层知识）
> **前置阅读**: [[05-网络/02-网络基础/09-cilium-ebpf-network-guide.md|Cilium eBPF 网络与安全实践指南]]、[[05-网络/02-网络基础/06-sdn-network-virtualization.md|SDN 网络虚拟化]]

---

## 📋 目录

- [一、概述：为什么 CPU 处理网络成为瓶颈](#一概述为什么-cpu-处理网络成为瓶颈)
- [二、DPU 三大家定位与规格对比](#二dpu-三大家定位与规格对比)
- [三、卸载层级演进：从 checksum 到完整 CNI 卸载](#三卸载层级演进从-checksum-到完整-cni-卸载)
- [四、与 Kubernetes 集成：host 与 DPU 的分工](#四与-kubernetes-集成host-与-dpu-的分工)
- [五、生产级落地路线：评估 PoC 清单](#五生产级落地路线评估-poc-清单)
- [六、性能收益与可观测性挑战](#六性能收益与可观测性挑战)
- [七、故障排查与诊断流程：双系统排障](#七故障排查与诊断流程双系统排障)
- [八、最佳实践、风险与坑](#八最佳实践风险与坑)
- [九、版本兼容性与参考链接](#九版本兼容性与参考链接)

---

## 一、概述：为什么 CPU 处理网络成为瓶颈

传统 SmartNIC 只做"网卡该做的事"（DMA、中断、简单封装）；DPU（Data Processing Unit）是**自带 CPU 核、内存与可编程加速引擎的独立计算系统**，能整节点接管网络、存储与安全基础设施任务，把宿主 CPU"卖回"给业务。理解 DPU 的价值，先要理解 CPU 处理网络的四个天花板：

### 1.1 单核吞吐上限

以 25GbE 满载的小包流量为例（64 字节帧，约 37Mpps），通用核走内核协议栈每包需数千个时钟周期，单核极限通常在**百 Kbps 到个位数 Mpps 量级**——即每核有效吞吐普遍在个位数 Gbps 以内。100G/400G 时代，纯 CPU 转发意味着要拿整颗 CPU 的多个核填网络，这些核本可以跑付费的业务 Pod。

### 1.2 上下文切换与中断开销

每包至少一次中断/软中断（NET_RX）、内核态与用户态边界切换，附带 cache/TLB 污染。PPS 越高，`si`（softirq）占比越高——`top` 里 `%soft` 飙到 30%+ 的机器，多半在网络软中断上空转。

### 1.3 Overlay 封解包的税

云原生网络天然多层封装：VXLAN/Geneve 隧道、Pod veth pair、Service NAT、加密（IPsec/WireGuard）。每一层都是纯 CPU 计算，且发生在**每个包、每个方向**上。万兆以上隧道流量轻松吃掉 4–8 个核。

### 1.4 基础设施与应用争抢

kube-proxy、CNI、网络策略、服务网格 sidecar、存储卸载（NVMe-oF）都在宿主机抢 CPU。核数固定时，基础设施每涨 10%，可售卖算力就跌 10%——这是公有云率先大规模上 DPU 的商业动因（AWS Nitro、Oracle OCI 的 SmartNIC 均此逻辑）。

```bash
# 🟢 观察当前节点网络 CPU 成本（定位是否值得卸载）
kubectl top nodes
ssh <node> mpstat -P ALL 1 5          # 关注 %soft 与 %sys
ssh <node> ethtool -S <nic> | grep -E 'rx_no_buffer|rx_missed'   # 丢包前兆
```

> **结论先行**：万兆以下、流量平缓的集群，CPU 处理网络完全够用，DPU 不是必需品；25G+ 高 PPS、overlay 多层、策略密集、或要把基础设施成本从宿主 CPU 剥离的场景，DPU 才进入射程。

---

## 二、DPU 三大家定位与规格对比

### 2.1 三个家族，三条路线

- **NVIDIA BlueField**：从 ConnectX 网卡演进而来，路线是"强劲 Arm SoC + 成熟 NIC 流水线 + DOCA 软件栈"，生态最完整，K8s/Cilium 集成最深。
- **AMD Pensando**：以 P4 可编程 NPU 为核心，策略与流表在可编程 ASIC 中执行；2022 年 AMD 以约 19 亿美元收购。**Oracle 一代经验**值得借鉴：OCI 在其"Zealot"论文中披露了大规模部署 Pensando SmartNIC 的工程实践——把 VCN/安全组/加密全部下沉网卡，宿主机只跑租户负载，验证了"网络即 DPU"的可行性，也暴露了固件回归与双系统运维的复杂度。
- **Intel IPU**：路线摇摆于 ASIC（Mount Evans/E2100）与 FPGA（Oak Springs Canyon）之间，强调与 Xeon 的协同卸载（DDIO、动态伸缩）；公开报道显示其后续代际规划多次调整，选型时须评估路线图风险。

### 2.2 规格对比表（公开资料整理，量级参考）

| 维度 | NVIDIA BlueField-3 | AMD Pensando（Elba/Salina 代） | Intel IPU E2100 / Mount Evans |
|------|-------------------|-------------------------------|-------------------------------|
| 处理单元 | 16× Arm Cortex-A78 | P4 可编程 NPU + Arm 核 | ASIC 加速器 + 多核 Arm |
| 端口速率 | 最高 400GbE（1×400 或 2×200） | Elba 至 200G；Salina 面向 400G | 200G 级 |
| 可编程模型 | DOCA SDK（C/Python）、OVS-DOCA、eBPF 支持逐步增强 | P4 语言定义流水线 | ACC 编程框架 / FPGA 位流 |
| K8s 生态 | Cilium per-node 模式官方文档化 | 云厂商内部栈为主 | 偏存储与基础设施卸载 |
| 加密卸载 | IPsec/TLS 线速 | MACsec/IPsec | IPsec |
| 软件成熟度 | 最完整（BFB 镜像、doca-tools、CI） | 强绑定云厂商场景 | 相对薄弱、路线图不确定 |

选型直觉：**要自己集成 K8s/Cilium → BlueField 优先；要可编程 dataplane 且有 P4 团队 → Pensando；已有强 Intel 供应链且偏存储卸载 → IPU**。所有规格以厂商当前 datasheet 为准，上表只定基调。

---

## 三、卸载层级演进：从 checksum 到完整 CNI 卸载

DPU 不是一步到位的魔法，理解卸载的层级谱系才能定位每代技术"卸掉了什么、还剩什么在 CPU"：

| 层级 | 技术 | 卸载内容 | 局限 |
|------|------|----------|------|
| L1 传统 offload | checksum/TSO/GSO/GRO/RSS | 校验和计算、分段合并、收包分发 | 转发决策仍在 CPU |
| L2 流表卸载 | OVS 内核→OVS-DPDK / RTE flow | 匹配-动作流表下沉网卡（tc flower） | 需要 DPDK 轮询核或固件流表；流表容量有限 |
| L3 eBPF/XDP | Cilium/XDP forward、XDP_TX | 包处理在驱动最早点执行，绕过协议栈 | 仍在宿主 CPU 上跑，只是"跑得更早更快" |
| L4 完整 CNI 卸载 | Cilium on BlueField（per-node）、OVS-DOCA | 整个节点网络（veth/隧道/策略/NAT）搬进 DPU | 双系统复杂度、可观测性重建 |
| L5 加密卸载 | IPsec/TLS 线速、kTLS | 对称加解密下沉 | 密钥管理跨系统 |

三 个关键认知：

1. **XDP 不是 DPU**。XDP 把 eBPF 程序挂在驱动收包最早点，仍是宿主 CPU 在执行，只是省掉了协议栈开销；DPU 是把执行位置物理迁移到另一颗芯片。两者互补：DPU 内部同样可以用类 eBPF 的方式编程（BlueField 上对宿主透明）。
2. **流表卸载的粒度决定收益**。粗粒度（五元组 megaflow）命中率高但策略语义弱；细粒度（每连接）语义全但表项暴涨。OVS-DOCA 与 Cilium 的硬件卸载都在这条曲线上找平衡。
3. **加密是最"值"的卸载**。IPsec/WireGuard 对称处理的 CPU 成本极高且无业务逻辑，搬到线速引擎收益最确定。

---

## 四、与 Kubernetes 集成：host 与 DPU 的分工

### 4.1 per-node 模式：Cilium 运行在 DPU 上

NVIDIA 与 Cilium 社区文档化的模式是**每个节点一块 DPU，Cilium agent 运行在 DPU 侧**：

```
┌───────────── 宿主机（无 Cilium、无 kube-proxy）─────────────┐
│  业务 Pod ── veth ── OVS/representor ──┐                     │
└────────────────────────────────────────┼────────────────────┘
                                         │ PCIe / SF (SFID)
┌──────────────────────── DPU ───────────▼────────────────────┐
│  OVS-DOCA（硬件流表） + Cilium agent（策略/身份/Service）      │
│  Arm 核运行完整 dataplane；隧道/加密线速执行                   │
└──────────────────────────────────────────────────────────────┘
```

要点：

- 宿主机看到的是干净的 representor 端口，Pod 网络对宿主几乎"零 CPU 占用"。
- DPU 上的 Cilium 是**独立的 agent 实例**，它 watch 同一个 K8s API（或经中继），负责本节点策略下发到 OVS-DOCA 流表。
- 网络策略、Service 负载均衡语义由 Cilium 保证，转发由硬件流表执行——"Cilium 的大脑 + DPU 的肌肉"。

```bash
# 🟢 确认 DPU 与宿主机的连接拓扑（宿主机侧）
lspci | grep -i mellanox
devlink dev eswitch show pci/0000:3b:00.0     # 查看 SF/representor

# 🟢 在 DPU 侧查看 OVS 与硬件卸载命中
ssh dpu-user@<dpu-oob-ip>
ovs-vsctl show
ovs-dpctl dump-flows -m type=ovs | head       # offloaded:yes 即硬件执行
```

### 4.2 运维含义：DPU 是第二台计算机

这是所有 DPU 方案共同的"隐藏账单"：

| 维度 | 传统节点 | DPU 节点 |
|------|----------|----------|
| 操作系统 | 1 套（宿主） | 2 套（宿主 + DPU 自带 Linux） |
| 监控对象 | 节点/Pod | 额外多一台"DPU 主机"（CPU/内存/温度/版本） |
| 升级 | k8s + 内核 | 增加 DPU 固件/BFB 镜像/DOCA 版本矩阵 |
| 故障域 | 节点宕 | DPU 宕 = 节点网络全断（且宿主可能看似健康） |

**故障域不对称**是最大风险：DPU 挂掉时宿主机往往仍在运行（本地盘、计算正常），但所有 Pod 网络中断。K8s 的 Node Condition 未必能及时反映"网卡侧死亡"，必须自建 DPU 健康上抛（见第六节）。

---

## 五、生产级落地路线：评估 PoC 清单

### 5.1 四阶段路线

```
阶段 0 基线测量（现有环境）
  └─ 量化网络 CPU 成本：si 占比、PPS/Gbps、overlay 层数
阶段 1 技术验证（2-4 台 PoC 节点）
  └─ 功能等价性：CNI 全语义（Service/策略/Ingress）在 DPU 模式下回归
阶段 2 性能与可观测性验证
  └─ 对比 A/B 机：吞吐/延迟/PPS/功耗/可观测性盲区评估
阶段 3 运维验证
  └─ 固件升级演练、DPU 故障注入、监控接入、runbook 成文
```

### 5.2 PoC 逐项清单

```text
功能等价性：
  [ ] Pod-to-Pod / Pod-to-Service / NodePort / Ingress 全通
  [ ] NetworkPolicy（含 CiliumNetworkPolicy L7 规则）语义一致
  [ ] 带宽限速（BandwidthManager/EDT）在硬件路径上是否生效
  [ ] MTU：隧道场景有效 MTU 与宿主侧 PMTUD 行为
  [ ] 多网卡/SR-IOV/GPU 直连（GPUDirect）共存
性能：
  [ ] 单流 vs 多流吞吐、小包 PPS（64B/512B/1518B 分档）
  [ ] p50/p99/p99.9 延迟对比（同规格 A/B 节点）
  [ ] 整机功耗与温度（满载 30 分钟）
  [ ] 宿主 CPU 释放量（mpstat si/sys 前后对比）
可观测性：
  [ ] flow 级可见性：OVS-DOCA / Cilium Hubble 覆盖率
  [ ] 指标导出到 Prometheus 的完整链路
  [ ] tcpdump/pcap 在硬件卸载路径上的可用性（或替代手段）
运维：
  [ ] DPU 固件升级演练（含失败回滚）
  [ ] DPU 断电/重启故障注入：节点状态、Pod 恢复时间
  [ ] 版本兼容矩阵固化（DOCA × 固件 × 宿主内核 × K8s × Cilium）
```

---

## 六、性能收益与可观测性挑战

### 6.1 收益量级讨论（务必以自测为准）

> %%inferred%% 以下为公开资料与常见实测的量级归纳，绝对值强依赖报文大小、流数、策略规模与版本，禁止直接写进采购承诺。

| 指标 | 典型讨论量级 |
|------|-------------|
| 宿主 CPU 释放 | 网络密集节点常见可释放数核至十余核（取决于原 overlay/策略负载） |
| 小包 PPS | 硬件转发可达数十 Mpps 级，远超通用核软转发数量级 |
| 延迟 | 快路径命中时可省协议栈开销（微秒级）；但首次包上送 Arm 核反而更慢——重连接 churn 场景需警惕 |
| 功耗 | DPU 自身 20-40W 量级，但对比释放的宿主核整体常为净收益 |
| 加密 | IPsec/TLS 线速卸载后，加密几乎不再构成吞吐瓶颈 |

**负面场景同样真实**：连接数极大且生命极短（如 DNS 风暴、爬虫类负载）会持续踩中"新流上送 CPU"的慢路径；流表容量打满后行为退化为软件路径或丢包。

### 6.1.1 验收口径建议

PoC 验收不要接受"峰值截图"，应约定可复现的口径：固定报文分档（64B/512B/1514B）、固定流数与新建连接速率、固定策略规模（如 1k 条 NetworkPolicy），在 A/B 同规格节点上由同一工具（如 TRex/iperf3/wrk2）各跑三轮取中位数；延迟报告 p99 与 p99.9 而非平均值，并把宿主 CPU 占用、整机功耗、DPU 温度一并写入结果表。

### 6.2 可观测性：被卸载掉的还有"可见性"

硬件路径上的包不再经过宿主协议栈，`tcpdump` 在物理口抓不到 Pod 流量、conntrack 表在 DPU 里、Hubble 若无对接则策略日志为空。三个补救层：

1. **OVS-DOCA 流统计**：`ovs-dpctl dump-flows` 的 packets/bytes 计数器，配合 `ovs-stats` 类 exporter 上报 Prometheus（按流粒度）。
2. **DPU 系统指标**：DPU 上的 node-exporter / 厂商 exporter（BlueField 有 DOCA Telemetry CLI）暴露 Arm 核、内存、温度、流表占用。
3. **应用层旁路观测**：流量镜像（ERSPAN/抓包 SF 口）或保留 eBPF 观测点在 representor 上。

```bash
# 🟢 DPU 侧导出遥测（BlueField 例）
ssh dpu-user@<dpu-oob-ip>
doca_telemetry --list-sources              # 查看可用遥测源
ovs-appctl dpif-netdev/subsort            # （软件路径调试用）

# 🟢 将 DPU 当独立主机接入 Prometheus 抓取
#   在 Prometheus scrape_config 增加:
#   - job_name: dpu-node
#     static_configs: [{targets: ['<dpu-oob-ip>:9100']}]
```

监控红线建议：DPU Arm 核利用率、流表占用率、温度、representor 队列丢包四项必须进告警；DPU NotReachable（OOB 探活）直接映射为节点网络不可用事件。

---

## 七、故障排查与诊断流程：双系统排障

DPU 节点排障的铁律：**先定位问题在哪一侧**（宿主 / PCIe 边界 / DPU），再动手。

```
Pod 网络异常（DPU 节点）
 ├─ 宿主机侧
 │   ├─ representor 是否存在：ip link | grep _0
 │   └─ SF/SR-IOV 状态：devlink dev eswitch show
 ├─ 边界
 │   ├─ PCIe 链路：lspci -vv（LnkSta）
 │   └─ MTU 不匹配（宿主口 vs DPU 口）
 └─ DPU 侧（OOB 带外登录）
     ├─ OVS 状态与流表：ovs-vsctl show / ovs-dpctl dump-flows
     ├─ Cilium agent 日志：crictl logs <cilium-pod>
     └─ 硬件计数：ethtool -S <pf> | grep -i drop
```

```bash
# 🟢 宿主机侧快速体检
lspci | grep -i mellanox; dmesg | grep -iE 'mlx5|nvme' | tail

# 🟢 DPU 带外登录后体检（BlueField 例）
uname -a; cat /etc/mlnx-release
ovs-vsctl show
systemctl status openvswitch   # DPU 内的 OVS 服务

# 🟡 重启 DPU 上的 OVS（会瞬时中断该节点 Pod 网络，先排空）
sudo systemctl restart openvswitch

# 🔴 重启/重灌 DPU（节点网络长时间中断，必须在维护窗口）
sudo bfb-install --rshim rshim0 --bfb <BlueField-X.bfb>
```

高频故障模式速查：

| 现象 | 首查位置 | 常见根因 |
|------|----------|----------|
| Pod 网络全断、宿主正常 | DPU OOB 探活 + OVS 状态 | DPU 宕机 / OVS 崩溃 / 流表刷空 |
| 间歇性丢包 | `ethtool -S` 队列 drop | 流表打满退化为慢路径、representor 队列溢出 |
| 新连接慢 | DPU Arm 核 CPU | 新流上送慢路径，首包在 Arm 上处理 |
| 升级后不通 | 版本矩阵 | 固件 × DOCA × 内核驱动不匹配 |

---

## 八、最佳实践、风险与坑

**最佳实践**

1. **先测量后采购**：没有阶段 0 的 CPU 网络成本数据，DPU 项目无法证明 ROI。
2. **版本矩阵即代码**：固件/BFB/DOCA/宿主内核/Cilium 的兼容矩阵固化在仓库，CI 校选型与升级提案。
3. **OOB 带外管理必建**：DPU 失联时唯一救命通道；OOB 网络与业务网络物理隔离。
4. **灰度接入**：按节点池滚动，先跑非核心负载一个季度再扩大。
5. **可观测性先行**：DPU 指标与 flow 导出在功能验证阶段就接入，不要等生产事故倒逼。
6. **保留软件回退路径**：设计上允许单节点切回宿主 CPU 模式（保留 Cilium 宿主侧配置能力），作为 DPU 故障的应急降级。

**风险与坑**

- ❌ **固件升级是硬中断**：DPU 固件/BFB 升级通常要求 DPU 重启即节点网络重启，必须当节点级维护对待；升级失败 brick 后需带外恢复，务必先在备件上演练。
- ❌ **双系统排障复杂度**：每台"节点"实为两台机器，runbook、值班技能、根因分析都要覆盖两侧；团队若无内核/网络纵深，学习曲线陡峭。
- ❌ **锁定风险**：DOCA/P4/ACC 各家 SDK 互不兼容，应用层写死厂商 API 后迁移成本极高——把厂商专有依赖隔离在基础设施层，K8s 侧保持 CNI 标准接口。
- ❌ **"线速"话术陷阱**：厂商宣称线速多指大包快路径；小包、新流、策略深层叠加时实际吞吐可能显著低于峰值，合同与验收按 PoC 实测口径。
- ❌ **路线图风险**：DPU 市场仍在整合期（收购、代际调整时有发生），采购前评估厂商该产品线的持续投入承诺。

---

## 九、版本兼容性与参考链接

### 9.1 版本与生态要点

- BlueField/DOCA 的兼容矩阵更新频繁，每个 DOCA 版本都有明确的 BlueField 固件、宿主驱动（MLNX_OFED）、宿主发行版对应表，升级永远从矩阵出发。
- Cilium 侧：硬件卸载相关能力随版本演进（v1.13+ 逐步增强），采用前核对所用 Cilium 版本的文档声明；本文相关背景见 [[05-网络/02-网络基础/09-cilium-ebpf-network-guide.md|Cilium eBPF 网络与安全实践指南]]。
- Intel IPU 路线图存在公开报道的调整，选型时索取厂商最新的产品路线承诺。
- 所有具体版本号以厂商官方文档当日为准，本文不锁定小版本号。

### 9.2 参考链接

- NVIDIA DOCA 文档：<https://docs.nvidia.com/doca/>
- NVIDIA BlueField DPU 产品页：<https://www.nvidia.com/en-us/networking/products/dpu/>
- Cilium 与 NVIDIA BlueField 集成讨论（官方博客/文档）：<https://docs.cilium.io/>
- OVS 硬件卸载（OVS-DOCA / tc flower）：<https://docs.openvswitch.org/en/latest/howto/offload/>
- Oracle OCI "Zealot: SmartNIC 卓越运营" 论文（USENIX ATC 2022）：<https://www.usenix.org/conference/atc22/presentation/firestone>
- AMD Pensando DPU：<https://www.amd.com/en/products/accelerators/pensando.html>
- Intel IPU：<https://www.intel.com/content/www/us/en/products/details/network-io/ipu.html>
- 相关内部文档：[[05-网络/02-网络基础/07-cni-plugin-comparison-selection.md|CNI 插件对比选型]]、[[05-网络/02-网络基础/06-sdn-network-virtualization.md|SDN 网络虚拟化]]、[[23-实体/04-网络/cilium.md|Cilium]]、[[23-实体/07-可观测性/prometheus.md|Prometheus]]

---

*DPU 卸载是把"网络基础设施"从 CPU 中剥离的架构选择：收益真实，代价是把复杂度转移到了双系统运维与可观测性重建。按本文 PoC 路线验证后再放量。*
