# Kubernetes v1.29 - v1.33 完整 Feature Gate 与特性参考手册

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 全部 Feature Gate 状态速查与配置参考

---

## 📋 目录

- [一、Feature Gate 说明](#一feature-gate-说明)
- [二、已 GA 特性 (默认启用)](#二已-ga-特性-默认启用)
- [三、Beta 特性 (默认启用)](#三beta-特性-默认启用)
- [四、Beta 特性 (默认禁用)](#四beta-特性-默认禁用)
- [五、Alpha 特性](#五alpha-特性)
- [六、已弃用/已移除 Feature Gate](#六已弃用已移除-feature-gate)
- [七、按组件分类的 Feature Gate](#七按组件分类的-feature-gate)
- [八、启用/禁用 Feature Gate](#八启用禁用-feature-gate)

---

## 一、Feature Gate 说明

```
Feature Gate 生命周期
├── Alpha → 默认禁用，可能不稳定，无向后兼容保证
├── Beta → 默认启用或禁用，趋于稳定
└── GA → 默认启用，稳定，将被锁定/移除

版本兼容性:
├── Alpha 特性: 可能随时变更或移除
├── Beta 特性: 基本稳定，但配置可能变更
└── GA 特性: 稳定，最终会被锁定并移除 Feature Gate
```

---

## 二、已 GA 特性 (默认启用)

### v1.33 GA 的新特性

| Feature Gate | 引入版本 | GA 版本 | 说明 | 配置 |
|:---|:---|:---|:---|:---|
| **SidecarContainers** | v1.28 Alpha | **v1.33** | 原生 Sidecar 容器支持 | 无需配置 |
| **DynamicResourceAllocation** | v1.26 Alpha | **v1.33** | GPU/FPGA 动态资源分配 | 需显式启用 FG |
| **TopologyManagerPolicyOptions** | v1.28 Alpha | **v1.33** | Pod 级 NUMA 拓扑策略 | 需显式启用 FG |
| **UserNamespacesSupport** | v1.25 Alpha | **v1.33** | 用户命名空间支持 | 需配置内核参数 |

### 之前版本已 GA

| Feature Gate | GA 版本 | 说明 |
|:---|:---|:---|
| **PodSecurity** | v1.25 | Pod Security Admission |
| **EphemeralContainers** | v1.25 | 临时调试容器 |
| **ServerSideApply** | v1.22 | 服务端应用 |
| **IPv6DualStack** | v1.23 | IPv4/IPv6 双栈 |
| **GracefulNodeShutdown** | v1.26 | 优雅节点关机 |
| **CSIInlineVolume** | v1.25 | CSI 内联卷 |
| **CSIMigration** | v1.25 | CSI 迁移 |
| **CronJobTimeZone** | v1.25 | CronJob 时区支持 |
| **JobTrackingWithFinalizers** | v1.25 | Job 跟踪 Finalizers |
| **SeccompDefault** | v1.25 | 默认 Seccomp 策略 |
| **LocalStorageCapacityIsolation** | v1.25 | 本地存储容量隔离 |
| **StatefulSetMinReadySeconds** | v1.25 | StatefulSet 最小就绪时间 |
| **IdentifyPodOS** | v1.25 | 识别 Pod OS |
| **DaemonSetUpdateSurge** | v1.25 | DaemonSet 更新激增 |
| **DelegateFSGroupToCSIDriver** | v1.26 | 委托 FSGroup 给 CSI |
| **JobMutableNodeSchedulingDirectives** | v1.26 | Job 可变节点调度指令 |
| **RetroactiveDefaultStorageClass** | v1.26 | 追溯默认 StorageClass |
| **DownwardAPIHugePages** | v1.27 | Downward API HugePages |
| **PodSchedulingReadiness** | v1.30 | Pod 调度就绪 |
| **ValidatingAdmissionPolicy** | v1.30 | CEL 准入策略 |
| **AppArmor** | v1.31 | AppArmor 安全配置 |
| **KubeletTracing** | v1.31 | Kubelet OpenTelemetry 追踪 |
| **PersistentVolumeLastPhaseTransitionTime** | v1.31 | PV 最后阶段转换时间 |

---

## 三、Beta 特性 (默认启用)

| Feature Gate | 引入版本 | Beta 版本 | 说明 | 禁用方式 |
|:---|:---|:---|:---|:---|
| **SchedulerQueueingHints** | v1.28 Alpha | **v1.33 Beta** | 调度器队列提示 | `--feature-gates=SchedulerQueueingHints=false` |
| **KubeletResourceMetrics** | v1.33 Alpha | **v1.33 Beta** | Kubelet 资源指标端点 | `--feature-gates=KubeletResourceMetrics=false` |
| **InPlacePodVerticalScaling** | v1.27 Alpha | v1.29-v1.32 Beta | 原地 Pod 资源调整 (v1.33 降为 Alpha) | - |
| **NFTablesProxyMode** | v1.31 Alpha | **v1.33 Beta** | nftables kube-proxy | `--feature-gates=NFTablesProxyMode=false` |
| **DRAControlPlaneController** | v1.32 Alpha | **v1.33 GA** | DRA 控制平面 | - |
| **VolumeAttributesClass** | v1.31 Alpha | v1.32-v1.33 Alpha | 卷属性类 | - |
| **PodLevelResources** | v1.33 Alpha | - | Pod 级资源限制 | - |

---

## 四、Beta 特性 (默认禁用)

| Feature Gate | 引入版本 | 说明 | 启用方式 |
|:---|:---|:---|:---|
| **NodeSwap** | v1.22 Alpha | 节点 Swap 支持 | `--feature-gates=NodeSwap=true` |
| **NodeLogQuery** | v1.30 Alpha | kubectl 节点日志查询 | `--feature-gates=NodeLogQuery=true` |
| **VolumeAttributesClass** | v1.31 Alpha | 动态调整存储性能 | `--feature-gates=VolumeAttributesClass=true` |
| **HonorPVReclaimPolicy** | v1.32 Alpha | 尊重 PV 回收策略 | `--feature-gates=HonorPVReclaimPolicy=true` |
| **PersistentVolumeDeleteProtection** | v1.33 Alpha | PV 删除保护 | `--feature-gates=PersistentVolumeDeleteProtection=true` |
| **ClusterTrustBundle** | v1.33 Alpha | 集群信任包 | `--feature-gates=ClusterTrustBundle=true` |
| **CrashDaemonSetPodAfterRestartLimits** | v1.33 Alpha | DaemonSet 崩溃后重启限制 | `--feature-gates=CrashDaemonSetPodAfterRestartLimits=true` |
| **IPsecEncryptedOverlay** | v1.33 Alpha | IPsec 加密覆盖网络 | `--feature-gates=IPsecEncryptedOverlay=true` |
| **ServiceAccountTokenJTI** | v1.33 Alpha | ServiceAccount Token JTI | `--feature-gates=ServiceAccountTokenJTI=true` |
| **ServiceAccountTokenNodeBindingValidation** | v1.33 Alpha | Token 节点绑定验证 | `--feature-gates=ServiceAccountTokenNodeBindingValidation=true` |
| **CPUManagerPolicyAlphaOptions** | v1.33 Alpha | CPU Manager Alpha 选项 | `--feature-gates=CPUManagerPolicyAlphaOptions=true` |
| **CPUManagerPolicyBetaOptions** | v1.33 Alpha | CPU Manager Beta 选项 | `--feature-gates=CPUManagerPolicyBetaOptions=true` |
| **MemoryQoS** | v1.27 Alpha | cgroup v2 内存 QoS | `--feature-gates=MemoryQoS=true` |
| **ProcMountType** | v1.12 Alpha | 进程挂载类型 | `--feature-gates=ProcMountType=true` |
| **JobSuccessPolicy** | v1.31 Alpha | Job 成功策略 | `--feature-gates=JobSuccessPolicy=true` |
| **CoordinatedLeaderElection** | v1.32 Alpha | 协调领导者选举 | `--feature-gates=CoordinatedLeaderElection=true` |
| **OrderedNamespaceDeletion** | v1.33 Alpha | 有序命名空间删除 | `--feature-gates=OrderedNamespaceDeletion=true` |
| **OnlyAllowNodeDeletionDiscovery** | v1.33 Alpha | 仅允许节点删除发现 | `--feature-gates=OnlyAllowNodeDeletionDiscovery=true` |
| **RetryGenerateName** | v1.33 Alpha | 重试生成名称 | `--feature-gates=RetryGenerateName=true` |
| **WatchListClient** | v1.33 Alpha | 监视列表客户端 | `--feature-gates=WatchListClient=true` |
| **ConcurrentWatchObjectDecode** | v1.33 Alpha | 并发监视对象解码 | `--feature-gates=ConcurrentWatchObjectDecode=true` |
| **StorageVersionAPI** | v1.33 Alpha | 存储版本 API | `--feature-gates=StorageVersionAPI=true` |
| **AggregatedDiscoveryEndpoint** | v1.33 Alpha | 聚合发现端点 | `--feature-gates=AggregatedDiscoveryEndpoint=true` |
| **InformerResourceVersion** | v1.33 Alpha | Informer 资源版本 | `--feature-gates=InformerResourceVersion=true` |
| **ListFullSecretsInEnvVar** | v1.33 Alpha | 环境变量中列出完整 Secret | `--feature-gates=ListFullSecretsInEnvVar=true` |
| **DisableNodeKubeProxyVersion** | v1.33 Alpha | 禁用节点 kube-proxy 版本 | `--feature-gates=DisableNodeKubeProxyVersion=true` |
| **AllowDNSOnlyNodeCSR** | v1.33 Alpha | 允许仅 DNS 节点 CSR | `--feature-gates=AllowDNSOnlyNodeCSR=true` |
| **AllowInsecureKubeletCertificateSigningRequests** | v1.33 Alpha | 允许不安全 kubelet CSR | `--feature-gates=AllowInsecureKubeletCertificateSigningRequests=true` |
| **DisableNodeCSIPlugin** | v1.33 Alpha | 禁用节点 CSI 插件 | `--feature-gates=DisableNodeCSIPlugin=true` |
| **SELinuxMount** | v1.30 Alpha | SELinux 挂载优化 | `--feature-gates=SELinuxMount=true` |
| **SELinuxChangePolicy** | v1.33 Alpha | SELinux 策略变更 | `--feature-gates=SELinuxChangePolicy=true` |
| **SupplementalGroupsPolicy** | v1.33 Alpha | 补充组策略 | `--feature-gates=SupplementalGroupsPolicy=true` |
| **ImageMaximumGCAge** | v1.33 Alpha | 镜像最大 GC 年龄 | `--feature-gates=ImageMaximumGCAge=true` |
| **ImageVolume** | v1.33 Alpha | 镜像卷 | `--feature-gates=ImageVolume=true` |
| **RelaxedEnvironmentVariableValidation** | v1.33 Alpha | 宽松环境变量验证 | `--feature-gates=RelaxedEnvironmentVariableValidation=true` |
| **RelaxedDNSSearchValidation** | v1.33 Alpha | 宽松 DNS 搜索验证 | `--feature-gates=RelaxedDNSSearchValidation=true` |
| **PodLifecycleSleepAction** | v1.33 Alpha | Pod 生命周期睡眠动作 | `--feature-gates=PodLifecycleSleepAction=true` |
| **PodLifecycleSleepActionAllowOnFinished** | v1.33 Alpha | 完成后允许睡眠动作 | `--feature-gates=PodLifecycleSleepActionAllowOnFinished=true` |
| **DynamicResourceAllocation** | v1.26 Alpha | DRA (v1.33 GA 但需显式启用) | `--feature-gates=DynamicResourceAllocation=true` |

---

## 五、Alpha 特性

| Feature Gate | 引入版本 | 说明 | 启用方式 |
|:---|:---|:---|:---|
| **InPlacePodVerticalScaling** | v1.27 Alpha → v1.29-32 Beta → **v1.33 Alpha** | 原地 Pod 资源调整 (降级) | `--feature-gates=InPlacePodVerticalScaling=true` |
| **CrossNamespaceVolumeDataSource** | v1.33 Alpha | 跨命名空间存储引用 | `--feature-gates=CrossNamespaceVolumeDataSource=true` |
| **BtreeWatchCache** | v1.33 Alpha | B-tree 监视缓存 | `--feature-gates=BtreeWatchCache=true` |
| **PortForwardWebsockets** | v1.33 Alpha | WebSocket 端口转发 | `--feature-gates=PortForwardWebsockets=true` |
| **StorageNamespaceIndex** | v1.33 Alpha | 存储命名空间索引 | `--feature-gates=StorageNamespaceIndex=true` |
| **AuthorizeWithSelectors** | v1.33 Alpha | 选择器授权 | `--feature-gates=AuthorizeWithSelectors=true` |
| **AuthorizeNodeWithSelectors** | v1.33 Alpha | 节点选择器授权 | `--feature-gates=AuthorizeNodeWithSelectors=true` |
| **CompileInstrumentationLogs** | v1.33 Alpha | 编译检测日志 | `--feature-gates=CompileInstrumentationLogs=true` |
| **ConsistentListFromCache** | v1.33 Alpha | 缓存一致列表 | `--feature-gates=ConsistentListFromCache=true` |
| **AnyVolumeDataSource** | v1.33 Alpha | 任意卷数据源 | `--feature-gates=AnyVolumeDataSource=true` |

---

## 六、已弃用/已移除 Feature Gate

| Feature Gate | 状态 | 说明 | 替代方案 |
|:---|:---|:---|:---|
| **IPv6DualStack** | 已移除 v1.33 | IPv6 双栈 | 默认启用，无需 FG |
| **ExpandCSIVolumes** | 已移除 v1.33 | CSI 卷扩展 | 默认启用，无需 FG |
| **CSIMigrationAWS** | 已移除 v1.33 | AWS CSI 迁移 | 默认启用，无需 FG |
| **CSIMigrationGCE** | 已移除 v1.33 | GCE CSI 迁移 | 默认启用，无需 FG |
| **CSIMigrationAzureDisk** | 已移除 v1.33 | Azure CSI 迁移 | 默认启用，无需 FG |
| **CSIMigrationAzureFile** | 已移除 v1.33 | Azure File CSI 迁移 | 默认启用，无需 FG |
| **CSIMigrationvSphere** | 已移除 v1.33 | vSphere CSI 迁移 | 默认启用，无需 FG |
| **CSIMigrationOpenStack** | 已移除 v1.33 | OpenStack CSI 迁移 | 默认启用，无需 FG |
| **DelegateFSGroupToCSIDriver** | 已移除 v1.33 | 委托 FSGroup | 默认启用，无需 FG |
| **KubeletCredentialProviders** | 已移除 v1.33 | Kubelet 凭证提供者 | 默认启用，无需 FG |
| **LegacyServiceAccountTokenTracking** | 已移除 v1.33 | 旧 SA Token 跟踪 | 默认启用，无需 FG |
| **LegacyServiceAccountTokenCleanUp** | 已移除 v1.33 | 旧 SA Token 清理 | 默认启用，无需 FG |
| **DisableNodeKubeProxyVersion** | 新增 v1.33 | 禁用节点 kube-proxy 版本 | - |
| **DisableNodeCSIPlugin** | 新增 v1.33 | 禁用节点 CSI 插件 | - |

---

## 七、按组件分类的 Feature Gate

### API Server

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| ValidatingAdmissionPolicy | GA v1.30 | CEL 准入策略 |
| AggregatedDiscoveryEndpoint | Alpha v1.33 | 聚合发现端点 |
| StorageVersionAPI | Alpha v1.33 | 存储版本 API |
| AuthorizeWithSelectors | Alpha v1.33 | 选择器授权 |
| AuthorizeNodeWithSelectors | Alpha v1.33 | 节点选择器授权 |
| ConsistentListFromCache | Alpha v1.33 | 缓存一致列表 |
| BtreeWatchCache | Alpha v1.33 | B-tree 监视缓存 |
| CoordinatedLeaderElection | Alpha v1.32 | 协调领导者选举 |
| RetryGenerateName | Alpha v1.33 | 重试生成名称 |

### Scheduler

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| DynamicResourceAllocation | GA v1.33 (需显式启用) | DRA |
| SchedulerQueueingHints | Beta v1.33 | 队列提示 |
| InPlacePodVerticalScaling | Alpha v1.33 | 原地调整 |
| TopologyManagerPolicyOptions | GA v1.33 (需显式启用) | NUMA 拓扑 |

### Kubelet

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| NodeSwap | Alpha | Swap 支持 |
| NodeLogQuery | Alpha | 节点日志查询 |
| KubeletResourceMetrics | Beta v1.33 | 资源指标 |
| KubeletTracing | GA v1.31 | OpenTelemetry |
| MemoryQoS | Alpha | 内存 QoS |
| CPUManagerPolicyAlphaOptions | Alpha v1.33 | CPU Alpha 选项 |
| CPUManagerPolicyBetaOptions | Alpha v1.33 | CPU Beta 选项 |
| ProcMountType | Alpha | 进程挂载类型 |
| SELinuxMount | Alpha | SELinux 挂载 |
| SELinuxChangePolicy | Alpha v1.33 | SELinux 策略 |
| SupplementalGroupsPolicy | Alpha v1.33 | 补充组策略 |
| ImageMaximumGCAge | Alpha v1.33 | 镜像 GC |
| ImageVolume | Alpha v1.33 | 镜像卷 |
| PodLifecycleSleepAction | Alpha v1.33 | 生命周期睡眠 |
| PodLifecycleSleepActionAllowOnFinished | Alpha v1.33 | 完成时睡眠 |
| GracefulNodeShutdown | GA v1.26 | 优雅关机 |
| DisableNodeKubeProxyVersion | Alpha v1.33 | 禁用 kube-proxy 版本 |
| DisableNodeCSIPlugin | Alpha v1.33 | 禁用 CSI 插件 |

### 工作负载

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| SidecarContainers | GA v1.33 | Sidecar 容器 |
| PodSchedulingReadiness | GA v1.30 | 调度就绪 |
| JobSuccessPolicy | Alpha | Job 成功策略 |
| JobMutableNodeSchedulingDirectives | GA v1.26 | 可变调度 |
| CronJobTimeZone | GA v1.25 | 时区支持 |
| StatefulSetStartOrdinal | GA | Start Ordinal |
| StatefulSetAutoDeletePVC | Beta | 自动删除 PVC |
| MaxUnavailableStatefulSet | Beta | 最大不可用 |

### 存储

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| VolumeAttributesClass | Alpha | 动态存储性能 |
| HonorPVReclaimPolicy | Alpha | 尊重回收策略 |
| PersistentVolumeDeleteProtection | Alpha v1.33 | 删除保护 |
| CrossNamespaceVolumeDataSource | Alpha v1.33 | 跨 NS 引用 |
| AnyVolumeDataSource | Alpha v1.33 | 任意数据源 |
| RetroactiveDefaultStorageClass | GA v1.26 | 追溯默认 SC |
| CSIMigration | GA | CSI 迁移 |

### 网络

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| NFTablesProxyMode | Beta v1.33 | nftables kube-proxy |
| IPsecEncryptedOverlay | Alpha v1.33 | IPsec 加密 |
| ServiceTrafficDistribution | Alpha | 流量分布 |
| EndpointSliceTerminatingCondition | GA | 终止条件 |
| EndpointSliceNodeName | GA | 节点名称 |

### 安全

| Feature Gate | 状态 | 说明 |
|:---|:---|:---|
| AppArmor | GA v1.31 | AppArmor 支持 |
| UserNamespacesSupport | GA v1.33 | 用户命名空间 |
| SeccompDefault | GA v1.25 | 默认 Seccomp |
| ClusterTrustBundle | Alpha v1.33 | 集群信任包 |
| ServiceAccountTokenJTI | Alpha v1.33 | Token JTI |
| ServiceAccountTokenNodeBindingValidation | Alpha v1.33 | 节点绑定验证 |
| BoundServiceAccountTokenVolume | GA v1.30 | 绑定 Token |
| LegacyServiceAccountTokenCleanUp | 已移除 | Token 清理 |

---

## 八、启用/禁用 Feature Gate

### kube-apiserver

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    - --feature-gates=DynamicResourceAllocation=true
    - --feature-gates=ValidatingAdmissionPolicy=true
```

### kube-scheduler

```yaml
# /etc/kubernetes/manifests/kube-scheduler.yaml
spec:
  containers:
  - name: kube-scheduler
    command:
    - kube-scheduler
    - --feature-gates=DynamicResourceAllocation=true
    - --feature-gates=SchedulerQueueingHints=true
```

### kubelet

```yaml
# /var/lib/kubelet/config.yaml
featureGates:
  DynamicResourceAllocation: true
  SchedulerQueueingHints: true
  NodeSwap: true
  NodeLogQuery: true
  InPlacePodVerticalScaling: true
  NFTablesProxyMode: true
  KubeletResourceMetrics: true
  VolumeAttributesClass: true
  HonorPVReclaimPolicy: true
```

### kube-proxy

```yaml
# kube-proxy ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    featureGates:
      NFTablesProxyMode: true
```

### 验证 Feature Gate 状态

```bash
# 查看 kubelet 的 Feature Gates
kubectl get --raw /api/v1/nodes/NODE_NAME/proxy/configz | jq '.kubeletconfig.featureGates'

# 查看 API Server 启动参数
kubectl get pods -n kube-system -l component=kube-apiserver -o json | \
  jq '.items[0].spec.containers[0].command | map(select(contains("feature-gates")))'

# 查看 Scheduler 启动参数
kubectl get pods -n kube-system -l component=kube-scheduler -o json | \
  jq '.items[0].spec.containers[0].command | map(select(contains("feature-gates")))'
```

---

## 参考链接

- [官方 Feature Gates 文档](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [Feature Gate 状态表](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates-removed/)
- [KEP 索引](https://www.kubernetes.dev/resources/keps/)
