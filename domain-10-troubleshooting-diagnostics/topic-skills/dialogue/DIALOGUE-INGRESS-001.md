---
dialogue_id: "DIALOGUE-INGRESS-001"
skill_id: "SKILL-INGRESS-001"
role: "remote-consultant"
language: "zh"
severity: "medium"
status: "reviewed"
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
title: "Ingress 规则不生效 — 远程顾问对话脚本"
category: dialogue
tags: ["dialogue", "remote-consultant", "troubleshooting", "visibility/public"]
---

# Ingress 规则不生效 — 远程顾问对话脚本

> 对应概念：[[concepts/ingress-controller.md|Ingress Controller]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：外部用户报告无法通过域名访问服务，Ingress 规则似乎不生效。

**顾问回应**：收到。请先确认：该域名对应的 Ingress 资源名称和所在命名空间是什么？

---

### 步骤 1: 确认 Ingress 资源存在且配置正确

**顾问**：请执行以下命令，确认 Ingress 资源的状态：

```bash
kubectl get ingress -n <namespace>
```

> **如果无法执行**：请通过集群管理控制台（如 Rancher、ACK Console）查看 Ingress 列表，或提供当前 kubectl 的报错信息。

```bash
kubectl describe ingress <ingress-name> -n <namespace>
```

> **如果无法执行**：请将 Ingress 的 YAML 配置截图或复制发给我。

**预期用户回复**：Ingress 资源存在，但 ADDRESS 列为空，或 Events 中有错误。

**下一步判断**：
- 若 ADDRESS 为空 → 进入步骤 2 检查 Ingress Class
- 若 Events 显示 backend 错误 → 进入步骤 4 检查 Service
- 若配置正常 → 进入步骤 3 检查 Controller Pod

---

### 步骤 2: 确认 Ingress Class 匹配

**顾问**：请检查 Ingress Class 配置是否匹配：

```bash
kubectl get ingressclass
```

> **如果无法执行**：请通过控制台查看集群中已注册的 IngressClass 列表。

```bash
kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.spec.ingressClassName}'
```

> **如果无法执行**：请查看 Ingress YAML 中 `spec.ingressClassName` 字段的值。

**预期用户回复**：IngressClass 存在，但名称与 Ingress 中声明的不一致，或 ingressClassName 为空。

**下一步判断**：
- 若 class 不匹配 → 进入步骤 6 修复方案（修正 class）
- 若 class 匹配 → 进入步骤 3 检查 Controller

---

### 步骤 3: 检查 Ingress Controller Pod 状态

**顾问**：请检查 Ingress Controller 的运行状态：

```bash
kubectl get pods -n kube-system | grep -E 'ingress|nginx'
```

> **如果无法执行**：请确认 Controller 所在的命名空间（可能是 kube-system、ingress-nginx 或其他），替换后重试。

```bash
kubectl logs -n kube-system deployment/<ingress-controller> --tail=50
```

> **如果无法执行**：请提供 Controller Pod 的状态截图（Running / Pending / CrashLoopBackOff）。

**预期用户回复**：Controller Pod 处于 CrashLoopBackOff 或 Pending 状态，或日志中有配置加载错误。

**下一步判断**：
- 若 Pod 异常 → 进入步骤 6 修复方案（重启 controller）
- 若 Pod 正常 → 进入步骤 4 检查后端 Service

---

### 步骤 4: 检查 Backend Service 是否存在

**顾问**：请验证 Ingress 规则中引用的后端 Service：

```bash
kubectl get svc -n <namespace>
```

> **如果无法执行**：请通过控制台查看该命名空间下的 Service 列表。

```bash
kubectl get endpoints <service-name> -n <namespace>
```

> **如果无法执行**：请检查对应 Service 是否有活跃的 Endpoint（即是否有 Pod 匹配 selector）。

**预期用户回复**：Service 不存在，或 Endpoints 为空（无后端 Pod）。

**下一步判断**：
- 若 Service 不存在或 Endpoints 为空 → 进入步骤 6 修复方案（修正 Service 或 Pod 标签）
- 若 Service 正常 → 进入步骤 5 检查证书配置

---

### 步骤 5: 检查证书配置

**顾问**：如果使用 HTTPS，请检查 TLS Secret：

```bash
kubectl get secret -n <namespace> | grep tls
```

> **如果无法执行**：请确认是否使用了 cert-manager 自动管理证书，或提供 Secret 列表。

```bash
kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.spec.tls[0].secretName}'
```

> **如果无法执行**：请查看 Ingress YAML 中 `spec.tls` 字段引用的 Secret 名称。

```bash
kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
```

> **如果无法执行**：请检查 Secret 是否存在且包含 `tls.crt` 和 `tls.key` 两个键。

**预期用户回复**：Secret 不存在、名称拼写错误，或证书已过期。

**下一步判断**：
- 若证书问题确认 → 进入步骤 6 修复方案（重新创建 secret）
- 若证书正常 → 提示检查 DNS 解析和外部负载均衡器配置

---

### 步骤 6: 提供修复方案

**顾问**：根据以上排查，请按对应根因执行修复：

#### 方案 A：修正 Ingress Class

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch ingress <ingress-name> -n <namespace> --type='merge' -p='{"spec":{"ingressClassName":"<correct-class>"}}'
```

> **如果无法执行 patch**：请使用 `kubectl edit ingress <ingress-name> -n <namespace>` 手动修改 `ingressClassName` 字段。

#### 方案 B：重启 Ingress Controller

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment/<ingress-controller> -n kube-system
```

> **如果无法执行**：请执行 `kubectl delete pod -n kube-system -l app=<ingress-controller-label>` 让 Deployment 自动重建 Pod。

#### 方案 C：重新创建 TLS Secret

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl create secret tls <secret-name> --cert=<cert-file> --key=<key-file> -n <namespace>
```

> **如果无法执行**：请将证书和私钥文件准备好，通过 `kubectl apply -f secret.yaml` 方式创建。如使用 cert-manager，请检查 Certificate 和 ClusterIssuer 状态。

#### 方案 D：修正 Backend Service

```bash
kubectl get pods -n <namespace> -l <service-selector>
```

> **如果无法执行**：请确认 Pod 标签是否与 Service 的 `spec.selector` 匹配。若 Pod 未运行，请排查 Pod 启动失败原因。

**验证修复**：

```bash
curl -H "Host:<your-domain>" http://<ingress-ip>/path -I
```

> **如果无法执行 curl**：请在本地浏览器访问域名，或使用 `wget -qO-` 替代验证。

---

## 相关概念

- [[concepts/ingress-controller.md|Ingress Controller]]
- [[concepts/service-networking.md|Service 网络模型]]
