# 集群升级进阶: 细节与回滚

## 源码路径

`cmd/kubeadm/app/cmd/upgrade/`
`cmd/kubeadm/app/phases/upgrade/`
`cmd/kubeadm/app/phases/etcd`

---

## 升级前检查

```bash
# kubeadm upgrade plan 输出详解:
kubeadm upgrade plan v1.29.0

# 检查结果:
# 1. 当前版本 vs 目标版本
# 2. 组件镜像是否需要更新
# 3. API 变更是否需要手动处理
# 4. 升级顺序建议
```

---

## etcd 单独升级

etcd 可以与 API Server 分离单独升级:

```bash
# 1. 升级 etcd (每个 control-plane 节点)
#    kubeadm upgrade apply 内部会调用 etcd upgrade
#    如果需要单独升级:
ETCDCTL_API=3 etcdctl snapshot save /tmp/etcd-backup.db

# 2. 手动升级 etcd 镜像 (修改 etcd.yaml 中的 image tag)
kubectl edit pod etcd-master -n kube-system

# 3. 重启 etcd
crictl stop <etcd-pod-id>
crictl rm <etcd-pod-id>

# 4. 等待 etcd 就绪
ETCDCTL_API=3 etcdctl endpoint health
```

---

## Feature Gates 处理

升级过程中需要处理特性门控变化:

```bash
# 1.28 → 1.29 特性变化:
# - SidecarContainers (1.28 默认开启)
# - InPlacePodVerticalScaling (1.27 alpha)

# 如果新版本有默认关闭的 alpha feature:
# kubeadm init 时指定:
--feature-gates=FeatureName=true

# 查看当前特性门控:
kubectl -n kube-system get pod kube-apiserver-master -o jsonpath='{.spec.containers[0].command}' | grep feature-gates
```

---

## 配置文件补丁 (Patches)

kubeadm 支持在升级时注入配置补丁:

```yaml
# InitConfiguration 中定义 patches:
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
spec:
  patches:
    directory: /etc/kubernetes/patches
---
# 补丁文件:
# /etc/kubernetes/patches/kube-apiserver+token.yaml
# /etc/kubernetes/patches/kube-controller-manager+extra-flags.yaml
```

---

## 升级失败回滚

```bash
# 场景: API Server 启动失败
# 原因: 证书/配置错误

# 1. 查看 kubelet 日志
journalctl -u kubelet -f --no-pager

# 2. 恢复静态 Pod manifests
ls /etc/kubernetes/tmp/backup-*/
# 假设备份在 /etc/kubernetes/tmp/backup-1700000000/
cp /etc/kubernetes/tmp/backup-1700000000/manifests/*.yaml /etc/kubernetes/manifests/

# 3. 重启 kubelet
systemctl restart kubelet

# 4. 等待恢复
kubectl get pods -n kube-system
```

---

## 降级注意事项

```
❌ 不支持从 1.29 降级到 1.28
✅ 只支持从 1.28 升级到 1.29

降级方案:
1. 从备份恢复 etcd 数据
2. 从备份恢复所有 manifests
3. 从备份恢复 /var/lib/kubelet/config.yaml
4. 恢复 etcd 镜像版本
```

---

## 升级过程中的数据面中断

```
升级 API Server 期间:
- API Server 短暂不可用 (~30s)
- 已有连接不受影响
- 新建连接失败

Worker 升级期间:
- 该节点上 Pod 不可调度
- 已运行 Pod 继续运行
- 建议: 先 drain 节点再升级

建议升级顺序:
1. 升级所有 control-plane (API Server 不可用时间短)
2. 升级所有 worker 节点 (使用 drain + uncordon)
```

---

## 升级后验证

```bash
# 1. 检查 API Server 健康
kubectl get --raw /healthz

# 2. 检查所有 control-plane Pod
kubectl get pods -n kube-system -l component=kube-apiserver

# 3. 检查 etcd 健康
ETCDCTL_API=3 etcdctl endpoint health

# 4. 检查版本
kubectl version
kubectl get nodes

# 5. 检查证书过期时间
kubeadm alpha certs check-expiration
```

---

## 自动升级 (云厂商)

| 方案 | 说明 |
|------|------|
| GKE | 自动升级最新版本 (可配置维护窗口) |
| EKS | 自动升级次版本，1个月内完成 |
| AKS | 手动触发或计划升级 |
| kubeadm | 手动升级，无自动机制 |

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 升级后 etcd 启动失败 | 数据目录不兼容 | 从快照恢复或降级 etcd 版本 |
| API Server crashloop | 证书 SAN 不正确 | 检查 /etc/kubernetes/pki |
| kubelet 无法重启 | cgroup driver 不匹配 | 修改 /var/lib/kubelet/config.yaml |
| 升级卡住 | 镜像拉取失败 | 检查镜像仓库/代理配置 |
