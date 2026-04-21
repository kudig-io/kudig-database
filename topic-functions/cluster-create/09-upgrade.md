# 集群升级流程

## 源码路径

`cmd/kubeadm/app/cmd/upgrade/`
`cmd/kubeadm/app/phases/upgrade/`

---

## 升级路径

```
1.24.x → 1.25.x → 1.26.x → 1.27.x → 1.28.x
```

**注意**: 不支持跨大版本升级，必须逐版本升级。

---

## 升级命令总览

```
┌─────────────────────────────────────────────────────────────┐
│  kubeadm upgrade apply v1.x.x   — 升级当前节点 (control-plane)│
│  kubeadm upgrade node           — 升级 worker 节点          │
│  kubeadm upgrade diff           — 查看组件配置差异           │
└─────────────────────────────────────────────────────────────┘
```

---

## kubeadm upgrade node (worker 节点)

```bash
# 在 worker 节点上执行 (不需要 --certificate-key)
kubeadm upgrade node

# 内部执行:
# 1. 备份 /var/lib/kubelet/config.yaml
# 2. 备份 /etc/kubernetes/kubelet.conf
# 3. 升级 kubelet/kubeadm 包
# 4. 重启 kubelet
# 5. 节点重新注册
```

**不需要 --certificate-key**: worker 节点不需要控制面证书，只需要更新 kubelet 配置。

---

## etcd 版本与 Kubernetes 版本兼容性

etcd 版本必须与 Kubernetes 版本匹配:

| Kubernetes | etcd | 备注 |
|-----------|------|------|
| 1.26 | 3.5.x | |
| 1.27 | 3.5.x | |
| 1.28 | 3.5.x | |
| 1.29 | 3.5.x | |

**etcd 不随 Kubernetes 大版本升级而升级**，需要单独升级。

---

## 升级阶段 (apply 节点)

```
┌─────────────────────────────────────────────────────────────┐
│              kubeadm upgrade apply                          │
├─────────────────────────────────────────────────────────────┤
│  1. preflight           预检 (检查版本兼容性)                 │
│  2. certs               备份并更新控制面证书                 │
│  3. control-plane       更新静态 Pod manifests               │
│  4. wait-control-plane  等待新版本组件就绪                   │
│  5. upload-config        上传新的 kubeadm ConfigMap          │
│  6. bootstrap-token     更新 bootstrap token (如果即将过期)   │
└─────────────────────────────────────────────────────────────┘
```

---

## 核心代码: upgrade.go

```go
// cmd/kubeadm/app/cmd/upgrade/apply.go
func RunApply(cmd *cobra.Command, args []string) error {
    // 1. 检查当前版本与目标版本兼容性
    if !canUpgradeVersion(current, new) {
        return errors.New("cannot upgrade from vX to vY across major versions")
    }

    // 2. 备份现有组件配置
    backupDir := fmt.Sprintf("/etc/kubernetes/tmp/backup-%d", time.Now().Unix())

    // 3. 升级 etcd (如果需要)
    if needsEtcdUpgrade() {
        upgradeEtcd()
    }

    // 4. 更新 API Server / Controller Manager / Scheduler manifests
    return upgradeControlPlane(newVersion)
}
```

---

## 证书升级

```go
// cmd/kubeadm/app/phases/upgrade/certs.go
func UpgradeCertificates(cfg *kubeadmapi.InitConfiguration) error {
    // 1. 备份: /etc/kubernetes/pki/* → backup/
    // 2. 重新生成 API Server / etcd 证书 (保留 SAN)
    // 3. 不重新生成 CA (CA 有效期 10 年)
}
```

---

## 静态 Pod 升级

```go
// cmd/kubeadm/app/phases/upgrade/controlplane.go
func UpgradeControlPlaneStaticPods(newImage string) error {
    // 1. 备份现有 manifest: /etc/kubernetes/manifests/*.yaml → backup/
    // 2. 修改 image tag 为新版本
    // 3. kubelet 自动检测变化并重启容器
    // 4. 等待 /healthz 通过
}
```

---

## 升级顺序

```
1. 升级所有 control-plane 节点 (按任意顺序)
    ↓
2. 升级 worker 节点 (可并行)
    ↓
3. 升级 CNI 插件 (如 Calico/Cilium)
    ↓
4. 升级 Ingress Controller / 其他插件
```

---

## 升级 Worker 节点

```bash
# 在 worker 节点执行
kubeadm upgrade node

# 或者在不执行命令的情况下，由 control-plane 触发 drain + upgrade
kubectl drain node <node> --ignore-daemonsets
# 手动升级 kubelet
apt-get install kubelet=1.x.x-1
systemctl restart kubelet
kubectl uncordon node <node>
```

---

## 升级检查

```bash
# 查看可升级版本
kubeadm upgrade plan

# 输出示例:
Components that must be upgraded manually after control-plane upgrade:
  kubelet: 1.28.0 → 1.29.0

Upgrade to the latest stable version:
  kube-apiserver: 1.28.0 → 1.29.0
  kube-controller-manager: 1.28.0 → 1.29.0
  kube-scheduler: 1.28.0 → 1.29.0
  kube-proxy: 1.28.0 → 1.29.0
  etcd: 3.5.x → 3.6.x
```

---

## 版本兼容性矩阵

| 组件 | kubeadm 版本兼容性 |
|------|-------------------|
| kubelet | 可比 API Server 低 2 个小版本 |
| kubectl | 可比 API Server 低/高 1 个小版本 |
| etcd | 与 API Server 同版本 |

---

## 回滚

```bash
# 如果升级失败，手动回滚:
# 1. 恢复静态 Pod manifests
cp /backup/*.yaml /etc/kubernetes/manifests/
# 2. 重启 kubelet
systemctl restart kubelet
# 3. 恢复 etcd 数据 (如果需要)
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| etcd 升级失败 | 数据目录不兼容 | 备份数据，使用 etcd 迁移工具 |
| API Server 无法启动 | etcd 连接失败 | 检查 etcd 健康状态 |
| kubelet 无法注册 | 证书 SAN 不含新 IP | 更新证书 |
