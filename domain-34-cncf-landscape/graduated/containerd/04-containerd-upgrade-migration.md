---
title: containerd 升级迁移指南
description: '## 1. 升级概述'
category: cncf-landscape
tags:
- k8s
- cncf
- containerd
- upgrade
- migration
- rollback
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 12min
intent_queries:
- containerd 如何升级
- containerd 升级 1.x 到 2.x
- containerd 回滚 如何操作
- containerd 迁移 步骤
trigger_keywords:
- containerd 升级
- containerd 迁移
- containerd 回滚
- containerd 降级
---

# containerd 升级迁移指南

> **版本**: v1.0 | **适用版本**: containerd 1.6 → 1.7 → 2.0 | **最后更新**: 2026-05

---

## 1. 升级概述

### 1.1 升级路径

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         containerd 升级路径                                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  当前版本                    推荐升级路径              最终目标                   │
│  ──────────────────────────────────────────────────────────────────            │
│  1.6.x    ───▶    1.7.8 (LTS)    ───▶    2.0.0 (最新)                          │
│                                                                                  │
│  1.7.x    ───▶    1.7.8 (LTS)    ───▶    2.0.0 (最新)                          │
│                                                                                  │
│  2.0.0    ───▶    2.0.x (最新)                                           │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 版本支持策略

| 版本类型 | 支持周期 | 说明 |
|----------|----------|------|
| **LTS (1.7.x)** | 12 个月 | 推荐生产环境使用 |
| **Current (2.0.x)** | 6 个月 | 包含最新功能，升级频繁 |
| **EOL 版本** | 不支持 | 请尽快升级 |

### 1.3 升级前检查清单

```bash
# 1. 确认当前版本
containerd --version

# 2. 检查依赖组件版本
crictl version
kubectl version --short
runc --version

# 3. 检查集群健康状态
kubectl get nodes
kubectl get pods -A | grep -v Running | grep -v Completed

# 4. 备份当前配置
cp /etc/containerd/config.toml /etc/containerd/config.toml.backup-$(date +%Y%m%d)

# 5. 检查磁盘空间
df -h /var/lib/containerd
```

---

## 2. 升级前准备

### 2.1 兼容性矩阵

| 组件 | 最低支持 1.7 | 最低支持 2.0 | 推荐版本 |
|------|---------------|--------------|----------|
| **Kubernetes** | 1.20 | 1.24 | 1.27+ |
| **crictl** | 1.26 | 1.27 | 1.28+ |
| **runc** | 1.1.0 | 1.2.0 | 1.2.2+ |
| **CNI plugins** | 0.9.0 | 1.0.0 | 1.2.0+ |
| **nerdctl** | 1.5.0 | 1.7.0 | 2.0+ |

### 2.2 备份策略

#### 2.2.1 配置备份

```bash
# 备份 containerd 配置
cp /etc/containerd/config.toml /etc/containerd/config.toml.backup-$(date +%Y%m%d)

# 备份 CRI 配置
cp /var/lib/kubelet/config.yaml /var/lib/kubelet/config.yaml.backup-$(date +%Y%m%d)

# 备份 kubelet 启动参数
cat /var/lib/kubelet/kubeadm-flags.env > /tmp/kubeadm-flags.backup
```

#### 2.2.2 数据备份

```bash
# 备份 containerd 数据目录（可选，用于重大版本升级）
# 注意：这可能需要大量时间和磁盘空间

# 估算数据大小
du -sh /var/lib/containerd

# 如果需要完整备份
tar -czf /backup/containerd-data-$(date +%Y%m%d).tar.gz /var/lib/containerd/

# 或者只备份元数据（适用于日常升级）
cp -r /var/lib/containerd/meta.json /backup/containerd-meta-$(date +%Y%m%d).json
```

#### 2.2.3 证书备份

```bash
# 备份证书目录
tar -czf /backup/containerd-certs-$(date +%Y%m%d).tar.gz /etc/containerd/certs.d/

# 备份仓库配置
cp -r /etc/containerd/certs.d /backup/certs.d.backup-$(date +%Y%m%d)
```

---

## 3. 原地升级 (In-Place Upgrade)

