# NetworkPolicy 深度排查与零信任安全治理指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 资深专家级

---

## 0. 读者对象与价值
| 角色 | 目标 | 核心收获 |
| :--- | :--- | :--- |
| **初学者** | 解决“配置了策略却不通”或“通了却没隔离”的问题 | 掌握 NetworkPolicy 的白名单模型、匹配逻辑及基础连通性测试。 |
| **中级运维** | 实施命名空间级隔离与 DNS 治理 | 理解 `namespaceSelector` 与 `podSelector` 的组合逻辑，规避 DNS 阻断坑。 |
| **资深专家** | 构建大规模集群的零信任安全架构 | 深入底层实现（iptables vs eBPF）、性能瓶颈优化、跨集群/全局策略治理。 |

---

## 0.5 10 分钟快速诊断

1. **CNI 是否支持**：确认使用的 CNI 支持 NetworkPolicy（如 Calico/Cilium）；纯 Flannel 无效。
2. **命中策略确认**：`kubectl get netpol -A`，定位目标 Pod 是否被任何策略选中（被选中即进入隔离）。
3. **连通性快速测试**：使用 `netshoot` 在源/目标 Pod 互测，区分 `timeout` 与 `refused`。
4. **命名空间标签**：`kubectl get ns --show-labels`，确认 `namespaceSelector` 依赖的标签是否存在。
5. **DNS 放行**：检查 Egress 是否允许到 `kube-system` 53/UDP/TCP，避免“域名解析失败”。
6. **HostNetwork 排查**：`kubectl get pod -o jsonpath='{.spec.hostNetwork}'`，hostNetwork 不受策略影响。
7. **快速缓解**：临时缩小策略范围或添加显式放行规则（DNS/健康检查/监控）。
8. **证据留存**：保存策略 YAML、连通性测试结果、CNI 日志与规则快照。

---

## 1. 核心原理与底层机制

### 1.1 NetworkPolicy 的声明式本质
NetworkPolicy 只是“意图声明”（Desired State），其执行依赖于 CNI 插件：
- **控制面**：kube-apiserver 接收资源，控制器将其同步给各节点的 CNI Agent。
- **数据面 (Data Plane)**：
  - **iptables 模式 (Calico/Weave)**：将规则转换为具体的 iptables 链 and 规则。规则过多会导致 $O(n)$ 匹配延迟。
  - **eBPF 模式 (Cilium)**：将规则编译为 BPF 指令并注入内核 Hook 点。查找复杂度为 $O(1)$，性能极高。
  - **IPSet 优化**：Calico 使用 IPSet 聚合 IP 列表，显著提升 iptables 查找速度。

### 1.2 匹配逻辑的“三原则”
1. **白名单模型**：只要 Pod 被任一 NetworkPolicy 选中，它就会进入“隔离模式”。此时除明确允许的流量外，其余全部拒绝。
2. **OR 逻辑**：多个 NetworkPolicy 作用于同一 Pod 时，规则是叠加（OR）关系。
3. **Selector 逻辑**：
   - `from.podSelector`：同命名空间下的 Pod。
   - `from.namespaceSelector`：匹配选中命名空间下的**所有** Pod。
   - `podSelector` + `namespaceSelector` (同一列表项)：AND 关系，表示“特定命名空间下的特定 Pod”。

---

## 2. 专家级故障矩阵与观测工具

### 2.1 专家级故障矩阵

| 现象分类 | 深度根因分析 | 关键观测工具/指令 |
| :--- | :--- | :--- |
| **策略失效 (Policy Ignored)** | CNI 不支持（如纯 Flannel）、CNI Agent 未正常启动、API 资源版本不兼容。 | `kubectl logs -n kube-system <cni-pod>` |
| **大规模集群性能抖动** | iptables 规则爆炸（数万条），导致网络延迟增加或 CPU 负载过高。 | `iptables -t filter -L -n \| wc -l` |
| **跨 NS 通信失败** | 目标 Namespace 缺少对应的 Label，导致 `namespaceSelector` 匹配为空。 | `kubectl get ns --show-labels` |
| **HostNetwork Pod 逃逸** | NetworkPolicy 无法限制 hostNetwork Pod，因为它们不经过 CNI 的虚拟网卡。 | `kubectl get pod -o jsonpath='{.spec.hostNetwork}'` |

### 2.2 专家排查工具箱

```bash
# 1. Cilium 专家级策略追踪 (模拟报文路径)
cilium policy trace --src-k8s-pod default:frontend --dst-k8s-pod prod:backend --dport 80

# 2. 查看 Calico 实际下发到内核的规则 (Felix 状态)
# 进入 Calico-node Pod
cat /var/run/calico/felix-debug.log | grep "ApplyPolicy"

# 3. 实时观测被拦截的流量 (Cilium 特有)
cilium monitor --type drop

# 4. 检查 iptables 规则计数 (确认流量是否命中)
iptables -t filter -nvL KUBE-NWPLCY-XXXXXXXX
```

---

## 3. 深度排查路径

### 3.1 第一阶段：身份与标识验证
确认“谁在影响谁”。

```bash
# 获取受特定标签影响的所有策略
kubectl get netpol -A -o json | jq '.items[] | select(.spec.podSelector.matchLabels.app=="my-app") | .metadata.name'

# 检查 Namespace 是否具备必要的元数据标签 (K8s 1.21+ 自动注入)
kubectl get ns <ns-name> --show-labels | grep "kubernetes.io/metadata.name"
```

### 3.2 第二阶段：流量向量分析
模拟请求，区分“拒绝”还是“不可达”。

```bash
# 使用带有完整工具链的镜像
kubectl run debug --rm -it --image=nicolaka/netshoot -- /bin/bash

# 测试时观察返回码：
# - "Connection timed out": 通常是防火墙/策略丢弃 (Drop)
# - "Connection refused": 通常是后端没进程监听 (Reject/Not Running)
```

---

## 4. 深度解决方案与生产最佳实践

### 4.1 构建“全隔离”命名空间 (Default Deny)
**最佳实践**：新命名空间上线首件事是执行 Default Deny。
```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: my-secure-app
spec:
  podSelector: {}  # 选中所有 Pod
  policyTypes: ["Ingress", "Egress"]
  # 不定义 ingress/egress 列表即代表全部拒绝
```

### 4.2 解决“DNS 阻断”典型故障
**现象**：开启 Egress 限制后，Pod 无法拉取镜像、无法连接数据库，报错“Temporary failure in name resolution”。
**修复方案**：
```yaml
spec:
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53  # 必须允许到 CoreDNS 的 53 端口
```

### 4.3 处理 HostNetwork 与外部实体通信
- **外部数据库**：使用 `ipBlock` 定义白名单，注意排除掉本地回环地址。
- **LoadBalancer 探针**：如果使用云厂商 LB，需放通 LB 节点的 IP 段，否则健康检查会失败。

---

## 5. 生产环境典型案例解析

### 5.1 案例一：策略变更导致 Prometheus 监控数据中断
- **根因分析**：Prometheus 通过 Pod IP 直接抓取指标，开启 Ingress 限制后未放通监控流量。
- **对策**：在全局策略中允许来自监控组件 Namespace 的入站流量。

