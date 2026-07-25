---
title: Flatcar Container Linux (entities)
description: '## 概述'
summary: 'Flatcar Container Linux 是为容器优化的不可变 Linux 发行版，是 CoreOS Container Linux 的延续和替代品。它提供最小化、自动更新、安全的容器运行环境。'
category: entities
tags:
- k8s
- cncf
- runtime
- flatcar
- etcd
- containerd
- crd
- operator
- serverless
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flatcar Container Linux 是什么
- 如何 Flatcar Container Linux
trigger_keywords:
- Flatcar
- Container
- Linux
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flatcar Container Linux

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Shell, Go

## 概述

Flatcar Container Linux 是为容器优化的不可变 Linux 发行版，是 CoreOS Container Linux 的延续和替代品（CoreOS 被 Red Hat 收购后停止维护）。Flatcar 由 Kinvolk（现 Microsoft）维护，2019 年加入 CNCF Sandbox，后晋升为 Incubating。它提供最小化、自动更新、安全的容器运行环境，是运行 Kubernetes 节点操作系统的理想选择之一。

## 核心特性

- **不可变基础设施**: 只读根文件系统（/usr），配置通过 Ignition 声明式管理
- **自动更新**: 内置 A/B 分区原子更新机制，回滚只需重启
- **最小化设计**: 只包含运行容器必需的组件，无包管理器
- **安全加固**: SELinux、只读 rootfs、自动安全补丁、内核模块签名
- **多平台支持**: AWS、Azure、GCP、VMware、裸金属、Equnix Metal
- **CoreOS 兼容**: 完全兼容 CoreOS Container Linux 的使用模式

## 架构

Flatcar 采用不可变 OS 设计理念。系统分区以只读方式挂载（/usr 为只读），用户配置和数据存储在 /etc 和 /var 中。Ignition（替代 cloud-init）在首次启动时从 JSON 配置（user-data）中配置用户、网络、systemd 服务和文件。自动更新使用 update-engine（后台检查更新）和 locksmith（协调重启），采用 A/B 分区方案实现原子更新——新系统写入备用分区，重启时切换。所有更新通过 Omaha 协议从 Flatcar 更新服务器拉取。

## Kubernetes 集成

Flatcar 是运行 Kubernetes 节点的理想 OS。只读 rootfs 消除了操作系统层面的配置漂移。Ignition 配置文件声明式定义节点初始化（网络、Docker/containerd、kubelet 参数）。自动更新确保安全补丁及时安装，配合 Kured 协调节点重启。容器运行时（containerd）预装或通过 Ignition 安装。在裸金属集群中，Flatcar + Ignition + Matchbox 实现完全自动化的 PXE 部署。

## 生产使用场景

1. **裸金属 Kubernetes**: 在自建数据中心使用 Flatcar 作为节点 OS，实现不可变基础设施
2. **安全合规**: 自动安全更新和只读 rootfs 满足等保和 SOC2 合规要求
3. **大规模部署**: 通过 Ignition + Matchbox 实现 PXE 批量部署
4. **边缘 IoT**: 在资源受限的边缘设备上运行轻量级容器

## 安装与配置

```bash
# Ignition 配置示例（config.ign）
{
  "ignition": { "version": "3.3.0" },
  "systemd": {
    "units": [
      { "name": "docker.service", "enabled": true },
      { "name": "containerd.service", "enabled": true }
    ]
  },
  "passwd": {
    "users": [{
      "name": "core",
      "sshAuthorizedKeys": ["ssh-ed25519 AAA..."]
    }]
  },
  "storage": {
    "files": [{
      "path": "/etc/sysctl.d/max-user-watches.conf",
      "contents": { "source": "data:,fs.inotify.max_user_watches=524288" }
    }]
  }
}
# 在云平台使用 Flatcar 镜像并传入 Ignition 配置作为 user-data
```

```yaml
# Kubernetes 节点 Ignition 配置 (kubeadm)
variant: flatcar
version: 1.0.0
systemd:
  units:
    - name: containerd.service
      enabled: true
    - name: kubelet.service
      enabled: true
      contents: |
        [Unit]
        Description=Kubernetes Kubelet
        After=containerd.service
        [Service]
        ExecStart=/opt/bin/kubelet \
          --container-runtime-endpoint=unix:///run/containerd/containerd.sock \
          --config=/etc/kubernetes/kubelet-config.yaml
        Restart=always
storage:
  files:
    - path: /etc/kubernetes/kubelet-config.yaml
      contents:
        inline: |
          apiVersion: kubelet.config.k8s.io/v1beta1
          kind: KubeletConfiguration
          maxPods: 110
          evictionHard:
            memory.available: "500Mi"
            nodefs.available: "10%"
```

## 运维操作

