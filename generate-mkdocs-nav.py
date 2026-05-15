#!/usr/bin/env python3
"""
Generate mkdocs.yml nav section from domain-* and topic-* folders.
"""

import os
import re

def extract_title(filepath):
    """Extract title from first H1 or H2 heading in markdown file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line.startswith('# '):
                    return line[2:].strip()
                if line.startswith('## '):
                    return line[3:].strip()
        return os.path.basename(filepath)
    except:
        return os.path.basename(filepath)

def find_first_md(folder, patterns):
    """Find first .md file matching patterns."""
    files = sorted(os.listdir(folder))

    # Check if folder has subdirectories (like domain-17) - use README.md
    has_subdirs = any(os.path.isdir(os.path.join(folder, f)) for f in files if not f.startswith('.'))
    if has_subdirs and 'README.md' in files:
        return 'README.md'

    for pat in patterns:
        for f in files:
            if f.startswith(pat) and f.endswith('.md'):
                return f
    # fallback: any .md file starting with 01 or 00
    for f in files:
        if f.endswith('.md') and (f.startswith('01-') or f.startswith('00-')):
            return f
    # fallback: first .md file alphabetically (but not README)
    for f in files:
        if f.endswith('.md') and f.lower() != 'readme.md':
            return f
    return None

def main():
    root = os.path.dirname(os.path.abspath(__file__))

    domains_1_12 = list(range(1, 13))
    domains_13_17 = list(range(13, 18))
    domains_18_plus = list(range(18, 41))

    sections = {
        '核心知识域 (Domain 1-12)': domains_1_12,
        '底层基础知识域 (Domain 13-17)': domains_13_17,
        '企业级运维专题 (Domain 18+)': domains_18_plus,
    }

    topics_order = [
        'topic-ai-agent', 'topic-ai-coding', 'topic-application-architecture',
        'topic-cheat-sheet', 'topic-deployment', 'topic-dictionary',
        'topic-febm', 'topic-fta', 'topic-functions', 'topic-index',
        'topic-java-kubernetes', 'topic-learn', 'topic-migration',
        'topic-presentations', 'topic-publish', 'topic-release-notes',
        'topic-skills', 'topic-structural-trouble-shooting', 'topic-terway'
    ]

    print("nav:")
    print("  - Home: index.md")

    for section_name, domain_nums in sections.items():
        print(f"  - {section_name}:")
        for num in domain_nums:
            folder = f"domain-{num}-"
            # Find the actual folder by prefix matching
            match_dir = None
            for d in os.listdir(root):
                if d.startswith(f"domain-{num}-"):
                    match_dir = d
                    break
            if match_dir:
                folder = match_dir
                patterns = ['01-', '00-']
                first_md = find_first_md(os.path.join(root, folder), patterns)
                if first_md:
                    print(f"    - {folder}: {folder}/{first_md}")
                else:
                    print(f"    - {folder}: {folder}/index.md")
            else:
                print(f"    # domain-{num} not found")

    print("  - 专题资源:")
    for topic in topics_order:
        topic_path = os.path.join(root, topic)
        if os.path.exists(topic_path):
            patterns = ['01-', '00-']
            first_md = find_first_md(topic_path, patterns)
            if first_md:
                print(f"    - {topic}: {topic}/{first_md}")
            else:
                print(f"    - {topic}: {topic}/index.md")
        else:
            print(f"    # {topic} not found")

    print("  - 可视化:")
    print("    - visualizations/index.md")

if __name__ == '__main__':
    main()