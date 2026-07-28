---
title: 04 - 生产环境部署 (Production Environment Deployment) [deployment]
description: 'title: 04 - 生产环境部署 (Production Environment Deployment)'
summary: 'title: 04 - 生产环境部署 (Production Environment Deployment)'
category: general
tags:
- deployment
- production
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- calico
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- 生产环境部署 (Production Environment Deployment) 是什么
- 如何 生产环境部署 (Production Environment Deployment)
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 生产环境部署
- Production
- Environment
- Deployment
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- cni-basics
- etcd-basics
- tls-basics
- backup-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 04 - 生产环境部署 (Production Environment Deployment)
description: '# 04 - 生产环境部署 (Production Environment Deployment)'
category: deployment
tags:
- k8s
- deployment
- rolling-update
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- [[prometheus|prometheus]]
- grafana
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 生产环境部署 (Production Environment Deployment) 是什么
- 如何 生产环境部署 (Production Environment Deployment)
trigger_keywords:
- 生产环境部署
- Production
- Environment
- Deployment
- deployment
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

# 04 - 生产环境部署 (Production Environment Deployment)

> **适用版本**: Kubernetes v1.25 - v1.32 | **适用场景**: 企业级生产部署 | **复杂度**: ⭐⭐⭐⭐⭐  
> **目标**: 搭建高可用、安全合规、可观测、可灾备的企业级 K8s 生产集群

---

<!-- chunk: 概述 -->## 概述

生产环境部署是 Kubernetes 最严格的场景，需要满足高可用 (HA)、安全合规、性能优化、灾备恢复等企业级要求。本文档覆盖从架构设计到落地实施的完整方案。

**本文你将学会**:
- 设计企业级分层架构和容量规划
- 搭建 3 Master + N Worker 高可用集群 (含 etcd 集群、HAProxy/Keepalived)
- 配置生产级网络（多平面、NetworkPolicy、零信任）
- 实施安全合规（OIDC、RBAC、CIS 基线、审计日志）
- 搭建分层存储架构（NVMe/SSD/HDD + Rook-Ceph）
- 配置生产级 Deployment（探针、PDB、拓扑分布、安全上下文）
- 部署监控告警体系（Prometheus + Grafana + Alertmanager）
- 建立备份灾备策略（etcd 快照 + Velero + 跨区域复制）
- 执行集群升级和证书轮转

**前置知识**: 建议先完成 [03-研发环境部署](./03-development-environment-deployment.md)，熟悉 kubeadm、Helm、监控和 RBAC。

---

<!-- chunk: 一、架构设计 -->## 一、架构设计

## 1.1 企业级分层架构

