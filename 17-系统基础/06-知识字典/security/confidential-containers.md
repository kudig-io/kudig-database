---
title: 机密容器
description: Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保...
summary: Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保...
category: dictionary
tags:
- k8s
- glossary
- security
- tee
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 机密容器 是什么
- Confidential Containers 详解
trigger_keywords:
- 机密容器
- Confidential Containers
- dictionary
prerequisites:
- kubernetes
---



# 机密容器（Confidential Containers）

## 概述

Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保护容器内的数据和代码，即使基础设施提供者也无法访问。

## 核心概念/原理

- **硬件 TEE**：利用 Intel SGX/TDX、AMD SEV、ARM CCA 等硬件安全扩展
- **Kubernetes 集成**：通过 RuntimeClass 透明使用机密容器
- **零信任**：保护数据在使用中的机密性（Data in Use）
- **CNCF Sandbox**：Intel/IBM/微软等联合推动

## 关键机制或特性

- Kata Containers + TEE 后端（Guest attestation）
- 远程证明（Remote Attestation）验证运行环境
- Peer Pods 支持裸金属和云 VM
- 机密计算友好的密钥管理（密钥只在 TEE 内可用）
- CoCo Operator 简化部署和配置
- 与 Key Broker Service（KBS）集成

## 使用场景与最佳实践

- 多方数据协作（数据可用但不可见）
- 金融/医疗等高敏感数据处理
- 多租户环境下的强隔离
- 云环境中保护租户工作负载
- 合规要求下的数据加密计算

## 架构深度解析

### CoCo 组件架构与数据流

