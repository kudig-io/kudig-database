#!/usr/bin/env python3
"""Ingest CNCF landscape docs into Obsidian entity pages."""
import os, json, re, hashlib, glob
from datetime import datetime

BASE = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"
SRC = os.path.join(BASE, "docs/domain-34-cncf-landscape")
ENT = os.path.join(BASE, "entities")
CONCEPTS = os.path.join(BASE, "concepts")
REFS = os.path.join(BASE, "references")
MANIFEST = os.path.join(BASE, ".manifest.json")
TODAY = datetime.now().strftime("%Y-%m-%d")

# Load existing manifest
manifest_path = MANIFEST
if os.path.exists(manifest_path):
    with open(manifest_path) as f:
        manifest = json.load(f)
else:
    manifest = {}

def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        h.update(f.read())
    return h.hexdigest()

def extract_frontmatter(content):
    """Extract YAML frontmatter as dict."""
    if content.startswith('---'):
        end = content.find('---', 3)
        if end > 0:
            fm_text = content[3:end].strip()
            # Simple parser
            d = {}
            for line in fm_text.split('\n'):
                line = line.strip()
                if ':' in line and not line.startswith('-'):
                    k, _, v = line.partition(':')
                    k = k.strip()
                    v = v.strip().strip("'\"")
                    if v:
                        d[k] = v
            return d
    return {}

def extract_section(content, header):
    """Extract content under a ## header."""
    pattern = rf'^## {re.escape(header)}\s*$'
    match = re.search(pattern, content, re.MULTILINE)
    if not match:
        return ""
    start = match.end()
    # Find next ## header
    next_h = re.search(r'^## ', content[start:], re.MULTILINE)
    end = start + next_h.start() if next_h else len(content)
    return content[start:end].strip()

def extract_field(content, field):
    """Extract a simple field value from content."""
    pattern = rf'^\|?\s*\*?\*?{re.escape(field)}\*?\*?\s*\|?\s*:?\s*(.+?)$'
    match = re.search(pattern, content, re.MULTILINE)
    if match:
        return match.group(1).strip().strip('|').strip()
    return ""

def detect_category(content):
    """Detect CNCF category from content."""
    cats = {
        'App Definition & Development': '应用定义与开发',
        'Orchestration & Management': '编排与管理',
        'Runtime': '运行时',
        'Provisioning': '供应',
        'Observability': '可观测性',
        'Platform': '平台',
        'Security': '安全',
        'Networking': '网络',
        'Storage': '存储',
        'Continuous Integration': '持续集成',
        'Streaming & Messaging': '流与消息',
        'Database': '数据库',
        'Key Management': '密钥管理',
        'Container Registry': '容器注册表',
        'Service Mesh': '服务网格',
        'Chaos Engineering': '混沌工程',
        'Edge Computing': '边缘计算',
        'Machine Learning': '机器学习',
        'Serverless': '无服务器',
    }
    for eng, zh in cats.items():
        if eng.lower() in content.lower():
            return zh
    return "云原生"

def extract_description(content, project_name):
    """Extract project description from content."""
    # Try intro/简介 section
    for header in ['简介', '项目概述', '概述', 'Introduction']:
        sec = extract_section(content, header)
        if sec:
            # Get first paragraph
            lines = [l.strip() for l in sec.split('\n') if l.strip() and not l.startswith('#') and not l.startswith('|') and not l.startswith('-') and not l.startswith('```') and not l.startswith('>')]
            if lines:
                return lines[0][:200]
    # Try first non-header paragraph after title
    lines = content.split('\n')
    for i, line in enumerate(lines):
        if line.startswith('# ') and not line.startswith('## '):
            for j in range(i+1, min(i+20, len(lines))):
                l = lines[j].strip()
                if l and not l.startswith('#') and not l.startswith('|') and not l.startswith('>') and not l.startswith('---') and not l.startswith('```') and not l.startswith('-'):
                    return l[:200]
    return f"{project_name} 是一个 CNCF 云原生项目。"

def extract_features(content):
    """Extract key features from content."""
    features = []
    for header in ['主要特性', '核心功能', '核心能力', '主要功能', 'Key Features', 'Features']:
        sec = extract_section(content, header)
        if sec:
            for line in sec.split('\n'):
                line = line.strip()
                if line.startswith('- **') or line.startswith('- '):
                    feat = line.lstrip('- ').strip()
                    if feat and len(feat) > 5:
                        features.append(feat[:120])
            if features:
                return features[:8]
    return []

def extract_github(content):
    """Extract GitHub URL."""
    match = re.search(r'https://github\.com/[^\s|)\"]+', content)
    return match.group(0).rstrip('|').strip() if match else ""

def extract_website(content):
    """Extract official website."""
    match = re.search(r'https?://[^\s|)\"]*\.(io|org|dev|com)[^\s|)\"]*', content)
    if match:
        url = match.group(0).rstrip('|').strip()
        if 'github' not in url:
            return url
    return ""

def make_entity_page(project_name, maturity, source_content, source_relpath):
    """Generate an entity page from source content."""
    fm = extract_frontmatter(source_content)
    title = fm.get('title', project_name.replace('-', ' ').title())
    desc = extract_description(source_content, project_name)
    cat = detect_category(source_content)
    features = extract_features(source_content)
    github = extract_github(source_content)
    website = extract_website(source_content)
    
    # Build tags
    tags = ['cncf', maturity, 'cloud-native', 'entity']
    if cat and cat != '云原生':
        tags.append(cat.replace(' ', '-').lower())
    
    # Build frontmatter
    yaml_tags = ', '.join(tags)
    sources_escaped = source_relpath.replace('"', '\\"')
    
    page = f"""---
title: "{title}"
category: entities
summary: "{desc.replace('"', "'")}"
tags: [{yaml_tags}]
sources: ["{sources_escaped}"]
created: {TODAY}
updated: {TODAY}
lifecycle: draft
lifecycle_changed: "{TODAY}"
tier: supporting
base_confidence: 0.7
cncf_maturity: {maturity}
---

# {title}

> **CNCF 成熟度**: {maturity} | **类别**: {cat}

## 概述

{desc}

## 基本信息

| 属性 | 值 |
|:---|:---|
| **CNCF 状态** | {maturity} |
| **类别** | {cat} |
"""
    if website:
        page += f"| **官方网站** | {website} |\n"
    if github:
        page += f"| **GitHub** | {github} |\n"
    
    page += "\n"
    
    if features:
        page += "## 核心功能\n\n"
        for f in features:
            page += f"- {f}\n"
        page += "\n"
    
    # Related projects
    page += """## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互。^[inferred]

## 相关链接

- [[concepts/cncf-landscape-overview|CNCF 生态总览]]
- [[references/cncf-project-matrix|CNCF 项目矩阵]]
"""
    
    # Add maturity-specific links
    if maturity == 'graduated':
        page += "- [[concepts/cloud-native-architecture|云原生架构]]\n"
    
    page += f"\n---\n*来源: {source_relpath}*\n"
    
    return page

def main():
    processed = 0
    updated = 0
    created = 0
    errors = []
    
    for maturity in ['graduated', 'incubating', 'sandbox']:
        maturity_dir = os.path.join(SRC, maturity)
        if not os.path.isdir(maturity_dir):
            continue
        
        for project_dir in sorted(os.listdir(maturity_dir)):
            project_path = os.path.join(maturity_dir, project_dir)
            if not os.path.isdir(project_path):
                continue
            
            # Find the .md file
            md_files = [f for f in os.listdir(project_path) if f.endswith('.md')]
            if not md_files:
                continue
            
            src_file = os.path.join(project_path, md_files[0])
            rel_src = os.path.relpath(src_file, BASE)
            
            try:
                with open(src_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Determine entity name
                entity_name = project_dir.lower().replace(' ', '-')
                entity_file = os.path.join(ENT, f"{entity_name}.md")
                rel_entity = os.path.relpath(entity_file, BASE)
                
                # Check if exists
                if os.path.exists(entity_file):
                    # Update: add source reference if missing
                    with open(entity_file, 'r', encoding='utf-8') as f:
                        existing = f.read()
                    
                    if rel_src not in existing:
                        # Add source to frontmatter
                        if 'sources:' in existing:
                            existing = existing.replace(
                                'sources: [',
                                f'sources: ["{rel_src}", ',
                                1
                            )
                        # Update frontmatter
                        if 'updated:' in existing:
                            existing = re.sub(
                                r'updated: \d{4}-\d{2}-\d{2}',
                                f'updated: {TODAY}',
                                existing
                            )
                        with open(entity_file, 'w', encoding='utf-8') as f:
                            f.write(existing)
                        updated += 1
                    processed += 1
                    continue
                
                # Create new entity page
                page = make_entity_page(entity_name, maturity, content, rel_src)
                os.makedirs(os.path.dirname(entity_file), exist_ok=True)
                with open(entity_file, 'w', encoding='utf-8') as f:
                    f.write(page)
                
                # Update manifest
                manifest[rel_src] = {
                    "sha256": sha256(src_file),
                    "processed": TODAY,
                    "output": rel_entity
                }
                
                created += 1
                processed += 1
                
            except Exception as e:
                errors.append(f"{rel_src}: {e}")
    
    # Save manifest
    with open(manifest_path, 'w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
    
    print(f"Processed: {processed}")
    print(f"Created: {created}")
    print(f"Updated: {updated}")
    if errors:
        print(f"Errors ({len(errors)}):")
        for e in errors:
            print(f"  {e}")

if __name__ == '__main__':
    main()
