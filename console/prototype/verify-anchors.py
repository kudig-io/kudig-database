#!/usr/bin/env python3
"""Prototype gate: every matrix cell must point at a real heading line in the source page.

Run from the repository root:
  python3 console/prototype/verify-anchors.py        # exit 1 on any mismatch

This is the D2 check from console/spec/acceptance.md, narrowed to what the prototype draws.
It reads console/data/graph/blocks.json and the ORIGINAL Markdown, so it fails if either drifts.
"""

import json
import pathlib
import re
import sys

FM = re.compile(r"\A---\n(.*?)\n---\n", re.S)
b = json.load(open('console/data/graph/blocks.json'))['pages']
# Entities come from console/spec/entities.json; each must resolve to a real page whose
# outline actually contains the section its dimension binding points at.
ents = json.loads(pathlib.Path('console/data/graph/entities.json').read_text())
checks = {}
for e in ents['entities']:
    if e.get('page'):
        titles = [v['title'] for v in (e.get('dims') or {}).values() if v.get('title')]
        if titles:
            checks[e['page'].rsplit('/', 1)[-1]] = titles[:3]
ok = True
for slug, wanted in checks.items():
    pid = next(k for k in b if k.endswith(slug))
    raw = pathlib.Path(pid + '.md').read_text()
    body = FM.sub('', raw, count=1).split('\n')
    for title in wanted:
        node = next((o for o in b[pid]['outline'] if title in o['t']), None)
        if not node:
            print('MISSING', slug, title)
            ok = False
            continue
        raw_line = body[node['s'] - 1]
        # `<!-- chunk: … -->` may share a line with the heading; the builder blanks it in place,
        # so the verifier must do the same before looking for the leading '#'.
        line = re.sub(r'<!--[\s\S]*?-->', '', raw_line).strip()
        hit = title in line and re.match(r'^#{2,3}\s', line) is not None   # H2 or H3 both count
        level = len(line) - len(line.lstrip('#'))
        print(('OK  ' if hit else 'FAIL'), slug[:22].ljust(24), 'L' + str(node['s']).rjust(3),
              f'H{level}', title.ljust(14), '→', line[:40])
        ok = ok and hit
print('ALL ANCHORS OK' if ok else 'MISMATCH')

# --- vocabulary regression (console/spec/acceptance.md §2.1) -------------------------------
vocab = json.loads(pathlib.Path('console/spec/dimensions.json').read_text())
dims = json.loads(pathlib.Path('console/data/graph/blocks.json').read_text())
known = {d['id'] for d in vocab['dimensions']}
pages = list(dims['pages'].values())
# The baseline in labels.md §4 is measured on 07-数据库中间件 (60 non-index pages), not the whole
# library — the share of pages mentioning "备份" differs by an order of magnitude between domains.
scope = [p for k, p in dims['pages'].items() if k.startswith('wiki/07-数据库中间件') and p['shape'] != 'index-like']
# 1) precision + recall of the 07-domain baseline (labels.md §4)
BASE = {'d-monitor': 0.63, 'd-backup': 0.45, 'd-ha': 0.32}
viol = []
for dim, want in BASE.items():
    got = sum(1 for p in scope if any(x['id'] == dim for x in p['dims'])) / max(len(scope), 1)
    viol.append((dim, round(got, 3), want))
bad_share = [v for v in viol if abs(v[1] - v[2]) > 0.15]  # tolerance: ±15 points
# 2) unknown dimension ids must never appear
all_ids = sorted({x['id'] for p in pages for x in p['dims']})
unknown = [i for i in all_ids if i not in known]
print(f'vocab shares on 07 domain ({len(scope)} pages) (got, want):', viol)
print('unknown dimension ids in blocks.json:', unknown or 'none')
if bad_share or unknown:
    print('VOCAB REGRESSION')
    sys.exit(1)
print('VOCAB OK')
sys.exit(0 if ok else 1)
