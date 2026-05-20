---
title: 03 - 研发环境部署 (Development Environment Deployment)
description: '# 03 - 研发环境部署 (Development Environment Deployment)'
category: deployment
tags:
- k8s
- deployment
- rolling-update
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- cilium
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 研发环境部署 (Development Environment Deployment) 是什么
- 如何 研发环境部署 (Development Environment Deployment)
trigger_keywords:
- 研发环境部署
- Development
- Environment
- Deployment
- deployment
---


# 03 - 研发环境部署 (Development Environment Deployment)

> **适用场景**: 开发团队、测试环境、CI/CD 集成 | **预计时间**: 2-4 小时 | **复杂度**: ⭐⭐⭐  
> **目标**: 搭建多节点集群 + 镜像仓库 + 监控 + 日志 + CI/CD，为团队提供完整的研发平台

---

## 概述

研发环境面向开发团队的日常协作需求：多人共享集群、隔离的命名空间、私有镜像仓库、监控告警、日志收集、CI/CD 流水线。本文档提供从裸机到完整研发平台的搭建全流程。

**本文你将学会**:
- 使用 kubeadm 搭建多节点集群 (1 Master + N Worker)
- 部署 Harbor 私有镜像仓库
- 配置 RBAC 多团队权限隔离
- 部署 Prometheus + Grafana 监控体系
- 部署 Loki 日志收集系统
- 安装 ArgoCD 实现 GitOps 工作流
- 使用 Kustomize 管理多环境配置

**前置知识**: 建议先完成 [02-单节点部署](./02-single-node-deployment.md)，熟悉系统准备和 kubeadm 基本操作。

---

## 环境定位与规划

### 研发环境体系

```
┌─────────────────────────────────────────────────────────────────┐
│  开发环境 (dev)        │ 功能验证，配置宽松，快速迭代           │
│  ──────────────────── │                                        │
│  • 开发者可自由部署    │ 资源配额较低，允许 debug 模式          │
│  • 自动化测试运行      │ 镜像可以使用 latest tag               │
├────────────────────────┼────────────────────────────────────────┤
│  测试环境 (staging)    │ 集成测试，接近生产配置                 │
│  ──────────────────── │                                        │
│  • QA 团队主导         │ 资源配额适中，使用指定版本镜像          │
│  • 完整功能测试        │ 启用 NetworkPolicy                     │
├────────────────────────┼────────────────────────────────────────┤
│  预生产环境 (pre-prod) │ 用户验收测试，完全复制生产配置          │
│  ──────────────────── │                                        │
│  • 发布前最终验证      │ 与生产环境相同的资源限制和安全策略      │
│  • 性能压测            │ 使用与生产相同的镜像 tag               │
└─────────────────────────────────────────────────────────────────┘
```

### 节点规划

| 节点角色 | 主机名 | IP | CPU | 内存 | 存储 | 说明 |
|---------|--------|-----|-----|------|------|------|
| Master | master-1 | 192.168.10.10 | 4核 | 8GB | 100GB SSD | 控制平面 (开发环境 1 Master 即可) |
| Worker | worker-1 | 192.168.10.11 | 4核 | 8GB | 100GB SSD + 200GB 数据盘 | 运行应用 Pod |
| Worker | worker-2 | 192.168.10.12 | 4核 | 8GB | 100GB SSD + 200GB 数据盘 | 运行应用 Pod |
| Infra | infra-1 | 192.168.10.20 | 4核 | 8GB | 500GB SSD | Harbor 镜像仓库 (可与 Worker 复用) |

> **备注**: 如果资源有限，Harbor 可以部署在 Worker 节点上，或使用 K8s 集群内部署。

### 网络规划

```yaml
# 研发环境网络规划
network_plan:
  node_cidr: "192.168.10.0/24"       # 节点网络 (物理/VM 网段)
  pod_cidr: "10.244.0.0/16"          # Pod 网络 (CNI 分配)
  service_cidr: "10.96.0.0/12"       # Service 网络 (K8s 分配)
  dns_domain: "dev.cluster.local"    # 集群内 DNS 域名

  # IP 分配规划:
  # 192.168.10.1      - 网关
  # 192.168.10.10     - Master
  # 192.168.10.11-19  - Worker 节点
  # 192.168.10.20-29  - 基础设施节点 (Harbor, NFS 等)
  # 192.168.10.100-199 - MetalLB 地址池 (如需 LoadBalancer)
```

