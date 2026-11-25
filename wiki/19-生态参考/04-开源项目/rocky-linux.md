---
title: Rocky Linux
description: Rocky Linux 是 CentOS 联合创始人发起的 RHEL 兼容企业级 Linux 发行版，CentOS 停更后的主流免费替代
summary: 'Rocky Linux 是与 RHEL 逐 bug 兼容的下游重建发行版，由 Rocky Enterprise Software Foundation（RESF）治理，广泛用作 Kubernetes 节点操作系统与 CentOS 迁移目标。'
category: entities
tags:
- linux
- distro
- rocky-linux
- rhel
- centos
- rocky
- os
- k8s-node
tier: supporting
created: '2026-11-18'
last_updated: 2026-11
difficulty: beginner
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 7min
intent_queries:
- Rocky Linux 是什么
- Rocky Linux 与 AlmaLinux CentOS 的区别
- CentOS 停更后如何迁移到 Rocky Linux
- Kubernetes 节点用 Rocky Linux
trigger_keywords:
- Rocky Linux
- rocky
- RHEL 兼容
- CentOS 替代
prerequisites:
- linux-basics
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与目标主机是否正确；是否具备相应的 root/订阅权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改系统状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Rocky Linux

> **治理方**: Rocky Enterprise Software Foundation（RESF） | **类别**: 企业级 Linux 发行版 | **上游**: Red Hat Enterprise Linux（下游重建）

## 概述

Rocky Linux 由 CentOS 联合创始人 **Gregory Kurtzer** 于 2020 年 12 月发起——起因是 Red Hat 宣布 CentOS 8 提前终止（EOL 从 2021 年底提前到 2020 年底），社区急需一个延续 CentOS 定位的免费企业级发行版。项目以 CentOS 早期成员 Rocky McGaugh 命名，由非营利组织 **Rocky Enterprise Software Foundation（RESF）** 治理。Rocky Linux 与 Red Hat Enterprise Linux **逐 bug 兼容**：以 RHEL 公开发布的源码包为上游原样重建、去除商标，二进制层面可直接运行 RHEL 的软件与补丁，因此成为国内迁移 CentOS、构建 K8s 节点镜像的主流选择之一，与 AlmaLinux 并列为两大 CentOS 替代品。

## 核心特性

- **RHEL 兼容**: 下游重建，与 RHEL 逐 bug 兼容，EL RPM 生态与 RHEL 完全互通
- **免费且开放**: 无订阅成本，ISO 与镜像全球镜像站分发，RESF 开放治理
- **版本跟随上游**: 大版本与 RHEL 对齐（8/9/10），小版本季度更新，生命周期与 RHEL 一致
- **补丁与内核**: 维护自有 kpatch 内核热补丁（含 Secure Boot 签名支持），安全公告与上游同步
- **质量保障**: 使用 openQA 做发行前自动化测试，覆盖安装、升级与桌面/服务器场景
- **云与容器生态**: AWS/GCP/Azure/主流云厂商提供官方镜像；`quay.io/rockylinux` 提供官方容器基础镜像

## 版本与生命周期

| 大版本 | 发布 | 对应 RHEL | 标准 EOL（对齐上游） | 状态 |
|---|---|---|---|---|
| Rocky 8 | 2021-04 | RHEL 8 | 2029-05 | 维护中 |
| Rocky 9 | 2022-07 | RHEL 9 | 2032-05 | 主力推荐 |
| Rocky 10 | 2025-06 | RHEL 10 | 2035 | 最新大版本 |

- CentOS 7（2024-06 EOL）与 CentOS 8（2021-12 提前终止）的迁移目标默认就是 Rocky Linux / AlmaLinux
- 跨大版本（如 8 → 9/10）**没有官方原地升级路径**，业界主流做法是全新安装 + 配置与应用迁移；小版本升级走 `dnf update`
- 行为变更提示：Rocky 8 起默认不启用 swap（K8s 节点反而需要显式处理 swap），与 CentOS 7 时代习惯不同

## 与相关发行版的关系

```text
Fedora ──上游──> CentOS Stream ──上游──> RHEL
                     │                      │
                     │          下游源码重建（去商标）
                     ▼                      ▼
               Rocky/Alma 的参考       Rocky Linux · AlmaLinux · Oracle Linux
```

- **CentOS Stream**：RHEL 的上游滚动预览，不是 CentOS 8 的"替代品"，Rocky/Alma 才是传统 CentOS（下游）定位的继承者
- **AlmaLinux**：同为 RHEL 下游重建，出身 CloudLinux，提供官方 in-place 升级工具 ELevate；选型上与 Rocky 无功能差异，主要看社区治理偏好
- **RHEL**：商业订阅版，Rocky 兼容但不包含 RHEL 的订阅支持与认证矩阵（部分 ISV 认证只覆盖 RHEL，需按需验证）

## Kubernetes 中的应用

