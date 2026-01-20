| 组件 | 角色 | 部署方式 | 关键配置 | 版本特定变更 | 监控指标 | 常见问题 | 运维最佳实践 |
|-----|------|---------|---------|-------------|---------|---------|-------------|
| **CoreDNS** | 集群DNS服务 | Deployment | `Corefile` ConfigMap | v1.28: DNS缓存优化; v1.30: 性能提升 | `coredns_dns_requests_total`, `coredns_dns_responses_total` | 查询超时; 上游失败 | 副本数=max(2, nodes/100); 启用缓存插件 |
| **Metrics Server** | 资源指标API | Deployment | `--kubelet-insecure-tls`(测试) | v0.6+: 稳定API | `metrics_server_*` | kubelet连接失败 | 生产使用TLS; HPA/VPA依赖 |
| **Ingress Controller** | 入口流量路由 | Deployment/DaemonSet | 因控制器而异 | v1.19: networking.k8s.io/v1稳定 | 控制器特定 | 证书错误; 后端不可达 | 高可用部署; 配置健康检查 |
| **CNI Plugin** | 容器网络 | DaemonSet/主机安装 | `/etc/cni/net.d/` | v1.25: 双栈增强; v1.28: 网络策略改进 | 插件特定 | IP分配失败; 网络分区 | 选择适合场景的CNI; 监控网络指标 |

---

## 4. 组件故障场景与恢复

### 4.1 控制平面故障场景

#### API Server 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **503 Service Unavailable** | 请求过载/etcd不健康 | 1. 检查`apiserver_current_inflight_requests`<br>2. 检查etcd健康 | 1. 增大`--max-requests-inflight`<br>2. 修复etcd | 启用APF流控; 监控P99延迟 |
| **证书过期** | 证书未自动轮转 | `kubeadm certs check-expiration` | `kubeadm certs renew all` | 启用证书自动轮转; 监控证书有效期 |
| **OOM Killed** | 内存不足 | `journalctl -u kube-apiserver` | 1. 增加内存限制<br>2. 减小Watch缓存 | 根据集群规模配置资源 |
| **审计日志磁盘满** | 审计日志未轮转 | `df -h /var/log` | 清理旧日志 | 配置日志轮转参数 |
| **etcd连接超时** | etcd响应慢/网络抖动 | 检查`etcd_request_duration_seconds` | 1. 优化etcd<br>2. 增加超时时间 | SSD存储; 压缩etcd历史版本 |

#### etcd 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **数据库空间满** | 超过quota-backend-bytes | `etcdctl alarm list` | 1. 压缩`etcdctl compact`<br>2. 碎片整理`etcdctl defrag`<br>3. 解除告警`etcdctl alarm disarm` | 自动压缩; 监控DB大小 |
| **Leader频繁切换** | 网络抖动/磁盘IO慢 | 检查`etcd_server_leader_changes_seen_total` | 1. 检查网络延迟<br>2. 更换SSD/NVMe<br>3. 调整心跳参数 | 专用网络; NVMe存储 |
| **集群脑裂** | 网络分区 | `etcdctl member list` | 1. 恢复网络<br>2. 移除故障节点`etcdctl member remove`<br>3. 添加新成员 | 3/5/7奇数节点; 跨机架部署 |
| **单节点故障** | 进程崩溃/磁盘损坏 | `systemctl status etcd` | 1. 重启etcd<br>2. 从快照恢复<br>3. 重新加入集群 | 定期备份快照; RAID磁盘 |
| **fsync延迟高** | 磁盘IO慢 | `etcd_disk_wal_fsync_duration_seconds` > 10ms | 更换SSD/NVMe; WAL独立分区 | 专用SSD; 禁用swap |

#### Scheduler 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **Pod长时间Pending** | 资源不足/调度约束 | `kubectl describe pod` 查看Events | 1. 增加节点<br>2. 放宽调度约束<br>3. 启用抢占 | 监控资源使用率; 预留buffer |
| **调度延迟高** | 大量待调度Pod/Filter插件慢 | 检查`scheduler_scheduling_attempt_duration_seconds` | 1. 增加Scheduler副本<br>2. 优化调度配置<br>3. 提高`--kube-api-qps` | 批量调度; 自定义调度器 |
| **Leader选举失败** | API Server不可达 | `kubectl logs -n kube-system kube-scheduler-*` | 修复API Server | HA部署Scheduler |

