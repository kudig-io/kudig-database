#!/usr/bin/env python3
"""build-index.py — 扫描 incidents/ 全部案例，校验元数据并重建 INDEX.md。

仅依赖 Python 3 标准库。用法:

    python3 tools/build-index.py           # 校验 + 重建 INDEX.md
    python3 tools/build-index.py --check   # 只校验，不写 INDEX.md（供 CI / 提交前检查）

枚举权威来源: docs/severity-and-taxonomy.md（修改枚举须两处同步）。
"""

import argparse
import re
import sys
from collections import Counter
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INCIDENTS_DIR = ROOT / "incidents"
INDEX_PATH = ROOT / "INDEX.md"

# ---- 枚举定义（与 docs/severity-and-taxonomy.md 保持一致）----
SEVERITIES = ["SEV-1", "SEV-2", "SEV-3", "SEV-4"]
DOMAINS = {
    "cloud-infrastructure": "云基础设施",
    "cdn-edge": "CDN 与边缘接入",
    "networking-dns": "网络 / DNS / BGP",
    "database-storage": "数据库与存储",
    "container-orchestration": "容器编排与服务发现",
    "ai-ml-services": "AI / ML 服务",
    "security-services": "安全产品可用性",
    "saas-platforms": "SaaS 与互联网平台",
    "messaging-streaming": "消息与流处理",
    "observability": "可观测性平台",
    "identity-access": "身份与访问控制",
    "business-operations": "商务运营",
    "sa-architecture": "SA 架构与客情",
    "public-services": "公共服务与政企",
}
ROOT_CAUSES = {
    "change-management": "变更管理",
    "config-error": "配置错误",
    "software-bug": "软件缺陷",
    "capacity-overload": "容量与过载",
    "operational-safeguard": "操作防护缺失",
    "network-routing": "网络与路由",
    "dependency-failure": "依赖故障",
    "security-attack": "安全攻击",
    "hardware-facility": "硬件与设施",
    "data-integrity": "数据完整性",
    "commercial": "商务与合同",
    "architecture-decision": "架构决策",
}
COMPANY_TYPES = ["cloud-native", "ai-native", "internet", "public-sector"]
STATUSES = ["draft", "published", "updated"]
IMPACT_SCOPES = ["global", "multi-region", "single-region", "partial"]
REQUIRED_FIELDS = [
    "id", "title", "company", "company_type", "domain", "date",
    "duration_minutes", "severity", "impact_scope", "root_cause_category",
    "root_cause_tags", "status", "last_updated", "sources",
]
FILENAME_RE = re.compile(r"^\d{4}-\d{2}-\d{2}-[a-z0-9]+(-[a-z0-9]+)*\.md$")
ID_RE = re.compile(r"^INC-\d{8}-[A-Z0-9-]+$")
DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def parse_frontmatter(path: Path):
    """解析文件头部 YAML Frontmatter（支持标量、内联列表、块列表的子集）。"""
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0].strip() != "---":
        return None, "缺少 Frontmatter 起始 '---'"
    meta, current_list_key = {}, None
    for i, line in enumerate(lines[1:], start=2):
        if line.strip() == "---":
            return meta, None
        if not line.strip():
            continue
        if line.startswith("  - ") and current_list_key:
            meta[current_list_key].append(line.strip()[2:].strip())
            continue
        m = re.match(r"^([A-Za-z_]+):\s*(.*)$", line)
        if not m:
            return None, f"L{i} 无法解析: {line!r}"
        key, value = m.group(1), m.group(2).strip()
        if value.startswith("[") and value.endswith("]"):
            meta[key] = [v.strip() for v in value[1:-1].split(",") if v.strip()]
            current_list_key = None
        elif value == "":
            meta[key] = []
            current_list_key = key
        else:
            meta[key] = value
            current_list_key = None
    return None, "Frontmatter 未以 '---' 闭合"


def validate(path: Path, meta: dict, seen_ids: dict):
    """校验单个案例的元数据，返回错误列表。"""
    errors = []
    rel = path.relative_to(ROOT)

    if not FILENAME_RE.match(path.name):
        errors.append(f"{rel}: 文件名不符合 <YYYY-MM-DD>-<公司slug>-<事件slug>.md")

    missing = [f for f in REQUIRED_FIELDS if f not in meta]
    if missing:
        errors.append(f"{rel}: 缺少必填字段 {missing}")
        return errors  # 字段不全时后续校验无意义

    if not ID_RE.match(meta["id"]):
        errors.append(f"{rel}: id 格式非法 '{meta['id']}'（应为 INC-<YYYYMMDD>-<大写slug>）")
    if meta["id"] in seen_ids:
        errors.append(f"{rel}: id '{meta['id']}' 与 {seen_ids[meta['id']]} 重复")
    else:
        seen_ids[meta["id"]] = rel

    checks = [
        ("severity", SEVERITIES), ("domain", list(DOMAINS)),
        ("root_cause_category", list(ROOT_CAUSES)),
        ("company_type", COMPANY_TYPES), ("status", STATUSES),
        ("impact_scope", IMPACT_SCOPES),
    ]
    for field, allowed in checks:
        if meta[field] not in allowed:
            errors.append(f"{rel}: {field}='{meta[field]}' 不在枚举 {allowed}")

    if meta["domain"] != path.parent.name:
        errors.append(f"{rel}: domain='{meta['domain']}' 与所在目录 '{path.parent.name}' 不一致")
    for field in ("date", "last_updated"):
        if not DATE_RE.match(str(meta[field])):
            errors.append(f"{rel}: {field}='{meta[field]}' 不是 YYYY-MM-DD")
    if not str(path.name).startswith(str(meta["date"])):
        errors.append(f"{rel}: 文件名日期与 date='{meta['date']}' 不一致")
    try:
        int(meta["duration_minutes"])
    except (TypeError, ValueError):
        errors.append(f"{rel}: duration_minutes='{meta['duration_minutes']}' 必须为整数（未披露填 -1）")
    if not isinstance(meta["sources"], list) or not meta["sources"]:
        errors.append(f"{rel}: sources 必须至少包含一条公开来源链接")
    if not isinstance(meta["root_cause_tags"], list):
        errors.append(f"{rel}: root_cause_tags 必须是列表")
    return errors