Rocky Linux 是主流 K8s 节点操作系统之一（见 [[wiki/01-集群基础与系统/09-Linux/16-k8s-node-os-support-matrix.md|K8s 节点操作系统支持矩阵]]，支持 8.x/9.x，RHEL 兼容档位）：

- **节点准备**：关闭/规划 swap、加载 br_netfilter/overlay 模块、配置容器运行时（containerd）、SELinux 可保持 `enforcing`（RHEL 系 K8s 支持 permissive→enforcing 路径）
- **基线加固**：节点镜像按 [[wiki/01-集群基础与系统/09-Linux/11-k8s-node-os-image-hardening-baseline.md|K8s 节点 OS 镜像加固基线]] 执行（最小化安装、CIS 基线、自动安全更新策略）
- **镜像生产**：Packer/镜像流水线产出 golden image，或用 kubean/kubespray 等工具批量安装（对 Rocky/EL 系均有内置支持）
- **基础镜像**：应用容器用 `quay.io/rockylinux/rocky` 官方镜像，与节点 OS 同源减少 glibc/ABI 差异

## 运维操作

```bash
# 🟢 查看版本与小版本（只读）
cat /etc/os-release && cat /etc/rocky-release

# 🟢 查看已启用的仓库（只读）
dnf repolist

# 🟡 小版本常规升级（修改系统状态，升级前务必建快照/备份）
dnf -y update

# 🟡 CentOS 8 系 → Rocky 8 迁移（社区工具 migrate2rocky，执行前确认业务窗口）
sudo bash migrate2rocky.sh
```

## 故障排查

- **dnf 报 GPG key / repository 错误**：多见于迁移或镜像站同步滞后，核对 `rocky-release` 包版本与仓库 `baseurl`；切换镜像站后 `dnf clean all && makecache`
- **mirrorlist 404**：镜像站下线或仓库路径变更，改用 `repo.d` 中的备用镜像或官方 `mirrors.rockylinux.org` 生成器
- **内核热补丁/驱动在 Secure Boot 下加载失败**：确认 kpatch 与 shim 签名链；第三方驱动需自签或关闭 Secure Boot（按安全基线评估）
- **K8s 节点起不来**：优先核对 swap 是否关闭、SELinux 策略拒绝（`ausearch -m avc`）、firewalld/NetworkPolicy 与云安全组端口矩阵
- **跨大版本升级失败**：Rocky 无官方 in-place 路径，不要用 `dnf system-upgrade` 强行跨 8→9；走重装迁移（配置文件、应用单元、容器数据卷按清单搬迁）

## 生产最佳实践

- 新建集群节点统一选 **Rocky 9**（或随 RHEL 10 成熟后评估 10），全集群单一小版本基线
- 订阅安全公告渠道（RESF/镜像站邮件列表），把 EOL 日期写入 CMDB 并提前一年规划升级
- 镜像构建固定小版本仓库快照（vault/时间点镜像），避免滚动更新导致不可复现构建
- 节点 OS 加固、内核参数与容器运行时配置全部进版本库（IaC），禁止手工漂移
- ISV/商业软件有 RHEL 认证要求时，先在 Rocky 上做兼容性验证再批量部署

## 对比

| 维度 | Rocky Linux | AlmaLinux | CentOS Stream | RHEL |
|---|---|---|---|---|
| 定位 | RHEL 下游重建（免费） | RHEL 下游重建（免费） | RHEL 上游滚动预览 | 商业订阅 |
| 兼容性 | 与 RHEL 逐 bug 兼容 | 与 RHEL 逐 bug 兼容 | 领先 RHEL 半个版本 | 基准 |
| 原地升级工具 | 无官方（重装为主） | ELevate | 滚动即升级 | leapp（有限场景） |
| 治理 | RESF（非营利） | AlmaLinux 基金会 | Red Hat 主导 | Red Hat |
| 企业支持 | RESF 订阅计划 | 商业支持伙伴 | 无（随 RHEL 订阅） | 官方订阅 + 认证矩阵 |
| 典型选择理由 | CentOS 迁移、国产化替代 | 同左，偏好基金会治理 | 想提前看到 RHEL 变化 | 需要认证与官方支持 |

## 参考链接

- 官方网站：https://rockylinux.org/
- GitHub：https://github.com/rocky-linux
- RESF：https://rockylinux.org/rocky-linux-faqs/
- 生命周期与 EOL：https://rockylinux.org/download/

## Related

- [[wiki/01-集群基础与系统/09-Linux/16-k8s-node-os-support-matrix.md|K8s 节点操作系统支持矩阵]]
- [[wiki/01-集群基础与系统/09-Linux/11-k8s-node-os-image-hardening-baseline.md|K8s 节点 OS 镜像加固基线]]
- [[wiki/01-集群基础与系统/09-Linux/17-k8s-node-os-issues.md|K8s 节点 OS 问题]]
- [[wiki/19-生态参考/README.md|生态参考域]]
