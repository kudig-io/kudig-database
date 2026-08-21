#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate markdown report from research JSON results."""
import json
import glob
import os
import re

BASE = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(BASE, "results")
OUTPUT = os.path.join(BASE, "report.md")

SKIP_INTERNAL = {"_source_file", "uncertain"}

CATEGORY_ORDER = [
    "基本信息", "硬件规格", "兼容性", "限制与约束",
    "配置与部署", "性能特征", "安全", "运维与生命周期", "经济性",
]


def load_fields_order(fields_path):
    """Extract category->field name order from fields.yaml (best effort)."""
    order = {}
    try:
        with open(fields_path, encoding="utf-8") as f:
            content = f.read()
        for m in re.finditer(r"^\s*-\s*name:\s*(.+?)\s*$", content, re.M):
            cat = m.group(1).strip()
            fields = []
            seg = content[m.end():]
            for fm in re.finditer(r"^\s*-\s*name:\s*(.+?)\s*$", seg, re.M):
                if fm.start() > seg.find("\n- name:", 1) and False:
                    pass
                fields.append(fm.group(1).strip())
            order[cat] = fields
    except Exception:
        pass
    return order


def flatten(d, prefix=""):
    """Yield (field_name, value) pairs from nested dicts."""
    for k, v in d.items():
        if isinstance(v, dict):
            yield from flatten(v, prefix + k + ".")
        else:
            yield prefix + k, v


def format_value(v):
    if v is None or v == "":
        return None
    if isinstance(v, str):
        s = v.strip()
        if not s or "[不确定]" in s:
            return None
        return s
    if isinstance(v, list):
        if not v:
            return None
        lines = []
        for item in v:
            if isinstance(item, dict):
                parts = [f"{kk}: {vv}" for kk, vv in item.items() if vv not in (None, "")]
                if parts:
                    lines.append(" | ".join(parts))
            elif isinstance(item, str):
                if item and "[不确定]" not in item:
                    lines.append(item)
        if not lines:
            return None
        if len(lines) == 1 and len(lines[0]) < 120:
            return lines[0]
        return "\n" + "\n".join(f"- {ln}" for ln in lines)
    if isinstance(v, dict):
        parts = []
        for kk, vv in v.items():
            fv = format_value(vv)
            if fv is not None:
                parts.append(f"{kk}: {fv}")
        if not parts:
            return None
        return "; ".join(parts)
    return str(v)


def get_uncertain_set(data):
    u = data.get("uncertain")
    if isinstance(u, list):
        return {str(x) for x in u}
    return set()


def collect_item(data, fields_order):
    """Return list of (category, [(field_name, value_str), ...]) preserving order."""
    uncertain = get_uncertain_set(data)
    sections = []
    categories = [c for c in CATEGORY_ORDER if c in data]
    for cat in categories:
        cat_data = data[cat]
        if not isinstance(cat_data, dict):
            continue
        rows = []
        seen = set()
        # preferred order from fields.yaml
        pref = fields_order.get(cat, [])
        for fname in pref:
            if fname in cat_data and fname not in seen:
                val = format_value(cat_data[fname])
                if val is not None and fname not in uncertain:
                    rows.append((fname, val))
                seen.add(fname)
        for fname, v in cat_data.items():
            if fname in seen:
                continue
            seen.add(fname)
            if fname in uncertain:
                continue
            val = format_value(v)
            if val is not None:
                rows.append((fname, val))
        sections.append((cat, rows))
    # extra fields not in known categories
    extras = []
    for k, v in data.items():
        if k in CATEGORY_ORDER or k in SKIP_INTERNAL or isinstance(v, (dict, list)):
            continue
        val = format_value(v)
        if val is not None and k not in uncertain:
            extras.append((k, val))
    if extras:
        sections.append(("其他信息", extras))
    return sections


def anchor(name):
    stem = os.path.splitext(os.path.basename(name))[0].lower()
    return re.sub(r"[^a-z0-9]+", "-", stem).strip("-")


def main():
    files = sorted(glob.glob(os.path.join(RESULTS_DIR, "*.json")))
    fields_order = load_fields_order(os.path.join(BASE, "fields.yaml"))

    items = []
    for fp in files:
        with open(fp, encoding="utf-8") as f:
            data = json.load(f)
        sections = collect_item(data, fields_order)
        items.append((os.path.basename(fp), data, sections))

    lines = []
    lines.append("# Kubernetes 硬件支持调研报告")
    lines.append("")
    lines.append("> 调研范围: 2024-2026 | 生成日期: 2026-08-09 | Items 总数: %d" % len(items))
    lines.append("")
    lines.append("## 目录")
    lines.append("")
    for i, (fname, data, sections) in enumerate(items, 1):
        cat = ""
        if "基本信息" in data and isinstance(data["基本信息"], dict):
            cat = data["基本信息"].get("类别", "")
        cat_part = f" - {cat}" if cat else ""
        lines.append(f"{i}. [{data.get('基本信息', {}).get('名称', fname)}](#{anchor(fname)}){cat_part}")
    lines.append("")

    for i, (fname, data, sections) in enumerate(items, 1):
        name = data.get("基本信息", {}).get("名称", fname)
        lines.append(f"## {i}. {name}")
        lines.append("")
        if "基本信息" in data and isinstance(data["基本信息"], dict):
            links = data["基本信息"].get("官方文档链接", "")
            if isinstance(links, str) and links.strip():
                lines.append(f"**官方文档**: {links}")
                lines.append("")
        for cat, rows in sections:
            if cat == "基本信息":
                continue
            lines.append(f"### {cat}")
            lines.append("")
            for fname, val in rows:
                lines.append(f"**{fname}**")
                lines.append("")
                if val.startswith("\n"):
                    lines.append(val.strip())
                else:
                    lines.append(val)
                lines.append("")
        lines.append("---")
        lines.append("")

    with open(OUTPUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"Report generated: {OUTPUT}")
    print(f"Items: {len(items)}")


if __name__ == "__main__":
    main()
