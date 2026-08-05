---
title: 16 - Kubernetes 故障排查专家级指南
description: '# 16 - Kubernetes 故障排查专家级指南'
summary: '在 Kubernetes 生产环境中，故障排查应遵循从底层到高层、从控制平面到数据平面的结构化路径。'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- prometheus
- cilium
- operator
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 故障排查专家级指南 是什么
- 如何 Kubernetes 故障排查专家级指南
- Kubernetes 1 architecture fundamentals 最佳实践
- Kubernetes 故障排查专家级指南 故障排查
- Kubernetes 故障排查专家级指南 排障步骤
trigger_keywords:
- Kubernetes
- 故障排查专家级指南
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
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
  path: ../容器运行时/
  label: '相关知识域: 容器运行时'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: skill
  path: ../故障诊断/topic-skills/19-skill-local-demo-guide.md
  label: '运维技能: 19-skill-local-demo-guide'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 16 - [[kubernetes|Kubernetes]] 故障排查专家级指南

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 一、架构级故障排查逻辑 -->
## 一、架构级故障排查逻辑

在 Kubernetes 生产环境中，故障排查应遵循从底层到高层、从控制平面到数据平面的结构化路径。

### 1.1 排查漏斗模型
1. **基础设施层**：计算、存储、网络连通性。
2. **控制平面层**：API Server 响应、[[etcd|etcd]] 延迟、调度器状态。
3. **数据平面层**：[[kubelet|Kubelet]] 状态、CNI/CSI 正常运行、节点资源压力。
4. **应用层**：Pod 状态、容器日志、探针失败。

---

<!-- chunk: 二、核心组件故障诊断 -->
## 二、核心组件故障诊断

### 2.1 API Server 问题
- **现象**：`kubectl` 响应慢或返回 5xx。
- **排查指标**：
  - `apiserver_request_duration_seconds` (P99 < 1s)
  - `apiserver_current_inflight_requests` (APF 限制检查)
- **常见原因**：etcd 性能瓶颈、APF 限流、大流量 Webhook 阻塞。
- **诊断命令**：
  ```bash
  # 🟢 检查 API Server 健康
  kubectl get --raw /healthz?verbose
  kubectl get --raw /livez?verbose
  kubectl get --raw /readyz?verbose
  
  # 🟢 检查 API 请求延迟
  kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket | head -20
  
  # 🟢 检查 APF 限流
  kubectl get --raw /metrics | grep apiserver_flowcontrol_rejected_requests_total
  
  # 🟢 检查 Webhook 延迟
  kubectl get --raw /metrics | grep apiserver_admission_webhook_admission_duration_seconds
  
  # 🟡 检查慢请求审计日志
  kubectl logs -n kube-system kube-apiserver-<node> | grep "slow" | tail -20
  ```
- **修复方案**：
  - Webhook 超时：调整 `timeoutSeconds`（默认 10s → 5s）
  - APF 限流：调整 `PriorityLevelConfiguration` 为核心组件预留带宽
  - etcd 瓶颈：参见 2.2 节

### 2.2 etcd 性能问题
- **现象**：Leader 频繁切换，写入延迟高。
- **诊断命令**：
  ```bash
  # 🟢 集群状态
  etcdctl endpoint status --write-out=table
  etcdctl endpoint health --cluster
  
  # 🟢 性能检查
  etcdctl check perf
  
  # 🟢 磁盘延迟（关键指标）
  # etcd_disk_wal_fsync_duration_seconds P99 应 < 10ms
  # etcd_disk_backend_commit_duration_seconds P99 应 < 25ms
  kubectl get --raw /metrics | grep etcd_disk
  
  # 🟢 数据库大小
  etcdctl endpoint status --write-out=table | awk '{print $4}'
  
  # 🟡 压缩和碎片整理
  etcdctl compact $(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')
  etcdctl defrag --cluster
  ```
- **优化点**：
  - 磁盘 IOPS：推荐 NVMe SSD，IOPS > 10,000
  - Heartbeat 间隔：跨 AZ 部署时调大 `--heartbeat-interval=500`
  - 数据库大小：定期压缩，保持 < 4GB
  - Learner 节点：新成员先作为 Learner 加入

### 2.3 调度器问题
- **现象**：Pod 长时间 Pending。
- **诊断命令**：
  ```bash
  # 🟢 查看调度失败原因
  kubectl describe pod <pod> | grep -A10 "Events"
  kubectl get events --field-selector reason=FailedScheduling --sort-by='.lastTimestamp'
  
  # 🟢 检查节点可分配资源
  kubectl describe nodes | grep -A5 "Allocated resources"
  
  # 🟢 检查调度器吐吐
  kubectl get --raw /metrics | grep scheduler_scheduling_attempt_total
  kubectl get --raw /metrics | grep scheduler_scheduling_duration_seconds
  ```
