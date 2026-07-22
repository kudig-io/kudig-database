---
title: Inclavare Containers (entities)
description: '## 概述'
summary: 'Inclavare Containers 是一个基于硬件可信执行环境 (TEE) 的机密容器项目。它利用 Intel SGX、ARM TrustZone 等硬件安全技术，在隔离的 Enclave 中运行容器工作负载，保护数据和代码的机密性和完整性。即使宿主机操作系统或 Hypervisor 被攻破，Enclave 内的数据也不会泄露。'
category: entities
tags:
- k8s
- cncf
- security
- inclavare-containers
- prometheus
- containerd
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Inclavare Containers 是什么
- 如何 Inclavare Containers
trigger_keywords:
- Inclavare
- Containers
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Inclavare Containers

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go, Rust, C

## 概述

Inclavare Containers（Inclavare 源自拉丁语"堡垒"）是由蚂蚁集团（阿里云安全团队）和 Intel 联合开发的开源机密容器（Confidential Container）项目，2021 年进入 CNCF Sandbox。它利用**硬件可信执行环境（Trusted Execution Environment, TEE）**——如 Intel SGX、ARM TrustZone、AMD SEV——在隔离的 **Enclave** 中运行容器工作负载，保护数据和代码的机密性和完整性。即使宿主机 OS、Hypervisor 甚至云管理员被攻破，Enclave 内的数据也不会泄露。

Inclavare 与 **Confidential Containers（CoCo）** CNCF 项目是互补关系——Inclavare 提供了基于 Intel SGX 的 Enclave 运行时实现（rune + Occlum/LibOS），是 CoCo 的一个后端选项。它兼容 OCI 标准，可与 containerd/CRI-O 无缝集成。

## Key Features

- **硬件 TEE 支持**：Intel SGX、ARM TrustZone、AMD SEV 多种 TEE 后端
- **Enclave 隔离**：容器工作负载在硬件隔离的 Enclave 中运行
- **LibOS 兼容**：通过 Occlum/Skeleton Key 等 LibOS 在 SGX 中运行未修改的应用
- **远程证明**：Attestation 机制验证 Enclave 的真实性和完整性
- **OCI 兼容**：符合 OCI Runtime 规范，与 containerd 集成
- **机密镜像传输**：镜像加密拉取，仅在 Enclave 内解密运行

## Architecture

Inclavare Containers 由 **rune**（OCI Runtime，类似 runc 但支持 Enclave）、**Occlum**（基于 Rust 的 LibOS，在 SGX Enclave 中运行应用）、**shim**（containerd-shim，桥接 CRI 和 rune）和 **Enclave Attestation Service**（远程证明服务）组成。容器镜像加密存储在 Registry 中，拉取时通过密钥协商协议在 Enclave 内解密。运行时，应用代码和数据在 SGX Enclave 中执行，宿主机和 Hypervisor 无法访问。

## K8s 集成

Inclavare 通过 containerd-shim 与 Kubernetes 集成。Pod 标注需要 Enclave 运行时后，containerd 通过 shim-rune 调用 rune（而非 runc）创建容器。rune 在 SGX Enclave 中启动 Occlum LibOS，加载应用代码执行。远程证明通过 Kubernetes Admission Webhook 在 Pod 启动前验证。

## 生产部署要点

- **EPC 内存规划**：SGX EPC 内存有限（通常 128-256 MB），合理规划应用内存使用
- **最小化 TCB**：减少 Enclave 内的代码量，降低可信计算基（TCB）复杂度
- **远程证明**：生产环境中始终启用远程证明，验证 Enclave 的真实性
- **密钥管理**：使用远程证明后的安全通道获取密钥，不要硬编码密钥
- **性能调优**：减少 Enclave 进出（ECALL/OCALL）次数，降低上下文切换开销

## 生产场景

1. **隐私计算（MPC）**：多方数据在 Enclave 中联合计算，数据不出 Enclave
2. **敏感数据处理**：金融/医疗数据处理应用在 TEE 中运行，防止数据泄露
3. **AI 模型保护**：珍贵的 AI 模型权重在 SGX 中推理，防止模型窃取
4. **区块链智能合约**：智能合约在 Enclave 中执行，保证执行可信

## 安装与配置

### 前提条件

```bash
# 节点需要支持 Intel SGX 硬件和驱动
# 安装 SGX 驱动和 SDK（Ubuntu 示例）
sudo apt install linux-modules-extra-$(uname -r)
sudo modprobe sgx

# 验证 SGX 支持
ls /dev/sgx_enclave /dev/sgx_provision
dmesg | grep -i sgx
```

### K8s 部署

