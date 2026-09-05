#!/usr/bin/env python3
"""
content-quality-metrics.py — 内容质量指标采集（nightly 与本地共用）

产出单项指标：
  - orphan_rate / avg_inbound : 孤儿页率与平均入链数（知识密度）
  - broken_wikilinks          : 断链数（与 quality.yml Broken Wikilink Gate 同逻辑镜像）
  - verified_upon             : 23-实体 / 26-技能 中缺少 verified-upon「截至版本」字段的比例
                                （知识保鲜约定：实体/技能页 frontmatter 加 verified-upon: k8s-1.31 等）

用法：
  python3 31-脚本/maintenance/content-quality-metrics.py --output 35-元数据/metrics/content-quality-latest.json
"""

import argparse
import datetime
import json
import os
import re
from collections import defaultdict
from pathlib import Path

try:
    import yaml
except ImportError:
    yaml = None

# ---- 与 quality.yml Broken Wikilink Gate 保持一致 ----
GATE_EXCL = {"node_modules", ".venv", ".git", "__pycache__", "30-站点", "32-发布", "33-源码"}
GATE_SCAN_EXCL = GATE_EXCL | {"37-归档", "36-报告", "35-元数据", "28-资产", "31-脚本"}
GATE_CONTENT = {"22-概念", "23-实体", "26-技能", "24-综合", "29-文档", "20-最佳实践", "25-研究", "27-标签"}
GATE_DOMAINS = {
    "01-集群基础", "02-工作负载", "05-网络", "06-存储", "08-安全", "09-可观测性",
    "10-平台工程", "11-发布变更", "12-可靠性", "19-故障诊断", "13-生产运维", "18-云厂商",
    "14-容器运行时", "15-AI基础设施", "16-专项技术", "07-数据库中间件", "17-系统基础",
    "03-清单模式", "21-生态参考", "04-应用模式",
}
WIKILINK_GATE_RE = re.compile(r"\[\[([^\]|#]+)(?:[#|][^\]]*)?\]\]")

# ---- 孤儿统计的扫描范围（与 backlink-orphans.py 一致）----
ORPHAN_EXCLUDE = {
    ".git", ".github", ".githooks", ".obsidian", ".claude", ".vscode", ".idea",
    ".venv", ".ruff_cache", "node_modules",
    ".codebuddy", ".comate", ".mimocode", ".qoder", ".understand-anything",
    ".wiki-meta", ".zread", ".impeccable", ".mimosa", ".video_agent",
    "_archives", "_staging", "assets", "web",
    "30-站点", "32-发布", "33-源码", "35-元数据", "36-报告", "37-归档", "GTM",
}
ORPHAN_SKIP_FILES = {"index.md", "log.md", "hot.md"}
LINK_RE = re.compile(r"\[\[([^\]|]+)(?:\|[^\]]+)?\]\]")

VERIFIED_DOMAINS = ("23-实体", "26-技能")
VERIFIED_FIELD = "verified-upon"


def normalize(link_text: str) -> str:
    return link_text.split("|")[0].strip().split("/")[-1].strip().lower()


def frontmatter(content: str):
    if not content.startswith("---"):
        return None
    parts = content.split("---", 2)
    if len(parts) != 3 or yaml is None:
        return None
    try:
        fm = yaml.safe_load(parts[1])
    except yaml.YAMLError:
        return None
    return fm if isinstance(fm, dict) else None


def broken_wikilinks(root: Path) -> int:
    names = set()
    for path in root.rglob("*.md"):
        if any(x in path.parts for x in GATE_EXCL) or path.parts[0].startswith("."):
            continue
        names.add(path.stem)
        names.add(path.name)

    broken = 0
    for path in root.rglob("*.md"):
        if any(x in path.parts for x in GATE_SCAN_EXCL) or path.parts[0].startswith("."):
            continue
        if path.parts[0] not in GATE_DOMAINS and path.parts[0] not in GATE_CONTENT:
            continue
        fence = False
        for line in path.read_text(encoding="utf-8").split("\n"):
            if line.lstrip().startswith("```"):
                fence = not fence
                continue
            if fence:
                continue
            masked = re.sub(r"`[^`]*`", "", line)
            for match in WIKILINK_GATE_RE.finditer(masked):
                target = match.group(1).strip().rstrip("\\")
                if not target:
                    continue
                stem = target.split("/")[-1]
                if stem.endswith(".md"):
                    stem = stem[:-3]
                if stem and stem not in names and target not in names:
                    broken += 1
    return broken


def orphan_stats(root: Path):
    pages = {}
    name_map = defaultdict(list)
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(d for d in dirnames if d not in ORPHAN_EXCLUDE)
        for name in filenames:
            if not name.endswith(".md") or name in ORPHAN_SKIP_FILES:
                continue
            path = Path(dirpath) / name
            rel = path.relative_to(root).as_posix()
            pages[rel] = path
            name_map[path.stem.lower()].append(rel)

    incoming = defaultdict(int)
    for rel, path in pages.items():
        body = path.read_text(encoding="utf-8")
        if body.startswith("---"):
            parts = body.split("---", 2)
            if len(parts) == 3:
                body = parts[2]
        names = {normalize(link) for link in LINK_RE.findall(body)}
        for name in names:
            for target in name_map.get(name, ()):
                if target != rel:
                    incoming[target] += 1

    orphans = sum(1 for rel in pages if incoming.get(rel, 0) == 0)
    avg_inbound = round(sum(incoming.values()) / len(pages), 2) if pages else 0
    return len(pages), orphans, avg_inbound


def verified_upon_stats(root: Path):
    checked = missing = 0
    for domain in VERIFIED_DOMAINS:
        for path in (root / domain).rglob("*.md"):
            fm = frontmatter(path.read_text(encoding="utf-8"))
            if fm is None or not fm.get("title") or fm.get("category") == "index":
                continue
            checked += 1
            if not fm.get(VERIFIED_FIELD):
                missing += 1
    return checked, missing


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--vault-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, help="JSON 输出路径")
    args = parser.parse_args()

    root = args.vault_root.resolve()
    total, orphans, avg_inbound = orphan_stats(root)
    checked, missing = verified_upon_stats(root)
    metrics = {
        "date": datetime.date.today().isoformat(),
        "pages_scanned": total,
        "orphans": orphans,
        "orphan_rate": round(orphans / total, 4) if total else 0,
        "avg_inbound": avg_inbound,
        "broken_wikilinks": broken_wikilinks(root),
        "verified_upon": {
            "checked": checked,
            "missing": missing,
            "coverage": round((checked - missing) / checked, 4) if checked else 0,
        },
    }
    payload = json.dumps(metrics, ensure_ascii=False, indent=2)
    print(payload)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
