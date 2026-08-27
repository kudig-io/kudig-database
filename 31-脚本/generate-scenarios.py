#!/usr/bin/env python3
"""
KUDIG-DATABASE 运维场景工单剧本生成器

将 20 个运维场景生成为「工单剧本」(Playbook)，输出到 13-生产运维/08-运维场景剧本/。

每个剧本包含: 触发条件 / 前置检查 / 快速决策树(mermaid) / 工作流分支 /
完工验证清单 / 常见陷阱 / 升级路径 / 资源编排(doc/fta/skill/case 关联)。

层次定位: domain 讲原理, 技能卡给动作, FTA 管推导 —— 本层把它们按真实场景串成工作流。

用法:
    python3 31-脚本/generate-scenarios.py               # 校验引用并生成
    python3 31-脚本/generate-scenarios.py --check-only  # 仅校验链接, 不写文件

链接纪律: 所有 doc:/fta:/skill:/case:/scenario: 引用在生成前解析为真实路径,
缺失即报错(exit 2), 防止死链入库。
"""

import argparse
import sys
from datetime import date
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent
OUT_DIR = BASE_DIR / "13-生产运维" / "08-运维场景剧本"

FTA_DIR = "19-故障诊断/06-FTA故障树/list"
SKILL_DIR = "19-故障诊断/08-技能体系"
CASE_DIR = "13-生产运维/05-工单案例"

TODAY = date.today().isoformat()
YEAR_MONTH = TODAY[:7]


# ---------------------------------------------------------------------------
# 引用解析与渲染
# ---------------------------------------------------------------------------

def split_ref(ref: str):
    body, _, label = ref.partition("|")
    rtype, _, value = body.partition(":")
    return rtype.strip(), value.strip(), label.strip()


def resolve_ref(ref: str):
    """'type:value[|label]' -> (absolute_path or None-if-in-batch, label, in_batch)."""
    rtype, value, label = split_ref(ref)
    if rtype == "doc":
        path = BASE_DIR / value
        default_label = Path(value).stem.replace("-", " ")
    elif rtype == "fta":
        path = BASE_DIR / FTA_DIR / value
        default_label = f"FTA · {value.removesuffix('-fta.md')}"
    elif rtype == "skill":
        path = BASE_DIR / SKILL_DIR / value
        num, _, rest = value.partition("-")
        default_label = f"{num} · {rest.removesuffix('.md').replace('-', ' ')}"
    elif rtype == "case":
        path = BASE_DIR / CASE_DIR / value
        default_label = "案例 · " + value.removeprefix("ticket-case-").removesuffix(".md")
    elif rtype == "scenario":
        return None, (label or value.removesuffix(".md")), True
    else:
        raise ValueError(f"未知引用类型: {ref!r}")
    if not label:
        label = default_label
    return path.resolve(), label.strip(), False


def ref_link(ref: str, missing: list, owner: str) -> str:
    """渲染为 Obsidian wiki link 并登记缺失。"""
    path, label, _ = resolve_ref(ref)
    if path is not None and not path.exists():
        missing.append((owner, ref))
        return f"`BROKEN:{ref}`"
    if path is not None:
        rel = path.relative_to(BASE_DIR).as_posix()
        return f"[[{rel}|{label}]]"
    # scenario 互引: 指向本批次产物
    _, value, _ = split_ref(ref)
    target = OUT_DIR / value
    rel = target.relative_to(BASE_DIR).as_posix().removesuffix(".md")
    return f"[[{rel}|{label}]]"


def parse_step(item):
    """步骤项 -> (text, refs:list[str]); 兼容 str / (text, ref) / (text, ref, ref, ...)"""
    if isinstance(item, str):
        return item, []
    return item[0], list(item[1:])


def render_steps(steps, missing, owner, indent=""):
    out = []
    for i, item in enumerate(steps, 1):
        text, refs = parse_step(item)
        head = f"{indent}{i}. {text}"
        if refs:
            links = "、".join(ref_link(r, missing, owner) for r in refs)
            head += f" → {links}"
        out.append(head)
    return "\n".join(out)


def shorten(text: str, n: int = 14) -> str:
    return text if len(text) <= n else text[: n - 1] + "…"


def build_mermaid(s: dict) -> str:
    lines = ["```mermaid", "graph TD"]
    lines.append(f'ROOT["{s["id"]} {s["title"]}"]')
    lines.append('PC{"前置检查"}')
    lines.append("ROOT --> PC")
    lines.append('PC -->|"通过"| EXEC["按分支执行"]')
    lines.append('PC -.->|"不满足"| ABORT["补齐条件后再进入"]')
    for i, br in enumerate(s.get("branches", []), 1):
        bid = f"BR{i}"
        lines.append(f'{bid}["{shorten(br["name"], 30)}"]')
        lines.append(f'ROOT -->|"{br["cond"]}"| {bid}')
        lines.append(f"{bid} --> EXEC")
    lines.append('VAL{"完工验证清单"}')
    lines.append("EXEC --> VAL")
    lines.append('VAL -->|"全绿"| DONE["场景关闭"]')
    lines.append('VAL -.->|"未达标"| ESCAL["走升级路径"]')
    lines.append("style ROOT fill:#ef4444,stroke:#b91c1c,color:#fff")
    lines.append("style DONE fill:#22c55e,stroke:#166534,color:#fff")
    lines.append("style ESCAL fill:#f59e0b,stroke:#b45309,color:#fff")
    lines.append("```")
    return "\n".join(lines)


# ===========================================================================
# 场景数据（工单剧本单一事实源）
# 所有引用必须是仓库内真实文件，脚本启动即全量校验。
# ===========================================================================

