---
title: "Image Pull Failure Remediation Playbook"
category: remediation
skill_set: "k8s-image-pull"
created: "2026-05-22"
updated: "2026-05-22"
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-IMG-001 v1.0 — Image Pull Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 修正镜像标签](#rem-001)
    - [REM-004 清理节点磁盘](#rem-004)
    - [REM-005 使用多平台镜像](#rem-005)
  - [🟡 中风险](#-中风险)
    - [REM-002 更新镜像仓库认证](#rem-002)
    - [REM-003 检查网络连通性](#rem-003)
    - [REM-006 处理仓库限流](#rem-006)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 镜像配置调整 | 可建议自动执行 |
| 中风险 | 🟡 | 网络或认证变更可能影响其他服务 | 建议操作并等待人工审批 |

## 修复操作

### 🟢 低风险

#### REM-001: 修正镜像标签

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].image}'
  # 确认镜像名称和标签
  ```
- **执行命令**:
  ```bash
  # 方案 A: 修正 Deployment 中的镜像标签
  kubectl set image deployment/<name> <container>=<correct-image>:<tag> -n <namespace>

  # 方案 B: 如果 tag 不存在，回滚到上一个已知好的版本
  kubectl rollout undo deployment/<name> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get pod <pod> -n <namespace>
  # 预期: 无 ImagePullBackOff
  ```
- **回滚命令**:
  ```bash
  kubectl set image deployment/<name> <container>=<old-image>:<old-tag> -n <namespace>
  ```

#### REM-004: 清理节点磁盘

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl describe node <node> | grep -A 5 "DiskPressure"
  # 或 SSH 到节点
  df -h /var/lib/containerd
  ```
- **执行命令**:
  ```bash
  # 在节点上执行
  crictl rmi --prune
  journalctl --vacuum-time=2d
  # 如需更多清理，参考 SKILL-NODE-001 REM-002
  ```
- **后置验证**:
  ```bash
  kubectl get node <node>
  # 预期: DiskPressure=False
  ```

#### REM-005: 使用多平台镜像

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get node <node> -o jsonpath='{.status.nodeInfo.architecture}'
  # 确认节点架构 (amd64/arm64)
  ```
- **执行命令**:
  ```bash
  # 方案 A: 使用支持多平台的镜像（如官方镜像通常支持）
  kubectl set image deployment/<name> <container>=<multiarch-image>:<tag> -n <namespace>

  # 方案 B: 为 arm64 节点指定 arm64 镜像
  kubectl set image deployment/<name> <container>=<image>:<tag>-arm64 -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l app=<label>
  # 预期: Pod 进入 Running
  ```

### 🟡 中风险

#### REM-002: 更新镜像仓库认证

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.\\.dockerconfigjson}' | base64 -d | jq .
  # 确认认证信息是否正确
  ```
- **执行命令**:
  ```bash
  # 方案 A: 重新创建 docker-registry secret
  kubectl create secret docker-registry <secret-name> \
    --docker-server=<registry-server> \
    --docker-username=<username> \
    --docker-password=<password> \
    --docker-email=<email> \
    -n <namespace> --dry-run=client -o yaml | kubectl apply -f -

  # 方案 B: 关联 secret 到 serviceaccount
  kubectl patch serviceaccount <sa-name> -n <namespace> -p \
    '{"imagePullSecrets":[{"name":"<secret-name>"}]}'
  ```
- **后置验证**:
  ```bash
  kubectl get pod <pod> -n <namespace>
  # 预期: 重新创建后无 ImagePullBackOff
  ```

#### REM-003: 检查网络连通性

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 在节点上测试
  curl -v https://<registry-server>/v2/
  # 或从 Pod 测试
  kubectl run test --rm -i --restart=Never --image=busybox -n <namespace> -- wget -O- https://<registry-server>/v2/ 2>&1
  ```
- **执行命令**:
  ```bash
  # 检查节点防火墙/安全组规则
  # 检查 DNS 解析
  kubectl run test --rm -i --restart=Never --image=busybox -n <namespace> -- nslookup <registry-server>
  # 检查是否使用了代理
  kubectl get node <node> -o yaml | grep -i proxy
  ```
- **后置验证**:
  ```bash
  # 重新创建 Pod 测试拉取
  kubectl delete pod <pod> -n <namespace>
  # 等待重建后检查状态
  ```

#### REM-006: 处理仓库限流

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  kubectl describe pod <pod> -n <namespace> | grep -A 5 "Events"
  # 查找 rate limit 或 429 错误
  ```
- **执行命令**:
  ```bash
  # 方案 A: 等待限流窗口重置（通常 1-6 小时）
  # 方案 B: 使用镜像缓存（如 harbor 作为 pull-through proxy）
  # 方案 C: 升级 registry 账户（如 Docker Hub Pro）
  # 方案 D: 切换到不限流的镜像仓库
  kubectl set image deployment/<name> <container>=<alternative-registry>/<image>:<tag> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get pod <pod> -n <namespace>
  # 预期: 限流解除后 Pod 正常启动
  ```

## 验证确认

### 即时验证

```bash
# V1: Pod 状态正常
kubectl get pod <pod> -n <namespace>
# 预期: Running 或 ContainerCreating（非 ImagePullBackOff）

# V2: 无镜像拉取失败事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod>
# 预期: 无 ImagePullBackOff、ErrImagePull、pull denied 事件

# V3: 容器已启动
kubectl get pod <pod> -n <namespace> -o jsonpath='{.status.containerStatuses[0].state}'
# 预期: {"running":{...}} 或 {"terminated":{...}}

# V4: 业务功能正常
# 通过应用健康检查端点或日志确认
```

### 解决确认标准

- [ ] Pod 状态不为 ImagePullBackOff 或 ErrImagePull
- [ ] 容器成功启动（state 为 running 或 terminated）
- [ ] 无镜像拉取相关失败事件
- [ ] 应用日志正常输出（非因镜像问题退出）

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| 镜像仓库完全不可用 | 所有节点无法访问仓库 |
| 凭证泄露风险 | 怀疑 registry secret 被泄露 |
| 供应链安全问题 | 镜像被篡改或存在漏洞 |

### 升级消息模板

```
【{severity}】Image Pull Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {namespace}/{pod} 无法拉取镜像
- 镜像: {image}
- 错误: {error_message}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-IMG-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
