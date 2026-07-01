---
title: K8s Performance Bottleneck 远程顾问对话脚本
summary: 性能瓶颈的远程顾问对话脚本，覆盖CPU、内存、磁盘、网络性能分析。
category: dialogue
tags:
- dialogue
- remote-advisor
- k8s-performance
- skill
- performance
- cpu
- memory
- io
- latency
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
relationships:
- target: '[[skills/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[entities/etcd.md]]'
  type: uses
- target: '[[entities/istio.md]]'
  type: uses
---


# K8s Performance Bottleneck 远程顾问对话脚本

> **角色设定**：你是部署在客户环境之外的远程性能顾问，无法直接连接集群。你只能通过对话指导现场工程师执行操作。
> **对话目标**：在 30 分钟内定位性能瓶颈的根因并给出修复方案。

---

## 对话入口

### 场景 A：工程师直接描述性能症状

工程师："应用响应特别慢，P99 延迟飙高了"

顾问："收到。这是典型的性能瓶颈症状。为了快速定位问题，请告诉我三个信息：
1. **影响范围**：是单个服务慢，还是多个服务都慢？影响多少节点？
2. **症状类型**：是 CPU 高、内存高、磁盘 IO 高，还是网络延迟大？
3. **发生时间**：问题是突然出现还是逐渐恶化？最近 24 小时是否有部署、扩缩容或配置变更？"

### 场景 B：工程师从监控告警进入

工程师："Prometheus 告警 container_cpu_cfs_throttled_seconds_total 很高"

顾问："告警已确认，CPU 节流可能是性能瓶颈的信号。请确认三点：
1. 这个告警涉及**多少个 Pod**？是否集中在特定节点？
2. 控制平面节点（master）的 CPU 和内存是否正常？
3. 业务侧是否有用户投诉延迟或超时？"

### 场景 C：工程师发现 Pod 被 OOMKilled

工程师："有几个 Pod 一直在 OOMKilled"

顾问："OOMKilled 说明内存 limit 不足或存在内存泄漏。请确认：
1. 涉及的 Pod 名称和命名空间？
2. 这些 Pod 是突然开始 OOM，还是随着负载增长逐渐出现的？
3. 最近是否有新版本发布或流量突增？"

### 场景 D：工程师从控制台发现节点负载高

工程师："控制台看到有几个节点 CPU 使用率 90% 以上"

顾问："节点高负载可能导致该节点上所有 Pod 性能下降。请切换到命令行执行以下操作，或者把控制台看到的节点资源使用截图发给我。如果控制台能显示具体是哪个进程或 Pod 占用了资源，也请一并告知。"

---

## Round 1：快速确认

顾问："现在开始第一轮诊断。请执行以下命令并粘贴完整输出：

```bash
kubectl top nodes
kubectl top pods --all-namespaces --sort-by=cpu | head -20
```

> **如果无法执行 kubectl top**：说明集群可能没有安装 metrics-server。请尝试以下替代方案：
> **替代方案 A**：通过 Prometheus / Grafana 查看节点和 Pod 的 CPU、内存使用率
> **替代方案 B**：通过集群管理控制台（如 Rancher、OpenShift Console、ACK 控制台）查看资源使用
> **替代方案 C**：请有权限的同事执行并截图发给你"

### 分支 1.1：单个 Pod CPU/内存高

工程师：（提供单个 Pod 资源使用高）

顾问："单个 Pod 资源使用高，当前为 P2 优先级（2 小时内修复）。下一步请执行：

```bash
kubectl describe pod <pod-name> -n <namespace> | grep -A 10 "Limits|Requests"
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 5 "resources:"
```

> **如果无法执行 describe**：请执行 `kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].resources}'` 获取资源限制信息。如果连 jsonpath 也无法使用，请直接告诉我该 Pod 的 CPU limit 和 memory limit 设置值。"

### 分支 1.2：多个 Pod 同时高负载

工程师：（提供多个 Pod 资源使用高）

顾问："多个 Pod 同时高负载，这可能是集群级或应用级的问题。请**立即**执行以下命令确认节点状态：

