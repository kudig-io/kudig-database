# Kubernetes Terway (Aliyun) 全栈进阶培训 (从入门到专家)

> **适用版本**: 阿里云 ACK v1.26 - v1.32 | **Terway 版本**: v1.5+
> **目标受众**: 阿里云开发者、网络架构师、SRE
> **培训时长**: 约 3 小时 (含 Q&A)
> **核心原则**: 理解云原生网络架构、掌握高性能 ENI 策略

---

## 第一阶段：快速入门与核心概念 (45min)

---

### Slide 1: 什么是 Terway

**要点:**
- 阿里云 ACK 自研的 Container Network Interface (CNI) 插件
- 深度集成阿里云 VPC/ENI 网络基础设施
- Pod IP 即 VPC 内网 IP, 无需 NAT 即可被 VPC 内其他资源直接访问
- ACK 集群默认 CNI 方案, 替代早期基于 Flannel 的网络方案

**与 Flannel 的关系:**
- Flannel 是 ACK 早期默认 CNI, 采用 Overlay (VXLAN) 方案
- Terway 在 Pod 直通 VPC、性能、NetworkPolicy 支持上全面优于 Flannel
- 新建 ACK 集群默认安装 Terway; 存量 Flannel 集群可按需迁移
- Flannel 目前仍用于 Windows 节点和不支持 ENI 的场景

**核心价值表:**

| 维度 | Terway | Flannel |
|:---|:---|:---|
| Pod 直通 VPC | 原生支持 | 不支持 (Overlay) |
| 性能损耗 | ~5% (ENI/ENIIP) | ~30% (VXLAN) |
| NetworkPolicy | 原生支持 (L3/L4) | 不支持 |
| SLB/ALB 联动 | 深度集成 | 需额外配置 |
| 安全组联动 | 节点级 + Pod 级 | 仅节点级 |

**Speaker Notes:**
- 开场先确认听众背景: 是否有 Flannel/Calico 使用经验
- 强调 Terway 的核心差异化在于 "Pod 直通 VPC", 这是阿里云场景下的独特优势
- 提到 Terway 以 Apache 2.0 开源: github.com/AliyunContainerService/terway
- 版本线: ACK v1.26+ 默认 Terway v1.3+, ACK v1.30+ 默认 v1.5+

---

### Slide 2: 三种核心模式对比

**推荐默认使用 ENIIP 模式。**

| 模式 | Pod IP 来源 | 网络接口 | 性能 (相对物理机) | 容量密度 | 适用场景 |
|:---|:---|:---|:---:|:---:|:---|
| **VPC 路由** | VPC 路由表条目 | veth pair + Node 网络栈 | ~70% | 低 (48 条路由上限) | 小规模集群、Flannel 迁移过渡 |
| **ENI 独占** | 独占 ENI 主 IP | ENI 直通 | ~95% | 低 (受 ENI 配额限制) | 核心数据库、网关、高性能隔离 |
| **ENIIP** | ENI 辅助 IP (Secondary IP) | veth pair + ENI | ~90% | 高 | 大规模通用场景、微服务、在线业务 |

**扩展模式 (了解即可):**

| 模式 | 性能 | 密度 | 内核要求 | 适用场景 |
|:---|:---:|:---:|:---|:---|
| **ENIIP-Trunking** | ~88% | 最高 (200+ Pod/节点) | 4.19+ | 超大规模、Serverless |
| **IPVlan** | ~95% | 高 | 4.19+ 且 eBPF | 极致性能、低延迟 |

**模式选择决策树:**

```
是否需要 Pod 直通 VPC?
  -- 否 --> 考虑 Flannel / Calico / Cilium
  -- 是
      |-- 节点规模 < 50, Pod 密度 < 30/节点 --> VPC 模式
      |-- 需要极致性能 + 低密度 --> ENI 独占模式
      |-- 通用大规模场景 --> ENIIP 模式 (推荐默认)
      |-- 超大规模 / Serverless --> ENIIP-Trunking
      |-- 极致性能 + 高密度 --> IPVlan (内核 4.19+)
```

**Speaker Notes:**
- ENIIP 是当前 ACK 集群的推荐默认模式, 覆盖 90% 以上的业务场景
- VPC 模式受限于 VPC 路由表条目数 (默认 48 条), 不适合大规模集群
- ENI 独占模式给每个 Pod 分配一整张 ENI, 性能最好但密度最低
- 两种扩展模式 (Trunking/IPVlan) 需要内核 4.19+, 后面第三阶段会详细讲

---

### Slide 3: Pod 直通 VPC 的意义

**网络拓扑简化:**
- Pod IP = VPC IP, VPC 内所有资源 (ECS、RDS、SLB) 可直接访问 Pod
- 消除 NAT 转发和 Overlay 封装, 网络路径从 Pod 直达 VPC 网关
- 排障时直接 ping Pod IP, 不需要穿透 Overlay 层

**安全组联动:**
- 节点级安全组: 所有 Pod 共享节点安全组, 统一管理出入站规则
- Pod 级安全组: 每个 Pod 可绑定独立安全组, 实现精细化访问控制
- 安全组规则直接作用于 VPC 网络平面, 无额外策略引擎开销

**SLB/ALB 集成:**
- LoadBalancer 类型 Service 自动关联阿里云 SLB/ALB
- SLB 后端直接挂载 Pod IP (ENIIP 模式), 无需经过 NodePort 转发
- 流量路径: Client --> SLB --> Pod IP (直通), 减少一跳

