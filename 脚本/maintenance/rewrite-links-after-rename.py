#!/usr/bin/env python3
"""批量修复 .md 文件里的 wikilink 与相对路径。

覆盖两类语法:
  [[path|alias]]   → 替换 path 部分
  [text](path)     → 替换 path 部分

替换规则: 把旧前缀替换为新前缀（字符串前缀匹配，最长优先）。
"""
import os
import re
import sys

# (old_prefix, new_prefix)
# 同时覆盖 batch-2（语义合并）+ batch-4（去编号）的路径变更。
MAPPING = [
    # ===== batch-4: 去编号改中文 =====
    ("故障诊断/01-resource-troubleshooting/", "故障诊断/资源排障/"),
    ("故障诊断/03-advanced-troubleshooting/", "故障诊断/高级排障/"),
    ("故障诊断/00-core-troubleshooting/", "故障诊断/核心排障/"),
    ("故障诊断/02-infrastructure-troubleshooting/", "故障诊断/基础设施排障/"),
    ("故障诊断/04-jvm-tuning/", "故障诊断/JVM调优/"),
    ("故障诊断/tools/", "故障诊断/工具/"),
    ("故障诊断/topic-febm/", "故障诊断/FEBM方法论/"),
    ("故障诊断/topic-fta/", "故障诊断/FTA故障树/"),
    ("故障诊断/topic-skills/", "故障诊断/技能体系/"),
    ("故障诊断/topic-multi-fault-scenarios/", "故障诊断/多故障场景/"),
    ("故障诊断/topic-qa-corpus/", "故障诊断/QA语料/"),
    ("故障诊断/topic-structural-trouble-shooting/", "故障诊断/高级排障/"),

    ("可观测性/01-overview/", "可观测性/总览/"),
    ("可观测性/02-metrics/", "可观测性/指标/"),
    ("可观测性/03-logging/", "可观测性/日志/"),
    ("可观测性/04-tracing/", "可观测性/链路追踪/"),
    ("可观测性/05-alerting/", "可观测性/告警/"),
    ("可观测性/06-slo-sli/", "可观测性/SLO-SLI/"),
    ("可观测性/07-tools/", "可观测性/工具/"),

    ("可靠性/01-backup-recovery/", "可靠性/备份恢复/"),
    ("可靠性/02-disaster-recovery/", "可靠性/灾难恢复/"),
    ("可靠性/03-capacity-planning/", "可靠性/容量规划/"),
    ("可靠性/04-slo-sli/", "可观测性/SLO-SLI/"),
    ("可靠性/05-chaos-engineering/", "可靠性/混沌工程/"),
    ("可靠性/06-postmortem/", "可靠性/事后复盘/"),
    ("可靠性/07-sre-practices/", "可靠性/SRE实践/"),
    ("可靠性/08-performance-testing/", "可靠性/性能测试/"),
    ("可靠性/09-disaster-recovery-playbooks/", "可靠性/灾难恢复/"),

    ("安全/01-identity-access/", "安全/身份与访问/"),
    ("安全/02-network-security/", "安全/网络安全/"),
    ("安全/03-runtime-security/", "安全/运行时安全/"),
    ("安全/04-policy-governance/", "安全/策略治理/"),
    ("安全/05-supply-chain/", "安全/供应链/"),
    ("安全/06-compliance/", "安全/合规审计/"),
    ("安全/07-incident-response/", "生产运维/事件响应/"),

    ("生产运维/01-finops/", "生产运维/成本治理/"),
    ("生产运维/02-governance/", "生产运维/集群治理/"),
    ("生产运维/03-incident-response/", "生产运维/事件响应/"),
    ("生产运维/04-green-computing/", "生产运维/绿色计算/"),
    ("生产运维/ticket-cases/", "生产运维/工单案例/"),
    ("生产运维/reply-templates/", "生产运维/回复话术/"),

    ("网络/00-core-k8s-networking/", "网络/K8s网络核心/"),
    ("网络/01-fundamentals/", "网络/网络基础/"),
    ("网络/02-service-mesh/", "网络/服务网格/"),
    ("网络/03-api-gateway/", "网络/API网关/"),
    ("网络/04-ebpf/", "网络/eBPF/"),
    ("网络/99-attachments/", "网络/附件/"),
    ("网络/topic-terway/", "网络/Terway/"),

    ("专项技术/01-edge-computing/", "专项技术/边缘计算/"),
    ("专项技术/02-webassembly/", "专项技术/WebAssembly/"),
    ("专项技术/03-extensions/", "专项技术/扩展机制/"),
    ("专项技术/04-serverless/", "专项技术/无服务器/"),
    ("专项技术/05-ebpf-programming/", "网络/eBPF/"),

    ("集群基础/01-architecture-overview/", "集群基础/架构总览/"),
    ("集群基础/02-design-principles/", "集群基础/设计原则/"),
    ("集群基础/03-control-plane/", "集群基础/控制平面/"),
    ("集群基础/04-api-versions/", "集群基础/API版本/"),
    ("集群基础/05-kubectl/", "集群基础/kubectl/"),
    ("集群基础/06-upgrade-paths/", "集群基础/升级路径/"),
    ("集群基础/07-performance-tuning/", "集群基础/性能调优/"),

    ("AI基础设施/01-ai-infra/", "AI基础设施/基础设施/"),
    ("AI基础设施/02-ai-agents/", "AI基础设施/AI-Agents/"),
    ("AI基础设施/03-agent-runtime/", "AI基础设施/Agent运行时/"),
    ("AI基础设施/topic-ai-coding/", "AI基础设施/AI编码/"),

    ("存储/01-k8s-storage/", "存储/K8s存储/"),
    ("存储/02-storage-fundamentals/", "存储/存储基础/"),
    ("存储/03-distributed-storage/", "存储/分布式存储/"),
    ("存储/04-stateful-app-storage/", "存储/有状态应用存储/"),

    ("清单模式/01-yaml-reference/", "清单模式/YAML参考/"),
    ("清单模式/02-kustomize-patterns/", "清单模式/Kustomize模式/"),
    ("清单模式/03-helm-values-patterns/", "清单模式/Helm值模式/"),

    ("工作负载/00-core-workloads/", "工作负载/核心工作负载/"),
    ("工作负载/topic-java-kubernetes/", "工作负载/Java-on-K8s/"),
    ("工作负载/topic-functions/", "平台工程/代码分析/functions-"),

    ("系统基础/02-hardware/", "系统基础/硬件/"),
    ("系统基础/03-kubernetes-events/", "系统基础/K8s事件/"),
    ("系统基础/topic-cheat-sheet/", "系统基础/速查卡/"),
    ("系统基础/topic-dictionary/", "系统基础/知识字典/"),

    ("平台工程/build/", "平台工程/构建/"),
    ("平台工程/operate/", "平台工程/运维/"),
    ("平台工程/governance/", "平台工程/治理/"),
    ("平台工程/developer-experience/", "平台工程/开发体验/"),
    ("平台工程/topic-code-analysis/", "平台工程/代码分析/"),

    ("数据库中间件/01-databases/", "数据库中间件/数据库/"),
    ("数据库中间件/02-cache/", "数据库中间件/缓存/"),
    ("数据库中间件/03-message-queues/", "数据库中间件/消息队列/"),
    ("数据库中间件/04-time-series-db/", "数据库中间件/时序数据库/"),
    ("数据库中间件/05-operator-management/", "数据库中间件/Operator管理/"),
    ("数据库中间件/06-data-streaming/", "数据库中间件/数据流/"),

    ("发布变更/01-gitops/", "发布变更/GitOps/"),
    ("发布变更/02-iac/", "发布变更/IaC/"),
    ("发布变更/03-change-management/", "发布变更/变更管理/"),
    ("发布变更/04-testing-quality/", "发布变更/测试质量/"),
    ("发布变更/topic-deployment/", "发布变更/部署方案/"),
    ("发布变更/topic-migration/", "发布变更/迁移方案/"),

    ("应用模式/sub-patterns/", "应用模式/子模式/"),
    ("应用模式/topic-application-architecture/", "应用模式/行业架构/"),
    ("应用模式/topic-production-patterns/", "应用模式/生产模式/"),

    ("生态参考/01-cncf-landscape/", "生态参考/CNCF全景/"),
    ("生态参考/02-papers/", "生态参考/论文/"),
    ("生态参考/topic-index/", "生态参考/领域索引/"),
    ("生态参考/topic-release-notes/", "生态参考/领域索引/"),
    ("生态参考/_archived-release-notes/", "_archives/release-notes/"),

    ("容器运行时/01-docker/", "容器运行时/Docker/"),
    ("容器运行时/02-image-management/", "容器运行时/镜像管理/"),
    ("容器运行时/03-containerd-cri-o/", "容器运行时/containerd-CRI-O/"),
    ("容器运行时/04-image-build/", "容器运行时/镜像构建/"),
    ("容器运行时/05-runtime-migration/", "容器运行时/运行时迁移/"),

    # ===== batch-2: 云厂商归一 =====
    ("云厂商/01-alibaba-cloud/", "云厂商/阿里云/"),
    ("云厂商/02-aws-eks/", "云厂商/AWS-EKS/"),
    ("云厂商/03-google-cloud-gke/", "云厂商/Google-GKE/"),
    ("云厂商/04-azure-aks/", "云厂商/Azure-AKS/"),
    ("云厂商/05-alicloud-ack/", "云厂商/阿里云/ack/"),
    ("云厂商/06-tencent-tke/", "云厂商/腾讯云TKE/"),
    ("云厂商/07-huawei-cce/", "云厂商/华为云CCE/"),
    ("云厂商/08-multi-cloud/", "云厂商/多云混合/"),
    ("云厂商/09-ucloud-uk8s/", "云厂商/其他云/UCloud-UK8S/"),
    ("云厂商/10-ibm-iks/", "云厂商/其他云/IBM-IKS/"),
    ("云厂商/11-oracle-oke/", "云厂商/其他云/Oracle-OKE/"),
    ("云厂商/12-volcengine-vek/", "云厂商/其他云/火山引擎-VEK/"),
    ("云厂商/13-ctyun-tke/", "云厂商/其他云/天翼云-TKE/"),
    ("云厂商/14-ecloud-cke/", "云厂商/其他云/移动云-CKE/"),
    ("云厂商/15-alicloud-apsara-ack/", "云厂商/阿里云/apsara/"),

    # ===== 98-merged-indexes 归档 =====
    # 这些路径基本不会出现在正文里，但以防万一指向旧索引
]