- **常见原因**：
  - 资源不足：requests 超过节点可分配量
  - 亲和性冲突：nodeSelector/affinity 无匹配节点
  - 污点未容忍：Taint 无对应 Toleration
  - PVC 未绑定：StorageClass 无可用 Provisioner

### 2.4 Kubelet 问题
- **现象**：节点 NotReady，Pod 无法启动。
- **诊断命令**：
  ```bash
  # SSH 到节点后:
  # 🟢 检查 kubelet 状态
  systemctl status kubelet
  journalctl -u kubelet --since "10 min ago" --no-pager | tail -50
  
  # 🟢 检查节点条件
  kubectl describe node <node> | grep -A20 "Conditions"
  
  # 🟢 检查容器运行时
  crictl ps
  crictl info | jq '.conditions'
  
  # 🟢 检查磁盘/内存压力
  df -h /var/lib/kubelet
  free -h
  dmesg | tail -20
  ```

---

<!-- chunk: 三、网络与存储排查 -->
## 三、网络与存储排查

### 3.1 CNI 网络问题 (以 [[cilium|Cilium]] 为例)
- **工具**：`cilium-health`
- **检查项**：
  - BPF Map 是否满额
  - 身份验证 (Identity) 冲突
  - 跨节点 VXLAN 封包问题

### 3.2 存储挂载失败
- **常见错误**：`Multi-Attach error`, `Timeout on mount`
- **排查路径**：
  - 检查 CSI Controller 容器日志
  - 检查节点上的 `/var/log/messages` 或 `dmesg`
  - 验证 PVC/PV 状态及 StorageClass 配置

---

<!-- chunk: 四、生产级诊断工具箱 -->
## 四、生产级诊断工具箱

| 工具 | 用途 | 专家提示 |
|:---|:---|:---|
| [[01-集群基础/05-kubectl/01-kubectl-debug-ephemeral-containers|kubectl-debug]] | 启动诊断容器 | 使用 `--ephemeral-containers` (v1.25+ GA) |
| **Cilium Hubble** | 网络流量可视化 | 观察 L7 层的拒绝原因 |
| **Inspektor Gadget** | eBPF 实时监控 | 捕获进程级文件读写和网络连接 |
| **Prometheus** | 长期趋势分析 | 关注组件的 Memory/CPU 波动 |

---

<!-- chunk: 五、经典案例库 -->
## 五、经典案例库

### 5.1 案例：APF 导致的诡异请求延迟
- **诊断**：通过 `apiserver_flowcontrol_rejected_requests_total` 发现大量请求被 drop。
- **解决**：调整 `PriorityLevelConfiguration` 和 `FlowSchema`，为核心 Operator 预留带宽。

### 5.2 案例：etcd 磁盘 IO 抖动引发集群雪崩
- **现象**：Kubelet 失去连接，控制平面无法修改资源。
- **诊断**：`etcd_disk_wal_fsync_duration_seconds` 超过 50ms。
- **解决**：迁移 etcd 数据目录到专用 SSD，并设置内核 `ionice`。

### 5.3 案例：Webhook 超时导致所有 Deployment 失败
- **现象**：任何 `kubectl apply` 都返回 `context deadline exceeded`。
- **诊断**：
  ```bash
  # 检查所有 Webhook 状态
  kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations
  # 检查 Webhook 服务 Endpoints
  kubectl get endpoints -n <webhook-ns>
  ```
- **根因**：某 Webhook 服务 Pod 崩溃，但 Webhook 配置未设置 `failurePolicy: Ignore`。
- **解决**：紧急删除故障 Webhook 配置，修复服务后重新注册。
- **预防**：所有非关键 Webhook 设置 `failurePolicy: Ignore` + `timeoutSeconds: 5`。

### 5.4 案例：CoreDNS 内存泄漏导致集群 DNS 间歇性失败
- **现象**：Pod 内 `nslookup` 随机超时，重启 CoreDNS 后暂时恢复。
- **诊断**：
  ```bash
  kubectl -n kube-system top pods -l k8s-app=kube-dns
  kubectl -n kube-system logs -l k8s-app=kube-dns --tail=100 | grep -i "memory\|oom"
  ```
- **根因**：CoreDNS 版本 Bug 导致缓存未释放。
- **解决**：升级 CoreDNS 到最新补丁版本 + 设置资源 limits 防止 OOM 影响节点。

### 5.5 案例：节点 IP 耗尽导致新 Pod 无法调度
- **现象**：Pod Pending，事件显示 `failed to allocate IP`。
- **诊断**：
  ```bash
  # 检查节点 IP 池（Cilium）
  kubectl get ciliumnodes -o json | jq '.items[].status.ipam'
  # 检查子网剩余 IP（云环境）
  # AWS: aws ec2 describe-subnets --subnet-ids <id> --query 'Subnets[0].AvailableIpAddressCount'
  ```