**Speaker Notes:**
- 核心价值一句话: "Pod 就是一台 VPC 内的虚拟机, 拥有独立 IP 和安全组"
- 与 Flannel 对比: Flannel 的 Pod IP 是 Overlay 网络地址, VPC 内不可路由, 需要 NAT 才能访问外部
- SLB 集成是实际业务中最常见的收益点: 传统 CNI 需要 NodePort 中转, Terway 直接挂 Pod IP
- 安全组联动在金融/政务场景下特别重要, 合规要求精细化网络隔离

---

### Slide 4: 快速验证 Terway 状态

**步骤 1: 确认 Terway DaemonSet 运行状态**

```bash
kubectl -n kube-system get ds terway-eniip -o wide
```

预期输出: 每个 Node 上运行一个 terway-eniip Pod, 状态为 Running

**步骤 2: 查看 eni-config ConfigMap**

```bash
kubectl -n kube-system get cm eni-config -o yaml
```

关键字段: `eni_type` (网络模式), `vswitches` (vSwitch ID), `security_group`

**步骤 3: 查看 Pod IP 和 ENI 信息**

```bash
kubectl get pods -o wide --all-namespaces | head -20
```

验证: Pod IP 属于 VPC CIDR 段 (如 192.168.x.x)

**步骤 4: 检查 Node 网络资源注解**

```bash
kubectl get node <node-name> -o jsonpath='{.metadata.annotations}' | jq .
```

关注: `alpha.kubernetes.io/provided-node-ip`, Terway 相关注解

**步骤 5: 查看 Terway CRD 资源**

```bash
kubectl get podeni -A
kubectl get nodenetworking -A
```

**Speaker Notes:**
- 这个环节建议现场 live demo, 让学员跟着操作
- 如果没有实验环境, 可以展示截图
- 重点检查: (1) DaemonSet 是否在所有节点 Running (2) Pod IP 是否在 VPC CIDR 内 (3) eni-config 配置是否正确
- 常见问题: 某个节点 terway-eniip 不在 Running 状态, 通常是因为 OpenAPI 调用失败或 RAM 权限不足

---

## 第二阶段：核心架构与深度原理 (60min)

---

### Slide 5: Terway 整体架构

**控制面 (Control Plane):**

| 组件 | 形态 | 职责 |
|:---|:---|:---|
| **Terway DaemonSet** | DaemonSet (每 Node 一个 Pod) | 运行 CNI 插件二进制, 执行 IPAM, 管理 ENI/IP 资源池, 处理 CNI ADD/DEL/CHECK |
| **Terway Controller** | Deployment (1 副本, 可选 HA) | Watch CRD 变更, 管理 ENI 生命周期, 节点资源协调, 垃圾回收, 状态同步 |
| **eni-config ConfigMap** | ConfigMap | 全局网络配置: VPC ID, vSwitch ID 列表, 安全组, 网络模式, IP 池大小 |

**数据面 (Data Plane):**

| 资源 | 来源 | 说明 |
|:---|:---|:---|
| **ENI (弹性网卡)** | 阿里云 ECS ENI | Pod 接入 VPC 的网络接口载体 |
| **ENIIP (辅助 IP)** | ENI Secondary IP | ENIIP 模式下 Pod 使用的 VPC IP 地址 |
| **veth pair** | Linux 网络设备 | 连接 Pod 网络命名空间与 ENI 的虚拟网线 |

**交互流程:**
```
kubelet --> CNI ADD --> terway-cni (binary)
                        |
                        v
              terway-daemon (gRPC)
                        |
                  +-----+-----+
                  |           |
            本地 IP 池    OpenAPI (阿里云)
            (命中直接分配)  (未命中则申请)
                  |           |
                  +-----+-----+
                        |
                        v
                  分配 IP --> 创建 veth pair --> Pod 网络就绪
```

**Speaker Notes:**
- DaemonSet 以 hostNetwork: true 方式运行, 确保 Node 网络栈可用于 ENI 管理
- Controller 是集群级别协调器, 负责跨节点的 ENI 生命周期管理和垃圾回收
- eni-config 是所有 Terway 组件启动时读取的配置源, 修改后需要滚动重启 DaemonSet
- DaemonSet 内部包含三个关键进程: terway-agent (gRPC/IPAM)、terway-daemon (ENI 管理)、terway-cni (二进制)

---

### Slide 6: ENIIP 模式数据流

**Pod 到 VPC 的数据路径:**

```
┌─────────────────────────────────────────────────────────────────┐
│                          ECS 节点                                │
│                                                                  │
│  ┌──────────────┐     ┌─────────────────────────────────────┐   │
│  │   Pod (netns)│     │           主网络命名空间              │   │
│  │              │     │                                      │   │
│  │  eth0        │     │  vethXXXX (host 侧)                  │   │
│  │ 192.168.0.10 │────>│     |                                │   │
│  │              │     │     v                                │   │
│  └──────────────┘     │  路由规则 (policy routing)           │   │
│                       │     |                                 │   │
│                       │     v                                 │   │
│                       │  ENI-1 (辅助 ENI)                     │   │
│                       │  主 IP: 192.168.0.254                 │   │
│                       │  辅助 IP: 192.168.0.10 (Pod A)       │   │
│                       │  辅助 IP: 192.168.0.11 (Pod B)       │   │
│                       │  辅助 IP: 192.168.0.12 (Pod C)       │   │
│                       └──────────┬──────────────────────────┘   │
│                                  │                               │
└──────────────────────────────────┼───────────────────────────────┘
                                   │
                                   v
                        ┌─────────────────────┐
                        │   阿里云 VPC 网络平面  │
                        │   (交换机 / 网关)     │
                        └─────────────────────┘
```

