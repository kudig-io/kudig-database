---
title: StatefulSet Pod 域名解析失败 — 远程顾问对话脚本
summary: StatefulSet Pod 域名解析失败 — 远程顾问对话脚本：kubectl get svc -n <namespace>
category: dialogue
tags:
- dialogue
- remote-consultant
- troubleshooting
- visibility/public
tier: supporting
created: 2026-05-21
updated: 2026-05-21
dialogue_id: DIALOGUE-HEADLESS-001
skill_id: SKILL-HEADLESS-001
role: remote-consultant
language: zh
severity: medium
status: reviewed
last_updated: 2026-05-21
---



# StatefulSet Pod 域名解析失败 — 远程顾问对话脚本

> 对应概念：[[concepts/headless-service.md|Headless Service]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：StatefulSet 的 Pod 域名解析失败，应用无法通过 `<pod-name>.<svc-name>` 互相发现。

**顾问回应**：收到。请先确认：StatefulSet 名称、Service 名称、命名空间，以及解析失败的具体报错是什么？

---

### 步骤 1: 检查 Headless Service 是否存在

**顾问**：请确认 Headless Service 是否正确创建：

```bash
kubectl get svc -n <namespace>
```

> **如果无法执行**：请通过控制台查看 Service 列表，确认目标 Service 的 TYPE 是否为 `ClusterIP` 且 `CLUSTER-IP` 为 `None`。

```bash
kubectl get svc <svc-name> -n <namespace> -o yaml | grep -E 'clusterIP:|publishNotReadyAddresses:'
```

> **如果无法执行**：请查看 Service YAML，确认 `spec.clusterIP` 是否为 `None`。这是 Headless Service 的关键特征。

**预期用户回复**：Service 存在，但 `clusterIP` 不是 `None`（即不是 Headless Service）。

**下一步判断**：
- 若 clusterIP 不为 None → 进入步骤 6 修复方案（创建 Headless Service）
- 若已是 Headless → 进入步骤 2 验证 DNS 记录

---

### 步骤 2: 验证 DNS 记录

**顾问**：请在 Pod 内或集群中验证 DNS 解析：

```bash
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup <pod-name>.<svc-name>.<namespace>.svc.cluster.local
```

> **如果无法执行**：若无法创建临时 Pod，请在现有 Pod 中执行：`kubectl exec <pod-name> -n <namespace> -- nslookup <pod-name>.<svc-name>.<namespace>.svc.cluster.local`。

```bash
kubectl get endpoints <svc-name> -n <namespace>
```

> **如果无法执行**：请确认 Headless Service 的 Endpoints 是否包含所有 StatefulSet Pod 的 IP。Headless Service 的 DNS 记录由 Endpoints 中的 Pod IP 生成。

**预期用户回复**：nslookup 返回 `NXDOMAIN` 或超时，或 Endpoints 中缺少部分 Pod IP。

**下一步判断**：
- 若 Endpoints 为空或不完整 → 检查 StatefulSet Pod 是否全部 Running
- 若 Endpoints 完整但 DNS 失败 → 进入步骤 3 检查 CoreDNS

---

### 步骤 3: 检查 CoreDNS 状态

**顾问**：请确认集群 DNS 组件 CoreDNS 是否正常运行：

```bash
kubectl get pods -n kube-system | grep -E 'coredns|dns'
```

> **如果无法执行**：请通过控制台查看 kube-system 命名空间下的 DNS 相关 Pod 状态。

```bash
kubectl logs -n kube-system deployment/coredns --tail=30
```

> **如果无法执行**：请提供 CoreDNS Pod 的状态截图（Running / CrashLoopBackOff / Pending）。

**预期用户回复**：CoreDNS Pod 处于 CrashLoopBackOff，或日志中有 `loop detected` 等错误。

**下一步判断**：
- 若 CoreDNS 异常 → 进入步骤 6 修复方案（重启 CoreDNS）
- 若 CoreDNS 正常 → 进入步骤 4 验证 StatefulSet 有序启动

---

### 步骤 4: 验证 StatefulSet 有序启动

**顾问**：请确认 StatefulSet Pod 是否按序启动且全部就绪：

```bash
kubectl get pods -n <namespace> -l app=<statefulset-label>
```

> **如果无法执行**：请执行 `kubectl get pods -n <namespace> -w` 观察 Pod 状态变化，或提供当前 Pod 列表。

```bash
kubectl get statefulset <sts-name> -n <namespace>
```

> **如果无法执行**：请确认 StatefulSet 的 READY 列是否为 `<replicas>/<replicas>`。若第一个 Pod 未就绪，后续 Pod 不会启动。

**预期用户回复**：StatefulSet 中部分 Pod 未就绪，导致后续 Pod 和 DNS 记录未创建。

**下一步判断**：
- 若 Pod 未就绪 → 先排查 Pod 启动失败原因（镜像、健康检查等），再验证 DNS
- 若 Pod 全部就绪但 DNS 仍失败 → 进入步骤 6 修复方案（检查 DNS 配置）

---

### 步骤 5: 检查 DNS 配置（resolv.conf）

**顾问**：请检查 Pod 内的 DNS 配置：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec <pod-name> -n <namespace> -- cat /etc/resolv.conf
```

> **如果无法执行**：请确认 Pod 中是否有 `/etc/resolv.conf` 文件，以及 `search` 和 `nameserver` 字段的内容。

```bash
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 5 'dnsPolicy:'
```

> **如果无法执行**：请确认 Pod 的 `dnsPolicy` 是否为 `ClusterFirst`（默认）。若使用 `Default`，Pod 将使用节点上的 DNS 配置，可能无法解析集群内部域名。

**预期用户回复**：`search` 域中不包含 `<namespace>.svc.cluster.local`，或 `dnsPolicy` 被错误设置为 `Default`。

**下一步判断**：
- 若 DNS 配置异常 → 进入步骤 6 修复方案（修正 DNS 配置）

---

### 步骤 6: 提供修复方案

**顾问**：根据排查结果，请按以下方案修复：

#### 方案 A：创建 Headless Service

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: <svc-name>
  namespace: <namespace>
spec:
  clusterIP: None
  selector:
    app: <statefulset-label>
  ports:
  - port: 80
    targetPort: 80
EOF
```

> **如果无法执行**：请手动创建 YAML 文件后执行 `kubectl apply -f headless-service.yaml`。注意 `clusterIP: None` 必须显式声明。

#### 方案 B：重启 CoreDNS

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment/coredns -n kube-system
```

> **如果无法执行**：请执行 `kubectl delete pod -n kube-system -l k8s-app=kube-dns` 或 `-l k8s-app=coredns`，让 Deployment 自动重建 Pod。

#### 方案 C：检查 StatefulSet 启动顺序

```bash
kubectl describe statefulset <sts-name> -n <namespace>
```

> **如果无法执行**：请确认 `podManagementPolicy` 是否为 `OrderedReady`（默认）。若第一个 Pod 未就绪，请排查其未就绪原因（如资源不足、镜像拉取失败）。

```bash
kubectl describe pod <sts-name>-0 -n <namespace>
```

> **如果无法执行**：请查看第一个 Pod 的 Events，解决启动阻塞问题后，后续 Pod 会自动启动。

#### 方案 D：修正 DNS 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch pod <pod-name> -n <namespace> --type='merge' -p='{"spec":{"dnsPolicy":"ClusterFirst","dnsConfig":{"searches":["<namespace>.svc.cluster.local","svc.cluster.local","cluster.local"]}}}'
```

> **如果无法执行**：Pod 的 dnsPolicy 通常不可直接 patch。请修改 StatefulSet 的 Pod template 后重新部署。

**验证修复**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec <pod-name> -n <namespace> -- nslookup <pod-name>.<svc-name>.<namespace>.svc.cluster.local
```

> **如果无法执行**：请使用 `dig` 或 `host` 命令替代。成功时应返回 Pod 的集群 IP 地址。

---

## 相关概念

- [[concepts/headless-service.md|Headless Service]]
- [[concepts/statefulset.md|StatefulSet]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
