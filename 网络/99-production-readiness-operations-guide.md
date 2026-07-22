---
title: Networking & Traffic 生产就绪运维指南
description: 面向生产环境的 K8s 网络与流量全栈就绪检查、风险缓解、日常运维及故障排查入口指南
summary: 面向生产环境的 K8s 网络与流量全栈就绪检查、风险缓解、日常运维及故障排查入口指南
category: networking
tags:
- production
- best-practices
- networking
- operations
- cni
- ingress
- service-mesh
- dns
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
- Networking & Traffic 生产就绪运维指南是什么
- 如何按生产环境要求运维 K8s 网络与流量
trigger_keywords:
- 生产就绪
- 运维指南
- networking
- traffic
- cni
- ingress
- coredns
prerequisites:
- kubectl-basics
- networking-basics
- cni-basics
- helm-basics
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


# Networking & Traffic 生产就绪运维指南

> **适用版本**: [[kubernetes.md|Kubernetes]] v1.28-v1.33 | **最后更新**: 2026-07 | **定位**: 网络域生产交付前的最终检查与日常运维入口

本文档是 网络 的生产就绪入口指南，覆盖集群网络从 CNI、Service、DNS、Ingress/Gateway 到 Service Mesh 的全栈生产检查、风险缓解、日常运维与故障排查。Kubernetes 网络是平台稳定性的底座，任何单点故障、配置漂移或容量不足都可能引发全集群级影响，因此必须在生产上线前完成系统化检查，并在运行期间建立可重复的运维节奏。

## 范围与目标

本指南面向已经具备 kubectl 与基础网络知识的 SRE、运维工程师和平台工程师，目标是在生产环境交付前提供一份可执行的网络域就绪检查清单，并补充运行期最常见的高危风险与操作步骤。阅读本文档后，读者应能够独立完成网络域的上线前 gate 检查、识别关键风险、执行日常巡检，并在故障发生时按图索骥进行初步定位。对于各组件的深层原理与专项配置，请参考文末推荐阅读中的专项页面；对于本域当前尚未充分覆盖的 Calico 生产运维、云厂商 CNI 运维、网络灾备、网络 SLO/SLI 及组件升级 runbook，也请参考本域缺口分析中规划的新文件进行补齐。

## 1. 生产环境检查清单

在集群正式承接生产流量前，SRE 应逐项完成以下检查，并保留检查记录作为上线 gate 的凭证。清单共 12 项，覆盖控制面高可用、容量规划、安全隔离、入口高可用、可观测性与灾难恢复六大维度。

| 序号 | 检查项 | 验收标准 | 关键命令/配置 |
|---|---|---|---|
| 1 | CNI 控制面高可用 | 控制面多副本且跨可用区，数据面无单点 | `kubectl get pods -n kube-system -l k8s-app=<cni> -o wide` |
| 2 | CNI 版本兼容性 | CNI、内核、Kubernetes 版本均在官方支持矩阵内 | `kubectl get nodes -o wide` + CNI release notes |
| 3 | IP 地址规划 | Pod CIDR / Service CIDR / ENI IP 未来 12 个月不溢出 | `calicoctl ipam show` / `terway-cli show` |
| 4 | MTU 一致性 | Pod ↔ 节点 ↔ 隧道/物理网卡 MTU 匹配，无隐式分片 | `ip link show` / `kubectl exec <pod> -- ip link show eth0` |
| 5 | NetworkPolicy 生效 | 默认拒绝 + 显式放行，策略可审计、可回滚 | `kubectl get networkpolicy -A` + Cilium/Calico 策略日志 |
| 6 | Ingress/Gateway 高可用 | 至少 3 副本，跨可用区，启用 HPA/PDB | `kubectl get pods -n ingress-nginx -o wide` |
| 7 | CoreDNS 容量 | 按节点/Pod 数量 HPA，缓存命中率 >80% | `kubectl get hpa -n kube-system` + CoreDNS metrics |
| 8 | kube-proxy 模式 | 大规模集群使用 IPVS，metrics 已暴露 | `kubectl get cm kube-proxy -n kube-system -o yaml \| grep mode` |
| 9 | TLS/证书生命周期 | 证书有效期 >30 天，自动续期已配置 | `openssl x509 -dates -noout` |
| 10 | 网络可观测性 | 流量、延迟、丢包、conntrack 有监控告警 | Prometheus node-exporter + CNI metrics |
| 11 | 灾难恢复预案 | CNI 控制面、CoreDNS、Ingress blackout 恢复步骤已文档化并演练 | 检查 DR runbook 与演练记录 |
| 12 | 出口流量治理 | Egress 默认受控，关键外部依赖有固定 NAT/IP | `kubectl get egressgateway` / 云厂商 NAT 网关 |