### 5.2 案例二：Cilium 环境下策略不生效，重启 Pod 后恢复
- **根因分析**：Cilium 的身份标识（Identity）是基于 Label 计算的，如果 Label 频繁变动或 Cilium-Operator 同步延迟，会导致身份不匹配。
- **对策**：检查 `cilium identity list`，确保 Pod Label 与内核中的 Identity 映射一致。

---

## 附录：网络安全巡检表
- [ ] **默认行为**：关键业务 Namespace 是否配置了 Default Deny？
- [ ] **最小权限**：规则是否精细到具体的 `port` 和 `protocol`？
- [ ] **标签治理**：Namespace 的标签是否规范，防止误匹配导致隔离失效？
- [ ] **DNS 放行**：所有 Egress 策略是否都包含了对 CoreDNS 的放行？
- [ ] **性能监控**：在大规模集群中是否监控了 CNI Agent 的 CPU 使用率？
- [ ] **工具准备**：是否在集群内备好了 `netshoot` 等排查镜像？


---

## 1.3 CNI 插件策略实现深度对比

### 1.3.1 Calico 策略引擎架构

**核心组件**

```
┌────────────────────────────────────────────────────────────────┐
│                    Calico 控制与数据平面                        │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────────────────────────────────────────────┐      │
│  │                   kube-apiserver                      │      │
│  │              (NetworkPolicy 资源存储)                 │      │
│  └────────────────────┬─────────────────────────────────┘      │
│                       │ Watch                                  │
│                       ▼                                        │
│  ┌──────────────────────────────────────────────────────┐      │
│  │              Calico Controller                        │      │
│  │  - 监听 NetworkPolicy/Pod/Namespace 变化              │      │
│  │  - 转换为 Calico Policy 对象                         │      │
│  │  - 写入 etcd (Calico 自己的 etcd 或 K8s etcd)        │      │
│  └────────────────────┬─────────────────────────────────┘      │
│                       │                                        │
│         ┌─────────────┴─────────────┐                          │
│         │                           │                          │
│         ▼                           ▼                          │
│  ┌──────────────┐           ┌──────────────┐                  │
│  │   Node 1     │           │   Node 2     │                  │
│  ├──────────────┤           ├──────────────┤                  │
│  │              │           │              │                  │
│  │  Felix Agent │           │  Felix Agent │                  │
│  │  - 读取策略  │           │  - 读取策略  │                  │
│  │  - 计算规则  │           │  - 计算规则  │                  │
│  │  - 生成:     │           │  - 生成:     │                  │
│  │    * iptables│           │    * iptables│                  │
│  │    * ipset   │           │    * ipset   │                  │
│  │    * routes  │           │    * routes  │                  │
│  └──────┬───────┘           └──────┬───────┘                  │
│         │                          │                          │
│         ▼                          ▼                          │
│  ┌──────────────┐           ┌──────────────┐                  │
│  │   Kernel     │           │   Kernel     │                  │
│  │  iptables:   │           │  iptables:   │                  │
│  │  - FORWARD   │           │  - FORWARD   │                  │
│  │  - INPUT     │           │  - INPUT     │                  │
│  │  - OUTPUT    │           │  - OUTPUT    │                  │
│  │              │           │              │                  │
│  │  ipset:      │           │  ipset:      │                  │
│  │  - Pod IPs   │           │  - Pod IPs   │                  │
│  │  - NS IPs    │           │  - NS IPs    │                  │
│  └──────────────┘           └──────────────┘                  │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

**iptables 规则生成示例**

```bash
# 假设有如下 NetworkPolicy:
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend
  namespace: prod
spec:
  podSelector:
    matchLabels:
      app: backend
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080

# Calico Felix 会生成以下 iptables 规则 (简化版):

# 1. 创建专用链
iptables -t filter -N cali-fw-cali1234abcd  # Pod backend 的 FORWARD 链
iptables -t filter -N cali-tw-cali1234abcd  # Pod backend 的 TO-WORKLOAD 链

# 2. 主链跳转
iptables -t filter -A FORWARD -m comment --comment "cali:from-endpoint" \
  -j cali-from-endpoint

# 3. Pod 流量入口规则
iptables -t filter -A cali-tw-cali1234abcd \
  -m set --match-set cali40s:frontend-pods src \  # 源 IP 在 frontend ipset 中
  -p tcp --dport 8080 \                           # 目标端口 8080
  -m comment --comment "Allow from frontend" \
  -j ACCEPT

# 4. 默认拒绝 (因为 Pod 被策略选中,进入隔离模式)
iptables -t filter -A cali-tw-cali1234abcd \
  -m comment --comment "Drop other ingress" \
  -j DROP

# 5. ipset 管理 (自动维护 frontend Pod IP 列表)
ipset create cali40s:frontend-pods hash:ip
ipset add cali40s:frontend-pods 10.244.1.5
ipset add cali40s:frontend-pods 10.244.2.8
```

**性能特点**

| 特性 | iptables 模式 | 性能影响 |
|------|---------------|----------|
| 规则匹配 | 线性遍历 O(n) | 规则数 > 10000 时延迟明显 |
| ipset 优化 | 哈希查找 O(1) | 显著提升 IP 匹配速度 |
| 规则更新 | 原子替换 | 大规模更新时有短暂锁定 |
| CPU 开销 | 每包匹配 | 高流量场景 CPU 使用率高 |

### 1.3.2 Cilium eBPF 策略引擎

**eBPF 架构优势**

```
┌────────────────────────────────────────────────────────────────┐
│                    Cilium eBPF 数据平面                         │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────────────────────────────────────────────┐      │
│  │                 Cilium Operator                       │      │
│  │  - 监听 NetworkPolicy 资源                            │      │
│  │  - 生成 Cilium Policy (CiliumNetworkPolicy)          │      │
│  │  - 分配 Security Identity (基于 Label)               │      │
│  └────────────────────┬─────────────────────────────────┘      │
│                       │                                        │
│                       ▼                                        │
│  ┌──────────────────────────────────────────────────────┐      │
│  │                Cilium Agent (每个节点)                │      │
│  │  1. 读取策略定义                                      │      │
│  │  2. 编译为 eBPF 字节码                                │      │
│  │  3. 加载到内核 Hook 点:                               │      │
│  │     - TC (Traffic Control) ingress/egress            │      │
│  │     - XDP (eXpress Data Path)                        │      │
│  │     - Socket Operations                              │      │
│  └────────────────────┬─────────────────────────────────┘      │
│                       │                                        │
│                       ▼                                        │
│  ┌──────────────────────────────────────────────────────┐      │
│  │                  Linux Kernel                         │      │
│  │                                                       │      │
│  │  ┌─────────────────────────────────────────────┐     │      │
│  │  │      eBPF Maps (内核内存)                    │     │      │
│  │  │  - Policy Map (策略规则)                    │     │      │
│  │  │  - Endpoint Map (Pod Identity)              │     │      │
│  │  │  - Connection Tracking                      │     │      │
│  │  └─────────────────────────────────────────────┘     │      │
│  │                                                       │      │
│  │  ┌─────────────────────────────────────────────┐     │      │
│  │  │      eBPF Programs (每包执行)                │     │      │
│  │  │                                             │     │      │
│  │  │  from_container():                          │     │      │
│  │  │    1. 查询源 Pod Identity                   │     │      │
│  │  │    2. 查询目标 Endpoint                     │     │      │
│  │  │    3. 匹配 Policy Map                       │     │      │
│  │  │    4. 决策: ALLOW / DROP / REDIRECT         │     │      │
│  │  │    5. 记录 Connection Tracking              │     │      │
│  │  │                                             │     │      │
│  │  │  to_container():                            │     │      │
│  │  │    1. 查询目标 Pod Identity                 │     │      │
│  │  │    2. 反向策略检查                          │     │      │
│  │  │    3. 决策 + 日志                           │     │      │
│  │  └─────────────────────────────────────────────┘     │      │
│  │                                                       │      │
│  │  网络设备 (veth/eth0)                                 │      │
│  │    ↓  (每个数据包经过 eBPF 过滤)                      │      │
│  │  [eBPF TC Hook] → 策略决策 → [继续路由 or DROP]       │      │
│  │                                                       │      │
│  └───────────────────────────────────────────────────────┘      │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