```
┌───────────────────────────── 生产集群架构 ─────────────────────────────┐
│                                                                         │
│  ┌─── 接入层 (Ingress Layer) ───────────────────────────────────────┐  │
│  │ 硬件 LB / 云 LB → Nginx Ingress (2+ 副本) → WAF / 限流         │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  ┌─── 控制平面 (Control Plane) ─────────────────────────────────────┐  │
│  │                                                                   │  │
│  │  HAProxy/Keepalived (VIP: 10.0.0.100)                            │  │
│  │         ↓              ↓              ↓                           │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐                       │  │
│  │  │ Master-1 │  │ Master-2 │  │ Master-3 │  ← API Server ×3     │  │
│  │  │ etcd-1   │  │ etcd-2   │  │ etcd-3   │  ← etcd 集群 ×3      │  │
│  │  └──────────┘  └──────────┘  └──────────┘                       │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  ┌─── 工作节点 (Worker Nodes) ──────────────────────────────────────┐  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  ... (N 节点)   │  │
│  │  │ W-1  │ │ W-2  │ │ W-3  │ │ W-4  │ │ W-5  │                  │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘                  │  │
│  │  按标签分组: app=web / app=api / app=worker / infra=true        │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                              ↓                                          │
│  ┌─── 基础设施层 (Infrastructure) ──────────────────────────────────┐  │
│  │  Prometheus + Grafana │ Loki + Promtail │ Velero │ Harbor        │  │
│  │  Rook-Ceph / NFS     │ MetalLB          │ cert-manager           │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

## 1.2 生产环境规模分级

| 规模等级 | 节点数 | Pod 数 | Master 规格 | Worker 规格 | etcd 部署方式 |
|----------|--------|--------|-------------|-------------|--------------|
| **小型** | 10-50 | 500-2000 | 4C/16G/200G SSD | 8C/32G/500G | Stacked (与 Master 共存) |
| **中型** | 50-200 | 2K-8K | 8C/32G/500G SSD | 16C/64G/1T | External (独立 etcd 集群) |
| **大型** | 200-1000 | 8K-40K | 16C/64G/1T NVMe | 32C/128G/2T | External + 高性能 NVMe |
| **超大型** | 1000+ | 40K+ | 32C/128G/2T NVMe | 按负载定制 | External + 分片 |

## 1.3 节点规划示例 (小型生产集群)

| 角色 | 主机名 | IP | CPU | 内存 | 存储 | 说明 |
|------|--------|-----|-----|------|------|------|
| Master+etcd | master-1 | 10.0.1.10 | 8C | 32GB | 500GB NVMe | 控制平面 + etcd |
| Master+etcd | master-2 | 10.0.1.11 | 8C | 32GB | 500GB NVMe | 控制平面 + etcd |
| Master+etcd | master-3 | 10.0.1.12 | 8C | 32GB | 500GB NVMe | 控制平面 + etcd |
| Worker | worker-1 | 10.0.1.21 | 16C | 64GB | 500GB NVMe + 2TB HDD | 应用节点 |
| Worker | worker-2 | 10.0.1.22 | 16C | 64GB | 500GB NVMe + 2TB HDD | 应用节点 |
| Worker | worker-3 | 10.0.1.23 | 16C | 64GB | 500GB NVMe + 2TB HDD | 应用节点 |
| LB (VIP) | - | 10.0.1.100 | - | - | - | HAProxy VIP |

## 1.4 部署前检查清单

```yaml
# 在开始部署前，确认以下事项全部完成:
production_precheck:
  business:
    - [ ] 明确业务 SLA 要求 (99.9%? 99.95%? 99.99%?)
    - [ ] 确定 RTO/RPO 目标 (恢复时间/数据丢失容忍)
    - [ ] 制定变更管理流程和审批链
    - [ ] 建立应急响应联系人清单
    - [ ] 准备回滚计划

  infrastructure:
    - [ ] 所有服务器到位并完成硬件验收
    - [ ] 网络连通性验证 (节点间、节点到外网、VLAN 划分)
    - [ ] 存储系统性能基准测试 (IOPS/吞吐量)
    - [ ] DNS 解析配置完成
    - [ ] NTP 时间同步已配置
    - [ ] 证书已准备 (或使用 kubeadm 自签)

  security:
    - [ ] 确定认证方式 (OIDC/LDAP/静态 Token)
    - [ ] 网络安全策略已规划 (NetworkPolicy/防火墙规则)
    - [ ] 审计日志存储已规划
    - [ ] 加密方案已确定 (etcd 加密、TLS 通信)
```

---

<!-- chunk: 二、部署 HAProxy + Keepalived (API Server 高可用) -->## 二、部署 HAProxy + Keepalived (API Server 高可用)

> **为什么需要 LB？** 3 个 Master 各有一个 API Server，kubectl 和 kubelet 需要一个统一入口。  
> HAProxy 做 TCP 负载均衡 + Keepalived 做 VIP 浮动 = API Server 高可用。

## 2.1 在所有 Master 节点安装 HAProxy + Keepalived

```bash
# 在 master-1, master-2, master-3 上执行
sudo apt-get install -y haproxy keepalived
# CentOS: sudo yum install -y haproxy keepalived
```

## 2.2 配置 HAProxy

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在所有 Master 节点配置 (内容相同)
sudo tee /etc/haproxy/haproxy.cfg << 'EOF'
global
    log /dev/log local0
    log /dev/log local1 notice
    maxconn 4096                     # 最大连接数
    daemon

defaults
    log     global
    mode    tcp                      # TCP 模式 (L4 负载均衡)
    option  tcplog
    option  dontlognull
    timeout connect 5s               # 连接超时
    timeout client  30s              # 客户端超时
    timeout server  30s              # 服务端超时
    retries 3                        # 重试次数

# ===== K8s API Server 前端 =====
frontend k8s-api
    bind *:8443                      # 监听 8443 端口 (避免与 API Server 的 6443 冲突)
    default_backend k8s-api-backend

# ===== K8s API Server 后端 =====
backend k8s-api-backend
    balance roundrobin               # 轮询策略
    option tcp-check                 # TCP 健康检查
    server master-1 10.0.1.10:6443 check fall 3 rise 2 inter 5s   # fall=连续失败3次标记down
    server master-2 10.0.1.11:6443 check fall 3 rise 2 inter 5s   # rise=连续成功2次标记up
    server master-3 10.0.1.12:6443 check fall 3 rise 2 inter 5s   # inter=检查间隔5秒

# ===== 状态监控页面 (可选) =====
listen stats
    bind *:9000
    mode http
    stats enable
    stats uri /stats
    stats auth admin:haproxy123      # 访问密码
EOF

# 重启 HAProxy
sudo systemctl restart haproxy
sudo systemctl enable haproxy

# 验证
sudo systemctl status haproxy
# 预期: Active: active (running)

# 查看监控页面: http://10.0.1.10:9000/stats
```
## 2.3 配置 Keepalived (VIP 浮动)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ===== master-1 (MASTER 角色，优先级最高) =====
sudo tee /etc/keepalived/keepalived.conf << 'EOF'
vrrp_script check_haproxy {
    script "/usr/bin/killall -0 haproxy"   # 检查 haproxy 进程是否存在
    interval 2                              # 每 2 秒检查一次
    weight -20                              # 检查失败时降低优先级 20
}