SCENARIOS = [
# ---------------------------------------------------------------- SC-01
{
"id": "SC-01", "name": "cluster-deployment",
"title": "集群部署", "title_en": "Cluster Deployment",
"group": "建设与交付", "read_time": "10min",
"description": "Kubernetes 集群从 0 到 1 的建设交付剧本：模式选型、部署执行、交付验收。",
"trigger_keywords": ["集群部署", "新建集群", "kubeadm init", "托管集群创建"],
"intent_queries": ["如何从零部署一个生产可用的 Kubernetes 集群", "新建 K8s 集群的验收清单有哪些"],
"primary_tag": "deployment",
"overview": (
    "覆盖托管云（ACK/EKS）、自建（kubeadm/sealos）与多集群纳管三条建设路径的标准化交付流程，"
    "串联网络方案选型、控制面高可用、证书体系与交付验收检查点，产出可直接承载生产的集群。"),
"triggers": [
    "新业务上线需要独立集群",
    "灾难后集群重建或机房迁移",
    "测试/预发环境扩容新集群",
    "旧集群退役前的替换建设",
],
"pre_checks": [
    "确认部署模式：托管服务 vs 自建 vs 混合（决定后续路径选择）",
    "确认版本策略：目标版本及 n-1 升级兼容窗口",
    "确认网络方案：CNI 选型与云厂商 VPC 路由模型（Terway/Calico/Cilium）",
    "确认容量基线：节点规格、可用区分布、控制面副本数（≥3）",
    ("确认周边依赖就绪：镜像仓库、DNS、存储后端、监控接入点",
     "doc:18-云厂商/README.md|云厂商集成要点"),
],
"branches": [
    {"cond": "云托管", "name": "路径 A · 云托管（ACK/EKS 等）",
     "steps": [
         "通过 IaC 或控制台创建集群，启用多可用区与受管控制面",
         ("按云厂商网络模型配置 CNI，预先评估 ENI/IP 配额上限",
          "case:ticket-case-001-terway-eni-exhaustion.md|Terway ENI 耗尽致 NotReady"),
         "配置节点池：系统盘/数据盘分离、标签污点规划、自动伸缩策略",
         ("对照云厂商故障树做预防性巡检", "fta:cloud-provider-fta.md"),
     ]},
    {"cond": "自建", "name": "路径 B · 自建（kubeadm/sealos）",
     "steps": [
         "初始化控制面 HA（堆叠 etcd ≥3 节点），记录 join 命令与 PKI 输出",
         ("安装 CNI 与 CoreDNS，验证跨节点 Pod 互通", "fta:cni-fta.md"),
         "加入 worker 节点并打标签/污点，锁定 kubelet 版本",
         ("排查部署期常见失败：证书、kubelet 启动、镜像预拉取", "fta:kubeadm-fta.md"),
     ]},
    {"cond": "纳入舰队", "name": "路径 C · 纳入既有多集群舰队",
     "steps": [
         ("明确纳管平面（注册中心/下发面），统一凭证与审计入口",
          "doc:13-生产运维/07-运维手册/05-multi-cluster-operations.md|多集群运维手册"),
         "打通网络边界：VPN/专线/东西向网关，验证跨集群 Service 可达性",
     ]},
],
"validation": [
    "所有节点 Ready 且无异常事件（kubectl get nodes / events）",
    "核心组件（apiserver/etcd/scheduler/cm）健康且副本数符合预期",
    "示例 Pod 跨节点调度成功，Service/DNS/PV 全链路可用",
    "备份机制已激活（etcd 快照定时任务 + 首份快照异地落盘）",
    "监控采集、告警路由、On-Call 排班均已接入",
],
"pitfalls": [
    "证书有效期使用默认值未延长，一年后爆发性过期",
    ("CNI 与云 VPC 路由冲突导致偶发丢包难复现",
     "case:ticket-case-001-terway-eni-exhaustion.md"),
    "控制面单副本先上线、HA 计划『以后再补』——然后没有了以后",
    "交付只验功能不验容量，上线首周即触发 Eviction",
],
"escalation": [
    ("部署阻塞控制面健康问题超过 4 小时", "升级至平台架构组并冻结其他变更"),
    ("涉及云厂商配额/底层限制", "提云厂商工单并附 FTA 定位证据链"),
],
"resources_docs": [
    "doc:01-集群基础/README.md|集群基础",
    "doc:13-生产运维/00-总览/01-production-readiness-operations-guide.md|生产就绪运营指南",
],
"resources_ftas": ["apiserver-fta.md", "etcd-fta.md", "node-fta.md"],
"resources_skills": ["12-control-plane-failure.md", "06-certificate-expiry.md"],
"related": [
    "scenario:upgrade-migration.md|SC-08 升级迁移",
    "scenario:daily-ops.md|SC-09 日常巡检",
    "scenario:multi-cluster.md|SC-17 多集群管理",
],
},
# ---------------------------------------------------------------- SC-02
{
"id": "SC-02", "name": "app-deployment",
"title": "应用发布", "title_en": "Application Deployment",
"group": "建设与交付", "read_time": "9min",
"description": "应用在 Kubernetes 上的标准发布/回滚剧本：发布前检查、分型执行、发布验证。",
"trigger_keywords": ["应用部署", "滚动更新", "rollout 卡住", "发布回滚"],
"intent_queries": ["如何在 Kubernetes 上安全地发布一个应用", "Deployment 滚动更新卡住怎么办"],
"primary_tag": "deployment",
"overview": (
    "覆盖无状态/有状态/守护/任务四类工作负载的标准发布动作，"
    "以探针、资源声明、配置分离三大前置检查挡住大部分发布事故。"),
"triggers": [
    "新应用首次上集群",
    "常规版本迭代/热修复发布",
    "批量重发布（镜像轮换、Secret 轮转）",
],
"pre_checks": [
    "资源声明齐全：requests/limits 与压测数据一致",
    ("镜像可用性与拉取凭证核对（留意内网域名超时历史）",
     "case:ticket-case-006-image-pull-acr-timeout.md|ACR 拉取超时"),
    ("重型应用的 OOM 参数与磁盘 IO 压力预案", "case:ticket-case-002-java-oom-essd-iohang.md|Java OOM + IO Hang"),
    "配置分离：ConfigMap/Secret 变更有版本化与生效策略",
    "探针三件套合理：liveness 不做重依赖检查、readiness 反映真实可用、startup 兜底慢启动",
],
"branches": [
    {"cond": "无状态应用", "name": "A · Deployment 发布",
     "steps": [
         "maxUnavailable/maxSurge 与 PDB 匹配，滚动期容量不减半",
         ("观察 rollout status 与 ReplicaSet 代际演进", "skill:09-deployment-rollout-failure.md"),
         ("副本不齐/状态异常时按症候分流排查", "skill:02-pod-crashloop-oomkilled.md"),
     ]},
    {"cond": "有状态应用", "name": "B · StatefulSet 发布",
     "steps": [
         "发布前校验 PVC 绑定状态与 storageClass 变更风险",
         ("更新中断时优先排查卷绑定类根因",
          "skill:23-statefulset-failure.md",
          "case:ticket-case-028-statefulset-pvc-unbound.md|PVC 未绑定"),
     ]},
    {"cond": "守护进程", "name": "C · DaemonSet 发布",
     "steps": [
         ("覆盖率不足时按节点调度链路排查",
          "skill:24-daemonset-failure.md",
          "case:ticket-case-025-daemonset-not-running-on-all-nodes.md|DS 未全覆盖"),
     ]},
    {"cond": "定时任务", "name": "D · Job/CronJob 发布",
     "steps": [
         ("关注 concurrencyPolicy 与 startingDeadlineSeconds 语义",
          "skill:25-job-cronjob-failure.md",
          "case:ticket-case-034-cronjob-stuck-job-skipped-schedule.md|CronJob 卡住跳调度"),
     ]},
],
"validation": [
    "全部副本 Ready，Endpoints 数量与期望一致",
    "灰度流量抽测通过：业务接口 + 日志无异常栈",
    "HPA/监控面板无毛刺，错误率回到基线",
    "保留上一版 RS/镜像 tag 以便快速回滚",
],
"pitfalls": [
    "镜像 tag 使用 latest 导致回滚语义失效",
    "terminationGracePeriodSeconds=0 引发连接复位闪断",
    "ConfigMap 依赖 rollout 重启生效但变更策略配成了忽略",
    "同命名空间多个部署同时滚动放大抖动",
],
"escalation": [
    ("发布验证连续两次未通过", "冻结该批次发布并回滚至上一稳定版"),
    ("疑似平台层问题（apiserver/webhook）", "转 SC-03 故障排查总纲"),
],
"resources_docs": [
    "doc:02-工作负载/README.md|工作负载域",
    "doc:03-清单模式/README.md|清单模式规范",
],
"resources_ftas": ["pod-fta.md", "deployment-fta.md", "statefulset-fta.md", "pdb-fta.md"],
"resources_skills": ["02-pod-crashloop-oomkilled.md", "08-pvc-storage-failure.md", "15-configmap-secret-failure.md"],
"related": [
    "scenario:gitops-workflow.md|SC-15 GitOps 工作流",
    "scenario:daily-ops.md|SC-09 日常巡检",
],
},
# ---------------------------------------------------------------- SC-03 (标杆)
{
"id": "SC-03", "name": "troubleshooting",
"title": "故障排查总纲", "title_en": "Troubleshooting Master Playbook",
"group": "稳定性保障", "read_time": "15min",
"description": "系统化故障排查总纲：从告警触发到根因闭环的五阶段方法论与组件级分流索引。",
"trigger_keywords": ["故障排查", "线上告警", "根因分析", "oncall 定级"],
"intent_queries": ["收到线上告警后如何系统性排查 K8s 问题", "如何快速给故障定级并圈定爆炸半径"],
"primary_tag": "troubleshooting",
"overview": (
    "一切专项排查的入口剧本。以『信息收集 → 影响评估 → 快速止血 → 根因定位 → 修复验证』五阶段为主线，"
    "阶段四按组件分流到专项剧本与 FTA 故障树。铁律：先取证再动手，避免无序重启销毁现场。"),
"triggers": [
    "任何 P0/P1 生产告警触发（节点/工作负载/网络/存储/控制面）",
    "用户报障：访问失败、性能骤降、功能异常",
    "巡检发现持续劣化的异常趋势",
],
"pre_checks": [
    "确认真伪与持续性：排除误报，等待至少一个完整指标周期",
    ("按升级矩阵完成初步定级（P0~P3）",
     "doc:13-生产运维/03-事件响应/01-escalation-matrix-severity-levels.md|事件升级矩阵"),
    "影响半径速记：节点数 / 工作负载 / 用户面 SLO 影响",
    "近 1 小时变更清单：发布、配置、扩缩容、证书、内核操作",
    ("P0 即刻拉起 War Room", "doc:13-生产运维/03-事件响应/02-war-room-coordination-procedures.md|War Room 协调规程"),
],
"branches": [
    {"cond": "现象广泛或来源不明", "name": "阶段 1-2 · 信息收集与定级（≤5 分钟）",
     "steps": [
         "kubectl get events --sort-by=.lastTimestamp 抓 Warning 波形",
         "kubectl top nodes / top pods --all-namespaces 找资源热点",
         ("将异常时间线与近期变更记录对齐", "doc:11-发布变更/04-变更管理/index.md|变更管理索引"),
         ("向干系人发出首份通报（模板化表达）",
          "doc:13-生产运维/03-事件响应/03-communication-templates-stakeholder.md|干系人沟通模板"),
     ]},
    {"cond": "影响持续扩大", "name": "阶段 3 · 快速止血（先恢复后定位）",
     "steps": [
         "决策优先级：切流降级 > 回滚最近变更 > 隔离故障单元 > 受控重启",
         "止血动作必须先截图留证再执行，防止现场销毁",
         ("操作前对照变更冻结窗口约束",
          "doc:13-生产运维/07-运维手册/07-change-freeze-policy.md|变更冻结策略"),
     ]},
    {"cond": "节点侧异常", "name": "分流 A · 节点",
     "steps": [
         ("NotReady/资源压力按卡片处置", "skill:01-node-notready.md", "skill:20-node-resource-pressure.md"),
         ("深入物理面根因用 FTA 推导", "fta:kubelet-fta.md", "fta:containerd-fta.md"),
     ]},
    {"cond": "工作负载侧异常", "name": "分流 B · Pod/控制器",
     "steps": [
         ("Pending/CrashLoop/OOM 三大症候对症下药", "skill:03-pod-pending.md", "skill:02-pod-crashloop-oomkilled.md"),
         ("疑难杂症按 Pod 创建端到端链路追踪", "fta:pod-creation-end-to-end-fta.md", "fta:scheduler-fta.md"),
     ]},
    {"cond": "网络侧异常", "name": "分流 C · 网络",
     "steps": [
         ("四跳法快速分层：DNS→Service→Ingress→Policy", "skill:04-dns-resolution-failure.md", "skill:05-service-connectivity.md"),
         ("复杂链路转入网络诊断专项剧本", "scenario:network-diagnosis.md|SC-11 网络诊断"),
     ]},
    {"cond": "存储侧异常", "name": "分流 D · 存储",
     "steps": [
         ("挂卷/Pending PVC 先看事件与 CSI 组件健康", "skill:08-pvc-storage-failure.md", "fta:csi-fta.md"),
         ("转专项剧本深挖", "scenario:storage-issues.md|SC-12 存储问题"),
     ]},
    {"cond": "控制面侧异常", "name": "分流 E · 控制平面",
     "steps": [
         ("首要保全证据：etcd 快照 + 日志归档，再谈修复",
          "skill:12-control-plane-failure.md", "fta:etcd-fta.md", "fta:apiserver-fta.md"),
         ("配额与权限类误判常被误认为控制面故障", "skill:10-rbac-quota-failure.md"),
     ]},
],
"validation": [
    "核心指标回到 7 天基线带宽内且持续 ≥2 个周期",
    "受影响业务接口成功率/延迟达标，无衍生告警",
    "止血手段副作用已评估（临时资源的回收计划明确）",
    ("48h 内产出复盘 RCA，结论回写知识库与 FTA",
     "doc:13-生产运维/03-事件响应/index.md|事件响应手册集"),
],
"pitfalls": [
    "没有留证就重启组件——根因永久丢失，只能靠猜",
    "只在容器内找问题，忽略节点级 DNS/IO/时钟漂移",
    "高峰期执行回滚引发二次事故，违背先扩容再变更的原则",
    "把症状当根因关单：重启恢复 ≠ 排查完成",
],
"escalation": [
    ("15 分钟内无有效止血路径的 P0", "升级值班经理并拉起 War Room"),
    ("疑似云产品侧故障", "提云厂商工单并同步初步证据包"),
],
"resources_docs": [
    "doc:19-故障诊断/README.md|故障诊断域",
    "doc:13-生产运维/03-事件响应/04-on-call-playbook.md|On-Call 手册",
    "doc:13-生产运维/07-运维手册/09-observability-operations.md|可观测性运营",
    "doc:13-生产运维/05-工单案例/ticket-routing-rules.md|工单路由规则",
],
"resources_ftas": ["pod-fta.md", "node-fta.md", "etcd-fta.md", "dns-fta.md", "csi-fta.md"],
"resources_skills": ["01-node-notready.md", "03-pod-pending.md", "04-dns-resolution-failure.md",
                     "11-image-pull-failure.md", "18-performance-bottleneck.md"],
"related": [
    "scenario:network-diagnosis.md|SC-11 网络诊断",
    "scenario:storage-issues.md|SC-12 存储问题",
    "scenario:performance-tuning.md|SC-04 性能调优",
    "scenario:security-incident.md|SC-13 安全事件响应",
],
},
# ---------------------------------------------------------------- SC-04
{
"id": "SC-04", "name": "performance-tuning",
"title": "性能调优", "title_en": "Performance Tuning",
"group": "稳定性保障", "read_time": "9min",
"description": "分层性能优化剧本：应用→容器→节点→控制面的瓶颈定位与调参纪律。",
"trigger_keywords": ["性能调优", "延迟升高", "CPU 打满", "性能瓶颈"],
"intent_queries": ["K8s 集群响应变慢如何分层定位性能瓶颈", "性能调优应该遵守什么纪律"],
"primary_tag": "performance",
"overview": "自上而下四层漏斗定位瓶颈，强调『先测量、再调整、可回退』的调参纪律，防止资源堆砌掩盖真因。",
"triggers": [
    "接口延迟突增 / 吞吐下滑逼近 SLO 缓冲带",
    "节点 CPU/内存/磁盘水位持续高于 80%",
    "调度延迟变大、Pod 启动时长上升",
],
"pre_checks": [
    "固定基线：与上周同期对比而非绝对值",
    "排除外部因素：下游依赖、流量结构变化、大促日程",
    "锁定观测窗口与采样精度（警惕均值掩盖长尾毛刺）",
],
"branches": [
    {"cond": "应用表现劣化", "name": "A · 应用与容器层",
     "steps": [
         ("profiling 对火焰图，重点看锁竞争与 GC 占比",
          "case:ticket-case-002-java-oom-essd-iohang.md|Java OOM + IO Hang"),
         "requests/limits 与实际峰值匹配度复盘，纠正失真声明",
     ]},
    {"cond": "节点指标异常", "name": "B · 节点与运行时层",
     "steps": [
         ("磁盘压力是隐形杀手：iostat/dmesg 双确认",
          "skill:20-node-resource-pressure.md",
          "case:ticket-case-014-node-disk-pressure.md|节点磁盘压力"),
         ("runtime 层日志与镜像占盘治理", "fta:containerd-fta.md"),
     ]},
    {"cond": "网络吞吐受限", "name": "C · 网络链路层",
     "steps": [
         ("conntrack 表饱和与 SNAT 端口耗尽检查", "fta:kube-proxy-fta.md"),
         ("CNI 数据面实现差异确认（eBPF vs iptables）", "fta:cilium-fta.md"),
     ]},
    {"cond": "API 响应迟缓", "name": "D · 控制平面层",
     "steps": [
         ("apiserver QPS/延迟与 etcd fsync 时延画像", "fta:apiserver-fta.md", "fta:etcd-fta.md"),
         ("弹性组件失灵也会伪装成性能问题", "skill:13-autoscaling-failure.md", "fta:hpa-fta.md"),
     ]},
],
"validation": [
    "目标指标改善达到预期且无新瓶颈转移（瓶颈不会消失只会搬家）",
    "所有参数变更记录于变更单并可一键回退",
    "压测复演一轮确认稳定性",
],
"pitfalls": [
    "盲目上调 requests 反而压缩可调度容量引发 Pending",
    "只盯 CPU 忽略 IO/网络等待占比",
    "同一时段叠加多项变更导致无法归因",
],
"escalation": [("调优涉及内核参数或发行版配置", "交由系统组评审窗口统一执行")],
"resources_docs": [
    "doc:13-生产运维/07-运维手册/09-observability-operations.md|可观测性运营",
    "doc:17-系统基础/README.md|系统基础(Linux)",
],
"resources_ftas": ["hpa-fta.md", "vpa-fta.md", "node-fta.md"],
"resources_skills": ["18-performance-bottleneck.md", "13-autoscaling-failure.md", "20-node-resource-pressure.md"],
"related": [
    "scenario:capacity-planning.md|SC-14 容量规划",
    "scenario:cost-optimization.md|SC-19 成本优化",
    "scenario:troubleshooting.md|SC-03 故障排查总纲",
],
},
# ---------------------------------------------------------------- SC-05
{
"id": "SC-05", "name": "security-hardening",
"title": "安全加固", "title_en": "Security Hardening",
"group": "安全合规", "read_time": "10min",
"description": "全生命周期安全基线建设剧本：认证授权、Pod 安全、网络隔离、密钥与供应链。",
"trigger_keywords": ["安全加固", "RBAC 最小权限", "NetworkPolicy 默认拒绝", "Pod 安全准入"],
"intent_queries": ["生产 K8s 集群需要哪些必做的安全加固项", "如何在不影响业务的情况下收紧 RBAC"],
"primary_tag": "security",
"overview": "以 CIS 映射为基线、以 audit→warn→enforce 渐进式落地为纪律，防止『一收紧就断服务』的事故型加固。",
"triggers": [
    "新集群上线前基线加固",
    "审计整改项/渗透测试修复",
    "等保与行业合规倒排工期",
],
"pre_checks": [
    "运行 CIS Benchmark 扫描获得差距清单",
    "业务方填写豁免申请（哪些服务依赖宽松权限及其原因）",
    "选定策略引擎与实施轨道（audit → warn → enforce 三段式）",
],
"branches": [
    {"cond": "权限收口", "name": "A · 认证与授权收敛",
     "steps": [
         ("RBAC 走聚合角色与准入复核，禁用 cluster-admin 泛授",
          "skill:10-rbac-quota-failure.md",
          "case:ticket-case-039-rbac-api-access-denied.md|RBAC AccessDenied 案例"),
     ]},
    {"cond": "负载基线", "name": "B · Pod 安全基线",
     "steps": [
         "namespace 按 PSA 等级分级落地（privileged/restricted）",
         ("特权容器与 hostPath 的例外审批流程固化", "fta:psp-scc-fta.md"),
     ]},
    {"cond": "边界隔离", "name": "C · 网络隔离",
     "steps": [
         ("default-deny 后白名单放行，切记预留 DNS 通路",
          "skill:22-networkpolicy-connectivity.md",
          "case:ticket-case-010-networkpolicy-blocks-traffic.md|Policy 断流案例"),
         ("复杂拓扑查询故障树对照", "fta:networkpolicy-fta.md"),
     ]},
    {"cond": "密钥与来源", "name": "D · 密钥管理与供应链",
     "steps": [
         ("etcd Secret 静态加密 + 外部 KMS，凭据定期轮转",
          "skill:15-configmap-secret-failure.md"),
         "镜像签名验证与准入扫描门禁（unsigned 一律 deny）",
     ]},
],
"validation": [
    "CIS 通过率达到设定目标且 audit 日志零误杀申诉",
    "红队抽查横向移动路径较加固前收窄可量化",
    "所有 enforce 级策略具备一键降级的操作文档",
],
"pitfalls": [
    "RBAC 收紧遗漏 CI/CD 服务账号，流水线半夜集体阵亡",
    "NetworkPolicy 只测 pod-to-pod 忘了 egress DNS/元数据端点",
    "跳过 audit 直奔 enforce，事后说不清影响了谁",
],
"escalation": [("涉及生产关键链路的强策略", "变更委员会评审并设一周观察期")],
"resources_docs": [
    "doc:08-安全/README.md|安全域",
    "doc:13-生产运维/02-集群治理/04-rbac-governance-model.md|RBAC 治理模型",
    "doc:13-生产运维/02-集群治理/03-admission-policy-governance.md|准入策略治理",
],
"resources_ftas": ["rbac-fta.md", "networkpolicy-fta.md", "certificate-fta.md"],
"resources_skills": ["10-rbac-quota-failure.md", "22-networkpolicy-connectivity.md", "26-namespace-quota-limitrange.md"],
"related": [
    "scenario:compliance-audit.md|SC-20 合规审计",
    "scenario:security-incident.md|SC-13 安全事件响应",
],
},
# ---------------------------------------------------------------- SC-06
{
"id": "SC-06", "name": "monitoring-alerting",
"title": "监控告警体系", "title_en": "Monitoring & Alerting",
"group": "稳定性保障", "read_time": "9min",
"description": "监控体系搭建与告警质量治理剧本：覆盖率建设、分级路由、风暴抑制与有效性演练。",
"trigger_keywords": ["监控告警", "Prometheus 告警规则", "告警风暴", "SLO 告警"],
"intent_queries": ["如何搭建一套不会让人麻木的监控告警体系", "告警太多应该如何治理"],
"primary_tag": "monitoring",
"overview": "先保『看得见』再治『看得清』：四级指标金字塔（infra→middleware→app→biz）＋三级告警分级＋例行有效性演练。",
"triggers": [
    "新业务/新集群接入监控",
    "无效告警占比超标（>30%）或爆发告警风暴",
    "SLO 周期评审发现覆盖盲区",
],
"pre_checks": [
    "盘点监控对象：绘制 infra→middleware→app→biz 四级资源层级图",
    ("对齐既有可观测运营基线（采集/存储/查询三层健康度）",
     "doc:13-生产运维/07-运维手册/09-observability-operations.md|可观测性运营手册"),
    "评估告警消费能力：值班者每小时可处理条数上限",
],
"branches": [
    {"cond": "体系建设期", "name": "A · 采集与视图建设",
     "steps": [
         ("黑盒+白盒双轨接入，exporter 清单化管理", "fta:monitoring-fta.md"),
         "核心大盘满足一屏定级：RED（业务）+ USE（资源）双视角",
     ]},
    {"cond": "告警质量治理期", "name": "B · 告警质量治理",
     "steps": [
         "每条规则回答四问：谁看/何时看/做什么动作/多久必须看",
         ("监控自身数据面故障要有自愈与降级方案",
          "skill:16-monitoring-alerting-failure.md",
          "case:ticket-case-015-prometheus-data-loss-slow-query.md|Prometheus 数据丢失"),
         "抑制规则成对维护：主告警自动抑制其衍生告警",
     ]},
    {"cond": "常态演练期", "name": "C · 有效性演练",
     "steps": [
         "季度注入式故障演练验证告警触达与文案可操作性",
         "SLO 多窗口燃烧率规则的召回/误报复盘",
     ]},
],
"validation": [
    "关键链路覆盖率 100%（每条告警可映射到组件矩阵）",
    "端到端触达实测 <2 分钟，夜间无效告警为零",
    "大盘-告警-Runbook 三者链接闭合可互跳",
],
"pitfalls": [
    "阈值拍脑袋设置，低于容量红线才预警毫无意义",
    "只有全局大盘，没有按业务 Owner 的视图归属",
    "静默全靠手工，缺少与变更窗口联动的自动静默",
],
"escalation": [("监控平台自身故障期间", "启用降级采集通道并将人工巡检频次翻倍")],
"resources_docs": [
    "doc:09-可观测性/README.md|可观测性域",
    "doc:13-生产运维/07-运维手册/06-sla-slo-definition-templates.md|SLA/SLO 模板",
],
"resources_ftas": ["monitoring-fta.md"],
"resources_skills": ["16-monitoring-alerting-failure.md", "17-logging-pipeline-failure.md"],
"related": [
    "scenario:daily-ops.md|SC-09 日常巡检",
    "scenario:troubleshooting.md|SC-03 故障排查总纲",
],
},
# ---------------------------------------------------------------- SC-07
{
"id": "SC-07", "name": "backup-restore",
"title": "备份恢复", "title_en": "Backup & Restore",
"group": "可靠性韧性", "read_time": "9min",
"description": "etcd/集群配置/持久数据的备份恢复与灾备演练剧本，对齐 RPO/RTO 目标。",
"trigger_keywords": ["备份恢复", "etcd 快照", "velero 备份", "灾备演练"],
"intent_queries": ["K8s 集群怎么备份才能真正恢复得回来", "etcd 快照恢复有哪些注意事项"],
"primary_tag": "backup",
"overview": "『可备份』只是起点，『可恢复』才是终点：所有备份必须经历实弹恢复演练才算数。",
"triggers": [
    "例行备份任务执行与抽查",
    "升级、大规模迁移前的保护性快照",
    "误删数据/损坏后的应急恢复",
],
"pre_checks": [
    "备份对象四象限清单：etcd 快照、资源清单(GitOps 库)、PV 数据、证书私钥",
    "RPO/RTO 目标书面化并与业务方签字确认",
    ("恢复环境与介质可达性预检", "doc:12-可靠性/02-灾难恢复/index.md|灾备恢复专题"),
],
"branches": [
    {"cond": "控制面受损", "name": "A · etcd 快照与恢复",
     "steps": [
         "snapshot save 定时执行 + 离线异地副本（校验 checksum）",
         ("恢复必须在隔离环境先行彩排验证", "fta:etcd-fta.md", "fta:backup-restore-fta.md"),
     ]},
    {"cond": "应用与数据", "name": "B · 应用级备份（Velero 类工具）",
     "steps": [
         "使用 hook 保证数据库一致性快照（fsfreeze/db dump）",
         "restore 后必须重建 ServiceAccount 与凭证绑定关系",
     ]},
    {"cond": "区域性故障", "name": "C · 容灾切换",
     "steps": [
         ("按多集群手册执行 DNS/GSLB 切换",
          "doc:13-生产运维/07-运维手册/05-multi-cluster-operations.md|多集群运维手册"),
         "切回正向演练同样计入达标项",
     ]},
],
"validation": [
    "季度恢复演练达标：RTO 实测 ≤ 目标值",
    "抽样 restore 的数据一致性哈希比对通过",
    "备份介质离线留存占比 ≥50%（防勒索逻辑）",
],
"pitfalls": [
    "只备份不演练——真出事才发现快照跨大版本不兼容",
    "证书随快照原样恢复导致集群 PKI 冲突",
    "PV 使用存储层快照却不停写，静默产生数据撕裂",
],
"escalation": [("恢复演练失败或 RTO 超标", "列入 P1 风险跟踪并暂停相关升级计划")],
"resources_docs": [
    "doc:12-可靠性/README.md|可靠性域",
    "doc:12-可靠性/02-灾难恢复/index.md|灾难恢复专题",
],
"resources_ftas": ["backup-restore-fta.md", "etcd-fta.md", "csi-fta.md"],
"resources_skills": ["12-control-plane-failure.md", "08-pvc-storage-failure.md"],
"related": [
    "scenario:upgrade-migration.md|SC-08 升级迁移",
    "scenario:multi-cluster.md|SC-17 多集群管理",
],
},
# ---------------------------------------------------------------- SC-08
{
"id": "SC-08", "name": "upgrade-migration",
"title": "升级迁移", "title_en": "Upgrade & Migration",
"group": "建设与交付", "read_time": "9min",
"description": "版本升级与集群迁移剧本：兼容性矩阵、逐级阶梯、废弃 API 清零与回退预案。",
"trigger_keywords": ["版本升级", "跨版本升级", "API 弃用", "集群迁移"],
"intent_queries": ["K8s 小版本升级的正确姿势是什么", "跨大版本升级要注意什么"],
"primary_tag": "upgrade",
"overview": "一次只升一级、插件与控制面的升级次序、每一级的逃生门——升级事故几乎全是纪律问题。",
"triggers": [
    "当前版本临近上游支持尾声",
    "新特性或安全合规要求强制版本门槛",
    "跨云/跨机房迁移伴随版本跃迁",
],
"pre_checks": [
    "兼容性矩阵确认：CNI/CSI/Ingress/runtime 与目标版本互相兼容",
    "deprecation 扫描：当前 API usage 对照目标版本移除清单归零",
    ("保护性快照已完成并通过校验", "scenario:backup-restore.md|SC-07 备份恢复"),
    "维护窗口公告发布且变更冻结生效",
],
"branches": [
    {"cond": "patch 版本", "name": "A · Patch 升级",
     "steps": [
         "节点池分批 drain → upgrade → uncordon，每批保有 ≥1/3 冗余",
         ("升级后核查组件信任链与 kubelet 凭证", "fta:cluster-upgrade-fta.md"),
     ]},
    {"cond": "minor/大版本", "name": "B · 跨版本阶梯",
     "steps": [
         "严禁跳级：x.y → x.y+1 逐级完成且每级回归测试",
         ("证书有效期护栏：不足 90 天先续期再升级",
          "case:ticket-case-005-kubelet-cert-expired.md|kubelet 证书过期",
          "skill:27-cluster-upgrade-migration.md"),
     ]},
    {"cond": "整体搬迁", "name": "C · 迁移上云/跨 Region",
     "steps": [
         "双跑并行：以 GitOps manifest 迁移为主，杜绝数据面手工拷贝",
         "DNS 灰度切换并保留 ≥7 天双栈回切窗口",
     ]},
],
"validation": [
    "全组件版本一致且无 mixed-version 告警",
    "废弃 API 审计查询结果归零",
    "试点业务全量回归 + 监控指标 72 小时平稳",
],
"pitfalls": [
    "插件（CNI/Webhook）滞后于控制面形成兼容裂缝",
    "升级期间并行其他变更，失败后无从归因",
    "金丝雀池遗漏网关/中间件节点批次",
],
"escalation": [("任一批次升级后数据面受损", "立即中止批次队列并回滚，转 SC-03 总纲处理")],
"resources_docs": [
    "doc:11-发布变更/04-变更管理/index.md|变更管理",
    "doc:01-集群基础/README.md|集群基础",
],
"resources_ftas": ["cluster-upgrade-fta.md", "kubeadm-fta.md", "certificate-fta.md"],
"resources_skills": ["27-cluster-upgrade-migration.md", "06-certificate-expiry.md"],
"related": [
    "scenario:backup-restore.md|SC-07 备份恢复",
    "scenario:cluster-deployment.md|SC-01 集群部署",
],
},
# ---------------------------------------------------------------- SC-09 (标杆)
{
"id": "SC-09", "name": "daily-ops",
"title": "日常运维巡检", "title_en": "Daily Operations",
"group": "稳定性保障", "read_time": "15min",
"description": "六板块例行巡检剧本：健康→容量→弹性→网络存储→可观测→清洁，护住日常稳定底线。",
"trigger_keywords": ["日常运维", "每日巡检", "例行检查", "健康巡检"],
"intent_queries": ["K8s 集群每天应该巡检哪些内容", "有没有一份可直接执行的每日运维清单"],
"primary_tag": "daily-ops",
"overview": (
    "把『每天随手看看』固化为六个可勾选板块：四个每日必检 + 两个每周深化。"
    "全部命令幂等只读，巡检结论落入日检记录形成趋势资产；"
    "黄灯项当场建跟踪单，红灯项立即转入 SC-03 总纲。"),
"triggers": [
    "每日固定巡检窗口（建议晨会前 30 分钟）",
    "节假日/大促期间的加强巡检模式",
    "重大变更次日增强复查",
],
"pre_checks": [
    ("复核昨夜值班交接事项，确认无遗留 P2 以上未闭环项",
     "doc:13-生产运维/03-事件响应/04-on-call-playbook.md|On-Call 手册"),
    ("浏览今日变更计划，标记高风险时段避开深度清理动作",
     "doc:13-生产运维/07-运维手册/02-change-management-guide.md|变更管理指南"),
    "打开当月巡检记录表准备趋势对照",
],
"branches": [
    {"cond": "每日必检", "name": "板块 ① 集群健康面（约 5 分钟）",
     "steps": [
         "nodes 全 Ready；NotReady/SchedulingDisabled 异常节点即时取证建单",
         "kube-system 与自研平台命名空间全部 Running 且无 CrashLoop",
         ("Warning events 近 12h 波形环比抬升即深挖",
          "doc:19-故障诊断/README.md|故障诊断域入口"),
     ]},
    {"cond": "每日必检", "name": "板块 ② 容量与配额面（约 5 分钟）",
     "steps": [
         "节点 allocatable 使用率三维热点排序（CPU/内存/磁盘）",
         ("证书 ≤30 天预警清单出具，续期任务挂起即跟进",
          "skill:06-certificate-expiry.md",
          "case:ticket-case-005-kubelet-cert-expired.md|kubelet 证书过期"),
         "namespace 配额命中率超过 85% 的提前介入复盘",
     ]},
    {"cond": "每日必检", "name": "板块 ③ 负载与弹性面（约 5 分钟）",
     "steps": [
         ("非 Running Pod 清零检查（Pending/ImagePullBackOff 等）",
          "skill:03-pod-pending.md"),
         "HPA/VPA 指标源健康度与近期伸缩行为合理性回顾",
         ("autoscaler 本体异常当日必须升级处理",
          "fta:cluster-autoscaler-fta.md",
          "case:ticket-case-020-cluster-autoscaler-scale-failure.md|CA 扩容失败"),
     ]},
    {"cond": "每日必检", "name": "板块 ④ 网络·存储·中间件面（约 5 分钟）",
     "steps": [
         ("Endpoints 为空的 Service 清单应为空集",
          "skill:05-service-connectivity.md",
          "case:ticket-case-019-kubeproxy-service-unreachable.md|proxy 断连"),
         "Pending 超过 1 小时的 PVC 与 CSI 组件心跳核查",
         ("CoreDNS 尾延迟与解析 QPS 异常波动筛查",
          "skill:04-dns-resolution-failure.md",
          "case:ticket-case-008-coredns-vpc-dns-forward.md|CoreDNS 转发案例"),
     ]},
    {"cond": "每周深化", "name": "板块 ⑤ 可观测与日志面（周检）",
     "steps": [
         ("Prometheus 存储增长斜率与 TSDB compaction 健康",
          "case:ticket-case-015-prometheus-data-loss-slow-query.md|Prometheus 劣化"),
         ("日志管道丢包率与缓冲区水位",
          "skill:17-logging-pipeline-failure.md"),
     ]},
    {"cond": "每周深化", "name": "板块 ⑥ 清洁与合规面（周检）",
     "steps": [
         ("completed Job 与 evicted Pod 的保留策略执行情况",
          "case:ticket-case-035-node-diskpressure-eviction.md|磁盘压力驱逐"),
         "孤儿 ConfigMap/PVC 审计并列回收白名单（双人复核）",
         ("巡检摘要按话术模板同步相关方",
          "doc:13-生产运维/06-回复话术/README.md|回复话术库"),
     ]},
],
"validation": [
    "六板块检查全部绿灯并签署电子巡检记录",
    "发现的黄灯项均已建立跟踪单（owner + deadline 齐备）",
    "趋势面板本周与前四周形态可比（无数据断点）",
],
"pitfalls": [
    "巡检变成刷新页面：发现红灯但不产出工单等于没巡",
    "只看均值不看尾部——P99 劣化总是先于阈值告警出现",
    "清理动作安排在业务高峰执行，误伤热点数据",
],
"escalation": [
    ("巡检中发现 P0 征兆", "当场转入 SC-03 总纲并移交 On-Call 接管"),
    ("连续两天同类黄灯", "立项专项分析并纳入周会汇报"),
],
"resources_docs": [
    "doc:13-生产运维/07-运维手册/01-production-sre-daily-ops.md|生产 SRE 日常运维手册",
    "doc:13-生产运维/07-运维手册/10-node-and-runtime-ops.md|节点与运行时运营",
    "doc:13-生产运维/05-工单案例/ticket-routing-rules.md|工单路由规则",
],
"resources_ftas": ["node-fta.md", "monitoring-fta.md"],
"resources_skills": ["03-pod-pending.md", "04-dns-resolution-failure.md", "06-certificate-expiry.md"],
"related": [
    "scenario:capacity-planning.md|SC-14 容量规划",
    "scenario:monitoring-alerting.md|SC-06 监控告警",
    "scenario:cost-optimization.md|SC-19 成本优化",
],
},
# ---------------------------------------------------------------- SC-10
{
"id": "SC-10", "name": "ai-infra-ops",
"title": "AI 基础设施运维", "title_en": "AI Infra Operations",
"group": "建设与交付", "read_time": "10min",
"description": "GPU 池化、模型 Serving 与训练任务的基础设施运维剧本。",
"trigger_keywords": ["GPU 运维", "推理服务", "device plugin", "训练任务"],
"intent_queries": ["K8s 上 GPU 任务调度失败如何排查", "LLM 推理服务的运维要点是什么"],
"primary_tag": "ai-infra",
"overview": "AI 负载的特殊性在于稀缺异构资源 + 长时任务 + 显存严苛约束，本剧本聚焦 GPU 分配链路与推理 Serving 两大主战场。",
"triggers": [
    "Pod 因 GPU 资源不足长期 Pending",
    "推理服务延迟抖动 / 显存命中率下降",
    "训练任务 OOM 或节点宕机后的续训需求",
],
"pre_checks": [
    "驱动/CUDA/容器运行时版本矩阵一致性确认",
    ("梳理调度器与配额策略现状", "doc:15-AI基础设施/README.md|AI 基础设施域"),
],
"branches": [
    {"cond": "GPU 供给", "name": "A · GPU 分配链路",
     "steps": [
         ("device-plugin 注册状态与时钟漂移核查", "fta:gpu-fta.md"),
         ("排除 GPU 因素后沿用通用 Pending 方法论", "skill:03-pod-pending.md"),
     ]},
    {"cond": "在线服务", "name": "B · 模型 Serving 运维",
     "steps": [
         "HPA 指标从 GPU 利用率切换为并发/RPS 口径",
         ("滚动发布按整卡粒度推进避免显存叠加溢出", "skill:13-autoscaling-failure.md"),
     ]},
    {"cond": "离线任务", "name": "C · 训练稳定性",
     "steps": [
         "checkpoint 间隔与对象存储落盘验证",
         "NCCL 慢节点自动化剔除（环状诊断脚本挂任务 HOOK）",
     ]},
],
"validation": [
    "GPU 整卡分配率与碎片率同时达标（碎片 <5%）",
    "推理服务 P99 达标且 OOM 无复现周期 ≥7 天",
    "断点续训演练 100% 成功",
],
"pitfalls": [
    "nvidia.com/gpu 整卡粗粒度申请导致小模型浪费半张卡",
    "共享存储高吞吐写打满对象存储配额拖垮全体训练任务",
    "在有任务运行的节点上直接热升级驱动",
],
"escalation": [("硬件级 XID 错误频发", "通知供应商换卡并把节点划入隔离池")],
"resources_docs": [
    "doc:15-AI基础设施/README.md|AI 基础设施域",
    "doc:16-专项技术/README.md|专项技术",
],
"resources_ftas": ["gpu-fta.md", "crd-operator-fta.md"],
"resources_skills": ["03-pod-pending.md", "13-autoscaling-failure.md", "18-performance-bottleneck.md"],
"related": [
    "scenario:performance-tuning.md|SC-04 性能调优",
    "scenario:capacity-planning.md|SC-14 容量规划",
],
},
# ---------------------------------------------------------------- SC-11
{
"id": "SC-11", "name": "network-diagnosis",
"title": "网络诊断", "title_en": "Network Diagnosis",
"group": "稳定性保障", "read_time": "10min",
"description": "五跳分段网络诊断剧本：DNS→Service→Ingress→Policy→Underlay 各个击破。",
"trigger_keywords": ["网络不通", "DNS 解析失败", "connection refused", "502 排查"],
"intent_queries": ["Pod 之间网络不通怎么层层排查", "Ingress 返回 502 问题出在哪一段"],
"primary_tag": "networking",
"overview": "二分法哲学：每一跳只回答通或不通，五跳之内问题必现形。全程坚持 tcpdump/conntrack 双证留痕。",
"triggers": [
    "东西向不通：Pod↔Pod / Pod↔Service 异常",
    "南北向异常：LB/Ingress 入口 502、404、超时",
    "外联异常：egress 白名单阻断、SNAT 端口耗尽",
],
"pre_checks": [
    ("第一跳固定先测 DNS（nslookup / 直查 CoreDNS）", "skill:04-dns-resolution-failure.md"),
    "抓取问题时间窗内的 NetworkPolicy 与配置变更 diff",
    "确定两端采样点（客户端 Pod 与服务端 Pod 同令牌请求）",
],
"branches": [
    {"cond": "集群内转发", "name": "A · Service/Endpoints 段",
     "steps": [
         ("selector 匹配与 readiness 状态核对", "skill:05-service-connectivity.md"),
         ("代理转发面深入排查", "fta:kube-proxy-fta.md", "fta:service-fta.md"),
     ]},
    {"cond": "南北入口", "name": "B · Ingress/Gateway 段",
     "steps": [
         ("controller 自身健康与重载失败优先排查",
          "skill:14-ingress-gateway-failure.md",
          "case:ticket-case-011-ingress-controller-pod-404-502.md|Ingress 404/502"),
         ("LB 配置类故障样本对照", "case:ticket-case-003-slb-backend-group-misconfig.md|SLB 后端组", "fta:nginx-ingress-fta.md"),
     ]},
    {"cond": "数据面嫌疑", "name": "C · CNI 数据面段",
     "steps": [
         ("VPC 路由与 ENI 类疑难集中营",
          "fta:terway-fta.md",
          "case:ticket-case-001-terway-eni-exhaustion.md|Terway ENI 耗尽"),
         ("跨发行版症状比对", "fta:calico-fta.md", "fta:cilium-fta.md"),
     ]},
    {"cond": "规则命中", "name": "D · NetworkPolicy 段",
     "steps": [
         ("以 implicit deny 视角自审放行链",
          "skill:22-networkpolicy-connectivity.md",
          "case:ticket-case-010-networkpolicy-blocks-traffic.md|Policy 断流",
          "fta:networkpolicy-fta.md"),
     ]},
],
"validation": [
    "原始故障路径复测 100% 连通并留存抓包证据",
    "相邻业务冒烟通过：证明修复无旁路损伤",
    "若根因为配额/容量，新增对应红线监控",
],
"pitfalls": [
    "ndots:5 造成搜索域爆炸，误判为上游 DNS 故障",
    "keepalive 长连接绕过了刚刚更新的 Endpoints",
    "只测 TCP 握手不验应用层语义——通了但不是你要的服务",
],
"escalation": [("触及 VPC/SLB 底层行为存疑", "提云厂商工单并附双向抓包证据")],
"resources_docs": [
    "doc:05-网络/README.md|网络域",
],
"resources_ftas": ["dns-fta.md", "cni-fta.md", "kube-proxy-fta.md", "ingress-fta.md"],
"resources_skills": ["04-dns-resolution-failure.md", "05-service-connectivity.md", "14-ingress-gateway-failure.md"],
"related": [
    "scenario:troubleshooting.md|SC-03 故障排查总纲",
    "scenario:mesh-ops.md|SC-16 服务网格运维",
],
},
# ---------------------------------------------------------------- SC-12
{
"id": "SC-12", "name": "storage-issues",
"title": "存储问题排查", "title_en": "Storage Issues",
"group": "稳定性保障", "read_time": "9min",
"description": "PV/PVC/CSI 全链路排查剧本：供应、绑定、挂载、IO 性能与回收五大段位。",
"trigger_keywords": ["PVC Pending", "挂载失败", "csi 异常", "存储 IO 高"],
"intent_queries": ["PVC 一直 Pending 如何排查", "StatefulSet 卷挂载失败怎么办"],
"primary_tag": "storage",
"overview": "以 CSI 事件流为主轴，一段一段排除供应→绑定→挂载→IO→回收的问题，拒绝笼统重启。",
"triggers": [
    "PVC Pending / 卷绑定失败",
    "Pod 挂卷报错 mount failed / timeout",
    "IO 延迟飙升、云盘性能打满类事件",
],
"pre_checks": [
    ("Describe PVC 摘取事件关键词（FailedBinding/ProvisioningFailed）", "skill:08-pvc-storage-failure.md"),
    ("CSI controller/node 组件心跳与版本匹配核查", "fta:csi-fta.md"),
],
"branches": [
    {"cond": "分配阶段", "name": "A · 供应与绑定段",
     "steps": [
         "storageClass 的 provisioner/volumeBindingMode 与区域亲和核对",
         ("扩缩容后插件实例缺失的典型样本",
          "case:ticket-case-004-csi-plugin-missing-after-scale.md|扩容后 CSI 插件缺失"),
     ]},
    {"cond": "使用阶段", "name": "B · 挂载段",
     "steps": [
         ("mount 错误码分类：权限/网络/残留挂载点",
          "case:ticket-case-028-statefulset-pvc-unbound.md|STS PVC 未绑定"),
         ("批量挂载失败按 StatefulSet 序号聚类定位",
          "skill:23-statefulset-failure.md"),
     ]},
    {"cond": "运行阶段", "name": "C · IO 性能段",
     "steps": [
         ("云盘突发带宽/限流阈值核查",
          "case:ticket-case-002-java-oom-essd-iohang.md|ESSD IO Hang"),
         "fsync 延迟异常纳入节点驱逐联动审查",
     ]},
    {"cond": "生命周期末端", "name": "D · 回收与扩容段",
     "steps": [
         "reclaimPolicy 对照业务预期（Deleted 的回收站语义确认）",
         "在线扩容前置条件：allowVolumeExpansion=true 且无快照链",
     ]},
],
"validation": [
    "新增示例 StatefulSet 完整走通供→绑→挂→IO 四步",
    "问题卷的性能曲线恢复正常区间",
    "回收类操作双人复核（防误删生产卷）",
],
"pitfalls": [
    "把云盘计费状态异常误诊为 CSI 故障",
    "同一可用区售罄反复重试 Provisioner 形成排队雪崩",
    "force-detach 施加于多挂载卷引发数据竞争",
],
"escalation": [("多租户同时挂载失败的底座故障", "广播暂停对应 StorageClass 新供给并升级")],
"resources_docs": [
    "doc:06-存储/README.md|存储域",
],
"resources_ftas": ["csi-fta.md", "statefulset-fta.md"],
"resources_skills": ["08-pvc-storage-failure.md", "23-statefulset-failure.md"],
"related": [
    "scenario:troubleshooting.md|SC-03 故障排查总纲",
    "scenario:backup-restore.md|SC-07 备份恢复",
],
},
# ---------------------------------------------------------------- SC-13
{
"id": "SC-13", "name": "security-incident",
"title": "安全事件响应", "title_en": "Security Incident Response",
"group": "安全合规", "read_time": "10min",
"description": "从发现到复盘的安全应急响应剧本：隔离、取证、清除、溯源、通报五步闭环。",
"trigger_keywords": ["入侵响应", "挖矿木马", "安全应急", "凭据泄露"],
"intent_queries": ["发现容器被入侵第一时间做什么", "K8s 挖矿事件的应急处置流程"],
"primary_tag": "security",
"overview": "铁律次序：先隔离、再取证、后清除；被感染资产一律重建而非原地清洗——假设对手已持久化。",
"triggers": [
    "入侵检测/挖矿特征命中告警",
    "异常外联至威胁情报黑名单 IP",
    "密钥泄露或审计日志中的越权痕迹",
],
"pre_checks": [
    ("按分级标准完成事件定性（SEV-S/P1/P2）",
     "doc:13-生产运维/03-事件响应/01-escalation-matrix-severity-levels.md|事件分级标准"),
    "开启专用安保频道（脱离日常 on-call 群）",
    "记录初始时间戳 T0，法务/合规全程跟随",
],
"branches": [
    {"cond": "攻击位于集群内", "name": "A · 隔离与止血",
     "steps": [
         ("Node taint + cordon，恶意命名空间策略全封闭",
          "skill:19-security-incident-response.md"),
         "吊销可疑 SA Token 与长效凭证，轮换镜像仓库密钥",
     ]},
    {"cond": "证据阶段", "name": "B · 取证保全",
     "steps": [
         "容器文件系统/内存快照上传证物桶（chain of custody 登记）",
         ("导出 apiserver audit log 固定时间窗",
          "doc:13-生产运维/03-事件响应/08-supply-chain-incident-response.md|供应链事件响应"),
     ]},
    {"cond": "清除阶段", "name": "C · 清除与重建",
     "steps": [
         "受感染节点与镜像一律重建不复用",
         ("容器运行时逃逸路径复核",
          "doc:13-生产运维/03-事件响应/07-container-runtime-threat-response.md|运行时威胁响应"),
     ]},
    {"cond": "对外沟通", "name": "D · 溯源与通报",
     "steps": [
         ("统一发言人使用既定话术对外通报",
          "doc:13-生产运维/03-事件响应/03-communication-templates-stakeholder.md|干系人沟通模板"),
         "IOC 回填威胁情报库并反向全网扫描存量",
     ]},
],
"validation": [
    "七日内无同类 IOC 复燃",
    "审计日志确认权限收敛到位（最小暴露面）",
    "复盘报告涉法部分经法务签核归档",
],
"pitfalls": [
    "直接 kill 进程留下持久化后门与定时任务后患",
    "取证前重启节点，内存证据灰飞烟灭",
    "只收敛受影响命名空间，遗漏横向移动副路径",
],
"escalation": [("确认数据外泄或勒索加密", "立即上升公司安全委员会并启动公关预案")],
"resources_docs": [
    "doc:08-安全/README.md|安全域",
    "doc:13-生产运维/03-事件响应/index.md|事件响应手册集",
],
"resources_ftas": ["webhook-admission-fta.md", "psp-scc-fta.md"],
"resources_skills": ["19-security-incident-response.md", "15-configmap-secret-failure.md"],
"related": [
    "scenario:security-hardening.md|SC-05 安全加固",
    "scenario:compliance-audit.md|SC-20 合规审计",
],
},
# ---------------------------------------------------------------- SC-14
{
"id": "SC-14", "name": "capacity-planning",
"title": "容量规划", "title_en": "Capacity Planning",
"group": "可靠性韧性", "read_time": "9min",
"description": "容量评估-建模-扩容-压测闭环剧本，服务大促备战与常态化水位管理。",
"trigger_keywords": ["容量规划", "扩容评估", "大促备战", "水位预警"],
"intent_queries": ["大促前如何评估集群容量是否充足", "headroom 应该预留多少合适"],
"primary_tag": "capacity",
"overview": "容量是一种预算：以业务增长率为输入锚点，沉淀水位→预测→压测→执行的季度轮回机制。",
"triggers": [
    "大促/活动前备战窗口开启",
    "节点水印连续一周高于 70%（CPU/内存任一维度）",
    "因资源不足导致的 Pending 占比抬头",
],
"pre_checks": [
    ("读取 SC-09 巡检沉淀的三个月水位趋势", "scenario:daily-ops.md|SC-09 日常巡检"),
    ("request 失真体检：real/request 比 <0.6 的右调清单",
     "doc:13-生产运维/01-成本治理/02-idle-resource-right-sizing.md|闲置资源右调"),
    "列出隐性天花板：IP 池/端口/安全组限额（别只算 CPU 内存）",
],
"branches": [
    {"cond": "短期缺口", "name": "A · 弹性与水平扩容",
     "steps": [
         ("CA 参数梳理（上下限/scale-down 延迟/冷启动压测）",
          "fta:cluster-autoscaler-fta.md",
          "case:ticket-case-045-cluster-autoscaler-scaleup-fail.md|CA 扩容失败"),
         "HPA 行为参数（stabilizationWindow）贴合业务波形",
     ]},
    {"cond": "结构性低效", "name": "B · 结构重排",
     "steps": [
         ("大小规格节点池混布降低碎片化 Pending",
          "case:ticket-case-012-pod-pending-resource-exhaustion.md|资源耗尽 Pending"),
         "反亲和与拓扑打散策略回归验证",
     ]},
    {"cond": "极限验证", "name": "C · 压测定型",
     "steps": [
         "影子流量全链路压测至目标峰值 ×1.3",
         ("容量基线纳入生产就绪度评审附件",
          "doc:13-生产运维/07-运维手册/03-capacity-planning-readiness.md|容量规划就绪指南"),
     ]},
],
"validation": [
    "峰值水位控制在 70% 预警线以下且有 headroom 台账",
    "弹性到位时间实测 <5 分钟",
    "成本影响测算随行提交（防止拍脑袋扩容）",
],
"pitfalls": [
    "只算 CPU 内存，IP 池枯竭使扩容全军覆没",
    "压测只打单接口，网关限流从未经受检验",
    "扩完不做缩容复盘，容量账永远虚胖",
],
"escalation": [("两周内无法满足确定性业务缺口", "上报架构委员会进入机型采购/混合云流程")],
"resources_docs": [
    "doc:13-生产运维/07-运维手册/03-capacity-planning-readiness.md|容量规划就绪指南",
    "doc:13-生产运维/01-成本治理/01-cost-allocation-chargeback.md|成本分摊",
],
"resources_ftas": ["cluster-autoscaler-fta.md", "hpa-fta.md"],
"resources_skills": ["13-autoscaling-failure.md", "26-namespace-quota-limitrange.md"],
"related": [
    "scenario:daily-ops.md|SC-09 日常巡检",
    "scenario:cost-optimization.md|SC-19 成本优化",
],
},
# ---------------------------------------------------------------- SC-15
{
"id": "SC-15", "name": "gitops-workflow",
"title": "GitOps 工作流", "title_en": "GitOps Workflow",
"group": "建设与交付", "read_time": "9min",
"description": "ArgoCD/Flux GitOps 体系搭建与运维剧本：仓库规范、同步治理、漂移管理与回滚设计。",
"trigger_keywords": ["gitops", "argocd", "漂移 drift", "声明式发布"],
"intent_queries": ["ArgoCD Application 卡在 Progressing 怎么办", "GitOps 多环境目录结构怎么设计"],
"primary_tag": "gitops",
"overview": "Git 即真理：仓库目录规范是第一公民，同步/漂移/回滚全部由提交驱动，集群侧禁止手改。",
"triggers": [
    "新 GitOps 体系初建或多集群推广",
    "Application 长期 OutOfSync/Progressing",
    "漂移告警：人工 kubectl 改动未经提交",
],
"pre_checks": [
    "仓库布局评审通过（app-of-apps / 环境 overlay 分层）",
    "CI 流水线与人工写入权限分离（一切变更 PR 化）",
    "敏感配置方案先行（SealedSecrets/ESO），禁止明文入库",
],
"branches": [
    {"cond": "同步异常", "name": "A · Sync 故障排查",
     "steps": [
         ("Progressing 卡死三分叉：webhook 不可达/hook 失败/history 上限",
          "fta:gitops-argocd-fta.md",
          "doc:11-发布变更/README.md|发布变更域"),
     ]},
    {"cond": "配置漂移", "name": "B · Drift 治理",
     "steps": [
         "autosync 策略分级：prod 手动+评审，dev 自动",
         "diff masking 屏蔽噪音字段（status 类字段）降噪",
     ]},
    {"cond": "发布编排", "name": "C · 发布与回滚编排",
     "steps": [
         ("与 SC-02 共用发布验证门禁", "scenario:app-deployment.md|SC-02 应用发布"),
         "rollback = git revert，彻底杜绝手改集群",
     ]},
],
"validation": [
    "漂移告警 MTTA <10 分钟且条条有 owner",
    "AppProject 权限最小化审计通过",
    "灾备演练：空集群从 repo 重建 <2 小时",
],
"pitfalls": [
    "CI 直接 kubectl apply 绕过 GitOps 制造永久漂移",
    "helm values 巨型 YAML 无人敢改，diff 失控",
    "拿 sync window 当变更管控反而延误事故止血",
],
"escalation": [("GitOps 系统自身不可用", "启用只读逃生账号 + 事故特批写入流程")],
"resources_docs": [
    "doc:11-发布变更/README.md|发布变更域",
    "doc:10-平台工程/README.md|平台工程域",
],
"resources_ftas": ["gitops-argocd-fta.md"],
"resources_skills": ["09-deployment-rollout-failure.md", "28-helm-chart-failure.md"],
"related": [
    "scenario:app-deployment.md|SC-02 应用发布",
    "scenario:upgrade-migration.md|SC-08 升级迁移",
],
},
# ---------------------------------------------------------------- SC-16
{
"id": "SC-16", "name": "mesh-ops",
"title": "服务网格运维", "title_en": "Service Mesh Operations",
"group": "建设与交付", "read_time": "9min",
"description": "Istio 网格部署、sidecar 生命周期、mTLS 切换与流量治理的运维剧本。",
"trigger_keywords": ["istio 运维", "envoy", "mtls 切换", "sidecar 注入"],
"intent_queries": ["sidecar 没注入怎么排查", "Istio 开启 mTLS 后服务互访失败怎么办"],
"primary_tag": "mesh",
"overview": "网格是把双刃剑：本剧本守住 sidecar 一致性、mTLS 灰度节奏、遥测开销三条防线。",
"triggers": [
    "namespace 打标后 Pod 未注入 sidecar",
    "mTLS STRICT 切换后互访 503/UH",
    "Envoy 配置下发异常或 sidecar CPU 飙升",
],
"pre_checks": [
    ("istiod 健康/注入 webhook 就绪/revision 策略确认", "fta:service-mesh-istio-fta.md"),
    "画出 PeerAuthentication 当前作用域（mesh/ns/port）",
],
"branches": [
    {"cond": "注入异常", "name": "A · Sidecar 注入与生命周期",
     "steps": [
         "label 注入策略后需 recreate 才生效的要点进模板",
         ("init 容器 iptables 写入失败排查样板", "fta:pod-fta.md"),
     ]},
    {"cond": "流量治理", "name": "B · VirtualService/DestinationRule",
     "steps": [
         ("subset 与一致性哈希依赖的全局局部性检查", "fta:gateway-api-fta.md"),
         "golden signals 按 service 级大盘建立",
     ]},
    {"cond": "零信任推进", "name": "C · mTLS 与授权策略",
     "steps": [
         "PERMISSIVE 观察 7 天再切 STRICT 的节奏铁律",
         "AuthorizationPolicy 拒绝原因采样进 access log 辅助排障",
     ]},
],
"validation": [
    "网格内工作负载注入覆盖率 100% 无裸奔",
    "mTLS 切换前后 RPC 成功率差值 <0.1%",
    "sidecar CPU/内存开销画像不超过申报阈值",
],
"pitfalls": [
    "outlierDetection 阈值配错把健康 Pod 全部 ejected",
    "根证书轮转期间集中 restart 未分批造成雪崩",
    "全网格 trace 采样 100% 把 Envoy 内存打爆",
],
"escalation": [("南北向网关全局故障", "先行执行绕过 mesh 的直连逃生预案")],
"resources_docs": [
    "doc:05-网络/README.md|网络域",
],
"resources_ftas": ["service-mesh-istio-fta.md", "gateway-api-fta.md", "higress-fta.md"],
"resources_skills": ["05-service-connectivity.md", "14-ingress-gateway-failure.md"],
"related": [
    "scenario:network-diagnosis.md|SC-11 网络诊断",
    "scenario:app-deployment.md|SC-02 应用发布",
],
},
# ---------------------------------------------------------------- SC-17
{
"id": "SC-17", "name": "multi-cluster",
"title": "多集群管理", "title_en": "Multi-Cluster Management",
"group": "可靠性韧性", "read_time": "10min",
"description": "多集群纳管、跨集群服务发现与灾备切换剧本，覆盖主备/双活/分片三种拓扑。",
"trigger_keywords": ["多集群管理", "舰队 fleet", "跨集群服务发现", "容灾切换演练"],
"intent_queries": ["多套 K8s 集群如何统一管理和下发变更", "跨集群容灾切换怎么做"],
"primary_tag": "multi-cluster",
"overview": "多集群的本质是治理半径扩大：统一的身份体系、统一的下发面、且故障切换必须可演练。",
"triggers": [
    "新集群并入舰队 / 旧集群退出",
    "跨集群调用延迟或流量环路",
    "年度容灾切换演练窗口",
],
"pre_checks": [
    ("舰队清单与健康画像仪表先行",
     "doc:13-生产运维/07-运维手册/05-multi-cluster-operations.md|多集群运维手册"),
    "身份联邦（OIDC/token 链）与网络边界（专线/peering）确认",
],
"branches": [
    {"cond": "主备拓扑", "name": "A · 主备容灾型",
     "steps": [
         ("以备份恢复驱动的接管流程为主轴（重点盯 RTO）",
          "scenario:backup-restore.md|SC-07 备份恢复"),
         "核心 DNS/GSLB 切换 runbook 每季一演",
     ]},
    {"cond": "双活拓扑", "name": "B · 双活互备型",
     "steps": [
         ("跨集群 service export/import 的治理规范", "fta:cloud-provider-fta.md"),
         "split-brain 阈值与第三方仲裁探针定义",
     ]},
    {"cond": "分片拓扑", "name": "C · 业务分片自治型",
     "steps": [
         "命名空间→集群的路由表版本化管理",
         "分片再平衡的操作预演脚本化",
     ]},
],
"validation": [
    "接管演练 RTO/RPO 实测值达标并留档",
    "配置偏差扫描（policy-as-code diff）报告零高危",
    "出口逃生：任一集群可单独摘除而不影响舰队",
],
"pitfalls": [
    "靠复制粘贴部署而非中心化下发——配置漂移无处不在",
    "缺乏跨集群链路追踪，排障只见一半调用链",
    "专线欠费悄悄断开两个月无人知晓",
],
"escalation": [("舰队级控制面瘫痪", "启用应急预案二把手 + 云厂商 TAM 绿色通道")],
"resources_docs": [
    "doc:13-生产运维/07-运维手册/05-multi-cluster-operations.md|多集群运维手册",
],
"resources_ftas": ["cloud-provider-fta.md", "cluster-autoscaler-fta.md"],
"resources_skills": ["26-namespace-quota-limitrange.md"],
"related": [
    "scenario:backup-restore.md|SC-07 备份恢复",
    "scenario:edge-ops.md|SC-18 边缘运维",
],
},
# ---------------------------------------------------------------- SC-18
{
"id": "SC-18", "name": "edge-ops",
"title": "边缘运维", "title_en": "Edge Computing Operations",
"group": "建设与交付", "read_time": "9min",
"description": "KubeEdge/OpenYurt 边缘集群的接入纳管、断网自治与云边协同运维剧本。",
"trigger_keywords": ["边缘计算", "kubeedge", "openyurt", "断网自治"],
"intent_queries": ["边缘节点频繁离线如何保证业务自治", "云边协同的监控采集怎么做"],
"primary_tag": "edge",
"overview": "边界的设计信仰是自治：云端大脑可以断联，边缘小脑必须能独立撑住本地业务。",
"triggers": [
    "新边缘站点接入 / 规模化批量下发",
    "弱网或断网投诉伴随本地业务中断",
    "边缘节点离线率超标",
],
"pre_checks": [
    "云边隧道模式确认（WebSocket tunnel / mTLS 元数据通道）",
    ("参考专项技术域的边缘实践沉淀", "doc:16-专项技术/README.md|专项技术域"),
    "边缘硬件档案：架构(ARM/x86)、内存上限、磁盘寿命",
],
"branches": [
    {"cond": "站点接入", "name": "A · 节点接入与纳管",
     "steps": [
         "证书/cgroup/runtime 依赖一次性装配包化",
         ("离线安装失败时的镜像与仓库排查套路", "skill:11-image-pull-failure.md"),
     ]},
    {"cond": "弱网自治", "name": "B · 断网自治",
     "steps": [
         "metaServer 本地缓存名单与应用白名单精细化",
         ("自治期间本地卷状态收集与回传补齐机制", "fta:csi-fta.md"),
     ]},
    {"cond": "批量运维", "name": "C · 批量下发与观测",
     "steps": [
         "站点分组灰度下发（升级波次表管控）",
         ("轻量日志/监控代理的瘦身策略", "fta:monitoring-fta.md"),
     ]},
],
"validation": [
    "拔纤演练：本地业务自治降级运行 8 小时无硬损",
    "回连后状态 reconciliation 一致性核对通过",
    "单站点故障不传染（信噪与队列隔离验证）",
],
"pitfalls": [
    "照搬中心集群的重型 DaemonSet——小机型跑不动",
    "云端控制器假定了 always-online 的交互逻辑",
    "忽视 NTP 导致证书校验与日志时序全面混乱",
],
"escalation": [("站点硬件级更换", "启动现场物流 SOP 并提供远程装配支援")],
"resources_docs": [
    "doc:16-专项技术/README.md|专项技术域",
    "doc:14-容器运行时/README.md|容器运行时域",
],
"resources_ftas": ["kubelet-fta.md", "cloud-provider-fta.md"],
"resources_skills": ["01-node-notready.md", "20-node-resource-pressure.md"],
"related": [
    "scenario:multi-cluster.md|SC-17 多集群管理",
    "scenario:daily-ops.md|SC-09 日常巡检",
],
},
# ---------------------------------------------------------------- SC-19
{
"id": "SC-19", "name": "cost-optimization",
"title": "成本优化", "title_en": "Cost Optimization",
"group": "经营效率", "read_time": "9min",
"description": "FinOps 成本优化循环剧本：可见性→右调优→弹性组合→平台化四板斧与治理固化。",
"trigger_keywords": ["成本优化", "finops", "spot 竞价实例", "资源利用率"],
"intent_queries": ["K8s 集群成本居高不下该怎么优化", "资源 requests 过大如何治理"],
"primary_tag": "cost",
"overview": "FinOps 不是砍预算，而是把每一分钱翻译成业务价值语言的可运营循环。原则：降本动作全部可回滚，SLO 对赌兜底。",
"triggers": [
    "月度账单环比上涨 >15% 或突破预算线",
    "利用率体检出炉肥胖清单（CPU 均值 <15% / 内存 <25%）",
    "新财年预算编制听证",
],
"pre_checks": [
    ("可见性先行：分摊标签覆盖率检查（无标签不入账）",
     "doc:13-生产运维/01-成本治理/01-cost-allocation-chargeback.md|成本分摊与退款"),
    "业务容忍度访谈：可抢占/可定时缩容的白名单",
    ("水位基线承接自 SC-14 容量规划", "scenario:capacity-planning.md|SC-14 容量规划"),
],
"branches": [
    {"cond": "声明失真", "name": "A · Request 右调优",
     "steps": [
         "基于 VPA recommend 曲线加权生成建议并灰度验证",
         ("闲置额度回收进公共池",
          "doc:13-生产运维/01-成本治理/02-idle-resource-right-sizing.md|闲置资源右调"),
     ]},
    {"cond": "计费结构", "name": "B · Spot/RI/弹性组合拳",
     "steps": [
         ("中断友好型负载迁 Spot 池 + 驱逐兜底方案",
          "doc:13-生产运维/01-成本治理/03-spot-instance-strategy.md|Spot 策略",
          "fta:cluster-autoscaler-fta.md"),
         ("稳态负载 RI/包年包月配比测算",
          "doc:13-生产运维/01-成本治理/05-kubernetes-cost-governance.md|K8s 成本治理"),
     ]},
    {"cond": "长期运营", "name": "C · FinOps 平台化",
     "steps": [
         ("Kubecost/OpenCost 落地与预算告警接线",
          "doc:13-生产运维/01-成本治理/04-kubecost-finops-automation.md|Kubecost 自动化"),
         ("面向管理层的账单叙事固定节奏",
          "doc:13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md|FinOps 运营 Runbook"),
     ]},
],
"validation": [
    "单位成本（每万次调用成本）环比改善可量化",
    "优化动作清单均可一键回滚且原配置已备份",
    "SLO 未劣化（与业务方事先对赌）",
],
"pitfalls": [
    "一刀切压缩 requests 制造 OOM——省小钱赔大钱",
    "Spot 大规模回收无分散策略导致业务团灭",
    "清理孤儿存储时误删仍被 StatefulSet 引用的 PV",
],
"escalation": [("预估月节省空间 >30%", "成立专项小组并制定季度 OKR")],
"resources_docs": [
    "doc:13-生产运维/01-成本治理/07-finops-cost-optimization-guide.md|FinOps 优化指南",
],
"resources_ftas": ["cluster-autoscaler-fta.md"],
"resources_skills": ["13-autoscaling-failure.md", "18-performance-bottleneck.md"],
"related": [
    "scenario:capacity-planning.md|SC-14 容量规划",
    "scenario:daily-ops.md|SC-09 日常巡检",
],
},
# ---------------------------------------------------------------- SC-20
{
"id": "SC-20", "name": "compliance-audit",
"title": "合规审计", "title_en": "Compliance & Audit",
"group": "安全合规", "read_time": "9min",
"description": "面向等保/ISO/行业监管的合规审计迎检与常态化治理剧本。",
"trigger_keywords": ["合规审计", "等保测评", "iso27001", "监管整改"],
"intent_queries": ["K8s 平台迎接等保测评要做哪些准备", "如何持续保持合规而不是临时抱佛脚"],
"primary_tag": "compliance",
"overview": "合规是被设计的而不是被应付的：控制条款→技术证据→自动化持续验证的三层映射体系。",
"triggers": [
    "等保三级/行业监管测评窗口临近",
    "客户安全问卷与大厂准入审计",
    "监管漏洞通报限期整改",
],
"pre_checks": [
    ("框架映射表：条款 ↔ CIS ↔ 内部控制项编号", "doc:08-安全/README.md|安全域导航"),
    "证据三库齐备：配置基线库 / 审计日志库 / 流程制度库",
    ("与 SC-05 加固基线联合排期", "scenario:security-hardening.md|SC-05 安全加固"),
],
"branches": [
    {"cond": "迎检冲刺", "name": "A · 差距评估与整改",
     "steps": [
         ("机器扫描 + 人工走查双轨取证",
          "doc:13-生产运维/02-集群治理/05-cluster-governance-lifecycle-compliance.md|集群治理生命周期合规"),
         "整改台账（责任人/期限/复测方式）每周更新",
     ]},
    {"cond": "常态治理", "name": "B · 策略即代码",
     "steps": [
         ("OPA/Kyverno 规则与条款编号双向注释挂钩",
          "doc:13-生产运维/02-集群治理/03-admission-policy-governance.md|准入策略治理"),
         ("audit log 保留期与防篡改(WORM)存储",
          "skill:15-configmap-secret-failure.md"),
     ]},
    {"cond": "供应链与数据", "name": "C · 供应链合规",
     "steps": [
         "SBOM 生成与准入签名验证链贯通",
         "数据分级存储位置映射（境内/境外区域约束台账）",
     ]},
],
"validation": [
    "复测得分达线且高风险项清零",
    "随机抽取 10 条证据可在 5 分钟内机器复现",
    "下一自评周期已录入系统日历防遗忘衰减",
],
"pitfalls": [
    "迎检前临时造材料——换个评测师立刻穿帮",
    "只审生产环境漏掉 CI/CD 测试环境的同等义务",
    "把『需可查看』误定为『可登录操作』造成过度授权",
],
"escalation": [("高危项 7 日无法整改", "报请风控委员会备案过渡补偿措施")],
"resources_docs": [
    "doc:08-安全/README.md|安全域",
    "doc:13-生产运维/02-集群治理/index.md|集群治理索引",
],
"resources_ftas": ["webhook-admission-fta.md", "rbac-fta.md"],
"resources_skills": ["19-security-incident-response.md", "10-rbac-quota-failure.md"],
"related": [
    "scenario:security-hardening.md|SC-05 安全加固",
    "scenario:security-incident.md|SC-13 安全事件响应",
],
},
]