### 3.1 从 1.6.x 升级到 1.7.x

#### 升级步骤

```bash
# 1. 封锁节点
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force

# 2. 停止 containerd
systemctl stop containerd

# 3. 安装新版本
# Ubuntu/Debian
apt-get update
apt-get install containerd.io=1.7.*

# 或者手动安装
wget https://github.com/containerd/containerd/releases/download/v1.7.8/containerd-1.7.8-linux-amd64.tar.gz
tar xvf containerd-1.7.8-linux-amd64.tar.gz -C /usr/local

# 4. 验证配置兼容性
containerd config migrate > /etc/containerd/config.toml.new
# 检查新配置文件是否有问题

# 5. 启动 containerd
systemctl start containerd

# 6. 验证
containerd --version
crictl info | grep -i version

# 7. 解锁节点
kubectl uncordon <node-name>

# 8. 验证 Pod 状态
kubectl get pods -A | grep <node-name>
```

#### 配置迁移说明

containerd 1.7 兼容 1.6 的配置文件，但某些选项可能需要更新：

```toml
# 需要确认的配置项
[plugins."io.containerd.grpc.v1.cri"]
  # 确认 snapshotter 配置正确
  snapshotter = "overlayfs"
  
  # 确认 cgroup 驱动配置
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
    SystemdCgroup = true
```

### 3.2 从 1.7.x 升级到 2.0.x

#### 升级步骤

```bash
# 1. 封锁节点
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force

# 2. 停止 containerd
systemctl stop containerd

# 3. 安装 containerd 2.0
# Ubuntu/Debian
apt-get install containerd.io=2.0.*

# 或者手动安装
wget https://github.com/containerd/containerd/releases/download/v2.0.0/containerd-2.0.0-linux-amd64.tar.gz
tar xvf containerd-2.0.0-linux-amd64.tar.gz -C /usr/local

# 4. 运行配置迁移
containerd config migrate > /etc/containerd/config.toml

# 5. 检查新配置
grep -E "(version|ttrpc|plugin)" /etc/containerd/config.toml

# 6. 确保 runc 版本兼容
runc --version  # 需要 1.2.0+
# 如果需要，更新 runc
wget https://github.com/opencontainers/runc/releases/download/v1.2.0/runc.amd64
mv runc.amd64 /usr/bin/runc
chmod +x /usr/bin/runc

# 7. 启动 containerd
systemctl start containerd

# 8. 验证
containerd --version
# Containerd Version: 2.0.0

crictl info | grep -i "RuntimeVersion"
# RuntimeVersion: 2.0.0

# 9. 检查插件状态
ctr plugin list

# 10. 解锁节点
kubectl uncordon <node-name>

# 11. 验证 Pod 运行状态
kubectl get pods -A | grep <node-name>
```

#### 2.0 特有配置更新

```toml
# /etc/containerd/config.toml (2.0 版)
version = 2

[grpc]
  address = "/run/containerd/containerd.sock"
  ttrpc_enabled = true  # 新增：启用 ttrpc

[plugins]
  # 2.0 动态插件配置
  [plugins."io.containerd.plugin.v2"]
    # 可以禁用不需要的插件
    # disabled_plugins = ["logging"]
```

---

## 4. 滚动升级策略

### 4.1 滚动升级流程

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         Rolling Upgrade Strategy                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Node-1          Node-2          Node-3          Node-4                         │
│  ──────          ──────          ──────          ──────                         │
│                                                                                  │
│  Step 1: 升级 Node-1                                                             │
│  ┌─────────┐                                                                   │
│  │ Node-1  │ ← 封锁 + 驱逐 + 升级 + 验证 + 解锁           │
│  │ 升级中  │                                                                   │
│  └─────────┘                                                                   │
│       ↓                                                                          │
│  Step 2: 等待 Node-1稳定，然后升级 Node-2                                        │
│  Step 3: 等待 Node-2稳定，然后升级 Node-3                                        │
│  Step 4: 等待 Node-3稳定，然后升级 Node-4                                        │
│                                                                                  │
│  关键：每个节点升级后，等待 5-10 分钟确认稳定再继续                                │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 批量升级脚本

