---
title: Kubernetes 集群创建操作指南
description: '# Kubernetes 集群创建操作指南'
category: references
tags:
- k8s
- operations
- cluster-create
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- helm
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 集群创建操作指南 是什么
- 如何 Kubernetes 集群创建操作指南
trigger_keywords:
- Kubernetes
- 集群创建操作指南
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
---

# Kubernetes 集群创建操作指南

### 01 Overview

#### 函数/流程签名

```go
func NewCmdInit(out io.Writer, initFlags *initFlags) *cobra.Command
func RunInit(cmd *cobra.Command, args []string, initOptions *InitOptions) error
func NewInitOptions() *InitOptions
func (o *InitOptions) Validate(cmd *cobra.Command) error
func (o *InitOptions) Run() error
func getVariantVersion(kubernetesVersion string, imageRepository string) (string, error)
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/cmd/init.go` | L45-L130 | `NewCmdInit` 命令注册 |
| `cmd/kubeadm/app/cmd/init.go` | L131-L350 | `RunInit` 主入口函数 |
| `cmd/kubeadm/app/cmd/init.go` | L351-L500 | 配置验证和默认值填充 |
| `cmd/kubeadm/app/cmd/phases/init/data/data.go` | L30-L200 | InitData 数据结构 |
| `cmd/kubeadm/app/cmd/phases/init/waitcontrolplane.go` | L25-L120 | 等待控制面就绪 |
| `cmd/kubeadm/app/cmd/phases/workflow/runner.go` | L40-L200 | 阶段执行引擎 |

#### InitOptions 参数

| 参数名 | 类型 | 说明 | 验证规则 |
|--------|------|------|---------|
| `cfgPath` | `string` | 配置文件路径 | 可选，与 CLI 参数互斥 |
| `kubernetesVersion` | `string` | Kubernetes 版本 | 必须是有效 semver (如 v1.28.0) |
| `controlPlaneEndpoint` | `string` | API Server 负载均衡地址 | 格式: host:port |
| `apiserverAdvertiseAddress` | `string` | API Server 广播地址 | 有效 IPv4/IPv6 地址 |
| `apiserverBindPort` | `int32` | API Server 监听端口 | 范围 1-65535，默认 6443 |
| `certificatesDir` | `string` | 证书存储目录 | 默认 /etc/kubernetes/pki |
| `criSocket` | `string` | CRI 运行时 socket 路径 | 默认自动检测 |
| `dryRun` | `bool` | 只打印不执行 | 默认 false |
| `featureGates` | `map[string]bool` | 特性门控 | 键必须是已知的 feature gate |
| `ignorePreflightErrors` | `[]string` | 忽略的预检错误 | 已知的错误代码列表 |
| `imageRepository` | `string` | 镜像仓库地址 | 默认 registry.k8s.io |
| `nodeName` | `string` | 节点名称 | 默认使用 hostname |
| `podNetworkCidr` | `string` | Pod 网络 CIDR | 有效 CIDR (如 10.244.0.0/16) |
| `serviceCidr` | `string` | Service 网络 CIDR | 有效 CIDR (默认 10.96.0.0/12) |
| `serviceDnsDomain` | `string` | 集群 DNS 域名 | 默认 cluster.local |
| `skipPhases` | `[]string` | 跳过的阶段列表 | 已知的 phase 名称 |
| `skipCertificateKeyPrint` | `bool` | 不打印证书密钥 | 默认 false |

---

### 02 Preflight

#### 函数/流程签名

```go
func RunInitMasterChecks(cfg *kubeadmapi.InitConfiguration, ignorePreflightErrors []string) error
func RunJoinNodeChecks(cfg *kubeadmapi.JoinConfiguration, ignorePreflightErrors []string) error
func checkPortOpen(port int) error
func IsContainerRuntimePresent() error
func IsKubernetesVersionSupported(version string) error
func checkSystemVerification() error
func RunChecks(checks []Checker, ignorePreflightErrors sets.Set[string]) error
type Checker interface { Name() string; Check() error }
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/preflight/checks.go` | L40-L200 | Checker 接口和核心检查 |
| `cmd/kubeadm/app/preflight/checks.go` | L201-L500 | 系统级检查函数 |
| `cmd/kubeadm/app/preflight/checks.go` | L501-L800 | 网络和端口检查 |
| `cmd/kubeadm/app/preflight/checks.go` | L801-L1100 | 证书和配置检查 |
| `cmd/kubeadm/app/cmd/init.go` | L200-L300 | init 预检入口 |
| `cmd/kubeadm/app/cmd/join.go` | L150-L250 | join 预检入口 |

