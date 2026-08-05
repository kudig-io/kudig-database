---
title: 06 - Terway 性能调优 (Performance Tuning)
description: '## 1. 网络模式性能对比'
summary: 'kubectl get node <node-name> -o jsonpath='{.status.allocatable.pods}''
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- kubelet
- prometheus
- cilium
- daemonset
- networkpolicy
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 10min
intent_queries:
- Terway 性能调优 (Performance Tuning) 是什么
- 如何 Terway 性能调优 (Performance Tuning)
trigger_keywords:
- Terway
- 性能调优
- Performance
- Tuning
- terway
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 06 - Terway 性能调优 (Performance Tuning)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

## 1. 网络模式性能对比

### 1.1 吞吐量与延迟基准

| 模式 | 吞吐量 (Gbps) | P99 延迟 | 连接数/秒 | 适用场景 |
|:---|:---:|:---:|:---:|:---|
| VPC 路由 | 5-8 | ~200µs | 中 (~5 万) | 兼容性优先、旧集群迁移、跨 VPC 场景 |
| ENI 独占 | 20-40 | ~50µs | 最高 (>20 万) | 核心数据库、网关、低延迟交易系统 |
| ENIIP | 15-25 | ~80µs | 高 (~15 万) | **推荐通用**、微服务、Web 应用 |
| IPVlan | 18-30 | ~60µs | 高 (~18 万) | 极致性能、AI/ML 训练、高吞吐计算 |

> **注意**: 实际性能取决于 ECS 实例规格、网络配置和内核版本。上表数据基于 ecs.g7.4xlarge 在同可用区 iperf3 测试得出。

### 1.2 模式选型决策矩阵

| 维度 | VPC 路由 | ENI 独占 | ENIIP | IPVlan |
|:---|:---:|:---:|:---:|:---:|
| 性能 | 低 | 最高 | 中高 | 高 |
| Pod 密度 | 最高 | 最低 | 高 | 高 |
| 配置复杂度 | 低 | 低 | 低 | 中 |
| 内核要求 | 任意 | 任意 | 任意 | 4.19+ |
| [[NetworkPolicy|NetworkPolicy]] | iptables | eBPF/iptables | eBPF/iptables | eBPF |
| 网络开销 | veth + 路由 | 无 | veth | 无 |
| 推荐度 | 兼容场景 | 极致性能 | **首选** | 高性能 |

---

## 2. Pod 容量计算

### 2.1 ENIIP 模式容量表

计算公式: **Pod 数 = (ENI 数 - 1) x 单 ENI 辅助 IP 数**

> 减 1 是因为主 ENI (eth0) 保留给节点自身使用。

| ECS 规格 | vCPU | 内存 (GiB) | ENI 数 | 单 ENI IP 数 | ENIIP Pod 数 | 计算过程 |
|:---|:---:|:---:|:---:|:---:|:---:|:---|
| ecs.g7.large | 2 | 8 | 3 | 6 | **12** | (3-1) x 6 |
| ecs.g7.xlarge | 4 | 16 | 4 | 10 | **30** | (4-1) x 10 |
| ecs.g7.2xlarge | 8 | 32 | 6 | 15 | **75** | (6-1) x 15 |
| ecs.g7.4xlarge | 16 | 64 | 8 | 30 | **210** | (8-1) x 30 |
| ecs.g7.8xlarge | 32 | 128 | 16 | 30 | **450** | (16-1) x 30 |