**关键机制:**
1. Pod 创建时, terway-cni 通过 gRPC 调用 terway-daemon 请求 IP
2. terway-daemon 从本地 IP 池分配一个 ENIIP, 创建 veth pair
3. veth pair 一端放入 Pod netns (命名为 eth0), 另一端留在主机
4. 配置策略路由: 将源 IP 为 Pod IP 的流量指向对应的 ENI
5. 出站: Pod --> veth pair --> 策略路由 --> ENI --> VPC
6. 入站: VPC --> ENI --> 辅助 IP --> veth pair --> Pod

**Speaker Notes:**
- 这是整个培训最核心的一张图, 建议画在白板上逐步讲解
- 关键点: 策略路由 (policy routing) 是 ENIIP 模式的核心, 确保不同 Pod 的流量走正确的 ENI
- 为什么需要策略路由: 一个节点可能有多张辅助 ENI, 每个 Pod 绑定在不同 ENI 的辅助 IP 上
- veth pair 是 Linux 内核级的虚拟网线, 性能损耗约 5-10%, IPVlan 模式可以绕过 veth
- 一个 ENI 的辅助 IP 数由 ECS 实例规格决定, 例如 ecs.g7.4xlarge 单 ENI 最多 30 个辅助 IP

---

### Slide 7: IPAM 机制

**IP 分配流程:**

```
Pod 创建请求 (kubelet)
        |
        v
  CNI ADD 调用
        |
        v
  terway-cni binary
        |
        v
  gRPC 请求 --> terway-daemon (IPAM 服务)
        |
        v
  本地 IP 池检查
        |
   +----+----+
   |         |
  命中      未命中
   |         |
   v         v
 直接分配   调用 OpenAPI
            AssignPrivateIpAddresses
            为 ENI 分配新的辅助 IP
               |
               v
            放入本地 IP 池后分配
        |
        v
  创建 veth pair + 策略路由
        |
        v
  Pod 网络就绪
```

**IP 释放流程:**

```
Pod 删除请求 (kubelet)
        |
        v
  CNI DEL 调用
        |
        v
  terway-cni binary
        |
        v
  gRPC 请求 --> terway-daemon
        |
        v
  回收 IP --> 放回本地 IP 池 (预热池)
        |
   +----+----+
   |         |
  池满      池未满
   |         |
   v         v
 调用 OpenAPI    保留在池中
 UnassignPrivate   供下一个 Pod 复用
 IpAddresses
```

**IPAM 关键参数:**

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `max_pool_size` | 5 | 本地 IP 预热池最大容量 |
| `min_pool_size` | 0 | 本地 IP 预热池最小容量 |
| `max_ip_per_eni` | 取决于实例规格 | 每个 ENI 可分配的辅助 IP 上限 |

**Speaker Notes:**
- IP 预热池是 Terway 性能优化的关键设计: 提前分配好 IP 放入池中, Pod 创建时直接命中, 减少一次 OpenAPI 调用
- OpenAPI 调用延迟通常 50-200ms, 预热池命中时 IP 分配延迟 < 5ms
- 预热池大小建议设置为 5-10, 覆盖一般的突发扩容场景
- IP 释放时不立即调用 OpenAPI 归还, 而是放回预热池, 这是为了减少 OpenAPI 调用次数
- OpenAPI 速率限制约 100 QPS, 大规模扩容时可能成为瓶颈

---

### Slide 8: CRD 资源模型

**Terway 定义的 CRD 资源:**

| CRD | 作用 | 关键字段 |
|:---|:---|:---|
| **PodENI** | 记录 Pod 与 ENI/IP 的绑定关系 | `status.eniID`, `status.ipv4Addr`, `status.phase` |
| **NodeNetworking** | 记录节点的 ENI 资源清单 | `status.eniInfos[]`, `status.nodeIP` |
| **PodNetworking** | 定义 Pod 级网络配置模板 (安全组、vSwitch、带宽) | `spec.securityGroupIDs`, `spec.vSwitchIDs`, `spec.eniType` |
| **ReservedIP** | 保留固定 IP 地址, 用于 StatefulSet 固定 IP 场景 | `spec.ipAddress`, `spec.networkInterfaceID` |
| **IPInstance** | 记录每个 IP 实例的生命周期状态 | `status.pod`, `status.node`, `status.phase` |

**CRD 关联关系:**

```
PodNetworking (网络模板)
      |
      | 引用
      v
PodENI (Pod-ENI 绑定)
      |
      | 关联
      v
NodeNetworking (节点资源)
      |
      | 记录
      v
IPInstance (IP 实例状态)
      |
      | 可能关联
      v
ReservedIP (固定 IP)
```

**CRD 查询命令:**

```bash
kubectl get podeni -A
kubectl get nodenetworking -A
kubectl get podnetworking -A
kubectl get reservedip -A
kubectl get ipinstance -A
```

