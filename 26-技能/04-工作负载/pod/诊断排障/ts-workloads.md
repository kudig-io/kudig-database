---
title: 工作负载故障排查
description: '# 工作负载故障排查'
summary: '2. **定位阶段**：Pending/ContainerCreating/Running/CrashLoopBackOff/OOMKilled，结合 Events 判断主因。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- workloads
- kubelet
- coredns
- docker
- statefulset
- daemonset
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 工作负载故障排查 是什么
- 如何 工作负载故障排查
trigger_keywords:
- 工作负载故障排查
prerequisites:
- kubectl-basics
- pod-lifecycle
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工作负载故障排查

### 01 Pod Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **四步法**：`kubectl get pod <pod> -o wide` → `kubectl describe pod <pod>` → `kubectl logs <pod>` → `kubectl exec <pod> -- <cmd>`。
2. **定位阶段**：Pending/ContainerCreating/Running/CrashLoopBackOff/OOMKilled，结合 Events 判断主因。
3. **镜像与拉取**：`kubectl describe pod | grep -A2 Image`，确认镜像、密钥、限流。
4. **资源与驱逐**：`kubectl top pod --containers`、`kubectl describe node | grep -A3 Pressure`。
5. **网络/存储**：必要时验证 DNS/Service 连通与 PVC 挂载状态。
6. **快速缓解**：
   - CrashLoop：先看 `--previous` 日志，临时放宽探针或回滚配置。
   - Pending：检查资源/污点/亲和/配额。
7. **证据留存**：保存 Events、日志、容器退出码与节点资源快照。

---

#### 2. 专家级问题矩阵与观测工具



#### 2.1 专家级问题矩阵（按生命周期分类）

#### 2.1.1 调度阶段问题

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **Pending: 资源不足** | CPU/Memory 碎片化（节点剩余资源分散），ResourceQuota 耗尽，PriorityClass 优先级不足 | `kubectl describe pod | grep "FailedScheduling"`；`kubectl describe node | grep -A5 "Allocated resources"` | 扩容节点；调整 requests；使用 Cluster Autoscaler |
| **Pending: 污点/亲和性** | 节点打了 `NoSchedule` 污点，Pod 未配置容忍；`requiredDuringScheduling` 亲和性无法满足 | `kubectl describe node | grep Taints`；`kubectl get pod -o yaml | grep -A10 affinity` | 添加 `tolerations`；放宽亲和性为 `preferred` |
| **Pending: PVC 未绑定** | StorageClass 不存在、后端存储配额不足、`volumeBindingMode: WaitForFirstConsumer` 延迟绑定 | `kubectl get pvc | grep Pending`；`kubectl describe pvc` | 检查 StorageClass；扩容后端；手动创建 PV |
| **Pending: 拓扑约束** | `topologySpreadConstraints` 约束无法满足（如强制均匀分布但节点不足） | `kubectl get pod -o yaml | grep -A5 topologySpreadConstraints` | 放宽 `whenUnsatisfiable: DoNotSchedule` 为 `ScheduleAnyway` |

#### 2.1.2 容器创建阶段问题

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **ImagePullBackOff** | 镜像不存在、Registry 凭证过期、镜像层损坏、Registry 速率限制（Docker Hub 100次/6h） | `kubectl describe pod | grep -A5 "Failed to pull image"`；`crictl pull <image>` 测试拉取 | 检查镜像 tag；重新创建 `imagePullSecrets`；使用镜像缓存代理 |
| **CreateContainerError** | Volume 挂载失败（PVC 不存在、CSI 驱动问题）、SecurityContext 冲突（如 `runAsUser: 0` 被 PSP 拒绝） | `kubectl describe pod | grep "CreateContainerError"`；`crictl ps -a | grep Error` | 检查 Volume 状态；调整 SecurityC
...(截断)

---

### 02 Deployment Troubleshooting

#### 0. 10 分钟快速诊断