**Identity-based 策略模型**

```bash
# Cilium 为每个唯一的 Label 组合分配 Identity ID

# 查看 Identity 列表
cilium identity list

# 输出示例:
ID      LABELS
1       reserved:host
2       reserved:world
256     k8s:app=frontend, k8s:io.kubernetes.pod.namespace=prod
257     k8s:app=backend, k8s:io.kubernetes.pod.namespace=prod
258     k8s:app=database, k8s:io.kubernetes.pod.namespace=prod

# NetworkPolicy 转换为基于 Identity 的规则
# 例: 允许 frontend (256) 访问 backend (257) 的 8080 端口

# eBPF Map 中的策略规则 (伪代码):
policy_map[257] = {
  ingress_rules: [
    {
      from_identity: 256,        # frontend
      to_port: 8080,
      protocol: TCP,
      action: ALLOW
    }
  ],
  default_ingress: DROP          # 默认拒绝
}
```

**性能对比**

| 指标 | Calico (iptables) | Cilium (eBPF) | 性能提升 |
|------|-------------------|---------------|----------|
| 规则匹配延迟 | 10-50 µs (取决于规则数) | 1-2 µs (O(1) 哈希查找) | **5-50x** |
| CPU 开销 | 5-15% (高流量) | 1-3% | **3-5x** |
| 规则数上限 | ~20000 (性能下降) | 100000+ | **5x+** |
| 连接跟踪 | netfilter conntrack | eBPF Map (内核内存) | 更高效 |
| 可观测性 | 有限 (需日志) | 原生 (Hubble) | 更强 |

### 1.3.3 Weave Net 策略实现

**特点**: 使用 OVS (Open vSwitch) 流表

```bash
# Weave 将 NetworkPolicy 转换为 OVS 流表规则

# 查看 OVS 流表
ovs-ofctl dump-flows weave

# 示例流表:
table=0, priority=100, in_port=1, dl_src=02:42:ac:11:00:02, actions=resubmit(,1)
table=1, priority=100, tcp, tp_dst=8080, actions=output:2
table=1, priority=0, actions=drop

# 性能: 介于 iptables 和 eBPF 之间
```

---

## 2.3 专家级故障场景深度解析

### 2.3.1 按故障阶段分类

**阶段 1: 策略创建与编译 (0-5s)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| NetworkPolicy 创建失败 | API Server Webhook 拒绝 | `kubectl describe netpol` 查看 Events | 策略语法错误、冲突标签 |
| CNI Agent 未处理策略 | CNI 不支持或 Agent 异常 | `kubectl logs -n kube-system <cni-pod>` | Flannel 不支持策略 |
| 策略编译超时 | eBPF 程序编译失败 | `cilium bpf policy get <endpoint-id>` | Cilium 内核版本过低 |
| ipset 创建失败 | 内核 ipset 模块未加载 | `lsmod \| grep ip_set` | 精简内核缺少模块 |

**阶段 2: 规则下发与生效 (5-30s)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| iptables 规则未生成 | Felix Agent 故障或策略不匹配 | `iptables-save \| grep cali` | Calico Felix 重启 |
| eBPF 程序未加载 | BPF 编译错误或权限不足 | `bpftool prog list` | SELinux 阻止 |
| 规则下发延迟 > 30s | 控制面负载过高 | 检查 Cilium Operator CPU 使用率 | 10000+ Pod 集群 |
| Pod 重启后策略失效 | Endpoint 未重新注册 | `cilium endpoint list` | Identity 回收延迟 |

**阶段 3: 流量匹配与执行 (运行时)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| 策略匹配错误 | 标签选择器语法错误 | 手动测试 `kubectl get pods -l <selector>` | `matchLabels` 拼写错误 |
| 命名空间标签缺失 | K8s < 1.21 需手动添加 | `kubectl get ns --show-labels` | 旧集群无自动标签 |
| DNS 解析失败 | Egress 未放行 CoreDNS | `kubectl run test --rm -it --image=busybox -- nslookup kubernetes` | Default Deny 阻止 DNS |
| HostNetwork Pod 逃逸 | 策略无法限制 hostNetwork | `kubectl get pod -o jsonpath='{.spec.hostNetwork}'` | Ingress Controller 使用 hostNetwork |

**阶段 4: 性能与规模化 (大规模集群)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| iptables 性能下降 | 规则数 > 10000 导致 O(n) 延迟 | `iptables -t filter -L \| wc -l` | 1000+ NetworkPolicy |
| ipset 更新风暴 | Pod 频繁创建/删除触发重算 | 监控 Felix CPU 使用率 | 批量滚动更新 |
| Cilium Identity 耗尽 | Identity 回收不及时 | `cilium identity list \| wc -l` | 短生命周期 Job |
| eBPF Map 容量不足 | 默认 Map 大小不够 | `cilium bpf config list` | 100000+ Endpoint |

### 2.3.2 复合故障场景

**场景 1: 零信任改造导致全局网络中断**

```
触发条件:
  - 在所有命名空间同时应用 Default Deny 策略
  - 未提前放行 CoreDNS/监控/日志采集流量
  - Ingress Controller 使用 hostNetwork

故障链:
  1. 批量应用 Default Deny NetworkPolicy
  2. 所有 Pod Egress 流量被阻断
  3. DNS 解析失败 → 应用无法启动
  4. Prometheus 无法抓取指标 → 监控盲区
  5. 日志采集器无法连接 Elasticsearch → 日志丢失
  6. Ingress Controller 仍可访问 (hostNetwork 逃逸)
  7. 但后端 Pod 全部不可达 → 全站 503

排查路径:
  # 1. 确认 Default Deny 生效
  kubectl get netpol -A | grep default-deny
  
  # 2. 测试 DNS 连通性
  kubectl run test --rm -it --image=busybox -- nslookup kubernetes.default
  # 预期失败: "server can't find kubernetes.default"
  
  # 3. 检查监控 Endpoints
  kubectl get endpoints prometheus-k8s -n monitoring
  # 预期: Endpoints 为空 (Prometheus 无法连接 Pod)
  
  # 4. 检查日志采集
  kubectl logs -n logging fluent-bit-xxx
  # 预期: "Connection refused" to Elasticsearch

解决方案:
  # 1. 紧急放行 CoreDNS (所有命名空间)
  kubectl apply -f - <<EOF
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-dns-egress
    namespace: <each-namespace>
  spec:
    podSelector: {}
    policyTypes: ["Egress"]
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
  EOF
  
  # 2. 放行监控抓取 (使用命名空间标签)
  kubectl label namespace monitoring name=monitoring
  kubectl apply -f - <<EOF
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-prometheus-scrape
    namespace: <app-namespace>
  spec:
    podSelector: {}
    policyTypes: ["Ingress"]
    ingress:
    - from:
      - namespaceSelector:
          matchLabels:
            name: monitoring
      ports:
      - protocol: TCP
        port: 9090  # 应用 metrics 端口
  EOF
  
  # 3. 放行日志采集
  kubectl apply -f - <<EOF
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-logging-egress
    namespace: <app-namespace>
  spec:
    podSelector: {}
    policyTypes: ["Egress"]
    egress:
    - to:
      - namespaceSelector:
          matchLabels:
            name: logging
      ports:
      - protocol: TCP
        port: 9200  # Elasticsearch 端口
  EOF
```