**Speaker Notes:**
- CRD 是 Terway 声明式管理的核心: 所有网络资源状态通过 CRD 暴露, 方便查询和排障
- PodENI 是排障中最常用的 CRD: 查看 Pod 绑定了哪个 ENI、分配了什么 IP、当前状态
- NodeNetworking 反映节点维度的 ENI 资源使用情况, 用于容量评估
- PodNetworking 允许为不同业务配置不同的网络策略 (安全组、vSwitch), v1.3+ 支持
- ReservedIP 配合 StatefulSet 使用, 实现 Pod 重建后 IP 不变

---

### Slide 9: 安全模型四层体系

**第一层: 节点安全组 (Node Security Group)**

- 所有 Pod 默认共享节点绑定的安全组
- 在 eni-config ConfigMap 中配置: `"security_group": "sg-2zexxxxx"`
- 管粒度粗, 适合统一出站/入站策略

**第二层: Pod 安全组 (Pod Security Group)**

- 通过 PodNetworking CRD 为特定 Pod 绑定独立安全组
- 实现精细化访问控制: 不同业务 Pod 使用不同安全组
- 要求: ENI 或 ENIIP 模式

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: db-pod-net
spec:
  securityGroupIDs:
    - sg-db-xxxxx
  vSwitchIDs:
    - vsw-2zexxxxx
  eniType: eniip
```

**第三层: NetworkPolicy (Kubernetes 原生)**

- Terway 完整实现 Kubernetes NetworkPolicy API
- 支持 L3 (IP/CIDR) 和 L4 (TCP/UDP/SCTP 端口) 策略
- 数据面实现: iptables (默认) 或 eBPF (v1.5+ 可选)
- 作用于 Pod 粒度, 命名空间级别

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: production
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
```

**第四层: RAM 权限控制**

- Terway 通过 ECS 实例 RAM Role 获取云资源操作权限
- 最小权限: ENI CRUD + IP 分配释放 + 查询实例/网卡
- 防止权限泄露: 不使用 AK/SK 硬编码, 使用临时凭证

**Speaker Notes:**
- 四层安全模型从粗到细: 节点安全组 (最粗) --> Pod 安全组 --> NetworkPolicy --> RAM (最细)
- 实际生产中, 节点安全组 + NetworkPolicy 组合使用最常见
- Pod 安全组适用于有严格网络隔离要求的场景 (如金融: 交易 Pod 和结算 Pod 不同的安全组)
- NetworkPolicy 的 iptables 实现在大规模集群 (1000+ NetworkPolicy) 下可能有性能问题, eBPF 方案可缓解
- RAM 权限不足是 Terway 部署失败的常见原因, 下一阶段排障部分会详细讲

---

## 第三阶段：生产部署与优化 (45min)

---

### Slide 10: 网络模式选型决策矩阵

**决策矩阵:**

| 决策维度 | VPC | ENI 独占 | ENIIP | ENIIP-Trunking | IPVlan |
|:---|:---:|:---:|:---:|:---:|:---:|
| 集群规模 < 100 节点 | OK | OK | 推荐 | 不必要 | 不必要 |
| 集群规模 100-1000 节点 | 不推荐 | 不推荐 | 推荐 | OK | OK |
| 集群规模 > 1000 节点 | 不可用 | 不可用 | OK | 推荐 | 推荐 |
| Pod 密度 < 30/节点 | OK | OK | 推荐 | 不必要 | 不必要 |
| Pod 密度 30-100/节点 | 不推荐 | 不推荐 | 推荐 | OK | OK |
| Pod 密度 > 100/节点 | 不可用 | 不可用 | OK | 推荐 | 推荐 |
| 网络延迟敏感 (< 0.1ms) | 不推荐 | 推荐 | OK | OK | 推荐 |
| 吞吐量敏感 (> 10Gbps) | 不推荐 | 推荐 | OK | OK | 推荐 |
| 需要 NetworkPolicy | 不推荐 | OK | 推荐 | 推荐 | 推荐 |
| 内核 < 4.19 | OK | OK | 推荐 | 不可用 | 不可用 |
| 混合 Linux/Windows | 不适用 | 不适用 | 仅 Linux | 仅 Linux | 仅 Linux |

**选型结论:**
- 默认推荐 ENIIP, 覆盖绝大多数场景
- 核心数据库/网关用 ENI 独占
- 超大规模/高密度用 ENIIP-Trunking
- 极致性能用 IPVlan

**Speaker Notes:**
- 实际项目中 90% 的场景选择 ENIIP 即可
- 决策矩阵建议打印给学员, 作为选型 checklist
- Trunking 和 IPVlan 对内核版本有要求, 选型前务必确认 ECS 实例的 OS 镜像内核版本
- VPC 模式只适合迁移过渡, 新集群不建议使用

---

### Slide 11: 容量规划

**ECS 规格与 Pod 容量计算:**

```
ENIIP 模式单节点最大 Pod 数 = (最大 ENI 数 - 1) * 单 ENI 最大辅助 IP 数
                              ^
                    保留一块 ENI 供节点自身使用

示例:
  ecs.g7.4xlarge (16C64G)
  最大 ENI: 8, 单 ENI 最大辅助 IP: 30
  最大 Pod 数 = (8 - 1) * 30 = 210
  实际推荐预留 10-20% 余量 --> 建议 170-190 Pod/节点
```

**常用规格速查:**

