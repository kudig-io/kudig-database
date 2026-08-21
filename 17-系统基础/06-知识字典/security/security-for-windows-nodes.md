---
title: Windows 节点安全
description: '# Windows 节点安全'
summary: '# Windows 节点安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Windows 节点安全 是什么
- 如何 Windows 节点安全
trigger_keywords:
- Windows
- 节点安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Windows 节点安全

## 概述

本页面描述了针对 Windows 操作系统的安全考虑和最佳实践。Windows 节点在 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 集群中的行为与 Linux 节点存在显著差异，特别是在 Secret 保护、容器用户和 Pod 安全隔离方面。

## 核心概念/原理

### Secret 数据保护

在 Windows 节点上，Secret 数据会以**明文**形式写入节点的本地存储（而 Linux 使用 tmpfs/内存文件系统）。因此，作为集群管理员，需要采取额外的保护措施。

### 容器用户

- Windows 容器提供两个默认用户账户：**ContainerUser** 和 **ContainerAdministrator**。
- 可以使用 `RunAsUsername` 为 Windows Pod 或容器指定以特定用户身份执行容器进程，这大致相当于 Linux 的 `RunAsUser`。
- 也可以在容器构建过程中向镜像添加本地用户，或者利用 **Group Managed [[service|Service]]Service Accounts]]（gMSA）** 以 Active Directory 身份运行容器。

### Pod 级安全隔离

- Linux 特有的 Pod 安全上下文机制（如 SELinux、AppArmor、Seccomp 和自定义 POSIX capabilities）在 Windows 节点上**不受支持**。
- Windows 不支持特权容器（Privileged containers），但可以使用 **HostProcess 容器**来执行许多在 Linux 上由特权容器完成的任务。

## 关键机制或特性

- **文件 ACL（访问控制列表）**：用于保护 Windows 节点上 [[secrets|Secrets]] 的文件位置。
- **BitLocker**：提供卷级加密，保护节点本地存储上的数据。
- **HostProcess 容器**：允许在 Windows 主机上以特权方式运行进程，是 Windows 上特权操作的替代方案。
- **RunAsUsername / gMSA**：支持以特定用户或 Active Directory 身份运行容器。

## 使用场景

- 在 Windows 节点上运行需要访问 Secret 的容器化工作负载。
- 需要在 Windows 容器中执行特权管理任务（如网络配置、系统管理）。
- 使用 Active Directory 身份集成运行 Windows 容器的企业环境。

## 最佳实践/注意事项

- **使用文件 ACL 和 BitLocker**：由于 Windows 不使用 tmpfs，必须额外保护 Secret 文件位置和存储卷。
- **理解 ContainerUser 与 ContainerAdministrator 的区别**：根据应用需求选择最小权限的默认账户，避免不必要的管理员权限。
- **使用 HostProcess 容器替代特权容器**：在 Windows 上执行需要主机访问权限的任务时，优先使用 HostProcess 容器。
- **注意 Linux 安全机制在 Windows 上无效**：不要期望 SELinux、AppArmor、Seccomp 等在 Windows 节点上生效；需要采用 Windows 原生的安全控制措施。
- **为本地存储启用 BitLocker 加密**：降低物理访问或底层存储泄露导致的数据暴露风险。

## 架构深度解析

### Windows 节点安全模型

