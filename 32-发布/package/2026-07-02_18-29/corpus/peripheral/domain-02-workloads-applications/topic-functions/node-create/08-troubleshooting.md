---
title: 节点故障排查手册 (topic-code-analysis)
description: 'title: 节点故障排查手册'
summary: 'title: 节点故障排查手册'
category: general
tags:
- reference
- troubleshooting
- etcd
- kubelet
- flannel
- calico
- coredns
- containerd
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点故障排查手册 是什么
- 如何 节点故障排查手册
- Kubernetes 07 platform engineering 最佳实践
- 节点故障排查手册 故障排查
- 节点故障排查手册 排障步骤
trigger_keywords:
- 节点故障排查手册
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 节点故障排查手册
description: '# 节点故障排查手册'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- kubelet
- flannel
- calico
- coredns
- containerd
- daemonset
last_updated: 2026-05-18
difficulty: intermediate
reading_level: intermediate
audience:
- Kubernetes 运维工程师
- 开发工程师
- SRE 工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes node NotReady troubleshooting
- kubelet startup failure diagnosis
- node disk pressure memory pressure troubleshooting
- container runtime failure排查
- 节点故障诊断完整流程
trigger_keywords:
- NotReady
- kubelet
- troubleshooting
- node failure
- disk pressure
- memory pressure
- OOM
- crictl
- journalctl
- kubectl describe node
- node not ready
- kubelet not running
- certificate expired
related_domains:
- domain-01-cluster-fundamentals
- domain-[[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-02-workloads-applications/topic-functions/cluster-delete/04-troubleshooting|12-troubleshooting]]
- domain-03-networking-traffic
related_topics:
- node-create/03-condition
- node-create/11-eviction
- node-create/12-monitoring
- cluster-create/03-certs
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 节点故障排查手册

## 概述

节点故障排查是 Kubernetes 运维中最常见且最关键的任务之一。节点问题可能表现为多种症状：节点 NotReady、Pod 无法启动、网络不通、磁盘满、内存不足等。这些问题的根本原因可能涉及 kubelet 配置错误、证书过期、容器运行时异常、网络插件问题、系统资源耗尽等多个层面。

有效的节点故障排查需要系统化的方法论：

1. **分层排查**：从底层（硬件/OS）向上排查（容器运行时 → kubelet → 控制面 → 网络）
2. **日志分析**：通过 systemd 日志、kubelet 日志、容器日志定位问题
3. **状态检查**：通过 kubectl 命令和 API 查询获取节点和 Pod 的当前状态
4. **对比分析**：将问题节点与正常节点对比，找出差异

本文档提供了全面的节点故障排查指南，涵盖 NotReady 节点、kubelet 启动失败、容器异常、网络问题、磁盘问题和 OOM 等常见场景的排查流程和解决方案。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 核心 | `pkg/kubelet/` | kubelet 主逻辑 |
| kubelet 工具 | `pkg/kubelet/util/` | 工具函数 |
| PLEG | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| 容器运行时 | `pkg/kubelet/cri/` | CRI 接口 |
| 驱逐管理 | `pkg/kubelet/eviction/` | 驱逐逻辑 |
| 节点状态 | `pkg/kubelet/nodestatus/` | 状态上报 |

---

## 一、节点 NotReady 排查

### 1.1 排查流程图

```
# 🟢 低风险：只读/信息收集，通常无副作用
节点 NotReady 排查:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. kubectl get nodes                                       │
  │     确认节点状态为 NotReady                                   │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  2. systemctl status kubelet                                │
  │     kubelet 是否在运行?                                       │
  │     ├── 未运行 → 启动失败排查 (第二节)                        │
  │     └── 运行中 → 继续                                        │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  3. journalctl -u kubelet -f --no-pager                     │
  │     查看 kubelet 日志中的错误信息                              │
  │     常见错误:                                                 │
  │     - 证书过期 → 证书排查                                     │
  │     - API Server 连接失败 → 网络排查                          │
  │     - cgroup 错误 → 配置排查                                  │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  4. kubectl describe node <node>                            │
  │     检查节点 Conditions:                                      │
  │     - Ready=False → kubelet 问题                             │
  │     - MemoryPressure=True → 内存不足                         │
  │     - DiskPressure=True → 磁盘不足                           │
  │     - NetworkUnavailable=True → 网络问题                     │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  5. 网络连通性检查                                            │
  │     curl -k https://<api-server>:6443/healthz                │
  │     curl -k https://127.0.0.1:2379/health                   │
  └─────────────────────────────────────────────────────────────┘
```
### 1.2 详细排查步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 确认节点状态
kubectl get nodes
# NAME      STATUS     ROLES           AGE    VERSION
# node-1    NotReady   control-plane   100d   v1.28.0

# Step 2: 检查 kubelet 状态
systemctl status kubelet
# 查看 Active 状态和最近的日志

# Step 3: 查看 kubelet 错误日志
journalctl -u kubelet -p err --no-pager -n 50

# Step 4: 检查节点 Conditions
kubectl get node <node> -o jsonpath='{.status.conditions[*]}' | jq .
# 或
kubectl describe node <node> | grep -A 10 "Conditions"