GROUP_ORDER = ["建设与交付", "稳定性保障", "可靠性韧性", "安全合规", "经营效率"]

ALL_REFS_MODE_DOC_PREFIX = "doc:"


# ---------------------------------------------------------------------------
# 场景文档渲染
# ---------------------------------------------------------------------------

def frontmatter_block(s: dict) -> str:
    auto_tags = {t.lower().replace(" ", "-") for t in s.get("trigger_keywords", [])[:3]}
    tags = ["scenario", "playbook", s["primary_tag"]] + sorted(auto_tags)
    lines = [
        "---",
        f'title: "{s["id"]} 场景剧本: {s["title"]}"',
        f'title_en: "{s["title_en"]}"',
        f'description: "{s["description"]}"',
        f'summary: "{s["description"]}"',
        "category: 生产运维/scenario-playbook",
        "tags:",
    ]
    lines += [f"- {t}" for t in dict.fromkeys(tags)]
    lines += [
        f'scenario_id: "{s["id"]}"',
        f'scenario_group: "{s["group"]}"',
        f'primary_tag: "{s["primary_tag"]}"',
        "tier: core",
        f"created: '{YEAR_MONTH}'",
        f"updated: '{YEAR_MONTH}'",
        "difficulty: advanced",
        "reading_level: advanced",
        "audience:",
        "- AI Agent",
        "- SRE",
        "- 运维工程师",
        f"estimated_read_time: {s['read_time']}",
        "trigger_keywords:",
    ]
    lines += [f"- {kw}" for kw in s.get("trigger_keywords", [])]
    lines.append("intent_queries:")
    lines += [f"- {q}" for q in s.get("intent_queries", [])]
    lines.append(f"last_updated: '{TODAY}'")
    lines.append("---")
    return "\n".join(lines)


