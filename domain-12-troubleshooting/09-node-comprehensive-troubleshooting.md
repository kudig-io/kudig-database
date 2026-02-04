# 09 - Node 全面故障排查 (Node Comprehensive Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 1. Node 状态诊断总览 (Node Status Overview)

### 1.1 Node 状态速查表

| 状态 | 含义 | 常见原因 | 优先检查项 |
|:---|:---|:---|:---|
| **Ready** | 节点正常 | - | 正常状态 |
| **NotReady** | 节点异常 | kubelet故障/网络/资源压力 | kubelet日志/节点资源 |
| **SchedulingDisabled** | 禁止调度 | 手动cordon/维护 | 检查是否手动操作 |
| **Unknown** | 状态未知 | 节点失联/API Server连接断开 | 网络/kubelet |
| **MemoryPressure** | 内存压力 | 内存不足 | 节点内存使用 |
| **DiskPressure** | 磁盘压力 | 磁盘空间不足 | 节点磁盘使用 |
| **PIDPressure** | PID压力 | 进程数过多 | 节点PID使用 |
| **NetworkUnavailable** | 网络不可用 | CNI未配置/故障 | CNI状态 |

### 1.2 Node Conditions 详解

```bash
# 查看节点状态
kubectl get nodes
kubectl describe node <node-name> | grep -A20 "Conditions:"
```

| Condition | True含义 | False含义 |
|:---|:---|:---|
| **Ready** | kubelet健康,可接收Pod | kubelet异常 |
| **MemoryPressure** | 节点内存不足 | 内存充足 |
| **DiskPressure** | 节点磁盘不足 | 磁盘充足 |
| **PIDPressure** | 进程数接近上限 | PID充足 |
| **NetworkUnavailable** | 网络未配置 | 网络正常 |

---

## 2. NotReady 状态排查 (NotReady Troubleshooting)

### 2.1 排查流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Node NotReady 排查流程                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐                                                           │
│   │Node NotReady│                                                           │
│   └──────┬──────┘                                                           │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: SSH到节点检查kubelet状态                      │                 │
│   │ systemctl status kubelet                             │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── kubelet未运行 ──▶ 启动kubelet                                 │
│          │         └─▶ systemctl start kubelet                             │
│          │                                                                  │
│          ├─── kubelet运行但报错 ──▶ 查看日志                                │
│          │         └─▶ journalctl -u kubelet -f                            │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查容器运行时                                │                 │
│   │ systemctl status containerd/docker                   │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 运行时未运行 ──▶ 启动运行时                                   │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查证书是否过期                              │                 │
│   │ openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates       │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 证书过期 ──▶ 轮换证书                                         │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查资源压力                                  │                 │
│   │ df -h / free -m / ps aux --sort=-%mem               │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 kubelet 排查命令

```bash
# === 检查kubelet服务状态 ===
systemctl status kubelet
systemctl is-active kubelet

# === 查看kubelet日志 ===
journalctl -u kubelet -f                    # 实时跟踪
journalctl -u kubelet --since "10 minutes ago"  # 最近10分钟
journalctl -u kubelet | grep -i error       # 过滤错误

# === 重启kubelet ===
systemctl restart kubelet

# === 检查kubelet配置 ===
cat /var/lib/kubelet/config.yaml
cat /etc/kubernetes/kubelet.conf

# === 检查kubelet证书 ===
ls -la /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

### 2.3 常见kubelet错误

| 错误信息 | 原因 | 解决方案 |
|:---|:---|:---|
| `Unable to connect to the server` | API Server不可达 | 检查网络/API Server |
| `certificate has expired` | 证书过期 | 轮换证书 |
| `failed to run Kubelet: cannot create certificate` | 证书问题 | 检查CA/重新生成 |
| `node not found` | 节点未注册 | 检查kubelet配置/重新注册 |
| `container runtime is down` | 运行时故障 | 重启containerd/docker |
| `PLEG is not healthy` | PLEG超时 | 检查运行时/重启kubelet |

---

## 3. 资源压力排查 (Resource Pressure Troubleshooting)

### 3.1 内存压力 (MemoryPressure)

```bash
# === 检查内存使用 ===
free -h
cat /proc/meminfo | grep -E "MemTotal|MemFree|MemAvailable|Buffers|Cached"

# === 查看内存使用最高的进程 ===
ps aux --sort=-%mem | head -20

# === 查看Pod内存使用 ===
kubectl top pods --all-namespaces --sort-by=memory

