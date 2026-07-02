---
title: Docker 到 containerd 迁移
description: 'dockershim 移除背景、Docker 配置映射、节点逐个迁移流程、回滚方案与验证清单完整指南'
summary: 'dockershim 移除背景、Docker 配置映射、节点逐个迁移流程、回滚方案与验证清单完整指南'
category: container-runtime
tags:
- docker
- containerd
- migration
- dockershim
- runtime-migration
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Docker 到 containerd 迁移 是什么
- 如何从 Docker 迁移到 containerd
- dockershim 移除怎么处理
trigger_keywords:
- docker
- containerd
- migration
- dockershim
- runtime
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# Docker 到 containerd 迁移

## 1. 背景

### 1.1 dockershim 移除时间线

| 版本 | 状态 |
|------|------|
| Kubernetes 1.20 | 宣布弃用 dockershim |
| Kubernetes 1.24 | dockershim 默认禁用 |
| Kubernetes 1.25 | dockershim 代码移除 |
| Kubernetes 1.26+ | 完全不支持 Docker 作为容器运行时 |

### 1.2 为什么要迁移

```
旧架构（dockershim）:
kubelet → dockershim → Docker daemon → containerd → runc

新架构（直接 containerd）:
kubelet → containerd → runc

优势:
- 减少一层抽象，降低延迟
- 消除 Docker daemon 依赖
- 更少的资源消耗
- 更好的 CRI 标准支持
- 持续的安全更新
```

### 1.3 迁移影响评估

```bash
# 1. 检查当前 Docker 版本
docker version
docker info

# 2. 检查 Kubernetes 版本
kubectl version --short
kubectl get nodes -o wide

# 3. 检查依赖 Docker 的组件
# - 使用 docker.sock 的 Pod
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.volumes[*].hostPath.path}{"\n"}{end}' | grep docker

# - 使用 Docker 命令的 Init Container
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.initContainers[]?.command[]? | contains("docker")) | .metadata.name'

# - 使用 Docker Socket 的 DaemonSet
kubectl get ds -A -o json | jq -r '.items[] | select(.spec.template.spec.volumes[]?.hostPath.path == "/var/run/docker.sock") | .metadata.name'
```

## 2. 配置映射

### 2.1 Docker daemon.json → containerd config.toml

```json
// Docker daemon.json
{
  "storage-driver": "overlay2",
  "storage-opts": ["overlay2.override_kernel_check=true"],
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "100m",
    "max-file": "3"
  },
  "registry-mirrors": ["https://mirror.example.com"],
  "insecure-registries": ["registry.internal:5000"],
  "exec-opts": ["native.cgroupdriver=systemd"],
  "default-runtime": "runc",
  "bip": "172.17.0.1/16",
  "mtu": 1500
}
```

```toml
# containerd config.toml 等效配置
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  # 存储驱动（默认 overlay2）
  snapshotter = "overlayfs"

[plugins."io.containerd.grpc.v1.cri"]
  # 日志配置
  [plugins."io.containerd.grpc.v1.cri".containerd]
    # 日志路径
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"

[plugins."io.containerd.grpc.v1.cri".cni]
  # CNI 配置目录
  bin_dir = "/opt/cni/bin"
  conf_dir = "/etc/cni/net.d"

[plugins."io.containerd.grpc.v1.cri".registry]
  # 注册表镜像
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
    endpoint = ["https://mirror.example.com"]
  # 不安全注册表
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.internal:5000"]
    endpoint = ["http://registry.internal:5000"]
  [plugins."io.containerd.grpc.v1.cri".registry.configs."registry.internal:5000".tls]
    insecure_skip_verify = true
```

### 2.2 Docker 命令 → crictl 命令映射

| Docker 命令 | crictl 命令 | 说明 |
|------------|------------|------|
| `docker ps` | `crictl ps` | 列出容器 |
| `docker ps -a` | `crictl ps -a` | 列出所有容器 |
| `docker logs <id>` | `crictl logs <id>` | 查看日志 |
| `docker exec -it <id> bash` | `crictl exec -it <id> bash` | 进入容器 |
| `docker images` | `crictl images` | 列出镜像 |
| `docker pull <image>` | `crictl pull <image>` | 拉取镜像 |
| `docker inspect <id>` | `crictl inspect <id>` | 检查容器 |
| `docker stats` | `crictl stats` | 资源使用 |
| `docker rm <id>` | `crictl rm <id>` | 删除容器 |
| `docker rmi <image>` | `crictl rmi <image>` | 删除镜像 |

