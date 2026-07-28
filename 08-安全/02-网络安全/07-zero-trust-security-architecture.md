---
title: 07-零信任安全架构
description: 'title: 07-零信任安全架构'
summary: 'title: 07-零信任安全架构'
category: general
tags:
- k8s
- production
- best-practice
- security
- architecture
- etcd
- istio
- docker
- opa
- falco
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 07-zero-trust-security-architecture的安全加固怎么做？
- 07-zero-trust-security-architecture的安全最佳实践
- 07-zero-trust-security-architecture有哪些安全风险？
trigger_keywords:
- 零信任安全架构
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- service-mesh-basics
- etcd-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 07-零信任安全架构
description: '# 07-零信任安全架构'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- [[etcd|etcd]]
- [[istio|istio]]
- docker
- opa
- falco
- postgresql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- 零信任安全架构 是什么
- 如何 零信任安全架构
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- 零信任安全架构
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

# 07-零信任安全架构

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 📋 概述 -->## 📋 概述

零信任安全架构是现代企业安全防护的核心理念。本文档详细介绍在Kubernetes环境中实施零信任安全策略的方法和最佳实践。

<!-- chunk: 🔐 零信任核心原则 -->## 🔐 零信任核心原则

## 身份验证与授权

## 1. 统一身份认证平台
```yaml
# Keycloak身份认证配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: keycloak
  namespace: security
spec:
  replicas: 3
  selector:
    matchLabels:
      app: keycloak
  template:
    metadata:
      labels:
        app: keycloak
    spec:
      containers:
      - name: keycloak
        image: quay.io/keycloak/keycloak:21.1.1
        env:
        - name: KEYCLOAK_ADMIN
          value: "admin"
        - name: KEYCLOAK_ADMIN_PASSWORD
          valueFrom:
            secretKeyRef:
              name: keycloak-admin-password
              key: password
        - name: KC_DB
          value: "postgres"
        - name: KC_DB_URL
          value: "jdbc:postgresql://postgresql:5432/keycloak"
        - name: KC_DB_USERNAME
          value: "keycloak"
        - name: KC_DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: keycloak-db-password
              key: password
        - name: KC_HOSTNAME
          value: "keycloak.example.com"
        - name: KC_HTTP_ENABLED
          value: "true"
        - name: KC_PROXY
          value: "edge"
        ports:
        - containerPort: 8080
        readinessProbe:
          httpGet:
            path: /realms/master
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
```

## 2. 多因素认证配置
```yaml
# MFA策略配置
apiVersion: keycloak.org/v1alpha1
kind: KeycloakRealm
metadata:
  name: production-realm
  namespace: security
spec:
  realm:
    id: production
    realm: production
    enabled: true
    registrationAllowed: false
    registrationEmailAsUsername: false
    rememberMe: false
    verifyEmail: true
    loginWithEmailAllowed: true
    duplicateEmailsAllowed: false
    resetPasswordAllowed: true
    editUsernameAllowed: false
    sslRequired: external
    bruteForceProtected: true
    permanentLockout: false
    maxFailureWaitSeconds: 900
    minimumQuickLoginWaitSeconds: 60
    waitIncrementSeconds: 60
    quickLoginCheckMilliSeconds: 1000
    maxDeltaTimeSeconds: 43200
    failureFactor: 30
```

## 动态访问控制

## 1. OPA Gatekeeper策略
```yaml
# OPA Gatekeeper约束模板
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredlabels
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredLabels
      validation:
        openAPIV3Schema:
          properties:
            labels:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiredlabels
        
        violation[{"msg": msg, "details": {"missing_labels": missing}}] {
          provided := {label | input.review.object.metadata.labels[label]}
          required := {label | label := input.parameters.labels[_]}
          missing := required - provided
          count(missing) > 0
          msg := sprintf("you must provide labels: %v", [missing])
        }
---
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: pod-must-have-owner
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
  parameters:
    labels: ["owner", "environment", "cost-center"]
```

## 2. 精细化RBAC控制
```yaml
# 基于角色的访问控制
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: developer-restricted
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-access
  namespace: development
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: developer-restricted
  apiGroup: rbac.authorization.k8s.io
```

<!-- chunk: 🛡️ 网络安全防护 -->## 🛡️ 网络安全防护

## 微分段网络策略

## 1. 层次化网络隔离
```yaml
# 应用层网络策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: app-tier-isolation
spec:
  podSelector:
    matchLabels:
      tier: application
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: frontend
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 5432
---
# 数据库层网络策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: database-tier-isolation
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: backend
    - namespaceSelector:
        matchLabels:
          name: analytics
    ports:
    - protocol: TCP
      port: 5432
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: TCP
      port: 53
```

## 2. 服务网格安全
```yaml
# Istio安全策略
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: frontend-policy
  namespace: frontend
spec:
  selector:
    matchLabels:
      app: frontend
  action: ALLOW
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/backend/sa/backend-service-account"]
    to:
    - operation:
        methods: ["GET", "POST"]
        paths: ["/api/*"]
  - from:
    - source:
        principals: ["cluster.local/ns/mobile/sa/mobile-app"]
    to:
    - operation:
        methods: ["GET"]
        paths: ["/public/*"]
---
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
```