清单中的前四项是网络底座稳定性的基础。CNI 控制面故障会直接导致 Pod 无法分配 IP，是最常见的 P0 场景之一；IP 地址规划不足则会在业务扩容时触发调度失败，且扩容 CIDR 通常需要计划内窗口；MTU 不一致往往表现为 HTTPS 握手后卡死或大包丢包，排查耗时较长；NetworkPolicy 如果配置不当，可能在升级或扩容时意外阻断合法流量，因此建议采用显式放行的白名单模式。后八项则面向生产运行期的可观测性、弹性和安全合规，缺一不可。

## 2. 关键风险与缓解措施

### 2.1 CNI 控制面故障导致 Pod 无法调度

**风险**: Calico typha、Cilium operator 或云厂商 CNI 控制面单点故障时，新 Pod 无法分配 IP，节点可能进入 NotReady，进而引发调度器将大量 Pod 堆积到健康节点，造成级联压力。

**缓解**:
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查控制面副本数与 PDB
kubectl get deploy -n kube-system -l k8s-app=calico-typha
kubectl get pdb -n kube-system

# 生产建议: Typha/Operator 至少 3 副本，PDB minAvailable=2
kubectl patch deploy calico-typha -n kube-system -p '{"spec":{"replicas":3}}'
```
同时应在变更窗口内对控制面进行滚动重启演练，确认新 Pod 调度不受影响，并监控 CNI 相关告警在重启期间是否误报。对于使用托管 CNI 的云集群，应确认云厂商 SLA 是否覆盖控制面可用性，并制定控制面不可控时的应急切换方案。

### 2.2 Pod IP / ENI IP 耗尽

**风险**: 大规模节点扩容或微服务密集部署时，IPAM 耗尽会导致 Pod 持续处于 ContainerCreating 状态，严重影响业务交付效率。云厂商 CNI 还可能因为 ENI/辅助 IP 配额不足而触发节点级失败。

**缓解**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Calico: 扩容 IPPool 或新增 Pool
calicoctl get ippool -o wide
calicoctl apply -f - <<EOF
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: pool-2
spec:
  cidr: 10.245.0.0/16
  natOutgoing: true
EOF

# 云厂商 CNI: 启用前缀分配 / Prefix Delegation
# AWS VPC CNI: ENABLE_PREFIX_DELEGATION=true
# Azure CNI: 启用 Overlay 或动态 IP 分配
```
建议每月 review IP 使用率趋势，并在使用率超过 70% 时触发扩容流程。对于阿里云 Terway 环境，可参考 Terway 专项指南中的 IPAM 监控方法，重点关注 `k8s.aliyun.com/allocated-eniips` 等节点注解。

### 2.3 Ingress 控制器单点故障

**风险**: Ingress 控制器单副本或同可用区部署，节点故障时外部流量中断。此外，配置热重载失败、证书更新异常也会导致入口层服务不可用。

**缓解**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查副本数、PDB、反亲和性
kubectl get deploy -n ingress-nginx
kubectl get pdb -n ingress-nginx
kubectl get pods -n ingress-nginx -o wide --show-labels

