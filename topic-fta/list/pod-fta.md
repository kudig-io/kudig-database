# Pod 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 Pod 异常的主要成因与路径，便于在 AIOps/Agent Workflow 中进行根因定位与自动化处置。
- **范围**：以 Kubernetes Pod 生命周期为主线，包含调度、镜像、运行时、健康检查、网络、存储、资源配额、安全策略、节点与控制面等因素。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Pod异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> SCH[调度失败/挂起]
  OR0 --> IMG[镜像相关异常]
  OR0 --> RT[运行时/启动异常]
  OR0 --> HC[健康检查失败]
  OR0 --> NET[网络异常]
  OR0 --> STO[存储异常]
  OR0 --> RES[资源与配额异常]
  OR0 --> SEC[安全与策略异常]
  OR0 --> NODE[节点与基础设施异常]
  OR0 --> CP[控制面与集群异常]
  OR0 --> LIFE[生命周期管理异常]
  OR0 --> CFG[配置与依赖异常]
  OR0 --> TIME[时间与证书异常]

  SCHOR{{OR}}
  SCH --> SCHOR
  SCHOR --> SCH1[节点不可用/污点无法容忍]
  SCHOR --> SCH2[资源不足导致无法调度]
  SCHOR --> SCH3[亲和/反亲和冲突]
  SCHOR --> SCH4[调度器异常或不可达]
  SCHOR --> SCH5[配额/命名空间限制]
  SCHOR --> SCH6[节点选择器/拓扑约束冲突]
  SCHOR --> SCH7[资源碎片化导致放置失败]

  IMGOR{{OR}}
  IMG --> IMGOR
  IMGOR --> IMG1[镜像不存在或标签错误]
  IMGOR --> IMG2[镜像仓库认证失败]
  IMGOR --> IMG3[镜像拉取网络失败]
  IMGOR --> IMG4[镜像格式/架构不匹配]
  IMGOR --> IMG5[镜像仓库限流/配额限制]
  IMGOR --> IMG6[镜像签名/校验失败]

  RTOR{{OR}}
  RT --> RTOR
  RTOR --> RT1[容器启动命令错误]
  RTOR --> RT2[容器依赖或配置缺失]
  RTOR --> RT3[容器运行时异常]
  RTOR --> RT4[频繁重启(CrashLoopBackOff)]
  RTOR --> RT5[OOMKilled]
  RTOR --> RT6[Init 容器失败]
  RTOR --> RT7[文件系统只读/权限异常]

  AND_RT4{{AND}}
  RT4 --> AND_RT4
  AND_RT4 --> RT4A[容器进程异常退出]
  AND_RT4 --> RT4B[重启策略为 Always 或 OnFailure]

  AND_RT5{{AND}}
  RT5 --> AND_RT5
  AND_RT5 --> RT5A[内存上限过低]
  AND_RT5 --> RT5B[内存峰值增长或泄漏]

  HCOR{{OR}}
  HC --> HCOR
  HCOR --> HC1[探针配置错误]
  HCOR --> HC2[应用启动耗时过长]
  HCOR --> HC3[依赖服务不可用]
  HCOR --> HC4[探针端口/协议不一致]

  AND_HC2{{AND}}
  HC2 --> AND_HC2
  AND_HC2 --> HC2A[启动耗时过长]
  AND_HC2 --> HC2B[启动探针/超时设置过短]

  NETOR{{OR}}
  NET --> NETOR
  NETOR --> NET1[DNS 解析失败]
  NETOR --> NET2[CNI 插件异常]
  NETOR --> NET3[网络策略阻断]
  NETOR --> NET4[Service/Endpoint 配置错误]
  NETOR --> NET5[跨节点网络不通]
  NETOR --> NET6[kube-proxy/iptables/ipvs 异常]
  NETOR --> NET7[CoreDNS 异常/延迟升高]

  STOOR{{OR}}
  STO --> STOOR
  STOOR --> STO1[PVC 未绑定或绑定失败]
  STOOR --> STO2[存储类/CSI 驱动异常]
  STOOR --> STO3[挂载权限/路径错误]
  STOOR --> STO4[存储性能/IO 异常]
  STOOR --> STO5[卷只读/卷损坏]
  STOOR --> STO6[多副本写冲突/RWX 争用]

  RESOR{{OR}}
  RES --> RESOR
  RESOR --> RES1[Requests/limits 配置不合理]
  RESOR --> RES2[命名空间资源配额不足]
  RESOR --> RES3[节点资源压力触发驱逐]
  RESOR --> RES4[CPU Throttling 严重]

  AND_RES3{{AND}}
  RES3 --> AND_RES3
  AND_RES3 --> RES3A[节点资源压力(内存/磁盘/CPU)]
  AND_RES3 --> RES3B[Pod 优先级低或 QoS 低]

  SECOR{{OR}}
  SEC --> SECOR
  SECOR --> SEC1[RBAC 权限不足]
  SECOR --> SEC2[Pod 安全策略/准入策略阻断]
  SECOR --> SEC3[镜像安全/签名校验失败]
  SECOR --> SEC4[Seccomp/AppArmor/SELinux 拦截]
  SECOR --> SEC5[准入 Webhook 超时/失败]

  NODEOR{{OR}}
  NODE --> NODEOR
  NODEOR --> NODE1[节点 NotReady/不可达]
  NODEOR --> NODE2[节点时钟漂移]
  NODEOR --> NODE3[内核/驱动异常]
  NODEOR --> NODE4[容器运行时服务异常]
  NODEOR --> NODE5[kubelet 异常或驱逐]
  NODEOR --> NODE6[磁盘满/镜像垃圾回收失败]

  CPOR{{OR}}
  CP --> CPOR
  CPOR --> CP1[API Server 不可用/超时]
  CPOR --> CP2[调度器异常]
  CPOR --> CP3[控制器管理器异常]
  CPOR --> CP4[etcd 异常]
  CPOR --> CP5[集群升级/版本兼容问题]

  LIFEOR{{OR}}
  LIFE --> LIFEOR
  LIFEOR --> LIFE1[优雅终止失败]
  LIFEOR --> LIFE2[探针失败触发重建]
  LIFEOR --> LIFE3[滚动升级配置错误]
  LIFEOR --> LIFE4[preStop/terminationGracePeriod 失效]

  CFGOR{{OR}}
  CFG --> CFGOR
  CFGOR --> CFG1[ConfigMap 缺失/未挂载]
  CFGOR --> CFG2[Secret 缺失/无权限]
  CFGOR --> CFG3[环境变量配置错误]
  CFGOR --> CFG4[ServiceAccount/Token 异常]
  CFGOR --> CFG5[依赖服务地址/证书配置错误]

  TIMEOR{{OR}}
  TIME --> TIMEOR
  TIMEOR --> TIME1[集群/节点证书过期]
  TIMEOR --> TIME2[时间同步失败导致 TLS 失败]
  TIMEOR --> TIME3[证书链不完整/根证书变更]
