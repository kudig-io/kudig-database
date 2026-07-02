---
title: 集群基础 生产就绪运维指南
description: 面向 SRE 的 Kubernetes 集群基础域生产就绪检查、日常运维与故障排查指南
summary: 面向 SRE 的 Kubernetes 集群基础域生产就绪检查、日常运维与故障排查指南
category: cluster
tags:
- production
- best-practices
- cluster
- operations
- control-plane
- etcd
- pki
- psa
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 集群基础 生产就绪运维指南是什么
- 如何按生产环境要求运维 Kubernetes 集群基础
trigger_keywords:
- 生产就绪
- 运维指南
- cluster
- control-plane
- etcd
- pki
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 集群基础 生产就绪运维指南

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产就绪运维手册

本指南聚焦 **domain-01-cluster-fundamentals**（集群基础域），为 SRE 和平台工程师提供在将集群投入生产前必须完成的检查清单、高风险缓解措施、日常运维操作及故障排查速查。集群基础域涵盖控制平面、etcd、API Server、kubelet、调度器、升级路径与性能调优等核心组件，是整个平台稳定性的根基。控制平面一旦出现故障，将影响全集群所有工作负载的调度、扩缩容与状态同步。与侧重架构设计的文档不同，本手册强调可落地的运维动作、可验证的检查项以及跨域协作边界。

本域当前主要缺口包括：控制平面事件运行手册、PKI 证书生命周期与自动轮换、节点生命周期与优雅维护、Pod Security Admission 强制执行落地、API Server 审计策略与 SIEM 集成。本指南将直接回应这些缺口，并给出可执行的命令与配置示例。

本手册建议每季度复审一次，或在发生控制平面 P1 事件、完成大版本升级、引入新可用区后及时更新。所有变更应通过标准变更工单执行，并在变更后 24 小时内完成监控基线复核。

---

## 1. 生产环境检查清单

在将集群标记为生产就绪前，必须逐项确认以下 12 项关键配置。任何一项未通过，都应在投产前完成修复并复测。

| 序号 | 检查项 | 验证命令 / 方法 | 合格标准 |
|---|---|---|---|
| 1 | 控制平面高可用 | `kubectl get nodes -l node-role.kubernetes.io/control-plane= -o wide` | 至少 3 个控制平面节点，分布在不同可用区，每个节点资源充足 |
| 2 | etcd 集群健康 | `ETCDCTL_API=3 etcdctl endpoint health --cluster` | 所有成员返回 `is healthy: true`，近 24 小时无 `leader changed` 事件 |
| 3 | API Server 证书有效期 | `openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates` | 所有内部证书剩余有效期 > 90 天，并设置自动告警 |
| 4 | 节点 OS 与内核版本 | `kubectl get nodes -o jsonpath='{.items[*].status.nodeInfo.osImage}'` | 统一基线镜像，内核 ≥ 5.4，关键补丁已应用 |
| 5 | kubelet 配置一致性 | `kubectl get --raw=/api/v1/nodes/<node>/proxy/configz \| jq '.kubeletconfig'` | systemReserved、kubeReserved、evictionHard 与平台基线一致 |
| 6 | Pod Security Admission | `kubectl get ns -o jsonpath='{.items[*].metadata.labels}'` | 所有命名空间已标注 enforce/audit/warn 级别，默认非 privileged |
| 7 | API Server 审计策略 | `ps aux \| grep audit-policy-file` | 已挂载审计策略文件，审计日志写入持久化路径并转发 SIEM |
| 8 | 核心组件监控覆盖 | `kubectl get servicemonitor -n monitoring` | API Server、etcd、kubelet、scheduler、controller-manager 均有监控指标 |
| 9 | 备份任务运行正常 | 检查 CronJob 与对象存储 | etcd 快照、证书、配置备份均按时完成，且恢复演练验证通过 |
| 10 | 集群版本与 skew | `kubectl version --short` | 控制平面与 kubelet 版本差 ≤ 2 个小版本，无 deprecated API 调用 |
| 11 | 资源配额与限制范围 | `kubectl get resourcequota,limitrange --all-namespaces` | 所有生产命名空间已配置 ResourceQuota 与 LimitRange |
| 12 | 灾难恢复演练记录 | 查阅变更平台 / Wiki | 近 90 天内完成 etcd 恢复或集群重建演练，并保留操作记录 |

### 1.1 生产就绪判定标准

建议采用三级判定：

- **绿色（Ready）**: 12 项全部通过，近 30 天无 P1 级以上控制平面故障，DR 演练通过。
- **黄色（Conditionally Ready）**: 1–2 项非关键项未通过（如文档、标签规范），已制定修复计划并在 7 天内完成。
- **红色（Not Ready）**: 任何关键项未通过（如 etcd 不健康、证书 < 30 天、无备份），禁止接入生产流量。

---

## 2. 关键风险与缓解措施

### 2.1 控制平面单点故障

**风险**: 单控制平面节点或 etcd 成员部署在同一可用区，一旦该可用区发生故障，API Server 无法响应，整个集群将陷入不可调度、不可变更状态，业务扩容与故障恢复均受阻。  
**缓解**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证控制平面节点分布
kubectl get nodes -l node-role.kubernetes.io/control-plane= \
  -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.topology\.kubernetes\.io/zone}{"\n"}{end}'

# 验证 etcd 成员分布与健康
ETCDCTL_API=3 etcdctl endpoint health --cluster \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

ETCDCTL_API=3 etcdctl endpoint status --cluster -w table \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```
若发现控制平面节点集中在同一可用区，应新增节点并重新分布 etcd 成员。禁止在控制平面节点上运行业务工作负载，确保节点故障时只影响控制平面副本而非业务。

### 2.2 证书过期导致集群瘫痪

**风险**: Kubernetes 内部 CA、apiserver、front-proxy、etcd 证书具有固定有效期，若未建立巡检与自动续期机制，证书过期将导致所有组件无法建立 TLS 连接，集群完全不可用。  
**缓解**:

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
# 检查所有证书有效期（kubeadm 集群）
kubeadm certs check-expiration

# 手动扫描证书并输出剩余天数
for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
  expiry=$(openssl x509 -in "$cert" -noout -enddate \| cut -d= -f2)
  days=$(( ($(date -d "$expiry" +%s) - $(date +%s)) / 86400 ))
  echo "$cert: $days days left"
done

# 续期所有证书并重启静态 Pod
kubeadm certs renew all
systemctl restart kubelet
```
建议在监控系统中配置 30 天、60 天、90 天三级告警，并将证书有效期纳入每日巡检。对于大规模集群，应评估 cert-manager 或自定义 Operator 实现 leaf 证书自动轮换。

### 2.3 Pod 安全策略缺失导致容器逃逸

**风险**: 特权容器、hostPath、hostNetwork、Capabilities 未受控时，攻击者一旦攻破容器，可轻易获取节点 root 权限，造成横向移动与数据泄露。  
**缓解**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为所有生产命名空间设置 baseline 或 restricted
kubectl label ns production \
  pod-security.kubernetes.io/enforce=baseline \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 批量检查特权 Pod
kubectl get pods --all-namespaces -o json \| \
  jq '.items[] \| select(.spec.containers[].securityContext.privileged == true) \| {ns: .metadata.namespace, pod: .metadata.name}'

# 推荐 Pod securityContext 模板
securityContext:
  runAsNonRoot: true
  seccompProfile:
    type: RuntimeDefault
  allowPrivilegeEscalation: false
  capabilities:
    drop:
    - ALL
```
对于需要特权能力的应用，应通过 OPA/Kyverno 进行白名单审批，并在独立命名空间使用 `privileged` 级别。

### 2.4 节点无通知下线导致工作负载中断

**风险**: 直接重启或下线节点，未触发优雅驱逐，会导致 Pod 被强制终止，业务出现 502/连接中断，StatefulSet 应用还可能面临数据不一致或脑裂。  
**缓解**:

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
# 标准维护流程
kubectl cordon <node>
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --grace-period=30 --timeout=300s
# 执行维护操作
kubectl uncordon <node>

# 强制排空（仅用于节点已不可恢复场景）
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force --grace-period=30
```
维护前需确认目标节点上的 Pod 已配置 PDB（PodDisruptionBudget），避免一次性驱逐过多副本。对于本地存储 Pod，需评估 `--delete-emptydir-data` 影响。

### 2.5 etcd 数据丢失或恢复流程未验证

