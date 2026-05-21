---
title: Kubernetes 生产环境部署模式架构详解
description: 'title: Kubernetes 生产环境部署模式架构详解'
category: general
tags:
- k8s
- production
- best-practice
- deployment
- architecture
- istio
- harbor
- hpa
- pdb
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 如何部署99-kubernetes-deployment-patterns-architecture？
- 99-kubernetes-deployment-patterns-architecture的部署模式和最佳实践
- 99-kubernetes-deployment-patterns-architecture生产环境部署
trigger_keywords:
- Kubernetes
- 生产环境部署模式架构详解
- workloads
- applications
prerequisites:
- kubectl-basics
- pod-lifecycle
- service-mesh-basics
---

title: Kubernetes 生产环境部署模式架构详解
description: '# Kubernetes 生产环境部署模式架构详解'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- istio
- harbor
- hpa
- pdb
- ingress
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 生产环境部署模式架构详解 是什么
- 如何 Kubernetes 生产环境部署模式架构详解
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- Kubernetes
- 生产环境部署模式架构详解
- production
- operations
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta.md
  label: '故障树: deployment'
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

# Kubernetes 生产环境部署模式架构详解

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 企业级部署模式架构设计，含完整 Mermaid 状态流转图  
> **目标读者**: SRE、DevOps、平台工程师

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、滚动更新 (Rolling Update)](#一滚动更新-rolling-update)
- [二、蓝绿部署 (Blue-Green Deployment)](#二蓝绿部署-blue-green-deployment)
- [三、金丝雀发布 (Canary Release)](#三金丝雀发布-canary-release)
- [四、A/B 测试部署](#四ab-测试部署)
- [五、影子流量部署 (Shadow / Mirror)](#五影子流量部署-shadow--mirror)
- [六、特性开关部署 (Feature Flag)](#六特性开关部署-feature-flag)
- [七、多环境晋升流水线](#七多环境晋升流水线)
- [八、部署模式选型决策树](#八部署模式选型决策树)

---

<!-- chunk: 一、滚动更新 (Rolling Update) -->## 一、滚动更新 (Rolling Update)

#<!-- chunk: 1.1 架构原理 -->## 1.1 架构原理

```mermaid
flowchart LR
    subgraph V1["版本 v1.0"]
        P1["Pod-1"]
        P2["Pod-2"]
        P3["Pod-3"]
    end

    subgraph Transition["滚动更新中"]
        N1["Pod-1 (v1.1)"]
        N2["Pod-2 (v1.0)"]
        N3["Pod-3 (v1.0)"]
    end

    subgraph V2["版本 v1.1"]
        M1["Pod-1"]
        M2["Pod-2"]
        M3["Pod-3"]
    end

    V1 -->|maxSurge=1<br>maxUnavailable=0| Transition
    Transition -->|逐步替换| V2

    style Transition fill:#fff8e1
```

#<!-- chunk: 1.2 状态机流转 -->## 1.2 状态机流转

```mermaid
stateDiagram-v2
    [*] --> Running: 创建 Deployment
    Running --> Updating: 应用新镜像

    Updating --> Progressing: 新 Pod 创建中
    Progressing --> Running: 新 Pod Ready
    Progressing --> Progressing: 继续替换下一个

    Running --> Stalled: 新 Pod 无法就绪
    Stalled --> RollingBack: 自动/手动回滚
    RollingBack --> Running: 回滚完成

    Running --> [*]: 删除 Deployment
```

#<!-- chunk: 1.3 生产配置 -->## 1.3 生产配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-service
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%          # 最多多创建 25% Pod
      maxUnavailable: 0      # 不允许不可用 Pod
  minReadySeconds: 30        # Pod ready 后等待 30s
  progressDeadlineSeconds: 600  # 10 分钟未完成视为失败
  template:
    spec:
      containers:
        - name: payment
          image: payment:v1.1
          readinessProbe:
            httpGet:
              path: /healthz
              port: 8080
            initialDelaySeconds: 10
            periodSeconds: 5
            failureThreshold: 3
```

---

<!-- chunk: 二、蓝绿部署 (Blue-Green Deployment) -->## 二、蓝绿部署 (Blue-Green Deployment)

#<!-- chunk: 2.1 架构原理 -->## 2.1 架构原理

```mermaid
flowchart TB
    subgraph LB["负载均衡层"]
        INGRESS["Ingress / Service"]
        SELECTOR["Selector: app=payment,version=blue"]
    end

    subgraph Blue["Blue 环境 (当前活跃)"]
        B1["payment-blue-1"]
        B2["payment-blue-2"]
        B3["payment-blue-3"]
    end

    subgraph Green["Green 环境 (新版本)"]
        G1["payment-green-1"]
        G2["payment-green-2"]
        G3["payment-green-3"]
    end

    USERS["用户流量"] --> INGRESS
    INGRESS -->|100%| Blue
    INGRESS -.->|0%| Green

    style Blue fill:#c8e6c9
    style Green fill:#fff8e1
```

#<!-- chunk: 2.2 切换流程 -->## 2.2 切换流程

```mermaid
sequenceDiagram
    participant Dev as 开发者
    participant CI as CI/CD
    participant K8s as Kubernetes
    participant Test as 验收测试
    participant LB as 负载均衡
    participant Users as 用户

    Dev->>CI: 提交 v2.0 代码
    CI->>K8s: 部署 Green 环境
    K8s->>K8s: Green Pod 就绪检查
    K8s-->>CI: 部署完成

    CI->>Test: 执行冒烟测试
    Test-->>CI: 测试通过

    CI->>LB: 切换流量 100% → Green
    LB->>Users: 流量导向 Green

    alt 发现问题
        CI->>LB: 回滚流量 → Blue
        LB->>Users: 流量恢复 Blue
    else 运行正常
        CI->>K8s: 删除 Blue 环境
    end
```

#<!-- chunk: 2.3 完整配置 -->## 2.3 完整配置

```yaml
# Blue 环境 (当前活跃)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-blue
spec:
  replicas: 5
  selector:
    matchLabels:
      app: payment
      version: blue
  template:
    metadata:
      labels:
        app: payment
        version: blue
    spec:
      containers:
        - name: payment
          image: payment:v1.0
---
# Green 环境 (新版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-green
spec:
  replicas: 5
  selector:
    matchLabels:
      app: payment
      version: green
  template:
    metadata:
      labels:
        app: payment
        version: green
    spec:
      containers:
        - name: payment
          image: payment:v2.0
---
# Service 切换
apiVersion: v1
kind: Service
metadata:
  name: payment
spec:
  selector:
    app: payment
    version: blue  # 切换为 green 完成发布
  ports:
    - port: 80
      targetPort: 8080
```

#<!-- chunk: 2.4 数据库兼容性处理 -->## 2.4 数据库兼容性处理

```mermaid
flowchart TB
    subgraph DBCompat["数据库兼容性策略"]
        subgraph Forward["向前兼容"]
            F1["v1.0 代码<br/>读/写旧表"]
            F2["v2.0 代码<br/>读旧表 + 写新旧表"]
            F3["v3.0 代码<br/>读/写新表"]
        end

        subgraph Backward["向后兼容"]
            B1["v2.0 读旧表"]
            B2["v1.0 读旧表"]
        end
    end

    F1 -->|部署 v2.0| F2
    F2 -->|双写验证| F2
    F2 -->|部署 v3.0| F3
    B1 -->|回滚| B2

    style F2 fill:#fff8e1
```

---

<!-- chunk: 三、金丝雀发布 (Canary Release) -->## 三、金丝雀发布 (Canary Release)

#<!-- chunk: 3.1 架构原理 -->## 3.1 架构原理

```mermaid
flowchart TB
    subgraph IngressLayer["入口层"]
        ING["Ingress /<br>Gateway API"]
        SPLIT["流量分割<br>90% / 10%"]
    end

    subgraph Stable["稳定版本 (90%)"]
        S1["Pod-1"]
        S2["Pod-2"]
        S3["Pod-3"]
    end

    subgraph Canary["金丝雀版本 (10%)"]
        C1["Pod-1"]
        C2["Pod-2"]
    end

    subgraph Metrics["指标监控"]
        LAT["延迟"]
        ERR["错误率"]
        CPU["CPU 使用率"]
    end

    USERS["用户"] --> ING --> SPLIT
    SPLIT -->|90%| Stable
    SPLIT -->|10%| Canary
    Stable --> METRICS
    Canary --> METRICS
    METRICS -->|自动分析| SPLIT

    style Stable fill:#c8e6c9
    style Canary fill:#ffe0b2
```

#<!-- chunk: 3.2 Flagger 自动金丝雀流程 -->## 3.2 Flagger 自动金丝雀流程

```mermaid
stateDiagram-v2
    [*] --> Initialized: 创建 Canary
    Initialized --> Waiting: 等待触发
    Waiting --> Progressing: 检测到镜像更新

    Progressing --> Promoting: 10% 流量 → 验证通过
    Promoting --> Progressing: 25% → 50% → 75%

    Progressing --> Promoting: 75% → 验证通过
    Promoting --> Succeeded: 100% 流量 + 验证通过
    Succeeded --> Finalizing: 删除旧版本
    Finalizing --> [*]: 发布完成

    Progressing --> Failing: 错误率 > 阈值
    Promoting --> Failing: 延迟 > 阈值
    Failing --> Rollbacking: 自动回滚
    Rollbacking --> Waiting: 回滚完成

    Succeeded --> Waiting: 等待下一次更新
```

#<!-- chunk: 3.3 Flagger + Gateway API 配置 -->## 3.3 Flagger + Gateway API 配置

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: payment-service
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: payment
  service:
    port: 8080
    targetPort: 8080
    gateways:
      - gateway-namespace/payment-gateway
    hosts:
      - payment.example.com
  analysis:
    interval: 30s
    threshold: 5
    maxWeight: 50
    stepWeight: 10
    metrics:
      - name: request-success-rate
        interval: 1m
        thresholdRange:
          min: 99
      - name: request-duration
        interval: 1m
        thresholdRange:
          max: 500
    webhooks:
      - name: load-test
        url: http://flagger-loadtester.test/
        timeout: 5s
        metadata:
          cmd: "hey -z 1m -q 10 -c 2 http://payment.example.com/"
      - name: conformance-test
        type: pre-rollout
        url: http://flagger-loadtester.test/
        timeout: 30s
        metadata:
          type: bash
          cmd: "curl -f http://payment-canary:8080/healthz"
```

#<!-- chunk: 3.4 渐进式流量调整 -->## 3.4 渐进式流量调整

```mermaid
flowchart LR
    subgraph Traffic["流量分配"]
        T0["Step 0<br/>Canary: 0%"]
        T1["Step 1<br/>Canary: 10%"]
        T2["Step 2<br/>Canary: 25%"]
        T3["Step 3<br/>Canary: 50%"]
        T4["Step 4<br/>Canary: 75%"]
        T5["Step 5<br/>Canary: 100%"]
    end

    T0 -->|部署触发| T1
    T1 -->|延迟 < 500ms<br>错误率 < 1%| T2
    T2 -->|指标正常| T3
    T3 -->|指标正常| T4
    T4 -->|指标正常| T5
    T5 -->|完成| DONE["金丝雀成功"]

    T1 -->|错误率 > 5%| ROLLBACK["自动回滚"]
    T2 -->|P99 延迟 > 2s| ROLLBACK
    T3 -->|CPU 飙升| ROLLBACK

    style T0 fill:#e3f2fd
    style T5 fill:#c8e6c9
    style ROLLBACK fill:#ffebee
```

---

<!-- chunk: 四、A/B 测试部署 -->## 四、A/B 测试部署

#<!-- chunk: 4.1 架构原理 -->## 4.1 架构原理

```mermaid
flowchart TB
    subgraph Routing["请求路由层"]
        ING["Ingress /<br>Istio VirtualService"]
        MATCH["匹配规则"]
    end

    subgraph A["版本 A (对照组)"]
        A1["内部用户"]
        A2["移动端用户"]
    end

    subgraph B["版本 B (实验组)"]
        B1["VIP 用户"]
        B2["特定地区"]
    end

    USERS["用户请求"] --> ING --> MATCH
    MATCH -->|Cookie: variant=A| A
    MATCH -->|Cookie: variant=B| B
    MATCH -->|Header: X-Canary=always| B

    A --> ANALYTICS["数据分析"]
    B --> ANALYTICS

    style B fill:#ffe0b2
```

#<!-- chunk: 4.2 Istio A/B 测试配置 -->## 4.2 Istio A/B 测试配置

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: payment-ab-test
spec:
  hosts:
    - payment.example.com
  http:
    - match:
        - headers:
            x-canary:
              exact: "true"
        - uri:
            prefix: /api/v2
      route:
        - destination:
            host: payment
            subset: v2
          weight: 100
    - route:
        - destination:
            host: payment
            subset: v1
          weight: 95
        - destination:
            host: payment
            subset: v2
          weight: 5
---
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: payment-dr
spec:
  host: payment
  subsets:
    - name: v1
      labels:
        version: v1.0
    - name: v2
      labels:
        version: v2.0
```

---

<!-- chunk: 五、影子流量部署 (Shadow / Mirror) -->## 五、影子流量部署 (Shadow / Mirror)

#<!-- chunk: 5.1 架构原理 -->## 5.1 架构原理

```mermaid
flowchart TB
    subgraph Production["生产流量"]
        USER["用户"]
        PROD["Production Service"]
        PROD_DB[(生产数据库)]
    end

    subgraph Shadow["影子环境"]
        SHADOW["Shadow Service"]
        SHADOW_DB[(影子数据库)]
    end

    subgraph Analysis["对比分析"]
        LATENCY["延迟对比"]
        RESPONSE["响应差异"]
        ERROR["错误率对比"]
    end

    USER --> PROD --> PROD_DB
    USER -.->|镜像流量| SHADOW
    SHADOW --> SHADOW_DB
    PROD --> ANALYSIS
    SHADOW --> ANALYSIS

    style Shadow fill:#f3e5f5
    style Analysis fill:#e8f5e9
```

#<!-- chunk: 5.2 Istio 流量镜像配置 -->## 5.2 Istio 流量镜像配置

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: payment-mirror
spec:
  hosts:
    - payment
  http:
    - route:
        - destination:
            host: payment
            subset: stable
          weight: 100
      mirror:
        host: payment
        subset: shadow
      mirrorPercentage:
        value: 100.0  # 镜像 100% 流量
```

---

<!-- chunk: 六、特性开关部署 (Feature Flag) -->## 六、特性开关部署 (Feature Flag)

#<!-- chunk: 6.1 架构原理 -->## 6.1 架构原理

```mermaid
flowchart TB
    subgraph Flags["特性开关平台"]
        UNLEASH["Unleash /<br>LaunchDarkly"]
        RULES["规则引擎"]
    end

    subgraph App["应用层"]
        SDK["Feature Flag SDK"]
        FEAT_A["新功能 A<br/>默认关闭"]
        FEAT_B["新功能 B<br/>灰度开启"]
    end

    subgraph Users["用户分组"]
        U1["普通用户<br/>功能关闭"]
        U2["Beta 用户<br/>功能开启"]
        U3["内部员工<br/>全部开启"]
    end

    UNLEASH --> RULES --> SDK
    SDK --> FEAT_A
    SDK --> FEAT_B
    FEAT_A --> U1
    FEAT_B --> U2
    FEAT_B --> U3

    style Flags fill:#e3f2fd
```

#<!-- chunk: 6.2 与 K8s 集成的特性开关 -->## 6.2 与 K8s 集成的特性开关

```yaml
# ConfigMap 作为简单特性开关
apiVersion: v1
kind: ConfigMap
metadata:
  name: feature-flags
data:
  NEW_PAYMENT_FLOW: "true"
  DARK_MODE: "false"
  BETA_API: "enabled"
---
# 应用通过环境变量读取
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-service
spec:
  template:
    spec:
      containers:
        - name: payment
          image: payment:v1.0
          envFrom:
            - configMapRef:
                name: feature-flags
```

---

<!-- chunk: 七、多环境晋升流水线 -->## 七、多环境晋升流水线

#<!-- chunk: 7.1 完整晋升流程 -->## 7.1 完整晋升流程

```mermaid
flowchart LR
    subgraph DEV["开发环境"]
        D1["Dev Cluster"]
        D2["特性分支部署"]
    end

    subgraph STAGING["预发环境"]
        S1["Staging Cluster"]
        S2["集成测试"]
        S3["性能测试"]
    end

    subgraph PROD["生产环境"]
        P1["Production Cluster"]
        P2["金丝雀发布"]
        P3["全量发布"]
    end

    subgraph DR["灾备环境"]
        DR1["DR Cluster"]
    end

    GIT["Git Tag"] -->|自动触发| D1
    D1 -->|合并 main| S1
    S1 -->|测试通过| P1
    P1 -->|全量发布| P3
    P3 -->|同步| DR1

    style DEV fill:#e3f2fd
    style STAGING fill:#fff8e1
    style PROD fill:#c8e6c9
    style DR fill:#ffebee
```

#<!-- chunk: 7.2 GitOps 晋升流水线 -->## 7.2 GitOps 晋升流水线

```mermaid
sequenceDiagram
    participant Dev as 开发者
    participant Git as Git 仓库
    participant CI as CI Pipeline
    participant Argo as Argo CD
    participant DevK8s as Dev 集群
    participant StgK8s as Staging 集群
    participant ProdK8s as Production 集群

    Dev->>Git: push to feature branch
    Git->>CI: 触发 CI
    CI->>CI: 构建 + 测试 + 扫描
    CI->>CI: 推送镜像到 Harbor

    Dev->>Git: PR → merge to main
    Git->>Argo: 检测 Git 变更
    Argo->>DevK8s: 同步 Dev 环境
    DevK8s-->>Argo: 同步完成

    Dev->>Git: git tag v1.0.0
    Git->>Argo: 检测 tag
    Argo->>StgK8s: 同步 Staging 环境
    StgK8s-->>Argo: 同步完成

    Dev->>Git: PR → merge to release
    Argo->>ProdK8s: 金丝雀发布
    ProdK8s-->>Argo: 验证通过
    Argo->>ProdK8s: 全量发布
```

---

<!-- chunk: 八、部署模式选型决策树 -->## 八、部署模式选型决策树

#<!-- chunk: 8.1 综合决策树 -->## 8.1 综合决策树

```mermaid
flowchart TD
    START([需要发布新版本])

    START --> Q1{是否需要零停机?}
    Q1 -->|否| RECREATE["重建部署<br/>Recreate<br/>⚠️ 有停机时间"]
    Q1 -->|是| Q2

    Q2{是否需要快速回滚?}
    Q2 -->|是| Q3
    Q2 -->|否| Q4

    Q3{资源是否充足?}
    Q3 -->|是| BLUE_GREEN["蓝绿部署<br/>Blue-Green<br/>✅ 秒级回滚"]
    Q3 -->|否| ROLLING["滚动更新<br/>Rolling Update<br/>⚡ 资源友好"]

    Q4{是否需要风险控制?}
    Q4 -->|是| Q5
    Q4 -->|否| ROLLING

    Q5{是否有完整监控?}
    Q5 -->|是| CANARY["金丝雀发布<br/>Canary<br/>🎯 渐进式风险"]
    Q5 -->|否| ROLLING

    Q5 -->|需要 A/B 测试| AB_TEST["A/B 测试<br/>🧪 数据驱动"]
    Q5 -->|需要性能对比| SHADOW["影子流量<br/>Shadow<br/>📊 无风险对比"]

    CANARY --> Q6{发布成功?}
    Q6 -->|是| FULL["全量发布"]
    Q6 -->|否| ROLLBACK["自动回滚"]

    style START fill:#bbdefb
    style BLUE_GREEN fill:#c8e6c9
    style CANARY fill:#ffe0b2
    style ROLLING fill:#fff8e1
    style RECREATE fill:#ffebee
    style ROLLBACK fill:#ffebee
```

#<!-- chunk: 8.2 模式对比矩阵 -->## 8.2 模式对比矩阵

| 模式 | 零停机 | 快速回滚 | 资源需求 | 复杂度 | 适用场景 |
|:---|:---:|:---:|:---:|:---:|:---|
| **重建 (Recreate)** | ❌ | ❌ | 低 | ⭐ | 开发环境、允许停机 |
| **滚动更新 (Rolling)** | ✅ | ⚠️ 慢 | 中 | ⭐⭐ | 标准生产发布 |
| **蓝绿 (Blue-Green)** | ✅ | ✅ 秒级 | 高 (2x) | ⭐⭐⭐ | 核心业务、需要秒级回滚 |
| **金丝雀 (Canary)** | ✅ | ✅ 自动 | 中 | ⭐⭐⭐⭐ | 风险控制、渐进发布 |
| **A/B 测试** | ✅ | ✅ | 中 | ⭐⭐⭐⭐ | 数据验证、用户实验 |
| **影子流量** | ✅ | N/A | 高 (2x) | ⭐⭐⭐⭐⭐ | 性能基准测试 |

---

<!-- chunk: 附录：生产环境部署检查清单 -->## 附录：生产环境部署检查清单

```bash
#!/bin/bash
# production-deployment-checklist.sh

APP="$1"
NAMESPACE="${2:-default}"

echo "=== ${APP} 生产环境部署检查清单 ==="

check() {
    local name="$1"
    local cmd="$2"
    echo -n "[ ] ${name}... "
    if eval "$cmd" >/dev/null 2>&1; then
        echo "✅ PASS"
        return 0
    else
        echo "❌ FAIL"
        return 1
    fi
}

# 1. 资源限制
check "CPU/Memory Limits 已设置" \
    "kubectl get deploy/${APP} -n ${NAMESPACE} -o json | jq -e '.spec.template.spec.containers[].resources.limits'

# 2. 健康检查
check "Liveness Probe 已配置" \
    "kubectl get deploy/${APP} -n ${NAMESPACE} -o json | jq -e '.spec.template.spec.containers[].livenessProbe'

check "Readiness Probe 已配置" \
    "kubectl get deploy/${APP} -n ${NAMESPACE} -o json | jq -e '.spec.template.spec.containers[].readinessProbe'

# 3. 安全上下文
check "SecurityContext 已配置" \
    "kubectl get deploy/${APP} -n ${NAMESPACE} -o json | jq -e '.spec.template.spec.securityContext'

# 4. PDB
check "PodDisruptionBudget 已创建" \
    "kubectl get pdb -n ${NAMESPACE} | grep ${APP}"

# 5. HPA
check "HorizontalPodAutoscaler 已创建" \
    "kubectl get hpa -n ${NAMESPACE} | grep ${APP}"

# 6. NetworkPolicy
check "NetworkPolicy 已创建" \
    "kubectl get networkpolicy -n ${NAMESPACE} | grep ${APP}"

# 7. 镜像签名
check "镜像使用具体标签 (非 latest)" \
    "kubectl get deploy/${APP} -n ${NAMESPACE} -o json | jq -r '.spec.template.spec.containers[].image' | grep -v ':latest'"

# 8. 资源配额
check "命名空间有 ResourceQuota" \
    "kubectl get resourcequota -n ${NAMESPACE}"

echo "=== 检查完成 ==="
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [Flagger 文档](https://flagger.app/)
- [Argo Rollouts](https://argoproj.github.io/argo-rollouts/)
- [Istio 流量管理](https://istio.io/latest/docs/concepts/traffic-management/)
- [Kubernetes Deployment 策略](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/#strategy)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-11-production-operations/MOC.md|domain-11-production-operations MOC]]
- [[domain-11-production-operations/README.md|Domain 17: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- [[domain-11-production-operations/00-open-source-projects-index.md|Domain-18 生产运维 — 开源项目索引]]
- [[domain-01-cluster-fundamentals/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- [[domain-01-cluster-fundamentals/02-multi-cloud-hybrid-deployment-strategy.md|02-多云混合部署策略]]
- [[domain-01-cluster-fundamentals/03-edge-computing-production-deployment.md|03-边缘计算生产部署]]
- [[domain-06-observability/04-enterprise-monitoring-system.md|04-企业级监控体系]]
- [[domain-06-observability/05-logging-collection-analysis-platform.md|05-日志收集分析平台]]
- [[domain-06-observability/06-apm-application-performance-monitoring.md|06-APM应用性能监控]]
- [[domain-05-security-compliance/07-zero-trust-security-architecture.md|07-零信任安全架构]]
- [[domain-05-security-compliance/08-cis-benchmark-compliance-audit.md|08-CIS基准合规检查]]
- [[domain-05-security-compliance/09-software-bill-of-materials.md|09-软件物料清单]]

## Related

- [[domain-20-application-patterns/20-microservice-governance-architecture.md|20-microservice-governance-architecture]]
- [[domain-20-application-patterns/45-smart-port-shipping.md|45-smart-port-shipping]]
- [[domain-20-application-patterns/65-autonomous-driving-sim.md|65-autonomous-driving-sim]]
- [[domain-20-application-patterns/19-cloudnative-devops-architecture.md|19-cloudnative-devops-architecture]]
- [[domain-20-application-patterns/84-national-park.md|84-national-park]]
- [[domain-20-application-patterns/83-cultural-digitization.md|83-cultural-digitization]]
- [[domain-20-application-patterns/94-smart-prison.md|94-smart-prison]]
- [[domain-20-application-patterns/30-hrtech-saas.md|30-hrtech-saas]]
- [[domain-20-application-patterns/68-quantum-computing-cloud.md|68-quantum-computing-cloud]]
- [[domain-20-application-patterns/64-ai-drug-discovery.md|64-ai-drug-discovery]]
- [[domain-20-application-patterns/91-urban-air-mobility.md|91-urban-air-mobility]]
- [[domain-20-application-patterns/21-cross-border-ecommerce.md|21-cross-border-ecommerce]]
- [[domain-20-application-patterns/69-6g-core-network.md|69-6g-core-network]]
- [[domain-20-application-patterns/71-smart-tax.md|71-smart-tax]]
- [[domain-20-application-patterns/03-cms-architecture.md|03-cms-architecture]]
- [[domain-20-application-patterns/85-hydrogen-energy.md|85-hydrogen-energy]]
- [[domain-20-application-patterns/18-data-midplatform-architecture.md|18-data-midplatform-architecture]]
- [[domain-20-application-patterns/16-video-shortform-architecture.md|16-video-shortform-architecture]]
- [[domain-20-application-patterns/55-crossborder-dtc.md|55-crossborder-dtc]]
- [[domain-20-application-patterns/27-hospitality-tourism.md|27-hospitality-tourism]]
- [[domain-20-application-patterns/40-cloud-gaming.md|40-cloud-gaming]]
- [[domain-20-application-patterns/87-flexible-manufacturing.md|87-flexible-manufacturing]]
- [[domain-20-application-patterns/34-sportstech.md|34-sportstech]]
- [[domain-20-application-patterns/93-digital-twin-factory.md|93-digital-twin-factory]]
- [[domain-20-application-patterns/28-proptech.md|28-proptech]]
- [[domain-20-application-patterns/09-gaming-backend-architecture.md|09-gaming-backend-architecture]]
- [[domain-20-application-patterns/59-industrial-internet-platform.md|59-industrial-internet-platform]]
- [[domain-20-application-patterns/54-social-gaming-metaverse.md|54-social-gaming-metaverse]]
- [[domain-20-application-patterns/31-instant-retail.md|31-instant-retail]]
- [[domain-20-application-patterns/22-nev-connected-vehicle.md|22-nev-connected-vehicle]]
- [[domain-20-application-patterns/33-crossborder-warehouse.md|33-crossborder-warehouse]]
- [[domain-20-application-patterns/05-online-education-architecture.md|05-online-education-architecture]]
- [[domain-20-application-patterns/70-ecny-cbdc.md|70-ecny-cbdc]]
- [[domain-20-application-patterns/62-distributed-energy.md|62-distributed-energy]]
- [[domain-20-application-patterns/75-affective-computing.md|75-affective-computing]]
- [[domain-20-application-patterns/50-unmanned-retail.md|50-unmanned-retail]]
- [[domain-20-application-patterns/77-fusion-energy-monitoring.md|77-fusion-energy-monitoring]]
- [[domain-20-application-patterns/42-secondhand-circular.md|42-secondhand-circular]]
- [[domain-20-application-patterns/79-polar-research.md|79-polar-research]]
- [[domain-20-application-patterns/26-aviation-travel.md|26-aviation-travel]]
- [[domain-20-application-patterns/80-tsn-network.md|80-tsn-network]]
- [[domain-20-application-patterns/43-enterprise-im.md|43-enterprise-im]]
- [[domain-20-application-patterns/73-smart-firefighting.md|73-smart-firefighting]]
- [[domain-20-application-patterns/14-smart-healthcare-architecture.md|14-smart-healthcare-architecture]]
- [[domain-20-application-patterns/96-carbon-capture.md|96-carbon-capture]]
- [[domain-20-application-patterns/60-v2x-autonomous-driving.md|60-v2x-autonomous-driving]]
- [[domain-20-application-patterns/74-immersive-xr.md|74-immersive-xr]]
- [[domain-20-application-patterns/78-deep-sea-exploration.md|78-deep-sea-exploration]]
- [[domain-20-application-patterns/12-smart-logistics-architecture.md|12-smart-logistics-architecture]]
- [[domain-20-application-patterns/51-smart-manufacturing-mes.md|51-smart-manufacturing-mes]]
- [[domain-20-application-patterns/08-ai-ml-inference-architecture.md|08-ai-ml-inference-architecture]]
- [[domain-20-application-patterns/23-xinchuang-it-innovation.md|23-xinchuang-it-innovation]]
- [[domain-20-application-patterns/47-smart-mining.md|47-smart-mining]]
- [[domain-20-application-patterns/58-web3-gamefi.md|58-web3-gamefi]]
- [[domain-20-application-patterns/29-agritech-iot.md|29-agritech-iot]]
- [[domain-20-application-patterns/57-digital-therapeutics.md|57-digital-therapeutics]]
- [[domain-20-application-patterns/92-smart-sports-venue.md|92-smart-sports-venue]]
- [[domain-20-application-patterns/76-synthetic-biology.md|76-synthetic-biology]]
- [[domain-20-application-patterns/61-smart-grid.md|61-smart-grid]]
- [[domain-20-application-patterns/17-saas-multitenant-architecture.md|17-saas-multitenant-architecture]]
- [[domain-20-application-patterns/11-smart-retail-architecture.md|11-smart-retail-architecture]]
- [[domain-20-application-patterns/25-quantitative-trading.md|25-quantitative-trading]]
- [[domain-20-application-patterns/81-smart-customs.md|81-smart-customs]]
- [[domain-20-application-patterns/24-insurtech.md|24-insurtech]]
- [[domain-20-application-patterns/90-neuromorphic-computing.md|90-neuromorphic-computing]]
- [[domain-20-application-patterns/46-satellite-internet.md|46-satellite-internet]]
- [[domain-20-application-patterns/52-smart-water.md|52-smart-water]]
- [[domain-20-application-patterns/86-solid-state-battery.md|86-solid-state-battery]]
- [[domain-20-application-patterns/67-brain-computer-interface.md|67-brain-computer-interface]]
- [[domain-20-application-patterns/82-legaltech.md|82-legaltech]]
- [[domain-20-application-patterns/15-energy-power-architecture.md|15-energy-power-architecture]]
- [[domain-20-application-patterns/37-pet-economy.md|37-pet-economy]]
- [[domain-20-application-patterns/49-livestream-ecommerce.md|49-livestream-ecommerce]]
- [[domain-20-application-patterns/66-space-internet.md|66-space-internet]]
- [[domain-20-application-patterns/06-fintech-architecture.md|06-fintech-architecture]]
- [[domain-20-application-patterns/88-nanomaterials.md|88-nanomaterials]]
- [[domain-20-application-patterns/10-social-media-architecture.md|10-social-media-architecture]]
- [[domain-20-application-patterns/39-smart-campus.md|39-smart-campus]]
- [[domain-20-application-patterns/13-digital-government-architecture.md|13-digital-government-architecture]]
- [[domain-20-application-patterns/48-vocational-edtech.md|48-vocational-edtech]]
- [[domain-20-application-patterns/72-digital-twin-city.md|72-digital-twin-city]]
- [[domain-20-application-patterns/32-smart-restaurant.md|32-smart-restaurant]]
- [[domain-20-application-patterns/89-crispr-gene-editing.md|89-crispr-gene-editing]]
- [[domain-20-application-patterns/56-smart-elderly-care.md|56-smart-elderly-care]]
- [[domain-20-application-patterns/44-martech-adtech.md|44-martech-adtech]]
- [[domain-20-application-patterns/95-industrial-metaverse.md|95-industrial-metaverse]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]

## See Also

- [[domain-06-observability/99-karpenter-node-autoscaling-guide.md|99-karpenter-node-autoscaling-guide]]
- [[domain-06-observability/99-keda-event-driven-autoscaling-guide.md|99-keda-event-driven-autoscaling-guide]]
- [[domain-01-cluster-fundamentals/99-kubernetes-multi-tenant-architecture.md|99-kubernetes-multi-tenant-architecture]]
- [[domain-01-cluster-fundamentals/99-kubernetes-production-architecture-blueprint.md|99-kubernetes-production-architecture-blueprint]]
