# K8S 基础加固 — 面试一问一答（概念 + 常见问题）

> 六大域：集群 / 节点 / 负载 / 网络 / 存储 / 安全
> 每题结构：核心概念 → 标准答法 → 常见追问

---

## 1. 集群（Cluster）

### Q1：控制平面有哪些组件？各自职责？加固思路？

**概念**
- kube-apiserver：集群唯一入口，所有操作经它认证/鉴权/准入。
- etcd：分布式 KV，存全部集群状态（含 Secret）。
- kube-scheduler：Pod 调度选节点。
- kube-controller-manager：各种控制器（Deployment、Node、Job 等）。
- cloud-controller-manager：对接云厂商 LB/路由/节点生命周期。

**加固答法**
1. apiserver：`--anonymous-auth=false`、启用 RBAC、启用审计日志、准入控制器（PSA、NodeRestriction、LimitRanger）、超时与限流（`--max-requests-inflight`）。
2. etcd：TLS 双向认证（client cert）、静态加密 Secret（EncryptionConfiguration，优先 KMS v2）、独立节点部署、定期快照备份、2379/2380 仅控制平面互通。
3. 网络：6443 仅对可信网段/跳板机开放；控制平面节点不跑业务 Pod（taint）。

**追问**
- apiserver 挂了会怎样？—— 已运行 Pod 不受影响，但不能变更、不能自愈调度。
- etcd 磁盘要求？—— SSD，`etcdctl check perf`，延迟高直接拖垮整个集群。

### Q2：etcd 为什么推荐三节点？备份恢复怎么做？

**概念**：Raft 一致性，法定人数 n/2+1，3 节点容忍 1 故障；偶数节点浪费（4 节点也只能容忍 1 台）。

**答法**
- 备份：`etcdctl snapshot save backup.db --endpoints ... --cacert --cert --key`
- 恢复：停机 → `etcdctl snapshot restore` 指定新 data-dir → 重启。
- 生产：CronJob 定时快照 + 推送对象存储，定期演练恢复。

**追问**：直接拷 /var/lib/etcd 行不行？—— 不行，运行中拷贝无一致性保证，快照有 revision 校验。

### Q3：集群升级的版本策略和安全流程？

- skew 策略：kubelet 最多落后 apiserver 2 个小版本（如 apiserver 1.30，kubelet ≥1.28）。
- 流程：备份 etcd → 升控制平面（HA 逐台）→ cordon + drain 工作节点 → 升 kubelet/kube-proxy → uncordon → 验证再下一台。
- 关注 deprecated API（`kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis`）。

### Q4：HA 集群架构怎么讲？

- stacked etcd（etcd 与控制平面同机，简单）vs external etcd（独立集群，隔离爆炸半径）。
- apiserver 前置 LB（keepalived+haproxy / 云 LB），多副本无状态横向扩。

---

## 2. 节点（Node）

### Q1：节点 OS 层加固清单？

- 最小化 OS（或专用镜像 Bottlerocket/Container-Optimized OS）、关闭无关服务端口。
- SSH 禁密码登录、禁 root 直登、跳板机访问。
- CIS Benchmark 基线扫描：`kube-bench`。
- 内核：启用 SELinux/AppArmor、关 ICMP redirects、`net.ipv4.ip_forward=1`、bridge-nf-call-iptables。
- 时间同步 NTP、日志轮转、fail2ban。

### Q2：kubelet 安全配置要点？

- `--anonymous-auth=false`
- `--authorization-mode=Webhook`（由 apiserver 鉴权）
- read-only port **10255 必须禁用**（旧版本默认开，泄露 Pod/节点信息）
- 10250 走证书认证 + RBAC（nodes/proxy、pods/exec 权限最小化）
- `rotateCertificates=true`、证书准入（NodeRestriction + Node Authorizer）

**追问**：10250 未鉴权后果？—— 未授权读取 Pod 列表、exec 进容器、拉镜像凭据，等于集群失陷。真实出现过多次大规模挖矿事件。

### Q3：节点资源耗尽防护（eviction 机制）？

- kubelet 驱逐硬阈值示例：
  ```
  --eviction-hard=memory.available<500Mi,nodefs.available<10%,imagefs.available<15%
  ```
- `system-reserved` / `kube-reserved` 为系统和 K8S 守护预留资源。
- Pod QoS：Guaranteed > Burstable > BestEffort，驱逐按反序。
- 节点压力时上报 NodeCondition（MemoryPressure/DiskPressure/PIDPressure），scheduler 停止调度。

### Q4：节点出问题如何安全下线？

`kubectl drain <node> --ignore-daemonsets --delete-emptydir-data`，先 cordon 防新 Pod 调度；有 PDB（PodDisruptionBudget）时驱逐受限，保证可用副本数。

---

## 3. 负载（Workload）