#### Controller Manager 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **Deployment不滚动更新** | Controller故障/API限流 | 检查`workqueue_depth` | 1. 重启Controller<br>2. 提高API QPS | 监控控制器队列深度 |
| **EndpointSlice未更新** | EndpointSlice Controller异常 | `kubectl get endpointslices` | 重启Controller Manager | 监控Service流量 |
| **Node驱逐失败** | 驱逐速率限制 | 检查`--node-eviction-rate` | 调整驱逐参数 | 配置合理驱逐速率 |

---

### 4.2 节点组件故障场景

#### kubelet 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **PLEG不健康** | CRI调用超时/容器运行时hung | `journalctl -u kubelet \| grep PLEG` | 1. 重启containerd<br>2. 重启kubelet<br>3. 检查磁盘IO | SSD存储; 监控PLEG延迟 |
| **Node NotReady** | kubelet未上报心跳 | `kubectl describe node` | 1. 重启kubelet<br>2. 检查网络 | 监控Node状态; 配置健康检查 |
| **磁盘压力驱逐** | 磁盘空间不足 | 检查`nodefs.available` | 1. 清理镜像`crictl rmi --prune`<br>2. 清理日志<br>3. 扩容磁盘 | 配置镜像GC; 日志轮转 |
| **内存压力驱逐** | 内存不足 | 检查`memory.available` | 1. 增加节点内存<br>2. 减少Pod副本 | 配置资源请求; 监控内存使用 |
| **CRI调用超时** | containerd响应慢 | `crictl ps` 超时 | 1. 重启containerd<br>2. 检查磁盘IO | 独立数据盘; 限制容器日志大小 |
| **证书轮转失败** | CSR未批准 | `kubectl get csr` | `kubectl certificate approve` | 启用自动批准CSR |

#### kube-proxy 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **Service不通** | iptables/IPVS规则未生成 | `iptables -t nat -L` / `ipvsadm -Ln` | 1. 重启kube-proxy<br>2. 检查日志 | 监控规则同步延迟 |
| **conntrack表满** | 高并发连接数 | `sysctl net.netfilter.nf_conntrack_count` | 增大`net.netfilter.nf_conntrack_max` | 调整conntrack参数; 启用conntrack清理 |
| **规则同步慢** | 大量Service变更 | 检查`kubeproxy_sync_proxy_rules_duration_seconds` | 切换IPVS模式 | 使用IPVS; 批量更新Service |
| **IPVS模块未加载** | 内核模块缺失 | `lsmod \| grep ip_vs` | `modprobe ip_vs` | 配置模块自动加载 |

#### Container Runtime 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **镜像拉取失败** | 网络问题/镜像不存在/认证失败 | `crictl pull <image>` | 1. 检查网络<br>2. 配置镜像加速器<br>3. 检查Secret | 使用内网镜像仓库; 预拉镜像 |
| **容器创建超时** | 磁盘IO慢/overlayfs问题 | `crictl ps -a` | 1. 重启containerd<br>2. 清理未使用镜像 | SSD存储; 定期GC |
| **containerd进程崩溃** | OOM/磁盘满 | `journalctl -u containerd` | 1. 增加内存<br>2. 扩容磁盘<br>3. 重启服务 | 监控资源使用; 限制容器日志 |
| **cgroup v2兼容问题** | 旧版containerd | 检查`/sys/fs/cgroup` | 升级containerd >= 1.6 | 使用新版运行时 |

---

## 5. 性能基准与容量规划

### 5.1 性能基准测试数据

#### API Server 性能基准

| 场景 | 集群规模 | QPS | P50延迟 | P99延迟 | 资源配置 |
|------|---------|-----|---------|---------|---------|
| **List Pods** | 5000 Pods | 100 | 50ms | 200ms | 4C8G |
| **Watch Pods** | 5000 Pods | 1000 events/s | 10ms | 50ms | 4C8G |
| **Create Pod** | 1000 req/s | 1000 | 100ms | 500ms | 8C16G |
| **Update Deployment** | 100 req/s | 100 | 150ms | 800ms | 4C8G |

#### etcd 性能基准

| 操作 | 延迟(SSD) | 延迟(NVMe) | 吞吐量 | 测试工具 |
|------|----------|-----------|--------|---------|
| **Write (fsync)** | 5-10ms | 1-3ms | 10000 writes/s | etcdctl benchmark |
| **Read** | <1ms | <0.5ms | 50000 reads/s | etcdctl benchmark |
| **Watch** | - | - | 10000 events/s | - |

```bash
# etcd性能测试
# 写入测试
etcdctl benchmark put --total=100000 --key-size=8 --val-size=256

# 读取测试
etcdctl benchmark range --total=100000 --key-size=8

# 混合测试
etcdctl benchmark txn-put --total=100000 --key-size=8 --val-size=256
```

#### Scheduler 性能基准

| 指标 | 小集群(<50节点) | 中集群(50-250节点) | 大集群(250-1000节点) |
|------|----------------|-------------------|---------------------|
| **调度吞吐量** | 100 Pods/s | 50 Pods/s | 20 Pods/s |
| **调度延迟 P99** | 50ms | 200ms | 1s |
| **内存使用** | 500MB | 2GB | 8GB |

---

### 5.2 容量规划指南

#### 集群规模评估

| 维度 | 小集群 | 中集群 | 大集群 | 超大集群 |
|------|-------|--------|--------|---------|
| **节点数** | <50 | 50-250 | 250-1000 | >1000 |
| **Pod数** | <1500 | 1500-7500 | 7500-30000 | >30000 |
| **Service数** | <500 | 500-2500 | 2500-10000 | >10000 |
| **Deployment数** | <200 | 200-1000 | 1000-5000 | >5000 |
| **CRD对象数** | <1000 | 1000-10000 | 10000-50000 | >50000 |

#### 控制平面资源配置

| 组件 | 小集群 | 中集群 | 大集群 | 超大集群 |
|------|-------|--------|--------|---------|
| **API Server** | 2C4G × 2 | 4C8G × 3 | 8C16G × 3 | 16C32G × 5 |
| **etcd** | 2C4G SSD × 3 | 4C8G SSD × 3 | 8C16G NVMe × 5 | 16C32G NVMe × 7 |
| **Scheduler** | 1C2G × 2 | 2C4G × 3 | 4C8G × 3 | 8C16G × 5 |
| **Controller Manager** | 2C4G × 2 | 4C8G × 3 | 8C16G × 3 | 16C32G × 5 |
| **合计 (最小)** | 14C24G | 30C56G | 60C120G | 130C260G |

#### 节点资源预留

```yaml
# kubelet配置 - 4C16G节点示例
systemReserved:
  cpu: 500m        # 系统进程预留
  memory: 1Gi
  ephemeral-storage: 10Gi

kubeReserved:
  cpu: 100m        # kubelet预留
  memory: 500Mi
  ephemeral-storage: 5Gi

# 可用于Pod的资源
# CPU: 4核 - 500m - 100m = 3400m
# Memory: 16Gi - 1Gi - 500Mi = 14.5Gi
```

#### etcd 存储容量规划

| 对象类型 | 单对象大小 | 10000对象 | 50000对象 | 100000对象 |
|---------|----------|----------|----------|-----------|
| **Pod** | ~5KB | 50MB | 250MB | 500MB |
| **Service** | ~2KB | 20MB | 100MB | 200MB |
| **Endpoint** | ~1KB | 10MB | 50MB | 100MB |
| **Event** | ~1KB | 10MB | 50MB | 100MB |
| **ConfigMap** | ~10KB | 100MB | 500MB | 1GB |
| **Secret** | ~5KB | 50MB | 250MB | 500MB |

```
推荐etcd存储配额:
- 小集群: 4GB
- 中集群: 8GB
- 大集群: 16GB
- 超大集群: 32GB
```