1. **更新状态**：`kubectl rollout status deployment <name> --timeout=5m`，看是否卡在 Progressing。
2. **副本/RS**：`kubectl get rs -l app=<label> --sort-by=.metadata.creationTimestamp`，确认新 RS 是否扩容。
3. **Pod 事件**：`kubectl describe pod <pod>`，区分 Pending/CrashLoop/Probe 失败。
4. **镜像/配置**：检查镜像版本、`imagePullSecrets`、ConfigMap/Secret 是否更新。
5. **策略参数**：核对 `maxUnavailable/maxSurge/minReadySeconds/progressDeadlineSeconds`。
6. **快速缓解**：
   - 回滚：`kubectl rollout undo deployment <name>`。
   - 降速：临时调低并发，防止健康检查抖动。
7. **证据留存**：保存 Deployment/RS/Pod 描述与 events。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
Deployment 问题
    │
    ├─► 检查 Deployment 状态
    │       │
    │       ├─► Deployment 不存在 ──► 检查创建命令和 YAML
    │       │
    │       ├─► Available=False ──► 检查 Pod 状态
    │       │
    │       └─► Progressing=False ──► 检查更新策略和资源
    │
    ├─► 检查 ReplicaSet 状态
    │       │
    │       ├─► 新 RS replicas=0 ──► 检查 Webhook/Admission
    │       │
    │       ├─► 新 RS 创建 Pod 失败 ──► 检查 Pod Events
    │       │
    │       └─► 旧 RS 未缩容 ──► 检查 maxUnavailable 和 Pod 健康
    │
    ├─► 检查 Pod 状态
    │       │
    │       ├─► Pending ──► 检查资源、调度、亲和性
    │       │       │
    │       │       ├─► Insufficient resources ──► 扩容节点或减少 requests
    │       │       ├─► Node selector mismatch ──► 修正标签或选择器
    │       │       ├─► Taints not tolerated ──► 添加 tolerations
    │       │       └─► PVC not bound ──► 检查存储类和 PV
    │       │
    │       ├─► ImagePullBackOff ──► 检查镜像和凭证
    │       │
    │       ├─► CrashLoopBackOff ──► 检查容器日志和配置
    │       │       │
    │       │       ├─► 应用启动失败 ──► 修复应用代码/配置
    │       │       ├─► 健康检查失败 ──► 调整探针配置
    │       │       └─► 依赖服务不可用 ──► 添加 init container 或重试
    │       │
    │       ├─► Running but Not Ready ──► 检查 readinessProbe
    │       │
    │       └─► OOMKilled ──► 增加内存限制
    │
    └─► 检查滚动更新
            │
            ├─► 更新卡住 ──► 检查 progressDeadlineSeconds
            │
            ├─► 新旧 Pod 并存过久 ──► 检查 minReadySeconds
            │
            └─► 更新后回滚 ──► 检查新版本问题
```

---

### 03 Statefulset Troubleshooting

#### 0. 10 分钟快速诊断

1. **Pod 序列**：`kubectl get sts <name> -o wide` 看 `current/update`；`kubectl get pods -l app=<sts>` 查看序号创建是否停滞。
2. **PVC 绑定**：`kubectl get pvc -l app=<sts>`，Pending 则先排存储类/配额/拓扑。
3. **Headless DNS**：`kubectl get svc <headless> -o yaml`，确认 `clusterIP: None`，并测试 Pod DNS。
4. **更新卡住**：`kubectl rollout status sts <name>`，看是否被探针/序列阻塞。
5. **快速缓解**：
   - 卡在前序 Pod：先修复 `-0` Pod 的 readiness 或回滚。
   - 存储问题：修复 CSI/VolumeAttachment，再恢复。
6. **证据留存**：保存 sts/pod/pvc 描述与相关事件。

---

#### 排查方法与步骤



#### 2.2 排查决策树

```
StatefulSet 问题
       │
       ├─── Pod 数量不足？
       │         │
       │         ├─ 是 ──→ 检查 Pod 状态
       │         │              │
       │         │              ├─ Pending ──→ 检查 PVC 绑定 / 节点资源 / 调度约束
       │         │              ├─ ContainerCreating ──→ 检查镜像拉取 / 存储挂载
       │         │              └─ 前序 Pod 未 Ready ──→ 排查前序 Pod 问题
       │         │
       │         └─ 否 ──→ 检查 Pod 运行状态
       │
       ├─── 网络标识异常？
       │         │
       │         ├─ Headless Service 存在？ ──→ 检查 Service selector
       │         ├─ DNS 解析失败？ ──→ 检查 CoreDNS / Service 配置
       │         └─ Pod 间通信失败？ ──→ 检查 NetworkPolicy / CNI
       │
       ├─── 存储问题？
       │         │
       │         ├─ PVC Pending ──→ 检查 StorageClass / PV 可用性
       │         ├─ 挂载失败 ──→ 检查 CSI 驱动 / 节点存储
       │         └─ 数据丢失 ──→ 检查 PV ReclaimPolicy / 实际存储
       │
       └─── 更新问题？
                 │
                 ├─ 更新卡住 ──→ 检查 Pod 健康检查 / 更新策略
                 ├─ 分区更新 ──→ 检查 partition 设置
                 └─ 需要回滚 ──→ 使用 rollout undo