```bash
#!/bin/bash
# rolling-upgrade-containerd.sh

set -e

NEW_VERSION="2.0.0"
DRAIN_TIMEOUT="600s"
CHECK_INTERVAL="30s"

echo "开始滚动升级到 containerd ${NEW_VERSION}"

# 获取所有节点
nodes=$(kubectl get nodes -o jsonpath='{.items[*].metadata.name}')

for node in $nodes; do
    echo "=========================================="
    echo "升级节点: $node"
    echo "=========================================="
    
    # 1. 封锁节点
    echo "封锁节点: $node"
    kubectl cordon "$node"
    
    # 2. 驱逐 Pod
    echo "驱逐 Pod: $node"
    kubectl drain "$node" \
        --ignore-daemonsets \
        --delete-emptydir-data \
        --force \
        --timeout="$DRAIN_TIMEOUT"
    
    # 3. 执行升级
    echo "升级 containerd: $node"
    ssh "$node" "systemctl stop containerd"
    ssh "$node" "wget -q https://github.com/containerd/containerd/releases/download/v${NEW_VERSION}/containerd-${NEW_VERSION}-linux-amd64.tar.gz -O /tmp/containerd.tar.gz"
    ssh "$node" "tar xf /tmp/containerd.tar.gz -C /usr/local"
    ssh "$node" "systemctl start containerd"
    
    # 4. 验证
    echo "验证: $node"
    sleep 10
    crictl info --node "$node" | grep -q "RuntimeVersion" && echo "✓ 升级成功"
    
    # 5. 解锁
    echo "解锁节点: $node"
    kubectl uncordon "$node"
    
    # 6. 等待稳定
    echo "等待节点稳定..."
    sleep 60
done

echo "滚动升级完成"
```

---

## 5. 回滚策略

### 5.1 回滚触发条件

| 条件 | 严重程度 | 是否回滚 |
|------|----------|----------|
| containerd 无法启动 | P0 | 是 |
| 所有容器创建失败 | P0 | 是 |
| 镜像拉取失败 (>5%) | P1 | 是 |
| API 响应延迟 > 500ms | P2 | 评估 |
| 内存占用增加 > 50% | P2 | 评估 |

### 5.2 回滚步骤

#### 5.2.1 单节点回滚

```bash
# 1. 封锁节点
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 2. 停止 containerd
systemctl stop containerd

# 3. 恢复配置
cp /etc/containerd/config.toml.backup-YYYYMMDD /etc/containerd/config.toml

# 4. 降级 containerd
# Ubuntu/Debian
apt-get install containerd.io=1.7.*

# 或手动降级
wget https://github.com/containerd/containerd/releases/download/v1.7.8/containerd-1.7.8-linux-amd64.tar.gz
tar xvf containerd-1.7.8-linux-amd64.tar.gz -C /usr/local

# 5. 启动
systemctl start containerd

# 6. 验证
containerd --version
crictl info

# 7. 解锁
kubectl uncordon <node-name>
```

#### 5.2.2 数据恢复回滚

```bash
# 如果需要恢复数据目录
systemctl stop containerd

# 恢复数据
rm -rf /var/lib/containerd/*
tar -xzf /backup/containerd-data-YYYYMMDD.tar.gz -C /

# 启动
systemctl start containerd

# 验证
crictl images
kubectl get pods -A
```

### 5.3 回滚后检查

```bash
# 1. 检查版本
containerd --version
runc --version

# 2. 检查服务状态
systemctl status containerd
journalctl -u containerd --since "5m" | grep -i error

# 3. 检查容器
crictl ps -a
crictl pods

# 4. 检查镜像
crictl images

# 5. 检查 Pod 状态
kubectl get pods -A | grep <node-name>
kubectl describe node <node-name>
```

---

## 6. 升级后验证

### 6.1 功能验证

```bash
# 1. 版本验证
containerd --version
crictl version

# 2. API 可用性
crictl info

# 3. 容器创建测试
cat << EOF | crictl run - pod.yaml container.yaml
{
  "metadata": {"name": "upgrade-test"},
  "image": {"image": "busybox:latest"},
  "command": ["echo", "upgrade test"],
  "linux": {}
}
EOF

# 4. 容器删除测试
crictl rm upgrade-test

# 5. 镜像拉取测试
crictl pull busybox:latest

# 6. Pod 创建测试
kubectl run test-pod --image=busybox --restart=Never -- sleep 10
kubectl delete pod test-pod
```

