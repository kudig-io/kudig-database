#!/usr/bin/env python3
"""
批量为缺少 summary/frontmatter 的核心页面补充字段。
- summary 优先使用 description，其次正文首段前 200 字符
- frontmatter 补充 title、category、tags、created
"""

import re
import yaml
from pathlib import Path
from datetime import datetime

# 知识库 domain 目录（已从 domain-NN-slug 改为中文命名）
DOMAINS = {'集群基础','工作负载','网络','存储','安全','可观测性','平台工程','发布变更','可靠性','故障诊断','生产运维','云厂商','容器运行时','AI基础设施','专项技术','数据库中间件','系统基础','清单模式','生态参考','应用模式'}


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def parse_frontmatter(text: str) -> tuple:
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    if not fm_match:
        return None, text
    return fm_match.group(1), text[fm_match.end():]


def load_fm(fm_text: str) -> dict:
    try:
        return yaml.safe_load(fm_text) or {}
    except Exception:
        return {}


def extract_first_paragraph(body: str) -> str:
    # 去掉 heading、列表、代码块等，找第一段文字
    lines = body.splitlines()
    paragraphs = []
    current = []

    for line in lines:
        stripped = line.strip()
        if not stripped:
            if current:
                paragraphs.append(' '.join(current))
                current = []
            continue
        # 跳过 markdown heading、列表、代码块、表格、引用
        if stripped.startswith('#') or stripped.startswith('-') or stripped.startswith('*') or stripped.startswith('|') or stripped.startswith('>') or stripped.startswith('```'):
            if current:
                paragraphs.append(' '.join(current))
                current = []
            continue
        # 去掉 inline markdown
        cleaned = re.sub(r'\[([^\]]+)\]\([^)]+\)', r'\1', stripped)  # [text](url)
        cleaned = re.sub(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', r'\1', cleaned)  # wikilinks
        cleaned = re.sub(r'`([^`]+)`', r'\1', cleaned)  # inline code
        cleaned = re.sub(r'\*\*([^*]+)\*\*', r'\1', cleaned)  # bold
        cleaned = re.sub(r'\*([^*]+)\*', r'\1', cleaned)  # italic
        if cleaned.strip():
            current.append(cleaned.strip())

    if current:
        paragraphs.append(' '.join(current))

    if not paragraphs:
        return ""

    first = paragraphs[0]
    # 限制 200 字符
    if len(first) > 200:
        first = first[:197] + '...'
    return first


def generate_summary(fm: dict, body: str) -> str:
    # 优先 description
    if fm.get('description') and isinstance(fm['description'], str):
        desc = fm['description'].strip()
        if len(desc) > 200:
            return desc[:197] + '...'
        return desc

    # 其次 title + 正文首段
    title = fm.get('title', '')
    para = extract_first_paragraph(body)
    if para:
        if title and not para.lower().startswith(title.lower()):
            summary = f"{title}：{para}"
        else:
            summary = para
        if len(summary) > 200:
            return summary[:197] + '...'
        return summary

    # 兜底
    return title if title else "待补充摘要"


def update_file(p: Path, rel: str, fm: dict, body: str, missing_summary: bool, missing_fields: list) -> dict:
    """更新文件，返回修改记录。"""
    changed = {'summary': False, 'frontmatter': False}

    # 生成 summary
    if missing_summary:
        fm['summary'] = generate_summary(fm, body)
        changed['summary'] = True

    # 补充基础字段
    if missing_fields:
        if 'title' in missing_fields:
            fm['title'] = Path(rel).stem.replace('-', ' ').replace('_', ' ').title()
        if 'category' in missing_fields:
            # 根据路径推断 category
            first_dir = rel.split('/')[0]
            if first_dir == 'concepts':
                fm['category'] = 'concepts'
            elif first_dir == 'entities':
                fm['category'] = 'entities'
            elif first_dir == 'skills':
                fm['category'] = 'skills'
            elif first_dir in DOMAINS:
                fm['category'] = first_dir
            elif first_dir == 'docs':
                fm['category'] = 'docs'
            else:
                fm['category'] = 'general'
        if 'tags' in missing_fields:
            # 从路径推断一个默认 tag
            stem = Path(rel).stem.lower()
            fm['tags'] = [stem] if stem else ['general']
        if 'created' in missing_fields:
            fm['created'] = datetime.now().strftime('%Y-%m-%d')
        changed['frontmatter'] = True

    # 序列化 frontmatter
    def str_representer(dumper, data):
        if '\n' in data:
            return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
        return dumper.represent_scalar('tag:yaml.org,2002:str', data)

    yaml.add_representer(str, str_representer)

    # 保持字段顺序
    ordered_fm = {}
    for key in ['title', 'description', 'category', 'tags', 'sources', 'created', 'updated', 'summary']:
        if key in fm:
            ordered_fm[key] = fm[key]
    for key in fm:
        if key not in ordered_fm:
            ordered_fm[key] = fm[key]

    fm_text = yaml.dump(ordered_fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
    new_text = f"---\n{fm_text}---\n{body}"

    try:
        p.write_text(new_text, encoding='utf-8')
    except PermissionError:
        return None

    return changed


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    required_fields = ['title', 'category', 'tags', 'created']

    summary_count = 0
    frontmatter_count = 0
    permission_errors = []

    md_files = [p for p in vault.rglob('*.md') if not is_excluded(str(p.relative_to(vault)))]

    print(f"Scanning {len(md_files)} core pages...")

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, body = parse_frontmatter(text)

        if fm_text is None:
            fm = {}
        else:
            fm = load_fm(fm_text)

        missing_summary = 'summary' not in fm
        missing_fields = [f for f in required_fields if f not in fm]

        if not missing_summary and not missing_fields:
            continue

        changed = update_file(p, rel, fm, body, missing_summary, missing_fields)
        if changed is None:
            permission_errors.append(rel)
            continue

        if changed['summary']:
            summary_count += 1
        if changed['frontmatter']:
            frontmatter_count += 1

    print(f"\nSummary added: {summary_count}")
    print(f"Frontmatter added/fixed: {frontmatter_count}")
    print(f"Permission errors: {len(permission_errors)}")
    if permission_errors:
        for rel in permission_errors[:10]:
            print(f"  {rel}")

    # 生成报告
    report_out = vault / '_reports/summary-frontmatter-fill-2026-06-26.md'
    lines = [
        "---",
        "title: Summary 与 Frontmatter 批量补充报告（2026-06-26）",
        "description: 为缺少 summary/frontmatter 的核心页面自动补充字段",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- summary",
        "- frontmatter",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# Summary 与 Frontmatter 批量补充报告",
        "",
        f"- **扫描页面数**: {len(md_files)}",
        f"- **新增 summary**: {summary_count}",
        f"- **补齐 frontmatter**: {frontmatter_count}",
        f"- **权限错误**: {len(permission_errors)}",
        "",
        "## 生成规则",
        "",
        "1. summary 优先使用 frontmatter 中的 description 字段",
        "2. 无 description 时提取正文第一段前 200 字符",
        "3. frontmatter 缺失 title/category/tags/created 时按路径推断默认值",
        "",
        "> ⚠️ 自动生成的 summary 和 category 仅为默认值，建议人工 review 关键页面。",
    ]
    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_out}")


if __name__ == "__main__":
    main()