vrrp_instance VI_1 {
    state MASTER                            # 主节点 (master-2/3 设为 BACKUP)
    interface eth0                          # 网卡名称 (根据实际情况修改，ip addr 查看)
    virtual_router_id 51                    # VRRP 组 ID (同组必须相同)
    priority 100                            # 优先级 (master-2=90, master-3=80)
    advert_int 1                            # 通告间隔 1 秒

    authentication {
        auth_type PASS
        auth_pass K8sHA!2024               # 认证密码 (所有节点必须相同)
    }

    virtual_ipaddress {
        10.0.1.100/24                      # VIP 地址 (kubectl 连接此地址)
    }

    track_script {
        check_haproxy                      # 关联 haproxy 健康检查
    }
}
EOF

# ===== master-2 (BACKUP 角色) =====
# 与 master-1 相同，但修改:
#   state BACKUP
#   priority 90

# ===== master-3 (BACKUP 角色) =====
# 与 master-1 相同，但修改:
#   state BACKUP
#   priority 80

# 启动 Keepalived (所有 Master 节点)
sudo systemctl restart keepalived
sudo systemctl enable keepalived

# 验证 VIP (在 master-1 上)
ip addr show eth0 | grep 10.0.1.100
# 预期: inet 10.0.1.100/24 scope global secondary eth0

# 验证 VIP 可访问
curl -k https://10.0.1.100:8443/healthz
# 预期: ok (API Server 还没部署前会连接失败，这是正常的)
```
---

<!-- chunk: 三、部署 HA 控制平面 (kubeadm) -->## 三、部署 HA 控制平面 (kubeadm)

## 3.1 所有节点: 系统准备

> 参考 [02-单节点部署 → 通用系统准备](./02-single-node-deployment.md)，在所有节点执行:
> swap 关闭、内核模块加载、网络参数配置、containerd 安装、kubeadm 安装、时间同步。

```bash
# 额外生产环境优化 (所有节点)
cat <<EOF | sudo tee /etc/sysctl.d/k8s-production.conf
# 文件描述符
fs.file-max = 1000000
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 1024

# 网络性能
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = bbr       # 启用 BBR 拥塞控制
net.ipv4.tcp_slow_start_after_idle = 0
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 600
net.core.netdev_max_backlog = 5000
net.core.somaxconn = 65535
net.ipv4.ip_local_port_range = 1024 65535

# conntrack
net.netfilter.nf_conntrack_max = 524288
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
EOF
sudo sysctl --system

# 增大文件描述符限制
cat <<EOF | sudo tee -a /etc/security/limits.conf
* soft nofile 655350
* hard nofile 655350
* soft nproc 655350
* hard nproc 655350
EOF
```

## 3.2 初始化第一个 Master

```yaml
# kubeadm-config-ha.yaml - 生产 HA 集群配置
cat > kubeadm-config-ha.yaml << 'EOF'
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.28.0
# ★ 关键: 指向 HAProxy VIP + 端口
controlPlaneEndpoint: "10.0.1.100:8443"

networking:
  podSubnet: "192.168.0.0/16"          # Calico 默认网段
  serviceSubnet: "10.96.0.0/12"
  dnsDomain: "cluster.local"

apiServer:
  certSANs:
  - "10.0.1.100"                        # HAProxy VIP
  - "10.0.1.10"                         # Master-1
  - "10.0.1.11"                         # Master-2
  - "10.0.1.12"                         # Master-3
  - "k8s-api.prod.example.com"          # 域名 (可选)
  - "127.0.0.1"
  - "localhost"
  extraArgs:
    # 认证相关
    anonymous-auth: "false"              # 禁止匿名访问
    authorization-mode: "Node,RBAC"      # 启用 Node + RBAC 授权
    enable-admission-plugins: "NodeRestriction,PodSecurity"
    # 审计日志
    audit-log-path: "/var/log/kubernetes/audit.log"
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"             # 单文件最大 100MB
    # 性能调优
    max-requests-inflight: "800"         # 最大并发请求 (默认 400)
    max-mutating-requests-inflight: "400"
    # etcd 相关
    etcd-compaction-interval: "5m"       # etcd 压缩间隔
  extraVolumes:
  - name: audit-log
    hostPath: /var/log/kubernetes
    mountPath: /var/log/kubernetes
    pathType: DirectoryOrCreate

