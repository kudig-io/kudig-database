---
title: GPU/AI 工作负载诊断 Runbook
description: 'Kubernetes GPU 调度、驱动、CUDA 与 AI/ML 训练推理工作负载的完整诊断排障指南'
summary: '覆盖 GPU 资源不可见、Pod 等 GPU、驱动/CUDA 兼容、MIG/time-slicing、多机多卡 NCCL 通信、显存耗尽等 10 类根因的三阶段诊断工作流与风险分级修复'
category: skills
tags:
- k8s
- skills
- runbook
- gpu
- nvidia
- cuda
- ai-infra
- device-plugin
tier: core
created: '2026-08-27'
last_updated: 2026-08
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI Infra 工程师
estimated_read_time: 15min
skill_id: SKILL-WORK-006
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
agent_execution_mode: L1-advisory
intent_queries:
- GPU 资源为什么没有显示在节点上
- 训练任务一直 Pending 怎么排查
- NCCL 通信超时如何定位
- CUDA driver version mismatch 怎么解决
trigger_keywords:
- gpu pending
- nvidia.com/gpu
- cuda mismatch
- nccl timeout
- mig
- time-slicing
- 显存不足
- 驱动异常
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
- container-runtime-basics
related_skills:
- "./ts-ai-ml-workloads.md"
- "./gpu-fta.md"
- "../node/"
cross_refs:
- type: fta
  path: ./gpu-fta.md
  label: 'GPU 故障树分析'
- type: doc
  path: ./ts-ai-ml-workloads.md
  label: 'AI/ML 工作负载速查'
- type: doc
  path: '../node/'
  label: '节点资源压力诊断'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# GPU/AI 工作负载诊断 / GPU & AI Workload Diagnosis

GPU 工作负载故障链比 CPU 应用长一层：容器 → CUDA runtime → NVIDIA Driver → 硬件。任何一层的版本错配都会以"看似无关"的上层报错呈现（例如 CUDA error 常源自驱动层）。本 Runbook 按「资源可见性 → 分配与调度 → 运行时兼容 → 性能与通信」四段闭环组织。

## 快速症状定位

| # | 症状 | 检测方法 | 置信度 |
|---|------|---------|--------|
| S1 | 节点 allocatable 中无 `nvidia.com/gpu` | 🟢 `kubectl get node <n> -o jsonpath='{.status.allocatable}'` | 0.95 |
| S2 | Pod Pending，事件提示 Insufficient nvidia.com/gpu | 🟢 `kubectl describe pod` events | 0.95 |
| S3 | 容器内 `nvidia-smi` 报 "could not communicate with driver" | 🟢 `kubectl exec ... -- nvidia-smi` | 0.90 |
| S4 | Pod 起来即 CrashLoop，日志含 CUDA driver version is insufficient | 🟢 容器日志尾部 | 0.95 |
| S5 | 多卡训练卡死或 NCCL timeout | 🟢 任务日志 + 网络设备状态 | 0.80 |
| S6 | MIG 切分后 pod 无法拿到期望 profile | 🟢 `kubectl describe node` 中 mig 资源名 | 0.90 |
| S7 | 显存随时间泄漏最终 OOM（进程被杀） | 🟢 DCGM 显存曲线 + dmesg | 0.85 |

**排除条件**：纯 CPU 侧调度问题 → 工作负载/pod 排查；节点整体 NotReady → 节点 Runbook；存储挂载影响训练数据读取 → 存储 Runbook。

## 快速分级

```
业务影响 × GPU 类型稀缺度
├── 全部 GPU 节点同时不可用（设备插件集群级故障）──→ P0
├── 单个训练任务卡调度但可改配规避 ──────────────→ P1
├── 推理服务显存 OOM 影响线上 API ────────────────→ P0/P1（按流量）
├── CUDA 版本不兼容（新镜像上线）────────────────→ P1（回退镜像快路径优先）
├── 多机通信抖动但有 checkpoint 兜底 ─────────────→ P2
└── 性能退化（吞吐下降）无硬故障 ─────────────────→ P2
```

**立即升级条件**：物理卡降级/掉卡事件（dmesg Xid 错误连续出现）——硬件更换流程需提前介入；疑似卡间 NVLink 故障引发集体 timeout。

## Phase 1 快速检查（🟢 只读）

