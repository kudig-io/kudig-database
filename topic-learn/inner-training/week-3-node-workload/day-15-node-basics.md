# Day 15: Node 节点基础

> **学习时间**: 4-5 小时 | **主题**: 节点概念、状态与管理操作

---

## 今日目标

- [ ] 理解 Node 在 K8S 中的角色与组件
- [ ] 掌握节点状态 (Conditions) 的含义
- [ ] 能够查看节点详细信息和资源使用
- [ ] 了解节点上运行的核心进程 (kubelet, kube-proxy)

---

## 理论学习 (2h)

### 必读文档

1. **K8S 架构与组件**
   - 文件: `../../../domain-1-architecture-fundamentals/02-core-components-deep-dive.md`
   - 重点: kubelet、kube-proxy、Container Runtime

2. **节点排障**
   - 文件: `../../../domain-12-troubleshooting/06-node-notready-diagnosis.md`
   - 重点: Node NotReady 常见原因

---

## 实践任务 (2.5h)

### 任务 1: 节点信息查看 (45min)

```bash
kubectl get nodes -o wide
kubectl describe node <node-name>
kubectl top nodes
kubectl get nodes -o custom-columns='NAME:.metadata.name,STATUS:.status.conditions[?(@.type=="Ready")].status,CPU:.status.capacity.cpu,MEM:.status.capacity.memory'
```

### 任务 2: 节点 Conditions 解读 (45min)

```bash
# 查看所有 Condition
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{range .status.conditions[*]}  {.type}={.status}{"\n"}{end}{"\n"}{end}'

# 关键 Conditions:
# Ready=True: 节点正常
# MemoryPressure=True: 内存压力
# DiskPressure=True: 磁盘压力
# PIDPressure=True: 进程数压力
# NetworkUnavailable=True: 网络不可用
```

### 任务 3: 节点资源分析 (30min)

```bash
# 查看节点资源分配情况
kubectl describe node <node-name> | grep -A 10 "Allocated resources"

# 查看节点上的 Pod
kubectl get pods -A --field-selector spec.nodeName=<node-name> -o wide
```

### 任务 4: 节点上的核心进程 (30min)

```bash
# 通过 SSH 登录节点后检查 (或通过 debug Pod)
kubectl debug node/<node-name> -it --image=busybox

# 检查 kubelet 状态
# systemctl status kubelet
# journalctl -u kubelet --no-pager -n 50

# 检查 kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=20
```

---

## 费曼复述 (0.5h)

1. **kubelet 在节点上做什么？它如何与 API Server 通信？**
2. **Node Ready 和 NotReady 的判断依据是什么？**
3. **节点出现 MemoryPressure 时，K8S 会如何处理？**

---

## 今日检验

- [ ] 能查看节点状态和资源使用情况
- [ ] 理解 Node Conditions 各字段的含义
- [ ] 能查看节点上运行的 Pod 和资源分配
- [ ] 了解 kubelet 和 kube-proxy 的作用

---

## 核心概念总结

| 组件 | 运行位置 | 作用 |
|------|---------|------|
| kubelet | 每个节点 | 管理 Pod 生命周期、汇报节点状态 |
| kube-proxy | 每个节点 | Service 网络转发 (iptables/IPVS) |
| Container Runtime | 每个节点 | 容器运行时 (containerd) |

---

## 明日预告

Day 16 将学习节点进阶管理: 标签、污点、排水等运维操作。
