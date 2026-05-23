---
title: 节点组件故障排查 (skills)
description: '# 节点组件故障排查'
category: skills
tags:
- k8s
- troubleshooting
- structural
- node-components
- etcd
- apiserver
- kubelet
- containerd
- cri-o
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点组件故障排查 是什么
- 如何 节点组件故障排查
trigger_keywords:
- 节点组件故障排查
prerequisites:
- kubectl-basics
- etcd-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# 节点组件故障排查

### 01 Kubelet TroubleshootingUDIG 故障排查 Prompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断与止血

1. **节点面状态**：`kubectl get nodes -o wide`，抽样 `kubectl describe node <name>` 查看 Conditions/Taints，区分单点 vs 批量问题。
2. **kubelet 存活**：节点上执行 `curl -s localhost:10248/healthz`、`systemctl status kubelet`，若健康探针失败优先查证书/配置/资源。
3. **资源与压力**：`free -m`、`df -h`、`df -i`、`pidstat -p $(pgrep kubelet)`，确认 Memory/Disk/PID Pressure；若磁盘吃满先清理 `/var/lib/containerd` 旧镜像与日志。
4. **CRI 交互**：`crictl info`、`crictl ps -a | head`，若 CRI 超时则检查 containerd/Docker 服务、cgroup 驱动一致性（`cat /var/lib/kubelet/config.yaml | grep cgroupDriver`）。
5. **PLEG/驱逐信号**：`journalctl -u kubelet | grep -E "PLEG is not healthy|eviction" | tail`，辨别是运行时阻塞还是驱逐触发。
6. **快速缓解**：
   - 将问题节点 `cordon`，必要时 `drain --ignore-daemonsets --delete-emptydir-data`。
   - 重启运行时与 kubelet（确认已备份配置/证书），并检查 cgroup 驱动一致后再放行。
   - 若磁盘/内存压力，立即清理镜像/容器/日志或扩容磁盘，调整 `evictionHard`。
7. **证据留存**：保存 kubelet/CRI 关键日志、节点 Conditions、磁盘/PID/内存快照，便于复盘。

---

#### 2. 排查方法与步骤



#### 2.1 排查原理：分层模型与核心机制

kubelet 的稳定依赖于多个层面的健康，深入理解其内部机制是高效排查的关键：

#### 2.1.1 宿主机环境层
- **内核版本要求**：推荐 4.19+ 内核，过旧内核缺少关键特性（如 cgroup v2 支持）
- **cgroup 子系统**：kubelet 通过 cgroup 限制容器资源，检查 `/sys/fs/cgroup` 挂载状态
- **磁盘 IO**：kubelet 日志、容器层、etcd 数据共用磁盘，高 IO 负载会拖慢所有组件
- **网络栈**：节点网络不通会导致 kubelet 无法上报心跳，触发 NotReady
- **文件描述符**：每个容器消耗多个 fd（日志、挂载、socket），`ulimit -n` 需设置足够大（推荐 65535+）

#### 2.1.2 容器运行时接口层（CRI）
- **CRI 架构**：kubelet → CRI API (gRPC) → containerd/CRI-O/Docker shim
- **关键操作超时**：
  - `runtimeRequestTimeout`（默认 2m）：CRI 操作超时时间
  - 超时会导致 kubelet 标记 PLEG 不健康
- **cgroup 驱动一致性**：kubelet 和 CRI 必须使用相同驱动（systemd 或 cgroupfs）
  ```bash
  # 检查 kubelet cgroup 驱动
  grep cgroupDriver /var/lib/kubelet/config.yaml
  # 检查 containerd cgroup 驱动
  grep SystemdCgroup /etc/containerd/config.toml
  # 两者必须一致！
  ```
- **镜像管理**：kubelet 委托 CRI 拉取镜像，CRI 超时会阻塞 Pod 创建

#### 2.1.3 网络插件接口层（CNI）
- **CNI 调用时机**：Pod 创建时调用 CNI 插件配置网络（veth pair、路由、iptables）
- **配置路径**：`/etc/cni/net.d/` 和 `/opt/cni/bin/`
- **常见问题**：CNI 二进制缺失、配置错误、IP 池耗尽、网络插件 Pod 未就绪