```bash
# Helm 安装 Inclavare
helm repo add inclavare https://inclavare-containers.github.io/charts
helm install inclavare inclavare/inclavare -n inclavare-system --create-namespace

# 验证组件状态
kubectl get pods -n inclavare-system
kubectl get runtimeclass rune
```

### SGX Enclave Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: sgx-app
  annotations:
    io.kubernetes.cri.containerd.rune: "rune"
spec:
  runtimeClassName: rune
  containers:
  - name: app
    image: registry.example.com/encrypted-app:latest
    resources:
      limits:
        sgx.intel.com/epc: "100Mi"  # 请求 SGX EPC 内存
    env:
    - name: OCCLUM_RELEASE_ENCLAVE
      value: "1"
---
# SGX 设备插件 DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: sgx-device-plugin
  namespace: inclavare-system
spec:
  selector:
    matchLabels:
      app: sgx-device-plugin
  template:
    spec:
      containers:
      - name: sgx-plugin
        image: intel/intel-sgx-plugin:latest
        securityContext:
          privileged: true
        volumeMounts:
        - name: dev-sgx
          mountPath: /dev/sgx
      volumes:
      - name: dev-sgx
        hostPath:
          path: /dev/sgx_enclave
```

## 运维操作

```bash
# 🟢 检查节点 SGX 资源可用性
kubectl describe node <node> | grep sgx.intel.com/epc

# 🟢 查看 Enclave Pod 运行状态
kubectl get pod sgx-app -o wide
kubectl logs sgx-app

# 🟡 部署新的 Enclave 应用
kubectl apply -f sgx-app.yaml

# 🔴 删除 Enclave Pod（释放 EPC 内存）
kubectl delete pod sgx-app

# 🔴 重启 Inclavare 组件
kubectl rollout restart deployment -n inclavare-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending: Insufficient sgx.intel.com/epc | EPC 内存不足 | `kubectl describe node <node> \| grep sgx` | 减少 EPC 请求或扩容节点 |
| Enclave 创建失败 | SGX 驱动未加载 | `dmesg \| grep sgx` | 加载 sgx 内核模块 |
| 远程证明失败 | 网络无法连接 IAS/DCAP | `curl -v https://api.trustedservices.intel.com` | 配置代理或离线证明 |
| containerd rune 运行时错误 | OCI runtime 未配置 | `cat /etc/containerd/config.toml \| grep rune` | 配置 containerd runtime handler |

**排查流程：**
```
Enclave Pod 启动失败
├── 检查硬件支持 → ls /dev/sgx_enclave
├── 检查驱动加载 → dmesg | grep sgx
├── 检查 RuntimeClass → kubectl get runtimeclass rune
├── 检查 EPC 资源 → kubectl describe node | grep sgx
└── 检查 containerd 配置 → containerd config dump | grep rune
```

## 生产案例

### 案例一：金融数据加密计算

- **场景**: 银行需要在不可信云环境中运行风控模型，确保客户数据不被云平台窃取
- **排查**: 使用 Inclavare Containers + Intel SGX，模型推理在 Enclave 中执行
- **方案**: 风控模型打包为 OCI 镜像，通过 Occlum LibOS 运行在 SGX 中，远程证明确保环境可信
- **效果**: 满足金融监管要求，数据在内存中加密，即使 root 权限也无法窃取

### 案例二：AI 模型保护

- **场景**: AI 公司提供模型即服务（MaaS），需防止模型权重被窃取
- **排查**: 模型权重在 SGX Enclave 中加载和推理，外部无法访问内存
- **方案**: 使用 Inclavare + Occlum，模型加密存储，仅在 Enclave 内解密执行
- **效果**: 模型权重完全保护，客户可验证运行环境（远程证明）

## 对比

| 特性 | Inclavare Containers | Confidential Containers (CoCo) | Gramine | Occlum | 适用场景 |
|------|---------------------|-------------------------------|---------|--------|----------|
| TEE 类型 | SGX/TrustZone/SEV | SGX/SEV/SNP/TDX | SGX | SGX | CoCo 最全面 |
| LibOS | Occlum/Skeleton | Kata + TEE | ✅ | ✅ | - |
| OCI 兼容 | ✅ | ✅ | ❌ | ❌ | Inclavare/CoCo |
| K8s 集成 | ✅ | ✅ | ⚠️ | ⚠️ | 云原生首选 |
| 成熟度 | Sandbox | Incubating | 独立项目 | 独立项目 | - |

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[atlantis]] — Atlantis
- [[实体/tetragon.md|[[Tetragon|tetragon]]]] — Tetragon
- [[submariner]] — Submariner
- deployment]] — Prometheus 高可用部署
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- inclavare-containers
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
