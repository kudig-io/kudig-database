#!/usr/bin/env python3
"""
导出 llm-wiki 语料到 release/，供 Agent 使用。
语料筛选严格由 _meta/corpus-config/profiles/<profile>.yaml 的 include/exclude 决定。

输出结构：
  output/
  ├── manifest.json        (含 profile 名称与 supplementary 计数)
  ├── index.json
  ├── corpus/
  │   ├── core/            (按 tier 分目录，不再生成冗余的 all/)
  │   ├── supporting/
  │   └── peripheral/
  ├── qa/
  │   ├── qa-corpus.jsonl
  │   └── raw/             (原始 QA YAML/JSON 源文件)
  └── metadata/
      ├── page-stats.json
      ├── intent-corpus/   (意图识别语料)
      ├── agent-specs/     (Agent 规范文档)
      └── taxonomy/        (分类体系)

用法:
  python export_corpus_for_nas.py                         # 默认: rag-full-profile.yaml → release/
  python export_corpus_for_nas.py -p rag-sre-profile.yaml # 切换 profile
  python export_corpus_for_nas.py -o /tmp/export          # 自定义输出目录
"""

import argparse
import fnmatch
import json
import re
import shutil
import subprocess
from collections import defaultdict
from datetime import datetime
from pathlib import Path

import yaml

# 始终排除的系统/构建目录（与 profile 无关，不可进入语料或 QA 扫描）
_HARD_EXCLUDED_DIRS = (
    '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
    '.comate/', '.claude/', '.codebuddy/', '.qoder/',
    '.understand-anything/', '.zread/', '.github/',
    'release/', 'node_modules/',
)
_HARD_EXCLUDED_SEGMENTS = {
    'node_modules', '_archives', '_raw', '_staging',
    '.git', '.obsidian', '.comate', '.claude', '.codebuddy',
    '.qoder', '.understand-anything', '.zread',
}
# 自身产物/备份目录的顶层前缀：任何 release* / 历史导出名都不可被重新当作源扫描
# （否则备份里的 qa-corpus.jsonl 会被 collect_qa_pairs 二次吸入，导致计数翻倍）
_OUTPUT_DIR_PREFIXES = ('release', 'kudig-corpus-export', 'corpus-export')
# QA 语料目录：其内容是训练数据（流入 qa/），不作为知识页面进入 corpus
# （脚本已用 copy_qa_raw_sources 将其导出到 qa/raw/，不应再以页面形式重复收录）
_QA_CORPUS_DIRS = (
    'domain-10-troubleshooting-diagnostics/topic-qa-corpus/',
)
# 始终排除的非文本后缀（语料库只承载文本知识）
_HARD_EXCLUDED_SUFFIXES = (
    '.pdf', '.png', '.jpg', '.jpeg', '.gif', '.svg', '.webp',
    '.xlsx', '.xls', '.doc', '.docx', '.ppt', '.pptx',
    '.zip', '.tar', '.gz', '.tgz', '.lock', '.base',
)


def _hard_excluded(rel: str) -> bool:
    """系统/构建/产物目录的硬排除，独立于 profile。"""
    if rel.startswith(_HARD_EXCLUDED_DIRS):
        return True
    if any(part in _HARD_EXCLUDED_SEGMENTS for part in Path(rel).parts):
        return True
    # 排除自身的产物/备份目录（release/、release.backup-*/、kudig-corpus-export/ 等）
    first = Path(rel).parts[0] if Path(rel).parts else ''
    if first.startswith(_OUTPUT_DIR_PREFIXES):
        return True
    return False


def load_profile(vault: Path, profile_name: str = 'rag-full-profile.yaml') -> dict:
    """从 _meta/corpus-config/profiles/ 加载语料 profile（单一事实来源）。"""
    profile_path = vault / '_meta' / 'corpus-config' / 'profiles' / profile_name
    if not profile_path.exists():
        raise FileNotFoundError(
            f"语料 profile 未找到: {profile_path}\n"
            f"可用 profile: {sorted(p.name for p in (vault / '_meta' / 'corpus-config' / 'profiles').glob('*.yaml'))}"
        )
    with open(profile_path, encoding='utf-8') as f:
        return yaml.safe_load(f)


