# 节点生命周期总览

## 源码路径

`pkg/kubelet/`
`pkg/kubelet/nodestatus/`
`pkg/kubelet/kubelet.go`

---

## 节点生命周期

```
节点生命周期:
  ┌─────────────────────────────────────────────────────────────┐
  │                     节点生命周期                             │
  ├─────────────────────────────────────────────────────────────┤
  │  1. 创建节点        物理/虚拟机部署                         │
  │  2. 安装运行时      containerd/Docker/cri-o                │
  │  3. 安装 kubelet    kubelet 二进制 + systemd                │
  │  4. kubeadm join    节点加入集群                            │
  │  5. kubelet 注册    CSR → Node 对象创建                      │
  │  6. 正常运行        调度 Pod、管理容器                        │
  │  7. 节点维护        drain → cordon → 升级 → uncordon        │
  │  8. 节点移除        drain → delete node → kubeadm reset     │
  └─────────────────────────────────────────────────────────────┘
```

---

## 节点组件

### kubelet

```bash
# kubelet 职责:
# 1. 节点注册到 API Server
# 2. 管理节点上所有 Pod (静态 Pod + API Server 调度)
# 3. 容器健康检查
# 4. 容器资源监控上报
# 5. 证书自动续期
# 6. 垃圾回收
```

### kube-proxy

```bash
# kube-proxy 职责:
# 1. 维护节点网络规则 (iptables/ipvs)
# 2. Service 负载均衡
# 3. 连接跟踪 (conntrack)
```

### CNI plugin

```bash
# CNI 职责:
# 1. Pod 网络命名空间创建
# 2. veth pair 配置
# 3. 网桥/路由配置
# 4. IP 分配 (通过 CNI IPAM)
```

---

## Node 对象

```bash
kubectl get nodes

# 查看节点详情
kubectl describe node <node-name>

# 查看节点资源
kubectl top node <node-name>
```

---

## 节点状态流转

```
                  ┌──────────────┐
                  │  NotRegistered │
                  └──────┬───────┘
                         │ kubelet 启动
                         ▼
                  ┌──────────────┐
                  │   Registered  │
                  └──────┬───────┘
                         │ kubelet 向 API Server 注册
                         ▼
                  ┌──────────────┐
           ┌─────│    Ready     │─────┐
           │     └──────────────┘     │
           │ 节点正常                   │ 节点异常
           ▼                           ▼
    ┌──────────────┐          ┌──────────────┐
    │   Ready      │          │  NotReady    │
    └──────────────┘          └──────────────┘
           │                           │
           │ kubelet 正常               │ kubelet 恢复
           ▼                           ▼
    ┌──────────────┐          ┌──────────────┐
    │   Ready     │          │    Ready     │
    └──────────────┘          └──────────────┘
```

---

## 节点角色

```bash
# control-plane 节点 (master)
kubectl get nodes -l node-role.kubernetes.io/control-plane

# worker 节点
kubectl get nodes -l !node-role.kubernetes.io/control-plane

# 或使用旧标签
kubectl get nodes -l node-role.kubernetes.io/master

# 打标签
kubectl label node <node-name> node-role.kubernetes.io/worker=worker

# 打污点
kubectl taint node <node-name> node-role.kubernetes.io/worker=worker:NoSchedule
```

---

## 节点条件 (Node Conditions)

```bash
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .status.conditions[*]}{.type}={.status}{" "}{end}{"\n"}{end}'

# 节点条件:
# Type                  Status  说明
# OutOfDisk            False    磁盘空间是否不足
# MemoryPressures      False    内存是否不足
# DiskPressure         False    磁盘是否不足
# PIDPressure          False    进程数是否过多
# NetworkUnavailable   False    网络是否不可用
# Ready                True     节点是否就绪
```

---

## 节点资源容量

```bash
# 查看节点资源
kubectl get node <node> -o jsonpath='{.status.capacity}'
kubectl get node <node> -o jsonpath='{.status.allocatable}'

# capacity: 节点总资源
# allocatable: 可分配给 Pod 的资源 (capacity - kubelet/system reserved)
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 节点 NotReady | kubelet 未启动/网络问题 | `systemctl status kubelet` |
| 节点 NetworkUnavailable | CNI 配置错误 | 检查 CNI 插件状态 |
| 节点磁盘不足 | 镜像/日志占用 | 清理磁盘 |