# 按长度倒序，确保最长前缀优先匹配
MAPPING.sort(key=lambda x: -len(x[0]))

WIKILINK_RE = re.compile(r"\[\[([^\]\|]+)(\|[^\]]+)?\]\]")
MDLINK_RE = re.compile(r"\[([^\]]*)\]\(([^)]+)\)")


# 单独子目录段映射（用于匹配路径中间的旧子目录名，例如 ../01-resource-troubleshooting/x.md）
# 注意: 仅当该段在路径中作为独立目录段出现时才替换，避免误改文件名。
SEGMENT_RENAME = {
    "01-resource-troubleshooting": "资源排障",
    "03-advanced-troubleshooting": "高级排障",
    "00-core-troubleshooting": "核心排障",
    "02-infrastructure-troubleshooting": "基础设施排障",
    "04-jvm-tuning": "JVM调优",
    "topic-febm": "FEBM方法论",
    "topic-fta": "FTA故障树",
    "topic-skills": "技能体系",
    "topic-multi-fault-scenarios": "多故障场景",
    "topic-qa-corpus": "QA语料",
    "topic-structural-trouble-shooting": "高级排障",
    "01-overview": "总览",
    "02-metrics": "指标",
    "03-logging": "日志",
    "04-tracing": "链路追踪",
    "05-alerting": "告警",
    "06-slo-sli": "SLO-SLI",
    "07-tools": "工具",
    "01-backup-recovery": "备份恢复",
    "02-disaster-recovery": "灾难恢复",
    "03-capacity-planning": "容量规划",
    "04-slo-sli": "SLO-SLI",
    "05-chaos-engineering": "混沌工程",
    "06-postmortem": "事后复盘",
    "07-sre-practices": "SRE实践",
    "08-performance-testing": "性能测试",
    "09-disaster-recovery-playbooks": "灾难恢复",
    "01-identity-access": "身份与访问",
    "02-network-security": "网络安全",
    "03-runtime-security": "运行时安全",
    "04-policy-governance": "策略治理",
    "05-supply-chain": "供应链",
    "06-compliance": "合规审计",
    "07-incident-response": "事件响应",
    "01-finops": "成本治理",
    "02-governance": "集群治理",
    "03-incident-response": "事件响应",
    "04-green-computing": "绿色计算",
    "ticket-cases": "工单案例",
    "reply-templates": "回复话术",
    "00-core-k8s-networking": "K8s网络核心",
    "01-fundamentals": "网络基础",
    "02-service-mesh": "服务网格",
    "03-api-gateway": "API网关",
    "04-ebpf": "eBPF",
    "99-attachments": "附件",
    "topic-terway": "Terway",
    "01-edge-computing": "边缘计算",
    "02-webassembly": "WebAssembly",
    "03-extensions": "扩展机制",
    "04-serverless": "无服务器",
    "05-ebpf-programming": "eBPF",
    "01-architecture-overview": "架构总览",
    "02-design-principles": "设计原则",
    "03-control-plane": "控制平面",
    "04-api-versions": "API版本",
    "05-kubectl": "kubectl",
    "06-upgrade-paths": "升级路径",
    "07-performance-tuning": "性能调优",
    "01-ai-infra": "基础设施",
    "02-ai-agents": "AI-Agents",
    "03-agent-runtime": "Agent运行时",
    "topic-ai-coding": "AI编码",
    "01-k8s-storage": "K8s存储",
    "02-storage-fundamentals": "存储基础",
    "03-distributed-storage": "分布式存储",
    "04-stateful-app-storage": "有状态应用存储",
    "01-yaml-reference": "YAML参考",
    "02-kustomize-patterns": "Kustomize模式",
    "03-helm-values-patterns": "Helm值模式",
    "00-core-workloads": "核心工作负载",
    "topic-java-kubernetes": "Java-on-K8s",
    "02-hardware": "硬件",
    "03-kubernetes-events": "K8s事件",
    "topic-cheat-sheet": "速查卡",
    "topic-dictionary": "知识字典",
    "build": "构建",
    "operate": "运维",
    "governance": "治理",
    "developer-experience": "开发体验",
    "topic-code-analysis": "代码分析",
    "01-databases": "数据库",
    "02-cache": "缓存",
    "03-message-queues": "消息队列",
    "04-time-series-db": "时序数据库",
    "05-operator-management": "Operator管理",
    "06-data-streaming": "数据流",
    "01-gitops": "GitOps",
    "02-iac": "IaC",
    "03-change-management": "变更管理",
    "04-testing-quality": "测试质量",
    "topic-deployment": "部署方案",
    "topic-migration": "迁移方案",
    "sub-patterns": "子模式",
    "topic-application-architecture": "行业架构",
    "topic-production-patterns": "生产模式",
    "01-cncf-landscape": "CNCF全景",
    "02-papers": "论文",
    "topic-index": "领域索引",
    "topic-release-notes": "领域索引",
    "_archived-release-notes": "_archives/release-notes",
    "01-docker": "Docker",
    "02-image-management": "镜像管理",
    "03-containerd-cri-o": "containerd-CRI-O",
    "04-image-build": "镜像构建",
    "05-runtime-migration": "运行时迁移",
    "01-alibaba-cloud": "阿里云",
    "02-aws-eks": "AWS-EKS",
    "03-google-cloud-gke": "Google-GKE",
    "04-azure-aks": "Azure-AKS",
    "05-alicloud-ack": "阿里云/ack",
    "06-tencent-tke": "腾讯云TKE",
    "07-huawei-cce": "华为云CCE",
    "08-multi-cloud": "多云混合",
    "09-ucloud-uk8s": "其他云/UCloud-UK8S",
    "10-ibm-iks": "其他云/IBM-IKS",
    "11-oracle-oke": "其他云/Oracle-OKE",
    "12-volcengine-vek": "其他云/火山引擎-VEK",
    "13-ctyun-tke": "其他云/天翼云-TKE",
    "14-ecloud-cke": "其他云/移动云-CKE",
    "15-alicloud-apsara-ack": "阿里云/apsara",
}