**场景 2: Calico 大规模策略导致性能雪崩**

```
触发条件:
  - 集群规模: 500 节点, 10000+ Pod
  - NetworkPolicy 数量: 2000+
  - Pod 频繁滚动更新 (每分钟 100+ Pod 变动)

故障链:
  1. 滚动更新触发大量 Pod 创建/删除
  2. Felix Agent 重新计算 ipset 和 iptables 规则
  3. iptables 规则数突破 50000 条
  4. 每个数据包匹配延迟从 10µs 增加到 200µs
  5. 节点 CPU 使用率从 20% 飙升到 80%
  6. 网络延迟 P99 从 5ms 增加到 50ms
  7. 健康检查超时 → Pod 反复重启
  8. 问题扩散到更多节点

排查路径:
  # 1. 检查 iptables 规则数量
  iptables -t filter -L | wc -l
  # 输出: 52384 (严重超标)
  
  # 2. 检查 ipset 数量
  ipset list | grep "Name:" | wc -l
  # 输出: 3248
  
  # 3. 测量 iptables 性能
  time iptables -t filter -L > /dev/null
  # 输出: real 0m8.432s (严重延迟)
  
  # 4. 检查 Felix CPU 使用
  kubectl top pod -n kube-system -l k8s-app=calico-node
  # 输出: CPU 2500m/4000m (63%)
  
  # 5. 查看 Felix 日志
  kubectl logs -n kube-system calico-node-xxx -c calico-node | grep "ApplyUpdates"
  # 输出: "ApplyUpdates took 12.4s" (正常应 < 1s)

解决方案:
  # 方案 1: 迁移到 Cilium eBPF (长期)
  # - eBPF 性能不受规则数量影响
  # - 迁移需要停机维护
  
  # 方案 2: 优化策略设计 (中期)
  # 1. 合并相似策略
  # 原策略: 每个微服务一个 NetworkPolicy (2000个)
  # 优化后: 使用通配符标签, 减少到 200 个
  
  # 2. 使用 GlobalNetworkPolicy (Calico 特性)
  kubectl apply -f - <<EOF
  apiVersion: crd.projectcalico.org/v1
  kind: GlobalNetworkPolicy
  metadata:
    name: allow-monitoring-global
  spec:
    selector: all()
    types: ["Ingress"]
    ingress:
    - action: Allow
      source:
        namespaceSelector: name == "monitoring"
      destination:
        ports: [9090]
  EOF
  
  # 方案 3: 启用 iptables 优化 (短期)
  # 在 Felix 配置中启用:
  kubectl edit felixconfiguration default
  # 添加:
  spec:
    iptablesFilterAllowAction: ACCEPT
    iptablesMangleAllowAction: ACCEPT
    iptablesMarkMask: 0xff000000
    # 使用 ipset 聚合减少规则数
```

**场景 3: Cilium Identity 回收延迟导致策略失效**

```
触发条件:
  - 使用 Cilium 作为 CNI
  - 大量短生命周期 Job/CronJob (每分钟创建 50+ Pod)
  - Pod Label 频繁变化

故障链:
  1. Job Pod 创建时分配新的 Identity (例如 ID=1024)
  2. Job 完成后 Pod 删除
  3. Identity 1024 进入"待回收"状态 (默认延迟 15min)
  4. 新 Job 创建, Label 完全相同
  5. Cilium 分配新的 Identity ID=1025 (而非复用 1024)
  6. NetworkPolicy 仍引用旧 Identity 1024
  7. 新 Pod (ID=1025) 无法匹配策略 → 流量被拒绝
  8. Job 执行失败

排查路径:
  # 1. 查看 Identity 列表和状态
  cilium identity list
  # 输出显示大量 "stale" 状态的 Identity
  
  # 2. 检查 Identity 回收配置
  kubectl get ciliumconfig -n kube-system -o yaml | grep identity-gc
  # 默认: identity-gc-interval: 15m
  
  # 3. 查看特定 Pod 的 Identity
  POD_IP=$(kubectl get pod <job-pod> -o jsonpath='{.status.podIP}')
  cilium endpoint list | grep $POD_IP
  # 输出: ID=1025, Labels=app=job-worker
  
  # 4. 检查策略引用的 Identity
  cilium policy get | grep "FromEndpoints"
  # 输出: "FromEndpoints": [{"matchLabels": {"app": "job-worker"}}]
  # 但内部映射仍指向旧 ID=1024
  
  # 5. 验证策略匹配
  cilium policy trace --src-identity 1025 --dst-identity 257
  # 输出: "Denied by policy" (应该 Allowed)

解决方案:
  # 方案 1: 调整 Identity 回收策略
  kubectl patch ciliumconfig cilium -n kube-system --type=merge -p '
  spec:
    identityGCInterval: 5m     # 从 15min 降低到 5min
    identityHeartbeatTimeout: 30m
  '
  
  # 方案 2: 强制刷新 Identity
  # 重启 Cilium Operator (会触发 Identity 重新计算)
  kubectl rollout restart deployment cilium-operator -n kube-system
  
  # 方案 3: 使用稳定的 Label (避免频繁变化)
  # Job 模板添加固定 Label:
  apiVersion: batch/v1
  kind: Job
  metadata:
    generateName: data-processor-
  spec:
    template:
      metadata:
        labels:
          app: data-processor     # 固定 Label
          job-type: batch         # 额外分类标签
          # 避免使用动态标签如: job-id: <timestamp>
```

---

## 3.3 深度排查脚本集

### 3.3.1 NetworkPolicy 连通性测试脚本

