#!/usr/bin/env python3
# ruff: noqa: E501  # 一次性映射脚本，映射表保持单行可读性
"""
2026-07-27 wikilink 目标重映射（对应 P0-1 项收尾）：
将指向已重命名/已迁移/目录本身的 wikilink 重定向到现存文件。
映射表为人工核对的显式清单；应用前逐条校验目标 stem 存在，缺失则跳过并告警。

用法: python3 code/31-脚本/maintenance/remap-wikilinks-20260727.py [--dry-run]
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
EXCL = {'node_modules', '.venv', '.git', '__pycache__', '30-站点', '33-源码', '32-发布',
        '37-归档', '36-报告', '35-元数据', '28-资产', '31-脚本'}
CONTENT = {'22-概念', '23-实体', '26-技能', '24-综合', '29-文档', '20-最佳实践', '25-研究', '27-标签'}
DOMAINS = {'01-集群基础', '02-工作负载', '05-网络', '06-存储', '08-安全', '09-可观测性',
           '10-平台工程', '11-发布变更', '12-可靠性', '19-故障诊断', '13-生产运维', '18-云厂商',
           '14-容器运行时', '15-AI基础设施', '16-专项技术', '07-数据库中间件', '17-系统基础',
           '03-清单模式', '21-生态参考', '04-应用模式'}

# 旧目标(链接原文中的 target 部分) -> 新目标路径（不含 .md）
REMAP = {
    # --- 目录链接 -> 目录 index ---
    'wiki/12-可靠性/01-备份恢复': 'wiki/12-可靠性/01-备份恢复/index',
    'wiki/12-可靠性/02-灾难恢复': 'wiki/12-可靠性/02-灾难恢复/index',
    'wiki/12-可靠性/03-容量规划': 'wiki/12-可靠性/03-容量规划/index',
    'wiki/12-可靠性/04-混沌工程': 'wiki/12-可靠性/04-混沌工程/index',
    'wiki/12-可靠性/05-事后复盘': 'wiki/12-可靠性/05-事后复盘/index',
    'wiki/09-可观测性/06-SLO-SLI': 'wiki/09-可观测性/06-SLO-SLI/01-slo-engineering-practice',
    'wiki/06-存储/03-分布式存储': 'wiki/06-存储/03-分布式存储/index',
    'wiki/15-AI基础设施/05-K8s-AI基础设施': 'wiki/15-AI基础设施/05-K8s-AI基础设施/index',
    'wiki/19-故障诊断/06-FTA故障树/list': 'wiki/19-故障诊断/06-FTA故障树/fta-index',
    'wiki/13-生产运维/升级策略': 'wiki/01-集群基础/06-升级路径/index',
    'wiki/01-集群基础/节点管理': 'wiki/22-概念/08-可靠性与运维/node-lifecycle-management',
    'wiki/02-工作负载/pod-scheduling': 'wiki/22-概念/07-调度与资源/scheduling-algorithm',
    # --- structural- 前缀重命名 ---
    'wiki/19-故障诊断/04-高级排障/00-configuration-first-methodology.md':
        'wiki/19-故障诊断/04-高级排障/structural-00-configuration-first-methodology',
    'wiki/19-故障诊断/04-高级排障/09-dra-troubleshooting.md':
        'wiki/19-故障诊断/04-高级排障/structural-09-dra-troubleshooting',
    'wiki/19-故障诊断/04-高级排障/10-etcd-maintenance.md':
        'wiki/19-故障诊断/04-高级排障/structural-10-etcd-maintenance',
    # --- FTA / FEBM 旧命名 ---
    'wiki/19-故障诊断/06-FTA故障树/01-fta-methodology-overview':
        'wiki/19-故障诊断/06-FTA故障树/fta-methodology-and-agentic-practices',
    'wiki/19-故障诊断/06-FTA故障树/02-pod-failure-fta': 'wiki/19-故障诊断/06-FTA故障树/list/pod-fta',
    'wiki/19-故障诊断/06-FTA故障树/03-node-failure-fta': 'wiki/19-故障诊断/06-FTA故障树/list/node-fta',
    'wiki/19-故障诊断/06-FTA故障树/04-network-failure-fta': 'wiki/19-故障诊断/06-FTA故障树/list/networkpolicy-fta',
    'wiki/19-故障诊断/06-FTA故障树/05-storage-failure-fta': 'wiki/19-故障诊断/06-FTA故障树/list/csi-fta',
    'wiki/19-故障诊断/07-FEBM方法论/01-febm-methodology-overview':
        'wiki/19-故障诊断/07-FEBM方法论/01-febm-theory-foundations',
    '01-node-notready-diagnosis.md': 'wiki/19-故障诊断/01-核心排障/06-node-notready-diagnosis',
    'node-notready': 'wiki/19-故障诊断/08-技能体系/01-node-notready',
    # --- 12-可靠性 旧命名（wiki/27-标签/sre.md 等）---
    'wiki/12-可靠性/03-容量规划/01-capacity-planning-methodology': 'wiki/12-可靠性/03-容量规划/01-capacity-planning-framework',
    'wiki/12-可靠性/03-容量规划/02-resource-forecasting-models': 'wiki/12-可靠性/03-容量规划/24-capacity-planning-forecasting',
    'wiki/12-可靠性/04-混沌工程/01-chaos-engineering-principles': 'wiki/12-可靠性/04-混沌工程/01-chaos-engineering-overview',
    'wiki/12-可靠性/04-混沌工程/02-chaos-mesh-practice': 'wiki/12-可靠性/04-混沌工程/02-chaos-mesh-deployment',
    'wiki/12-可靠性/04-混沌工程/03-litmus-chaos-experiments': 'wiki/12-可靠性/04-混沌工程/04-litmus-practices',
    'wiki/12-可靠性/05-事后复盘/01-blameless-postmortem-guide': 'wiki/12-可靠性/05-事后复盘/01-blameless-postmortem-template',
    'wiki/12-可靠性/05-事后复盘/02-postmortem-template': 'wiki/12-可靠性/05-事后复盘/03-incident-postmortem-template',
    'wiki/12-可靠性/05-事后复盘/03-incident-review-process': 'wiki/12-可靠性/05-事后复盘/02-postmortem-culture-guide',
    'wiki/13-生产运维/03-事件响应/04-incident-postmortem-template.md': 'wiki/12-可靠性/05-事后复盘/03-incident-postmortem-template',
    # --- 10-平台工程 / 11-发布变更 旧命名 ---
    'wiki/10-平台工程/01-构建/02-idp-architecture-design': 'wiki/10-平台工程/01-构建/02-idp-design-principles',
    'wiki/10-平台工程/01-构建/04-developer-portal-patterns': 'wiki/10-平台工程/01-构建/04-backstage-catalog-techdocs',
    'wiki/10-平台工程/03-治理/01-platform-governance-model': 'wiki/10-平台工程/03-治理/index',
    'wiki/10-平台工程/03-治理/02-policy-as-code-governance': 'wiki/25-研究/02-网络与安全/policy-as-code-security',
    'wiki/10-平台工程/03-治理/03-cost-governance-optimization': 'wiki/10-平台工程/03-治理/09-cost-optimization-finops',
    'wiki/10-平台工程/04-开发体验/01-developer-experience-overview': 'wiki/10-平台工程/04-开发体验/27-developer-experience-engineering',
    'wiki/10-平台工程/04-开发体验/02-self-service-infrastructure': 'wiki/10-平台工程/04-开发体验/index',
    'wiki/10-平台工程/04-开发体验/03-golden-path-templates': 'wiki/10-平台工程/01-构建/08-golden-paths-design',
    'wiki/11-发布变更/01-GitOps/01-argocd-enterprise-gitops': 'wiki/11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops',
    'wiki/11-发布变更/01-GitOps/02-flux-enterprise-gitops': 'wiki/11-发布变更/01-GitOps/06-flux-gitops-continuous-delivery',
    'wiki/11-发布变更/02-IaC/01-terraform-kubernetes-infrastructure': 'wiki/11-发布变更/02-IaC/01-terraform-enterprise-iac',
    'wiki/11-发布变更/02-IaC/02-crossplane-cloud-native-iac': 'wiki/11-发布变更/02-IaC/05-crossplane-enterprise-orchestration',
    'wiki/10-平台工程/06-代码分析/functions-MOC': 'wiki/10-平台工程/06-代码分析/MOC',
    'wiki/10-平台工程/06-代码分析/deployment-create/总览': 'wiki/10-平台工程/06-代码分析/deployment-create/01-overview',
    'wiki/10-平台工程/06-代码分析/cluster-create/总览': 'wiki/10-平台工程/06-代码分析/cluster-create/01-overview',
    'wiki/10-平台工程/06-代码分析/cluster-delete/总览': 'wiki/10-平台工程/06-代码分析/cluster-delete/01-overview',
    'wiki/10-平台工程/06-代码分析/node-create/总览': 'wiki/10-平台工程/06-代码分析/node-create/01-overview',
    'wiki/10-平台工程/06-代码分析/functions-node-create/总览': 'wiki/10-平台工程/06-代码分析/functions-node-create/01-overview',
    # --- wiki/15-AI基础设施/05 旧文件名 ---
    'wiki/15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-deployment.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-sharing-patterns',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/02-gpu-sharing-mig-timeslicing.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-sharing-patterns',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/03-vllm-production-deployment.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/02-vllm-inference-serving-production',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/04-triton-inference-server.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/03-tgi-triton-tensorrt-serving',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/05-kserve-model-serving.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/04-kserve-model-serving-platform',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/06-volcano-batch-scheduler.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/06-training-operators-volcano-mpi',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/07-kuberay-distributed-training.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/05-kuberay-distributed-computing',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/08-finetuning-infrastructure.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/10-finetuning-peft-lora-deepspeed',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/09-vector-database-k8s.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/08-vector-database-k8s-milvus-qdrant',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/10-rdma-high-performance-networking.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/09-rdma-infiniband-gpudirect-networking',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/11-ai-observability-stack.md': 'wiki/15-AI基础设施/05-K8s-AI基础设施/12-ai-observability-arize-phoenix',
    'wiki/15-AI基础设施/05-K8s-AI基础设施/12-ai-workload-security.md': 'wiki/15-AI基础设施/01-基础设施/11-ai-security-model-protection',
    'wiki/15-AI基础设施/01-基础设施/10-model-deployment-serving': 'wiki/15-AI基础设施/05-K8s-AI基础设施/04-kserve-model-serving-platform',
    # --- 中文概念链接（相关阅读）---
    'GPU调度与资源管理': 'wiki/15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving',
    'Kubeflow训练平台': 'wiki/23-实体/11-AI与边缘/kubeflow',
    'Prometheus监控体系': 'wiki/23-实体/07-可观测性/prometheus',
    'vLLM推理引擎部署': 'wiki/15-AI基础设施/05-K8s-AI基础设施/02-vllm-inference-serving-production',
    'K8s资源配额与LimitRange': 'wiki/17-系统基础/06-知识字典/configuration/resource-quota',
    'AI集群多租户隔离': 'wiki/15-AI基础设施/05-K8s-AI基础设施/14-gpu-cost-attribution-multitenant',
    'K8s存储与PV管理': 'wiki/17-系统基础/06-知识字典/storage/persistent-volume',
    'Volcano批量调度': 'wiki/23-实体/09-编排调度/volcano',
    'RAG知识库架构': 'wiki/15-AI基础设施/05-K8s-AI基础设施/08-vector-database-k8s-milvus-qdrant',
    'K8s有状态服务运维': 'wiki/22-概念/02-工作负载/statefulset',
    'K8s网络策略与CNI': 'wiki/23-实体/02-K8s核心组件/networkpolicy',
    'K8s节点管理与运维': 'wiki/22-概念/08-可靠性与运维/node-lifecycle-management',
    'Kueue与YuniKorn批量调度': 'wiki/15-AI基础设施/05-K8s-AI基础设施/07-batch-scheduling-kueue-yunikorn',
    'AI高性能网络': 'wiki/15-AI基础设施/05-K8s-AI基础设施/09-rdma-infiniband-gpudirect-networking',
    'K8s Ingress与流量管理': 'wiki/22-概念/03-网络/ingress',
    'AI可观测性平台': 'wiki/15-AI基础设施/05-K8s-AI基础设施/12-ai-observability-arize-phoenix',
    'Agent可观测性': 'wiki/15-AI基础设施/05-K8s-AI基础设施/12-ai-observability-arize-phoenix',
    'LLM Gateway与推理路由': 'wiki/15-AI基础设施/05-K8s-AI基础设施/11-llm-gateway-routing-cost',
    '可观测性': 'wiki/09-可观测性/README',
    '网络': 'wiki/05-网络/README',
    '安全': 'wiki/08-安全/README',
    'AI基础设施': 'wiki/15-AI基础设施/README',
    # --- 概念/实体缺页 -> 现存等价页 ---
    'incident-management': 'wiki/22-概念/08-可靠性与运维/incident-management-patterns',
    'error-budget': 'wiki/09-可观测性/06-SLO-SLI/02-error-budget-policy',
    'internal-developer-platform': 'wiki/22-概念/13-research-2025-2026/07-Platform-Engineering',
    'wiki/22-概念/gitops-deployment.md': 'wiki/22-概念/12-研究/gitops-tool-evolution',
    'wiki/25-研究/multi-cluster-security-governance.md': 'wiki/25-研究/02-网络与安全/zero-trust-k8s-security',
    # --- 知识字典缺条目 -> 现存等价页 ---
    'wiki/17-系统基础/06-知识字典/security/zero-trust.md': 'wiki/08-安全/07-零信任架构/index',
    'wiki/17-系统基础/06-知识字典/observability/hubble.md': 'wiki/23-实体/04-网络/cilium',
    'wiki/17-系统基础/06-知识字典/operations/alerting.md': 'wiki/09-可观测性/05-告警/index',
    'wiki/17-系统基础/06-知识字典/platform-engineering/slo.md': 'wiki/09-可观测性/06-SLO-SLI/01-slo-engineering-practice',
    'wiki/17-系统基础/06-知识字典/security/audit-logging.md': 'wiki/09-可观测性/03-日志/12-logging-auditing',
    'wiki/17-系统基础/06-知识字典/observability/audit-logging.md': 'wiki/09-可观测性/03-日志/12-logging-auditing',
    'wiki/17-系统基础/06-知识字典/platform-engineering/admission-webhook.md': 'wiki/17-系统基础/06-知识字典/security/admission-controller',
    'wiki/17-系统基础/06-知识字典/workloads/job-cronjob.md': 'wiki/02-工作负载/01-核心工作负载/05-job-cronjob-advanced',
    'wiki/17-系统基础/06-知识字典/storage/storageclasses.md': 'wiki/17-系统基础/06-知识字典/storage/persistent-volume',
    'audit-logging': 'wiki/09-可观测性/03-日志/12-logging-auditing',
}

WIKI = re.compile(r'\[\[([^\[\]]+?)\]\]')
INLINE_CODE = re.compile(r'`[^`]*`')


def is_content(rel):
    return rel.parts and (rel.parts[0] in DOMAINS or rel.parts[0] in CONTENT)


def main() -> None:
    dry = '--dry-run' in sys.argv
    # 校验映射目标存在
    stems = set()
    for p in ROOT.rglob('*.md'):
        rel = p.relative_to(ROOT)
        if rel.parts[0] in {'node_modules', '30-站点', '33-源码', '32-发布'} or rel.parts[0].startswith('.'):
            continue
        stems.add(p.stem)
    valid = {}
    for old, new in REMAP.items():
        if new.split('/')[-1] in stems:
            valid[old] = new
        else:
            print(f"[warn] 映射目标不存在, 跳过: {old} -> {new}")

    n_files = n_hits = 0
    for p in sorted(ROOT.rglob('*.md')):
        rel = p.relative_to(ROOT)
        if any(x in rel.parts for x in EXCL) or not is_content(rel):
            continue
        lines = p.read_text(encoding='utf-8', errors='ignore').split('\n')
        in_fence = False
        changed = False
        for i, line in enumerate(lines):
            if line.lstrip().startswith('```'):
                in_fence = not in_fence
                continue
            if in_fence or '[[' not in line:
                continue
            is_table = line.lstrip().startswith('|')
            masked = INLINE_CODE.sub(lambda m: '\x00' * len(m.group()), line)

            def fix(m):
                nonlocal changed, n_hits
                inner = m.group(1)
                if '\x00' in inner:
                    return m.group(0)
                escaped = '\\|' in inner
                if escaped:
                    target, alias = inner.split('\\|', 1)
                elif '|' in inner:
                    target, alias = inner.split('|', 1)
                else:
                    target, alias = inner, None
                heading = None
                if '#' in target:
                    target, heading = target.split('#', 1)
                key = target.strip()
                if key not in valid:
                    return m.group(0)
                new_target = valid[key]
                if heading:
                    new_target += f'#{heading}'
                if alias is None:
                    disp = key.split('/')[-1]
                    alias = disp[:-3] if disp.endswith('.md') else disp
                sep = '\\|' if (escaped or is_table) else '|'
                changed = True
                n_hits += 1
                return f'[[{new_target}{sep}{alias}]]'

            out, pos = [], 0
            for m in WIKI.finditer(masked):
                out.append(line[pos:m.start()])
                out.append(fix(m))
                pos = m.end()
            out.append(line[pos:])
            lines[i] = ''.join(out)
        if changed:
            n_files += 1
            if not dry:
                p.write_text('\n'.join(lines), encoding='utf-8')
            print(f"[fix] {rel}")
    print(f"\n重映射: {n_hits} 处 / {n_files} 个文件")


if __name__ == '__main__':
    main()