### Q1：Pod 安全标准（PSS）三级是什么？

- **Privileged**：无限制（默认）。
- **Baseline**：禁 hostNetwork/hostPID/hostIPC、hostPath、特权容器、新增 capabilities。
- **Restricted**：Baseline + 必须 runAsNonRoot、seccomp、drop ALL capabilities、禁 privilege escalation。

落地方式：Pod Security Admission，按 namespace 打标签：
```yaml
pod-security.kubernetes.io/enforce: restricted
pod-security.kubernetes.io/audit: restricted
pod-security.kubernetes.io/warn: restricted
```

**追问**：PSP 去哪了？—— PodSecurityPolicy 1.25 移除，被 PSA 取代；更细粒度用 OPA Gatekeeper/Kyverno。

### Q2：一个「安全达标」的 Pod securityContext 长什么样？

```yaml
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  fsGroup: 2000
  seccompProfile:
    type: RuntimeDefault
containers:
- securityContext:
    allowPrivilegeEscalation: false
    readOnlyRootFilesystem: true
    capabilities:
      drop: ["ALL"]
```

### Q3：镜像安全全流程？

- 构建：最小基础镜像（distroless/scratch）、多阶段构建、不内置密钥。
- 入库：私有 Harbor + Trivy 扫描，高危漏洞阻断。
- 部署：禁 `:latest`、imagePullPolicy=Always（私有）、digest 锁定（`image@sha256:...`）。
- 准入：cosign 签名验签，未签名拒绝部署。

### Q4：ServiceAccount 安全？

- 禁用默认 SA 自动挂载：`automountServiceAccountToken: false`。
- 每应用独立 SA，RBAC 最小权限。
- TokenRequest API / 投影卷 token：有过期时间、绑定 Pod 生命周期（旧版永久 token 已淘汰）。

### Q5：ResourceQuota / LimitRange？

- ResourceQuota：namespace 级总量上限（CPU/内存/Pod 数/PV 数）。
- LimitRange：单容器默认值与上下限，防止无 request 的 Pod 抢占资源。
- 组合使用防止「一个 namespace 拖垮整个集群」。

### Q6：健康探针与优雅下线？

- liveness（挂了重启）vs readiness（摘流量）vs startup（慢启动保护）。
- 优雅下线：preStop hook + terminationGracePeriodSeconds，处理 SIGTERM、排空连接。

---

## 4. 网络（Network）

### Q1：K8S 默认网络安全吗？NetworkPolicy 怎么用？

**概念**：默认全互通（flat network，any-to-any），无隔离。

**答法**：白名单模型——
1. 先 default-deny：
```yaml
kind: NetworkPolicy
spec:
  podSelector: {}
  policyTypes: [Ingress, Egress]
```
2. 再按需放行（podSelector / namespaceSelector / ipBlock，端口级）。

**追问**
- 所有 CNI 都支持吗？—— 不是。Calico/Cilium/Weave 支持，Flannel 不支持。
- L7 控制怎么办？—— NetworkPolicy 只到 L3/L4；L7 用 Cilium 扩展策略或 Service Mesh。

### Q2：东西向流量加密方案？

- CNI 层：Cilium / Calico WireGuard 透明加密，性能损耗小。
- Mesh 层：Istio/Linkerd mTLS（`PeerAuthentication mode: STRICT`），带身份认证。

### Q3：入口（Ingress）加固？

- TLS 强制（https redirect），cert-manager 自动签发轮换证书。
- WAF（ModSecurity/云 WAF）、速率限制、IP 白名单 annotation。
- 内外入口分离：internal LB 注解区分内网 IngressClass。
- 禁默认后端泄露信息、关闭 server token。

### Q4：CoreDNS 安全？

- `dnsPolicy`：业务 Pod 用 `ClusterFirst`，不要 `Default`（继承宿主机 resolv.conf，可被污染）。
- CoreDNS 只监听集群网段；开启 query log 审计；防 DNS 放大攻击做出口限速。

### Q5：Service 类型安全考量？

- ClusterIP（默认，仅集群内）> NodePort > LoadBalancer > ExternalName。
- NodePort 会在所有节点开端口，暴露面大，生产慎用。
- ExternalName 可指向任意外部域名，有 SSRF 类风险，注意审计。

---

## 5. 存储（Storage）

### Q1：Secret 是加密的吗？

**不是。** 默认只是 base64 编码，任何能 `kubectl get secret -o yaml` 的人都能解码。

**加固四层**
1. etcd 静态加密：EncryptionConfiguration（aescbc → 优先 **KMS v2**，密钥不落盘）。
2. RBAC：限制 secret 的 get/list/watch，运维也按 namespace 收敛。
3. 外部密管：Vault、云 KMS + External Secrets Operator，Secret 不落 etcd 或仅存密文引用。
4. 使用方式：volume 挂载优于 env 注入（env 会出现在 crash dump、describe、子进程环境）。