controllerManager:
  extraArgs:
    bind-address: "0.0.0.0"
    terminated-pod-gc-threshold: "50"    # 终止 Pod 回收阈值
    node-monitor-period: "5s"            # 节点监控周期
    node-monitor-grace-period: "40s"     # 节点不响应宽限期

scheduler:
  extraArgs:
    bind-address: "0.0.0.0"

etcd:
  local:
    extraArgs:
      auto-compaction-retention: "8"     # etcd 自动压缩保留 8 小时
      quota-backend-bytes: "8589934592"  # etcd 存储配额 8GB
      snapshot-count: "10000"            # 快照触发的事务数

---
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
nodeRegistration:
  kubeletExtraArgs:
    rotate-certificates: "true"          # 自动轮转证书
EOF
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在 master-1 上初始化
sudo kubeadm init --config kubeadm-config-ha.yaml --upload-certs

# ★ --upload-certs 关键! 会将证书加密上传到集群中，其他 Master 加入时自动获取

# 预期输出 (记住以下关键信息):
# You can now join any number of the control-plane node running the following command on each as root:
#   kubeadm join 10.0.1.100:8443 --token xxxx \
#     --discovery-token-ca-cert-hash sha256:xxxx \
#     --control-plane --certificate-key xxxx
#              ↑ Master 加入命令 (带 --control-plane)
#
# Then you can join any number of worker nodes by running the following on each as root:
#   kubeadm join 10.0.1.100:8443 --token xxxx \
#     --discovery-token-ca-cert-hash sha256:xxxx
#              ↑ Worker 加入命令

# 配置 kubectl
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config

# 验证 (此时只有 1 个 Master，状态 NotReady 等 CNI)
kubectl get nodes
```
## 3.3 加入其他 Master 节点

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在 master-2 和 master-3 上执行 (使用 kubeadm init 输出的 control-plane join 命令)
sudo kubeadm join 10.0.1.100:8443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane \
  --certificate-key <certificate-key>

# 预期输出:
# This node has joined the cluster and a new control plane instance was created.

# 在每个新 Master 上配置 kubectl
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config
```
## 3.4 加入 Worker 节点

```bash
# 在每个 Worker 节点执行 (使用 kubeadm init 输出的 worker join 命令)
sudo kubeadm join 10.0.1.100:8443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash>
```

## 3.5 安装 CNI (Calico 生产配置)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Tigera Operator 安装 Calico (推荐的生产安装方式)
kubectl create -f https://raw.githubusercontent.com/projectcalico/calico/v3.26.0/manifests/tigera-operator.yaml

# 创建 Calico 自定义资源
cat <<EOF | kubectl apply -f -
apiVersion: operator.tigera.io/v1
kind: Installation
metadata:
  name: default
spec:
  calicoNetwork:
    bgp: Disabled                        # 非 BGP 场景禁用 (VXLAN 模式)
    ipPools:
    - blockSize: 26                      # 每个节点分配 /26 = 64 个 Pod IP
      cidr: 192.168.0.0/16              # 与 kubeadm podSubnet 一致
      encapsulation: VXLANCrossSubnet    # 跨子网使用 VXLAN，同子网直接路由
      natOutgoing: Enabled
    nodeAddressAutodetectionV4:
      interface: eth0                    # 指定网卡 (根据实际修改)
EOF

# 等待 Calico 就绪
kubectl get pods -n calico-system -w
# 预期: calico-node-xxx (每个节点一个), calico-typha-xxx, calico-kube-controllers 都是 Running

# 验证所有节点 Ready
kubectl get nodes
# 预期:
# NAME       STATUS   ROLES           AGE   VERSION
# master-1   Ready    control-plane   10m   v1.28.0
# master-2   Ready    control-plane   8m    v1.28.0
# master-3   Ready    control-plane   8m    v1.28.0
# worker-1   Ready    <none>          5m    v1.28.0
# worker-2   Ready    <none>          5m    v1.28.0
# worker-3   Ready    <none>          5m    v1.28.0

# 验证 etcd 集群健康
sudo etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health --cluster
# 预期: 3 个 endpoint 都是 healthy

# 验证 HA: VIP 可用
curl -k https://10.0.1.100:8443/healthz
# 预期: ok
```
---

<!-- chunk: 四、安全合规部署 -->## 四、安全合规部署

## 4.1 零信任网络策略

```yaml
# 生产环境: 默认拒绝所有流量，按需开放
# default-deny.yaml

# --- 默认拒绝所有 Ingress 和 Egress ---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production         # 每个生产 namespace 都要应用
spec:
  podSelector: {}               # 匹配所有 Pod
  policyTypes:
  - Ingress
  - Egress

---
# --- 允许 DNS 查询 (否则 Pod 无法解析域名) ---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53

---
# --- 允许特定应用间通信 ---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
```

## 4.2 RBAC 生产策略

```yaml
# 生产环境 RBAC: 最小权限原则
# --- 只读查看者 (运维值班人员) ---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: prod-viewer
rules:
- apiGroups: ["", "apps", "batch", "networking.k8s.io"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]

