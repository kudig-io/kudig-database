---
title: 安全事件应急响应 / Security Incident Response
description: '# 安全事件应急响应 / Security Incident Response'
summary: '安全事件应急响应是 [[Kubernetes|Kubernetes]] 集群运维中**最紧迫且风险最高**的场景之一。与传统基础设施问题不同，安全事件具有时间敏感性、影响扩散性和取证必要性三大特征。一旦发生活跃入侵，每分钟的延迟都可能导致数据泄露范围扩大、横向移动加剧或证据被销毁。'
category: security
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- prometheus
- istio
- cilium
tier: supporting
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- 安全事件应急响应 / Security Incident Response 是什么
- 如何 安全事件应急响应 / Security Incident Response
trigger_keywords:
- container escape
- 容器逃逸
- privilege escalation
- 权限提升
- suspicious process
- 可疑进程
- cryptominer detected
- 挖矿程序
- secret leaked
- 凭据泄露
- unauthorized access
- 未授权访问
- abnormal network traffic
- 异常网络流量
- supply chain attack
- 供应链攻击
- cve vulnerability
- CVE漏洞
- compliance violation
- 合规违规
- pod security violation
- lateral movement
- 横向移动
- dns tunneling
- DNS隧道
- image vulnerability
- 镜像漏洞
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- policy-basics
skill_id: SKILL-18_SECURITY_INCIDENT_RESPONSE-001
skill_name: 安全事件应急响应 / Security Incident Response
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L1-advisory
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




<!-- condition: kubectl get events -A --sort-by='.lastTimestamp' | grep -E 'Warning|Forbidden|Denied' | tail -20 显示异常安全事件 -->

# 安全事件应急响应 / Security Incident Response

---

## 1. 概述

安全事件应急响应是 [[Kubernetes|Kubernetes]] 集群运维中**最紧迫且风险最高**的场景之一。与传统基础设施问题不同，安全事件具有时间敏感性、影响扩散性和取证必要性三大特征。一旦发生活跃入侵，每分钟的延迟都可能导致数据泄露范围扩大、横向移动加剧或证据被销毁。

### 覆盖范围

本 [[SKILL|Skill]] 覆盖以下安全事件类型：

1. **容器逃逸检测与遏制**: 检测并阻止攻击者从容器内部突破到宿主机
2. **供应链攻击响应**: 恶意镜像、被篡改的 base image、依赖注入等
3. **网络异常流量检测**: 横向移动、DNS 隧道、异常外连（特别是矿池连接）
4. **Secret/凭据泄露应急处理**: 敏感信息暴露后的紧急轮换和影响控制
5. **合规检查与审计**: CIS Benchmark、Pod Securityod Security Standards]] 违规检测
6. **取证与证据保全**: 确保事件可追溯、证据链完整

### 典型触发场景

1. **运行时安全告警**: Falco/Tetragon 检测到容器内可疑进程执行（如 shell 反弹、挖矿程序启动）
2. **镜像扫描发现高危漏洞**: Trivy/Grype 扫描发现 CRITICAL CVE，且该镜像已在生产环境运行
3. **审计日志异常模式**: 短时间内大量 `exec`/`attach` 请求、异常的 Secret 访问
4. **网络流量异常**: 容器建立到已知恶意 IP/域名的连接，或检测到 DNS 隧道特征

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `pods`, `pods/log`, `pods/exec`, `pods/eviction`, `secrets`, `events`, `serviceaccounts`, `clusterroles`, `clusterrolebindings`, `configmaps`, `nodes` 的 `get/list/watch`
  - 应急响应权限: `pods` 的 `delete`, `pods/eviction` 的 `create`, `networkpolicies` 的 `create/update`
  - 审计权限: 访问 Kubernetes 审计日志（通常需要节点文件系统访问或日志系统权限）
  - 验证命令: `kubectl auth can-i list secrets`
- **安全工具**: 部署运行时安全工具（Falco/Tetragon）、镜像扫描工具（Trivy/Grype）
- **安全工具**: 部署运行时安全工具（Falco/Tetragon）、镜像扫描工具（Trivy/Grype）
- **SSH/节点访问**: 取证阶段需要对受影响节点的特权访问
- **审计日志**: Kubernetes 审计日志已启用并可访问
- **证据存储**: 准备好安全的证据存储位置（非受感染系统）

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | Falco/Tetragon 告警：容器内可疑进程执行（如 shell、curl、wget、nc、/bin/sh -i）/ Runtime security alert: suspicious process execution in container | Falco 日志 `kubectl logs -n falco deploy/falco` 或 Tetragon events | 0.95 | 合法的调试操作（kubectl exec 由授权人员执行）；CI/CD 构建容器中的正常行为 |
| SP-02 | 特权容器或 hostPID/hostNetwork 异常使用 / Privileged container or hostPID/hostNetwork abuse | `kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: privileged={.spec.containers[*].securityContext.privileged}, hostPID={.spec.hostPID}, hostNetwork={.spec.hostNetwork}{"\n"}{end}'` | 0.85 | 合法的系统组件（CNI、CSI、监控 DaemonSet）；已知需要特权的工作负载 |
| SP-03 | 未知进程建立外部网络连接（特别是矿池端口 3333/4444/8333）/ Unknown process establishing external connections (especially mining pool ports) | 容器内 `ss -tnp` 或 eBPF 网络监控；Cilium Hubble 流日志 | 0.90 | 合法的外部 API 调用；CDN/对象存储访问 |
| SP-04 | 容器文件系统出现异常二进制文件 / Anomalous binary files in container filesystem | `kubectl exec POD -- find / -type f -executable -newer /proc/1/exe -not -path "/proc/*" -not -path "/sys/*" 2>/dev/null` | 0.85 | 应用正常的文件生成；构建时产生的临时文件 |
| SP-05 | 镜像安全扫描发现 CRITICAL/HIGH CVE / Image scan reveals CRITICAL/HIGH CVE | `trivy image IMAGE` 输出 CRITICAL/HIGH 漏洞；Harbor/ACR 安全扫描报告 | 0.80 | 漏洞已有缓解措施且不可利用；仅在非暴露组件中存在 |
| SP-06 | Secret 内容出现在日志/Git 仓库中 / Secret content exposed in logs or Git repository | `kubectl logs POD` 中出现 base64 解码后的敏感信息；GitHub/GitLab 安全扫描告警 | 0.95 | 测试环境的虚假凭据；已过期/已轮换的凭据 |
| SP-07 | 审计日志中异常 API 调用模式（大量 exec/attach/secrets 访问）/ Abnormal API call patterns in audit logs | 审计日志分析 `grep -E "exec|attach|secrets" /var/log/kubernetes/audit/audit.log` | 0.80 | 正常的批量运维操作；CI/CD 部署过程 |
| SP-08 | Pod Security Admission 拒绝事件增多 / Increased Pod Security Admission rejection events | `kubectl get events -A --field-selector reason=FailedCreate | grep -i security` | 0.75 | 新部署的应用配置不当；PSA 策略刚启用导致的过渡期问题 |
| SP-09 | NetworkPolicy 之外的异常网络流量 / Anomalous network traffic outside NetworkPolicy rules | Cilium Hubble flows 显示被 drop 的流量；Calico flow logs 异常连接 | 0.85 | NetworkPolicy 配置不完整；新服务尚未添加到策略 |
| SP-10 | ServiceAccount Token 被非预期 Pod 使用 / ServiceAccount Token used by unexpected Pod | 审计日志中 ServiceAccount 的 user.username 与预期 Pod 不匹配 | 0.90 | Token 在多个 Pod 间合法共享（不推荐但合法） |
| SP-11 | DNS 查询异常模式（高频、异常域名、tunneling 特征）/ Abnormal DNS query patterns (high frequency, suspicious domains, tunneling characteristics) | CoreDNS 日志分析；DNS 流量监控显示长子域名、高频查询、TXT 记录异常 | 0.85 | 合法的高频 DNS 服务发现；CDN/动态域名解析 |
| SP-12 | 镜像 digest 不匹配或未签名镜像运行 / Image digest mismatch or unsigned image running | `cosign verify IMAGE` 失败；`crane digest IMAGE` 与预期不符 | 0.90 | 镜像仓库同步延迟；签名策略尚未完全部署 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "Falco 告警说有可疑进程在容器里执行"
- "发现容器在连接挖矿矿池地址"
- "镜像扫描发现严重漏洞 CVE-XXXX"
- "有人在 Git 里提交了生产环境密钥"
- "Pod 被 PodSecurity 拒绝创建"
- "审计日志里发现大量异常的 exec 请求"
- "容器里出现了不明来源的二进制文件"
- "ServiceAccount 被其他 Pod 非法使用"
- "DNS 流量异常，怀疑有数据外泄"
- "容器可能发生了逃逸"