# 生产最小配置: 3 副本 + PDB minAvailable=2 + 跨 AZ topologySpreadConstraints
```
同时应配置 `config_last_reload_successful` 和 `nginx_ingress_controller_ssl_expire_time_seconds` 等关键指标的告警。对于七层流量较大的场景，建议将 Ingress 控制器与后端 Service 之间通过 Service Mesh 进行灰度与熔断治理。详情参考 [[K8s网络核心/26-ingress-production-best-practices.md|Ingress 生产最佳实践]]。

### 2.4 CoreDNS 级联故障

**风险**: CoreDNS 副本不足、CPU 限流或上游 DNS 不可达，导致全集群 DNS 解析超时。由于绝大多数微服务依赖服务名解析，DNS 故障会快速演变为应用级雪崩。

**缓解**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 扩容并启用 HPA
kubectl get deploy coredns -n kube-system
kubectl get hpa coredns -n kube-system
```
Corefile 缓存片段:
```
cache 30 {
  success 9984 300
  denial 9984 60
}
```

生产环境建议为 CoreDNS 配置独立的节点亲和或反亲和策略，避免与 CPU 密集型负载共存。同时应监控 `coredns_dns_request_duration_seconds` 的 P99 延迟，并设置 forward 上游超时的兜底策略。

### 2.5 TLS 证书过期

**风险**: Ingress / Gateway TLS 证书过期会导致 HTTPS 流量中断，用户侧出现证书错误，严重影响业务可信度和可用性。

**缓解**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 监控证书过期时间
kubectl get secret <tls-secret> -n <ns> -o jsonpath='{.data.tls\.crt}' \
  | base64 -d | openssl x509 -noout -dates

# cert-manager 自动续期
kubectl get certificate,certificaterequest,order -A
```
建议设置 30 天、14 天、7 天三级证书过期告警，并对 cert-manager 自身进行高可用部署和监控，避免证书管理器自身故障导致续期失败。对于内部服务间通信，应同步规划 mTLS 证书轮换策略。

### 2.6 Service Mesh 控制面故障

**风险**: Istio/Linkerd 控制面异常会导致 Sidecar 配置无法下发，新 Pod 启动失败或流量策略失效。

**缓解**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查控制面 Pod 与 Webhook
kubectl get pods -n istio-system
kubectl get mutatingwebhookconfiguration istio-sidecar-injector

# 确认 Sidecar 注入状态
kubectl get pods -n <app-ns> -o jsonpath='{.items[*].spec.containers[*].name}'
```
Service Mesh 的引入会显著增加网络栈复杂度，建议仅在确有 L7 治理需求的命名空间启用 Sidecar 注入，并为控制面配置独立的节点池和 PDB。

## 3. 日常运维操作

### 3.1 每日健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. CNI 组件
kubectl get pods -n kube-system -l k8s-app=<cni> -o wide

# 2. CoreDNS
kubectl get pods,svc,ep -n kube-system -l k8s-app=kube-dns
kubectl top pods -n kube-system -l k8s-app=kube-dns

# 3. Ingress
kubectl get pods -n ingress-nginx
kubectl top pods -n ingress-nginx

# 4. Service Endpoints
kubectl get endpoints -A | grep -v '<none>'

# 5. 节点网络内核参数
for n in $(kubectl get nodes -o name); do
  kubectl debug $n -it --image=nicolaka/netshoot -- sysctl \
    net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables \
    net.netfilter.nf_conntrack_count net.netfilter.nf_conntrack_max
done
```
每日检查应形成自动化脚本或 Dashboard，重点观察 CNI 和 CoreDNS 的 CPU/内存使用率、Ingress 的 5xx 率以及节点 conntrack 使用率。任何一项接近阈值都应提前介入，而不是等到告警触发。对于大规模集群，建议将每日巡检结果写入变更管理系统，便于后续审计。

### 3.2 网络策略合规审计

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出所有 NetworkPolicy
kubectl get networkpolicy -A

# Cilium: 实时查看策略命中
cilium policy get
hubble observe --verdict DROPPED

# Calico: 查看全局策略
calicoctl get networkpolicy -A
calicoctl get globalnetworkpolicy
```
建议每季度进行一次网络策略审计，清理过期策略，确认默认拒绝原则得到执行，并验证关键命名空间的流量路径与策略预期一致。审计过程中如发现大量 `DROPPED`  verdict 且业务无异常，可能是策略过于宽松或监控噪声，需要进一步收敛告警。

### 3.3 组件升级前检查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# CNI 升级前 dry-run
helm upgrade --install cilium cilium/cilium -n kube-system \
  --version <target> --dry-run --debug