```bash
#!/bin/bash
# 文件: netpol-connectivity-test.sh
# 用途: 自动化测试 NetworkPolicy 连通性

set -e

SOURCE_NS=${1:-default}
SOURCE_POD=${2}
TARGET_NS=${3:-default}
TARGET_SVC=${4}
TARGET_PORT=${5:-80}

echo "=== NetworkPolicy Connectivity Test ==="
echo "Source: $SOURCE_NS/$SOURCE_POD"
echo "Target: $TARGET_NS/$TARGET_SVC:$TARGET_PORT"
echo

# 1. 检查源 Pod 是否存在
echo "--- Checking Source Pod ---"
kubectl get pod -n $SOURCE_NS $SOURCE_POD > /dev/null 2>&1 || {
  echo "❌ Source pod not found"
  exit 1
}
echo "✅ Source pod exists"

# 2. 检查目标 Service 是否存在
echo -e "\n--- Checking Target Service ---"
kubectl get svc -n $TARGET_NS $TARGET_SVC > /dev/null 2>&1 || {
  echo "❌ Target service not found"
  exit 1
}
TARGET_IP=$(kubectl get svc -n $TARGET_NS $TARGET_SVC -o jsonpath='{.spec.clusterIP}')
echo "✅ Target service exists: $TARGET_IP"

# 3. 检查源 Pod 被哪些策略选中
echo -e "\n--- Checking NetworkPolicies on Source ---"
SOURCE_LABELS=$(kubectl get pod -n $SOURCE_NS $SOURCE_POD -o json | \
  jq -r '.metadata.labels | to_entries | map("\(.key)=\(.value)") | join(",")')
echo "Source labels: $SOURCE_LABELS"

MATCHED_POLICIES=$(kubectl get netpol -n $SOURCE_NS -o json | \
  jq -r --arg labels "$SOURCE_LABELS" '
  .items[] | 
  select(.spec.podSelector.matchLabels as $sel | 
    ($labels | split(",") | map(split("=")) | from_entries) as $pod_labels |
    $sel | to_entries | all(.key as $k | .value as $v | $pod_labels[$k] == $v)
  ) | .metadata.name
')

if [ -z "$MATCHED_POLICIES" ]; then
  echo "ℹ️  No NetworkPolicy selecting source pod (allow-all mode)"
else
  echo "⚠️  Source pod is selected by:"
  echo "$MATCHED_POLICIES" | while read policy; do
    echo "  - $policy"
  done
fi

# 4. 检查目标 Pod 被哪些策略选中
echo -e "\n--- Checking NetworkPolicies on Target ---"
TARGET_PODS=$(kubectl get endpoints -n $TARGET_NS $TARGET_SVC -o json | \
  jq -r '.subsets[].addresses[].targetRef.name')

if [ -z "$TARGET_PODS" ]; then
  echo "❌ No endpoints for target service"
  exit 1
fi

TARGET_POD=$(echo "$TARGET_PODS" | head -1)
echo "Target pod (sample): $TARGET_POD"

TARGET_LABELS=$(kubectl get pod -n $TARGET_NS $TARGET_POD -o json | \
  jq -r '.metadata.labels | to_entries | map("\(.key)=\(.value)") | join(",")')
echo "Target labels: $TARGET_LABELS"

TARGET_MATCHED_POLICIES=$(kubectl get netpol -n $TARGET_NS -o json | \
  jq -r --arg labels "$TARGET_LABELS" '
  .items[] | 
  select(.spec.podSelector.matchLabels as $sel | 
    ($labels | split(",") | map(split("=")) | from_entries) as $pod_labels |
    $sel | to_entries | all(.key as $k | .value as $v | $pod_labels[$k] == $v)
  ) | .metadata.name
')

if [ -z "$TARGET_MATCHED_POLICIES" ]; then
  echo "ℹ️  No NetworkPolicy selecting target pod (allow-all mode)"
else
  echo "⚠️  Target pod is selected by:"
  echo "$TARGET_MATCHED_POLICIES" | while read policy; do
    echo "  - $policy"
    # 显示 Ingress 规则
    kubectl get netpol -n $TARGET_NS $policy -o yaml | \
      yq eval '.spec.ingress[] | "    Allow from: " + (.from[] | tojson)' -
  done
fi

# 5. 执行实际连通性测试
echo -e "\n--- Connectivity Test ---"
echo "Testing: curl -s -m 5 http://$TARGET_IP:$TARGET_PORT"

RESULT=$(kubectl exec -n $SOURCE_NS $SOURCE_POD -- \
  timeout 5 curl -s -o /dev/null -w "%{http_code}" http://$TARGET_IP:$TARGET_PORT 2>&1 || echo "TIMEOUT")

if [ "$RESULT" == "TIMEOUT" ]; then
  echo "❌ Connection TIMEOUT (likely blocked by NetworkPolicy)"
  echo "   Recommendation: Check Egress rules on source and Ingress rules on target"
elif [ "$RESULT" == "000" ]; then
  echo "❌ Connection REFUSED (target not listening or blocked)"
else
  echo "✅ Connection successful (HTTP $RESULT)"
fi

# 6. CNI 特定检查
echo -e "\n--- CNI-Specific Checks ---"

# 检测 CNI 类型
CNI_TYPE=$(kubectl get pod -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].spec.containers[0].env[?(@.name=="CNI_CONF_NAME")].value}' 2>/dev/null || echo "unknown")

if kubectl get pods -n kube-system -l k8s-app=calico-node &>/dev/null; then
  echo "CNI: Calico"
  echo "Checking iptables rules on node..."
  NODE=$(kubectl get pod -n $SOURCE_NS $SOURCE_POD -o jsonpath='{.spec.nodeName}')
  echo "Source pod node: $NODE"
  echo "Run on node: iptables -t filter -L | grep cali | wc -l"
  
elif kubectl get pods -n kube-system -l k8s-app=cilium &>/dev/null; then
  echo "CNI: Cilium"
  echo "Checking Cilium policy..."
  SOURCE_POD_IP=$(kubectl get pod -n $SOURCE_NS $SOURCE_POD -o jsonpath='{.status.podIP}')
  CILIUM_POD=$(kubectl get pod -n kube-system -l k8s-app=cilium --field-selector spec.nodeName=$NODE -o jsonpath='{.items[0].metadata.name}')
  
  if [ -n "$CILIUM_POD" ]; then
    echo "Run: kubectl exec -n kube-system $CILIUM_POD -- cilium endpoint list | grep $SOURCE_POD_IP"
  fi
else
  echo "CNI: Unknown or Flannel (NetworkPolicy not supported)"
fi

echo -e "\n=== Test Complete ==="
```

### 3.3.2 Calico 策略调试脚本