| ECS 规格 | 最大 ENI | 单 ENI 辅助 IP | 理论最大 Pod | 推荐上限 (80%) |
|:---|:---:|:---:|:---:|:---:|
| ecs.g7.xlarge (4C16G) | 4 | 10 | 30 | 24 |
| ecs.g7.2xlarge (8C32G) | 6 | 15 | 75 | 60 |
| ecs.g7.4xlarge (16C64G) | 8 | 30 | 210 | 168 |
| ecs.g7.8xlarge (32C128G) | 16 | 30 | 450 | 360 |

**vSwitch CIDR 规划:**

```
vSwitch CIDR 规划公式:
  所需 IP 数 = 集群最大节点数 * 单节点最大 Pod 数 + 预留 (20%)

示例:
  100 节点集群, ecs.g7.4xlarge, 单节点最大 210 Pod
  所需 IP = 100 * 210 * 1.2 = 25,200
  需要 /18 (16,384) 不够, /17 (32,768) 足够

推荐 CIDR:
  小型集群 (50 节点以内):  /20 (4,096 IP)
  中型集群 (50-200 节点):  /18 (16,384 IP)
  大型集群 (200-500 节点): /17 (32,768 IP)
  超大集群 (500+ 节点):    /16 (65,536 IP) 或更大
```

**多可用区部署建议:**
- 每个可用区创建独立 vSwitch, 在 eni-config 中配置多 vSwitch
- vSwitch CIDR 不重叠, 统一在 VPC CIDR 内划分
- 推荐至少 2 个可用区, 实现跨可用区高可用

**Speaker Notes:**
- 容量规划是生产部署的第一步, 规划不足会导致 IP 耗尽、无法创建 Pod
- 公式中的 "-1" 是因为主 ENI 用于节点自身通信, 不能分配给 Pod
- 实际推荐不超过理论值的 80%, 剩余 20% 用于突发扩容和 IPAM 缓冲
- vSwitch 创建后 CIDR 不可修改, 建议初始规划时适当放大
- 可用区间的 vSwitch IP 用量可能不均衡, 建议按最大可用区节点数规划每个 vSwitch

---

### Slide 12: NetworkPolicy 实战

**iptables vs Cilium eBPF 对比:**

| 维度 | iptables (Terway 默认) | eBPF (Terway v1.5+ 可选) |
|:---|:---|:---|
| 规则更新延迟 | 秒级 (iptables-restore) | 毫秒级 (BPF map update) |
| 大规模策略性能 | 1000+ 规则时下降明显 | 规模无关 (map 查找 O(1)) |
| 内核版本要求 | 无特殊要求 | 4.19+ 推荐 5.10+ |
| 调试难度 | iptables -L -n -v 可查 | bpftool 可查 |
| 成熟度 | 生产验证充分 | 较新, 建议灰度验证 |

**示例策略: 命名空间隔离 + 白名单**

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-monitoring
  namespace: production
spec:
  podSelector: {}
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: monitoring
      ports:
        - protocol: TCP
          port: 9090
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-api-to-db
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: postgres
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: api-server
      ports:
        - protocol: TCP
          port: 5432
```

**排障命令:**

```bash
kubectl get networkpolicy -A
kubectl describe networkpolicy <name> -n <ns>
iptables -L -n -v | grep <pod-ip>
```

**Speaker Notes:**
- 生产环境推荐先部署 default-deny-all, 再逐步放通白名单
- iptables 模式下, 大量 NetworkPolicy 会导致规则膨胀, 影响新建连接性能
- 如果集群 NetworkPolicy 数量超过 500, 建议评估 eBPF 模式
- NetworkPolicy 仅支持 L3/L4, 不支持 HTTP 路径级别的策略 (需 Cilium)
- 调试 NetworkPolicy 时, 先确认策略是否生效 (iptables 规则), 再检查安全组是否冲突

---

### Slide 13: 固定 IP (StatefulSet)

**使用场景:**
- 数据库主从切换后需要保持 IP 不变
- 传统应用硬编码 IP 地址
- 防火墙/白名单基于 IP 授权

**配置步骤:**

步骤 1: 创建 PodNetworking (指定安全组和 vSwitch)

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: fixed-ip-net
spec:
  securityGroupIDs:
    - sg-2zexxxxx
  vSwitchIDs:
    - vsw-2zexxxxx
  eniType: eniip
```

步骤 2: StatefulSet 添加注解

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
spec:
  replicas: 3
  template:
    metadata:
      annotations:
        k8s.v1.cni.cncf.io/networks: fixed-ip-net
    spec:
      containers:
        - name: postgres
          image: postgres:15