---

## 一、搭建 K8s 集群 (kubeadm)

### 1.1 所有节点: 系统准备

> **重要**: 以下操作需要在 **所有节点** (Master + Worker) 上执行。  
> 详细说明参见 [02-单节点部署 → 通用系统准备](./02-single-node-deployment.md)。

```bash
# ===== 在每个节点上执行 (可用 ansible/ssh 批量) =====

# 1. 设置主机名 (每个节点不同)
sudo hostnamectl set-hostname <master-1 / worker-1 / worker-2>

# 2. 配置所有节点的 hosts 解析
cat >> /etc/hosts << 'EOF'
192.168.10.10 master-1
192.168.10.11 worker-1
192.168.10.12 worker-2
EOF

# 3. 关闭 swap
sudo swapoff -a
sudo sed -i '/swap/s/^/#/' /etc/fstab

# 4. 加载内核模块
cat <<EOF | sudo tee /etc/modules-load.d/k8s.conf
overlay
br_netfilter
EOF
sudo modprobe overlay
sudo modprobe br_netfilter

# 5. 配置内核网络参数
cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
EOF
sudo sysctl --system

# 6. 安装 containerd
sudo apt-get update && sudo apt-get install -y containerd
sudo mkdir -p /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
sudo systemctl restart containerd && sudo systemctl enable containerd

# 7. 安装 kubeadm kubelet kubectl
sudo apt-get install -y apt-transport-https ca-certificates curl gpg
sudo mkdir -p -m 755 /etc/apt/keyrings
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.28/deb/Release.key | \
  sudo gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.28/deb/ /' | \
  sudo tee /etc/apt/sources.list.d/kubernetes.list
sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl
sudo apt-mark hold kubelet kubeadm kubectl

# 8. 时间同步
sudo apt-get install -y chrony
sudo systemctl enable chrony && sudo systemctl start chrony
```

### 1.2 Master 节点: 初始化控制平面

```yaml
# 创建 kubeadm 配置文件: kubeadm-config.yaml
# 使用配置文件比命令行参数更清晰、可追溯

cat > kubeadm-config.yaml << 'EOF'
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.28.0          # K8s 版本
controlPlaneEndpoint: "192.168.10.10:6443"  # 控制平面地址 (单 Master 用 IP)
                                     # 多 Master HA 时用 LB VIP 或域名
networking:
  podSubnet: "10.244.0.0/16"        # Pod 网络段 (需与 CNI 配置一致)
  serviceSubnet: "10.96.0.0/12"     # Service 网络段
  dnsDomain: "dev.cluster.local"    # 集群 DNS 域名

apiServer:
  certSANs:                         # API Server 证书的 SAN (Subject Alternative Name)
  - "master-1"                      # 主机名
  - "192.168.10.10"                 # Master IP
  - "127.0.0.1"                     # 本地回环
  - "dev-k8s.example.com"           # 域名 (可选，后续可能用到)
  extraArgs:
    audit-log-path: "/var/log/kubernetes/audit.log"     # 审计日志路径
    audit-log-maxage: "30"                               # 审计日志保留天数
    audit-log-maxbackup: "10"                            # 审计日志最大备份数

controllerManager:
  extraArgs:
    bind-address: "0.0.0.0"         # 监听所有网卡 (方便 Prometheus 采集)

scheduler:
  extraArgs:
    bind-address: "0.0.0.0"         # 同上
EOF
```

```bash
# 在 Master 节点执行初始化
sudo kubeadm init --config kubeadm-config.yaml

# 预期输出 (关键部分):
# Your Kubernetes control-plane has initialized successfully!
# ...
# kubeadm join 192.168.10.10:6443 --token xxxx --discovery-token-ca-cert-hash sha256:xxxx
# ← 记住这条命令! Worker 节点加入时需要

# 配置 kubectl
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config

# 验证
kubectl get nodes
# 预期: master-1 NotReady (等待 CNI 安装)
```