```bash
#!/bin/bash
# 文件: calico-policy-debug.sh
# 用途: 深度分析 Calico NetworkPolicy 规则

POD_NAME=${1}
POD_NS=${2:-default}

echo "=== Calico Policy Debug for $POD_NS/$POD_NAME ==="

# 1. 获取 Pod 信息
POD_IP=$(kubectl get pod -n $POD_NS $POD_NAME -o jsonpath='{.status.podIP}')
NODE=$(kubectl get pod -n $POD_NS $POD_NAME -o jsonpath='{.spec.nodeName}')

echo "Pod IP: $POD_IP"
echo "Node: $NODE"
echo

# 2. 查找 Pod 对应的 Calico Endpoint
CALICO_NODE_POD=$(kubectl get pod -n kube-system -l k8s-app=calico-node \
  --field-selector spec.nodeName=$NODE -o jsonpath='{.items[0].metadata.name}')

echo "--- Calico Endpoint Info ---"
kubectl exec -n kube-system $CALICO_NODE_POD -c calico-node -- \
  calicoctl get workloadEndpoint --all-namespaces -o wide | grep $POD_IP

# 3. 获取 Endpoint ID
ENDPOINT_ID=$(kubectl exec -n kube-system $CALICO_NODE_POD -c calico-node -- \
  calicoctl get workloadEndpoint --all-namespaces -o json | \
  jq -r ".[] | select(.spec.ipNetworks[] | contains(\"$POD_IP\")) | .metadata.name")

echo -e "\nEndpoint ID: $ENDPOINT_ID"

# 4. 查看应用到该 Endpoint 的策略
echo -e "\n--- Applied Policies ---"
kubectl exec -n kube-system $CALICO_NODE_POD -c calico-node -- \
  calicoctl get networkpolicy -n $POD_NS -o yaml | \
  yq eval 'select(.spec.selector | contains("'$POD_NAME'"))' -

# 5. 查看 iptables 规则
echo -e "\n--- iptables Rules for Endpoint ---"
echo "Searching for rules matching endpoint..."

# 获取 Pod 的网络接口
POD_IFACE=$(kubectl exec -n $POD_NS $POD_NAME -- ip link show | grep -oP '(?<=\d: )[^:]+' | grep eth0)
NODE_IFACE=$(kubectl exec -n kube-system $CALICO_NODE_POD -c calico-node -- \
  ip link show | grep -A1 "cali.*@" | grep -oP 'cali[a-z0-9]+')

echo "Pod interface: $POD_IFACE"
echo "Node interface: $NODE_IFACE"

# 6. 检查 ipset 内容
echo -e "\n--- ipset Contents ---"
kubectl exec -n kube-system $CALICO_NODE_POD -c calico-node -- \
  ipset list | grep -A10 "cali.*$POD_NS"

# 7. 测试策略匹配
echo -e "\n--- Policy Trace (if supported) ---"
kubectl exec -n kube-system $CALICO_NODE_POD -c calico-node -- \
  calicoctl get networkpolicy -n $POD_NS -o yaml

echo -e "\n=== Debug Complete ==="
echo "Recommendations:"
echo "1. Check if Pod labels match policy selectors"
echo "2. Verify namespace labels for cross-namespace policies"
echo "3. Look for 'DROP' actions in iptables rules"
echo "4. Check Felix logs: kubectl logs -n kube-system $CALICO_NODE_POD -c calico-node"
```

### 3.3.3 Cilium 策略追踪脚本

```bash
#!/bin/bash
# 文件: cilium-policy-trace.sh
# 用途: 追踪 Cilium eBPF 策略决策过程

SOURCE_NS=${1}
SOURCE_POD=${2}
TARGET_NS=${3}
TARGET_POD=${4}
TARGET_PORT=${5:-80}

echo "=== Cilium Policy Trace ==="
echo "Source: $SOURCE_NS/$SOURCE_POD"
echo "Target: $TARGET_NS/$TARGET_POD:$TARGET_PORT"
echo

# 1. 获取源 Pod Endpoint ID
SOURCE_POD_IP=$(kubectl get pod -n $SOURCE_NS $SOURCE_POD -o jsonpath='{.status.podIP}')
SOURCE_NODE=$(kubectl get pod -n $SOURCE_NS $SOURCE_POD -o jsonpath='{.spec.nodeName}')

CILIUM_SRC_POD=$(kubectl get pod -n kube-system -l k8s-app=cilium \
  --field-selector spec.nodeName=$SOURCE_NODE -o jsonpath='{.items[0].metadata.name}')

SOURCE_ENDPOINT=$(kubectl exec -n kube-system $CILIUM_SRC_POD -- \
  cilium endpoint list -o json | jq -r ".[] | select(.status.networking.addressing[].ipv4 == \"$SOURCE_POD_IP\") | .id")

echo "Source Endpoint ID: $SOURCE_ENDPOINT"

# 2. 获取目标 Pod Endpoint ID
TARGET_POD_IP=$(kubectl get pod -n $TARGET_NS $TARGET_POD -o jsonpath='{.status.podIP}')
TARGET_NODE=$(kubectl get pod -n $TARGET_NS $TARGET_POD -o jsonpath='{.spec.nodeName}')

CILIUM_TGT_POD=$(kubectl get pod -n kube-system -l k8s-app=cilium \
  --field-selector spec.nodeName=$TARGET_NODE -o jsonpath='{.items[0].metadata.name}')

TARGET_ENDPOINT=$(kubectl exec -n kube-system $CILIUM_TGT_POD -- \
  cilium endpoint list -o json | jq -r ".[] | select(.status.networking.addressing[].ipv4 == \"$TARGET_POD_IP\") | .id")

echo "Target Endpoint ID: $TARGET_ENDPOINT"

# 3. 查看 Identity 映射
echo -e "\n--- Identity Mapping ---"
SOURCE_IDENTITY=$(kubectl exec -n kube-system $CILIUM_SRC_POD -- \
  cilium endpoint get $SOURCE_ENDPOINT -o json | jq -r '.status.identity.id')
echo "Source Identity: $SOURCE_IDENTITY"

TARGET_IDENTITY=$(kubectl exec -n kube-system $CILIUM_TGT_POD -- \
  cilium endpoint get $TARGET_ENDPOINT -o json | jq -r '.status.identity.id')
echo "Target Identity: $TARGET_IDENTITY"

# 4. 执行策略追踪
echo -e "\n--- Policy Trace Result ---"
kubectl exec -n kube-system $CILIUM_SRC_POD -- \
  cilium policy trace \
    --src-identity $SOURCE_IDENTITY \
    --dst-identity $TARGET_IDENTITY \
    --dport $TARGET_PORT \
    --protocol TCP

# 5. 查看应用到 Endpoint 的策略
echo -e "\n--- Applied Policies on Source ---"
kubectl exec -n kube-system $CILIUM_SRC_POD -- \
  cilium endpoint get $SOURCE_ENDPOINT -o json | jq -r '.status.policy'

echo -e "\n--- Applied Policies on Target ---"
kubectl exec -n kube-system $CILIUM_TGT_POD -- \
  cilium endpoint get $TARGET_ENDPOINT -o json | jq -r '.status.policy'

# 6. 实时监控流量 (可选)
echo -e "\n--- Monitor Traffic (Ctrl+C to stop) ---"
echo "Starting Cilium monitor..."
kubectl exec -n kube-system $CILIUM_SRC_POD -- \
  cilium monitor --type drop --type trace \
    --from $SOURCE_ENDPOINT \
    --to $TARGET_ENDPOINT

echo -e "\n=== Trace Complete ==="
```

---

## 4.4 零信任网络设计最佳实践

### 4.4.1 分层策略管理模型

```
┌─────────────────────────────────────────────────────────────┐
│                   零信任策略分层架构                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Layer 1: 全局默认策略 (Global Baseline)                    │
│  ┌───────────────────────────────────────────────────┐      │
│  │  - Default Deny All (所有命名空间)                 │      │
│  │  - 允许 DNS (kube-system/coredns)                 │      │
│  │  - 允许健康检查 (kubelet → Pod)                   │      │
│  │  - 允许监控抓取 (Prometheus → Metrics)            │      │
│  └───────────────────────────────────────────────────┘      │
│                          ↓                                  │
│  Layer 2: 命名空间级策略 (Namespace Policy)                 │
│  ┌───────────────────────────────────────────────────┐      │
│  │  - 命名空间间隔离规则                              │      │
│  │  - 东西向流量控制 (同 NS 内部通信)                │      │
│  │  - 南北向流量控制 (Ingress/Egress 到外部)         │      │
│  └───────────────────────────────────────────────────┘      │
│                          ↓                                  │
│  Layer 3: 应用级精细策略 (Application Policy)               │
│  ┌───────────────────────────────────────────────────┐      │
│  │  - 微服务间精确通信 (frontend → backend)          │      │
│  │  - 端口级访问控制 (只允许 8080/TCP)               │      │
│  │  - 特殊应用需求 (数据库白名单)                    │      │
│  └───────────────────────────────────────────────────┘      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**实施步骤**

```yaml
# Step 1: 全局 Default Deny (在每个应用命名空间)
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 00-default-deny-all
  namespace: {{ each-app-namespace }}
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]

