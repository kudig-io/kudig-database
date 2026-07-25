#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""修复 技能/ 目录重组后失效的绝对路径引用（Obsidian wiki 链接与 md 链接）。

策略：对所有以 `技能/` 开头且当前磁盘上不存在的链接目标，按 basename 在
`技能/` 下重新解析；basename 唯一命中则重写路径，歧义/未命中则记录报告。

用法：
  python3 脚本/fix_skill_refs.py            # dry-run，仅打印统计与报告
  python3 脚本/fix_skill_refs.py --apply    # 实际写回
"""
import os
import re
import sys
import json
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKILL = os.path.join(ROOT, "技能")
APPLY = "--apply" in sys.argv

# 1. basename -> [相对路径(技能/...)] 索引
basename_index = defaultdict(list)
for dirpath, _dirs, files in os.walk(SKILL):
    for fn in files:
        if not fn.endswith(".md"):
            continue
        rel = os.path.relpath(os.path.join(dirpath, fn), ROOT).replace(os.sep, "/")
        basename_index[fn].append(rel)

# 2. 链接匹配
WIKI = re.compile(r"\[\[([^\]\|#]+?)(#[^\]\|]+)?(\|[^\]]*)?\]\]")
MD = re.compile(r"\]\(([^)]+?)\)")

# 目标是否为「技能/ 开头」的内部路径
def is_skill_target(t):
    t = t.strip()
    return t.startswith("技能/") and t.endswith(".md")

# 残余歧义显式消歧（basename 相同、无路径线索的少数文件）
EXPLICIT = {
    "技能/k8s-pod-security-guide.md": "技能/安全/pod-security/最佳实践/k8s-pod-security-guide.md",
    "技能/best-practices/infrastructure/kubernetes-cluster.md": "技能/集群运维/cluster-upgrade/最佳实践/infra-kubernetes-cluster.md",
    "技能/best-practices/best-practices/infrastructure/kubernetes-cluster.md": "技能/集群运维/cluster-upgrade/最佳实践/infra-kubernetes-cluster.md",
    "技能/best-practices/README.md": "技能/集群运维/cluster-upgrade/最佳实践/bp-README.md",
    "技能/best-practices/MOC.md": "技能/集群运维/cluster-upgrade/最佳实践/bp-MOC.md",
    "技能/best-practices/index.md": "技能/集群运维/cluster-upgrade/最佳实践/bp-index.md",
    "技能/best-practices/best-practices/README.md": "技能/集群运维/cluster-upgrade/最佳实践/bp-README-alt.md",
    "技能/best-practices/scenarios/README.md": "技能/集群运维/cluster-upgrade/最佳实践/scen-README.md",
    "技能/best-practices/scenarios/MOC.md": "技能/集群运维/cluster-upgrade/最佳实践/scen-MOC.md",
    "技能/best-practices/deployment/MOC.md": "技能/集群运维/cluster-deployment/MOC.md",
    "技能/training-public/README.md": "技能/工作负载/pod/培训/training-public-README.md",
    "技能/training-public/MOC.md": "技能/工作负载/pod/培训/training-public-MOC.md",
    "技能/training-public/public-training/README.md": "技能/工作负载/pod/培训/public-training-README.md",
    "技能/training-public/topic-presentations/README.md": "技能/工作负载/pod/培训/presentations/README.md",
    "技能/training-public/topic-presentations/MOC.md": "技能/工作负载/pod/培训/presentations/MOC.md",
    "技能/training-public/public-training/one-month/README.md": "技能/工作负载/pod/培训/public-one-month/README.md",
    "技能/training-public/public-training/one-month/projects/p5-graduation-project.md": "技能/工作负载/pod/培训/public-one-month/projects/p5-graduation-project.md",
    "技能/training-public/public-training/one-month/resources/commands-cheatsheet.md": "技能/工作负载/pod/培训/public-one-month/resources/commands-cheatsheet.md",
    "技能/training-public/public-training/one-month/resources/knowledge-map.md": "技能/工作负载/pod/培训/public-one-month/resources/knowledge-map.md",
    "技能/training-public/public-training/one-month/resources/reading-sequence.md": "技能/工作负载/pod/培训/public-one-month/resources/reading-sequence.md",
    "技能/training-public/fundamentals/01-what-is-kubernetes.md": "技能/工作负载/pod/培训/01-what-is-kubernetes.md",
    "技能/training-public/fundamentals/02-pod-basics.md": "技能/工作负载/pod/培训/public-fundamentals-02-pod-basics.md",
    "技能/training-public/fundamentals/03-deployment-basics.md": "技能/工作负载/deployment/培训/03-deployment-basics.md",
    "技能/training-lecturer/README.md": "技能/工作负载/pod/培训/lecturer/lecturer-README.md",
    "技能/training-lecturer/01-introduction/01-what-is-kubernetes.md": "技能/工作负载/pod/培训/lecturer/01-what-is-kubernetes.md",
    "技能/training-lecturer/02-getting-started/02-pod-basics.md": "技能/工作负载/pod/培训/lecturer/02-pod-basics.md",
    "技能/training-lecturer/02-getting-started/03-deployment-basics.md": "技能/工作负载/deployment/培训/lecturer/03-deployment-basics.md",
    "技能/training-lecturer/12-decision-tree/decision-tree-mermaid.md": "技能/工作负载/pod/培训/lecturer/decision-tree-mermaid.md",
    "技能/training-lecturer/11-oncall-qa/oncall-quick-qa.md": "技能/工作负载/pod/培训/lecturer/oncall-quick-qa.md",
    "技能/node/01-node-notready.md": "技能/节点/node/01-node-notready-diagnosis.md",
}

def _suffix_score(target, cand):
    a = target.split("/")
    b = cand.split("/")
    n = 0
    for x, y in zip(reversed(a), reversed(b)):
        if x == y:
            n += 1
        else:
            break
    return n

def resolve(target):
    """返回 (new_path or None, status)。status: ok/exists/ambiguous/missing"""
    target = target.strip()
    disk = os.path.join(ROOT, target)
    if os.path.exists(disk):
        return None, "exists"
    if target in EXPLICIT and os.path.exists(os.path.join(ROOT, EXPLICIT[target])):
        return EXPLICIT[target], "ok"
    base = target.split("/")[-1]
    cands = basename_index.get(base, [])
    if len(cands) == 1:
        return cands[0], "ok"
    if len(cands) == 0:
        return None, "missing"
    # 多候选：按尾部路径段匹配数消歧，取唯一最高分
    scored = sorted(((_suffix_score(target, c), c) for c in cands), reverse=True)
    if scored[0][0] >= 2 and (len(scored) == 1 or scored[0][0] > scored[1][0]):
        return scored[0][1], "ok"
    return None, "ambiguous"

stats = defaultdict(int)
report_amb = []
report_miss = []
changed_files = 0
total_fixed = 0

md_files = []
for dirpath, _dirs, files in os.walk(ROOT):
    if "/.git" in dirpath.replace(os.sep, "/"):
        continue
    for fn in files:
        if fn.endswith(".md"):
            md_files.append(os.path.join(dirpath, fn))

for path in md_files:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    orig = text
    file_fixed = 0

    def wiki_sub(m):
        global file_fixed
        tgt, anchor, alias = m.group(1), m.group(2) or "", m.group(3) or ""
        if not is_skill_target(tgt):
            return m.group(0)
        new, st = resolve(tgt)
        stats[st] += 1
        if st == "ok":
            file_fixed += 1
            return f"[[{new}{anchor}{alias}]]"
        if st == "ambiguous":
            report_amb.append((path, tgt))
        elif st == "missing":
            report_miss.append((path, tgt))
        return m.group(0)

    def md_sub(m):
        global file_fixed
        tgt = m.group(1)
        # 去掉可能的 anchor 和 title
        core = tgt.split(" ")[0]
        anchor = ""
        if "#" in core:
            core, anchor = core.split("#", 1)
            anchor = "#" + anchor
        if not is_skill_target(core):
            return m.group(0)
        new, st = resolve(core)
        stats["md_" + st] += 1
        if st == "ok":
            file_fixed += 1
            return f"]({new}{anchor})"
        if st == "ambiguous":
            report_amb.append((path, core))
        elif st == "missing":
            report_miss.append((path, core))
        return m.group(0)

    text = WIKI.sub(wiki_sub, text)
    text = MD.sub(md_sub, text)

    if text != orig:
        changed_files += 1
        total_fixed += file_fixed
        if APPLY:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text)

print("=== stats ===")
for k, v in sorted(stats.items()):
    print(f"{v:6d}  {k}")
print(f"changed_files={changed_files} total_fixed={total_fixed} apply={APPLY}")
print(f"\n=== ambiguous ({len(report_amb)}) sample ===")
for p, t in report_amb[:25]:
    print(f"  {os.path.relpath(p, ROOT)} :: {t}")
print(f"\n=== missing ({len(report_miss)}) sample ===")
for p, t in report_miss[:40]:
    print(f"  {os.path.relpath(p, ROOT)} :: {t}")
