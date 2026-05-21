#!/usr/bin/env python3
"""Fix wiki pages with bad summaries - re-extract from source content"""
import json, os, re, glob

VAULT = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"
TODAY = "2026-05-21"

def read_file(path):
    with open(os.path.join(VAULT, path), 'r', encoding='utf-8') as f:
        return f.read()

def write_file(path, content):
    with open(os.path.join(VAULT, path), 'w', encoding='utf-8') as f:
        f.write(content)

def extract_sections(body):
    sections = {}
    current_key = None
    current_lines = []
    for line in body.split('\n'):
        if line.startswith('## '):
            if current_key:
                sections[current_key] = '\n'.join(current_lines).strip()
            current_key = line[3:].strip()
            current_lines = []
        else:
            current_lines.append(line)
    if current_key:
        sections[current_key] = '\n'.join(current_lines).strip()
    return sections

def get_real_summary(body, sections):
    """Extract a meaningful summary from the body content"""
    for key in ['简介', '项目概述', '产品简介', '概述', '产品定位', '核心定位']:
        if key in sections:
            text = sections[key].strip()
            for para in text.split('\n\n'):
                para = para.strip()
                if (len(para) > 20 and not para.startswith('#') and
                    not para.startswith('|') and not para.startswith('description')):
                    if len(para) > 200:
                        para = para[:197] + '...'
                    return para
    # Fallback: find first meaningful paragraph in body
    for line in body.split('\n'):
        line = line.strip()
        if (line and not line.startswith('#') and not line.startswith('|') and
            not line.startswith('>') and not line.startswith('---') and
            not line.startswith('-') and not line.startswith('title:') and
            not line.startswith('description:') and len(line) > 30):
            if len(line) > 200:
                line = line[:197] + '...'
            return line
    return None

# Scan all wiki files for bad summaries
bad_summaries = [
    "description: '## 项目概述'",
    "description: '# ",
    "title: ",
]

fixed = 0
for subdir in ['entities', 'references', 'concepts']:
    for fpath in glob.glob(os.path.join(VAULT, subdir, '*.md')):
        content = read_file(os.path.relpath(fpath, VAULT))
        
        # Check if summary is bad
        match = re.search(r'summary: "([^"]*)"', content)
        if not match:
            continue
        summary = match.group(1)
        
        is_bad = any(summary.startswith(bad) for bad in bad_summaries) or len(summary) < 15
        if not is_bad:
            continue
        
        # Find the source file from frontmatter
        src_match = re.search(r'sources: \["([^"]*)"', content)
        if not src_match:
            continue
        src_path = src_match.group(1)
        
        # Read source and extract real summary
        try:
            src_content = read_file(src_path)
        except:
            continue
        
        # Remove frontmatter from source
        if src_content.startswith('---'):
            end = src_content.find('---', 3)
            if end != -1:
                body = src_content[end+3:].strip()
            else:
                body = src_content
        else:
            body = src_content
        
        sections = extract_sections(body)
        real_summary = get_real_summary(body, sections)
        
        if real_summary and real_summary != summary:
            # Fix the summary in the wiki file
            new_content = content.replace(
                f'summary: "{summary}"',
                f'summary: "{real_summary[:200]}"'
            )
            write_file(os.path.relpath(fpath, VAULT), new_content)
            fixed += 1
            fname = os.path.basename(fpath)
            print(f"  FIXED {fname}: {real_summary[:80]}...")

print(f"\nTotal fixed: {fixed}")
