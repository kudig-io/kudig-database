# 21 - 多集群安全管理与联邦认证

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 专家级 | **参考**: [Kubernetes Federation v2](https://github.com/kubernetes-sigs/kubefed) | [Red Hat Advanced Cluster Management](https://www.redhat.com/en/technologies/management/advanced-cluster-management) | [Rancher Fleet](https://fleet.rancher.io/)

## 一、多集群安全架构设计

### 1.1 多集群管理架构全景

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      Multi-Cluster Security Management Architecture                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                        Central Management Plane                                │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                          Hub Cluster                                    │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   ACM       │  │   Rancher   │  │   ArgoCD    │  │   GitOps    │         │   │ │
│  │  │  │   管理中心   │  │   管理平台   │  │   多集群    │  │   配置管理   │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                      Identity Federation                                │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Dex        │  │   Keycloak  │  │   Okta      │  │   AzureAD   │         │   │ │
│  │  │  │   身份联合   │  │   身份管理   │  │   云身份    │  │   企业目录  │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                      Policy Management                                  │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   OPA        │  │   Kyverno   │  │   Config    │  │   Policy    │         │   │ │
│  │  │  │   策略引擎   │  │   策略管理   │  │   同步      │  │   审计      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                        Managed Cluster Plane                                   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                           Cluster 1 (Prod-East)                          │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Spoke     │  │   Agent     │  │   Monitor   │  │   Backup    │         │   │ │
│  │  │  │   注册      │  │   代理      │  │   监控      │  │   备份      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                           Cluster 2 (Prod-West)                          │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Spoke     │  │   Agent     │  │   Monitor   │  │   Backup    │         │   │ │
│  │  │  │   注册      │  │   代理      │  │   监控      │  │   备份      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                           Cluster N (Dev/Test)                           │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Spoke     │  │   Agent     │  │   Monitor   │  │   Backup    │         │   │ │
│  │  │  │   注册      │  │   代理      │  │   监控      │  │   备份      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                        Cross-Cluster Security                                  │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                      Federated Authentication                           │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   OIDC       │  │   SAML      │  │   LDAP      │  │   JWT       │         │   │ │
│  │  │  │   联合认证   │  │   企业单点  │  │   目录服务   │  │   令牌      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                      Unified Authorization                              │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   RBAC       │  │   ABAC      │  │   Policy    │  │   Audit     │         │   │ │
│  │  │  │   统一权限   │  │   属性控制   │  │   策略      │  │   审计      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                      Shared Secrets Management                         │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Vault      │  │   External  │  │   Sealed    │  │   GitOps    │         │   │ │
│  │  │  │   密钥存储   │  │   Secrets   │  │   Secrets   │  │   同步      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 多集群安全挑战与解决方案

#### 主要安全挑战

| 挑战 | 影响 | 解决方案 |
|------|------|---------|
| **身份一致性** | 权限分散、管理复杂 | 统一身份提供商 + 联邦认证 |
| **策略同步** | 安全标准不统一 | 策略即代码 + 自动同步 |
| **密钥管理** | 凭证泄露风险 | 外部密钥存储 + 自动轮换 |
| **网络隔离** | 横向渗透风险 | 服务网格 + 零信任网络 |
| **监控统一** | 盲点检测困难 | 集中式日志 + 统一告警 |

## 二、统一身份认证体系

### 2.1 身份联合架构

#### Dex身份提供商配置

```yaml
# 01-dex-configuration.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dex
  namespace: security
spec:
  replicas: 3
  selector:
    matchLabels:
      app: dex
  template:
    metadata:
      labels:
        app: dex
    spec:
      containers:
      - name: dex
        image: dexidp/dex:v2.37.0
        args:
        - dex
        - serve
        - /etc/dex/config.yaml
        ports:
        - containerPort: 5556
          name: oidc
        - containerPort: 5557
          name: metrics
        volumeMounts:
        - name: config
          mountPath: /etc/dex
        - name: tls-certs
          mountPath: /etc/dex/certs
        readinessProbe:
          httpGet:
            path: /.well-known/openid-configuration
            port: 5556
            scheme: HTTPS
        livenessProbe:
          httpGet:
            path: /healthz
            port: 5556
            scheme: HTTPS
      volumes:
      - name: config
        configMap:
          name: dex-config
      - name: tls-certs
        secret:
          secretName: dex-tls
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: dex-config
  namespace: security
data:
  config.yaml: |
    issuer: https://dex.k8s.example.com
    storage:
      type: kubernetes
      config:
        inCluster: true
    
    web:
      https: 0.0.0.0:5556
      tlsCert: /etc/dex/certs/tls.crt
      tlsKey: /etc/dex/certs/tls.key
    
    connectors:
    - type: github
      id: github
      name: GitHub
      config:
        clientID: $GITHUB_CLIENT_ID
        clientSecret: $GITHUB_CLIENT_SECRET
        redirectURI: https://dex.k8s.example.com/callback
        orgs:
        - name: your-organization
        
    - type: ldap
      id: ldap
      name: Corporate LDAP
      config:
        host: ldap.example.com:636
        insecureNoSSL: false
        bindDN: cn=admin,dc=example,dc=com
        bindPW: $LDAP_BIND_PASSWORD
        usernamePrompt: Email Address
        userSearch:
          baseDN: ou=People,dc=example,dc=com
          filter: "(objectClass=person)"
          username: mail
          idAttr: DN
          emailAttr: mail
          nameAttr: cn
        groupSearch:
          baseDN: ou=Groups,dc=example,dc=com
          filter: "(objectClass=groupOfNames)"
          userMatchers:
          - userAttr: DN
            groupAttr: member
          nameAttr: cn
    
    oauth2:
      skipApprovalScreen: true
    
    staticClients:
    - id: kubernetes
      redirectURIs:
      - 'https://k8s-cluster1.example.com/callback'
      - 'https://k8s-cluster2.example.com/callback'
      name: 'Kubernetes Clusters'
      secret: $KUBERNETES_CLIENT_SECRET
    
    enablePasswordDB: false
```

### 2.2 跨集群RBAC同步

#### 统一权限管理控制器

```yaml
# 02-rbac-sync-controller.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rbac-sync-controller
  namespace: security
spec:
  replicas: 2
  selector:
    matchLabels:
      app: rbac-sync-controller
  template:
    metadata:
      labels:
        app: rbac-sync-controller
    spec:
      serviceAccountName: rbac-sync
      containers:
      - name: controller
        image: security/rbac-sync-controller:latest
        env:
        - name: MANAGED_CLUSTERS
          value: "cluster1,cluster2,cluster3"
        - name: DRY_RUN
          value: "false"
        - name: SYNC_INTERVAL
          value: "300"  # 5分钟同步一次
        volumeMounts:
        - name: kubeconfig
          mountPath: /etc/kubernetes
          readOnly: true
      volumes:
      - name: kubeconfig
        secret:
          secretName: managed-clusters-kubeconfig
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: rbac-sync-policy
  namespace: security
data:
  sync-policy.yaml: |
    # 统一RBAC同步策略
    synchronization:
      # 基础角色同步
      roles:
        - name: "cluster-admin"
          rules:
          - apiGroups: ["*"]
            resources: ["*"]
            verbs: ["*"]
          clusters: ["all"]
          
        - name: "namespace-admin"
          rules:
          - apiGroups: [""]
            resources: ["pods", "services", "configmaps", "secrets"]
            verbs: ["get", "list", "create", "update", "delete"]
          clusters: ["development", "staging"]
          
      # 用户组映射
      groupMappings:
        - groupName: "platform-team"
          roleBindings:
          - roleName: "cluster-admin"
            clusters: ["all"]
            
        - groupName: "application-team"
          roleBindings:
          - roleName: "namespace-admin"
            namespaces: ["app-*"]
            clusters: ["development", "staging"]
            
      # 排除规则
      exclusions:
        - clusters: ["production-east"]
          roles: ["cluster-admin"]
          reason: "Production cluster requires manual approval"
```

## 三、多集群密钥管理

### 3.1 HashiCorp Vault联邦部署

#### Vault多集群配置

```hcl
# 03-vault-federation.hcl
# Vault主集群配置
storage "raft" {
  path = "/vault/data"
  node_id = "vault-main"
}

listener "tcp" {
  address = "0.0.0.0:8200"
  tls_cert_file = "/vault/certs/vault.crt"
  tls_key_file = "/vault/certs/vault.key"
}

seal "awskms" {
  region     = "us-west-2"
  kms_key_id = "alias/vault-seal-key"
}

api_addr = "https://vault-main.example.com:8200"
cluster_addr = "https://vault-main.example.com:8201"

# 启用身份联合
identity {
  oidc {
    issuer = "https://dex.k8s.example.com"
    client_id = "vault"
    client_secret = "vault-client-secret"
    redirect_uri = "https://vault-main.example.com:8200/ui/vault/auth/oidc/oidc/callback"
  }
}

# Vault卫星集群配置
storage "raft" {
  path = "/vault/data"
  node_id = "vault-satellite"
}

listener "tcp" {
  address = "0.0.0.0:8200"
  tls_cert_file = "/vault/certs/vault.crt"
  tls_key_file = "/vault/certs/vault.key"
}

# 连接到主集群
ha_storage "raft" {
  path = "/vault/data"
  leader_api_addr = "https://vault-main.example.com:8200"
}

api_addr = "https://vault-satellite.example.com:8200"
cluster_addr = "https://vault-satellite.example.com:8201"
```

#### Vault策略同步配置

```yaml
# 04-vault-policy-sync.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: vault-policy-sync
  namespace: security
spec:
  schedule: "*/30 * * * *"  # 每30分钟同步一次
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: policy-sync
            image: vault:latest
            env:
            - name: VAULT_ADDR_MAIN
              value: "https://vault-main.example.com:8200"
            - name: VAULT_ADDR_SATELLITE
              value: "https://vault-satellite.example.com:8200"
            - name: VAULT_TOKEN
              valueFrom:
                secretKeyRef:
                  name: vault-root-token
                  key: token
            command:
            - "/bin/sh"
            - "-c"
            - |
              # 同步策略
              policies=$(vault policy list)
              for policy in $policies; do
                if [ "$policy" != "root" ] && [ "$policy" != "default" ]; then
                  vault policy read "$policy" > "/tmp/${policy}.hcl"
                  VAULT_ADDR=$VAULT_ADDR_SATELLITE vault policy write "$policy" "/tmp/${policy}.hcl"
                fi
              done
              
              # 同步认证方法配置
              auth_methods=$(vault auth list -format=json | jq -r 'keys[]')
              for method in $auth_methods; do
                if [ "$method" != "token/" ]; then
                  config=$(vault read "auth/${method%/}/config" -format=json)
                  VAULT_ADDR=$VAULT_ADDR_SATELLITE vault write "auth/${method%/}/config" @- <<< "$config"
                fi
              done
              
              echo "Vault policy synchronization completed at $(date)"
          restartPolicy: OnFailure
```

### 3.2 外部密钥同步机制

#### External Secrets Operator多集群配置

```yaml
# 05-eso-multicluster.yaml
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-backend
spec:
  provider:
    vault:
      server: "https://vault.example.com:8200"
      path: "secret"
      version: "v2"
      auth:
        jwt:
          mountPath: "kubernetes"
          role: "external-secrets"
          serviceAccountRef:
            name: "external-secrets"
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
  namespace: production
spec:
  secretStoreRef:
    kind: ClusterSecretStore
    name: vault-backend
  target:
    name: database-creds
    creationPolicy: Owner
    template:
      engineVersion: v2
      data:
        username: "{{ .username }}"
        password: "{{ .password }}"
        host: "{{ .host }}"
        port: "{{ .port }}"
  data:
  - secretKey: username
    remoteRef:
      key: "database/prod/credentials"
      property: "username"
  - secretKey: password
    remoteRef:
      key: "database/prod/credentials"
      property: "password"
  - secretKey: host
    remoteRef:
      key: "database/prod/connection"
      property: "host"
  - secretKey: port
    remoteRef:
      key: "database/prod/connection"
      property: "port"
---
# 跨集群密钥复制
apiVersion: external-secrets.io/v1beta1
kind: PushSecret
metadata:
  name: replicate-secrets
spec:
  deletionPolicy: Delete
  secretStoreRefs:
  - name: vault-backend-cluster1
    kind: ClusterSecretStore
  - name: vault-backend-cluster2
    kind: ClusterSecretStore
  selector:
    secret:
      name: shared-secrets
  data:
    - match:
        secretKey: api-key
        remoteRef:
          remoteKey: "shared/api-key"
    - match:
        secretKey: client-cert
        remoteRef:
          remoteKey: "shared/client-cert"
```

## 四、统一安全策略管理

### 4.1 策略即代码实现

#### 统一策略仓库结构

```yaml
# 06-unified-policy-repo.yaml
repository:
  name: "kubernetes-security-policies"
  structure:
    policies/
      network/
        allow-dns.yaml
        default-deny.yaml
        ingress-whitelist.yaml
      rbac/
        platform-admin.yaml
        namespace-editor.yaml
        readonly-viewer.yaml
      runtime/
        restricted-pod-security.yaml
        allowed-capabilities.yaml
        seccomp-profiles.yaml
      compliance/
        cis-benchmark.yaml
        pci-dss.yaml
        hipaa.yaml
    clusters/
      production-east.yaml
      production-west.yaml
      development.yaml
      staging.yaml
    sync/
      policy-sync-controller.yaml
      cluster-registration.yaml
```

#### 策略同步控制器

```go
// 07-policy-sync-controller.go
package main

import (
    "context"
    "fmt"
    "log"
    "time"
    
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/client-go/kubernetes"
    "k8s.io/client-go/tools/clientcmd"
)

type PolicySyncController struct {
    clients map[string]kubernetes.Interface
    policies []Policy
}

type Policy struct {
    Name      string   `yaml:"name"`
    Content   string   `yaml:"content"`
    Clusters  []string `yaml:"clusters"`
    Namespace string   `yaml:"namespace"`
}

func (psc *PolicySyncController) syncPolicies() error {
    for _, policy := range psc.policies {
        for _, clusterName := range policy.Clusters {
            client, exists := psc.clients[clusterName]
            if !exists {
                log.Printf("Cluster %s not found", clusterName)
                continue
            }
            
            // 应用策略到目标集群
            err := psc.applyPolicy(client, policy)
            if err != nil {
                log.Printf("Failed to apply policy %s to cluster %s: %v", 
                    policy.Name, clusterName, err)
            } else {
                log.Printf("Successfully applied policy %s to cluster %s", 
                    policy.Name, clusterName)
            }
        }
    }
    return nil
}

func (psc *PolicySyncController) applyPolicy(client kubernetes.Interface, policy Policy) error {
    // 根据策略类型应用不同的Kubernetes资源
    switch {
    case policy.Name == "network-policy":
        return psc.applyNetworkPolicy(client, policy)
    case policy.Name == "rbac-policy":
        return psc.applyRBACPolicy(client, policy)
    default:
        return fmt.Errorf("unsupported policy type: %s", policy.Name)
    }
}

func main() {
    // 初始化多集群客户端
    clusters := []string{"cluster1", "cluster2", "cluster3"}
    clients := make(map[string]kubernetes.Interface)
    
    for _, cluster := range clusters {
        config, err := clientcmd.BuildConfigFromFlags("", fmt.Sprintf("/kubeconfig/%s", cluster))
        if err != nil {
            log.Fatalf("Failed to build config for cluster %s: %v", cluster, err)
        }
        
        client, err := kubernetes.NewForConfig(config)
        if err != nil {
            log.Fatalf("Failed to create client for cluster %s: %v", cluster, err)
        }
        
        clients[cluster] = client
    }
    
    controller := &PolicySyncController{
        clients: clients,
        policies: loadPolicies(), // 从Git仓库加载策略
    }
    
    // 启动定时同步
    ticker := time.NewTicker(5 * time.Minute)
    defer ticker.Stop()
    
    for {
        select {
        case <-ticker.C:
            if err := controller.syncPolicies(); err != nil {
                log.Printf("Policy sync failed: %v", err)
            }
        }
    }
}
```

## 五、集中监控与告警

### 5.1 统一日志收集架构

#### 多集群EFK堆栈配置

```yaml
# 08-centralized-logging.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fluentd-aggregator
  namespace: logging
spec:
  replicas: 3
  selector:
    matchLabels:
      app: fluentd-aggregator
  template:
    metadata:
      labels:
        app: fluentd-aggregator
    spec:
      containers:
      - name: fluentd
        image: fluent/fluentd-kubernetes-daemonset:v1.14.6-debian-elasticsearch7-1.0
        env:
        - name: FLUENT_ELASTICSEARCH_HOST
          value: "elasticsearch.logging.svc.cluster.local"
        - name: FLUENT_ELASTICSEARCH_PORT
          value: "9200"
        - name: FLUENTD_SYSTEMD_CONF
          value: "disable"
        volumeMounts:
        - name: varlog
          mountPath: /var/log
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
        - name: fluentd-config
          mountPath: /fluentd/etc
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
      - name: fluentd-config
        configMap:
          name: fluentd-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
  namespace: logging
data:
  fluent.conf: |
    <source>
      @type forward
      port 24224
      bind 0.0.0.0
    </source>
    
    <match kubernetes.**>
      @type elasticsearch
      host "#{ENV['FLUENT_ELASTICSEARCH_HOST']}"
      port "#{ENV['FLUENT_ELASTICSEARCH_PORT']}"
      logstash_format true
      logstash_prefix "k8s-${record['kubernetes']['namespace_name']}"
      include_tag_key true
      type_name "access_log"
      tag_key "@log_name"
      
      <buffer>
        @type file
        path /var/log/fluentd-buffers/kubernetes.system.buffer
        flush_mode interval
        retry_type exponential_backoff
        flush_thread_count 2
        flush_interval 5s
        retry_forever
        retry_max_interval 30
        chunk_limit_size 2M
        queue_limit_length 8
        overflow_action block
      </buffer>
    </match>
    
    # 安全日志特殊处理
    <match kubernetes.var.log.containers.*security*.log>
      @type copy
      <store>
        @type elasticsearch
        host "#{ENV['FLUENT_ELASTICSEARCH_HOST']}"
        port "#{ENV['FLUENT_ELASTICSEARCH_PORT']}"
        logstash_format true
        logstash_prefix "security-logs"
        include_tag_key true
      </store>
      <store>
        @type slack
        webhook_url "#{ENV['SLACK_WEBHOOK_URL']}"
        channel "#security-alerts"
        username "Security Bot"
        icon_emoji ":rotating_light:"
        message_keys "log,message,level"
      </store>
    </match>
```

### 5.2 统一告警与通知

#### 多集群告警规则

```yaml
# 09-unified-alerting.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: multicluster-security-alerts
  namespace: monitoring
spec:
  groups:
  - name: multicluster.security
    rules:
    # 跨集群异常检测
    - alert: CrossClusterUnauthorizedAccess
      expr: sum by(cluster, namespace) (
        rate(kube_auth_user_requests_total{verb=~"create|update|delete", code=~"4.."}[5m])
      ) > 10
      for: 2m
      labels:
        severity: critical
        category: "unauthorizedAccess"
      annotations:
        summary: "跨集群未授权访问检测"
        description: "集群 {{ $labels.cluster }} 中命名空间 {{ $labels.namespace }} 发现 {{ $value }} 次未授权访问尝试"
        
    # 多集群策略违规
    - alert: MultiClusterPolicyViolation
      expr: sum by(policy, cluster) (
        rate(policy_violations_total[10m])
      ) > 5
      for: 5m
      labels:
        severity: warning
        category: "policyViolation"
      annotations:
        summary: "多集群策略违规"
        description: "策略 {{ $labels.policy }} 在集群 {{ $labels.cluster }} 中违反 {{ $value }} 次"
        
    # 联邦认证异常
    - alert: FederationAuthFailure
      expr: sum by(cluster) (
        rate(oidc_auth_failures_total[5m])
      ) > 3
      for: 3m
      labels:
        severity: critical
        category: "authentication"
      annotations:
        summary: "联邦认证失败"
        description: "集群 {{ $labels.cluster }} 中OIDC认证失败次数: {{ $value }}"
        
    # 密钥同步异常
    - alert: SecretSyncFailure
      expr: sum by(secret, cluster) (
        rate(secret_sync_failures_total[10m])
      ) > 2
      for: 5m
      labels:
        severity: warning
        category: "secrets"
      annotations:
        summary: "密钥同步失败"
        description: "密钥 {{ $labels.secret }} 在集群 {{ $labels.cluster }} 同步失败"
```

## 六、多集群安全运维最佳实践

### 6.1 部署与维护脚本

#### 多集群安全检查脚本

```bash
#!/bin/bash
# 10-multicluster-security-check.sh

CLUSTERS=("prod-east" "prod-west" "dev" "staging")
SECURITY_CHECKS=(
    "rbac_consistency"
    "network_policy_coverage"
    "secret_management"
    "compliance_status"
    "vulnerability_scan"
)

function check_cluster_security() {
    local cluster=$1
    local kubeconfig="/kubeconfig/${cluster}"
    
    echo "=== Security Check for Cluster: ${cluster} ==="
    
    # 1. RBAC一致性检查
    echo "1. Checking RBAC consistency..."
    kubectl --kubeconfig=${kubeconfig} get clusterroles,clusterrolebindings | \
        grep -E "(admin|edit|view)" | \
        sort > "/tmp/${cluster}_rbac.txt"
    
    # 2. 网络策略覆盖率
    echo "2. Checking network policy coverage..."
    total_namespaces=$(kubectl --kubeconfig=${kubeconfig} get ns --no-headers | wc -l)
    protected_namespaces=$(kubectl --kubeconfig=${kubeconfig} get networkpolicies --all-namespaces --no-headers | \
        awk '{print $1}' | sort | uniq | wc -l)
    coverage_percent=$((protected_namespaces * 100 / total_namespaces))
    echo "Network policy coverage: ${coverage_percent}% (${protected_namespaces}/${total_namespaces})"
    
    # 3. 密钥管理检查
    echo "3. Checking secret management..."
    kubectl --kubeconfig=${kubeconfig} get secrets --all-namespaces | \
        grep -E "(tls|opaque)" | \
        wc -l
    
    # 4. 合规状态检查
    echo "4. Checking compliance status..."
    kubectl --kubeconfig=${kubeconfig} get pods --all-namespaces -o jsonpath='{.items[*].spec.containers[*].securityContext.runAsNonRoot}' | \
        tr ' ' '\n' | grep -c "true" || echo "0"
    
    # 5. 漏洞扫描状态
    echo "5. Checking vulnerability scan status..."
    kubectl --kubeconfig=${kubeconfig} get jobs -n security -l app=vulnerability-scanner | \
        grep -c "1/1" || echo "0"
}

function generate_unified_report() {
    echo "=== Multi-Cluster Security Report ===" > /reports/security_report_$(date +%Y%m%d).txt
    echo "Generated at: $(date)" >> /reports/security_report_$(date +%Y%m%d).txt
    echo "" >> /reports/security_report_$(date +%Y%m%d).txt
    
    for cluster in "${CLUSTERS[@]}"; do
        echo "Cluster: ${cluster}" >> /reports/security_report_$(date +%Y%m%d).txt
        cat "/tmp/${cluster}_security_check.txt" >> /reports/security_report_$(date +%Y%m%d).txt
        echo "---" >> /reports/security_report_$(date +%Y%m%d).txt
    done
}

# 执行检查
for cluster in "${CLUSTERS[@]}"; do
    check_cluster_security "${cluster}" > "/tmp/${cluster}_security_check.txt"
done

# 生成统一报告
generate_unified_report

echo "Multi-cluster security check completed!"
echo "Report saved to: /reports/security_report_$(date +%Y%m%d).txt"
```

这份多集群安全管理与联邦认证文档提供了企业级的多集群安全解决方案，涵盖了身份联合、策略同步、密钥管理和统一监控等核心要素。