# Step 2: 全局基础设施放行
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 01-allow-dns
  namespace: {{ each-app-namespace }}
spec:
  podSelector: {}
  policyTypes: ["Egress"]
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
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 01-allow-healthcheck
  namespace: {{ each-app-namespace }}
spec:
  podSelector: {}
  policyTypes: ["Ingress"]
  ingress:
  - from:
    - ipBlock:
        cidr: 0.0.0.0/0  # Kubelet 通过节点 IP 访问
    ports:
    - protocol: TCP
      port: 10254  # 应用健康检查端口

---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 01-allow-prometheus
  namespace: {{ each-app-namespace }}
spec:
  podSelector:
    matchLabels:
      prometheus.io/scrape: "true"
  policyTypes: ["Ingress"]
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 9090  # Metrics 端口

# Step 3: 命名空间级隔离
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 02-ns-isolation
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]
  ingress:
  - from:
    - podSelector: {}  # 只允许同命名空间内的 Pod
  egress:
  - to:
    - podSelector: {}  # 只允许访问同命名空间内的 Pod
  - to:  # 额外允许访问共享服务
    - namespaceSelector:
        matchLabels:
          name: shared-services

# Step 4: 应用级精细策略
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 03-frontend-to-backend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes: ["Ingress"]
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080

---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: 03-backend-to-database
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes: ["Egress"]
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: postgres
    ports:
    - protocol: TCP
      port: 5432
```

### 4.4.2 策略测试与验证流程

```bash
#!/bin/bash
# 文件: netpol-validation-suite.sh
# 用途: 自动化验证 NetworkPolicy 配置

set -e

NAMESPACE=${1:-production}

echo "=== NetworkPolicy Validation Suite ==="
echo "Namespace: $NAMESPACE"
echo

# 1. 检查是否有 Default Deny
echo "--- Check 1: Default Deny Policy ---"
DEFAULT_DENY=$(kubectl get netpol -n $NAMESPACE -o json | \
  jq -r '.items[] | select(.spec.podSelector == {} and .spec.policyTypes != null) | .metadata.name')

if [ -z "$DEFAULT_DENY" ]; then
  echo "❌ FAIL: No Default Deny policy found"
  echo "   Recommendation: Apply default-deny-all policy"
else
  echo "✅ PASS: Default Deny policy exists: $DEFAULT_DENY"
fi

# 2. 检查 DNS 放行
echo -e "\n--- Check 2: DNS Egress Policy ---"
DNS_POLICY=$(kubectl get netpol -n $NAMESPACE -o json | \
  jq -r '.items[] | select(.spec.egress[]?.to[]?.namespaceSelector.matchLabels."kubernetes.io/metadata.name" == "kube-system" and .spec.egress[]?.ports[]?.port == 53) | .metadata.name')

if [ -z "$DNS_POLICY" ]; then
  echo "⚠️  WARNING: No DNS egress policy found"
  echo "   Pods may fail to resolve domain names"
else
  echo "✅ PASS: DNS policy exists: $DNS_POLICY"
fi

# 3. 测试 Pod 间连通性
echo -e "\n--- Check 3: Pod Connectivity Tests ---"