```bash
kubectl get nodes -o wide
kubectl describe node <高负载节点名> | grep -A 15 "Allocated resources"
```

> **如果无法执行 describe node**：请通过控制台查看高负载节点的资源分配情况。如果多个节点同时高负载且超过节点容量的 80%，这个问题可能升级到 P1（30 分钟内修复）。"

### 分支 1.3：节点整体高负载

工程师：（提供节点整体负载高）

顾问："节点整体高负载会影响该节点上所有 Pod 的性能。请执行以下检查：

```bash
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> -o wide
kubectl describe node <node-name> | grep -E "Pressure|DiskPressure|MemoryPressure|PIDPressure"
```

> **如果无法执行**：请通过集群控制台查看该节点的 Pod 列表和节点状态。如果节点有 Pressure 类污点，说明资源已饱和。"

### 分支 1.4：工程师无法执行任何命令

工程师："kubectl 连不上，也没有监控"

顾问："命令和监控都无法使用，信息严重不足。请尝试以下替代方案：

**替代方案 A（应用日志）**：查看应用自身的日志文件（/var/log/）或应用控制台，是否有 OutOfMemory、连接超时、线程池耗尽等错误

**替代方案 B（业务反馈）**：直接询问业务团队或用户，了解延迟增加的具体时间点和受影响的功能模块

**替代方案 C（云厂商控制台）**：通过云厂商控制台查看节点和 Pod 的资源使用趋势

> 如果以上都无法获取，且业务已中断，请**立即升级**到高级支持，当前信息不足以进行有效诊断。"

---

## Round 2：深度诊断

### 场景：已完成快速确认，进入深度诊断

顾问："现在进入深度诊断。根据 Round 1 的结果，我们需要进一步定位瓶颈来源。"

#### 分支 2.1：CPU 瓶颈（CPU Throttling / 高使用率）

工程师："Pod CPU 使用率接近 limit，且被节流"

顾问："CPU 瓶颈已确认。请执行以下命令深入分析：

```bash
# 查看 Pod 的 CPU 节流情况
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].name}'

# 查看节点上的 CPU 详细分配
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 查看该 Pod 的 CPU 使用历史（如有 metrics-server 或 Prometheus）
kubectl top pod <pod-name> -n <namespace>
```

> **如果无法执行 top**：请通过 Prometheus 查询 `container_cpu_usage_seconds_total` 和 `container_cpu_cfs_throttled_seconds_total` 指标。如果没有 Prometheus，请告诉我该 Pod 的 CPU request 和 limit 值，以及节点总 CPU 核心数。

同时请确认：
1. 该 Pod 的 CPU request 是否设置为 0 或未设置？
2. 节点上是否运行了其他高 CPU 占用的 Pod？
3. 最近是否有流量突增或定时任务触发？"

#### 分支 2.2：内存瓶颈（OOMKilled / 高内存使用）

工程师："Pod 被 OOMKilled，内存使用很高"

顾问："内存瓶颈已确认。请执行以下命令深入分析：

```bash
# 查看 Pod 的内存限制和重启次数
kubectl describe pod <pod-name> -n <namespace> | grep -E "OOMKilled|Restart Count|Limits"

# 查看节点上的内存分配情况
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 查看该 Pod 的内存使用趋势
kubectl top pod <pod-name> -n <namespace>
```

> **如果无法执行 top**：请通过 Prometheus 查询 `container_memory_working_set_bytes` 和 `container_memory_usage_bytes` 指标。如果没有监控，请告诉我该 Pod 的 memory limit 值和节点总内存大小。

同时请确认：
1. Pod 的内存使用是否持续增长（疑似内存泄漏）？
2. 最近是否有大数据量处理或缓存预热操作？
3. 同节点上的其他 Pod 是否也有内存压力？"

#### 分支 2.3：磁盘 IO 瓶颈（IO Wait 高 / 读写慢）

工程师："应用读写很慢，节点 iowait 很高"

顾问："磁盘 IO 瓶颈已确认。请执行以下命令深入分析：

**如果你能通过 SSH 连接到该节点**：