def _match_exclude(rel: str, pattern: str) -> bool:
    """判断 rel 是否命中单条 exclude 规则。"""
    p = pattern.strip()
    # 目录前缀规则（以 / 结尾）
    if p.endswith('/'):
        prefix = p
        return rel.startswith(prefix)
    # ** 通配：剥成路径与 basename 两种 fnmatch
    if '**' in p:
        core = p.replace('**/', '').replace('**', '*')
        return fnmatch.fnmatch(rel, p) or fnmatch.fnmatch(rel, core) or fnmatch.fnmatch(Path(rel).name, core)
    # 普通通配：对全路径与 basename 各试一次
    return fnmatch.fnmatch(rel, p) or fnmatch.fnmatch(Path(rel).name, p)


def should_include(rel: str, profile: dict) -> bool:
    """严格按 profile 的 include + exclude 决定 rel 是否进入语料。"""
    if _hard_excluded(rel):
        return False
    if any(rel.lower().endswith(s) for s in _HARD_EXCLUDED_SUFFIXES):
        return False
    # QA 训练数据目录不作为知识页面（其内容经 qa/ 通道导出）
    if rel.startswith(_QA_CORPUS_DIRS):
        return False
    # include 白名单：必须落在某个 include path 之下
    includes = [item['path'] for item in (profile.get('include') or [])
                if isinstance(item, dict) and 'path' in item]
    if not any(rel.startswith(inc) for inc in includes):
        return False
    # exclude 黑名单：命中任意一条即排除
    for pat in (profile.get('exclude') or []):
        if isinstance(pat, str) and _match_exclude(rel, pat):
            return False
    return True


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


def estimate_tokens(text: str) -> int:
    return len(text) // 4


def _is_qa_file(qf: Path, rel_str: str) -> bool:
    """判断文件是否为 QA 语料文件，避免误匹配 ar-QA.json、faq.yaml 等。"""
    if 'qa-corpus' in rel_str:
        return True
    stem = qf.stem.lower()
    if stem.endswith('-qa') or stem.startswith('qa-') or stem == 'qa':
        return True
    return False


def collect_qa_pairs(vault: Path) -> list:
    """收集 QA 语料对，支持多种 schema。"""
    qa_pairs = []

    for qf in vault.rglob('*'):
        if not qf.is_file():
            continue
        if qf.suffix not in ['.json', '.jsonl', '.yaml', '.yml']:
            continue
        if 'schema' in qf.name.lower():
            continue
        rel_str = str(qf.relative_to(vault)).lower()
        if _hard_excluded(str(qf.relative_to(vault))):
            continue
        if not _is_qa_file(qf, rel_str):
            continue

        try:
            content = qf.read_text(encoding='utf-8')

            if qf.suffix == '.jsonl':
                for line in content.splitlines():
                    line = line.strip()
                    if not line:
                        continue
                    data = json.loads(line)
                    if isinstance(data, dict):
                        qa_pairs.extend(_normalize_qa_item(data, str(qf.relative_to(vault))))

            elif qf.suffix == '.json':
                data = json.loads(content)
                if isinstance(data, list):
                    for item in data:
                        if isinstance(item, dict):
                            qa_pairs.extend(_normalize_qa_item(item, str(qf.relative_to(vault))))
                elif isinstance(data, dict):
                    # 可能是 {qa_pairs: [...]} 结构
                    for key in ['qa_pairs', 'pairs', 'items', 'data']:
                        if key in data and isinstance(data[key], list):
                            for item in data[key]:
                                if isinstance(item, dict):
                                    qa_pairs.extend(_normalize_qa_item(item, str(qf.relative_to(vault))))

            elif qf.suffix in ['.yaml', '.yml']:
                data = yaml.safe_load(content)
                if isinstance(data, list):
                    for item in data:
                        if isinstance(item, dict):
                            qa_pairs.extend(_normalize_qa_item(item, str(qf.relative_to(vault))))
                elif isinstance(data, dict):
                    for key in ['qa_pairs', 'pairs', 'items', 'data']:
                        if key in data and isinstance(data[key], list):
                            for item in data[key]:
                                if isinstance(item, dict):
                                    qa_pairs.extend(_normalize_qa_item(item, str(qf.relative_to(vault))))

        except Exception as e:
            print(f"  Warning: failed to parse {qf}: {e}")

    # 去重：同一份生成数据常以 .json/.yaml 双格式、以及 *-all 与 p0/p1/p2 合集形式重复，
    # 按 (input, output) 归一化后保留首次出现，避免训练语料重复。
    seen = set()
    deduped = []
    for q in qa_pairs:
        key = (
            _norm_text(q.get('input', '')),
            _norm_text(q.get('output', '')),
        )
        if key in seen:
            continue
        seen.add(key)
        deduped.append(q)
    if len(deduped) != len(qa_pairs):
        print(f"  QA dedup: {len(qa_pairs)} -> {len(deduped)} (removed {len(qa_pairs) - len(deduped)} duplicates)")
    return deduped


