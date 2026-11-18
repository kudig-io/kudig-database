#!/usr/bin/env python3
"""
Agent QA 对语料生成脚本（适配 2026-07 目录重组后的布局）
从现有 domain/topic 文档自动生成 question-answer 对,
用于 RAG 评测、Agent fine-tuning、和检索质量验证。

输出格式: 每个 domain 一个 YAML 文件, 包含 QA pairs
输出目录: wiki/19-故障诊断/10-QA语料/

与旧版的差异:
- 路径基于脚本位置解析（ROOT），不再硬编码绝对路径
- domain 目录按当前布局解析（如 网络 -> wiki/05-网络）
- FTA / 技能 / 应用架构源目录指向现行路径
- 写入前与已有输出合并: source 仍存在的条目原样保留
  （含手工补充/修复的条目），source 已悬空的条目删除，
  新生成且未重复的条目追加
- --output-dir 可指定输出目录试运行，默认写入真实语料目录

用法:
  python3 code/scripts/generate-qa-corpus.py                    # 合并写入真实语料目录
  python3 code/scripts/generate-qa-corpus.py --output-dir /tmp/qa-test   # 试运行
"""

import argparse
import os
import re
import yaml
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WIKI_DIR = ROOT / "wiki"
DEFAULT_OUTPUT_DIR = WIKI_DIR / "19-故障诊断" / "10-QA语料"

# FTA 故障树 / 技能体系 / 应用架构（行业架构）三个专用源目录（现行布局）
FTA_LIST_DIR = WIKI_DIR / "19-故障诊断" / "06-FTA故障树" / "list"
TOPIC_SKILLS_DIR = WIKI_DIR / "19-故障诊断" / "08-技能体系"
APP_ARCH_DIR = WIKI_DIR / "04-应用模式" / "02-行业架构"

# 核心 domain 列表 (优先生成 QA 对)，实际目录按 wiki/NN-<name> 解析
CORE_DOMAINS = [
    "集群基础",
    "工作负载",
    "网络",
    "存储",
    "安全",
    "可观测性",
    "平台工程",
    "发布变更",
    "可靠性",
    "故障诊断",
    "生产运维",
    "云厂商",
    "容器运行时",
    "AI基础设施",
    "专项技术",
    "数据库中间件",
    "系统基础",
    "清单模式",
    "生态参考",
    "应用模式",
]

# 排除目录
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules', '.obsidian', '.zread', '.claude', '.codebuddy', '.comate', '.github', '__pycache__'}


def resolve_domain_dir(name: str):
    """把 domain 名解析为现行目录，如 网络 -> wiki/05-网络。"""
    if not WIKI_DIR.is_dir():
        return None
    candidates = [d for d in sorted(WIKI_DIR.iterdir())
                  if d.is_dir() and (d.name == name or d.name.endswith("-" + name))]
    return candidates[0] if candidates else None


def safe_output_path(base, *parts):
    """加固：输出约束在 base 内防路径穿越（对现有常量输入行为不变）。"""
    base_real = os.path.realpath(base)
    target = os.path.realpath(os.path.join(base_real, *parts))
    if os.path.commonpath([base_real, target]) != base_real:
        raise ValueError(f"output path escapes base dir: {target}")
    return Path(target)


def parse_frontmatter(content: str) -> tuple:
    """解析 YAML front matter"""
    stripped = content.lstrip()
    if not stripped.startswith('---'):
        return {}, content
    end_match = re.search(r'\n---\s*\n', stripped[4:])
    if not end_match:
        return {}, content
    yaml_str = stripped[4:end_match.start() + 4]
    body = stripped[end_match.end() + 4:]
    try:
        fm = yaml.safe_load(yaml_str) or {}
        return fm, body
    except yaml.YAMLError:
        return {}, content


def split_readme_frontmatter(text: str) -> tuple:
    """拆出 README 已有的 frontmatter 原文（保留手工元数据）。"""
    if text.startswith('---\n'):
        m = re.search(r'\n---\s*\n', text[4:])
        if m:
            return text[:m.end() + 4]
    return ""


