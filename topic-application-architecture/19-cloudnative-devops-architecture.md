# 云原生 DevOps 平台 Kubernetes 生产架构设计

> **适用场景**: 企业级 DevOps 平台 / GitOps / 持续交付 / 平台工程 (Platform Engineering) / IDP 内部开发者平台  
> **云厂商**: 阿里云 ACK + 云效 / MSE / ARMS 产品体系  
> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **目标读者**: DevOps 架构师、平台工程师、SRE、阿里云解决方案架构师

---

## 📋 目录

- [一、整体架构全景](#一整体架构全景)
- [二、GitOps 交付流水线架构](#二gitops-交付流水线架构)
- [三、多环境晋升架构](#三多环境晋升架构)
- [四、可观测性驱动发布架构](#四可观测性驱动发布架构)
- [五、安全供应链 (SLSA) 架构](#五安全供应链-slsa-架构)
- [六、平台工程 (IDP) 架构](#六平台工程-idp-架构)
- [七、成本治理与 FinOps 架构](#七成本治理与-finops-架构)
- [八、ACK 阿里云部署架构](#八ack-阿里云部署架构)

---

## 一、整体架构全景

```mermaid
flowchart TB
    subgraph Dev["开发者"]
        IDE_DEV["IDE<br/>VSCode/JetBrains"]
        CLI_DEV["CLI<br/>kubectl/helm"]
        PORTAL_DEV["开发者门户<br/>Backstage"]
    end

    subgraph Code["代码层"]
        GIT["Git 仓库<br/>GitHub/GitLab"]
        MR["Merge Request<br/>Code Review"]
        SCAN["代码扫描<br">SonarQube/SAST"]
    end

    subgraph CI["持续集成"]
        BUILD["镜像构建<br">Kaniko/BuildKit"]
        TEST["测试<br">单元/集成/E2E"]
        SIGN["镜像签名<br">cosign/notation"]
        PUSH["推送镜像<br">ACR 企业版"]
    end

    subgraph CD["持续交付"]
        ARGO_CD["Argo CD<br">GitOps 同步"]
        FLAGGER_CD["Flagger<br">渐进发布"]
        SECRET_OPS["External Secrets<br">Vault/KMS"]
    end

    subgraph Runtime["运行时"]
        ACK_DEV["ACK 开发集群"]
        ACK_STG["ACK 预发集群"]
        ACK_PROD["ACK 生产集群"]
    end

    subgraph ObservabilityDevOps["可观测性"]
        TRACE_DEV["链路追踪<br">ARMS/SkyWalking"]
        METRIC_DEV["指标<br">Prometheus/ARMS"]
        LOG_DEV["日志<br">SLS/Loki"]
        PROFILING["持续剖析<br">ARMS  Profiler"]
    end

    Dev --> Code --> CI --> CD --> Runtime
    CD --> ObservabilityDevOps
    Runtime --> ObservabilityDevOps

    style CI fill:#e3f2fd
    style CD fill:#fff8e1
    style ObservabilityDevOps fill:#e8f5e9
```

### 阿里云产品映射

| 架构层 | 阿里云方案 | 开源替代 |
|:---|:---|:---|
| 代码仓库 | **云效 Codeup** | GitLab / GitHub |
| CI/CD | **云效流水线** / **ACK + Argo** | Jenkins / Tekton |
| 镜像仓库 | **ACR 企业版** | Harbor |
| GitOps | **ACK + Argo CD** | Argo CD / Flux |
| 制品管理 | **云效制品库** | Nexus / Artifactory |
| 测试 | **云效测试管理** | SonarQube |
| 可观测性 | **ARMS** + **SLS** | Prometheus + Grafana + Loki |
| 安全 | **云安全中心** + **ACR 镜像扫描** | Trivy / Falco |

---

## 二、GitOps 交付流水线架构

```mermaid
flowchart TB
    subgraph GitRepo["Git 仓库 (Single Source of Truth)"]
        APP_CODE["应用代码"]
        CHARTS["Helm Charts"]
        KUSTOMIZE["Kustomize Overlays"]
        POLICIES["OPA Policies"]
    end

    subgraph CIPipeline["CI 流水线"]
        BUILD_IMG["构建镜像"]
        TEST_IMG["测试"]
        SCAN_IMG["安全扫描"]
        PUSH_ACR["推送到 ACR"]
    end

    subgraph GitOpsEngine["GitOps 引擎"]
        ARGO["Argo CD"]
        FLUX_CD["Flux CD"]
        SEALED_SECRETS["Sealed Secrets"]
    end

    subgraph Clusters["目标集群"]
        DEV_CLUSTER["开发集群"]
        STAGING_CLUSTER["预发集群"]
        PROD_CLUSTER["生产集群"]
    end

    GitRepo --> CIPipeline --> GitOpsEngine --> Clusters
    GitRepo -.->|直接同步| GitOpsEngine

    style CIPipeline fill:#e3f2fd
    style GitOpsEngine fill:#fff8e1
    style Clusters fill:#e8f5e9
```

### Argo CD Application 配置

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: ecommerce-prod
  namespace: argocd
  finalizers:
    - resources-finalizer.argocd.argoproj.io
spec:
  project: production
  source:
    repoURL: https://github.com/org/gitops-manifests.git
    targetRevision: main
    path: overlays/production/ecommerce
    helm:
      valueFiles:
        - values-production.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: ecommerce-prod
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
      allowEmpty: false
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
      - PruneLast=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
  revisionHistoryLimit: 10
---
# ApplicationSet 多环境/多租户批量部署
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: saas-tenants
  namespace: argocd
spec:
  generators:
    - list:
        elements:
          - tenant: tenant-a
            env: production
            replicas: 10
          - tenant: tenant-b
            env: production
            replicas: 5
          - tenant: tenant-c
            env: staging
            replicas: 2
  template:
    metadata:
      name: '{{tenant}}-saas-app'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/gitops-manifests.git
        targetRevision: main
        path: base/saas-app
        helm:
          parameters:
            - name: tenantId
              value: '{{tenant}}'
            - name: replicaCount
              value: '{{replicas}}'
      destination:
        server: https://kubernetes.default.svc
        namespace: '{{tenant}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

---

## 三、多环境晋升架构

```mermaid
flowchart LR
    subgraph DevEnv["开发环境"]
        DEV_CODE["Feature Branch"]
        DEV_TEST["单元测试"]
        DEV_PREVIEW["Preview 环境"]
    end

    subgraph StagingEnv["预发环境"]
        STG_MERGE["Merge to Main"]
        STG_INTEGRATION["集成测试"]
        STG_E2E["E2E 测试"]
        STG_PERF["性能测试"]
    end

    subgraph ProdEnv["生产环境"]
        PROD_CANARY["金丝雀 5%"]
        PROD_GRAY["灰度 50%"]
        PROD_FULL["全量 100%"]
    end

    DevEnv --> StagingEnv --> ProdEnv

    style DevEnv fill:#e3f2fd
    style StagingEnv fill:#fff8e1
    style ProdEnv fill:#c8e6c9
```

---

## 四、可观测性驱动发布架构

```mermaid
flowchart TB
    subgraph Deploy["部署触发"]
        NEW_VERSION["新版本发布"]
        CANARY_DEPLOY["金丝雀部署"]
    end

    subgraph Metrics["指标采集"]
        ERROR_RATE["错误率"]
        LATENCY_P99["P99 延迟"]
        THROUGHPUT["吞吐量"]
        CUSTOM_METRIC["业务指标"]
    end

    subgraph Analysis["自动分析"]
        BASELINE["基线对比<br">历史版本"]
        THRESHOLD["阈值检查<br">SLO"]
        ANOMALY["异常检测<br">AI 模型"]
    end

    subgraph Action["自动决策"]
        PROMOTE["自动晋升<br">指标正常"]
        ROLLBACK_AUTO["自动回滚<br">指标异常"]
        ALERT_OPS["告警人工介入<br">边界情况"]
    end

    Deploy --> Metrics --> Analysis --> Action

    style Analysis fill:#e3f2fd
    style Action fill:#e8f5e9
```

---

## 五、安全供应链 (SLSA) 架构

```mermaid
flowchart TB
    subgraph Source["源码安全"]
        SAST["SAST 扫描<br">代码漏洞"]
        DEPENDENCY["依赖检查<br">SCA"]
        LICENSE["许可证合规<br">FOSSA"]
    end

    subgraph Build["构建安全"]
        SBOM["SBOM 生成<br">物料清单"]
        SIGN_BUILD["构建签名<br">Sigstore"]
        PROVENANCE["来源证明<br">SLSA Provenance"]
    end

    subgraph Image["镜像安全"]
        SCAN_IMAGE["镜像扫描<br">Trivy/ACR 扫描"]
        SIGN_IMAGE["镜像签名<br">cosign"]
        POLICY_IMAGE["准入策略<br">Kyverno/OPA"]
    end

    subgraph RuntimeSec["运行安全"]
        RUNTIME_SCAN["运行时扫描<br">Falco"]
        VULN_DB["漏洞数据库<br">持续监控"]
    end

    Source --> Build --> Image --> RuntimeSec

    style Source fill:#e3f2fd
    style Build fill:#fff8e1
    style Image fill:#e8f5e9
```

---

## 六、平台工程 (IDP) 架构

```mermaid
flowchart TB
    subgraph Portal["开发者门户 (Backstage)"]
        CATALOG_SW["软件目录<br">服务/组件/资源"]
        TEMPLATE_SW["脚手架模板<br">快速创建"]
        DOC_SW["技术文档<br">API/架构"]
        COST_SW["成本看板<br">FinOps"]
    end

    subgraph PlatformServices["平台服务"]
        ENV_MGMT["环境管理<br">一键创建"]
        DB_MGMT["数据库自助<br">申请/扩容"]
        SECRET_MGMT["密钥管理<br">自动注入"]
        MONITORING_MGMT["监控自助<br">一键接入"]
    end

    subgraph GoldenPath["黄金路径"]
        CREATE_SERVICE["创建服务"]
        SETUP_CI["配置 CI/CD"]
        DEPLOY_AUTO["自动部署"]
        OBSERVE_AUTO["自动观测"]
    end

    Portal --> PlatformServices --> GoldenPath

    style Portal fill:#e3f2fd
    style PlatformServices fill:#fff8e1
    style GoldenPath fill:#e8f5e9
```

---

## 七、成本治理与 FinOps 架构

```mermaid
flowchart TB
    subgraph CostData["成本数据"]
        K8S_COST["K8s 资源<br">CPU/内存/GPU"]
        STORAGE_COST["存储<br">块存储/对象存储"]
        NETWORK_COST["网络<br">公网/跨区"]
        LICENSE_COST["软件许可"]
    end

    subgraph Allocation["成本分摊"]
        LABEL_COST["标签分摊<br">团队/项目/环境"]
        NAMESPACE_COST["Namespace 分摊"]
        POD_COST["Pod 级分摊"]
    end

    subgraph Optimization["成本优化"]
        RIGHT_SIZE["Right-sizing<br">资源优化"]
        SPOT["Spot 实例<br">弹性负载"]
        AUTO_SCALE["自动扩缩<br">HPA/VPA/Karpenter"]
        SCHEDULE["定时启停<br">开发/测试环境"]
    end

    CostData --> Allocation --> Optimization

    style Allocation fill:#e3f2fd
    style Optimization fill:#e8f5e9
```

---

## 八、ACK 阿里云部署架构

### 多集群 GitOps 管理

```yaml
# ACK 多集群 kubeconfig Secret
apiVersion: v1
kind: Secret
metadata:
  name: ack-clusters
  namespace: argocd
  labels:
    argocd.argoproj.io/secret-type: cluster
type: Opaque
stringData:
  name: ack-prod-hangzhou
  server: https://cluster-api.aliyuncs.com:6443
  config: |
    {
      "bearerToken": "<token>",
      "tlsClientConfig": {
        "insecure": false,
        "caData": "<base64-ca>"
      }
    }
---
# 阿里云 ARMS 应用监控接入
apiVersion: arms.aliyun.com/v1beta1
kind: ArmsApplicationMonitor
metadata:
  name: ecommerce-monitor
  namespace: production
spec:
  appName: ecommerce-order-service
  language: java
  agentVersion: "3.0"
  enable: true
  configs:
    - name: sampling_rate
      value: "10"
    - name: slow_sql_threshold
      value: "500"
---
# SLS 日志采集配置
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: app-logs
  namespace: production
spec:
  projectName: k8s-log-cluster-prod
  logstoreName: app-logs
  shardCount: 2
  lifeCycle: 30
  logtailConfig:
    inputType: file
    configName: app-logs
    inputDetail:
      logType: json_log
      logPath: /app/logs
      filePattern: "*.json.log"
      dockerFile: true
      dockerIncludeLabel:
        app: "*"
```

---

## 参考链接

- [阿里云云效](https://www.aliyun.com/product/yunxiao)
- [阿里云 ACR](https://www.aliyun.com/product/acr)
- [Argo CD 文档](https://argo-cd.readthedocs.io/)
- [Backstage 文档](https://backstage.io/docs/)
- [SLSA 框架](https://slsa.dev/)
