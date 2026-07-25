---
title: Manpage 安装指南
description: '# Manpage 安装指南'
summary: '# Manpage 安装指南'
category: general
tags:
- k8s
- etcd
- prometheus
- istio
- cilium
- helm
- argocd
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Manpage 安装指南 是什么
- 如何 Manpage 安装指南
trigger_keywords:
- Manpage
- 安装指南
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- cilium-basics
- etcd-basics
- tls-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Manpage 安装指南

本文档介绍如何将 KUDIG-DATABASE 的 manpage 安装到您的系统中。

## 快速安装

### 一键安装脚本

```bash
# Linux
sudo bash -c '
  cd /path/to/kudig-database
  cp -r man/man1/* /usr/local/share/man/man1/ 2>/dev/null || mkdir -p /usr/local/share/man/man1/
  cp -r man/man8/* /usr/local/share/man/man8/ 2>/dev/null || mkdir -p /usr/local/share/man/man8/
  mandb 2>/dev/null || true
  echo "Manpages installed successfully!"
'

# macOS
sudo bash -c '
  cd /path/to/kudig-database
  mkdir -p /usr/local/share/man/man1 /usr/local/share/man/man8
  cp man/man1/* /usr/local/share/man/man1/
  cp man/man8/* /usr/local/share/man/man8/
  echo "Manpages installed successfully!"
'
```

## 详细安装步骤

### Linux (Debian/Ubuntu)

```bash
# 1. 进入项目目录
cd /path/to/kudig-database

# 2. 复制 manpage 到系统目录
sudo mkdir -p /usr/local/share/man/man1
sudo mkdir -p /usr/local/share/man/man8
sudo cp man/man1/*.1 /usr/local/share/man/man1/
sudo cp man/man8/*.8 /usr/local/share/man/man8/

# 3. 更新 man 数据库
sudo mandb

# 4. 验证安装
man kudig-stats
man kubernetes
```

### Linux (RHEL/CentOS/Fedora)

```bash
# 1. 进入项目目录
cd /path/to/kudig-database

# 2. 复制 manpage 到系统目录
sudo mkdir -p /usr/local/share/man/man1
sudo mkdir -p /usr/local/share/man/man8
sudo cp man/man1/*.1 /usr/local/share/man/man1/
sudo cp man/man8/*.8 /usr/local/share/man/man8/

# 3. 更新 man 数据库
sudo mandb

# 4. 验证安装
man kudig-stats
```

### macOS

```bash
# 1. 进入项目目录
cd /path/to/kudig-database

# 2. 复制 manpage 到系统目录
sudo mkdir -p /usr/local/share/man/man1
sudo mkdir -p /usr/local/share/man/man8
sudo cp man/man1/*.1 /usr/local/share/man/man1/
sudo cp man/man8/*.8 /usr/local/share/man/man8/

# 3. 验证安装
man kudig-stats
man kubernetes
```

### 使用 Homebrew (macOS/Linux)

如果您使用 Homebrew，可以将 manpage 安装到 Homebrew 的 man 目录：

```bash
# 查找 Homebrew 的 man 目录
brew --prefix  # 例如: /opt/homebrew 或 /usr/local

# 复制 manpage
BREW_PREFIX=$(brew --prefix)
mkdir -p $BREW_PREFIX/share/man/man1
mkdir -p $BREW_PREFIX/share/man/man8
cp man/man1/*.1 $BREW_PREFIX/share/man/man1/
cp man/man8/*.8 $BREW_PREFIX/share/man/man8/

# 验证
man kudig-stats
```

## 项目本地使用

如果不想安装到系统，可以在项目本地使用：

### 方法一：使用相对路径

```bash
cd /path/to/kudig-database
man ./man/man1/kudig-stats.1
man ./man/man8/kubernetes.8
```

### 方法二：设置 MANPATH

```bash
# 临时设置（当前终端会话）
export MANPATH="$MANPATH:/path/to/kudig-database/man"
man kudig-stats

# 永久设置（添加到 ~/.bashrc、~/.zshrc 或 ~/.bash_profile）
echo 'export MANPATH="$MANPATH:/path/to/kudig-database/man"' >> ~/.bashrc
source ~/.bashrc
```

### 方法三：创建别名

```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
alias kudig-man='man -M /path/to/kudig-database/man'

# 使用
kudig-man kudig-stats
kudig-man kubernetes
```

## 卸载

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Linux
sudo rm -f /usr/local/share/man/man1/kudig-*.1
sudo rm -f /usr/local/share/man/man8/{kubernetes,prometheus,etcd,containerd,cilium,helm,argocd,istio,velero,[[cert-manager|cert-manager]]}.8
sudo mandb

# macOS
sudo rm -f /usr/local/share/man/man1/kudig-*.1
sudo rm -f /usr/local/share/man/man8/{kubernetes,prometheus,etcd,containerd,cilium,helm,argocd,istio,velero,cert-manager}.8

# 从 ~/.bashrc 或 ~/.zshrc 中移除 MANPATH 设置
```
## 验证安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出所有可用的 KUDIG manpage
man -k kudig 2>/dev/null || echo "man -k 需要 whatis 数据库"

# 查看特定命令
man kudig-stats
man kudig-quality
man kudig-validate
man kudig-fta-viz

# 查看核心产品
man kubernetes
man prometheus
man etcd
man containerd
man cilium
man helm
man argocd
man istio
man velero
man cert-manager
```
## 故障排查

### man 命令找不到页面

```bash
# 检查 man 路径
manpath

# 检查具体页面是否存在
ls -la /usr/local/share/man/man1/kudig-stats.1

# 直接指定路径测试
man /usr/local/share/man/man1/kudig-stats.1
```

### 页面格式显示异常

```bash
# 设置正确的 locale
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8

# 重新查看
man kudig-stats
```

### mandb 命令不存在 (macOS)

macOS 不需要运行 `mandb`，直接复制文件即可使用。

### 权限问题

```bash
# 确保文件权限正确
sudo chmod 644 /usr/local/share/man/man1/*.1
sudo chmod 644 /usr/local/share/man/man8/*.8
```

## 支持的系统

| 系统 | 版本 | 状态 |
|:---|:---|:---|
| Ubuntu | 20.04+ | ✅ 支持 |
| Debian | 10+ | ✅ 支持 |
| RHEL/CentOS | 7+ | ✅ 支持 |
| Fedora | 35+ | ✅ 支持 |
| macOS | 11+ | ✅ 支持 |
| Arch Linux | - | ✅ 支持 |
| Alpine Linux | 3.14+ | ✅ 支持 |

## 相关文档

- [Manpage 索引](README.md)
- [项目主文档](../README.md)
- [项目脚本](../脚本/README.md)


<!-- risk-assessed -->