# Step 5: 检查 API Server 连接
curl -k https://<api-server-endpoint>:6443/healthz
# ok

# Step 6: 检查 etcd 连接 (仅控制面节点)
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

# Step 7: 检查 kubelet 健康
curl -k https://localhost:10250/healthz
```
---

## 二、kubelet 启动失败排查

### 2.1 常见启动错误

```bash
# 查看启动错误
journalctl -u kubelet -p err --no-pager -n 50

# 常见错误 1: 配置文件错误
# 错误信息: "failed to load kubelet config file"
# 解决: 验证配置文件
kubelet --config=/var/lib/kubelet/config.yaml --validate

# 常见错误 2: cgroup driver 不匹配
# 错误信息: "Failed to start container manager" / "cgroup driver mismatch"
# 解决: 统一 kubelet 和 containerd 的 cgroup driver
# /var/lib/kubelet/config.yaml → cgroupDriver: systemd
# /etc/containerd/config.toml → SystemdCgroup = true

# 常见错误 3: 证书问题
# 错误信息: "x509: certificate has expired" / "tls: private key does not match"
# 解决: 检查证书
ls -la /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

### 2.2 配置文件验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 kubelet 配置
kubelet --validate --config=/var/lib/kubelet/config.yaml

# 查看当前配置
kubectl get --raw /api/v1/nodes/<node>/proxy/configz | jq .

# 检查 systemd 配置
systemctl cat kubelet
cat /etc/systemd/system/kubelet.service.d/10-kubeadm.conf

# 检查容器运行时连接
crictl info
crictl ps

# 手动启动 kubelet (调试)
kubelet --config=/var/lib/kubelet/config.yaml --v=4
```
---

## 三、容器启动失败排查

### 3.1 容器状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有容器状态
crictl ps -a
# CONTAINER ID   IMAGE         STATE    NAME       ATTEMPT
# abc123         nginx:latest  Exited   web-app    3

# 查看容器日志
crictl logs <container-id>
crictl logs --tail=100 <container-id>

# 查看 Pod 列表
crictl pods

# 查看容器详情
crictl inspect <container-id>

# 查看 Pod 详情
crictl inspectp <pod-id>
```
### 3.2 常见容器错误

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ImagePullBackOff
kubectl describe pod <pod> | grep -A 5 "Events"
# 原因: 镜像拉取失败
# 解决: 检查镜像名称、imagePullSecrets、网络

# CrashLoopBackOff
kubectl logs <pod> --previous
# 原因: 容器启动后崩溃
# 解决: 检查应用日志、配置、资源限制

# OOMKilled
kubectl describe pod <pod> | grep -A 3 "Last State"
# 原因: 内存超过 limit
# 解决: 增加 memory limit 或优化应用

# CreateContainerConfigError
kubectl describe pod <pod> | grep -A 5 "Events"
# 原因: ConfigMap/Secret 不存在或引用错误
# 解决: 检查 ConfigMap/Secret 是否存在

# 手动重启容器 (kubelet 会自动重建)
crictl stop <container-id> && crictl rm <container-id>
```
---

## 四、网络问题排查

### 4.1 网络排查流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 CNI 配置
ls /etc/cni/net.d/
cat /etc/cni/net.d/*.conflist

# 2. 检查 CNI 二进制
ls /opt/cni/bin/

# 3. 检查网桥
ip link show type bridge
bridge fdb show

# 4. 检查路由
ip route

# 5. 检查 veth pair
ip link show type veth

# 6. 检查 iptables
iptables -L -n -v
iptables -t nat -L -n -v

# 7. 测试 Pod 间通信
kubectl run -it --rm debug --image=busybox -- wget -qO- http://<pod-ip>:<port>

# 8. 测试 DNS
kubectl run -it --rm debug --image=busybox -- nslookup kubernetes.default

# 9. 抓包
tcpdump -i any -n port 80 -w capture.pcap
tcpdump -i cni0 -n host <target-ip>
```
### 4.2 常见网络问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Pod 无法跨节点通信
# 排查:
ip route | grep -v default       # 检查跨节点路由
ip link show flannel.1           # 检查隧道接口 (Flannel)
ip link show tunl0               # 检查隧道接口 (Calico IPIP)
# 解决: 重启 CNI DaemonSet

# DNS 不通
# 排查:
kubectl get svc -n kube-system kube-dns
kubectl get endpoints -n kube-system kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns
# 解决: 重启 CoreDNS, 检查 Service/Endpoints

# Service 无法访问
# 排查:
kubectl get endpoints <service-name>
iptables -t nat -L KUBE-SERVICES -n
# 解决: 检查 Service selector, Pod labels

# Pod 无法访问外部网络
# 排查:
iptables -t nat -L POSTROUTING -n
# 解决: 检查 MASQUERADE 规则, 节点网络
```
---

## 五、磁盘问题排查

### 5.1 磁盘检查

```bash
# 查看磁盘使用
df -h
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/sda1       100G   85G   15G  85% /

# 检查 inode 使用
df -i
# Filesystem      Inodes   IUsed   IFree IUse% Mounted on
# /dev/sda1      6553600  524288 6029312   8% /

