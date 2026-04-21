# kubeadm 不安装的组件

## kubeadm init 安装了什么

kubeadm init 安装的**仅是最小可运行集群**:

```
✅ API Server (静态 Pod)
✅ kube-controller-manager (静态 Pod)
✅ kube-scheduler (静态 Pod)
✅ etcd (静态 Pod)
✅ kubelet (systemd 服务)
✅ kube-proxy (DaemonSet)
✅ CoreDNS (Deployment)
❌ 网络插件 (CNI) - 必须手动安装
```

---

## 必须手动安装的组件

### 1. CNI 网络插件

```bash
# kubeadm init 完成后，CoreDNS 处于 Pending 状态
# 因为节点之间网络不通

# 安装 Calico
kubectl apply -f https://docs.projectcalico.org/manifests/calico.yaml

# 安装 Cilium
cilium install

# 安装 Flannel
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# 安装 Weave
kubectl apply -f "https://cloud.weave.works/k8s/net?k8s-version=$(kubectl version | base64 | tr -d '\n')"
```

---

### 2. metrics-server (HPA/监控)

```bash
# 安装 metrics-server (用于 kubectl top 和 HPA)
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# 修改 deployment 添加 --kubelet-insecure-tls (测试环境)
kubectl patch deployment metrics-server -n kube-system --type=json \
  -p '[{"op":"add","path":"/spec/template/spec/containers/0/args","value":["--kubelet-insecure-tls"]}]'
```

---

### 3. Kubernetes Dashboard (Web UI)

```bash
# 安装 Dashboard
kubectl apply -f https://raw.githubusercontent.com/kubernetes/dashboard/v2.7.0/aio/deploy/recommended.yaml

# 创建 admin 用户
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ServiceAccount
metadata:
  name: admin-user
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: admin-user
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- kind: ServiceAccount
  name: admin-user
  namespace: kube-system
EOF

# 获取 Token
kubectl -n kube-system create token admin-user
```

---

### 4. Ingress Controller

```bash
# 安装 NGINX Ingress Controller
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-v1.8.0/deploy/static/provider/cloud/deploy.yaml

# 或使用 Helm
helm install ingress-nginx ingress-nginx/ingress-nginx -n ingress-nginx --create-namespace
```

---

### 5. Cert-Manager (证书管理)

```bash
# 安装 cert-manager (用于自动管理 TLS 证书)
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.13.0/cert-manager.yaml
```

---

### 6. External-DNS (DNS 自动同步)

```bash
# external-dns 自动根据 Ingress/Service 创建 DNS 记录
helm install external-dns external-dns/external-dns \
  --set provider=aws \
  --set aws.zone-type=public
```

---

### 7. Cluster Autoscaler (节点弹性伸缩)

```bash
# AWS Cluster Autoscaler
kubectl apply -f https://raw.githubusercontent.com/kubernetes/autoscaler/master/cluster-autoscaler/cloudprovider/aws/examples/cluster-autoscaler-autodiscover.yaml
```

---

## kubeadm 不配置的安全项

### 1. API Server 匿名访问

```bash
# 默认: 匿名访问已启用
# 修改为禁用:
--anonymous-auth=false
```

### 2. API Server AlwaysAuthenticate

```bash
# 建议启用AlwaysPullImages admission
--enable-admission-plugins=AlwaysPullImages,NodeRestriction
```

### 3. PodSecurityPolicy / PodSecurity

```bash
# K8s 1.25+ 需要迁移到 PodSecurity
# 检查是否已配置 PodSecurity
```

---

## kubeadm 不配置的存储

### 默认无 StorageClass

```bash
# 查看 StorageClass
kubectl get storageclass

# 无输出 = 需要手动配置
# 常见云厂商会自动创建
# AWS: gp3 (EBS)
# GCP: standard
# Azure: managed-premium
# 私有环境需要手动配置 (NFS/Ceph/Longhorn)
```

---

## kubeadm 不配置的网络

### NodePort 范围

```bash
# 默认 30000-32767
# 可在 kubeadm init 时修改:
--service-node-port-range=20000-40000
```

### 负载均衡器

```bash
# kubeadm 不配置 LoadBalancer
# 私有环境需要:
# - MetalLB (静态地址分配)
# - kube-vip (VIP 管理)
# 云厂商自动使用 Cloud LB
```

---

## 最小化生产集群清单

```bash
# 1. kubeadm init (基础集群)
# 2. 安装 CNI (网络)
# 3. 安装 metrics-server (监控/HPA)
# 4. 配置 StorageClass (存储)
# 5. 配置 Ingress Controller (入口)
# 6. 配置 LoadBalancer (MetalLB/kube-vip)
# 7. 配置 cert-manager (证书)
# 8. 配置备份 (Velero)
# 9. 配置日志 (ELK/Loki)
# 10. 配置监控 (Prometheus/Grafana)
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| CoreDNS Pending | CNI 未安装 | 安装 Calico/Cilium/Flannel |
| HPA 不工作 | metrics-server 未安装 | `kubectl top pod` 确认 |
| 无法访问 Dashboard | 未创建 admin token | 创建 ServiceAccount 并绑定 cluster-admin |
| 无 StorageClass | 私有环境无默认存储 | 安装 NFS/Longhorn/Ceph |
| NodePort 不工作 | 防火墙阻止 30000-32767 | 开放端口 |
