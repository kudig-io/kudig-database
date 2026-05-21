---
title: 07-零信任安全架构
description: 'title: 07-零信任安全架构'
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

title: 07-零信任安全架构
description: '# 07-零信任安全架构'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- etcd
- istio
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

#<!-- chunk: 身份验证与授权 -->## 身份验证与授权

##<!-- chunk: 1. 统一身份认证平台 -->## 1. 统一身份认证平台
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

##<!-- chunk: 2. 多因素认证配置 -->## 2. 多因素认证配置
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

#<!-- chunk: 动态访问控制 -->## 动态访问控制

##<!-- chunk: 1. OPA Gatekeeper策略 -->## 1. OPA Gatekeeper策略
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

##<!-- chunk: 2. 精细化RBAC控制 -->## 2. 精细化RBAC控制
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

#<!-- chunk: 微分段网络策略 -->## 微分段网络策略

##<!-- chunk: 1. 层次化网络隔离 -->## 1. 层次化网络隔离
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

##<!-- chunk: 2. 服务网格安全 -->## 2. 服务网格安全
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

#<!-- chunk: HashiCorp Vault集成 -->## HashiCorp Vault集成

##<!-- chunk: 1. Vault Operator部署 -->## 1. Vault Operator部署
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

##<!-- chunk: 2. 应用密钥注入 -->## 2. 应用密钥注入
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

#<!-- chunk: 运行时安全监控 -->## 运行时安全监控

##<!-- chunk: 1. Falco入侵检测 -->## 1. Falco入侵检测
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

##<!-- chunk: 2. 自定义威胁规则 -->## 2. 自定义威胁规则
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

#<!-- chunk: 安全事件响应 -->## 安全事件响应

##<!-- chunk: 1. 自动化响应机制 -->## 1. 自动化响应机制
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

#<!-- chunk: 安全基线检查 -->## 安全基线检查

##<!-- chunk: 1. CIS基准自动化检查 -->## 1. CIS基准自动化检查
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

##<!-- chunk: 2. 安全配置审计 -->## 2. 安全配置审计
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

#<!-- chunk: 漏洞扫描集成 -->## 漏洞扫描集成

##<!-- chunk: 1. Trivy镜像扫描 -->## 1. Trivy镜像扫描
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

##<!-- chunk: 2. 运行时安全扫描 -->## 2. 运行时安全扫描
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

#<!-- chunk: 安全架构部署 -->## 安全架构部署
- [ ] 部署统一身份认证平台
- [ ] 实施多因素认证机制
- [ ] 配置动态访问控制策略
- [ ] 建立微分段网络隔离
- [ ] 集成密钥管理系统
- [ ] 部署运行时安全监控

#<!-- chunk: 威胁防护体系 -->## 威胁防护体系
- [ ] 实施入侵检测和预防系统
- [ ] 建立安全事件响应机制
- [ ] 配置自动化威胁响应
- [ ] 实施持续安全监控
- [ ] 建立威胁情报集成
- [ ] 配置安全日志分析

#<!-- chunk: 合规与审计 -->## 合规与审计
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

- [[domain-11-production-operations/MOC.md|domain-11-production-operations MOC]]
- [[domain-11-production-operations/README.md|Domain 17: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- [[domain-11-production-operations/00-open-source-projects-index.md|Domain-18 生产运维 — 开源项目索引]]
- [[domain-01-cluster-fundamentals/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- [[domain-01-cluster-fundamentals/02-multi-cloud-hybrid-deployment-strategy.md|02-多云混合部署策略]]
- [[domain-01-cluster-fundamentals/03-edge-computing-production-deployment.md|03-边缘计算生产部署]]
- [[domain-06-observability/04-enterprise-monitoring-system.md|04-企业级监控体系]]
- [[domain-06-observability/05-logging-collection-analysis-platform.md|05-日志收集分析平台]]
- [[domain-06-observability/06-apm-application-performance-monitoring.md|06-APM应用性能监控]]
- [[domain-05-security-compliance/08-cis-benchmark-compliance-audit.md|08-CIS基准合规检查]]
- [[domain-05-security-compliance/09-software-bill-of-materials.md|09-软件物料清单]]
- [[domain-08-release-change-management/10-gitops-pipeline-practices.md|10-GitOps流水线实践]]

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

- [[domain-05-security-compliance/README.md|返回目录]]- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]

## See Also

- [[domain-06-observability/05-logging-collection-analysis-platform.md|05-logging-collection-analysis-platform]]
- [[domain-06-observability/06-apm-application-performance-monitoring.md|06-apm-application-performance-monitoring]]
- [[domain-05-security-compliance/08-cis-benchmark-compliance-audit.md|08-cis-benchmark-compliance-audit]]
- [[domain-05-security-compliance/09-software-bill-of-materials.md|09-software-bill-of-materials]]
