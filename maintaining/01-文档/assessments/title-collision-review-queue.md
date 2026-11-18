---
title: 同题异版人工评审单（P1-④）
category: references
tags: [reorg, dedup, review, corpus]
tier: supporting
created: '2026-11-18'
last_updated: '2026-11-18'
---

# 同题异版人工评审单（P1-④）

> 由 `review-title-collisions.py` 生成，数据源 `reorg-inventory.json`。
> 分类：索引/标签/跨层撞题签认 **94** 组（签认即关闭）；真同题异版 **58** 组（逐组裁决）；terway 三向专项 **8** 项。
> 裁决填法：在「你的裁决」列填 `按建议` / `保留副本来路径` / `保留全部（同场景不同实例）` / `需看全文 diff`。
> 「提示」列：相似度 <30% 时建议先确认是否为同场景不同实例（如不同工单号），低相似合并需谨慎。
> 相似度口径：行级 Jaccard（保守，跨结构重排的两版会显著偏低）；「差异摘要」以双方独有章节为准更可靠。

## A. terway 三向专项（F3，plan S-1）

> plan 主场 = `05-网络/06-Terway/`；本表用于确认各主题的择优方向与 23-实体侧压卡。
> **✅ 已裁决并执行（2026-11-18）**：全部按主场；18 个独有节回填主场（01-product 4 节、07-fta 4 节、其余 6 主题各 1-2 节），锚节编号顺延；`01-K8s网络核心/42-49` 旧版 8 篇落桩，`网络-qa` source 同步改写（726 条，悬空 0）。下表为裁决时快照，源侧行数现为桩。
> **待裁决**：B（58 组）、C（94 组）。23-实体侧 8 张卡按 plan S-1「压成 1 张速览卡」仍为待办。

| 主题 | 05/01 系列 (行) | 05/06 主场 (行) | 23/04 (行) | 05/01 vs 主场 相似度 | 差异摘要 | 你的裁决 |
|---|---:|---:|---:|---:|---|---|
| 产品概览 | 3 | 444 | 167 | 0% | 主场独有 41 节: 1. 产品定位, 10. 故障排查表, 11. 监控告警… | |
| 架构深潜 | 3 | 1009 | 109 | 0% | 主场独有 43 节: 1. 整体架构, 1.1 网络拓扑全景图, 1.2 核心设计原则… | |
| 使用指南 | 3 | 1173 | 114 | 0% | 主场独有 61 节: 1. 安装与初始化, 1.1 ACK 托管集群自动安装, 1.2 查看当前配置… | |
| CRD 操作 | 3 | 1470 | 123 | 0% | 主场独有 48 节: 1. CRD 资源架构, 10. 命令速查表, 11. NetworkAttachmentDefinition (Multus)… | |
| 运维手册 | 3 | 1515 | 109 | 0% | 主场独有 51 节: 1. 健康检查, 1.1 快速诊断命令, 1.2 terway-cli 诊断工具… | |
| 测试验证 | 3 | 1222 | 105 | 0% | 主场独有 52 节: 1. Pod 网络基础验证, 1.1 创建测试 Pod, 1.2 验证 Pod IP (ENIIP 模式)… | |
| 性能调优 | 3 | 748 | 111 | 0% | 主场独有 47 节: 1. 网络模式性能对比, 1.1 吞吐量与延迟基准, 1.2 模式选型决策矩阵… | |
| 故障排查 | 3 | 926 | 127 | 0% | 主场独有 62 节: 1. FTA 故障树全景, 10. AND 门组合问题, 10.1 IP 完全耗尽 (vSwitch 空 + IP 泄漏)… | |

## B. 真同题异版（逐组裁决）

| # | 标题 | 成员（主场**加粗**） | 主场建议(规则) | 相似度 vs 主场 | 行数(主/副) | 差异摘要 | 提示 | 你的裁决 |
|---:|---|---|---|---:|---|---|---|---|
| 1 | statefulset pod 启动失败：pvc 未绑定 | **wiki/13-生产运维/05-工单案例/ticket-case-028-statefulset-pvc-unbound.md**<br>wiki/13-生产运维/05-工单案例/ticket-case-033-statefulset-pvc-unbound.md<br>wiki/13-生产运维/05-工单案例/ticket-case-038-statefulset-pvc-unbound.md<br>wiki/13-生产运维/05-工单案例/ticket-case-043-statefulset-pvc-unbound.md | R-SHORT | 14%, 12%, 11% | 270/270/227/200 | 章节结构一致，差异在正文措辞/数据 | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 2 | pod 安全策略 | wiki/17-系统基础/06-知识字典/security/pod-security-policies.md<br>**wiki/17-系统基础/06-知识字典/security/pod-security-policy.md**<br>skills/04-工作负载/pod/安全/字典-pod-security-policies.md<br>skills/04-工作负载/pod/安全/字典-pod-security-policy.md | R-WIKI | 14%, 6%, 26% | 197/199/57/59 | 副本独有 7 节: PSP 准入评估链路（v1.21-，已弃用）, 使用场景, 替代方案…；主场独有 6 节: PSP 校验字段全景与决策流, PSP 的功能（已弃用）, PSP 的历史… | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 3 | k8s 网络快问快答与面试模拟对话 | wiki/05-网络/01-K8s网络核心/58-kubernetes-network-quick-qa 2.md<br>wiki/05-网络/01-K8s网络核心/58-kubernetes-network-quick-qa 3.md<br>**wiki/05-网络/01-K8s网络核心/58-kubernetes-network-quick-qa.md** | R-SHORT | 40%, 75% | 768/361/623 | 主场独有 19 节: 43. 发往 ClusterIP 的数据包，在 iptables 模式下究竟在哪台机器的哪个链上被 DNAT 成 Pod IP？kube-proxy 的规则数量与 Service/Endpoint 数量是什么关系，为什么大规模集群有性能问题而 IPVS 能缓解？, 44. conntrack 表满（`nf_conntrack: table full, dropping packet`）时丢的是什么包，为什么表现为新建连接失败但已有连接正常？UDP 服务（如 CoreDNS）高并发下的 conntrack 竞态发生在哪一步，常见的两种缓解手段是什么？, 45. Flannel 的 VXLAN 与 host-gw 后端转发路径有什么本质区别？封装开销各是多少？Directrouting 混合模式解决了什么？… |  | |
| 4 | k8s 存储高级快问快答（12 题） | **wiki/06-存储/01-K8s存储/23-k8s-storage-advanced-quick-qa 2.md**<br>wiki/06-存储/01-K8s存储/23-k8s-storage-advanced-quick-qa 3.md<br>wiki/06-存储/01-K8s存储/23-k8s-storage-advanced-quick-qa 4.md | R-SHORT | 79%, 56% | 196/235/318 | 章节结构一致，差异在正文措辞/数据 |  | |
| 5 | ingress 控制器 pod 异常导致业务访问 404/502 | wiki/13-生产运维/05-工单案例/ticket-case-011-ingress-controller-pod-404-502.md<br>wiki/13-生产运维/05-工单案例/ticket-case-036-ingress-controller-pod-exception.md<br>**wiki/13-生产运维/05-工单案例/ticket-case-046-ingress-controller-404-502.md** | R-SHORT | 12%, 13% | 259/187/210 | 章节结构一致，差异在正文措辞/数据 | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 6 | ingress 控制器 pod 异常导致 404/502 | wiki/13-生产运维/05-工单案例/ticket-case-026-ingress-controller-pod-502.md<br>wiki/13-生产运维/05-工单案例/ticket-case-031-ingress-controller-oom-502-404.md<br>**wiki/13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md** | R-SHORT | 12%, 14% | 215/226/226 | 章节结构一致，差异在正文措辞/数据 | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 7 | pod crashloopbackoff & oomkilled 诊断与修复 | **wiki/19-故障诊断/08-技能体系/02-pod-crashloop-oomkilled.md**<br>skills/04-工作负载/pod/01-pod-crashloop-oomkilled.md<br>skills/04-工作负载/pod/诊断排障/技能体系-02-pod-crashloop-oomkilled.md | R-WIKI | 3%, 100% | 1537/538/1537 | 副本独有 22 节: 10.2 生产案例, 10.3 混沌工程验证, 10.4 监控告警配置…；主场独有 33 节: 10.2 深度知识引用, 10.3 关键知识点备忘, 10.4 Skill 改进记录… |  | |
| 8 | kubernetes 通用最佳实践参考 (best-practices) | wiki/20-最佳实践/01-best-practices/common-best-practices-2.md<br>**wiki/20-最佳实践/01-best-practices/common-best-practices.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/common-best-practices.md | R-WIKI | 97%, 94% | 745/745/745 | 章节结构一致，差异在正文措辞/数据 |  | |
| 9 | 第二课：pod - k8s 的最小调度单元 [fundamentals] | skills/04-工作负载/pod/培训/03-pod-basics.md<br>**skills/04-工作负载/pod/培训/lecturer/02-pod-basics.md**<br>skills/04-工作负载/pod/培训/lecturer-02-pod-basics.md | R-TRAIN | 98%, 99% | 438/438/438 | 章节结构一致，差异在正文措辞/数据 |  | |
| 10 | kubernetes 多集群 fleet gitops 运维指南 | **wiki/11-发布变更/01-GitOps/10-fleet-gitops-operations-guide.md**<br>wiki/11-发布变更/01-GitOps/14-fleet-gitops-operations-guide.md | R-SHORT | 9% | 997/472 | 副本独有 10 节: 1.1 Fleet GitOps 分层架构, 2.1 基础设施前提, 2.2 必备工具…；主场独有 34 节: 1.1 适用场景, 1.2 适用范围, 1.3 不适用场景… | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 11 | kubernetes 灾难恢复与业务连续性 runbook | **wiki/12-可靠性/02-灾难恢复/13-disaster-recovery-bc-runbook-v1.md**<br>wiki/12-可靠性/02-灾难恢复/25-disaster-recovery-bc-runbook-v2.md | R-SHORT | 4% | 597/294 | 副本独有 13 节: 1.1 DR 架构模式选型, 2.1 备份基础设施, 2.2 必备工具…；主场独有 16 节: 2.1 信息清单（必须提前维护）, 2.2 工具与权限, 2.3 备份就绪检查（每日自动巡检）… | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 12 | 节点磁盘压力 diskpressure 导致 pod 被驱逐 | **wiki/13-生产运维/05-工单案例/ticket-case-035-node-diskpressure-eviction.md**<br>wiki/13-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md | R-SHORT | 16% | 239/226 | 副本独有 1 节: 风险与影响评估 | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 13 | 容器运行时安全加固指南 | wiki/14-容器运行时/03-containerd-CRI-O/08-runtime-security-hardening-2.md<br>**wiki/14-容器运行时/03-containerd-CRI-O/09-runtime-security-hardening.md** | R-SHORT | 19% | 362/304 | 副本独有 10 节: 3. 核心概念/架构, 4.1 启用默认 seccomp Profile, 4.2 AppArmor 配置（Ubuntu/Debian）…；主场独有 15 节: 3. 核心概念与架构, 3.1 运行时安全分层, 3.2 关键原则… | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 14 | k8s 运维诊断技能库 (02-ai-agents) | **wiki/15-AI基础设施/02-AI-Agents/openclaw-workspace/SKILL.md**<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/SKILL.md | R-SHORT | 95% | 386/393 | 副本独有 1 节: 相关概念 |  | |
| 15 | 02 - pod 与容器生命周期事件 | **wiki/17-系统基础/04-K8s事件/02-pod-container-lifecycle-events.md**<br>skills/04-工作负载/pod/生命周期与事件/01-pod-container-lifecycle-events.md | R-WIKI | 99% | 2523/2523 | 章节结构一致，差异在正文措辞/数据 |  | |
| 16 | eks 身份认证 — irsa 与 pod identity | **wiki/18-云厂商/02-AWS-EKS/04-eks-iam-irsa-pod-identity.md**<br>skills/04-工作负载/pod/云厂商/01-eks-iam-irsa-pod-identity.md | R-WIKI | 98% | 428/428 | 章节结构一致，差异在正文措辞/数据 |  | |
| 17 | pod pending 状态深度诊断 | **wiki/19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md**<br>skills/04-工作负载/pod/诊断排障/01-pod-pending-diagnosis.md | R-WIKI | 99% | 1808/1808 | 章节结构一致，差异在正文措辞/数据 |  | |
| 18 | 节点组件故障排查 | **wiki/19-故障诊断/04-高级排障/01-node-component-troubleshooting.md**<br>skills/03-节点/node/03-node-component-troubleshooting.md | R-WIKI | 4% | 803/501 | 副本独有 38 节: 1. 概述, 2. 症状识别, 3. kubelet 故障排查…；主场独有 27 节: 1. 概述与诊断框架, 1.1 节点组件概述, 1.2 常见问题现象分类… | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 19 | 备份/恢复异常故障树分析 | **wiki/19-故障诊断/06-FTA故障树/list/backup-restore-fta.md**<br>skills/02-控制面/etcd/backup-restore-fta.md | R-WIKI | 6% | 779/405 | 副本独有 27 节: 1. 概述, 10. 知识进化, 2. 症状识别…；主场独有 9 节: Agent 友好性, FTA 评审检查表, JSON 工作流（含与/或门控件）… | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 20 | pod pending 调度失败诊断与修复 | **wiki/19-故障诊断/08-技能体系/03-pod-pending.md**<br>skills/04-工作负载/pod/诊断排障/技能体系-03-pod-pending.md | R-WIKI | 100% | 1566/1566 | 章节结构一致，差异在正文措辞/数据 |  | |
| 21 | node notready 远程顾问对话脚本 | wiki/19-故障诊断/08-技能体系/skill-set/k8s-node-notready/DIALOGUE.md<br>**wiki/19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md** | R-SHORT | 94% | 624/604 | 章节结构一致，差异在正文措辞/数据 |  | |
| 22 | 命令输出解读语料 — agent 诊断推理核心数据 [故障诊断] | **wiki/19-故障诊断/10-QA语料/command-output-diagnosis.md**<br>wiki/19-故障诊断/10-QA语料/command-output-diagnosis.with_actions.md | R-SHORT | 17% | 488/673 | 副本独有 3 节: 4.1 Service 无法访问, 4.2 CoreDNS 解析失败, 4.3 Ingress 502 Bad Gateway；主场独有 3 节: 4.1 [[service|Service]] 无法访问, 4.2 [[coredns|CoreDNS]] 解析失败, 4.3 [[ingress|Ingress]] 502 Bad Gateway | ⚠️ 低相似(<30%)：先确认是否同场景不同实例 | |
| 23 | 命令输出诊断语料 — p0 优先级 | **wiki/19-故障诊断/10-QA语料/generated/command-output-diagnosis-p0.md**<br>wiki/19-故障诊断/10-QA语料/generated/command-output-diagnosis-p0.with_actions.md | R-SHORT | 78% | 19785/21891 | 副本独有 62 节: Domain: CERT (9 条), Domain: CP (13 条), Domain: GENERAL (30 条)…；主场独有 107 节: Domain: CERT (15 条), Domain: CP (15 条), Domain: GENERAL (36 条)… |  | |
| 24 | 命令输出诊断语料 — p1 优先级 | **wiki/19-故障诊断/10-QA语料/generated/command-output-diagnosis-p1.md**<br>wiki/19-故障诊断/10-QA语料/generated/command-output-diagnosis-p1.with_actions.md | R-SHORT | 78% | 19785/21891 | 副本独有 62 节: Domain: CERT (9 条), Domain: CP (13 条), Domain: GENERAL (30 条)…；主场独有 107 节: Domain: CERT (15 条), Domain: CP (15 条), Domain: GENERAL (36 条)… |  | |
| 25 | 命令输出诊断语料 — p2 优先级 | **wiki/19-故障诊断/10-QA语料/generated/command-output-diagnosis-p2.md**<br>wiki/19-故障诊断/10-QA语料/generated/command-output-diagnosis-p2.with_actions.md | R-SHORT | 78% | 19785/21891 | 副本独有 62 节: Domain: CERT (9 条), Domain: CP (13 条), Domain: GENERAL (30 条)…；主场独有 107 节: Domain: CERT (15 条), Domain: CP (15 条), Domain: GENERAL (36 条)… |  | |
| 26 | p0 核心场景手工种子 i-o 对 | **wiki/19-故障诊断/10-QA语料/seed/p0-core-scenarios.md**<br>wiki/19-故障诊断/10-QA语料/seed/p0-core-scenarios.with_actions.md | R-SHORT | 36% | 743/1173 | 章节结构一致，差异在正文措辞/数据 |  | |
| 27 | kubernetes 网络配置最佳实践 [infrastructure] | wiki/20-最佳实践/01-best-practices/infrastructure/networking.md<br>**wiki/20-最佳实践/03-infrastructure/networking.md** | R-20 | 97% | 565/558 | 主场独有 1 节: See Also |  | |
| 28 | kubernetes 部署策略最佳实践 [operations] | **wiki/20-最佳实践/01-best-practices/operations/deployment.md**<br>skills/04-工作负载/deployment/最佳实践/deployment-alt.md | R-WIKI | 96% | 737/722 | 章节结构一致，差异在正文措辞/数据 |  | |
| 29 | kubernetes 密钥管理最佳实践 [security] | **wiki/20-最佳实践/01-best-practices/security/secrets-management.md**<br>skills/07-安全/rbac/最佳实践/secrets-management-alt.md | R-WIKI | 100% | 570/569 | 章节结构一致，差异在正文措辞/数据 |  | |
| 30 | kubernetes production best practices — o | **wiki/20-最佳实践/06-operations/01-production-operations-excellence-checklist.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/01-production-operations-excellence-checklist.md | R-WIKI | 98% | 370/370 | 章节结构一致，差异在正文措辞/数据 |  | |
| 31 | 场景: ai 基础设施运维 | **wiki/20-最佳实践/07-scenarios/ai-infra-ops.md**<br>skills/03-节点/gpu/最佳实践/ai-infra-ops.md | R-WIKI | 64% | 129/89 | 主场独有 4 节: 案例1：GPU 节点驱动升级导致训练任务全部失败, 案例2：推理服务 GPU 显存 OOM, 生产案例… |  | |
| 32 | 场景: 应用部署 | **wiki/20-最佳实践/07-scenarios/app-deployment.md**<br>skills/04-工作负载/deployment/最佳实践/app-deployment.md | R-WIKI | 63% | 140/95 | 主场独有 4 节: 案例1：滚动更新导致服务短暂不可用, 案例2：资源限制设置不当导致 OOM, 生产案例… |  | |
| 33 | 场景: 备份恢复 | **wiki/20-最佳实践/07-scenarios/backup-restore.md**<br>skills/02-控制面/etcd/最佳实践/backup-restore.md | R-WIKI | 66% | 132/92 | 主场独有 4 节: 案例1：etcd 备份恢复失败导致集群不可用, 案例2：Velero 备份遗漏 PV 数据, 生产案例… |  | |
| 34 | 场景: 容量规划 | **wiki/20-最佳实践/07-scenarios/capacity-planning.md**<br>skills/04-工作负载/hpa-vpa/最佳实践/capacity-planning.md | R-WIKI | 67% | 129/91 | 主场独有 4 节: 案例1：业务突增导致集群资源耗尽, 案例2：etcd 磁盘空间不足导致集群不可用, 生产案例… |  | |
| 35 | 场景: 集群部署 | **wiki/20-最佳实践/07-scenarios/cluster-deployment.md**<br>skills/01-集群运维/cluster-deployment/cluster-deployment.md | R-WIKI | 67% | 136/96 | 主场独有 4 节: 案例1：kubeadm 初始化失败导致集群无法启动, 案例2：节点加入失败 - 证书过期, 生产案例… |  | |
| 36 | 场景: 合规审计 | **wiki/20-最佳实践/07-scenarios/compliance-audit.md**<br>skills/07-安全/rbac/最佳实践/compliance-audit.md | R-WIKI | 66% | 131/92 | 主场独有 4 节: 案例1：安全审计发现特权容器泛滥, 案例2：RBAC 权限过大导致安全事件, 生产案例… |  | |
| 37 | 场景: 成本优化 | **wiki/20-最佳实践/07-scenarios/cost-optimization.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/cost-optimization.md | R-WIKI | 64% | 129/89 | 主场独有 4 节: 案例1：资源过度分配导致集群成本浪费 60%, 案例2：未清理的测试环境持续产生费用, 生产案例… |  | |
| 38 | 场景: 日常运维 | **wiki/20-最佳实践/07-scenarios/daily-ops.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/daily-ops.md | R-WIKI | 66% | 130/91 | 主场独有 4 节: 案例1：日常巡检遗漏导致证书过期, 案例2：日志轮转未配置导致磁盘满, 生产案例… |  | |
| 39 | 场景: 边缘计算运维 | **wiki/20-最佳实践/07-scenarios/edge-ops.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/edge-ops.md | R-WIKI | 66% | 129/90 | 主场独有 4 节: 案例1：边缘节点网络断连后 Pod 状态不一致, 案例2：边缘 OTA 升级失败导致批量节点失联, 生产案例… |  | |
| 40 | 场景: gitops 工作流 | **wiki/20-最佳实践/07-scenarios/gitops-workflow.md**<br>skills/01-集群运维/gitops-argocd/最佳实践/gitops-workflow.md | R-WIKI | 65% | 132/92 | 主场独有 4 节: 案例1：GitOps 同步失败导致配置漂移, 案例2：Git 仓库权限配置错误导致未授权部署, 生产案例… |  | |
| 41 | 场景: service mesh 运维 | **wiki/20-最佳实践/07-scenarios/mesh-ops.md**<br>skills/05-网络/service-mesh/最佳实践/mesh-ops.md | R-WIKI | 65% | 130/90 | 主场独有 4 节: 案例1：Istio Sidecar 注入导致服务启动失败, 案例2：服务网格升级导致流量中断, 生产案例… |  | |
| 42 | 场景: 监控告警 | **wiki/20-最佳实践/07-scenarios/monitoring-alerting.md**<br>skills/08-可观测性/monitoring/最佳实践/monitoring-alerting.md | R-WIKI | 64% | 134/92 | 主场独有 4 节: 案例1：告警风暴导致关键告警被淹没, 案例2：监控盲区导致故障发现延迟, 生产案例… |  | |
| 43 | 场景: 多集群管理 | **wiki/20-最佳实践/07-scenarios/multi-cluster.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/multi-cluster.md | R-WIKI | 63% | 129/88 | 主场独有 4 节: 案例1：多集群配置同步不一致导致服务异常, 案例2：跨集群服务发现失败, 生产案例… |  | |
| 44 | 场景: 网络诊断 | **wiki/20-最佳实践/07-scenarios/network-diagnosis.md**<br>skills/05-网络/service/最佳实践/network-diagnosis.md | R-WIKI | 65% | 139/97 | 主场独有 4 节: 案例1：Pod 间通信间歇性超时, 案例2：Service 无法访问但 Pod 直接访问正常, 生产案例… |  | |
| 45 | 场景: 性能调优 | **wiki/20-最佳实践/07-scenarios/performance-tuning.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/performance-tuning.md | R-WIKI | 65% | 134/93 | 主场独有 4 节: 案例1：API Server 延迟飙升导致集群响应慢, 案例2：Pod 调度延迟影响业务扩容, 生产案例… |  | |
| 46 | 场景: 安全加固 | **wiki/20-最佳实践/07-scenarios/security-hardening.md**<br>skills/07-安全/rbac/最佳实践/security-hardening.md | R-WIKI | 65% | 134/93 | 主场独有 4 节: 案例1：匿名访问未禁用导致数据泄露, 案例2：Secret 明文存储被窃取, 生产案例… |  | |
| 47 | 场景: 安全事件响应 | **wiki/20-最佳实践/07-scenarios/security-incident.md**<br>skills/07-安全/rbac/最佳实践/security-incident.md | R-WIKI | 65% | 132/92 | 主场独有 4 节: 案例1：容器逃逸导致节点被入侵, 案例2：镜像供应链攻击, 生产案例… |  | |
| 48 | 场景: 存储问题排查 | **wiki/20-最佳实践/07-scenarios/storage-issues.md**<br>skills/06-存储/csi-storage/最佳实践/storage-issues.md | R-WIKI | 66% | 130/92 | 主场独有 4 节: 案例1：PV 容量不足导致数据库写入失败, 案例2：存储后端故障导致多节点 IO hang, 生产案例… |  | |
| 49 | 场景: 故障排查 | **wiki/20-最佳实践/07-scenarios/troubleshooting.md**<br>skills/04-工作负载/pod/最佳实践/troubleshooting.md | R-WIKI | 65% | 131/91 | 主场独有 4 节: 案例1：DNS 解析间歇性失败导致服务抖动, 案例2：节点 NotReady 但 Pod 仍在运行, 生产案例… |  | |
| 50 | 场景: 升级迁移 | **wiki/20-最佳实践/07-scenarios/upgrade-migration.md**<br>skills/01-集群运维/cluster-upgrade/最佳实践/upgrade-migration.md | R-WIKI | 65% | 130/90 | 主场独有 4 节: 案例1：K8s 版本升级后 API 废弃导致服务中断, 案例2：跨集群迁移时 PV 数据丢失, 生产案例… |  | |
| 51 | 第三课：deployment - 应用部署管理器 [fundamentals] | skills/04-工作负载/deployment/培训/01-deployment-basics.md<br>**skills/04-工作负载/deployment/培训/lecturer/01-deployment-basics.md** | R-TRAIN | 98% | 433/433 | 章节结构一致，差异在正文措辞/数据 |  | |
| 52 | pod 异常故障树分析 (skills) | **skills/04-工作负载/pod/pod-fta.md**<br>skills/04-工作负载/pod/诊断排障/pod-fta-故障树list.md | R-SHORT | 35% | 102/151 | 副本独有 5 节: 告警规则, 案例1: 资源不足导致大规模 Pending, 案例2: 镜像拉取失败导致 CrashLoopBackOff…；主场独有 3 节: 升级决策点, 案例 1: Pod CrashLoopBackOff——配置错误导致启动失败, 案例 2: Pod Pending——资源请求超出节点可分配量 |  | |
| 53 | customizing pods | **skills/04-工作负载/pod/上游源码/kubectl-docs/1.18/customizing_pod_templates.md**<br>skills/04-工作负载/pod/上游源码/kubectl-docs/1.20/customizing_pod_templates.md | R-SHORT | 97% | 359/359 | 章节结构一致，差异在正文措辞/数据 |  | |
| 54 | port forward | **skills/04-工作负载/pod/上游源码/kubectl-docs/1.18/port_forward_to_pods.md**<br>skills/04-工作负载/pod/上游源码/kubectl-docs/1.20/port_forward_to_pods.md | R-SHORT | 90% | 70/84 | 副本独有 1 节: Pod in a Service |  | |
| 55 | 第一课：kubernetes 入门 [fundamentals] | skills/04-工作负载/pod/培训/02-what-is-kubernetes.md<br>**skills/04-工作负载/pod/培训/lecturer/01-what-is-kubernetes.md** | R-TRAIN | 98% | 360/360 | 章节结构一致，差异在正文措辞/数据 |  | |
| 56 | day 19: pod 容器组基础 | skills/04-工作负载/pod/培训/inner-day-19-pod-basics.md<br>**skills/04-工作负载/pod/培训/inner-training/week-3-node-workload/day-19-pod-basics.md** | R-TRAIN | 99% | 572/572 | 章节结构一致，差异在正文措辞/数据 |  | |
| 57 | 故障排查决策树 - mermaid 可视化版 [troubleshooting] | **skills/04-工作负载/pod/培训/lecturer/decision-tree-mermaid.md**<br>skills/04-工作负载/pod/培训/troubleshooting/decision-tree-mermaid.md | R-SHORT | 100% | 572/572 | 章节结构一致，差异在正文措辞/数据 |  | |
| 58 | 工单数字人快速问答 - on-call 速查 [oncall-qa] | **skills/04-工作负载/pod/培训/lecturer/oncall-quick-qa.md**<br>skills/04-工作负载/pod/培训/oncall-qa/oncall-quick-qa.md | R-SHORT | 100% | 1316/1316 | 章节结构一致，差异在正文措辞/数据 |  | |

