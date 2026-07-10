---
title: Kubernetes 多租户与资源隔离生产架构
description: 'title: Kubernetes 多租户与资源隔离生产架构'
summary: 'title: Kubernetes 多租户与资源隔离生产架构'
category: general
tags:
- k8s
- production
- best-practice
- architecture
- etcd
- apiserver
- scheduler
- prometheus
- grafana
- cilium
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 99-kubernetes-multi-tenant-architecture的架构设计
- 99-kubernetes-multi-tenant-architecture的组件和交互
- 99-kubernetes-multi-tenant-architecture的系统设计
trigger_keywords:
- Kubernetes
- 多租户与资源隔离生产架构
- workloads
- applications
prerequisites:
- kubectl-basics
- pod-lifecycle
- prometheus-basics
- monitoring-basics
- iac-basics
- cilium-basics
- etcd-basics
- kafka-basics
- redis-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: [[Kubernetes|Kubernetes]] 多租户与资源隔离生产架构
description: '# Kubernetes 多租户与资源隔离生产架构'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- [[etcd|etcd]]
- apiserver
- scheduler
- [[Prometheus|prometheus]]
- grafana
- [[Cilium|cilium]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 多租户与资源隔离生产架构 是什么
- 如何 Kubernetes 多租户与资源隔离生产架构
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- Kubernetes
- 多租户与资源隔离生产架构
- production
- operations
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Kubernetes 多租户与资源隔离生产架构

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 企业级多租户架构设计，含完整 Mermaid 隔离模型图  
> **目标读者**: 平台架构师、安全工程师、SRE

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、多租户隔离模型](#一多租户隔离模型)
- [二、Namespace 级隔离架构](#二namespace-级隔离架构)
- [三、节点池级隔离架构](#三节点池级隔离架构)
- [四、虚拟集群隔离 (vCluster)](#四虚拟集群隔离-vcluster)
- [五、资源配额与限制架构](#五资源配额与限制架构)
- [六、网络隔离架构](#六网络隔离架构)
- [七、Pod 安全标准实施架构](#七pod-安全标准实施架构)
- [八、成本分摊与 FinOps 架构](#八成本分摊与-finops-架构)
- [九、多租户平台即服务 (PaaS) 架构](#九多租户平台即服务-paas-架构)

---

<!-- chunk: 一、多租户隔离模型 -->## 一、多租户隔离模型

## 1.1 隔离层级金字塔

```mermaid
flowchart TB
    subgraph L1["L1: 集群级隔离"]
        C1["独立集群"]
        C2["独立集群"]
    end

    subgraph L2["L2: 虚拟集群隔离"]
        V1["vCluster"]
        V2["vCluster"]
    end

    subgraph L3["L3: 节点池隔离"]
        N1["专用节点池"]
        N2["专用节点池"]
    end

    subgraph L4["L4: Namespace 隔离"]
        NS1["Namespace A"]
        NS2["Namespace B"]
    end

    subgraph L5["L5: Pod 级隔离"]
        P1["SecurityContext"]
        P2["NetworkPolicy"]
    end

    L1 -->|最高隔离<br/>最高成本| L2 --> L3 --> L4 --> L5

    style L1 fill:#ffccbc
    style L2 fill:#ffe0b2
    style L3 fill:#fff9c4
    style L4 fill:#c8e6c9
    style L5 fill:#b3e5fc
```

## 1.2 租户隔离矩阵

| 隔离级别 | 实现方式 | 安全性 | 资源利用率 | 运维复杂度 | 适用场景 |
|:---|:---|:---:|:---:|:---:|:---|
| **集群级** | 独立 K8s 集群 | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | 强合规要求 (金融、政务) |
| **虚拟集群** | vCluster / Kamaji | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | 中型团队、开发测试 |
| **节点池级** | 污点/标签 + 亲和性 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | GPU/高安全负载 |
| **Namespace 级** | RBAC + NetworkPolicy + ResourceQuota | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | 标准多租户 |
| **Pod 级** | SecurityContext + Seccomp | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | 基础隔离 |

## 1.3 多租户架构全景

```mermaid
flowchart TB
    subgraph Platform["平台层"]
        BACKSTAGE["Backstage IDP"]
        ARGO["Argo CD"]
        MONITOR["统一监控"]
    end

    subgraph ControlPlane["共享控制平面"]
        API["API Server"]
        ETCD["etcd"]
        SCHED["Scheduler"]
    end

    subgraph TenantA["租户 A (Team Alpha)"]
        NS_A1["ns: alpha-prod"]
        NS_A2["ns: alpha-staging"]
        SA_A["ServiceAccount"]
        RQ_A["ResourceQuota"]
    end

    subgraph TenantB["租户 B (Team Beta)"]
        NS_B1["ns: beta-prod"]
        NS_B2["ns: beta-staging"]
        SA_B["ServiceAccount"]
        RQ_B["ResourceQuota"]
    end

    subgraph TenantC["租户 C (Team Gamma)"]
        NS_C1["ns: gamma-prod"]
        NS_C2["ns: gamma-staging"]
        SA_C["ServiceAccount"]
        RQ_C["ResourceQuota"]
    end

    subgraph Infra["共享基础设施"]
        INGRESS["Ingress Controller"]
        MONITORING["Prometheus / Grafana"]
        LOGGING["Loki / Fluent Bit"]
        REGISTRY["Harbor"]
    end

    Platform --> ControlPlane
    ControlPlane --> TenantA
    ControlPlane --> TenantB
    ControlPlane --> TenantC

    TenantA -.->|使用| Infra
    TenantB -.->|使用| Infra
    TenantC -.->|使用| Infra

    style TenantA fill:#e3f2fd
    style TenantB fill:#e8f5e9
    style TenantC fill:#fff3e0
    style Infra fill:#f3e5f5
```

---

<!-- chunk: 二、Namespace 级隔离架构 -->## 二、Namespace 级隔离架构

## 2.1 命名空间设计模式

```mermaid
flowchart TB
    subgraph Org["组织架构"]
        TEAM_A["Team Alpha"]
        TEAM_B["Team Beta"]
        TEAM_C["Team Gamma"]
    end

    subgraph Namespaces["命名空间设计"]
        subgraph TeamA_NS["Team Alpha"]
            A_PROD["alpha-prod"]
            A_STG["alpha-staging"]
            A_DEV["alpha-dev"]
            A_TOOLS["alpha-tools"]
        end

        subgraph TeamB_NS["Team Beta"]
            B_PROD["beta-prod"]
            B_STG["beta-staging"]
            B_DEV["beta-dev"]
        end

        subgraph Shared_NS["共享服务"]
            MONITORING["monitoring"]
            LOGGING["logging"]
            INGRESS_NS["ingress-nginx"]
            CERT_MANAGER["cert-manager"]
        end
    end

    TEAM_A --> TeamA_NS
    TEAM_B --> TeamB_NS
    TEAM_C --> Shared_NS

    style TeamA_NS fill:#e3f2fd
    style TeamB_NS fill:#e8f5e9
    style Shared_NS fill:#fff3e0
```

## 2.2 Namespace 模板化创建

```yaml
# namespace-template.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: team-alpha-prod
  labels:
    team: alpha
    environment: production
    cost-center: "CC-1234"
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
# 资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-alpha-quota
  namespace: team-alpha-prod
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 100Gi
    limits.cpu: "40"
    limits.memory: 200Gi
    pods: "50"
    services: "10"
    persistentvolumeclaims: "10"
    secrets: "20"
    configmaps: "20"
---
# 限制范围
apiVersion: v1
kind: LimitRange
metadata:
  name: team-alpha-limits
  namespace: team-alpha-prod
spec:
  limits:
    - default:
        cpu: "1"
        memory: 2Gi
      defaultRequest:
        cpu: "200m"
        memory: 512Mi
      max:
        cpu: "4"
        memory: 16Gi
      min:
        cpu: "50m"
        memory: 128Mi
      type: Container
---
# 默认网络策略：拒绝所有入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: team-alpha-prod
spec:
  podSelector: {}
  policyTypes:
    - Ingress
---
# 允许同命名空间通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: team-alpha-prod
spec:
  podSelector: {}
  ingress:
    - from:
        - podSelector: {}
  policyTypes:
    - Ingress
---
# RBAC 绑定
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-alpha-admin
  namespace: team-alpha-prod
subjects:
  - kind: Group
    name: team-alpha
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: admin
  apiGroup: rbac.authorization.k8s.io
```

## 2.3 命名空间生命周期管理

```mermaid
stateDiagram-v2
    [*] --> Provisioning: 创建请求
    Provisioning --> Active: 资源配置完成

    Active --> QuotaExceeded: 超出配额
    QuotaExceeded --> Active: 清理资源 / 扩容

    Active --> ViolationDetected: 安全违规
    ViolationDetected --> Enforcing: 策略执行
    Enforcing --> Active: 修复完成
    Enforcing --> Suspended: 未修复

    Active --> ScheduledDeletion: 项目结束
    ScheduledDeletion --> Archived: 数据归档
    Archived --> [*]: 清理完成
    Suspended --> ScheduledDeletion
```

---

<!-- chunk: 三、节点池级隔离架构 -->## 三、节点池级隔离架构

## 3.1 多节点池隔离模型

```mermaid
flowchart TB
    subgraph ControlPlane["控制平面"]
        API["API Server"]
        SCHED["Scheduler<br/>+ Taint/Toleration 过滤"]
    end

    subgraph NodePools["节点池"]
        subgraph SystemPool["系统节点池<br/>NoSchedule"]
            SN1[Monitoring]
            SN2[Ingress]
            SN3[Logging]
        end

        subgraph GeneralPool["通用节点池"]
            GN1["Team A Pod"]
            GN2["Team B Pod"]
        end

        subgraph SecurePool["安全节点池<br/>专用硬件"]
            SEC1["金融交易 Pod"]
            SEC2["支付处理 Pod"]
        end

        subgraph GPUPool["GPU 节点池<br/>nvidia.com/gpu"]
            GPU1["AI 训练"]
            GPU2["推理服务"]
        end

        subgraph SpotPool["Spot 节点池<br/>允许中断"]
            SP1["批处理 Job"]
            SP2["CI/CD Runner"]
        end
    end

    API --> SCHED
    SCHED -->|污点过滤| SystemPool
    SCHED -->|默认调度| GeneralPool
    SCHED -->|节点选择器| SecurePool
    SCHED -->|GPU 资源| GPUPool
    SCHED -->|容忍中断| SpotPool

    style SystemPool fill:#ffebee
    style SecurePool fill:#fff3e0
    style GPUPool fill:#e3f2fd
    style SpotPool fill:#e8f5e9
```

## 3.2 节点池配置

```yaml
# 系统节点池：禁止业务 Pod 调度
apiVersion: v1
kind: Node
metadata:
  name: system-node-1
  labels:
    node-role.kubernetes.io/system: "true"
spec:
  taints:
    - key: node-role.kubernetes.io/system
      value: "true"
      effect: NoSchedule
---
# 安全节点池：物理隔离
apiVersion: v1
kind: Node
metadata:
  name: secure-node-1
  labels:
    node-type: secure
    compliance-level: pci-dss
spec:
  taints:
    - key: node-type
      value: secure
      effect: NoSchedule
---
# 业务 Pod 调度到安全节点
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-service
spec:
  template:
    spec:
      nodeSelector:
        node-type: secure
      tolerations:
        - key: node-type
          operator: Equal
          value: secure
          effect: NoSchedule
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values:
                      - payment
              topologyKey: kubernetes.io/hostname
```

---

<!-- chunk: 四、虚拟集群隔离 (vCluster) -->## 四、虚拟集群隔离 (vCluster)

## 4.1 vCluster 架构

```mermaid
flowchart TB
    subgraph HostCluster["宿主机集群 (Host Cluster)"]
        subgraph HostControlPlane["宿主机控制平面"]
            H_API["API Server"]
            H_ETCD["etcd"]
            H_SCHED["Scheduler"]
        end

        subgraph vClusterA["vCluster A"]
            V_API_A["vCluster API Server<br/>(Pod)"]
            V_CM_A["vCluster Controller Manager"]
            V_NS_A["工作负载 Namespace"]
        end

        subgraph vClusterB["vCluster B"]
            V_API_B["vCluster API Server<br/>(Pod)"]
            V_CM_B["vCluster Controller Manager"]
            V_NS_B["工作负载 Namespace"]
        end
    end

    subgraph Users["用户"]
        U_A["Team A"]
        U_B["Team B"]
    end

    U_A -->|kubectl| V_API_A
    U_B -->|kubectl| V_API_B
    V_API_A -->|Sync| H_API
    V_API_B -->|Sync| H_API
    H_API --> H_SCHED --> H_ETCD

    style vClusterA fill:#e3f2fd
    style vClusterB fill:#e8f5e9
```

## 4.2 vCluster 创建

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 vCluster CLI
curl -L -o vcluster "https://github.com/loft-sh/vcluster/releases/latest/download/vcluster-linux-amd64" && \
  sudo install -c -m 0755 vcluster /usr/local/bin

# 创建虚拟集群
vcluster create team-alpha-vcluster \
  --namespace team-alpha \
  --expose-local=false \
  --connect=false

# 连接虚拟集群
vcluster connect team-alpha-vcluster --namespace team-alpha

# 在 vCluster 中操作（完全隔离）
kubectl get nodes  # 只看到 vCluster 的虚拟节点
kubectl create namespace production
kubectl apply -f deployment.yaml
```
---

<!-- chunk: 五、资源配额与限制架构 -->## 五、资源配额与限制架构

## 5.1 层级配额模型

```mermaid
flowchart TB
    subgraph ClusterQuota["集群级配额"]
        CQ["ClusterResourceQuota"]
    end

    subgraph NamespaceQuota["命名空间级配额"]
        RQ_CPU["requests.cpu: 100"]
        RQ_MEM["requests.memory: 500Gi"]
        RQ_PODS["pods: 200"]
    end

    subgraph PodQuota["Pod 级限制"]
        LR_DEFAULT["默认: cpu 500m / mem 1Gi"]
        LR_MAX["最大: cpu 4 / mem 16Gi"]
    end

    subgraph Runtime["运行时限制"]
        CGROUP_CPU["cgroup cpu.max"]
        CGROUP_MEM["cgroup memory.max"]
    end

    CQ --> NamespaceQuota
    NamespaceQuota --> PodQuota
    PodQuota --> Runtime

    style ClusterQuota fill:#ffccbc
    style NamespaceQuota fill:#ffe0b2
    style PodQuota fill:#fff9c4
```

## 5.2 资源配额监控

```yaml
# PrometheusRule: 资源配额告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: resource-quota-alerts
spec:
  groups:
    - name: quota-alerts
      rules:
        - alert: NamespaceQuotaHigh
          expr: |
            (
              kube_resourcequota{resource="requests.cpu",type="used"}
              /
              kube_resourcequota{resource="requests.cpu",type="hard"}
            ) > 0.8
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} CPU 配额使用率超过 80%"

        - alert: NamespaceQuotaExceeded
          expr: |
            (
              kube_resourcequota{resource="pods",type="used"}
              /
              kube_resourcequota{resource="pods",type="hard"}
            ) >= 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "命名空间 {{ $labels.namespace }} Pod 配额已用完"
```

---

<!-- chunk: 六、网络隔离架构 -->## 六、网络隔离架构

## 6.1 零信任网络模型

```mermaid
flowchart TB
    subgraph External["外部流量"]
        INGRESS["Ingress Controller"]
        WAF["WAF"]
    end

    subgraph DMZ["DMZ 层"]
        GW["API Gateway<br/>认证/限流"]
    end

    subgraph Services["服务层"]
        FRONTEND["Frontend<br/>允许: 外部"]
        BACKEND["Backend<br/>允许: Frontend, Monitoring"]
        DATABASE["Database<br/>允许: Backend"]
    end

    subgraph Monitoring["监控层"]
        PROM["Prometheus<br/>允许: 所有 (只读)"]
    end

    External --> INGRESS --> WAF --> GW
    GW --> FRONTEND
    FRONTEND --> BACKEND
    BACKEND --> DATABASE
    PROM -.->| scraping| FRONTEND
    PROM -.->| scraping| BACKEND

    style DMZ fill:#fff3e0
    style Services fill:#e3f2fd
    style Monitoring fill:#e8f5e9
```

## 6.2 Cilium L7 策略

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: backend-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
    - fromEndpoints:
        - matchLabels:
            app: frontend
            k8s:io.kubernetes.pod.namespace: production
      toPorts:
        - ports:
            - port: "8080"
              protocol: TCP
          rules:
            http:
              - method: GET
                path: "/api/v1/.*"
              - method: POST
                path: "/api/v1/orders"
    - fromEndpoints:
        - matchLabels:
            app: prometheus
            k8s:io.kubernetes.pod.namespace: monitoring
      toPorts:
        - ports:
            - port: "9090"
              protocol: TCP
          rules:
            http:
              - method: GET
                path: "/metrics"
```

---

<!-- chunk: 七、Pod 安全标准实施架构 -->## 七、Pod 安全标准实施架构

## 7.1 PSA (Pod Security Admission) 架构

```mermaid
flowchart TB
    subgraph Admission["准入控制"]
        API["API Server"]
        PSA["Pod Security Admission<br/>内置插件"]
        MUTATE["Mutating Webhook"]
        VALIDATE["Validating Webhook"]
    end

    subgraph Policies["安全策略层级"]
        PRIVILEGED["privileged<br/>无限制"]
        BASELINE["baseline<br/>最小限制"]
        RESTRICTED["restricted<br/>最严格"]
    end

    subgraph Enforcement["执行模式"]
        ENFORCE["enforce<br/>拒绝违规"]
        AUDIT["audit<br/>记录审计日志"]
        WARN["warn<br/>返回警告"]
    end

    API --> PSA
    PSA --> MUTATE --> VALIDATE
    VALIDATE --> PRIVILEGED
    VALIDATE --> BASELINE
    VALIDATE --> RESTRICTED
    PRIVILEGED --> ENFORCE
    BASELINE --> AUDIT
    RESTRICTED --> WARN

    style RESTRICTED fill:#c8e6c9
    style ENFORCE fill:#ffebee
```

## 7.2 PSA 实施配置

```yaml
# 集群级配置
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
  - name: PodSecurity
    configuration:
      apiVersion: pod-security.admission.config.k8s.io/v1
      kind: PodSecurityConfiguration
      defaults:
        enforce: "restricted"
        audit: "restricted"
        warn: "restricted"
      exemptions:
        usernames: []
        runtimeClasses: []
        namespaces:
          - kube-system
          - ingress-nginx
          - monitoring
          - cert-manager
---
# 命名空间级覆盖
apiVersion: v1
kind: Namespace
metadata:
  name: legacy-app
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.33
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

## 7.3 合规 Pod 模板

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: compliant-app
spec:
  securityContext:
    runAsNonRoot: true
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: app
      image: myapp:v1.0
      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        runAsUser: 1000
        runAsGroup: 1000
        capabilities:
          drop:
            - ALL
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
        limits:
          cpu: 500m
          memory: 512Mi
      volumeMounts:
        - name: tmp
          mountPath: /tmp
        - name: cache
          mountPath: /cache
  volumes:
    - name: tmp
      emptyDir: {}
    - name: cache
      emptyDir:
        sizeLimit: 100Mi
```

---

<!-- chunk: 八、成本分摊与 FinOps 架构 -->## 八、成本分摊与 FinOps 架构

## 8.1 成本归因模型

```mermaid
flowchart TB
    subgraph Allocation["成本分摊"]
        LABELS["标签体系"]
        NS["命名空间"]
        NODEPOOL["节点池"]
        WORKLOAD["工作负载"]
    end

    subgraph Calculation["计算层"]
        KUBECOST["Kubecost /<br>OpenCost"]
        PROM_DATA["Prometheus 指标"]
    end

    subgraph Report["报告层"]
        TEAM_REPORT["团队报表"]
        PROJECT_REPORT["项目报表"]
        OPTIMIZE["优化建议"]
    end

    LABELS -->|team, project, env| KUBECOST
    NS --> KUBECOST
    NODEPOOL -->|node-type, spot| KUBECOST
    WORKLOAD -->|deployment, statefulset| KUBECOST
    PROM_DATA --> KUBECOST
    KUBECOST --> TEAM_REPORT
    KUBECOST --> PROJECT_REPORT
    KUBECOST --> OPTIMIZE

    style Allocation fill:#e3f2fd
    style Calculation fill:#fff8e1
    style Report fill:#e8f5e9
```

## 8.2 标签规范

```yaml
# 强制标签规范
apiVersion: v1
kind: Namespace
metadata:
  name: team-alpha-prod
  labels:
    # 组织信息
    company.com/team: "alpha"
    company.com/cost-center: "CC-1234"
    company.com/project: "payment-platform"
    # 环境信息
    company.com/environment: "production"
    company.com/criticality: "tier-1"
    # 技术信息
    company.com/domain: "finance"
    company.com/data-classification: "confidential"
---
# Kyverno 策略：强制标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-labels
spec:
  validationFailureAction: Enforce
  rules:
    - name: check-required-labels
      match:
        any:
          - resources:
              kinds:
                - Namespace
      validate:
        message: "Namespace 必须包含 required 标签"
        pattern:
          metadata:
            labels:
              company.com/team: "?*"
              company.com/cost-center: "?*"
              company.com/environment: "?*"
```

---

<!-- chunk: 九、多租户平台即服务 (PaaS) 架构 -->## 九、多租户平台即服务 (PaaS) 架构

## 9.1 自服务平台架构

```mermaid
flowchart TB
    subgraph Portal["开发者门户"]
        BACKSTAGE["Backstage"]
        TEMPLATES["项目模板"]
        CATALOG["服务目录"]
    end

    subgraph GitOps["GitOps 引擎"]
        ARGO["Argo CD"]
        CROSSPLANE["Crossplane"]
        TF["Terraform"]
    end

    subgraph Platform["平台服务"]
        subgraph Provisioning["资源供给"]
            NS["Namespace 创建"]
            RQ["配额设置"]
            RBAC["权限配置"]
            NETPOL["网络隔离"]
        end

        subgraph Services["共享服务"]
            DB["CloudNativePG"]
            CACHE["Redis"]
            MQ["Kafka"]
            REGISTRY["Harbor"]
        end
    end

    subgraph TenantClusters["租户工作负载"]
        T1["Team A 应用"]
        T2["Team B 应用"]
        T3["Team C 应用"]
    end

    BACKSTAGE --> TEMPLATES --> ARGO
    CATALOG --> CROSSPLANE
    ARGO --> Provisioning
    CROSSPLANE --> TF --> Provisioning
    Provisioning --> TenantClusters
    Services -.->|共享| TenantClusters

    style Portal fill:#e3f2fd
    style GitOps fill:#e8f5e9
    style Platform fill:#fff8e1
```

## 9.2 自助服务流程

```mermaid
sequenceDiagram
    participant Dev as 开发者
    participant Portal as Backstage
    participant Git as Git 仓库
    participant Argo as Argo CD
    participant K8s as Kubernetes
    participant Platform as 平台服务

    Dev->>Portal: 申请新项目
    Portal->>Portal: 选择项目模板
    Portal->>Git: 创建 Git Repo + 配置
    Git->>Argo: 触发应用创建
    Argo->>K8s: 创建 Namespace
    Argo->>K8s: 配置 ResourceQuota
    Argo->>K8s: 配置 NetworkPolicy
    Argo->>K8s: 配置 RBAC
    K8s->>Platform: 申请数据库
    Platform->>K8s: 创建 CloudNativePG 集群
    K8s-->>Argo: 同步完成
    Argo-->>Portal: 项目就绪
    Portal-->>Dev: 返回 kubeconfig + 文档
```

---

<!-- chunk: 附录：多租户检查清单 -->## 附录：多租户检查清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# multi-tenant-checklist.sh

NAMESPACE="$1"

echo "=== 命名空间 ${NAMESPACE} 多租户合规检查 ==="

# 1. 网络隔离
echo "[1] 网络策略检查"
kubectl get networkpolicy -n ${NAMESPACE} -o json | jq '.items | length'

# 2. 资源配额
echo "[2] 资源配额检查"
kubectl get resourcequota -n ${NAMESPACE}

# 3. 限制范围
echo "[3] 限制范围检查"
kubectl get limitrange -n ${NAMESPACE}

# 4. RBAC
echo "[4] RBAC 检查"
kubectl get rolebinding,clusterrolebinding -n ${NAMESPACE}

# 5. Pod 安全
echo "[5] Pod 安全配置检查"
kubectl get pods -n ${NAMESPACE} -o json | jq '
  .items[] |
  select(.spec.containers[].securityContext.allowPrivilegeEscalation != false)
  | .metadata.name'

# 6. 标签规范
echo "[6] 标签规范检查"
kubectl get namespace ${NAMESPACE} -o json | jq '.metadata.labels | keys'

# 7. 成本标签
echo "[7] 成本归因标签"
kubectl get all -n ${NAMESPACE} -o json | jq '
  .items[] | select(.metadata.labels["company.com/team"] == null)
  | .metadata.name'

echo "=== 检查完成 ==="
```
---

<!-- chunk: 参考链接 -->## 参考链接

- [Kubernetes 多租户指南](https://kubernetes.io/docs/concepts/security/multi-tenancy/)
- [vCluster 文档](https://www.vcluster.com/docs/)
- [Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [Kubecost](https://www.kubecost.com/)
- [Backstage](https://backstage.io/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生产运维 MOC
- [[生产运维/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[集群基础/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 09-软件物料清单

## Related

- 20-microservice-governance-architecture
- 45-smart-port-shipping
- 65-autonomous-driving-sim
- 19-cloudnative-devops-architecture
- 84-national-park
- 83-cultural-digitization
- 94-smart-prison
- 30-hrtech-saas
- 68-quantum-computing-cloud
- 64-ai-drug-discovery
- 91-urban-air-mobility
- 21-cross-border-ecommerce
- 69-6g-core-network
- 71-smart-tax
- 03-cms-architecture
- 85-hydrogen-energy
- 18-data-midplatform-architecture
- 16-video-shortform-architecture
- 55-crossborder-dtc
- 27-hospitality-tourism
- 40-cloud-gaming
- 87-flexible-manufacturing
- 34-sportstech
- 93-digital-twin-factory
- 28-proptech
- 09-gaming-backend-architecture
- 59-industrial-internet-platform
- 54-social-gaming-metaverse
- 31-instant-retail
- 22-nev-connected-vehicle
- 33-crossborder-warehouse
- 05-online-education-architecture
- 70-ecny-cbdc
- 62-distributed-energy
- 75-affective-computing
- 50-unmanned-retail
- 77-fusion-energy-monitoring
- 42-secondhand-circular
- 79-polar-research
- 26-aviation-travel
- 80-tsn-network
- 43-enterprise-im
- 73-smart-firefighting
- 14-smart-healthcare-architecture
- 96-carbon-capture
- 60-v2x-autonomous-driving
- 74-immersive-xr
- 78-deep-sea-exploration
- 12-smart-logistics-architecture
- 51-smart-manufacturing-mes
- 08-ai-ml-inference-architecture
- 23-xinchuang-it-innovation
- 47-smart-mining
- 58-web3-gamefi
- 29-agritech-iot
- 57-digital-therapeutics
- 92-smart-sports-venue
- 76-synthetic-biology
- 61-smart-grid
- 17-saas-multitenant-architecture
- 11-smart-retail-architecture
- 25-quantitative-trading
- 81-smart-customs
- 24-insurtech
- 90-neuromorphic-computing
- 46-satellite-internet
- 52-smart-water
- 86-solid-state-battery
- 67-brain-computer-interface
- 82-legaltech
- 15-energy-power-architecture
- 37-pet-economy
- 49-livestream-ecommerce
- 66-space-internet
- 06-fintech-architecture
- 88-nanomaterials
- 10-social-media-architecture
- 39-smart-campus
- 13-digital-government-architecture
- 48-vocational-edtech
- 72-digital-twin-city
- 32-smart-restaurant
- 89-crispr-gene-editing
- 56-smart-elderly-care
- 44-martech-adtech
- 95-industrial-metaverse
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/openkruise-index.md|OpenKruise 全局索引]]

## See Also

- 99-keda-event-driven-autoscaling-guide
- 99-kubernetes-deployment-patterns-architecture
- [[集群基础/99-kubernetes-production-architecture-blueprint.md|99-kubernetes-production-architecture-blueprint]]
- [[集群基础/01-production-architecture-design-principles.md|01-production-architecture-design-principles]]


<!-- risk-assessed -->
