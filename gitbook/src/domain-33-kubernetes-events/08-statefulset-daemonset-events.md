# 08 - StatefulSet 与 DaemonSet 控制器事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录 StatefulSet 和 DaemonSet 控制器产生的所有事件。**

---

## 📋 目录

- [事件概览](#事件概览)
- [StatefulSet 控制器事件](#statefulset-控制器事件)
- [DaemonSet 控制器事件](#daemonset-控制器事件)
- [StatefulSet 特性说明](#statefulset-特性说明)
- [DaemonSet 特性说明](#daemonset-特性说明)
- [实战案例](#实战案例)
- [最佳实践](#最佳实践)
- [相关文档](#相关文档)

---

## 事件概览

### 事件汇总表

| 事件原因 | 类型 | 来源组件 | 资源类型 | 生产频率 | 版本 |
|:---|:---|:---|:---|:---|:---|
| **StatefulSet Events** |
| SuccessfulCreate | Normal | statefulset-controller | StatefulSet | 高频 | v1.5+ |
| SuccessfulDelete | Normal | statefulset-controller | StatefulSet | 中频 | v1.5+ |
| FailedCreate | Warning | statefulset-controller | StatefulSet | 中频 | v1.5+ |
| SuccessfulUpdate | Normal | statefulset-controller | StatefulSet | 中频 | v1.7+ |
| FailedUpdate | Warning | statefulset-controller | StatefulSet | 低频 | v1.7+ |
| UnhealthyPodEviction | Warning | statefulset-controller | StatefulSet | 低频 | v1.28+ |
| **DaemonSet Events** |
| SuccessfulCreate | Normal | daemon-set-controller | DaemonSet | 高频 | v1.2+ |
| SuccessfulDelete | Normal | daemon-set-controller | DaemonSet | 中频 | v1.2+ |
| FailedCreate | Warning | daemon-set-controller | DaemonSet | 中频 | v1.2+ |
| FailedDaemonPod | Warning | daemon-set-controller | DaemonSet | 低频 | v1.2+ |
| FailedPlacement | Warning | daemon-set-controller | DaemonSet | 低频 | v1.12+ |
| SelectingAll | Warning | daemon-set-controller | DaemonSet | 罕见 | v1.2+ |
| MissingSelector | Warning | daemon-set-controller | DaemonSet | 罕见 | v1.2+ |

### 控制器特性对比

| 特性 | StatefulSet | DaemonSet |
|:---|:---|:---|
| **部署模式** | 有序创建/删除(OrderedReady) 或 并行(Parallel) | 每节点一个副本 |
| **Pod 命名** | 固定序号: name-0, name-1, name-2 | 随机后缀 |
| **调度方式** | 标准调度器 | v1.12+ 使用标准调度器 |
| **存储管理** | volumeClaimTemplates 自动创建 PVC | 手动管理 |
| **网络标识** | Headless Service 提供稳定网络标识 | 无特殊要求 |
| **滚动更新** | 支持 OnDelete/RollingUpdate | 支持 OnDelete/RollingUpdate |
| **回滚** | 不支持自动回滚 | 不支持自动回滚 |

---

## StatefulSet 控制器事件

### `SuccessfulCreate` - Pod 创建成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | statefulset-controller |
| **关联资源** | StatefulSet |
| **适用版本** | v1.5+ |
| **生产频率** | 高频 |

#### 事件含义

StatefulSet 控制器成功创建了一个 Pod。此事件表示 Pod 对象已被创建到 API Server，但不代表 Pod 已运行。

#### 典型事件消息

```
create Pod web-0 in StatefulSet default/web successful
create Pod web-1 in StatefulSet default/web successful
```

#### 影响面说明

- **正常生命周期**: 扩容、初始部署时的正常事件
- **有序创建**: 在 `OrderedReady` 模式下,只有前一个 Pod Running&Ready 后才创建下一个
- **并行创建**: 在 `Parallel` 模式下,所有 Pod 同时创建
- **PVC 绑定**: 如果定义了 volumeClaimTemplates,会同时创建对应的 PVC

#### 排查建议

**查看 Pod 创建顺序:**
```bash
# 查看 StatefulSet 事件时间线
kubectl describe statefulset web

# 查看 Pod 创建时间
kubectl get pods -l app=web -o custom-columns=NAME:.metadata.name,CREATED:.metadata.creationTimestamp
```

**查看 PVC 自动创建:**
```bash
# StatefulSet 会自动为每个 Pod 创建 PVC
kubectl get pvc -l app=web
```

#### 解决建议

正常事件,无需处理。如需优化:

**1. 调整创建策略:**
```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  podManagementPolicy: Parallel  # OrderedReady(默认) 或 Parallel
  replicas: 3
```

**2. 监控创建速度:**
```bash
# 有序创建模式下,查看为什么前一个 Pod 未 Ready
kubectl describe pod web-0
kubectl logs web-0
```

---

### `SuccessfulDelete` - Pod 删除成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | statefulset-controller |
| **关联资源** | StatefulSet |
| **适用版本** | v1.5+ |
| **生产频率** | 中频 |

#### 事件含义

StatefulSet 控制器成功删除了一个 Pod。通常发生在缩容、滚动更新或删除 StatefulSet 时。

#### 典型事件消息

```
delete Pod web-2 in StatefulSet default/web successful
delete Pod web-1 in StatefulSet default/web successful
```

#### 影响面说明

- **有序删除**: 在 `OrderedReady` 模式下,从最大序号到最小序号依次删除
- **并行删除**: 在 `Parallel` 模式下,所有 Pod 同时删除
- **PVC 保留**: 删除 Pod 时,PVC 默认**不会被删除**(数据保护)
- **终止宽限期**: 遵守 `terminationGracePeriodSeconds` 设置

#### 排查建议

**查看删除顺序:**
```bash
# 查看删除事件
kubectl get events --field-selector involvedObject.kind=StatefulSet,involvedObject.name=web

# 查看 Pod 删除时间戳
kubectl get pods -l app=web -o yaml | grep deletionTimestamp
```

**确认 PVC 保留:**
```bash
# PVC 仍然存在
kubectl get pvc -l app=web
```

#### 解决建议

正常事件,注意数据管理:

**1. PVC 清理策略:**
```yaml
# StatefulSet 删除时,PVC 默认保留
# 需手动删除或使用自定义控制器清理
kubectl delete pvc data-web-0 data-web-1 data-web-2
```

**2. 设置合理的终止宽限期:**
```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 30  # 给应用足够时间优雅关闭
```

---

### `FailedCreate` - Pod 创建失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | statefulset-controller |
| **关联资源** | StatefulSet |
| **适用版本** | v1.5+ |
| **生产频率** | 中频 |

#### 事件含义

StatefulSet 控制器无法创建 Pod,通常是 API 调用失败或配置错误导致。

#### 典型事件消息

```
create Pod web-0 in StatefulSet default/web failed error: persistentvolumeclaim "data-web-0" not found
create Pod web-1 failed: pods "web-1" is forbidden: exceeded quota: compute-quota
create Pod web-2 failed: Pod "web-2" is invalid: spec.containers[0].image: Required value
```

#### 影响面说明

- **阻断部署**: 在 `OrderedReady` 模式下,创建失败会阻止后续 Pod 创建
- **无限重试**: 控制器会持续重试创建操作
- **PVC 依赖**: 如果 PVC 不存在或无法绑定,Pod 无法创建
- **配额限制**: 资源配额、LimitRange 可能阻止创建

#### 排查建议

**1. 查看详细错误:**
```bash
# 查看 StatefulSet 事件
kubectl describe statefulset web

# 查看控制器日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep statefulset
```

**2. 检查常见原因:**
```bash
# 检查 PVC 状态
kubectl get pvc -l app=web
kubectl describe pvc data-web-0

# 检查 StorageClass
kubectl get storageclass
kubectl describe storageclass standard

# 检查资源配额
kubectl describe resourcequota -n default

# 检查 LimitRange
kubectl describe limitrange -n default
```

**3. 验证 Pod 模板:**
```bash
# 检查 Pod 模板有效性
kubectl get statefulset web -o yaml | kubectl create --dry-run=server -f -
```

#### 解决建议

**1. PVC 问题 - 预创建 PVC:**
```bash
# 手动创建 PVC(如果自动创建失败)
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-web-0
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: "standard"
  resources:
    requests:
      storage: 10Gi
EOF
```

**2. 配额问题 - 调整配额或请求:**
```bash
# 查看当前配额使用情况
kubectl describe quota compute-quota

# 调整 StatefulSet 资源请求
kubectl edit statefulset web
```

**3. 镜像问题 - 修复镜像引用:**
```bash
# 修复镜像地址
kubectl set image statefulset/web app=nginx:1.21
```

**4. 权限问题 - 检查 RBAC:**
```bash
# 检查 ServiceAccount 权限
kubectl get serviceaccount default -o yaml
kubectl describe rolebinding -n default
```

---

### `SuccessfulUpdate` - Pod 更新成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | statefulset-controller |
| **关联资源** | StatefulSet |
| **适用版本** | v1.7+ |
| **生产频率** | 中频 |

#### 事件含义

StatefulSet 控制器成功更新了一个 Pod。在 RollingUpdate 策略下,控制器会按序号从大到小依次删除并重建 Pod。

#### 典型事件消息

```
update Pod web-2 in StatefulSet default/web successful
update Pod web-1 in StatefulSet default/web successful
```

#### 影响面说明

- **滚动更新**: 从最大序号开始,逐个删除并重建 Pod
- **分区更新**: 通过 `partition` 参数可以控制更新范围
- **有序等待**: 每个 Pod 必须 Running&Ready 后才更新下一个
- **手动控制**: `OnDelete` 策略需要手动删除 Pod 触发更新

#### 排查建议

**查看更新进度:**
```bash
# 查看 StatefulSet 状态
kubectl rollout status statefulset/web

# 查看更新事件
kubectl describe statefulset web

# 查看 Pod 版本分布
kubectl get pods -l app=web -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image
```

**查看更新策略:**
```bash
kubectl get statefulset web -o jsonpath='{.spec.updateStrategy}'
```

#### 解决建议

正常事件,可优化更新策略:

**1. 使用分区更新(金丝雀发布):**
```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 2  # 只更新序号 >= 2 的 Pod
```

**2. 使用 OnDelete 策略(手动控制):**
```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  updateStrategy:
    type: OnDelete  # 必须手动删除 Pod 才会更新
```

**3. 设置合理的就绪探针:**
```yaml
spec:
  template:
    spec:
      containers:
      - name: app
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
```

---

### `FailedUpdate` - Pod 更新失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | statefulset-controller |
| **关联资源** | StatefulSet |
| **适用版本** | v1.7+ |
| **生产频率** | 低频 |

#### 事件含义

StatefulSet 控制器在更新过程中遇到错误,无法完成 Pod 的重建或更新操作。

#### 典型事件消息

```
update Pod web-1 in StatefulSet default/web failed error: pods "web-1" already exists
update Pod web-2 failed: persistentvolumeclaim "data-web-2" not found
```

#### 影响面说明

- **更新中断**: 更新流程会停滞在失败的 Pod
- **后续阻塞**: 后续 Pod 不会被更新(有序更新机制)
- **服务降级**: 部分 Pod 可能处于旧版本,部分处于新版本
- **PVC 依赖**: PVC 问题会导致 Pod 无法启动

#### 排查建议

**1. 查看失败原因:**
```bash
# 查看 StatefulSet 事件
kubectl describe statefulset web

# 查看失败的 Pod
kubectl get pods -l app=web
kubectl describe pod web-1
```

**2. 检查 PVC 状态:**
```bash
# 查看 PVC 绑定状态
kubectl get pvc -l app=web
kubectl describe pvc data-web-1
```

**3. 检查 Pod 状态:**
```bash
# 查看 Pod 详情
kubectl get pod web-1 -o yaml

# 查看 Pod 事件
kubectl describe pod web-1
```

#### 解决建议

**1. Pod 卡在 Terminating 状态:**
```bash
# 查看 Pod 终止状态
kubectl get pod web-1 -o yaml | grep deletionTimestamp

# 如果 Pod 长时间 Terminating,强制删除(谨慎操作)
kubectl delete pod web-1 --grace-period=0 --force
```

**2. PVC 问题:**
```bash
# 检查 PVC 是否被其他 Pod 占用(ReadWriteOnce 模式)
kubectl get pods -o wide | grep data-web-1

# 检查 StorageClass 可用性
kubectl get storageclass
kubectl get pv | grep data-web-1
```

**3. 回滚更新:**
```bash
# StatefulSet 不支持自动回滚,需手动修改
kubectl edit statefulset web
# 或使用 kubectl apply 恢复旧版本配置

# 如果使用了 partition,可调整 partition 值
kubectl patch statefulset web -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":3}}}}'
```

**4. 重建 StatefulSet(保留 PVC):**
```bash
# 删除 StatefulSet 但保留 Pod
kubectl delete statefulset web --cascade=orphan

# 重新创建 StatefulSet(会接管现有 Pod)
kubectl apply -f statefulset.yaml
```

---

### `UnhealthyPodEviction` - 不健康 Pod 驱逐

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | statefulset-controller |
| **关联资源** | StatefulSet |
| **适用版本** | v1.28+ |
| **生产频率** | 低频 |

#### 事件含义

StatefulSet 控制器检测到不健康的 Pod,并根据 `podManagementPolicy` 决定是否驱逐该 Pod。这是 v1.28 引入的增强特性。

#### 典型事件消息

```
evicting unhealthy pod web-1 due to PodDisruptionCondition
evicting unhealthy pod web-2, reason: NodeShutdown
```

#### 影响面说明

- **自动恢复**: 控制器会删除不健康的 Pod 并重建
- **节点故障**: 节点不可达或关闭时触发
- **PodDisruptionCondition**: 基于 Pod 的 DisruptionTarget condition 判断
- **仅 StatefulSet**: 此特性目前仅适用于 StatefulSet

#### 排查建议

**1. 查看 Pod DisruptionTarget condition:**
```bash
# 查看 Pod 的 Conditions
kubectl get pod web-1 -o jsonpath='{.status.conditions[?(@.type=="DisruptionTarget")]}'

# 查看完整 Pod 状态
kubectl describe pod web-1
```

**2. 检查节点状态:**
```bash
# 查看节点是否不可达
kubectl get nodes
kubectl describe node <node-name>

# 查看节点事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name>
```

**3. 查看 PDB 配置:**
```bash
# 检查 PodDisruptionBudget
kubectl get pdb
kubectl describe pdb web-pdb
```

#### 解决建议

**1. 节点故障恢复:**
```bash
# 如果节点恢复,Pod 会自动重建
kubectl get nodes

# 如果节点永久下线,从集群移除
kubectl delete node <node-name>
```

**2. 配置 PodDisruptionBudget:**
```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2  # 至少保持 2 个副本可用
  selector:
    matchLabels:
      app: web
```

**3. 启用 Node 驱逐特性:**
```yaml
# Kubelet 配置(v1.28+)
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  StatefulSetAutoDeletePVC: true  # 自动删除 PVC(可选)
```

---

## DaemonSet 控制器事件

### `SuccessfulCreate` - Pod 创建成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.2+ |
| **生产频率** | 高频 |

#### 事件含义

DaemonSet 控制器成功在节点上创建了一个 Pod。每当有新节点加入集群,或 DaemonSet 首次部署时,都会触发此事件。

#### 典型事件消息

```
Created pod: fluentd-abc123
Created pod: fluentd-def456
```

#### 影响面说明

- **每节点一副本**: DaemonSet 确保每个符合条件的节点运行一个 Pod 副本
- **自动调度**: v1.12+ 使用标准调度器,v1.12- 使用 DaemonSet 控制器直接调度
- **节点选择器**: 通过 nodeSelector/nodeAffinity 控制 Pod 分布
- **容忍度**: 通过 tolerations 控制是否在有污点的节点运行

#### 排查建议

**查看 Pod 分布:**
```bash
# 查看 DaemonSet Pod 分布
kubectl get pods -l app=fluentd -o wide

# 查看节点数量
kubectl get nodes

# 查看 DaemonSet 状态
kubectl get daemonset fluentd
# DESIRED: 期望副本数
# CURRENT: 当前副本数
# READY: 就绪副本数
# UP-TO-DATE: 最新版本副本数
# AVAILABLE: 可用副本数
```

**检查节点选择:**
```bash
# 查看 DaemonSet 的节点选择器
kubectl get daemonset fluentd -o jsonpath='{.spec.template.spec.nodeSelector}'

# 查看节点标签
kubectl get nodes --show-labels
```

#### 解决建议

正常事件,可优化调度策略:

**1. 使用节点选择器:**
```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  template:
    spec:
      nodeSelector:
        disktype: ssd  # 仅在 SSD 节点运行
```

**2. 使用节点亲和性(更灵活):**
```yaml
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/control-plane
                operator: DoesNotExist  # 不在 master 节点运行
```

**3. 配置容忍度:**
```yaml
spec:
  template:
    spec:
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule  # 允许在 master 节点运行
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute  # 在 NotReady 节点也保持运行
```

---

### `SuccessfulDelete` - Pod 删除成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.2+ |
| **生产频率** | 中频 |

#### 事件含义

DaemonSet 控制器成功删除了一个 Pod。通常发生在节点移除、DaemonSet 更新或节点不再符合调度条件时。

#### 典型事件消息

```
Deleted pod: fluentd-abc123
Deleted pod: fluentd-def456
```

#### 影响面说明

- **节点移除**: 节点从集群移除时,对应的 DaemonSet Pod 会被删除
- **选择器变更**: 修改 nodeSelector/nodeAffinity 后,不符合条件的节点上的 Pod 被删除
- **滚动更新**: RollingUpdate 策略下,旧版本 Pod 被删除
- **DaemonSet 删除**: 删除 DaemonSet 时,所有 Pod 被删除

#### 排查建议

**查看删除原因:**
```bash
# 查看 DaemonSet 事件
kubectl describe daemonset fluentd

# 查看节点状态
kubectl get nodes

# 查看 Pod 删除时间
kubectl get events --field-selector involvedObject.kind=Pod,reason=Killing
```

**检查节点选择器变更:**
```bash
# 查看 DaemonSet 配置变更历史
kubectl rollout history daemonset fluentd

# 查看当前节点选择器
kubectl get daemonset fluentd -o yaml | grep -A 5 nodeSelector
```

#### 解决建议

正常事件,注意以下场景:

**1. 节点维护时保留 DaemonSet Pod:**
```bash
# 给节点打污点但不驱逐 DaemonSet Pod
kubectl taint nodes node1 maintenance=true:NoSchedule

# DaemonSet 需配置容忍度
kubectl patch daemonset fluentd -p '{"spec":{"template":{"spec":{"tolerations":[{"key":"maintenance","operator":"Exists"}]}}}}'
```

**2. 滚动更新控制:**
```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 每次最多删除 1 个 Pod
```

---

### `FailedCreate` - Pod 创建失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.2+ |
| **生产频率** | 中频 |

#### 事件含义

DaemonSet 控制器无法在节点上创建 Pod,通常是由于资源不足、端口冲突或配置错误导致。

#### 典型事件消息

```
Error creating: pods "fluentd-abc123" is forbidden: exceeded quota: compute-quota
Error creating: admission webhook "validate.pod.admission" denied the request
Error creating: Pod "fluentd-def456" is invalid: spec.containers[0].ports[0].hostPort: Invalid value: 9200: host port is already allocated
```

#### 影响面说明

- **节点覆盖不全**: 部分节点可能无法运行 DaemonSet Pod
- **服务降级**: 节点级服务(如日志采集)可能中断
- **无限重试**: 控制器会持续重试创建操作
- **调度失败**: v1.12+ 调度器失败会反映在 Pod 事件中

#### 排查建议

**1. 查看详细错误:**
```bash
# 查看 DaemonSet 事件
kubectl describe daemonset fluentd

# 查看 Pod 创建失败事件
kubectl get events --field-selector involvedObject.kind=Pod,type=Warning

# 查看控制器日志(v1.12-)
kubectl logs -n kube-system -l component=kube-controller-manager | grep daemon
```

**2. 检查常见原因:**
```bash
# 检查资源配额
kubectl describe resourcequota -n default

# 检查节点资源
kubectl describe nodes | grep -A 5 "Allocated resources"

# 检查端口冲突(hostPort)
kubectl get pods --all-namespaces -o json | jq '.items[] | select(.spec.containers[].ports[]?.hostPort) | {name: .metadata.name, namespace: .metadata.namespace, hostPort: .spec.containers[].ports[].hostPort}'
```

**3. 检查准入控制器:**
```bash
# 查看准入 webhook
kubectl get validatingwebhookconfiguration
kubectl get mutatingwebhookconfiguration

# 测试 Pod 创建(dry-run)
kubectl create -f pod.yaml --dry-run=server
```

#### 解决建议

**1. 端口冲突 - 使用 hostNetwork 或修改端口:**
```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  template:
    spec:
      hostNetwork: true  # 使用主机网络
      containers:
      - name: fluentd
        ports:
        - containerPort: 24224
          hostPort: 24224  # 确保端口唯一
```

**2. 资源不足 - 调整资源请求:**
```yaml
spec:
  template:
    spec:
      containers:
      - name: fluentd
        resources:
          requests:
            cpu: 100m      # 降低 CPU 请求
            memory: 128Mi  # 降低内存请求
          limits:
            cpu: 200m
            memory: 256Mi
```

**3. 准入控制器问题 - 修复或豁免:**
```yaml
# 如果准入 webhook 误拦截,可临时禁用
kubectl label namespace kube-system admission.webhook/ignore=true

# 或修复 Pod 配置以通过准入检查
```

**4. 调度失败(v1.12+) - 检查调度器事件:**
```bash
# 查看 Pod 调度事件
kubectl describe pod fluentd-abc123
# 查找 "Failed to schedule" 或 "Unschedulable" 事件
```

---

### `FailedDaemonPod` - DaemonSet Pod 失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.2+ |
| **生产频率** | 低频 |

#### 事件含义

DaemonSet 控制器检测到某个节点上的 Pod 处于失败状态(Failed Phase),通常需要人工介入排查。

#### 典型事件消息

```
Found failed daemon pod fluentd-abc123 on node node1, will try to kill it
```

#### 影响面说明

- **Pod 重启**: 控制器会尝试删除失败的 Pod 并重建
- **节点问题**: 可能指示节点存在配置或资源问题
- **镜像问题**: 镜像拉取失败或启动命令错误
- **持续失败**: 如果问题未解决,Pod 会陷入 CrashLoopBackOff

#### 排查建议

**1. 查看 Pod 失败原因:**
```bash
# 查看 Pod 状态
kubectl get pods -l app=fluentd -o wide

# 查看 Pod 详情
kubectl describe pod fluentd-abc123

# 查看 Pod 日志
kubectl logs fluentd-abc123
kubectl logs fluentd-abc123 --previous  # 查看上一次运行的日志
```

**2. 检查节点状态:**
```bash
# 查看节点是否异常
kubectl describe node node1

# 查看节点资源使用
kubectl top node node1
kubectl top pod -l app=fluentd
```

**3. 检查镜像和启动配置:**
```bash
# 查看镜像拉取状态
kubectl get events --field-selector involvedObject.name=fluentd-abc123,reason=Failed

# 测试镜像拉取
kubectl run test --image=fluentd/fluentd:v1.14 --rm -it --restart=Never -- /bin/sh
```

#### 解决建议

**1. 镜像问题 - 修复镜像地址:**
```bash
# 修改 DaemonSet 镜像
kubectl set image daemonset/fluentd fluentd=fluentd/fluentd:v1.14

# 或配置镜像拉取策略
kubectl patch daemonset fluentd -p '{"spec":{"template":{"spec":{"containers":[{"name":"fluentd","imagePullPolicy":"IfNotPresent"}]}}}}'
```

**2. 启动失败 - 检查启动命令和健康检查:**
```yaml
spec:
  template:
    spec:
      containers:
      - name: fluentd
        livenessProbe:
          httpGet:
            path: /health
            port: 24220
          initialDelaySeconds: 30  # 给足启动时间
          periodSeconds: 10
        startupProbe:
          httpGet:
            path: /health
            port: 24220
          failureThreshold: 30
          periodSeconds: 10
```

**3. 权限问题 - 配置 SecurityContext:**
```yaml
spec:
  template:
    spec:
      containers:
      - name: fluentd
        securityContext:
          privileged: true  # 某些 DaemonSet 需要特权模式
      serviceAccountName: fluentd  # 使用专用 ServiceAccount
```

---

### `FailedPlacement` - Pod 放置失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.12+ |
| **生产频率** | 低频 |

#### 事件含义

DaemonSet 控制器(通过标准调度器)无法在节点上放置 Pod,通常是由于节点资源不足、污点限制或亲和性规则不匹配。

#### 典型事件消息

```
failed to place pod on node1: node(s) didn't match pod affinity rules
failed to place pod on node2: insufficient cpu
failed to place pod on node3: node had taint {key=special:NoSchedule}, and pod didn't tolerate it
```

#### 影响面说明

- **调度器管理**: v1.12+ DaemonSet 使用标准调度器,调度失败会记录此事件
- **节点不可用**: 部分节点无法运行 DaemonSet Pod
- **资源限制**: 节点资源不足导致无法调度
- **污点限制**: 节点污点阻止 Pod 调度

#### 排查建议

**1. 查看调度失败原因:**
```bash
# 查看 DaemonSet 事件
kubectl describe daemonset fluentd

# 查看 Pod 调度事件(Pending 状态的 Pod)
kubectl get pods -l app=fluentd | grep Pending
kubectl describe pod fluentd-pending-pod

# 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler | grep -i daemonset
```

**2. 检查节点资源:**
```bash
# 查看节点可分配资源
kubectl describe nodes | grep -A 5 "Allocatable"

# 查看 DaemonSet 资源请求
kubectl get daemonset fluentd -o jsonpath='{.spec.template.spec.containers[0].resources}'
```

**3. 检查节点污点和容忍度:**
```bash
# 查看节点污点
kubectl describe nodes | grep Taints

# 查看 DaemonSet 容忍度
kubectl get daemonset fluentd -o jsonpath='{.spec.template.spec.tolerations}'
```

#### 解决建议

**1. 添加容忍度:**
```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  template:
    spec:
      tolerations:
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute
      - key: node.kubernetes.io/unreachable
        operator: Exists
        effect: NoExecute
      - key: node.kubernetes.io/disk-pressure
        operator: Exists
        effect: NoSchedule
      - key: node.kubernetes.io/memory-pressure
        operator: Exists
        effect: NoSchedule
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule
```

**2. 降低资源请求:**
```yaml
spec:
  template:
    spec:
      containers:
      - name: fluentd
        resources:
          requests:
            cpu: 50m       # 降低 CPU 请求
            memory: 64Mi   # 降低内存请求
```

**3. 修改节点亲和性:**
```yaml
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/worker
                operator: Exists  # 仅匹配 worker 节点
```

---

### `SelectingAll` - 选择所有 Pod

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.2+ |
| **生产频率** | 罕见 |

#### 事件含义

DaemonSet 的选择器(selector)为空或过于宽泛,导致控制器可能选择到集群中所有的 Pod,这是配置错误。

#### 典型事件消息

```
This daemon set is selecting all pods. A non-empty selector is required.
```

#### 影响面说明

- **配置错误**: DaemonSet 必须有明确的 selector
- **阻止创建**: Kubernetes 会拒绝创建没有 selector 的 DaemonSet
- **API 验证**: 通常在 API 验证阶段就会被拦截

#### 排查建议

**查看 DaemonSet 配置:**
```bash
# 查看 selector 配置
kubectl get daemonset fluentd -o yaml | grep -A 3 selector

# 正确的配置应该包含 matchLabels
```

#### 解决建议

**添加正确的 selector:**
```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd
spec:
  selector:
    matchLabels:
      app: fluentd  # 必须与 template.metadata.labels 匹配
  template:
    metadata:
      labels:
        app: fluentd
    spec:
      containers:
      - name: fluentd
        image: fluentd/fluentd:v1.14
```

---

### `MissingSelector` - 缺少选择器

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | daemon-set-controller |
| **关联资源** | DaemonSet |
| **适用版本** | v1.2+ |
| **生产频率** | 罕见 |

#### 事件含义

DaemonSet 没有定义 selector 字段,无法选择和管理 Pod。这是严重的配置错误。

#### 典型事件消息

```
DaemonSet is missing selector
```

#### 影响面说明

- **无法创建**: API Server 会拒绝创建缺少 selector 的 DaemonSet
- **配置校验**: 通常在客户端或 API 验证时就会报错
- **版本兼容**: v1.9+ 强制要求 selector 字段

#### 排查建议

**验证 DaemonSet 配置:**
```bash
# 使用 dry-run 验证配置
kubectl apply -f daemonset.yaml --dry-run=server

# 查看验证错误
kubectl create -f daemonset.yaml
```

#### 解决建议

**添加 selector 字段:**
```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd
spec:
  selector:  # 必需字段
    matchLabels:
      app: fluentd
      component: logging
  template:
    metadata:
      labels:  # 必须包含 selector 中的所有标签
        app: fluentd
        component: logging
    spec:
      containers:
      - name: fluentd
        image: fluentd/fluentd:v1.14
```

---

## StatefulSet 特性说明

### Pod 管理策略

StatefulSet 提供两种 Pod 管理策略:

#### 1. OrderedReady(默认)

- **有序创建**: Pod 按序号 0, 1, 2, ... 依次创建
- **有序删除**: Pod 按序号 N, N-1, ..., 1, 0 依次删除
- **等待就绪**: 每个 Pod 必须 Running&Ready 后才创建/删除下一个
- **适用场景**: 有主从关系的分布式系统(如 MySQL 主从、Zookeeper)

```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  podManagementPolicy: OrderedReady  # 默认值
  replicas: 3
```

**行为示例:**
```
扩容: web-0 Ready → 创建 web-1 → web-1 Ready → 创建 web-2
缩容: 删除 web-2 → web-2 Terminated → 删除 web-1 → web-1 Terminated → 删除 web-0
```

#### 2. Parallel

- **并行创建**: 所有 Pod 同时创建,不等待前一个 Ready
- **并行删除**: 所有 Pod 同时删除
- **快速扩缩容**: 不保证顺序,速度更快
- **适用场景**: 无依赖关系的分布式系统(如无状态的缓存层)

```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  podManagementPolicy: Parallel
  replicas: 3
```

**行为示例:**
```
扩容: 同时创建 web-0, web-1, web-2
缩容: 同时删除 web-0, web-1, web-2
```

### PVC 自动管理

StatefulSet 通过 `volumeClaimTemplates` 自动为每个 Pod 创建独立的 PVC:

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: web
spec:
  serviceName: "nginx"
  replicas: 3
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: "standard"
      resources:
        requests:
          storage: 10Gi
```

**PVC 命名规则:**
```
<volumeClaimTemplate name>-<statefulset name>-<ordinal>

例如:
data-web-0
data-web-1
data-web-2
```

**重要特性:**

1. **自动创建**: StatefulSet 创建 Pod 时自动创建对应 PVC
2. **不自动删除**: 删除 StatefulSet 或缩容时,PVC **不会被删除**(数据保护)
3. **稳定绑定**: Pod 重建后会重新绑定到同名 PVC
4. **手动清理**: 需要手动删除不再使用的 PVC

```bash
# 删除 StatefulSet 后,PVC 仍然存在
kubectl delete statefulset web
kubectl get pvc  # data-web-0, data-web-1, data-web-2 仍存在

# 需手动删除
kubectl delete pvc data-web-0 data-web-1 data-web-2
```

### 滚动更新策略

#### 1. RollingUpdate(默认)

```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0  # 默认值,更新所有 Pod
```

**更新顺序:**
- 从最大序号到最小序号依次更新: web-2 → web-1 → web-0
- 每个 Pod 必须 Running&Ready 后才更新下一个
- 更新方式: 删除旧 Pod → 创建新 Pod

**分区更新(Canary/Blue-Green):**
```yaml
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 2  # 仅更新序号 >= 2 的 Pod
```

示例:
```
replicas: 5, partition: 2
更新影响: web-4, web-3, web-2(更新)
保持旧版: web-1, web-0(不更新)
```

#### 2. OnDelete

```yaml
apiVersion: apps/v1
kind: StatefulSet
spec:
  updateStrategy:
    type: OnDelete  # 手动控制更新
```

**更新行为:**
- 修改 StatefulSet 后,**不会自动更新** Pod
- 必须**手动删除** Pod,控制器才会使用新配置重建
- 适用于需要精细控制更新流程的场景

```bash
# 手动触发更新
kubectl delete pod web-2  # 控制器会用新配置重建 web-2
kubectl delete pod web-1
kubectl delete pod web-0
```

### 稳定的网络标识

StatefulSet 需要配合 Headless Service 使用,提供稳定的网络标识:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx
spec:
  clusterIP: None  # Headless Service
  selector:
    app: nginx
  ports:
  - port: 80
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: web
spec:
  serviceName: "nginx"  # 关联 Headless Service
  replicas: 3
  template:
    metadata:
      labels:
        app: nginx
```

**稳定的 DNS 记录:**
```
<pod-name>.<service-name>.<namespace>.svc.cluster.local

例如:
web-0.nginx.default.svc.cluster.local
web-1.nginx.default.svc.cluster.local
web-2.nginx.default.svc.cluster.local
```

**验证:**
```bash
# 从集群内访问
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup web-0.nginx.default.svc.cluster.local

# 稳定性: Pod 重建后,DNS 记录不变
```

---

## DaemonSet 特性说明

### 调度机制演进

#### v1.12 之前: DaemonSet 控制器直接调度

- 控制器直接设置 Pod 的 `spec.nodeName`,绕过调度器
- 不经过调度器的策略检查(如亲和性、资源检查)
- 简单高效,但功能受限

#### v1.12+: 使用标准调度器

- DaemonSet Pod 通过标准调度器调度
- 支持完整的调度特性:
  - Node affinity
  - Taints and tolerations
  - Pod priority and preemption
  - Resource requests and limits
- 调度失败会有调度器事件记录

**查看调度器处理 DaemonSet:**
```bash
# 查看 Pod 调度事件
kubectl describe pod <daemonset-pod> | grep -A 5 "Events"

# 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler | grep -i daemon
```

### 节点选择机制

#### 1. nodeSelector(简单)

```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  template:
    spec:
      nodeSelector:
        disktype: ssd
        region: us-west
```

#### 2. nodeAffinity(灵活)

```yaml
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/worker
                operator: Exists
              - key: disktype
                operator: In
                values: ["ssd", "nvme"]
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: region
                operator: In
                values: ["us-west"]
```

#### 3. Tolerations(污点容忍)

DaemonSet 通常需要配置丰富的 tolerations,以确保在各种节点上运行:

```yaml
spec:
  template:
    spec:
      tolerations:
      # 允许在 master/control-plane 节点运行
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule
      - key: node-role.kubernetes.io/master
        operator: Exists
        effect: NoSchedule
      
      # 允许在 NotReady/Unreachable 节点运行
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute
      - key: node.kubernetes.io/unreachable
        operator: Exists
        effect: NoExecute
      
      # 允许在资源压力节点运行
      - key: node.kubernetes.io/disk-pressure
        operator: Exists
        effect: NoSchedule
      - key: node.kubernetes.io/memory-pressure
        operator: Exists
        effect: NoSchedule
      - key: node.kubernetes.io/pid-pressure
        operator: Exists
        effect: NoSchedule
      
      # 允许在未初始化节点运行
      - key: node.kubernetes.io/unschedulable
        operator: Exists
        effect: NoSchedule
      
      # 允许在网络不可用节点运行
      - key: node.kubernetes.io/network-unavailable
        operator: Exists
        effect: NoSchedule
```

**常见 DaemonSet 类型的容忍度配置:**

**CNI 网络插件(必须在所有节点运行):**
```yaml
tolerations:
- operator: Exists  # 容忍所有污点
```

**监控/日志采集(应在所有节点运行):**
```yaml
tolerations:
- key: node-role.kubernetes.io/control-plane
  operator: Exists
- key: node.kubernetes.io/not-ready
  operator: Exists
  effect: NoExecute
- key: node.kubernetes.io/unreachable
  operator: Exists
  effect: NoExecute
```

**GPU 驱动(仅在 GPU 节点运行):**
```yaml
nodeSelector:
  accelerator: nvidia-gpu
tolerations:
- key: nvidia.com/gpu
  operator: Exists
```

### 滚动更新策略

#### 1. RollingUpdate(默认)

```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 默认值,每次最多更新 1 个 Pod
```

**更新行为:**
- 逐节点更新: 删除旧 Pod → 创建新 Pod → 等待新 Pod Ready → 更新下一节点
- maxUnavailable: 控制同时更新的最大节点数
  - 数字: 最多 N 个节点同时更新
  - 百分比: 最多 N% 的节点同时更新

**示例:**
```yaml
# 快速更新(风险较高)
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 5  # 同时更新 5 个节点

# 安全更新(速度较慢)
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 每次只更新 1 个节点
```

#### 2. OnDelete

```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  updateStrategy:
    type: OnDelete  # 手动控制更新
```

**更新行为:**
- 修改 DaemonSet 后,**不会自动更新** Pod
- 必须**手动删除** Pod,控制器才会使用新配置重建
- 适用于需要手动控制更新节点顺序的场景

```bash
# 手动按节点逐个更新
kubectl delete pod fluentd-node1
# 等待 Pod Ready 后继续
kubectl delete pod fluentd-node2
```

### 主机资源访问

DaemonSet 通常需要访问主机资源(文件系统、网络、进程等):

#### 1. hostPath 卷

```yaml
spec:
  template:
    spec:
      containers:
      - name: fluentd
        volumeMounts:
        - name: varlog
          mountPath: /var/log
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
```

#### 2. hostNetwork

```yaml
spec:
  template:
    spec:
      hostNetwork: true  # 使用主机网络命名空间
      dnsPolicy: ClusterFirstWithHostNet  # DNS 策略
```

#### 3. hostPID / hostIPC

```yaml
spec:
  template:
    spec:
      hostPID: true  # 访问主机进程
      hostIPC: true  # 访问主机 IPC
```

#### 4. SecurityContext(特权模式)

```yaml
spec:
  template:
    spec:
      containers:
      - name: node-exporter
        securityContext:
          privileged: true  # 特权容器
          capabilities:
            add: ["SYS_ADMIN", "NET_ADMIN"]
```

---

## 实战案例

### 案例 1: StatefulSet 有序扩容卡住

**现象:**
```bash
$ kubectl get pods -l app=mysql
NAME      READY   STATUS    RESTARTS   AGE
mysql-0   1/1     Running   0          5m
mysql-1   0/1     Pending   0          30s
```

**事件:**
```
FailedCreate: create Pod mysql-1 in StatefulSet default/mysql failed error: persistentvolumeclaim "data-mysql-1" not found
```

**排查:**
```bash
# 查看 PVC 状态
$ kubectl get pvc
NAME          STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
data-mysql-0  Bound     pv-001   10Gi       RWO            standard       5m
data-mysql-1  Pending                                      standard       30s

# 查看 PVC 详情
$ kubectl describe pvc data-mysql-1
Events:
  Warning  ProvisioningFailed  waiting for a volume to be created, either by external provisioner or by manual PV creation
```

**原因:** StorageClass 没有配置动态供应,或 PV 资源不足。

**解决:**
```bash
# 方案 1: 手动创建 PV
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-mysql-1
spec:
  capacity:
    storage: 10Gi
  accessModes:
    - ReadWriteOnce
  hostPath:
    path: /data/mysql-1
EOF

# 方案 2: 配置动态供应
kubectl patch storageclass standard -p '{"provisioner": "kubernetes.io/gce-pd"}'
```

---

### 案例 2: StatefulSet 滚动更新回滚

**现象:**
```bash
$ kubectl rollout status statefulset/web
Waiting for 1 pods to be ready...
Waiting for 1 pods to be ready...
```

**事件:**
```
FailedUpdate: update Pod web-2 in StatefulSet default/web failed
```

**排查:**
```bash
# 查看 Pod 状态
$ kubectl get pods -l app=web
NAME    READY   STATUS             RESTARTS   AGE
web-0   1/1     Running            0          10m
web-1   1/1     Running            0          9m
web-2   0/1     CrashLoopBackOff   5          3m

# 查看 Pod 日志
$ kubectl logs web-2
Error: Invalid configuration: missing required field "database.host"
```

**原因:** 新版本配置错误,导致 Pod 无法启动。

**解决:**
```bash
# 方案 1: 使用 partition 暂停更新
kubectl patch statefulset web -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":2}}}}'
# 现在只有 web-2 使用新版本,web-0 和 web-1 保持旧版本

# 方案 2: 完全回滚(手动修改配置)
kubectl edit statefulset web
# 修改 image 或配置回旧版本

# 方案 3: 使用 OnDelete 策略手动控制
kubectl patch statefulset web -p '{"spec":{"updateStrategy":{"type":"OnDelete"}}}'
kubectl delete pod web-2  # 手动删除问题 Pod
```

---

### 案例 3: DaemonSet 未在 Master 节点运行

**现象:**
```bash
$ kubectl get pods -l app=node-exporter -o wide
NAME                  READY   STATUS    RESTARTS   AGE   NODE
node-exporter-abc12   1/1     Running   0          5m    worker-1
node-exporter-def34   1/1     Running   0          5m    worker-2
# master 节点上没有 Pod
```

**事件:**
```
FailedPlacement: failed to place pod on master-1: node had taint {node-role.kubernetes.io/control-plane:NoSchedule}
```

**排查:**
```bash
# 查看 master 节点污点
$ kubectl describe node master-1 | grep Taints
Taints: node-role.kubernetes.io/control-plane:NoSchedule

# 查看 DaemonSet 容忍度
$ kubectl get daemonset node-exporter -o jsonpath='{.spec.template.spec.tolerations}'
[]
```

**原因:** DaemonSet 未配置容忍 master 节点污点。

**解决:**
```bash
# 添加容忍度
kubectl patch daemonset node-exporter -p '
{
  "spec": {
    "template": {
      "spec": {
        "tolerations": [
          {
            "key": "node-role.kubernetes.io/control-plane",
            "operator": "Exists",
            "effect": "NoSchedule"
          }
        ]
      }
    }
  }
}'

# 验证
$ kubectl get pods -l app=node-exporter -o wide
NAME                  READY   STATUS    RESTARTS   AGE   NODE
node-exporter-abc12   1/1     Running   0          5m    worker-1
node-exporter-def34   1/1     Running   0          5m    worker-2
node-exporter-ghi56   1/1     Running   0          10s   master-1
```

---

### 案例 4: DaemonSet hostPort 冲突

**现象:**
```bash
$ kubectl get pods -l app=fluentd
NAME            READY   STATUS    RESTARTS   AGE
fluentd-abc12   1/1     Running   0          5m
fluentd-def34   0/1     Pending   0          5m
```

**事件:**
```
FailedCreate: Error creating: pods "fluentd-def34" is forbidden: host port 24224 is already allocated
```

**排查:**
```bash
# 查看 DaemonSet hostPort 配置
$ kubectl get daemonset fluentd -o yaml | grep -A 5 hostPort
ports:
- containerPort: 24224
  hostPort: 24224
  protocol: TCP

# 检查是否有其他 Pod 使用相同 hostPort
$ kubectl get pods --all-namespaces -o json | jq -r '.items[] | select(.spec.containers[].ports[]?.hostPort==24224) | {name:.metadata.name, namespace:.metadata.namespace, node:.spec.nodeName}'
```

**原因:** 
1. 节点上已有其他 Pod 使用相同 hostPort
2. 或 DaemonSet 配置错误导致同一节点创建多个 Pod

**解决:**
```bash
# 方案 1: 移除 hostPort,使用 hostNetwork
kubectl patch daemonset fluentd --type json -p='[{"op": "remove", "path": "/spec/template/spec/containers/0/ports/0/hostPort"}]'
kubectl patch daemonset fluentd -p '{"spec":{"template":{"spec":{"hostNetwork":true}}}}'

# 方案 2: 修改 hostPort 值
kubectl patch daemonset fluentd --type json -p='[{"op": "replace", "path": "/spec/template/spec/containers/0/ports/0/hostPort", "value": 24225}]'

# 方案 3: 检查节点选择器,确保每节点只有一个 Pod
kubectl get daemonset fluentd -o yaml | grep -A 3 nodeSelector
```

---

### 案例 5: StatefulSet PVC 遗留清理

**现象:**
```bash
# 删除 StatefulSet 后缩容后,PVC 仍然存在
$ kubectl delete statefulset web --cascade=orphan
$ kubectl scale statefulset web --replicas=1

$ kubectl get pvc
NAME        STATUS   VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
data-web-0  Bound    pv-001   10Gi       RWO            standard       10m
data-web-1  Bound    pv-002   10Gi       RWO            standard       10m
data-web-2  Bound    pv-003   10Gi       RWO            standard       10m
# web-1 和 web-2 已不存在,但 PVC 保留
```

**影响:** PVC 继续占用存储配额和底层存储资源。

**解决:**
```bash
# 方案 1: 手动删除不需要的 PVC
kubectl delete pvc data-web-1 data-web-2

# 方案 2: 使用脚本批量清理
#!/bin/bash
STATEFULSET_NAME="web"
REPLICAS=$(kubectl get statefulset $STATEFULSET_NAME -o jsonpath='{.spec.replicas}')

# 获取所有 PVC
kubectl get pvc -l app=$STATEFULSET_NAME -o name | while read pvc; do
  # 提取序号
  ordinal=$(echo $pvc | grep -oP '\d+$')
  if [ "$ordinal" -ge "$REPLICAS" ]; then
    echo "Deleting unused PVC: $pvc (ordinal $ordinal >= replicas $REPLICAS)"
    kubectl delete $pvc
  fi
done

# 方案 3: 使用 StatefulSetAutoDeletePVC 特性(v1.27+ Alpha, v1.31+ Beta)
# 需在 kube-controller-manager 启用 feature gate
--feature-gates=StatefulSetAutoDeletePVC=true

# 然后在 StatefulSet 中配置
apiVersion: apps/v1
kind: StatefulSet
spec:
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Delete  # StatefulSet 删除时删除 PVC
    whenScaled: Delete   # 缩容时删除 PVC
```

---

## 最佳实践

### StatefulSet 最佳实践

#### 1. 选择合适的 Pod 管理策略

```yaml
# 有主从关系 → OrderedReady
apiVersion: apps/v1
kind: StatefulSet
spec:
  podManagementPolicy: OrderedReady
  # 示例: MySQL 主从,ZooKeeper

# 无依赖关系 → Parallel
apiVersion: apps/v1
kind: StatefulSet
spec:
  podManagementPolicy: Parallel
  # 示例: 独立的缓存节点
```

#### 2. 使用 Headless Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  clusterIP: None  # Headless
  selector:
    app: web
---
apiVersion: apps/v1
kind: StatefulSet
spec:
  serviceName: "web"  # 必须指定
```

#### 3. 配置合理的健康检查

```yaml
spec:
  template:
    spec:
      containers:
      - name: app
        startupProbe:  # 启动探针(给足启动时间)
          httpGet:
            path: /health
            port: 8080
          failureThreshold: 30
          periodSeconds: 10
        livenessProbe:  # 存活探针
          httpGet:
            path: /health
            port: 8080
          periodSeconds: 10
        readinessProbe:  # 就绪探针(决定是否接收流量)
          httpGet:
            path: /ready
            port: 8080
          periodSeconds: 5
```

#### 4. 使用分区更新实现金丝雀发布

```bash
# 1. 设置 partition,先更新一个 Pod(最大序号)
kubectl patch statefulset web -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":2}}}}'
# replicas=3 时,只有 web-2 更新

# 2. 观察 web-2 运行情况
kubectl logs web-2
kubectl get pod web-2 -o wide

# 3. 确认无问题后,逐步降低 partition
kubectl patch statefulset web -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":1}}}}'
# 现在 web-1 和 web-2 更新

# 4. 全部更新
kubectl patch statefulset web -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":0}}}}'
```

#### 5. PVC 管理策略

```yaml
# v1.27+ 配置自动删除策略
apiVersion: apps/v1
kind: StatefulSet
spec:
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Retain  # Delete 或 Retain
    whenScaled: Retain   # Delete 或 Retain
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
```

#### 6. 设置 PodDisruptionBudget

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2  # 至少保持 2 个副本
  selector:
    matchLabels:
      app: web
```

---

### DaemonSet 最佳实践

#### 1. 配置完整的容忍度

```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  template:
    spec:
      # 基础容忍度(适用于日志/监控 DaemonSet)
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute
      - key: node.kubernetes.io/unreachable
        operator: Exists
        effect: NoExecute
      - key: node.kubernetes.io/disk-pressure
        operator: Exists
        effect: NoSchedule
      - key: node.kubernetes.io/memory-pressure
        operator: Exists
        effect: NoSchedule
      - key: node.kubernetes.io/unschedulable
        operator: Exists
        effect: NoSchedule

      # CNI 网络插件需要容忍所有污点
      # tolerations:
      # - operator: Exists
```

#### 2. 设置合理的资源请求和限制

```yaml
spec:
  template:
    spec:
      containers:
      - name: fluentd
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
```

#### 3. 使用 updateStrategy 控制更新速度

```yaml
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 保守更新
      # maxUnavailable: 20%  # 或使用百分比
```

#### 4. 使用 hostNetwork 时配置 DNS 策略

```yaml
spec:
  template:
    spec:
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet  # 重要!
```

#### 5. 配置 priorityClassName

```yaml
apiVersion: apps/v1
kind: DaemonSet
spec:
  template:
    spec:
      priorityClassName: system-node-critical  # 系统级 DaemonSet
      # priorityClassName: system-cluster-critical
```

#### 6. 使用节点亲和性精确控制调度

```yaml
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              # 仅在 worker 节点运行
              - key: node-role.kubernetes.io/control-plane
                operator: DoesNotExist
              # 排除特定节点
              - key: node-type
                operator: NotIn
                values: ["special"]
```

#### 7. 设置合理的终止宽限期

```yaml
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 30  # 给 DaemonSet 足够时间清理
```

---

## 相关文档

### Kubernetes 官方文档

- **StatefulSet**: https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/
- **DaemonSet**: https://kubernetes.io/docs/concepts/workloads/controllers/daemonset/
- **Pod 管理策略**: https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/#pod-management-policies
- **StatefulSet 更新策略**: https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/#update-strategies
- **DaemonSet 更新策略**: https://kubernetes.io/docs/tasks/manage-daemon/update-daemon-set/

### KUDIG-DATABASE 相关文档

- **[Domain-33] 01 - Pod 生命周期事件**: Pod 相关事件详解
- **[Domain-33] 02 - Deployment 滚动更新事件**: Deployment 控制器事件
- **[Domain-33] 03 - ReplicaSet 副本管理事件**: ReplicaSet 控制器事件
- **[Domain-33] 06 - 调度器事件与调度失败**: 调度相关事件
- **[Domain-33] 10 - PVC/PV 存储事件**: 存储事件详解
- **[Topic] StatefulSet 故障排查**: 结构化故障排查文档
- **[Topic] DaemonSet 故障排查**: 结构化故障排查文档

### 故障排查相关

- **StatefulSet 常见问题**: 
  - PVC 绑定失败
  - 有序扩容卡住
  - 滚动更新失败
  - 网络标识异常
- **DaemonSet 常见问题**:
  - 节点覆盖不全
  - hostPort 冲突
  - 资源不足导致调度失败
  - 权限问题

### 监控告警

**StatefulSet 关键指标:**
```promql
# Pod 数量偏差
kube_statefulset_status_replicas_ready != kube_statefulset_replicas

# 更新进度
kube_statefulset_status_replicas_updated < kube_statefulset_replicas

# PVC 使用率
kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.8
```

**DaemonSet 关键指标:**
```promql
# 期望副本数与当前副本数偏差
kube_daemonset_status_desired_number_scheduled != kube_daemonset_status_current_number_scheduled

# 就绪副本数
kube_daemonset_status_number_ready < kube_daemonset_status_desired_number_scheduled

# 更新进度
kube_daemonset_status_updated_number_scheduled < kube_daemonset_status_desired_number_scheduled
```

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 08/15
