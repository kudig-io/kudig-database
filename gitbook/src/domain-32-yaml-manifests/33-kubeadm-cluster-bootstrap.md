# 33 - kubeadm 集群引导配置 YAML 参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

## 目录

- [1. 概述](#1-概述)
- [2. ClusterConfiguration](#2-clusterconfiguration)
- [3. InitConfiguration](#3-initconfiguration)
- [4. JoinConfiguration](#4-joinconfiguration)
- [5. ResetConfiguration](#5-resetconfiguration)
- [6. 内部机制](#6-内部机制)
- [7. 生产案例](#7-生产案例)

---

## 1. 概述

### 1.1 kubeadm 配置 API 版本演进

```yaml
# v1.25-v1.32 推荐使用的 API 版本
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration
---
# v1.28+ 新增
apiVersion: kubeadm.k8s.io/v1beta3
kind: ResetConfiguration
```

### 1.2 配置文件使用方式

```bash
# 初始化集群
kubeadm init --config kubeadm-init-config.yaml

# 节点加入集群
kubeadm join --config kubeadm-join-config.yaml

# 重置节点
kubeadm reset --config kubeadm-reset-config.yaml

# 打印默认配置
kubeadm config print init-defaults
kubeadm config print join-defaults

# 验证配置
kubeadm config validate --config kubeadm-config.yaml
```

---

## 2. ClusterConfiguration

### 2.1 完整字段规范

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
# Kubernetes 版本
kubernetesVersion: v1.32.0

# 集群名称（默认 kubernetes）
clusterName: production-cluster

# 控制平面端点（HA 场景必填）
controlPlaneEndpoint: "k8s-api.example.com:6443"

# 证书目录
certificatesDir: /etc/kubernetes/pki

# 镜像仓库配置
imageRepository: registry.aliyuncs.com/google_containers
# DNS 域名
dnsDomain: cluster.local

# 网络配置
networking:
  # Service CIDR（默认 10.96.0.0/12）
  serviceSubnet: "10.96.0.0/12"
  # Pod CIDR（需要与 CNI 一致）
  podSubnet: "10.244.0.0/16"
  # DNS 域名
  dnsDomain: cluster.local

# etcd 配置
etcd:
  # 本地 etcd（单节点或 stacked HA）
  local:
    imageRepository: "registry.aliyuncs.com/google_containers"
    imageTag: "3.5.9-0"
    dataDir: /var/lib/etcd
    # etcd 额外参数
    extraArgs:
      listen-metrics-urls: "http://0.0.0.0:2381"
      quota-backend-bytes: "8589934592"  # 8GB
      auto-compaction-retention: "1"
      max-request-bytes: "33554432"      # 32MB
      snapshot-count: "10000"
    # 证书 SAN
    serverCertSANs:
      - "etcd.example.com"
      - "10.0.0.10"
    peerCertSANs:
      - "etcd.example.com"
      - "10.0.0.10"
  
  # 外部 etcd（生产推荐）
  # external:
  #   endpoints:
  #     - https://10.0.0.11:2379
  #     - https://10.0.0.12:2379
  #     - https://10.0.0.13:2379
  #   caFile: /etc/kubernetes/pki/etcd/ca.crt
  #   certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
  #   keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

# API Server 配置
apiServer:
  # 额外参数
  extraArgs:
    # 审计配置
    audit-log-path: /var/log/kubernetes/audit/audit.log
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    # 认证配置
    enable-admission-plugins: NodeRestriction,PodSecurityPolicy
    authorization-mode: Node,RBAC
    # API 优先级和公平性
    enable-priority-and-fairness: "true"
    # 功能特性门控
    feature-gates: "APIServerTracing=true,EphemeralContainers=true"
    # 匿名访问
    anonymous-auth: "false"
    # 最大请求数
    max-requests-inflight: "400"
    max-mutating-requests-inflight: "200"
    # OIDC 认证
    oidc-issuer-url: "https://oidc.example.com"
    oidc-client-id: "kubernetes"
    oidc-username-claim: "email"
    oidc-groups-claim: "groups"
  
  # 额外卷挂载
  extraVolumes:
    - name: audit-policy
      hostPath: /etc/kubernetes/audit-policy.yaml
      mountPath: /etc/kubernetes/audit-policy.yaml
      readOnly: true
      pathType: File
    - name: audit-log
      hostPath: /var/log/kubernetes/audit
      mountPath: /var/log/kubernetes/audit
      pathType: DirectoryOrCreate
  
  # 证书 SAN（重要：所有访问 API 的地址）
  certSANs:
    - "k8s-api.example.com"
    - "api.k8s.local"
    - "10.0.0.10"
    - "10.0.0.11"
    - "10.0.0.12"
    - "192.168.1.100"  # VIP
    - "127.0.0.1"
  
  # 超时配置
  timeoutForControlPlane: 4m0s

# Controller Manager 配置
controllerManager:
  extraArgs:
    # 节点监控
    node-monitor-grace-period: "40s"
    node-monitor-period: "5s"
    # Pod 驱逐
    pod-eviction-timeout: "5m0s"
    # 集群签名
    cluster-signing-duration: "87600h"  # 10 years
    # 并发数
    concurrent-deployment-syncs: "5"
    concurrent-endpoint-syncs: "5"
    concurrent-gc-syncs: "20"
    concurrent-namespace-syncs: "10"
    concurrent-replicaset-syncs: "5"
    concurrent-service-syncs: "1"
    # 功能特性门控
    feature-gates: "RotateKubeletServerCertificate=true"
    # 终止 Pod 优雅时长
    terminated-pod-gc-threshold: "10000"
  
  extraVolumes:
    - name: flex-volume
      hostPath: /usr/libexec/kubernetes/kubelet-plugins/volume/exec
      mountPath: /usr/libexec/kubernetes/kubelet-plugins/volume/exec
      pathType: DirectoryOrCreate

# Scheduler 配置
scheduler:
  extraArgs:
    # 功能特性门控
    feature-gates: "EphemeralContainers=true"
    # 调度配置
    profiling: "false"
    v: "2"
  
  extraVolumes: []

---
```

### 2.2 基础配置示例

```yaml
# 最小化配置（单节点开发环境）
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
networking:
  podSubnet: "10.244.0.0/16"
```

### 2.3 生产级配置示例

```yaml
# 生产环境 HA 配置（Stacked etcd）
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
clusterName: prod-k8s-cluster
controlPlaneEndpoint: "k8s-api.prod.example.com:6443"
imageRepository: registry.cn-hangzhou.aliyuncs.com/google_containers

networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "172.16.0.0/16"
  dnsDomain: cluster.local

etcd:
  local:
    dataDir: /var/lib/etcd
    extraArgs:
      listen-metrics-urls: "http://0.0.0.0:2381"
      quota-backend-bytes: "8589934592"
      auto-compaction-retention: "1"
      max-request-bytes: "33554432"
      snapshot-count: "10000"
      heartbeat-interval: "100"
      election-timeout: "1000"

apiServer:
  extraArgs:
    # 审计日志
    audit-log-path: /var/log/kubernetes/audit/audit.log
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    # 准入控制
    enable-admission-plugins: "NodeRestriction,PodSecurityPolicy,ResourceQuota,LimitRanger"
    # 性能优化
    max-requests-inflight: "400"
    max-mutating-requests-inflight: "200"
    # 安全加固
    anonymous-auth: "false"
    profiling: "false"
    enable-bootstrap-token-auth: "true"
    # 加密配置
    encryption-provider-config: /etc/kubernetes/encryption-config.yaml
    # 服务账户密钥
    service-account-issuer: "https://kubernetes.default.svc.cluster.local"
    service-account-signing-key-file: /etc/kubernetes/pki/sa.key
    service-account-key-file: /etc/kubernetes/pki/sa.pub
  
  extraVolumes:
    - name: audit-policy
      hostPath: /etc/kubernetes/audit-policy.yaml
      mountPath: /etc/kubernetes/audit-policy.yaml
      readOnly: true
      pathType: File
    - name: audit-log
      hostPath: /var/log/kubernetes/audit
      mountPath: /var/log/kubernetes/audit
      pathType: DirectoryOrCreate
    - name: encryption-config
      hostPath: /etc/kubernetes/encryption-config.yaml
      mountPath: /etc/kubernetes/encryption-config.yaml
      readOnly: true
      pathType: File
  
  certSANs:
    - "k8s-api.prod.example.com"
    - "10.0.1.10"
    - "10.0.1.11"
    - "10.0.1.12"
    - "192.168.100.100"  # HAProxy/Keepalived VIP
    - "127.0.0.1"

controllerManager:
  extraArgs:
    node-monitor-grace-period: "40s"
    node-monitor-period: "5s"
    pod-eviction-timeout: "1m0s"
    terminated-pod-gc-threshold: "10000"
    feature-gates: "RotateKubeletServerCertificate=true"
    profiling: "false"

scheduler:
  extraArgs:
    profiling: "false"
    feature-gates: "EphemeralContainers=true"
```

### 2.4 外部 etcd 配置示例

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "k8s-api.example.com:6443"

networking:
  podSubnet: "10.244.0.0/16"

etcd:
  external:
    endpoints:
      - https://etcd1.example.com:2379
      - https://etcd2.example.com:2379
      - https://etcd3.example.com:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

apiServer:
  certSANs:
    - "k8s-api.example.com"
    - "10.0.0.10"
    - "10.0.0.11"
    - "10.0.0.12"
```

---

## 3. InitConfiguration

### 3.1 完整字段规范

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration

# Bootstrap Token 配置
bootstrapTokens:
  # Token ID（6位字符）
  - token: "abcdef.0123456789abcdef"
    description: "kubeadm bootstrap token"
    # Token 用途
    usages:
      - signing
      - authentication
    # Token 有效期（默认 24h）
    ttl: "24h"
    # 额外组
    groups:
      - system:bootstrappers:kubeadm:default-node-token
  
  # 第二个 Token（用于控制平面节点加入）
  - token: "uvwxyz.0123456789uvwxyz"
    description: "control plane join token"
    usages:
      - signing
      - authentication
    ttl: "1h"
    groups:
      - system:bootstrappers:kubeadm:default-node-token

# 节点注册配置
nodeRegistration:
  # 节点名称（默认使用主机名）
  name: master01.k8s.local
  
  # CRI Socket 路径
  # containerd（默认）
  criSocket: unix:///var/run/containerd/containerd.sock
  # CRI-O
  # criSocket: unix:///var/run/crio/crio.sock
  # Docker（通过 cri-dockerd，已弃用）
  # criSocket: unix:///var/run/cri-dockerd.sock
  
  # 污点配置
  taints:
    # 默认控制平面污点
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule
    # 自定义污点
    - key: node-role.kubernetes.io/master
      effect: NoSchedule
  
  # Kubelet 额外参数
  kubeletExtraArgs:
    # 节点标签
    node-labels: "node.kubernetes.io/master=,node.kubernetes.io/node-type=control-plane"
    # 资源预留
    kube-reserved: "cpu=500m,memory=512Mi,ephemeral-storage=1Gi"
    system-reserved: "cpu=500m,memory=512Mi,ephemeral-storage=1Gi"
    eviction-hard: "memory.available<256Mi,nodefs.available<10%,imagefs.available<15%"
    # 镜像拉取
    max-parallel-image-pulls: "3"
    # 日志配置
    v: "2"
    # 特性门控
    feature-gates: "RotateKubeletServerCertificate=true,GracefulNodeShutdown=true"
    # 容器日志
    container-log-max-size: "10Mi"
    container-log-max-files: "5"
  
  # 忽略预检错误
  # 注意：生产环境谨慎使用
  ignorePreflightErrors:
    - NumCPU
    - Mem
    # - all  # 忽略所有错误（不推荐）
  
  # 镜像拉取策略
  imagePullPolicy: IfNotPresent

# 本地 API Endpoint（本节点）
localAPIEndpoint:
  # 本节点 IP（用于生成证书和 kubelet 配置）
  advertiseAddress: 10.0.1.10
  # API Server 监听端口（默认 6443）
  bindPort: 6443

# 跳过阶段（高级用法）
# skipPhases:
#   - addon/kube-proxy
#   - addon/coredns

# 补丁（高级用法，修改静态 Pod 配置）
# patches:
#   directory: /etc/kubernetes/patches

---
```

### 3.2 基础初始化配置

```yaml
# 最小化初始化配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 10.0.1.10
  bindPort: 6443
nodeRegistration:
  criSocket: unix:///var/run/containerd/containerd.sock
```

### 3.3 生产初始化配置

```yaml
# 生产环境初始化配置（第一个控制平面节点）
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration

bootstrapTokens:
  - token: "9a08jv.c0izixklcxtmnze7"
    description: "Production cluster bootstrap token"
    usages:
      - signing
      - authentication
    ttl: "24h"
    groups:
      - system:bootstrappers:kubeadm:default-node-token

nodeRegistration:
  name: k8s-master01.prod.example.com
  criSocket: unix:///var/run/containerd/containerd.sock
  
  taints:
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule
  
  kubeletExtraArgs:
    node-labels: "node-role.kubernetes.io/master=,environment=production,zone=az1"
    kube-reserved: "cpu=1000m,memory=2Gi,ephemeral-storage=10Gi"
    system-reserved: "cpu=1000m,memory=2Gi,ephemeral-storage=10Gi"
    eviction-hard: "memory.available<512Mi,nodefs.available<10%,imagefs.available<15%"
    max-parallel-image-pulls: "5"
    feature-gates: "RotateKubeletServerCertificate=true,GracefulNodeShutdown=true"
    container-log-max-size: "50Mi"
    container-log-max-files: "10"
    rotate-certificates: "true"
    read-only-port: "0"
    event-burst: "50"
    event-qps: "5"

localAPIEndpoint:
  advertiseAddress: 10.0.1.10
  bindPort: 6443

---
# 配合 ClusterConfiguration 使用
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
clusterName: prod-k8s-cluster
controlPlaneEndpoint: "k8s-api.prod.example.com:6443"
imageRepository: registry.cn-hangzhou.aliyuncs.com/google_containers

networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "172.16.0.0/16"
  dnsDomain: cluster.local

apiServer:
  certSANs:
    - "k8s-api.prod.example.com"
    - "10.0.1.10"
    - "10.0.1.11"
    - "10.0.1.12"
    - "192.168.100.100"  # VIP
    - "127.0.0.1"
```

### 3.4 离线部署配置

```yaml
# 离线环境初始化配置（使用本地镜像仓库）
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration

nodeRegistration:
  name: k8s-master01
  criSocket: unix:///var/run/containerd/containerd.sock
  kubeletExtraArgs:
    pod-infra-container-image: "harbor.local.corp/google_containers/pause:3.9"
  imagePullPolicy: IfNotPresent

localAPIEndpoint:
  advertiseAddress: 192.168.1.10
  bindPort: 6443

---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
# 使用内网镜像仓库
imageRepository: harbor.local.corp/google_containers

networking:
  podSubnet: "10.244.0.0/16"

# 配置 DNS（使用本地 DNS）
dns:
  imageRepository: harbor.local.corp/coredns
  imageTag: v1.11.1

# 使用本地 etcd 镜像
etcd:
  local:
    imageRepository: harbor.local.corp/google_containers
    imageTag: 3.5.9-0
```

---

## 4. JoinConfiguration

### 4.1 完整字段规范

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration

# 节点注册配置（同 InitConfiguration）
nodeRegistration:
  name: worker01.k8s.local
  criSocket: unix:///var/run/containerd/containerd.sock
  
  # 工作节点通常不需要污点
  taints: []
  # 控制平面节点需要污点
  # taints:
  #   - key: node-role.kubernetes.io/control-plane
  #     effect: NoSchedule
  
  kubeletExtraArgs:
    node-labels: "node.kubernetes.io/worker=,workload-type=general,zone=az1"
    kube-reserved: "cpu=500m,memory=1Gi,ephemeral-storage=5Gi"
    system-reserved: "cpu=500m,memory=1Gi,ephemeral-storage=5Gi"
    eviction-hard: "memory.available<256Mi,nodefs.available<10%"
    max-parallel-image-pulls: "5"
    feature-gates: "GracefulNodeShutdown=true"
  
  ignorePreflightErrors:
    - DirAvailable--etc-kubernetes-manifests

# 发现配置（如何找到集群）
discovery:
  # 方式1: 使用 Bootstrap Token 发现（推荐）
  bootstrapToken:
    # Token 值
    token: "abcdef.0123456789abcdef"
    # API Server 地址
    apiServerEndpoint: "k8s-api.example.com:6443"
    # CA 证书哈希（用于验证 API Server）
    # 获取方式: openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
    #           openssl rsa -pubin -outform der 2>/dev/null | \
    #           openssl dgst -sha256 -hex | sed 's/^.* //'
    caCertHashes:
      - "sha256:1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
    # 不安全模式（跳过证书验证，仅用于测试）
    # unsafeSkipCAVerification: true
  
  # 方式2: 使用配置文件发现（更安全）
  # file:
  #   kubeConfigPath: /etc/kubernetes/bootstrap-kubelet.conf
  
  # 方式3: 使用 token 文件发现
  # token: abcdef.0123456789abcdef
  # caCertPath: /etc/kubernetes/pki/ca.crt
  
  # 超时配置
  timeout: 5m0s
  # TLS 引导超时
  tlsBootstrapToken: "abcdef.0123456789abcdef"

# 控制平面配置（仅用于控制平面节点加入）
controlPlane:
  # 本地 API Endpoint
  localAPIEndpoint:
    advertiseAddress: 10.0.1.11
    bindPort: 6443
  
  # 证书密钥（用于加密证书传输）
  # 在第一个控制平面节点上生成:
  # kubeadm init phase upload-certs --upload-certs
  # 会输出类似: --certificate-key <key>
  certificateKey: "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"

# 跳过阶段
# skipPhases:
#   - preflight

# 补丁
# patches:
#   directory: /etc/kubernetes/patches

---
```

### 4.2 工作节点加入配置

```yaml
# 基础工作节点加入配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration

discovery:
  bootstrapToken:
    token: "abcdef.0123456789abcdef"
    apiServerEndpoint: "k8s-api.example.com:6443"
    caCertHashes:
      - "sha256:1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"

nodeRegistration:
  criSocket: unix:///var/run/containerd/containerd.sock
  name: worker01
  kubeletExtraArgs:
    node-labels: "node-role.kubernetes.io/worker="
```

### 4.3 生产工作节点加入配置

```yaml
# 生产环境工作节点加入配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration

discovery:
  bootstrapToken:
    token: "9a08jv.c0izixklcxtmnze7"
    apiServerEndpoint: "k8s-api.prod.example.com:6443"
    caCertHashes:
      - "sha256:a1b2c3d4e5f61234567890abcdef1234567890abcdef1234567890abcdef1234"
  timeout: 5m0s

nodeRegistration:
  name: k8s-worker01.prod.example.com
  criSocket: unix:///var/run/containerd/containerd.sock
  taints: []
  
  kubeletExtraArgs:
    # 节点标签（用于调度）
    node-labels: "node-role.kubernetes.io/worker=,workload-type=general,environment=production,zone=az1,disk-type=ssd"
    # 资源预留
    kube-reserved: "cpu=500m,memory=1Gi,ephemeral-storage=5Gi"
    system-reserved: "cpu=500m,memory=1Gi,ephemeral-storage=5Gi"
    # 驱逐阈值
    eviction-hard: "memory.available<512Mi,nodefs.available<10%,imagefs.available<15%,pid.available<1000"
    eviction-soft: "memory.available<1Gi,nodefs.available<15%,imagefs.available<20%"
    eviction-soft-grace-period: "memory.available=1m30s,nodefs.available=2m,imagefs.available=2m"
    # 镜像拉取
    max-parallel-image-pulls: "5"
    serialize-image-pulls: "false"
    # 特性门控
    feature-gates: "GracefulNodeShutdown=true,RotateKubeletServerCertificate=true"
    # 容器日志
    container-log-max-size: "50Mi"
    container-log-max-files: "10"
    # 安全
    rotate-certificates: "true"
    read-only-port: "0"
    # 性能
    event-burst: "50"
    event-qps: "5"
    registry-burst: "10"
    registry-qps: "5"
```

### 4.4 控制平面节点加入配置（HA）

```yaml
# HA 集群第二/三个控制平面节点加入配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration

discovery:
  bootstrapToken:
    token: "abcdef.0123456789abcdef"
    apiServerEndpoint: "k8s-api.example.com:6443"
    caCertHashes:
      - "sha256:1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"

nodeRegistration:
  name: k8s-master02
  criSocket: unix:///var/run/containerd/containerd.sock
  taints:
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule
  
  kubeletExtraArgs:
    node-labels: "node-role.kubernetes.io/master=,environment=production,zone=az2"
    kube-reserved: "cpu=1000m,memory=2Gi,ephemeral-storage=10Gi"
    system-reserved: "cpu=1000m,memory=2Gi,ephemeral-storage=10Gi"
    eviction-hard: "memory.available<512Mi,nodefs.available<10%"
    feature-gates: "RotateKubeletServerCertificate=true"

# 控制平面配置（关键！）
controlPlane:
  localAPIEndpoint:
    advertiseAddress: 10.0.1.11  # 本节点 IP
    bindPort: 6443
  
  # 证书密钥（从第一个控制平面节点获取）
  # 生成方式: kubeadm init phase upload-certs --upload-certs
  certificateKey: "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"
```

### 4.5 使用文件发现（更安全）

```yaml
# 使用 kubeconfig 文件发现（避免 token 泄露）
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration

# 使用预先准备的 kubeconfig 文件
discovery:
  file:
    kubeConfigPath: /etc/kubernetes/bootstrap-kubelet.conf
  tlsBootstrapToken: "abcdef.0123456789abcdef"

nodeRegistration:
  criSocket: unix:///var/run/containerd/containerd.sock
  name: worker01
```

**准备 bootstrap-kubelet.conf 文件**:

```bash
# 在控制平面节点上
kubectl config set-cluster kubernetes \
  --certificate-authority=/etc/kubernetes/pki/ca.crt \
  --embed-certs=true \
  --server=https://k8s-api.example.com:6443 \
  --kubeconfig=/tmp/bootstrap-kubelet.conf

kubectl config set-credentials tls-bootstrap-token-user \
  --token=abcdef.0123456789abcdef \
  --kubeconfig=/tmp/bootstrap-kubelet.conf

kubectl config set-context tls-bootstrap-token-user@kubernetes \
  --cluster=kubernetes \
  --user=tls-bootstrap-token-user \
  --kubeconfig=/tmp/bootstrap-kubelet.conf

kubectl config use-context tls-bootstrap-token-user@kubernetes \
  --kubeconfig=/tmp/bootstrap-kubelet.conf

# 复制到工作节点
scp /tmp/bootstrap-kubelet.conf worker01:/etc/kubernetes/
```

---

## 5. ResetConfiguration

### 5.1 完整字段规范

```yaml
# v1.28+ 新增配置（用于 kubeadm reset）
apiVersion: kubeadm.k8s.io/v1beta3
kind: ResetConfiguration

# 强制重置（不询问确认）
force: false

# 清理 /etc/cni/net.d 目录
cleanupTmpDir: true

# 证书目录
certificatesDir: /etc/kubernetes/pki

# CRI Socket
criSocket: unix:///var/run/containerd/containerd.sock

# 跳过阶段
# skipPhases:
#   - remove-etcd-member
#   - cleanup-node

# 忽略预检错误
# ignorePreflightErrors:
#   - all

# 卸载 CNI 配置（v1.30+）
unmountFlags:
  # 超时时间
  timeout: 30s

---
```

### 5.2 基础重置配置

```yaml
# 基础重置配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: ResetConfiguration
force: true
cleanupTmpDir: true
```

### 5.3 生产重置配置

```yaml
# 生产环境重置配置（谨慎使用）
apiVersion: kubeadm.k8s.io/v1beta3
kind: ResetConfiguration

# 交互式确认
force: false

# 清理临时目录
cleanupTmpDir: true

# CRI Socket
criSocket: unix:///var/run/containerd/containerd.sock

# 不删除 etcd 成员（手动删除）
skipPhases:
  - remove-etcd-member

# 卸载超时
unmountFlags:
  timeout: 60s
```

**完整重置流程（手动）**:

```bash
# 1. 驱逐 Pod（可选）
kubectl drain <node-name> --delete-emptydir-data --force --ignore-daemonsets

# 2. 删除节点
kubectl delete node <node-name>

# 3. 重置节点
kubeadm reset --config kubeadm-reset-config.yaml

# 4. 清理 iptables/ipvs 规则
iptables -F && iptables -t nat -F && iptables -t mangle -F && iptables -X
ipvsadm --clear

# 5. 清理 CNI 配置
rm -rf /etc/cni/net.d
rm -rf /var/lib/cni

# 6. 清理 containerd 状态
crictl rmp -fa
crictl rmi -a

# 7. 重启 containerd 和 kubelet
systemctl restart containerd
systemctl stop kubelet
```

---

## 6. 内部机制

### 6.1 kubeadm init 阶段

```bash
# 查看所有阶段
kubeadm init phase --help

# kubeadm init 完整阶段流程
kubeadm init \
  --config kubeadm-init-config.yaml

# 等价于以下阶段（按顺序执行）:
```

#### 阶段 1: preflight（预检）

```bash
kubeadm init phase preflight --config kubeadm-init-config.yaml

# 检查项:
# - 操作系统和内核版本
# - Swap 是否关闭
# - CPU 核心数（至少 2 核）
# - 内存大小（至少 1700MB）
# - 端口占用（6443, 10250, 10259, 10257, 2379-2380）
# - 容器运行时可用性
# - /etc/kubernetes 目录是否为空
# - kubelet 版本兼容性
# - 防火墙和 iptables 配置
```

#### 阶段 2: certs（生成证书）

```bash
# 生成所有证书
kubeadm init phase certs all --config kubeadm-init-config.yaml

# 或逐个生成:
kubeadm init phase certs ca                    # CA 根证书
kubeadm init phase certs apiserver             # API Server 证书
kubeadm init phase certs apiserver-kubelet-client  # API Server 访问 Kubelet 证书
kubeadm init phase certs front-proxy-ca        # 前端代理 CA
kubeadm init phase certs front-proxy-client    # 前端代理客户端证书
kubeadm init phase certs etcd-ca               # etcd CA
kubeadm init phase certs etcd-server           # etcd Server 证书
kubeadm init phase certs etcd-peer             # etcd Peer 证书
kubeadm init phase certs etcd-healthcheck-client  # etcd 健康检查客户端证书
kubeadm init phase certs apiserver-etcd-client # API Server 访问 etcd 证书
kubeadm init phase certs sa                    # Service Account 密钥对

# 证书存储位置:
# /etc/kubernetes/pki/
# ├── ca.crt                          # Kubernetes CA 证书
# ├── ca.key                          # Kubernetes CA 私钥
# ├── apiserver.crt                   # API Server 证书
# ├── apiserver.key                   # API Server 私钥
# ├── apiserver-kubelet-client.crt    # API Server 访问 Kubelet 证书
# ├── apiserver-kubelet-client.key
# ├── front-proxy-ca.crt              # 前端代理 CA
# ├── front-proxy-ca.key
# ├── front-proxy-client.crt
# ├── front-proxy-client.key
# ├── sa.pub                          # Service Account 公钥
# ├── sa.key                          # Service Account 私钥
# └── etcd/
#     ├── ca.crt                      # etcd CA 证书
#     ├── ca.key
#     ├── server.crt                  # etcd Server 证书
#     ├── server.key
#     ├── peer.crt                    # etcd Peer 证书
#     ├── peer.key
#     ├── healthcheck-client.crt      # etcd 健康检查客户端证书
#     └── healthcheck-client.key

# 查看证书有效期
kubeadm certs check-expiration

# 证书续期
kubeadm certs renew all
```

#### 阶段 3: kubeconfig（生成 kubeconfig 文件）

```bash
# 生成所有 kubeconfig
kubeadm init phase kubeconfig all --config kubeadm-init-config.yaml

# 或逐个生成:
kubeadm init phase kubeconfig admin              # admin.conf
kubeadm init phase kubeconfig kubelet            # kubelet.conf
kubeadm init phase kubeconfig controller-manager # controller-manager.conf
kubeadm init phase kubeconfig scheduler          # scheduler.conf

# 生成的文件:
# /etc/kubernetes/
# ├── admin.conf                 # 集群管理员配置（用于 kubectl）
# ├── kubelet.conf               # Kubelet 配置
# ├── controller-manager.conf    # Controller Manager 配置
# └── scheduler.conf             # Scheduler 配置
```

#### 阶段 4: control-plane（生成静态 Pod 清单）

```bash
# 生成所有控制平面组件
kubeadm init phase control-plane all --config kubeadm-init-config.yaml

# 或逐个生成:
kubeadm init phase control-plane apiserver
kubeadm init phase control-plane controller-manager
kubeadm init phase control-plane scheduler

# 生成的文件:
# /etc/kubernetes/manifests/
# ├── kube-apiserver.yaml
# ├── kube-controller-manager.yaml
# └── kube-scheduler.yaml
```

#### 阶段 5: etcd（生成 etcd 静态 Pod 清单）

```bash
# 生成 etcd（仅本地 etcd）
kubeadm init phase etcd local --config kubeadm-init-config.yaml

# 生成的文件:
# /etc/kubernetes/manifests/
# └── etcd.yaml
```

#### 阶段 6: upload-config（上传配置到 ConfigMap）

```bash
# 上传配置
kubeadm init phase upload-config all --config kubeadm-init-config.yaml

# 或逐个上传:
kubeadm init phase upload-config kubeadm  # kubeadm-config ConfigMap
kubeadm init phase upload-config kubelet  # kubelet-config-1.32 ConfigMap

# 生成的 ConfigMap:
# - kube-system/kubeadm-config        # kubeadm 配置
# - kube-system/kubelet-config-1.32   # Kubelet 配置
```

#### 阶段 7: upload-certs（上传证书到 Secret）

```bash
# 上传证书（用于 HA 集群）
kubeadm init phase upload-certs --upload-certs --config kubeadm-init-config.yaml

# 会打印证书密钥:
# --certificate-key <key>

# 生成的 Secret:
# - kube-system/kubeadm-certs  # 证书（加密存储，2小时后自动删除）
```

#### 阶段 8: mark-control-plane（标记控制平面节点）

```bash
# 标记节点
kubeadm init phase mark-control-plane --config kubeadm-init-config.yaml

# 添加标签和污点:
# 标签:
#   - node-role.kubernetes.io/control-plane=""
#   - node-role.kubernetes.io/master=""  # v1.24- 兼容
# 污点:
#   - node-role.kubernetes.io/control-plane:NoSchedule
```

#### 阶段 9: bootstrap-token（创建 Bootstrap Token）

```bash
# 创建 Bootstrap Token
kubeadm init phase bootstrap-token --config kubeadm-init-config.yaml

# 生成的 Secret:
# - kube-system/bootstrap-token-<token-id>  # Bootstrap Token
```

#### 阶段 10: kubelet-finalize（完成 Kubelet 配置）

```bash
# 完成 Kubelet 配置
kubeadm init phase kubelet-finalize all --config kubeadm-init-config.yaml

# 子阶段:
kubeadm init phase kubelet-finalize experimental-cert-rotation  # 启用证书轮换
```

#### 阶段 11: addon（安装插件）

```bash
# 安装所有插件
kubeadm init phase addon all --config kubeadm-init-config.yaml

# 或逐个安装:
kubeadm init phase addon coredns     # CoreDNS
kubeadm init phase addon kube-proxy  # kube-proxy

# 生成的资源:
# - CoreDNS: Deployment, Service, ConfigMap, ServiceAccount, ClusterRole, ClusterRoleBinding
# - kube-proxy: DaemonSet, ServiceAccount, ClusterRoleBinding, ConfigMap, Role, RoleBinding
```

### 6.2 kubeadm join 阶段

```bash
# 查看所有阶段
kubeadm join phase --help

# kubeadm join 完整阶段流程
kubeadm join k8s-api.example.com:6443 \
  --config kubeadm-join-config.yaml

# 等价于以下阶段:

# 1. preflight - 预检
kubeadm join phase preflight --config kubeadm-join-config.yaml

# 2. control-plane-prepare - 准备控制平面（仅控制平面节点）
kubeadm join phase control-plane-prepare all --config kubeadm-join-config.yaml
# 子阶段:
# - download-certs: 下载证书
# - certs: 生成证书
# - kubeconfig: 生成 kubeconfig
# - control-plane: 生成控制平面静态 Pod

# 3. kubelet-start - 启动 Kubelet
kubeadm join phase kubelet-start --config kubeadm-join-config.yaml
# - 下载 kubelet-config ConfigMap
# - 写入 /var/lib/kubelet/config.yaml
# - 写入 /etc/kubernetes/bootstrap-kubelet.conf
# - 启动 Kubelet

# 4. control-plane-join - 加入控制平面（仅控制平面节点）
kubeadm join phase control-plane-join all --config kubeadm-join-config.yaml
# 子阶段:
# - etcd: 添加 etcd 成员
# - update-status: 更新状态
# - mark-control-plane: 标记节点
```

### 6.3 kubeadm reset 阶段

```bash
# kubeadm reset 阶段
kubeadm reset --config kubeadm-reset-config.yaml

# 等价于:

# 1. preflight - 预检
kubeadm reset phase preflight

# 2. remove-etcd-member - 删除 etcd 成员（控制平面节点）
kubeadm reset phase remove-etcd-member

# 3. cleanup-node - 清理节点
kubeadm reset phase cleanup-node
# - 停止 kubelet
# - 删除 /etc/kubernetes/
# - 删除 /var/lib/kubelet/
# - 删除 /var/lib/etcd/
# - 删除 /etc/cni/net.d/
# - 清理容器
```

### 6.4 配置存储位置

```bash
# 证书
/etc/kubernetes/pki/
/etc/kubernetes/pki/etcd/

# Kubeconfig
/etc/kubernetes/admin.conf
/etc/kubernetes/kubelet.conf
/etc/kubernetes/controller-manager.conf
/etc/kubernetes/scheduler.conf

# 静态 Pod 清单
/etc/kubernetes/manifests/
/etc/kubernetes/manifests/kube-apiserver.yaml
/etc/kubernetes/manifests/kube-controller-manager.yaml
/etc/kubernetes/manifests/kube-scheduler.yaml
/etc/kubernetes/manifests/etcd.yaml

# Kubelet 配置
/var/lib/kubelet/config.yaml
/var/lib/kubelet/kubeadm-flags.env

# etcd 数据
/var/lib/etcd/

# CNI 配置
/etc/cni/net.d/

# Containerd 配置
/etc/containerd/config.toml
```

---

## 7. 生产案例

### 7.1 案例 1: HA 控制平面（Stacked etcd）

**架构**:
- 3 个控制平面节点（master01, master02, master03）
- etcd 运行在控制平面节点上（stacked）
- HAProxy + Keepalived 提供 VIP

**网络规划**:
- VIP: 192.168.100.100
- master01: 10.0.1.10
- master02: 10.0.1.11
- master03: 10.0.1.12
- Pod CIDR: 172.16.0.0/16
- Service CIDR: 10.96.0.0/12

#### 步骤 1: 配置 HAProxy + Keepalived

**所有控制平面节点安装 HAProxy 和 Keepalived**:

```bash
apt-get install -y haproxy keepalived
```

**/etc/haproxy/haproxy.cfg** (所有节点相同):

```haproxy
global
    log /dev/log local0
    log /dev/log local1 notice
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin
    stats timeout 30s
    user haproxy
    group haproxy
    daemon

defaults
    log     global
    mode    tcp
    option  tcplog
    option  dontlognull
    timeout connect 5000
    timeout client  50000
    timeout server  50000

frontend k8s-api
    bind *:6443
    mode tcp
    option tcplog
    default_backend k8s-api

backend k8s-api
    mode tcp
    option tcp-check
    balance roundrobin
    server master01 10.0.1.10:6443 check fall 3 rise 2
    server master02 10.0.1.11:6443 check fall 3 rise 2
    server master03 10.0.1.12:6443 check fall 3 rise 2
```

**/etc/keepalived/keepalived.conf** (master01):

```conf
vrrp_script check_haproxy {
    script "killall -0 haproxy"
    interval 3
    weight -2
    fall 10
    rise 2
}

vrrp_instance VI_1 {
    state MASTER
    interface eth0
    virtual_router_id 51
    priority 101
    advert_int 1
    
    authentication {
        auth_type PASS
        auth_pass k8s_ha_pass
    }
    
    virtual_ipaddress {
        192.168.100.100
    }
    
    track_script {
        check_haproxy
    }
}
```

**/etc/keepalived/keepalived.conf** (master02/master03，priority 改为 100/99):

```conf
# 同上，修改:
state BACKUP
priority 100  # master03 改为 99
```

**启动服务**:

```bash
systemctl enable haproxy keepalived
systemctl start haproxy keepalived
systemctl status haproxy keepalived
```

#### 步骤 2: 初始化第一个控制平面节点

**kubeadm-init-config.yaml** (master01):

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 10.0.1.10
  bindPort: 6443
nodeRegistration:
  name: master01
  criSocket: unix:///var/run/containerd/containerd.sock
  taints:
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule
  kubeletExtraArgs:
    node-labels: "node-role.kubernetes.io/master=,zone=az1"

---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
clusterName: prod-k8s-cluster
controlPlaneEndpoint: "192.168.100.100:6443"  # VIP
imageRepository: registry.aliyuncs.com/google_containers

networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "172.16.0.0/16"
  dnsDomain: cluster.local

etcd:
  local:
    dataDir: /var/lib/etcd
    extraArgs:
      listen-metrics-urls: "http://0.0.0.0:2381"

apiServer:
  certSANs:
    - "192.168.100.100"  # VIP
    - "10.0.1.10"
    - "10.0.1.11"
    - "10.0.1.12"
    - "master01"
    - "master02"
    - "master03"
    - "127.0.0.1"
  extraArgs:
    audit-log-path: /var/log/kubernetes/audit/audit.log
    audit-log-maxage: "30"
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
  extraVolumes:
    - name: audit-policy
      hostPath: /etc/kubernetes/audit-policy.yaml
      mountPath: /etc/kubernetes/audit-policy.yaml
      readOnly: true
      pathType: File
    - name: audit-log
      hostPath: /var/log/kubernetes/audit
      mountPath: /var/log/kubernetes/audit
      pathType: DirectoryOrCreate

controllerManager:
  extraArgs:
    node-monitor-grace-period: "40s"
    pod-eviction-timeout: "1m0s"

scheduler:
  extraArgs: {}
```

**执行初始化**:

```bash
# 初始化
kubeadm init --config kubeadm-init-config.yaml --upload-certs

# 保存输出信息:
# 1. kubeadm join 命令（工作节点）
# 2. kubeadm join 命令（控制平面节点，包含 --certificate-key）

# 配置 kubectl
mkdir -p $HOME/.kube
cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
chown $(id -u):$(id -g) $HOME/.kube/config

# 安装 CNI（Calico）
kubectl apply -f https://docs.projectcalico.org/manifests/calico.yaml
```

#### 步骤 3: 加入第二个控制平面节点

**kubeadm-join-config.yaml** (master02):

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration

discovery:
  bootstrapToken:
    token: "abcdef.0123456789abcdef"  # 从 kubeadm init 输出获取
    apiServerEndpoint: "192.168.100.100:6443"
    caCertHashes:
      - "sha256:xxxx..."  # 从 kubeadm init 输出获取

nodeRegistration:
  name: master02
  criSocket: unix:///var/run/containerd/containerd.sock
  taints:
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule
  kubeletExtraArgs:
    node-labels: "node-role.kubernetes.io/master=,zone=az2"

controlPlane:
  localAPIEndpoint:
    advertiseAddress: 10.0.1.11
    bindPort: 6443
  certificateKey: "xxxx..."  # 从 kubeadm init 输出获取
```

**执行加入**:

```bash
kubeadm join --config kubeadm-join-config.yaml
```

#### 步骤 4: 加入第三个控制平面节点

**同步骤 3，修改 IP 和节点名**

#### 步骤 5: 验证集群

```bash
# 查看节点
kubectl get nodes -o wide

# 查看控制平面 Pod
kubectl get pods -n kube-system -o wide

# 查看 etcd 成员
kubectl -n kube-system exec -it etcd-master01 -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list -w table

# 查看集群健康状态
kubectl get componentstatuses
kubectl get --raw='/readyz?verbose'
```

---

### 7.2 案例 2: HA 控制平面（外部 etcd）

**架构**:
- 3 个控制平面节点（master01, master02, master03）
- 3 个独立 etcd 节点（etcd01, etcd02, etcd03）
- HAProxy + Keepalived 提供 VIP

**网络规划**:
- VIP: 192.168.100.100
- master01: 10.0.1.10
- master02: 10.0.1.11
- master03: 10.0.1.12
- etcd01: 10.0.2.10
- etcd02: 10.0.2.11
- etcd03: 10.0.2.12

#### 步骤 1: 部署外部 etcd 集群

**所有 etcd 节点执行**:

```bash
# 下载 etcd
ETCD_VER=v3.5.9
wget https://github.com/etcd-io/etcd/releases/download/${ETCD_VER}/etcd-${ETCD_VER}-linux-amd64.tar.gz
tar xzf etcd-${ETCD_VER}-linux-amd64.tar.gz
cp etcd-${ETCD_VER}-linux-amd64/etcd* /usr/local/bin/

# 创建目录
mkdir -p /etc/etcd /var/lib/etcd
```

**生成 etcd 证书**（在任意节点执行）:

```bash
# 使用 cfssl 生成证书
cat > ca-config.json <<EOF
{
  "signing": {
    "default": {
      "expiry": "87600h"
    },
    "profiles": {
      "etcd": {
        "usages": ["signing", "key encipherment", "server auth", "client auth"],
        "expiry": "87600h"
      }
    }
  }
}
EOF

cat > ca-csr.json <<EOF
{
  "CN": "etcd",
  "key": {
    "algo": "rsa",
    "size": 2048
  },
  "names": [
    {
      "C": "CN",
      "ST": "Beijing",
      "L": "Beijing",
      "O": "etcd",
      "OU": "System"
    }
  ]
}
EOF

cfssl gencert -initca ca-csr.json | cfssljson -bare ca

cat > etcd-csr.json <<EOF
{
  "CN": "etcd",
  "hosts": [
    "127.0.0.1",
    "10.0.2.10",
    "10.0.2.11",
    "10.0.2.12",
    "etcd01",
    "etcd02",
    "etcd03"
  ],
  "key": {
    "algo": "rsa",
    "size": 2048
  },
  "names": [
    {
      "C": "CN",
      "ST": "Beijing",
      "L": "Beijing",
      "O": "etcd",
      "OU": "System"
    }
  ]
}
EOF

cfssl gencert -ca=ca.pem -ca-key=ca-key.pem -config=ca-config.json -profile=etcd etcd-csr.json | cfssljson -bare etcd

# 分发证书到所有 etcd 节点
for host in etcd01 etcd02 etcd03; do
  scp ca.pem etcd.pem etcd-key.pem $host:/etc/etcd/
done
```

**/etc/systemd/system/etcd.service** (etcd01):

```ini
[Unit]
Description=etcd
Documentation=https://github.com/coreos/etcd
After=network.target

[Service]
Type=notify
ExecStart=/usr/local/bin/etcd \
  --name=etcd01 \
  --data-dir=/var/lib/etcd \
  --listen-peer-urls=https://10.0.2.10:2380 \
  --listen-client-urls=https://10.0.2.10:2379,https://127.0.0.1:2379 \
  --advertise-client-urls=https://10.0.2.10:2379 \
  --initial-advertise-peer-urls=https://10.0.2.10:2380 \
  --initial-cluster=etcd01=https://10.0.2.10:2380,etcd02=https://10.0.2.11:2380,etcd03=https://10.0.2.12:2380 \
  --initial-cluster-token=etcd-cluster \
  --initial-cluster-state=new \
  --cert-file=/etc/etcd/etcd.pem \
  --key-file=/etc/etcd/etcd-key.pem \
  --peer-cert-file=/etc/etcd/etcd.pem \
  --peer-key-file=/etc/etcd/etcd-key.pem \
  --trusted-ca-file=/etc/etcd/ca.pem \
  --peer-trusted-ca-file=/etc/etcd/ca.pem \
  --client-cert-auth \
  --peer-client-cert-auth
Restart=on-failure
RestartSec=5
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

**启动 etcd**:

```bash
systemctl daemon-reload
systemctl enable etcd
systemctl start etcd
systemctl status etcd

# 验证
etcdctl --endpoints=https://10.0.2.10:2379,https://10.0.2.11:2379,https://10.0.2.12:2379 \
  --cacert=/etc/etcd/ca.pem \
  --cert=/etc/etcd/etcd.pem \
  --key=/etc/etcd/etcd-key.pem \
  member list -w table
```

#### 步骤 2: 初始化第一个控制平面节点

**复制 etcd 证书到控制平面节点**:

```bash
mkdir -p /etc/kubernetes/pki/etcd
cp /etc/etcd/ca.pem /etc/kubernetes/pki/etcd/ca.crt
cp /etc/etcd/etcd.pem /etc/kubernetes/pki/apiserver-etcd-client.crt
cp /etc/etcd/etcd-key.pem /etc/kubernetes/pki/apiserver-etcd-client.key
```

**kubeadm-init-config.yaml** (master01):

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 10.0.1.10
  bindPort: 6443
nodeRegistration:
  name: master01
  criSocket: unix:///var/run/containerd/containerd.sock
  taints:
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule

---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "192.168.100.100:6443"

networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "172.16.0.0/16"

etcd:
  external:
    endpoints:
      - https://10.0.2.10:2379
      - https://10.0.2.11:2379
      - https://10.0.2.12:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

apiServer:
  certSANs:
    - "192.168.100.100"
    - "10.0.1.10"
    - "10.0.1.11"
    - "10.0.1.12"
```

**执行初始化**:

```bash
kubeadm init --config kubeadm-init-config.yaml --upload-certs
```

---

### 7.3 案例 3: 离线部署

**准备工作**:
1. 下载镜像并推送到内网镜像仓库
2. 配置内网 DNS
3. 准备离线安装包

#### 步骤 1: 准备镜像

```bash
# 在联网机器上导出镜像
kubeadm config images list --kubernetes-version v1.32.0
# registry.k8s.io/kube-apiserver:v1.32.0
# registry.k8s.io/kube-controller-manager:v1.32.0
# registry.k8s.io/kube-scheduler:v1.32.0
# registry.k8s.io/kube-proxy:v1.32.0
# registry.k8s.io/coredns/coredns:v1.11.1
# registry.k8s.io/pause:3.9
# registry.k8s.io/etcd:3.5.9-0

# 拉取镜像
kubeadm config images pull --kubernetes-version v1.32.0

# 打 tag 并推送到内网仓库
for img in $(kubeadm config images list --kubernetes-version v1.32.0); do
  newimg=$(echo $img | sed 's|registry.k8s.io|harbor.local.corp/k8s|')
  docker tag $img $newimg
  docker push $newimg
done

# 导出镜像（可选）
docker save -o k8s-images-v1.32.0.tar \
  registry.k8s.io/kube-apiserver:v1.32.0 \
  registry.k8s.io/kube-controller-manager:v1.32.0 \
  registry.k8s.io/kube-scheduler:v1.32.0 \
  registry.k8s.io/kube-proxy:v1.32.0 \
  registry.k8s.io/coredns/coredns:v1.11.1 \
  registry.k8s.io/pause:3.9 \
  registry.k8s.io/etcd:3.5.9-0

# 在离线环境导入
docker load -i k8s-images-v1.32.0.tar
```

#### 步骤 2: 配置 containerd 使用内网镜像仓库

**/etc/containerd/config.toml**:

```toml
version = 2

[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "harbor.local.corp/k8s/pause:3.9"
  
  [plugins."io.containerd.grpc.v1.cri".registry]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = ["https://harbor.local.corp"]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.k8s.io"]
        endpoint = ["https://harbor.local.corp/k8s"]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."quay.io"]
        endpoint = ["https://harbor.local.corp/quay"]
    
    [plugins."io.containerd.grpc.v1.cri".registry.configs]
      [plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.local.corp"]
        [plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.local.corp".tls]
          insecure_skip_verify = true
        [plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.local.corp".auth]
          username = "admin"
          password = "Harbor12345"
```

```bash
systemctl restart containerd
```

#### 步骤 3: kubeadm 配置

**kubeadm-init-config.yaml**:

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 192.168.1.10
  bindPort: 6443
nodeRegistration:
  criSocket: unix:///var/run/containerd/containerd.sock
  kubeletExtraArgs:
    pod-infra-container-image: "harbor.local.corp/k8s/pause:3.9"

---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
imageRepository: harbor.local.corp/k8s

networking:
  podSubnet: "10.244.0.0/16"

dns:
  imageRepository: harbor.local.corp/coredns
  imageTag: v1.11.1

etcd:
  local:
    imageRepository: harbor.local.corp/k8s
    imageTag: 3.5.9-0
```

**执行初始化**:

```bash
kubeadm init --config kubeadm-init-config.yaml
```

---

## 总结

### kubeadm 配置最佳实践

1. **生产环境必须**:
   - HA 控制平面（至少 3 节点）
   - 外部 etcd 或定期备份
   - 配置审计日志
   - 启用证书自动轮换
   - 使用 VIP/负载均衡器

2. **安全加固**:
   - 关闭匿名访问
   - 启用 RBAC
   - 配置 Pod Security Policy
   - 加密 Secret 数据
   - 定期更新证书

3. **性能优化**:
   - 调整 API Server 并发数
   - 配置 etcd 存储限额
   - 优化 Controller Manager 并发数
   - 配置合理的资源预留

4. **监控运维**:
   - 配置 Prometheus 监控
   - 收集审计日志
   - 定期备份 etcd
   - 制定证书续期流程

---

**相关文档**:
- [01 - Pod YAML 参考](./01-pod-yaml-reference.md)
- [15 - PersistentVolume 和 PersistentVolumeClaim YAML 参考](./15-pv-pvc-yaml-reference.md)
- [20 - ServiceAccount 和 RBAC YAML 参考](./20-serviceaccount-rbac-yaml-reference.md)

**官方文档**:
- [kubeadm Reference](https://kubernetes.io/docs/reference/setup-tools/kubeadm/)
- [Creating Highly Available Clusters with kubeadm](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/high-availability/)