```bash
# 🟢 检查 Flatcar 版本
ssh core@<node> cat /etc/flatcar/update.conf
ssh core@<node> cat /usr/share/flatcar/version.txt

# 🟢 检查自动更新状态
ssh core@<node> systemctl status update-engine
ssh core@<node> journalctl -u update-engine --tail=20

# 🟢 检查当前活动分区
ssh core@<node> flatcar-update-engine --status
ssh core@<node> cgpt show /dev/sda

# 🟡 手动触发更新检查
ssh core@<node> sudo update_engine_client --check_for_update

# 🟡 配置更新通道
ssh core@<node> sudo sed -i 's/GROUP=.*/GROUP=stable/' /etc/flatcar/update.conf

# 🟡 禁用自动更新 (维护期间)
ssh core@<node> sudo systemctl mask update-engine
ssh core@<node> sudo systemctl mask locksmithd

# 🟡 重新启用自动更新
ssh core@<node> sudo systemctl unmask update-engine
ssh core@<node> sudo systemctl unmask locksmithd

# 🔴 手动回滚到上一版本
ssh core@<node> sudo flatcar-update-engine --rollback
ssh core@<node> sudo systemctl reboot

# 🟢 检查节点重启协调 (配合 Kured)
kubectl get pods -n kured -o wide
kubectl logs -n kured -l app=kured --tail=20
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 节点无法启动 | Ignition 配置错误 | 检查控制台日志 | 修复 Ignition 配置 |
| 自动更新失败 | 网络无法访问更新服务器 | `journalctl -u update-engine` | 检查网络/代理配置 |
| 更新后服务异常 | 新版本不兼容 | `flatcar-update-engine --status` | 回滚到上一版本 |
| 磁盘空间不足 | /var 分区写满 | `df -h` | 清理容器/日志 |
| SSH 无法连接 | 密钥未配置/防火墙 | 检查安全组/密钥 | 修复 SSH 配置 |
| containerd 未启动 | 配置错误 | `systemctl status containerd` | 检查 config.toml |

### 排查流程

```
Flatcar 节点异常
├── 节点无法启动
│   ├── 检查云平台控制台日志
│   ├── 验证 Ignition 配置语法 (ignition-validate)
│   ├── 检查磁盘分区状态
│   └── 尝试 PXE/ISO 救援模式
├── 更新问题
│   ├── systemctl status update-engine → 检查更新服务
│   ├── journalctl -u update-engine → 查看更新日志
│   ├── 检查网络连通性 (public.update.core-os.net)
│   └── flatcar-update-engine --rollback → 回滚
└── 容器运行时问题
    ├── systemctl status containerd → 检查服务状态
    ├── crictl ps → 检查容器状态
    ├── journalctl -u containerd → 查看日志
    └── 检查 /etc/containerd/config.toml
```

## 生产案例

### 案例 1: 裸金属 K8s 集群不可变基础设施

- **场景**: 自建数据中心 50 节点 K8s 集群，需要安全合规的节点 OS
- **排查**: Ubuntu 节点配置漂移严重；安全补丁管理混乱
- **方案**: 迁移到 Flatcar；Ignition 声明式配置；自动更新 + Kured 协调重启；只读 rootfs 消除漂移
- **效果**: 配置漂移归零；安全补丁 24h 内自动安装；合规审计通过

### 案例 2: 自动更新导致内核模块不兼容

- **场景**: Flatcar 自动更新后，某 GPU 节点 NVIDIA 驱动加载失败
- **排查**: 新内核版本与 NVIDIA 驱动不兼容；update-engine 日志显示更新成功
- **方案**: 回滚到上一版本；配置更新通道为 stable (更保守)；GPU 节点单独管理更新窗口
- **效果**: GPU 节点恢复正常；更新策略更精细化

## 对比与替代方案

| 维度 | Flatcar | Talos Linux | Bottlerocket | Ubuntu + kubeadm |
|------|---------|-------------|--------------|------------------|
| 不可变 OS | ✅ | ✅ | ✅ | ❌ |
| 自动更新 | ✅ A/B 分区 | ✅ 不可变 | ✅ | ❌ 手动 |
| 包管理器 | ❌ | ❌ | ❌ | ✅ apt |
| K8s 专属 | ❌ 通用 | ✅ | ✅ | ❌ 通用 |
| 配置方式 | Ignition | API | TOML | cloud-init |
| 社区规模 | 中 | 中 | AWS 支持 | 最大 |
| 适用场景 | CoreOS 用户 | 全新 K8s 集群 | AWS 环境 | 通用 |

## 检查清单

- [ ] Flatcar 版本为最新 stable
- [ ] Ignition 配置已验证 (ignition-validate)
- [ ] 自动更新已启用 (update-engine + locksmithd)
- [ ] Kured 已部署协调节点重启
- [ ] SSH 密钥已配置
- [ ] 监控覆盖节点健康状态
- [ ] 回滚方案已验证
- [ ] 容器运行时配置正确

## 参考链接

- [[etcd]]
- [[containerd]]
- [[22-概念/15-运行时与系统/container-runtime-comparison.md|container-runtime-comparison]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[23-实体/09-编排调度/kured.md|kured]] — Kured

## Related

- [[serverless-devs]] — Serverless Devs
- [[sermant]] — Sermant
- [[loxilb]] — LoxiLB
- [[kube-ovn]] — Kube-OVN
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference

<!-- risk-assessed -->
