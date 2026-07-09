---
title: 故障排查与诊断 生产就绪运维指南
description: 面向生产环境的故障排查与诊断域检查清单、风险缓解、日常运维及跨域协作指南
summary: 面向生产环境的故障排查与诊断域检查清单、风险缓解、日常运维及跨域协作指南
category: troubleshooting
tags:
- production
- best-practices
- troubleshooting
- operations
- diagnostics
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
- 故障排查与诊断 生产就绪运维指南是什么
- 如何按生产环境要求运维 故障排查与诊断
trigger_keywords:
- 生产就绪
- 运维指南
- 故障排查
- 诊断
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
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


# 故障排查与诊断 生产就绪运维指南

> **适用版本**: Kubernetes v1.28 - v1.33 | **最后更新**: 2026-07 | **难度**: 高级

本指南面向 SRE、平台工程师和运维负责人，定义「故障排查与诊断」域在生产环境中应具备的能力基线。目标不是重复讲解具体排障步骤，而是建立一套可审计、可演练、可交接的生产就绪运维框架，确保团队能在故障窗口内快速收敛根因、控制爆炸半径并恢复服务。

### 生产就绪目标

- **可发现**：任何异常都能在 5 分钟内被监控、事件或用户报告捕获，并有明确的入口开始排查。
- **可定位**：值班工程师能在 15 分钟内完成初步分诊，确定问题属于计算、网络、存储、安全还是外部依赖。
- **可恢复**：关键故障具备经过评审的 runbook 或技能卡，执行步骤包含确认命令、影响评估与回滚方案。
- **可复盘**：所有 P1 及以上事件在 48 小时内完成时间线整理、根因分析与行动项跟踪。

---

## 1. 生产环境检查清单

在将本域能力声明为生产就绪前，建议逐项核对以下检查点：

1. **排障入口统一化**：所有工程师使用同一套 kubectl context 命名规范与默认 namespace，禁止直接 ssh 到节点执行未记录命令；优先通过 `kubectl debug` 与临时容器采集证据。
2. **标准化信息收集脚本就绪**：每个集群至少具备 `get-events.sh`、`collect-node-logs.sh`、`snapshot-pod-state.sh`，输出格式固定为带时间戳的目录结构。
3. **核心组件可观测覆盖**：API Server、etcd、scheduler、controller-manager、kubelet、CoreDNS 的关键指标已接入监控，且具备按 symptom 跳转的 Grafana/告警链接。
4. **事件保留策略合规**：`event-ttl` 与外部事件存储（如 Loki/SLS）保存时间 ≥ 30 天，满足事后复盘与审计需求。
5. **节点诊断权限最小化**：Node 问题排查通过 RBAC 绑定到 `cluster-reader` 与 `node-reader` 角色，禁止在生产环境使用 `cluster-admin` 日常账号。
6. **排障工具镜像受控**：`nicolaka/netshoot`、`busybox`、`alpine` 等 debug 镜像来自内部 Harbor 并经过安全扫描，不允许直接从 Docker Hub 拉取。
7. **变更关联能力可用**：具备最近 24 小时内的 deployment、node、NetworkPolicy、ConfigMap/Secret、证书变更时间线，能按 symptom 快速回溯。
8. **故障树与技能卡映射完成**：常见 P0/P1 故障（Pod CrashLoopBackOff、Node NotReady、Service 不可达、证书过期）已映射到 FTA 与技能卡，新值班工程师 5 分钟内可定位入口。
9. **演练与评审机制落地**：每季度至少执行一次基于真实混沌场景（如节点故障、CNI 中断、证书过期）的桌面演练或真实演练，并输出 postmortem。
10. **应急隔离预案受审**：存在明确的 namespace/集群级紧急隔离流程（网络断连、密钥吊销、workload 暂停），并通过安全与合规团队评审。
11. **多云/混合环境差异化 runbook**：若集群跨 ACK/EKS/GKE/AKS，必须分别准备对应 cloud provider 的排查入口与限制说明。
12. **值班交接与升级路径清晰**：每个严重度等级（S0-S3）对应明确的响应时限、值班升级群、战争室入口与回滚授权人。

---

## 2. 关键风险与缓解措施

### 2.1 信息收集不完整导致根因误判

**风险**：生产故障往往伴随时间压力，工程师容易跳过 `kubectl describe` 与 Events，直接凭经验执行高危命令，导致误删 Pod 或错误驱逐节点。

