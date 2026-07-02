---
title: Pod 间网络不通，怀疑 NetworkPolicy 阻断 — 远程顾问对话脚本
summary: Pod 间网络不通，怀疑 NetworkPolicy 阻断 — 远程顾问对话脚本：kubectl get pods -n kube-system
  | grep -E 'calico|cilium|weave'
category: dialogue
tags:
- dialogue
- remote-consultant
- troubleshooting
- visibility/public
tier: supporting
created: 2026-05-21
updated: 2026-05-21
dialogue_id: DIALOGUE-NETWORKPOLICY-001
skill_id: SKILL-NETWORKPOLICY-001
role: remote-consultant
language: zh
severity: medium
status: reviewed
last_updated: 2026-05-21
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod 间网络不通，怀疑 NetworkPolicy 阻断 — 远程顾问对话脚本

> 对应概念：[[concepts/network-policy.md|NetworkPolicy]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：用户报告 Pod 间网络不通，怀疑是 NetworkPolicy 阻断了流量。

**顾问回应**：收到。请先确认：出现网络不通的源 Pod 和目标 Pod 分别位于哪个命名空间，以及它们的具体名称？

---

### 步骤 1: 确认 CNI 支持 NetworkPolicy

**顾问**：请执行以下命令，确认集群 CNI 插件是否支持 NetworkPolicy：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n kube-system | grep -E 'calico|cilium|weave'
```
> **如果无法执行**：请通过集群管理控制台查看 kube-system 命名空间下的 CNI 相关 Pod，或提供当前集群使用的 CNI 类型（如 Calico、Cilium、Flannel 等）。

**预期用户回复**：CNI 为 Calico/Cilium/其他，或未发现相关 Pod。

**下一步判断**：
- 若 CNI 不支持 NetworkPolicy（如 Flannel）→ 问题与 NetworkPolicy 无关，需排查其他网络问题
- 若 CNI 支持 → 进入步骤 2 检查现有策略

---

### 步骤 2: 检查现有 NetworkPolicy

**顾问**：请检查集群中所有命名空间的 NetworkPolicy 配置：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy --all-namespaces
```
> **如果无法执行**：请提供目标命名空间下的 NetworkPolicy 列表，或确认是否部署了任何 NetworkPolicy 资源。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy -n <namespace> -o yaml
```
> **如果无法执行**：请将 NetworkPolicy 的 YAML 配置截图或复制发给我。

**预期用户回复**：存在 deny-all 或限制 ingress/egress 的 NetworkPolicy。

**下一步判断**：
- 若存在限制流量的策略 → 进入步骤 3 测试连通性
- 若不存在任何策略 → 问题可能不在 NetworkPolicy，需排查 CNI 或路由问题

---

### 步骤 3: 测试 Pod 间连通性

**顾问**：请进入源 Pod 测试到目标地址的网络连通性：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec <source-pod> -n <namespace> -- ping -c 4 <target-ip>
```
> **如果无法执行**：请在目标 Pod 所在节点上执行 `ping <target-pod-ip>`，或提供源 Pod 到目标 Service 的连通性测试结果。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec <source-pod> -n <namespace> -- nc -zv <target-ip> <port>
```
> **如果无法执行**：请使用 `telnet <target-ip> <port>` 或 `curl -v` 测试目标端口是否可达。

**预期用户回复**：ping/nc 不通，确认网络被阻断。

**下一步判断**：
- 若网络不通 → 进入步骤 4 验证规则匹配
- 若网络通但应用层不通 → 检查目标应用端口和服务状态

---

### 步骤 4: 验证 NetworkPolicy 规则匹配

**顾问**：请查看具体 NetworkPolicy 的规则详情：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy <policy-name> -n <namespace> -o yaml
```
> **如果无法执行**：请将 NetworkPolicy 的完整 YAML 配置复制给我，特别关注 `spec.ingress.from` 和 `spec.egress.to` 字段。

**预期用户回复**：NetworkPolicy 中 podSelector 或 namespaceSelector 未匹配到目标 Pod。

**下一步判断**：
- 若规则未匹配源/目标 Pod → 进入步骤 5 检查 selector 配置
- 若规则匹配但端口协议限制 → 进入步骤 6 修复方案

---

### 步骤 5: 检查 namespaceSelector 和 podSelector

**顾问**：请检查 NetworkPolicy 中的 selector 是否匹配实际 Pod 标签：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> --show-labels
```
> **如果无法执行**：请提供源 Pod 和目标 Pod 的 `metadata.labels` 内容。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy <policy-name> -n <namespace> -o jsonpath='{.spec.ingress[0].from}'
```
> **如果无法执行**：请手动查看 NetworkPolicy YAML 中 `ingress.from` 和 `egress.to` 部分的 selector 配置，与 Pod 标签逐一比对。

**预期用户回复**：Pod 标签与 selector 不匹配，或缺少必要的 namespaceSelector 配置。

**下一步判断**：
- 若 selector 不匹配 → 进入步骤 6 修复方案（放宽规则）
- 若 selector 匹配但协议端口不对 → 进入步骤 6 修复方案（调整端口）

---

### 步骤 6: 提供修复方案

**顾问**：根据以上排查，请按对应根因执行修复：

#### 方案 A：放宽 NetworkPolicy 规则

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl edit networkpolicy <policy-name> -n <namespace>
```
修改 `spec.ingress.from` 或 `spec.egress.to` 中的 selector，确保匹配源/目标 Pod 的标签和命名空间。

> **如果无法执行 edit**：请使用 `kubectl patch` 或准备修改后的 YAML 文件执行 `kubectl apply -f`。

#### 方案 B：添加 allow-all 策略（临时）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-all
  namespace: <namespace>
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - {}
  egress:
    - {}
EOF
```
> **如果无法执行**：请手动在控制台创建允许所有流量的 NetworkPolicy，确认连通性后删除该临时策略。

#### 方案 C：确认端口和协议

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy <policy-name> -n <namespace> -o jsonpath='{.spec.ingress[*].ports}'
```
> **如果无法执行**：请检查 NetworkPolicy 中 `ports` 字段的 `protocol` 和 `port` 是否与目标服务实际监听的一致。

**验证修复**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec <source-pod> -n <namespace> -- ping -c 4 <target-ip>
```
> **如果无法执行**：请在应用层面验证功能是否正常，确认 Pod 间通信已恢复。

---

## 相关概念

- [[concepts/network-policy.md|NetworkPolicy]]
- [[entities/cni.md|CNI 插件]]
- [[skills/best-practices/best-practices/security/pod-security.md|Pod 安全策略]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
