# 节点升级

## 源码路径

`cmd/kubeadm/app/cmd/upgrade/`
`pkg/kubelet/`

---

## 节点升级概述

```
集群升级类型:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. Control Plane 升级    先升级 master 节点                │
  │  2. Worker 节点升级        后升级 worker 节点                │
  │  3. kubelet/kubectl 升级  单独升级 kubelet 二进制           │
  └─────────────────────────────────────────────────────────────┘
```

---

## kubeadm upgrade node

```bash
# 在 worker 节点上执行 (不传 --certificate-key)
kubeadm upgrade node

# 内部流程:
# 1. 备份 /var/lib/kubelet/config.yaml
# 2. 备份 /etc/kubernetes/kubelet.conf
# 3. 升级 kubelet/kubeadm 包
# 4. 重启 kubelet 服务
```

---

## kubelet 二进制升级

### apt (Debian/Ubuntu)

```bash
# 查看可升级版本
apt-cache policy kubelet

# 升级到指定版本
sudo apt-get install kubelet=1.29.0-*

# 查看当前版本
kubelet --version
```

### yum/dnf (RHEL/CentOS)

```bash
sudo yum install kubelet-1.29.0 --disableexcludes=kubernetes
```

---

## 节点 OS 升级

```bash
# 1. drain 节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 2. 升级 OS 包
sudo apt-get update && sudo apt-get upgrade -y

# 3. 如果需要升级内核
sudo apt-get install linux-image-5.15.0-xxx-generic
sudo reboot

# 4. uncordon
kubectl uncordon <node>
```

---

## 滚动升级策略

```
升级顺序:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. 升级所有 control-plane 节点 (任意顺序)                   │
  │  2. 升级所有 worker 节点 (可并行)                           │
  │  3. 升级 CNI 插件                                           │
  │  4. 升级其他集群组件                                         │
  └─────────────────────────────────────────────────────────────┘
```

---

## 节点升级检查

```bash
# 升级后验证
kubectl get nodes

# 检查 kubelet 版本
kubectl top node <node>

# 查看 kubelet 日志
journalctl -u kubelet --no-pager -n 50

# 检查节点状态
kubectl describe node <node> | grep -A 5 "Conditions"
```

---

## 降级 (不推荐)

```bash
# 降级 kubelet
sudo apt-get install kubelet=1.28.0-*

# 重启
sudo systemctl restart kubelet

# 注意: 降级可能导致 API 兼容性问题
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| kubelet 启动失败 | cgroup driver 不匹配 | 检查 /var/lib/kubelet/config.yaml |
| 证书续期失败 | API Server 连接失败 | 检查 API Server 是否就绪 |
| 镜像拉取失败 | 权限问题 | 检查 imagePullSecrets |
