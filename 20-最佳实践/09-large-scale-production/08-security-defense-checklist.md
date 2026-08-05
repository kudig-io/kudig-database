---
title: 护网/攻防演练检查项
description: 大规模 Kubernetes 集群护网（攻防演练/重保）专项安全检查清单，覆盖暴露面收敛、认证授权、运行时安全、镜像供应链、网络隔离、审计溯源与应急响应
summary: 护网前安全加固 checklist：攻击面收敛、RBAC 收敛、Pod 与运行时安全、镜像供应链、网络隔离、审计日志、监测处置与应急预案
category: references
tags:
- k8s
- checklist
- security
- hardening
- incident-response
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- 安全工程师
- SRE
- 运维负责人
estimated_read_time: 25min
---

# 护网/攻防演练检查项

> **使用场景**：护网（网络安全攻防演练）、重大活动保障（重保）前的专项安全加固与自查。✅ 必须 / 🔶 建议。
>
> 原则：**收敛暴露面 → 最小权限 → 可检测 → 可处置**。护网期间同步执行变更冻结。

## 1. 暴露面收敛（第一优先级）

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 1.1 | ✅ | APIServer（6443）不暴露公网；如需远程管理走 VPN/跳板机/零信任网关 | 公网扫描（nmap/测绘平台自查） |
| 1.2 | ✅ | etcd（2379/2380）仅控制面内网可达 | 安全组规则审计 |
| 1.3 | ✅ | kubelet（10250/10255）仅控制面来源可达，10255 只读端口已禁用 | `nmap -p 10250,10255 <node>` |
| 1.4 | ✅ | NodePort 范围安全组收敛，无意外对公网开放 | 安全组 + Service 清单交叉审计 |
| 1.5 | ✅ | 全量盘点对外入口：Ingress / LoadBalancer / NodePort / SLB，输出清单并逐项确认归属 | 暴露面清单 |
| 1.6 | ✅ | 废弃服务下线：无人维护的测试环境、历史 LB、僵尸 Ingress 全部清理 | 清单复核 |
| 1.7 | ✅ | 节点 SSH：禁用密码登录、限制来源 IP（堡垒机）、密钥轮换 | sshd_config 审计 |
| 1.8 | 🔶 | 控制台类组件（Dashboard/Kiali/Grafana/Prometheus UI）禁止公网暴露，强制认证 + 来源限制 | 逐项核验 |

## 2. 认证与授权（RBAC 收敛）

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 2.1 | ✅ | 匿名访问关闭：`--anonymous-auth=false`，`system:anonymous` 与 `system:unauthenticated` 无任何绑定 | 启动参数 + `kubectl get clusterrolebindings` 审计 |
| 2.2 | ✅ | cluster-admin 持有者清单：仅保留必要人员/系统，其余全部收敛 | ClusterRoleBinding 审计 |
| 2.3 | ✅ | 高危权限审计：`*` 权限、`escalate`/`impersonate`/`bind`、`pods/exec`、`secrets get/list` 的授予对象全部复核 | RBAC 审计工具（rakkess/kubectl-who-can） |
| 2.4 | ✅ | 默认 SA 不挂载 token：`automountServiceAccountToken: false` 为默认，例外清单复核 | 命名空间审计 |
| 2.5 | ✅ | 长期有效 token/kubeconfig 清理：旧版 SA secret token 删除，改用 TokenRequest（短期）+ 定期轮换 | Secret 审计 |
| 2.6 | ✅ | 离职/转岗人员权限回收流程执行；共享账号清零 | 账号清单 |
| 2.7 | ✅ | CI/CD 与系统组件 SA 权限最小化：按命名空间收敛，无 cluster-scope 宽权 | 逐 SA 复核 |
| 2.8 | 🔶 | 特权账号操作强制走审批/双人复核（护网期间） | 流程核验 |

