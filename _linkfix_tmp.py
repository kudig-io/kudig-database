#!/usr/bin/env python3
"""历史断链治理：盘点(scan) / 修复(fix) 引擎。
用法:
  python3 _linkfix_tmp.py scan          # 全量盘点与归因分类，输出统计与样本
  python3 _linkfix_tmp.py fix --dry     # 批次1+2 试运行
  python3 _linkfix_tmp.py fix --run     # 批次1+2 执行
"""
import os, re, sys, json, unicodedata
from collections import Counter, defaultdict
from urllib.parse import unquote, quote

ROOT = os.path.dirname(os.path.abspath(__file__))
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'node_modules', '.obsidian',
                '.understand-anything', '.qoder', '.zread', '.zcode', '.mimocode',
                '.codebuddy', '.comate', '.claude', '.github', '33-源码'}

WIKI_RE = re.compile(r"\[\[([^\[\]\|#]+?)(#[^\]\|]*)?((?:\\?\|)[^\[\]]*)?\]\]")
MDLINK_RE = re.compile(r"(\]\()(<?)([^()\s>]+)(>?)((?:\s+\"[^\"]*\")?\))")

def all_md_files():
    out = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS and not (dirpath == ROOT and d.startswith('.'))]
        for f in filenames:
            if f.endswith('.md'):
                out.append(os.path.join(dirpath, f))
    return out

def norm(p):
    return unicodedata.normalize('NFC', p)

def build_index(files):
    """basename(小写、NFC) -> [repo相对路径]"""
    idx = defaultdict(list)
    existing = set()
    for f in files:
        rel = norm(os.path.relpath(f, ROOT))
        existing.add(rel)
        idx[os.path.basename(rel).lower()].append(rel)
    return idx, existing

def is_external(t):
    return t.startswith(('http://', 'https://', 'mailto:', 'ftp:', '#', 'tel:'))

def classify_target(rel_target, existing, idx):
    """rel_target: repo 根相对路径（NFC）。返回 (status, candidates)"""
    if rel_target in existing:
        return 'ok', []
    # 非 md 目标（图片等）只检查存在性
    if not rel_target.endswith('.md'):
        if os.path.exists(os.path.join(ROOT, rel_target)):
            return 'ok', []
        base = os.path.basename(rel_target).lower()
        cands = idx.get(base, [])
        return ('unique' if len(cands) == 1 else 'multi' if cands else 'dead'), cands
    cands = idx.get(os.path.basename(rel_target).lower(), [])
    if len(cands) == 1:
        return 'unique', cands
    if len(cands) > 1:
        return 'multi', cands
    return 'dead', []

def pick_multi(rel_target, cands):
    """歧义匹配启发式：最长路径后缀重合优先，其次同顶级目录。"""
    tparts = rel_target.split('/')
    best, best_score = None, -1
    for c in cands:
        cparts = c.split('/')
        # 从尾部对齐计数相同段数
        score = 0
        for a, b in zip(reversed(tparts), reversed(cparts)):
            if a == b:
                score += 1
            else:
                break
        # 同顶级目录加权
        if tparts[0] == cparts[0]:
            score += 0.5
        if score > best_score:
            best, best_score = c, score
    # 至少要有文件名相同之外的 1 段路径重合或同顶级目录，否则视为不可判定
    return best if best_score >= 1.5 else None

