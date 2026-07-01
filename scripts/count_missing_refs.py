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
    "domain-01-cluster-fundamentals", "domain-02-workloads-applications",
    "domain-03-networking-traffic", "domain-04-storage-data",
    "domain-05-security-compliance", "domain-06-observability",
    "domain-07-platform-engineering", "domain-08-release-change-management",
    "domain-09-reliability-engineering", "domain-10-troubleshooting-diagnostics",
    "domain-11-production-operations", "domain-12-cloud-providers",
    "domain-13-container-runtime", "domain-14-ai-ml-infra",
    "domain-15-specialized-tech", "domain-16-database-middleware",
    "domain-17-system-foundation", "domain-18-manifests-patterns",
    "domain-19-landscape-references", "domain-20-application-patterns",
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
