#!/usr/bin/env python3
"""
主生成脚本：从 Skills 和 FTA 生成命令输出→诊断 I-O 对语料
Usage:
    python generate.py --priority P0 --output ../../19-故障诊断/10-QA语料/generated/
    python generate.py --priority all --output ../../19-故障诊断/10-QA语料/generated/
"""

import argparse
import json
import yaml
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Any

from extractors.skill_extractor import SkillExtractor
from extractors.fta_extractor import FTAExtractor


# P0 优先 Skill/FTA 映射（核心故障场景）
P0_PRIORITY = {
    'skills': [
        '01-node-notready',
        '02-pod-crashloop-oomkilled',
        '03-pod-pending',
        '04-dns-resolution-failure',
        '05-service-connectivity',
        '06-certificate-expiry',
        '07-pvc-storage-failure',
        '08-deployment-rollout-failure',
        '09-rbac-quota-failure',
        '10-image-pull-failure',
        '11-control-plane-failure',
    ],
    'ftas': [
        'node-fta',
        'pod-fta',
        'dns-fta',
        'service-fta',
        'certificate-fta',
        'etcd-fta',
        'apiserver-fta',
        'scheduler-fta',
        'deployment-fta',
        'daemonset-fta',
    ]
}

P1_PRIORITY = {
    'skills': [
        '12-autoscaling-failure',
        '13-ingress-gateway-failure',
        '14-configmap-secret-failure',
        '15-monitoring-alerting-failure',
        '16-logging-pipeline-failure',
        '17-node-resource-pressure',
        '19-node-resource-pressure',
        '20-networkpolicy-connectivity',
        '22-daemonset-failure',
        '24-namespace-quota-limitrange',
        '25-cluster-upgrade-migration',
    ],
    'ftas': [
        'ingress-fta',
        'nginx-ingress-fta',
        'gateway-api-fta',
        'hpa-fta',
        'csi-fta',
        'calico-fta',
        'cilium-fta',
        'flannel-fta',
        'networkpolicy-fta',
        'service-mesh-istio-fta',
    ]
}

P2_PRIORITY = {
    'skills': [
        '18-crd-operator-failure',
        '21-job-cronjob-failure',
        '23-statefulset-failure',
    ],
    'ftas': [
        'gpu-fta',
        'cluster-autoscaler-fta',
        'monitoring-fta',
        'crd-operator-fta',
        'helm-fta',
        'backup-restore-fta',
        'cloud-provider-fta',
    ]
}


def load_config(config_path: str = None) -> Dict[str, Any]:
    """加载配置文件"""
    if config_path and Path(config_path).exists():
        with open(config_path, 'r') as f:
            return yaml.safe_load(f) or {}
    return {}


def merge_with_seed(io_pairs: List[Dict], seed_dir: Path) -> List[Dict]:
    """合并手工种子数据"""
    if not seed_dir.exists():
        return io_pairs

    seed_pairs = []
    for seed_file in sorted(seed_dir.glob('*.md')):
        content = seed_file.read_text(encoding='utf-8')
        # 提取 YAML 代码块中的 I-O 对
        import re
        for block in re.finditer(r'```yaml\n(.*?)```', content, re.DOTALL):
            try:
                pair = yaml.safe_load(block.group(1))
                if pair and isinstance(pair, dict) and 'io_pair_id' in pair:
                    seed_pairs.append(pair)
            except yaml.YAMLError:
                continue

    if seed_pairs:
        print(f"  [Seed] 加载手工种子: {len(seed_pairs)} 条")
        # 用种子覆盖自动生成的（按 io_pair_id 去重）
        existing_ids = {p['io_pair_id'] for p in io_pairs}
        for seed in seed_pairs:
            if seed['io_pair_id'] not in existing_ids:
                io_pairs.append(seed)

    return io_pairs