#### 预检项目列表

| 检查名称 | 类型 | 说明 | 失败条件 |
|---------|------|------|---------|
| `NumCPU` | 系统 | CPU 核心数 | < 2 核 (init), < 1 核 (join) |
| `Mem` | 系统 | 内存大小 | < 1700MB |
| `Swap` | 系统 | swap 状态 | swap 开启 |
| `FileContent--proc-sys-net-ipv4-ip_forward` | 内核 | IP 转发 | ip_forward=0 |
| `FileContent--proc-sys-net-bridge-bridge-nf-call-iptables` | 内核 | bridge iptables | 未设置 |
| `Port-6443` | 网络 | API Server 端口 | 端口被占用 |
| `Port-2379` | 网络 | etcd 客户端端口 | 端口被占用 |
| `Port-2380` | 网络 | etcd 对等端口 | 端口被占用 |
| `Port-10250` | 网络 | kubelet 端口 | 端口被占用 |
| `Port-10259` | 网络 | scheduler 端口 | 端口被占用 |
| `Port-10257` | 网络 | controller-manager 端口 | 端口被占用 |
| `CRI` | 运行时 | CRI 运行时 | socket 不存在或不可连接 |
| `Service-Docker` | 服务 | Docker 服务 | Docker 运行但非推荐 |
| `IsPrivilegedUser` | 权限 | root 权限 | 非 root 用户 |
| `KubernetesVersion` | 版本 | K8s 版本兼容性 | kubeadm 和 kubelet 版本不匹配 |
| `Firewalld` | 网络 | 防火墙状态 | firewalld 运行中 |
| `DirAvailable--etc-kubernetes-manifests` | 文件 | manifest 目录 | 目录已存在且非空 |
| `DirAvailable--var-lib-etcd` | 文件 | etcd 数据目录 | 目录已存在且非空 |

---

### 03 Certs

#### 函数/流程签名

```go
func NewPKI(cfg *kubeadmapi.InitConfiguration) (*pkiutil.Certificates, error)
func GenerateRootCA(cfg *kubeadmapi.InitConfiguration) error
func CreateCertAndKeyFiles(caCert *x509.Certificate, caKey crypto.Signer, certConfig *certutil.Config) error
func CreateServiceAccountKeyPair(keyPath, pubPath string) error
func RenewCerts(cfg *kubeadmapi.InitConfiguration) error
func CheckCertExpiration(certDir string) error
func LoadCertificate(certPath string) (*x509.Certificate, error)
func ValidateCertPeriod(cert *x509.Certificate, currentTime time.Time) error
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/phases/certs/certs.go` | L30-L200 | 证书生成主入口 |
| `cmd/kubeadm/app/phases/certs/rootca.go` | L25-L150 | 根 CA 生成 |
| `cmd/kubeadm/app/phases/certs/apiserver.go` | L30-L200 | API Server 证书生成 |
| `cmd/kubeadm/app/phases/certs/etcd.go` | L25-L250 | etcd 证书生成 |
| `cmd/kubeadm/app/phases/certs/frontproxy.go` | L25-L100 | front-proxy 证书 |
| `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` | L40-L500 | PKI 工具函数 |
| `cmd/kubeadm/app/util/pkiutil/csr.go` | L30-L150 | CSR 生成 |
| `staging/src/k8s.io/client-go/util/cert/cert.go` | L30-L300 | 证书工具库 |

#### 证书生成参数

| 参数名 | 类型 | 说明 | 验证规则 |
|--------|------|------|---------|
| `certificatesDir` | `string` | 证书存储目录 | 默认 `/etc/kubernetes/pki`，必须可写 |
| `caCertFile` | `string` | CA 证书文件路径 | 必须是 PEM 格式 |
| `caKeyFile` | `string` | CA 私钥文件路径 | 必须是 PEM 格式 RSA/ECDSA 密钥 |
| `cfg.APIEndpoint.AdvertiseAddress` | `string` | API Server 广播地址 | 有效 IPv4/IPv6 |
| `cfg.APIServer.CertSANs` | `[]string` | API Server 证书 SAN | 可包含 DNS/IP/URI |
| `cfg.Etcd.Local` | `LocalEtcd` | 本地 etcd 配置 | 包含服务器/对等/客户端证书配置 |

---

### 04 Kubeconfig

#### 概述

