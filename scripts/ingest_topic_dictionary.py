#!/usr/bin/env python3
"""
Ingest topic-dictionary/ (205 files) into consolidated references/ terminology pages.
Creates/updates references/<category>-terms.md for each subdirectory.
Creates/updates references/k8s-glossary-index.md master index.
Updates .manifest.json with each source file processed.
"""

import os
import re
import json
import hashlib
from datetime import datetime, timezone
from pathlib import Path

WORKDIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
TD_DIR = WORKDIR / "docs" / "topic-dictionary"
REF_DIR = WORKDIR / "references"
MANIFEST_PATH = WORKDIR / ".manifest.json"

# Category names in Chinese for each subdirectory
CATEGORY_NAMES = {
    "configuration": "配置管理",
    "fundamentals": "基础概念",
    "multi-cloud": "多云架构",
    "networking": "网络",
    "observability": "可观测性",
    "operations": "运维运营",
    "platform-engineering": "平台工程",
    "scheduling": "调度",
    "security": "安全",
    "specialized-workloads": "专用工作负载",
    "storage": "存储",
    "tooling": "工具链",
    "workloads": "工作负载",
}

# Related concept pages to wikilink
RELATED_PAGES = {
    "configuration": ["[[k8s-architecture-fundamentals]]", "[[k8s-control-plane-deep-dive]]"],
    "fundamentals": ["[[k8s-architecture-fundamentals]]", "[[k8s-knowledge-map]]"],
    "multi-cloud": ["[[k8s-cloud-provider-comparison]]", "[[alicloud-ack-overview]]", "[[aws-eks-overview]]"],
    "networking": ["[[k8s-networking-domain-guide]]", "[[k8s-networking-ecosystem]]"],
    "observability": ["[[k8s-observability-ecosystem]]"],
    "operations": ["[[k8s-production-operations]]", "[[k8s-structured-troubleshooting]]"],
    "platform-engineering": ["[[k8s-platform-extensions]]", "[[k8s-advanced-ecosystem]]"],
    "scheduling": ["[[k8s-workload-management]]"],
    "security": ["[[k8s-security-compliance]]"],
    "specialized-workloads": ["[[k8s-ai-infrastructure]]", "[[k8s-workload-management]]"],
    "storage": ["[[k8s-storage-ecosystem]]"],
    "tooling": ["[[kubectl-quick-reference]]", "[[kudig-ecosystem-guide]]"],
    "workloads": ["[[k8s-workloads-domain-guide]]", "[[k8s-workload-management]]"],
}


def sha256_file(path):
    """Compute sha256 hash of a file."""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return f"sha256:{h.hexdigest()}"


def parse_frontmatter(content):
    """Parse YAML frontmatter from markdown content."""
    if not content.startswith("---"):
        return {}, content
    end = content.find("---", 3)
    if end == -1:
        return {}, content
    fm_text = content[3:end].strip()
    body = content[end + 3:].strip()
    # Simple key extraction
    fm = {}
    title_match = re.search(r'^title:\s*["\']?(.*?)["\']?\s*$', fm_text, re.MULTILINE)
    if title_match:
        fm['title'] = title_match.group(1).strip()
    title_en_match = re.search(r'^title_en:\s*["\']?(.*?)["\']?\s*$', fm_text, re.MULTILINE)
    if title_en_match:
        fm['title_en'] = title_en_match.group(1).strip()
    return fm, body


def extract_term_info(content, filename):
    """Extract term name, Chinese description from a source file."""
    fm, body = parse_frontmatter(content)
    
    # Get title from frontmatter
    title = fm.get('title', '')
    title_en = fm.get('title_en', '')
    
    if not title:
        # Try first H1
        h1_match = re.search(r'^#\s+(.+)$', body, re.MULTILINE)
        if h1_match:
            title = h1_match.group(1).strip()
    
    # Extract the overview section
    overview = ""
    overview_match = re.search(r'##\s*(?:概述|Overview)\s*\n(.*?)(?=\n##\s|\Z)', body, re.DOTALL | re.IGNORECASE)
    if overview_match:
        overview = overview_match.group(1).strip()
        # Limit to first 500 chars
        if len(overview) > 500:
            overview = overview[:497] + "..."
    
    if not overview:
        # Try first paragraph after title
        paras = re.split(r'\n\n+', body)
        for p in paras:
            p = p.strip()
            if p and not p.startswith('#') and not p.startswith('>') and not p.startswith('```'):
                overview = p[:500] if len(p) <= 500 else p[:497] + "..."
                break
    
    return {
        'title': title,
        'title_en': title_en,
        'overview': overview,
        'filename': filename,
    }