```
┌──────────────────────────────────────────────────────────────┐
│  Pod 调度 → RuntimeClass: kata-coco-cc                        │
│   │                                                          │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ CoCo Operator（Deployment）                              │  │
│  │ ├─ 管理 Kata RuntimeClass / PeerPod CRD                 │  │
│  │ └─ 编排 KBS（Key Broker Service）部署                    │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 创建 Pod                       │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ containerd → Kata shim（cloud-hypervisor/QEMU）          │  │
│  │ └─ 机密 VM（Guest OS + 工作负载容器）                    │  │
│  │    └─ TEE 硬件：Intel SGX/TDX / AMD SEV-SNP / ARM CCA   │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ attestation（远程证明）        │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ KBS（Key Broker Service）                                │  │
│  │ ├─ 校验 attestation 报告                                │  │
│  │ └─ 通过后释放工作负载密钥（仅在 TEE 内可用）             │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（confidential-containers 组织）

| 组件 | 仓库/路径 | 关键职责 |
|---|---|---|
| Operator | github.com/confidential-containers/operator | RuntimeClass/PeerPod CRD 编排 |
| Kata 运行时 | github.com/kata-containers/kata-containers | TEE 后端与 shim v2 |
| KBS | github.com/confidential-containers/kbs | 密钥代理与证明校验 |
| Attestation | github.com/confidential-containers/attestation-agent | 收集 TEE 证据生成报告 |
| Trustee | github.com/confidential-containers/trustee | AS（证明服务）参考实现 |

### 流程步骤

1. 用户按 `RuntimeClass: kata-coco-cc` 创建 Pod，调度器选择支持 TEE 的节点。
2. containerd 调用 Kata shim v2 启动机密 VM，工作负载容器运行在 VM 内。
3. attestation-agent 收集硬件证据（SGX quote / SEV attestation report）并签名。
4. KBS 验证证据与策略（镜像哈希、CPU 型号、固件版本），通过后签发工作负载密钥。
5. 密钥仅注入 TEE 内，宿主机与云厂商侧均不可见，数据在内存中也保持加密。

## 生产案例

### 案例 1：远程证明失败导致工作负载无法启动（2024 年金融云评测环境）

| 时间 | 事件 |
|---|---|
| T+0 | 部署 CoCo + KBS 后创建机密 Pod，卡在 ContainerCreating |
| T+15min | KBS 日志显示 attestation 校验失败：`policy check failed: mismatched firmware` |
| T+40min | 确认 KBS 策略中的固件版本号早于节点实际版本（升级 BIOS 后未同步） |
| T+2h | 更新 KBS 策略允许新固件版本，Pod 正常启动 |

- **根因**：远程证明策略硬编码固件版本，节点固件升级后策略未同步更新。
- **修复命令**（更新 KBS 策略并重载）：
```bash
# 🔴 更新 KBS 策略（AS policy 允许目标固件版本）后重启 KBS
kubectl -n coco-tenant delete pod -l app=kbs --grace-period=0 --force
# 🟢 验证证明链路
kubectl logs -n coco-tenant deploy/kbs | grep -i attestation
```

### 案例 2：机密 VM 性能开销超出预期

- **现象**：机密 Pod 启动时间从 2s 增长到 30s+，CPU 密集型任务吞吐下降约 20%。
- **诊断**：`kata-runtime metrics` 显示 VM 启动占 90% 时间；SEV 内存加密使访存开销增大；页面交换触发机密页加密/解密。
- **修复**：预分配 VM 内存避免动态加密开销；机密 VM 用于高敏感数据而非通用算力；对性能敏感路径使用 TDX（性能开销小于 SEV）。

## 对比评测

| 维度 | CoCo（Kata+TEE） | Kata 普通模式 | 裸 TEE（SGX/SEV 直用） |
|---|---|---|---|
| 隔离强度 | 硬件级（TEE+VM） | VM 级（无 TEE） | 硬件级但无容器编排 |
| 工作负载兼容 | 标准 OCI 容器 | 标准 OCI 容器 | 需定制 SDK/镜像 |
| 编排集成 | RuntimeClass 透明接入 | RuntimeClass | 需自研集成 |
| 性能开销 | 中高（VM+TEE） | 中 | 低 |
| 密钥管理 | KBS 内置 | 无 | 需自建 |

- **选型建议**：需要透明容器体验 + 硬件级机密性选 CoCo；仅隔离不涉密选 Kata；极致性能且可改应用选裸 TEE。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| Pod 卡 ContainerCreating | RuntimeClass 缺失 / Kata 未装 | `kubectl get runtimeclass`、`crictl info` |
| attestation 校验失败 | KBS 策略过旧 / 证书过期 | `kubectl logs -l app=kbs`、`openssl x509 -in cert -noout -dates` |
| 密钥未注入 | KBS 网络不通 / 策略拒绝 | `kubectl get events --sort-by=.lastTimestamp` |
| 启动极慢 | VM 内存分配不足 | `kata-runtime metrics`、调整 memory 超卖 |
| 节点不支持 TEE | CPU/BIOS 未开启 | `cpuid | grep -i tdx/sgx`、BIOS 设置 |

## 生产部署清单

- [ ] 确认节点 CPU 支持目标 TEE 特性（SGX/TDX/SEV-SNP）并在 BIOS 开启
- [ ] 安装 CoCo Operator 与 Kata RuntimeClass，端到端验证普通 Pod 迁移
- [ ] 配置 KBS 高可用（多副本 + 持久化），证书使用权威 CA 签发
- [ ] 建立 attestation 策略基线（镜像白名单 + 固件版本），纳入 GitOps 管理
- [ ] 监控 TEE 可用节点数、证明成功率、VM 启动延迟并配置告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | attestation 大面积失败或密钥泄露风险 | 立即回滚到普通 Kata RuntimeClass，暂停机密工作负载 |
| P1 | TEE 驱动/固件升级 | 先升级策略基线，灰度迁移 10% 工作负载观察证明成功率 |
| P2 | CoCo 新版本发布 | 测试集群验证后按节点池滚动升级，保留回滚 RuntimeClass |

## 面试要点

> 以下 Q&A 覆盖机密容器面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Confidential Containers 与普通容器隔离的本质区别是什么？**
   A：普通容器依赖内核命名空间 + cgroups，隔离边界是软件层，云厂商/宿主机 root 仍可访问内存与磁盘数据；CoCo 用 TEE 硬件（SGX/TDX/SEV-SNP/CCA）把容器放进机密 VM，内存加密且 CPU 固件强制隔离，宿主机即使拿到内存镜像也无法解密，实现"数据可用但不可见"。

2. **Q：远程证明（Remote Attestation）在 CoCo 中起什么作用？**
   A：远程证明解决"如何信任运行环境"问题：attestation-agent 在 TEE 内生成带硬件签名的证据（SGX quote、SEV 报告），KBS 用权威参考值（固件、镜像哈希、策略）校验并判定该环境是否可信。只有证明通过，KBS 才释放工作负载密钥。它防止恶意宿主机伪装 TEE 窃取密钥。

3. **Q：CoCo 生产落地的关键成本与风险有哪些？**
   A：成本：VM 启动延迟（10-30s）、TEE 内存加密的性能开销（5-20%）、专用硬件与固件维护；风险：attestation 策略僵化导致升级困难、KBS 单点故障、密钥轮换复杂。建议：机密 VM 只承载高敏数据计算，通用算力保持普通 Pod，并让 KBS 与策略走 GitOps 审计闭环。

## 运维要点

- 部署形态：CoCo Operator + Kata + KBS 三件套；KBS 必须多副本 + 加密持久卷。
- 证书管理：KBS/AS 证书纳入统一 PKI，提前 30 天轮换预警；attestation 证书与节点绑定。
- 升级节奏：TEE 驱动、固件、Kata 版本三者联动升级，先升级策略基线再升级组件。
- 排障入口：先看 attestation 成功率指标，再查 KBS 日志与策略，最后查节点 TEE 状态。
- 安全基线：KBS 密钥加密存储、审计所有证明请求、限制 attestation 策略修改权限。

## 参考链接

- https://confidentialcontainers.org/
- https://github.com/confidential-containers

## Related

- [[17-系统基础/06-知识字典/fundamentals/kata-containers.md|Kata Containers]]
- [[17-系统基础/06-知识字典/security/vault.md|Vault]]
- [[17-系统基础/06-知识字典/security/spiffe-spire-identity.md|SPIFFE/SPIRE]]
