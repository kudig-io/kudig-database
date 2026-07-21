---
title: 27 - 节点与节点池管理 (Node & NodePool Management)
description: '# 27 - 节点与节点池管理 (Node & NodePool Management)'
summary: '# 27 - 节点与节点池管理 (Node & NodePool Management)'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- kubelet
- hpa
- vpa
- pdb
- daemonset
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 节点与节点池管理 (Node & NodePool Management) 是什么
- 如何 节点与节点池管理 (Node & NodePool Management)
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 节点与节点池管理
- Node
- NodePool
- Management
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/topic-fta/list/node-fta.md
  label: '故障树: node'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 27 - 节点与节点池管理 (Node & NodePool Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [ACK NodePool](https://help.aliyun.com/document_detail/160490.html)

<!-- chunk: 节点池 (NodePool) 核心架构 -->
## 节点池 (NodePool) 核心架构

| 功能 (Feature) | 描述 (Description) | 生产建议 (Best Practice) |
|---------------|-------------------|--------------------------|
| **弹性伸缩 (ASG)** | 自动增加/减少 ECS 实例 | 开启 `cluster-autoscaler` 配合 HPA/VPA, 为不同业务建独立 NodePool |
| **多规格混合** | 选定多种 ECS 规格 | 将 Spot 与按量/包年实例混用, 设置合适 `expander` 策略(如 `least-waste`) |
| **自定义 OS** | 支持 Alibaba Cloud Linux / ContainerOS | 生产环境推荐 ContainerOS, 统一 OS 版本, 禁止手工登录改配置 |
| **自动修复** | 节点 NotReady 时自动重启或替换 | 关键生产环境必须开启, 同时结合 `PodDisruptionBudget` 控制重启节奏 |
| **分级隔离** | 不同安全等级/环境的节点池 | 通过 Node 标签/污点严格区分 `prod/staging/dev` 与 `internet/intranet` |

<!-- chunk: 节点生命周期与运维流程 (Node Lifecycle) -->
## 节点生命周期与运维流程 (Node Lifecycle)

| 阶段 | 关键操作 | 建议命令 | 注意事项 |
|------|----------|----------|----------|
| **准备 (Provision)** | 通过 NodePool 创建/扩容节点 | ACK 控制台或 `cluster-autoscaler` | 统一镜像与 [[kubelet|kubelet]] 配置, 预置监控/日志 [[DaemonSet|DaemonSet]] |
| **接入 (Join)** | 节点加入集群并打标签 | `kubectl label nodes` / `kubectl taint nodes` | 加入后立刻补齐 `env=prod`、`zone=xxx` 等业务标签 |
| **维护 (Maintain)** | 打补丁/升级内核/重启宿主机 | `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` | 搭配 PDB, 控制同时维护的节点数量 |
| **下线 (Decommission)** | 永久移除节点 | `kubectl drain` → `kubectl delete node` | 先确认无绑定本地盘/本地日志, 相关 Pod 已在其他节点稳定运行 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点 & 池
kubectl get nodes -o wide
kubectl get nodepool -A 2>/dev/null || echo "在 ACK 控制台查看 NodePool 配置"

# 按标签筛选节点
kubectl get nodes -l env=prod
```
<!-- chunk: 调度与隔离: 标签与污点 (Label & Taint) -->
## 调度与隔离: 标签与污点 (Label & Taint)

| 能力 | 示例 | 作用 |
|------|------|------|
| **节点标签 (Label)** | `node.[[Kubernetes|kubernetes]].io/instance-type=ecs.g7.xlarge` | 匹配大规格计算节点, 用于 CPU 密集型服务 |
| | `zone=cn-hangzhou-h` | 控制跨 AZ 分布, 与 PV 拓扑、SLB 匹配 |
| **节点污点 (Taint)** | `kubectl taint nodes node1 role=system:NoSchedule` | 仅允许带对应容忍 (Toleration) 的系统 Pod 调度上去 |
| **Pod 亲和/反亲和** | `topologyKey: kubernetes.io/hostname` | 同一业务副本分散到不同节点/机架, 提升高可用 |

```yaml
# 专用 GPU 节点池示例
apiVersion: v1
kind: Pod
metadata:
  name: gpu-job
spec:
  nodeSelector:
    aliyun.accelerator/nvidia_name: "V100"
  tolerations:
  - key: "gpu-only"
    operator: "Equal"
    value: "true"
    effect: "NoSchedule"
```

<!-- chunk: 资源预留 (Resource Reservation) -->
## 资源预留 (Resource Reservation)

通过 `kubelet` 参数控制系统稳定性：
- `--system-reserved`: CPU/Memory 为 OS 进程预留。
- `--kube-reserved`: 为 K8s 组件 (Kubelet, Proxy) 预留。
- `--eviction-hard`: 设置硬驱逐阈值 (如 `memory<500Mi`) 防范宿主机崩溃。

> **生产建议**: 将系统/组件预留总和控制在节点容量的 10%~20%, 对大节点适当上浮。

<!-- chunk: 节点健康检查自动化 -->
## 节点健康检查自动化

### 一键节点健康检查脚本

```bash
#!/bin/bash
# 🟢 低风险：只读检查
# 节点健康综合检查

NODE=${1:-$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')}

echo "=== 节点健康检查: $NODE ==="

# 1. 节点状态
echo "--- 1. 节点状态 ---"
kubectl get node "$NODE" -o custom-columns=\
NAME:.metadata.name,\
STATUS:.status.conditions[-1].type,\
READY:.status.conditions[?(@.type=="Ready")].status,\
AGE:.metadata.creationTimestamp

# 2. 节点 Conditions 检查
echo ""
echo "--- 2. Conditions 详情 ---"
kubectl get node "$NODE" -o json | jq -r '
  .status.conditions[] |
  "\(.type): \(.status) (\(.reason // "N/A")) - \(.message // "")"
'

# 3. 资源使用情况
echo ""
echo "--- 3. 资源使用 ---"
kubectl top node "$NODE" 2>/dev/null || echo "metrics-server 不可用"

# 4. 节点容量与可分配
echo ""
echo "--- 4. 容量与可分配 ---"
kubectl get node "$NODE" -o json | jq -r '
  "Capacity:",
  "  CPU: \(.status.capacity.cpu)",
  "  Memory: \(.status.capacity.memory)",
  "  Pods: \(.status.capacity.pods)",
  "Allocatable:",
  "  CPU: \(.status.allocatable.cpu)",
  "  Memory: \(.status.allocatable.memory)",
  "  Pods: \(.status.allocatable.pods)"
'

# 5. 节点上的 Pod 数量
echo ""
echo "--- 5. Pod 分布 ---"
POD_COUNT=$(kubectl get pods -A --field-selector=spec.nodeName="$NODE" --no-headers | wc -l)
echo "节点上 Pod 数量: $POD_COUNT"

# 6. 节点标签与污点
echo ""
echo "--- 6. 标签与污点 ---"
echo "标签:"
kubectl get node "$NODE" -o json | jq -r '.metadata.labels | to_entries[] | "  \(.key)=\(.value)"' | head -10
echo "污点:"
kubectl get node "$NODE" -o jsonpath='{.spec.taints}' | jq -r '.[]? | "  \(.key)=\(.value):\(.effect)"' 2>/dev/null || echo "  无污点"

# 7. kubelet 版本
echo ""
echo "--- 7. 组件版本 ---"
kubectl get node "$NODE" -o jsonpath='kubelet: {.status.nodeInfo.kubeletVersion}, OS: {.status.nodeInfo.osImage}, Kernel: {.status.nodeInfo.kernelVersion}'
echo ""
```

### 节点健康 PrometheusRule

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: node-health-alerts
  namespace: monitoring
spec:
  groups:
    - name: node-health
      rules:
        - alert: NodeNotReady
          expr: kube_node_status_condition{condition="Ready",status="true"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "节点 {{ $labels.node }} NotReady 超过 5 分钟"
            runbook: "检查 kubelet 状态、网络连通性、资源压力"

        - alert: NodeHighCPU
          expr: |
            100 - (avg by(instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 90
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.instance }} CPU 使用率 > 90%"

        - alert: NodeHighMemory
          expr: |
            (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100 > 90
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.instance }} 内存使用率 > 90%"

        - alert: NodeDiskPressure
          expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "节点 {{ $labels.node }} 磁盘压力"
            runbook: "清理镜像/日志，扩容磁盘"

        - alert: NodePodCapacityNearFull
          expr: |
            kubelet_running_pods / kube_node_status_capacity{resource="pods"} > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} Pod 数量接近上限 (>90%)"
```

<!-- chunk: 节点自动修复 -->
## 节点自动修复 (Node Auto-Repair)

### 修复策略对比

| 修复方式 | 触发条件 | 修复动作 | 适用场景 |
|----------|----------|----------|----------|
| kubelet 自恢复 | kubelet 进程崩溃 | systemd 自动重启 | 所有环境 |
| Node Problem Detector | 内核错误/硬件故障 | 标记节点 + 告警 | 生产环境 |
| Cluster Autoscaler 替换 | 节点 NotReady > 10min | 删除并重建节点 | 云环境 |
| ACK 自动修复 | 节点异常 | 重启/替换 ECS | 阿里云 ACK |
| 手动 Drain + 替换 | 硬件故障确认 | 人工介入 | 裸金属/特殊场景 |

### Node Problem Detector 部署

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-problem-detector
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: node-problem-detector
  template:
    spec:
      containers:
        - name: node-problem-detector
          image: registry.k8s.io/node-problem-detector/node-problem-detector:v0.8.18
          command:
            - /node-problem-detector
            - --logtostderr
            - --config.system-log-monitor=/config/kernel-monitor.json,/config/docker-monitor.json
          securityContext:
            privileged: true
          resources:
            requests:
              cpu: 20m
              memory: 64Mi
            limits:
              cpu: 200m
              memory: 128Mi
          volumeMounts:
            - name: log
              mountPath: /var/log
              readOnly: true
            - name: config
              mountPath: /config
              readOnly: true
      volumes:
        - name: log
          hostPath:
            path: /var/log
        - name: config
          configMap:
            name: node-problem-detector-config
```

<!-- chunk: 节点升级策略 -->
## 节点升级策略

### 滚动升级流程

```
1. 准备工作
   ├── 确认目标版本兼容性
   ├── 备份关键配置
   └── 通知相关团队

2. 逐节点升级（每次 1-2 个节点）
   ├── kubectl cordon <node>          # 标记不可调度
   ├── kubectl drain <node> \         # 驱逐 Pod
   │     --ignore-daemonsets \
   │     --delete-emptydir-data \
   │     --grace-period=60
   ├── 执行节点升级（OS/kubelet）
   ├── systemctl restart kubelet
   ├── kubectl uncordon <node>        # 恢复调度
   └── 验证节点状态与 Pod 运行

3. 升级后验证
   ├── 检查所有节点版本一致
   ├── 验证关键服务运行正常
   └── 监控告警无异常
```

### 升级命令参考

```bash
# 🟡 中风险：会修改节点状态
# 1. 标记节点不可调度
kubectl cordon node-1

# 2. 驱逐 Pod（尊重 PDB）
kubectl drain node-1 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --grace-period=60 \
  --timeout=300s

# 3. 升级完成后恢复调度
kubectl uncordon node-1

# 4. 验证节点状态
kubectl get node node-1 -o wide
kubectl get pods -A --field-selector=spec.nodeName=node-1
```

<!-- chunk: 节点池容量规划 -->
## 节点池容量规划

### 容量规划公式

```
所需节点数 = ceil(总 Pod 资源请求 / 单节点可分配资源 × 冗余系数)

冗余系数建议：
- 生产环境: 1.3 - 1.5 (预留 30-50% 缓冲)
- 测试环境: 1.1 - 1.2
- 开发环境: 1.0 - 1.1
```

### 节点规格选型表

| 业务类型 | 推荐规格 | CPU:内存比 | 适用场景 |
|----------|----------|------------|----------|
| Web/API 服务 | 4C8G / 8C16G | 1:2 | 通用微服务 |
| 计算密集型 | 8C16G / 16C32G | 1:2 | 视频转码/科学计算 |
| 内存密集型 | 4C16G / 8C32G | 1:4 | 缓存/内存数据库 |
| GPU 计算 | 8C32G + V100/A100 | - | AI 训练/推理 |
| 存储密集型 | 4C16G + 大磁盘 | 1:4 | 日志/监控存储 |

### 容量监控 Dashboard PromQL

```promql
# 集群整体资源使用率
sum(kube_pod_container_resource_requests{resource="cpu"}) /
sum(kube_node_status_allocatable{resource="cpu"}) * 100

# 按节点池分组的使用率
sum by (node_pool) (
  kube_pod_container_resource_requests{resource="cpu"}
) /
sum by (node_pool) (
  kube_node_status_allocatable{resource="cpu"}
) * 100

# 预测资源耗尽时间
predict_linear(
  sum(kube_pod_container_resource_requests{resource="cpu"})[7d:1h],
  7 * 24 * 3600
) > sum(kube_node_status_allocatable{resource="cpu"})
```

<!-- chunk: 节点故障排查 -->
## 节点故障排查

### 故障排查决策树

```
节点 NotReady
├── kubelet 状态检查
│   ├── systemctl status kubelet → 服务是否运行
│   ├── journalctl -u kubelet -f → 查看日志
│   └── kubelet 证书是否过期
├── 网络连通性
│   ├── ping 控制平面 VIP
│   ├── curl -k https://<apiserver>:6443/healthz
│   └── 检查 CNI 插件状态
├── 资源压力
│   ├── df -h → 磁盘使用率
│   ├── free -m → 内存使用率
│   └── top → CPU 负载
└── 系统组件
    ├── containerd/docker 状态
    ├── 内核日志 dmesg | tail -50
    └── 时间同步 chronyc tracking
```

### 常见节点故障修复表

| 故障现象 | 可能原因 | 排查命令 | 修复措施 |
|----------|----------|----------|----------|
| NotReady | kubelet 崩溃 | `systemctl status kubelet` | `systemctl restart kubelet` |
| NotReady | 证书过期 | `openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates` | 重新签发证书 |
| DiskPressure | 镜像/日志占满 | `df -h /var/lib/containerd` | `crictl rmi --prune` 清理镜像 |
| MemoryPressure | Pod 内存泄漏 | `kubectl top pods --sort-by=memory` | 驱逐异常 Pod |
| PIDPressure | 进程数超限 | `cat /proc/sys/kernel/pid_max` | 调大 pid_max 或限制 Pod 进程数 |
| 网络不通 | CNI 异常 | `kubectl logs -n kube-system -l app=calico-node` | 重启 CNI DaemonSet |
| 时间不同步 | NTP 异常 | `chronyc tracking` | `chronyc makestep` |

<!-- chunk: 节点安全加固 -->
## 节点安全加固

### 安全加固检查清单

| 检查项 | 命令 | 期望结果 |
|--------|------|----------|
| SSH 禁用密码登录 | `grep PasswordAuthentication /etc/ssh/sshd_config` | no |
| 禁用 root 远程登录 | `grep PermitRootLogin /etc/ssh/sshd_config` | no |
| kubelet 只读端口关闭 | `grep readOnlyPort /var/lib/kubelet/config.yaml` | 0 |
| kubelet 匿名认证关闭 | `grep anonymous /var/lib/kubelet/config.yaml` | enabled: false |
| 容器运行时版本 | `containerd --version` | 最新稳定版 |
| 内核参数加固 | `sysctl net.ipv4.ip_forward` | 1 (K8s 需要) |
| 审计日志开启 | `grep audit /etc/kubernetes/manifests/kube-apiserver.yaml` | 已配置 |

### 节点安全加固脚本

```bash
#!/bin/bash
# 🟡 中风险：会修改节点配置
# 节点安全加固（生产环境谨慎执行）

echo "=== 节点安全加固 ==="

# 1. 内核参数加固
cat >> /etc/sysctl.d/99-kubernetes-hardening.conf << EOF
# 禁用 ICMP 重定向
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0

# 禁用源路由
net.ipv4.conf.all.accept_source_route = 0

# 启用 SYN Cookie
net.ipv4.tcp_syncookies = 1

# 反向路径过滤
net.ipv4.conf.all.rp_filter = 1
EOF

sysctl --system

# 2. 文件权限加固
chmod 644 /etc/kubernetes/admin.conf 2>/dev/null || true
chmod 644 /etc/kubernetes/kubelet.conf 2>/dev/null || true
chmod 600 /var/lib/kubelet/pki/*.pem 2>/dev/null || true

echo "=== 加固完成 ==="
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 工作负载 MOC
- [[工作负载/README.md|Domain-4: Kubernetes工作负载管理]]
- Domain-4 工作负载 — 开源项目索引
- 01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## See Also

- 16-runtime-class-configuration
- 17-container-images-registry
- 19-scheduler-configuration
- 20-kubelet-configuration

## Related

- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]


<!-- risk-assessed -->
