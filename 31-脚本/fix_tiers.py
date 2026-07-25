#!/usr/bin/env python3
"""
纠正 tier 与图结构明显矛盾的前端字段（仅处理违反 wiki 定义的明确案例）。

wiki 规则：
  - core:        高入链（≥5）或桥接位
  - supporting:  默认（中等连接）
  - peripheral:  低连接（≤1 入链）

仅纠正「tier 与入链数自相矛盾」的案例（非人工意图覆盖）：
  - peripheral & 入链 ≥ 20  → core        (高连接中枢被误降)
  - peripheral & 入链 5-19  → supporting  (中等连接被误降)
  - core     & 入链 = 0     → supporting  (零连接被误提)

写回源文件 frontmatter 的 tier: 字段，其它不变。
用法:
  python scripts/fix_tiers.py            # 预览
  python scripts/fix_tiers.py --write    # 实际写回
"""
import argparse
import re
import sys
from pathlib import Path

_FM_RE = re.compile(r'^---\n(.*?)\n---\n', re.DOTALL)
_TIER_RE = re.compile(r'^tier:\s*(\S+)', re.MULTILINE)


def compute_incoming(vault: Path, pages: dict) -> dict:
    """复用 export 脚本的入链计算逻辑。pages: {rel: full_text}。"""
    lookup = {}
    for rel in pages:
        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel
    incoming = {rel: 0 for rel in pages}
    for rel, text in pages.items():
        for link in re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', text):
            tgt = link.split('#')[0].split('?')[0].strip().lower()
            if tgt in incoming:
                incoming[lookup[tgt]] += 1
            elif '/' in tgt and tgt.split('/')[-1] in {Path(r).name.lower() for r in pages}:
                base = tgt.split('/')[-1]
                for r in pages:
                    if Path(r).name.lower() == base:
                        incoming[r] += 1
                        break
    return incoming


def desired_tier(cur: str, links: int) -> str:
    if cur == 'peripheral':
        if links >= 20:
            return 'core'
        if links >= 5:
            return 'supporting'
    if cur == 'core' and links == 0:
        return 'supporting'
    return cur


def main():
    ap = argparse.ArgumentParser(description='纠正 tier 与入链矛盾')
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--vault', '-v', default=str(Path(__file__).resolve().parent.parent))
    args = ap.parse_args()
    vault = Path(args.vault)

    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from export_corpus_for_nas import load_profile, should_include  # noqa: E402

    profile = load_profile(vault, 'rag-full-profile.yaml')

    pages = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        if not should_include(rel, profile):
            continue
        try:
            text = p.read_text(encoding='utf-8')
        except Exception:
            continue
        pages[rel] = text

    incoming = compute_incoming(vault, pages)

    changes = []
    for rel, text in pages.items():
        m = _FM_RE.match(text)
        if not m:
            continue
        tm = _TIER_RE.search(m.group(1))
        if not tm:
            continue
        cur = tm.group(1).strip()
        if cur not in ('core', 'supporting', 'peripheral'):
            continue
        new = desired_tier(cur, incoming.get(rel, 0))
        if new != cur:
            changes.append((rel, cur, new, incoming.get(rel, 0)))

    print(f"待纠正: {len(changes)} 个页面")
    by = {'peripheral->core': 0, 'peripheral->supporting': 0, 'core->supporting': 0}
    for rel, cur, new, links in sorted(changes, key=lambda x: -x[3]):
        by[f'{cur}->{new}'] = by.get(f'{cur}->{new}', 0) + 1
        if args.write or len(changes) <= 80:
            print(f"  入链{links:4d}  {cur:11s} -> {new:11s}  {rel}")

    print(f"\n分布: {by}")
    if args.write:
        for rel, cur, new, _ in changes:
            p = vault / rel
            text = p.read_text(encoding='utf-8')
            m = _FM_RE.match(text)
            fm = m.group(1)
            new_fm = _TIER_RE.sub(f'tier: {new}', fm, count=1)
            p.write_text(f'---\n{new_fm}\n---\n' + text[m.end():], encoding='utf-8')
        print(f"[WRITE] 已写回 {len(changes)} 个文件")
    else:
        print("[DRY-RUN] 加 --write 实际写回")


if __name__ == '__main__':
    main()
