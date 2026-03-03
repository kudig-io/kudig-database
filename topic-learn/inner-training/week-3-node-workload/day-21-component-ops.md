# Day 21: K8S 组件运维

> **学习时间**: 4-5 小时 | **主题**: 核心组件状态检查与故障处理

---

## 今日目标

- [ ] 理解 ACK 托管版与专有版的组件架构差异
- [ ] 掌握核心组件 (apiserver / etcd / controller-manager / scheduler) 状态检查
- [ ] 能够排查 kube-system 命名空间中关键组件异常
- [ ] 了解 ACK 集群组件升级与自定义参数调整

---

## 理论学习 (2h)

### 必读文档

1. **ACK 集群组件管理**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/210-ack-cluster-management.md`
   - 重点: 托管版管控面组件 vs 专有版自建组件

2. **K8S 控制面组件**
   - 文件: `../../../domain-04-control-plane/01-api-server.md`
   - 重点: apiserver 核心功能、健康检查端点

3. **组件故障排查**
   - 文件: `../../../domain-12-troubleshooting/01-troubleshooting-overview.md`
   - 重点: 组件级故障定位思路

### 阅读要点

- **ACK 托管版**: 管控面 (apiserver/etcd/controller-manager/scheduler) 由阿里云托管，用户无需维护
- **ACK 专有版**: 管控面运行在用户自己的 ECS 上，需要用户运维
- **kube-system 关键组件**: CoreDNS、kube-proxy、Terway/Flannel CNI、CSI 插件、ARMS/Prometheus 等
- **组件健康检查端点**: `/healthz`、`/livez`、`/readyz`
- **ACK 组件管理**: 通过控制台或 API 管理集群插件 (addon)

---

## 实践任务 (2.5h)

### 任务 1: 集群组件总览与状态检查 (40min)

```bash
# 查看 kube-system 命名空间中的所有 Pod
kubectl get pods -n kube-system -o wide

# 查看各组件 Pod 运行状态
kubectl get pods -n kube-system --sort-by='.status.phase'

# 检查 apiserver 健康状态 (托管版通过 kubeconfig 端点)
kubectl get --raw /healthz
kubectl get --raw /livez
kubectl get --raw /readyz

# 查看集群版本信息
kubectl version --short 2>/dev/null || kubectl version

# 查看集群组件状态 (专有版)
kubectl get componentstatuses 2>/dev/null || echo "托管版不支持此命令"
```

### 任务 2: CoreDNS 检查与排查 (40min)

```bash
# 查看 CoreDNS 运行状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl get svc -n kube-system kube-dns

# 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# DNS 解析测试
kubectl run dns-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- nslookup kubernetes.default

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=30

# CoreDNS 指标检查
kubectl get --raw /api/v1/namespaces/kube-system/pods/$(kubectl get pod -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].metadata.name}'):9153/proxy/metrics 2>/dev/null | head -20
```

### 任务 3: kube-proxy 与网络插件检查 (40min)

```bash
# 查看 kube-proxy 运行状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl get ds -n kube-system kube-proxy

# 查看 kube-proxy 配置
kubectl get configmap kube-proxy -n kube-system -o yaml | head -40

# 查看 CNI 插件状态 (Terway 或 Flannel)
kubectl get pods -n kube-system | grep -E "terway|flannel"
kubectl get ds -n kube-system | grep -E "terway|flannel"

# 查看 CSI 存储插件
kubectl get pods -n kube-system | grep csi
```

### 任务 4: ACK 集群组件 (Addon) 管理 (30min)

```bash
# 通过 API 查看集群已安装的组件
aliyun cs GET /clusters/<cluster_id>/components

# 查看指定组件详情
aliyun cs GET /clusters/<cluster_id>/components/<component_name>

# 升级组件 (如 CoreDNS)
# 注意: 生产环境请先在测试集群验证
aliyun cs POST /clusters/<cluster_id>/components/<component_name>/upgrade

# 查看组件升级状态
aliyun cs GET /clusters/<cluster_id>/components/<component_name>

# 通过控制台路径: 集群详情 → 组件管理 → 查看/升级
```

---

## 费曼复述 (0.5h)

1. **ACK 托管版和专有版在组件运维层面有什么区别？**
2. **如果 CoreDNS 出现故障，集群中会出现什么表现？如何排查？**
3. **kube-proxy 的作用是什么？它的异常会影响哪些功能？**

---

## 今日检验

- [ ] 能列出 kube-system 中的关键组件并检查其状态
- [ ] 能排查 CoreDNS 相关问题
- [ ] 能检查 kube-proxy 和 CNI 插件运行状态
- [ ] 了解 ACK 组件 (Addon) 管理方式

---

## 核心概念总结

| 组件 | 作用 | 托管版维护方 | 检查方式 |
|------|------|-------------|---------|
| apiserver | API 入口 | 阿里云 | `/healthz` `/readyz` |
| etcd | 状态存储 | 阿里云 | 托管版无需关注 |
| controller-manager | 控制循环 | 阿里云 | 托管版无需关注 |
| scheduler | Pod 调度 | 阿里云 | 托管版无需关注 |
| CoreDNS | DNS 解析 | 用户 | `kubectl get pods -n kube-system` |
| kube-proxy | Service 转发 | 用户 | `kubectl get ds kube-proxy` |
| Terway/Flannel | Pod 网络 | 用户 | `kubectl get ds -n kube-system` |
| CSI 插件 | 存储卷 | 用户 | `kubectl get pods -n kube-system` |

---

## 明日预告

恭喜完成第三周学习！请完成 Week 3 自测 (checkpoint.md)，然后进入 Week 4 网络与存储专题。