**缓解措施**：
- 强制使用标准化收集脚本：
  ```bash
  ./scripts/collect-incident-snapshot.sh \
    --namespace <ns> \
    --resource <pod|node|deployment> \
    --name <name> \
    --output /incident/$(date +%Y%m%d-%H%M%S)-<ticket>
  ```
- 脚本至少包含：`get -o yaml`、`describe`、`logs --previous`、`events --field-selector`、`top`、`get node -o wide`。
- 所有收集操作只读，写入受控目录，便于事后审计。

### 2.2 生产环境排障工具引入 supply-chain 风险

**风险**：临时容器使用的 debug 镜像来源不可控，可能包含恶意二进制或 CVE，且直接以 privileged 模式运行会扩大攻击面。

**缓解措施**：
- 在内部镜像仓库维护 hardened debug 镜像，例如 `harbor.example.com/sre/netshoot:v0.0.5-scanned`。
- 通过 Admission Policy 限制非白名单镜像进入生产 namespace：
  ```yaml
  # 仅允许指定 debug 镜像与 registry
  imageAllowlist:
    - "harbor.example.com/sre/*"
    - "registry.k8s.io/pause*"
  ```
- 临时容器默认禁用 privileged，确需 escalated 权限时走工单审批并记录审计日志。

### 2.3 变更漂移与版本差异引发难以复现的故障

**风险**：集群组件、CRD、Operator、节点镜像版本不一致，导致同一 symptom 在不同环境表现不同，排查路径失效。

**缓解措施**：
- 每日巡检输出「版本漂移矩阵」：
  ```bash
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\t"}{.status.nodeInfo.osImage}{"\n"}{end}'
  kubectl get crd -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.versions[*].name}{"\n"}{end}'
  ```
- 对关键组件（CNI、CSI、Ingress controller、cert-manager）启用 GitOps 版本锁定，drift 超过 1 个 patch 版本即触发告警。
- 建立按 Kubernetes 小版本（1.28/1.29/1.30...）分类的已知问题矩阵，定期从 release notes 与 CVE 公告更新。

### 2.4 排障过程中触发二次事故

**风险**：重启 Pod、驱逐节点、删除 finalizer、缩容 deployment 等操作可能在高峰期引发级联故障。

**缓解措施**：
- 任何写操作执行前必须确认 PDB、HPA、VPA 状态：
  ```bash
  kubectl get pdb -n <ns>
  kubectl get hpa,vpa -n <ns>
  ```
- 采用「影响面评估模板」：列出受影响 Pod 数量、流量比例、数据持久化状态、回滚命令，由二线 SRE 确认后执行。
- 对关键 namespace 启用 ResourceQuota 与 LimitRange，避免排障过程中的临时资源申请耗尽集群。

### 2.5 告警噪音淹没真实故障

**风险**：大量低优先级告警或缺少上下文的告警在夜间爆发，导致值班人员疲劳并延迟对真实 P0 事件的响应。

**缓解措施**：
- 建立告警分级与抑制规则：基础设施层告警优先于应用层重复告警，同一根因的告警在 Alertmanager 中聚合为单条通知。
- 每条 P1/P0 告警必须附带 "Runbook 链接"、"最近相关变更"、"受影响的 namespace/service" 三个上下文字段。
- 每月进行一次告警质量评审，对连续 3 次未触发人工干预的告警下调严重度或关闭。

---

## 3. 日常运维操作

### 3.1 晨间健康巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 集群总体状态
kubectl get nodes -o wide
kubectl get componentstatuses  # 仅作参考，生产环境建议看 metric/endpoint

# 2. 异常 Pod 概览（排除 Completed）
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

# 3. 近 1 小时异常事件聚合
kubectl get events -A --sort-by='.lastTimestamp' | tail -n 50 | grep -E "Warning|Error|Failed|BackOff|NotReady"

# 4. 证书剩余有效期（kubeadm 集群）
kubeadm certs check-expiration

# 5. 节点资源水位
kubectl top nodes
```
### 3.2 命名空间级故障初筛

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
NS=<namespace>

# 负载状态
kubectl get deploy,sts,ds -n $NS -o wide

# Pod 级别
kubectl get pods -n $NS -o wide
kubectl describe pods -n $NS | grep -E "Events:|Warning|Error|Failed" | head -n 30

# 网络与存储
kubectl get svc,ing,netpol -n $NS
kubectl get pvc -n $NS

# 变更追踪
kubectl rollout history deploy/<name> -n $NS
```
### 3.3 使用临时容器深入诊断

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入目标 Pod 的网络命名空间进行抓包或 DNS 测试
kubectl debug <pod> -n <ns> -it --image=harbor.example.com/sre/netshoot:v0.0.5-scanned --target=<container>

