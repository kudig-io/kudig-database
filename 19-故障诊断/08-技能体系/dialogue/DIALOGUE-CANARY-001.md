---
title: 金丝雀发布后部分用户报告错误 — 远程顾问对话脚本
summary: 金丝雀发布后部分用户报告错误 — 远程顾问对话脚本：kubectl get deployment -n <namespace>
category: dialogue
tags:
- dialogue
- remote-consultant
- troubleshooting
- visibility/public
tier: supporting
created: 2026-05-21
updated: 2026-05-21
dialogue_id: DIALOGUE-CANARY-001
skill_id: SKILL-CANARY-001
role: remote-consultant
language: zh
severity: high
status: reviewed
last_updated: 2026-05-21
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 金丝雀发布后部分用户报告错误 — 远程顾问对话脚本

> 对应概念：[[22-概念/09-平台与发布/canary-deployment.md|金丝雀部署]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：金丝雀发布后，部分用户报告遇到错误，怀疑金丝雀版本有问题。

**顾问回应**：收到。请先确认：金丝雀版本的 Deployment 名称、当前副本比例，以及用户报告的错误类型（500、超时、功能异常）是什么？

---

### 步骤 1: 确认金丝雀比例

**顾问**：请检查金丝雀版本和稳定版本的副本数对比：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deployment -n <namespace>
```
> **如果无法执行**：请通过控制台查看 Deployment 列表，对比稳定版本和金丝雀版本的 READY 列数值。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deployment <stable-deployment> -n <namespace> -o jsonpath='{.spec.replicas}'
kubectl get deployment <canary-deployment> -n <namespace> -o jsonpath='{.spec.replicas}'
```
> **如果无法执行**：请提供两个 Deployment 的副本数，计算金丝雀流量比例（canary / (stable + canary)）。

**预期用户回复**：金丝雀副本数过高（如 50%），导致受影响用户范围较大。

**下一步判断**：
- 若金丝雀比例过高 → 进入步骤 6 修复方案（降低比例）
- 若比例合理 → 进入步骤 2 检查 Ingress Canary 配置

---

### 步骤 2: 检查 Ingress Canary Annotation

**顾问**：如果使用 Ingress 控制金丝雀流量，请检查 annotation 配置：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ingress <ingress-name> -n <namespace> -o yaml | grep -A 10 'annotations:'
```
> **如果无法执行**：请将 Ingress YAML 中的 annotations 部分复制发给我，重点关注 `canary`、`canary-weight`、`canary-by-header` 等字段。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ingress <ingress-name> -n <namespace> -o jsonpath='{.metadata.annotations}'
```
> **如果无法执行**：请确认金丝雀是基于权重（weight）还是请求头（header）分流。若为 weight，确认 `nginx.ingress.kubernetes.io/canary-weight` 的值是否合理。

**预期用户回复**：canary-weight 设置为 50 或更高，或缺少 `canary: "true"` 注解导致规则未生效。

**下一步判断**：
- 若 canary-weight 过高 → 进入步骤 6 修复方案（降低权重）
- 若 Ingress 配置正常 → 进入步骤 3 查看错误日志

---

### 步骤 3: 查看错误日志

**顾问**：请查看金丝雀版本 Pod 的日志，定位错误根因：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n <namespace> -l version=canary --tail=100
```
> **如果无法执行**：请选取单个金丝雀 Pod 执行 `kubectl logs <canary-pod> -n <namespace> --tail=100`。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n <namespace> -l version=canary --previous --tail=50
```
> **如果无法执行**：若 Pod 有重启，请添加 `--previous` 查看上一次运行的崩溃日志。

**预期用户回复**：日志中出现 NullPointerException、数据库连接失败、配置项缺失等应用错误。

**下一步判断**：
- 若日志显示应用代码错误 → 进入步骤 6 修复方案（回滚或修复代码）
- 若日志正常但仍有用户报错 → 进入步骤 4 检查指标监控

---

### 步骤 4: 检查指标监控

**顾问**：请检查金丝雀版本的资源使用和性能指标：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl top pods -n <namespace> -l version=canary
```
> **如果无法执行**：若 top 命令不可用，请执行 `kubectl describe pod <canary-pod> -n <namespace> | grep -A 5 'Resources:'` 查看资源请求与实际使用。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa -n <namespace>
```
> **如果无法执行**：请确认金丝雀版本是否配置了 HPA。若流量突增但 HPA 未扩容，可能导致响应缓慢或 OOM。

**预期用户回复**：金丝雀 Pod CPU 或内存接近限制，或 OOMKilled 导致服务中断。

**下一步判断**：
- 若资源不足 → 进入步骤 6 修复方案（扩容或增加资源）
- 若资源正常 → 提示检查下游依赖服务（数据库、缓存、外部 API）

---

### 步骤 5: 确认错误影响范围

**顾问**：请通过 Ingress 或负载均衡器确认金丝雀流量特征：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ingress <ingress-name> -n <namespace> -o yaml | grep -E 'canary-by-header|canary-weight'
```
> **如果无法执行**：请确认受影响用户是否有共同特征（如特定地域、特定客户端版本、特定请求路径）。若为 header 金丝雀，可快速缩小影响范围。

**预期用户回复**：金丝雀按特定 header（如 `X-Canary: always`）分流，仅内部测试用户受影响。

**下一步判断**：
- 若仅内部用户受影响 → 降低比例后继续观察
- 若生产用户广泛受影响 → 立即执行步骤 6 回滚

---

### 步骤 6: 提供修复方案

**顾问**：根据排查结果，请按以下方案处理：

#### 方案 A：降低 Canary 比例

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch ingress <ingress-name> -n <namespace> --type='merge' -p='{"metadata":{"annotations":{"nginx.ingress.kubernetes.io/canary-weight":"5"}}}'
```
> **如果无法执行 patch**：请使用 `kubectl edit ingress` 将 `canary-weight` 从当前值降低到 5% 或更低，保存后立即生效。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl scale deployment <canary-deployment> --replicas=1 -n <namespace>
```
> **如果无法执行**：请通过 `kubectl edit deployment` 修改 `spec.replicas` 为更低值，减少金丝雀实例数量。

#### 方案 B：回滚到稳定版本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch ingress <ingress-name> -n <namespace> --type='merge' -p='{"metadata":{"annotations":{"nginx.ingress.kubernetes.io/canary-weight":"0"}}}'
```
> **如果无法执行 patch**：请将 canary-weight 设为 0，或删除金丝雀 Ingress 资源，使流量 100% 路由到稳定版本。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl scale deployment <canary-deployment> --replicas=0 -n <namespace>
```
> **如果无法执行**：若需立即停止金丝雀服务，可将副本数缩至 0。

#### 方案 C：修复代码后重新发布

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl set image deployment/<canary-deployment> <container>=<fixed-image>:<tag> -n <namespace>
```
> **如果无法执行**：请通过 CI/CD 发布修复后的镜像，然后执行 `kubectl rollout restart deployment/<canary-deployment> -n <namespace>`。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout status deployment/<canary-deployment> -n <namespace>
```
> **如果无法执行**：请观察金丝雀 Pod 状态变为 Running 且就绪后，再逐步提升 canary-weight。

#### 方案 D：扩容金丝雀实例

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment <canary-deployment> -n <namespace> --type='merge' -p='{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"limits":{"cpu":"<higher-cpu>","memory":"<higher-mem>"}}}]}}}}'
```
> **如果无法执行 patch**：请使用 `kubectl edit deployment` 增加 resources.limits，或检查 HPA 配置确保能正常扩容。

**验证修复**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> -l version=canary
kubectl logs -n <namespace> -l version=canary --tail=20 | grep -i error
```
> **如果无法执行**：请确认错误日志不再出现，且监控面板中金丝雀版本的错误率恢复正常。

---

## 相关概念

- [[22-概念/09-平台与发布/canary-deployment.md|金丝雀部署]]
- [[22-概念/03-网络/ingress-controller.md|Ingress Controller]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