def fmt_duration(minutes: int) -> str:
    if minutes < 0:
        return "未披露"
    if minutes < 60:
        return f"{minutes}m"
    h, m = divmod(minutes, 60)
    return f"{h}h{m:02d}m" if m else f"{h}h"


def incident_row(meta: dict, path: Path) -> str:
    rel = path.relative_to(ROOT).as_posix()
    return (
        f"| {meta['date']} | [{meta['company']}]({rel}) | {meta['severity']} "
        f"| {fmt_duration(int(meta['duration_minutes']))} | `{meta['root_cause_category']}` "
        f"| {meta['title']} |"
    )

TABLE_HEADER = "| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |\n|---|---|---|---|---|---|"


def build_index(incidents: list) -> str:
    """incidents: [(meta, path)]，按日期倒序生成 INDEX.md 内容。"""
    incidents = sorted(incidents, key=lambda x: x[0]["date"], reverse=True)
    total = len(incidents)
    by_sev = Counter(m["severity"] for m, _ in incidents)
    by_domain = Counter(m["domain"] for m, _ in incidents)
    by_cause = Counter(m["root_cause_category"] for m, _ in incidents)
    by_type = Counter(m["company_type"] for m, _ in incidents)
    tag_counter = Counter(t for m, _ in incidents for t in m["root_cause_tags"])
    drafts = sum(1 for m, _ in incidents if m["status"] == "draft")

    out = [
        "# INDEX · 故障案例总索引",
        "",
        f"> ⚠️ 本文件由 `tools/build-index.py` 自动生成，**禁止手工编辑**。",
        f"> 生成日期: {date.today().isoformat()} · 案例总数: **{total}**"
        + (f"（含 draft {drafts} 篇）" if drafts else ""),
        "",
        "## 统计概览",
        "",
        "| 维度 | 分布 |",
        "|---|---|",
        "| 严重等级 | " + " · ".join(f"{s} × {by_sev[s]}" for s in SEVERITIES if by_sev[s]) + " |",
        "| 公司类型 | " + " · ".join(f"{t} × {by_type[t]}" for t in COMPANY_TYPES if by_type[t]) + " |",
        "| 高频标签 | " + " · ".join(f"`{t}` × {c}" for t, c in tag_counter.most_common(8)) + " |",
        "",
        "## 全部案例（按日期倒序）",
        "",
        TABLE_HEADER,
    ]
    out += [incident_row(m, p) for m, p in incidents]

    out += ["", "## 按技术领域", ""]
    for domain, zh in DOMAINS.items():
        rows = [(m, p) for m, p in incidents if m["domain"] == domain]
        if not rows:
            continue
        out += [f"### {zh} `{domain}`（{len(rows)}）", "", TABLE_HEADER]
        out += [incident_row(m, p) for m, p in rows]
        out.append("")

    out += ["## 按根因分类", ""]
    for cause, zh in ROOT_CAUSES.items():
        rows = [(m, p) for m, p in incidents if m["root_cause_category"] == cause]
        if not rows:
            continue
        out += [f"### {zh} `{cause}`（{len(rows)}）", "", TABLE_HEADER]
        out += [incident_row(m, p) for m, p in rows]
        out.append("")

    out += ["## 按年份", ""]
    years = sorted({m["date"][:4] for m, _ in incidents}, reverse=True)
    for year in years:
        rows = [(m, p) for m, p in incidents if m["date"].startswith(year)]
        out += [f"### {year}（{len(rows)}）", "", TABLE_HEADER]
        out += [incident_row(m, p) for m, p in rows]
        out.append("")

    return "\n".join(out).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="校验案例元数据并重建 INDEX.md")
    parser.add_argument("--check", action="store_true", help="只校验，不写 INDEX.md")
    args = parser.parse_args()

    files = sorted(
        p for p in INCIDENTS_DIR.rglob("*.md")
        if p.name != "README.md" and p.parent != INCIDENTS_DIR
    )
    if not files:
        print("未找到任何案例文件", file=sys.stderr)
        return 1

    incidents, all_errors, seen_ids = [], [], {}
    for path in files:
        meta, err = parse_frontmatter(path)
        if err:
            all_errors.append(f"{path.relative_to(ROOT)}: {err}")
            continue
        errors = validate(path, meta, seen_ids)
        if errors:
            all_errors.extend(errors)
        else:
            incidents.append((meta, path))

    if all_errors:
        print(f"校验失败，共 {len(all_errors)} 个问题:", file=sys.stderr)
        for e in all_errors:
            print(f"  ✗ {e}", file=sys.stderr)
        return 1

    print(f"✓ 校验通过: {len(incidents)} 个案例，元数据全部合法")
    if args.check:
        return 0

    INDEX_PATH.write_text(build_index(incidents), encoding="utf-8")
    print(f"✓ 已重建 {INDEX_PATH.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