**English ticket descriptions**:
- "Falco detected suspicious shell execution in container"
- "Container connecting to known cryptomining pool"
- "Critical CVE found in production image"
- "Production secrets leaked to public Git repo"
- "Pod creation blocked by PodSecurityAdmission"
- "Abnormal exec/attach requests in audit logs"
- "Unknown binary appeared in container filesystem"
- "ServiceAccount token stolen or misused"
- "DNS tunneling suspected, possible data exfiltration"
- "Potential container escape detected"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 漏洞仅存在于开发/测试环境镜像 | 漏洞管理流程 | 非紧急安全事件，按正常漏洞修复流程处理 |
| Pod Security 拒绝是由于新应用配置错误 | SKILL-POD-001 | 配置问题，非安全事件 |
| 网络异常是由于 NetworkPolicy 配置缺失 | SKILL-NET-001 | 配置问题，非攻击 |
| 审计日志中的 exec 是合法运维操作（有工单记录） | 不适用本 Skill | 合法操作 |
| Secret 泄露已超过 30 天且凭据已轮换 | 安全审计复盘 | 历史事件，非应急响应 |
| 容器进程为已知的合法监控/诊断工具 | 不适用本 Skill | 误报 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断安全事件的严重性和影响范围：

**Step T1**: 确认事件类型和受影响范围（30 秒）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 快速检查受影响的 Pod/Namespace
kubectl get pods -A -o wide | grep -E "SUSPECT_POD_NAME|AFFECTED_NAMESPACE"

# 检查该 Pod 的安全上下文
kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{.spec.containers[*].securityContext}'

# 检查该 Pod 所在节点
kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{.spec.nodeName}'
```
> **判断规则**:
> - 如果 Pod 以 privileged=true 运行 → **P0**（容器逃逸风险极高）
> - 如果涉及生产 Namespace → **至少 P1**
> - 如果多个 Namespace/Pod 受影响 → 提升一级

**Step T2**: 隔离受影响工作负载（60 秒）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl apply/create/replace`：创建/变更集群资源

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
# 立即对可疑 Pod 应用 NetworkPolicy 隔离（阻止所有出入流量）
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: emergency-isolate-SUSPECT_POD
  namespace: NAMESPACE
spec:
  podSelector:
    matchLabels:
      app: SUSPECT_APP_LABEL
  policyTypes:
  - Ingress
  - Egress
EOF

# 如果节点可能被入侵，cordon 该节点
kubectl cordon NODE_NAME
```
> **判断规则**:
> - 如果隔离成功执行 → 继续 T3 收集证据
> - 如果隔离失败（如 NetworkPolicy 不支持）→ 考虑直接删除 Pod 或 drain 节点

**Step T3**: 收集初始证据（120 秒）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 保存 Pod 详细信息
kubectl describe pod SUSPECT_POD -n NAMESPACE > /tmp/evidence-pod-describe.txt

# 导出 Pod YAML
kubectl get pod SUSPECT_POD -n NAMESPACE -o yaml > /tmp/evidence-pod-yaml.txt

# 保存容器进程列表（如果 Pod 仍在运行）
kubectl exec SUSPECT_POD -n NAMESPACE -- ps aux > /tmp/evidence-processes.txt 2>/dev/null

# 保存网络连接
kubectl exec SUSPECT_POD -n NAMESPACE -- ss -tnp > /tmp/evidence-connections.txt 2>/dev/null

# 保存容器日志
kubectl logs SUSPECT_POD -n NAMESPACE --all-containers > /tmp/evidence-logs.txt
```
> **判断规则**:
> - 证据收集成功 → 继续详细诊断
> - 证据收集部分失败（容器已终止/不可访问）→ 转向节点级取证

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| 活跃的安全入侵证据（数据正在泄露、挖矿程序运行中、横向移动进行中） | **P0** | 需立即遏制攻击，防止进一步损害。数据泄露可能触发合规通知义务 | 立即响应，15min 内遏制 |
| 特权容器被入侵或检测到容器逃逸尝试 | **P0** | 攻击者可能已获得节点级访问权限，影响整个节点上的工作负载 | 立即响应，30min 内确认范围 |
| 已发现漏洞但无活跃利用证据（CRITICAL CVE 在生产运行） | **P1** | 存在被利用风险，需要紧急修复但非即时威胁 | 2h 内评估，24h 内修复 |
| Secret/凭据泄露但尚未发现被利用 | **P1** | 凭据可能被利用，需要立即轮换并监控 | 1h 内轮换，持续监控 |
| 合规偏差/策略违规（Pod Security 违规、未签名镜像） | **P2** | 降低安全态势但无直接威胁 | 24h 内修复 |
| 单次异常但无后续活动（可能是误报或被阻止的攻击） | **P2** | 需要调查但不紧急 | 48h 内调查 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至安全团队 / CISO**：

- **大规模入侵**: 多个节点检测到同一攻击特征（供应链攻击可能性）
- **数据泄露确认**: 有证据表明敏感数据已被外传
- **勒索软件**: 检测到文件加密行为或勒索信息
- **内部威胁**: 攻击来源指向内部人员（审计日志中有合法凭据的异常使用）
- **法律/合规影响**: 事件可能触发数据泄露通知义务（GDPR 72 小时、等保即时）
- **关键基础设施**: 受影响系统涉及支付、身份认证、核心业务数据

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速评估与隔离（5 分钟内完成）

> **目标**: 快速确认威胁存在并隔离受影响资源，防止攻击扩散。所有命令优先保护证据。
> **预计耗时**: 3-5 分钟

**Step D1.1**: 识别受影响的 Pod/Node
- **命令**:
  ```bash
  # 根据告警信息定位可疑 Pod
  kubectl get pods -A -o wide | grep -E "SUSPECT_PATTERN"
  
  # 或根据 Falco 告警中的 container ID 反查
  kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}:{range .status.containerStatuses[*]}{.containerID}{" "}{end}{"\n"}{end}' | grep CONTAINER_ID
  ```
- **超时**: 10s
- **预期输出模式**: Pod 名称、Namespace、所在 Node
- **判断规则**:
  - 找到可疑 Pod → 记录 Pod 名、Namespace、Node，继续 D1.2
  - 未找到 Pod（可能已被删除）→ 跳转 D2.8 节点级取证
  - 多个 Pod 匹配 → 按时间顺序处理，优先处理最近创建的

**Step D1.2**: 检查容器进程
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 如果 Pod 仍在运行，检查进程
  kubectl exec SUSPECT_POD -n NAMESPACE -- ps aux
  
  # 或使用 crictl 直接检查（需 SSH 到节点）
  ssh NODE_IP "crictl inspect CONTAINER_ID | jq '.info.pid'"
  ssh NODE_IP "ps -p PID -o pid,ppid,user,comm,args"
  ```
- **超时**: 15s
- **预期输出模式**: 进程列表，包含 PID、用户、命令
- **判断规则**:
  - 发现可疑进程（shell、miner、wget、curl、nc 等）→ 确认活跃入侵，升级为 P0
  - 仅有应用正常进程 → 可能是误报或攻击已停止，继续 D1.3
  - exec 失败（容器无 shell）→ 转向节点级检查 D2.8

**Step D1.3**: 检查网络连接
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查容器的网络连接
  kubectl exec SUSPECT_POD -n NAMESPACE -- ss -tnp 2>/dev/null || \
  kubectl exec SUSPECT_POD -n NAMESPACE -- cat /proc/net/tcp
  
  # 检查是否有到已知恶意 IP 的连接
  kubectl exec SUSPECT_POD -n NAMESPACE -- ss -tnp | grep -E "3333|4444|8333|14444|45700"
  ```
- **超时**: 10s
- **预期输出模式**: 网络连接列表
- **判断规则**:
  - 发现到矿池端口（3333/4444/8333/14444/45700）的连接 → RC-008（挖矿）
  - 发现到异常外部 IP 的连接 → 需进一步验证 IP 声誉
  - 无异常连接 → 继续 D1.4

**Step D1.4**: 检查文件系统异常
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 查找最近创建/修改的可执行文件
  kubectl exec SUSPECT_POD -n NAMESPACE -- find / -type f -executable \
    -newer /proc/1/exe -not -path "/proc/*" -not -path "/sys/*" 2>/dev/null
  
  # 查找隐藏文件
  kubectl exec SUSPECT_POD -n NAMESPACE -- find / -name ".*" -type f \
    -not -path "/proc/*" -not -path "/sys/*" 2>/dev/null | head -50
  ```
- **超时**: 30s
- **预期输出模式**: 文件列表
- **判断规则**:
  - 发现异常可执行文件（/tmp、/dev/shm 下的二进制）→ 保存文件 hash，确认恶意软件
  - 发现大量隐藏文件 → 可能是 rootkit，升级处理
  - 无异常 → 继续 D1.5

**Step D1.5**: 检查安全上下文
- **命令**:
  ```bash
  # 检查 Pod 的安全上下文
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{
    "privileged": {.spec.containers[*].securityContext.privileged},
    "hostPID": {.spec.hostPID},
    "hostNetwork": {.spec.hostNetwork},
    "hostIPC": {.spec.hostIPC},
    "runAsRoot": {.spec.containers[*].securityContext.runAsUser},
    "capabilities": {.spec.containers[*].securityContext.capabilities}
  }'
  
  # 检查挂载点（可能有敏感主机路径）
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{range .spec.volumes[*]}{.name}: {.hostPath.path}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: 安全配置信息
- **判断规则**:
  - `privileged=true` → 容器逃逸风险极高（RC-001），立即升级为 P0
  - `hostPID=true` 或 `hostNetwork=true` → 可能已有主机访问能力
  - 挂载了敏感路径（/etc、/var/run/docker.sock）→ 容器逃逸风险
  - 安全上下文正常 → 继续 D1.6

