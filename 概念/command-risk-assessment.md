---
title: 命令风险分级与安全生产规范
description: 云原生运维命令的五级风险评估标准、危险命令清单与生产操作红线
summary: 云原生运维命令的五级风险评估标准、危险命令清单与生产操作红线
category: concepts
tags:
- concepts
- security
- production
- risk-assessment
- command-safety
- visibility/public
tier: supporting
sources:
- kubernetes.io
- etcd.io
- internal-production-runbooks
created: '2026-07-01'
last_updated: 2026-07-01
difficulty: intermediate
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 命令风险分级与安全生产规范

> 本文件是 KUDIG 语料库中**所有命令的风险评估基准**。各知识页的命令代码块按此标准标注风险等级。
> 生产环境执行任何 🟠 及以上风险命令前，必须遵循本规范的前置检查与审批流程。

## 1. 风险分级定义（五级）

| 等级 | 标识 | 含义 | 审批要求 |
|------|------|------|----------|
| 🔴 灾难性 | `[RISK: 灾难性]` | 不可逆 / 全局破坏 / 数据丢失 | 变更窗口 + 双人复核 + 事前备份 + 回滚方案 |
| 🟠 高危 | `[RISK: 高]` | 影响业务流量 / 节点状态 / 内核运行 | 变更工单 + 影响评估 + 计划回滚 |
| 🟡 中危 | `[RISK: 中]` | 变更集群资源状态（可重建） | 确认目标 + 建议 `--dry-run` |
| 🟢 低危 | `[RISK: 低]` | 只读查询，无副作用 | 无 |
| ⚪ 无风险 | — | 无状态命令（echo/export/cd） | 无 |

> 🟢 低危（kubectl get/describe/logs/top、docker ps 等）为安全生产友好操作，全库不逐块标注，统一归为本级。

## 2. 标注格式约定

### 2.1 横幅（代码块前）

```markdown
```

### 2.2 行内注释（仅 🔴 灾难性命令）

```bash
kubectl delete namespace prod-app

```

## 3. 🔴 灾难性命令清单（最高优先级管控）

| 命令模式 | 风险说明 | 前置检查 | 回滚 |
|----------|----------|----------|------|
| `kubectl delete ns/<name>` `kubectl delete namespace <name>` | **永久删除**命名空间及其下**全部**资源（Deployment/Service/PVC/Secret/ConfigMap...），不可恢复 | `kubectl get all,cm,secret,pvc -n <ns>`；确认非 `kube-system`/`kube-public`；生产须工单+双人 | 无，需从备份/etcd快照恢复 |
| `etcdctl snapshot restore` | 用快照**覆盖** etcd 数据目录，集群状态强制回退到快照时刻 | 停止所有 apiserver；确认快照时间点；当前数据先备份 | 用覆盖前的数据目录恢复 |
| `etcdctl member remove` | 从集群移除 etcd 成员，误删多数派会**导致集群不可用 / 数据丢失** | 确认成员数 ≥3 且保留多数派；确认非 leader | 重新 `member add` 并同步 |
| `kubeadm reset` | 清理节点上**所有** K8s 配置/证书/CNI/网络规则，节点脱离集群 | 确认节点已 `kubectl delete node`；已 drain；非控制面（或已降级） | 重新 `kubeadm join/init` |
| `kubectl delete pod <p> --force --grace-period=0` | 强制删除 Pod，**跳过**优雅终止（preStop/hooks/数据刷盘），可能丢数据 | 确认 Pod 无状态或可容忍数据丢失；非 StatefulSet | kube-scheduler 重新调度 |
| `kubectl delete <res> --all [-n <ns>]` | 批量删除某类**全部**资源，误删波及面巨大 | 先 `--dry-run=client`；明确资源范围；确认命名空间 | 重新 apply 声明式清单 |
| `rm -rf /` `rm -rf /var/...` `rm -rf $.../*` | 删除系统/数据文件，可能**摧毁节点或丢失全部数据** | 二次确认路径；生产禁用通配；改用 `mv` 备份替代 | 从备份/镜像重建 |
| `docker system prune -af` `docker rm -f` `docker rmi -f` | 强制清理镜像/容器/网络/卷，正在运行的容器会被杀 | 确认无运行中容器依赖；`docker volume ls` 确认卷 | 重新拉取镜像/重建容器 |
| `helm uninstall <release>` | 删除 release 及其释放的所有资源 | `helm get all <release>` 确认；检查 hook 资源是否保留 | `helm rollback`（需 release 历史未清） |
| `kubectl delete pv/pvc --all` | 批量删除持久卷，**可能永久丢失存储数据** | 确认 PV 的 `persistentVolumeReclaimPolicy`；先备份卷数据 | 从存储后端快照恢复 |

## 4. 🟠 高危命令清单

| 命令模式 | 风险说明 | 前置检查 |
|----------|----------|----------|
| `kubectl drain <node>` | 驱逐节点上所有 Pod，业务流量受影响 | 确认副本数/可用节点充足；检查 PDB；低峰期执行 |
| `kubectl cordon <node>` | 标记节点不可调度（不驱逐现有 Pod） | 确认集群有其他可调度节点 |
| `kubectl taint nodes ...` | 添加/移除污点，影响后续 Pod 调度 | 确认 tolerations 匹配；影响范围评估 |
| `kubectl scale ... --replicas=0` | 工作负载缩容到 0，**立即停服** | 确认非生产或计划内停服；通知下游 |
| `sysctl -w <key>=<val>` | 实时修改内核参数，**全局**生效，错误值可致内核不稳定/网络异常 | 先 `sysctl -a | grep` 确认可选值；记录原值用于回滚；避免在流量高峰改 |
| `systemctl stop/restart/disable <svc>` | 停止/重启系统服务（kubelet/containerd/docker/etcd），影响节点上所有容器 | 确认服务依赖；评估容器重启连锁；低峰期 |
| `chmod -R` `chown -R`（系统目录） | 递归改权限/属主，误操作**破坏系统文件访问**导致服务无法启动 | 二次确认路径；避免对 `/etc /var/lib /usr` 操作 |
| `iptables -F` `iptables -P INPUT DROP` | 清空/改防火墙规则，**可能立即断网（含 SSH）** | 确认控制台/带外通道可用；先保存规则 `iptables-save` |

## 5. 🟡 中危写操作清单

| 命令模式 | 建议 |
|----------|------|
| `kubectl apply/create/replace` | 先 `--dry-run=client -o yaml` 审查；确认目标命名空间 |
| `kubectl delete <res> <name>`（普通） | 确认资源名拼写；声明式资源可由 apply 重建 |
| `kubectl edit/patch` | 修改运行中资源，建议 `kubectl diff` 或先 patch `--dry-run` |
| `kubectl label/annotate` | 改变元数据可能影响选择器/控制器行为 |
| `helm upgrade/install` | 先 `helm upgrade --dry-run`；检查 values diff |
| `kubectl exec` | 进入容器执行任意命令，可能改变容器状态 |
| `kubectl rollout undo/restart` | 触发滚动变更，影响 workload 副本 |

## 6. 生产操作红线

以下操作在生产环境**必须**满足全部条件方可执行：

1. **变更窗口**：🔴 命令仅在批准的维护窗口执行
2. **双人复核**：🔴🟠 命令需第二人确认（命令 + 目标）
3. **事前备份**：涉及 etcd/PVC/数据库的 🔴 命令必须先备份
4. **回滚预案**：每个 🔴🟠 命令须有书面回滚步骤
5. **影响广播**：执行前通知受影响团队
6. **窗口守护**：变更期间 SRE 在线监控，异常立即回滚

## 7. 相关参考

- [[command-doc-map|命令文档映射]]
- [[安全/README|安全与合规]]
- [[可靠性/README|可靠性工程（备份/恢复/DR）]]
- [[生产运维/README|生产运维]]

```

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