---
# --- 命名空间管理员 (应用团队负责人) ---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: namespace-admin
rules:
- apiGroups: ["", "apps", "batch", "networking.k8s.io", "autoscaling"]
  resources: ["deployments", "services", "configmaps", "secrets", "ingresses",
              "horizontalpodautoscalers", "jobs", "cronjobs", "pods"]
  verbs: ["*"]
- apiGroups: [""]
  resources: ["pods/log", "pods/exec"]
  verbs: ["get", "create"]
# 注意: 不包含 nodes, namespaces, clusterroles 等集群级资源

---
# --- 绑定到 OIDC 组 ---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: production-admin-binding
  namespace: production
subjects:
- kind: Group
  name: "prod-admins"           # OIDC/LDAP 组
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: namespace-admin
  apiGroup: rbac.authorization.k8s.io
```

## 4.3 Pod 安全标准 (Pod Security Standards)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 对生产 namespace 启用 restricted 安全级别
kubectl label namespace production \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 这意味着该 namespace 中的 Pod 必须:
# - 以非 root 用户运行 (runAsNonRoot: true)
# - 禁止特权容器 (privileged: false)
# - 禁止提权 (allowPrivilegeEscalation: false)
# - 只能使用受限的 capabilities
# - 使用只读根文件系统 (readOnlyRootFilesystem: true)
```
## 4.4 审计日志策略

```yaml
# audit-policy.yaml - 放在 /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 不记录健康检查和只读请求
- level: None
  resources:
  - group: ""
    resources: ["events"]
- level: None
  users: ["system:kube-proxy"]
  verbs: ["watch"]

# 记录 Secret 的所有操作 (不记录内容，只记录元数据)
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets"]

# 记录所有写操作的请求体
- level: Request
  verbs: ["create", "update", "patch", "delete"]
  resources:
  - group: ""
    resources: ["pods", "services", "configmaps"]
  - group: "apps"
    resources: ["deployments", "statefulsets", "daemonsets"]

# 其他请求记录元数据
- level: Metadata
  omitStages:
  - RequestReceived
```

---

<!-- chunk: 五、生产级 Deployment 模板 -->## 五、生产级 Deployment 模板

## 5.1 完整的 Web 应用 Deployment

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-application
  namespace: production
  labels:
    app: web-application
    version: v1.2.3
    tier: frontend
    env: production
  annotations:
    deployment.kubernetes.io/revision: "1"
spec:
  replicas: 6                           # 生产环境至少 3 副本
  revisionHistoryLimit: 10              # 保留 10 个历史版本 (用于回滚)
  progressDeadlineSeconds: 600          # 部署超时 10 分钟

  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2                       # 滚动更新时最多多出 2 个 Pod
      maxUnavailable: 1                 # 滚动更新时最多不可用 1 个 Pod

  selector:
    matchLabels:
      app: web-application

  template:
    metadata:
      labels:
        app: web-application
        version: v1.2.3
      annotations:
        prometheus.io/scrape: "true"    # 允许 Prometheus 采集指标
        prometheus.io/port: "9090"      # 指标端口

    spec:
      serviceAccountName: web-app-sa    # 使用专用 ServiceAccount (不要用 default)
      automountServiceAccountToken: false  # 不自动挂载 SA Token (除非需要)

      # ===== 拓扑分布: 跨可用区均匀分布 =====
      topologySpreadConstraints:
      - maxSkew: 1                      # 最大偏差 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule  # 不满足时不调度 (严格)
        labelSelector:
          matchLabels:
            app: web-application

      # ===== 反亲和: 不要调度到同一节点 =====
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values: ["web-application"]
            topologyKey: kubernetes.io/hostname

      containers:
      - name: web-app
        # ★ 生产必须使用 sha256 digest，不要用 tag (防止镜像被篡改)
        image: registry.prod.local/web-app:v1.2.3@sha256:abcdef123456...
        imagePullPolicy: IfNotPresent

        ports:
        - name: http
          containerPort: 8080
        - name: metrics
          containerPort: 9090

        # ===== 资源限制 (生产必须设置) =====
        resources:
          requests:                     # 调度依据，保证能获得的最小资源
            cpu: "200m"
            memory: "256Mi"
          limits:                       # 上限，超过 CPU 会限流，超过内存会 OOM Kill
            cpu: "1"
            memory: "1Gi"

        # ===== 存活探针: 检测应用是否还活着 =====
        livenessProbe:
          httpGet:
            path: /health               # 健康检查路径
            port: 8080
          initialDelaySeconds: 60       # 启动后 60 秒开始检查 (给应用启动时间)
          periodSeconds: 10             # 每 10 秒检查一次
          timeoutSeconds: 5             # 超时 5 秒
          failureThreshold: 3           # 连续失败 3 次标记为不健康 → 重启容器

        # ===== 就绪探针: 检测应用是否准备好接收流量 =====
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10       # 启动后 10 秒开始检查
          periodSeconds: 5              # 每 5 秒检查一次
          failureThreshold: 3           # 连续失败 3 次从 Service Endpoints 移除

        # ===== 启动探针: 保护慢启动应用 =====
        startupProbe:
          httpGet:
            path: /startup
            port: 8080
          failureThreshold: 60          # 最多等 60 × 5s = 300s (5 分钟)
          periodSeconds: 5
          # 启动探针成功前，liveness 和 readiness 不会生效

        # ===== 安全上下文 (生产必须) =====
        securityContext:
          allowPrivilegeEscalation: false  # 禁止提权
          readOnlyRootFilesystem: true     # 只读根文件系统
          runAsNonRoot: true               # 以非 root 运行
          runAsUser: 1000                  # 指定 UID
          capabilities:
            drop: ["ALL"]                  # 删除所有 Linux capabilities

        volumeMounts:
        - name: tmp
          mountPath: /tmp                  # 需要写入的临时目录

      volumes:
      - name: tmp
        emptyDir: {}                       # 临时卷 (Pod 删除后消失)

