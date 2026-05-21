---
title: K0s
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- flannel
- calico
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K0s 是什么
- 如何 K0s
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- K0s
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- cni-basics
- etcd-basics
- mysql-basics
---

title: K0s
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- flannel
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- K0s 是什么
- 如何 K0s
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- K0s
- cncf
- landscape
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

# K0s

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://k0sproject.io/ |
| **GitHub** | https://github.com/k0sproject/k0s |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |
| **最新版本** | v1.31+ |

---

## 项目概述

K0s 是一个轻量级、全功能的 Kubernetes 发行版，打包为单一二进制文件，零依赖、零摩擦地安装和运行。k0s 的设计目标是简化 Kubernetes 的安装、运维和升级过程，适用于从边缘设备到大规模数据中心的各种场景。

### 核心特性

- **单一二进制文件**: 所有组件打包为一个可执行文件，无外部运行时依赖
- **零摩擦安装**: 无需预装依赖，一条命令即可启动集群
- **FIPS 140-2 合规**: 提供符合 FIPS 标准的构建版本
- **自动化生命周期管理**: 内置 k0sctl 工具实现集群的自动化部署和升级
- **多架构支持**: 支持 x86_64、ARM64、ARMv7 架构
- **可嵌入性**: 可作为 Go 库嵌入其他项目
- **Konnectivity 支持**: 内置 Konnectivity 服务实现控制平面与工作节点的安全通信

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│                  k0s Binary                       │
│                                                   │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ API      │  │ Controller│  │ Scheduler     │  │
│  │ Server   │  │ Manager   │  │               │  │
│  └────┬─────┘  └────┬─────┘  └───────┬───────┘  │
│       │              │                │           │
│  ┌────┴──────────────┴────────────────┴───────┐  │
│  │            Embedded etcd / External DB      │  │
│  │        (etcd / MySQL / PostgreSQL)          │  │
│  └─────────────────────────────────────────────┘  │
│                                                   │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ kubelet  │  │containerd│  │ kube-proxy /   │  │
│  │          │  │          │  │ kube-router    │  │
│  └──────────┘  └──────────┘  └───────────────┘  │
│                                                   │
│  ┌──────────────────────────────────────────┐    │
│  │        Konnectivity (Tunnel)              │    │
│  └──────────────────────────────────────────┘    │
└──────────────────────────────────────────────────┘
```

### 节点角色

| 角色 | 说明 | 运行组件 |
|:---|:---|:---|
| **Controller** | 控制平面节点 | API Server, Controller Manager, Scheduler, etcd |
| **Worker** | 工作节点 | kubelet, containerd, kube-proxy |
| **Controller+Worker** | 混合节点 | 所有组件（适合小型集群） |
| **Single** | 单节点模式 | 所有组件（用于开发测试） |

---

## 快速开始

### 安装 k0s

```bash
# 使用安装脚本
curl -sSLf https://get.k0s.sh | sudo sh

# 验证安装
k0s version
```

### 单节点快速启动

```bash
# 启动单节点集群（包含控制平面和工作负载）
sudo k0s install controller --single
sudo k0s start

# 等待集群就绪
sudo k0s status

# 使用 kubectl
sudo k0s kubectl get nodes
sudo k0s kubectl get pods -A
```

### 多节点集群部署

```bash
# 在控制平面节点上
sudo k0s install controller
sudo k0s start

# 生成 Worker 加入令牌
sudo k0s token create --role=worker > worker-token.txt

# 在 Worker 节点上
sudo k0s install worker --token-file /path/to/worker-token.txt
sudo k0s start
```

---

## 配置详解

### k0s.yaml 配置文件

```yaml
apiVersion: k0s.k0sproject.io/v1beta1
kind: ClusterConfig
metadata:
  name: my-k0s-cluster
spec:
  api:
    # API Server 监听地址
    address: 192.168.1.10
    port: 6443
    # 外部访问地址（用于证书 SAN）
    externalAddress: k8s.example.com
    sans:
      - 192.168.1.10
      - k8s.example.com
  
  storage:
    # 数据存储类型: etcd, kine（支持 MySQL/PostgreSQL/SQLite）
    type: etcd
    etcd:
      peerAddress: 192.168.1.10
  
  network:
    provider: calico  # 可选: kuberouter, calico, custom
    podCIDR: 10.244.0.0/16
    serviceCIDR: 10.96.0.0/12
    calico:
      mode: vxlan  # 可选: vxlan, ipip, bird
      wireguard: false
  
  podSecurityPolicy:
    defaultPolicy: 00-k0s-privileged
  
  telemetry:
    enabled: false
```

### 使用外部数据库（Kine）

```yaml
apiVersion: k0s.k0sproject.io/v1beta1
kind: ClusterConfig
spec:
  storage:
    type: kine
    kine:
      # MySQL
      dataSource: "mysql://user:password@tcp(mysql-host:3306)/k0s"
      # 或 PostgreSQL
      # dataSource: "postgres://user:password@postgres-host:5432/k0s?sslmode=disable"
```

### 高可用 (HA) 集群配置

```yaml
apiVersion: k0s.k0sproject.io/v1beta1
kind: ClusterConfig
spec:
  api:
    externalAddress: k8s-lb.example.com  # 负载均衡器地址
    sans:
      - k8s-lb.example.com
      - 192.168.1.10
      - 192.168.1.11
      - 192.168.1.12
  storage:
    type: etcd
    etcd:
      peerAddress: 192.168.1.10
