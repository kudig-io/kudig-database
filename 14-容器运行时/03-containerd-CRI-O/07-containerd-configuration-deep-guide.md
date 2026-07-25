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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| containerd 启动失败 | 配置文件语法错误 | `containerd config dump` | 校验 TOML 语法，恢复备份 |
| cgroup 驱动不匹配 | SystemdCgroup 配置错误 | `containerd config dump | grep SystemdCgroup` | 与 kubelet 保持一致 |
| 镜像拉取失败 | registry mirror 配置错误 | `crictl pull <image>` | 检查 certs.d 配置 |
| 插件加载失败 | 插件二进制缺失 | `journalctl -u containerd | grep plugin` | 安装对应插件 |
| 配置修改不生效 | 未重启服务 | `systemctl restart containerd` | 修改后必须重启 |
| 磁盘空间不足 | 数据目录配置不当 | `du -sh /var/lib/containerd/` | 配置独立数据盘 |
| OOM 事件 | 内存限制未配置 | `dmesg | grep oom` | 配置 cgroup 内存限制 |
| 日志过大 | 日志级别过高 | `du -sh /var/log/containerd*` | 调整为 info 级别 + 轮转 |

## 配置模板（生产环境）

```toml
# /etc/containerd/config.toml 生产配置示例
version = 2

[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
  [plugins."io.containerd.grpc.v1.cri".registry]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = ["https://mirror.internal:5000"]
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 配置管理 | 纳入 Ansible/Salt 版本化 | 避免手动修改 |
| 校验 | 重启前执行 `containerd config dump` | 确认配置有效 |
| cgroup | SystemdCgroup = true | 与 kubelet 保持一致 |
| 数据目录 | 使用独立数据盘 | 避免影响系统盘 |
| 日志 | info 级别 + logrotate | 避免磁盘占满 |
| 备份 | 修改前备份 config.toml | 便于回滚 |
| 升级 | 先 dump 配置再升级 | 确认新版本兼容性 |
| 监控 | 监控 containerd 进程资源 | CPU/内存/文件描述符 |

## 相关工具

| 工具 | 用途 | 使用方式 |
|------|------|----------|
| containerd | 运行时守护进程 | `systemctl status containerd` |
| containerd config | 配置校验 | `containerd config dump` |
| crictl | CRI 调试 | `crictl info` |
| toml-sort | TOML 格式化 | `pip install toml-sort` |
| Ansible | 配置管理 | playbook 自动化 |
| journalctl | 日志查看 | `journalctl -u containerd` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 配置文件在哪里？ | 默认 /etc/containerd/config.toml |
| 如何生成默认配置？ | `containerd config default > /etc/containerd/config.toml` |
| 修改配置后如何生效？ | `systemctl restart containerd` |
| version 2 和 3 的区别？ | v3 是 containerd 2.0 新格式，插件路径变化 |
| 如何配置多个运行时？ | 在 runtimes 下添加多个 runtime 配置块 |
| registry mirror 如何配置？ | 配置 certs.d 目录或 registry.mirrors |
| 如何限制 containerd 资源？ | 通过 systemd drop-in 配置 |
| 配置热加载支持吗？ | 不支持，必须重启服务 |

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| 镜像拉取慢 | 配置 mirror | registry.mirrors 指向内网 |
| 并发拉取 | 调整并发度 | max_concurrent_downloads = 10 |
| 磁盘 I/O 高 | 独立数据盘 | /var/lib/containerd 挂载 SSD |
| 内存占用高 | 调整 GC | 配置合理的 image gc 阈值 |
| 启动慢 | 减少插件 | 禁用不需要的插件 |
| 日志过大 | 调整级别 | info + logrotate |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| containerd_process_cpu | CPU 使用率 | > 80% |
| containerd_process_memory | 内存使用 | > 1GB |
| containerd_process_fds | 文件描述符 | > 10000 |
| containerd_restart_count | 重启次数 | > 0 |
| config_reload_errors | 配置加载错误 | > 0 |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| socket 权限 | 600，仅 root | 避免未授权访问 |
| 数据目录 | 独立挂载，权限 700 | 避免信息泄露 |
| 日志 | 不含敏感信息 | 避免记录 token |
| 插件 | 最小化启用 | 减小攻击面 |
| 升级 | 及时更新 | 修复已知漏洞 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| config v2 | config v3 | 升级 containerd 2.0→调整插件路径 |
| Docker | containerd | 安装 containerd→生成配置→迁移 |
| 单运行时 | 多运行时 | 添加 runtime 配置块 |
| 无 mirror | 有 mirror | 配置 certs.d 目录 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 配置语法 | `containerd config dump` | 无错误 |
| cgroup 驱动 | `grep SystemdCgroup` | true |
| 服务状态 | `systemctl status containerd` | active |
| CRI 连通 | `crictl info` | 返回 JSON |
| 镜像拉取 | `crictl pull <image>` | 成功 |
| 日志 | `journalctl -u containerd` | 无错误 |
| 磁盘 | `du -sh /var/lib/containerd/` | < 80% |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| config v1 | containerd 1.0-1.3 | 初始配置格式 |
| config v2 | containerd 1.4-1.7 | 插件路径规范化 |
| config v3 | containerd 2.0+ | 新插件架构，路径变化 |

## 架构对比

```text
containerd 配置层次：