```

## 5.2 PodDisruptionBudget (PDB)

> **PDB 是什么？** 保证在维护操作 (如 `kubectl drain`) 时，始终保持最低可用副本数。

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
  namespace: production
spec:
  minAvailable: "60%"              # 至少保持 60% 的 Pod 可用
  # 或者: maxUnavailable: 1       # 最多允许 1 个 Pod 不可用
  selector:
    matchLabels:
      app: web-application
```

---

<!-- chunk: 六、监控告警体系 -->## 六、监控告警体系

## 6.1 Prometheus + Grafana 生产部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 生产环境使用更详细的配置
helm install monitoring prometheus-community/kube-prometheus-stack \
  --namespace monitoring --create-namespace \
  --set prometheus.prometheusSpec.retention=30d \
  --set prometheus.prometheusSpec.retentionSize=80GB \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
  --set prometheus.prometheusSpec.replicas=2 \
  --set prometheus.prometheusSpec.replicaExternalLabelName=prometheus_replica \
  --set alertmanager.alertmanagerSpec.replicas=3 \
  --set alertmanager.alertmanagerSpec.storage.volumeClaimTemplate.spec.resources.requests.storage=10Gi \
  --set grafana.persistence.enabled=true \
  --set grafana.persistence.size=20Gi \
  --set grafana.adminPassword='<strong-password>'

# 参数说明:
# retention=30d         - 指标保留 30 天
# retentionSize=80GB    - 存储上限 80GB (先达到的条件生效)
# replicas=2            - Prometheus 双副本 (高可用)
# alertmanager replicas=3 - Alertmanager 3 副本
```
## 6.2 关键告警规则

```yaml
# critical-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: critical-alerts
  namespace: monitoring
spec:
  groups:
  - name: cluster-health
    rules:
    # ===== 节点宕机 =====
    - alert: NodeDown
      expr: up{job="node-exporter"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "节点 {{ $labels.instance }} 宕机"
        description: "节点 {{ $labels.instance }} 已离线超过 2 分钟"

    # ===== API Server 不可用 =====
    - alert: APIServerDown
      expr: up{job="apiserver"} == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "API Server {{ $labels.instance }} 不可用"

    # ===== etcd 集群成员不足 =====
    - alert: EtcdMembersDown
      expr: count(etcd_server_has_leader{job="kube-etcd"}) < 3
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "etcd 集群成员不足 (当前 {{ $value }} 个)"

    # ===== Pod CrashLooping =====
    - alert: PodCrashLooping
      expr: increase(kube_pod_container_status_restarts_total[10m]) > 5
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"

    # ===== 磁盘即将满 =====
    - alert: DiskSpaceLow
      expr: (node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) < 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} 根分区剩余空间不足 10%"

    # ===== Deployment 副本不匹配 =====
    - alert: DeploymentReplicasMismatch
      expr: kube_deployment_status_replicas_available != kube_deployment_spec_replicas
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 可用副本数与期望不匹配"
```

---

<!-- chunk: 七、备份灾备策略 -->## 七、备份灾备策略

## 7.1 etcd 自动备份

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# /usr/local/bin/etcd-backup.sh - etcd 定时备份脚本
set -euo pipefail

BACKUP_DIR="/backup/etcd"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SNAPSHOT_NAME="etcd-snapshot-${TIMESTAMP}.db"
RETENTION_DAYS=30

# 创建备份目录
mkdir -p ${BACKUP_DIR}

# 创建快照
ETCDCTL_API=3 etcdctl snapshot save ${BACKUP_DIR}/${SNAPSHOT_NAME} \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证快照完整性
ETCDCTL_API=3 etcdctl snapshot status ${BACKUP_DIR}/${SNAPSHOT_NAME} --write-out=table

# 上传到远程存储 (可选)
# aws s3 cp ${BACKUP_DIR}/${SNAPSHOT_NAME} s3://backup-bucket/etcd/
# 或 rsync -avz ${BACKUP_DIR}/${SNAPSHOT_NAME} backup-server:/backup/etcd/

# 清理旧备份
find ${BACKUP_DIR} -name "etcd-snapshot-*.db" -mtime +${RETENTION_DAYS} -delete

echo "[$(date)] etcd 备份完成: ${SNAPSHOT_NAME}"
```
```bash
# 设置 cron 定时任务 (每天凌晨 2 点)
sudo chmod +x /usr/local/bin/etcd-backup.sh
echo "0 2 * * * /usr/local/bin/etcd-backup.sh >> /var/log/etcd-backup.log 2>&1" | sudo crontab -
```