#### 网络带宽规划

| 场景 | 带宽需求 | 说明 |
|------|---------|------|
| **控制平面互联** | 1Gbps | Master节点间 |
| **Node → Master** | 100Mbps | 心跳+指标上报 |
| **Node间通信** | 10Gbps | Pod互访 (高流量场景) |
| **镜像拉取** | 1Gbps | 加速启动 |

---

## 6. 生产级配置模板

### 6.1 HA控制平面 kubeadm 配置

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "loadbalancer.example.com:6443"  # LB地址
certificatesDir: /etc/kubernetes/pki
imageRepository: registry.k8s.io
clusterName: production-cluster

# === etcd配置 ===
etcd:
  external:
    endpoints:
    - https://192.168.1.101:2379
    - https://192.168.1.102:2379
    - https://192.168.1.103:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

# === 网络配置 ===
networking:
  serviceSubnet: 10.96.0.0/12
  podSubnet: 10.244.0.0/16
  dnsDomain: cluster.local

# === API Server配置 ===
apiServer:
  timeoutForControlPlane: 5m
  certSANs:
  - loadbalancer.example.com
  - 192.168.1.100
  - 127.0.0.1
  extraArgs:
    # 并发控制
    max-requests-inflight: "800"
    max-mutating-requests-inflight: "400"
    # 审计日志
    audit-log-path: /var/log/kubernetes/audit.log
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    # etcd优化
    etcd-compaction-interval: "5m"
    # 事件TTL
    event-ttl: "1h"
  extraVolumes:
  - name: audit-policy
    hostPath: /etc/kubernetes/audit-policy.yaml
    mountPath: /etc/kubernetes/audit-policy.yaml
    readOnly: true
  - name: audit-log
    hostPath: /var/log/kubernetes
    mountPath: /var/log/kubernetes

# === Scheduler配置 ===
scheduler:
  extraArgs:
    kube-api-qps: "100"
    kube-api-burst: "200"
    config: /etc/kubernetes/scheduler-config.yaml
  extraVolumes:
  - name: scheduler-config
    hostPath: /etc/kubernetes/scheduler-config.yaml
    mountPath: /etc/kubernetes/scheduler-config.yaml
    readOnly: true

# === Controller Manager配置 ===
controllerManager:
  extraArgs:
    # 并发控制
    concurrent-deployment-syncs: "10"
    concurrent-replicaset-syncs: "10"
    concurrent-statefulset-syncs: "10"
    # API限流
    kube-api-qps: "100"
    kube-api-burst: "200"
    # 节点驱逐
    node-eviction-rate: "0.1"
    pod-eviction-timeout: "5m"

---
# kubelet配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: systemd
maxPods: 110
systemReserved:
  cpu: 500m
  memory: 1Gi
kubeReserved:
  cpu: 100m
  memory: 500Mi
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
imageGCHighThresholdPercent: 80
imageGCLowThresholdPercent: 70
```

---

## 7. 诊断与排查工具

### 7.1 快速诊断脚本

```bash
#!/bin/bash
# k8s-healthcheck.sh - Kubernetes集群健康检查

echo "=== 1. 集群基础信息 ==="
kubectl version --short
kubectl cluster-info

echo -e "\n=== 2. 节点状态 ==="
kubectl get nodes -o wide

echo -e "\n=== 3. 控制平面组件状态 ==="
kubectl get pods -n kube-system -o wide

echo -e "\n=== 4. 组件健康检查 ==="
echo "API Server:"
curl -k https://127.0.0.1:6443/healthz && echo " - OK"

echo "etcd:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

echo -e "\n=== 5. 资源使用情况 ==="
kubectl top nodes

echo -e "\n=== 6. 关键指标检查 ==="
echo "待调度Pod数:"
kubectl get pods --all-namespaces --field-selector=status.phase=Pending | wc -l

echo "NotReady节点数:"
kubectl get nodes --no-headers | grep -v "Ready" | grep "NotReady" | wc -l