# 查看大文件
du -sh /var/log/*
du -sh /var/lib/containerd/*
du -sh /var/lib/kubelet/*
du -sh /var/lib/etcd/*

# 查看大日志
find /var/log -name "*.log" -size +100M -ls
journalctl --disk-usage
```

### 5.2 磁盘清理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 清理容器镜像 (containerd)
crictl rmi --prune

# 清理停止的容器
crictl rmp

# 清理日志
journalctl --vacuum-size=100M
journalctl --vacuum-time=7d

# 清理旧镜像
crictl images | grep -v "$(crictl ps -a -q | xargs -I{} crictl inspect {} | jq -r '.status.imageRef')" | awk '{print $3}' | xargs -r crictl rmi

# 注意: 不要手动删除 /var/lib/kubelet/ 下的文件
# 不要手动删除 /var/lib/containerd/ 下的文件（使用 crictl 命令）
```
---

## 六、内存不足（OOM）排查

### 6.1 内存检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 OOM 事件
dmesg | grep -i "out of memory"
dmesg | grep -i "killed process"
dmesg | grep -i "oom"

# 查看内存使用
free -h
kubectl top node <node>

# 查看 kubelet 内存使用
top -p $(pidof kubelet)

# 查看 cgroup 内存使用 (cgroup v2)
cat /sys/fs/cgroup/kubepods/memory.current
cat /sys/fs/cgroup/kubepods/memory.max

# 查看被 OOM Kill 的 Pod
kubectl get events --all-namespaces | grep OOMKilled
```
### 6.2 内存优化

```bash
# 调整 kubelet 资源限制
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
MemoryMax=1G
MemoryHigh=900M

# 调整驱逐阈值
# /var/lib/kubelet/config.yaml
evictionHard:
  memory.available: "200Mi"

# 调整系统资源预留
--kube-reserved=cpu=500m,memory=1Gi
--system-reserved=cpu=500m,memory=1Gi

# 启用 swap (不推荐，Kubernetes 默认禁止 swap)
# swapoff -a
# 如果必须启用:
# kubelet --fail-swap-on=false
```

---

## 七、综合故障排查命令速查

### 7.1 节点级命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 节点概览
kubectl get node <node>
kubectl describe node <node>

# kubelet 健康
curl -k https://localhost:10250/healthz
curl -k https://localhost:10250/metrics

# kubelet 统计
curl -k https://localhost:10250/stats/summary

# 容器运行时
crictl info
crictl ps
crictl pods

# 系统资源
df -h
free -h
top -b -n 1 | head -20
cat /proc/meminfo
cat /proc/cpuinfo
```
### 7.2 网络命令

```bash
# 基本网络检查
ip link show
ip addr show
ip route
ip neigh

# DNS 检查
nslookup kubernetes.default
dig @10.96.0.10 kubernetes.default.svc.cluster.local

# 端口检查
ss -tlnp
ss -tnp | grep :6443

# 连通性检查
ping <api-server-ip>
curl -k https://<api-server>:6443/healthz
traceroute <target-ip>
```

### 7.3 证书检查

```bash
# kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject

# CA 证书
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -dates

# 证书链验证
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /var/lib/kubelet/pki/kubelet-client-current.pem
```

---

## 八、常见故障排查速查表

| 问题 | 排查命令 | 常见原因 | 解决方案 |
|------|---------|---------|---------|
| kubelet NotReady | `journalctl -u kubelet` | kubelet 崩溃/配置错误 | 根据日志修复 |
| 容器无法启动 | `crictl logs <id>` | 镜像/配置/资源问题 | 检查镜像和配置 |
| 网络不通 | `ip link/route` | CNI 问题 | 重启 CNI DaemonSet |
| 磁盘满 | `df -h` | 镜像/日志堆积 | 清理磁盘 |
| OOM | `dmesg | grep kill` | 内存不足 | 减少资源/扩容 |
| 证书过期 | `openssl x509 -noout -dates` | 证书过期 | 续期证书 |
| DNS 不通 | `nslookup kubernetes.default` | CoreDNS 异常 | 重启 CoreDNS |
| Pod 一直 Pending | `kubectl describe pod` | 资源不足/调度约束 | 扩容/调整约束 |
| Pod 一直 Terminating | `kubectl describe pod` | 优雅终止失败 | `kubectl delete --force` |
| 节点频繁 NotReady | `journalctl -u kubelet` | 网络不稳定/负载过高 | 检查网络/增加资源 |

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `syncPod` | `pkg/kubelet/kubelet.go` | Pod 同步逻辑 |
| `PLEG` | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| `handleNotReadyNode` | `pkg/controller/nodelifecycle/` | 节点不就绪处理 |
| `evictPods` | `pkg/kubelet/eviction/` | Pod 驱逐 |
| `statusManager` | `pkg/kubelet/status/` | 状态管理 |
| `probeManager` | `pkg/kubelet/prober/` | 健康检查探针 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-17-system-foundation/速查卡/networking.md|networking]]
- [[domain-17-system-foundation/速查卡/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]


<!-- risk-assessed -->
