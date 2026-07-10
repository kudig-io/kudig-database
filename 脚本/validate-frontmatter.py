#!/usr/bin/env python3
"""
Validate frontmatter fields in KUDIG knowledge base documents.

Checks for required fields:
  - intent_queries: AI grounding queries
  - trigger_keywords: NLP trigger keywords
  - reading_level: beginner/intermediate/advanced/expert
  - audience: target readers
  - estimated_read_time: parsing-friendly time string

Usage:
    python3 scripts/validate-frontmatter.py [--fix] [--verbose] [paths...]
"""

import os
import sys
import re
import argparse
import yaml
from pathlib import Path

# Required front matter fields per document type
DOMAIN_FIELDS = ['title', 'category', 'last_updated', 'difficulty']
TOPIC_FIELDS = ['title', 'last_updated', 'difficulty']
SKILL_FIELDS = ['skill_id', 'skill_name', 'version', 'category']
FTA_FIELDS = ['fta_id', 'title', 'component', 'severity']

# AI-specific fields (for RAG quality)
AI_FIELDS = ['intent_queries', 'trigger_keywords']

# Reading experience fields
READING_FIELDS = ['reading_level', 'audience', 'estimated_read_time', 'prerequisites']

ALL_REQUIRED = DOMAIN_FIELDS + AI_FIELDS + READING_FIELDS

def parse_frontmatter(content):
    """Extract front matter key-value pairs from markdown using PyYAML."""
    fm = {}
    if not content.startswith('---'):
        return fm

    lines = content.split('\n')[1:]
    yaml_lines = []
    for line in lines:
        if line == '---':
            break
        yaml_lines.append(line)

    if yaml_lines:
        try:
            parsed = yaml.safe_load('\n'.join(yaml_lines))
            if isinstance(parsed, dict):
                fm = parsed
        except yaml.YAMLError:
            pass
    return fm

def check_document(filepath):
    """Validate a single document's front matter."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        return [], [f"Cannot read {filepath}: {e}"]

    fm = parse_frontmatter(content)
    missing = []
    warnings = []

    # Determine document type
    relpath = str(filepath)
    if 'topic-fta/' in relpath or relpath.endswith('-fta.md'):
        required = set(FTA_FIELDS + AI_FIELDS + READING_FIELDS)
    elif 'topic-skills/' in relpath or relpath.endswith('-skill.md'):
        required = set(SKILL_FIELDS + AI_FIELDS + READING_FIELDS)
    elif 'domain-' in relpath:
        required = set(DOMAIN_FIELDS + AI_FIELDS + READING_FIELDS)
    else:
        required = set(ALL_REQUIRED[:6])  # Basic fields

    for field in required:
        if field not in fm:
            missing.append(field)

    # Check intent_queries format
    if 'intent_queries' in fm:
        iq = fm['intent_queries']
        if not isinstance(iq, list):
            warnings.append('intent_queries must be a list')

    return missing, warnings

def main():
    parser = argparse.ArgumentParser(description='Validate frontmatter in KUDIG docs')
    parser.add_argument('--fix', action='store_true', help='Attempt to fix missing fields')
    parser.add_argument('--verbose', '-v', action='store_true', help='Show all files checked')
    parser.add_argument('paths', nargs='*', default=['.'], help='Paths to check')
    args = parser.parse_args()

    total = 0
    issues = []

    for base in args.paths:
        if os.path.isfile(base):
            files = [base]
        else:
            files = Path(base).rglob('*.md')

        for filepath in files:
            if '/.venv/' in str(filepath) or '/site/' in str(filepath):
                continue
            total += 1
            missing, warnings = check_document(filepath)

            if missing or warnings:
                rel = os.path.relpath(filepath, os.getcwd())
                if missing:
                    issues.append((rel, 'missing', missing))
                if warnings:
                    issues.append((rel, 'warning', warnings))

                print(f"[{'MISSING' if missing else 'WARN' }] {rel}")
                if missing:
                    print(f"         Missing: {', '.join(missing)}")
                if warnings and args.verbose:
                    print(f"         Warning: {', '.join(warnings)}")

    print(f"\n--- Summary ---")
    print(f"Checked: {total} files")
    print(f"With issues: {len(issues)}")

    if issues:
        print(f"\nFiles needing attention:")
        for rel, itype, fields in issues[:20]:
            print(f"  {rel}: {fields}")
        if len(issues) > 20:
            print(f"  ... and {len(issues) - 20} more")

    return 0 if not issues else 1

if __name__ == '__main__':
    sys.exit(main() or 0)