```

步骤 3: 验证固定 IP

```bash
kubectl get reservedip -A
kubectl get podeni -A | grep postgres
```

删除 Pod 后重建, IP 保持不变。

**注意事项:**
- 仅支持 StatefulSet, Deployment 的 Pod 重建后 IP 会变化
- 固定 IP 会占用 vSwitch IP 池, 需纳入容量规划
- 节点下线时固定 IP 的 Pod 迁移到新节点后 IP 保持不变 (跨节点固定)

**Speaker Notes:**
- 固定 IP 是 Terway v1.3+ 的特性, 低版本不支持
- 底层实现: 创建 ReservedIP CRD 记录 IP 与 StatefulSet 的绑定关系
- 固定 IP 的 Pod 删除后, IP 不会释放回 vSwitch, 而是标记为 Reserved
- 生产环境中, 固定 IP 通常用于数据库 (MySQL/Redis/PostgreSQL) 和传统中间件
- 注意固定 IP 数量计入 vSwitch 总 IP 容量

---

### Slide 14: GC 机制与资源回收

**GC 触发时机:**

| 触发条件 | 说明 |
|:---|:---|
| Pod 删除 | 回收 ENIIP, 放回预热池或调用 OpenAPI 释放 |
| 节点下线 | 回收该节点上所有 ENI 资源 |
| CRD 状态不一致 | PodENI 记录的 Pod 已不存在, 但 ENIIP 未释放 |
| 定期巡检 | Controller 默认每 5 分钟执行一次全量 GC |

**GC 工作流程:**

```
Terway Controller 定期巡检
        |
        v
  遍历所有 PodENI / IPInstance
        |
        v
  检查关联的 Pod 是否存在
        |
   +----+----+
   |         |
  存在      不存在
   |         |
  跳过      标记为孤儿资源
              |
              v
        执行 GC 策略
              |
        +-----+-----+
        |           |
    固定 IP      普通 IP
        |           |
     保留 IP      释放 IP
        |           |
     (ReservedIP)  调用 OpenAPI
                    UnassignPrivateIpAddresses
                    或 DeleteNetworkInterface
```

**关键调优参数:**

| 参数 | 位置 | 说明 |
|:---|:---|:---|
| `--gc-interval` | Controller 启动参数 | GC 巡检间隔, 默认 5 分钟 |
| `--ip-reclaim-duration` | Controller 启动参数 | IP 被标记为孤儿后的保留时间, 默认 5 分钟 |
| `max_pool_size` | eni-config | 本地预热池大小, 影响回收后 IP 的去向 |

**常见 GC 问题:**
- IP 泄漏: GC 未及时回收, vSwitch IP 逐渐耗尽
- 解决: 缩短 gc-interval 或手动触发 GC
- ENI 残留: 节点被强制删除后 ENI 未释放
- 解决: Controller 下一次巡检会自动清理, 或手动 `kubectl delete podeni <name>`

**Speaker Notes:**
- GC 机制是 v1.4 增强的重点, 之前版本 IP 泄漏问题比较严重
- 如果发现 vSwitch IP 被占用但找不到对应 Pod, 先查 PodENI 和 IPInstance CRD
- 生产环境建议将 gc-interval 设置为 2-3 分钟, ip-reclaim-duration 设置为 3 分钟
- 手动紧急回收: `kubectl delete podeni <name>` 会触发 Controller 回收关联的 ENI 资源
- 监控指标: `terway_gc_total`, `terway_gc_errors` 用于观察 GC 运行状态

---

## 第四阶段：排障与 SRE 运维 (30min)

---

### Slide 15: 10 分钟快速诊断

**5 步排障流程:**

**步骤 1: 确认 Pod 状态 (1 分钟)**

```bash
kubectl get pod <pod-name> -n <ns> -o wide
kubectl describe pod <pod-name> -n <ns>
```

关注: ContainerCreating 卡住、NetworkPluginNotReady、FailedCreate

**步骤 2: 检查 Terway DaemonSet (2 分钟)**

```bash
kubectl -n kube-system get ds terway-eniip
kubectl -n kube-system logs terway-eniip-xxxxx --tail=100
```

关注: OpenAPI 调用失败、IP 分配错误、ENI 配额不足

**步骤 3: 检查 IP 资源 (2 分钟)**

```bash
kubectl get podeni -A | grep <node>
kubectl get nodenetworking <node> -o yaml
```

关注: IP 池耗尽、ENI 配额达到上限、vSwitch IP 不足

**步骤 4: 检查网络连通性 (3 分钟)**

```bash
kubectl exec -it <pod-a> -- ping <pod-b-ip>
kubectl exec -it <pod-a> -- curl <service-ip>:<port>
kubectl exec -it <pod-a> -- traceroute <pod-b-ip>
```

关注: 跨节点不通、同节点不通、DNS 解析失败

**步骤 5: 检查安全组和 NetworkPolicy (2 分钟)**

```bash
kubectl get networkpolicy -A
iptables -L -n -v | grep <pod-ip>
```

关注: 安全组规则阻断、NetworkPolicy 误拦截

**Speaker Notes:**
- 这个 5 步流程覆盖了 80% 的 Terway 网络故障
- 最常见的问题: IP 耗尽 (步骤 3) 和 OpenAPI 限流 (步骤 2 日志)
- 建议学员在实验环境中模拟每种故障, 建立肌肉记忆
- 步骤 2 的日志是排障的金矿: 几乎所有 Terway 错误都会在日志中体现

---

### Slide 16: 常见故障与处理

**故障 1: IP 耗尽 (VSwitch IP 不足)**

| 症状 | 原因 | 处理 |
|:---|:---|:---|
| Pod 卡在 ContainerCreating | vSwitch CIDR 内所有 IP 已分配 | 扩展 vSwitch CIDR 或新增 vSwitch |
| terway 日志: `NoAvailableIP` | IPAM 本地池和 OpenAPI 均无可用 IP | 释放未使用的固定 IP 或清理泄漏 IP |

紧急处理:
```bash
kubectl get ipinstance -A | grep -v Running
kubectl delete ipinstance <leaked-ip-instance>
```

**故障 2: ENI 配额不足**

| 症状 | 原因 | 处理 |
|:---|:---|:---|
| terway 日志: `ENIQuotaExceeded` | ECS 实例 ENI 数达到上限 | 升级 ECS 规格 (更大规格更多 ENI) |
| 部分 Pod 无法获得 IP | 新 Pod 需要新 ENI 但已达上限 | 切换到 ENIIP 模式 (单 ENI 多 IP) |

检查配额:
```bash
kubectl get nodenetworking <node> -o yaml | grep -A 20 eniInfos
```

**故障 3: 跨节点 Pod 不通**

| 症状 | 原因 | 处理 |
|:---|:---|:---|
| Pod A ping Pod B 超时 | 安全组未放通对方 CIDR | 检查并更新安全组规则 |
| 仅特定端口不通 | NetworkPolicy 拦截 | 检查 NetworkPolicy 规则 |
| 部分节点间不通 | vSwitch 路由未配置 | 检查 VPC 路由表条目 |

**故障 4: OpenAPI 限流**

| 症状 | 原因 | 处理 |
|:---|:---|:---|
| terway 日志: `Throttling` | OpenAPI QPS 超限 (默认 100 QPS) | 申请提升 API 配额 |
| Pod 创建缓慢 (数秒) | IP 分配等待 OpenAPI 重试 | 增大预热池 (max_pool_size) |

**Speaker Notes:**
- IP 耗尽是最常见也是最紧急的故障, 建议提前配置告警
- ENI 配额不足通常是因为选了过小的 ECS 规格, 容量规划阶段就要避免
- 跨节点不通的排障关键: 先确认安全组, 再确认 NetworkPolicy, 最后检查 VPC 路由
- OpenAPI 限流在大规模扩容时容易触发, 预热池是最有效的缓解手段

---

### Slide 17: 监控告警配置

**核心监控指标:**

| 指标名 | 类型 | 说明 |
|:---|:---|:---|
| `terway_alloc_ip_duration_ms` | Histogram | IP 分配延迟, 反映 OpenAPI 响应速度 |
| `terway_ip_pool_size` | Gauge | 本地预热池当前 IP 数 |
| `terway_eni_count` | Gauge | 当前节点 ENI 数量 |
| `terway_gc_total` | Counter | GC 执行次数 |
| `terway_gc_errors` | Counter | GC 执行失败次数 |
| `terway_openapi_calls_total` | Counter | OpenAPI 调用次数 |
| `terway_openapi_errors_total` | Counter | OpenAPI 调用失败次数 |

**PrometheusRule 告警示例:**

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-alerts
  namespace: kube-system
spec:
  groups:
    - name: terway.rules
      rules:
        - alert: TerwayIPAllocSlow
          expr: histogram_quantile(0.95, rate(terway_alloc_ip_duration_ms_bucket[5m])) > 500
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Terway IP 分配延迟过高"
            description: "节点 {{ $labels.node }} IP 分配 P95 延迟超过 500ms"

        - alert: TerwayIPPoolExhausted
          expr: terway_ip_pool_size == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Terway IP 预热池耗尽"
            description: "节点 {{ $labels.node }} IP 预热池为空"

        - alert: TerwayOpenAPIErrors
          expr: rate(terway_openapi_errors_total[5m]) > 0.1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Terway OpenAPI 调用错误率过高"
            description: "节点 {{ $labels.node }} OpenAPI 错误率超过 10%"

        - alert: TerwayGCFailure
          expr: rate(terway_gc_errors[10m]) > 0
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Terway GC 执行失败"
            description: "节点 {{ $labels.node }} GC 持续失败"
```