def extract_sections(body: str) -> list:
    """提取 H2/H3 章节标题"""
    sections = []
    for match in re.finditer(r'^(#{2,3})\s+(.+?)(?:\s*\{.*\})?$', body, re.MULTILINE):
        level = len(match.group(1))
        title = match.group(2).strip()
        # 去掉 emoji
        title = re.sub(r'[\U0001f300-\U0001f9ff]', '', title).strip()
        sections.append({"level": level, "title": title, "pos": match.start()})
    return sections


def extract_key_concepts(body: str) -> list:
    """从正文中提取关键技术概念"""
    concepts = []
    # 匹配加粗的技术术语
    for match in re.finditer(r'\*\*([^*]{2,30})\*\*', body):
        term = match.group(1).strip()
        if not re.match(r'^[\d\s]+$', term) and len(term) >= 2:
            concepts.append(term)
    # 去重保持顺序
    seen = set()
    unique = []
    for c in concepts:
        if c not in seen:
            seen.add(c)
            unique.append(c)
    return unique[:20]


def extract_code_blocks(body: str) -> list:
    """提取代码块 (用于生成命令类 QA)"""
    blocks = []
    for match in re.finditer(r'```(\w*)\n(.*?)```', body, re.DOTALL):
        lang = match.group(1) or 'text'
        code = match.group(2).strip()
        if lang in ('bash', 'shell', 'yaml', 'json') and len(code) > 10:
            blocks.append({"lang": lang, "code": code[:500]})
    return blocks[:10]


def generate_qa_from_doc(filepath: Path, fm: dict, body: str) -> list:
    """从单个文档生成 QA 对"""
    qa_pairs = []
    title = fm.get('title', filepath.stem)

    sections = extract_sections(body)
    code_blocks = extract_code_blocks(body)

    # 1. 概念类 QA (从标题和 H2 章节)
    if sections:
        for sec in sections[:5]:
            if sec['level'] == 2:
                sec_title = sec['title']
                # 清理标题中的 emoji 和数字
                clean_title = re.sub(r'^[\d一二三四五六七八九十]+[、.．\s]+', '', sec_title)
                clean_title = re.sub(r'[\U0001f300-\U0001f9ff]', '', clean_title).strip()
                if clean_title:
                    qa_pairs.append({
                        "type": "concept",
                        "question": f"在 {title} 中, {clean_title} 是什么?",
                        "answer": f"参见 {filepath.relative_to(ROOT)} 的「{clean_title}」章节。",
                        "source": str(filepath.relative_to(ROOT)),
                        "section": clean_title,
                    })

    # 2. 操作类 QA (从代码块)
    for block in code_blocks[:3]:
        if block['lang'] in ('bash', 'shell'):
            # 提取第一条命令
            first_cmd = block['code'].split('\n')[0].strip()
            if first_cmd.startswith('#'):
                first_cmd_lines = block['code'].split('\n')
                for line in first_cmd_lines:
                    if line.strip() and not line.strip().startswith('#'):
                        first_cmd = line.strip()
                        break
            if first_cmd and not first_cmd.startswith('#'):
                qa_pairs.append({
                    "type": "operation",
                    "question": f"如何执行与 {title} 相关的操作: {first_cmd[:60]}?",
                    "answer": f"参见 {filepath.relative_to(ROOT)} 中的操作步骤。",
                    "source": str(filepath.relative_to(ROOT)),
                    "command": first_cmd[:100],
                })

    # 3. 最佳实践类 QA (从包含"最佳实践"、"建议"、"注意事项"的段落)
    practice_patterns = [
        r'(?:最佳实践|建议|注意事项|生产建议|推荐配置|重要提示)[：:]\s*(.{20,200})',
        r'(?:Best Practice|Recommendation|Note)[：:]\s*(.{20,200})',
    ]
    for pattern in practice_patterns:
        for match in re.finditer(pattern, body):
            advice = match.group(1).strip()
            qa_pairs.append({
                "type": "best_practice",
                "question": f"关于 {title}, 有什么最佳实践?",
                "answer": advice[:300],
                "source": str(filepath.relative_to(ROOT)),
            })

    # 4. 故障排查类 QA (从 troubleshooting 相关内容)
    if 'troubleshoot' in str(filepath).lower() or 'troubleshoot' in body.lower():
        qa_pairs.append({
            "type": "troubleshooting",
            "question": f"如何排查 {title} 相关的故障?",
            "answer": f"参见 {filepath.relative_to(ROOT)} 的故障排查章节。",
            "source": str(filepath.relative_to(ROOT)),
        })

    # 5. 对比类 QA (从包含"对比"、"区别"、"vs"的内容)
    if any(kw in body.lower() for kw in ['对比', '区别', ' vs ', 'versus', '比较']):
        qa_pairs.append({
            "type": "comparison",
            "question": f"{title} 中有哪些技术对比?",
            "answer": f"参见 {filepath.relative_to(ROOT)} 中的对比分析。",
            "source": str(filepath.relative_to(ROOT)),
        })

    return qa_pairs[:10]  # 每个文档最多 10 个 QA