```

---

## k0sctl 自动化部署

### k0sctl.yaml 配置

```yaml
apiVersion: k0sctl.k0sproject.io/v1beta1
kind: Cluster
metadata:
  name: production-cluster
spec:
  k0s:
    version: "1.31.0+k0s.0"
    config:
      spec:
        network:
          provider: calico
  hosts:
    - role: controller
      ssh:
        address: 192.168.1.10
        user: root
        keyPath: ~/.ssh/id_rsa
      installFlags:
        - --disable-components=metrics-server
    - role: controller
      ssh:
        address: 192.168.1.11
        user: root
        keyPath: ~/.ssh/id_rsa
    - role: controller
      ssh:
        address: 192.168.1.12
        user: root
        keyPath: ~/.ssh/id_rsa
    - role: worker
      ssh:
        address: 192.168.1.20
        user: root
        keyPath: ~/.ssh/id_rsa
    - role: worker
      ssh:
        address: 192.168.1.21
        user: root
        keyPath: ~/.ssh/id_rsa
```

### k0sctl 操作命令

```bash
# 部署集群
k0sctl apply --config k0sctl.yaml

# 获取 kubeconfig
k0sctl kubeconfig --config k0sctl.yaml > ~/.kube/config

# 集群备份
k0sctl backup --config k0sctl.yaml

# 集群升级
k0sctl apply --config k0sctl.yaml  # 更新 version 后重新 apply

# 集群重置
k0sctl reset --config k0sctl.yaml
```

---

## 高级功能

### 系统组件管理

```yaml
# 禁用或启用特定系统组件
spec:
  extensions:
    helm:
      repositories:
        - name: traefik
          url: https://traefik.github.io/charts
      charts:
        - name: traefik
          chartname: traefik/traefik
          version: "25.0.0"
          namespace: traefik
          values: |
            service:
              type: LoadBalancer
            ports:
              web:
                port: 8000
              websecure:
                port: 8443
```

### 离线安装

```bash
# 创建离线安装包
k0s airgap list-images | xargs -I{} docker pull {}
k0s airgap list-images | xargs -I{} docker save {} -o images.tar

# 将镜像导入到目标节点
sudo k0s install controller --enable-worker \
  --config k0s.yaml

# 使用本地镜像 bundle
sudo mkdir -p /var/lib/k0s/images
sudo cp bundle_file /var/lib/k0s/images/bundle.tar
```

### 自动更新 (Autopilot)

```yaml
apiVersion: autopilot.k0sproject.io/v1beta2
kind: Plan
metadata:
  name: autopilot-update
spec:
  id: update-v1.31
  timestamp: "2026-03-01T00:00:00Z"
  commands:
    - k0supdate:
        version: v1.31.0+k0s.0
        targets:
          controllers:
            discovery:
              static:
                nodes:
                  - controller-0
                  - controller-1
          workers:
            discovery:
              static:
                nodes:
                  - worker-0
                  - worker-1
```

---

## 监控与运维

### 集群状态检查

```bash
# 查看集群状态
sudo k0s status

# 查看组件状态
sudo k0s kubectl get componentstatuses

# 查看 etcd 状态
sudo k0s etcd member-list

# 备份 etcd
sudo k0s backup --save-path /backup/
```

### Prometheus 监控集成

```yaml
# 通过 Helm 扩展部署 Prometheus Stack
spec:
  extensions:
    helm:
      repositories:
        - name: prometheus-community
          url: https://prometheus-community.github.io/helm-charts
      charts:
        - name: kube-prometheus-stack
          chartname: prometheus-community/kube-prometheus-stack
          version: "55.0.0"
          namespace: monitoring
          values: |
            prometheus:
              prometheusSpec:
                retention: 15d
                storageSpec:
                  volumeClaimTemplate:
                    spec:
                      accessModes: ["ReadWriteOnce"]
                      resources:
                        requests:
                          storage: 50Gi
            grafana:
              enabled: true
```

---

## 与其他 K8s 发行版对比

| 特性 | k0s | k3s | kubeadm | MicroK8s |
|:---|:---|:---|:---|:---|
| **打包方式** | 单一二进制 | 单一二进制 | 工具集 | Snap 包 |
| **外部依赖** | 无 | 无 | 需要容器运行时 | Snap |
| **默认 CNI** | kube-router/Calico | Flannel | 无 | Calico |
| **默认存储** | etcd | SQLite/etcd | etcd | Dqlite |
| **HA 支持** | 内置 | 内置 | 手动配置 | 内置 |
| **Windows 节点** | 支持 | 不支持 | 支持 | 不支持 |
| **自动更新** | Autopilot | System Upgrade Controller | 手动 | Snap 自动 |

---

## 最佳实践

1. **生产环境部署**: 使用至少 3 个 Controller 节点实现 HA，使用 k0sctl 进行自动化部署
2. **网络选择**: 大规模集群推荐使用 Calico 的 BGP 模式，小型集群使用默认 kube-router
3. **存储后端**: 大规模集群（100+ 节点）考虑使用外部 etcd 或 PostgreSQL
4. **安全加固**: 启用 Pod Security Standards，配置审计日志，定期轮转证书
5. **升级策略**: 使用 Autopilot 实现无中断滚动升级，先升级 Controller 再升级 Worker
6. **备份恢复**: 定期执行 `k0s backup`，存储到外部安全位置

---

## 参考资源

- [k0s 官方文档](https://docs.k0sproject.io/)
- [k0s GitHub 仓库](https://github.com/k0sproject/k0s)
- [k0sctl 自动化工具](https://github.com/k0sproject/k0sctl)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