> 完整容量速查表见 [01-product.md 第 7 节](./01-product.md#7-ecs-实例规格-eni-限制速查)。

### 2.2 ENI 独占模式容量表

| ECS 规格 | ENI 数 | ENI 独占 Pod 数 | 说明 |
|:---|:---:|:---:|:---|
| ecs.g7.large | 3 | **2** | 扣除主 ENI |
| ecs.g7.xlarge | 4 | **3** | |
| ecs.g7.2xlarge | 6 | **5** | |
| ecs.g7.4xlarge | 8 | **7** | |
| ecs.g7.8xlarge | 16 | **15** | |

### 2.3 查询节点实际容量

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node <node-name> -o jsonpath='{.status.allocatable.pods}'
kubectl describe node <node-name> | grep -A 5 "Capacity"
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node <node-name> -o jsonpath='{.status.capacity}' | jq .
```
> 详细容量规划参考: [domain-03-networking-traffic/05-terway-advanced-guide.md](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/01-terway-advanced-guide.md)

---

## 3. 内核与系统调优

### 3.1 网卡多队列 (Multi-Queue)

网卡多队列将网络中断分散到多个 CPU 核心处理，避免单核瓶颈。

```bash
ethtool -l eth0
```

```bash
ethtool -L eth0 combined 8
```

建议值: `combined` 设为 **vCPU 数 / 2** (不超过网卡支持的最大队列数)。

### 3.2 内核网络参数

通过 init container 或 [[DaemonSet|DaemonSet]] 在节点上持久化以下参数:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
sysctl -w net.core.somaxconn=65535
sysctl -w net.ipv4.tcp_max_syn_backlog=65535
sysctl -w net.core.netdev_max_backlog=65535
sysctl -w net.core.rmem_max=16777216
sysctl -w net.core.wmem_max=16777216
sysctl -w net.ipv4.tcp_rmem="4096 87380 16777216"
sysctl -w net.ipv4.tcp_wmem="4096 65536 16777216"
sysctl -w net.ipv4.tcp_tw_reuse=1
sysctl -w net.ipv4.ip_local_port_range="1024 65535"
sysctl -w net.ipv4.tcp_fin_timeout=15
sysctl -w net.ipv4.tcp_keepalive_time=600
sysctl -w net.ipv4.tcp_keepalive_intvl=30
sysctl -w net.ipv4.tcp_keepalive_probes=3
sysctl -w net.ipv4.tcp_slow_start_after_idle=0
sysctl -w net.ipv4.tcp_no_metrics_save=1
sysctl -w net.ipv4.tcp_syncookies=1
```

以 DaemonSet 方式持久化:

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: sysctl-tuner
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: sysctl-tuner
  template:
    metadata:
      labels:
        app: sysctl-tuner
    spec:
      hostPID: true
      hostNetwork: true
      initContainers:
      - name: sysctl
        image: busybox:1.36
        securityContext:
          privileged: true
        command:
        - sh
        - -c
        - |
          sysctl -w net.core.somaxconn=65535
          sysctl -w net.ipv4.tcp_max_syn_backlog=65535
          sysctl -w net.core.netdev_max_backlog=65535
          sysctl -w net.ipv4.tcp_tw_reuse=1
          sysctl -w net.ipv4.ip_local_port_range="1024 65535"
      containers:
      - name: pause
        image: registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9
```

### 3.3 IPVlan 模式优化

IPVlan L2 模式绕过 veth pair，减少一次上下文切换，显著降低延迟:

```
传统 ENIIP 数据路径:
  Pod → veth pair (netns 切换) → 宿主 netns → ENI 辅助 IP → VPC 交换机
  |<---------- 用户空间/内核空间切换 ---------->|

IPVlan L2 数据路径:
  Pod → IPVlan 子接口 (共享 netns) → ENI → VPC 交换机
  |<------ 无额外上下文切换，直接内核转发 ------>|
```

要求:
- 内核 4.19+ (推荐 5.10+)
- Terway v1.3+
- ENI 多 IP 功能已开启

### 3.4 NUMA 感知 (大规格实例)

16+ vCPU 的大规格实例通常为多 NUMA 架构，跨 NUMA 访问网络中断会显著增加延迟。

```bash
lscpu | grep "NUMA node"
numactl --hardware
```

配置 IRQ 亲和性，将网卡中断绑定到 ENI 所在的 NUMA 节点:

```bash
cat /proc/interrupts | grep eth
```

```bash
for irq in $(cat /proc/interrupts | grep eth0 | awk -F: '{print $1}'); do
  echo $((numa_node_mask)) > /proc/irq/$irq/smp_affinity
done
```

使用 `irqbalance` 服务自动管理 (推荐):

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
systemctl enable irqbalance
systemctl start irqbalance
```
[[Kubernetes|Kubernetes]] NUMA 感知调度 (需要开启 Topology Manager):

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
topologyManagerPolicy: best-effort
cpuManagerPolicy: static
```

---

## 4. 大规格实例优化

### 4.1 实例规格选择指南

| 工作负载类型 | 推荐规格 | 推荐模式 | 理由 |
|:---|:---|:---|:---|
| Web/API 服务 | ecs.g7.xlarge - 2xlarge | ENIIP | 性能充足，密度合适 |
| 核心数据库 | ecs.g7.4xlarge+ | ENI 独占 | 最低延迟，最高吞吐 |
| 高密度微服务 | ecs.g7.8xlarge+ | ENIIP / Trunking | 最大 Pod 密度 |
| 网关/代理 | ecs.g7.4xlarge+ | ENI 独占 / IPVlan | 高连接数，低延迟 |
| AI/ML 训练 | ecs.gn7i-c16g1.4xlarge+ | IPVlan | RDMA 支持，高吞吐 |
| 流媒体/CDN | ecs.g7.8xlarge+ | IPVlan / ENI 独占 | 高带宽需求 |
| 批处理任务 | ecs.g7.2xlarge - 4xlarge | ENIIP | 性价比优先 |

### 4.2 Trunking 模式 (高密度场景)

对于 ecs.g7.8xlarge 及以上规格，可启用 ENI Trunking 模式进一步扩展 Pod 密度:

```json
{
  "version": "1",
  "network_type": "ENIIP",
  "vswitches": {"cn-hangzhou-b": ["vsw-xxx"]},
  "security_group": "sg-xxx",
  "enable_eni_trunking": true
}
```

Trunking 模式通过 VLAN 子接口复用 ENI，单节点 Pod 密度可达 **500+**。

---

## 5. IP 分配性能优化

### 5.1 IP 池预热

Terway 支持预分配 IP 到本地池，减少 Pod 创建时的 OpenAPI 调用延迟。

关键参数:

| 参数 | 说明 | 建议值 |
|:---|:---|:---|
| `ENI_PRE_ALLOCATE` | 预创建 ENI 数量 | 1-2 |
| `IP_PRE_ALLOCATE` | 每个 ENI 预分配 IP 数 | 3-5 |
| `max_pool_size` | IP 池最大容量 | 10-20 |
| `min_pool_size` | IP 池最小容量 | 3-5 |

ConfigMap 配置:

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "ENIIP",
      "vswitches": {"cn-hangzhou-b": ["vsw-xxx"]},
      "security_group": "sg-xxx",
      "max_pool_size": 20,
      "min_pool_size": 5
    }
```

### 5.2 OpenAPI 调用优化

| 优化项 | 建议值 | 说明 |
|:---|:---|:---|
| IP 池大小 | 10-20 | 根据节点 Pod 密度调整，避免频繁 API 调用 |
| API 重试次数 | 3 次 | 避免临时限流导致分配失败 |
| 多 vSwitch 分配 | 2-3 个 | 分散 API 调用到不同 vSwitch，降低单 vSwitch 限流风险 |
| vSwitch 分布 | 多可用区 | 提高可用性和 API 并发能力 |

多 vSwitch 配置示例:

```json
{
  "vswitches": {
    "cn-hangzhou-b": ["vsw-b-001", "vsw-b-002"],
    "cn-hangzhou-g": ["vsw-g-001"]
  }
}
```

### 5.3 IP 分配耗时分析

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system <terway-pod> | grep "allocate.*cost"
kubectl logs -n kube-system <terway-pod> | grep "api.*latency"
```
正常基线: IP 分配 < 2s (池内 IP < 100ms，需新建 IP < 2s)。

---

## 6. eBPF 加速

### 6.1 Cilium eBPF 替代 kube-proxy

Terway 集成 Cilium eBPF 数据面，替代 iptables/kube-proxy，显著降低服务访问延迟:

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-config
  namespace: kube-system
data:
  terway-config: |
    {
      "kube_proxy_replacement": "strict",
      "enable_bpf_masquerade": true,
      "enable_bandwidth_manager": true,
      "enable_endpoint_routes": true
    }
```

### 6.2 eBPF 加速效果

| 指标 | iptables (kube-proxy) | eBPF (Cilium) | 提升幅度 |
|:---|:---:|:---:|:---:|
| Service 连接建立 P99 | ~1.5ms | ~0.3ms | **5x** |
| Service 转发延迟 | ~100µs | ~20µs | **5x** |
| 规则更新延迟 | 秒级 | 毫秒级 | **100x** |
| CPU 开销 (1000 Service) | ~5% | ~0.5% | **10x** |
| 可观测性 | 无 | Hubble 集成 | - |

### 6.3 带宽管理器

开启带宽管理器后，eBPF 在 socket 层直接执行 EDT (Earliest Departure Time) 限速，避免 qdisc 队列堆积:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n kube-system <terway-pod> -- \
  cilium bandwidth list
```
要求: 内核 5.10+，Terway v1.5+。

### 6.4 iptables → eBPF 迁移指南

**迁移步骤:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
Step 1: 确认前置条件
  - 内核 >= 4.19 (推荐 5.10+)
  - Terway >= v1.3 (推荐 v1.5+)
  - Cilium 版本兼容 (见下表)
  - 节点无 BPF 程序冲突 (tcpdump -i any 方式不会冲突)

Step 2: 切换网络策略引擎
  kubectl edit cm terway-config -n kube-system
  # 修改: "network_policy": "ebpf"  (原来是 "iptables")
  kubectl rollout restart ds/terway-eniip -n kube-system

Step 3: 验证 eBPF 数据面
  kubectl exec -n kube-system <cilium-pod> -- cilium status
  kubectl exec -n kube-system <cilium-pod> -- cilium policy list
  # 确认 NetworkPolicy 规则已加载到 eBPF

Step 4: 观察运行状态
  # 监控 24 小时，确认无网络异常
  kubectl logs -n kube-system -l k8s-app=cilium --since=1h | grep -iE "error|warn|drop"
```
**Cilium 版本兼容性矩阵:**

| Terway 版本 | 最低 Cilium 版本 | 推荐 Cilium 版本 | 说明 |
|:---|:---:|:---:|:---|
| v1.3.x | 1.12+ | 1.12.x | eBPF 基础支持，L3/L4 策略 |
| v1.4.x | 1.12+ | 1.13.x | 增强 GC + IPv6 双栈 |
| v1.5.x | 1.14+ | 1.14.x | 完整 eBPF 数据面，带宽管理器 |
| v1.6+ (计划) | 1.14+ | 1.15.x | kube-proxy 完全替代 |

**回滚步骤:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

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
# 1. 切换回 iptables 模式
kubectl edit cm terway-config -n kube-system
# 将 network_policy 从 "ebpf" 改回 "iptables"

# 2. 重启 Terway 使配置生效
kubectl rollout restart ds/terway-eniip -n kube-system

# 3. 等待所有节点 Terway Pod 重新就绪
kubectl rollout status ds/terway-eniip -n kube-system --timeout=300s

# 4. 确认 iptables 规则已恢复
kubectl exec -n kube-system <terway-pod> -- iptables -L -n | grep -c terway
```
> **注意**: 回滚期间 NetworkPolicy 会有短暂的未生效窗口，建议在维护窗口操作。

**已知不兼容性:**

| 问题 | 影响 | 解决方案 |
|:---|:---|:---|
| 部分 NetworkPolicy extensions 不支持 | `ipBlock` 中的 `except` 在 eBPF 模式下可能不完整 | 升级 Cilium 至 1.14+ 或拆分为多条规则 |
| kube-proxy iptables 模式冲突 | iptables 与 eBPF 在 hook 点冲突 | 使用 `kube_proxy_replacement: strict` 完全替代 |
| HostPort Service | eBPF 模式下 HostPort 行为差异 | 使用 NodePort 或 LoadBalancer 替代 |
| SCTP 协议 | eBPF 对 SCTP 支持有限 | 暂不支持，需保持 iptables 模式 |

**验证命令:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 eBPF 数据面状态
kubectl exec -n kube-system <cilium-pod> -- cilium status --verbose
kubectl exec -n kube-system <cilium-pod> -- cilium bpf tunnel list
kubectl exec -n kube-system <cilium-pod> -- cilium endpoint list

# 确认 iptables 规则已清理 (eBPF 模式下应为空或极少)
kubectl exec -n kube-system <terway-pod> -- iptables -t filter -L TERWAY -n 2>/dev/null || echo "TERWAY chain not found (expected in eBPF mode)"

# 性能对比验证
kubectl exec -n kube-system <cilium-pod> -- cilium metrics list | grep -E "drop|forward|policy"
```
---

## 7. ENI 预热

### 7.1 预分配机制

Terway 在节点启动时通过 ENI 预热机制提前创建弹性网卡并分配辅助 IP，避免 Pod 首次调度时的冷启动延迟。

预分配工作流程:

```
节点启动 → Terway DaemonSet 启动 → 检查 eni-config 配置
  → 调用 OpenAPI 创建辅助 ENI → 分配辅助 IP → 加入本地 IP 池
  → Pod 创建时直接从池中获取 IP (ms 级)
```

### 7.2 ENI 预热配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "ENIIP",
      "vswitches": {"cn-hangzhou-b": ["vsw-xxx"]},
      "security_group": "sg-xxx",
      "hot_plug": true,
      "max_pool_size": 20,
      "min_pool_size": 5,
      "eni_cap": 3
    }