**Step D1.6**: 快速隔离措施
- **命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl cordon`：标记节点不可调度
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 方案 1: 应用 NetworkPolicy 隔离（推荐，保留取证能力）
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: emergency-isolate-$(date +%s)
    namespace: NAMESPACE
  spec:
    podSelector:
      matchLabels:
        app: SUSPECT_APP_LABEL
    policyTypes:
    - Ingress
    - Egress
  EOF
  
  # 方案 2: 如果需要更强隔离，cordon 所在节点
  kubectl cordon NODE_NAME
  
  # 方案 3: 极端情况下，删除 Pod（会丢失部分证据）
  # kubectl delete pod SUSPECT_POD -n NAMESPACE --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
  ```
- **超时**: 15s
- **风险级别**: 🟡 中（NetworkPolicy 可能影响合法流量；cordon 会阻止新 Pod 调度）
- **判断规则**:
  - NetworkPolicy 成功应用 → 隔离完成，进入 Phase 2 深度取证
  - 节点已 cordon → 考虑是否需要 drain（取决于攻击严重性）
  - 隔离失败 → 升级处理

---

### Phase 2: 深度取证（只读为主，保全证据）

> **目标**: 收集完整证据链，分析攻击路径和影响范围。所有操作以只读为主，避免破坏证据。
> **前提**: Phase 1 隔离已完成，或确认无活跃威胁
> **预计耗时**: 15-30 分钟

**Step D2.1**: 审计日志分析
- **命令**:
  ```bash
  # 检查 kube-apiserver 审计日志（如果直接访问）
  kubectl logs -n kube-system -l component=kube-apiserver --tail=1000 | \
    grep -iE "exec|attach|create.*secret|delete.*secret" | tail -100
  
  # 或从审计日志文件分析
  grep -E "exec|attach" /var/log/kubernetes/audit/audit.log | \
    jq -r 'select(.verb=="create") | "\(.requestReceivedTimestamp) \(.user.username) \(.verb) \(.objectRef.resource)/\(.objectRef.name)"' | tail -50
  
  # 检查特定时间范围内的异常活动
  grep "SUSPECT_POD" /var/log/kubernetes/audit/audit.log | \
    jq -r '"\(.requestReceivedTimestamp) \(.user.username) \(.verb) \(.objectRef.resource)"'
  ```
- **超时**: 30s
- **预期输出模式**: 审计事件列表
- **判断规则**:
  - 大量 exec/attach 请求来自同一 ServiceAccount → 可能是自动化攻击或被盗用
  - 异常时间段的活动（非工作时间）→ 可疑，需关联其他证据
  - 发现对 secrets 的异常访问 → RC-004（凭据泄露风险）

**Step D2.2**: 容器镜像验证
- **命令**:
  ```bash
  # 获取 Pod 使用的镜像
  IMAGE=$(kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{.spec.containers[0].image}')
  
  # 验证镜像签名（需要 cosign）
  cosign verify --key /path/to/cosign.pub $IMAGE
  
  # 获取镜像 digest 并与预期对比
  crane digest $IMAGE
  
  # 检查镜像是否在允许列表中
  kubectl get pods -A -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | sort -u | grep -v "approved-registry"
  ```
- **超时**: 30s
- **预期输出模式**: 签名验证结果、镜像 digest
- **判断规则**:
  - 签名验证失败 → RC-003（可能是恶意镜像）
  - digest 与预期不符 → 镜像可能被篡改
  - 来自未知仓库 → 需要额外审查

**Step D2.3**: 镜像漏洞扫描
- **命令**:
  ```bash
  # 使用 Trivy 扫描镜像漏洞
  trivy image --severity CRITICAL,HIGH $IMAGE
  
  # 或使用 Grype
  grype $IMAGE
  
  # 如果本地有镜像，检查 SBOM
  syft $IMAGE -o spdx-json > /tmp/sbom.json
  ```
- **超时**: 120s（镜像扫描可能较慢）
- **预期输出模式**: 漏洞列表、SBOM
- **判断规则**:
  - 发现 CRITICAL CVE 且有已知 exploit → RC-002（CVE 利用）
  - 发现恶意包或被注入的依赖 → RC-007（供应链攻击）
  - 无高危漏洞 → 漏洞可能不是入口点

**Step D2.4**: SBOM 验证
- **命令**:
  ```bash
  # 生成并验证 SBOM
  syft $IMAGE -o cyclonedx-json > /tmp/sbom-current.json
  
  # 与基线对比（如果有）
  diff /tmp/sbom-baseline.json /tmp/sbom-current.json
  
  # 检查是否有异常依赖
  jq '.components[] | select(.name | test("(crypto|miner|backdoor)"; "i"))' /tmp/sbom-current.json
  ```
- **超时**: 60s
- **预期输出模式**: SBOM 组件列表
- **判断规则**:
  - 发现异常组件（与基线不符）→ RC-007（供应链攻击）
  - SBOM 与预期一致 → 排除供应链问题

