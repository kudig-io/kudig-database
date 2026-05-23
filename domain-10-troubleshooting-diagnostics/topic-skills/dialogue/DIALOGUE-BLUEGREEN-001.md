---
dialogue_id: "DIALOGUE-BLUEGREEN-001"
skill_id: "SKILL-BLUEGREEN-001"
role: "remote-consultant"
language: "zh"
severity: "high"
status: "reviewed"
created: 2026-05-21
updated: 2026-05-21
title: "蓝绿部署切换后服务不可用 — 远程顾问对话脚本"
category: dialogue
tags: ["dialogue", "remote-consultant", "troubleshooting", "visibility/public"]
---

# 蓝绿部署切换后服务不可用 — 远程顾问对话脚本

> 对应概念：[[concepts/blue-green-deployment|蓝绿部署]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：执行蓝绿部署切换流量后，服务不可用，用户报告大量错误。

**顾问回应**：收到。这是紧急问题。请先确认：当前 Service selector 指向的是蓝色还是绿色版本？影响范围是否涉及生产流量？

---

### 步骤 1: 确认蓝绿两个 Deployment 都 Running

**顾问**：请确认蓝色和绿色两个版本的 Deployment 状态：

```bash
kubectl get deployment -n <namespace>
```

> **如果无法执行**：请通过控制台查看 Deployment 列表，确认蓝色和绿色版本的 READY 列是否均为 `<replicas>/<replicas>`。

```bash
kubectl get pods -n <namespace> -l version=blue
kubectl get pods -n <namespace> -l version=green
```

> **如果无法执行**：请根据实际标签（如 `app=myapp,version=blue`）调整 selector 后重试，或提供所有 Pod 的列表。

**预期用户回复**：绿色版本的 Pod 未全部就绪，或存在 CrashLoopBackOff 的 Pod。

**下一步判断**：
- 若绿色 Pod 未就绪 → 进入步骤 4 检查 readinessProbe
- 若绿色 Pod 全部就绪 → 进入步骤 2 检查 Service selector

---

### 步骤 2: 检查 Service Selector

**顾问**：请确认 Service 的 selector 当前指向了哪个版本：

```bash
kubectl get svc <svc-name> -n <namespace> -o yaml | grep -A 5 'selector:'
```

> **如果无法执行**：请将 Service YAML 中的 `spec.selector` 部分复制发给我。

```bash
kubectl get endpoints <svc-name> -n <namespace>
```

> **如果无法执行**：请确认 Endpoints 中的 IP 是否属于绿色版本的 Pod。若 Endpoints 为空，说明 selector 未匹配到任何 Pod。

**预期用户回复**：Service selector 已切换为 `version: green`，但 Endpoints 中缺少绿色 Pod 的 IP。

**下一步判断**：
- 若 Endpoints 为空或不完整 → 进入步骤 3 验证绿环境健康
- 若 Endpoints 完整但服务仍不可用 → 进入步骤 4 检查 readinessProbe

---

### 步骤 3: 验证绿环境健康

**顾问**：请验证绿色版本 Pod 的实际状态：

```bash
kubectl get pods -n <namespace> -l version=green -o wide
```

> **如果无法执行**：请提供绿色 Pod 的 STATUS 和 RESTARTS 列信息。

```bash
kubectl logs -n <namespace> -l version=green --tail=50
```

> **如果无法执行**：请选取单个绿色 Pod 执行 `kubectl logs <green-pod> -n <namespace> --tail=50`。

**预期用户回复**：绿色 Pod 处于 Running 状态，但日志中有启动错误或连接数据库失败的记录。

**下一步判断**：
- 若日志显示应用启动失败 → 进入步骤 6 修复方案（修复绿环境代码/配置）
- 若日志正常但 readinessProbe 失败 → 进入步骤 4 检查探针

---

### 步骤 4: 检查 ReadinessProbe

**顾问**：请检查绿色版本 Pod 的就绪探针配置：

```bash
kubectl describe pod <green-pod> -n <namespace> | grep -A 10 'Conditions:'
```

> **如果无法执行**：请执行 `kubectl get pod <green-pod> -n <namespace> -o yaml | grep -A 15 'readinessProbe:'`。

```bash
kubectl get pod <green-pod> -n <namespace> -o yaml | grep -A 15 'readinessProbe:'
```

> **如果无法执行**：请将 Pod YAML 中的 readinessProbe 配置复制发给我，重点关注 `path`、`port`、`initialDelaySeconds` 和 `periodSeconds`。

**预期用户回复**：readinessProbe 的 HTTP path 返回 404，或端口配置错误，或 `initialDelaySeconds` 太短导致应用尚未启动完成。

**下一步判断**：
- 若 readinessProbe 配置错误 → 进入步骤 6 修复方案（修复探针）
- 若探针正常但应用仍不健康 → 提示检查应用配置或依赖服务

---

### 步骤 5: 快速回滚确认（紧急决策）

**顾问**：如果生产流量持续受损，请优先执行回滚：

```bash
kubectl patch svc <svc-name> -n <namespace> --type='merge' -p='{"spec":{"selector":{"version":"blue"}}}'
```

> **如果无法执行 patch**：请使用 `kubectl edit svc <svc-name> -n <namespace>` 手动将 selector 改回 `version: blue`，保存后立即生效。

**预期用户回复**：回滚后蓝色版本服务恢复正常，确认绿色版本存在问题。

**下一步判断**：
- 若回滚后恢复 → 进入步骤 6 修复方案（修复绿环境后重新切换）
- 若回滚后仍异常 → 提示检查蓝色版本是否也受影响

---

### 步骤 6: 提供修复方案

**顾问**：根据排查结果，请按以下方案修复：

#### 方案 A：回滚 Selector 到蓝色版本

```bash
kubectl patch svc <svc-name> -n <namespace> --type='merge' -p='{"spec":{"selector":{"version":"blue","app":"<app-name>"}}}'
```

> **如果无法执行 patch**：请使用 `kubectl edit svc` 修改 selector，或通过 GitOps/ArgoCD 回滚到上一个版本。

#### 方案 B：修复绿环境 ReadinessProbe

```bash
kubectl patch deployment <green-deployment> -n <namespace> --type='merge' -p='{"spec":{"template":{"spec":{"containers":[{"name":"<container>","readinessProbe":{"httpGet":{"path":"/health","port":8080},"initialDelaySeconds":30,"periodSeconds":10}}]}}}}'
```

> **如果无法执行 patch**：请使用 `kubectl edit deployment <green-deployment>` 修改 readinessProbe 的配置（path、port、initialDelaySeconds）。

#### 方案 C：重新部署绿版本

```bash
kubectl rollout restart deployment <green-deployment> -n <namespace>
```

> **如果无法执行**：请先修正绿色版本的镜像或配置，然后执行 `kubectl rollout status deployment <green-deployment> -n <namespace>` 确认新版本就绪后再切换流量。

#### 方案 D：修复代码后重新发布

```bash
kubectl set image deployment/<green-deployment> <container>=<new-image>:<fixed-tag> -n <namespace>
```

> **如果无法执行**：请通过 CI/CD 流水线重新构建并发布修复后的镜像，然后执行 rollout restart 或 apply 新 YAML。

**验证修复**：

```bash
kubectl get pods -n <namespace> -l version=green
kubectl get svc <svc-name> -n <namespace> -o yaml | grep selector -A 3
curl -s http://<svc-ip>/health
```

> **如果无法执行 curl**：请在 Pod 内执行 `wget -qO- http://<svc-name>.<namespace>.svc.cluster.local/health` 验证服务健康。

---

## 相关概念

- [[concepts/blue-green-deployment|蓝绿部署]]
- [[concepts/deployment-controller-architecture|Deployment 控制器]]