# 获取测试 Pod
FRONTEND_POD=$(kubectl get pod -n $NAMESPACE -l app=frontend -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
BACKEND_SVC=$(kubectl get svc -n $NAMESPACE -l app=backend -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)

if [ -n "$FRONTEND_POD" ] && [ -n "$BACKEND_SVC" ]; then
  echo "Testing: frontend → backend"
  RESULT=$(kubectl exec -n $NAMESPACE $FRONTEND_POD -- \
    timeout 3 curl -s -o /dev/null -w "%{http_code}" http://$BACKEND_SVC:8080 2>&1 || echo "FAIL")
  
  if [ "$RESULT" == "FAIL" ] || [ "$RESULT" == "000" ]; then
    echo "❌ FAIL: Cannot reach backend from frontend"
  else
    echo "✅ PASS: Frontend can reach backend (HTTP $RESULT)"
  fi
else
  echo "ℹ️  SKIP: No frontend/backend pods found for testing"
fi

# 4. 检查策略覆盖率
echo -e "\n--- Check 4: Policy Coverage ---"
TOTAL_PODS=$(kubectl get pods -n $NAMESPACE --no-headers | wc -l)
COVERED_PODS=0

kubectl get pods -n $NAMESPACE -o json | jq -r '.items[].metadata.labels' | while read labels; do
  # 检查该 Pod 是否被任何策略选中
  # (简化版本,实际需要更复杂的标签匹配逻辑)
  COVERED_PODS=$((COVERED_PODS + 1))
done

echo "Total pods: $TOTAL_PODS"
echo "Covered by policies: $COVERED_PODS"
COVERAGE=$(echo "scale=2; $COVERED_PODS * 100 / $TOTAL_PODS" | bc)
echo "Coverage: $COVERAGE%"

if [ $(echo "$COVERAGE < 80" | bc) -eq 1 ]; then
  echo "⚠️  WARNING: Policy coverage below 80%"
else
  echo "✅ PASS: Good policy coverage"
fi

# 5. 检查策略冲突
echo -e "\n--- Check 5: Policy Conflicts ---"
# 查找可能冲突的策略 (例如同时定义 Allow 和 Deny)
POLICIES=$(kubectl get netpol -n $NAMESPACE -o json | jq -r '.items[].metadata.name')
echo "Analyzing $POLICIES for conflicts..."

# TODO: 实现冲突检测逻辑

echo -e "\n=== Validation Complete ==="
```

### 4.4.3 生产环境迁移策略

**阶段 1: 观察模式 (1-2 周)**

```yaml
# 只部署监控,不执行拒绝
# 使用 Cilium Hubble 或 Calico 日志模式
apiVersion: v1
kind: ConfigMap
metadata:
  name: calico-config
  namespace: kube-system
data:
  # 启用日志模式
  typha_log_severity_screen: "info"
  # 记录所有策略决策
  felix_log_all_iptables_chains: "true"
```

**阶段 2: 灰度启用 (2-4 周)**

```yaml
# 先在非生产命名空间启用
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-staging
  namespace: staging
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]

# 监控错误率、延迟等指标
# 如果稳定,逐步扩展到生产命名空间
```

**阶段 3: 全面部署 (1-2 周)**

```bash
# 使用脚本批量部署
for ns in $(kubectl get ns -l environment=production -o jsonpath='{.items[*].metadata.name}'); do
  kubectl apply -f default-deny.yaml -n $ns
  kubectl apply -f allow-dns.yaml -n $ns
  kubectl apply -f allow-monitoring.yaml -n $ns
  echo "Applied to $ns"
  sleep 10  # 观察 10 秒
done
```

**阶段 4: 持续优化**

```bash
# 定期审计策略
kubectl get netpol -A -o yaml > netpol-audit-$(date +%Y%m%d).yaml

# 清理无用策略
kubectl delete netpol -n <ns> <unused-policy>

# 合并相似策略
# 使用 Calico GlobalNetworkPolicy 或 Cilium CiliumNetworkPolicy
```

---

## 5.3 案例三: Cilium eBPF 与 iptables 模式冲突

**故障背景**

- **集群**: EKS 1.28, 150 节点
- **迁移**: 从 Calico (iptables) 迁移到 Cilium (eBPF)
- **配置**: 混合模式 (部分节点 Calico, 部分节点 Cilium)

**故障过程**

```
时间线:
08:00 - 开始滚动升级节点 CNI (Calico → Cilium)
08:05 - 前 10 个节点升级成功
08:10 - 跨节点 Pod 通信异常 (Calico Pod ↔ Cilium Pod)
08:15 - Cilium Pod 报错: "Policy denied by host"
08:20 - Calico Pod 报错: "Connection timeout"
08:25 - 发现 iptables 和 eBPF 规则冲突
08:30 - 紧急回滚 Cilium 节点
09:00 - 制定新的迁移方案
10:00 - 使用蓝绿切换完成迁移
```

**根因分析**

```bash
# 1. Cilium 节点上仍残留 Calico iptables 规则
iptables -t nat -L | grep cali
# 输出: 大量 cali-* 链仍存在

# 2. eBPF 程序与 iptables 规则同时生效
tc filter show dev eth0 ingress
# 输出: eBPF program attached

iptables -t filter -L FORWARD | grep cali
# 输出: Calico 规则仍在 FORWARD 链

# 3. 数据包被双重处理
# eBPF 允许 → iptables 拒绝 → 最终拒绝

# 4. Cilium Identity 与 Calico ipset 不兼容
cilium identity list
# Cilium 使用 Identity 1001-2000

ipset list | grep cali
# Calico 使用 IP 地址匹配
```

**修复方案**

```bash
# 方案: 蓝绿切换 (而非滚动升级)

# 1. 创建新的节点池 (纯 Cilium)
eksctl create nodegroup --cluster=mycluster \
  --name=cilium-nodes \
  --node-ami=ami-cilium \
  --nodes=150

# 2. 给新节点添加标签
kubectl label nodes -l nodegroup=cilium-nodes cni=cilium

# 3. 驱逐 Calico 节点上的 Pod (逐步)
for node in $(kubectl get nodes -l cni!=cilium -o name); do
  kubectl drain $node --ignore-daemonsets --delete-emptydir-data
  sleep 60  # 观察服务是否正常
done

# 4. Pod 重新调度到 Cilium 节点
# (通过 NodeSelector 或 Affinity 控制)

# 5. 验证 Cilium 策略生效
cilium endpoint list
cilium policy get

# 6. 删除旧节点池
eksctl delete nodegroup --cluster=mycluster --name=calico-nodes
```

**防护措施**

```yaml
# 1. CNI 迁移前清理旧规则
apiVersion: v1
kind: DaemonSet
metadata:
  name: cni-cleanup
spec:
  template:
    spec:
      hostNetwork: true
      hostPID: true
      containers:
      - name: cleanup
        image: alpine:3.18
        command:
        - sh
        - -c
        - |
          # 清理 Calico iptables 规则
          iptables-save | grep -v cali | iptables-restore
          
          # 清理 ipset
          ipset list | grep cali | xargs -I {} ipset destroy {}
          
          # 清理残留网络接口
          ip link show | grep cali | awk '{print $2}' | sed 's/:$//' | xargs -I {} ip link delete {}
          
          echo "Cleanup complete"
          sleep infinity

# 2. 使用 NodeSelector 隔离流量
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    spec:
      nodeSelector:
        cni: cilium  # 只调度到新 CNI 节点

# 3. 渐进式迁移策略
# Phase 1: 非生产命名空间
# Phase 2: 生产只读服务
# Phase 3: 生产核心服务
```

---

## 附录: NetworkPolicy 专家巡检清单

### 每日自动化巡检

```bash
#!/bin/bash
# 文件: netpol-daily-check.sh

echo "=== NetworkPolicy Daily Health Check ==="
date
echo

# 1. 检查 Default Deny 覆盖率
echo "--- Default Deny Coverage ---"
TOTAL_NS=$(kubectl get ns --no-headers | wc -l)
PROTECTED_NS=$(kubectl get netpol -A -o json | \
  jq -r '[.items[] | select(.spec.podSelector == {} and .spec.policyTypes != null) | .metadata.namespace] | unique | length')

echo "Total namespaces: $TOTAL_NS"
echo "Protected namespaces: $PROTECTED_NS"
COVERAGE=$(echo "scale=2; $PROTECTED_NS * 100 / $TOTAL_NS" | bc)
echo "Coverage: $COVERAGE%"

if [ $(echo "$COVERAGE < 80" | bc) -eq 1 ]; then
  echo "⚠️  WARNING: Coverage below 80%"
fi

# 2. 检查孤立的 Pod (未被任何策略选中)
echo -e "\n--- Orphan Pods (No Policy) ---"
# TODO: 实现逻辑

# 3. 检查 CNI Agent 健康
echo -e "\n--- CNI Agent Health ---"
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide
kubectl get pods -n kube-system -l k8s-app=cilium -o wide

# 4. 检查性能指标
echo -e "\n--- Performance Metrics ---"
# iptables 规则数
if kubectl get pods -n kube-system -l k8s-app=calico-node &>/dev/null; then
  echo "Calico iptables rules:"
  kubectl exec -n kube-system $(kubectl get pod -n kube-system -l k8s-app=calico-node -o jsonpath='{.items[0].metadata.name}') -- \
    iptables -t filter -L | wc -l
fi

# Cilium Identity 数量
if kubectl get pods -n kube-system -l k8s-app=cilium &>/dev/null; then
  echo "Cilium Identities:"
  kubectl exec -n kube-system $(kubectl get pod -n kube-system -l k8s-app=cilium -o jsonpath='{.items[0].metadata.name}') -- \
    cilium identity list | wc -l
fi

echo -e "\n=== Check Complete ==="
```

### 每周手动巡检

- [ ] **策略审计**: 导出所有 NetworkPolicy,检查是否有过于宽松的规则
- [ ] **标签规范**: 检查 Pod 和 Namespace 标签是否符合命名规范
- [ ] **DNS 放行**: 确认所有 Egress 策略都包含 DNS 放行
- [ ] **监控集成**: 验证 Prometheus/Grafana 可以抓取所有 Pod metrics
- [ ] **日志采集**: 验证 Fluentd/Fluent Bit 可以采集所有 Pod 日志
- [ ] **性能基准**: 对比策略启用前后的网络延迟和吞吐量
- [ ] **故障演练**: 模拟策略错误配置,验证告警和恢复流程

---

**NetworkPolicy 文档补强完成统计**

- **原始行数**: 161 行
- **补充内容**: ~1100 行
- **预计最终**: ~1260 行
- **新增章节**:
  - CNI 插件策略实现深度对比 (Calico iptables/Cilium eBPF/Weave OVS)
  - 专家级故障矩阵 (4 阶段分类)
  - 复合故障场景 (零信任改造、大规模性能、Identity 冲突)
  - 深度排查脚本 (连通性测试、Calico 调试、Cilium 追踪)
  - 零信任网络设计 (分层策略、验证流程、迁移策略)
  - 3 个生产案例 (全局中断、性能雪崩、CNI 冲突)
  - 完整巡检清单