```bash
# crictl 配置
cat > /etc/crictl.yaml << 'EOF'
runtime-endpoint: unix:///run/containerd/containerd.sock
image-endpoint: unix:///run/containerd/containerd.sock
timeout: 10
debug: false
EOF
```

## 3. 节点逐个迁移流程

### 3.1 迁移前准备

```bash
# 1. 备份当前配置
sudo cp /etc/docker/daemon.json /etc/docker/daemon.json.backup
sudo cp /etc/containerd/config.toml /etc/containerd/config.toml.backup 2>/dev/null

# 2. 导出 Docker 镜像列表
docker images --format '{{.Repository}}:{{.Tag}}' | sort > docker-images.txt

# 3. 确保 containerd 已安装
containerd --version
# 如果未安装
sudo apt-get install -y containerd.io

# 4. 生成默认配置
sudo mkdir -p /etc/containerd
containerd config default > /etc/containerd/config.toml

# 5. 配置 systemd cgroup 驱动
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
```

### 3.2 迁移步骤（逐节点）

```bash
# ===== 节点迁移流程 =====

# Step 1: 标记节点不可调度
kubectl cordon node-1

# Step 2: 驱逐工作负载
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --force --grace-period=30

# Step 3: 停止 kubelet
sudo systemctl stop kubelet

# Step 4: 停止 Docker（可选，保留用于回滚）
sudo systemctl stop docker
sudo systemctl stop docker.socket

# Step 5: 启动 containerd
sudo systemctl enable containerd
sudo systemctl start containerd

# Step 6: 配置 kubelet 使用 containerd
# 编辑 /etc/default/kubelet 或 /etc/sysconfig/kubelet
KUBELET_EXTRA_ARGS="--container-runtime-endpoint=unix:///run/containerd/containerd.sock"
# 或
sudo tee /etc/default/kubelet << 'EOF'
KUBELET_EXTRA_ARGS="--container-runtime-endpoint=unix:///run/containerd/containerd.sock"
EOF

# Step 7: 启动 kubelet
sudo systemctl daemon-reload
sudo systemctl start kubelet

# Step 8: 验证节点状态
kubectl get node node-1 -o wide
# 确认 CONTAINER-RUNTIME 显示 containerd

# Step 9: 验证 Pod 运行
kubectl get pods --field-selector spec.nodeName=node-1 -A

# Step 10: 恢复调度
kubectl uncordon node-1

# Step 11: 验证新 Pod 调度
kubectl run test-nginx --image=nginx --restart=Never
kubectl get pod test-nginx -o wide
```

### 3.3 自动化迁移脚本

```bash
#!/bin/bash
# migrate-to-containerd.sh
# 用法: ./migrate-to-containerd.sh <node-name>

set -euo pipefail

NODE=$1
echo "开始迁移节点: $NODE"

# Step 1: 节点排水
echo "标记节点不可调度并驱逐 Pod..."
kubectl cordon $NODE
kubectl drain $NODE --ignore-daemonsets --delete-emptydir-data --force --grace-period=30

# Step 2: 停止服务
echo "停止 Docker 和 kubelet..."
sudo systemctl stop kubelet
sudo systemctl stop docker docker.socket

# Step 3: 配置 containerd
echo "配置 containerd..."
sudo mkdir -p /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml > /dev/null
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml

# Step 4: 启动 containerd
echo "启动 containerd..."
sudo systemctl enable containerd
sudo systemctl start containerd

# Step 5: 配置 kubelet
echo "配置 kubelet 使用 containerd..."
sudo tee /etc/default/kubelet << 'EOF'
KUBELET_EXTRA_ARGS="--container-runtime-endpoint=unix:///run/containerd/containerd.sock"
EOF

# Step 6: 启动 kubelet
echo "启动 kubelet..."
sudo systemctl daemon-reload
sudo systemctl start kubelet

# Step 7: 验证
echo "验证节点状态..."
sleep 10
kubectl get node $NODE -o wide
kubectl uncordon $NODE

echo "迁移完成！"
```