#### 2.1.4 存储插件接口层（CSI）
- **卷挂载流程**：kubelet → CSI Plugin → 云厂商 API → 挂载到宿主机 → bind mount 到容器
- **挂载点泄露**：CSI 插件问题会导致挂载点僵死，kubelet 卡在清理阶段
- **检查命令**：`mount | grep kubernetes.io`

#### 2.1.5 配置与证书层
- **主配置文件**：`/var/lib/kubelet/config.yaml`（推荐）或启动参数
- **证书文件**：
  - `/var/lib/kubelet/pki/kubelet-client-current.pem`：kubelet 客户端证书
  - `/var/lib/kubelet/pki/kubelet.crt`：kubelet 服务端证书
- **证书轮转机制**：
  - `rotateCertificates: true`：启用自动轮转
  - kubelet 在证书到期前自动生成 CSR（CertificateSigningRequest）
  - Controlle
...(截断)

---

### 02 Kube Proxy Troubleshooting

#### 0. 10 分钟快速诊断

1. **组件存活与模式**：`kubectl -n kube-system get pods -l k8s-app=kube-proxy -o wide`；`kubectl -n kube-system get cm kube-proxy -o yaml | grep mode` 确认 iptables/IPVS。
2. **服务/后端核对**：`kubectl get svc <ns>/<svc> -o wide && kubectl get endpoints <ns>/<svc>`，确认 Endpoints 是否为空或不均衡。
3. **数据面规则**：
   - iptables：`iptables-save | grep KUBE-SERVICES | wc -l`，`iptables -t nat -L KUBE-SERVICES -n | head`。
   - IPVS：`ipvsadm -Ln --stats | head`，关注缺失/未同步的虚拟服务。
4. **conntrack/内核健康**：`conntrack -S` 观察表使用率；`dmesg | grep conntrack | tail`；`sysctl net.netfilter.nf_conntrack_max`。
5. **NodePort/外访**：若 NodePort 不通，检查宿主机防火墙/云安全组；`nc -zv <node> <nodePort>` 与 `tcpdump -i eth0 port <nodePort>`。
6. **快速缓解**：
   - 单节点问题：重启该节点 kube-proxy Pod；若规则缺失，删除 Pod 触发重建规则。
   - 大规模性能：切换 IPVS、开启 `strictARP`，调高 conntrack 表并开启连接回收参数；限制 Service 爆炸增长。
   - Endpoints 空：修复上游工作负载或健康检查，避免空代理。
7. **证据留存**：保存规则导出、kube-proxy 日志、conntrack 统计、问题节点的 iptables/ipvs 快照。

---

#### 排查方法与步骤（高级工作流）



#### 排查模型：三级跳

1. **Control Plane**：确认 API Server 中 Service 和 Endpoints 是否正确对齐。
2. **Data Plane (Kernel)**：确认内核规则（iptables/IPVS）是否已生成并正确映射到 Endpoints。
3. **Environment Layer**：确认内核参数（conntrack_max）、宿主机防火墙、CNI 网络连通性。

---

### 03 Container Runtime Troubleshooting

#### 0. 10 分钟快速诊断

1. **服务与 Socket**：`systemctl status containerd`（或 dockerd/CRI-O）；`ls -l /run/containerd/containerd.sock`，若不存在或权限拒绝先处理服务启动。
2. **快速复现**：`crictl ps -a | head`、`crictl info`，若超时则查看 `journalctl -u containerd --since 5m` 错误。
3. **磁盘/inode**：`df -h /var/lib/containerd /var/lib/docker`、`df -i`，确认空间/ inode；必要时 `crictl images | wc -l` 评估清理。
4. **镜像/拉取链路**：`crictl pull <image>` 复现，观察 TLS/认证/429 错误；检查 `/etc/containerd/config.toml` registry mirror 与限速设置。
5. **Overlay/挂载**：`mount | grep overlay | head`，若报错 `invalid argument` 需检查内核版本与层级限制；`dmesg | tail`。
6. **快速缓解**：
   - 空间不足：`crictl rmi --prune`、清理未用 snapshots/logs。
   - 服务卡死：重启 containerd/kubelet（先 cordon 节点），确认 cgroup 驱动一致。
   - 拉取受限：切换最近的镜像镜像源/私有 cache，开启镜像预热或 P2P。
7. **证据留存**：保留 containerd 日志、`crictl info` 输出、磁盘/挂载快照、失败的 `crictl pull` 错误信息。

---

#### 排查方法与步骤