echo "CrashLoopBackOff Pod数:"
kubectl get pods --all-namespaces --field-selector=status.phase=Running | \
  grep -c "CrashLoopBackOff"

echo -e "\n=== 7. 证书有效期检查 ==="
kubeadm certs check-expiration 2>/dev/null || echo "需在master节点执行"

echo -e "\n=== 8. etcd数据库大小 ==="
ETCDCTL_API=3 etcdctl endpoint status --write-out=table \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

echo -e "\n=== 9. 组件日志快速检查 (最近10行错误) ==="
echo "kubelet:"
journalctl -u kubelet --no-pager -n 10 | grep -i error

echo "containerd:"
journalctl -u containerd --no-pager -n 10 | grep -i error
```

### 7.2 常用诊断命令

```bash
# === 组件状态检查 ===
# 检查所有组件
kubectl get componentstatuses

# 详细API就绪检查
kubectl get --raw='/readyz?verbose'

# === 节点诊断 ===
# 节点详细信息
kubectl describe node <node-name>

# 节点容量与分配
kubectl describe node <node-name> | grep -A 5 "Allocated resources"

# 驱逐Pod
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# === Pod诊断 ===
# Pod详细信息
kubectl describe pod <pod-name> -n <namespace>

# Pod日志 (最近100行)
kubectl logs <pod-name> -n <namespace> --tail=100

# 前一个容器日志 (重启后查看)
kubectl logs <pod-name> -n <namespace> --previous

# 进入Pod调试
kubectl exec -it <pod-name> -n <namespace> -- sh

# === etcd诊断 ===
# 查看成员列表
ETCDCTL_API=3 etcdctl member list --write-out=table

# 查看告警
ETCDCTL_API=3 etcdctl alarm list

# 查看性能指标
ETCDCTL_API=3 etcdctl endpoint health
ETCDCTL_API=3 etcdctl endpoint status --write-out=table

# === 容器运行时诊断 ===
# 列出所有容器
crictl ps -a

# 查看容器日志
crictl logs <container-id>

# 检查镜像
crictl images

# 容器详细信息
crictl inspect <container-id>

# === 网络诊断 ===
# Service Endpoints
kubectl get endpoints <service-name> -n <namespace>

# Service详细信息
kubectl describe service <service-name> -n <namespace>

# IPVS规则 (IPVS模式)
ipvsadm -Ln

# iptables规则 (iptables模式)
iptables -t nat -L -n -v | grep <service-name>

# DNS测试
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup kubernetes
```

---

## ACK特定组件集成

| 组件 | ACK托管方式 | 配置入口 | 特殊优化 | 监控集成 |
|-----|------------|---------|---------|---------|
| **控制平面** | 全托管(Pro版) | 控制台 | 自动HA, 自动升级 | ARMS自动接入 |
| **etcd** | 托管或独立 | 控制台 | 自动备份 | 云监控集成 |
| **CoreDNS** | 托管 | ConfigMap | 阿里云DNS集成 | SLS日志 |
| **Ingress** | ALB/Nginx可选 | 控制台+YAML | SLB自动绑定 | ALB监控 |
| **CNI** | Terway/Flannel | 创建时选择 | ENI直通性能 | 网络监控 |

---

## 组件版本兼容性矩阵

| 组件 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|-----|-------|-------|-------|-------|-------|-------|-------|-------|
| **etcd** | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x |
| **containerd** | 1.6+ | 1.6+ | 1.7+ | 1.7+ | 1.7+ | 1.7+/2.0 | 1.7+/2.0 | 2.0+ |
| **CoreDNS** | 1.9+ | 1.9+ | 1.10+ | 1.10+ | 1.11+ | 1.11+ | 1.11+ | 1.11+ |
| **Metrics Server** | 0.6+ | 0.6+ | 0.6+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ |
| **Calico** | 3.24+ | 3.25+ | 3.26+ | 3.27+ | 3.27+ | 3.28+ | 3.28+ | 3.28+ |
| **Cilium** | 1.12+ | 1.13+ | 1.14+ | 1.14+ | 1.15+ | 1.15+ | 1.16+ | 1.16+ |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-01