# 节点级诊断（生成 node shell 并限制只读挂载）
kubectl debug node/<node> -it --image=harbor.example.com/sre/netshoot:v0.0.5-scanned --profile=restricted

# 复制出关键日志
kubectl cp <ns>/<pod>:/var/log/app /tmp/incident-logs/<pod>/
```
### 3.4 事件与日志归档

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出某 namespace 全量事件为 JSON，便于 SIEM 或 postmortem
kubectl get events -n <ns> -o json > /tmp/events-<ns>-$(date +%Y%m%d-%H%M%S).json

# 导出节点 journal（需节点级权限）
kubectl debug node/<node> --image=alpine -- cat /host/var/log/pods/... > /tmp/node-logs-<node>.log
```
### 3.5 容量与饱和度巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Pod 资源申请与实际使用对比
kubectl top pods -A --containers | sort -k4 -hr | head -n 20

# 检查是否存在大量未调度的 Pod
kubectl get pods -A --field-selector=status.phase=Pending | wc -l

# 检查 API Server 请求延迟与并发（需 metrics-server 或 Prometheus）
kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket

# 检查 etcd 磁盘延迟与 DB 大小（在 control-plane 节点执行）
etcdctl endpoint status -w table --cluster
```
### 3.6 变更前影响评估

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看即将部署版本的 diff
kubectl diff -f <new-manifest>.yaml

# 检查目标 namespace 内是否存在正在进行的滚动更新或自动伸缩事件
kubectl get rollout -n <ns>
kubectl get hpa -n <ns>

# 确认 PodDisruptionBudget 是否允许安全中断
kubectl get pdb -n <ns> -o wide
```
### 3.7 排障后证据固化

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 归档事件、日志与资源快照
kubectl get events -A -o json > /postmortem/<ticket>/events.json
kubectl get all -n <ns> -o yaml > /postmortem/<ticket>/resources.yaml

# 2. 记录已执行的修复命令与时间线
# 建议写入共享文档或 incident channel，避免口头交接丢失关键信息

# 3. 确认恢复指标回到基线
kubectl top pods -n <ns>
kubectl get pods -n <ns> | grep -v Running
```
### 3.8 云厂商 CLI 辅助确认

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ACK/阿里云
aliyun cs GET /k8s/<cluster-id>/nodes

# EKS/AWS
aws eks describe-cluster --name <cluster>
aws ec2 describe-instances --filters Name=tag:eks:cluster-name,Values=<cluster>

# GKE
kubectl get nodes -o json | jq -r '.items[].metadata.labels["cloud.google.com/gke-nodepool"]'
```
---

## 4. 故障排查速查

