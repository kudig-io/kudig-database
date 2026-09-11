#!/usr/bin/env python3
"""Sync GTM landing page stats with actual repo content counts.

Recounts the 21 source domains (01-21) and the distilled layers
(22-概念 / 23-实体 / 24-综合 / 26-技能), then rewrites the LINES array,
the headline totals, the 故障诊断 count and the ticker UPDATED date in
GTM/index.html so the page never drifts from the repository.

Usage:
  python3 gtm-sync-stats.py           # rewrite GTM/index.html in place
  python3 gtm-sync-stats.py --check   # exit 1 if the page is stale (for CI)
"""

import argparse
import datetime
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
HTML_PATH = ROOT / "GTM" / "index.html"

DISTILLED_DIRS = ["22-概念", "23-实体", "24-综合", "26-技能"]
DIAG_DIR = "19-故障诊断"


def count_md(directory: Path) -> int:
    return len(list(directory.rglob("*.md")))


def domain_dirs() -> list[Path]:
    dirs = [
        p for p in ROOT.iterdir()
        if p.is_dir() and re.match(r"^(0[1-9]|1[0-9]|20|21)-", p.name)
    ]
    return sorted(dirs)


def lines_block(entries: list[tuple[str, str, int]]) -> str:
    rows = [f'["{num}","{name}",{count}]' for num, name, count in entries]
    lines = []
    for i in range(0, len(rows), 4):
        chunk = ",".join(rows[i:i + 4])
        if i + 4 < len(rows):
            chunk += ","
        lines.append("    " + chunk)
    block = "var LINES = [\n" + "\n".join(lines) + "\n  ];"
    if len(re.findall(r'\["\d{2}"', block)) != len(entries):
        raise ValueError("LINES block generation lost entries")
    return block


def last_content_date() -> str:
    targets = [p.name for p in domain_dirs()] + DISTILLED_DIRS
    try:
        out = subprocess.run(
            ["git", "log", "-1", "--format=%cs", "--", *targets],
            cwd=ROOT, capture_output=True, text=True, check=True,
        ).stdout.strip()
        return out or datetime.date.today().isoformat()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return datetime.date.today().isoformat()


def sync(check_only: bool) -> int:
    html = HTML_PATH.read_text(encoding="utf-8")
    original = html

    domains = [(p.name[:2], p.name[3:], count_md(p)) for p in domain_dirs()]
    if len(domains) != 21:
        print(f"ERROR: expected 21 source domains, found {len(domains)}", file=sys.stderr)
        return 2
    source_total = sum(c for _, _, c in domains)
    distilled_total = sum(count_md(ROOT / d) for d in DISTILLED_DIRS)
    total = source_total + distilled_total
    diag_count = count_md(ROOT / DIAG_DIR)
    updated = last_content_date()

    html, n_lines = re.subn(
        r"var LINES = \[.*?\];", lines_block(domains), html, flags=re.DOTALL)

    # Headline totals ("4,750 篇" style, always comma-formatted >= 1,000).
    html, n_totals = re.subn(r"\b\d{1,2},\d{3} 篇", f"{total:,} 篇", html)
    html, n_docs = re.subn(r"[\d,]+ DOCS", f"{total:,} DOCS", html)

    # 故障诊断 count in the dual-read section.
    html, n_diag = re.subn(
        r"故障诊断 [\d,]+ 篇", f"故障诊断 {diag_count:,} 篇", html)

    # Ticker freshness date.
    html, n_date = re.subn(
        r"UPDATED: \d{4}-\d{2}-\d{2}", f"UPDATED: {updated}", html)

    changed = [f"LINES array (x{n_lines})", f"totals (篇 x{n_totals}, DOCS x{n_docs})",
               f"诊断域 {diag_count} (x{n_diag})", f"UPDATED {updated} (x{n_date})"]
    summary = (f"source(01-21)={source_total:,} distilled={distilled_total:,} "
               f"total={total:,} {DIAG_DIR}={diag_count}")

    if check_only:
        if html != original:
            print(f"STALE: {HTML_PATH} does not match repo counts. Run gtm-sync. ({summary})")
            return 1
        print(f"OK: GTM page stats up to date. {summary}")
        return 0

    if html != original:
        HTML_PATH.write_text(html, encoding="utf-8")
        print(f"updated: {', '.join(changed)}")
    else:
        print("no changes needed")
    print(summary)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="verify the page is current without writing; exit 1 if stale")
    args = parser.parse_args()
    return sync(check_only=args.check)


if __name__ == "__main__":
    sys.exit(main())