## 4. 回滚方案

### 4.1 回滚步骤

```bash
# 如果迁移失败，回滚到 Docker

# Step 1: 标记节点不可调度
kubectl cordon node-1
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --force

# Step 2: 停止 kubelet 和 containerd
sudo systemctl stop kubelet
sudo systemctl stop containerd

# Step 3: 恢复 Docker
sudo systemctl start docker

# Step 4: 恢复 kubelet 配置
sudo rm /etc/default/kubelet
# 或恢复备份
sudo cp /etc/docker/daemon.json.backup /etc/docker/daemon.json

# Step 5: 启动 kubelet
sudo systemctl start kubelet

# Step 6: 验证
kubectl get node node-1 -o wide
# 确认 CONTAINER-RUNTIME 显示 docker

kubectl uncordon node-1
```

### 4.2 容器恢复

```bash
# 检查容器状态
crictl ps -a

# 如果容器丢失，重新创建
kubectl get pods --field-selector spec.nodeName=node-1 -A
# 对于 StatefulSet，Kubernetes 会自动恢复

# 验证所有 Pod 正常
kubectl get pods -A --field-selector spec.nodeName=node-1
```

## 5. 验证清单

### 5.1 迁移后验证

```bash
# 1. 节点状态
kubectl get node node-1 -o wide
# 期望: STATUS=Ready, CONTAINER-RUNTIME=containerd://...

# 2. 系统 Pod
kubectl get pods -n kube-system --field-selector spec.nodeName=node-1
# 期望: 所有 Pod Running

# 3. 应用 Pod
kubectl get pods -A --field-selector spec.nodeName=node-1
# 期望: 所有 Pod Running

# 4. containerd 状态
sudo systemctl status containerd
crictl ps | head -5

# 5. 日志检查
journalctl -u containerd --since "1 hour ago" | grep -i error
journalctl -u kubelet --since "1 hour ago" | grep -i error

# 6. 资源使用
crictl stats

# 7. 网络连通性
kubectl run nettest --image=busybox --restart=Never -- ping -c 3 8.8.8.8
kubectl logs nettest
kubectl delete pod nettest
```

### 5.2 集群级验证

```bash
# 1. 所有节点运行时一致性
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'

# 2. 集群组件健康
kubectl get cs
kubectl cluster-info

# 3. 核心服务
kubectl get pods -n kube-system | grep -E "apiserver|controller|scheduler|etcd|proxy|coredns"

# 4. 存储和网络插件
kubectl get pods -n kube-system | grep -E "flannel|calico|weave|cilium|rook|longhorn"
```

## 6. 迁移后清理

```bash
# 1. 清理 Docker 数据（可选，释放磁盘空间）
sudo apt-get remove -y docker-ce docker-ce-cli containerd.io
sudo rm -rf /var/lib/docker
sudo rm -rf /var/run/docker.sock

# 2. 清理 Docker 配置
sudo rm /etc/docker/daemon.json.backup

# 3. 更新 crictl 配置
sudo tee /etc/crictl.yaml << 'EOF'
runtime-endpoint: unix:///run/containerd/containerd.sock
image-endpoint: unix:///run/containerd/containerd.sock
timeout: 10
debug: false
EOF

# 4. 验证清理
which docker
# 应该返回 "docker not found"
```

## 7. 生产最佳实践

| 实践 | 建议 |
|------|------|
| 迁移顺序 | 先测试环境，再非生产，最后生产 |
| 节点批次 | 每次 1-2 个节点，验证后再继续 |
| 监控 | 迁移后监控 24 小时 |
| 回滚准备 | 保留 Docker 和配置备份 7 天 |
| 文档 | 记录每个节点的迁移状态 |
| 窗口 | 选择低峰期进行迁移 |

## Related

- [[domain-13-container-runtime/03-containerd-cri-o/01-containerd-production-operations|containerd 生产运维]]
- [[domain-13-container-runtime/05-runtime-migration/02-containerd-to-cri-o-migration|containerd 到 CRI-O 迁移]]

## See Also

- [Kubernetes dockershim 迁移指南](https://kubernetes.io/docs/tasks/administer-cluster/migrating-from-dockershim/)
- [containerd 文档](https://containerd.io/docs/)
