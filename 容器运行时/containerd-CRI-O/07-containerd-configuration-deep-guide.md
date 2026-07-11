---
title: containerd config.toml 深度配置指南
description: containerd config.toml 全量字段解析，涵盖 snapshotter、CNI、registry mirror、runtime shim 与版本迁移
summary: containerd config.toml 全量字段解析，涵盖 snapshotter、CNI、registry mirror、runtime shim 与版本迁移
category: container-runtime
tags:
- containerd
- cri
- runtime
- config
- snapshotter
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 平台工程师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# containerd config.toml 深度配置指南

## 概述

`/etc/containerd/config.toml` 是 containerd 守护进程的核心配置入口，控制 CRI 插件、snapshotter、CNI、registry mirror、runtime shim 与流式日志。containerd 1.7+/2.0 采用 `version = 2` schema，旧字段（如 `registry.mirrors`）在 2.0 中迁移至 `config_path` 目录式配置。

## 配置文件骨架

```toml
version = 2
root = "/var/lib/containerd"
state = "/run/containerd"
# 在 shim 异常时是否让 containerd 退出（生产建议 false）
disabled_plugins = []
oom_score = -999

[grpc]
  address = "/run/containerd/containerd.sock"
  uid = 0
  gid = 0

[debug]
  level = "info"
```

## CRI 插件关键段

```toml
[plugins."io.containerd.grpc.v1.cri"]
  # sandbox（pause）镜像，必须可达
  sandbox_image = "registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9"
  # 容器最大并发镜像拉取数
  max_concurrent_downloads = 5

  [plugins."io.containerd.grpc.v1.cri".containerd]
    # 默认 snapshotter
    snapshotter = "overlayfs"
    default_runtime_name = "runc"
    # Pod 沙?箱镜像按需拉取（懒加载）
    disable_snapshot_annotations = false

    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
```

## Registry Mirror（两种写法）

### containerd 1.x：内联 mirrors

```toml
[plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
  endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
[plugins."io.containerd.grpc.v1.cri".registry.configs."registry.internal:5000".tls]
  insecure_skip_verify = true
```

### containerd 2.0：目录式 config_path

```toml
[plugins."io.containerd.grpc.v1.cri".registry]
  config_path = "/etc/containerd/certs.d"
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为 docker.io 配置 mirror host
sudo mkdir -p /etc/containerd/certs.d/docker.io
sudo tee /etc/containerd/certs.d/docker.io/hosts.toml <<'EOF'
server = "https://registry-1.docker.io"
[host."https://registry.cn-hangzhou.aliyuncs.com"]
  capabilities = ["pull", "resolve"]
EOF
```

## CNI 配置

```toml
[plugins."io.containerd.grpc.v1.cri".cni]
  bin_dir = "/opt/cni/bin"
  conf_dir = "/etc/cni/net.d"
  conf_template = ""
  # 默认网络名称，与 kubelet --cni-conf-dir 中文件 name 字段对应
  use_internal_loopback_ip = true
```

## Shim 与 RuntimeClass

每个 runtime entry 可绑定不同的 OCI runtime（runc/runsc/kata），供 RuntimeClass handler 引用：

```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
  runtime_type = "io.containerd.runsc.v1"
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
  runtime_type = "io.containerd.kata.v2"
```

## 校验与重载

> ⚠️ **🟠 高危操作** — 影响节点上所有容器，需变更窗口与回滚

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 语法校验 → 备份 → 重启
sudo containerd config dump >/dev/null
sudo cp /etc/containerd/config.toml{,.bak}
sudo systemctl restart containerd
```

## 1.x → 2.0 迁移要点

| 变更项 | 1.x | 2.0 |
|---|---|---|
| registry | `registry.mirrors.*` 内联 | `config_path` 目录式 |
| CRI sandbox | `io.containerd.grpc.v1.cri` | 同名但部分字段移除 |
| shim v1 | 支持 | 已移除，仅 shim v2 |

## 生产检查清单

- [ ] `sandbox_image` 指向内网可达 pause 镜像
- [ ] `SystemdCgroup = true` 与 kubelet cgroup 驱动一致
- [ ] registry mirror 已配置且 `crictl pull` 验证通过
- [ ] 配置文件已纳入配置管理（Ansible/Salt）并做版本化
- [ ] 重启前已执行 `containerd config dump` 校验

## 相关文档

- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]
- [[容器运行时/containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]]
- [[容器运行时/运行时迁移/01-docker-to-containerd-migration.md|Docker 到 containerd 迁移]]

<!-- risk-assessed -->
