# 节点故障排查

## 源码路径

`pkg/kubelet/`
`pkg/kubelet/util/`

---

## 节点 NotReady 排查

```bash
# 1. 检查 kubelet 状态
systemctl status kubelet

# 2. 查看 kubelet 日志
journalctl -u kubelet -f --no-pager

# 3. 检查节点条件
kubectl get node <node> -o jsonpath='{.status.conditions}'

# 4. 检查 API Server 连接
curl -k https://<api-server>:6443/healthz

# 5. 检查 etcd 连接
curl -k https://127.0.0.1:2379/health
```

---

## kubelet 启动失败

```bash
# 查看启动错误
journalctl -u kubelet -p err --no-pager -n 50

# 常见错误:
# 1. 配置文件错误
kubelet --config=/var/lib/kubelet/config.yaml --validate

# 2. cgroup driver 不匹配
# containerd 使用 systemd，但 kubelet 配置为 cgroupfs
# 解决: kubelet config.yaml 中设置 cgroupDriver: systemd

# 3. 证书问题
# 检查 /var/lib/kubelet/pki/
ls -la /var/lib/kubelet/pki/
```

---

## 容器启动失败

```bash
# 查看容器状态
crictl ps -a

# 查看容器日志
crictl logs <container-id>

# 查看 pod
crictl pods

# 手动重启容器
crictl stop <container-id> && crictl rm <container-id>

# kubelet 会自动重建
```

---

## 网络问题

```bash
# 检查 CNI 状态
cat /etc/cni/net.d/

# 检查网桥
ip link show type bridge

# 检查路由
ip route

# 检查 veth pair
ip link | grep veth

# 测试 DNS
kubectl run -it --rm debug --image=busybox -- nslookup kubernetes.default
```

---

## 磁盘问题

```bash
# 查看磁盘使用
df -h

# 检查 inode
df -i

# 查看大文件
du -sh /var/log/*
du -sh /var/lib/containerd/*

# 清理
docker system prune -a
containerd system prune
```

---

## 内存不足 (OOM)

```bash
# 查看 OOM 事件
dmesg | grep -i "out of memory"
dmesg | grep -i kill

# 查看 kubelet 内存使用
top -p $(pidof kubelet)

# 调整 kubelet 资源限制
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
MemoryMax=512M
```

---

## 常见故障排查命令

```bash
# 节点概览
kubectl get node <node>
kubectl describe node <node>

# kubelet 健康
curl -k https://localhost:10250/healthz

# kubelet stats
curl -k https://localhost:10250/stats/summary

# 容器运行时
crictl info
crictl ps

# 系统资源
df -h; free -h; top
```

---

## 常见问题

| 问题 | 排查命令 | 解决 |
|------|---------|------|
| kubelet NotReady | `journalctl -u kubelet` | 根据日志修复 |
| 容器无法启动 | `crictl logs <id>` | 检查镜像/配置 |
| 网络不通 | `ip link/route` | 检查 CNI |
| 磁盘满 | `df -h` | 清理 |
| OOM | `dmesg \| grep kill` | 减少资源/扩容 |