def apply_segment_rename(path):
    """把 path 中任何匹配 SEGMENT_RENAME 的目录段替换为新名。

    支持形式: 'old/x', '../old/x', '../../old/x', './old/x', 'foo/old/x',
              '中文域/old/x', '中文域/old' (无尾部 slash), '[[old/x]]'。
    """
    # 把路径按 '/' 切分，逐段替换；保留前导 '../' / './'
    changed = False
    # 用正则找每个路径段
    def _repl(m):
        nonlocal changed
        seg = m.group(0)
        if seg in SEGMENT_RENAME:
            changed = True
            return SEGMENT_RENAME[seg]
        return seg
    # 仅替换路径中作为目录段出现（后面紧跟 / 或结尾）的旧名
    # 先处理中间段：'old/' -> 'new/'
    new_path = re.sub(r"(?<![\w\-])([A-Za-z0-9_\-]+)(?=/)", lambda m: _repl_match(m), path)
    # 再处理末尾段（没有尾部 /），仅在前面是 '/' 的情况下
    def _tail(m):
        nonlocal changed
        seg = m.group(1)
        if seg in SEGMENT_RENAME:
            changed = True
            return "/" + SEGMENT_RENAME[seg]
        return m.group(0)
    new_path = re.sub(r"/([\w\-]+)$", _tail, new_path)
    return new_path