```bash
# 检查磁盘 IO 状态
iostat -x 1 5
# 或
iotop -o -b -n 5

# 检查磁盘空间
df -h

# 检查挂载点类型（本地盘 / 网络存储）
mount | grep -E 'csi|nfs|ceph|ebs|disk'
```

> **如果无法 SSH**：请执行以下 kubectl 替代命令：

```bash
# 查看存储类和相关 PVC
kubectl get pvc -n <namespace>
kubectl get pv | grep <pvc-name>

# 查看 StorageClass 的 provisioner
kubectl get storageclass
```

同时请确认：
1. 使用的是本地存储还是网络存储（NFS / Ceph / EBS / NAS）？
2. 磁盘使用率是否接近 100%？
3. 是否有大量日志写入或临时文件产生？"

#### 分支 2.4：网络延迟瓶颈（网络延迟高 / 带宽不足）

工程师："服务间调用延迟很高"

顾问："网络瓶颈已确认。请执行以下命令深入分析：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Pod 的网络连接状态
kubectl exec -it <pod-name> -n <namespace> -- netstat -s | grep -E 'drop|error|timeout'

# 查看 Service 的 Endpoints
kubectl get endpoints <service-name> -n <namespace>

# 查看 CNI Pod 状态
kubectl get pods -n kube-system | grep -E 'cni|calico|flannel|cilium|weave'
```

> **如果无法 kubectl exec**：请尝试以下替代方案：
> **替代方案 A**：在节点上执行 `ss -s` 或 `netstat -s` 查看网络统计
> **替代方案 B**：通过监控查看节点网络带宽使用率（rx/tx 字节数）
> **替代方案 C**：使用 `kubectl debug` 创建诊断 Pod：`kubectl run netshoot --rm -it --image=nicolaka/netshoot -- /bin/bash`，在 netshoot 中执行 `ping` 或 `curl` 测试

同时请确认：
1. 延迟高是发生在 Pod 到 Service、Pod 到 Pod，还是 Pod 到外部服务？
2. 是否使用了 Service Mesh（[[entities/istio.md|Istio]] / Linkerd）？如果是，请检查 sidecar 资源使用
3. 节点网络带宽使用率是否接近上限？"

#### 分支 2.5：应用级性能问题（非基础设施瓶颈）

工程师："CPU/内存/IO/网络都正常，但应用响应还是慢"

顾问："基础设施资源正常，问题可能出在应用内部。请执行以下分析：

```bash
# 查看应用日志中的性能相关错误
kubectl logs <pod-name> -n <namespace> --tail=200 | grep -iE 'timeout|slow|latency|queue|pool|thread'

# 查看 Pod 的启动和就绪探针配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].livenessProbe}{.spec.containers[0].readinessProbe}'

# 查看 HPA 状态（如果配置了自动扩缩容）
kubectl get hpa -n <namespace>
```

> **如果无法获取日志**：请通过日志聚合系统（ELK / Loki / SLS）查询该 Pod 的性能相关日志。

同时请确认：
1. 应用是否使用了连接池？连接池是否耗尽？
2. 应用是否有线程阻塞或死锁？
3. 数据库查询是否变慢？是否需要添加索引或优化 SQL？"

---

## Round 3：修复方案与执行

### 场景：已完成深度诊断，进入修复阶段

#### 分支 3.1：CPU Limit 过低导致节流（RC-001）

顾问："根因确认：CPU limit 过低导致容器被节流。

**修复步骤**（请按顺序执行）：

步骤 1：调整 CPU limit

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl edit deployment/<deployment-name> -n <namespace>
# 修改 resources.limits.cpu 为当前值的 1.5-2 倍
```

> **如果无法执行 edit**：请准备一个新的 YAML 文件，增加 `resources.limits.cpu` 值，然后执行 `kubectl apply -f <new-deployment.yaml>`。如果不知道应该设置多少，请告诉我当前 limit 值和节点总 CPU，我来帮你计算。

> **如果无法修改 Deployment**：请确认你是否有该命名空间的 edit 权限。如果没有，请联系有权限的同事执行，或提交变更工单。

步骤 2：观察滚动更新

```bash
kubectl rollout status deployment/<deployment-name> -n <namespace>
```