kubeconfig 是 Kubernetes 客户端工具（kubectl、helm、控制器等）连接 API Server 的配置文件。它包含了集群的访问地址、CA 证书、用户身份证书等关键信息。在 `kubeadm init` 过程中，kubeconfig 阶段负责为集群管理员、kubelet、Controller Manager 和 Scheduler 四个身份生成各自的 kubeconfig 文件。

每个 kubeconfig 文件对应一个特定的身份（Identity），这个身份由证书中的 Common Name（CN）和 Organization（O）字段决定。API Server 的 RBAC 授权系统根据这些身份信息来决定该客户端可以执行哪些操作。

理解 kubeconfig 的生成逻辑对于以下场景至关重要：

- **权限管理**：理解每个组件的权限来源和范围
- **故障排查**：kubeconfig 配置错误是常见的连接问题
- **安全审计**：追踪哪些身份拥有集群管理权限
- **多集群管理**：kubeconfig 的合并和切换机制

本文档详细分析四类 kubeconfig 文件的生成逻辑、各组件的身份映射、核心源码实现以及 kubeconfig 管理的最佳实践。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubeconfig 生成 | `cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go` | 生成逻辑 |
| kubeconfig 工具 | `cmd/kubeadm/app/util/kubeconfig/` | 辅助函数 |
| client-go 配置 | `staging/src/k8s.io/client-go/tools/clientcmd/` | kubeconfig 解析 |
| API 类型 | `staging/src/k8s.io/client-go/tools/clientcmd/api/` | kubeconfig API |
| 证书工具 | `cmd/kubeadm/app/util/pkiutil/` | 证书操作 |

---

#### 1.1 四类 kubeconfig 文件

| 文件 | 用途 | 使用者 | 证书身份 |
|------|------|--------|---------|
| `admin.conf` | 集群管理 | kubectl, helm | `O=system:masters, CN=kubernetes-admin` |
| `kubelet.conf` | 节点连接 API Server | kubelet | `O=system:nodes, CN=system:node:<name>` |
| `controller-manager.conf` | CM 连接 API Server | kube-controller-manager | `CN=system:kube-controller-manager` |
| `scheduler.conf` | Scheduler 连接 API Server | kube-scheduler | `CN=system:kube-scheduler` |

---

### 05 Control Plane

#### 函数/流程签名

```go
func CreateStaticPodManifests(cfg *kubeadmapi.InitConfiguration) error
func CreateAPIServerManifest(cfg *kubeadmapi.InitConfiguration) error
func CreateControllerManagerManifest(cfg *kubeadmapi.InitConfiguration) error
func CreateSchedulerManifest(cfg *kubeadmapi.InitConfiguration) error
func CreateEtcdManifest(cfg *kubeadmapi.InitConfiguration) error
func getAPIServerCommand(cfg *kubeadmapi.InitConfiguration) []string
func getControllerManagerCommand(cfg *kubeadmapi.InitConfiguration) []string
func getSchedulerCommand(cfg *kubeadmapi.InitConfiguration) []string
func getEtcdCommand(cfg *kubeadmapi.InitConfiguration) []string
func waitForControlPlane(timeout time.Duration) error
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/phases/controlplane/manifests.go` | L35-L250 | 静态 Pod manifest 生成 |
| `cmd/kubeadm/app/phases/controlplane/manifests.go` | L251-L450 | API Server 命令参数构建 |
| `cmd/kubeadm/app/phases/controlplane/manifests.go` | L451-L600 | Controller Manager 参数 |
| `cmd/kubeadm/app/phases/controlplane/manifests.go` | L601-L700 | Scheduler 参数 |
| `cmd/kubeadm/app/phases/etcd/local.go` | L30-L200 | etcd manifest 生成 |
| `cmd/kubeadm/app/phases/controlplane/wait.go` | L25-L120 | 等待控制面就绪 |
| `cmd/kubeadm/app/util/staticpod/utils.go` | L30-L200 | 静态 Pod 工具函数 |

#### API Server 启动参数