def render_markdown(io_pairs: List[Dict], priority: str) -> str:
    """渲染为 Markdown 文件"""
    lines = [
        "---",
        f"title: 命令输出诊断语料 — {priority} 优先级",
        f"description: 从 Skills 和 FTA 自动提取的命令输出→诊断 I-O 对，{priority} 优先级",
        "category: agent-corpus",
        "tags:",
        "- k8s",
        "- troubleshooting",
        "- command-output",
        "- diagnosis",
        "- agent",
        "- corpus",
        f"- priority-{priority.lower()}",
        f"last_updated: '{datetime.now().strftime('%Y-%m-%d')}'",
        "difficulty: advanced",
        "reading_level: advanced",
        "audience:",
        "- AI Agent",
        "- SRE",
        "- 运维工程师",
        "---",
        "",
        f"# 命令输出诊断语料 — {priority} 优先级",
        "",
        f"> **生成时间**: {datetime.now().isoformat()}",
        f"> **I-O 对总数**: {len(io_pairs)}",
        f"> **优先级**: {priority}",
        f"> **生成方式**: 自动提取 + 人工种子",
        "> **用途**: Agent 接收命令输出后，直接匹配诊断结论",
        "",
        "---",
        "",
    ]

    # 按 domain 分组
    from collections import defaultdict
    by_domain = defaultdict(list)
    for pair in io_pairs:
        domain = pair['io_pair_id'].split('-')[1] if '-' in pair['io_pair_id'] else 'GENERAL'
        by_domain[domain].append(pair)

    for domain in sorted(by_domain.keys()):
        pairs = by_domain[domain]
        lines.append(f"## Domain: {domain} ({len(pairs)} 条)")
        lines.append("")

        for pair in pairs:
            lines.append(f"### {pair['io_pair_id']} — {pair.get('scenario', 'Unknown')}")
            lines.append("")
            lines.append("```yaml")
            # 清理后序列化
            clean = {k: v for k, v in pair.items() if v not in (None, [], '')}
            yaml_str = yaml.dump(clean, allow_unicode=True, sort_keys=False, default_flow_style=False)
            lines.append(yaml_str.rstrip())
            lines.append("```")
            lines.append("")

    # 统计附录
    lines.append("---")
    lines.append("")
    lines.append("## 统计信息")
    lines.append("")
    lines.append(f"| 指标 | 数值 |")
    lines.append(f"|------|------|")
    lines.append(f"| 总 I-O 对数 | {len(io_pairs)} |")

    severity_counts = {}
    for p in io_pairs:
        s = p.get('severity', 'unknown')
        severity_counts[s] = severity_counts.get(s, 0) + 1
    for s in ['critical', 'high', 'medium', 'low']:
        if s in severity_counts:
            lines.append(f"| severity={s} | {severity_counts[s]} |")

    lines.append("")
    lines.append("---")
    lines.append("*本文件由 31-脚本/corpus-generator/generate.py 自动生成*")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="生成命令输出诊断 I-O 对语料")
    parser.add_argument('--priority', choices=['P0', 'P1', 'P2', 'all'], default='P0',
                        help='要处理的优先级')
    parser.add_argument('--output', type=str, required=True,
                        help='输出目录')
    parser.add_argument('--skills-dir', type=str,
                        default='19-故障诊断/08-技能体系',
                        help='Skills 目录')
    parser.add_argument('--fta-dir', type=str,
                        default='19-故障诊断/06-FTA故障树/list',
                        help='FTA 目录')
    parser.add_argument('--seed-dir', type=str,
                        default='19-故障诊断/10-QA语料/seed',
                        help='手工种子目录')
    parser.add_argument('--config', type=str, default=None,
                        help='配置文件路径')
    args = parser.parse_args()

    # 解析路径（支持相对路径）
    base_dir = Path(__file__).parent.parent.parent
    skills_dir = base_dir / args.skills_dir
    fta_dir = base_dir / args.fta_dir
    seed_dir = base_dir / args.seed_dir
    output_dir = base_dir / args.output
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"=" * 60)
    print(f"命令输出诊断语料生成器")
    print(f"Skills 目录: {skills_dir}")
    print(f"FTA 目录: {fta_dir}")
    print(f"种子目录: {seed_dir}")
    print(f"输出目录: {output_dir}")
    print(f"优先级: {args.priority}")
    print(f"=" * 60)

    all_pairs = []

    # 确定要处理的优先级列表
    priorities = ['P0', 'P1', 'P2'] if args.priority == 'all' else [args.priority]

    for priority in priorities:
        print(f"\n--- 处理 {priority} ---")
        config = P0_PRIORITY if priority == 'P0' else (P1_PRIORITY if priority == 'P1' else P2_PRIORITY)

        # 提取 Skills
        if skills_dir.exists():
            skill_extractor = SkillExtractor(str(skills_dir))
            skill_pairs = skill_extractor.extract_all(priority_skills=config['skills'])
            all_pairs.extend(skill_pairs)
        else:
            print(f"  [Warning] Skills 目录不存在: {skills_dir}")

        # 提取 FTA
        if fta_dir.exists():
            fta_extractor = FTAExtractor(str(fta_dir))
            fta_pairs = fta_extractor.extract_all(priority_ftas=config['ftas'])
            all_pairs.extend(fta_pairs)
        else:
            print(f"  [Warning] FTA 目录不存在: {fta_dir}")

    # 合并种子数据
    all_pairs = merge_with_seed(all_pairs, seed_dir)

    # 去重（按 io_pair_id）
    seen = {}
    unique_pairs = []
    for p in all_pairs:
        iid = p.get('io_pair_id', '')
        if iid and iid not in seen:
            seen[iid] = True
            unique_pairs.append(p)

    print(f"\n{'=' * 60}")
    print(f"生成完成: {len(unique_pairs)} 条唯一 I-O 对")
    print(f"{'=' * 60}")

    # 写入文件
    if args.priority == 'all':
        # 按优先级拆分为多个文件
        for priority in priorities:
            # 这里简化处理，实际可以按 priority 标记过滤
            md_content = render_markdown(unique_pairs, priority)
            output_file = output_dir / f"command-output-diagnosis-{priority.lower()}.md"
            output_file.write_text(md_content, encoding='utf-8')
            print(f"  写入: {output_file} ({len(md_content)} bytes)")
    else:
        md_content = render_markdown(unique_pairs, args.priority)
        output_file = output_dir / f"command-output-diagnosis-{args.priority.lower()}.md"
        output_file.write_text(md_content, encoding='utf-8')
        print(f"  写入: {output_file} ({len(md_content)} bytes)")

    # 同时写入 JSON 便于程序消费
    json_file = output_dir / f"command-output-diagnosis-{args.priority.lower()}.json"
    with open(json_file, 'w', encoding='utf-8') as f:
        json.dump(unique_pairs, f, ensure_ascii=False, indent=2)
    print(f"  写入: {json_file}")

    # 写入 YAML
    yaml_file = output_dir / f"command-output-diagnosis-{args.priority.lower()}.yaml"
    with open(yaml_file, 'w', encoding='utf-8') as f:
        yaml.dump(unique_pairs, f, allow_unicode=True, sort_keys=False)
    print(f"  写入: {yaml_file}")


if __name__ == '__main__':
    main()