def _norm_text(s) -> str:
    """归一化文本用于去重：去除首尾空白、折叠连续空白。"""
    return re.sub(r'\s+', ' ', str(s)).strip()


def _normalize_qa_item(item: dict, source: str) -> list:
    """将不同 schema 的 QA item 标准化为 input/output 格式。"""
    results = []

    # Schema 1: input/output
    if 'input' in item and 'output' in item:
        results.append({
            'source': source,
            'input': item['input'],
            'output': item['output'],
            'tags': item.get('tags', []),
            'type': item.get('type', 'qa'),
        })

    # Schema 2: question/answer
    elif 'question' in item and 'answer' in item:
        results.append({
            'source': source,
            'input': item['question'],
            'output': item['answer'],
            'tags': item.get('tags', []),
            'type': item.get('type', 'qa'),
        })

    # Schema 3: command/output_pattern/diagnosis/action (工单诊断语料)
    elif 'command' in item or 'output_pattern' in item:
        input_text = f"命令: {item.get('command', '')}\n"
        if item.get('output_pattern'):
            input_text += f"输出:\n{item['output_pattern']}\n"
        input_text += f"场景: {item.get('scenario', '')}\n"
        input_text += f"严重级别: {item.get('severity', '')}"

        output_text = ""
        if item.get('diagnosis'):
            if isinstance(item['diagnosis'], list):
                output_text += "诊断:\n" + "\n".join(item['diagnosis']) + "\n"
            else:
                output_text += f"诊断: {item['diagnosis']}\n"
        if item.get('action'):
            if isinstance(item['action'], list):
                output_text += "操作:\n" + "\n".join(item['action']) + "\n"
            else:
                output_text += f"操作: {item['action']}\n"

        results.append({
            'source': source,
            'input': input_text.strip(),
            'output': output_text.strip(),
            'tags': item.get('tags', []),
            'type': item.get('type', 'diagnosis'),
            'skill_ref': item.get('skill_ref', ''),
            'io_pair_id': item.get('io_pair_id', ''),
        })

    return results


def copy_qa_raw_sources(vault: Path, qa_dir: Path) -> int:
    """复制 QA 语料原始文件（YAML/JSON/MD）到 qa/raw/。"""
    raw_dir = qa_dir / 'raw'
    raw_dir.mkdir(parents=True, exist_ok=True)
    qa_corpus_dir = vault / 'domain-10-troubleshooting-diagnostics' / 'topic-qa-corpus'
    if not qa_corpus_dir.exists():
        return 0

    count = 0
    for f in qa_corpus_dir.rglob('*'):
        if not f.is_file():
            continue
        rel = f.relative_to(qa_corpus_dir)
        rel_str = str(rel)
        if rel_str.startswith('_schema/') or rel_str.startswith('.'):
            continue
        target = raw_dir / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(f, target)
        count += 1

    schema_dir = qa_corpus_dir / '_schema'
    if schema_dir.exists():
        out_schema = raw_dir / '_schema'
        out_schema.mkdir(parents=True, exist_ok=True)
        for f in schema_dir.iterdir():
            if f.is_file():
                shutil.copy2(f, out_schema / f.name)
                count += 1

    return count


def copy_intent_corpus(vault: Path, metadata_dir: Path) -> int:
    """复制意图识别语料 JSONL。"""
    intent_dir = metadata_dir / 'intent-corpus'
    intent_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    agent_specs = vault / 'docs' / 'agent-specs'
    if not agent_specs.exists():
        return 0
    for f in agent_specs.iterdir():
        if f.suffix == '.jsonl':
            shutil.copy2(f, intent_dir / f.name)
            count += 1
    return count


def copy_agent_specs(vault: Path, metadata_dir: Path) -> int:
    """复制 Agent 规范文档（.md）。"""
    specs_dir = metadata_dir / 'agent-specs'
    specs_dir.mkdir(parents=True, exist_ok=True)
    agent_specs = vault / 'docs' / 'agent-specs'
    if not agent_specs.exists():
        return 0
    count = 0
    for f in agent_specs.iterdir():
        if f.is_file() and f.suffix == '.md':
            shutil.copy2(f, specs_dir / f.name)
            count += 1
    return count