## C. 索引/标签/跨层撞题（列表签认即关闭）

> 成员少于 2 篇同体裁文章：索引/README/标签页撞题，或字典/概念/实体/速查卡各一的跨层撞题——章程允许共存。

| # | 标题 | 成员（行数） | 你的签认 |
|---:|---|---|---|
| 1 | ack 关联产品 - ebs 云盘存储 (elastic block stora | wiki/18-云厂商/01-阿里云/公有云-ACK/007-ack-ebs-storage.md（184 行）<br>wiki/23-实体/13-云厂商与发行版/006-ack-ebs-storage.md（145 行） | |
| 2 | ack 关联产品 - ecs 计算资源 | wiki/18-云厂商/01-阿里云/公有云-ACK/002-ack-ecs-compute.md（252 行）<br>wiki/23-实体/13-云厂商与发行版/001-ack-ecs-compute.md（166 行） | |
| 3 | ack 关联产品 - ram 权限与授权 (ram & rrsa) | wiki/18-云厂商/01-阿里云/公有云-ACK/005-ack-ram-authorization.md（188 行）<br>wiki/23-实体/13-云厂商与发行版/004-ack-ram-authorization.md（133 行） | |
| 4 | ack 关联产品 - ros 资源编排 (iac) | wiki/18-云厂商/01-阿里云/公有云-ACK/006-ack-ros-iac.md（187 行）<br>wiki/23-实体/13-云厂商与发行版/005-ack-ros-iac.md（143 行） | |
| 5 | ack 关联产品 - vpc 网络 | wiki/18-云厂商/01-阿里云/公有云-ACK/004-ack-vpc-network.md（173 行）<br>wiki/23-实体/13-云厂商与发行版/003-ack-vpc-network.md（136 行） | |
| 6 | ack 关联产品 - 负载均衡 (slb/nlb/alb) | wiki/18-云厂商/01-阿里云/公有云-ACK/003-ack-slb-nlb-alb.md（178 行）<br>wiki/23-实体/13-云厂商与发行版/002-ack-slb-nlb-alb.md（147 行） | |
| 7 | advanced pod configuration | wiki/17-系统基础/06-知识字典/workloads/advanced-pod-configuration.md（219 行）<br>skills/04-工作负载/pod/配置与字典/advanced-pod-configuration.md（219 行） | |
| 8 | assigning pods to nodes | wiki/17-系统基础/06-知识字典/scheduling/assigning-pods-to-nodes.md（244 行）<br>skills/04-工作负载/pod/调度/assigning-pods-to-nodes.md（244 行） | |
| 9 | best practices | wiki/20-最佳实践/01-best-practices/index.md（22 行）<br>wiki/20-最佳实践/index.md（27 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/bp-index-alt.md（22 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/bp-index.md（27 行） | |
| 10 | cloud native buildpacks | wiki/17-系统基础/06-知识字典/tooling/buildpacks.md（53 行）<br>wiki/23-实体/08-交付与制品/buildpacks.md（223 行） | |
| 11 | cncf incubating projects | wiki/21-生态参考/01-CNCF全景/incubating/01-cncf-incubating-projects.md（187 行）<br>wiki/21-生态参考/01-CNCF全景/incubating/index.md（15 行） | |
| 12 | cncf sandbox projects | wiki/21-生态参考/01-CNCF全景/sandbox/01-cncf-sandbox-projects.md（195 行）<br>wiki/21-生态参考/01-CNCF全景/sandbox/index.md（15 行） | |
| 13 | configuration | skills/04-工作负载/pod/培训/lecturer/lecturer-05-configuration-index.md（11 行）<br>skills/04-工作负载/pod/培训/lecturer/lecturer-06-configuration-index.md（11 行） | |
| 14 | container runtime | wiki/14-容器运行时/README.md（48 行）<br>wiki/22-概念/15-运行时与系统/container-runtime.md（268 行） | |
| 15 | containerd | wiki/17-系统基础/06-知识字典/fundamentals/containerd.md（86 行）<br>wiki/26-标签/01-核心平台/containerd.md（238 行） | |
| 16 | cronjob | wiki/17-系统基础/06-知识字典/workloads/cronjob.md（227 行）<br>wiki/22-概念/02-工作负载/cronjob.md（319 行） | |
| 17 | daemonset | wiki/17-系统基础/06-知识字典/workloads/daemonset.md（247 行）<br>wiki/22-概念/02-工作负载/daemonset.md（361 行） | |
| 18 | deployment | wiki/17-系统基础/06-知识字典/workloads/deployment.md（109 行）<br>wiki/20-最佳实践/02-deployment/index.md（19 行）<br>wiki/20-最佳实践/06-operations/deployment.md（188 行）<br>wiki/23-实体/02-K8s核心组件/deployment.md（147 行）<br>skills/01-集群运维/cluster-deployment/index.md（19 行） | |
| 19 | deployments | wiki/17-系统基础/06-知识字典/workloads/deployments.md（253 行）<br>wiki/22-概念/02-工作负载/deployments.md（304 行） | |
| 20 | dns for services and pods | wiki/17-系统基础/06-知识字典/networking/dns-for-services-and-pods.md（199 行）<br>skills/04-工作负载/pod/配置与字典/dns-for-services-and-pods.md（199 行） | |
| 21 | docker | wiki/14-容器运行时/01-Docker/index.md（31 行）<br>wiki/17-系统基础/05-速查卡/docker.md（506 行）<br>wiki/17-系统基础/06-知识字典/fundamentals/docker.md（74 行）<br>wiki/23-实体/03-运行时/docker.md（196 行） | |
| 22 | downward api | wiki/17-系统基础/06-知识字典/workloads/downward-api.md（281 行）<br>wiki/22-概念/02-工作负载/downward-api.md（335 行） | |
| 23 | dynamic resource allocation | wiki/17-系统基础/06-知识字典/scheduling/dynamic-resource-allocation.md（210 行）<br>wiki/22-概念/07-调度与资源/dynamic-resource-allocation.md（251 行） | |
| 24 | ephemeral containers | wiki/17-系统基础/06-知识字典/workloads/ephemeral-containers.md（175 行）<br>wiki/22-概念/02-工作负载/ephemeral-containers.md（260 行） | |
| 25 | gang scheduling | wiki/17-系统基础/06-知识字典/scheduling/gang-scheduling.md（164 行）<br>wiki/22-概念/07-调度与资源/gang-scheduling.md（298 行） | |
| 26 | gateway api | wiki/17-系统基础/05-速查卡/gateway-api.md（473 行）<br>wiki/17-系统基础/06-知识字典/networking/gateway-api.md（237 行） | |
| 27 | gitops | wiki/11-发布变更/01-GitOps/index.md（36 行）<br>wiki/17-系统基础/06-知识字典/operations/gitops.md（65 行）<br>wiki/26-标签/05-交付与运维/gitops.md（168 行） | |
| 28 | hashicorp vault | wiki/17-系统基础/06-知识字典/security/vault.md（211 行）<br>wiki/23-实体/06-安全/vault.md（230 行） | |
| 29 | helm | wiki/17-系统基础/06-知识字典/tooling/helm.md（95 行）<br>wiki/26-标签/05-交付与运维/helm.md（214 行） | |
| 30 | horizontal pod autoscaler | wiki/22-概念/07-调度与资源/horizontal-pod-autoscaler.md（211 行）<br>skills/04-工作负载/pod/资源与自动扩缩/horizontal-pod-autoscaler.md（210 行） | |
| 31 | horizontal pod autoscaling | wiki/17-系统基础/06-知识字典/workloads/horizontal-pod-autoscaling.md（243 行）<br>skills/04-工作负载/pod/资源与自动扩缩/字典-horizontal-pod-autoscaling.md（243 行） | |
| 32 | infrastructure | wiki/20-最佳实践/01-best-practices/infrastructure/index.md（13 行）<br>wiki/20-最佳实践/03-infrastructure/index.md（13 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/infra-index-alt.md（13 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/infra-index.md（13 行） | |
| 33 | ingress | wiki/17-系统基础/06-知识字典/networking/ingress.md（248 行）<br>wiki/22-概念/03-网络/ingress.md（331 行） | |
| 34 | init containers | wiki/17-系统基础/06-知识字典/workloads/init-containers.md（211 行）<br>wiki/22-概念/02-工作负载/init-containers.md（271 行） | |
| 35 | k8s 问题分布与 mttr 基准 | wiki/22-概念/08-可靠性与运维/K8s-故障分布与-MTTR-基准.md（96 行）<br>wiki/22-概念/08-可靠性与运维/k8s-mttr-benchmark.md（281 行） | |
| 36 | kube-proxy | wiki/17-系统基础/06-知识字典/fundamentals/kube-proxy.md（54 行）<br>wiki/23-实体/02-K8s核心组件/kube-proxy.md（137 行） | |
| 37 | kube-scheduler | wiki/22-概念/07-调度与资源/kube-scheduler.md（211 行）<br>wiki/23-实体/02-K8s核心组件/kube-scheduler.md（300 行） | |
| 38 | kubectl | wiki/01-集群基础/05-kubectl/index.md（21 行）<br>wiki/17-系统基础/06-知识字典/tooling/kubectl.md（63 行） | |
| 39 | kubelet | wiki/17-系统基础/06-知识字典/fundamentals/kubelet.md（158 行）<br>wiki/23-实体/02-K8s核心组件/kubelet.md（315 行） | |
| 40 | kubernetes | wiki/17-系统基础/06-知识字典/fundamentals/kubernetes.md（423 行）<br>wiki/22-概念/01-核心架构/kubernetes.md（607 行） | |
| 41 | kubernetes 最佳实践指南 (best-practices) | wiki/20-最佳实践/01-best-practices/README.md（899 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/bp-README-alt.md（890 行） | |
| 42 | kubernetes 最佳实践指南 (skills) | wiki/20-最佳实践/README.md（942 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/bp-README.md（930 行） | |
| 43 | kudig prompt 模板集：故障排查、架构评审、配置生成与学习路径 | wiki/23-实体/15-参考与索引/kudig-prompts-catalog.md（68 行）<br>skills/04-工作负载/pod/方法论/agent/kudig-prompts-catalog.md（107 行） | |
| 44 | networking | wiki/19-故障诊断/04-高级排障/structural-03-networking/index.md（20 行）<br>wiki/26-标签/02-网络与存储/networking.md（184 行）<br>skills/04-工作负载/pod/培训/lecturer/lecturer-03-networking-index.md（11 行）<br>skills/04-工作负载/pod/培训/lecturer/lecturer-04-networking-index.md（11 行） | |
| 45 | networkpolicy | wiki/22-概念/03-网络/network-policy.md（300 行）<br>wiki/23-实体/02-K8s核心组件/networkpolicy.md（284 行） | |
| 46 | nodepool 异常故障树分析 | wiki/19-故障诊断/06-FTA故障树/list/nodepool-fta.md（345 行）<br>skills/03-节点/node/reference/nodepool-fta.md（231 行） | |
| 47 | observability | wiki/09-可观测性/README.md（50 行）<br>wiki/20-最佳实践/01-best-practices/observability/index.md（13 行）<br>wiki/20-最佳实践/05-observability/index.md（13 行）<br>wiki/26-标签/04-可观测性/observability.md（202 行）<br>skills/08-可观测性/monitoring/最佳实践/obs-index-alt.md（13 行）<br>skills/08-可观测性/monitoring/最佳实践/obs-index.md（13 行） | |
| 48 | oncall qa | skills/04-工作负载/pod/培训/lecturer/lecturer-11-oncall-qa-index.md（11 行）<br>skills/04-工作负载/pod/培训/oncall-qa/index.md（11 行） | |
| 49 | operations | wiki/20-最佳实践/01-best-practices/operations/index.md（13 行）<br>wiki/20-最佳实践/06-operations/index.md（14 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/ops-index-alt.md（13 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/ops-index.md（13 行） | |
| 50 | operator pattern (crd + controller) | wiki/17-系统基础/06-知识字典/platform-engineering/operator-pattern.md（99 行）<br>wiki/22-概念/01-核心架构/operator-pattern.md（235 行） | |
| 51 | papers | wiki/21-生态参考/02-论文/index.md（36 行）<br>wiki/26-标签/07-参考与最佳实践/papers.md（70 行） | |
| 52 | pod group policies | wiki/17-系统基础/06-知识字典/workloads/pod-group-policies.md（181 行）<br>skills/04-工作负载/pod/配置与字典/pod-group-policies.md（181 行） | |
| 53 | pod hostname | wiki/17-系统基础/06-知识字典/workloads/pod-hostname.md（233 行）<br>skills/04-工作负载/pod/配置与字典/pod-hostname.md（233 行） | |
| 54 | pod lifecycle (concepts) | wiki/17-系统基础/06-知识字典/workloads/pod-lifecycle.md（97 行）<br>wiki/22-概念/02-工作负载/pod-lifecycle.md（193 行）<br>skills/04-工作负载/pod/概念原理/字典-pod-lifecycle.md（94 行） | |
| 55 | pod overhead | wiki/17-系统基础/06-知识字典/scheduling/pod-overhead.md（143 行）<br>wiki/22-概念/07-调度与资源/pod-overhead.md（265 行）<br>skills/04-工作负载/pod/调度/字典-pod-overhead.md（143 行） | |
| 56 | pod priority and preemption | wiki/17-系统基础/06-知识字典/scheduling/pod-priority-and-preemption.md（195 行）<br>skills/04-工作负载/pod/调度/pod-priority-and-preemption.md（195 行） | |
| 57 | pod quality of service classes | wiki/17-系统基础/06-知识字典/workloads/pod-quality-of-service-classes.md（206 行）<br>skills/04-工作负载/pod/资源与自动扩缩/pod-quality-of-service-classes.md（206 行） | |
| 58 | pod scheduling readiness | wiki/17-系统基础/06-知识字典/scheduling/pod-scheduling-readiness.md（147 行）<br>skills/04-工作负载/pod/调度/pod-scheduling-readiness.md（147 行） | |
| 59 | pod topology spread constraints | wiki/17-系统基础/06-知识字典/scheduling/pod-topology-spread-constraints.md（217 行）<br>skills/04-工作负载/pod/调度/pod-topology-spread-constraints.md（217 行） | |
| 60 | pod 安全标准 | wiki/17-系统基础/06-知识字典/security/pod-security-standards.md（82 行）<br>skills/04-工作负载/pod/安全/字典-pod-security-standards.md（81 行） | |
| 61 | pods | wiki/17-系统基础/06-知识字典/workloads/pods.md（291 行）<br>wiki/22-概念/02-工作负载/pods.md（339 行）<br>skills/04-工作负载/pod/概念原理/字典-pods.md（258 行） | |
| 62 | reference | wiki/19-故障诊断/08-技能体系/skill-set/k8s-autoscaling/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-certificate-expiry/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-config-secret/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-control-plane/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-deployment-rollout/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-dns-failure/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-image-pull/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-ingress-gateway/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-logging-pipeline/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-monitoring-alerting/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/index.md（14 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-performance/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-pod-crashloop/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-rbac-quota/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-security-incident/reference/index.md（11 行）<br>wiki/19-故障诊断/08-技能体系/skill-set/k8s-service-connectivity/reference/index.md（11 行）<br>wiki/26-标签/07-参考与最佳实践/reference.md（226 行） | |
| 63 | replicaset | wiki/17-系统基础/06-知识字典/workloads/replicaset.md（177 行）<br>wiki/22-概念/02-工作负载/replicaset.md（282 行） | |
| 64 | research | wiki/23-实体/01-research/index.md（28 行）<br>wiki/25-研究/index.md（74 行）<br>wiki/26-标签/06-AI与专项/research.md（93 行） | |
| 65 | resource management for pods and contain | wiki/17-系统基础/06-知识字典/configuration/resource-management-for-pods-and-containers.md（264 行）<br>skills/04-工作负载/pod/资源与自动扩缩/resource-management-for-pods-and-containers.md（264 行） | |
| 66 | scenarios | wiki/19-故障诊断/08-技能体系/skills-run/scenarios/index.md（11 行）<br>wiki/20-最佳实践/07-scenarios/index.md（36 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/scen-index.md（35 行） | |
| 67 | secrets | wiki/17-系统基础/06-知识字典/configuration/secrets.md（235 行）<br>wiki/22-概念/05-安全/secrets.md（281 行） | |
| 68 | security | wiki/20-最佳实践/01-best-practices/security/index.md（13 行）<br>wiki/20-最佳实践/08-security/index.md（13 行）<br>wiki/26-标签/03-安全与合规/security.md（204 行）<br>skills/07-安全/rbac/最佳实践/sec-index-alt.md（13 行）<br>skills/07-安全/rbac/最佳实践/sec-index.md（13 行） | |
| 69 | security & compliance open source projec | wiki/08-安全/00-总览/00-open-source-projects-index.md（319 行）<br>wiki/09-可观测性/00-总览/00-open-source-projects-index.md（62 行） | |
| 70 | service | wiki/17-系统基础/06-知识字典/networking/service.md（303 行）<br>wiki/22-概念/03-网络/service.md（344 行） | |
| 71 | sidecar containers | wiki/17-系统基础/06-知识字典/workloads/sidecar-containers.md（178 行）<br>wiki/22-概念/02-工作负载/sidecar-containers.md（270 行） | |
| 72 | software supply chain security | wiki/17-系统基础/06-知识字典/security/supply-chain-security.md（97 行）<br>wiki/22-概念/05-安全/supply-chain-security.md（234 行） | |
| 73 | statefulset | wiki/22-概念/02-工作负载/statefulset.md（331 行）<br>wiki/23-实体/02-K8s核心组件/statefulset.md（299 行） | |
| 74 | storage | wiki/19-故障诊断/04-高级排障/structural-04-storage/index.md（15 行）<br>wiki/26-标签/02-网络与存储/storage.md（204 行）<br>skills/04-工作负载/pod/培训/lecturer/lecturer-07-storage-index.md（11 行） | |
| 75 | terway 全项目资源索引 | wiki/05-网络/06-Terway/terway-index.md（335 行）<br>wiki/21-生态参考/03-领域索引/terway-index.md（224 行） | |
| 76 | topic-best-practices moc (skills) | wiki/20-最佳实践/MOC.md（119 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/bp-MOC.md（90 行） | |
| 77 | topic-scenarios moc | wiki/20-最佳实践/07-scenarios/MOC.md（132 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/scen-MOC.md（100 行） | |
| 78 | trivy | wiki/17-系统基础/06-知识字典/security/trivy.md（211 行）<br>wiki/23-实体/06-安全/trivy.md（278 行） | |
| 79 | troubleshooting | wiki/26-标签/04-可观测性/troubleshooting.md（211 行）<br>skills/04-工作负载/pod/培训/lecturer/lecturer-09-troubleshooting-index.md（12 行）<br>skills/04-工作负载/pod/培训/troubleshooting/index.md（11 行） | |
| 80 | vertical pod autoscaling | wiki/17-系统基础/06-知识字典/workloads/vertical-pod-autoscaling.md（231 行）<br>skills/04-工作负载/pod/资源与自动扩缩/vertical-pod-autoscaling.md（231 行） | |
| 81 | week 2 security monitoring | skills/04-工作负载/pod/培训/inner-training/week-2-security-monitoring/index.md（22 行）<br>skills/07-安全/rbac/培训/week2-security-monitoring-index.md（14 行） | |
| 82 | week 3 node workload | skills/04-工作负载/pod/培训/inner-training/week-3-node-workload/index.md（22 行）<br>skills/04-工作负载/pod/培训/week3-node-workload-index.md（17 行） | |
| 83 | week 4 network storage | skills/04-工作负载/pod/培训/inner-training/week-4-network-storage/index.md（22 行）<br>skills/04-工作负载/pod/培训/week4-network-storage-index.md（17 行） | |
| 84 | workload api | wiki/17-系统基础/06-知识字典/workloads/workload-api.md（177 行）<br>wiki/22-概念/02-工作负载/workload-api.md（276 行） | |
| 85 | workloads | wiki/19-故障诊断/04-高级排障/structural-05-workloads/index.md（16 行）<br>skills/04-工作负载/pod/培训/lecturer/lecturer-10-workloads-index.md（11 行） | |
| 86 | 命名空间 | wiki/17-系统基础/06-知识字典/fundamentals/namespace.md（45 行）<br>wiki/17-系统基础/06-知识字典/fundamentals/namespaces.md（63 行） | |
| 87 | 控制器管理器 | wiki/17-系统基础/06-知识字典/fundamentals/controller-manager.md（58 行）<br>wiki/17-系统基础/06-知识字典/fundamentals/kube-controller-manager.md（57 行） | |
| 88 | 服务账号 | wiki/17-系统基础/06-知识字典/security/service-account.md（207 行）<br>wiki/17-系统基础/06-知识字典/security/service-accounts.md（110 行） | |
| 89 | 生产场景导航 | wiki/20-最佳实践/07-scenarios/README.md（123 行）<br>skills/01-集群运维/cluster-upgrade/最佳实践/scen-README.md（85 行） | |
| 90 | 端点 | wiki/17-系统基础/06-知识字典/networking/endpoint.md（181 行）<br>wiki/17-系统基础/06-知识字典/networking/endpoints.md（179 行） | |
| 91 | 网络策略 | wiki/17-系统基础/06-知识字典/networking/network-policy.md（196 行）<br>wiki/17-系统基础/06-知识字典/networking/networkpolicy.md（219 行） | |
| 92 | 调度器 | wiki/17-系统基础/06-知识字典/fundamentals/kube-scheduler.md（61 行）<br>wiki/17-系统基础/06-知识字典/scheduling/scheduler.md（63 行） | |
| 93 | 资源配额 | wiki/17-系统基础/06-知识字典/configuration/resource-quota.md（45 行）<br>wiki/17-系统基础/06-知识字典/scheduling/resource-quota.md（70 行） | |
| 94 | 限制范围 | wiki/17-系统基础/06-知识字典/configuration/limit-range.md（45 行）<br>wiki/17-系统基础/06-知识字典/scheduling/limitrange.md（76 行） | |