**Grafana Dashboard 建议:**
- Panel 1: 各节点 IP 分配延迟 P50/P95/P99
- Panel 2: 各节点预热池 IP 数量趋势
- Panel 3: OpenAPI 调用量和错误率
- Panel 4: ENI 使用率 (已用/配额)
- Panel 5: GC 执行频率和失败率

**Speaker Notes:**
- 上述 PrometheusRule 可直接复制到生产环境使用
- TerwayIPAllocSlow 和 TerwayIPPoolExhausted 是最重要的两个告警
- Grafana Dashboard 建议按节点维度展示, 方便定位问题节点
- 如果使用阿里云 ARMS/Prometheus, 可以直接导入预置的 Terway 监控大盘

---

### Slide 18: SRE 运维红线

**红线 1: IP 资源规划必须预留 20% 余量**

严禁在 vSwitch IP 使用率超过 80% 时继续扩容集群。必须提前规划下一轮 vSwitch 扩容。

**红线 2: 高并发业务必须评估 ENI 规格**

ECS 实例规格直接决定 Pod 密度上限。禁止使用 ecs.u1 等经济型规格承载高密度业务 (ENI 数少, 辅助 IP 少)。

**红线 3: 核心数据库/网关必须使用 ENI 独占或 IPVlan 模式**

核心数据库 (MySQL/Redis) 和网关 (Nginx/Envoy) 对网络性能敏感, 必须使用高性能模式。

**红线 4: 生产环境必须配置 Terway 监控告警**

至少配置 IP 分配延迟和预热池耗尽两个告警规则。禁止无监控运行 Terway 集群。

**红线 5: 严禁直接修改 eni-config 后不滚动重启 DaemonSet**

eni-config 修改后必须执行 `kubectl rollout restart ds terway-eniip -n kube-system`, 否则配置不生效。

**红线 6: 跨 VPC 场景必须通过 CEN 打通**