/etc/containerd/config.toml
  ├── version (2 或 3)
  ├── root (数据目录)
  ├── state (状态目录)
  ├── [plugins]
  │    ├── cri (K8s 集成)
  │    │    ├── sandbox_image
  │    │    ├── containerd.runtimes
  │    │    └── registry.mirrors
  │    ├── snapshotter
  │    └── 其他插件
  └── [debug] (调试配置)
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 小集群 | 默认配置 | 足够 |
| 大集群 | 独立数据盘 + SSD | 性能 |
| 多运行时 | 多个 runtime 块 | 隔离 |
| 高并发 | max_concurrent=10 | 拉取性能 |

## 检查清单（补充）

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 配置版本 | `head -1 config.toml` | version = 2/3 |
| 数据目录 | `du -sh /var/lib/containerd/` | < 80% |
| 插件状态 | `containerd plugins ls` | 无错误 |
| 运行时 | `crictl info` | 包含预期 runtime |
| 日志 | `journalctl -u containerd` | 无错误 |

## 常见问题 FAQ（补充）

| 问题 | 解答 |
|------|------|
| config.toml 与命令行参数优先级？ | 命令行参数 > config.toml > 默认值 |
| 如何生成默认配置？ | `containerd config default > /etc/containerd/config.toml` |
| version 2 和 3 配置区别？ | v3 移除废弃插件，统一插件路径，推荐新部署使用 |
| 如何热重载配置？ | `systemctl reload containerd`，部分配置需重启 |
| 多运行时如何配置？ | 在 `[plugins."io.containerd.grpc.v1.cri".containerd.runtimes]` 下添加 |
| 如何配置镜像代理？ | 配置 `[plugins."io.containerd.grpc.v1.cri".registry.mirrors]` |
| 数据目录磁盘满了怎么办？ | 调整 GC 阈值或扩展磁盘，`containerd content ls` 检查 |
| 如何启用 metrics？ | 配置 `[metrics]` 段，暴露 /v1/metrics 端点 |

## 性能调优参数

| 参数 | 默认值 | 生产建议 | 说明 |
|------|--------|----------|------|
| `oom_score` | 0 | -999 | 降低 containerd 被 OOM kill 概率 |
| `max_recv_message_size` | 16MB | 32MB | 大镜像元数据 |
| `max_send_message_size` | 16MB | 32MB | 大镜像元数据 |
| `grpc_max_recv_message_size` | 16MB | 32MB | gRPC 接收限制 |
| `debug.level` | info | warn | 生产减少日志量 |
| `snapshotter` | overlayfs | 按硬件选择 | 存储驱动选择 |
| `disable_snapshot_annotations` | false | true | 减少元数据开销 |

## 相关文档

- [[14-容器运行时/03-containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]
- [[14-容器运行时/03-containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]]
- [[14-容器运行时/05-运行时迁移/01-docker-to-containerd-migration.md|Docker 到 containerd 迁移]]

<!-- risk-assessed -->
