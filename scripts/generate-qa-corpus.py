#!/usr/bin/env python3
"""
Agent QA 对语料生成脚本
从现有 domain/topic 文档自动生成 question-answer 对,
用于 RAG 评测、Agent fine-tuning、和检索质量验证。

输出格式: 每个 domain 一个 YAML 文件, 包含 QA pairs
"""

import os
import re
import yaml
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
OUTPUT_DIR = BASE_DIR / "domain-10-troubleshooting-diagnostics" / "topic-qa-corpus"

# 核心 domain 列表 (优先生成 QA 对)
CORE_DOMAINS = [
    "domain-01-cluster-fundamentals",
    "domain-02-workloads-applications",
    "domain-03-networking-traffic",
    "domain-04-storage-data",
    "domain-05-security-compliance",
    "domain-06-observability",
    "domain-07-platform-engineering",
    "domain-08-release-change-management",
    "domain-09-reliability-engineering",
    "domain-10-troubleshooting-diagnostics",
    "domain-11-production-operations",
    "domain-12-cloud-providers",
    "domain-13-container-runtime",
    "domain-14-ai-ml-infra",
    "domain-15-specialized-tech",
    "domain-16-database-middleware",
    "domain-17-system-foundation",
    "domain-18-manifests-patterns",
    "domain-19-landscape-references",
    "domain-20-application-patterns",
]

# 排除目录
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules', '.obsidian', '.zread', '.claude', '.codebuddy', '.comate', '.github'}


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
    category = fm.get('category', '')
    tags = fm.get('tags', [])

    sections = extract_sections(body)
    concepts = extract_key_concepts(body)
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
                        "answer": f"参见 {filepath.relative_to(BASE_DIR)} 的「{clean_title}」章节。",
                        "source": str(filepath.relative_to(BASE_DIR)),
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
                    "answer": f"参见 {filepath.relative_to(BASE_DIR)} 中的操作步骤。",
                    "source": str(filepath.relative_to(BASE_DIR)),
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
                "source": str(filepath.relative_to(BASE_DIR)),
            })

    # 4. 故障排查类 QA (从 troubleshooting 相关内容)
    if 'troubleshoot' in str(filepath).lower() or 'troubleshoot' in body.lower():
        qa_pairs.append({
            "type": "troubleshooting",
            "question": f"如何排查 {title} 相关的故障?",
            "answer": f"参见 {filepath.relative_to(BASE_DIR)} 的故障排查章节。",
            "source": str(filepath.relative_to(BASE_DIR)),
        })

    # 5. 对比类 QA (从包含"对比"、"区别"、"vs"的内容)
    if any(kw in body.lower() for kw in ['对比', '区别', ' vs ', 'versus', '比较']):
        qa_pairs.append({
            "type": "comparison",
            "question": f"{title} 中有哪些技术对比?",
            "answer": f"参见 {filepath.relative_to(BASE_DIR)} 中的对比分析。",
            "source": str(filepath.relative_to(BASE_DIR)),
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
                "answer": f"参见 {filepath.relative_to(BASE_DIR)} 的故障树分析。",
                "source": str(filepath.relative_to(BASE_DIR)),
            })

    # 提取诊断步骤
    diagnosis_steps = re.findall(r'(?:诊断|排查|检查|验证)[步骤方法]*[：:]\s*(.{10,150})', body)
    if diagnosis_steps:
        qa_pairs.append({
            "type": "diagnosis",
            "question": f"如何诊断 {title} 相关问题?",
            "answer": f"参见 {filepath.relative_to(BASE_DIR)} 的诊断步骤。",
            "source": str(filepath.relative_to(BASE_DIR)),
        })

    return qa_pairs