def scan(fix=False, run=False):
    files = all_md_files()
    idx, existing = build_index(files)
    stats = Counter()
    samples = defaultdict(list)
    dead_targets = Counter()
    multi_unresolved = Counter()
    changed_files = 0

    for f in files:
        src_rel = norm(os.path.relpath(f, ROOT))
        src_dir = os.path.dirname(src_rel)
        with open(f, encoding='utf-8') as fh:
            text = fh.read()
        orig = text

        # --- 路径式 wikilink ---
        def wiki_sub(m):
            target, anchor, alias = m.group(1), m.group(2) or '', m.group(3) or ''
            t = norm(target.strip())
            if '/' not in t:
                stats['wiki_nameonly'] += 1
                return m.group(0)
            rt = t if t.endswith('.md') else t + '.md'
            status, cands = classify_target(rt, existing, idx)
            if status == 'ok':
                stats['wiki_ok'] += 1
                return m.group(0)
            stats['wiki_' + status] += 1
            new = None
            if status == 'unique':
                new = cands[0]
            elif status == 'multi':
                new = pick_multi(rt, cands)
                if new is None:
                    multi_unresolved[rt] += 1
                    stats['wiki_multi_unresolved'] += 1
            else:
                dead_targets[rt] += 1
            if new and fix:
                stats['wiki_fixed'] += 1
                nt = new[:-3] if not t.endswith('.md') else new
                return f'[[{nt}{anchor}{alias}]]'
            if len(samples['wiki_' + status]) < 5:
                samples['wiki_' + status].append((src_rel, t))
            return m.group(0)

        text = WIKI_RE.sub(wiki_sub, text)

        # --- markdown 相对链接 ---
        def md_sub(m):
            pre, lt, target, gt, post = m.groups()
            t = target
            if is_external(t):
                return m.group(0)
            anchor = ''
            if '#' in t:
                t, anchor = t.split('#', 1)
                anchor = '#' + anchor
            if not t:
                return m.group(0)
            tdec = norm(unquote(t))
            if t.startswith('/'):
                rel_target = tdec.lstrip('/')
            else:
                rel_target = norm(os.path.normpath(os.path.join(src_dir, tdec)))
            if rel_target.startswith('..'):
                stats['md_outside'] += 1
                return m.group(0)
            if not os.path.splitext(rel_target)[1]:
                # 目录链接
                if os.path.isdir(os.path.join(ROOT, rel_target)):
                    stats['md_ok'] += 1
                else:
                    stats['md_dir_dead'] += 1
                    if len(samples['md_dir_dead']) < 5:
                        samples['md_dir_dead'].append((src_rel, t))
                return m.group(0)
            status, cands = classify_target(rel_target, existing, idx)
            if status == 'ok':
                stats['md_ok'] += 1
                return m.group(0)
            stats['md_' + status] += 1
            new = None
            if status == 'unique':
                new = cands[0]
            elif status == 'multi':
                new = pick_multi(rel_target, cands)
                if new is None:
                    multi_unresolved[rel_target] += 1
                    stats['md_multi_unresolved'] += 1
            else:
                dead_targets[rel_target] += 1
            if new and fix:
                stats['md_fixed'] += 1
                newrel = os.path.relpath(new, src_dir) if src_dir else new
                enc = quote(newrel, safe='/-_.~()')
                return f'{pre}{lt}{enc}{anchor}{gt}{post}'
            if len(samples['md_' + status]) < 5:
                samples['md_' + status].append((src_rel, t))
            return m.group(0)

        text = MDLINK_RE.sub(md_sub, text)

        if fix and run and text != orig:
            with open(f, 'w', encoding='utf-8') as fh:
                fh.write(text)
            changed_files += 1
        elif fix and text != orig:
            changed_files += 1

    print(json.dumps(stats, ensure_ascii=False, indent=1, sort_keys=True))
    print(f'\nchanged_files: {changed_files} (run={run})')
    print('\n--- 样本 ---')
    for k, v in samples.items():
        print(f'[{k}]')
        for s, t in v:
            print(f'  {s} -> {t}')
    print('\n--- 死链目标 TOP30 ---')
    for t, c in dead_targets.most_common(30):
        print(f'  {c:5d}  {t}')
    print('\n--- 歧义未决 TOP20 ---')
    for t, c in multi_unresolved.most_common(20):
        print(f'  {c:5d}  {t}')
    # 落盘完整死链清单
    with open(os.path.join(ROOT, '_linkfix_dead.json'), 'w', encoding='utf-8') as fh:
        json.dump({'dead': dead_targets.most_common(), 'multi_unresolved': multi_unresolved.most_common()}, fh, ensure_ascii=False, indent=1)

if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else 'scan'
    if mode == 'scan':
        scan(fix=False)
    elif mode == 'fix':
        scan(fix=True, run='--run' in sys.argv)