# Ingress 控制器滚动升级
kubectl set image deployment/ingress-nginx-controller \
  -n ingress-nginx controller=registry.k8s.io/ingress-nginx/controller:<new> \
  --record
kubectl rollout status deployment/ingress-nginx-controller -n ingress-nginx
```
网络组件升级应遵循先在非生产环境灰度、再按可用区滚动、最后全量升级的节奏。升级前必须备份当前 CNI 配置和 Ingress 控制器 Deployment，确保可回滚。升级期间应暂停非紧急的证书更新和策略变更，避免多变更叠加导致定位困难。

### 3.4 容量与性能基线采集

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 采集关键基线指标
kubectl top nodes
kubectl top pods -A --sort-by=cpu
conntrack -L | wc -l
ss -s

# 网络吞吐基线
iperf3 -c <server-ip> -t 30 -P 4
```
建议每月采集一次网络基线，包括节点间带宽、Pod 间延迟、conntrack 使用率和 Ingress P99 延迟。基线数据可用于判断后续故障是否属于正常波动，也是容量规划的重要依据。

## 4. 故障排查速查

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| DNS 解析超时 5s | CoreDNS 副本不足 / NetworkPolicy 阻断 / 上游 DNS 故障 | `kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100` | 扩容 CoreDNS；检查 NetworkPolicy；修复 forward 上游 |
| Service ClusterIP 不通 | Endpoints 为空 / kube-proxy 规则缺失 | `kubectl get ep <svc> -n <ns>`; `iptables -t nat -L KUBE-SVC-XXXX` | 修复 selector；重启 kube-proxy |
| 跨节点 Pod 不通 | CNI 隧道端口被安全组阻断 / MTU 不一致 | 多跳 `tcpdump`; `ping -M do -s 1422` | 放行 VXLAN 4789/8472；统一 MTU |
| Ingress 返回 5xx | 后端健康检查失败 / 配置重载失败 | `kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller`; `curl localhost:10254/metrics` | 修复后端；回滚 Ingress 配置 |
| 随机丢包 / 新连接失败 | conntrack 表满 | `conntrack -L \| wc -l`; `cat /proc/sys/net/netfilter/nf_conntrack_count` | 调大 `nf_conntrack_max`；启用 eBPF CNI 绕过 conntrack |
| 外部访问间歇性失败 | SNAT 端口耗尽 / Egress IP 不足 | `ss -s`; 云厂商 NAT 监控 | 增加 NAT IP / 启用 SNAT 端口分配 |
| HTTPS 握手后卡死 | MTU 不匹配导致大包丢包 | `kubectl exec <pod> -- ping -M do -s 1472 <target>` | 修正 Pod/隧道/物理网卡 MTU |
| Service Mesh 服务间调用失败 | Sidecar 注入失败 / DestinationRule 配置错误 | `kubectl get pods -n <ns>`; `istioctl analyze` | 重新注入 Sidecar；修正 DestinationRule |

排查网络故障时，建议遵循先控制面后数据面、先节点后 Pod、先同节点后跨节点的分层思路。对于复杂场景，可结合 [[../故障诊断/FTA故障树/fta-index.md|故障树]] 进行结构化分析。在排查过程中应做好抓包和日志留存，便于事后复盘。

## 5. 与其他域的协作边界

| 本域职责 | 协作域 | 边界说明 |
|---|---|---|
| CNI / Service / Ingress 高可用架构 | [[../集群基础/01-production-architecture-design-principles.md|集群基础]] | 本域提供网络组件 HA 要求，集群域负责控制面、节点与升级基线 |
| NetworkPolicy / mTLS / 证书生命周期 | [[../安全/网络安全/02-network-security-policies.md|安全]] | 本域实施 L3/L4 网络隔离，安全域负责零信任架构、密钥与合规审计 |
| 网络指标、流量拓扑、告警 | [[../可观测性/指标/99-prometheus-enterprise-guide.md|可观测性]] | 本域定义网络黄金信号，可观测域负责采集、存储与可视化 |
| 网络 SLO/SLI、容灾演练 | [[../可靠性/灾难恢复/99-velero-backup-recovery-guide.md|可靠性]] | 本域提供网络组件 RTO/RPO 要求，可靠性域负责整体灾备设计 |
| 网络故障树与现场诊断 | [[../故障诊断/FTA故障树/fta-index.md|故障诊断]] | 本域提供网络专业知识，排障域负责结构化诊断流程 |
| 值班、变更、事件响应 | [[../生产运维/03-on-call-playbook.md|生产运维]] | 本域提供网络专项 runbook，生产运维域负责值班体系与变更管理 |