def copy_taxonomy(vault: Path, metadata_dir: Path) -> int:
    """复制分类体系文件。"""
    tax_dir = metadata_dir / 'taxonomy'
    tax_dir.mkdir(parents=True, exist_ok=True)
    meta_dir = vault / '_meta'
    count = 0
    if meta_dir.exists():
        for name in ['taxonomy.md', 'schema.md', 'README.md']:
            f = meta_dir / name
            if f.exists():
                shutil.copy2(f, tax_dir / name)
                count += 1
    top_files = ['STRUCTURE.md', 'AGENTS.md']
    for name in top_files:
        f = vault / name
        if f.exists():
            shutil.copy2(f, tax_dir / name)
            count += 1
    return count


def export_corpus(vault: Path, output_dir: Path, profile: dict, profile_name: str):
    if output_dir.exists():
        subprocess.run(['chmod', '-R', 'u+rwx', str(output_dir)], check=False, capture_output=True)
        subprocess.run(['chflags', '-R', 'nouchg', str(output_dir)], check=False, capture_output=True)
        subprocess.run(['rm', '-rf', str(output_dir)], check=True)
    output_dir.mkdir(parents=True)

    corpus_dir = output_dir / 'corpus'
    qa_dir = output_dir / 'qa'
    metadata_dir = output_dir / 'metadata'

    # 仅按 tier 分目录；不再生成冗余的 corpus/all/（= core+peripheral+supporting 的并集）
    for d in [corpus_dir / 'core', corpus_dir / 'supporting', corpus_dir / 'peripheral', qa_dir, metadata_dir]:
        d.mkdir(parents=True, exist_ok=True)

    # 严格按 profile 的 include/exclude 筛选语料页面
    md_files = [p for p in vault.rglob('*.md') if should_include(str(p.relative_to(vault)), profile)]

    pages = {}
    incoming = defaultdict(int)
    lookup = {}

    # 第一遍：收集信息
    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, body = parse_frontmatter(text)
        fm = load_fm(fm_text) if fm_text else {}

        tier = fm.get('tier', 'supporting')
        if tier not in ['core', 'supporting', 'peripheral']:
            tier = 'supporting'

        pages[rel] = {
            'source_path': rel,
            'title': fm.get('title', Path(rel).stem),
            'summary': fm.get('summary', ''),
            'category': fm.get('category', ''),
            'tags': [t for t in (fm.get('tags') or []) if isinstance(t, str)],
            'tier': tier,
            'body': body,
            'full_text': text,
            'tokens': estimate_tokens(text),
            'chars': len(text),
        }

        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel

    # 计算入链
    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['full_text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in lookup:
                incoming[lookup[target]] += 1
            elif '/' in target:
                basename = target.split('/')[-1]
                if basename in lookup:
                    incoming[lookup[basename]] += 1

    # 导出语料文件
    manifest_pages = []
    tier_counts = defaultdict(int)

    for rel, info in pages.items():
        tier = info['tier']
        tier_counts[tier] += 1

        # 仅写入对应 tier 目录（不再冗余写入 corpus/all/）
        target_path = corpus_dir / tier / rel
        target_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(vault / rel, target_path)

        manifest_pages.append({
            'path': rel,
            'title': info['title'],
            'summary': info['summary'],
            'category': info['category'],
            'tags': info['tags'],
            'tier': tier,
            'incoming_links': incoming[rel],
            'tokens': info['tokens'],
        })

    # 收集 QA
    qa_pairs = collect_qa_pairs(vault)
    qa_path = qa_dir / 'qa-corpus.jsonl'
    with open(qa_path, 'w', encoding='utf-8') as f:
        for pair in qa_pairs:
            f.write(json.dumps(pair, ensure_ascii=False) + '\n')

    # 补充数据：原始 QA 源文件、意图语料、Agent 规范、分类体系
    qa_raw_count = copy_qa_raw_sources(vault, qa_dir)
    intent_count = copy_intent_corpus(vault, metadata_dir)
    specs_count = copy_agent_specs(vault, metadata_dir)
    taxonomy_count = copy_taxonomy(vault, metadata_dir)

    # 生成 index.json
    index = {
        'total_pages': len(pages),
        'total_tokens': sum(p['tokens'] for p in pages.values()),
        'tier_counts': dict(tier_counts),
        'qa_pairs': len(qa_pairs),
        'pages': manifest_pages,
    }
    index_path = output_dir / 'index.json'
    index_path.write_text(json.dumps(index, ensure_ascii=False, indent=2), encoding='utf-8')

    # 生成 manifest.json
    manifest = {
        'name': 'kudig-database-corpus',
        'version': '2026.07.02',
        'generated_at': datetime.now().isoformat(),
        'source_vault': str(vault),
        'profile': profile_name,
        'total_pages': len(pages),
        'total_tokens': sum(p['tokens'] for p in pages.values()),
        'tier_counts': dict(tier_counts),
        'qa_pairs': len(qa_pairs),
        'supplementary': {
            'qa_raw_files': qa_raw_count,
            'intent_corpus_files': intent_count,
            'agent_spec_files': specs_count,
            'taxonomy_files': taxonomy_count,
        },
        'files': {
            'corpus': 'corpus/',
            'index': 'index.json',
            'qa': 'qa/qa-corpus.jsonl',
            'qa_raw': 'qa/raw/',
            'intent_corpus': 'metadata/intent-corpus/',
            'agent_specs': 'metadata/agent-specs/',
            'taxonomy': 'metadata/taxonomy/',
            'metadata': 'metadata/page-stats.json',
        },
    }
    manifest_path = output_dir / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')

    # 生成 metadata
    stats = {
        'total_pages': len(pages),
        'total_tokens': sum(p['tokens'] for p in pages.values()),
        'tier_counts': dict(tier_counts),
        'category_counts': dict(),
        'tag_counts': dict(),
    }
    cat_counts = defaultdict(int)
    tag_counts = defaultdict(int)
    for info in pages.values():
        cat_counts[info['category']] += 1
        for tag in info['tags']:
            tag_counts[tag] += 1
    stats['category_counts'] = dict(cat_counts)
    stats['tag_counts'] = dict(tag_counts)

    stats_path = metadata_dir / 'page-stats.json'
    stats_path.write_text(json.dumps(stats, ensure_ascii=False, indent=2), encoding='utf-8')

    subprocess.run(['chflags', '-R', 'nouchg', str(output_dir)], check=False,
                   capture_output=True)

    print(f"\nExport completed: {output_dir}")
    print(f"  Total pages: {len(pages)}")
    print(f"  Total tokens: {sum(p['tokens'] for p in pages.values()):,}")
    print(f"  Tier counts: {dict(tier_counts)}")
    print(f"  QA pairs: {len(qa_pairs)}")
    print("  Supplementary:")
    print(f"    - QA raw files: {qa_raw_count}")
    print(f"    - Intent corpus files: {intent_count}")
    print(f"    - Agent spec files: {specs_count}")
    print(f"    - Taxonomy files: {taxonomy_count}")
    print("  Files:")
    print(f"    - {manifest_path}")
    print(f"    - {index_path}")
    print(f"    - {qa_path}")
    print(f"    - {qa_dir / 'raw'}")
    print(f"    - {metadata_dir / 'intent-corpus'}")
    print(f"    - {metadata_dir / 'agent-specs'}")
    print(f"    - {metadata_dir / 'taxonomy'}")
    print(f"    - {stats_path}")


def main():
    parser = argparse.ArgumentParser(description='Export KUDIG Database corpus to NAS')
    parser.add_argument('--vault', '-v', type=str,
                        default=str(Path(__file__).resolve().parent.parent),
                        help='Path to the vault root (default: script parent directory)')
    parser.add_argument('--output', '-o', type=str, default='release',
                        help='Output directory for exported corpus (default: release)')
    parser.add_argument('--profile', '-p', type=str, default='rag-full-profile.yaml',
                        help='Corpus profile under _meta/corpus-config/profiles/ (default: rag-full-profile.yaml)')
    args = parser.parse_args()

    vault = Path(args.vault).resolve()
    output_dir = Path(args.output)

    if not vault.is_dir():
        print(f"Error: vault path does not exist: {vault}")
        return

    profile = load_profile(vault, args.profile)
    print(f"Using profile: {args.profile}")

    export_corpus(vault, output_dir, profile, args.profile)


if __name__ == "__main__":
    main()