| 现象 | 可能根因 | 确认命令 | 缓解/修复 |
|------|---------|---------|----------|
| Pod 处于 `Pending` | 资源不足 / 污点容忍不匹配 / PVC 未绑定 | `kubectl describe pod` + `kubectl get nodes -l` + `kubectl get pvc` | 扩容节点、调整 tolerations、检查 StorageClass / PV |
| `CrashLoopBackOff` | 启动失败、探针配置错误、依赖不可用 | `kubectl logs --previous` + `kubectl describe pod` | 修复代码/配置、放宽 initialDelaySeconds、检查依赖 endpoint |
| `ImagePullBackOff` | 镜像不存在、拉取凭证缺失、仓库限流 | `kubectl describe pod \| grep -i image` + `kubectl get secret` | 更新 image tag、创建 imagePullSecret、切换镜像仓库 |
| Node `NotReady` | kubelet 异常、CNI 故障、磁盘/内存压力 | `kubectl describe node` + `kubectl get node -o yaml` + 节点 journal | 重启 kubelet、修复 CNI、清理磁盘、按 drain 流程替换节点 |
| Service 访问超时 | Endpoint 缺失、kube-proxy 异常、NetworkPolicy 阻断 | `kubectl get endpoints` + `iptables-save \| grep <svc>` + `kubectl get netpol` | 检查 selector、重启 kube-proxy/CNI、放宽策略 |
| DNS 解析失败 | CoreDNS 缩容、转发器异常、ndots 问题 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` + `dig @<coredns-ip>` | 扩容 CoreDNS、修复上游 DNS、调整 ndots/search |
| HPA 未生效 | metrics-server 异常、未配置 resources.requests | `kubectl describe hpa` + `kubectl get apiservices` | 修复 metrics-server、补充 requests、重启 HPA |
| 证书过期告警 | kubeadm CA/leaf cert、cert-manager、ingress TLS 到期 | `kubeadm certs check-expiration` + `kubectl get certificate -A` | 执行证书续期、验证 cert-manager issuer、轮换 ingress secret |
| Pod OOMKilled | 内存 limit 过小或内存泄漏 | `kubectl describe pod \| grep -i oom` + `kubectl top pod --containers` | 调高 limit、修复泄漏、启用 VPA 建议 |
| 节点磁盘压力 | 镜像/日志/emptyDir 占满、GC 未触发 | `kubectl describe node \| grep -i pressure` + `df -h /var/lib/kubelet` | 清理镜像与日志、调整 evictionHard 阈值、扩容磁盘 |

---

## 5. 与其他域的协作边界

故障排查与诊断不是孤立域，生产就绪要求明确与其他域的责任接口：

- **与 [[可观测性/README.md|可观测性域]] 协作**：本域负责基于已有 metrics/logs/traces 进行根因定位；可观测性域负责采集、存储、告警质量与 SLO 看板。排查时优先使用可观测性域定义的 golden signal 与告警链接。
- **与 [[安全/README.md|安全合规域]] 协作**：安全事件（异常 RBAC、证书泄露、容器逃逸）的初步隔离由本域执行（如 cordon 节点、隔离 namespace），深度取证、合规报告与修复由安全域主导。涉及紧急锁定时参考 紧急隔离预案（待补充）（规划中）。
- **与 [[可靠性/README.md|可靠性工程域]] 协作**：本域解决「已发生故障」的短期止血；可靠性域负责事后复盘、SLO 修复、混沌工程演练与灾备切换。postmortem 应同步到可靠性域的知识库。
- **与 [[网络/README.md|网络域]] 协作**：CNI、Service、Ingress、DNS、NetworkPolicy 的网络层根因由网络域提供专家意见；本域负责现象确认、信息收集与初步包捕获。
- **与 [[生产运维/README.md|生产运维域]] 协作**：值班响应、变更审批、事件升级与生产公告由生产运维域统一调度；本域提供技术判断与恢复命令。
- **与 [[云厂商/README.md|云厂商域]] 协作**：当 symptom 指向底层 IaaS（如节点 NotReady 由云盘延迟导致、API throttling）时，本域收集证据并提交给云厂商域进行厂商侧工单与升级。

### 协作边界示例：一次 Node NotReady 事件

1. 本域值班工程师通过 `kubectl describe node` 与节点日志初步判断为云盘 I/O 延迟引发 kubelet PLEG 超时。
2. 将证据（节点名称、kubelet 日志片段、云监控截图）提交给云厂商域。
3. 云厂商域向云厂商开 ticket 并协调更换节点；本域同步执行 Pod 迁移与受影响业务通知。
4. 事件恢复后，可靠性工程域主导 postmortem，安全合规域确认是否有审计日志缺失。

---

## 6. 审计与合规注意事项

- 所有生产排障命令建议通过堡垒机或审计 shell 执行，保留完整命令历史不少于 180 天。
- 使用 `kubectl auth can-i` 定期审计排障账号权限，确保不存在过度授权：
  ```bash
  kubectl auth can-i --list --as=system:serviceaccount:ops:troubleshooter
  ```
- 涉及敏感 namespace（如支付、用户数据）的排障必须双人复核，并在工单系统中登记。
- 临时容器与 debug Pod 的生命周期必须受 TTL 限制，排障结束后立即清理，避免长期驻留。

---

## 7. 推荐阅读

### 本域核心排障指南

- [[topic-structural-trouble-shooting/01-control-plane/01-apiserver-troubleshooting.md|API Server 故障排查指南]]
- [[topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md|Pod 故障排查与运行机制深度指南]]
- [[topic-structural-trouble-shooting/02-node-components/04-node-troubleshooting.md|节点问题专项排查指南]]
- [[topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md|NetworkPolicy 故障排查]]
- [[topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 故障排查]]
- [[topic-structural-trouble-shooting/06-security-auth/02-certificate-troubleshooting.md|证书故障排查]]

### 相关域扩展阅读

- [[可观测性/README.md|可观测性域 — 监控与告警]]
- [[安全/README.md|安全合规域 — 安全事件响应]]
- [[可靠性/README.md|可靠性工程域 — SRE 与灾备]]
- [[生产运维/README.md|生产运维域 — 值班与事件管理]]

---

*本文件作为 故障诊断 的生产就绪入口，建议在每次重大演练或架构变更后复审并更新检查清单。*


<!-- risk-assessed -->