### 1.3 Worker 节点: 加入集群

```bash
# 在每个 Worker 节点执行 (使用 kubeadm init 输出的 join 命令)
sudo kubeadm join 192.168.10.10:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash>

# 预期输出:
# This node has joined the cluster:
# * Certificate signing request was sent to apiserver and a response was received.
# * The Kubelet was informed of the new secure connection details.

# 如果 token 过期了 (默认 24 小时有效)，在 Master 上重新生成
kubeadm token create --print-join-command
# 预期: 输出一条完整的 kubeadm join 命令
```

### 1.4 安装 CNI 网络插件

```bash
# 推荐 Calico (支持 NetworkPolicy，研发环境需要权限隔离)
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.26.0/manifests/calico.yaml

# 等待所有节点 Ready (约 1-2 分钟)
kubectl get nodes -w
# 预期输出:
# NAME       STATUS   ROLES           AGE   VERSION
# master-1   Ready    control-plane   5m    v1.28.0
# worker-1   Ready    <none>          3m    v1.28.0
# worker-2   Ready    <none>          3m    v1.28.0

# 验证所有系统 Pod 正常
kubectl get pods -n kube-system
# 预期: 所有 Pod 都是 Running 或 Completed 状态

# 测试跨节点 Pod 通信
kubectl run test-1 --image=busybox --restart=Never -- sleep 3600
kubectl run test-2 --image=busybox --restart=Never -- sleep 3600
# 等待 Pod Running 后
TEST1_IP=$(kubectl get pod test-1 -o jsonpath='{.status.podIP}')
kubectl exec test-2 -- ping -c 3 $TEST1_IP
# 预期: 3 packets transmitted, 3 received, 0% packet loss
kubectl delete pod test-1 test-2  # 清理
```

### 1.5 安装 Helm (后续组件安装依赖)

```bash
# macOS
brew install helm

# Linux
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash

# 验证
helm version
# 预期: version.BuildInfo{Version:"v3.13.x", ...}

# 添加常用仓库
helm repo add stable https://charts.helm.sh/stable
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add grafana https://grafana.github.io/helm-charts
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo update
```

---

## 二、CNI 网络插件选型

> **备注**: 第 1.4 步已安装 Calico，这里提供详细的选型参考。

| 场景需求 | Calico | Cilium | Flannel | Weave |
|---------|--------|--------|---------|-------|
| **网络策略 (NetworkPolicy)** | 强 | 最强 (L3-L7) | 不支持 | 支持 |
| **性能** | 高 | 最高 (eBPF) | 高 | 中等 |
| **eBPF 支持** | 可选 | 原生 | 否 | 否 |
| **可观察性** | 中等 | 优秀 (Hubble) | 基本 | 中等 |
| **安装复杂度** | 简单 | 中等 | 极简 | 简单 |
| **社区活跃度** | 高 | 很高 | 高 | 一般 |
| **研发环境推荐** | **推荐** | **推荐** | 仅简单场景 | 不推荐 |

**研发环境建议**: Calico (成熟稳定) 或 Cilium (功能强大，但资源占用稍高)。

---

## 三、安装 Ingress Controller

```bash
# 使用 Helm 安装 Nginx Ingress Controller
helm install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --create-namespace \
  --set controller.service.type=NodePort \
  --set controller.service.nodePorts.http=30080 \
  --set controller.service.nodePorts.https=30443
# 参数说明:
# service.type=NodePort  - 通过节点端口暴露 (无需 LoadBalancer)
# nodePorts.http=30080   - 固定 HTTP 端口为 30080 (默认随机)
# nodePorts.https=30443  - 固定 HTTPS 端口为 30443

# 等待就绪
kubectl wait --for=condition=available deployment/ingress-nginx-controller \
  -n ingress-nginx --timeout=120s

# 验证
kubectl get pods -n ingress-nginx
# 预期: ingress-nginx-controller-xxx  Running
kubectl get svc -n ingress-nginx
# 预期: 看到 NodePort 30080 和 30443

# 测试 Ingress: 创建示例
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: test-ingress
  namespace: default
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  ingressClassName: nginx
  rules:
  - host: test.dev.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: kubernetes
            port:
              number: 443
EOF

# 在本机 hosts 添加: 192.168.10.11 test.dev.local
# 浏览器访问: http://test.dev.local:30080
kubectl delete ingress test-ingress  # 清理
```

