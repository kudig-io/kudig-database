# kubelet 进阶配置

## 源码路径

`pkg/kubelet/apis/config/`
`pkg/kubelet/kubelet.go`

---

## kubelet 配置文件

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
address: 0.0.0.0
port: 10250
readOnlyPort: 10255
cgroupDriver: systemd
cgroupVersion: 2
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
serverTLSBootstrap: true
rotateCertificates: true
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: 2h0m0s
  bootstrap:
    enabled: true
authorization:
  mode: Webhook
runtimeRequestTimeout: 2m0s
evictionHard:
  memory.available: 100Mi
  nodefs.available: 10%
  imagefs.available: 15%
  nodefs.inodesFree: 5%
evictionSoft:
  memory.available: 200Mi
evictionSoftGracePeriod:
  memory.available: 1m30s
evictionPressureTransitionPeriod: 5m
maxPods: 110
podPidsLimit: 4096
```

---

## cgroup driver

```bash
# systemd vs cgroupfs
# 推荐使用 systemd

# 检查当前 cgroup driver
docker info | grep "Cgroup Driver"
# 或
cat /sys/fs/cgroup/cgroup.controllers

# kubelet 必须与 container runtime 使用相同的 cgroup driver
# 不匹配会导致资源限制失效
```

---

## kubelet 日志配置

```bash
# kubelet 日志位置
# /var/log/journal/
# 或 journalctl -u kubelet

# 配置日志级别
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
Environment="KUBELET_LOG_LEVEL=--v=2"

# 日志级别:
# --v=0: 最低
# --v=2: 默认
# --v=4: 调试
# --v=9: 最详细
```

---

## kubelet 资源预留

```bash
# kubelet 启动参数 (systemd drop-in)
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
Environment="KUBELET_SYSTEM_PODS_MEMORY_LIMIT=--enforce-node-allocatable=memory"

# 推荐预留:
# --kube-reserved=cpu=500m,memory=1Gi
# --system-reserved=cpu=500m,memory=1Gi
# --eviction-hard=memory.available<500Mi
```

---

## maxPods

```bash
# 默认 110 个 Pod
# 可调整
--max-pods=250

# 查看当前限制
kubectl get node <node> -o jsonpath='{.status.capacity.pods}'
```

---

## podPidsLimit

```bash
# 每个 Pod 最大 PID 数
--pod-pids-limit=4096

# 防止 PID 耗尽
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| cgroup driver 不匹配 | kubelet 与 containerd driver 不同 | 统一配置 |
| Pod 数量不足 | maxPods 限制 | 增加 --max-pods |
| 日志过大 | 日志级别太高 | 调整 KUBELET_LOG_LEVEL |