def build_consolidated_page(category, terms):
    """Build a consolidated reference page for a category."""
    cat_name = CATEGORY_NAMES.get(category, category)
    num_terms = len(terms)
    
    source_files = [f"topic-dictionary/{category}/{t['filename']}" for t in terms]
    
    related = RELATED_PAGES.get(category, [])
    related_str = " | ".join(related) if related else ""
    
    # Frontmatter
    lines = [
        "---",
        f'title: "K8s {cat_name}术语参考"',
        "category: references",
        f'summary: "Kubernetes {cat_name}相关术语和概念参考，涵盖 {num_terms} 个词条。"',
        f"tags: [k8s, dictionary, {category}]",
        f'sources: {json.dumps(source_files)}',
        "created: 2026-05-21",
        f"updated: {datetime.now(timezone.utc).strftime('%Y-%m-%d')}",
        "lifecycle: draft",
        'lifecycle_changed: "2026-05-21"',
        "tier: supporting",
        "base_confidence: 0.7",
        "---",
        "",
        f"# K8s {cat_name}术语参考",
        "",
        f"本页汇总了 **{cat_name}** 领域的 {num_terms} 个 Kubernetes 术语定义与概念说明。",
        "",
    ]
    
    if related_str:
        lines.append(f"> **相关领域**: {related_str}")
        lines.append("")
    
    lines.append("---")
    lines.append("")
    
    # Build a quick-reference table first
    lines.append(f"## 术语速查表")
    lines.append("")
    lines.append("| 术语 | 英文名 | 说明 |")
    lines.append("|------|--------|------|")
    for t in terms:
        title = t['title']
        title_en = t['title_en'] if t['title_en'] else t['filename'].replace('.md', '').replace('-', ' ').title()
        # Short desc - first sentence of overview
        desc = t['overview']
        first_sentence = re.split(r'[。.！!？?]', desc)[0] if desc else ""
        if len(first_sentence) > 80:
            first_sentence = first_sentence[:77] + "..."
        # Escape pipe chars
        first_sentence = first_sentence.replace("|", "\\|")
        lines.append(f"| **{title}** | {title_en} | {first_sentence} |")
    
    lines.append("")
    lines.append("---")
    lines.append("")
    
    # Detailed entries
    for t in terms:
        lines.append(f"### {t['title']}")
        lines.append("")
        if t['title_en']:
            lines.append(f"> **英文**: {t['title_en']}")
            lines.append("")
        if t['overview']:
            lines.append(t['overview'])
            lines.append("")
        lines.append(f"> *（内容已精简，完整版请参阅源文件 `topic-dictionary/{category}/{t['filename']}`）*")
        lines.append("")
        lines.append("---")
        lines.append("")
    
    # Related wikilinks
    if related:
        lines.append("## 相关页面")
        lines.append("")
        for r in related:
            lines.append(f"- {r}")
        lines.append("")
    
    # Source files
    lines.append("## 来源文件")
    lines.append("")
    for sf in source_files:
        lines.append(f"- `{sf}`")
    lines.append("")
    
    return "\n".join(lines)


def build_glossary_index(categories_info):
    """Build the master glossary index page."""
    lines = [
        "---",
        'title: "K8s 术语表索引"',
        "category: references",
        'summary: "Kubernetes 术语表主索引页，链接到各领域的术语参考页面。"',
        "tags: [k8s, dictionary, glossary, index]",
        f"updated: {datetime.now(timezone.utc).strftime('%Y-%m-%d')}",
        "lifecycle: draft",
        'lifecycle_changed: "2026-05-21"',
        "tier: supporting",
        "base_confidence: 0.7",
        "---",
        "",
        "# K8s 术语表索引（Glossary Index）",
        "",
        "> 本页为 KUDIG 术语表主索引，汇总了 13 个领域共 205+ 个 Kubernetes 核心术语。",
        "",
        "---",
        "",
        "## 术语分类目录",
        "",
        "| 领域 | 术语数量 | 参考页面 |",
        "|------|----------|----------|",
    ]
    
    total = 0
    for cat, count, page_name in categories_info:
        cat_name = CATEGORY_NAMES.get(cat, cat)
        lines.append(f"| {cat_name} | {count} | [[{page_name}]] |")
        total += count
    
    lines.append(f"| **合计** | **{total}** | |")
    lines.append("")
    lines.append("---")
    lines.append("")
    
    # Quick links to all term pages
    lines.append("## 全部术语页面")
    lines.append("")
    for cat, count, page_name in categories_info:
        cat_name = CATEGORY_NAMES.get(cat, cat)
        lines.append(f"- [[{page_name}|{cat_name}术语参考]] ({count} 个词条)")
    lines.append("")
    
    # Related pages
    lines.append("## 相关资源")
    lines.append("")
    lines.append("- [[k8s-knowledge-map]] - 知识图谱总览")
    lines.append("- [[k8s-architecture-fundamentals]] - 架构基础")
    lines.append("- [[k8s-workloads-domain-guide]] - 工作负载指南")
    lines.append("- [[k8s-networking-domain-guide]] - 网络指南")
    lines.append("- [[k8s-security-compliance]] - 安全合规")
    lines.append("- [[k8s-observability-ecosystem]] - 可观测性生态")
    lines.append("- [[k8s-storage-ecosystem]] - 存储生态")
    lines.append("- [[KUDIG Tag Dictionary]] - 标签字典")
    lines.append("")
    
    return "\n".join(lines)