---

## 四、部署 Harbor 私有镜像仓库

> **Harbor** 是企业级容器镜像仓库，支持镜像签名、漏洞扫描、RBAC 权限、镜像复制。

### 4.1 使用 Helm 部署 Harbor (推荐)

```bash
# 添加 Harbor 仓库
helm repo add harbor https://helm.goharbor.io
helm repo update

# 创建 Harbor 命名空间
kubectl create namespace harbor

# 安装 Harbor
helm install harbor harbor/harbor \
  --namespace harbor \
  --set expose.type=nodePort \
  --set expose.nodePort.ports.http.nodePort=30002 \
  --set expose.nodePort.ports.https.nodePort=30003 \
  --set externalURL=https://192.168.10.11:30003 \
  --set harborAdminPassword=Harbor12345 \
  --set persistence.enabled=true \
  --set persistence.persistentVolumeClaim.registry.size=50Gi \
  --set persistence.persistentVolumeClaim.database.size=5Gi \
  --set trivy.enabled=true
# 参数说明:
# expose.type=nodePort        - 通过 NodePort 暴露
# externalURL                 - Harbor 对外访问地址
# harborAdminPassword         - admin 初始密码
# persistence.enabled=true    - 持久化存储 (生产必须)
# trivy.enabled=true          - 启用 Trivy 漏洞扫描

# 等待所有组件就绪 (约 2-5 分钟)
kubectl get pods -n harbor -w
# 预期: 所有 Pod (core, database, jobservice, portal, redis, registry, trivy) 为 Running

# 访问: https://192.168.10.11:30003
# 用户名: admin  密码: Harbor12345
```

### 4.2 配置节点信任 Harbor (自签名证书)

```bash
# 在每个需要 push/pull 镜像的节点上执行:

# 1. 获取 Harbor 的 CA 证书
kubectl get secret harbor-ingress -n harbor -o jsonpath='{.data.ca\.crt}' | base64 -d > harbor-ca.crt

# 2. 配置 containerd 信任
sudo mkdir -p /etc/containerd/certs.d/192.168.10.11:30003
sudo cp harbor-ca.crt /etc/containerd/certs.d/192.168.10.11:30003/ca.crt

# 3. 配置 Docker 信任 (如果使用 Docker CLI 推送镜像)
sudo mkdir -p /etc/docker/certs.d/192.168.10.11:30003
sudo cp harbor-ca.crt /etc/docker/certs.d/192.168.10.11:30003/ca.crt

# 4. 测试登录
docker login 192.168.10.11:30003 -u admin -p Harbor12345
# 预期: Login Succeeded

# 5. 测试推送镜像
docker pull nginx:alpine
docker tag nginx:alpine 192.168.10.11:30003/library/nginx:alpine
docker push 192.168.10.11:30003/library/nginx:alpine
# 预期: 推送成功
```

---

## 五、安全配置 - RBAC 多团队隔离

### 5.1 Namespace 策略

```bash
# 为每个团队/环境创建独立 Namespace
kubectl create namespace dev-team-a      # A 团队开发环境
kubectl create namespace dev-team-b      # B 团队开发环境
kubectl create namespace staging         # 测试环境
kubectl create namespace pre-production  # 预生产环境

# 为 Namespace 添加标签 (方便管理和 NetworkPolicy)
kubectl label namespace dev-team-a env=dev team=team-a
kubectl label namespace dev-team-b env=dev team=team-b
kubectl label namespace staging env=staging
kubectl label namespace pre-production env=pre-prod
```

### 5.2 RBAC 角色定义