def render_pitfalls(s: dict, missing, owner) -> str:
    out = []
    for p in s["pitfalls"]:
        text, refs = parse_step(p)
        line = f"- ⚠️ {text}"
        if refs:
            links = "、".join(ref_link(r, missing, owner) for r in refs)
            line += f" → {links}"
        out.append(line)
    return "\n".join(out)


def render_resources_sections(s: dict, missing) -> str:
    parts = []
    sections = [
        ("领域文档（原理与规范）", s["resources_docs"]),
        ("FTA 故障树（根因推导）", [f"fta:{x}" for x in s["resources_ftas"]]),
        ("操作技能卡（原子动作）", [f"skill:{x}" for x in s["resources_skills"]]),
    ]
    for title, refs in sections:
        rows = [ref_link(r, missing, s["name"]) for r in refs]
        parts.append(f"### {title}\n")
        if rows:
            parts.append("\n".join(f"- {r}" for r in rows))
        else:
            parts.append("_暂无_")
        parts.append("")
    return "\n".join(parts)


def render_scenario(s: dict, all_missing: list) -> str:
    L = []
    add = L.append

    add(frontmatter_block(s)); add("")
    add(f"# {s['id']} 场景剧本: {s['title']}"); add("")
    add(f"> **ID**: `{s['id']}` · **分组**: {s['group']} · **英文**: {s['title_en']} · **更新**: {TODAY}")
    add("> **层次定位**: 工单剧本编排层 —— 回答「什么场景、按什么顺序、调用哪些资源」。")
    add("> Domain 讲原理，Skill 给动作，FTA 管推导；本页负责把它们串成可执行的工作流。")
    add("")
    add("## 一、适用场景（何时进入本剧本）"); add("")
    for t in s["triggers"]:
        add(f"- {t}")
    add("")
    add("## 二、场景概述"); add(""); add(s["overview"]); add("")
    add("## 三、前置检查（开工门槛，逐项勾选）"); add("")
    for c in s["pre_checks"]:
        text, refs = parse_step(c)
        line = f"- [ ] {text}"
        if refs:
            links = "、".join(ref_link(r, all_missing, s["name"]) for r in refs)
            line += f" → {links}"
        add(line)
    add("")
    add("## 四、快速决策树"); add(""); add(build_mermaid(s)); add("")
    add("## 五、工作流分支"); add("")
    for br in s["branches"]:
        add(f"### {br['name']}")
        add("")
        add(f"> 条件: {br['cond']}")
        add("")
        add(render_steps(br["steps"], all_missing, s["name"]))
        add("")
    add("## 六、完工验证清单"); add("")
    for v in s["validation"]:
        text, refs = parse_step(v)
        line = f"- [ ] {text}"
        if refs:
            links = "、".join(ref_link(r, all_missing, s["name"]) for r in refs)
            line += f" → {links}"
        add(line)
    add("")
    add("## 七、常见陷阱（前人踩坑榜）"); add("")
    add(render_pitfalls(s, all_missing, s["name"]))
    add("")
    add("## 八、升级路径"); add("")
    add("| 触发条件 | 升级动作 |")
    add("|---|---|")
    for cond, act in s["escalation"]:
        add(f"| {cond} | {act} |")
    add("")
    add("## 九、资源编排（跨层素材索引）"); add("")
    add(render_resources_sections(s, all_missing))
    add("## 十、相邻场景"); add("")
    rel_lines = [f"- {ref_link(r, all_missing, s['name'])}" for r in s["related"]]
    add("\n".join(rel_lines) if rel_lines else "_无_")
    add("")
    add("---")
    add("")
    add(f"*本文档由 `31-脚本/generate-scenarios.py` 于 {TODAY} 自动生成。"
        "请修改脚本中的场景数据后重新生成，勿直接编辑本文件。*")
    add("")
    return "\n".join(L)


