#!/usr/bin/env python3
# ruff: noqa: E501  # 一次性映射脚本，映射表保持单行可读性
"""P0-1 最终收尾：多义/无匹配 wikilink 显式重映射（2026-07-27）

处理 remap-title-links 无法自动解决的两类目标：
1. 多义标题（多文件同标题）→ 人工选定规范目标（优先 20-最佳实践/01-best-practices
   规范层与 22-概念/23-实体 提炼层）
2. 无标题匹配（旧目录链接、异名标题）→ 人工核对现存等价页

所有映射目标在应用前校验存在性，缺失则告警且跳过。

用法:
    python3 remap-final-20260727.py --dry-run
    python3 remap-final-20260727.py
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
EXCL = {'node_modules', '.venv', '.git', '__pycache__', '30-站点', '32-发布', '33-源码'}
SCAN_EXCL = EXCL | {'37-归档', '36-报告', '35-元数据', '28-资产', '31-脚本'}
WIKI = re.compile(r'\[\[([^\[\]]+?)\]\]')

# 断链目标 -> 现存文件相对路径（无扩展名）
REMAP = {
    # --- 多义标题：选定规范目标 ---
    'Kubernetes 存储配置最佳实践': '20-最佳实践/01-best-practices/infrastructure/storage',
    'Kubernetes 灾难恢复最佳实践': '20-最佳实践/01-best-practices/operations/disaster-recovery',
    'Kubernetes 部署策略最佳实践': '20-最佳实践/01-best-practices/operations/deployment',
    'Kubernetes 通用最佳实践参考': '20-最佳实践/01-best-practices/common-best-practices',
    'Kubernetes 集群配置最佳实践': '20-最佳实践/01-best-practices/infrastructure/kubernetes-cluster',
    'Kubernetes 网络配置最佳实践': '20-最佳实践/01-best-practices/infrastructure/networking',
    'Kubernetes 日志管理最佳实践': '20-最佳实践/01-best-practices/observability/logging',
    'AI Agent 工程专题': '15-AI基础设施/02-AI-Agents/README',
    'Pod CrashLoopBackOff & OOMKilled 诊断与修复': '19-故障诊断/08-技能体系/02-pod-crashloop-oomkilled',
    'Container Runtime': '22-概念/15-运行时与系统/container-runtime',
    'HashiCorp Vault': '23-实体/06-安全/vault',
    'PDB 异常故障树分析': '19-故障诊断/06-FTA故障树/list/pdb-fta',
    'API Server 异常故障树分析': '19-故障诊断/06-FTA故障树/list/apiserver-fta',
    'Pod Lifecycle': '22-概念/02-工作负载/pod-lifecycle',
    # --- 无标题匹配：人工核对等价页 ---
    'Docker & Containerd 速查卡': '17-系统基础/05-速查卡/docker',
    '网络诊断速查卡': '17-系统基础/05-速查卡/networking',
    'KUDIG 故障排查 Prompt 模板': '23-实体/15-参考与索引/kudig-prompts-catalog',
    'KUDIG Database — Global MOC': 'README',
    'KUDIG Database': 'README',
    'urunc (Unikernel Container Runtime)': '23-实体/03-运行时/urunc',
    'KAITO (Kubernetes AI Toolchain Operator)': '23-实体/11-AI与边缘/kaito',
    'Secrets Management': '08-安全/07-零信任架构/02-secrets-management-deep-dive',
    'Manage Persistent Storage': '26-技能/06-存储/csi-storage/manage-persistent-storage',
    '15-AI基础设施/03-inference-serving/': '15-AI基础设施/01-基础设施/17-llm-inference-serving',
    '15-AI基础设施/02-gpu-scheduling/': '15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving',
    '09-可观测性/03-tracing/': '09-可观测性/04-链路追踪/index',
    '22-概念/internal-developer-platform': '22-概念/09-平台与发布/platform-engineering-idp',
    '22-概念/error-budget': '22-概念/08-可靠性与运维/slo-error-budget-framework',
    '22-概念/incident-management': '22-概念/08-可靠性与运维/incident-management-patterns',
    '22-概念/troubleshooting-methodology': '19-故障诊断/00-总览/03-systematic-troubleshooting-methodology',
    '22-概念/root-cause-analysis': '23-实体/15-参考与索引/fta-febm-methodology',
    '19-故障诊断/07-FEBM方法论/02-evidence-collection-guide': '19-故障诊断/07-FEBM方法论/02-febm-technical-implementation',
    '19-故障诊断/07-FEBM方法论/03-hypothesis-verification': '19-故障诊断/07-FEBM方法论/01-febm-theory-foundations',
    '19-故障诊断/01-核心排障/01-pod-lifecycle-troubleshooting': '19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting',
    '19-故障诊断/01-核心排障/02-scheduling-resource-troubleshooting': '19-故障诊断/01-核心排障/05-pod-pending-diagnosis',
    '19-故障诊断/01-核心排障/05-control-plane-troubleshooting': '19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting',
}


def parse_inner(inner: str):
    escaped = '\\|' in inner
    work = inner.replace('\\|', '\x01')
    if '|' in work:
        target, alias = work.split('|', 1)
    elif '\x01' in work:
        target, alias = work.split('\x01', 1)
    else:
        target, alias = work, None
    heading = None
    if '#' in target:
        target, heading = target.split('#', 1)
    return target.strip().rstrip('\\'), heading, (alias.replace('\x01', '|') if alias else None), escaped


def main():
    dry = '--dry-run' in sys.argv
    # 目标存在性校验
    warn = 0
    for src, dst in REMAP.items():
        if not (ROOT / (dst + '.md')).exists():
            print(f"[WARN] 目标不存在: {src} -> {dst}")
            warn += 1
    if warn:
        print(f"共 {warn} 条映射目标缺失，请修正后重试")
        sys.exit(1)

    changed_files = 0
    changed_links = 0
    for p in sorted(ROOT.rglob('*.md')):
        if any(x in p.parts for x in SCAN_EXCL) or p.relative_to(ROOT).parts[0].startswith('.'):
            continue
        text = p.read_text(encoding='utf-8')
        lines = text.splitlines(keepends=True)
        in_fence = False
        modified = False
        out = []
        for line in lines:
            if line.lstrip().startswith('```'):
                in_fence = not in_fence
                out.append(line)
                continue
            if in_fence:
                out.append(line)
                continue
            code_spans = [(m.start(), m.end()) for m in re.finditer(r'`[^`]*`', line)]

            def in_code(pos):
                return any(s <= pos < e for s, e in code_spans)

            def repl(m):
                nonlocal modified, changed_links
                target, heading, alias, escaped = parse_inner(m.group(1))
                if target not in REMAP:
                    return m.group(0)
                new_target = REMAP[target]
                display = alias if alias else target.rstrip('/')
                sep = '\\|' if escaped else '|'
                frag = f'#{heading}' if heading else ''
                modified = True
                changed_links += 1
                return f'[[{new_target}{frag}{sep}{display}]]'

            out.append(WIKI.sub(lambda m: m.group(0) if in_code(m.start()) else repl(m), line))
        if modified:
            changed_files += 1
            if not dry:
                p.write_text(''.join(out), encoding='utf-8')
    print(f"{'[DRY-RUN] ' if dry else ''}修改文件={changed_files} 重映射链接={changed_links}")


if __name__ == '__main__':
    main()