# === 检查驱逐阈值 ===
kubectl get node <node-name> -o jsonpath='{.status.allocatable.memory}'
cat /var/lib/kubelet/config.yaml | grep -A10 evictionHard
```

**内存压力解决方案**:

| 方案 | 操作 | 风险 |
|:---|:---|:---|
| **清理缓存** | `echo 3 > /proc/sys/vm/drop_caches` | 低 |
| **驱逐低优先级Pod** | 自动或手动驱逐 | 中 |
| **调整驱逐阈值** | 修改kubelet配置 | 低 |
| **扩展节点内存** | 升级节点 | 低 |
| **减少工作负载** | 迁移Pod | 中 |

### 3.2 磁盘压力 (DiskPressure)

```bash
# === 检查磁盘使用 ===
df -h
df -i   # 检查inode

# === 查看大文件/目录 ===
du -sh /* | sort -hr | head -20
du -sh /var/lib/containerd/*
du -sh /var/lib/docker/*
du -sh /var/log/*

# === 清理容器资源 ===
crictl rmi --prune           # 清理未使用镜像
docker system prune -af      # Docker清理

# === 清理日志 ===
find /var/log -name "*.log" -size +100M -exec truncate -s 0 {} \;
journalctl --vacuum-size=500M
```

**磁盘清理检查清单**:

| 清理项 | 命令 | 释放空间 |
|:---|:---|:---|
| **未使用镜像** | `crictl rmi --prune` | 高 |
| **已停止容器** | `crictl rm $(crictl ps -a -q)` | 中 |
| **日志文件** | `journalctl --vacuum-size=500M` | 中 |
| **临时文件** | `rm -rf /tmp/*` | 低 |
| **旧内核** | `apt autoremove` | 中 |

### 3.3 PID压力 (PIDPressure)

```bash
# === 检查PID使用 ===
cat /proc/sys/kernel/pid_max
ps aux | wc -l

# === 查看进程数最多的用户/进程 ===
ps aux | awk '{print $1}' | sort | uniq -c | sort -rn | head

# === 检查kubelet PID限制 ===
cat /var/lib/kubelet/config.yaml | grep podPidsLimit

# === 调整系统PID限制 ===
sysctl -w kernel.pid_max=131072
```

---

## 4. 网络问题排查 (Network Troubleshooting)

### 4.1 NetworkUnavailable 排查

```bash
# === 检查CNI状态 ===
ls /etc/cni/net.d/
cat /etc/cni/net.d/*.conf

# === 检查CNI Pod ===
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l app=flannel

# === 检查网络接口 ===
ip addr
ip route

# === 检查iptables ===
iptables -t nat -L -n | head -50
iptables -L -n | head -50
```

### 4.2 节点网络连通性

```bash
# === 测试与API Server连接 ===
curl -k https://<api-server>:6443/healthz

# === 测试与其他节点连接 ===
ping <other-node-ip>

# === 检查防火墙 ===
iptables -L INPUT -n
firewall-cmd --list-all    # RHEL/CentOS

# === 检查必需端口 ===
ss -tlnp | grep -E "6443|10250|10251|10252"
```

### 4.3 必需端口列表

| 端口 | 组件 | 用途 |
|:---:|:---|:---|
| 6443 | API Server | Kubernetes API |
| 2379-2380 | etcd | etcd客户端/对等通信 |
| 10250 | kubelet | kubelet API |
| 10251 | kube-scheduler | 调度器 |
| 10252 | kube-controller-manager | 控制器管理器 |
| 10255 | kubelet | 只读端口(可选) |
| 30000-32767 | NodePort | Service NodePort范围 |

---

## 5. 容器运行时排查 (Container Runtime Troubleshooting)

### 5.1 containerd 排查

```bash
# === 检查服务状态 ===
systemctl status containerd
systemctl is-active containerd

# === 查看日志 ===
journalctl -u containerd -f
journalctl -u containerd --since "10 minutes ago"

# === 检查配置 ===
cat /etc/containerd/config.toml

# === 使用crictl ===
crictl info
crictl ps
crictl images
crictl pods

# === 重启containerd ===
systemctl restart containerd
```

### 5.2 Docker 排查 (如果使用)

```bash
# === 检查服务状态 ===
systemctl status docker
docker info

# === 查看日志 ===
journalctl -u docker -f

# === 检查存储驱动 ===
docker info | grep "Storage Driver"

# === 清理资源 ===
docker system prune -af
docker volume prune -f
```

### 5.3 PLEG (Pod Lifecycle Event Generator) 问题

```bash
# === 症状 ===
# kubelet日志: "PLEG is not healthy"
# 节点变为NotReady

# === 原因 ===
# 容器运行时响应慢
# 节点上Pod过多
# 存储I/O问题

# === 排查 ===
crictl ps | wc -l                    # 检查容器数量
iostat -x 1                          # 检查I/O等待
journalctl -u containerd | grep -i error

# === 解决 ===
# 1. 减少节点Pod数量
# 2. 重启containerd
# 3. 检查存储性能
```

---

## 6. 证书问题排查 (Certificate Troubleshooting)

### 6.1 检查证书状态

```bash
# === 检查kubelet证书 ===
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -text | grep -E "Not Before|Not After"

# === 检查API Server证书 ===
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text | grep -E "Not Before|Not After"

# === 批量检查所有证书 ===
for cert in /etc/kubernetes/pki/*.crt; do
  echo "=== $cert ==="
  openssl x509 -in $cert -noout -enddate
done

# === 检查证书是否即将过期 ===
kubeadm certs check-expiration
```

### 6.2 证书轮换

```bash
# === kubeadm管理的集群 ===
kubeadm certs renew all

# === 手动轮换kubelet证书 ===
# 1. 启用自动轮换 (kubelet配置)
rotateCertificates: true

# 2. 重启kubelet
systemctl restart kubelet

# === 检查CSR ===
kubectl get csr
kubectl certificate approve <csr-name>
```

---

## 7. 节点维护操作 (Node Maintenance)

### 7.1 安全下线节点

```bash
# === Step 1: 标记不可调度 ===
kubectl cordon <node-name>

# === Step 2: 驱逐Pod ===
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# === Step 3: 执行维护 ===
# ... 维护操作 ...

# === Step 4: 恢复调度 ===
kubectl uncordon <node-name>
```

### 7.2 强制删除节点

```bash
# === 删除节点 (节点已失联) ===
kubectl delete node <node-name>

# === 清理节点上的Pod ===
kubectl get pods --all-namespaces -o wide | grep <node-name>
kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0
```

### 7.3 节点重新加入集群

```bash
# === kubeadm集群 ===
# 1. 在Master获取join命令
kubeadm token create --print-join-command

# 2. 在节点执行
kubeadm reset -f
kubeadm join <master>:6443 --token <token> --discovery-token-ca-cert-hash <hash>
```

---

## 8. 监控与告警 (Monitoring & Alerting)

### 8.1 关键监控指标

| 指标 | Prometheus查询 | 告警阈值 |
|:---|:---|:---|
| **节点状态** | `kube_node_status_condition{condition="Ready",status="true"}` | = 0 |
| **内存压力** | `kube_node_status_condition{condition="MemoryPressure",status="true"}` | = 1 |
| **磁盘压力** | `kube_node_status_condition{condition="DiskPressure",status="true"}` | = 1 |
| **CPU使用率** | `100 - (avg by(instance)(irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)` | > 90% |
| **内存使用率** | `(1 - node_memory_MemAvailable_bytes/node_memory_MemTotal_bytes) * 100` | > 90% |
| **磁盘使用率** | `(1 - node_filesystem_avail_bytes/node_filesystem_size_bytes) * 100` | > 85% |

### 8.2 告警规则示例

```yaml
groups:
- name: node.rules
  rules:
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Node {{ $labels.node }} is not ready"
      
  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has memory pressure"
      
  - alert: NodeDiskPressure
    expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has disk pressure"
```

---

## 9. 一键诊断脚本 (Diagnostic Script)

```bash
#!/bin/bash
NODE=$1

if [ -z "$NODE" ]; then
  echo "Usage: $0 <node-name>"
  exit 1
fi

echo "=== Node Status ==="
kubectl get node $NODE -o wide

echo -e "\n=== Node Conditions ==="
kubectl describe node $NODE | grep -A15 "Conditions:"

echo -e "\n=== Node Resources ==="
kubectl describe node $NODE | grep -A10 "Allocated resources:"

echo -e "\n=== Node Events ==="
kubectl describe node $NODE | grep -A20 "Events:"

echo -e "\n=== Pods on Node ==="
kubectl get pods --all-namespaces -o wide --field-selector spec.nodeName=$NODE

echo -e "\n=== System Pods Status ==="
kubectl get pods -n kube-system -o wide --field-selector spec.nodeName=$NODE
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