```

---

### 04 Daemonset Troubleshooting

#### 0. 10 分钟快速诊断

1. **期望/实际**：`kubectl get ds <name> -o wide`，对比 DESIRED/CURRENT/READY。
2. **节点选择**：核对 `nodeSelector/nodeAffinity/tolerations` 是否覆盖目标节点。
3. **Pod 事件**：`kubectl describe pod <ds-pod>`，区分调度失败与容器启动失败。
4. **镜像/权限**：检查 `ImagePullBackOff` 与 `permission denied` 类报错。
5. **更新策略**：`kubectl get ds <name> -o jsonpath='{.spec.updateStrategy}'`，确认 RollingUpdate 参数。
6. **快速缓解**：
   - 规则错配：修正 selector/affinity 后滚动重建。
   - 关键系统 DS：必要时临时 `maxUnavailable=0` 保持可用性。
7. **证据留存**：保存 ds/pod 描述与 events。

---

#### 排查方法与步骤



#### 2.3 排查决策树

```
DaemonSet 问题
       │
       ├─── DESIRED 数量不对？
       │         │
       │         ├─ 为 0 ──→ 检查 nodeSelector/nodeAffinity 配置
       │         │
       │         └─ 小于节点数 ──→ 检查节点标签匹配
       │
       ├─── CURRENT < DESIRED？
       │         │
       │         ├─ Pod Pending ──→ 检查污点容忍/资源/镜像
       │         │
       │         └─ 无 Pod 创建 ──→ 检查控制器日志
       │
       ├─── READY < CURRENT？
       │         │
       │         ├─ CrashLoopBackOff ──→ 检查应用日志/权限/配置
       │         │
       │         ├─ Running 但 NotReady ──→ 检查健康检查配置
       │         │
       │         └─ ContainerCreating ──→ 检查镜像/存储/网络
       │
       └─── UP-TO-DATE < DESIRED？
                 │
                 ├─ 更新策略 OnDelete ──→ 需手动删除旧 Pod
                 │
                 └─ RollingUpdate 卡住 ──→ 检查 maxUnavailable/Pod 问题