> **如果无法执行 rollout status**：请执行 `kubectl get pods -n <namespace> -w` 观察新 Pod 是否成功 Running。

步骤 3：验证 CPU 节流是否消失

```bash
# 查看 CPU 节流指标（如有 Prometheus）
# rate(container_cpu_cfs_throttled_seconds_total[5m])

# 或使用 top 观察
kubectl top pod <pod-name> -n <namespace>
```

> **警告**：提高 CPU limit 可能导致节点上其他 Pod 的 CPU 分配减少。请确认节点整体 CPU 使用率不超过 80%。"

#### 分支 3.2：内存 Limit 过低导致 OOM（RC-002）

顾问："根因确认：内存 limit 过低导致 OOMKilled。

**修复步骤**（请按顺序执行）：

步骤 1：调整内存 limit

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl edit deployment/<deployment-name> -n <namespace>
# 修改 resources.limits.memory 为当前值的 1.5-2 倍
```

> **如果无法执行 edit**：请准备新的 YAML 文件，增加 `resources.limits.memory` 值，然后 `kubectl apply -f <new-deployment.yaml>`。如果不确定设置多少，请告诉我当前 limit 值和节点总内存。

> **如果无法修改 Deployment**：请联系有权限的同事执行，或提交变更工单。

步骤 2：如果节点整体内存不足

```bash
# 查看节点内存分配
kubectl describe node <node-name> | grep -A 15 "Allocated resources"
```

> **如果节点内存已饱和**：请考虑以下方案：
> 1. 水平扩容节点（增加新节点到集群）
> 2. 清理非必要 Pod：`kubectl delete pod <old-pod> -n <namespace>`（仅删除可重建的 Pod）
> 3. 如果使用了 HPA，暂时降低副本数以释放内存

步骤 3：验证 OOM 是否消失

```bash
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].restartCount}'
```

> **如果 Restart Count 仍在增加**：说明内存仍然不够或存在内存泄漏。请进一步增加 limit 或检查应用代码。"

#### 分支 3.3：节点资源饱和（RC-003）

顾问："根因确认：节点资源饱和（CPU/内存/磁盘）。

**修复步骤**（请按顺序执行）：

步骤 1：临时缓解——驱逐低优先级 Pod

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 标记节点不可调度，防止新 Pod 调度上来
kubectl cordon <node-name>

# 驱逐节点上的 Pod（保留 DaemonSet）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
```

> **如果 drain 卡住**：添加 `--grace-period=30 --timeout=120s --force` 参数，或手动删除无法驱逐的 Pod。
> **如果无法 drain**：请手动 cordon 节点后，逐个删除该节点上的业务 Pod（控制器会自动在其他节点重建）。

步骤 2：扩容节点

```bash
# 查看当前节点池
kubectl get nodes -l <node-pool-label>

# 触发节点扩容（如使用 Cluster Autoscaler，检查 CA Pod 状态）
kubectl get pods -n kube-system | grep cluster-autoscaler
```

> **如果无法自动扩容**：请通过云厂商控制台或基础设施团队手动添加新节点。

步骤 3：恢复节点调度

```bash
kubectl uncordon <node-name>
```

> **注意**：节点资源饱和可能是容量规划不足的信号。建议后续评估是否需要增加节点规格或数量。"

#### 分支 3.4：磁盘 IO 饱和（RC-005）

顾问："根因确认：磁盘 IO 饱和。

**修复步骤**（请按顺序执行）：

步骤 1：清理磁盘空间

```bash
# 如果能 SSH 到节点
crictl rmi --prune
find /var/log -name '*.log' -size +100M -exec ls -lh {} \;
# 手动清理过大的日志文件
```

> **如果无法 SSH**：请通过以下方式清理：
> 1. 删除旧 Pod 日志：`kubectl logs` 确认日志已采集到外部系统后，清理节点日志
> 2. 调整 logrotate 策略，配置更激进的日志轮转
> 3. 如果使用的是 EmptyDir 卷，检查是否有大文件并清理

步骤 2：优化存储配置