## 7.2 Velero 集群备份

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 Velero CLI
# macOS: brew install velero
# Linux:
# wget https://github.com/vmware-tanzu/velero/releases/download/v1.12.0/velero-v1.12.0-linux-amd64.tar.gz
# tar -xvf velero-v1.12.0-linux-amd64.tar.gz
# sudo mv velero-v1.12.0-linux-amd64/velero /usr/local/bin/

# 安装到集群 (以 MinIO 为后端存储示例)
velero install \
  --provider aws \
  --plugins velero/velero-plugin-for-aws:v1.8.0 \
  --bucket velero-backups \
  --secret-file ./credentials-velero \
  --backup-location-config region=minio,s3ForcePathStyle=true,s3Url=http://minio.storage:9000 \
  --use-volume-snapshots=false

# 创建定时备份计划
velero schedule create daily-backup \
  --schedule="0 2 * * *" \
  --ttl 168h \
  --include-namespaces production,staging \
  --exclude-resources events
# 参数说明:
# schedule       - cron 表达式 (每天凌晨 2 点)
# ttl            - 备份保留 168 小时 (7 天)
# include        - 只备份指定 namespace
# exclude        - 排除 events (占空间且无用)

# 查看备份
velero backup get
velero backup describe daily-backup-xxxx

# 恢复
velero restore create --from-backup daily-backup-xxxx
```
---

<!-- chunk: 八、证书管理与轮转 -->## 八、证书管理与轮转

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看证书过期时间
sudo kubeadm certs check-expiration
# 预期输出: 各组件证书的过期日期 (默认 1 年)

# 手动轮转所有证书
sudo kubeadm certs renew all

# 轮转后需要重启控制平面组件
sudo systemctl restart kubelet
# 或重启静态 Pod (移动 manifest 文件后放回):
# sudo mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
# sleep 5
# sudo mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/

# 自动轮转: kubelet 证书轮转 (kubeadm 默认启用)
# 检查: cat /var/lib/kubelet/config.yaml | grep rotateCertificates
# 应为: rotateCertificates: true
```
---

<!-- chunk: 九、集群升级流程 -->## 九、集群升级流程

> **生产升级原则**: 一次只升级一个小版本 (如 1.27 → 1.28)，不能跳版本。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# ===== 1. 升级前准备 =====
# 备份 etcd
/usr/local/bin/etcd-backup.sh

# 检查当前版本和可升级版本
kubectl get nodes
sudo kubeadm upgrade plan

# ===== 2. 升级第一个 Master =====
# 升级 kubeadm
sudo apt-get update
sudo apt-get install -y --allow-change-held-packages kubeadm=1.29.0-*
sudo kubeadm upgrade plan
sudo kubeadm upgrade apply v1.29.0

# 升级 kubelet 和 kubectl
sudo apt-get install -y --allow-change-held-packages kubelet=1.29.0-* kubectl=1.29.0-*
sudo systemctl daemon-reload
sudo systemctl restart kubelet

# ===== 3. 升级其他 Master =====
# (在 master-2 和 master-3 上)
sudo apt-get install -y --allow-change-held-packages kubeadm=1.29.0-*
sudo kubeadm upgrade node  # 注意: 不是 upgrade apply
sudo apt-get install -y --allow-change-held-packages kubelet=1.29.0-* kubectl=1.29.0-*
sudo systemctl daemon-reload && sudo systemctl restart kubelet

# ===== 4. 逐个升级 Worker =====
# 在 Master 上: 排空 Worker 节点
kubectl drain worker-1 --ignore-daemonsets --delete-emptydir-data

# 在 Worker 节点上: 升级
sudo apt-get install -y --allow-change-held-packages kubeadm=1.29.0-*
sudo kubeadm upgrade node
sudo apt-get install -y --allow-change-held-packages kubelet=1.29.0-*
sudo systemctl daemon-reload && sudo systemctl restart kubelet