```bash
# D1.1 GPU 资源全景：谁有卡、有多少、被谁占着
kubectl get nodes -o json | jq -r '.items[] | select(.status.allocatable["nvidia.com/gpu"]) |
  {name:.metadata.name, total:.status.allocatable["nvidia.com/gpu"]}'
kubectl describe node <gpu-node> | grep -A6 "Allocated resources"

# D1.2 设备插件健康（第一嫌疑组件）
kubectl get pods -A | grep -E "nvidia-device-plugin|gpu-feature-discovery|mig"
kubectl get ds -n kube-system nvidia-device-plugin -o wide    # 各节点 READY=1 才算健康

# D1.3 目标 Pod 的完整事件链
kubectl describe pod <pod> -n <ns> | sed -n '/Events:/,$p'

# D1.4 驱动基线三件套（登录 GPU 节点执行，🟢 只读）
nvidia-smi                        # 卡拓扑与驱动版本
nvcc --version 2>/dev/null || true
cat /proc/driver/nvidia/version

# D1.5 容器侧可见性测试
kubectl exec -it <pod> -n <ns> -- bash -c 'nvidia-smi; ls /dev/nvidia*'   # 无 /dev/* 说明设备透传缺失
```

## Phase 2 深度检查（🟢 只读）

```bash
# D2.1 节点上 GPU 容器运行时配置（nvidia runtime 是否注册）
grep -r "nvidia" /etc/containerd/config.toml /etc/docker/daemon.json 2>/dev/null
crictl info | grep -i -m3 runtime      # 默认运行时指向谁

# D2.2 Kernel 日志中的 GPU 硬件级错误
dmesg -T | grep -iE "nvidia|xid|nvrm" | tail -40
# Xid 是官方错误码表索引：Xid 79(掉卡) 31(内存页错误) 48(双位错误) 直接对应不同硬件处置路径

# D2.3 DCGM exporter 指标核查
kubectl get pods -A | grep dcgm
kubectl port-forward svc/<dcgm-svc> -n <ns> 9400:9400 &
curl -s localhost:9400/metrics | grep -E "DCGM_FI_DEV_GPU_UTIL|DCGM_FI_DEV_FB_USED|DCGM_FI_DEV_XID_ERRORS" | head

# D2.4 CUDA/Driver 兼容矩阵核对
# 容器 tag (如 nvcr.io/nvidia/pytorch:24.01-py3) 的 CUDA 要求 vs nvidia-smi 报告的 Driver CUDA Capability
kubectl exec <pod> -- env | grep -E "^CUDA"
nvidia-smi | head -4                      # 右上角 CUDA Version 是驱动支持上限而非已装版本

# D2.5 MIG 拓扑与 profile 占用
nvidia-smi mig -lgip                       # 列出可用 GPU Instance Profile
kubectl get node <n> -o json | jq '.status.allocatable' | grep mig

# D2.6 共享策略确认（time-slicing 配置漂移检测）
kubectl get cm nvidia-plugin-configs -o yaml 2>/dev/null     # replicas 数是否符合登记值

# D2.7 NCCL 集合通信环境变量一致性（多机训练）
kubectl exec <pod-rank0> -- env | grep -E "NCCL|RDMA" 
# 重点：NCCL_SOCKET_IFNAME 是否存在、NCCL_IB_HCA 与实际 IB 设备对应关系
```

## Phase 3 主动探测（🟡 低风险）

```bash
# D3.1 在沙箱 pod 内做最小化 GPU 自检（分配+计算单元验证）
kubectl run gpu-selftest --rm -it --image=nvcr.io/nvidia/cuda:12.4.1-base-ubuntu22.04 \
  --limits nvidia.com/gpu=1 --restart=Never \
  -- bash -c "nvidia-smi && deviceQuery 2>/dev/null || (apt-get update >/dev/null 2>&1; true)"

# D3.2 两容器跨机 ping/带宽抽测（诊断 IB/RDMA 链路）
kubectl exec a -- ibstat 2>/dev/null || echo "no rdma tooling in image"

# D3.3 用 memcheck 定位偶发计算错误（长耗时，仅在复现窗口用）
kubectl exec <pod> -- cuda-memcheck --basic ./your_kernel_test
```

## 根因分类与修复

### 根因清单