```yaml
# rbac-dev-team.yaml - 开发团队权限配置
cat > rbac-dev-team.yaml << 'EOF'
# --- 1. 开发者角色: 可以管理自己 namespace 的大部分资源 ---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: dev-team-a
  name: developer
rules:
- apiGroups: ["", "apps", "batch"]  # 核心 API + apps + batch
  resources: ["pods", "deployments", "services", "configmaps", "secrets",
              "jobs", "cronjobs", "replicasets", "statefulsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods/log", "pods/exec", "pods/portforward"]  # 调试权限
  verbs: ["get", "create"]
- apiGroups: [""]
  resources: ["events"]
  verbs: ["get", "list", "watch"]  # 只能查看事件
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

---
# --- 2. 绑定到 team-a-developers 组 ---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-a-developer-binding
  namespace: dev-team-a
subjects:
- kind: Group
  name: team-a-developers    # OIDC/LDAP 组名
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: developer
  apiGroup: rbac.authorization.k8s.io

---
# --- 3. 集群只读角色 (所有开发者共享) ---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-reader
rules:
- apiGroups: [""]
  resources: ["namespaces", "nodes"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["storage.k8s.io"]
  resources: ["storageclasses"]
  verbs: ["get", "list", "watch"]
EOF

kubectl apply -f rbac-dev-team.yaml
```

### 5.3 资源配额

```yaml
# resource-quota.yaml - 为每个团队设置资源上限
cat > resource-quota.yaml << 'EOF'
# --- 资源配额: 限制团队可使用的总资源量 ---
apiVersion: v1
kind: ResourceQuota
metadata:
  name: dev-team-a-quota
  namespace: dev-team-a
spec:
  hard:
    requests.cpu: "8"            # 所有 Pod 的 CPU 请求总和上限
    requests.memory: 16Gi        # 所有 Pod 的内存请求总和上限
    limits.cpu: "16"             # 所有 Pod 的 CPU 限制总和上限
    limits.memory: 32Gi          # 所有 Pod 的内存限制总和上限
    pods: "50"                   # 最多 50 个 Pod
    persistentvolumeclaims: "10" # 最多 10 个 PVC
    services.loadbalancers: "2"  # 最多 2 个 LoadBalancer
    services.nodeports: "5"      # 最多 5 个 NodePort

---
# --- 默认限制: 如果 Pod 没有设置 resources，自动添加 ---
apiVersion: v1
kind: LimitRange
metadata:
  name: dev-default-limits
  namespace: dev-team-a
spec:
  limits:
  - default:          # 默认 limits (没设置时自动添加)
      cpu: 500m
      memory: 512Mi
    defaultRequest:   # 默认 requests (没设置时自动添加)
      cpu: 100m
      memory: 128Mi
    max:              # 单个容器最大值
      cpu: "4"
      memory: 8Gi
    min:              # 单个容器最小值
      cpu: 50m
      memory: 64Mi
    type: Container
EOF

kubectl apply -f resource-quota.yaml

# 验证配额
kubectl describe resourcequota dev-team-a-quota -n dev-team-a
# 预期: 显示 Used 和 Hard 列
```

---

## 六、监控告警系统

### 6.1 部署 Prometheus + Grafana (kube-prometheus-stack)