# 在 Master 上: 恢复调度
kubectl uncordon worker-1

# 对 worker-2, worker-3 重复以上步骤

# ===== 5. 验证升级 =====
kubectl get nodes
# 预期: 所有节点版本为 v1.29.0
kubectl get pods -A | grep -v Running | grep -v Completed
# 预期: 无异常 Pod
```
---

<!-- chunk: 十、成本优化 -->## 十、成本优化

## 10.1 资源配额

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: production-quota
  namespace: production
spec:
  hard:
    requests.cpu: "40"
    requests.memory: 80Gi
    limits.cpu: "80"
    limits.memory: 160Gi
    pods: "200"
    persistentvolumeclaims: "50"
```

## 10.2 节点自动伸缩 (Cluster Autoscaler)

> **备注**: Cluster Autoscaler 主要适用于云环境，自建机房通常使用预规划容量。

```yaml
# 云环境节点组配置示例
nodeGroups:
  # 按需实例 (关键业务)
  - name: on-demand-group
    instanceType: "m5.2xlarge"
    minSize: 3
    maxSize: 20
    labels:
      lifecycle: on-demand
      workload: critical

  # Spot 实例 (非关键业务，成本优化)
  - name: spot-group
    instanceType: "m5.2xlarge"
    minSize: 0
    maxSize: 30
    spotInstance: true
    labels:
      lifecycle: spot
      workload: batch
    taints:
    - key: spot
      value: "true"
      effect: NoSchedule
```

---

<!-- chunk: 十一、架构评估指标 -->## 十一、架构评估指标

| 类别 | 指标 | 目标值 |
|------|------|--------|
| **可用性** | API Server SLI | ≥ 99.95% |
| **可用性** | MTBF (平均无问题时间) | > 720 小时 |
| **可用性** | MTTR (平均恢复时间) | < 30 分钟 |
| **性能** | API 响应 P99 | < 1 秒 |
| **性能** | Pod 调度延迟 | < 5 秒 |
| **性能** | 容器启动时间 | < 30 秒 |
| **安全** | 漏洞扫描覆盖率 | 100% |
| **安全** | CIS 基线合规率 | 100% |
| **备份** | etcd 备份频率 | 每小时 |
| **备份** | 恢复演练频率 | 每季度 |

---

<!-- chunk: 十二、实施检查清单 -->## 十二、实施检查清单

## 部署实施阶段
- [ ] HAProxy + Keepalived 部署完成，VIP 可用
- [ ] 3 个 Master 节点加入集群
- [ ] N 个 Worker 节点加入集群
- [ ] etcd 集群 3 节点健康
- [ ] Calico CNI 安装完成，跨节点 Pod 通信正常
- [ ] 所有系统 Pod Running

## 安全加固阶段
- [ ] NetworkPolicy 默认拒绝已应用
- [ ] RBAC 策略已配置
- [ ] Pod Security Standards 已启用
- [ ] 审计日志已配置
- [ ] 证书过期时间已确认

## 可观测性阶段
- [ ] Prometheus + Grafana 部署完成
- [ ] 关键告警规则已配置
- [ ] 告警通知渠道已测试
- [ ] 日志收集系统已部署

## 灾备保障阶段
- [ ] etcd 自动备份已配置并验证
- [ ] Velero 备份计划已创建
- [ ] 灾难恢复流程已文档化
- [ ] 恢复演练已执行

## 运营就绪阶段
- [ ] 集群升级流程已文档化
- [ ] 值班和应急响应流程已建立
- [ ] 容量规划已完成
- [ ] 性能基准测试已执行

---

**部署原则**: 零停机更新是底线，可观测性是保障，安全性是前提，成本优化是目标。

---

**来源文档**: `集群基础/24-production-deployment-best-practices.md`, `工作负载/02-deployment-production-patterns.md`, `集群基础/01-production-architecture-design-principles.md`, `集群基础/12-cluster-deployment-patterns.md`

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-deployment MOC
- [[11-发布变更/06-部署方案/README.md|Kubernetes 部署方案指南 (Deployment Guide)]]
- [[11-发布变更/06-部署方案/01-local-demo-deployment.md|01 - 本机单机 Demo 部署]]
- [[11-发布变更/06-部署方案/02-single-node-deployment.md|02 - 单节点部署 (Single Node All-in-One)]]
- [[11-发布变更/06-部署方案/03-development-environment-deployment.md|03 - 研发环境部署 (Development Environment Deployment)]]

## Related

- [[README|README]]
- [[MOC|MOC]]
- [[17-系统基础/05-速查卡/go.md|go]]
- [[17-系统基础/05-速查卡/helm.md|helm]]
- [[17-系统基础/05-速查卡/k8s.md|k8s]]

```

<!-- risk-assessed -->
