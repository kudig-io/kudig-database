#!/usr/bin/env python3
"""
KUDIG-DATABASE Frontmatter 质量扫描与修复脚本

Phase 1 (Scan): 扫描所有文档，报告缺失/异常的 frontmatter 字段
Phase 2 (Fix):  自动补全缺失的基础字段 (title, description, category, tags, difficulty)

不删除任何已有内容，只添加缺失字段。
"""

import re
import sys
from collections import defaultdict
from datetime import date
from pathlib import Path

import yaml

BASE_DIR = Path(__file__).parent.parent
TODAY = date.today().isoformat()[:7]  # YYYY-MM

EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules',
                '.obsidian', '.zread', '.claude', '.codebuddy', '.comate',
                '.github', '.understand-anything', '.qoder', '.mimocode', '.zcode',
                '28-资产', '30-站点', '31-脚本', '32-发布', '33-源码',
                '34-源码分析', '35-元数据', '36-报告', '37-归档'}

# 顶层目录 → 默认标签映射（2026-07 目录结构：NN-中文名）
DIR_TAGS = {
    "01-集群基础": ["k8s", "architecture", "control-plane"],
    "02-工作负载": ["k8s", "workload", "deployment"],
    "03-清单模式": ["yaml", "reference", "manifests"],
    "04-应用模式": ["architecture", "application-patterns"],
    "05-网络": ["k8s", "networking", "service"],
    "06-存储": ["k8s", "storage", "pv"],
    "07-数据库中间件": ["database", "middleware"],
    "08-安全": ["k8s", "security", "rbac"],
    "09-可观测性": ["observability", "monitoring", "prometheus"],
    "10-平台工程": ["platform", "idp"],
    "11-发布变更": ["gitops", "cicd", "release"],
    "12-可靠性": ["reliability", "sre", "disaster-recovery"],
    "13-生产运维": ["k8s", "production", "daily-ops"],
    "14-容器运行时": ["docker", "container", "containerd"],
    "15-AI基础设施": ["ai", "gpu", "k8s"],
    "16-专项技术": ["ebpf", "edge", "wasm"],
    "17-系统基础": ["linux", "fundamentals"],
    "18-云厂商": ["cloud", "multi-cloud"],
    "19-故障诊断": ["k8s", "troubleshooting", "guide"],
    "20-最佳实践": ["best-practice", "k8s"],
    "21-生态参考": ["cncf", "ecosystem", "reference"],
}

# 提炼层目录 → 默认标签映射
TOPIC_TAGS = {
    "22-概念": ["concept", "reference"],
    "23-实体": ["entity", "reference"],
    "24-综合": ["synthesis", "cross-domain"],
    "25-研究": ["research", "paper"],
    "26-技能": ["skill", "daily-ops"],
    "27-标签": ["tag", "index"],
    "29-文档": ["docs", "reference"],
}


def parse_frontmatter(content: str) -> tuple:
    """Parse frontmatter, return (fm_dict, full_fm_text, start, end)."""
    content_stripped = content.lstrip()
    if not content_stripped.startswith("---"):
        return None, "", 0, 0
    end = content_stripped.find("---", 3)
    if end == -1:
        return None, "", 0, 0
    fm_text = content_stripped[3:end].strip()
    try:
        fm = yaml.safe_load(fm_text)
        if not fm:
            fm = {}
        return fm, fm_text, 0, end + 3
    except Exception:
        return None, "", 0, 0


def get_title_from_body(content: str) -> str:
    """Extract title from first heading in body."""
    match = re.search(r'^#{1,2}\s+(.+?)(?:\s*\{.*\})?$', content, re.MULTILINE)
    if match:
        return re.sub(r'[\U0001f300-\U0001f9ff]', '', match.group(1)).strip()
    return ""