### 6.2 性能基准

```bash
# 记录性能指标（与升级前对比）
# 容器启动延迟
time crictl run --timeout 60s pod.yaml container.yaml

# 内存占用
ps aux | grep containerd | grep -v grep

# gRPC 连接数
ss -tlnp | grep containerd

# 指标采集
curl -s http://127.0.0.1:1338/v1/metrics | grep containerd
```

### 6.3 集群健康检查

```bash
# 1. 检查所有节点 Ready
kubectl get nodes

# 2. 检查 Pod 状态
kubectl get pods -A | grep -v Running | grep -v Completed

# 3. 检查 containerd 日志
journalctl -u containerd --since "1h" | grep -iE "error|warn" | tail -20

# 4. 检查事件
kubectl get events -A | grep -i containerd | tail -10
```

---

## 7. 常见问题处理

### 7.1 升级失败问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| **服务启动失败** | 配置文件语法错误 | 恢复备份配置，检查语法 |
| **Socket 不存在** | 权限问题或路径错误 | 检查 /run/containerd 目录权限 |
| **插件加载失败** | 版本不兼容 | 检查 runc 版本，更新或降级 |
| **镜像拉取失败** | registry 配置丢失 | 恢复 certs.d 目录 |

### 7.2 升级后问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| **ttrpc 连接失败** | 网络配置问题 | 检查 socket 权限，禁用 ttrpc 尝试 |
| **容器创建慢** | snapshotter 问题 | 检查 overlayfs 挂载状态 |
| **内存占用高** | 内存泄漏 | 检查 shim 进程，重启服务 |

---

## 8. 自动化升级

### 8.1 Ansible Playbook 示例

```yaml
# containerd-upgrade.yml
---
- hosts: k8s-nodes
  become: yes
  vars:
    containerd_version: "2.0.0"
    containerd_package: "containerd.io"
    
  tasks:
    - name: Backup current config
      shell: |
        cp /etc/containerd/config.toml /etc/containerd/config.toml.backup-{{ ansible_date_time.epoch }}
        
    - name: Drain node
      shell: |
        kubectl drain {{ inventory_hostname }} --ignore-daemonsets --delete-emptydir-data --force
        
    - name: Stop containerd
      systemd:
        name: containerd
        state: stopped
        
    - name: Upgrade containerd
      apt:
        name: "{{ containerd_package }}={{ containerd_version }}"
        state: present
        
    - name: Migrate config
      shell: containerd config migrate > /etc/containerd/config.toml
      
    - name: Start containerd
      systemd:
        name: containerd
        state: started
        
    - name: Verify upgrade
      shell: |
        containerd --version
        crictl info | grep -i version
        
    - name: Uncordon node
      shell: kubectl uncordon {{ inventory_hostname }}
```

---

## 9. 长期维护

### 9.1 版本更新计划

| 阶段 | 时间 | 任务 |
|------|------|------|
| **评估** | 版本发布后 2 周 | 评估新版本特性 |
| **测试** | 版本发布后 4 周 | 在测试环境验证 |
| **试点** | 版本发布后 8 周 | 选择性节点升级 |
| **推广** | 版本发布后 12 周 | 全量滚动升级 |

### 9.2 监控告警

```yaml
# Prometheus 告警规则
- alert: ContainerdVersionOutdated
  expr: containerd_build_info{version="1.6"} or containerd_build_info{version="1.7"}
  for: 1h
  labels:
    severity: warning
  annotations:
    summary: "containerd 版本过旧"
    description: "节点 {{ $labels.instance }} 使用过旧的 containerd 版本 {{ $labels.version }}"

- alert: ContainerdPluginLoadFailure
  expr: rate(containerd_plugin_errors_total[5m]) > 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "containerd 插件加载失败"
    description: "节点 {{ $labels.instance }} 插件加载失败"
```

---

**维护者**: Kudig Team | **许可证**: MIT