```bash
# 如果使用 PVC，检查 StorageClass 是否有更快存储选项
kubectl get storageclass
# 考虑将高性能应用的存储迁移到 SSD / 本地盘类型
```

> **如果无法更换 StorageClass**：请考虑将应用的临时文件写入内存（emptyDir with medium: Memory）或调整应用减少磁盘 IO。

步骤 3：验证 IO 是否恢复

```bash
# 能 SSH 到节点时
iostat -x 1 5
```

> **如果无法 SSH**：请通过监控观察节点磁盘 IO 使用率是否下降。"

#### 分支 3.5：网络延迟 / 带宽不足（RC-006）

顾问："根因确认：网络延迟或带宽不足。

**修复步骤**（请按顺序执行）：

步骤 1：检查 CNI Pod 状态

```bash
kubectl get pods -n kube-system | grep -E 'cni|calico|flannel|cilium|weave'
kubectl logs -n kube-system <cni-pod-name> --tail=100
```

> **如果无法查看 CNI 日志**：请检查云厂商控制台是否有网络相关告警，或检查节点上的网络接口状态。

步骤 2：检查 Service Mesh（如使用）

```bash
# 检查 Istio sidecar 资源使用
kubectl top pod <pod-name> -n <namespace> -c istio-proxy
# 或检查 Linkerd proxy
```

> **如果 sidecar 资源不足**：请增加 sidecar 的 CPU/memory limit。

步骤 3：优化网络配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 检查 Service 的 Endpoints 分布
kubectl get endpoints <service-name> -n <namespace>

# 考虑使用拓扑感知路由（Topology Aware Routing）
kubectl annotate service <service-name> service.kubernetes.io/topology-mode=Auto
```

> **如果无法优化网络**：请考虑在业务层增加缓存、减少跨可用区调用、或使用本地缓存替代远程调用。"

---

## 升级路径

当满足以下条件之一时，顾问应明确建议**升级**到高级支持或值班经理：

### 🔴 立即升级（P0）

- **核心服务 P99 延迟超过 SLA** 且持续恶化，业务已中断
- **多个节点同时资源饱和**（CPU > 90% 或内存 > 95%），且无法快速扩容
- **集群级别资源耗尽**，新 Pod 无法调度
- **性能问题伴随节点 NotReady** 或 Pod 频繁驱逐，呈扩散趋势
- **磁盘 IO 饱和导致 [[entities/etcd.md|etcd]] 响应慢**，影响整个集群稳定性

### 🟠 建议升级（P1）

- **应用级性能问题**涉及核心业务，但基础设施资源正常（需要应用开发团队介入）
- **内存泄漏**确认但无法定位到具体代码位置（需要开发团队排查）
- **数据库查询慢**导致应用性能差（需要 DBA 介入优化）
- **网络问题涉及底层基础设施**（交换机、VPC、ISP），超出 Kubernetes 层面
- **性能优化需要架构调整**（如引入缓存、消息队列、读写分离）

### 🟡 可能升级（P2→P1）

- **修复后性能仍未达标**，需要更深入的应用 profiling（如 Java 的 JFR / Go 的 pprof）
- **工程师对执行修复命令存在顾虑**（如担心影响线上业务）
- **修复步骤需要维护窗口**，但当前处于业务高峰
- **性能问题反复出现**，需要容量规划专家评估

### 升级话术

顾问："当前情况已超出本 [[skills/skill-k8s-node-notready-SKILL.md|Skill]] 的自主修复范围，建议立即升级。

**请执行以下操作**：
1. 通知值班经理 / 高级 SRE 团队 / 应用开发团队
2. 在工单系统中标记优先级为 P0/P1
3. 保持当前收集的所有诊断信息，准备交接
4. 如果需要，我可以协助整理当前已确认的性能瓶颈和已执行的诊断步骤

**当前已确认信息**：
- 影响范围：X 个 Pod / Y 个节点 / Z 个命名空间
- 瓶颈类型：CPU / 内存 / IO / 网络 / 应用
- 已排查根因：[已排除/已确认]
- 已尝试修复：[已执行的操作]
- 当前状态：[资源使用率 / 延迟指标]

请把这些信息同步给接手的高级工程师。"

---


### 分支 1.4：阿里云ACK/专有云性能排查

工程师："我们在阿里云ACK/专有云环境，集群性能有问题"

顾问："阿里云环境有额外的性能维度，请按以下顺序排查：

**步骤 1：阿里云ECS实例性能检查**
```bash
# 检查ECS实例规格和性能
aliyun ecs DescribeInstances --InstanceIds '["<id>"]'