**Step D2.5**: 运行时安全事件检查
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查 Falco 日志
  kubectl logs -n falco deploy/falco --since=1h | grep -i "SUSPECT_POD"
  
  # 检查 Tetragon 事件
  kubectl logs -n kube-system ds/tetragon --since=1h | grep -i "SUSPECT"
  
  # 检查 Falco 事件计数
  kubectl exec -n falco deploy/falco -- falcoctl events \
    --filter "container.name=SUSPECT_CONTAINER" --last 1h
  ```
- **超时**: 30s
- **预期输出模式**: 安全事件列表
- **判断规则**:
  - 多个安全规则触发 → 攻击活动活跃，需立即遏制
  - 特定规则触发（如 "Terminal shell in container"）→ 定位攻击类型
  - 无相关事件 → 可能是新型攻击或运行时监控未覆盖

**Step D2.6**: ServiceAccount Token 审计
- **命令**:
  ```bash
  # 检查 Pod 使用的 ServiceAccount
  SA=$(kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{.spec.serviceAccountName}')
  
  # 检查该 SA 的权限
  kubectl auth can-i --list --as=system:serviceaccount:NAMESPACE:$SA
  
  # 检查该 SA 的 token 被哪些 Pod 挂载
  kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.serviceAccountName}{"\n"}{end}' | grep $SA
  
  # 检查 SA 的 secrets
  kubectl get serviceaccount $SA -n NAMESPACE -o jsonpath='{.secrets[*].name}'
  ```
- **超时**: 20s
- **预期输出模式**: SA 权限列表和使用情况
- **判断规则**:
  - SA 具有过多权限（cluster-admin 级别）→ RC-005（RBAC 过宽）
  - Token 被多个 Pod 使用且有异常 Pod → RC-004（Token 泄露/滥用）
  - 权限符合最小权限原则 → SA 配置合理

**Step D2.7**: 网络流量分析
- **命令**:
  ```bash
  # 如果使用 Cilium，检查 Hubble 流日志
  hubble observe --namespace NAMESPACE --pod SUSPECT_POD --last 1h
  
  # 如果使用 Calico，检查 flow logs
  kubectl logs -n calico-system ds/calico-node --since=1h | grep "SUSPECT_POD_IP"
  
  # 检查 NetworkPolicy 是否生效
  kubectl get networkpolicy -n NAMESPACE
  kubectl describe networkpolicy -n NAMESPACE
  
  # 检查 DNS 查询日志
  kubectl logs -n kube-system deploy/coredns --since=1h | grep "SUSPECT_POD_IP"
  ```
- **超时**: 30s
- **预期输出模式**: 网络流日志
- **判断规则**:
  - 发现到已知恶意 IP 的流量 → 确认外连行为
  - 大量 DNS 查询到异常域名 → RC-009（DNS 隧道）
  - 内部横向连接异常 → RC-006（横向移动）

**Step D2.8**: 节点级取证
- **命令**:
  ```bash
  # SSH 到受影响节点后执行
  
  # 检查 kubelet 日志
  ssh NODE_IP "journalctl -u kubelet --since '2 hours ago' | grep -iE 'error|warning|exec|attach'"
  
  # 检查系统审计日志
  ssh NODE_IP "ausearch -m execve --start today | grep -iE 'miner|shell|curl|wget|nc' | tail -50"
  
  # 检查可疑进程
  ssh NODE_IP "ps auxf | grep -iE 'miner|xmr|stratum' | grep -v grep"
  
  # 检查异常网络连接
  ssh NODE_IP "ss -tnp | grep -E ':3333|:4444|:8333'"
  
  # 检查异常 cron 任务
  ssh NODE_IP "crontab -l; ls -la /etc/cron.d/; cat /etc/cron.d/*"
  
  # 检查是否有 rootkit 痕迹
  ssh NODE_IP "rkhunter --check --skip-keypress 2>/dev/null || echo 'rkhunter not installed'"
  ```
- **超时**: 60s
- **预期输出模式**: 系统级日志和进程信息
- **判断规则**:
  - 在节点层面发现恶意进程 → 容器逃逸已发生（RC-012）
  - 发现持久化机制（cron、systemd service）→ 攻击者试图保持访问
  - 节点层面无异常 → 攻击可能被限制在容器内

---

### Phase 3: 影响范围评估

> **目标**: 确定攻击的完整影响范围，为修复和报告提供依据
> **预计耗时**: 10-20 分钟

**Step D3.1**: 受影响的 Namespace/Deployment 列表
- **命令**:
  ```bash
  # 列出所有使用相同镜像的 Deployment
  AFFECTED_IMAGE="IMAGE:TAG"
  kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.containers[*].image}{"\n"}{end}' | grep "$AFFECTED_IMAGE"
  
  # 列出所有使用相同 ServiceAccount 的 Pod
  kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.serviceAccountName}{"\n"}{end}' | grep "$SA"
  
  # 检查同一节点上的所有 Pod（如果节点可能被入侵）
  kubectl get pods -A --field-selector spec.nodeName=NODE_NAME -o wide
  ```
- **超时**: 20s
- **判断规则**:
  - 多个 Namespace 使用相同受影响镜像 → 影响范围扩大
  - 关键系统 Namespace（kube-system）受影响 → 升级处理

**Step D3.2**: 受影响的 Secret/ConfigMap 范围
- **命令**:
  ```bash
  # 检查可疑 Pod 挂载的 Secret
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{range .spec.volumes[*]}{.secret.secretName}{"\n"}{end}'
  
  # 检查这些 Secret 还被哪些其他 Pod 使用
  for secret in $(kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{range .spec.volumes[*]}{.secret.secretName}{" "}{end}'); do
    echo "Secret: $secret is used by:"
    kubectl get pods -n NAMESPACE -o jsonpath='{range .items[*]}{range .spec.volumes[*]}{.secret.secretName}{end} -> {.metadata.name}{"\n"}{end}' | grep $secret
  done
  
  # 检查 Secret 的最近访问记录（审计日志）
  grep "secrets/$SECRET_NAME" /var/log/kubernetes/audit/audit.log | tail -20
  ```
- **超时**: 30s
- **判断规则**:
  - Secret 被多个系统使用 → 泄露影响范围大
  - 审计日志显示异常访问模式 → 确认 Secret 可能已泄露

**Step D3.3**: 数据泄露范围评估
- **命令**:
  ```bash
  # 检查可疑 Pod 可以访问的数据资源
  # 1. 检查 PVC 挂载
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{range .spec.volumes[*]}{.persistentVolumeClaim.claimName}{"\n"}{end}'
  
  # 2. 检查 ConfigMap（可能包含敏感配置）
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{range .spec.volumes[*]}{.configMap.name}{"\n"}{end}'
  
  # 3. 检查 SA 可以访问的其他资源
  kubectl auth can-i --list --as=system:serviceaccount:NAMESPACE:$SA -n NAMESPACE
  ```
- **超时**: 20s
- **判断规则**:
  - Pod 可以访问生产数据库凭据 → 数据泄露风险高
  - Pod 具有集群级访问权限 → 任何资源都可能被访问

**Step D3.4**: 横向移动路径分析
- **命令**:
  ```bash
  # 检查从当前 Pod/SA 可以访问的其他资源
  kubectl auth can-i create pods --as=system:serviceaccount:NAMESPACE:$SA
  kubectl auth can-i get secrets --as=system:serviceaccount:NAMESPACE:$SA
  kubectl auth can-i exec pods --as=system:serviceaccount:NAMESPACE:$SA
  
  # 检查 NetworkPolicy 允许的连接目标
  kubectl get pods -n NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}: {.status.podIP}{"\n"}{end}'
  
  # 检查同 Namespace 内的服务
  kubectl get svc -n NAMESPACE
  ```
- **超时**: 20s
- **判断规则**:
  - SA 可以 exec 到其他 Pod → 横向移动路径存在
  - NetworkPolicy 未限制 namespace 内流量 → 可访问其他 Pod

**Step D3.5**: 时间线重建
- **命令**:
  ```bash
  # 重建事件时间线
  echo "=== Timeline ==="
  
  # Pod 创建时间
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='Pod created: {.metadata.creationTimestamp}'
  
  # 容器启动时间
  kubectl get pod SUSPECT_POD -n NAMESPACE -o jsonpath='{range .status.containerStatuses[*]}Container {.name} started: {.state.running.startedAt}{"\n"}{end}'
  
  # 相关事件
  kubectl get events -n NAMESPACE --sort-by=.lastTimestamp | grep SUSPECT_POD | tail -20
  
  # 首次安全告警时间（从 Falco 日志）
  kubectl logs -n falco deploy/falco | grep SUSPECT_POD | head -5
  
  # 最近的审计日志条目
  grep SUSPECT_POD /var/log/kubernetes/audit/audit.log | jq -r '.requestReceivedTimestamp' | sort | head -5
  ```
- **超时**: 30s
- **判断规则**:
  - 攻击窗口（从首次异常到检测）> 1 小时 → 影响可能更广
  - 攻击窗口 < 15 分钟 → 快速检测，影响可能有限

**Step D3.6**: 合规影响评估
- **命令**:
  ```bash
  # 检查受影响的 Namespace 是否有合规标签
  kubectl get namespace NAMESPACE -o jsonpath='{.metadata.labels}' | grep -iE "pii|gdpr|hipaa|pci|sensitive"
  
  # 检查受影响的 Secret 类型
  kubectl get secret -n NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}: {.type}{"\n"}{end}'
  
  # 检查是否涉及客户数据
  # （这需要结合业务上下文判断）
  ```
- **超时**: 10s
- **判断规则**:
  - 涉及 PII/GDPR 标签的 Namespace → 可能触发数据泄露通知义务
  - 涉及支付/认证相关 Secret → 需要即时通知安全团队和合规团队

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 风险 | 诊断证据 | FTA 映射 |
|--------|------|------|------|---------|---------|
| RC-001 | **容器以特权模式运行被利用** — 攻击者利用 privileged 容器获取主机访问权限，可能导致完全的集群接管 | ~15% | ⚫ 严重 | D1.5 显示 privileged=true；D2.8 节点层发现异常进程 | security-fta: BE-privileged-container |
| RC-002 | **已知 CVE 漏洞被利用** — 攻击者利用公开的 CVE 漏洞获取容器内代码执行或权限提升 | ~15% | 🔴 高 | D2.3 发现 CRITICAL CVE 且有 PoC；D2.1/D2.5 显示相关攻击痕迹 | security-fta: BE-cve-exploit |
| RC-003 | **恶意/被篡改的容器镜像** — 镜像在构建或分发过程中被注入恶意代码，或使用了恶意的基础镜像 | ~12% | ⚫ 严重 | D2.2 签名验证失败；D2.4 SBOM 显示异常组件；D2.3 发现嵌入式恶意代码 | security-fta: BE-malicious-image |
| RC-004 | **Secret 明文暴露** — 敏感凭据通过日志、环境变量、Git 仓库或其他渠道泄露 | ~10% | 🔴 高 | D2.1 审计日志显示 Secret 被异常访问；外部渠道发现泄露证据 | security-fta: BE-secret-leak |
| RC-005 | **RBAC 权限过宽** — ServiceAccount 被授予过多权限，攻击者可利用获取更大的攻击面 | ~8% | 🟡 中 | D2.6 显示 SA 具有 cluster-admin 或 secrets access；D3.4 显示可横向移动 | security-fta: BE-rbac-over-permissive |
| RC-006 | **NetworkPolicy 缺失导致横向移动** — 未配置或配置不当的 NetworkPolicy 允许攻击者在 Pod 间移动 | ~7% | 🟡 中 | D2.7 显示无 NetworkPolicy；D3.4 可连接到其他敏感 Pod | security-fta: BE-no-network-policy |
| RC-007 | **供应链依赖被注入恶意代码** — 应用依赖的第三方库或构建工具被投毒 | ~6% | ⚫ 严重 | D2.4 SBOM 对比发现新增/变更的依赖；D2.3 发现依赖中的恶意代码 | security-fta: BE-supply-chain |
| RC-008 | **加密挖矿（cryptominer）部署** — 攻击者在容器中部署挖矿程序消耗计算资源 | ~6% | 🔴 高 | D1.2 发现挖矿进程；D1.3 发现到矿池的连接；D2.5 Falco 告警 cryptomining | security-fta: BE-cryptominer |
| RC-009 | **DNS 隧道数据外泄** — 攻击者通过 DNS 查询隐蔽地外传数据 | ~5% | 🔴 高 | D2.7 DNS 日志显示异常模式（长子域名、高频 TXT 查询） | security-fta: BE-dns-tunnel |
| RC-010 | **Pod Security Standards 未启用/配置过松** — 缺乏运行时安全策略，允许危险配置的 Pod 运行 | ~5% | 🟡 中 | D1.5 显示危险配置被允许；无 PSA enforcement | security-fta: BE-pss-not-enforced |
| RC-011 | **过期/泄露的 ServiceAccount Token** — Token 被窃取或长期未轮换导致被滥用 | ~5% | 🟡 中 | D2.6 Token 在多个位置被使用；D2.1 审计日志显示异常来源 | security-fta: BE-token-leak |
| RC-012 | **容器逃逸** — 攻击者利用内核漏洞或配置错误（如 hostPath 挂载）突破容器边界 | ~3% | ⚫ 严重 | D2.8 在节点层发现攻击进程；D1.5 有危险的 volume mount | security-fta: BE-container-escape |
| RC-013 | **Admission Controller 绕过** — 攻击者找到方法绕过准入控制器的安全检查 | ~3% | 🔴 高 | 违规 Pod 成功创建但 PSA/OPA 应该阻止；D2.1 审计日志显示绕过尝试 | security-fta: BE-admission-bypass |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 启用/加强 Pod Security Standards
- **适用根因**: RC-010
- **前置检查**:
  ```bash
  # 检查当前 Namespace 的 PSA 配置
  kubectl get namespace NAMESPACE -o jsonpath='{.metadata.labels}' | grep -i "pod-security"
  
  # 检查集群级别的 PSA 配置
  kubectl get admissionconfiguration -o yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 为 Namespace 添加 PSA 标签（warn 模式，不阻止但告警）
  kubectl label namespace NAMESPACE pod-security.kubernetes.io/warn=restricted --overwrite
  
  # 如需强制执行（会阻止违规 Pod）
  kubectl label namespace NAMESPACE pod-security.kubernetes.io/enforce=baseline --overwrite
  kubectl label namespace NAMESPACE pod-security.kubernetes.io/enforce-version=latest --overwrite
  ```
- **后置验证**:
  ```bash
  kubectl get namespace NAMESPACE -o jsonpath='{.metadata.labels}'
  # 预期: 包含 pod-security.kubernetes.io/enforce=baseline
  
  # 测试：尝试创建违规 Pod（应该被拒绝或告警）
  kubectl run test-privileged --image=nginx --privileged -n NAMESPACE --dry-run=server
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  kubectl label namespace NAMESPACE pod-security.kubernetes.io/enforce- pod-security.kubernetes.io/warn-
  ```

#### REM-002: 部署/更新 NetworkPolicy 隔离
- **适用根因**: RC-006
- **前置检查**:
  ```bash
  # 检查现有 NetworkPolicy
  kubectl get networkpolicy -n NAMESPACE
  
  # 检查 CNI 是否支持 NetworkPolicy
  kubectl get pods -n kube-system -l k8s-app=calico-node -o name || \
  kubectl get pods -n kube-system -l k8s-app=cilium -o name
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 部署默认拒绝所有入站流量的 NetworkPolicy
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: default-deny-ingress
    namespace: NAMESPACE
  spec:
    podSelector: {}
    policyTypes:
    - Ingress
  EOF
  
  # 部署默认拒绝所有出站流量（更严格）
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: default-deny-egress
    namespace: NAMESPACE
  spec:
    podSelector: {}
    policyTypes:
    - Egress
  EOF
  
  # 允许必要的出站（DNS）
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-dns-egress
    namespace: NAMESPACE
  spec:
    podSelector: {}
    policyTypes:
    - Egress
    egress:
    - to:
      - namespaceSelector:
          matchLabels:
            kubernetes.io/metadata.name: kube-system
      ports:
      - protocol: UDP
        port: 53
  EOF
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get networkpolicy -n NAMESPACE
  # 预期: 显示已创建的 NetworkPolicy
  
  # 测试网络隔离效果
  kubectl exec test-pod -n NAMESPACE -- curl -s --max-time 5 external-service.other-namespace.svc || echo "Blocked as expected"
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete networkpolicy default-deny-ingress default-deny-egress allow-dns-egress -n NAMESPACE
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-003: Secret 紧急轮换
- **适用根因**: RC-004, RC-011
- **影响说明**: 轮换 Secret 会导致使用该 Secret 的应用暂时无法访问相关服务，直到应用重启或重新加载配置。需要协调相关服务的重启。
- **审批提示**: "检测到 Secret `SECRET_NAME` 可能已泄露，建议立即轮换。轮换后使用该 Secret 的 Pod 需要重启。是否批准？"
- **前置检查**:
  ```bash
  # 确认泄露的 Secret
  kubectl get secret SECRET_NAME -n NAMESPACE
  
  # 列出使用该 Secret 的 Pod
  kubectl get pods -n NAMESPACE -o jsonpath='{range .items[*]}{range .spec.volumes[*]}{.secret.secretName}{end} -> {.metadata.name}{"\n"}{end}' | grep SECRET_NAME
  
  # 备份现有 Secret（用于紧急回滚）
  kubectl get secret SECRET_NAME -n NAMESPACE -o yaml > /tmp/secret-backup-$(date +%s).yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 生成新的凭据值（示例：数据库密码）
  NEW_PASSWORD=$(openssl rand -base64 32)
  
  # 更新 Secret
  kubectl create secret generic SECRET_NAME \
    --from-literal=password="$NEW_PASSWORD" \
    --dry-run=client -o yaml | kubectl apply -f -
  
  # 或直接 patch
  kubectl patch secret SECRET_NAME -n NAMESPACE -p "{\"data\":{\"password\":\"$(echo -n $NEW_PASSWORD | base64)\"}}"
  
  # 触发使用该 Secret 的 Deployment 重启
  kubectl rollout restart deployment DEPLOYMENT_NAME -n NAMESPACE
  ```