def main():
    OUTPUT_DIR.mkdir(exist_ok=True)
    total_qa = 0

    # 处理核心 domain
    for domain_name in CORE_DOMAINS:
        domain_dir = BASE_DIR / domain_name
        if not domain_dir.exists():
            continue

        qa_pairs = []
        for md_file in sorted(domain_dir.rglob('*.md')):
            if md_file.name == 'README.md':
                continue
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue

            fm, body = parse_frontmatter(content)
            doc_qa = generate_qa_from_doc(md_file, fm, body)
            qa_pairs.extend(doc_qa)

        if qa_pairs:
            output_file = OUTPUT_DIR / f"{domain_name}-qa.yaml"
            with open(output_file, 'w', encoding='utf-8') as f:
                yaml.dump({
                    "domain": domain_name,
                    "total_questions": len(qa_pairs),
                    "generated_at": "2026-05-19",
                    "qa_pairs": qa_pairs,
                }, f, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
            total_qa += len(qa_pairs)
            print(f"  {domain_name}: {len(qa_pairs)} QA pairs")

    # 处理 FTA 故障树
    fta_dir = BASE_DIR / 'domain-10-troubleshooting-diagnostics' / 'topic-fta' / 'list'
    if fta_dir.exists():
        qa_pairs = []
        for md_file in sorted(fta_dir.glob('*.md')):
            if md_file.name == 'README.md':
                continue
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            doc_qa = generate_fta_qa(md_file, fm, body)
            qa_pairs.extend(doc_qa)

        if qa_pairs:
            output_file = OUTPUT_DIR / "topic-fta-qa.yaml"
            with open(output_file, 'w', encoding='utf-8') as f:
                yaml.dump({
                    "domain": "topic-fta",
                    "total_questions": len(qa_pairs),
                    "generated_at": "2026-05-19",
                    "qa_pairs": qa_pairs,
                }, f, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
            total_qa += len(qa_pairs)
            print(f"  topic-fta: {len(qa_pairs)} QA pairs")

    # 处理 topic-skills
    skills_dir = BASE_DIR / 'domain-10-troubleshooting-diagnostics' / 'topic-skills'
    if skills_dir.exists():
        qa_pairs = []
        for md_file in sorted(skills_dir.glob('*.md')):
            if md_file.name == 'README.md':
                continue
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            doc_qa = generate_qa_from_doc(md_file, fm, body)
            qa_pairs.extend(doc_qa)

        if qa_pairs:
            output_file = OUTPUT_DIR / "topic-skills-qa.yaml"
            with open(output_file, 'w', encoding='utf-8') as f:
                yaml.dump({
                    "domain": "topic-skills",
                    "total_questions": len(qa_pairs),
                    "generated_at": "2026-05-19",
                    "qa_pairs": qa_pairs,
                }, f, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
            total_qa += len(qa_pairs)
            print(f"  topic-skills: {len(qa_pairs)} QA pairs")

    # 处理 application-architecture
    app_dir = BASE_DIR / 'domain-20-application-patterns' / 'topic-application-architecture'
    if app_dir.exists():
        qa_pairs = []
        for md_file in sorted(app_dir.glob('*.md')):
            if md_file.name == 'README.md':
                continue
            try:
                content = md_file.read_text(encoding='utf-8')
            except Exception:
                continue
            fm, body = parse_frontmatter(content)
            doc_qa = generate_qa_from_doc(md_file, fm, body)
            qa_pairs.extend(doc_qa)

        if qa_pairs:
            output_file = OUTPUT_DIR / "topic-application-architecture-qa.yaml"
            with open(output_file, 'w', encoding='utf-8') as f:
                yaml.dump({
                    "domain": "topic-application-architecture",
                    "total_questions": len(qa_pairs),
                    "generated_at": "2026-05-19",
                    "qa_pairs": qa_pairs,
                }, f, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
            total_qa += len(qa_pairs)
            print(f"  topic-application-architecture: {len(qa_pairs)} QA pairs")

    # 生成索引
    index = {
        "description": "KUDIG-DATABASE Agent QA 对语料库索引",
        "generated_at": "2026-05-19",
        "total_qa_pairs": total_qa,
        "files": [],
    }
    for f in sorted(OUTPUT_DIR.glob('*-qa.yaml')):
        with open(f, 'r', encoding='utf-8') as fh:
            data = yaml.safe_load(fh)
        index["files"].append({
            "file": f.name,
            "domain": data.get("domain", ""),
            "count": data.get("total_questions", 0),
        })

    with open(OUTPUT_DIR / 'README.md', 'w', encoding='utf-8') as f:
        f.write("# Agent QA 对语料库\n\n")
        f.write(f"> **生成日期**: 2026-05-19\n")
        f.write(f"> **QA 对总数**: {total_qa}\n\n")
        f.write("## 用途\n\n")
        f.write("- RAG 检索质量评测\n")
        f.write("- Agent fine-tuning 数据\n")
        f.write("- 检索相关性验证\n\n")
        f.write("## 文件索引\n\n")
        f.write("| 文件 | 领域 | QA 数量 |\n")
        f.write("|------|------|--------|\n")
        for item in index["files"]:
            f.write(f"| {item['file']} | {item['domain']} | {item['count']} |\n")

    print(f"\n{'='*60}")
    print(f"QA 对语料生成完成:")
    print(f"  总 QA 对: {total_qa}")
    print(f"  输出目录: {OUTPUT_DIR}")


if __name__ == '__main__':
    main()