| 参数名 | 类型 | 说明 | 默认值 |
|--------|------|------|--------|
| `--advertise-address` | `string` | 广播地址 | 节点 IP |
| `--bind-address` | `string` | 监听地址 | `0.0.0.0` |
| `--secure-port` | `int` | HTTPS 端口 | `6443` |
| `--etcd-servers` | `[]string` | etcd 集群地址 | `https://127.0.0.1:2379` |
| `--service-cluster-ip-range` | `string` | Service CIDR | `10.96.0.0/12` |
| `--client-ca-file` | `string` | 客户端 CA 文件 | `/etc/kubernetes/pki/ca.crt` |
| `--tls-cert-file` | `string` | TLS 证书文件 | `/etc/kubernetes/pki/apiserver.crt` |
| `--tls-private-key-file` | `string` | TLS 私钥文件 | `/etc/kubernetes/pki/apiserver.key` |
| `--kubelet-client-certificate` | `string` | kubelet 客户端证书 | `/etc/kubernetes/pki/apiserver-kubelet-client.crt` |
| `--kubelet-client-key` | `string` | kubelet 客户端私钥 | `/etc/kubernetes/pki/apiserver-kubelet-client.key` |
| `--authorization-mode` | `string` | 授权模式 | `Node,RBAC` |
| `--enable-admission-plugins` | `string` | 启用的准入插件 | `NodeRestriction` |
| `--service-account-signing-key-file` | `string` | SA 签名密钥 | `/etc/kubernetes/pki/sa.key` |
| `--service-account-issuer` | `string` | SA 签发者 | `https://kubernetes.default.svc.cluster.local` |
| `--allow-privileged` | `bool` | 允许特权容器 | `true` |

---

### 06 Join

#### 函数/流程签名

```go
func NewCmdJoin(out io.Writer, joinFlags *joinFlags) *cobra.Command
func RunJoin(cmd *cobra.Command, args []string, joinOptions *JoinOptions) error
func (o *JoinOptions) Run(data *joinData) error
func discoveryFor(cfg *kubeadmapi.JoinConfiguration) (*clientset.Clientset, error)
func loadDiscoveryBootstrapToken(cfg *kubeadmapi.JoinConfiguration) (*clientset.Clientset, error)
func TLSBootstrap(cfg *kubeadmapi.JoinConfiguration, client clientset.Interface) error
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/cmd/join.go` | L50-L250 | `RunJoin` 主入口 |
| `cmd/kubeadm/app/phases/join/discovery.go` | L30-L200 | 集群发现机制 |
| `cmd/kubeadm/app/phases/join/controlplanejoin.go` | L30-L250 | control-plane join |
| `cmd/kubeadm/app/phases/kubelet/config.go` | L40-L200 | kubelet 配置写入 |
| `cmd/kubeadm/app/phases/bootstraptoken/node/token.go` | L30-L150 | Bootstrap Token |

#### JoinConfiguration 参数

| 参数名 | 类型 | 说明 | 验证规则 |
|--------|------|------|---------|
| `discovery.bootstrapToken.apiServerEndpoint` | `string` | API Server 地址 | host:port 格式 |
| `discovery.bootstrapToken.token` | `string` | Bootstrap Token | `[a-z0-9]{6}.[a-z0-9]{16}` |
| `discovery.bootstrapToken.caCertHashes` | `[]string` | CA 证书哈希 | `sha256:<hex>` |
| `discovery.timeout` | `*metav1.Duration` | 发现超时 | 默认 5 分钟 |
| `nodeRegistration.criSocket` | `string` | CRI socket 路径 | 有效 socket 路径 |
| `nodeRegistration.name` | `string` | 节点名称 | 默认 hostname |
| `controlPlane` | `*JoinControlPlane` | 控制面加入配置 | 含 certificateKey |

---

### 07 Etcd

#### 函数/流程签名

```go
func CreateLocalEtcdStaticPodManifest(cfg *kubeadmapi.InitConfiguration) error
func getEtcdCommand(cfg *kubeadmapi.InitConfiguration) []string
func waitForEtcd(client clientset.Interface, timeout time.Duration) error
func checkEtcdHealth(endpoint string, certsDir string) error
func getEtcdPodSpec(cfg *kubeadmapi.InitConfiguration) (*v1.Pod, error)
func CreateEtcdStaticPodManifestHA(cfg *kubeadmapi.InitConfiguration, endpoints []string) error
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/phases/etcd/local.go` | L35-L250 | 本地 etcd manifest 生成 |
| `cmd/kubeadm/app/phases/etcd/local.go` | L251-L400 | HA etcd manifest 生成 |
| `cmd/kubeadm/app/util/etcd/etcdutil.go` | L30-L200 | etcd 工具函数 |
| `cmd/kubeadm/app/util/etcd/etcdutil.go` | L201-L350 | etcd 健康检查 |
| `staging/src/k8s.io/apiserver/pkg/storage/storagebackend/factory/etcd3.go` | L50-L300 | API Server → etcd 连接 |