- **后置验证**:
  ```bash
  # 确认 Secret 已更新
  kubectl get secret SECRET_NAME -n NAMESPACE -o jsonpath='{.metadata.resourceVersion}'
  
  # 确认 Pod 已重启并使用新 Secret
  kubectl get pods -n NAMESPACE -l app=AFFECTED_APP
  
  # 验证应用功能正常
  kubectl logs -l app=AFFECTED_APP -n NAMESPACE --tail=20
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl apply -f /tmp/secret-backup-*.yaml
  kubectl rollout restart deployment DEPLOYMENT_NAME -n NAMESPACE
  ```

#### REM-004: RBAC 权限收紧
- **适用根因**: RC-005
- **影响说明**: 收紧 RBAC 权限可能导致部分功能失效。需要仔细审查当前权限使用情况。
- **审批提示**: "ServiceAccount `SA_NAME` 权限过宽，建议收紧到最小权限。此操作可能影响依赖这些权限的功能。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前 SA 的所有 RoleBinding/ClusterRoleBinding
  kubectl get rolebindings,clusterrolebindings -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {range .subjects[*]}{.kind}/{.name} {end}-> {.roleRef.name}{"\n"}{end}' | grep SA_NAME
  
  # 查看权限详情
  kubectl auth can-i --list --as=system:serviceaccount:NAMESPACE:SA_NAME
  
  # 审计该 SA 最近的实际使用
  grep "serviceaccount:NAMESPACE:SA_NAME" /var/log/kubernetes/audit/audit.log | \
    jq -r '.verb + " " + .objectRef.resource' | sort | uniq -c | sort -rn
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 创建新的最小权限 Role
  cat <<EOF | kubectl apply -f -
  apiVersion: rbac.authorization.k8s.io/v1
  kind: Role
  metadata:
    name: SA_NAME-minimal
    namespace: NAMESPACE
  rules:
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list"]
  # 添加实际需要的最小权限
  EOF
  
  # 删除旧的过宽绑定
  kubectl delete rolebinding OLD_BINDING_NAME -n NAMESPACE
  
  # 创建新的绑定
  kubectl create rolebinding SA_NAME-minimal \
    --role=SA_NAME-minimal \
    --serviceaccount=NAMESPACE:SA_NAME \
    -n NAMESPACE
  ```
- **后置验证**:
  ```bash
  # 验证新权限
  kubectl auth can-i --list --as=system:serviceaccount:NAMESPACE:SA_NAME
  
  # 监控应用日志确认无权限错误
  kubectl logs -l app=AFFECTED_APP -n NAMESPACE | grep -i "forbidden|unauthorized"
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 记录操作前需要先备份原有 RoleBinding
  kubectl apply -f /tmp/rolebinding-backup.yaml
  kubectl delete role SA_NAME-minimal -n NAMESPACE
  ```

#### REM-005: 镜像策略加固（仅允许签名镜像）
- **适用根因**: RC-003, RC-007
- **影响说明**: 启用镜像签名验证后，未签名的镜像将无法部署。需要确保所有合法镜像已签名。
- **审批提示**: "建议启用镜像签名验证策略，阻止未签名镜像运行。这可能影响尚未签名的镜像部署。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前运行的镜像签名状态
  for pod in $(kubectl get pods -A -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | sort -u); do
    echo -n "$pod: "
    cosign verify --key /path/to/cosign.pub $pod 2>/dev/null && echo "SIGNED" || echo "NOT SIGNED"
  done
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 使用 Kyverno 部署镜像签名验证策略
  cat <<EOF | kubectl apply -f -
  apiVersion: kyverno.io/v1
  kind: ClusterPolicy
  metadata:
    name: verify-image-signature
  spec:
    validationFailureAction: Enforce
    background: false
    rules:
    - name: verify-signature
      match:
        any:
        - resources:
            kinds:
            - Pod
      verifyImages:
      - imageReferences:
        - "your-registry.com/*"
        attestors:
        - entries:
          - keys:
              publicKeys: |
                -----BEGIN PUBLIC KEY-----
                YOUR_PUBLIC_KEY_HERE
                -----END PUBLIC KEY-----
  EOF
  
  # 或使用 OPA/Gatekeeper 约束
  ```
