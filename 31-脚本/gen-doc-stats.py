#!/usr/bin/env python3
"""
Generate document statistics report for KUDIG knowledge base.

Reports:
  - Documents per domain/topic
  - Documents per difficulty level
  - Documents missing front matter fields
  - AI-ready documents (with intent_queries)

Usage:
    python3 scripts/gen-doc-stats.py [--output table|json|markdown] [--filter domain|topic|all]

Exit codes:
    0 = success
    1 = error
"""

import os
import sys
import re
import json
import argparse
from pathlib import Path
from collections import defaultdict

def parse_frontmatter(content):
    """Extract front matter fields."""
    if not content.startswith('---'):
        return {}
    lines = content.split('\n')[1:]
    fm = {}
    for line in lines:
        if line == '---':
            break
        match = re.match(r'^(\w+):\s*(.*)$', line)
        if match:
            fm[match.group(1)] = match.group(2).strip()
    return fm

def scan_directory(base_path, doc_type=None):
    """Scan directory for documents and extract stats."""
    stats = {
        'total': 0,
        'by_folder': defaultdict(int),
        'by_difficulty': defaultdict(int),
        'by_audience': defaultdict(lambda: defaultdict(int)),
        'with_intent_queries': 0,
        'with_reading_level': 0,
        'missing_fields': defaultdict(int),
    }

    folders = ['domain-', 'topic-']
    if doc_type == 'domain':
        folders = ['domain-']
    elif doc_type == 'topic':
        folders = ['topic-']

    for folder in folders:
        for doc_dir in sorted(Path(base_path).glob(f'{folder}*')):
            if not doc_dir.is_dir():
                continue
            folder_name = doc_dir.name

            for md_file in doc_dir.rglob('*.md'):
                if '/.venv/' in str(md_file) or '/site/' in str(md_file):
                    continue
                if '/node_modules/' in str(md_file):
                    continue

                stats['total'] += 1
                stats['by_folder'][folder_name] += 1

                try:
                    with open(md_file, 'r', encoding='utf-8') as f:
                        content = f.read(4096)  # Only read front matter
                except:
                    continue

                fm = parse_frontmatter(content)

                if fm.get('difficulty'):
                    stats['by_difficulty'][fm['difficulty']] += 1
                else:
                    stats['missing_fields']['difficulty'] += 1

                if 'intent_queries' in fm:
                    stats['with_intent_queries'] += 1

                if 'reading_level' in fm:
                    stats['with_reading_level'] += 1

                # Parse audience if present
                audience_match = re.search(r'audience:\s*\[(.*?)\]', content)
                if audience_match:
                    for aud in re.findall(r'"(\w+)"', audience_match.group(1)):
                        stats['by_audience'][aud][folder_name] += 1

    return stats

def print_table(stats):
    """Print stats as ASCII table."""
    print("\n=== KUDIG Document Statistics ===\n")
    print(f"Total documents: {stats['total']}")
    print(f"AI-ready (intent_queries): {stats['with_intent_queries']}")
    print(f"With reading_level: {stats['with_reading_level']}\n")

    print("-- Documents by Folder --")
    print(f"{'Folder':<40} {'Count':>8}")
    print("-" * 50)
    for folder, count in sorted(stats['by_folder'].items()):
        print(f"{folder:<40} {count:>8}")

    print("\n-- Documents by Difficulty --")
    print(f"{'Difficulty':<20} {'Count':>8}")
    print("-" * 30)
    for diff, count in sorted(stats['by_difficulty'].items()):
        print(f"{diff:<20} {count:>8}")

    if stats['missing_fields']:
        print("\n-- Missing Front Matter Fields --")
        print(f"{'Field':<30} {'Count':>8}")
        print("-" * 40)
        for field, count in sorted(stats['missing_fields'].items(), key=lambda x: -x[1]):
            print(f"{field:<30} {count:>8}")

def print_markdown(stats):
    """Print stats as Markdown table."""
    print("\n## Document Statistics\n")

    print(f"| Metric | Value |")
    print(f"|:---|---:|")
    print(f"| Total Documents | {stats['total']} |")
    print(f"| AI-ready (intent_queries) | {stats['with_intent_queries']} |")
    print(f"| With reading_level | {stats['with_reading_level']} |")

    print("\n### Documents by Folder\n")
    print(f"| Folder | Count |")
    print(f"|:---|---:|")
    for folder, count in sorted(stats['by_folder'].items()):
        print(f"| {folder} | {count} |")

    print("\n### Documents by Difficulty\n")
    print(f"| Difficulty | Count |")
    print(f"|:---|---:|")
    for diff, count in sorted(stats['by_difficulty'].items()):
        print(f"| {diff} | {count} |")

def print_json(stats):
    """Print stats as JSON."""
    # Convert defaultdict to plain dict for JSON serialization
    clean = {
        'total': stats['total'],
        'with_intent_queries': stats['with_intent_queries'],
        'with_reading_level': stats['with_reading_level'],
        'by_folder': dict(stats['by_folder']),
        'by_difficulty': dict(stats['by_difficulty']),
        'by_audience': {k: dict(v) for k, v in stats['by_audience'].items()},
        'missing_fields': dict(stats['missing_fields']),
    }
    print(json.dumps(clean, indent=2, ensure_ascii=False))

def main():
    parser = argparse.ArgumentParser(description='Generate KUDIG document statistics')
    parser.add_argument('--output', choices=['table', 'markdown', 'json'], default='table',
                        help='Output format')
    parser.add_argument('--filter', choices=['domain', 'topic', 'all'], default='all',
                        help='Filter by document type')
    parser.add_argument('--base', default='.', help='Base directory to scan')
    args = parser.parse_args()

    stats = scan_directory(args.base, args.filter)

    if args.output == 'json':
        print_json(stats)
    elif args.output == 'markdown':
        print_markdown(stats)
    else:
        print_table(stats)

    return 0

if __name__ == '__main__':
    sys.exit(main() or 0)