```bash
# 这是最主流的 K8s 监控方案，包含:
# - Prometheus (指标采集和存储)
# - Grafana (可视化仪表盘)
# - Alertmanager (告警路由)
# - node-exporter (节点指标)
# - kube-state-metrics (K8s 对象状态指标)

helm install monitoring prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --set grafana.adminPassword=admin123 \
  --set prometheus.prometheusSpec.retention=15d \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=50Gi \
  --set grafana.persistence.enabled=true \
  --set grafana.persistence.size=10Gi \
  --set alertmanager.alertmanagerSpec.storage.volumeClaimTemplate.spec.resources.requests.storage=5Gi
# 参数说明:
# retention=15d          - 指标数据保留 15 天
# storageSpec            - Prometheus 数据持久化 (50GB)
# grafana.persistence    - Grafana 仪表盘和配置持久化
# alertmanager storage   - Alertmanager 数据持久化

# 等待就绪 (约 2-3 分钟)
kubectl get pods -n monitoring
# 预期: 所有 Pod Running (prometheus, grafana, alertmanager, operator, node-exporter, kube-state-metrics)

# 访问 Grafana (端口转发)
kubectl port-forward svc/monitoring-grafana -n monitoring 3000:80
# 浏览器打开: http://localhost:3000
# 用户名: admin  密码: admin123

# 访问 Prometheus (端口转发)
kubectl port-forward svc/monitoring-kube-prometheus-prometheus -n monitoring 9090:9090
# 浏览器打开: http://localhost:9090

# 或创建 NodePort 方便团队访问
kubectl patch svc monitoring-grafana -n monitoring -p '{"spec":{"type":"NodePort","ports":[{"port":80,"nodePort":30300}]}}'
# 访问: http://192.168.10.11:30300
```

### 6.2 内置仪表盘

安装后自动包含的 Grafana 仪表盘:
- **Kubernetes / Compute Resources / Cluster** - 集群总体资源使用
- **Kubernetes / Compute Resources / Namespace (Pods)** - 按命名空间查看
- **Node Exporter / Nodes** - 节点 CPU、内存、磁盘、网络
- **CoreDNS** - DNS 查询性能
- **etcd** - etcd 性能指标

### 6.3 部署 Loki 日志系统

```bash
# Loki + Promtail: 轻量级日志收集方案 (与 Grafana 完美集成)
helm install loki grafana/loki-stack \
  --namespace monitoring \
  --set promtail.enabled=true \
  --set loki.persistence.enabled=true \
  --set loki.persistence.size=50Gi \
  --set grafana.enabled=false   # 复用已安装的 Grafana

# 等待就绪
kubectl get pods -n monitoring -l app=loki
kubectl get pods -n monitoring -l app=promtail
# 预期: loki-0 Running, promtail-xxx Running (每个节点一个)

# 在 Grafana 中添加 Loki 数据源:
# 1. 打开 Grafana → Configuration → Data Sources → Add data source
# 2. 选择 Loki
# 3. URL: http://loki:3100
# 4. Save & Test

# 查询日志示例 (在 Grafana Explore 中):
# {namespace="dev-team-a"}                      # 按命名空间
# {namespace="dev-team-a", container="nginx"}   # 按容器名
# {namespace="dev-team-a"} |= "error"           # 搜索关键词
```

---

## 七、GitOps 工作流 (ArgoCD)

### 7.1 安装 ArgoCD

```bash
# 创建命名空间
kubectl create namespace argocd

# 安装 ArgoCD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 等待就绪 (约 1-2 分钟)
kubectl get pods -n argocd
# 预期: argocd-server, argocd-repo-server, argocd-application-controller 等都是 Running

# 暴露 ArgoCD UI (NodePort 方式)
kubectl patch svc argocd-server -n argocd -p '{"spec":{"type":"NodePort","ports":[{"port":443,"nodePort":30443}]}}'

# 获取初始 admin 密码
kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d
# 预期: 输出一串随机密码

# 访问: https://192.168.10.11:30443
# 用户名: admin  密码: 上面获取的密码

# (可选) 安装 ArgoCD CLI
# macOS: brew install argocd
# Linux: curl -sSL -o argocd https://github.com/argoproj/argo-cd/releases/latest/download/argocd-linux-amd64 && chmod +x argocd && sudo mv argocd /usr/local/bin/
```

### 7.2 创建 ArgoCD Application