**风险**: 备份任务虽然成功，但若未定期做恢复演练，真正灾难发生时可能发现备份损坏、版本不兼容或恢复命令错误，导致 RPO 无法兑现，业务数据回退到不可接受的时间点。  
**缓解**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 每周恢复演练：将最新快照恢复到临时目录
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd/latest.db \
  --data-dir=/tmp/etcd-restore-test \
  --name=test \
  --initial-cluster=test=http://localhost:2380 \
  --initial-advertise-peer-urls=http://localhost:2380

# 验证数据目录生成与完整性
ls -l /tmp/etcd-restore-test/member/
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd/latest.db

# 生产级 etcd 备份脚本核心命令
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd/etcd-$(date +%Y%m%d-%H%M%S).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```
备份文件应加密后上传至异地对象存储，并定期验证快照 `snapshot status` 输出无错误。

---

## 3. 日常运维操作

### 3.1 每日晨检命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 集群版本与节点状态
kubectl version --short
kubectl get nodes -o wide

# 2. 控制平面 Pod 状态
kubectl get pods -n kube-system -l tier=control-plane

# 3. 关键事件巡检
kubectl get events --all-namespaces --sort-by='.lastTimestamp' \| tail -n 20

# 4. 证书剩余有效期扫描
kubeadm certs check-expiration

# 5. etcd 健康检查
ETCDCTL_API=3 etcdctl endpoint health --cluster

# 6. 节点资源使用率 Top10
kubectl top nodes
kubectl top pods --all-namespaces --sort-by=cpu \| head -n 10
```
### 3.2 节点维护流程

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
# 标记节点不可调度并排空
kubectl cordon <node>
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --grace-period=30 --timeout=300s

# 执行内核升级 / 安全补丁 / 硬件维护后
kubectl uncordon <node>

# 验证 Pod 重新调度与健康
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node>
```
### 3.3 控制平面扩缩容（kubeadm）

```bash
# 生成加入证书
kubeadm init phase upload-certs --upload-certs

# 在新控制平面节点执行
kubeadm join <control-plane-endpoint>:6443 --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane --certificate-key <certificate-key>
```

### 3.4 审计策略更新

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
# 备份当前策略并应用新版
sudo cp /etc/kubernetes/policies/audit-policy.yaml /etc/kubernetes/policies/audit-policy.yaml.bak.$(date +%s)
sudo cp audit-policy-v2.yaml /etc/kubernetes/policies/audit-policy.yaml
sudo systemctl restart kubelet

# 验证审计日志正常写入
tail -f /var/log/kubernetes/audit.log
```
### 3.5 容量与性能基线巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 统计集群对象规模
kubectl get nodes --no-headers \| wc -l
kubectl get pods --all-namespaces --no-headers \| wc -l
kubectl get secrets --all-namespaces --no-headers \| wc -l

# API Server 请求延迟（需 Prometheus）
# histogram_quantile(0.99, apiserver_request_duration_seconds_bucket{verb!="WATCH"})

# etcd 数据库大小
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table
```
建议将对象数量、API Server P99 延迟、etcd DB 大小纳入每周容量Review，超过基线 80% 时触发扩容评估。

---

## 4. 故障排查速查

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| `kubectl` 所有命令超时 | API Server 不可用 / 证书过期 | `curl -k https://<apiserver>:6443/healthz` | 检查 `kube-apiserver` Pod 日志；若证书过期执行 `kubeadm certs renew all` |
| 节点状态 `NotReady` | kubelet 停止 / CNI 异常 / 磁盘压力 | `journalctl -u kubelet -f` / `kubectl describe node <node>` | 重启 kubelet；清理镜像/日志；检查 CNI Pod |
| Pod 持续 `Pending` | 资源不足 / 污点 / 调度约束 | `kubectl describe pod <pod> -n <ns>` | 扩容节点、调整 Pod 亲和性/容忍、检查 ResourceQuota |
| etcd 成员 `unhealthy` | 网络分区 / 磁盘 IO 瓶颈 / 任期风暴 | `etcdctl endpoint status --cluster -w table` | 修复网络；使用 SSD；必要时替换成员 |
| 集群升级失败 | 版本 skew / 镜像拉取失败 / etcd 不兼容 | `kubeadm upgrade plan` | 按官方路径逐小版本升级；先升级控制平面再升级节点 |
| PSA 拦截 Pod 创建 | SecurityContext 不满足 enforced 级别 | `kubectl get events -n <ns> \| grep violated` | 调整 Pod securityContext 或降低命名空间 enforce 级别 |
| 节点磁盘压力导致驱逐 | 镜像/日志/emptyDir 占满 | `df -h` / `kubectl describe node <node>` | 调整 kubelet 驱逐阈值；启用日志轮转；清理未使用镜像 |
| kubelet 频繁重启 | 配置错误 / cgroup 驱动不匹配 | `journalctl -u kubelet --since "1 hour ago"` | 修正 kubelet 配置；统一 containerd 与 kubelet cgroup 驱动 |
| scheduler 调度延迟高 | 调度队列堆积 / 节点资源碎片化 | `kubectl logs -n kube-system kube-scheduler-<node>` | 启用多调度器或调整调度策略；扩容节点 |
| controller-manager 不同步 | leader 选举异常 / 权限不足 | `kubectl logs -n kube-system kube-controller-manager-<node>` | 检查 RBAC 与 ServiceAccount；重启 leader 实例 |