```
┌──────────────────────────────────────────────────────────────┐
│  Windows Node（kubelet + containerd）                          │
│  ├─ 进程隔离：Windows Server 容器（Job Object / 命名空间）     │
│  │   └─ Hyper-V 隔离（内核级隔离，v1.26+ 支持 HostProcess）    │
│  ├─ 安全上下文差异：                                          │
│  │   ├─ 不支持：privileged / capabilities / SELinux /         │
│  │   │   seccomp / AppArmor / readOnlyRootFilesystem          │
│  │   ├─ 支持：runAsUsername / runAsNonRoot /                  │
│  │   │   windowsOptions（gmsaCredentialSpec / hostProcess）    │
│  │   └─ HostProcess 容器：访问 Windows 主机（需 Windows 10+）  │
│  ├─ 网络：Windows 防火墙（HNS）+ NetworkPolicy（Calico/        │
│  │   Antrea 支持 Windows）                                    │
│  └─ 存储：CSI 卷（SMB/azureDisk 等），NTFS 权限                │
└──────────────────────────────────────────────────────────────┘
│ 与 Linux 节点的差异点（PSA 不适用于 Windows Pod）              │
│ ├─ PodSecurityContext 多数字段忽略（不报错）                  │
│ ├─ seccomp/kernel hardening 不可用                            │
│ └─ 需使用 Windows-specific 安全基线                           │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| Windows 支持 | `pkg/kubelet/nodestatus/` | 节点 OS 识别与特性上报 |
| Windows 安全 | `pkg/securitycontext/` | windowsOptions 处理 |
| HostProcess | `pkg/kubelet/container/` | hostProcess 容器启动逻辑 |
| GMSA | `pkg/kubelet/kubelet_pods.go` | gmsaCredentialSpec 注入 |

### 流程步骤

1. 节点注册时上报 `kubernetes.io/os=windows`，调度器按节点选择器分发。
2. Pod 定义 `securityContext.windowsOptions`（runAsUsername/gmsa/hostProcess）。
3. kubelet 在 Windows 上启动容器时应用 Windows 特定的隔离与凭据配置。
4. 不支持字段（privileged/capabilities/seccomp）被忽略但不报错——需在策略层兜底。
5. 网络策略由支持 Windows 的 CNI（Calico/Antrea）执行。

## 生产案例

### 案例 1：GMSA 配置错误导致 AD 集成应用全部启动失败

| 时间 | 事件 |
| --- | --- |
| T+0 | 迁移 AD 认证应用（.NET）到 Windows 节点，启用 GMSA |
| T+30min | 容器全部 CrashLoopBackOff，报"找不到凭据" |
| T+2h | 定位：gmsaCredentialSpec 引用的 ConfigMap 与集群不匹配，CCG 插件未安装 |
| T+5h | 安装 CCG 插件、修正 credential spec 并重启工作负载 |
| T+1d | 验证 AD 集成应用正常读写域资源 |

- **根因分析**：Windows 容器访问 AD 需要 GMSA + CCG（Container Credential Guard）插件；credential spec 内容与域环境不匹配时静默失败，错误信息不直观。
- **修复命令**：
```bash
# 1. 检查 gmsa 配置（只读）
kubectl get configmap gmsa-spec -n kube-system -o yaml
kubectl get pod <pod> -o yaml | grep -A5 windowsOptions
# 2. 校验节点是否安装 CCG 插件（PowerShell）
Get-Service ccg.exe
# 3. 修正后重启（🟡 中风险）
kubectl rollout restart deployment/ad-app -n prod
```

### 案例 2：NetworkPolicy 对 Windows Pod 不生效导致安全审计失败

| 时间 | 事件 |
| --- | --- |
| T+0 | 合规审计要求全部 Pod 实施 NetworkPolicy |
| T+1w | 审计发现 Windows 命名空间 Pod 可自由访问外网 |
| T+3d | 定位：默认 CNI（Flannel/windows 模式）不支持 NetworkPolicy，未启用 Calico |
| T+2w | 迁移到 Calico（Windows 支持）、验证策略生效，审计通过 |

- **根因分析**：Windows 节点的网络栈（HNS）与 Linux 不同，NetworkPolicy 必须由支持 Windows 的 CNI 实现；Flannel Windows 模式不支持策略。
- **修复命令**：
```bash
# 1. 确认 CNI 能力（只读）
kubectl get ds -n kube-system | grep -iE "calico|flannel|antrea"
# 2. 为 Windows 节点部署 Calico（🔴 高风险：网络变更需维护窗口）
kubectl apply -f https://docs.projectcalico.org/manifests/calico-windows.yaml
# 3. 验证策略对 Windows Pod 生效
kubectl exec -n app win-pod -- powershell Test-NetConnection db-svc -Port 5432  # 🟢 只读
```

## 对比评测

| 维度 | Linux Pod | Windows Pod（Process） | Windows HostProcess |
| --- | --- | --- | --- |
| 隔离级别 | 内核 namespace | Job Object | 主机进程 |
| 安全字段 | capabilities/seccomp | 仅 windowsOptions | 需审批（特权） |
| 镜像兼容 | OCI/Linux | OCI/Windows | 宿主 OS 匹配 |
| 典型场景 | 通用服务 | .NET/AD 集成 | 节点运维（日志/网络） |

**选型建议**：默认 Process 隔离；强隔离需求用 Hyper-V 隔离（v1.26+）；节点级运维任务用 HostProcess 但严格审批。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 容器无法启动 | 镜像平台不匹配（linux/amd64） | 检查镜像 manifest：`docker manifest inspect` |
| GMSA 失败 | CCG 未安装/凭据 spec 错误 | 检查节点 CCG 服务与 ConfigMap |
| 网络策略无效 | CNI 不支持 Windows | 换 Calico/Antrea 并验证 |
| 权限不足 | runAsUsername 无效/组缺失 | 使用 gmsa 或自定义账号 |
| 节点 NotReady | Windows 补丁/containerd 版本 | `Get-EventLog -LogName System` 排查 |

## 生产部署清单

- [ ] 确认镜像平台为 windows/amd64（servercore/sac 版本匹配）
- [ ] Windows 专用安全基线文档化（PSA 不覆盖的字段清单）
- [ ] GMSA/CCG 插件在全部 Windows 节点预装并验证
- [ ] 部署支持 Windows 的 CNI（Calico/Antrea）并验证 NetworkPolicy
- [ ] HostProcess 容器白名单审批流程

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 安全审计要求 Windows Pod 网络隔离但 CNI 不支持 | 优先迁移 Calico/Antrea |
| P1 | 关键应用依赖 GMSA 但节点无 CCG | 全部 Windows 节点补装并验证 |
| P2 | 混合节点集群无 Windows 安全基线文档 | 建立文档与巡检项 |

## 面试要点

1. **Q：Windows 节点上哪些 Kubernetes 安全特性不可用？**
   A：privileged、capabilities、SELinux、seccomp、AppArmor、readOnlyRootFilesystem 等 Linux 内核安全字段均不支持（被忽略不报错）。可用的是 runAsUsername、runAsNonRoot 与 windowsOptions（GMSA/HostProcess）。因此 Pod Security Admission 无法直接约束 Windows Pod，需配合 Windows 特定基线。
2. **Q：什么是 HostProcess 容器？什么时候用？**
   A：HostProcess 容器（v1.22+ alpha，1.26+ beta）直接运行在 Windows 主机上，拥有节点权限，用于节点级运维（日志采集、网络配置、安全代理），相当于 Windows 版的特权容器。应严格审批、限定镜像与账号。
3. **Q：混合集群（Linux + Windows）安全策略如何设计？**
   A：分层：一是节点层面按 OS 分别加固（Windows 补丁、防火墙、GMSA 最小化）；二是网络层面用支持双平台的 CNI 统一 NetworkPolicy；三是策略层面 PSA 管 Linux Pod，Windows Pod 用 Kyverno/Gatekeeper 自定义规则兜底；四是镜像层面统一供应链扫描。

## 运维要点

- 补丁管理：Windows 节点补丁周期独立于集群升级，纳入月度窗口。
- 防火墙：HNS 与 Windows 防火墙联动，出网默认白名单。
- 审计：Windows 事件日志（Security）对接 SIEM，覆盖 AD 集成场景。
- 容量规划：Windows 节点内存开销高于 Linux，预留 20-30% 余量。
- 排障入口：先分 OS 排查（Windows 事件日志 vs kubelet 日志），再查安全相关配置。

## 参考链接

- https://kubernetes.io/docs/concepts/security/windows-security/

## Related

- [[17-系统基础/06-知识字典/security/admission-controller.md|准入控制器]]
- [[17-系统基础/06-知识字典/security/application-security-checklist.md|应用安全清单]]
- [[17-系统基础/06-知识字典/security/athenz.md|Athenz 身份认证与授权]]


<!-- risk-assessed -->