```yaml
# argocd-app.yaml - 定义一个 GitOps 应用
cat > argocd-app.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: dev-app                       # 应用名称
  namespace: argocd                   # ArgoCD 命名空间
spec:
  project: default                    # ArgoCD 项目

  source:
    repoURL: https://github.com/your-org/k8s-manifests.git  # Git 仓库地址
    targetRevision: HEAD              # 分支/Tag/Commit
    path: environments/development    # 仓库中的路径

  destination:
    server: https://kubernetes.default.svc  # 目标集群 (当前集群)
    namespace: dev-team-a             # 目标命名空间

  syncPolicy:
    automated:                        # 自动同步配置
      prune: true                     # 自动删除 Git 中已移除的资源
      selfHeal: true                  # 自动修复集群中的手动更改
      allowEmpty: false               # 不允许删除所有资源
    syncOptions:
    - CreateNamespace=true            # 自动创建命名空间
    - PrunePropagationPolicy=foreground  # 删除资源时的传播策略
    retry:
      limit: 5                        # 同步失败重试次数
      backoff:
        duration: 5s
        maxDuration: 3m0s
EOF

kubectl apply -f argocd-app.yaml
```

---

## 八、多环境配置管理 (Kustomize)

> **Kustomize** 是 K8s 内置的配置管理工具，通过 overlay 机制管理多环境差异。

### 8.1 目录结构

```
k8s-manifests/
├── base/                        # 基础配置 (所有环境共享)
│   ├── kustomization.yaml
│   ├── deployment.yaml
│   ├── service.yaml
│   └── configmap.yaml
├── environments/
│   ├── development/             # 开发环境覆盖
│   │   ├── kustomization.yaml
│   │   └── patches/
│   │       └── deployment-patch.yaml
│   ├── staging/                 # 测试环境覆盖
│   │   ├── kustomization.yaml
│   │   └── patches/
│   │       └── deployment-patch.yaml
│   └── production/              # 生产环境覆盖
│       ├── kustomization.yaml
│       └── patches/
│           └── deployment-patch.yaml
```

### 8.2 配置示例

```yaml
# --- base/kustomization.yaml ---
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - deployment.yaml
  - service.yaml
  - configmap.yaml

# --- base/deployment.yaml ---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
spec:
  replicas: 1
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: app:latest
        ports:
        - containerPort: 8080
        envFrom:
        - configMapRef:
            name: app-config

# --- environments/development/kustomization.yaml ---
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - ../../base                   # 引用基础配置
namePrefix: dev-                 # 为所有资源名添加 dev- 前缀
namespace: dev-team-a            # 设置命名空间
patches:
  - path: patches/deployment-patch.yaml

# --- environments/development/patches/deployment-patch.yaml ---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
spec:
  replicas: 2                    # 开发环境 2 副本
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: "100m"
            memory: "256Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
```

```bash
# 预览渲染结果 (不实际部署)
kubectl kustomize environments/development/

# 部署到集群
kubectl apply -k environments/development/

# ArgoCD + Kustomize 结合:
# 在 ArgoCD Application 的 source.path 指向 environments/development
```

---

## 九、集群日常运维

### 9.1 日常检查脚本

```bash
#!/bin/bash
# daily-check.sh - 研发环境日常巡检脚本
echo "===== 1. 节点状态 ====="
kubectl get nodes -o wide

echo -e "\n===== 2. 异常 Pod ====="
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded | grep -v Completed

echo -e "\n===== 3. 资源使用 ====="
kubectl top nodes
echo ""
kubectl top pods -A --sort-by=memory | head -15

echo -e "\n===== 4. PVC 状态 ====="
kubectl get pvc -A | grep -v Bound

echo -e "\n===== 5. 证书过期时间 ====="
sudo kubeadm certs check-expiration 2>/dev/null || echo "kubeadm not available"

echo -e "\n===== 6. 磁盘使用 ====="
df -h | grep -E '^/dev'

echo -e "\n===== 7. 近期告警 ====="
kubectl get events -A --sort-by='.lastTimestamp' --field-selector type=Warning | tail -10
```

### 9.2 版本升级流程

```bash
# ===== 升级前检查 =====
# 1. 查看当前版本
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}'

# 2. 检查异常 Pod
kubectl get pods -A | grep -v Running | grep -v Completed

# 3. 备份 etcd
ETCDCTL_API=3 sudo etcdctl snapshot save /backup/etcd-pre-upgrade.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# ===== 升级步骤 (以 1.28 → 1.29 为例) =====
# 详细步骤参考 Kubernetes 官方文档: https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/
# 1. 升级 kubeadm
# 2. kubeadm upgrade plan
# 3. kubeadm upgrade apply v1.29.0
# 4. 逐个升级 kubelet (drain → upgrade → uncordon)
```