def generate_fta_qa(filepath: Path, fm: dict, body: str) -> list:
    """为 FTA 故障树文档生成专门的 QA 对"""
    qa_pairs = []
    title = fm.get('title', filepath.stem)

    # 提取故障树的顶事件和底事件
    top_events = re.findall(r'(?:顶事件|Top Event|故障现象)[：:]\s*(.+)', body)
    bottom_events = re.findall(r'(?:底事件|Bottom Event|根因|原因)[：:]\s*(.+)', body)

    if top_events:
        for event in top_events[:3]:
            qa_pairs.append({
                "type": "fault_tree",
                "question": f"当出现「{event.strip()}」时, 可能的根因是什么?",
                "answer": f"参见 {filepath.relative_to(ROOT)} 的故障树分析。",
                "source": str(filepath.relative_to(ROOT)),
            })

    # 提取诊断步骤
    diagnosis_steps = re.findall(r'(?:诊断|排查|检查|验证)[步骤方法]*[：:]\s*(.{10,150})', body)
    if diagnosis_steps:
        qa_pairs.append({
            "type": "diagnosis",
            "question": f"如何诊断 {title} 相关问题?",
            "answer": f"参见 {filepath.relative_to(ROOT)} 的诊断步骤。",
            "source": str(filepath.relative_to(ROOT)),
        })

    return qa_pairs


def iter_doc_files(root: Path, output_dir: Path):
    """按目录约定遍历文档，跳过 README、排除目录与 QA 输出目录自身。"""
    for md_file in sorted(root.rglob('*.md')):
        if md_file.name == 'README.md':
            continue
        if EXCLUDE_DIRS & set(md_file.parts):
            continue
        if output_dir in md_file.parents:
            continue
        yield md_file


def entry_key(entry: dict) -> tuple:
    return (entry.get('type'), entry.get('question'), entry.get('source'))


def merge_pairs(existing: list, generated: list) -> list:
    """保留 source 仍存在的既有条目（含手工条目），追加未重复的新条目。"""
    kept = [e for e in existing
            if isinstance(e, dict) and e.get('source') and (ROOT / e['source']).exists()]
    seen = {entry_key(e) for e in kept}
    for g in generated:
        key = entry_key(g)
        if key not in seen:
            seen.add(key)
            kept.append(g)
    return kept


def load_existing(path: Path) -> list:
    if not path.exists():
        return []
    try:
        data = yaml.safe_load(path.read_text(encoding='utf-8')) or {}
    except yaml.YAMLError:
        return []
    pairs = data.get('qa_pairs')
    return pairs if isinstance(pairs, list) else []