#### 2.1 排查逻辑：剥洋葱法

1. **接口层**：`crictl info` 是否能通？
2. **进程层**：`containerd-shim` 和 `runc` 是否正常？
3. **内核层**：`dmesg` 是否有 OOM 或文件系统报错？
4. **资源层**：Inode、磁盘空间、PID 限制是否触达？

---

### 04 Node Troubleshooting

#### 0. 10 分钟快速诊断

1. **确认影响面**：`kubectl get nodes -o wide`，统计 NotReady/Unknown 节点比例，区分单点 vs 批量。
2. **抽样深描**：对 1-2 个异常节点执行 `kubectl describe node <name>`，关注 Conditions、Taints、Allocatable/Capacity、近期事件（心跳超时/驱逐）。
3. **资源与压力**：登陆节点 `free -m`、`df -h`、`df -i`、`pidstat -p $(pgrep kubelet)`，查 Memory/Disk/PIDPressure；`dmesg | tail` 识别硬件/IO 报错。
4. **网络连通**：节点到 API Server `curl -k https://$APISERVER:6443/healthz`，检查安全组/防火墙/路由；批量抖动时考虑上游网络分区。
5. **驱逐/维护状态**：确认是否被 `cordon`/`drain` 或自动污点；检查 `GracefulNodeShutdown`（v1.26+）和 PodDisruptionConditions。
6. **快速缓解**：
   - 单节点异常：`cordon` 并修复资源/网络/磁盘，必要时换机或迁移工作负载。
   - 批量波动：降低驱逐速率（调整 Node Controller 参数），暂停大规模变更，优先恢复网络/APIServer。
   - 污染/幽灵节点：清理失联节点 (`kubectl delete node <name>`) 前先确认无跑动 Pod。
7. **证据留存**：记录 describe 输出、Conditions/Taints 快照、系统日志、网络探测结果，便于复盘。

#### 排查方法与步骤



#### 2.1 排查决策树

```
节点问题
    │
    ├─── 节点 NotReady？
    │         │
    │         ├─ kubelet 状态 ──→ systemctl status kubelet
    │         ├─ 容器运行时 ──→ systemctl status containerd
    │         ├─ 网络问题 ──→ 检查节点网络连通性
    │         └─ 资源压力 ──→ 检查 Conditions
    │
    ├─── 资源压力？
    │         │
    │         ├─ MemoryPressure ──→ 检查内存使用/OOM
    │         ├─ DiskPressure ──→ 检查磁盘/inode
    │         └─ PIDPressure ──→ 检查进程数
    │
    ├─── Pod 无法调度？
    │         │
    │         ├─ 污点问题 ──→ 检查节点污点和 Pod 容忍
    │         ├─ 亲和性问题 ──→ 检查节点标签和亲和性规则
    │         ├─ 资源不足 ──→ 检查可用资源
    │         └─ 拓扑约束 ──→ 检查 topologySpreadConstraints
    │
    └─── Pod 被驱逐？
              │
              ├─ 优先级 ──→ 检查 PriorityClass
              ├─ QoS 类别 ──→ 检查资源配置
              └─ 驱逐策略 ──→ 检查 kubelet 配置
```

---

### 05 Image Registry Troubleshooting

#### 0. 10 分钟快速诊断