- **后置验证**:
  ```bash
  # 测试：尝试部署未签名镜像
  kubectl run test-unsigned --image=unsigned-image:latest --dry-run=server
  # 预期: 被策略拒绝
  
  # 验证已签名镜像可以正常部署
  kubectl run test-signed --image=signed-image:latest --dry-run=server
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete clusterpolicy verify-image-signature
  ```

#### REM-006: 部署运行时安全工具（Falco/Tetragon）
- **适用根因**: 预防性措施，适用于发现安全监控盲区时
- **影响说明**: 部署运行时安全工具会增加节点资源消耗（CPU ~100m-200m, Memory ~256Mi-512Mi per node）。
- **审批提示**: "建议部署运行时安全监控工具 Falco/Tetragon 以增强检测能力。这将在每个节点上运行 DaemonSet。是否批准？"
- **前置检查**:
  ```bash
  # 检查是否已部署
  kubectl get pods -n falco -l app=falco
  kubectl get pods -n kube-system -l app.kubernetes.io/name=tetragon
  
  # 检查节点资源余量
  kubectl top nodes
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

  ```bash
  # 使用 Helm 部署 Falco
  helm repo add falcosecurity https://falcosecurity.github.io/charts
  helm repo update
  helm install falco falcosecurity/falco \
    --namespace falco --create-namespace \
    --set falcosidekick.enabled=true \
    --set falcosidekick.webui.enabled=true
  
  # 或部署 Tetragon
  helm repo add cilium https://helm.cilium.io
  helm install tetragon cilium/tetragon \
    --namespace kube-system
  ```
- **后置验证**:
  ```bash
  # 验证 Falco 运行状态
  kubectl get pods -n falco
  kubectl logs -n falco deploy/falco --tail=50
  
  # 测试告警（执行可疑命令）
  kubectl run test-shell --image=alpine --rm -it --restart=Never -- sh -c "cat /etc/shadow"
  # 预期: Falco 产生告警
  ```
- **回滚命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

  ```bash
  helm uninstall falco -n falco  # ⚠️ 删除 release 及关联资源
  kubectl delete namespace falco  # ⚠️ 不可逆：永久删除命名空间及全部资源
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-007: 受感染 Pod/Node 隔离与清除
- **适用根因**: RC-001, RC-002, RC-008, RC-012
- **影响说明**: 隔离和清除受感染资源会导致该资源上的所有工作负载中断。需要确保业务连续性计划就位。
- **操作步骤**:
  1. **确认已完成证据保全**:
     ```bash
     # 确认所有取证数据已保存到安全位置
     ls -la /tmp/evidence-*
     ```
  2. **应用严格隔离**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 如果尚未隔离，应用最严格的 NetworkPolicy
     kubectl apply -f - <<EOF
     apiVersion: networking.k8s.io/v1
     kind: NetworkPolicy
     metadata:
       name: total-isolation
       namespace: NAMESPACE
     spec:
       podSelector:
         matchLabels:
           app: INFECTED_APP
       policyTypes:
       - Ingress
       - Egress
     EOF
     ```
  3. **Cordon 并 Drain 受影响节点**（如果节点可能被入侵）:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

     ```bash
     kubectl cordon INFECTED_NODE
     kubectl drain INFECTED_NODE --ignore-daemonsets --delete-emptydir-data --force --grace-period=60
     ```
  4. **删除受感染 Pod**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

     ```bash
     # 强制删除（不等待优雅终止）
     kubectl delete pod INFECTED_POD -n NAMESPACE --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
     ```
  5. **清理节点**（如果节点层被入侵）:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

     ```bash
     # SSH 到节点
     ssh NODE_IP
     
     # 杀死可疑进程
     kill -9 PID_OF_MALICIOUS_PROCESS
     
     # 删除持久化机制
     rm -f /etc/cron.d/malicious_cron
     systemctl stop malicious_service && systemctl disable malicious_service
     
     # 扫描并清除恶意文件
     find / -type f -executable -newer /proc/1/exe -not -path "/proc/*" -exec rm -f {} \; 2>/dev/null
     
     # 重启节点以确保清除
     reboot
     ```
- **安全检查**:
  - 确认所有证据已保存
  - 确认业务已切换到冗余实例
  - 通知相关团队
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 如果误删除了正常 Pod，重新部署
  kubectl apply -f deployment.yaml
  
  # 恢复节点调度
  kubectl uncordon NODE_NAME
  ```

#### REM-008: 恶意镜像清除与替换
- **适用根因**: RC-003, RC-007
- **影响说明**: 替换镜像需要重新部署应用，可能导致服务短暂中断。
- **操作步骤**:
  1. **确定安全的镜像版本**:
     ```bash
     # 检查镜像仓库中的历史版本
     crane ls IMAGE_REPO --full-ref
     
     # 验证历史版本的签名
     cosign verify --key /path/to/cosign.pub IMAGE_REPO:SAFE_TAG
     ```
  2. **更新 Deployment 使用安全镜像**:
     ```bash
     kubectl set image deployment/DEPLOYMENT_NAME \
       CONTAINER_NAME=IMAGE_REPO:SAFE_TAG \
       -n NAMESPACE
     ```
  3. **等待滚动更新完成**:
     ```bash
     kubectl rollout status deployment/DEPLOYMENT_NAME -n NAMESPACE
     ```
  4. **从镜像仓库删除/标记恶意镜像**:
     ```bash
     # 删除恶意镜像（具体命令取决于镜像仓库）
     # Harbor 示例:
     curl -X DELETE "https://harbor.example.com/api/v2.0/projects/PROJECT/repositories/REPO/artifacts/DIGEST"
     
     # 或添加标记
     crane tag IMAGE_REPO:MALICIOUS IMAGE_REPO:QUARANTINED
     ```
  5. **更新准入策略阻止恶意镜像**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 添加黑名单规则
     kubectl apply -f - <<EOF
     apiVersion: kyverno.io/v1
     kind: ClusterPolicy
     metadata:
       name: block-malicious-image
     spec:
       validationFailureAction: Enforce
       rules:
       - name: block-image
         match:
           any:
           - resources:
               kinds:
               - Pod
         validate:
           message: "This image is blocked due to security concerns"
           pattern:
             spec:
               containers:
               - image: "!IMAGE_REPO:MALICIOUS_TAG"
     EOF
     ```
- **安全检查**:
  - 新镜像经过安全扫描
  - 新镜像签名有效
  - 应用功能正常
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 如果新镜像有问题，回滚到上一个已知安全版本
  kubectl rollout undo deployment/DEPLOYMENT_NAME -n NAMESPACE
  ```

#### REM-009: ServiceAccount Token 强制轮换
- **适用根因**: RC-011
- **影响说明**: 轮换 SA Token 会使现有 Token 失效，使用该 Token 的所有 Pod 需要重启。
- **操作步骤**:
  1. **列出所有使用该 SA 的 Pod**:
     ```bash
     kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.serviceAccountName}{"\n"}{end}' | grep SA_NAME
     ```
  2. **删除 SA 对应的 Secret（触发轮换）**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 对于 legacy token（K8s <1.24）
     TOKEN_SECRET=$(kubectl get sa SA_NAME -n NAMESPACE -o jsonpath='{.secrets[0].name}')
     kubectl delete secret $TOKEN_SECRET -n NAMESPACE
     
     # 对于 BoundServiceAccountToken（K8s 1.24+），需要重建 SA
     kubectl get sa SA_NAME -n NAMESPACE -o yaml > /tmp/sa-backup.yaml
     kubectl delete sa SA_NAME -n NAMESPACE
     kubectl apply -f /tmp/sa-backup.yaml
     ```
  3. **重启所有使用该 SA 的 Pod**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     for deploy in $(kubectl get deploy -n NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}'); do
       SA=$(kubectl get deploy $deploy -n NAMESPACE -o jsonpath='{.spec.template.spec.serviceAccountName}')
       if [ "$SA" == "SA_NAME" ]; then
         kubectl rollout restart deployment/$deploy -n NAMESPACE
       fi
     done
     ```
  4. **验证新 Token 正常工作**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

     ```bash
     kubectl exec POD_NAME -n NAMESPACE -- cat /var/run/secrets/kubernetes.io/serviceaccount/token
     # 确认 Token 有效
     ```
