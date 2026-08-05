---
title: 症状 → SOP 映射手册
description: '- [C. 网络异常类](#c-网络异常类)'
summary: '- [C. 网络异常类](#c-网络异常类)'
category: troubleshooting
tags:
- k8s
- troubleshooting
- debugging
- fault-analysis
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- 症状 → SOP 映射手册 是什么
- 如何 症状 → SOP 映射手册
- Kubernetes 12 troubleshooting 最佳实践
- 症状 → SOP 映射手册 故障排查
- 症状 → SOP 映射手册 排障步骤
trigger_keywords:
- 症状
- SOP
- 映射手册
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- cilium-basics
- cni-basics
- etcd-basics
- redis-basics
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 症状 → SOP 映射手册

> **文档类型**: Agent 工单兜底手册 | **适用版本**: K8s 1.28-1.33 | **症状数**: 40+ | **最后更新**: 2026-05
> **使用场景**: 用户描述症状 → Agent 检索对应 SOP → 给出检查命令和修复步骤

---

<!-- chunk: 目录 -->
## 目录

- [A. Pod 异常类](#a-pod-异常类)
- [B. 节点异常类](#b-节点异常类)
- [C. 网络异常类](#c-网络异常类)
- [D. 存储异常类](#d-存储异常类)
- [E. 调度与资源类](#e-调度与资源类)
- [F. 安全/RBAC 类](#f-安全rbac-类)
- [G. 控制平面组件类](#g-控制平面组件类)
- [H. 滚动更新/发布类](#h-滚动更新发布类)
- [I. 可观测性/监控类](#i-可观测性监控类)
- [J. 杂项高频场景](#j-杂项高频场景)

---

<!-- chunk: A. Pod 异常类 -->
## A. Pod 异常类

### A-1. Pod 卡在 Pending 状态

**症状描述**: `kubectl get pod` 显示 Pod 一直是 Pending，超过 2 分钟无变化

**诊断路径**：

**路径 1: 资源不足导致调度失败**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -A5 "Conditions"
判断逻辑:
  - Conditions 中 Type=PodScheduled, Status=False, Reason=Unschedulable
  - Message 包含 "Insufficient cpu" 或 "Insufficient memory"
修复步骤:
  1. kubectl top nodes  # 查看节点资源使用率
  2. 如节点资源充足 → 可能是 requests 设置过高（降低 requests 或使用更大节点）
  3. 如节点资源不足 → 扩容节点池或减少其他 Pod 资源 requests
```
**路径 2: 污点未容忍**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

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
检查命令: kubectl describe node | grep -i taint
判断逻辑:
  - 节点有 NoSchedule 污点（如 node.kubernetes.io/not-ready）
  - Pod spec 中无对应 tolerations
修复步骤:
  1. kubectl describe pod <pod-name> 查看 tolerations
  2. kubectl taint nodes <node-name> <taint-key>-  # 临时移除污点（仅测试）
  3. 或在 Pod spec 中添加: tolerations: [{key: "<taint>", operator: "Exists"}]
```
**路径 3: PVC 可用区不匹配**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pvc <pvc-name>
判断逻辑:
  - Message 包含 "no available volume zone" 或 "volume node affinity conflict"
  - StorageClass 的 volumeBindingMode 未设为 WaitForFirstConsumer
修复步骤:
  1. 修改 StorageClass: volumeBindingMode: WaitForFirstConsumer
  2. 或在 Pod spec 中使用 nodeSelector 匹配 PVC 所在节点
```
**路径 4: 调度器异常**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get events --sort-by='.lastTimestamp' | grep -i scheduling
判断逻辑:
  - Events 显示 "skip scheduling because of unfinished predicates"
修复步骤:
  1. kubectl logs -n kube-system kube-scheduler-xxx --tail=50
  2. 等待 30s 后重试（调度器缓存同步延迟）
```
---

### A-2. Pod 卡在 ContainerCreating 状态

**症状描述**: `kubectl get pod` 显示 ContainerCreating，长时间未进入 Running

**诊断路径**：

**路径 1: 镜像拉取失败（最常见）**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -A3 "Containers"
判断逻辑:
  - State: Waiting, Reason: ImagePullBackOff
  - 或 Reason: ErrImagePull
修复步骤:
  1. 检查镜像地址是否正确: kubectl get pod -o jsonpath='{.spec.containers[*].image}'
  2. 在节点上手动测试: crictl pull <image>
  3. 如 403/401 → 检查 imagePullSecrets 是否正确配置
  4. 如 404 → 确认镜像 tag 存在
  5. 如 timeout → 检查节点到 registry 网络连通性
```
**路径 2: CNI 网络初始化失败**

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

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
检查命令: kubectl describe pod <pod-name> | grep -i sandbox
判断逻辑:
  - Events 包含 "Failed to create pod sandbox" + "network type"
修复步骤:
  1. kubectl get pods -n kube-system -l k8s-app=kube-cni  # CNI pod 状态
  2. 在节点上: cat /etc/cni/net.d/  # 检查 CNI 配置
  3. 重启 CNI 插件: systemctl restart kubelet
```
**路径 3: 存储卷挂载失败**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -i mount
判断逻辑:
  - Events 包含 "MountVolume.SetUp failed"
修复步骤:
  1. kubectl describe pvc <pvc-name>  # PVC 状态
  2. kubectl get pod -o yaml | grep -i volume  # 检查 volume 配置
  3. 检查 CSI driver 是否正常运行: kubectl get pods -n kube-system | grep csi
```
**路径 4: 安全上下文（SecurityContext）错误**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -i security
判断逻辑:
  - Events 包含 "container security context" + "denied"
修复步骤:
  1. 检查 Pod spec 的 securityContext 配置
  2. 检查 PSP（Pod Security Policy）是否允许该配置
  3. 调整 securityContext 或 PSP 配置
```
---

### A-3. Pod 处于 CrashLoopBackOff 状态

**症状描述**: Pod 反复重启，每次启动后立即崩溃

**诊断路径**：

**路径 1: 应用启动命令/参数错误**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl logs <pod-name> --previous  # 查看上一次容器的日志
判断逻辑:
  - 日志中包含 panic / exception / 错误堆栈
  - Exit Code 非 0（通常为 1）
修复步骤:
  1. 分析日志具体错误（如缺失配置文件、端口占用）
  2. 检查 Pod spec 的 command/args 是否正确
  3. 检查 Dockerfile 的 ENTRYPOINT/CMD
```
**路径 2: 依赖服务不可达**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl logs <pod-name>
判断逻辑:
  - 日志包含 "connection refused" / "dial tcp" / "no such host"
  - 应用需要连接数据库/Redis/其他微服务
修复步骤:
  1. 检查依赖服务是否正常运行: kubectl get svc
  2. 检查 Service/Endpoints 是否正确配置
  3. 在 Pod 内测试网络连通性: kubectl exec <pod> -- nslookup <service-name>
```
**路径 3: 健康检查（liveness/readiness）失败导致重启**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -i probe
判断逻辑:
  - 重启次数持续增加，Restart Count > 5
  - Events 中有 Liveness probe failed
修复步骤:
  1. 检查 livenessProbe/readinessProbe 配置
  2. 暂时禁用 livenessProbe（仅测试用），确认应用本身正常
  3. 调整 probe 的 initialDelaySeconds/failureThreshold
```
---

### A-4. Pod 被 Evicted（驱逐）

**症状描述**: `kubectl get pod` 显示 Evicted，`kubectl get events` 有 Evicted 记录

**诊断路径**：

**路径 1: 节点资源压力导致低优先级 Pod 被驱逐**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe node <node-name>
判断逻辑:
  - Node 有 MemoryPressure / DiskPressure / PIDPressure
  - 被驱逐的 Pod QoS 等级为 BestEffort 或 Burstable（低优先级）
修复步骤:
  1. 扩容节点池（增加节点或增大节点规格）
  2. 提高被驱逐 Pod 的 QoS 等级（设置 Guaranteed 的 resources）
  3. 降低节点上其他 Pod 的资源使用
```
**路径 2: 节点不可 pressure 但 Pod 仍被驱逐**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

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
检查命令: kubectl describe node <node-name> | grep -i taint
判断逻辑:
  - 节点有 NoExecute 污点（kubelet 自动添加的驱逐类污点）
修复步骤:
  1. 确认 Pod 的 tolerations 是否匹配节点污点
  2. 修复节点问题后，手动移除污点: kubectl taint nodes <node-name> <key>-
```
---

### A-5. Pod 处于 Terminating 状态无法删除

**症状描述**: `kubectl get pod` 显示 Terminating，超过 1 分钟未完成删除

**诊断路径**：

**路径 1: [[Finalizers|Finalizers]] 阻塞删除**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl get pod <pod-name> -o yaml | grep -i finalizers
判断逻辑:
  - metadata.deletionTimestamp 已设置
  - metadata.finalizers 非空
修复步骤:
  1. 分析是哪个 finalizer 阻塞（通常是自定义 Controller）
  2. 如需强制删除: kubectl patch pod <pod-name> -p '{"metadata":{"finalizers":null}}'
  3. 或联系 Controller 所有者解决
```
**路径 2: 容器优雅终止超时**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -i signal
判断逻辑:
  - Pod 有 SIGTERM 记录但进程未响应
  - terminationGracePeriodSeconds 内未完成
修复步骤:
  1. 增加 terminationGracePeriodSeconds
  2. 检查应用是否正确处理 SIGTERM 信号
  3. 如确认可强制删除: kubectl delete pod <pod-name> --grace-period=0
```
**路径 3: PVC 阻塞 Pod 删除**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
检查命令: kubectl describe pod <pod-name> | grep -i volume
判断逻辑:
  - Pod 卡在 Terminating 且有 volume mount
修复步骤:
  1. 检查 PVC 是否卡在 Terminating: kubectl get pvc
  2. 检查 CSI driver 是否正常
  3. 强制删除（不推荐用于有状态应用）: kubectl delete pod <pod-name> --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
```
---

### A-6. Pod 的 Ready 状态为 False

**症状描述**: `kubectl get pod` 显示 `0/1` Ready，持续不恢复

**诊断路径**：

**路径 1: 应用未通过就绪探针**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe pod <pod-name> | grep -i "Ready"
判断逻辑:
  - Conditions 中 Type=Ready, Status=False
  - 重启次数可能不高（进程未崩溃，但探针失败）
修复步骤:
  1. 检查 readinessProbe 配置
  2. 在 Pod 内手动测试探针端点: kubectl exec <pod> -- curl -s localhost:<probe-port>
  3. 调整 initialDelaySeconds / periodSeconds / threshold
```
**路径 2: Pod 处于启动中（刚创建，探针还在初始化中）**
```
判断逻辑:
  - 启动时间 < initialDelaySeconds
  - 这是正常现象，等待初始化完成后 Ready 会变为 True
修复步骤:
  - 无需处理，等待即可（通常 30s-2min 内恢复）
```

---

### A-7. OOMKilled（内存超限杀死）

**症状描述**: Pod 状态显示 OOMKilled，容器退出码 137

**诊断路径**：

**路径 1: Pod memory limit 设置过低**
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl top pod <pod-name> -n <namespace>
判断逻辑:
  - 实际内存使用接近或等于 limit
  - container 用量 + overhead > memory limit
修复步骤:
  1. 增加 memory limit: kubectl set resources pod <pod-name> -limit=memory=2Gi
  2. 或优化应用内存使用（JVM heap 调优等）
```
**路径 2: 应用内存泄漏**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl top pod --sort-by=memory -n <namespace> | head -10
判断逻辑:
  - 内存使用持续增长，每次重启后继续上升
修复步骤:
  1. 分析堆内存 dump（jmap / jhat）
  2. 定位内存泄漏代码
  3. 修复后重新部署
```
---

<!-- chunk: B. 节点异常类 -->
## B. 节点异常类

### B-1. 节点处于 NotReady 状态

**症状描述**: `kubectl get node` 显示节点 Ready=False，持续不恢复

**诊断路径**：

**路径 1: Kubelet 进程崩溃**

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

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
检查命令: SSH 到节点: systemctl status kubelet
判断逻辑:
  - kubelet 服务状态不是 active (running)
修复步骤:
  1. systemctl restart kubelet
  2. journalctl -u kubelet --since "30 minutes ago" | tail -50  # 查看崩溃日志
  3. 如因 OOM → 增加节点内存或减少 Pod 数量
```
**路径 2: 节点网络分区**
```
检查命令: SSH 到节点: ping -c 3 8.8.8.8
判断逻辑:
  - 节点无法访问外部网络，API Server 心跳中断
修复步骤:
  1. 检查节点防火墙/安全组规则
  2. 检查节点网络配置（路由表、VPC 配置）
  3. 如集群网络插件异常 → 重启网络插件
```

**路径 3: [[etcd|etcd]] 心跳超时（控制平面节点）**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get nodes -o wide | grep <node-name>
判断逻辑:
  - 节点是控制平面节点，Ready=False 伴随 etcd 健康检查失败
修复步骤:
  1. 检查 etcd 日志: kubectl logs -n kube-system etcd-<node-name> --tail=50
  2. 检查磁盘 I/O: iostat -x 1 5
  3. 如磁盘延迟过高 → 优化磁盘或移动 etcd 到更高性能存储
```
---

### B-2. 节点无法新增 Pod（NoSchedule）

**症状描述**: 节点 Ready 但新 Pod 调度到此节点时报错 `didn't tolerate`

**诊断路径**：

**路径 1: 节点有 NoSchedule 污点**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

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
检查命令: kubectl describe node <node-name> | grep -A5 Taints
判断逻辑:
  - Taints 包含 node.kubernetes.io/not-ready:NoSchedule
修复步骤:
  1. 确认节点已恢复健康
  2. 自动移除: kubectl taint nodes <node-name> node.kubernetes.io/not-ready-
```
**路径 2: 磁盘/内存压力大**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe node <node-name> | grep -i pressure
判断逻辑:
  - Conditions 有 MemoryPressure=True 或 DiskPressure=True
修复步骤:
  1. kubectl top nodes 查看资源使用率
  2. 清理节点磁盘空间或增加节点资源
  3. 如问题持续 → cordon 节点进行维护
```
---

### B-3. 节点上部分 Pod 无法启动（其他 Pod 正常）

**诊断路径**：

**路径 1: 该 Pod 的资源 requests 超过了节点的 allocatable**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe node <node-name> | grep -A5 "Allocatable"
判断逻辑:
  - Pod 的 resources.requests.cpu/memory > node allocatable
修复步骤:
  1. 修改 Pod 的 resource requests
  2. 或使用更大规格的节点
```
**路径 2: Pod 有节点亲和性要求但无匹配节点**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe pod <pod-name> | grep -i nodeSelector
判断逻辑:
  - Pod 指定了 nodeSelector，但该节点无对应 label
修复步骤:
  1. 添加节点 label: kubectl label nodes <node-name> <label-key>=<label-value>
  2. 或调整 Pod 的 nodeSelector
```
---

<!-- chunk: C. 网络异常类 -->
## C. 网络异常类

### C-1. Service 无法访问（内部）

**症状描述**: 从一个 Pod 访问 ClusterIP Service 超时或 Connection refused

**诊断路径**：

**路径 1: Service 背后无健康 Pod（Endpoints 为空）**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get endpoints -n <namespace> <svc-name>
判断逻辑:
  - ENDPOINTS 列显示 <none> 或为空
修复步骤:
  1. kubectl get pods -n <namespace> -l <selector>  # 确认 Pod 是否存在
  2. kubectl describe pod <pod-name> | grep -i "Conditions"  # 检查 Pod 是否 Ready
  3. 如 Pod 存在但不 Ready → 按 Pod 异常类排查
```
**路径 2: kube-proxy 异常**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl logs -n kube-system kube-proxy-<node-name> --tail=50
判断逻辑:
  - kube-proxy 日志中有异常或报错
修复步骤:
  1. kubectl get daemonset kube-proxy -n kube-system  # 确认 DaemonSet 健康
  2. 检查 iptables/ipvs 规则: SSH 到节点执行 iptables-save | grep <svc-name>
  3. 重启 kube-proxy: kubectl delete pod -n kube-system -l k8s-app=kube-proxy
```
**路径 3: 网络策略（[[NetworkPolicy|NetworkPolicy]]）阻止**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get networkpolicy -n <namespace>
判断逻辑:
  - 存在 NetworkPolicy 限制了入站/出站流量
修复步骤:
  1. kubectl describe networkpolicy <np-name> -n <namespace>  # 查看策略详情
  2. 临时禁用 NetworkPolicy 测试
  3. 修正 policy 规则，确保允许所需流量
```
**路径 4: CoreDNS/Corefile 配置错误**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl exec -it <pod> -- nslookup kubernetes.default
判断逻辑:
  - nslookup 超时或返回 SERVFAIL
修复步骤:
  1. kubectl get pods -n kube-system -l k8s-app=kube-dns  # CoreDNS 状态
  2. kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50  # 查看日志
  3. kubectl describe configmap coredns -n kube-system  # 检查 Corefile 配置
```
---

### C-2. Ingress 访问返回 503/502/504

**症状描述**: 外部访问 Ingress 返回 503 Service Temporarily Unavailable 或 502/504

**诊断路径**：

**路径 1: 所有 Backend Pod 均不健康**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get endpoints -n <namespace> <ingress-backend-svc>
判断逻辑:
  - ENDPOINTS 为空或有部分 IP 但都不是 Ready
修复步骤:
  1. kubectl get pods -n <namespace> -l <selector>  # 检查 Pod 状态
  2. 如所有 Pod 均不 Ready → 按 Pod 异常类排查
```
**路径 2: Health Check 配置错误导致所有 Pod 被标记为 Unhealthy**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe ingress <name> -n <namespace>
判断逻辑:
  - Ingress 配置了 health check 但路径/端口错误
修复步骤:
  1. 检查 Ingress annotations 中的 health check 配置
  2. 在 Pod 内手动测试 health check 路径: kubectl exec <pod> -- curl -s <health-check-path>
  3. 修正配置或关闭 health check
```
**路径 3: Ingress Controller 自身异常**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n ingress-nginx
判断逻辑:
  - ingress-nginx-controller pod 不在 Running 状态
修复步骤:
  1. kubectl logs -n ingress-nginx ingress-nginx-controller-xxx --tail=50
  2. 检查资源限制（CPU/内存）是否不足
  3. 检查 ConfigMap 配置是否错误
```
**路径 4: TLS 证书问题**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe ingress <name> -n <namespace> | grep -i tls
判断逻辑:
  - TLS secret 不存在或已过期
修复步骤:
  1. kubectl get secret -n <namespace> | grep <tls-secret-name>
  2. 检查证书到期时间: kubectl describe secret <tls-secret> -n <namespace>
  3. 更新证书
```
---

### C-3. DNS 解析失败（集群内部）

**症状描述**: Pod 内 `nslookup <service-name>` 超时或返回 SERVFAIL

**诊断路径**：

**路径 1: CoreDNS Pod 不健康**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n kube-system -l k8s-app=kube-dns
判断逻辑:
  - CoreDNS pod 不在 Running 状态
修复步骤:
  1. kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50
  2. kubectl describe configmap coredns -n kube-system  # 检查 Corefile
```
**路径 2: Service 的域名格式错误**
```
判断逻辑:
  - 尝试解析的域名格式不标准（应使用 <svc-name>.<namespace>.svc.cluster.local）
修复步骤:
  1. 使用完整域名测试: nslookup my-svc.my-ns.svc.cluster.local
  2. 确认使用正确的集群内部域名格式
```

---

### C-4. Pod 之间网络不通（跨节点）

**症状描述**: Node A 上的 Pod 无法访问 Node B 上的 Pod

**诊断路径**：

**路径 1: CNI 插件异常**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n kube-system -l k8s-app=...-cni
判断逻辑:
  - CNI Pod（如 calico-node/cilium-agent）不健康
修复步骤:
  1. SSH 到问题节点: systemctl status <cni-service>
  2. 检查 CNI 配置: cat /etc/cni/net.d/
  3. 重启 CNI 服务或重置 CNI 配置
```
**路径 2: 节点安全组/VPC 路由问题**
```
检查命令: SSH 到目标节点: ping -c 3 <other-node-ip>
判断逻辑:
  - 节点间网络不通（跨节点通信基础链路）
修复步骤:
  1. 检查节点安全组规则（允许跨节点通信的端口）
  2. 检查 VPC 路由表
  3. 联系网络管理员
```

---

<!-- chunk: D. 存储异常类 -->
## D. 存储异常类

### D-1. PVC 处于 Pending 状态无法绑定

**症状描述**: `kubectl get pvc` 显示 Pending，超过 5 分钟无变化

**诊断路径**：

**路径 1: StorageClass 不存在或 Provisioner 异常**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get sc
判断逻辑:
  - StorageClass 不存在
  - 或 Provisioner driver 未启动
修复步骤:
  1. kubectl get pods -n kube-system | grep csi  # CSI driver 状态
  2. 如无对应 CSI driver → 安装云厂商 CSI
  3. 使用默认 StorageClass: kubectl get storageclass
```
**路径 2: 集群存储配额耗尽**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe namespace <namespace> | grep -i "capacity"
判断逻辑:
  - 命名空间的存储资源配额已达到上限
修复步骤:
  1. kubectl get resourcequota -n <namespace>
  2. 联系集群管理员增加存储配额
```
**路径 3: PVC 的 volumeBindingMode 与拓扑不匹配**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe storageclass <sc-name>
判断逻辑:
  - volumeBindingMode: Immediate 但集群无节点满足拓扑要求
修复步骤:
  1. 修改 StorageClass: volumeBindingMode: WaitForFirstConsumer
  2. 或配置正确的 allowedTopologies
```
---

### D-2. Pod 挂载 PVC 失败

**症状描述**: Pod 启动失败，Events 显示 MountVolume.SetUp failed

**诊断路径**：

**路径 1: PVC 未正确绑定**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pvc <pvc-name>
判断逻辑:
  - PVC STATUS 不是 Bound
修复步骤:
  1. 按 D-1 排查 PVC Pending 问题
  2. 确保 PVC 已 Bound 后再重新创建 Pod
```
**路径 2: CSI driver 问题**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n kube-system | grep csi
判断逻辑:
  - CSI driver pod 异常或重启中
修复步骤:
  1. kubectl logs -n kube-system <csi-driver-pod> --tail=50
  2. 检查云厂商控制台存储卷状态
```
---

### D-3. 存储卷性能下降（延迟高/IOPS 低）

**诊断路径**：

**路径 1: 云盘/存储卷性能等级选择错误**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pvc <pvc-name> | grep -i "type"
判断逻辑:
  - 使用了低性能存储等级（SSD PL0 vs PL1/PL2）
修复步骤:
  1. 确认业务对 IOPS 的需求
  2. 切换到更高性能等级的存储（如 ESSD PL1 → PL2）
  3. 注意：云盘性能等级变更通常需要重新创建卷
```
---

<!-- chunk: E. 调度与资源类 -->
## E. 调度与资源类

### E-1. Pod 调度成功但无法启动（资源争抢）

**症状描述**: Pod 被调度到某节点但立即被 Evicted 或进入 CrashLoop

**诊断路径**：

**路径 1: 节点资源不足，kubelet 主动驱逐**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe node <node-name> | grep -i pressure
判断逻辑:
  - Node 有 MemoryPressure / DiskPressure
修复步骤:
  1. kubectl top nodes  # 查看资源使用率
  2. 扩容节点或减少节点上 Pod 数量
  3. 提高 Pod 的 resource requests（增加 QoS 等级）
```
---

### E-2. HPA 不扩缩容（指标正常但无反应）

**症状描述**: CPU 使用率 > 80%（超过 target）但 Pod 数量不变

**诊断路径**：

**路径 1: 已达到 MaxReplicas**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl get hpa -n <namespace>
判断逻辑:
  - REPLICAS 已达到 MAX 值
修复步骤:
  1. 检查 MAX 值是否合理: kubectl describe hpa <name> | grep -i "max"
  2. 如需更高上限: kubectl patch hpa <name> -p '{"spec":{"maxReplicas":10}}'
```
**路径 2: 指标采集异常（metrics-server 问题）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get apiservices | grep metrics
判断逻辑:
  - metrics.k8s.io APIService 不可用
修复步骤:
  1. kubectl get pods -n kube-system | grep metrics-server
  2. kubectl logs -n kube-system metrics-server-xxx --tail=50
  3. 重启 metrics-server: kubectl delete pod -n kube-system -l k8s-app=metrics-server
```
**路径 3: Pod 未设置 resource requests（无法触发扩缩容）**
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe pod <pod-name> | grep -i "requests"
判断逻辑:
  - Pod 没有设置 cpu/memory requests
修复步骤:
  1. 为 Pod 添加 resource requests: kubectl set resources pod <pod-name> --requests=cpu=100m,memory=128Mi
  2. 或在 Deployment spec 中设置 resources.requests
```
---

### E-3. Pod 无法调度（多个节点均不可用）

**症状描述**: Pod 调度失败，Message 显示 0/N nodes available

**诊断路径**：

**路径 1: 集群整体资源不足**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe node  # 查看所有节点 allocatable
判断逻辑:
  - 所有节点的 allocatable 资源总和 < Pod 的 requests
修复步骤:
  1. 扩容集群（增加节点）
  2. 降低 Pod 的 resource requests
```
**路径 2: 存在特殊污点导致所有节点不可用**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get nodes -o jsonpath='{.items[*].spec.taints[*]}'
判断逻辑:
  - 所有节点都有污点，且 Pod 没有对应的 tolerations
修复步骤:
  1. 检查 Pod spec 的 tolerations
  2. 如需跳过所有污点: tolerations: [{operator: "Exists"}]
```
---

### E-4. ResourceQuota / LimitRange 限制触发

**症状描述**: 创建资源时报错 "exceeded quota" 或 "cannot update limits"

**诊断路径**：

**路径 1: 命名空间 ResourceQuota 限制**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe resourcequota -n <namespace>
判断逻辑:
  - Status 中显示某类资源已超硬性限制
修复步骤:
  1. kubectl get resourcequota -n <namespace>
  2. 联系集群管理员增加 quota
  3. 或清理命名空间中不再使用的资源
```
**路径 2: LimitRange 默认值限制**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe limitrange -n <namespace>
判断逻辑:
  - Pod 的 resource limit 超过了 LimitRange 的最大值
修复步骤:
  1. 降低 Pod 的 resource requests/limits
  2. 或修改 LimitRange 的 max 值
```
---

<!-- chunk: F. 安全/RBAC 类 -->
## F. 安全/RBAC 类

### F-1. RBAC 权限不足（Forbidden）

**症状描述**: 执行 kubectl 操作报错 "forbidden" / "Unauthorized"

**诊断路径**：

**路径 1: ServiceAccount 权限不足**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<ns>:<sa-name>
判断逻辑:
  - 返回 "no"，说明该 SA 无此操作权限
修复步骤:
  1. 创建 Role/ClusterRole 并绑定到 SA: kubectl create role <role-name> --verb=get,list --resource=pods -n <namespace>
  2. kubectl create rolebinding <name> --role=<role-name> --serviceaccount=<ns>:<sa-name> -n <namespace>
```
**路径 2: 使用的 kubeconfig 没有正确凭证**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl config view
判断逻辑:
  - kubeconfig 中用户凭证为空或过期
修复步骤:
  1. 更新 kubeconfig 中的证书/token
  2. 重新认证: kubectl login <api-server-url>
```
---

### F-2. ServiceAccount Token 无法使用（401/403）

**症状描述**: 使用 ServiceAccount Token 访问 API 返回认证失败

**诊断路径**：

**路径 1: Token 已过期（K8s 1.22+ ServiceAccount Token 不自动过期，但手动创建的可能会）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get secret -n <namespace> | grep <sa-name>
判断逻辑:
  - Secret 类型为 kubernetes.io/service-account-token 但 token 已过期
修复步骤:
  1. 删除旧 Secret 并重新创建: kubectl delete secret <secret-name> -n <namespace>
  2. K8s 会自动创建新 token（通过 Annotations 触发）
```
**路径 2: 误用 user token 而非 SA token**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
判断逻辑:
  - kubeconfig 使用的是某用户的凭证，不是 ServiceAccount
修复步骤:
  1. 使用 SA 方式: kubectl create token <sa-name> -n <namespace>
  2. 或检查 kubeconfig 上下文配置
```
---

### F-3. Pod 无法挂载 Secret/ConfigMap

**症状描述**: Pod 启动失败，Events 显示 "Invalid value" "must be positive"

**诊断路径**：

**路径 1: Secret/ConfigMap 不存在**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get configmap <name> -n <namespace>
判断逻辑:
  - ConfigMap 不存在，但 Pod spec 引用了它
修复步骤:
  1. 创建 ConfigMap，或修正 Pod spec 中的引用名称
```
**路径 2: 引用了已删除的 key**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get configmap <name> -n <namespace> -o yaml | grep <key-name>
判断逻辑:
  - ConfigMap 中不存在 Pod spec 中引用的 key
修复步骤:
  1. 在 ConfigMap 中添加缺失的 key，或修正 Pod spec 中的 key 名称
```
---

<!-- chunk: G. 控制平面组件类 -->
## G. 控制平面组件类

### G-1. API Server 无响应

**症状描述**: kubectl 命令超时或返回 "connection refused"

**诊断路径**：

**路径 1: API Server Pod 未运行**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n kube-system | grep kube-apiserver
判断逻辑:
  - kube-apiserver-<node> pod 不在 Running 状态
修复步骤:
  1. kubectl describe pod -n kube-system kube-apiserver-<node-name>  # 查看 Events
  2. 检查 etcd 连接: kubectl logs -n kube-system kube-apiserver-<node> --tail=20 | grep etcd
  3. 如证书问题 → 重启 API Server（kubeadm 会自动重新加载证书）
```
**路径 2: kube-apiserver 进程崩溃（etcd 连接问题）**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: SSH 到控制平面节点: systemctl status kube-apiserver
判断逻辑:
  - kube-apiserver 服务状态不是 active
修复步骤:
  1. journalctl -u kube-apiserver --since "10 minutes ago" | tail -50
  2. 检查 etcd 健康: ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key endpoint health
```
---

### G-2. kube-scheduler 不调度新 Pod

**症状描述**: 新 Pod 全部卡在 Pending，调度器无响应

**诊断路径**：

**路径 1: kube-scheduler 异常**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n kube-system | grep kube-scheduler
判断逻辑:
  - kube-scheduler pod 不在 Running 状态
修复步骤:
  1. kubectl logs -n kube-system kube-scheduler-<node> --tail=50
  2. 检查调度器配置（kube-scheduler.conf）是否存在
  3. 重启调度器: kubectl delete pod -n kube-system -l k8s-app=kube-scheduler
```
---

### G-3. kube-controller-manager 异常

**症状描述**: Deployment/ReplicaSet 不创建 Pod，Service 不更新 Endpoints

**诊断路径**：

**路径 1: kube-controller-manager 异常**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n kube-system | grep kube-controller-manager
判断逻辑:
  - kube-controller-manager pod 不在 Running 状态
修复步骤:
  1. kubectl logs -n kube-system kube-controller-manager-<node> --tail=50
  2. 检查 controller manager 配置和证书
  3. 重启: kubectl delete pod -n kube-system -l k8s-app=kube-controller-manager
```
---

<!-- chunk: H. 滚动更新/发布类 -->
## H. 滚动更新/发布类

### H-1. Deployment 滚动更新卡住

**症状描述**: `kubectl get deployment` 显示 UP-TO-DATE 未全部完成， AVAILABLE 低于 Desired

**诊断路径**：

**路径 1: 新 Pod 启动失败（ImagePullBackOff / CrashLoopBackOff）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pods -n <namespace> -l app=<name>
判断逻辑:
  - 新版本 Pod 处于异常状态（Waiting/CrashLoopBackOff）
修复步骤:
  1. kubectl describe pod <new-pod-name> 查看具体错误
  2. 修复镜像或配置问题
  3. 回滚: kubectl rollout undo deployment/<name> -n <namespace>
```
**路径 2: maxSurge/maxUnavailable 配置导致无法推进**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe deployment <name> | grep -i "strategy"
判断逻辑:
  - maxSurge=0 且 maxUnavailable=0（不可能的状态）或设置过于保守
修复步骤:
  1. 调整滚动更新策略: kubectl patch deployment <name> -p '{"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxSurge":1,"maxUnavailable":0}}}}'
  2. 或手动推进: kubectl rollout pause deployment / kubectl rollout resume deployment
```
**路径 3: PDB (PodDisruptionBudget) 保护导致无法替换**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl get pdb -n <namespace>
判断逻辑:
  - 存在 PDB 限制了最大中断数，新 Pod 无法创建
修复步骤:
  1. 暂时放宽 PDB: kubectl patch pdb <name> -p '{"spec":{"maxUnavailable":1}}'
  2. 完成后恢复
```
---

### H-2. Deployment 回滚失败

**症状描述**: `kubectl rollout undo` 执行后仍无法恢复正常

**诊断路径**：

**路径 1: 回滚的目标版本本身有问题**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl rollout history deployment/<name> -n <namespace>
判断逻辑:
  - 当前版本和回滚版本都有问题
修复步骤:
  1. 查看历史版本: kubectl rollout history deployment/<name> --revision=3
  2. 选择更早的健康版本: kubectl rollout undo deployment/<name> --to-revision=<N>
  3. 或手动修复镜像/配置后重新部署
```
---

### H-3. Helm Release 升级失败

**症状描述**: `helm upgrade` 执行报错，Release 处于 failed 状态

**诊断路径**：

**路径 1: 模板渲染错误（values 配置问题）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: helm diff upgrade <release> <chart> -n <namespace> --show-only <templates>
判断逻辑:
  - helm 报错 YAMLSyntaxError / template 错误
修复步骤:
  1. 检查 values 文件语法
  2. 使用 --dry-run 预检查: helm upgrade --dry-run --debug <release> <chart>
  3. 修正 values 后重试
```
**路径 2: 依赖版本冲突**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: helm dependency build <chart>
判断逻辑:
  - chart 依赖的子 chart 版本与集群不兼容
修复步骤:
  1. 更新 chart 依赖: helm dependency update <chart>
  2. 或降级 Helm 版本
```
---

<!-- chunk: I. 可观测性/监控类 -->
## I. 可观测性/监控类

### I-1. Prometheus 指标丢失

**症状描述**: Prometheus 中找不到某 Pod/Services 的指标

**诊断路径**：

**路径 1: ServiceMonitor / PodMonitor 未正确配置**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get servicemonitor -n <namespace>
判断逻辑:
  - ServiceMonitor 不存在或 selector 不匹配
修复步骤:
  1. 确认应用已暴露 metrics 端点（通常 /metrics）
  2. 创建/修正 ServiceMonitor 的 selector
  3. 检查 endpoints 配置中 port/path 是否正确
```
**路径 2: Prometheus 未发现 ServiceMonitor**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl logs -n monitoring prometheus-prometheus-0 --tail=50 | grep <service-monitor-name>
判断逻辑:
  - Prometheus 日志中没有该 ServiceMonitor 的加载记录
修复步骤:
  1. 确认 ServiceMonitor 在正确的 namespace（Prometheus 扫描范围）
  2. 检查 Prometheus 配置（PodMonitor 需 PrometheusAgent 开启）
```
---

<!-- chunk: J. 杂项高频场景 -->
## J. 杂项高频场景

### J-1. kubectl exec / attach 失败

**症状描述**: `kubectl exec -it <pod> -- /bin/bash` 报错 "unable to upgrade connection"

**诊断路径**：

**路径 1: API Server 与 Pod 之间的隧道中断**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get node  # 确认节点状态
判断逻辑:
  - Pod 所在节点 NotReady 或网络异常
修复步骤:
  1. 检查节点状态: kubectl describe node <node-name>
  2. SSH 到节点检查 kubelet 状态
```
**路径 2: RBAC 权限不足**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl auth can-i exec pod --as=system:serviceaccount:<ns>:<sa>
判断逻辑:
  - 返回 no
修复步骤:
  1. 创建 Role 并绑定: kubectl create role pod-exec --verb=get,create --resource=pods/exec -n <namespace>
  2. kubectl create rolebinding <name> --role=pod-exec --serviceaccount=<ns>:<sa> -n <namespace>
```
---

### J-2. kubectl cp 文件传输失败

**症状描述**: `kubectl cp local-file <pod>:/path` 报错 "unable to upgrade connection"

**诊断路径**：

**路径 1: Pod 内容器未启动（容器运行时问题）**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pod <pod-name>
判断逻辑:
  - Pod 状态不是 Running
修复步骤:
  1. 解决 Pod 启动问题后再尝试 cp
  2. 或使用其他方式传输（如 wget/curl 从 Pod 内下载）
```
---

### J-3. kubelet 日志显示 "PLEG is not healthy"

**症状描述**: 节点 NotReady，日志中有 PLEG 不健康报错

**诊断路径**：

**路径 1: 容器运行时（containerd/cri-o）异常导致 PLEG 检测失败**

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

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
检查命令: SSH 到节点: systemctl status containerd
判断逻辑:
  - containerd 服务异常或重启中
修复步骤:
  1. systemctl restart containerd
  2. journalctl -u containerd --since "10 minutes ago" | tail -50
```
---

<!-- chunk: K. kubectl cp / apply 错误类 -->
## K. kubectl cp / apply 错误类

### K-1. kubectl cp 上传/下载文件失败

**症状描述**: `kubectl cp` 执行时报 "unable to upgrade connection" 或 "not a tar archive"

**诊断路径**：

**路径 1: 容器未运行**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get pod <pod-name> -n <namespace>
判断逻辑:
  - Pod 状态不是 Running
修复步骤:
  1. 先解决 Pod 启动问题（按 A 类 Pod 异常排查）
  2. Pod 进入 Running 后再执行 cp
expected_output: "kubectl get pod 显示 1/1 Running"  # 正常
```
**路径 2: 文件格式错误（复制二进制文件时）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl cp <pod>:/path/file ./local -n <namespace>
判断逻辑:
  - 报错 "not a tar archive"
修复步骤:
  1. 使用 tar 打包后传输: kubectl exec <pod> -- tar cf - /path/file | tar xf - -C /local
  2. 或使用 base64 编码: kubectl exec <pod> -- base64 /path/file 输出后本地解码
expected_output: "文件成功复制，无错误"  # 正常
```
**路径 3: 路径不存在**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl exec <pod> -- ls -la /path
判断逻辑:
  - 容器内 /path 目录或文件不存在
修复步骤:
  1. 确认目标路径正确（区分相对路径和绝对路径）
  2. cp 时加 -n 指定命名空间: kubectl cp <file> <ns>/<pod>:/path
expected_output: "ls 输出文件存在"  # 正常
```
---

### K-2. kubectl apply 报 field is forbidden / missing required field

**症状描述**: `kubectl apply -f xxx.yaml` 报错 "unknown field" 或 "missing required field"

**诊断路径**：

**路径 1: 字段名拼写错误**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl apply -f deployment.yaml --validate=true --dry-run=client
判断逻辑:
  - 报错显示 "unknown field 'replics'"（应为 replicas）
修复步骤:
  1. 使用 kubectl explain deployment.spec 确认正确字段名
  2. 修正 YAML 中的字段名后重试
  3. 使用 kubey: kubectl apply --dry-run=server 在 apiserver 端验证（更严格）
expected_output: "dry-run 无报错"  # 正常
```
**路径 2: 字段不可变更（已存在的资源）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl get <resource> <name> -o yaml | grep <field>
判断逻辑:
  - 报错 "is immutable" 或 "field is forbidden"
修复步骤:
  1. 如是 PVC 的 storageClassName → 删除重建（PVC 不支持 update）
  2. 如是 Deployment 的 selector → 不可变更，需重建 Deployment
  3. 使用 patch 替代 apply: kubectl patch -f deployment.yaml
expected_output: "patch 成功返回 updated"  # 正常
```
**路径 3: 缺少必需字段**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl apply -f deployment.yaml
判断逻辑:
  - 报错 "missing required field 'selector'"
修复步骤:
  1. 检查 YAML 的 spec.selector 是否存在（Deployment 创建后 selector 不可改）
  2. 使用 kubectl create --dry-run=client -f deployment.yaml 查看缺少的字段
expected_output: "create/dry-run 无报错"  # 正常
```
---

### K-3. Pod 内 curl localhost:<port> 返回 404（应用层问题）

**症状描述**: Pod 内访问 localhost:8080 返回 404，但 Pod 就绪、Service 正常

**诊断路径**：

**路径 1: 应用本身路由配置问题（K8s 外部问题）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl exec <pod-name> -- curl -s localhost:<port>/<path>
判断逻辑:
  - 应用返回 404，说明应用本身没有该路由
修复步骤:
  1. kubectl logs <pod-name> 查看应用日志，确认路由注册情况
  2. 检查应用的配置文件（configmap/环境变量）中的 base path
  3. 如是 Java Spring Boot: 确认 server.servlet.context-path
  4. 如是 Node.js Express: 确认 app.use() 路由注册顺序
expected_output: "curl 返回 200/301/302（非 404/500）"  # 正常
```
**路径 2: 应用启动时端口绑定到 0.0.0.0 而非 127.0.0.1**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl exec <pod-name> -- netstat -tlnp | grep <port>
判断逻辑:
  - 端口监听在 0.0.0.0:8080 而非 127.0.0.1:8080（正常），但 curl localhost 仍 404
修复步骤:
  1. 确认应用正常加载了路由配置
  2. 检查应用健康检查路径（如 /actuator/health /health）是否为 200
  3. 如有 sidecar 容器，sidecar 可能拦截了 localhost 流量
expected_output: "netstat 显示端口监听，应用返回 2xx/3xx"  # 正常
```
---

<!-- chunk: L. 调度与容量规划类 -->
## L. 调度与容量规划类

### L-1. HPA 扩到 maxReplicas 但 CPU/Memory 仍然高

**症状描述**: HPA 显示 REPLICAS == MAX，但仍触发扩容（CPU 持续 > 80%）

**诊断路径**：

**路径 1: 扩容上限设置不足**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl get hpa <name> -n <namespace>
判断逻辑:
  - MAX 值设置过低，无法满足当前业务负载
修复步骤:
  1. 检查 maxReplicas 是否合理（考虑当前峰值 QPS / 单 Pod 处理能力）
  2. 计算: maxReplicas >= 峰值 QPS / 单 Pod QPS
  3. 临时方案: kubectl patch hpa <name> -p '{"spec":{"maxReplicas":20}}'
  4. 长期方案: 重新评估容量规划，调整应用架构或规格
expected_output: "扩容后 CPU% 下降到 < 80%，AP 收敛"  # 正常
```
**路径 2: 应用存在内存泄漏或异常导致资源消耗持续增长**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl top pods -n <namespace> --sort-by=memory | head -10
判断逻辑:
  - 内存使用持续上升，即使扩容也无法解决
修复步骤:
  1. 抓取 heap dump 分析内存泄漏: kubectl exec <pod> -- jmap -dump:format=b,file=/tmp/heap.hprof <pid>
  2. 检查是否有连接池泄漏（数据库/Redis 连接未释放）
  3. 修复应用代码后重新部署
expected_output: "资源使用率稳定，HPA 不再持续扩容"  # 正常
```
**路径 3: metrics-server 采集延迟导致 HPA 决策滞后**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl logs -n kube-system metrics-server-xxx --tail=20
判断逻辑:
  - metrics-server 延迟导致 HPA 基于旧数据决策
修复步骤:
  1. 检查 metrics-server 是否正常: kubectl get pods -n kube-system | grep metrics-server
  2. 重启 metrics-server: kubectl delete pod -n kube-system -l k8s-app=metrics-server
expected_output: "metrics-server Running 且无异常日志"  # 正常
```
---

### L-2. CronJob 未触发（时区/schedule 问题）

**症状描述**: CronJob 的下一次执行时间超过预期，或完全未触发

**诊断路径**：

**路径 1: schedule 时区未设置（默认 UTC）**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe cronjob <name> -n <namespace> | grep -i schedule
判断逻辑:
  - schedule 使用了 "0 9 * * *"（以为是北京时间 9 点，实际是 UTC 9 点 = 北京时间 17 点）
修复步骤:
  1. 添加 spec.timeZone: "Asia/Shanghai"（K8s 1.27+）
  2. 或将 schedule 转换为 UTC 时间（如北京时间 9 点 → UTC 1 点 → "0 1 * * *"）
expected_output: "kubectl get cronjob <name> -o jsonpath='{.status.nextScheduleTime}' 显示下次执行时间正确"  # 正常
```
**路径 2: 上一批次 Job 未完成（ConcurrencyPolicy=Forbid）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl get jobs -n <namespace> | grep <cronjob-name>
判断逻辑:
  - 有历史 Job 处于 Running 状态，新 Job 被禁止（ConcurrencyPolicy=Forbid）
修复步骤:
  1. 检查 Running Job: kubectl get pods -n <namespace> -l job-name=<job-name>
  2. 如 Job 卡住: kubectl delete job <job-name> -n <namespace>
  3. 修改 CronJob 的 concurrencyPolicy: Allow（允许并发）或 Replace（替换）
  4. 如长期卡住，检查 Job 的 activeDeadlineSeconds 是否设置过短
expected_output: "ConcurrentJob 结束后新 Job 自动触发"  # 正常
```
**路径 3: CronJob 被暂停（spec.paused=true）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe cronjob <name> -n <namespace> | grep -i paused
判断逻辑:
  - spec.paused: true 表示暂停创建新 Job
修复步骤:
  1. kubectl patch cronjob <name> -n <namespace> -p '{"spec":{"paused":false}}'
  2. 或在 YAML 中设置 paused: false 后 apply
expected_output: "CronJob 状态中 paused 为 false，下次调度正常触发"  # 正常
```
**路径 4: CronJob 已错过调度窗口（startDeadlineSeconds 不够）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
检查命令: kubectl describe cronjob <name> -n <namespace> | grep -i "Last schedule time"
判断逻辑:
  - 最后调度时间与预期不符，且 events 无新 Job 创建记录
修复步骤:
  1. 确认 CronJob 的 startDeadlineSeconds（默认 10s）是否足够
  2. 增大 startDeadlineSeconds: kubectl patch cronjob <name> -p '{"spec":{"startDeadlineSeconds":300}}'
  3. 手动触发一次测试（临时）: kubectl create job test-manual --from=cronjob/<name> -n <namespace>
expected_output: "手动创建的 Job 正常运行"  # 正常
```
---

<!-- chunk: M. Pod 终止与优雅关闭类 -->
## M. Pod 终止与优雅关闭类

### M-1. Pod 卡在 Terminating（Graceful Shutdown 超时）

**症状描述**: `kubectl get pod` 显示 Terminating，超过 30s（默认 grace period）仍未删除

**诊断路径**：

**路径 1: 应用未正确处理 SIGTERM**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

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
检查命令: kubectl describe pod <pod-name> | grep -i "Termination"
判断逻辑:
  - Pod 有 Termination 状态但 graceful period 已超时
修复步骤:
  1. 检查应用是否在代码中正确注册了 SIGTERM 处理（优雅关闭连接、释放资源）
  2. Java: 添加 shutdown hook，Spring Boot 确保 server.shutdown 设置为 graceful
  3. Go: 在 signal.Notify 中处理 os.Interrupt
  4. 临时方案: 增加 terminationGracePeriodSeconds: kubectl patch pod <pod> -p '{"spec":{"terminationGracePeriodSeconds":60}}'
expected_output: "Pod 在 grace period 内完成终止并消失"  # 正常
```
**路径 2: preStop hook 执行时间过长**
```
# 🟢 低风险：只读/信息收集，通常无副作用
检查命令: kubectl describe pod <pod-name> | grep -i "preStop"
判断逻辑:
  - spec.containers[].lifecycle.preStop.exec 定义的清理命令执行超时
修复步骤:
  1. 检查 preStop hook 命令是否耗时过长（如调用外部 API、等待清理完成）
  2. 简化 preStop 逻辑（只做快速清理，如关闭连接，延迟操作放到 SIGTERM handler）
  3. 如 preStop 需要网络调用，确保 timeout 设置足够（如 preStop.exec.timeoutSeconds）
expected_output: "preStop 在 timeout 内完成"  # 正常
```
**路径 3: Finalizers 阻塞删除**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl edit/patch`：修改运行中的资源

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
检查命令: kubectl get pod <pod-name> -o yaml | grep finalizers
判断逻辑:
  - metadata.finalizers 非空，且 deletionTimestamp 已设置但 Pod 不消失
修复步骤:
  1. 分析 finalizer 类型（通常是自定义 Controller 需要清理资源）
  2. 强制删除（无状态应用）: kubectl delete pod <pod> --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
  3. 或联系 Controller 所有者清理资源后删除 finalizer: kubectl patch pod <pod> -p '{"metadata":{"finalizers":null}}'
expected_output: "Pod 立即删除（强制）或 Controller 处理后正常删除"  # 正常
```
**路径 4: PVC 挂载导致无法终止**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
检查命令: kubectl describe pod <pod-name> | grep -i "volume"
判断逻辑:
  - Pod 有 volume mount，CSI 驱动未及时清理
修复步骤:
  1. 检查 PVC 状态: kubectl get pvc -n <namespace>
  2. 检查 CSI driver 是否正常: kubectl get pods -n kube-system | grep csi
  3. 如有状态应用，谨慎处理；如是无状态应用: kubectl delete pod <pod> --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
expected_output: "Pod 删除完成，PVC 仍存在（未删除）"  # 正常
```
---

<!-- chunk: 附录：症状快速索引 -->
## 附录：症状快速索引

| 症状 | 优先排查 |
|------|---------|
| Pod Pending | describe pod → Conditions → Unschedulable |
| Pod ContainerCreating | describe pod → Containers → ImagePullBackOff |
| Pod CrashLoopBackOff | logs --previous → Exit Code |
| Pod Evicted | describe node → MemoryPressure/DiskPressure |
| Pod Terminating | get pod -o yaml → finalizers |
| Pod OOMKilled | describe pod → Reason: OOMKilled |
| Service 无法访问 | get endpoints → <none> |
| Ingress 503 | get endpoints → backend all unhealthy |
| DNS 不通 | exec nslookup → CoreDNS status |
| 节点 NotReady | SSH kubelet status / journal |
| PVC Pending | get sc / get pvc → status |
| HPA 不扩缩容 | get hpa → REPLICAS == MAX 或 metrics-server 异常 |
| RBAC Forbidden | auth can-i → RoleBinding |
| kubectl exec 失败 | node status / RBAC can-i exec |
| API Server 无响应 | etcd health / apiserver pod status |
| kubectl cp 失败 | get pod → Running 确认 + 文件路径检查 |
| kubectl apply 报错 | dry-run=client → 确认字段名 |
| curl localhost 404 | logs → 检查应用路由配置 |
| HPA 扩到 MAX 仍高 | maxReplicas 是否足够 / 应用内存泄漏排查 |
| CronJob 未触发 | spec.timeZone / paused / ConcurrencyPolicy 检查 |
| Pod Terminating 卡住 | SIGTERM handler / preStop / finalizers 检查 |

---

<!-- chunk: 元数据 -->
## 元数据

```yaml
---
id: SOP-SYMPTOM-MAPPING-001
domain: troubleshooting
type: symptom-sop-mapping
tags: [sop, symptom-mapping, agent-corpus, k8s-1.28-1.33, ticket-handling]
intent_queries:
  - "Pod 卡住怎么办"
  - "服务访问不了怎么排查"
  - "节点 NotReady 怎么处理"
  - "Ingress 503 是什么原因"
  - "PVC 绑定失败怎么解决"
difficulty: intermediate
target_roles: [sre, ops-engineer, support-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-command-output/00-command-output-root-cause-parser.md
  - domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md
  - domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-10-troubleshooting-diagnostics MOC
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[domain-10-troubleshooting-diagnostics/核心排障/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[01-pod-pending-diagnosis|Pod Pending 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-10-troubleshooting-diagnostics/00-core-troubleshooting/01-oom-memory-diagnosis|OOM 和内存问题诊断]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-pod-comprehensive-troubleshooting|Pod 全面故障排查]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/01-node-comprehensive-troubleshooting|Node 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/05-event-driven-architecture-troubleshooting|41-event-driven-architecture-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/06-chaos-engineering-fault-injection-testing|42-chaos-engineering-fault-injection-testing]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/08-kind-k3s-single-node-troubleshooting|44-kind-k3s-single-node-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-10-troubleshooting-diagnostics/04-jvm-tuning/02-java-performance-resource-sizing-guide|99-java-performance-resource-sizing-guide]]

```

<!-- risk-assessed -->
