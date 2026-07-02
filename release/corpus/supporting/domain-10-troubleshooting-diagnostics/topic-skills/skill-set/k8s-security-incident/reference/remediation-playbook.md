---
title: Security Incident Response Playbook
summary: Security Incident Response Playbook：识别 → 遏制 → 根除 → 恢复 → 总结
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-security-incident
last_updated: 2026-05-22
---



# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-SEC-002 v1.0 — Security Incident Response
> **⚠️ L1-advisory**: 所有修复操作仅作为建议，必须由安全团队审批后人工执行。

## 目录

- [响应阶段](#响应阶段)
- [遏制措施](#遏制措施)
  - [🔴 高风险 — 需安全团队审批](#-高风险)
    - [REM-001 隔离受感染节点](#rem-001)
    - [REM-002 吊销凭证和令牌](#rem-002)
    - [REM-003 阻断恶意镜像](#rem-003)
    - [REM-004 撤销异常 RBAC](#rem-004)
    - [REM-005 回滚配置变更](#rem-005)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 响应阶段

```
识别 → 遏制 → 根除 → 恢复 → 总结
```

**Agent 职责范围**: 仅协助"识别"阶段的信息收集。"遏制"及后续阶段由安全团队主导。

## 遏制措施

### 🔴 高风险

> **所有以下操作需安全团队书面审批后方可执行。**

#### REM-001: 隔离受感染节点

- **适用根因**: RC-001（容器逃逸）
- **前置检查**:
  ```bash
  kubectl get nodes
  kubectl get pods --all-namespaces --field-selector spec.nodeName=<infected-node>
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # Step 1: Cordon 节点（阻止新 Pod 调度）
  kubectl cordon <infected-node>

  # Step 2: 驱逐非系统 Pod
  kubectl drain <infected-node> --ignore-daemonsets --delete-emptydir-data --force --timeout=300s

  # Step 3: 标记节点用于取证
  kubectl label node <infected-node> security.quarantine=true --overwrite
  ```
- **安全检查**:
  - 确认被驱逐的 Pod 可以在其他节点运行
  - 保留节点用于取证，不要立即清理
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  kubectl uncordon <infected-node>
  kubectl label node <infected-node> security.quarantine-
  ```

#### REM-002: 吊销凭证和令牌

- **适用根因**: RC-004（未授权访问）
- **前置检查**:
  ```bash
  # 查看可疑的 ServiceAccount 令牌使用
  kubectl get events --all-namespaces | grep -i "serviceaccount|token"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除可疑 ServiceAccount
  kubectl delete serviceaccount <suspicious-sa> -n <namespace>

  # 删除关联的 Secret
  kubectl delete secret <suspicious-sa-token> -n <namespace>

  # 如果怀疑 kubeconfig 泄露，轮换集群证书
  kubeadm certs renew all
  # 注意：此操作会影响所有客户端连接
  ```
- **安全检查**:
  - 确认删除不会影响合法服务
  - 提前通知受影响团队
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 重新创建 ServiceAccount
  kubectl create serviceaccount <sa-name> -n <namespace>
  ```

#### REM-003: 阻断恶意镜像

- **适用根因**: RC-003（供应链攻击）
- **前置检查**:
  ```bash
  # 确认恶意镜像标签
  kubectl get pods --all-namespaces -o jsonpath='{range .items[*].spec.containers[*]}{.image}{"\n"}{end}' | sort -u
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 方案 A: 使用 Admission Webhook 阻断
  # 配置 OPA/Gatekeeper 或 Kyverno 策略

  # 方案 B: 删除使用恶意镜像的 Pod
  kubectl get pods --all-namespaces -o json | \
    jq -r '.items[] | select(.spec.containers[].image | test("<malicious-image>")) | "\(.metadata.namespace)/\(.metadata.name)"' | \
    xargs -I {} sh -c 'kubectl delete pod $(echo {} | cut -d/ -f2) -n $(echo {} | cut -d/ -f1)'

  # 方案 C: 从镜像仓库删除恶意镜像
  # 具体命令取决于仓库类型（Harbor、ECR、ACR 等）
  ```
- **安全检查**:
  - 确认阻断的镜像确实是恶意的
  - 准备替代镜像用于恢复
- **回滚命令**:
  ```bash
  # 使用已知良好的镜像重新部署
  kubectl set image deployment/<name> <container>=<safe-image>:<tag> -n <namespace>
  ```

#### REM-004: 撤销异常 RBAC

- **适用根因**: RC-002（权限提升）
- **前置检查**:
  ```bash
  kubectl get clusterrolebinding -o json | jq -r '.items[] | "\(.metadata.name):\(.roleRef.name):\(.subjects[]? | "\(.kind):\(.name)")"'
  kubectl get rolebinding --all-namespaces -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name):\(.roleRef.name):\(.subjects[]? | "\(.kind):\(.name)")"'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除可疑的 clusterrolebinding
  kubectl delete clusterrolebinding <suspicious-binding>

  # 删除可疑的 rolebinding
  kubectl delete rolebinding <suspicious-binding> -n <namespace>

  # 删除可疑的 ClusterRole/Role
  kubectl delete clusterrole <suspicious-role>
  kubectl delete role <suspicious-role> -n <namespace>
  ```
- **安全检查**:
  - 删除前导出配置备份
  - 确认删除不会影响系统组件
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f <backup-of-role>.yaml
  ```

#### REM-005: 回滚配置变更

- **适用根因**: RC-005（内部威胁/配置漂移）
- **前置检查**:
  ```bash
  # 查看最近的配置变更
  kubectl get events --all-namespaces | grep -i "patch|update|delete"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 回滚 Deployment
  kubectl rollout undo deployment/<name> -n <namespace>

  # 回滚 ConfigMap（如果已备份）
  kubectl apply -f <backup-configmap>.yaml

  # 如果需要，从 Git 恢复配置
  # git checkout <commit> -- <config-files>
  # kubectl apply -f <config-files>
  ```
- **安全检查**:
  - 确认回滚版本是安全的
  - 通知变更相关方
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<name> -n <namespace>
  ```

## 验证确认

### 即时验证

```bash
# V1: 无特权容器
kubectl get pods --all-namespaces -o json | jq -r '.items[] | select(.spec.containers[]?.securityContext?.privileged == true) | "\(.metadata.namespace)/\(.metadata.name)"'
# 预期: 无输出

# V2: 无主机命名空间共享
kubectl get pods --all-namespaces -o json | jq -r '.items[] | select(.spec.hostNetwork == true or .spec.hostPID == true) | "\(.metadata.namespace)/\(.metadata.name)"'
# 预期: 仅系统组件（如有）

# V3: cluster-admin 绑定已审查
kubectl get clusterrolebinding -o json | jq -r '.items[] | select(.roleRef.name == "cluster-admin") | .metadata.name'
# 预期: 仅已知的管理员绑定

# V4: 无可疑镜像
kubectl get pods --all-namespaces -o jsonpath='{range .items[*].spec.containers[*]}{.image}{"\n"}{end}' | sort -u
# 预期: 无已知恶意镜像

# V5: 审计日志正常
# 确认审计日志系统仍在正常运行
```

### 解决确认标准

- [ ] 受感染资源已隔离或删除
- [ ] 凭证已轮换
- [ ] 恶意镜像已被阻断
- [ ] 异常权限已撤销
- [ ] 安全监控恢复正常
- [ ] 事后分析报告已启动

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| 确认数据泄露 | 需法务和合规团队介入 |
| 监管报告要求 | PCI-DSS、GDPR、等保等 |
| 攻击持续进行 | 需要 24/7 安全运营中心支持 |

### 升级消息模板

```
【SECURITY INCIDENT】K8s Cluster - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 事件级别: {severity}
- 发现时间: {detection_time}
- 影响范围: 
  - 受影响节点: {affected_nodes}
  - 受影响命名空间: {affected_namespaces}
  - 疑似泄露数据: {data_exposure}
- 初步评估:
  - 攻击向量: {attack_vector}
  - 根因假设: {suspected_root_cause}
- 已采取遏制措施: {containment_actions}
- 需要: {action_needed}
- Skill 版本: SKILL-SEC-002 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 证据保留清单

安全事件响应过程中必须保留以下证据：

1. **容器镜像快照**: `crictl inspect <container-id>`
2. **节点日志**: `/var/log` 和 journalctl
3. **审计日志**: [[Kubernetes|Kubernetes]] Audit Logs
4. **网络流量**: 如有抓包，保存 pcap 文件
5. **内存转储**: 如进行，保存到安全存储
6. **时间线**: 详细的事件时间线记录

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub
