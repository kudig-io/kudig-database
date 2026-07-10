---
title: K8s Security Incident Response 远程顾问对话脚本
summary: 安全事件的远程顾问对话脚本，覆盖容器逃逸、异常网络、权限提升检测。
category: dialogue
tags:
- dialogue
- remote-advisor
- k8s-security-incident
- skill
- security
- incident-response
- forensics
- breach
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
relationships:
- target: '[[技能/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[实体/cilium.md]]'
  type: uses
- target: '[[系统基础/知识字典/configuration/configmaps.md]]'
  type: uses
- target: '[[系统基础/知识字典/fundamentals/nodes.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# K8s Security Incident Response 远程顾问对话脚本

> **角色设定**：你是部署在客户环境之外的远程安全顾问，无法直接连接集群。你只能通过对话指导现场工程师执行操作。
> **⚠️ 重要提醒**：本 [[技能/skill-k8s-node-notready-SKILL.md|Skill]] 为 **L1-advisory** 模式。所有操作均为建议性质，必须由安全团队和人工审批后执行。Agent 不会自动执行任何可能影响证据或扩大影响的操作。
> **对话目标**：在 30 分钟内完成安全事件的初步识别、遏制建议和风险评估。

---

## 对话入口

### 场景 A：工程师发现异常容器进程

工程师："Falco 告警说有个 Pod 在运行异常进程"

顾问："收到，这可能是安全事件。为了快速评估风险，请告诉我三个信息：
1. **影响范围**：涉及几个 Pod / 节点？是否涉及核心命名空间（kube-system）？
2. **告警详情**：Falco 告警的具体规则名称是什么？（如 `Terminal shell in container`、`Write to /etc`）
3. **紧急程度**：这个异常进程是否涉及数据外发、权限提升或敏感目录访问？"

### 场景 B：工程师发现未授权 API 访问

工程师："审计日志里发现大量未授权的 API 调用"

顾问："未授权 API 访问是严重安全事件。请确认三点：
1. **来源追踪**：未授权调用的来源 IP 和 UserAgent 是什么？是内部还是外部来源？
2. **时间范围**：异常调用从什么时候开始？持续多久了？
3. **访问范围**：尝试访问了哪些资源？（secrets / [[系统基础/知识字典/configuration/configmaps.md|configmaps]] / pods / [[系统基础/知识字典/fundamentals/nodes.md|nodes]]）"

### 场景 C：工程师发现可疑镜像或后门

工程师："扫描器发现正在运行的 Pod 里有已知 CVE 或后门"

顾问："可疑镜像是严重的供应链安全问题。请确认：
1. **镜像信息**：受影响的镜像名称、tag、镜像仓库地址？
2. **运行状态**：有多少 Pod 正在运行该镜像？分布在哪些节点？
3. **影响评估**：这些 Pod 是否处理敏感数据？是否有网络外连权限？"

### 场景 D：工程师从监控发现异常网络行为

工程师："监控系统看到有 Pod 在连接外部可疑 IP"

顾问："异常网络连接可能意味着数据外泄或 C2 通信。请确认：
1. **目标 IP**：外部 IP 的地理位置和归属组织？
2. **通信特征**：是持续性连接还是间歇性连接？流量大小如何？
3. **Pod 身份**：发起连接的 Pod 名称、命名空间、所属应用是什么？"

---

## Round 1：快速确认与证据保全

顾问："现在开始第一轮诊断。在安全事件中，**证据保全优先于修复**。请执行以下只读命令并粘贴完整输出：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认当前安全上下文
kubectl auth whoami

# 查看异常 Pod 的基本信息
kubectl get pod <pod-name> -n <namespace> -o wide

# 查看异常 Pod 的 SecurityContext
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.securityContext}{.spec.containers[0].securityContext}'
```
> **如果无法执行 kubectl**：请通过集群管理控制台（如 Rancher、OpenShift Console）查看 Pod 详情，或者请有权限的同事执行并截图。

> **⚠️ 关键提醒**：在收集证据前，**不要删除或重启**任何可疑 Pod，以免破坏取证线索。"

### 分支 1.1：特权容器或 root 运行

工程师：（提供 Pod securityContext 信息）

顾问："发现特权容器或 root 运行配置。这是高危配置。请**立即**执行证据收集：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 收集 Pod 完整配置
kubectl get pod <pod-name> -n <namespace> -o yaml > /tmp/suspicious-pod-<pod-name>.yaml

# 查看 Pod 的事件历史
kubectl get events --field-selector involvedObject.name=<pod-name> -n <namespace> --sort-by='.lastTimestamp'

# 查看 Pod 的网络策略（如有）
kubectl get networkpolicy -n <namespace>
```
> **如果无法执行**：请通过控制台截图保存 Pod 配置页面，或请同事协助导出 YAML。

当前优先级评估：
- 如果是特权容器（privileged: true）→ **P0-CRITICAL**，需要立即隔离
- 如果是 root 运行（runAsUser: 0）但非特权 → **P1**，需要评估权限提升风险
- 如果 securityContext 配置正常 → 继续检查其他指标"

### 分支 1.2：异常网络连接确认

工程师：（确认 Pod 有异常外连行为）

顾问："异常网络连接已确认。请**立即**收集网络证据：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 的 ServiceAccount 和 RBAC 权限
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.serviceAccountName}'
kubectl auth can-i --list --as=system:serviceaccount:<namespace>:<sa-name>

# 查看相关的 NetworkPolicy
kubectl get networkpolicy -n <namespace> -o yaml

# 查看 Pod 的进出连接（如节点可访问）
# SSH 到节点后执行
ss -ntp | grep <pod-ip>
```
> **如果无法 SSH 到节点**：请尝试：
> **替代方案 A**：通过云厂商控制台查看节点网络流日志（VPC Flow Logs）
> **替代方案 B**：在集群内启动诊断 Pod 执行抓包：`kubectl run tcpdump --rm -it --image=nicolaka/netshoot -- tcpdump -i any host <pod-ip>`
> **替代方案 C**：查看防火墙/安全组日志，确认外部连接目标

当前优先级评估：
- 如果连接目标是已知恶意 IP（C2 服务器）→ **P0-CRITICAL**
- 如果目标不明但持续通信 → **P1**
- 如果是一次性连接且无数据传输 → **P2**，持续观察"

### 分支 1.3：镜像来源可疑

工程师：（确认镜像仓库或 tag 可疑）

顾问："可疑镜像已确认。请**立即**收集镜像证据：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看镜像完整信息
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].image}'

# 查看镜像拉取策略和 ImagePullSecret
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'

# 查看使用的 ServiceAccount
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.serviceAccountName}'
```
> **如果无法获取镜像详情**：请通过容器运行时命令检查：
> **替代方案 A**：SSH 到节点执行 `crictl images | grep <image-name>` 或 `docker images | grep <image-name>`
> **替代方案 B**：查看镜像仓库的审计日志，确认镜像推送者和推送时间
> **替代方案 C**：通过镜像扫描平台（Trivy / Snyk / Clair）查看该镜像的漏洞报告

当前优先级评估：
- 如果镜像是从非官方仓库拉取且包含恶意代码 → **P0-CRITICAL**
- 如果镜像包含高危 CVE 且正在处理敏感数据 → **P1**
- 如果镜像只是旧版本包含中危漏洞 → **P2**"

### 分支 1.4：审计日志异常

工程师：（提供审计日志异常信息）

顾问："审计日志异常需要立即分析。请收集以下信息：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看审计日志中该用户的访问记录（如日志在集群内）
kubectl logs -n kube-system <audit-log-pod> | grep <user/ip> | tail -50

# 查看 RBAC 绑定
kubectl get clusterrolebinding,rolebinding --all-namespaces | grep <user/sa>

# 查看敏感资源访问
kubectl get secrets --all-namespaces -o json | jq '.items[] | {name: .metadata.name, namespace: .metadata.namespace}'
```
> **如果无法查看审计日志**：请尝试：
> **替代方案 A**：通过日志聚合系统（ELK / Loki / SLS）查询审计日志
> **替代方案 B**：查看 kube-apiserver 的审计日志文件（如可访问 master 节点）：`/var/log/kubernetes/audit.log`
> **替代方案 C**：联系集群管理员导出审计日志

当前优先级评估：
- 如果未授权访问了 secrets 或 kube-system 命名空间 → **P0-CRITICAL**
- 如果访问了 configmaps 或普通 Pod → **P1**
- 如果只是 list/get 操作且未成功 → **P2**"

---

## Round 2：深度分析与风险评估

### 场景：已完成初步确认，进入深度分析

顾问："现在进入深度分析阶段。根据 Round 1 的结果，我们需要评估攻击面、权限提升可能性和数据泄露风险。"

#### 分支 2.1：容器逃逸风险评估

工程师："可疑 Pod 是 privileged 模式，且有 hostPath 挂载"

顾问："容器逃逸风险极高。请**立即**执行风险评估：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Pod 的挂载配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.volumes}' | jq .

# 检查是否挂载了敏感主机目录
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -E 'hostPath|hostPID|hostNetwork|hostIPC'

# 检查 Pod 的 capabilities
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].securityContext.capabilities}'

# 查看节点上该容器的信息（SSH 到节点）
# crictl ps | grep <pod-name>
# crictl inspect <container-id>
```
> **如果无法 SSH**：请尝试 `kubectl debug node/<node-name> -it --image=busybox`，在节点命名空间中检查挂载信息。

风险评估：
1. **如果挂载了 /var/run/docker.sock 或 /proc** → 几乎可以确认可容器逃逸 → **立即隔离节点**
2. **如果启用了 hostPID 或 hostNetwork** → 可访问主机进程或网络 → **高风险**
3. **如果 capabilities 包含 CAP_SYS_ADMIN** → 可执行 mount 操作 → **高风险**

> **遏制建议**：在获得安全团队审批前，不要直接删除 Pod。应先隔离节点：`kubectl cordon <node-name>`，然后评估是否需要隔离到独立网络。"

#### 分支 2.2：RBAC 权限提升分析

工程师："发现异常 ServiceAccount 绑定了 cluster-admin 权限"

顾问："权限提升是严重的横向移动风险。请执行以下分析：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看异常 ServiceAccount 的所有绑定
kubectl get clusterrolebinding,rolebinding --all-namespaces -o yaml | grep -B 5 -A 5 '<sa-name>'

# 查看 cluster-admin 的所有绑定（排查过度授权）
kubectl get clusterrolebinding -o yaml | grep -E 'cluster-admin|name: '

# 查看该 ServiceAccount 创建的 Token
kubectl get secret -n <namespace> | grep <sa-name>

# 查看 Token 的使用记录（如审计日志可用）
kubectl logs -n kube-system <audit-pod> | grep '<sa-name>' | tail -30
```
> **如果无法查看审计日志**：请通过以下方式评估：
> **替代方案 A**：检查该 ServiceAccount 关联的 Pod 列表：`kubectl get pods --all-namespaces -o json | jq '.items[] | select(.spec.serviceAccountName=="<sa-name>") | {name: .metadata.name, namespace: .metadata.namespace}'`
> **替代方案 B**：检查最近是否有新的 ClusterRoleBinding 创建：`kubectl get clusterrolebinding --sort-by='.metadata.creationTimestamp' | tail -20`
> **替代方案 C**：检查是否有异常用户通过该 SA 的 Token 访问 API：`kubectl get events --all-namespaces | grep -i 'unauthorized|forbidden' | tail -20`

风险评估：
1. **如果 cluster-admin 绑定是最近创建的且非预期** → 可能是攻击者提权 → **P0**
2. **如果 Token 被外部使用** → 凭证泄露 → **P0**
3. **如果绑定是历史遗留但存在过度授权** → 配置风险 → **P1**"

#### 分支 2.3：数据泄露风险评估

工程师："可疑 Pod 有访问 secrets 的权限，且存在外连"

顾问："数据泄露风险极高。请**立即**评估泄露范围：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看该 Pod 可访问的所有 secrets
kubectl auth can-i list secrets --as=system:serviceaccount:<namespace>:<sa-name>
kubectl auth can-i get secrets --as=system:serviceaccount:<namespace>:<sa-name>

# 查看该命名空间下的所有 secrets
kubectl get secrets -n <namespace>

# 查看可疑 Pod 的网络策略
cat <<EOF | kubectl apply -f -  # 临时限制出网（需审批）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-egress-suspicious
  namespace: <namespace>
spec:
  podSelector:
    matchLabels:
      <pod-labels>
  policyTypes:
  - Egress
EOF
```
> **如果无法创建 NetworkPolicy**：请尝试：
> **替代方案 A**：通过云厂商安全组 / 防火墙规则限制该 Pod 所在节点的出网
> **替代方案 B**：如果是 [[实体/cilium.md|Cilium]]，使用 Calico NetworkPolicy 或 CiliumNetworkPolicy 限制
> **替代方案 C**：如果以上都无法执行，请立即联系网络团队手动阻断目标 IP

风险评估：
1. **如果可访问的 secrets 包含数据库密码、API 密钥** → 敏感凭证泄露 → **P0**
2. **如果外连流量大且加密** → 可能已发生数据外泄 → **P0**
3. **如果外连流量小且为明文** → 可能是 C2 心跳 → **P1**"

#### 分支 2.4：供应链攻击分析

工程师："可疑镜像的 Dockerfile 被篡改，包含恶意命令"

顾问："供应链攻击影响范围广，需要全面评估。请执行以下分析：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看使用该镜像的所有 Pod
kubectl get pods --all-namespaces -o json | \
  jq '.items[] | select(.spec.containers[].image | contains("<image-name>")) | {name: .metadata.name, namespace: .metadata.namespace}'

# 查看镜像的创建时间和层信息（如可访问镜像仓库）
# docker inspect <image> 或 crane config <image>

# 查看 Deployment/StatefulSet/DaemonSet 中使用该镜像的所有工作负载
kubectl get deployment,statefulset,daemonset --all-namespaces -o yaml | grep -B 10 '<image-name>'
```
> **如果无法分析镜像**：请尝试：
> **替代方案 A**：通过镜像仓库 Web UI 查看镜像的构建历史和签名信息
> **替代方案 B**：检查 CI/CD 流水线日志，确认最近是否有异常构建或推送
> **替代方案 C**：检查镜像的 digest 是否与预期一致：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].imageID}'`

风险评估：
1. **如果镜像是 DaemonSet 使用的** → 影响所有节点 → **P0**
2. **如果镜像是核心业务 Deployment 使用的** → 影响业务和数据安全 → **P1**
3. **如果镜像只是测试环境使用的** → 影响范围可控 → **P2**"

#### 分支 2.5：内部威胁 / 配置漂移分析

工程师："发现最近有人手动修改了 RBAC 和网络策略"

顾问："内部威胁或配置漂移需要追踪变更来源。请执行以下分析：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看最近 24 小时的 RBAC 变更
kubectl get clusterrolebinding,rolebinding,clusterrole,role --all-namespaces -o yaml | grep -E 'creationTimestamp|uid'

# 查看 ConfigMap 和 Secret 的最近变更
kubectl get configmap,secret --all-namespaces -o yaml | grep -E 'creationTimestamp|resourceVersion' | head -50

# 查看最近的审计事件（如可用）
kubectl logs -n kube-system <audit-log-pod> | grep -E 'create|update|delete|patch' | grep -E 'rbac|secret|configmap|networkpolicy' | tail -50
```
> **如果无法查看审计日志**：请尝试：
> **替代方案 A**：检查 GitOps 仓库（ArgoCD / Flux）的提交历史，确认最近是否有异常变更
> **替代方案 B**：检查 CI/CD 流水线的最近执行记录
> **替代方案 C**：询问团队成员，确认是否有人手动执行过 kubectl 命令

风险评估：
1. **如果变更是非工作时间进行的且未经授权** → 内部威胁 → **P0**
2. **如果变更是配置漂移（无 Git 记录）** → 配置管理失控 → **P1**
3. **如果变更是正常流程但权限过大** → 配置风险 → **P2**"

---

## Round 3：遏制建议与升级决策

### 场景：已完成深度分析，进入遏制和升级阶段

> **⚠️ 重要声明**：以下所有遏制操作均需安全团队审批。顾问仅提供建议，不直接执行任何修改操作。

#### 分支 3.1：容器逃逸事件遏制

顾问："根因确认：容器逃逸风险。当前建议的遏制措施：

**遏制步骤**（需安全团队审批后执行）：

步骤 1：隔离节点

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

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
# 标记节点不可调度
kubectl cordon <node-name>

# 如果确认已逃逸，考虑隔离节点网络
# 通过云厂商安全组限制节点出网
```
> **如果无法 cordon**：请立即联系集群管理员执行。如果管理员不可用，请通过云厂商控制台将节点从负载均衡器中移除。

步骤 2：保存证据

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出可疑 Pod 的完整信息
kubectl get pod <pod-name> -n <namespace> -o yaml > /tmp/evidence-pod-<pod-name>.yaml

# 导出节点信息
kubectl get node <node-name> -o yaml > /tmp/evidence-node-<node-name>.yaml

# 保存容器日志
kubectl logs <pod-name> -n <namespace> --previous > /tmp/evidence-pod-<pod-name>.log
```
> **如果无法保存日志**：请确保日志已采集到外部系统（ELK / Loki），并标记保留。

步骤 3：评估是否需要终止节点

> **终止节点前必须确认**：
> 1. 已保存所有证据
> 2. 已通知安全团队和法务团队
> 3. 已评估业务影响（该节点上的其他 Pod）

> **升级**：容器逃逸属于 **P0-CRITICAL**，必须立即升级到安全响应团队和高级 SRE。"

#### 分支 3.2：RBAC 滥用事件遏制

顾问："根因确认：RBAC 权限提升。当前建议的遏制措施：

**遏制步骤**（需安全团队审批后执行）：

步骤 1：撤销异常权限

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
# 删除异常的 ClusterRoleBinding
kubectl delete clusterrolebinding <suspicious-binding>

# 删除异常的 RoleBinding
kubectl delete rolebinding <suspicious-binding> -n <namespace>
```
> **如果无法删除**：请立即联系集群管理员。删除前请确认：
> 1. 该绑定不是生产必需的
> 2. 已备份绑定配置（`kubectl get clusterrolebinding <name> -o yaml`）
> 3. 已通知相关应用团队

步骤 2：轮换受损凭证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除可疑 ServiceAccount 的 Token Secret
kubectl delete secret <sa-token-secret> -n <namespace>

# 或重新创建 ServiceAccount
kubectl delete sa <suspicious-sa> -n <namespace>
```
> **如果无法删除 Token**：请考虑：
> 1. 创建新的 ServiceAccount 并更新 Pod 引用
> 2. 如果 Pod 无法直接更新，考虑重建 Deployment

步骤 3：审计权限范围

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查是否还有其他过度授权
kubectl get clusterrolebinding,rolebinding --all-namespaces -o yaml | grep -E 'cluster-admin|edit|admin'
```
> **升级**：RBAC 滥用属于 **P1**，如果涉及 cluster-admin 或敏感数据访问，升级到 **P0**。"

#### 分支 3.3：恶意镜像事件遏制

顾问："根因确认：恶意镜像 / 供应链攻击。当前建议的遏制措施：

**遏制步骤**（需安全团队审批后执行）：

步骤 1：停止受影响的工作负载

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 缩容受影响 Deployment（不删除，保留证据）
kubectl scale deployment <deployment-name> -n <namespace> --replicas=0

# 或暂停 DaemonSet
kubectl patch daemonset <daemonset-name> -n <namespace> -p '{"spec":{"template":{"spec":{"nodeSelector":{"non-existing":"true"}}}}}'
```
> **如果无法 scale**：请考虑：
> 1. 通过 NetworkPolicy 阻断该 Pod 的网络通信
> 2. 通过 PodDisruptionBudget 限制
> 3. 联系集群管理员直接删除 Pod（保留日志和事件）

步骤 2：阻断镜像来源

```bash
# 如果使用的是 Admission Controller（OPA / Kyverno），添加镜像黑名单策略
# 如果使用的是镜像仓库，删除或标记该镜像为不安全
```

> **如果无法阻断镜像仓库**：请通知镜像仓库管理员和 CI/CD 团队，暂停使用该镜像仓库的自动构建。

步骤 3：扫描和替换镜像

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用可信镜像替换
kubectl set image deployment/<deployment-name> <container>=<trusted-image>:<tag> -n <namespace>
```
> **升级**：恶意镜像属于 **P0-CRITICAL**，如果镜像是 DaemonSet 使用的，立即升级到最高优先级。"

#### 分支 3.4：数据泄露事件遏制

顾问："根因确认：数据泄露风险。当前建议的遏制措施：

**遏制步骤**（需安全团队审批后执行）：

步骤 1：阻断外连

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建临时出网限制 NetworkPolicy
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: emergency-deny-egress
  namespace: <namespace>
spec:
  podSelector:
    matchLabels:
      <suspicious-pod-labels>
  policyTypes:
  - Egress
  egress: []
EOF
```
> **如果无法创建 NetworkPolicy**：请尝试：
> 1. 通过云厂商安全组限制节点出网
> 2. 通过 Calico GlobalNetworkPolicy 限制
> 3. 联系网络团队手动阻断目标 IP

步骤 2：轮换可能泄露的凭证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果 secrets 可能被读取，立即更新
kubectl delete secret <potentially-leaked-secret> -n <namespace>
# 重新创建新的 secret，并更新引用它的所有 Pod
```
> **如果无法立即轮换**：请评估泄露范围，优先轮换数据库密码、云厂商 API 密钥等高敏感凭证。

步骤 3：通知相关方

> **数据泄露事件必须通知**：
> 1. 安全团队
> 2. 法务 / 合规团队
> 3. 数据保护官（DPO）
> 4. 受影响的业务团队

> **升级**：数据泄露属于 **P0-CRITICAL**，必须立即升级。"

---

## 升级路径

当满足以下条件之一时，顾问应明确建议**升级**到安全响应团队：

### 🔴 立即升级（P0-CRITICAL）

- **确认容器逃逸**（privileged Pod + hostPath / hostPID / hostNetwork）
- **确认数据泄露**（可疑 Pod 读取了 secrets 且有外连行为）
- **确认恶意镜像在运行**（镜像包含后门、挖矿程序、C2 客户端）
- **确认未授权访问核心资源**（kube-system / secrets / nodes）
- **cluster-admin 权限被异常绑定**且无法确认来源
- **多个节点同时出现安全告警**（可能是蠕虫或自动化攻击）

### 🟠 建议升级（P1）

- **可疑网络行为**但无法确认恶意性
- **镜像包含高危 CVE**且处理敏感数据
- **RBAC 配置漂移**导致过度授权
- **审计日志发现大量异常操作**但来源不确定
- **内部人员手动执行了敏感操作**（如修改 RBAC、访问 secrets）

### 🟡 可能升级（P2→P1）

- **漏洞扫描告警**但尚未确认利用
- **配置不符合安全基线**但无直接威胁
- **工程师对执行遏制操作存在顾虑**
- **需要法务 / 合规团队介入**评估数据泄露影响

### 升级话术

顾问："当前安全事件已超出本 Skill 的自主处理范围，**强烈建议立即升级**到安全响应团队。

**请执行以下操作**：
1. **立即通知**安全响应团队（Security On-Call）和值班经理
2. **在工单系统中标记**为 Security Incident / P0
3. **保存所有证据**：Pod YAML、日志、审计记录、截图
4. **不要删除或修改**任何可疑资源（等待安全团队取证）
5. 如果需要，我可以协助整理当前已确认的安全事件摘要

**当前已确认信息**：
- 事件类型：容器逃逸 / RBAC 滥用 / 恶意镜像 / 数据泄露 / 异常访问
- 影响范围：X 个 Pod / Y 个节点 / Z 个命名空间
- 可疑资源：[Pod 名称 / SA 名称 / 镜像地址 / IP 地址]
- 已收集证据：[证据文件列表]
- 遏制状态：[已隔离 / 待审批 / 无法执行]

**⚠️ 特别提醒**：
- 在安全团队到达前，**不要重启或删除可疑 Pod**
- 如果必须遏制，优先使用只读操作（cordon、NetworkPolicy）
- 所有操作记录时间戳和操作人

请把这些信息同步给安全响应团队负责人。"

---


### 分支 1.4：阿里云ACK/专有云安全事件排查

工程师："我们在阿里云ACK/专有云环境，发现安全事件"

顾问："阿里云环境有额外的安全维度，请按以下顺序排查：

**步骤 1：阿里云云安全中心检查**
```bash
# 检查云安全中心告警
aliyun sas DescribeAlarmEventList --Type <type>

# 检查漏洞扫描结果
aliyun sas DescribeVulList --VulStatus <status>

# 检查基线检查
aliyun sas DescribeCheckWarningSummary
```

> **如果无法执行aliyun CLI**：请登录云安全中心控制台，告诉我：
> 1. 是否有未处理的安全告警？
> 2. 是否有漏洞需要修复？
> 3. 是否有异常登录或操作记录？

**步骤 2：ACK安全组件检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查Security Center Agent
kubectl get pods -n kube-system | grep security

# 检查网络策略
kubectl get networkpolicy -A

# 检查Pod安全策略
kubectl get psp

# 检查镜像扫描结果
aliyun cr GET /repos/<ns>/<repo>/scanResults
```
**步骤 3：专有云安全特殊考虑**
- 专有云有独立的合规要求
- 检查天基安全审计日志
- 确认ASO安全基线配置
- 检查飞天组件安全补丁

**步骤 4：阿里云特定修复**

如云安全中心告警：
1. 登录云安全中心控制台
2. 查看告警详情和处理建议
3. 按建议修复漏洞或配置
4. 确认告警消除

如镜像存在漏洞：
```bash
# 触发镜像扫描
aliyun cr ScanRepo --RepoNamespace <ns> --RepoName <repo> --Tag <tag>

# 查看扫描结果
aliyun cr GetRepoScanResult --RepoNamespace <ns> --RepoName <repo> --Tag <tag>
```

**阿里云控制台路径**：
- 云安全中心：阿里云首页 → 云安全中心
- 容器镜像安全：ACR控制台 → 镜像仓库 → 安全扫描
- ACK安全：ACK控制台 → 集群详情 → 安全管理

> **安全事件升级**：如涉及专有云底层安全漏洞，立即联系阿里云安全团队。


## 附录：常用命令速查

| 目的 | 命令 | 替代方案 |
|------|------|----------|
| 查看 Pod 安全上下文 | `kubectl get pod -o jsonpath='{.spec.securityContext}'` | `kubectl get pod -o yaml` |
| 查看 RBAC 权限 | `kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa>` | `kubectl describe clusterrolebinding` |
| 查看 ServiceAccount | `kubectl get sa -n <ns>` | 控制台 |
| 查看 NetworkPolicy | `kubectl get networkpolicy -n <ns>` | Calico/Cilium CLI |
| 保存 Pod 证据 | `kubectl get pod -o yaml > file.yaml` | 控制台导出 |
| 查看审计日志 | `kubectl logs -n kube-system <audit-pod>` | 日志平台 / 节点文件 |
| 查看镜像信息 | `kubectl get pod -o jsonpath='{.spec.containers[*].image}'` | `crictl images` / 控制台 |
| 隔离节点 | `kubectl cordon <node>` | 云厂商控制台 |
| 缩容 Deployment | `kubectl scale deployment --replicas=0` | GitOps / ArgoCD |
| 创建 NetworkPolicy | `kubectl apply -f <policy.yaml>` | Calico GlobalNetworkPolicy |

---

> 本对话脚本基于 SKILL-SEC-002（K8s Security Incident Response 诊断与响应）设计。
> 完整根因目录参考 `reference/root-cause-catalog.md`
> 完整修复手册参考 `reference/remediation-playbook.md`
## Related

- [[实体/deployment.md|Deployment]]


<!-- risk-assessed -->
