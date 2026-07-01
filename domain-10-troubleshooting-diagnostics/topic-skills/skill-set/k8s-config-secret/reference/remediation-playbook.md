---
title: ConfigMap & Secret Failure Remediation Playbook
summary: ConfigMap & Secret Failure Remediation Playbook：kubectl get configmap <name>
  -n <namespace> -o jsonpath='{.data}' kubectl get secret <name> -n <namespace> -o
  jsonpath='{.data}'
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-config-secret
last_updated: 2026-05-22
---



# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-CFG-001 v1.0 — ConfigMap & Secret Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 修正 key 名](#rem-001)
    - [REM-002 重新编码 Secret](#rem-002)
    - [REM-003 拆分大 ConfigMap](#rem-003)
    - [REM-005 修正挂载路径](#rem-005)
  - [🟡 中风险](#-中风险)
    - [REM-004 重新创建 immutable 配置](#rem-004)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 配置更新 | 可建议自动执行 |
| 中风险 | 🟡 | 重新创建配置（可能触发 Pod 重启） | 建议操作并等待人工审批 |

## 修复操作

### 🟢 低风险

#### REM-001: 修正 key 名

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl get configmap <name> -n <namespace> -o jsonpath='{.data}'
  kubectl get secret <name> -n <namespace> -o jsonpath='{.data}'
  # 对比 Pod spec 中引用的 key 名
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 添加缺失的 key
  kubectl patch configmap <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/data/<key>", "value": "<value>"}]'

  # 方案 B: 修正 Pod 中的 key 引用
  kubectl patch pod <pod> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/containers/0/env/0/valueFrom/configMapKeyRef/key", "value": "<correct-key>"}]'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl get configmap <name> -n <namespace>
  kubectl delete pod <pod> -n <namespace>  # 重建 Pod 使新配置生效
  ```

#### REM-002: 重新编码 Secret

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl get secret <name> -n <namespace> -o jsonpath='{.data.<key>}' | base64 -d
  # 检查解码后的内容是否正确
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 正确创建 Secret（数据会自动 base64 编码）
  kubectl create secret generic <name> \
    --from-literal=<key>=<value> \
    -n <namespace> --dry-run=client -o yaml | kubectl apply -f -

  # 或使用文件
  kubectl create secret generic <name> \
    --from-file=<key>=<path/to/file> \
    -n <namespace> --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  kubectl get secret <name> -n <namespace> -o jsonpath='{.data.<key>}' | base64 -d
  # 预期: 解码后的内容正确
  ```

#### REM-003: 拆分大 ConfigMap

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get configmap <name> -n <namespace> -o yaml | wc -c
  # ConfigMap + Secret 大小限制约 1MiB
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 将大配置拆分为多个 ConfigMap
  kubectl create configmap <name>-part1 --from-file=part1.conf -n <namespace>
  kubectl create configmap <name>-part2 --from-file=part2.conf -n <namespace>

  # 更新 Pod 挂载多个 ConfigMap
  kubectl patch pod <pod> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/volumes/-", "value":
    {"name":"config-part2","configMap":{"name":"<name>-part2"}}}]'
  ```
- **后置验证**:
  ```bash
  kubectl get configmap -n <namespace>
  # 每个 ConfigMap 大小应在限制内
  ```

#### REM-005: 修正挂载路径

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.volumes}'
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[*].volumeMounts}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修正挂载路径
  kubectl patch pod <pod> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/containers/0/volumeMounts/0/mountPath", "value": "/etc/app"}]'

  # 修正 subPath
  kubectl patch pod <pod> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/containers/0/volumeMounts/0/subPath", "value": "app.conf"}]'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get pod <pod> -n <namespace>
  kubectl exec <pod> -n <namespace> -- ls -la <mount-path>
  ```

### 🟡 中风险

#### REM-004: 重新创建 immutable 配置

- **适用根因**: RC-004
- **影响说明**: immutable ConfigMap/Secret 不能被修改，需要删除后重新创建，这会触发引用它们的 Pod 重新创建。
- **操作步骤**:
  1. **备份现有配置**:
     ```bash
     kubectl get configmap <name> -n <namespace> -o yaml > /tmp/<name>-backup.yaml
     ```
  2. **编辑备份文件修改内容**:
     ```bash
     vim /tmp/<name>-backup.yaml
     # 修改 data 中的内容，移除 resourceVersion 和 uid
     ```
  3. **删除并重新创建**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete configmap <name> -n <namespace>
     kubectl apply -f /tmp/<name>-backup.yaml
     ```
  4. **等待 Pod 重建**:
     ```bash
     kubectl get pods -n <namespace> -w
     ```
- **安全检查**:
  - 确认删除期间不会影响生产流量（如有备用 Pod）
  - 备份原始配置
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/<name>-backup.yaml
  ```

## 验证确认

### 即时验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# V1: Pod 不在 CreateContainerConfigError
kubectl get pod <pod> -n <namespace>

# V2: ConfigMap/Secret 存在
kubectl get configmap <name> -n <namespace>
kubectl get secret <name> -n <namespace>

# V3: 容器内配置正确
kubectl exec <pod> -n <namespace> -- cat <mount-path>/<key>
# 预期: 内容正确

# V4: 环境变量正确
kubectl exec <pod> -n <namespace> -- env | grep <ENV_NAME>
# 预期: 值正确
```

### 解决确认标准

- [ ] Pod 状态不为 CreateContainerConfigError
- [ ] 容器内挂载的文件内容与预期一致
- [ ] 环境变量值正确
- [ ] Secret 数据 base64 解码后正确
- [ ] 配置大小在 1MiB 限制内
- [ ] （如使用 immutable）重建后 Pod 正常运行

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| Secret 泄露 | 需要轮换凭据并通知安全团队 |
| 配置变更导致服务不可用 | 可能涉及应用级问题 |

### 升级消息模板

```
【{severity}】ConfigMap & Secret Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {namespace}/{pod} 配置挂载失败
- 配置: {config_name}
- 错误: {error_message}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-CFG-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```


## 参见

- [[remediation-playbook]] — reference 领域核心页面

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub
