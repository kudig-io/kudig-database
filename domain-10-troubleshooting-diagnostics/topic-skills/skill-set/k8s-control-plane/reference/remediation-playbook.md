---
title: "Control Plane Failure Remediation Playbook"
category: remediation
skill_set: "k8s-control-plane"
created: "2026-05-22"
updated: "2026-05-22"
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-CTRL-001 v1.0 — Control Plane Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 自动证书轮转](#rem-001)
  - [🟡 中风险](#-中风险)
    - [REM-002 清理 [[etcd|etcd]] 数据/磁盘](#rem-002)
    - [REM-005 重启 Scheduler](#rem-005)
    - [REM-006 重启 Controller Manager](#rem-006)
    - [REM-007 释放控制平面资源](#rem-007)
  - [🔴 高风险](#-高风险)
    - [REM-003 修复 etcd 网络/成员](#rem-003)
    - [REM-004 重启 API Server](#rem-004)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 证书自动轮转 | 可建议自动执行 |
| 中风险 | 🟡 | 组件重启或资源清理 | 建议操作并等待人工审批 |
| 高风险 | 🔴 | 控制平面核心组件操作 | 仅提供操作指导，由人工执行 |

## 修复操作

### 🟢 低风险

#### REM-001: 自动证书轮转

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 检查证书有效期
  openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -enddate
  kubeadm certs check-expiration
  ```
- **执行命令**:
  ```bash
  # 使用 kubeadm 自动轮转所有证书
  kubeadm certs renew all

  # 重启控制平面组件使新证书生效
  # 如果使用 static pods，移动清单文件触发重启
  mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
  sleep 5
  mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/

  mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/
  sleep 5
  mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/

  mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/
  sleep 5
  mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/

  # 更新 kubeconfig
  kubeadm init phase kubeconfig all
  ```
- **后置验证**:
  ```bash
  kubeadm certs check-expiration
  kubectl cluster-info
  ```
- **回滚命令**:
  ```bash
  # 恢复证书备份（如果 kubeadm 前做了备份）
  cp /etc/kubernetes/pki.bak/* /etc/kubernetes/pki/
  ```

### 🟡 中风险

#### REM-002: 清理 etcd 数据/磁盘

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 检查 etcd 数据目录大小
  du -sh /var/lib/etcd
  df -h /var/lib/etcd
  # 检查 etcd 警报
  kubectl exec <etcd-pod> -n kube-system -- etcdctl alarm list
  ```
- **执行命令**:
  ```bash
  # 方案 A: 清理 etcd 告警（NOSPACE）
  kubectl exec <etcd-pod> -n kube-system -- etcdctl alarm disarm
  kubectl exec <etcd-pod> -n kube-system -- etcdctl defrag

  # 方案 B: 扩容 etcd 数据盘
  # 在云环境中扩展挂载盘，然后扩展文件系统
  ```
- **后置验证**:
  ```bash
  kubectl exec <etcd-pod> -n kube-system -- etcdctl endpoint status
  kubectl exec <etcd-pod> -n kube-system -- etcdctl alarm list
  ```

#### REM-005: 重启 Scheduler

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get pods -n kube-system | grep scheduler
  kubectl logs <scheduler-pod> -n kube-system --tail=50
  ```
- **执行命令**:
  ```bash
  # 如果是 static pod
  mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/
  sleep 10
  mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/

  # 如果是 systemd 管理
  systemctl restart kube-scheduler
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n kube-system | grep scheduler
  kubectl create deployment test-scheduler --image=nginx --dry-run=client -o yaml | kubectl apply -f -
  kubectl get pods -l app=test-scheduler
  kubectl delete deployment test-scheduler
  ```

#### REM-006: 重启 Controller Manager

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  kubectl get pods -n kube-system | grep controller-manager
  kubectl logs <controller-manager-pod> -n kube-system --tail=50
  ```
- **执行命令**:
  ```bash
  # static pod
  mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/
  sleep 10
  mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n kube-system | grep controller-manager
  # 检查 Deployment 副本是否被正确管理
  kubectl get deployment -n <namespace>
  ```

#### REM-007: 释放控制平面资源

- **适用根因**: RC-007
- **前置检查**:
  ```bash
  # 在控制平面节点上
  top -bn1 | head -20
  df -h
  free -h
  ```
- **执行命令**:
  ```bash
  # 清理日志
  journalctl --vacuum-time=1d
  find /var/log -name "*.gz" -delete

  # 清理已退出的容器
  crictl rmi --prune
  crictl rm $(crictl ps -a -q)
  ```
- **后置验证**:
  ```bash
  df -h
  free -h
  kubectl get node <control-plane-node>
  ```

### 🔴 高风险

#### REM-003: 修复 etcd 网络/成员

- **适用根因**: RC-003
- **影响说明**: etcd 成员操作可能导致数据丢失或集群分裂。必须在理解 Raft 协议的基础上操作。
- **操作步骤**:
  1. **检查成员状态**:
     ```bash
     kubectl exec <etcd-pod> -n kube-system -- etcdctl member list
     kubectl exec <etcd-pod> -n kube-system -- etcdctl endpoint status --cluster
     ```
  2. **如果成员失联**:
     ```bash
     # 从集群移除失联成员
     kubectl exec <etcd-pod> -n kube-system -- etcdctl member remove <member-id>
     # 在新节点上重新加入
     kubeadm join phase control-plane-join etcd --config <config>
     ```
  3. **如果 etcd 数据损坏**:
     ```bash
     # 从其他健康成员恢复快照
     kubectl exec <healthy-etcd-pod> -n kube-system -- etcdctl snapshot save /tmp/snapshot.db
     # 在问题节点恢复
     etcdctl snapshot restore /tmp/snapshot.db --data-dir=/var/lib/etcd-restored
     ```
- **安全检查**:
  - 确保集群仍有 majority 可用
  - 备份 etcd 数据后再操作
- **回滚方案**:
  - 从 snapshot 恢复

#### REM-004: 重启 API Server

- **适用根因**: RC-004
- **影响说明**: API Server 重启期间集群完全不可操作。HA 集群中一次只重启一个实例。
- **操作步骤**:
  1. **确认 HA 状态**:
     ```bash
     # 确认还有其他健康的 API Server
     kubectl get endpoints kubernetes -o jsonpath='{.subsets[*].addresses[*].ip}'
     ```
  2. **重启**:
     ```bash
     mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
     sleep 10
     mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
     ```
  3. **验证**:
     ```bash
     kubectl cluster-info
     ```

## 验证确认

### 即时验证

```bash
# V1: API Server 可达
kubectl cluster-info

# V2: 控制平面节点 Ready
kubectl get nodes -l node-role.kubernetes.io/control-plane

# V3: 核心 Pod Running
kubectl get pods -n kube-system

# V4: etcd 健康
kubectl exec <etcd-pod> -n kube-system -- etcdctl endpoint health

# V5: 可创建测试 Pod
kubectl run test-cp --image=nginx --rm -i --restart=Never -- /bin/true
```

### 解决确认标准

- [ ] API Server 响应正常
- [ ] 所有控制平面节点 Ready
- [ ] kube-apiserver、etcd、scheduler、controller-manager Pod Running
- [ ] etcd 集群健康且有 leader
- [ ] 新 Pod 可以正常调度和创建
- [ ] 无控制平面相关告警

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| etcd 失去 majority | 需要数据恢复专家介入 |
| API Server 证书链断裂 | 需要手动重建 PKI |
| 控制平面被入侵 | 安全事件响应流程 |

### 升级消息模板

```
【{severity}】Control Plane Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {component} 异常
- 影响范围: 
  - 集群操作: {cluster_operational}
  - 新 Pod 调度: {scheduling_status}
  - 现有工作负载: {workload_impact}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-CTRL-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