def write_qa_file(path: Path, domain: str, pairs: list):
    path.write_text(yaml.dump({
        "domain": domain,
        "total_questions": len(pairs),
        "generated_at": date.today().isoformat(),
        "qa_pairs": pairs,
    }, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description='生成/合并 Agent QA 对语料')
    parser.add_argument('--output-dir', default=str(DEFAULT_OUTPUT_DIR),
                        help=f'输出目录（默认 {DEFAULT_OUTPUT_DIR.relative_to(ROOT)}），用于试运行')
    args = parser.parse_args()

    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    total_qa = 0
    skipped_domains = []

    # 处理核心 domain
    for domain_name in CORE_DOMAINS:
        domain_dir = resolve_domain_dir(domain_name)
        if domain_dir is None or not domain_dir.exists():
            skipped_domains.append(domain_name)
            continue

        generated = []
        for md_file in iter_doc_files(domain_dir, output_dir):
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            generated.extend(generate_qa_from_doc(md_file, fm, body))

        if not generated and not (output_dir / f"{domain_name}-qa.yaml").exists():
            continue

        output_file = safe_output_path(output_dir, f"{domain_name}-qa.yaml")
        pairs = merge_pairs(load_existing(output_file), generated)
        write_qa_file(output_file, domain_name, pairs)
        total_qa += len(pairs)
        print(f"  {domain_name}: {len(pairs)} QA pairs")

    # 处理 FTA 故障树
    if FTA_LIST_DIR.exists():
        generated = []
        for md_file in iter_doc_files(FTA_LIST_DIR, output_dir):
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            generated.extend(generate_fta_qa(md_file, fm, body))

        output_file = safe_output_path(output_dir, "topic-fta-qa.yaml")
        pairs = merge_pairs(load_existing(output_file), generated)
        write_qa_file(output_file, "topic-fta", pairs)
        total_qa += len(pairs)
        print(f"  topic-fta: {len(pairs)} QA pairs")

    # 处理 topic-skills
    if TOPIC_SKILLS_DIR.exists():
        generated = []
        for md_file in iter_doc_files(TOPIC_SKILLS_DIR, output_dir):
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            generated.extend(generate_qa_from_doc(md_file, fm, body))

        output_file = safe_output_path(output_dir, "topic-skills-qa.yaml")
        pairs = merge_pairs(load_existing(output_file), generated)
        write_qa_file(output_file, "topic-skills", pairs)
        total_qa += len(pairs)
        print(f"  topic-skills: {len(pairs)} QA pairs")

    # 处理 application-architecture（现行目录 04-应用模式/02-行业架构）
    if APP_ARCH_DIR.exists():
        generated = []
        for md_file in iter_doc_files(APP_ARCH_DIR, output_dir):
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            generated.extend(generate_qa_from_doc(md_file, fm, body))

        output_file = safe_output_path(output_dir, "topic-application-architecture-qa.yaml")
        pairs = merge_pairs(load_existing(output_file), generated)
        write_qa_file(output_file, "topic-application-architecture", pairs)
        total_qa += len(pairs)
        print(f"  topic-application-architecture: {len(pairs)} QA pairs")

    # 生成索引 README（保留已有 frontmatter）
    readme_path = output_dir / 'README.md'
    frontmatter = ""
    if readme_path.exists():
        frontmatter = split_readme_frontmatter(readme_path.read_text(encoding='utf-8'))

    lines = ["# Agent QA 对语料库", "",
             f"> **生成日期**: {date.today().isoformat()}",
             f"> **QA 对总数**: {total_qa}", "",
             "## 用途", "",
             "- RAG 检索质量评测",
             "- Agent fine-tuning 数据",
             "- 检索相关性验证", "",
             "## 文件索引", "",
             "| 文件 | 领域 | QA 数量 |",
             "|------|------|--------|"]
    for f in sorted(output_dir.glob('*-qa.yaml')):
        with open(f, 'r', encoding='utf-8') as fh:
            data = yaml.safe_load(fh)
        lines.append(f"| {f.name} | {data.get('domain', '')} | {data.get('total_questions', 0)} |")
    with safe_output_path(output_dir, 'README.md').open('w', encoding='utf-8') as f:
        f.write(frontmatter + "\n".join(lines) + "\n")

    print(f"\n{'='*60}")
    print(f"QA 对语料生成完成:")
    print(f"  总 QA 对: {total_qa}")
    print(f"  输出目录: {output_dir}")
    if skipped_domains:
        print(f"  未解析到的 domain: {', '.join(skipped_domains)}")


if __name__ == '__main__':
    main()