明确协作边界可以避免上线前责任不清、运行期间互相推诿。网络域应主动输出网络组件的 SLO 要求、变更窗口建议和应急预案，而其他域则提供平台级支撑。跨域变更评审时，网络域代表应参与涉及 CNI、Ingress、证书和 DNS 的变更，确保网络层面的影响被充分评估。

## 6. 推荐阅读

### 本域专项指南
- [[K8s网络核心/33-network-troubleshooting.md|网络故障诊断与链路排查]]
- [[K8s网络核心/27-cni-troubleshooting-optimization.md|CNI 故障排查与优化]]
- [[K8s网络核心/28-coredns-troubleshooting-optimization.md|CoreDNS 故障排查与性能优化]]
- [[K8s网络核心/26-ingress-production-best-practices.md|Ingress 生产最佳实践]]
- [[K8s网络核心/34-network-performance-tuning.md|网络性能调优]]
- [[K8s网络核心/16-networkpolicy-deep-practice.md|NetworkPolicy 深度实践]]
- [[K8s网络核心/18-network-encryption-mtls.md|网络加密与 mTLS]]
- [[K8s网络核心/32-multi-cluster-networking.md|多集群网络]]
- [[K8s网络核心/09-kube-proxy-modes-performance.md|kube-proxy 模式与性能]]
- [[API网关/11-api-gateway-security-practices.md|API Gateway 安全实践]]
- [[服务网格/99-linkerd-service-mesh-guide.md|Linkerd Service Mesh 指南]]

### 本域规划补齐文件（缺口分析推荐）
- Calico 生产运维指南（待补充）
- 云厂商 CNI 运维指南（待补充）
- 网络 SLO/SLI 指南（待补充）
- 网络组件升级 Runbook（待补充）
- 网络灾难恢复 Runbook（待补充）

### 跨域参考
- [[../集群基础/01-production-architecture-design-principles.md|集群生产架构设计原则]]
- [[../安全/网络安全/02-network-security-policies.md|网络安全策略]]
- [[../可观测性/指标/99-prometheus-enterprise-guide.md|Prometheus 企业级监控指南]]
- [[../可靠性/灾难恢复/99-velero-backup-recovery-guide.md|Velero 备份恢复指南]]
- [[../故障诊断/FTA故障树/list/networkpolicy-fta.md|NetworkPolicy 故障树]]
- [[../故障诊断/FTA故障树/list/dns-fta.md|DNS 故障树]]
- [[../故障诊断/FTA故障树/list/ingress-fta.md|Ingress 故障树]]

## 7. 快速检查脚本

```bash
#!/bin/bash
# 🟢 低风险：网络域生产就绪快速检查

echo "=== CNI 状态 ==="
kubectl get pods -n kube-system -l k8s-app=cilium -o wide 2>/dev/null || \
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide 2>/dev/null || \
kubectl get pods -n kube-system -l app=terway-eniip -o wide

echo -e "\n=== CoreDNS 状态 ==="
kubectl get pods,svc,ep -n kube-system -l k8s-app=kube-dns

echo -e "\n=== Ingress 状态 ==="
kubectl get pods -n ingress-nginx -o wide 2>/dev/null || \
kubectl get pods -n istio-system -l app=istio-ingressgateway -o wide 2>/dev/null

echo -e "\n=== 空 Endpoints 检查 ==="
kubectl get endpoints -A | grep '<none>'

echo -e "\n=== conntrack 使用率 ==="
for n in $(kubectl get nodes -o name | head -3); do
  echo "Node: $n"
  kubectl debug $n -it --image=nicolaka/netshoot -- cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null
done

echo -e "\n=== 检查完成 ==="
```

<!-- risk-assessed -->