## 3. Pod 与运行时安全

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 3.1 | ✅ | Pod Security Standards：生产命名空间 enforce ≥ baseline，核心系统 restricted | `kubectl get ns -L pod-security.kubernetes.io/enforce` |
| 3.2 | ✅ | 存量违规 Pod 清零：privileged、hostNetwork、hostPID、hostPath（/、/proc、/var/run/docker.sock 等敏感路径）挂载审计 | OPA/Kyverno 报告 + `kubectl get pods -A -o json` 扫描 |
| 3.3 | ✅ | 容器逃逸面收敛：无 `CAP_SYS_ADMIN`、`SYS_PTRACE` 等高危 capability；seccomp RuntimeDefault 或更严 | SecurityContext 扫描 |
| 3.4 | ✅ | runAsNonRoot 覆盖率 100%（例外走豁免清单） | 策略报告 |
| 3.5 | ✅ | docker.sock / containerd.sock 挂载清零 | Pod spec 扫描 |
| 3.6 | 🔶 | 运行时入侵检测（Falco/eBPF）部署，告警接入 SOC | 告警验证 |
| 3.7 | 🔶 | 节点级加固：CIS Benchmark 扫描通过率 ≥ 90% | kube-bench 报告 |

## 4. 镜像与供应链安全

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 4.1 | ✅ | 生产集群只允许受信仓库镜像（准入控制强制），禁止 `:latest` | 准入策略核验 |
| 4.2 | ✅ | 存量运行镜像全量扫描：高危漏洞（有 exploit/可达）清单 → 修复或豁免审批 | 扫描报告（Trivy/云扫描） |
| 4.3 | ✅ | CI 流水线镜像扫描门禁：高危阻断 | 流水线核验 |
| 4.4 | ✅ | 基础镜像收敛：统一受信基础镜像库，禁止公网随意拉取 | 仓库策略 |
| 4.5 | 🔶 | 镜像签名验证（cosign/notation）核心系统强制 | 准入策略 |
| 4.6 | 🔶 | SBOM 生成与留存（核心应用） | CI 产物核验 |

## 5. 网络隔离

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 5.1 | ✅ | 命名空间默认拒绝 NetworkPolicy（default-deny）+ 白名单放行，至少覆盖生产核心命名空间 | 策略核验 + 连通性抽测 |
| 5.2 | ✅ | 管理面（监控/日志/告警组件）与业务面网络策略分离 | 策略清单 |
| 5.3 | ✅ | 出向（Egress）管控：核心命名空间禁止任意外联，走统一出口代理 | 策略核验 |
| 5.4 | ✅ | 云安全组/NAC 复核：最小开放原则，删除冗余规则 | 云平台审计 |
| 5.5 | 🔶 | 东西向加密（mTLS/IPsec/WireGuard）核心链路启用 | 服务网格/CNI 核验 |
| 5.6 | 🔶 | DNS 劫持防护：CoreDNS 上游受信，禁止 Pod 使用外部 DNS 绕过审计 | NetworkPolicy + 策略核验 |

## 6. 审计与溯源

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 6.1 | ✅ | APIServer 审计日志开启并外送异地（防篡改），策略覆盖：Secret 读写、exec、特权操作、RBAC 变更 | 审计配置 + 外送链路验证 |
| 6.2 | ✅ | 审计日志留存 ≥ 180 天（护网要求通常 ≥ 6 个月） | 存储策略核验 |
| 6.3 | ✅ | 节点主机日志（auth/secure、syslog）集中采集 | 日志平台核验 |
| 6.4 | ✅ | 容器日志集中留存，关键应用 ≥ 90 天 | 日志平台核验 |
| 6.5 | ✅ | 溯源能力验证：给定一个可疑操作（如 secret 读取），能从审计日志定位到 人/SA + 源 IP + 时间 | 演练验证 |
| 6.6 | 🔶 | 安全监控规则上线：异常 RBAC 变更、夜间敏感操作、批量 secret 读取、异常外联等行为检测 | SOC 规则核验 |