```

| 参数 | 说明 |
|:---|:---|
| `hot_plug` | 开启 ENI 热插拔预热 |
| `max_pool_size` | IP 池上限，防止过度占用 IP 资源 |
| `min_pool_size` | IP 池下限，低于此值触发异步补充 |
| `eni_cap` | 预创建的 ENI 数量上限 |

### 7.3 监控预热状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system <terway-pod> | grep "pool|pre-allocate|eni.*attach"
```
---

## 8. 监控与观测

### 8.1 关键性能指标

| 指标类别 | 指标名 | 采集方式 | 说明 |
|:---|:---|:---|:---|
| 分配性能 | IP 分配耗时 | Terway 日志 | Pod 获得端到端 IP 的时间 |
| API 延迟 | OpenAPI 调用延迟 | Terway 日志 | 调用阿里云 API 的响应时间 |
| 吞吐量 | 节点网络吞吐 | Prometheus node_exporter | 节点级收发带宽 |
| 连接率 | TCP 连接建立速率 | Prometheus | 每秒新建连接数 |
| 丢包率 | 网卡丢包统计 | ethtool / Prometheus | rx/tx 丢包和错误 |
| IP 池 | 池内可用 IP 数 | Terway metrics | 当前池内 IP 数量 |

### 8.2 日志分析

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system <terway-pod> | grep "allocate.*cost"
kubectl logs -n kube-system <terway-pod> | grep "api.*latency"
kubectl logs -n kube-system <terway-pod> | grep "error|fail|timeout"
```
### 8.3 iperf3 基准测试

服务端:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run iperf3-server --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- -s
kubectl expose pod iperf3-server --port 5201 --target-port=5201
```
客户端 - 吞吐量测试:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run iperf3-client --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- \
  -c <server-ip> -t 30 -P 4 -i 5