---

## 5. 与其他域的协作边界

集群基础域是平台稳定性的底座，但以下问题需要与相邻域协同处理：

- **网络与流量管理**：CNI 插件、kube-proxy、CoreDNS 的升级与故障排查由 [[domain-03-networking-traffic/README.md|domain-03-networking-traffic]] 负责；集群基础域只关注节点网络基线参数与 kubelet 网络插件配置。
- **安全合规**：Pod Security Admission、RBAC、证书生命周期、审计策略的**策略制定**由 [[domain-05-security-compliance/README.md|domain-05-security-compliance]] 负责；集群基础域负责**在控制平面启用与落地**这些策略。
- **可观测性**：监控告警体系、日志聚合、SLO/SLI 由 [[domain-06-observability/README.md|domain-06-observability]] 负责；集群基础域提供 API Server、etcd、kubelet 等核心组件的指标暴露与审计日志输出。
- **平台工程**：多集群治理、GitOps、IaC、租户平台由 [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] 负责；集群基础域提供稳定、可扩展、安全的单集群底座。
- **可靠性工程**：灾难恢复 RTO/RPO 设计、混沌工程、PDB 由 [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] 负责；集群基础域提供 etcd 备份、控制平面 HA、节点维护等基础能力。
- **生产运维**：值班、事件响应、变更管理由 [[domain-11-production-operations/README.md|domain-11-production-operations]] 负责；集群基础域提供上述故障排查速查与日常运维操作输入。

---

## 6. 推荐阅读

### 同域深度参考

- [[domain-01-cluster-fundamentals/01-production-architecture-design-principles.md|生产架构设计原则]]
- [[domain-01-cluster-fundamentals/01-architecture-overview/17-production-operations-best-practices.md|生产环境运维最佳实践]]
- [[domain-01-cluster-fundamentals/03-control-plane/03-plane-high-availability.md|控制平面高可用]]
- [[domain-01-cluster-fundamentals/03-control-plane/10-plane-backup-disaster-recovery.md|控制平面备份与灾备方案]]
- [[domain-01-cluster-fundamentals/03-control-plane/11-etcd-deep-dive.md|etcd 深度解析]]
- [[domain-01-cluster-fundamentals/03-control-plane/12-apiserver-deep-dive.md|API Server 深度解析]]
- [[domain-01-cluster-fundamentals/06-upgrade-paths/99-kubernetes-v1.33-upgrade-guide.md|Kubernetes v1.33 升级指南]]

### 跨域协作参考

- [[domain-03-networking-traffic/README.md|domain-03-networking-traffic]] — 网络流量管理
- [[domain-05-security-compliance/README.md|domain-05-security-compliance]] — 安全合规
- [[domain-06-observability/README.md|domain-06-observability]] — 可观测性
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台工程
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] — 可靠性工程
- [[domain-11-production-operations/README.md|domain-11-production-operations]] — 生产运维

---

*本指南基于 KUDIG 集群基础域现状与生产就绪 gap 分析编写，重点补齐控制平面运行手册、PKI 生命周期、节点维护、PSA 落地与审计日志等运维缺口。*


<!-- risk-assessed -->