- **解决**：扩展子网 CIDR / 增加节点 / 调整 Pod CIDR 分配。

---

## 六、预防性监控与告警

### 控制平面关键告警

| 指标 | 阈值 | 级别 | 含义 |
|------|------|------|------|
| `apiserver_request_duration_seconds` P99 | > 1s | Warning | API 响应慢 |
| `etcd_disk_wal_fsync_duration_seconds` P99 | > 10ms | Critical | etcd 磁盘慢 |
| `etcd_server_leader_changes_seen_total` | > 3/h | Critical | Leader 不稳定 |
| `scheduler_scheduling_attempt_total{result="error"}` | > 0 | Warning | 调度失败 |
| `kubelet_node_status_condition{condition="Ready"}` | != 1 | Critical | 节点异常 |
| `rest_client_requests_total{code=~"5.."}` | 突增 | Warning | 组件通信异常 |

### 数据平面关键告警

| 指标 | 阈值 | 级别 | 含义 |
|------|------|------|------|
| `container_memory_working_set_bytes / limits` | > 90% | Warning | 内存即将 OOM |
| `container_cpu_cfs_throttled_periods_total` | 突增 | Warning | CPU 被限流 |
| `kube_pod_container_status_restarts_total` | > 3/h | Warning | 频繁重启 |
| `kube_pod_status_phase{phase="Pending"}` | 持续 > 5min | Warning | 调度失败 |
| `node_filesystem_avail_bytes / size_bytes` | < 15% | Critical | 磁盘即将满 |

---

