#!/usr/bin/env python3
"""
语料覆盖率验证器
检查 I-O 对语料是否充分覆盖了 Skills 和 FTA
"""

import argparse
import yaml
import json
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Any


class CoverageChecker:
    """覆盖率检查器"""

    COVERAGE_RULES = {
        'skill_coverage': {
            'target': 0.80,
            'description': '80% 的 Skill 至少关联 3 条 I-O 对',
        },
        'fta_coverage': {
            'target': 0.60,
            'description': '60% 的 FTA 文件至少关联 2 条 I-O 对',
        },
        'severity_balance': {
            'target': 'critical:high:medium:low ≈ 3:4:2:1',
            'description': '严重度分布合理',
        },
        'command_diversity': {
            'target': 0.90,
            'description': '90% 的命令不重复',
        },
        'domain_coverage': {
            'target': 0.85,
            'description': '85% 的故障域有 I-O 对覆盖',
        }
    }

    EXPECTED_DOMAINS = [
        'NODE', 'POD', 'DNS', 'NET', 'CERT', 'CP', 'ETCD',
        'WORK', 'STORAGE', 'INGRESS', 'SEC', 'UPGRADE',
        'OBS', 'SCALE', 'GPU', 'HELM', 'WEBHOOK', 'CONFIG'
    ]

    def __init__(self, skills_dir: str, fta_dir: str, corpus_dir: str):
        self.skills_dir = Path(skills_dir)
        self.fta_dir = Path(fta_dir)
        self.corpus_dir = Path(corpus_dir)
        self.io_pairs: List[Dict[str, Any]] = []
        self.results = {}

    def load_corpus(self) -> int:
        """加载所有语料文件"""
        count = 0
        for ext in ['*.json', '*.yaml', '*.yml']:
            for corpus_file in self.corpus_dir.rglob(ext):
                try:
                    with open(corpus_file, 'r', encoding='utf-8') as f:
                        if corpus_file.suffix == '.json':
                            data = json.load(f)
                        else:
                            data = yaml.safe_load(f)
                        if isinstance(data, list):
                            self.io_pairs.extend(data)
                            count += len(data)
                        elif isinstance(data, dict) and 'io_pairs' in data:
                            self.io_pairs.extend(data['io_pairs'])
                            count += len(data['io_pairs'])
                except Exception as e:
                    print(f"  [Warning] 加载失败 {corpus_file}: {e}")

        # 也扫描 Markdown 中的 YAML 块
        for md_file in self.corpus_dir.rglob('*.md'):
            try:
                content = md_file.read_text(encoding='utf-8')
                import re
                for block in re.finditer(r'```yaml\n(.*?)```', content, re.DOTALL):
                    try:
                        pair = yaml.safe_load(block.group(1))
                        if pair and isinstance(pair, dict) and 'io_pair_id' in pair:
                            self.io_pairs.append(pair)
                            count += 1
                    except yaml.YAMLError:
                        continue
            except Exception as e:
                print(f"  [Warning] 扫描失败 {md_file}: {e}")

        return count

    def check_skill_coverage(self) -> Dict:
        """检查 Skill 覆盖率"""
        skill_files = list(self.skills_dir.glob('*.md'))
        total_skills = len(skill_files)

        skill_counts = defaultdict(int)
        for pair in self.io_pairs:
            ref = pair.get('skill_ref', '')
            for skill_file in skill_files:
                if skill_file.stem in ref or ref in skill_file.name:
                    skill_counts[skill_file.name] += 1

        skills_with_min_3 = sum(1 for c in skill_counts.values() if c >= 3)
        coverage = skills_with_min_3 / total_skills if total_skills > 0 else 0

        return {
            'total_skills': total_skills,
            'skills_covered': len(skill_counts),
            'skills_with_min_3': skills_with_min_3,
            'coverage': coverage,
            'target': self.COVERAGE_RULES['skill_coverage']['target'],
            'passed': coverage >= self.COVERAGE_RULES['skill_coverage']['target'],
            'gaps': [
                f"{name}: {count} 条"
                for name, count in sorted(skill_counts.items(), key=lambda x: x[1])
                if count < 3
            ][:10]
        }

    def check_fta_coverage(self) -> Dict:
        """检查 FTA 覆盖率"""
        fta_files = list(self.fta_dir.glob('*.md'))
        total_ftas = len(fta_files)

        fta_counts = defaultdict(int)
        for pair in self.io_pairs:
            ref = pair.get('fta_ref', '')
            for fta_file in fta_files:
                if fta_file.stem in ref or ref in fta_file.name:
                    fta_counts[fta_file.name] += 1

        ftas_with_min_2 = sum(1 for c in fta_counts.values() if c >= 2)
        coverage = ftas_with_min_2 / total_ftas if total_ftas > 0 else 0

        return {
            'total_ftas': total_ftas,
            'ftas_covered': len(fta_counts),
            'ftas_with_min_2': ftas_with_min_2,
            'coverage': coverage,
            'target': self.COVERAGE_RULES['fta_coverage']['target'],
            'passed': coverage >= self.COVERAGE_RULES['fta_coverage']['target'],
            'gaps': [
                f"{name}: {count} 条"
                for name, count in sorted(fta_counts.items(), key=lambda x: x[1])
                if count < 2
            ][:10]
        }

    def check_severity_balance(self) -> Dict:
        """检查严重度分布"""
        severity_counts = defaultdict(int)
        for pair in self.io_pairs:
            s = pair.get('severity', 'unknown')
            severity_counts[s] += 1

        total = sum(severity_counts.values())
        if total == 0:
            return {'passed': False, 'distribution': {}, 'message': '无语料'}

        expected = {'critical': 0.30, 'high': 0.40, 'medium': 0.20, 'low': 0.10}
        distribution = {s: c / total for s, c in severity_counts.items()}

        max_deviation = 0
        for s, exp in expected.items():
            actual = distribution.get(s, 0)
            deviation = abs(actual - exp)
            max_deviation = max(max_deviation, deviation)

        passed = max_deviation < 0.15

        return {
            'total': total,
            'distribution': {s: f"{v*100:.1f}%" for s, v in distribution.items()},
            'counts': dict(severity_counts),
            'max_deviation': f"{max_deviation*100:.1f}%",
            'passed': passed
        }

    def check_command_diversity(self) -> Dict:
        """检查命令多样性"""
        commands = [pair.get('command', '') for pair in self.io_pairs]
        total = len(commands)
        unique = len(set(commands))
        ratio = unique / total if total > 0 else 0

        return {
            'total_commands': total,
            'unique_commands': unique,
            'ratio': ratio,
            'target': self.COVERAGE_RULES['command_diversity']['target'],
            'passed': ratio >= self.COVERAGE_RULES['command_diversity']['target'],
            'top_duplicates': self._find_duplicate_commands(commands)
        }

    def _find_duplicate_commands(self, commands: List[str]) -> List[Dict]:
        """找出重复最多的命令"""
        counts = defaultdict(int)
        for cmd in commands:
            normalized = self._normalize_command(cmd)
            counts[normalized] += 1

        duplicates = [
            {'command': cmd, 'count': count}
            for cmd, count in sorted(counts.items(), key=lambda x: -x[1])
            if count > 2
        ]
        return duplicates[:5]

    def _normalize_command(self, cmd: str) -> str:
        """归一化命令用于比较"""
        import re
        cmd = re.sub(r'\s+\S+-[a-z0-9]{5,}\b', ' <name>', cmd)
        cmd = re.sub(r'\s+<\S+>\b', ' <name>', cmd)
        return cmd.strip()

    def check_domain_coverage(self) -> Dict:
        """检查 Domain 覆盖率"""
        domains_found = set()
        for pair in self.io_pairs:
            iid = pair.get('io_pair_id', '')
            parts = iid.split('-')
            if len(parts) >= 2:
                domains_found.add(parts[1])

        covered = len(domains_found & set(self.EXPECTED_DOMAINS))
        total = len(self.EXPECTED_DOMAINS)
        coverage = covered / total if total > 0 else 0

        return {
            'expected_domains': self.EXPECTED_DOMAINS,
            'found_domains': sorted(domains_found),
            'covered': covered,
            'total_expected': total,
            'coverage': coverage,
            'target': self.COVERAGE_RULES['domain_coverage']['target'],
            'passed': coverage >= self.COVERAGE_RULES['domain_coverage']['target'],
            'missing': [d for d in self.EXPECTED_DOMAINS if d not in domains_found]
        }

    def run_all_checks(self) -> Dict:
        """运行所有检查"""
        print("\n加载语料...")
        count = self.load_corpus()
        print(f"  加载了 {count} 条 I-O 对")

        print("\n运行覆盖率检查...")
        self.results = {
            'skill_coverage': self.check_skill_coverage(),
            'fta_coverage': self.check_fta_coverage(),
            'severity_balance': self.check_severity_balance(),
            'command_diversity': self.check_command_diversity(),
            'domain_coverage': self.check_domain_coverage(),
        }
        return self.results

    def print_report(self):
        """打印检查报告"""
        print("\n" + "=" * 70)
        print("命令输出诊断语料 -- 覆盖率验证报告")
        print("=" * 70)

        r = self.results['skill_coverage']
        status = "PASS" if r['passed'] else "FAIL"
        print(f"\n1. Skill 覆盖率 [{status}]")
        print(f"   Skills 总数: {r['total_skills']}")
        print(f"   已覆盖: {r['skills_covered']} ({r['skills_with_min_3']} 个有 >=3 条)")
        print(f"   覆盖率: {r['coverage']*100:.1f}% (目标: {r['target']*100:.0f}%)")
        if r['gaps']:
            print(f"   Top 缺口: {', '.join(r['gaps'][:3])}")

        r = self.results['fta_coverage']
        status = "PASS" if r['passed'] else "FAIL"
        print(f"\n2. FTA 覆盖率 [{status}]")
        print(f"   FTA 总数: {r['total_ftas']}")
        print(f"   已覆盖: {r['ftas_covered']} ({r['ftas_with_min_2']} 个有 >=2 条)")
        print(f"   覆盖率: {r['coverage']*100:.1f}% (目标: {r['target']*100:.0f}%)")
        if r['gaps']:
            print(f"   Top 缺口: {', '.join(r['gaps'][:3])}")

        r = self.results['severity_balance']
        status = "PASS" if r['passed'] else "WARN"
        print(f"\n3. 严重度分布 [{status}]")
        print(f"   总数: {r['total']}")
        print(f"   分布: {r['distribution']}")
        print(f"   最大偏差: {r['max_deviation']}")

        r = self.results['command_diversity']
        status = "PASS" if r['passed'] else "FAIL"
        print(f"\n4. 命令多样性 [{status}]")
        print(f"   总命令: {r['total_commands']}")
        print(f"   唯一命令: {r['unique_commands']}")
        print(f"   去重率: {r['ratio']*100:.1f}% (目标: {r['target']*100:.0f}%)")
        if r['top_duplicates']:
            print(f"   重复命令: {r['top_duplicates'][0]['command']} ({r['top_duplicates'][0]['count']} 次)")

        r = self.results['domain_coverage']
        status = "PASS" if r['passed'] else "FAIL"
        print(f"\n5. Domain 覆盖 [{status}]")
        print(f"   期望 Domain: {r['total_expected']}")
        print(f"   已覆盖: {r['covered']}")
        print(f"   覆盖率: {r['coverage']*100:.1f}% (目标: {r['target']*100:.0f}%)")
        if r['missing']:
            print(f"   缺失: {', '.join(r['missing'])}")

        all_passed = all(r['passed'] for r in self.results.values())
        print("\n" + "=" * 70)
        if all_passed:
            print("所有检查通过！语料覆盖率达到要求。")
        else:
            print("部分检查未通过，建议补充缺失的语料。")
        print("=" * 70)

    def save_report(self, output_path: str):
        """保存 JSON 报告"""
        report = {
            'summary': {
                'total_io_pairs': len(self.io_pairs),
                'all_passed': all(r['passed'] for r in self.results.values()),
                'checks_passed': sum(1 for r in self.results.values() if r['passed']),
                'checks_total': len(self.results)
            },
            'details': self.results,
            'recommendations': self._generate_recommendations()
        }
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(report, f, ensure_ascii=False, indent=2)
        print(f"\n报告已保存: {output_path}")

    def _generate_recommendations(self) -> List[str]:
        """生成改进建议"""
        recs = []
        r = self.results['skill_coverage']
        if not r['passed']:
            recs.append(f"补充 Skill 语料: {r['total_skills'] - r['skills_covered']} 个 Skill 无 I-O 对覆盖")
        r = self.results['fta_coverage']
        if not r['passed']:
            recs.append(f"补充 FTA 语料: {r['total_ftas'] - r['ftas_covered']} 个 FTA 无 I-O 对覆盖")
        r = self.results['domain_coverage']
        if r['missing']:
            recs.append(f"补充 Domain 语料: 缺失 {', '.join(r['missing'])}")
        if not recs:
            recs.append("语料覆盖良好，建议持续维护更新")
        return recs


def main():
    parser = argparse.ArgumentParser(description="验证命令输出诊断语料覆盖率")
    parser.add_argument('--skills-dir', type=str, required=True)
    parser.add_argument('--fta-dir', type=str, required=True)
    parser.add_argument('--corpus-dir', type=str, required=True)
    parser.add_argument('--output', type=str, default='corpus-coverage-report.json')
    args = parser.parse_args()

    checker = CoverageChecker(args.skills_dir, args.fta_dir, args.corpus_dir)
    checker.run_all_checks()
    checker.print_report()
    checker.save_report(args.output)


if __name__ == '__main__':
    main()