<!-- chunk: 🔑 密钥管理 -->## 🔑 密钥管理

## HashiCorp Vault集成

## 1. Vault Operator部署
```yaml
# Vault集群配置
apiVersion: vault.banzaicloud.com/v1alpha1
kind: Vault
metadata:
  name: vault
  namespace: security
spec:
  size: 3
  image: vault:1.12.0
  bankVaultsImage: ghcr.io/bank-vaults/bank-vaults:latest
  statsdDisabled: true
  serviceType: ClusterIP
  serviceAccount: vault
  config:
    storage:
      raft:
        path: /vault/data
    listener:
      tcp:
        address: "[::]:8200"
        tls_disable: true
    api_addr: https://vault:8200
    cluster_addr: "https://vault:8201"
    ui: true
  externalConfig:
    policies:
    - name: kubernetes-reader
      rules: path "secret/data/kubernetes/*" {
              capabilities = ["read"]
            }
    auth:
    - type: kubernetes
      config:
        kubernetes_host: https://kubernetes.default.svc
      roles:
      - name: app-role
        bound_service_account_names: ["default"]
        bound_service_account_namespaces: ["production"]
        policies: ["kubernetes-reader"]
        ttl: 1h
```

## 2. 应用密钥注入
```yaml
# Vault Agent Injector配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
spec:
  template:
    metadata:
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/agent-inject-secret-database-config: "secret/data/database/prod"
        vault.hashicorp.com/agent-inject-template-database-config: |
          {
            "username": "{{ .Data.username }}",
            "password": "{{ .Data.password }}",
            "host": "{{ .Data.host }}",
            "port": "{{ .Data.port }}"
          }
        vault.hashicorp.com/role: "app-role"
    spec:
      serviceAccountName: app-service-account
      containers:
      - name: app
        image: secure-app:latest
        env:
        - name: DATABASE_CONFIG_PATH
          value: "/vault/secrets/database-config"
```

<!-- chunk: 🕵️ 威胁检测与响应 -->## 🕵️ 威胁检测与响应

## 运行时安全监控

## 1. Falco入侵检测
```yaml
# Falco配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
  namespace: security
spec:
  selector:
    matchLabels:
      app: falco
  template:
    metadata:
      labels:
        app: falco
    spec:
      containers:
      - name: falco
        image: falcosecurity/falco-no-driver:0.34.1
        securityContext:
          privileged: true
        env:
        - name: FALCO_FRONTEND
          value: "noninteractive"
        - name: SKIP_DRIVER_LOADER
          value: "true"
        volumeMounts:
        - mountPath: /host/var/run/docker.sock
          name: docker-socket
        - mountPath: /host/root
          name: rootfs
          readOnly: true
        - mountPath: /host/boot
          name: boot-fs
          readOnly: true
        - mountPath: /host/lib/modules
          name: lib-modules
          readOnly: true
        - mountPath: /host/usr
          name: usr-fs
          readOnly: true
        - mountPath: /host/etc
          name: etc-fs
          readOnly: true
        - mountPath: /host/proc
          name: proc-fs
          readOnly: true
      volumes:
      - name: docker-socket
        hostPath:
          path: /var/run/docker.sock
      - name: rootfs
        hostPath:
          path: /
      - name: boot-fs
        hostPath:
          path: /boot
      - name: lib-modules
        hostPath:
          path: /lib/modules
      - name: usr-fs
        hostPath:
          path: /usr
      - name: etc-fs
        hostPath:
          path: /etc
      - name: proc-fs
        hostPath:
          path: /proc
```

## 2. 自定义威胁规则
```yaml
# Falco自定义规则
- rule: Detect crypto miners
  desc: Detection of crypto mining activity
  condition: >
    spawned_process and proc.name in (miner, xmrig, ccminer) 
    or proc.cmdline contains "stratum+tcp"
  output: >
    Crypto miner detected (user=%user.name command=%proc.cmdline)
  priority: CRITICAL
  tags: [process, mitre_execution]

- rule: Suspicious network connection
  desc: Outbound connection to known malicious IPs
  condition: >
    outbound and fd.sip in (192.168.1.100, 10.0.0.50)
    and not proc.name in (curl, wget, kubectl)
  output: >
    Suspicious outbound connection (command=%proc.cmdline ip=%fd.sip)
  priority: WARNING
  tags: [network, mitre_exfiltration]
```

## 安全事件响应

## 1. 自动化响应机制
```yaml
# 安全事件响应工作流
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: security-incident-response
  namespace: security
spec:
  entrypoint: incident-handler
  templates:
  - name: incident-handler
    steps:
    - - name: assess-threat
        template: threat-assessment
    - - name: isolate-affected
        template: isolate-workload
        when: "{{steps.assess-threat.outputs.result}} == high"
    - - name: collect-evidence
        template: forensic-collection
    - - name: notify-stakeholders
        template: incident-notification
---
apiVersion: batch/v1
kind: Job
metadata:
  name: isolate-workload
  namespace: security
spec:
  template:
    spec:
      containers:
      - name: isolator
        image: security-tools:latest
        command:
        - /bin/sh
        - -c
        - |
          # 隔离受影响的工作负载
          kubectl cordon $NODE_NAME
          kubectl drain $NODE_NAME --ignore-daemonsets --delete-emptydir-data
          kubectl apply -f restrictive-network-policy.yaml
        env:
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
      restartPolicy: Never
```