**维护者**: Kusheet SRE Team | **作者**: Allen Galler
---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[01-集群基础/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## 七、自动化诊断脚本

### 集群健康一键检查

```bash
#!/bin/bash
# 🟢 只读：K8s 集群健康一键检查
echo "=== K8s 集群健康检查 $(date) ==="

# 1. 控制平面
echo -n "[1/8] API Server: "
kubectl get --raw /healthz 2>/dev/null && echo "" || echo "❌ 不可达"

echo -n "[2/8] etcd: "
kubectl get --raw /healthz/etcd 2>/dev/null && echo "" || echo "❌"

echo -n "[3/8] 调度器: "
kubectl get --raw /healthz/scheduler 2>/dev/null && echo "" || echo "❌"

# 2. 节点状态
echo -n "[4/8] 节点状态: "
NOT_READY=$(kubectl get nodes --no-headers | grep -v " Ready" | wc -l | tr -d ' ')
TOTAL=$(kubectl get nodes --no-headers | wc -l | tr -d ' ')
echo "$((TOTAL - NOT_READY))/$TOTAL Ready"

# 3. 系统 Pod
echo -n "[5/8] 系统 Pod: "
UNHEALTHY=$(kubectl get pods -n kube-system --no-headers | grep -v Running | grep -v Completed | wc -l | tr -d ' ')
echo "$UNHEALTHY 个异常"

# 4. 资源压力
echo "[6/8] 资源压力:"
kubectl get nodes -o custom-columns=
  NAME:.metadata.name,CPU:.status.conditions[?(@.type=="MemoryPressure")].status,MEM:.status.conditions[?(@.type=="DiskPressure")].status \
  --no-headers | grep -v "False" | head -5

# 5. Pending Pod
echo -n "[7/8] Pending Pod: "
kubectl get pods -A --field-selector status.phase=Pending --no-headers | wc -l | tr -d ' '

# 6. 最近告警事件
echo "[8/8] 最近警告事件:"
kubectl get events -A --field-selector type=Warning --sort-by='.lastTimestamp' --no-headers | tail -5

echo ""
echo "=== 检查完成 ==="
```

### Pod 故障快速诊断

```bash
#!/bin/bash
# 🟢 只读：Pod 故障快速诊断
POD=$1
NS=${2:-default}

echo "=== Pod 诊断: $NS/$POD ==="

# 状态概览
echo "[1] 状态:"
kubectl get pod $POD -n $NS -o custom-columns=
  STATUS:.status.phase,RESTARTS:.status.containerStatuses[0].restartCount,NODE:.spec.nodeName

# 事件
echo "[2] 事件:"
kubectl get events -n $NS --field-selector involvedObject.name=$POD --sort-by='.lastTimestamp' | tail -10

# 容器状态
echo "[3] 容器状态:"
kubectl get pod $POD -n $NS -o jsonpath='{range .status.containerStatuses[*]}{.name}: {.state}{"\n"}{end}'

# 最近日志
echo "[4] 最近日志:"
kubectl logs $POD -n $NS --tail=20 2>/dev/null || echo "  无法获取日志"

# 资源使用
echo "[5] 资源使用:"
kubectl top pod $POD -n $NS 2>/dev/null || echo "  metrics 不可用"

echo "=== 诊断完成 ==="
```

## 八、事故复盘模板

### Post-Mortem 结构

```markdown
# 事故复盘: [事故标题]

## 概要
- **日期**: YYYY-MM-DD HH:MM - HH:MM (UTC)
- **影响范围**: [受影响的服务/用户数]
- **严重级别**: SEV-1/2/3/4
- **发现方式**: 告警/用户报告/巡检

## 时间线
| 时间 | 事件 |
|------|------|
| HH:MM | 告警触发 |
| HH:MM | On-call 确认 |
| HH:MM | 根因定位 |
| HH:MM | 修复执行 |
| HH:MM | 服务恢复 |

## 根因分析 (5 Whys)
1. Why: 服务不可用 → Pod CrashLoopBackOff
2. Why: Pod 崩溃 → OOMKilled
3. Why: 内存溢出 → 内存泄漏
4. Why: 泄漏原因 → 缓存未设置上限
5. Why: 未设置上限 → 代码审查遗漏

## 修复措施
- **即时**: [紧急修复动作]
- **短期**: [1-2 周内完成]
- **长期**: [架构/流程改进]

## 经验教训
- **做得好**: ...
- **待改进**: ...
- **行动项**: [Owner + Deadline]
```

## 九、混沌工程验证

### 故障注入测试矩阵

| 故障场景 | 注入方式 | 预期行为 | 验证指标 |
|----------|----------|----------|----------|
| 节点宕机 | `kubectl cordon` + drain | Pod 迁移到其他节点 | 恢复时间 < 5min |
| API Server 不可用 | 停止 apiserver Pod | 已运行 Pod 不受影响 | 数据平面持续服务 |
| etcd Leader 切换 | 停止 Leader | 自动选举新 Leader | 切换 < 10s |
| DNS 失败 | 删除 CoreDNS Pod | 已缓存解析正常 | 新解析延迟 < 30s |
| 网络分区 | iptables 隔离节点 | 节点标记 NotReady | Pod 重调度 < 5min |
| 磁盘压力 | 填充磁盘 | 节点 DiskPressure | Pod 被驱逐 |

### LitmusChaos 实验示例

```yaml
# 节点 CPU 压力实验
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosExperiment
metadata:
  name: node-cpu-hog
spec:
  definition:
    scope: Namespaced
    permissions:
      - apiGroups: [""]
        resources: ["pods"]
        verbs: ["create", "delete", "get", "list"]
    image: litmuschaos/go-runner:latest
    args:
      - -c
      - ./experiments -name node-cpu-hog
    env:
      - name: TOTAL_CHAOS_DURATION
        value: "60"       # 60秒
      - name: NODE_CPU_CORE
        value: "2"        # 压力 2 核
---
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: node-cpu-engine
  namespace: staging
spec:
  appinfo:
    appns: staging
    applabel: app=web
  experiments:
    - name: node-cpu-hog
      spec:
        probe:
          - name: check-pod-health
            type: ContinuousProbe
            mode: Continuous
            runProperties:
              probeTimeout: 5
              interval: 2
            cmdProbe:
              command: "kubectl get pods -n staging -l app=web -o jsonpath='{.items[*].status.phase}'"
              comparator:
                type: string
                criteria: equals
                value: Running
```

## 十、可观测性驱动调试

### Trace-Log-Metric 关联查询

```promql
# 1. 发现异常: API 延迟突增
histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket{verb="GET"}[5m]))

# 2. 定位范围: 哪个资源类型慢
sum by (resource) (rate(apiserver_request_duration_seconds_sum{verb="GET"}[5m]))
/ sum by (resource) (rate(apiserver_request_duration_seconds_count{verb="GET"}[5m]))

# 3. 关联 etcd: 是否存储层慢
histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m]))

# 4. 关联节点: 是否资源压力
kube_node_status_condition{condition="MemoryPressure", status="true"} == 1
```

### 分层排查决策树

```
服务异常
├── 控制平面可达?
│   ├── No → API Server/etcd/网络
│   └── Yes
│       ├── Pod 状态正常?
│       │   ├── No → 调度/资源/镜像/探针
│       │   └── Yes
│       │       ├── 网络连通?
│       │       │   ├── No → CNI/Service/DNS/NetworkPolicy
│       │       │   └── Yes
│       │       │       ├── 应用日志有错误?
│       │       │       │   ├── Yes → 应用层 Bug
│       │       │       │   └── No → 性能/容量问题
```

## See Also

- 14-security-architecture
- 15-observability-architecture
- 17-production-operations-best-practices
- 18-upgrade-migration-strategy


<!-- risk-assessed -->