## 7. 密钥与敏感数据

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 7.1 | ✅ | Secret 静态加密启用（EncryptionConfiguration + KMS） | etcd 直查验证密文 |
| 7.2 | ✅ | 敏感配置无硬编码：代码库/ConfigMap/镜像全量扫描（gitleaks/trufflehog） | 扫描报告 |
| 7.3 | ✅ | 高危凭据轮换：云 AK/SK、数据库密码、证书私钥在护网前完成轮换 | 轮换记录 |
| 7.4 | 🔶 | 外部密钥管理（Vault/云 KMS）核心应用接入，Secret 不落 Git | 方案核验 |

## 8. 护网期间运行机制

| # | 级别 | 检查项 |
|---|---|---|
| 8.1 | ✅ | **变更冻结**：护网期间非紧急变更全部冻结，紧急变更走特批流程 |
| 8.2 | ✅ | 值守安排：7×24 安全值守表，明确 研判（15min）→ 处置（30min）→ 上报 SLA |
| 8.3 | ✅ | 应急预案：入侵处置（隔离节点/Pod、吊销凭据、镜像取证）、数据泄露、DDoS、勒索场景 Runbook |
| 8.4 | ✅ | 一键处置能力：节点隔离脚本（cordon + 安全组隔离）、Pod 快速下线、凭据批量吊销，均已演练 |
| 8.5 | ✅ | 取证工具就位：节点快照、内存/磁盘取证流程、审计日志快速检索 |
| 8.6 | 🔶 | 蜜罐/诱捕：伪造高权 kubeconfig、蜜罐 Pod，提升攻击发现能力 |
| 8.7 | ✅ | 护网后复盘：攻击事件台账、失分点整改项、回归检查 |

## 9. 快速自查命令集

```bash
# 匿名访问与宽权绑定 🟢
kubectl get clusterrolebindings -o json | \
  jq '.items[] | select(.subjects[]?.name=="system:anonymous" or .subjects[]?.name=="system:unauthenticated")'

# cluster-admin 持有者 🟢
kubectl get clusterrolebindings -o json | \
  jq -r '.items[] | select(.roleRef.name=="cluster-admin") | .metadata.name, (.subjects[]? | "  - \(.kind)/\(.name)")'

# 特权 Pod 扫描 🟢
kubectl get pods -A -o json | \
  jq -r '.items[] | select(any(.spec.containers[]?.securityContext.privileged; .)) | "\(.metadata.namespace)/\(.metadata.name)"'

# hostPath 敏感挂载 🟢
kubectl get pods -A -o json | \
  jq -r '.items[] | select(.spec.volumes[]?.hostPath) | "\(.metadata.namespace)/\(.metadata.name)"' | sort -u

# 公网 LB 盘点 🟢
kubectl get svc -A -o json | \
  jq -r '.items[] | select(.spec.type=="LoadBalancer") | "\(.metadata.namespace)/\(.metadata.name) \(.status.loadBalancer.ingress)"'
```

## 10. 常见失分点（红队视角）

| 失分点 | 攻击路径 |
|---|---|
| 公网暴露的 APIServer/Dashboard | 弱口令/已知漏洞 → 集群接管 |
| 特权 Pod + hostPath docker.sock | 容器逃逸 → 节点控制 → 集群横向 |
| 宽权 SA token 泄露（日志/前端报错/Git 泄露） | token 直用 → API 接管 |
| 测试环境弱安全被突破后横向到生产 | 网络未隔离 → 核心数据失守 |
| 无审计日志/日志本地留存 | 攻击者清理痕迹，无法溯源定责 |

## Related

- [[06-initialization-checklist|初始化配置检查项（安全基线部分）]]
- [[07-pre-production-checklist|生产上线前检查项]]
- [[03-workload|工作负载最佳实践（安全基线）]]
- [[20-最佳实践/07-scenarios/security-hardening|安全加固场景]]
- [[20-最佳实践/07-scenarios/security-incident|安全事件响应场景]]