def render_index(all_missing: list) -> str:
    L = []
    add = L.append
    grouped = {}
    for s in SCENARIOS:
        grouped.setdefault(s["group"], []).append(s)

    add("---")
    add('title: "运维场景剧本总索引"')
    add('description: "工单剧本编排层总入口 — 20 个生产运维场景的可执行工作流导航"')
    add('summary: "按建设交付/稳定性/可靠性/安全合规/经营效率分组的 20 个运维场景工单剧本"')
    add("category: index")
    a_tags = ["index", "scenario", "playbook"]
    add("tags:")
    add("\n".join(f"- {t}" for t in a_tags))
    add("tier: core")
    add(f"created: '{YEAR_MONTH}'")
    add(f"updated: '{YEAR_MONTH}'")
    add(f"last_updated: '{TODAY}'")
    add("---")
    add("")
    add("# 运维场景剧本总索引")
    add("")
    add(f"> **剧本总数**: {len(SCENARIOS)} · **层次**: 知识使用侧编排层 · **更新**: {TODAY}")
    add("> **理念**: domain 讲原理、skills 给动作、FTA 管推导——本层把它们按真实场景串成可执行工作流。")
    add("> **使用**: 按触发条件进入剧本 → 过前置检查门槛 → 沿分支执行 → 跑完验证清单才可关单。")
    add("")
    for g in GROUP_ORDER:
        ss = grouped.get(g, [])
        if not ss:
            continue
        add(f"## {g}")
        add("")
        add("| ID | 剧本 | 一句话说明 |")
        add("|---|---|---|")
        for s in ss:
            rel = f"13-生产运维/08-运维场景剧本/{s['name']}"
            add(f"| {s['id']} | [[{rel}|{s['title']}]] | {s['description']} |")
        add("")
    add("## 场景间关系速览")
    add("")
    add("- 学习新人路径: SC-09 日常巡检 → SC-03 故障排查 → 其余专项")
    add("- 事故响应链: SC-03 总纲 ↔ SC-11/SC-12 专项 ↔ SC-13 安全事件")
    add("- 建设交付链: SC-01 部署 → SC-02 发布 → SC-15 GitOps → SC-08 升级")
    add("- 经营与容量: SC-14 容量 ↔ SC-19 成本 ↔ SC-09 巡检数据回流")
    add("")
    add("---")
    add("")
    add(f"*本索引由 `31-脚本/generate-scenarios.py` 于 {TODAY} 自动生成。*")
    add("")
    return "\n".join(L)