```

---

## 生产级观测与证据
- **事件**：`kubectl describe pod` 中的 `Warning` 事件（如 `FailedScheduling`、`BackOff`、`Unhealthy`、`FailedMount`、`Evicted` 等）。
- **关键指标**：`kube_pod_status_phase`、`kube_pod_container_status_restarts_total`、`kube_pod_container_status_last_terminated_reason`、`container_memory_working_set_bytes`、`container_cpu_cfs_throttled_seconds_total`、`node_memory_MemAvailable_bytes`、`node_filesystem_avail_bytes`、`kube_node_status_condition`、`coredns_dns_request_count_total`。
- **关键日志**：`kubelet`、`containerd`/`dockerd`、`apiserver`、`scheduler`、`controller-manager`、`etcd`、`coredns`、`cni`、`csi`、`admission webhook`。
- **配置核对**：`Deployment/StatefulSet`、`ConfigMap`/`Secret`、探针参数、`requests/limits`、`imagePullSecrets`、`securityContext`、`networkPolicy`。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    {
      "name": "开始",
      "action": "start",
      "step": "start_pod_fta",
      "next_step": "event_pod_abnormal"
    },
    {
      "name": "顶事件: Pod异常",
      "action": "event",
      "step": "event_pod_abnormal",
      "description": "Pod 出现 Pending/CrashLoopBackOff/OOMKilled/NotReady 等异常表现",
      "next_step": "gate_root_or"
    },
    {
      "name": "根因 OR 门",
      "action": "gate_or",
      "step": "gate_root_or",
      "description": "任一子事件成立即可触发 Pod 异常",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "cat_scheduling",
        "cat_image",
        "cat_runtime",
        "cat_healthcheck",
        "cat_network",
        "cat_storage",
        "cat_resource",
        "cat_security",
        "cat_node",
        "cat_controlplane",
        "cat_lifecycle",
        "cat_config",
        "cat_time"
      ]
    },

    {
      "name": "调度失败/挂起",
      "action": "event",
      "step": "cat_scheduling",
      "description": "Pod 长时间 Pending 或调度失败",
      "next_step": "gate_scheduling_or"
    },
    {
      "name": "调度 OR 门",
      "action": "gate_or",
      "step": "gate_scheduling_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "evt_node_unready",
        "evt_resource_insufficient",
        "evt_affinity_conflict",
        "evt_scheduler_down",
        "evt_ns_quota",
        "evt_node_selector_conflict",
        "evt_fragmentation"
      ]
    },
    { "name": "节点不可用/污点无法容忍", "action": "event", "step": "evt_node_unready" },
    { "name": "资源不足导致无法调度", "action": "event", "step": "evt_resource_insufficient" },
    { "name": "亲和/反亲和冲突", "action": "event", "step": "evt_affinity_conflict" },
    { "name": "调度器异常或不可达", "action": "event", "step": "evt_scheduler_down" },
    { "name": "命名空间配额/限制", "action": "event", "step": "evt_ns_quota" },
    { "name": "节点选择器/拓扑约束冲突", "action": "event", "step": "evt_node_selector_conflict" },
    { "name": "资源碎片化导致放置失败", "action": "event", "step": "evt_fragmentation" },

    {
      "name": "镜像相关异常",
      "action": "event",
      "step": "cat_image",
      "description": "ImagePullBackOff/ErrImagePull",
      "next_step": "gate_image_or"
    },
    {
      "name": "镜像 OR 门",
      "action": "gate_or",
      "step": "gate_image_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "evt_image_not_found",
        "evt_image_auth_fail",
        "evt_image_network_fail",
        "evt_image_arch_mismatch",
        "evt_image_rate_limit",
        "evt_image_signature_fail"
      ]
    },
    { "name": "镜像不存在或标签错误", "action": "event", "step": "evt_image_not_found" },
    { "name": "镜像仓库认证失败", "action": "event", "step": "evt_image_auth_fail" },
    { "name": "镜像拉取网络失败", "action": "event", "step": "evt_image_network_fail" },
    { "name": "镜像架构/格式不匹配", "action": "event", "step": "evt_image_arch_mismatch" },
    { "name": "镜像仓库限流/配额限制", "action": "event", "step": "evt_image_rate_limit" },
    { "name": "镜像签名/校验失败", "action": "event", "step": "evt_image_signature_fail" },

    {
      "name": "运行时/启动异常",
      "action": "event",
      "step": "cat_runtime",
      "description": "容器启动失败/CrashLoopBackOff/OOMKilled",
      "next_step": "gate_runtime_or"
    },
    {
      "name": "运行时 OR 门",
      "action": "gate_or",
      "step": "gate_runtime_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "evt_cmd_error",
        "evt_dependency_missing",
        "evt_runtime_error",
        "evt_crashloop",
        "evt_oomkilled",
        "evt_init_fail",
        "evt_fs_readonly"
      ]
    },
    { "name": "容器启动命令错误", "action": "event", "step": "evt_cmd_error" },
    { "name": "容器依赖或配置缺失", "action": "event", "step": "evt_dependency_missing" },
    { "name": "容器运行时异常", "action": "event", "step": "evt_runtime_error" },
    { "name": "Init 容器失败", "action": "event", "step": "evt_init_fail" },
    { "name": "文件系统只读/权限异常", "action": "event", "step": "evt_fs_readonly" },

    {
      "name": "频繁重启 (CrashLoopBackOff)",
      "action": "event",
      "step": "evt_crashloop",
      "next_step": "gate_crashloop_and"
    },
    {
      "name": "CrashLoop AND 门",
      "action": "gate_and",
      "step": "gate_crashloop_and",
      "control": "and_gate",
      "gate_type": "AND",
      "next_steps": ["evt_container_exit", "evt_restart_policy"]
    },
    { "name": "容器进程异常退出", "action": "event", "step": "evt_container_exit" },
    { "name": "重启策略为 Always/OnFailure", "action": "event", "step": "evt_restart_policy" },

    {
      "name": "OOMKilled",
      "action": "event",
      "step": "evt_oomkilled",
      "next_step": "gate_oom_and"
    },
    {
      "name": "OOM AND 门",
      "action": "gate_and",
      "step": "gate_oom_and",
      "control": "and_gate",
      "gate_type": "AND",
      "next_steps": ["evt_mem_limit_low", "evt_mem_spike"]
    },
    { "name": "内存上限过低", "action": "event", "step": "evt_mem_limit_low" },
    { "name": "内存峰值增长/泄漏", "action": "event", "step": "evt_mem_spike" },

    {
      "name": "健康检查失败",
      "action": "event",
      "step": "cat_healthcheck",
      "description": "Readiness/Liveness/Startup 探针失败",
      "next_step": "gate_hc_or"
    },
    {
      "name": "健康检查 OR 门",
      "action": "gate_or",
      "step": "gate_hc_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_probe_bad", "evt_startup_timeout", "evt_dependency_down", "evt_probe_port_mismatch"]
    },
    { "name": "探针配置错误", "action": "event", "step": "evt_probe_bad" },
    {
      "name": "启动超时",
      "action": "event",
      "step": "evt_startup_timeout",
      "next_step": "gate_startup_and"
    },
    {
      "name": "启动超时 AND 门",
      "action": "gate_and",
      "step": "gate_startup_and",
      "control": "and_gate",
      "gate_type": "AND",
      "next_steps": ["evt_startup_slow", "evt_probe_timeout_short"]
    },
    { "name": "启动耗时过长", "action": "event", "step": "evt_startup_slow" },
    { "name": "启动探针/超时设置过短", "action": "event", "step": "evt_probe_timeout_short" },
    { "name": "依赖服务不可用", "action": "event", "step": "evt_dependency_down" },
    { "name": "探针端口/协议不一致", "action": "event", "step": "evt_probe_port_mismatch" },

    {
      "name": "网络异常",
      "action": "event",
      "step": "cat_network",
      "next_step": "gate_net_or"
    },
    {
      "name": "网络 OR 门",
      "action": "gate_or",
      "step": "gate_net_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "evt_dns_fail",
        "evt_cni_fail",
        "evt_netpolicy_block",
        "evt_service_misconfig",
        "evt_crossnode_unreachable",
        "evt_kubeproxy_fail",
        "evt_coredns_slow"
      ]
    },
    { "name": "DNS 解析失败", "action": "event", "step": "evt_dns_fail" },
    { "name": "CNI 插件异常", "action": "event", "step": "evt_cni_fail" },
    { "name": "网络策略阻断", "action": "event", "step": "evt_netpolicy_block" },
    { "name": "Service/Endpoint 配置错误", "action": "event", "step": "evt_service_misconfig" },
    { "name": "跨节点网络不通", "action": "event", "step": "evt_crossnode_unreachable" },
    { "name": "kube-proxy/iptables/ipvs 异常", "action": "event", "step": "evt_kubeproxy_fail" },
    { "name": "CoreDNS 异常/延迟升高", "action": "event", "step": "evt_coredns_slow" },

    {
      "name": "存储异常",
      "action": "event",
      "step": "cat_storage",
      "next_step": "gate_storage_or"
    },
    {
      "name": "存储 OR 门",
      "action": "gate_or",
      "step": "gate_storage_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "evt_pvc_unbound",
        "evt_csi_fail",
        "evt_mount_perm",
        "evt_io_latency",
        "evt_volume_readonly",
        "evt_rwx_contention"
      ]
    },
    { "name": "PVC 未绑定或绑定失败", "action": "event", "step": "evt_pvc_unbound" },
    { "name": "存储类/CSI 驱动异常", "action": "event", "step": "evt_csi_fail" },
    { "name": "挂载权限/路径错误", "action": "event", "step": "evt_mount_perm" },
    { "name": "存储性能/IO 异常", "action": "event", "step": "evt_io_latency" },
    { "name": "卷只读/卷损坏", "action": "event", "step": "evt_volume_readonly" },
    { "name": "多副本写冲突/RWX 争用", "action": "event", "step": "evt_rwx_contention" },

    {
      "name": "资源与配额异常",
      "action": "event",
      "step": "cat_resource",
      "next_step": "gate_resource_or"
    },
    {
      "name": "资源 OR 门",
      "action": "gate_or",
      "step": "gate_resource_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_limits_bad", "evt_quota_low", "evt_evicted", "evt_cpu_throttle"]
    },
    { "name": "Requests/limits 配置不合理", "action": "event", "step": "evt_limits_bad" },
    { "name": "命名空间资源配额不足", "action": "event", "step": "evt_quota_low" },
    {
      "name": "节点驱逐",
      "action": "event",
      "step": "evt_evicted",
      "next_step": "gate_evicted_and"
    },
    {
      "name": "驱逐 AND 门",
      "action": "gate_and",
      "step": "gate_evicted_and",
      "control": "and_gate",
      "gate_type": "AND",
      "next_steps": ["evt_node_pressure", "evt_low_priority"]
    },
    { "name": "节点资源压力(内存/磁盘/CPU)", "action": "event", "step": "evt_node_pressure" },
    { "name": "Pod 优先级低或 QoS 低", "action": "event", "step": "evt_low_priority" },
    { "name": "CPU Throttling 严重", "action": "event", "step": "evt_cpu_throttle" },

    {
      "name": "安全与策略异常",
      "action": "event",
      "step": "cat_security",
      "next_step": "gate_security_or"
    },
    {
      "name": "安全 OR 门",
      "action": "gate_or",
      "step": "gate_security_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_rbac_denied", "evt_admission_block", "evt_image_policy", "evt_seccomp_block", "evt_webhook_timeout"]
    },
    { "name": "RBAC 权限不足", "action": "event", "step": "evt_rbac_denied" },
    { "name": "准入策略/PSA/OPA 阻断", "action": "event", "step": "evt_admission_block" },
    { "name": "镜像安全/签名校验失败", "action": "event", "step": "evt_image_policy" },
    { "name": "Seccomp/AppArmor/SELinux 拦截", "action": "event", "step": "evt_seccomp_block" },
    { "name": "准入 Webhook 超时/失败", "action": "event", "step": "evt_webhook_timeout" },

    {
      "name": "节点与基础设施异常",
      "action": "event",
      "step": "cat_node",
      "next_step": "gate_node_or"
    },
    {
      "name": "节点 OR 门",
      "action": "gate_or",
      "step": "gate_node_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": [
        "evt_node_notready",
        "evt_clock_skew",
        "evt_kernel_issue",
        "evt_runtime_service",
        "evt_kubelet_issue",
        "evt_disk_full"
      ]
    },
    { "name": "节点 NotReady/不可达", "action": "event", "step": "evt_node_notready" },
    { "name": "节点时钟漂移", "action": "event", "step": "evt_clock_skew" },
    { "name": "内核/驱动异常", "action": "event", "step": "evt_kernel_issue" },
    { "name": "容器运行时服务异常", "action": "event", "step": "evt_runtime_service" },
    { "name": "kubelet 异常或驱逐", "action": "event", "step": "evt_kubelet_issue" },
    { "name": "磁盘满/镜像回收失败", "action": "event", "step": "evt_disk_full" },

    {
      "name": "控制面与集群异常",
      "action": "event",
      "step": "cat_controlplane",
      "next_step": "gate_cp_or"
    },
    {
      "name": "控制面 OR 门",
      "action": "gate_or",
      "step": "gate_cp_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_apiserver_down", "evt_scheduler_issue", "evt_controller_issue", "evt_etcd_issue", "evt_upgrade_incompat"]
    },
    { "name": "API Server 不可用/超时", "action": "event", "step": "evt_apiserver_down" },
    { "name": "调度器异常", "action": "event", "step": "evt_scheduler_issue" },
    { "name": "控制器管理器异常", "action": "event", "step": "evt_controller_issue" },
    { "name": "etcd 异常", "action": "event", "step": "evt_etcd_issue" },
    { "name": "集群升级/版本兼容问题", "action": "event", "step": "evt_upgrade_incompat" },

    {
      "name": "生命周期管理异常",
      "action": "event",
      "step": "cat_lifecycle",
      "next_step": "gate_life_or"
    },
    {
      "name": "生命周期 OR 门",
      "action": "gate_or",
      "step": "gate_life_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_graceful_fail", "evt_probe_recreate", "evt_rollout_bad", "evt_prestop_fail"]
    },
    { "name": "优雅终止失败", "action": "event", "step": "evt_graceful_fail" },
    { "name": "探针失败触发重建", "action": "event", "step": "evt_probe_recreate" },
    { "name": "滚动升级配置错误", "action": "event", "step": "evt_rollout_bad" },
    { "name": "preStop/terminationGracePeriod 失效", "action": "event", "step": "evt_prestop_fail" },

    {
      "name": "配置与依赖异常",
      "action": "event",
      "step": "cat_config",
      "next_step": "gate_config_or"
    },
    {
      "name": "配置 OR 门",
      "action": "gate_or",
      "step": "gate_config_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_cfg_missing", "evt_secret_missing", "evt_env_bad", "evt_sa_token_bad", "evt_dep_endpoint_bad"]
    },
    { "name": "ConfigMap 缺失/未挂载", "action": "event", "step": "evt_cfg_missing" },
    { "name": "Secret 缺失/无权限", "action": "event", "step": "evt_secret_missing" },
    { "name": "环境变量配置错误", "action": "event", "step": "evt_env_bad" },
    { "name": "ServiceAccount/Token 异常", "action": "event", "step": "evt_sa_token_bad" },
    { "name": "依赖服务地址/证书配置错误", "action": "event", "step": "evt_dep_endpoint_bad" },

    {
      "name": "时间与证书异常",
      "action": "event",
      "step": "cat_time",
      "next_step": "gate_time_or"
    },
    {
      "name": "时间/证书 OR 门",
      "action": "gate_or",
      "step": "gate_time_or",
      "control": "or_gate",
      "gate_type": "OR",
      "next_steps": ["evt_cert_expired", "evt_time_skew_tls", "evt_ca_chain_bad"]
    },
    { "name": "集群/节点证书过期", "action": "event", "step": "evt_cert_expired" },
    { "name": "时间同步失败导致 TLS 失败", "action": "event", "step": "evt_time_skew_tls" },
    { "name": "证书链不完整/根证书变更", "action": "event", "step": "evt_ca_chain_bad" },

    {
      "name": "结束",
      "action": "end",
      "step": "end_pod_fta"
    }
  ]
}
```