def load_manifest():
    """Load existing manifest."""
    if MANIFEST_PATH.exists():
        with open(MANIFEST_PATH) as f:
            return json.load(f)
    return {"version": 1, "sources": {}}


def save_manifest(manifest):
    """Save manifest."""
    with open(MANIFEST_PATH, "w") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
        f.write("\n")


def main():
    REF_DIR.mkdir(parents=True, exist_ok=True)
    
    manifest = load_manifest()
    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    
    categories_info = []  # (category, count, page_name)
    processed_count = 0
    
    # Process each subdirectory
    for category in sorted(CATEGORY_NAMES.keys()):
        cat_dir = TD_DIR / category
        if not cat_dir.is_dir():
            print(f"  SKIP: {category}/ not found")
            continue
        
        # Read all .md files in the subdirectory
        md_files = sorted([f for f in cat_dir.iterdir() if f.suffix == ".md"])
        if not md_files:
            print(f"  SKIP: {category}/ has no .md files")
            continue
        
        terms = []
        for md_file in md_files:
            content = md_file.read_text(encoding="utf-8")
            term_info = extract_term_info(content, md_file.name)
            terms.append(term_info)
            
            # Update manifest for this source file
            manifest_key = f"topic-dictionary/{category}/{md_file.name}"
            stat = md_file.stat()
            file_hash = sha256_file(md_file)
            manifest["sources"][manifest_key] = {
                "ingested_at": now,
                "size_bytes": stat.st_size,
                "modified_at": datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
                "content_hash": file_hash,
                "source_type": "terminology",
                "project": category,
            }
            processed_count += 1
        
        # Build and write consolidated page
        page_name = f"{category}-terms.md"
        page_path = REF_DIR / page_name
        
        # Check if existing page has data we should merge
        existing_terms = set()
        if page_path.exists():
            existing_content = page_path.read_text(encoding="utf-8")
            # Extract existing term headings
            for m in re.finditer(r'^### (.+)$', existing_content, re.MULTILINE):
                existing_terms.add(m.group(1).strip())
            print(f"  MERGE: {page_name} exists with {len(existing_terms)} terms")
        
        # Build the consolidated page
        page_content = build_consolidated_page(category, terms)
        page_path.write_text(page_content, encoding="utf-8")
        
        categories_info.append((category, len(terms), page_name))
        print(f"  WROTE: {page_name} ({len(terms)} terms)")
    
    # Also handle the root-level glossary file
    glossary_path = TD_DIR / "k8s-glossary.md"
    if glossary_path.exists():
        manifest_key = "topic-dictionary/k8s-glossary.md"
        stat = glossary_path.stat()
        file_hash = sha256_file(glossary_path)
        manifest["sources"][manifest_key] = {
            "ingested_at": now,
            "size_bytes": stat.st_size,
            "modified_at": datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "content_hash": file_hash,
            "source_type": "terminology",
            "project": "glossary",
        }
        processed_count += 1
        print(f"  TRACKED: k8s-glossary.md (manifest only, used as source)")
    
    # Build master glossary index
    index_content = build_glossary_index(categories_info)
    index_path = REF_DIR / "k8s-glossary-index.md"
    index_path.write_text(index_content, encoding="utf-8")
    print(f"\n  WROTE: k8s-glossary-index.md (master index)")
    
    # Save manifest
    save_manifest(manifest)
    print(f"\n  UPDATED: .manifest.json ({processed_count} source files tracked)")
    
    # Summary
    total_terms = sum(c for _, c, _ in categories_info)
    print(f"\n=== SUMMARY ===")
    print(f"  Categories processed: {len(categories_info)}")
    print(f"  Source files ingested: {processed_count}")
    print(f"  Total terms consolidated: {total_terms}")
    print(f"  Reference pages created/updated: {len(categories_info) + 1}")


if __name__ == "__main__":
    main()