def scan_file(filepath: Path) -> dict:
    """Scan a file for frontmatter issues."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return {"file": str(filepath), "status": "read_error"}

    fm, fm_text, start, end = parse_frontmatter(content)

    if fm is None:
        return {
            "file": str(filepath),
            "status": "no_frontmatter",
            "missing": ["title", "description", "category", "tags", "difficulty",
                        "reading_level", "audience", "estimated_read_time",
                        "last_updated", "authors", "k8s_versions"],
        }

    missing = []
    required_fields = {
        "title": "文档标题",
        "description": "一句话摘要",
        "category": "所属分类",
        "tags": "标签",
        "difficulty": "难度等级",
        "reading_level": "阅读等级",
        "audience": "目标读者",
        "estimated_read_time": "阅读时间",
        "last_updated": "更新日期",
        "authors": "作者",
        "k8s_versions": "K8s版本",
    }

    for field, desc in required_fields.items():
        if field not in fm or (isinstance(fm[field], (list, str)) and not fm[field]):
            missing.append(field)

    return {
        "file": str(filepath),
        "status": "has_frontmatter",
        "missing": missing,
        "existing_fields": list(fm.keys()),
    }


def fix_file(filepath: Path, result: dict, base_dir: Path) -> bool:
    """Fix missing frontmatter fields in a file."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return False

    fm, fm_text, start_offset, end_offset = parse_frontmatter(content)

    # Determine directory for default tags
    rel = filepath.relative_to(base_dir)
    parts = rel.parts
    dir_name = parts[0] if parts else ""

    # Build fixes
    fixes = {}
    if fm is None:
        # No frontmatter at all — create one
        title = get_title_from_body(content)
        if not title:
            title = filepath.stem.replace("-", " ").replace("_", " ").title()

        tags = DIR_TAGS.get(dir_name, TOPIC_TAGS.get(dir_name, ["k8s"]))
        fixes["title"] = title
        fixes["description"] = title
        fixes["category"] = dir_name
        fixes["tags"] = tags
        fixes["difficulty"] = "intermediate"
        fixes["reading_level"] = "intermediate"
        fixes["audience"] = ["SRE", "DevOps"]
        fixes["estimated_read_time"] = "10min"
        fixes["last_updated"] = TODAY
        fixes["authors"] = [{"name": "KUDIG Team", "role": "contributor"}]
        fixes["k8s_versions"] = ["1.28", "1.29", "1.30", "1.31", "1.32"]

        # Prepend frontmatter
        fm_yaml = yaml.dump(fixes, default_flow_style=False, allow_unicode=True, sort_keys=False)
        new_content = "---\n" + fm_yaml + "---\n\n" + content.lstrip()
        filepath.write_text(new_content, encoding="utf-8")
        return True

    # Existing frontmatter — fill missing fields
    for field in result.get("missing", []):
        if field == "title":
            title = get_title_from_body(content)
            fixes[field] = title if title else filepath.stem.replace("-", " ").title()
        elif field == "description":
            title = fm.get("title", filepath.stem)
            fixes[field] = str(title)[:100]
        elif field == "category":
            fixes[field] = dir_name
        elif field == "tags":
            fixes[field] = DIR_TAGS.get(dir_name, TOPIC_TAGS.get(dir_name, ["k8s"]))
        elif field == "difficulty":
            fixes[field] = "intermediate"
        elif field == "reading_level":
            fixes[field] = "intermediate"
        elif field == "audience":
            fixes[field] = ["SRE", "DevOps"]
        elif field == "estimated_read_time":
            fixes[field] = "10min"
        elif field == "last_updated":
            fixes[field] = TODAY
        elif field == "authors":
            fixes[field] = [{"name": "KUDIG Team", "role": "contributor"}]
        elif field == "k8s_versions":
            fixes[field] = ["1.28", "1.29", "1.30", "1.31", "1.32"]

    if not fixes:
        return False

    # Merge fixes into existing fm
    for k, v in fixes.items():
        fm[k] = v

    # Rebuild the frontmatter block
    new_fm_yaml = yaml.dump(fm, default_flow_style=False, allow_unicode=True, sort_keys=False)
    new_fm_block = "---\n" + new_fm_yaml + "---"

    # Find the original fm block in content
    stripped = content.lstrip()
    if stripped.startswith("---"):
        end_idx = content.find("---", len(content) - len(stripped) + 3)
        if end_idx != -1:
            # Find the second ---
            search_from = len(content) - len(stripped) + 3
            second_dash = content.find("---", search_from)
            if second_dash != -1:
                # Find end of that line
                eol = content.find("\n", second_dash)
                if eol != -1:
                    new_content = content[:search_from] + new_fm_block + content[eol + 1:]
                    filepath.write_text(new_content, encoding="utf-8")
                    return True

    return False


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "scan"

    # Collect all md files
    md_files = []
    for d in sorted(BASE_DIR.iterdir()):
        if not d.is_dir() or d.name in EXCLUDE_DIRS:
            continue
        for f in d.rglob("*.md"):
            if f.name in ("README.md", "MOC.md"):
                continue
            md_files.append(f)

    print("=" * 70)
    print(f"Frontmatter Quality检查 (mode={mode})")
    print(f"扫描范围: {len(md_files)} 文件")
    print("=" * 70)

    results = []
    for f in md_files:
        r = scan_file(f)
        results.append(r)

    # Summary
    no_fm = [r for r in results if r["status"] == "no_frontmatter"]
    has_fm = [r for r in results if r["status"] == "has_frontmatter"]
    complete = [r for r in has_fm if not r["missing"]]
    incomplete = [r for r in has_fm if r["missing"]]

    print(f"\n总文件数:   {len(results)}")
    print(f"无 frontmatter: {len(no_fm)}")
    print(f"有 frontmatter: {len(has_fm)}")
    print(f"  完整:      {len(complete)}")
    print(f"  缺失字段:  {len(incomplete)}")

    # Field-level stats
    field_missing_count = defaultdict(int)
    for r in results:
        for f in r.get("missing", []):
            field_missing_count[f] += 1

    if field_missing_count:
        print("\n缺失字段统计:")
        for field, count in sorted(field_missing_count.items(), key=lambda x: -x[1]):
            print(f"  {field:25s} {count:5d} 文件缺失")

    # Fix mode
    if mode == "fix":
        print(f"\n{'='*70}")
        print("开始修复...")
        print("=" * 70)
        fixed = 0
        for r in results:
            if r.get("missing") or r["status"] == "no_frontmatter":
                fpath = Path(r["file"])
                if fix_file(fpath, r, BASE_DIR):
                    fixed += 1
        print(f"\n修复完成: {fixed} 文件")

    # Print files with most missing fields (top 20)
    if mode == "scan":
        print("\n缺失字段最多的文件 (Top 20):")
        sorted_results = sorted(results, key=lambda x: -len(x.get("missing", [])))
        for r in sorted_results[:20]:
            if r.get("missing"):
                print(f"  {r['file']}: {len(r['missing'])} 缺失 - {r['missing'][:5]}")


if __name__ == "__main__":
    main()
