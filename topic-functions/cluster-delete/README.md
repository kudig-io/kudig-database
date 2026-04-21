# Cluster Delete — Kubernetes 集群删除源码分析

本模块基于 Kubernetes 官方源码（`kubernetes/kubernetes`），系统梳理集群删除/重置的完整逻辑，涵盖 `kubeadm reset`、节点删除、etcd 成员移除、容器/网络/数据清理等关键流程。

---

## 文档索引

| 文档 | 内容 |
|------|------|
| [01-overview](01-overview.md) | 删除流程总览：入口分析、阶段划分、命令对比 |
| [02-reset](02-reset.md) | kubeadm reset 源码分析：ResetConfiguration、Phase 注册、核心流程 |
| [03-delete-node](03-delete-node.md) | 节点删除流程：kubectl drain/delete、Node 对象生命周期、优雅删除 |
| [04-cleanup](04-cleanup.md) | 清理机制：容器移除、目录清理、kubeconfig 删除、Unmount 逻辑 |
| [05-etcd-cleanup](05-etcd-cleanup.md) | etcd 数据清理与成员移除：RemoveStackedEtcdMember、数据目录、外部 etcd |
| [06-force-delete](06-force-delete.md) | 强制删除与异常场景：--force、跳过阶段、不可达节点处理 |
| [07-ha-delete](07-ha-delete.md) | HA 集群删除注意事项：控制面顺序、etcd 仲裁、负载均衡器清理 |
| [08-cloud-delete](08-cloud-delete.md) | 云厂商集群删除方案对比：EKS/AKS/GKE/ACK/TKE vs kubeadm |
| [09-reset-phase-commands](09-reset-phase-commands.md) | reset 子命令与 Phase 操作速查：phase 详细参数、场景命令、配置文件 |
| [10-security-delete](10-security-delete.md) | 删除时的安全清理：证书/密钥/凭证、etcd 数据擦除、RBAC 残留、systemd |
| [11-network-cleanup](11-network-cleanup.md) | 网络清理详解：CNI 配置、iptables/ipvs 规则、路由、虚拟接口、命名空间 |
| [12-troubleshooting](12-troubleshooting.md) | 删除故障排查手册：reset 卡住、etcd 移除失败、容器/umount/drain 异常 |

---

## 源码参考

- kubeadm reset 入口: `cmd/kubeadm/app/cmd/reset.go`
- reset phase 定义: `cmd/kubeadm/app/cmd/phases/reset/`
  - `preflight.go` — 预检阶段
  - `removeetcdmember.go` — etcd 成员移除
  - `cleanupnode.go` — 节点清理
  - `unmount.go` / `unmount_linux.go` — 卸载逻辑
  - `data.go` — resetData 接口定义
- etcd 操作: `cmd/kubeadm/app/phases/etcd/local.go`
- workflow Runner: `cmd/kubeadm/app/cmd/phases/workflow/runner.go`
- API 类型: `cmd/kubeadm/app/apis/kubeadm/types.go` → `ResetConfiguration`

---

## 版本说明

- 基于 Kubernetes v1.28 - v1.32 源码分析
- `ResetConfiguration` 自 v1beta3 起成为独立 API 类型
- `--cleanup-tmp-dir` 标志自 v1.28 起可用
- `UnmountFlags` 字段用于控制 Linux unmount2() 系统调用行为
