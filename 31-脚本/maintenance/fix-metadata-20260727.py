#!/usr/bin/env python3
"""
2026-07-27 元数据一次性修复（对应 P0-2 项）：
1. 修复内容层 YAML frontmatter 损坏（summary/description 单引号嵌套导致解析失败）
2. tier 非法值 critical → core（schema.md 仅允许 core/supporting/peripheral）
3. 非 index/log 内容页缺失 tier → 默认补 supporting
4. 缺失 frontmatter 的 README/上游文档 → 补最小 frontmatter（title/category/tags/tier）

用法: python3 31-脚本/maintenance/fix-metadata-20260727.py [--dry-run]
"""
import json
import pathlib
import re
import sys

import yaml

ROOT = pathlib.Path(__file__).resolve().parents[2]
EXCL = {'node_modules', '.venv', '.git', '__pycache__', '30-站点', '32-发布',
        '33-源码', '37-归档', '36-报告', '35-元数据', '28-资产', '31-脚本'}
CONTENT = {'22-概念', '23-实体', '26-技能', '24-综合', '29-文档', '20-最佳实践', '25-研究', '27-标签'}
DOMAINS = {'01-集群基础', '02-工作负载', '05-网络', '06-存储', '08-安全', '09-可观测性',
           '10-平台工程', '11-发布变更', '12-可靠性', '19-故障诊断', '13-生产运维', '18-云厂商',
           '14-容器运行时', '15-AI基础设施', '16-专项技术', '07-数据库中间件', '17-系统基础',
           '03-清单模式', '21-生态参考', '04-应用模式'}

KV_LINE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):[ \t]+(\S.*)$")


def is_content(p: pathlib.Path) -> bool:
    return bool(p.parts) and (p.parts[0] in DOMAINS or p.parts[0] in CONTENT)


def repair_yaml_block(fm_text: str) -> str:
    """逐行修复无法解析的标量值：以 JSON 双引号重新序列化。"""
    fixed_lines = []
    for line in fm_text.split('\n'):
        m = KV_LINE.match(line)
        if m:
            try:
                yaml.safe_load(line)
            except yaml.YAMLError:
                key, raw = m.group(1), m.group(2).strip()
                # 剥掉最外层引号后按 JSON 安全引用
                if len(raw) >= 2 and raw[0] == raw[-1] and raw[0] in ("'", '"'):
                    raw = raw[1:-1]
                fixed_lines.append(f"{key}: {json.dumps(raw, ensure_ascii=False)}")
                continue
        fixed_lines.append(line)
    return '\n'.join(fixed_lines)


def minimal_frontmatter(body: str) -> str:
    title = None
    for line in body.split('\n'):
        if line.startswith('# '):
            title = line[2:].strip()
            break
    title = title or 'Untitled'
    fm = {'title': title, 'category': 'reference', 'tags': ['reference'],
          'tier': 'supporting', 'created': '2026-07-27'}
    return yaml.safe_dump(fm, allow_unicode=True, sort_keys=False)


def main() -> None:
    dry = '--dry-run' in sys.argv
    stats = {'yaml_fixed': 0, 'yaml_unfixable': 0, 'tier_critical': 0,
             'tier_added': 0, 'fm_added': 0}

    for p in sorted(ROOT.rglob('*.md')):
        rel = p.relative_to(ROOT)
        if any(x in rel.parts for x in EXCL) or not is_content(rel):
            continue
        txt = p.read_text(encoding='utf-8', errors='ignore')

        # 4) 无 frontmatter
        if not txt.startswith('---'):
            new = f"---\n{minimal_frontmatter(txt)}---\n\n{txt.lstrip()}"
            if not dry:
                p.write_text(new, encoding='utf-8')
            stats['fm_added'] += 1
            print(f"[fm+] {rel}")
            continue

        parts = txt.split('---', 2)
        if len(parts) < 3:
            stats['yaml_unfixable'] += 1
            print(f"[!!] 无闭合 frontmatter: {rel}")
            continue
        fm_text, body = parts[1], parts[2]

        changed = False
        try:
            fm = yaml.safe_load(fm_text)
        except yaml.YAMLError:
            fm_text2 = repair_yaml_block(fm_text)
            try:
                fm = yaml.safe_load(fm_text2)
                fm_text = fm_text2
                changed = True
                stats['yaml_fixed'] += 1
                print(f"[yaml] {rel}")
            except yaml.YAMLError as e:
                stats['yaml_unfixable'] += 1
                print(f"[!!] 修复失败 {rel}: {str(e).splitlines()[0]}")
                continue
        if not isinstance(fm, dict):
            continue

        # 2) tier: critical → core
        tier = fm.get('tier')
        if tier == 'critical':
            fm_text = re.sub(r'^tier:\s*critical\s*$', 'tier: core',
                             fm_text, count=1, flags=re.M)
            changed = True
            stats['tier_critical'] += 1
            print(f"[tier] critical→core {rel}")
        # 3) 缺 tier（index.md/log.md 按 schema 豁免）
        elif tier is None and p.name not in ('index.md', 'log.md'):
            fm_text = fm_text.rstrip('\n') + '\ntier: supporting\n'
            changed = True
            stats['tier_added'] += 1
            print(f"[tier+] supporting {rel}")

        if changed and not dry:
            p.write_text(f"---{fm_text}---{body}", encoding='utf-8')

    print('\n== 汇总 ==')
    for k, v in stats.items():
        print(f"{k}: {v}")


if __name__ == '__main__':
    main()