### Q2：PV/PVC 访问控制？

- accessModes：ReadWriteOnce（单节点）/ ReadOnlyMany / ReadWriteMany（需存储支持）。
- `fsGroup` 控制挂载卷文件属组，配合 runAsUser。
- 禁用 hostPath（Restricted PSS 直接拦截）——hostPath 可挂宿主机 /，逃逸神器。

### Q3：容器逃逸和存储的关系？

- 挂载 Docker/containerd socket、hostPath 挂根目录、特权容器 = 逃逸通道。
- 答法：PSA Restricted + admission 拦截 + 运行时 Falco 检测挂载行为。

### Q4：备份方案 Velero？

- Velero = 集群资源对象（etcd 数据）+ PV 数据（restic/快照）一体备份。
- 定时 schedule、备份加密、跨云/异地存储、定期恢复演练。
- VolumeSnapshot（CSI）做存储层快照，恢复 RTO 更低。

---

## 6. 安全（Security 综合）

### Q1：RBAC 设计与常见误区？

**概念**：Role（namespace 内）/ ClusterRole（集群级）；RoleBinding / ClusterRoleBinding 绑定到 User/Group/ServiceAccount。

**要点**
- 最小权限：禁止 wildcard（`resources: ["*"], verbs: ["*"]`），cluster-admin 仅极少数人持有，配 MFA + 审批。
- 经典考点：**RoleBinding 引用 ClusterRole 时，权限仍限定在 RoleBinding 所在 namespace**——可复用定义、不放大范围。
- 审计：`kubectl auth can-i --list -n xxx --as system:serviceaccount:xxx:yyy`。

### Q2：审计日志怎么配置？看什么事件？

- apiserver `--audit-policy-file`，级别：None / Metadata / Request / RequestResponse。
- Secret 等敏感资源只记 Metadata（避免泄内容）。
- 高优告警事件：cluster-admin/ClusterRoleBinding 变更、pods/exec、secrets 批量 get、未知 IP 访问 apiserver。
- 日志外送 SIEM（Filebeat → ES / 云日志），本地不落敏感内容。

### Q3：准入控制（Admission Controller）体系？

- 内置链：Mutating → Validating。
- 关键内置：PodSecurity（PSA）、NodeRestriction、LimitRanger、ResourceQuota、ImagePolicyWebhook。
- 策略即代码：OPA Gatekeeper / Kyverno（CEL），示例策略：禁特权容器、镜像必须来自私有仓库、必须设 resources limits。

### Q4：运行时安全？

- Falco（eBPF）：容器内交互式 shell、写 /etc、/proc 异常、反连 C2、挂载敏感路径 → 告警。
- 配合 seccomp（限制系统调用）、AppArmor/SELinux（MAC）、只读根文件系统缩小攻击面。

### Q5：供应链安全（重点高频）？

- SLSA 框架认知；SBOM（syft 生成）随镜像交付。
- 镜像签名 cosign（keyless/KMS）+ admission 验签。
- 公网镜像统一走私有仓库代理拉取，入仓即扫描。
- CI 凭证最小化、制品不可变（digest）。

### Q6：多租户隔离怎么讲？

- 软隔离：namespace + RBAC + NetworkPolicy + ResourceQuota。
- 硬隔离：vCluster / 独立集群；节点级：taints + nodeSelector 分池。
- 数据面：每租户独立 SA、独立 Secret 域、审计按租户维度。

### Q7：集群失陷应急响应（IR）？

1. 隔离：cordon 节点、NetworkPolicy 断网、保留现场（勿直接删 Pod）。
2. 取证：审计日志 + Falco 事件 + 容器层 diff。
3. 凭证轮换：全部 SA token（1.24+ 逐个重建）、集群 CA、etcd 加密密钥、云密钥。
4. 重建：从可信备份重建受影响负载/整个集群，复盘加固。

---

## 高频追问速查表

| 追问 | 一句话答案 |
|---|---|
| Secret 加密了吗 | 默认仅 base64，要开 etcd 静态加密（KMS v2 优先） |
| 10255 端口 | kubelet 只读端口，必须禁用 |
| NetworkPolicy 默认 | 默认全通，先 default-deny 再白名单 |
| PSP 现状 | 1.25 移除，PSA（PSS 三级）替代 |
| etcd 节点数 | 奇数，3 容忍 1 故障，Raft 法定人数 |
| RoleBinding+ClusterRole | 权限限定在 binding 所在 namespace |
| QoS 驱逐顺序 | BestEffort → Burstable → Guaranteed |
| hostPath | 禁！逃逸通道，Restricted PSS 拦截 |
| 镜像 latest | 禁用，digest 锁定 + 签名验签 |
| NodePort 风险 | 全节点开端口，暴露面大 |