Pod IP 仅在本 VPC 内可路由。跨 VPC 访问 Pod IP 必须通过 CEN (云企业网) 或 VPN 网关, 禁止依赖公网 NAT。

**红线 7: NetworkPolicy 变更必须经过测试环境验证**

NetworkPolicy 误配可能导致业务大面积不可用。所有策略变更必须在测试环境验证后再上线。

**Speaker Notes:**
- 红线 1-2 是容量相关, 是 Terway 生产环境出问题的最大根源
- 红线 3 是性能相关, 核心业务不能在 ENIIP 模式下妥协
- 红线 4 是可观测性, 没有监控的 Terway 集群等于盲飞
- 红线 5 是操作规范, eni-config 修改后不重启是最常见的"配了不生效"问题
- 红线 6-7 是架构相关, 跨 VPC 和 NetworkPolicy 变更需要格外小心
- 建议将红线清单纳入团队 Onboarding 文档和 Change Management 流程

---

## 附录

---

### 推荐阅读

**topic-terway/ 专题文档 (由浅入深):**

| 序号 | 文件路径 | 内容说明 |
|:---|:---|:---|
| 1 | `topic-terway/01-product.md` | 产品概览: 定位、版本历史、模式总览、与其他 CNI 对比 |
| 2 | `topic-terway/02-architecture.md` | 架构原理: 控制面/数据面详解、IPAM、CRD 模型、安全模型 |
| 3 | `topic-terway/03-usage.md` | 使用指南: 安装配置、模式切换、NetworkPolicy、固定 IP |
| 4 | `topic-terway/03b-crd-operations.md` | CRD 操作: PodENI/NodeNetworking/ReservedIP CRUD 实战 |
| 5 | `topic-terway/04-operations.md` | 运维手册: 健康检查、GC 机制、升级策略、故障排查 |
| 6 | `topic-terway/05-testing.md` | 测试验证: 网络连通性、NetworkPolicy 测试、ENI 配额验证 |
| 7 | `topic-terway/06-performance.md` | 性能调优: 模式对比、内核调优、基准测试 |
| 8 | `topic-terway/07-troubleshooting-fta.md` | 故障树分析: 结构化排障方法 |

**domain-5-networking/ 通用网络知识:**

| 序号 | 文件路径 | 内容说明 |
|:---|:---|:---|
| 1 | `domain-5-networking/05-terway-advanced-guide.md` | Terway 高级指南: 模式对比、ENIIP 详解、容量规划 |
| 2 | `domain-5-networking/37-terway-resources-crud-operations.md` | Terway CRD 资源 CRUD 操作指南 |
| 3 | `domain-5-networking/38-terway-gc-mechanism.md` | Terway GC 垃圾回收机制详解 |

---

### 实验环境准备清单

**阿里云资源准备:**

| 资源 | 规格/配置 | 数量 | 说明 |
|:---|:---|:---:|:---|
| ACK 集群 | 托管版, Kubernetes v1.30+ | 1 | 开通时选择 Terway 网络插件 |
| ECS 节点 | ecs.g7.xlarge (4C16G) | 3+ | 第七代实例, ENI 配额充足 |
| VPC | CIDR: 192.168.0.0/16 | 1 | Pod 和节点共用 VPC |
| vSwitch | 可用区 A: 192.168.0.0/20 | 1 | 至少 /20, 预留足够 IP |
| 安全组 | 默认放通内网 | 1 | 测试环境可适当放宽 |
| RAM 角色 | ECS 实例角色, Terway 最小权限 | - | 参见 01-product.md 第 5 节 |

**客户端工具:**

| 工具 | 版本要求 | 说明 |
|:---|:---|:---|
| kubectl | v1.30+ | 与集群版本匹配 |
| jq | 最新版 | 解析 JSON 输出 |
| tcpdump | 最新版 | 网络抓包排障 |
| ping / traceroute | 系统自带 | 网络连通性测试 |

**实验练习:**

1. 确认 Terway 状态和 Pod IP (Slide 4)
2. 部署 NetworkPolicy 实现命名空间隔离 (Slide 12)
3. 创建 StatefulSet 并验证固定 IP (Slide 13)
4. 模拟 IP 耗尽故障并处理 (Slide 16)
5. 配置 PrometheusRule 告警 (Slide 17)

---

### Q&A 提问引导

**入门级问题:**
- Terway 和 Flannel 的核心区别是什么?
- ENIIP 模式下, Pod 的 IP 从哪里来?
- 如何查看当前集群使用的 Terway 网络模式?

**进阶级问题:**
- 为什么 ENIIP 模式需要策略路由 (policy routing)?
- IP 预热池的设计目的是什么? 池大小应该如何设置?
- NetworkPolicy 的 iptables 实现和 eBPF 实现各有什么优劣?

**专家级问题:**
- 跨 VPC 场景下, Pod 固定 IP 如何实现?
- 如何设计一个支持 5000 节点的 Terway 集群容量规划?
- Terway GC 机制在什么场景下可能出现 IP 泄漏? 如何快速定位和修复?

---

> 交叉引用: 本培训内容深度关联 `topic-terway/` 专题 (8 个文件) 和 `domain-5-networking/` 网络知识库 (3 个 Terway 相关文件)。建议培训前完成 `topic-terway/01-product.md` 和 `02-architecture.md` 的预读。

---

**Kusheet Project** | 作者: Allen Galler (allengaller@gmail.com)