1. **快速定位失败**：在问题 Pod 上 `kubectl describe pod <name> | grep -A2 -E "Image|ErrImage|BackOff|429|unauthorized"`，记录错误码（DNS/TLS/401/429/空间）。
2. **连通性与 TLS**：`nslookup <registry>`、`curl -Iv https://<registry>/v2/`，若证书错误检查 CA/中间证书；云私有域注意 443/5000 安全组。
3. **认证与凭据**：`crictl pull <image> --creds user:pass` 验证，检查 `imagePullSecrets`、SA 绑定；`cat ~/.docker/config.json` 或 `/etc/containerd/config.toml` registry 配置。
4. **速率与并发**：观察 `toomanyrequests`/`rate limit exceeded`，临时切换私有镜像缓存/镜像加速器，或降低批量创建并开启预拉取。
5. **磁盘与缓存**：`df -h /var/lib/containerd`、`df -i`，空间/ inode 不足会导致拉取中断；必要时 `crictl rmi --prune` 清理未用镜像。
6. **镜像一致性**：确认是否使用 Digest（SHA256）而非可变 Tag；比对多架构 Manifest，避免架构不匹配导致 `exec format error`。
7. **证据留存**：保存 describe 输出、crictl pull 错误、TLS/CA 信息、registry 配置与监控（拉取时延/429 次数），便于复盘。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
镜像拉取问题
      │
      ├─── ImagePullBackOff / ErrImagePull？
      │         │
      │         ├─ "not found" / "manifest unknown"
      │         │       └─► 检查镜像名称、标签是否正确
      │         │
      │         ├─ "unauthorized" / "authentication required"
      │         │       └─► 检查 imagePullSecrets 配置
      │         │
      │         ├─ "connection refused" / "timeout"
      │         │       └─► 检查网络连通性、DNS、防火墙
      │         │
      │         ├─ "x509: certificate" 错误
      │         │       └─► 检查 TLS 证书配置
      │         │
      │         └─ "toomanyrequests"
      │                 └─► 速率限制，使用镜像代理或认证
      │
      ├─── 镜像版本不对？
      │         │
      │         └─► 检查 imagePullPolicy 和标签
      │
      └─── 架构不匹配？
                │
                └─► 检查镜像支持的平台 (amd64/arm64)
```

---

### 06 Gpu Device Plugin Troubleshooting

#### 0. 10 分钟快速诊断

1. **组件与资源可见性**：`kubectl -n kube-system get ds -l name=nvidia-device-plugin-daemonset -o wide`（或对应厂商插件）；`kubectl get node <name> -o jsonpath='{.status.allocatable.nvidia\.com/gpu}'`。
2. **驱动健康**：节点执行 `nvidia-smi`，若报错检查驱动/XID；`dmesg | grep -i nvidia | tail`。
3. **Pod 事件**：对 Pending/失败的 GPU Pod `kubectl describe pod`，查看调度原因（资源不足/拓扑/亲和性）或启动错误（挂载/环境变量缺失）。
4. **插件日志与注册**：`kubectl logs -n kube-system ds/nvidia-device-plugin-daemonset -c nvidia-device-plugin-ctr --tail=50`，确认 `ListAndWatch`/`Allocate` 是否报错；查看 `/var/lib/kubelet/device-plugins/kubelet_internal_checkpoint`。
5. **MIG/时间片/NUMA**：检查是否开启 MIG，规格是否匹配；时间片共享需插件版本 ≥0.13；跨 NUMA 部署可需 `TopologyManager` 设置。
6. **快速缓解**：
   - 单节点异常：`cordon` 节点，重载驱动或重启插件 DaemonSet；若 XID 持续，重启机器或下架 GPU。
   - 资源碎片：执行排空重调度，或调整请求规格/关闭 MIG 分片以释放连续资源。
   - 配置错误：回滚自定义插件镜像/参数，恢复官方默认 DaemonSet。
7. **证据留存**：保存插件日志、`nvidia-smi` 输出、Pod 事件、Node allocatable/已分配快照、XID 代码及 dmesg 片段。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
GPU/设备 Pod 问题
        │
        ▼
┌───────────────────┐
│  Pod 状态是什么？  │
└───────────────────┘
        │
        ├── Pending ──────────────────────────────────────┐
        │                                                  │
        │   ┌─────────────────────────────────────────┐   │
        │   │ 检查调度事件                            │   │
        │   │ kubectl describe pod <pod>              │   │
        │   └─────────────────────────────────────────┘   │
        │                  │                               │
        │                  ▼                               │
        │   ┌─────────────────────────────────────────┐   │
        │   │ Insufficient nvidia.com/gpu?            │   │
        │   └─────────────────────────────────────────┘   │
        │          │                │                      │
        │         是               否                      │
        │          │                │                      │
        │          ▼                ▼                      │
        │   ┌────────────┐   ┌────────────────┐           │
        │   │ 检查 Node  │   │ 检查其他资源   │           │
        │   │ GPU 容量   │   │ 或 affinity    │           │
        │   └────────────┘   └────────────────┘           │
        │          │                                       │
        │          ▼                                       │
        │   ┌─────────────────────────────────────────┐   │
        │   │ Node 有 GPU Capacity?                   │   │
        │   └─────────
...(截断)

## 相关链接

- [[skills/troubleshoot-node-issues.md|节点故障排查]]
- [[skills/monitor-kubernetes-metrics.md|K8s 监控指标]]

## Related

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