```

---

### 05 Job Cronjob Troubleshooting

#### 0. 10 分钟快速诊断

1. **Cron 调度**：`kubectl get cronjob <name> -o wide`，确认 `SUSPEND` 与 `LAST SCHEDULE`。
2. **Job 状态**：`kubectl get job -o wide`，观察 `active/succeeded/failed`。
3. **Pod 失败**：`kubectl get pods -l job-name=<job> --field-selector=status.phase!=Running`，结合 `kubectl logs --previous`。
4. **时间与时区**：检查 `spec.timeZone` 与集群时间，排除时区偏移。
5. **并发策略**：确认 `concurrencyPolicy` 与 `startingDeadlineSeconds`。
6. **快速缓解**：
   - 失败重试：临时增大 `backoffLimit` 或延长 `activeDeadlineSeconds`。
   - 积压清理：调整 history limit 并清理历史 Job。
7. **证据留存**：保存 cronjob/job/pod 描述与事件。

---

#### 排查方法与步骤



#### 2.3 排查决策树

```
Job/CronJob 问题
       │
       ├─── Job 未完成？
       │         │
       │         ├─ Pod 未创建 ──→ 检查 Job 配置 / 资源配额
       │         ├─ Pod Pending ──→ 检查调度约束 / 节点资源
       │         ├─ Pod 失败重试 ──→ 检查应用日志 / 退出码
       │         └─ Pod 运行未退出 ──→ 检查应用逻辑 / 死锁
       │
       ├─── CronJob 未触发？
       │         │
       │         ├─ suspend: true ──→ 取消挂起
       │         ├─ schedule 格式错误 ──→ 修正 cron 表达式
       │         ├─ 时区偏差 ──→ 检查 timeZone 配置
       │         └─ startingDeadlineSeconds 过短 ──→ 调整或检查调度延迟
       │
       ├─── Job 积压/并发问题？
       │         │
       │         ├─ concurrencyPolicy: Allow ──→ 考虑改为 Forbid/Replace
       │         ├─ Job 执行时间过长 ──→ 优化任务或调整调度间隔
       │         └─ 资源不足 ──→ 扩容或限制并行数
       │
       └─── 历史 Job 未清理？
                 │
                 ├─ 检查 history limits ──→ 调整 successfulJobsHistoryLimit
                 └─ 手动清理 ──→ 删除历史 Job
```

---

### 06 Configmap Secret Troubleshooting

#### 0. 10 分钟快速诊断

1. **资源存在**：`kubectl get cm,secret -n <ns>`，确认名称与命名空间。
2. **Pod 事件**：`kubectl describe pod <pod>` 查找 `not found`/`key not defined`/`MountVolume`。
3. **注入方式**：确认是 env/envFrom 还是 volume；env 变更需重启。
4. **Secret 解码**：`kubectl get secret <name> -o jsonpath='{.data.<key>}' | base64 -d` 验证内容。
5. **权限与 SA**：检查 RBAC 与 ServiceAccount 的 `imagePullSecrets` 绑定。
6. **快速缓解**：
   - 子路径 subPath：改为目录挂载或重启 Pod。
   - 热更新迟滞：确认 kubelet sync 周期与应用热加载能力。
7. **证据留存**：保存资源 YAML、Pod 事件与容器内验证结果。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
# 🟢 低风险：只读/信息收集，通常无副作用
ConfigMap/Secret 问题
        │
        ├─── Pod 启动失败？
        │         │
        │         ├─ "not found" ──→ 检查资源是否存在/命名空间是否正确
        │         ├─ "key not defined" ──→ 检查 key 名称是否正确
        │         ├─ "forbidden" ──→ 检查 RBAC 权限
        │         └─ "MountVolume failed" ──→ 检查挂载配置
        │
        ├─── 配置未生效？
        │         │
        │         ├─ 使用环境变量 ──→ 需要重启 Pod
        │         ├─ 使用 subPath ──→ 不支持热更新，需重启
        │         ├─ 使用卷挂载 ──→ 等待 kubelet 同步 (默认 1 分钟)
        │         └─ 应用未重新加载 ──→ 检查应用是否支持热加载
        │
        ├─── Secret 数据问题？
        │         │
        │         ├─ 数据乱码 ──→ 检查 base64 编码
        │         ├─ 数据不完整 ──→ 检查 YAML 格式/换行符
        │         └─ 无法解码 ──→ 使用 stringData 或正确编码
        │
        └─── 镜像拉取问题？
                  │
                  ├─ imagePullSecrets 未配置 ──→ 添加 imagePullSecrets
                  ├─ Secret 数据错误 ──→ 重新创建 docker-registry secret
                  └─ ServiceAccount 未关联 ──→ 配置 SA 的 imagePullSecrets
```
## 相关链接

- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]
- [[26-技能/03-节点/node/诊断排障/troubleshoot-node-issues.md|节点故障排查]]

## Related

- [[23-实体/02-K8s核心组件/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[coredns]] — CoreDNS
- [[cni]] — CNI (Container Network Interface)
- [[26-技能/04-工作负载/deployment/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[26-技能/04-工作负载/statefulset/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/daemonset-fta.md|DaemonSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