```
客户端 - UDP 吞吐量测试:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run iperf3-client-udp --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- \
  -c <server-ip> -u -b 10G -t 30 -P 4
```
客户端 - 延迟测试 (JSON 输出):

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run iperf3-client-latency --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- \
  -c <server-ip> -t 10 --json
```
### 8.4 Ping 延迟测试

同节点 Pod 间延迟:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run ping-same-node \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --command -- ping -c 100 -i 0.1 <same-node-pod-ip>
```
跨节点 Pod 间延迟:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run ping-cross-node \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --command -- ping -c 100 -i 0.1 <cross-node-pod-ip>
```
---

## 9. 生产环境性能基线

### 9.1 参考基线表

| 指标 | 基线值 | 告警阈值 | 说明 |
|:---|:---|:---|:---|
| Pod IP 分配 P99 | < 2s | > 5s | 含 OpenAPI 调用，池内 IP 应 < 100ms |
| 同节点 Pod 延迟 P99 | < 100µs | > 500µs | veth pair / IPVlan 开销 |
| 跨节点 Pod 延迟 P99 | < 200µs | > 1ms | 含 VPC 网络一跳 |
| 吞吐量 (ENIIP) | > 10 Gbps | < 5 Gbps | 基于 ecs.g7.4xlarge |
| 吞吐量 (ENI 独占) | > 20 Gbps | < 10 Gbps | 基于 ecs.g7.4xlarge |
| TCP 连接建立 P99 | < 500µs | > 2ms | 含 Service 转发 |
| IP 池命中率 | > 95% | < 80% | 池内 IP 直接分配比例 |
| OpenAPI 调用 P99 | < 1s | > 3s | CreateNetworkInterface 等 |

### 9.2 基线测试条件

- ECS 实例: ecs.g7.4xlarge (16 vCPU / 64 GiB)
- 操作系统: Alibaba Cloud Linux 3 (内核 5.10)
- Terway 版本: v1.5+
- 网络模式: ENIIP
- 测试工具: iperf3, ping, wrk
- 测试时间: 30 秒，取 P99 值

---

## 10. 性能故障排查

### 10.1 ENI 分配延迟诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system <terway-pod> --tail=500 | grep -E "allocate|eni|ip.*assign"
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get events -n kube-system --sort-by='.lastTimestamp' | grep terway
```
常见原因及处理:

| 现象 | 可能原因 | 处理方法 |
|:---|:---|:---|
| IP 分配 > 5s | OpenAPI 限流 | 增大 IP 池，启用多 vSwitch |
| IP 分配失败 | vSwitch IP 耗尽 | 扩容 vSwitch CIDR 或新增 vSwitch |
| ENI 创建超时 | ECS ENI 配额不足 | 升级实例规格或申请配额提升 |
| Pod 卡在 ContainerCreating | Terway 未就绪 | 检查 DaemonSet 状态和节点 ENI 状态 |

### 10.2 网络延迟与丢包诊断

检查网卡统计信息:

```bash
ethtool -S eth0 | grep -E "drop|error|discard|miss"
```

```bash
ip -s link show eth0
```

检查连接跟踪表:

```bash
conntrack -C
sysctl net.netfilter.nf_conntrack_max
sysctl net.netfilter.nf_conntrack_count
```

检查路由缓存和 ARP:

```bash
ip route show cache
ip neigh show
```

### 10.3 内核网络参数验证

```bash
sysctl -a | grep -E "somaxconn|tcp_max_syn|netdev_max|tcp_tw_reuse|ip_local_port_range|rmem_max|wmem_max"
```

### 10.4 网卡队列与中断检查

```bash
ethtool -l eth0
cat /proc/interrupts | grep eth0
cat /proc/softirqs | grep net
```

