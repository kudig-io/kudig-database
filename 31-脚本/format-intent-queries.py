#!/usr/bin/env python3
"""
Format intent_queries in KUDIG documents to standard format.

Standard format:
  - English: verb phrase ("how to diagnose...", "troubleshoot...")
  - Chinese: question or statement ("etcd 写入延迟高怎么排查", "kubeadm init 失败排查步骤")

Usage:
    python3 scripts/format-intent-queries.py [--dry-run] [paths...]

Exit codes:
    0 = no changes needed
    1 = changes applied (or errors)
"""

import os
import sys
import re
import argparse
from pathlib import Path

def parse_frontmatter(content):
    """Extract front matter."""
    if not content.startswith('---'):
        return None, None
    lines = content.split('\n')
    end = None
    for i, line in enumerate(lines[1:], 1):
        if line == '---':
            end = i
            break
    if end is None:
        return None, None
    fm = ''.join(lines[:end+1])
    return fm, content[len(fm):]

def extract_intent_queries(content):
    """Extract intent_queries list from content."""
    match = re.search(r'intent_queries:\s*\n((?:\s+-.*\n)*)', content)
    if not match:
        return []
    items = re.findall(r'\s+-\s+(.*)', match.group(1))
    return items

def format_line(line):
    """Ensure consistent format for each query line."""
    line = line.strip().strip('"\'')
    if not line:
        return None
    # Remove leading/trailing quotes
    line = line.strip('"\'')
    return line

def format_intent_queries(queries):
    """Format list of queries to standard format."""
    formatted = []
    for q in queries:
        q = q.strip().strip('"\'')
        if not q:
            continue
        formatted.append(f'  - "{q}"')
    return formatted

def process_file(filepath, dry_run=False):
    """Process a single file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        return False, f"Cannot read: {e}"

    fm, rest = parse_frontmatter(content)
    if fm is None:
        return False, "No front matter found"

    if 'intent_queries:' not in content:
        return False, None  # Skip if no intent_queries

    queries = extract_intent_queries(content)
    if not queries:
        return False, "Empty intent_queries"

    formatted = format_intent_queries(queries)
    new_fm = re.sub(
        r'intent_queries:\s*\n((?:\s+-.*\n)*)',
        'intent_queries:\n' + '\n'.join(formatted) + '\n',
        fm
    )

    if new_fm == fm:
        return False, None  # No changes

    if not dry_run:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_fm + rest)
    return True, None

def main():
    parser = argparse.ArgumentParser(description='Format intent_queries in KUDIG docs')
    parser.add_argument('--dry-run', action='store_true', help='Preview changes without applying')
    parser.add_argument('paths', nargs='*', default=['domain-', 'topic-'], help='Paths to process')
    args = parser.parse_args()

    changed = 0
    skipped = 0
    errors = 0

    for base in args.paths:
        for filepath in Path('.').glob(f'{base}*/**/*.md'):
            if '/.venv/' in str(filepath) or '/site/' in str(filepath):
                continue
            ok, msg = process_file(filepath, args.dry_run)
            if ok:
                print(f"{'[DRY-RUN] ' if args.dry_run else ''}Fixed: {filepath}")
                changed += 1
            elif msg:
                if 'Empty' in msg or 'No front' in msg:
                    skipped += 1
                else:
                    print(f"[ERROR] {filepath}: {msg}", file=sys.stderr)
                    errors += 1

    print(f"\n--- Summary ---")
    print(f"Changed: {changed}{' (dry-run)' if args.dry_run else ''}")
    print(f"Skipped: {skipped}")
    print(f"Errors: {errors}")

    return 0 if not errors else 1

if __name__ == '__main__':
    sys.exit(main() or 0)