<!-- chunk: 🔍 合规与审计 -->## 🔍 合规与审计

## 安全基线检查

## 1. CIS基准自动化检查
```yaml
# kube-bench配置
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cis-benchmark
  namespace: security
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          hostPID: true
          containers:
          - name: kube-bench
            image: aquasec/kube-bench:latest
            command: ["kube-bench", "run", "--targets", "master,node,etcd"]
            volumeMounts:
            - name: var-lib-etcd
              mountPath: /var/lib/etcd
              readOnly: true
            - name: etc-kubernetes
              mountPath: /etc/kubernetes
              readOnly: true
            - name: etc-systemd
              mountPath: /etc/systemd
              readOnly: true
          volumes:
          - name: var-lib-etcd
            hostPath:
              path: "/var/lib/etcd"
          - name: etc-kubernetes
            hostPath:
              path: "/etc/kubernetes"
          - name: etc-systemd
            hostPath:
              path: "/etc/systemd"
          restartPolicy: OnFailure
```

## 2. 安全配置审计
```yaml
# Kubernetes安全审计配置
apiVersion: audit.k8s.io/v1
kind: Policy
metadata:
  name: security-audit-policy
rules:
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]
  verbs: ["create", "update", "delete"]
  
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  verbs: ["create", "update", "delete"]
  
- level: Request
  resources:
  - group: "networking.k8s.io"
    resources: ["networkpolicies"]
  verbs: ["create", "update", "delete"]
  
- level: None
  users: ["system:kube-proxy"]
  verbs: ["watch"]
```

<!-- chunk: 🛠️ 安全工具链 -->## 🛠️ 安全工具链

## 漏洞扫描集成

## 1. Trivy镜像扫描
```yaml
# Trivy扫描CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: trivy-scan
  namespace: security
spec:
  schedule: "0 1 * * *"  # 每天凌晨1点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: trivy
            image: aquasec/trivy:latest
            command:
            - trivy
            - image
            - --severity
            - HIGH,CRITICAL
            - --format
            - json
            - --output
            - /tmp/results.json
            - $(IMAGE_TO_SCAN)
            env:
            - name: IMAGE_TO_SCAN
              value: "myapp:latest"
            volumeMounts:
            - name: tmp
              mountPath: /tmp
          volumes:
          - name: tmp
            emptyDir: {}
          restartPolicy: OnFailure
```

## 2. 运行时安全扫描
```yaml
# Clair配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: clair
  namespace: security
spec:
  replicas: 1
  selector:
    matchLabels:
      app: clair
  template:
    metadata:
      labels:
        app: clair
    spec:
      containers:
      - name: clair
        image: quay.io/coreos/clair:latest
        ports:
        - containerPort: 6060
        - containerPort: 6061
        env:
        - name: CLAIR_CONF
          value: /clair/config.yaml
        volumeMounts:
        - name: config
          mountPath: /clair
      volumes:
      - name: config
        configMap:
          name: clair-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: clair-config
  namespace: security
data:
  config.yaml: |
    clair:
      database:
        type: pgsql
        options:
          source: host=postgresql port=5432 user=clair dbname=clair sslmode=disable
      api:
        port: 6060
        healthport: 6061
        timeout: 900s
```

<!-- chunk: 🔧 实施检查清单 -->## 🔧 实施检查清单

## 安全架构部署
- [ ] 部署统一身份认证平台
- [ ] 实施多因素认证机制
- [ ] 配置动态访问控制策略
- [ ] 建立微分段网络隔离
- [ ] 集成密钥管理系统
- [ ] 部署运行时安全监控

## 威胁防护体系
- [ ] 实施入侵检测和预防系统
- [ ] 建立安全事件响应机制
- [ ] 配置自动化威胁响应
- [ ] 实施持续安全监控
- [ ] 建立威胁情报集成
- [ ] 配置安全日志分析

## 合规与审计
- [ ] 实施安全基线自动化检查
- [ ] 配置安全审计日志收集
- [ ] 建立合规性监控体系
- [ ] 实施漏洞扫描和管理
- [ ] 建立安全态势感知
- [ ] 维护安全文档和报告

---

*本文档为企业级零信任安全架构提供全面的设计方案和实施指导*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生产运维 MOC
- [[13-生产运维/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[01-集群基础/02-设计原则/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 08-CIS基准合规检查
- 09-软件物料清单
- 10-GitOps流水线实践

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

- [[08-安全/README.md|返回目录]]- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]

## See Also

- 05-logging-collection-analysis-platform
- 06-apm-application-performance-monitoring
- 08-cis-benchmark-compliance-audit
- 09-software-bill-of-materials

## 相关合成分析

- [[22-概念/05-安全/service-mesh-zero-trust-security.md|Service Mesh 零信任安全架构]]



<!-- risk-assessed -->