- **安全检查**:
  - 所有使用该 SA 的 Pod 已重启
  - 应用功能正常
  - 审计日志中无旧 Token 的使用
- **回滚方案**:
  ```bash
  # SA Token 轮换通常不需要回滚
  # 如果应用无法启动，检查 RBAC 权限是否正确
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: 集群级安全加固（全面审计与重建）
- **适用根因**: RC-001, RC-003, RC-007, RC-012（大规模入侵场景）
- **审批要求**: 需要 CISO + 高级 SRE + 基础设施 Team Lead 审批
- **数据备份**: 完整的集群配置备份、etcd 快照、所有 Secret 的安全备份
- **操作步骤**:
  1. **全面审计当前安全态势**:
     ```bash
     # 运行 CIS Benchmark 检查
     kube-bench run --targets master,node
     
     # 导出所有安全相关配置
     kubectl get psp,pdb,networkpolicies,roles,rolebindings,clusterroles,clusterrolebindings -A -o yaml > security-config-backup.yaml
     
     # 扫描所有运行中镜像
     for image in $(kubectl get pods -A -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | sort -u); do
       trivy image --severity CRITICAL,HIGH $image >> image-scan-results.txt 2>/dev/null
     done
     ```
  2. **部署安全加固措施**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

     ```bash
     # 启用全局 PSA（enforce restricted）
     for ns in $(kubectl get ns -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | grep -v kube-system); do
       kubectl label namespace $ns pod-security.kubernetes.io/enforce=restricted --overwrite
     done
     
     # 部署默认 NetworkPolicy 到所有 namespace
     for ns in $(kubectl get ns -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}'); do
       kubectl apply -n $ns -f default-deny-networkpolicy.yaml
     done
     
     # 轮换所有 Secret
     # （需要按业务优先级分批执行）
     
     # 部署审计策略
     kubectl apply -f enhanced-audit-policy.yaml
     ```
  3. **重建受影响组件**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     # 重新部署所有可能受影响的工作负载
     kubectl rollout restart deployment --all -n AFFECTED_NAMESPACE
     
     # 考虑重建受影响节点
     # （参见 REM-007）
     ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 使用备份恢复配置
  kubectl apply -f security-config-backup.yaml
  
  # 如果全面加固导致业务中断，按优先级逐步回滚
  # 优先恢复核心业务，然后逐步重新应用安全措施
  ```

#### REM-011: 供应链安全体系建设
- **适用根因**: RC-003, RC-007（供应链攻击后的系统性修复）
- **审批要求**: 需要 CISO + 架构委员会审批
- **操作步骤**:
  1. **部署镜像签名基础设施**:
     ```bash
     # 生成 cosign 密钥对
     cosign generate-key-pair
     
     # 配置 CI/CD 在构建时签名
     # （集成到 Jenkins/GitLab CI/GitHub Actions）
     ```
  2. **部署 SBOM 生成和验证**:
     ```bash
     # 在构建流水线中添加 SBOM 生成
     syft IMAGE -o cyclonedx-json > sbom.json
     
     # 签名 SBOM
     cosign attach sbom --sbom sbom.json IMAGE
     cosign sign --key cosign.key IMAGE
     ```
  3. **部署准入控制策略**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

     ```bash
     # 使用 Kyverno/OPA 强制镜像签名验证
     # 使用 Sigstore Policy Controller
     helm install policy-controller sigstore/policy-controller \
       --namespace cosign-system --create-namespace
     ```
  4. **配置镜像仓库安全**:
     ```bash
     # 启用漏洞扫描
     # 配置镜像扫描策略
     # 配置镜像保留策略
     ```
- **回滚方案**:
  - 供应链安全体系是增量建设，不建议完全回滚
  - 如果特定策略过于严格，可以调整为 warn 模式

---

## 7. 验证确认

### 7.1 即时验证（修复后 5-10 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V1: 确认恶意活动已停止
kubectl exec AFFECTED_POD -n NAMESPACE -- ps aux 2>/dev/null | grep -iE "miner|shell" || echo "No suspicious processes"
# 预期: 无可疑进程

# V2: 确认网络隔离生效
kubectl exec AFFECTED_POD -n NAMESPACE -- curl -s --max-time 5 http://external-service.com || echo "Network isolated"
# 预期: 网络请求失败（被隔离）

# V3: 确认安全扫描通过
trivy image $(kubectl get pod AFFECTED_POD -n NAMESPACE -o jsonpath='{.spec.containers[0].image}') --severity CRITICAL,HIGH
# 预期: 无 CRITICAL/HIGH 漏洞（如果已替换镜像）

# V4: 确认 PSA 策略生效
kubectl get namespace NAMESPACE -o jsonpath='{.metadata.labels}'
# 预期: 包含 pod-security.kubernetes.io/enforce

# V5: 确认 NetworkPolicy 已部署
kubectl get networkpolicy -n NAMESPACE
# 预期: 显示隔离/限制策略

# V6: 确认 Secret 已轮换
kubectl get secret ROTATED_SECRET -n NAMESPACE -o jsonpath='{.metadata.resourceVersion}'
# 预期: 版本号已更新

# V7: 确认运行时监控正常
kubectl logs -n falco deploy/falco --tail=10 | grep -i "rule"
# 预期: Falco 正常输出规则匹配信息

# V8: 确认无新的安全告警
kubectl get events -A --field-selector reason=FailedValidation | grep -c security || echo "0"
# 预期: 0 或减少
```
### 7.2 短期监控（30 分钟 - 4 小时）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Falco 告警数量 | `falco_events_total` | 下降或稳定 | 新增相同类型告警 |
| 异常网络连接 | Cilium Hubble flows / Calico flow logs | 无新增异常外连 | 发现到恶意 IP 的连接 |
| Secret 访问审计 | `kubectl get --raw /metrics | grep audit` | 正常访问模式 | 异常来源的 Secret 访问 |
| Pod 安全违规 | `kubectl get events -A --field-selector reason=FailedCreate` | 无新增安全违规 | 新的 PSA 拒绝事件 |
| 镜像拉取策略 | `kube_pod_container_status_waiting_reason{reason="ImagePullBackOff"}` | 无异常 | 未签名镜像被拒绝（预期）或签名验证失败 |
| ServiceAccount 使用 | 审计日志中 SA 的使用模式 | 正常使用 | 异常 SA 使用或被弃用 SA 的活动 |
| DNS 查询模式 | CoreDNS 日志 | 正常查询 | 异常域名或 tunneling 特征 |
| CPU/内存异常 | `container_cpu_usage_seconds_total` | 正常范围 | 异常高使用（可能是挖矿） |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认安全事件已得到控制：

- [ ] 所有恶意进程已终止，无活跃攻击
- [ ] 受影响的 Pod/Node 已隔离或清除
- [ ] 安全扫描无新增 CRITICAL/HIGH 漏洞
- [ ] 泄露的凭据已轮换且新凭据正常工作
- [ ] NetworkPolicy 隔离策略已生效
- [ ] Pod Security Standards 已启用并 enforce
- [ ] 运行时安全监控已部署且正常工作
- [ ] 审计日志中无新的可疑活动
- [ ] 受影响的应用已恢复正常服务
- [ ] 事件报告已完成并通知相关方
- [ ] 合规通知义务已评估并履行（如适用）

### 7.4 回归检测（24-72 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 相同攻击特征复现 | Falco/Tetragon 告警监控 | 持续 | 立即重新进入本 Skill 诊断流程 |
| 新的 CVE 披露 | 漏洞情报订阅 | 每日 | 检查受影响镜像并计划升级 |
| 凭据滥用迹象 | 审计日志异常访问监控 | 每小时 | 确认新凭据安全性，追查异常访问 |
| 横向移动尝试 | NetworkPolicy 拒绝日志 | 每小时 | 分析被拒绝的流量来源 |
| 供应链异常 | 镜像签名验证失败告警 | 持续 | 审查镜像来源和构建流程 |
| 内部威胁迹象 | 特权操作审计 | 每日 | 审查异常的管理操作 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **活跃数据泄露** | 有证据表明敏感数据正在被外传 | 任何阶段发现外传行为 |
| **多节点同时告警** | 3 个以上节点同时出现相同安全告警，可能是供应链攻击 | Phase 1 快速评估时 |
| **控制平面入侵** | 攻击涉及 kube-system namespace 或控制平面组件 | 任何阶段发现控制平面异常 |
| **诊断超时** | 诊断工作流执行超过 **30 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 关键修复操作执行 **2 次**仍未成功 | REM-xxx 执行后验证失败 |
| **合规影响** | 事件可能触发数据泄露通知义务 | Phase 3 评估确认涉及 PII/敏感数据 |
| **内部威胁** | 攻击来源指向内部人员或合法凭据 | 审计日志分析发现内部账号异常 |

### 8.2 升级消息模板

```
【{severity}】安全事件应急响应 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 事件类型: {incident_type}（容器入侵/供应链攻击/凭据泄露/...）
- 发现时间: {detection_time}
- 影响范围:
  - 受影响 Namespace: {affected_namespaces}
  - 受影响 Pod 数量: {affected_pod_count}
  - 受影响 Node 数量: {affected_node_count}
  - 是否涉及敏感数据: {sensitive_data_involved}