# 检查云盘性能（IOPS/吞吐量）
aliyun ecs DescribeDiskMonitorData --DiskId <disk-id> --StartTime <time>

# 检查SLB性能指标
aliyun slb DescribeLoadBalancerHTTPListenerAttribute --LoadBalancerId <id>
```

> **如果无法执行aliyun CLI**：请登录阿里云控制台，告诉我：
> 1. ECS实例CPU/内存利用率趋势
> 2. 云盘IOPS和延迟指标
> 3. 是否有性能瓶颈告警？

**步骤 2：ACK节点性能检查**
```bash
# 检查节点规格是否匹配负载
kubectl describe node <node> | grep -A 5 "Allocated resources"

# 检查Pod资源限制是否合理
kubectl top pod -A --sort-by=cpu | head -20

# 检查节点磁盘IO
kubectl debug node/<node> -it --image=busybox -- df -h
```

**步骤 3：专有云性能特殊考虑**
- 专有云底层资源由飞天调度
- 检查飞天资源池状态
- 确认天基性能监控数据
- 检查物理机资源利用率

**步骤 4：阿里云特定优化**

如ECS规格不足：
```bash
# 升级ECS实例规格
aliyun ecs ModifyInstanceSpec --InstanceId <id> --InstanceType <type>

# 或使用ESS自动扩容
aliyun ess ExecuteScalingRule --ScalingRuleId <rule-id>
```

如云盘IOPS不足：
1. 升级云盘类型（普通→SSD→ESSD）
2. 启用云盘性能级别（PL0→PL1→PL2）
3. 使用本地SSD作为缓存

**阿里云控制台路径**：
- ECS监控：ECS控制台 → 实例详情 → 监控
- 云盘监控：ECS控制台 → 存储与快照 → 云盘详情
- ACK监控：ACK控制台 → 集群详情 → 节点监控


## 附录：常用命令速查

| 目的 | 命令 | 替代方案 |
|------|------|----------|
| 查看节点资源使用 | `kubectl top nodes` | Prometheus / 控制台 |
| 查看 Pod 资源使用 | `kubectl top pod <name> -n <ns>` | Prometheus / 控制台 |
| 查看 Pod 详情 | `kubectl describe pod <name> -n <ns>` | `kubectl get pod -o yaml` |
| 查看节点资源分配 | `kubectl describe node <name>` | `kubectl get node -o yaml` |
| 查看 Pod 日志 | `kubectl logs <name> -n <ns>` | 日志平台 / 节点日志文件 |
| 进入容器调试 | `kubectl exec -it <name> -n <ns> -- /bin/sh` | `kubectl debug` |
| SSH 节点检查 IO | `iostat -x 1 5` | `kubectl debug node/<name>` |
| 调整资源限制 | `kubectl edit deployment/<name>` | `kubectl apply -f` |
| 查看 HPA 状态 | `kubectl get hpa -n <ns>` | 控制台 |
| 查看 StorageClass | `kubectl get storageclass` | 控制台 |

---

> 本对话脚本基于 SKILL-PERF-001（K8s Performance Bottleneck 诊断与修复）设计。
> 完整根因目录参考 `reference/root-cause-catalog.md`
> 完整修复手册参考 `reference/remediation-playbook.md`

## 相关案例

- [[concepts/case-studies/2026-09-01-gpu-memory-leak.md|2026-09-01-gpu-memory-leak]]
- [[concepts/case-studies/2026-10-05-节点内核参数不一致导致sysctl配置冲突.md|2026-10-05-节点内核参数不一致导致sysctl配置冲突]]
## Related

- [[entities/deployment.md|Deployment]]
- [[entities/kubernetes.md|Kubernetes (CNCF Graduated)]]
