#!/usr/bin/env python3
"""
Cross-reference enhancer for KUDIG knowledge base.
Adds related_docs to markdown frontmatter based on content analysis.
"""

import os
import re

def get_domain_name(domain_path):
    """Extract domain name from path."""
    folder = os.path.basename(domain_path)
    # Remove number prefix
    match = re.match(r'(\d+)-(.+)', folder)
    if match:
        num, name = match.groups()
        return f"Domain-{num}: {name.replace('-', ' ').title()}"
    return folder

def find_related_content(domain_path, doc_title):
    """Find potentially related content based on keywords."""
    related = []
    parent = os.path.dirname(domain_path)

    keywords = extract_keywords(doc_title)

    for item in os.listdir(parent):
        item_path = os.path.join(parent, item)
        if not os.path.isdir(item_path):
            continue
        if item == os.path.basename(domain_path):
            continue

        # Check for keyword matches in folder name
        item_keywords = item.lower().replace('-', ' ').replace('_', ' ')
        for kw in keywords:
            if kw.lower() in item_keywords:
                first_md = find_first_md(item_path)
                if first_md:
                    related.append({
                        'path': f"../{item}/{first_md}",
                        'type': 'related',
                        'desc': get_domain_name(item_path)
                    })

    return related[:4]  # Limit to 4 related docs

def extract_keywords(title):
    """Extract keywords from title."""
    words = re.findall(r'[A-Z][a-z]+|[a-z]+', title)
    # Filter common words
    stopwords = {'the', 'and', 'for', 'with', 'overview', 'guide', 'deep', 'dive'}
    return [w for w in words if w.lower() not in stopwords and len(w) > 3]

def find_first_md(folder):
    """Find first .md file in folder - prefer 01-*.md or README.md."""
    files = sorted(os.listdir(folder))
    # First, look for 01-*.md
    for f in files:
        if f.startswith('01-') and f.endswith('.md'):
            return f
    # Then README.md
    if 'README.md' in files:
        return 'README.md'
    # Then any non-00 file
    for f in files:
        if f.endswith('.md') and not f.startswith('00-'):
            return f
    # Fallback to any .md
    for f in files:
        if f.endswith('.md'):
            return f
    return None

def enhance_frontmatter(filepath):
    """Add cross-references to frontmatter."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # Check if already has related_docs
        if 'related_docs:' in content:
            return False

        # Get title
        title_match = re.search(r'^title:\s*["\']?(.+?)["\']?\s*$', content, re.MULTILINE)
        if not title_match:
            return False

        title = title_match.group(1)
        related = find_related_content(os.path.dirname(filepath), title)

        if not related:
            return False

        # Find insertion point (after first ---)
        lines = content.split('\n')
        insert_idx = None
        for i, line in enumerate(lines):
            if line == '---' and i > 0:
                insert_idx = i + 1
                break

        if insert_idx is None:
            return False

        # Build related_docs section
        related_section = ['related_docs:']
        for r in related:
            related_section.append(f"  - path: \"{r['path']}\"")
            related_section.append(f"    type: \"{r['type']}\"")
            related_section.append(f"    desc: \"{r['desc']}\"")

        lines = lines[:insert_idx] + related_section + [''] + lines[insert_idx:]

        with open(filepath, 'w', encoding='utf-8') as f:
            f.write('\n'.join(lines))

        return True

    except Exception as e:
        return False

def main():
    """Process all overview documents."""
    base = os.path.dirname(os.path.abspath(__file__))

    domains = [d for d in os.listdir(base) if d.startswith('domain-')]
    topics = [d for d in os.listdir(base) if d.startswith('topic-')]

    enhanced = []
    for d in domains + topics:
        path = os.path.join(base, d)
        first_md = find_first_md(path)
        if first_md:
            filepath = os.path.join(path, first_md)
            if enhance_frontmatter(filepath):
                enhanced.append(filepath)

    print(f"Enhanced {len(enhanced)} files with cross-references:")
    for f in enhanced[:10]:
        print(f"  + {os.path.basename(os.path.dirname(f))}/{os.path.basename(f)}")

if __name__ == '__main__':
    main()