def _repl_match(m):
    seg = m.group(1)
    if seg in SEGMENT_RENAME:
        return SEGMENT_RENAME[seg]
    return seg


def rewrite_wikilink(m):
    target = m.group(1)
    alias = m.group(2) or ""
    # 优先用长前缀（域名+子目录）映射
    matched = False
    for old, new in MAPPING:
        if target.startswith(old):
            target = new + target[len(old):]
            matched = True
            break
        old_no_slash = old.rstrip("/")
        if target == old_no_slash or target.startswith(old_no_slash + "/"):
            target = new.rstrip("/") + target[len(old_no_slash):]
            matched = True
            break
    if not matched:
        # 兜底：按目录段重命名（处理 ../old/x 或 old/x 形式）
        target = apply_segment_rename(target)
    return f"[[{target}{alias}]]"


def rewrite_mdlink(m):
    text, href = m.group(1), m.group(2)
    if href.startswith(("http://", "https://", "mailto:", "#", "<")):
        return m.group(0)
    matched = False
    for old, new in MAPPING:
        if href.startswith(old):
            href = new + href[len(old):]
            matched = True
            break
        if href.startswith(old.rstrip("/") + ".md"):
            href = new.rstrip("/") + ".md" + href[len(old.rstrip("/") + ".md"):]
            matched = True
            break
    if not matched:
        href = apply_segment_rename(href)
    return f"[{text}]({href})"


def rewrite_file(path):
    try:
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
    except UnicodeDecodeError:
        return 0
    new = WIKILINK_RE.sub(rewrite_wikilink, content)
    new = MDLINK_RE.sub(rewrite_mdlink, new)
    if new != content:
        with open(path, "w", encoding="utf-8") as f:
            f.write(new)
        return 1
    return 0


def main():
    root = os.path.abspath(".")
    changed = 0
    scanned = 0
    skip_dirs = {".git", ".venv", "node_modules", "__pycache__", ".obsidian", ".ruff_cache",
                 "_archives", "code"}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip_dirs]
        for fn in filenames:
            if not fn.endswith(".md"):
                continue
            full = os.path.join(dirpath, fn)
            scanned += 1
            changed += rewrite_file(full)
    print(f"scanned={scanned} changed={changed}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