| RC ID | 根因 | 典型证据 | 首选修复 | 风险 |
|-------|------|---------|---------|------|
| RC-001 | 设备插件 DaemonSet 未跑齐/崩溃 | ds partial ready、plugin 日志报 driver lib 缺失 | 重装 plugin 或修正 helm values | 🟡 |
| RC-002 | Container Runtime 未启用 nvidia runtime hook | crictl info 默认 runc；/dev/nvidia* 不进容器 | 修 daemon.json/config.toml 后 restart runtime | 🔴 |
| RC-003 | Driver/CUDA 大版本错配 | CUDA insufficient 类日志 | 回退镜像到匹配 CUDA 或滚动升 driver | 🟡 |
| RC-004 | 调度约束过严（affinity/taint/quota） | events 含 FailedScheduling 各分支细节 | 放宽 selector 或申请 quota 提升 | 🟢 |
| RC-005 | MIG profile 分片无法满足请求 | allocatable mig*.replica 与 requests 不匹配 | 调整副本策略或换 profile 粒度 | 🟡 |
| RC-006 | Time-slicing 副本数超卖导致推理劣化/OOM | replicas 高且 FB_USED 打满 | 下调 replicas，接入监控告警 | 🟡 |
| RC-007 | Xid 硬件故障（掉卡/ECC 双位错） | dmesg 连续 Xid 79/48 | 隔离节点走硬件更换流程 | 🔴 |
| RC-008 | NCCL/RDMA 配置漂移（IFNAME/HCA/GID） | 训练日志 stuck at nccl init；ibstat 正常 | 校准 env 注入或 networkpolicy 例外 | 🟡 |
| RC-009 | 应用侧显存泄漏 | 进程 RSS/显存随批次单调上涨 | 引入 profiling（nsight / torch.cuda.memory_report）| 🟢 |
| RC-010 | Kubernetes GFD/标签缺失导致自动节点选择失效 | node 无 nvidia.com/gpu.product 类 label | 重启 gfd 组件并验证 labels | 🟢 |

### 关键修复动作详解

**REM-A 修复设备插件（RC-001）🟡**

```bash
helm upgrade nvidia-device-plugin nvdp/nvidia-device-plugin \
  -n kube-system --reuse-values --debug        # 先 --debug 干跑看渲染产物
kubectl rollout restart ds -n kube-system nvidia-device-plugin
```

验证回到 Phase 1 D1.1/D1.2 两个只读命令即可。

**REM-B 修复 Runtime Hook（RC-002）🔴 — 需节点操作审批**

修改 `default-runtime-name = nvidia` 并重启该节点所有负载所在 containerd（生产必须 drain 后进行）。集群统一建议：安装 NVIDIA GPU Operator 自动化管理 runtime 与 driver 生命周期，避免手工漂移。

**REM-C CUDA 兼容对齐（RC-003）🟡**

两条路径二选一：
1. **快路径**：镜像回退到最后一个通过回归的 tag（CI 中记录的 cuda driver hash 对应版本）
2. **慢路径**：滚动升级节点 driver（先用 cordon+drain 隔离一张卡池灰度）

**REM-D NCCL 参数校准（RC-008）🟡**

```yaml
# 在 torchjob/MPISpec 注入标准环境样例
env:
- name: NCCL_SOCKET_IFNAME
  value: eth0                # 必须是容器内真实存在的接口名
- name: NCCL_DEBUG
  value: INFO                # 仅诊断期开启，正常收敛后移除以降噪
- name: NCCL_IB_GID_INDEX
  value: "3"                 # RoCE 场景常用固定 GID 避免 auto-select 漂移
```

## 验证清单

| 编号 | 项目 | 通过标准 |
|-----|------|---------|
| V1 | 目标节点 allocatable 恢复应有卡数（D1.1） | ✅ |
| V2 | 新派发 pod 能取到 GPU 且容器内 nvidia-smi 可见全部预期设备 | ✅ |
| V3 | 训练任务吞吐恢复基线 ±5%（如已在跟踪） | ✅ |
| V4 | 无新增 Xid/NCCL 报错写入 kernel 或应用日志（观察 ≥30min） | ✅ |
| V5 | DC GM 监控面板各指标恢复正常形状 | ✅ |
| V6 | 复盘项：变更窗口内的相关告警规则覆盖本次征兆 | ✅（防复发配套）|

## 附录 A：常见 Xid 错误速查

| Xid | 含义 | 处置等级 |
|-----|------|---------|
| 13/16 | 显存页脱机处理中 | 观察计数，持续增长需隔离 |
| 31 | 内存页错误（单比特 ECC 已纠正） | 注意趋势 |
| 45/63 | Preemptive Cleanup/Row remapping | 通常无害 |
| 74 | NVLINK 错误 | 检查拓扑与线缆 |
| 79 | 卡从总线坠落 | 🔴 立即隔离节点硬件排查 |
| 94/95 | Contained/completed ECC double-bit error | 🔴 存储局部受损，走 RMA |

## 附录 B：云厂商特异性

| 环境 | 关键差异 |
|------|---------|
| ACK | 推荐 gpu-prometheusExporter 由 ARMS 托管；GN7i 等实例族注意驱动白名单版本 |
| EKS | AWS 分发的 amazon-k8s-device-plugin 区分 MIG 策略 env；Neuron 实例另有专属运维栈 |
| GKE | 由 GKE 自动管理驱动（DAEMONSET/COS 方式），禁手改宿主驱动 |
| 自建 | 一切默认自管，强烈建议引入 GPU Operator 收敛散乱脚本 |