### 9.3 HPA 自动扩缩容

```yaml
# hpa.yaml - 基于 CPU 的水平自动扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: dev-team-a
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  minReplicas: 2          # 最少 2 个副本
  maxReplicas: 10         # 最多 10 个副本
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70    # CPU 使用率超过 70% 触发扩容
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80    # 内存使用率超过 80% 触发扩容
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60   # 扩容冷却 60 秒 (防止频繁扩容)
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容冷却 300 秒 (防止频繁缩容)
```

---

## 十、开发者入门指南

> **场景**: 新开发者加入团队后，如何快速开始使用研发 K8s 集群。

### 10.1 获取集群访问权限

```bash
# 1. 管理员为开发者创建 kubeconfig
# (通常通过 OIDC/LDAP 认证，这里演示手动方式)

# 管理员在 Master 上执行:
# 生成开发者的 kubeconfig (限定 namespace)
kubectl config view --raw > developer-kubeconfig.yaml
# 将此文件安全地发送给开发者

# 2. 开发者配置本机
mkdir -p ~/.kube
cp developer-kubeconfig.yaml ~/.kube/config

# 3. 验证
kubectl get pods -n dev-team-a
```

### 10.2 推荐开发工具

| 工具 | 类型 | 说明 |
|------|------|------|
| **k9s** | CLI | 交互式终端 K8s 管理工具，类似 htop |
| **Lens** | GUI | K8s 桌面 IDE，可视化管理 |
| **kubectl** | CLI | 官方命令行工具 |
| **kubectx/kubens** | CLI | 快速切换 context/namespace |
| **stern** | CLI | 多 Pod 日志聚合查看 |

```bash
# 安装常用工具 (macOS)
brew install k9s kubectx stern

# k9s: 交互式管理
k9s -n dev-team-a

# kubens: 快速切换 namespace
kubens dev-team-a

# stern: 同时查看多个 Pod 的日志
stern app -n dev-team-a
```

---

## 验收清单

- [ ] 多节点集群搭建完成，所有节点 Ready
- [ ] CNI 网络正常，跨节点 Pod 可通信
- [ ] Helm 安装完成
- [ ] Ingress Controller 部署完成并可访问
- [ ] Harbor 镜像仓库部署完成，可 push/pull 镜像
- [ ] RBAC 权限配置完成，团队间资源隔离
- [ ] 资源配额生效
- [ ] Prometheus + Grafana 监控系统可用
- [ ] Loki 日志收集系统可用
- [ ] ArgoCD 部署完成并可访问 UI
- [ ] 能成功使用 Kustomize 部署多环境应用

---

## 常见问题 (FAQ)

### Q1: Worker 节点 join 失败

```bash
# 检查网络连通性
ping 192.168.10.10
telnet 192.168.10.10 6443

# token 过期重新生成
kubeadm token create --print-join-command

# 如果之前 join 过需要先 reset
sudo kubeadm reset -f
# 然后重新 join
```

### Q2: Harbor Pod 一直 Pending

```bash
# 通常是 StorageClass 问题
kubectl describe pod -n harbor | grep -A 5 Events
# 如果提示 PVC Pending，检查 StorageClass
kubectl get storageclass
# 需要先安装 StorageClass (如 local-path-provisioner)
```

### Q3: Prometheus 数据丢失 (Pod 重启后)

```bash
# 检查 PVC 是否正常绑定
kubectl get pvc -n monitoring
# 如果 PVC 为 Pending，说明存储未配置
# 确保安装时设置了 persistence 参数
```

---

**下一步**: 掌握研发环境后，前往 → [04-production-environment-deployment.md](./04-production-environment-deployment.md) 学习生产级部署。

---

**来源文档**: `domain-1-architecture-fundamentals/12-cluster-deployment-patterns.md`, `domain-9-platform-ops/02-cluster-lifecycle-management.md`, `domain-4-workloads/02-deployment-production-patterns.md`