#### etcd 启动参数

| 参数名 | 类型 | 说明 | 默认值 |
|--------|------|------|--------|
| `--name` | `string` | etcd 成员名称 | 节点主机名 |
| `--data-dir` | `string` | 数据存储目录 | `/var/lib/etcd` |
| `--listen-client-urls` | `[]string` | 客户端监听地址 | `https://127.0.0.1:2379,https://<ip>:2379` |
| `--listen-peer-urls` | `[]string` | 对等通信监听地址 | `https://<ip>:2380` |
| `--advertise-client-urls` | `[]string` | 客户端广播地址 | `https://<ip>:2379` |
| `--initial-advertise-peer-urls` | `[]string` | 对等广播地址 | `https://<ip>:2380` |
| `--initial-cluster` | `string` | 初始集群成员 | `<name>=https://<ip>:2380` |
| `--initial-cluster-token` | `string` | 集群 token | `etcd-cluster` |
| `--initial-cluster-state` | `string` | 初始状态 | `new` / `existing` |
| `--client-cert-auth` | `bool` | 客户端证书认证 | `true` |
| `--cert-file` | `string` | 服务端证书 | `/etc/kubernetes/pki/etcd/server.crt` |
| `--key-file` | `string` | 服务端私钥 | `/etc/kubernetes/pki/etcd/server.key` |
| `--peer-cert-file` | `string` | 对等证书 | `/etc/kubernetes/pki/etcd/peer.crt` |
| `--peer-key-file` | `string` | 对等私钥 | `/etc/kubernetes/pki/etcd/peer.key` |
| `--peer-client-cert-auth` | `bool` | 对等客户端证书认证 | `true` |
| `--trusted-ca-file` | `string` | 受信 CA 文件 | `/etc/kubernetes/pki/etcd/ca.crt` |
| `--peer-trusted-ca-file` | `string` | 对等受信 CA | `/etc/kubernetes/pki/etcd/ca.crt` |
| `--snapshot-count` | `int` | 快照阈值 | `10000` |
| `--heartbeat-interval` | `string` | 心跳间隔 | `500ms` (推荐 100-500ms) |
| `--election-timeout` | `string` | 选举超时 | `5000ms` (推荐 1000-5000ms) |

---

### 08 Ha

#### 概述

高可用（High Availability, HA）是生产环境 Kubernetes 集群的基本要求。单控制面节点的集群存在单点故障风险——一旦控制面节点宕机，集群将无法创建新的 Pod、处理 API 请求或进行调度决策。高可用控制面通过部署多个控制面节点和负载均衡器来消除单点故障，确保集群在部分节点故障时仍能正常提供服务。

kubeadm 支持两种高可用 etcd 拓扑：

- **Stacked etcd（堆叠模式）**：etcd 运行在控制面节点上，与 API Server 等组件共享同一台机器。这是 kubeadm 推荐的方式，部署简单，但 etcd 和控制面组件共享资源。
- **External etcd（外部模式）**：etcd 运行在独立的节点上，与控制面节点分离。这种方式性能更好，隔离性更强，但需要额外的机器和维护成本。

两种模式都使用负载均衡器来分发 API Server 的流量。`--control-plane-endpoint` 参数是 HA 配置的核心，所有组件的 kubeconfig 文件都通过它连接到负载均衡器而非单个 API Server。

本文档详细分析 kubeadm HA 的架构设计、配置方法、新增控制面节点的流程、etcd 高可用机制以及 Controller Manager 和 Scheduler 的 Leader Election。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 控制面 Phase | `cmd/kubeadm/app/cmd/phases/init/controlplane.go` | 静态 Pod 生成 |
| Join 命令 | `cmd/kubeadm/app/cmd/join.go` | 控制面 join 逻辑 |
| 证书上传 | `cmd/kubeadm/app/phases/uploadconfig/` | upload-certs |
| etcd 管理 | `cmd/kubeadm/app/phases/etcd/` | etcd 成员管理 |
| kubeconfig | `cmd/kubeadm/app/phases/kubeconfig/` | kubeconfig 生成 |
| Leader Election | `staging/src/k8s.io/client-go/tools/leaderelection/` | 选主机制 |

---

#### 1.1 Stacked etcd 模式



---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[skills/troubleshoot-node-issues.md|节点故障排查]]
- [[references/k8s-knowledge-map.md|K8s 知识图谱]]
- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[docker]] — Docker
- [[entities/kubelet.md|kubelet]] — kubelet
- [[helm]] — Helm
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