### 10.5 TCP 重传与拥塞分析

```bash
netstat -s | grep -E "retransmit|segment"
ss -ti
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
nstat -az | grep -E "TcpRetransSegs|TcpOutRsts"
```
---

## 11. 交叉引用

### 11.1 本专题内文档

| 文档 | 说明 |
|:---|:---|
| [01-product.md](./01-product.md) | Terway 产品概览、版本历史、模式总览 |
| [02-architecture.md](./02-architecture.md) | 架构原理、数据面/控制面、CRD 资源模型 |
| [03-usage.md](./[[32-发布/package/2026-07-02_18-40/corpus/core/domain-03-networking-traffic/topic-terway/01-usage|03-usage]].md) | 安装配置、模式切换、NetworkPolicy、固定 IP |
| [04-operations.md](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-03-networking-traffic/topic-terway/03-operations.md) | 健康检查、GC 机制、升级策略、监控告警 |
| [05-testing.md](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-03-networking-traffic/topic-terway/04-testing.md) | Pod 网络验证、连通性测试、NetworkPolicy 测试 |

### 11.2 库内关联文档

| 文档 | 说明 |
|:---|:---|
| [domain-03-networking-traffic/05-terway-advanced-guide.md](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/01-terway-advanced-guide.md) | Terway 高级指南（模式对比、ENIIP 详解、容量规划） |
| [domain-03-networking-traffic/37-terway-resources-crud-operations.md](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/29-terway-resources-crud-operations.md) | Terway 实例 CRUD 操作、CRD 资源管理 |
| [domain-03-networking-traffic/38-terway-gc-mechanism.md](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/30-terway-gc-mechanism.md) | Terway GC 垃圾回收机制 |
| [domain-03-networking-traffic/34-network-performance-tuning.md](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/26-network-performance-tuning.md) | 网络性能调优通用指南 |
| [domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md](../domain-10-troubleshooting-diagnostics/高级排障/03-networking/07-terway-troubleshooting.md) | Terway 结构化故障排查 |
| [domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md](../domain-10-troubleshooting-diagnostics/FTA故障树/list/terway-fta.md) | Terway 异常 FTA 故障树 |
| [domain-11-production-operations/topic-presentations/kubernetes-terway-presentation.md](../domain-11-production-operations/topic-presentations/kubernetes-terway-presentation.md) | Terway 全栈进阶培训 |

---

**Kusheet Project** | 作者: Allen Galler (allengaller@gmail.com)

## Related

- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]

```

<!-- risk-assessed -->