# ---------------------------------------------------------------------------
# 入口
# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description="生成运维场景工单剧本")
    ap.add_argument("--check-only", action="store_true", help="仅校验引用链接，不写文件")
    args = ap.parse_args()

    missing = []
    rendered = [(s, render_scenario(s, missing)) for s in SCENARIOS]
    index_md = render_index(missing)

    # scenario: 互引目标即为本批次产物, 单独补验名字合法
    for owner, ref in list(missing):
        pass  # 占位: 未来增加跨批次引用时在此扩展

    if missing:
        print(f"[FAIL] 发现 {len(missing)} 个失效引用:")
        for owner, ref in sorted(set(missing)):
            print(f"  - ({owner}) {ref}")
        return 2

    print(f"[OK] 引用校验通过: {len(SCENARIOS)} 场景 + 总索引, 0 死链")

    if args.check_only:
        print("[DONE] --check-only: 未写入任何文件")
        return 0

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for s, content in rendered:
        p = OUT_DIR / f"{s['name']}.md"
        p.write_text(content, encoding="utf-8")
        print(f"  write {p.relative_to(BASE_DIR)}")
    ipath = OUT_DIR / "index.md"
    ipath.write_text(index_md, encoding="utf-8")
    print(f"  write {ipath.relative_to(BASE_DIR)}")
    print(f"[DONE] 共写入 {len(SCENARIOS) + 1} 个文件 → {OUT_DIR.relative_to(BASE_DIR)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