- 当前状态:
  - 隔离状态: {isolation_status}（已隔离/部分隔离/未隔离）
  - 攻击状态: {attack_status}（活跃/已遏制/未知）
- 已完成诊断:
  - Phase 1 快速评估: {phase1_summary}
  - Phase 2 深度取证: {phase2_summary}
  - Phase 3 影响评估: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 攻击向量: {attack_vector}
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 合规影响:
  - 是否触发通知义务: {notification_required}
  - 建议通知时限: {notification_deadline}
- 需要:
  - {action_needed}
- 证据保存位置: {evidence_location}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-SECURITY-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供安全团队/SRE 接手：

1. **事件时间线**:
   - 首次告警时间
   - 各阶段诊断开始/结束时间
   - 关键发现时间点
   - 隔离措施执行时间

2. **完整诊断路径**:
   - 按时间顺序列出已执行的每个诊断步骤及输出
   - 每步的判断结论

3. **证据清单**:
   ```
   /tmp/evidence-pod-describe.txt
   /tmp/evidence-pod-yaml.txt
   /tmp/evidence-processes.txt
   /tmp/evidence-connections.txt
   /tmp/evidence-logs.txt
   /tmp/evidence-audit-logs.txt
   /tmp/evidence-falco-events.txt
   /tmp/evidence-sbom.json
   /tmp/evidence-trivy-scan.txt
   ```

4. **已排除的根因**:
   - 列出已通过诊断排除的根因及排除依据

5. **可能的根因假设**:
   - 基于已有证据提出的根因假设及置信度

6. **受影响资源清单**:
   - 受影响的 Pod/Deployment/Node 列表
   - 可能泄露的 Secret 列表
   - 需要轮换的凭据列表

7. **已执行的遏制措施**:
   - NetworkPolicy 隔离状态
   - 已 cordon 的节点
   - 已删除的资源

8. **待执行的修复操作**:
   - 按优先级排序的待办事项

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Pod Security Admission | GA（默认启用） | GA | GA | GA | GA |
| ValidatingAdmissionPolicy | beta | beta | GA | GA | GA |
| BoundServiceAccountTokenVolume | GA | GA | GA | GA | GA |
| ServiceAccountTokenNodeBinding | alpha | beta | beta | GA | GA |
| UserNamespacesPodSecurityStandards | alpha | alpha | beta | beta | beta |
| AppArmor Support | beta | beta | GA | GA | GA |
| Seccomp Default | GA | GA | GA | GA | GA |
| RuntimeClass | GA | GA | GA | GA | GA |
| PodDisruptionConditions | beta | GA | GA | GA | GA |
| CEL for Admission Control | beta | beta | GA | GA | GA |
| Audit Event Rate Limiting | 改进 | 改进 | 改进 | 增强 | 增强 |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl auth can-i --list` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl debug` with ephemeral containers | GA | GA | GA | GA | GA |
| `kubectl get --raw /api/v1/...` | 支持 | 支持 | 支持 | 支持 | 支持 |
| ValidatingAdmissionPolicy 诊断 | beta API | beta API | GA API | GA API | GA API |
| 审计日志格式 | v1 | v1 | v1 | v1 增强 | v1 增强 |
| ServiceAccount Token 审计 | 支持 | 支持 | 增强 | 增强 | 增强 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| NetworkPolicy | networking.k8s.io/v1 | v1 | v1 | v1 | v1 |
| PodSecurityPolicy | **已移除** | 已移除 | 已移除 | 已移除 | 已移除 |
| ValidatingAdmissionPolicy | admissionregistration.k8s.io/v1beta1 | v1beta1 | v1 | v1 | v1 |
| ServiceAccount | v1 | v1 | v1 | v1 | v1 |
| Secret | v1 | v1 | v1 | v1 | v1 |
| RBAC | rbac.authorization.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.4 版本相关的安全注意事项

- **[v1.28+]**: PodSecurityPolicy 已完全移除，必须迁移到 Pod Security Admission 或第三方策略引擎（OPA/Kyverno）：
  - 检查 `kubectl get psp` 是否仍有遗留配置
  - 确认 PSA 标签已应用到所有 namespace

- **[v1.29+]**: ServiceAccountTokenNodeBinding 进入 beta：
  - Token 绑定到特定节点，增强了 Token 的安全性
  - 影响横向移动检测：Token 在其他节点上使用应该被拒绝

- **[v1.30+]**: ValidatingAdmissionPolicy GA：
  - 可以使用 CEL 编写准入策略，替代 webhook
  - 性能更好，但需要学习 CEL 语法
  - 诊断时检查 `kubectl get validatingadmissionpolicy`

- **[v1.31+]**: ServiceAccountTokenNodeBinding GA：
  - 生产环境应启用此特性以增强安全性
  - 影响 Token 泄露后的横向移动能力

- **[v1.32+]**: UserNamespacesPodSecurityStandards 增强：
  - 更细粒度的用户命名空间支持
  - 影响容器逃逸风险评估

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将合法调试误判为攻击** | 容器内检测到 shell 进程 | 开发人员或 SRE 使用 `kubectl exec` 进行合法调试 | 关联审计日志，确认 exec 操作来源是否为已知人员；检查是否有相关变更工单 |
| **将高频 DNS 查询误判为隧道** | DNS 查询量异常高，有长子域名 | 服务网格/服务发现的正常行为（如 Istio sidecar、consul-connect）| 了解集群中的服务发现架构；检查域名模式是否符合已知服务 |
| **将监控工具误判为恶意软件** | 容器内发现 curl、wget 进程 | Prometheus exporter 或健康检查脚本的正常行为 | 检查进程的父进程和启动参数；对比应用预期行为 |
| **将 CI/CD 活动误判为攻击** | 大量 exec/attach 操作、Secret 访问 | 正常的 CI/CD 部署流程 | 关联部署时间线；检查操作来源是否为 CI/CD ServiceAccount |
| **将特权 DaemonSet 误判为入侵** | 发现 privileged 容器 | CNI、CSI、监控等系统组件的正常配置 | 检查 Pod 是否属于 kube-system 或已知系统 namespace |
| **将 CVE 误判为活跃利用** | 镜像扫描发现 CRITICAL CVE | 漏洞存在但不可利用（依赖关系不满足、组件未使用）| 结合运行时分析确认漏洞是否可达；检查是否有利用尝试的证据 |
| **将 NetworkPolicy 拒绝误判为攻击** | 大量网络流量被拒绝 | 新服务部署后 NetworkPolicy 未更新 | 关联部署时间线；检查拒绝的流量是否来自合法服务 |
| **将时钟偏差导致的 Token 失效误判为 Token 泄露** | Token 验证失败，触发安全告警 | 节点时钟不同步导致 JWT Token 验证失败 | 检查节点时间同步状态；关联 SKILL-NODE-001 中的 NTP 诊断 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Kubernetes 安全架构 | `安全/` | 理解 K8s 安全模型和防御深度 |
| 云原生安全最佳实践 | `安全/` | 安全加固方案设计 |
| 供应链安全 | `安全/` | 供应链攻击防御和 SBOM 管理 |
| 安全故障排查 | `故障诊断/32-security-troubleshooting.md` | 详细的安全问题诊断流程 |
| 容器逃逸技术 | 外部引用: container-escape-techniques | 理解攻击者视角，改进防御 |
| Falco 规则配置 | Falco 官方文档 | 自定义运行时检测规则 |
| cosign/Sigstore | Sigstore 官方文档 | 镜像签名验证深度配置 |
| Kubernetes 审计日志 | K8s 官方文档 | 审计策略配置和日志分析 |
| CIS Kubernetes Benchmark | CIS 官方 | 合规检查清单 |
| Pod Security Standards | K8s 官方文档 | PSA 配置和迁移指南 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 13 个根因、11 个修复操作 | 首批 Skill 库建设，基于安全事件响应最佳实践和 MITRE ATT&CK 框架 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **机密信息检测工具集成**: 与 GitHub Secret Scanning、GitLeaks、Trufflehog 的集成诊断流程
2. **云厂商安全服务集成**: AWS GuardDuty、Azure Defender、阿里云安全中心的告警关联
3. **取证工具链**: 完整的取证工具包（容器取证、内存取证、网络取证）使用指南
4. **威胁情报集成**: IOC (Indicators of Compromise) 查询和关联分析
5. **安全事件自动化响应**: SOAR (Security Orchestration, Automation and Response) 集成
6. **多集群安全事件**: 跨集群的安全事件关联和响应
7. **合规报告模板**: GDPR、SOC2、等保 2.0 事件报告模板
8. **红队/渗透测试场景**: 与合法渗透测试活动的区分方法
9. **零信任架构**: 零信任网络下的安全事件响应差异
10. **eBPF 深度监控**: 基于 Tetragon/bpftrace 的高级取证技术

## Related

- [[生态参考/topic-index/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
