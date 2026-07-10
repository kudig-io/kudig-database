#!/usr/bin/env python3
"""Count wikilink references for candidate terms missing from glossary."""
import re, subprocess
from pathlib import Path

# Read candidates
with open("/tmp/r8_cand.txt") as f:
    candidates = [l.strip() for l in f if l.strip()]

# Read existing
with open("/tmp/r8_existing.txt") as f:
    existing = set(l.strip() for l in f if l.strip())

# Filter out existing
candidates = [c for c in candidates if c not in existing]

# Search directories
dirs = [
    "集群基础", "工作负载",
    "网络", "存储",
    "安全", "可观测性",
    "平台工程", "发布变更",
    "可靠性", "故障诊断",
    "生产运维", "云厂商",
    "容器运行时", "AI基础设施",
    "专项技术", "数据库中间件",
    "系统基础", "清单模式",
    "生态参考", "应用模式",
    "concepts", "synthesis", "entities", "best-practices",
]

results = []
for term in candidates:
    pattern = f"[[{term}]]"
    count = 0
    for d in dirs:
        p = Path(d)
        if not p.exists():
            continue
        for md in p.rglob("*.md"):
            try:
                content = md.read_text(encoding="utf-8", errors="ignore")
                if pattern in content:
                    count += 1
            except:
                pass
    if count >= 3:
        results.append((count, term))

results.sort(reverse=True)
for count, term in results[:60]:
    print(f"{count:3d} {term}")
