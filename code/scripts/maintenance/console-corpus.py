#!/usr/bin/env python3
"""Rebuild the console corpus across the WHOLE library and ship the page bodies.

`console/` renders every content page of the repository, not one domain.
This builder writes:

  console/data/index.json            every page: id, title, partition, domain,
                                     seven-layer footprint, tier/category/level,
                                     counts (diagrams/tables/lines/fences), and
                                     resolved in-library wikilink edges.
  console/data/domains.json          navigation spine: partition → domain → pages.
  console/data/graph/diagram.json    2,284 Mermaid blocks with render verdict +
                                     parse-error excerpt, per domain.
  console/data/graph/coverage.json   domain × layer matrix + per-layer gap lists
                                     (gap list capped at 200).
  console/data/graph/structure.json  partitions, tiers, categories, hubs, orphans.
  console/content/<page-id>.json     the segments of one page: verbatim body split
                                     into small files a reader fetches per page:
                                       ["t", text]           paragraph
                                       ["c", lang, text]     fenced code
                                       ["m", id, kind, v]    Mermaid block by id
                                       ["d", …]              Mermaid block that fails
                                                             to parse, kept verbatim
                                     (no monolithic console/content/index.json: it
                                     was a debug-only duplicate that nothing loads
                                     at runtime — dropped from the repo 2026-11-24).

Excluded, stated once: `code/repo/` (upstream vendor checkouts, rebuilt by
`make repo-sync`), `31-归档`, `03-发布`, `04-报告`, `27-资产`, dot-directories.

Usage:
  python3 code/scripts/maintenance/console-corpus.py            # rebuild everything
  python3 code/scripts/maintenance/console-corpus.py --check    # exit 1 if stale (CI)
  python3 code/scripts/maintenance/console-corpus.py --summary  # totals only
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DATA = ROOT / "console" / "data"
CONTENT = ROOT / "console" / "content"
VALIDATION = DATA / "mermaid-validation.json"
SCHEMA = "kudig-console/3"

PARTITIONS = [
    ("wiki", "语料正文", "30 个知识域"),
    ("skills", "Agent 技能卡", "8 个技能域"),
    ("maintaining", "维护资产", "文档 / 元数据 / 发布 / 报告"),
    ("code", "可执行资产", "脚本 / 自动化 / 调研"),
]
SKIP_DIRS = {"31-归档", "03-发布", "04-报告", "27-资产", "node_modules", "__pycache__", ".git", ".impeccable", ".venv"}

FM = re.compile(r"\A---\n(.*?)\n---\n", re.S)
FENCE = re.compile(r"^\s*```(.*)$")
MERMAID_BLOCK = re.compile(r"```mermaid[ \t]*\n(.*?)```", re.S)
H12 = re.compile(r"^(#{1,2})\s+(.+?)\s*$")
WIKILINK = re.compile(r"\[\[([^\]|#]+)(?:#[^\]|]*)?(?:\|[^\]]*)?\]\]")
SECTION = re.compile(r"^##\s+\d+[.、]")
TABLE_ROW = re.compile(r"^\|.*\|\s*$")
CODE_OPEN = re.compile(r"^\s*```(\w*)")
CHUNK_MARKER = re.compile(r"<!--[\s\S]*?-->")
HEADING = re.compile(r"^(#{2,3})\s+(.+?)\s*#*\s*$")
NUMBER_PREFIX = re.compile(r"^\d+(?:\.\d+)*[\.、]?\s*")
#: A dimension is "landed" in a page when an H2/H3 matches one of its patterns.
DIMENSIONS_FILE = ROOT / "console" / "spec" / "dimensions.json"
#: Fallback only: the workbench must still build if the vocabulary file is missing.
DIMENSIONS_FALLBACK = [
    {"id": "d-fault", "name": "故障处置", "group": "lifecycle",
     "patterns": ["故障", "排查", "排障", "troubleshoot", "remediation"], "h2": True, "h3": True},
]


def load_dimensions() -> tuple[list[dict], str]:
    """Read the controlled vocabulary (console/spec/dimensions.json); returns (dims, version)."""
    if not DIMENSIONS_FILE.exists():
        return DIMENSIONS_FALLBACK, "fallback"
    doc = json.loads(DIMENSIONS_FILE.read_text(encoding="utf-8"))
    return list(doc["dimensions"]), str(doc.get("version", ""))

LAYERS: list[dict[str, object]] = [
    {"id": "L0", "name": "总览", "criterion": "适用场景 / 核心职责", "patterns": ["适用场景", "核心职责"]},
    {"id": "L1", "name": "概念", "criterion": "核心概念 / 术语", "patterns": ["核心概念", "术语"]},
    {"id": "L2", "name": "架构", "criterion": "架构 / 拓扑 / 信息流", "patterns": ["架构", "拓扑", "信息流"]},
    {"id": "L3", "name": "操作", "criterion": "标准操作 / 部署 / 步骤",
     "patterns": ["标准操作", "操作流程", "部署", "步骤"]},
    {"id": "L4", "name": "检查", "criterion": "检查点 / 验证 / 监控",
     "patterns": ["检查点", "验证", "监控指标", "slo"]},
    {"id": "L5", "name": "故障", "criterion": "故障 / 排查 / 排障",
     "patterns": ["故障", "排查", "排障", "troubleshoot"]},
    {"id": "L6", "name": "参考", "criterion": "参考 / Runbook / 阅读",
     "patterns": ["参考", "runbook", "推荐阅读", "延伸"]},
]

DIAGRAM_KINDS: list[tuple[str, str, str]] = [
    ("graph", "graph", "流程图"), ("flowchart", "graph", "流程图"), ("sequenceDiagram", "sequence", "时序图"),
    ("stateDiagram", "state", "状态图"), ("classDiagram", "class", "类图"),
    ("mindmap", "mindmap", "思维导图"), ("timeline", "timeline", "时间线"),
    ("gantt", "gantt", "甘特图"), ("pie", "pie", "饼图"), ("quadrantChart", "quadrant", "象限图"),
    ("xychart-beta", "xychart", "图表"), ("xychart", "xychart", "图表"),
    ("venn-beta", "venn", "韦恩图"), ("venn", "venn", "韦恩图"),
    ("gitGraph", "gitGraph", "Git 图"), ("erDiagram", "er", "ER 图"),
    ("journey", "journey", "旅程图"), ("requirementDiagram", "requirement", "需求图"),
]
TIER_ORDER = {"core": 0, "supporting": 1, "reference": 2, "peripheral": 3}


# --------------------------------------------------------------------------- scan

def iter_pages() -> list[Path]:
    pages: list[Path] = []
    for root, _, _ in PARTITIONS:
        base = ROOT / root
        if not base.is_dir():
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS and not d.startswith("."))
            if Path(dirpath).relative_to(ROOT).parts[:2] == ("code", "repo"):
                dirnames[:] = []
                continue
            pages.extend(Path(dirpath) / name for name in sorted(filenames) if name.endswith(".md"))
    return sorted(pages)


def fm_value(fm: str, key: str) -> str:
    match = re.search(rf"^{key}:\s*(.+?)\s*$", fm, re.M)
    return match.group(1).strip().strip("\"'") if match else ""


def headings_of(body: str) -> list[str]:
    out: list[str] = []
    fenced = False
    for line in body.splitlines():
        if FENCE.match(line):
            fenced = not fenced
            continue
        if not fenced:
            match = H12.match(line)
            if match:
                out.append(match.group(2).strip().lower())
    return out


def layer_bits(headings: list[str]) -> str:
    return "".join(
        "1" if any(str(p) in h for p in layer["patterns"] for h in headings) else "0"  # type: ignore[union-attr]
        for layer in LAYERS
    )


def diagram_kind(code: str) -> str:
    first = next((line.strip() for line in code.strip().splitlines() if line.strip()), "")
    for prefix, kind, _ in DIAGRAM_KINDS:
        if first.startswith(prefix):
            return kind
    return "other"


def domain_label(name: str) -> str:
    match = re.match(r"^(\d{2})-(.+)$", name)
    return f"{match.group(1)} {match.group(2)}" if match else name


def classify(path: Path) -> tuple[str, str, str]:
    parts = path.relative_to(ROOT).parts
    partition = parts[0]
    if partition == "wiki" and len(parts) > 2:
        return partition, f"wiki/{parts[1]}", parts[1]
    if partition in ("skills", "code") and len(parts) > 2:
        return partition, f"{partition}/{parts[1]}/{parts[2]}", f"{parts[1]} {domain_label(parts[2])}"
    if partition == "maintaining" and len(parts) > 2:
        return partition, f"maintaining/{parts[1]}", parts[1]
    return partition, f"{partition}/00-根", f"{partition} 根目录"


def segment(body: str) -> list:
    """Split a page body into drawable segments; every fenced block closes on its own line.

    Fences are closed only by a line that is exactly ``` (optionally indented), so a
    block that loses its opening fence cannot swallow the rest of the page.
    """
    segs: list = []
    buffer: list[str] = []
    lines = body.split("\n")
    i = 0

    def flush() -> None:
        if buffer:
            text = "\n".join(buffer).strip("\n")
            if text.strip():
                segs.append(["t", text])
            buffer.clear()

    close = re.compile(r"^\s*```\s*$")
    fence_open = re.compile(r"^\s*```")
    while i < len(lines):
        line = lines[i]
        opening = re.match(r"^\s*```\s*([\w+-]*(?:\s+[\w+#.-]+)*)\s*$", line)
        if opening:
            flush()
            lang = (opening.group(1).split() or [""])[0]
            chunk: list[str] = []
            i += 1
            while i < len(lines) and not close.match(lines[i]):
                # a nested opening fence closes the outer block (malformed source):
                # the outer block cannot legally contain a fence
                if fence_open.match(lines[i]):
                    break
                chunk.append(lines[i])
                i += 1
            i += 1
            code = "\n".join(chunk)
            if lang.lower() == "mermaid":
                segs.append(["m", code])
            else:
                segs.append(["c", "" if lang in ("", "text", "txt") else lang, code])
            continue
        buffer.append(line)
        i += 1
    flush()
    return segs


def declared_kind(code: str) -> str:
    """The keyword the author wrote (what the page *meant*), for the viewer's label."""
    first = next((line.strip() for line in code.strip().splitlines() if line.strip()), "")
    return first.split()[0] if first else ""


def renderable_kind(code: str, verdict: dict[str, str], did: str) -> tuple[str, str]:
    """(kind, verdict) — verdict ok / parse / no-type / other, never invented."""
    status = verdict.get(did, "ok")
    return diagram_kind(code), status


# --------------------------------------------------------------------------- build


def page_shape(page: dict) -> str:
    """Classify a page by its own block statistics (rules mirror console/spec/data-contract.md §3.4)."""
    landed = page["layers"].count("1")
    if landed == 0 and page["links"] > 10:
        return "index-like"
    code = sum(page["fence"].values())
    if page.get("h3", 0) == 0 and code < 3:
        return "prose"
    if page.get("tables", 0) >= 2 and code < 6:
        return "table-heavy"
    if code >= 8:
        return "ops-heavy"
    return "structured"


def match_dimensions(headings: list[tuple[int, int, str]], dims: list[dict]) -> dict[str, dict]:
    """First matching dimension per heading; a heading can only claim one dimension.

    Levels are honoured: `h2` / `h3` on a dimension decide which heading levels may match it.
    """
    found: dict[str, dict] = {}
    for level, line, text in headings:
        low = text.lower()
        for dim in dims:
            allowed = dim.get("h3") if level == 3 else dim.get("h2")
            if not allowed:
                continue
            if any(str(p) in low for p in dim["patterns"]):  # type: ignore[union-attr]
                hit = found.setdefault(str(dim["id"]), {
                    "id": dim["id"], "name": dim["name"], "group": dim["group"],
                    "strength": "section", "line": line, "title": text, "hits": 0,
                })
                hit["hits"] += 1
                if level == 3 and hit["strength"] == "section":
                    hit["strength"] = "subsection"
                break
    return found


def outline(lines: list[str]) -> tuple[list[dict], list[dict], list[dict]]:
    """Walk the body once: headings with spans + payloads, tables, code blocks.

    Line numbers are 1-based over the body (frontmatter removed) — the anchor base is `body`.
    """
    heads: list[dict] = []
    tables: list[dict] = []
    codes: list[dict] = []
    current_top: dict | None = None
    current_sub: dict | None = None
    fenced = False
    fence_start = 0
    fence_lang = ""
    for idx, raw in enumerate(lines, 1):
        line = raw.rstrip()
        if fenced:
            if re.match(r"^\s*```\s*$", line):
                fenced = False
                codes.append({"line": fence_start, "lang": fence_lang, "lines": idx - fence_start - 1,
                              "h2": current_top["index"] if current_top else None,
                              "h3": current_sub["index"] if current_sub else None})
            continue
        opening = CODE_OPEN.match(line)
        if opening:
            fenced = True
            fence_start = idx
            fence_lang = (opening.group(1).split() or [""])[0]
            continue
        head = HEADING.match(line)
        if head:
            level = len(head.group(1))
            title = NUMBER_PREFIX.sub("", head.group(2)).strip()
            node = {"level": level, "title": title, "start": idx, "index": 0}
            if level == 2:
                current_top = node
                current_sub = None
                heads.append(node)
                node["index"] = len([h for h in heads if h["level"] == 2])
            else:
                current_sub = node
                heads.append(node)
                node["index"] = len([h for h in heads if h["level"] == 3])
            node["h2"] = current_top["index"] if current_top else None
            continue
        if TABLE_ROW.match(line) and not line.strip().startswith("|---"):
            if len(line.replace("|", "").strip()) >= 2:
                tables.append({"line": idx, "h2": current_top["index"] if current_top else None,
                               "h3": current_sub["index"] if current_sub else None})
    # close spans: a section ends one line before the next same-or-higher heading (or EOF)
    for i, node in enumerate(heads):
        node["end"] = len(lines)
        for follower in heads[i + 1:]:
            if follower["level"] <= node["level"]:
                node["end"] = follower["start"] - 1
                break
    # attach payload counts
    for node in heads:
        span = range(node["start"], node["end"] + 1)
        node["tables"] = sum(1 for t in tables if t["line"] in span)
        node["codes"] = sum(1 for c in codes if c["line"] in span)
        if node["level"] == 3:
            node["tables"] = sum(1 for t in tables if t.get("h3") == node["index"])
            node["codes"] = sum(1 for c in codes if c.get("h3") == node["index"])
    for node in heads:
        node.pop("h2", None)
    return heads, tables, codes


ENTITIES_FILE = ROOT / "console" / "spec" / "entities.json"


def load_entities() -> tuple[list[dict], dict]:
    """Read console/spec/entities.json; returns (entities, groups)."""
    if not ENTITIES_FILE.exists():
        return [], {}
    doc = json.loads(ENTITIES_FILE.read_text(encoding="utf-8"))
    return list(doc["entities"]), dict(doc["groups"])


def resolve_entities(entities: list[dict], page_ids: list[str]) -> list[dict]:
    """Bind each entity to the page that argues for it (pageHints, first hit wins).

    An entity without a resolvable page is dropped with `page: null` so the UI can say so
    instead of silently mis-pointing.
    """
    out = []
    for ent in entities:
        page = None
        for hint in ent.get("pageHints", []):
            page = next((pid for pid in page_ids if hint in pid), None)
            if page:
                break
        out.append({"id": ent["id"], "name": ent["name"], "group": ent["group"],
                    "aliases": ent.get("aliases", []), "page": page,
                    "hint": next((h for h in ent.get("pageHints", []) if page and h in page), None)})
    return out


def build_entities(entities: list[dict], groups: dict, blocks_by_page: dict) -> dict:
    """Emit entities.json payload: entities bound to pages, plus each page's dimension set."""
    resolved = resolve_entities(entities, sorted(blocks_by_page))
    matrix = {}
    for ent in resolved:
        page = ent["page"]
        ent["dims"] = {d["id"]: {"line": d["line"], "strength": d["strength"], "title": d["title"]}
                       for d in blocks_by_page.get(page, {}).get("dims", [])} if page else {}
        matrix[ent["id"]] = ent["dims"]
    return {"schema": "kudig-console-entities/1", "groups": groups,
            "entities": resolved, "dimensionIndex": matrix}


def build_blocks(page: dict, lines: list[str], dims: list[dict]) -> dict:
    heads, tables, codes = outline(lines)
    h2 = [h for h in heads if h["level"] == 2]
    h3 = [h for h in heads if h["level"] == 3]
    hits = match_dimensions([(h["level"], h["start"], h["title"]) for h in heads], dims)
    return {
        "lineBase": "body",
        "lines": len(lines),
        "shape": page["shape"],
        "outline": [{"l": h["level"], "n": h["index"], "t": h["title"], "s": h["start"], "e": h["end"],
                     "tb": h["tables"], "cd": h["codes"]} for h in heads],
        "tables": [{"l": t["line"], "h2": t["h2"], "h3": t["h3"], "rows": 0} for t in tables],
        "codes": [{"l": c["line"], "lang": c["lang"], "n": c["lines"]} for c in codes],
        "dims": list(hits.values()),
        "h2": len(h2),
        "h3": len(h3),
    }

def build() -> tuple[dict[Path, str], dict]:
    validation = json.loads(VALIDATION.read_text(encoding="utf-8")) if VALIDATION.exists() else {}
    dims, dim_version = load_dimensions()
    entities, groups = load_entities()
    files = iter_pages()
    pages: list[dict] = []
    content: dict[str, str] = {}
    blocks: list[dict] = []
    blocks_by_page: dict[str, dict] = {}
    stems: dict[str, list[str]] = {}

    for path in files:
        text = path.read_text(encoding="utf-8", errors="ignore")
        match = FM.match(text)
        fm = match.group(1) if match else ""
        body = text[match.end():] if match else text
        page_id = path.relative_to(ROOT).with_suffix("").as_posix()
        partition, domain_id, domain_name = classify(path)
        title = fm_value(fm, "title") or path.stem.replace("-", " ")
        headings = headings_of(body)
        fences = Counter(m.group(1) for m in (CODE_OPEN.match(line) for line in body.splitlines()) if m and m.group(1))
        stems[page_id] = sorted({Path(t.strip()).stem for t in WIKILINK.findall(body) if t.strip()})

        # `<!-- chunk: … -->` is a build artefact of the corpus pipeline. Blank it in place so
        # body line numbers keep matching the source file exactly (anchors depend on this).
        body = CHUNK_MARKER.sub(lambda m: "\n" * m.group(0).count("\n"), body)
        segs = segment(body)
        malformed = sum(1 for seg in segs if seg[0] == "c" and FENCE.search(seg[2]))
        mermaid_ids: list[str] = []
        kept: list = []
        for seg in segs:
            if seg[0] != "m":
                kept.append(seg)
                continue
            did = f"{page_id}#{len(mermaid_ids)}"
            mermaid_ids.append(did)
            kind, verdict = renderable_kind(seg[1], validation, did)
            if verdict == "ok":
                kept.append(["m", did, kind, seg[1], declared_kind(seg[1])])
            else:
                kept.append(["d", did, kind, verdict, seg[1]])
            blocks.append({"id": did, "page": page_id, "d": domain_id, "kind": kind, "v": verdict,
                           "len": len(seg[1])})
        content[page_id] = kept

        page_draft = {
            "layers": layer_bits(headings),
            "links": len(stems[page_id]),
            "fence": dict(fences.most_common(6)),
            "h3": sum(1 for line in body.splitlines() if line.startswith("### ")),
            "tables": sum(1 for line in body.splitlines() if TABLE_ROW.match(line)),
        }
        page_draft["shape"] = page_shape(page_draft)
        block_data = build_blocks(page_draft, body.split("\n"), dims)
        blocks_by_page[page_id] = block_data

        pages.append({
            "id": page_id,
            "t": title,
            "p": partition,
            "d": domain_id,
            "dn": domain_label(domain_name),
            "layers": layer_bits(headings),
            "tier": fm_value(fm, "tier"),
            "cat": fm_value(fm, "category"),
            "diff": fm_value(fm, "difficulty"),
            "date": fm_value(fm, "last_updated") or fm_value(fm, "updated"),
            "n": len(body.splitlines()),
            "sec": sum(1 for line in body.splitlines() if SECTION.match(line)) or len(headings),
            "dm": len(mermaid_ids),
            "tb": sum(1 for line in body.splitlines() if TABLE_ROW.match(line)),
            "fence": dict(fences.most_common(6)),
            "links": len(stems[page_id]),
            "bad": malformed,
            "shape": block_data["shape"],
        })

    by_stem: dict[str, list[dict]] = defaultdict(list)
    for page, path in zip(pages, files, strict=True):
        by_stem[path.stem].append(page)
    inbound: Counter[str] = Counter()
    for page in pages:
        targets = set()
        for stem in stems.get(page["id"], []):
            for hit in by_stem.get(stem, [])[:1]:
                if hit["id"] != page["id"]:
                    targets.add(hit["id"])
        page["out"] = sorted(targets)
        for target in page["out"]:
            inbound[target] += 1
    for page in pages:
        page["in"] = inbound.get(page["id"], 0)

    domains = build_domains(pages)
    outputs: dict[Path, str] = {
        DATA / "index.json": canonical_json({"schema": SCHEMA, "pages": pages}),
        DATA / "domains.json": canonical_json(domains),
        DATA / "graph" / "diagram.json": canonical_json(build_diagram(pages, blocks)),
        DATA / "graph" / "coverage.json": canonical_json(build_coverage(pages)),
        DATA / "graph" / "structure.json": canonical_json(build_structure(pages, domains, blocks)),
        DATA / "graph" / "entities.json": canonical_json(
            build_entities(entities, groups, blocks_by_page)
        ),
        DATA / "graph" / "blocks.json": canonical_json({
            "schema": "kudig-console-blocks/2",
            "dimensionsVersion": dim_version,
            "pages": blocks_by_page,
        }),
    }
    return outputs, content


def canonical_json(payload: object) -> str:
    """One line, no spaces — this is what --check compares, so it must be stable."""
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":"))


def dump_json(path: Path, payload: object) -> str:
    """Serialise (payload may be an object *or* an already-serialised JSON string)."""
    return payload if isinstance(payload, str) else canonical_json(payload)


def build_domains(pages: list[dict]) -> dict:
    trees = []
    for root, name, _ in PARTITIONS:
        members = [p for p in pages if p["p"] == root]
        if not members:
            continue
        groups: dict[str, list[dict]] = defaultdict(list)
        for page in members:
            groups[page["d"]].append(page)
        domains = []
        for key in sorted(groups):
            group = groups[key]
            tiers = Counter(p["tier"] for p in group)
            domains.append({
                "id": key,
                "name": group[0]["dn"],
                "count": len(group),
                "core": tiers.get("core", 0),
                "dm": sum(p["dm"] for p in group),
                "landed": sum(p["layers"].count("1") for p in group),
            })
        trees.append({"id": root, "name": name, "count": len(members),
                      "diagrams": sum(p["dm"] for p in members), "domains": domains})
    return {"schema": SCHEMA, "partitions": trees}


def build_diagram(pages: list[dict], blocks: list[dict]) -> dict:
    by_kind = Counter(b["kind"] for b in blocks)
    verdicts = Counter(b["v"] for b in blocks)
    by_domain: dict[str, Counter] = defaultdict(Counter)
    by_page: dict[str, Counter] = defaultdict(Counter)
    for block in blocks:
        by_domain[block["d"]][block["kind"]] += 1
        by_page[block["page"]][block["kind"]] += 1
    heaviest = sorted(({"id": p["id"], "t": p["t"], "d": p["d"], "dm": p["dm"]} for p in pages if p["dm"]),
                      key=lambda p: (-p["dm"], p["id"]))[:40]
    broken = [{"id": b["id"], "page": b["page"], "kind": b["kind"], "v": b["v"], "len": b["len"]}
              for b in blocks if b["v"] != "ok"]
    return {
        "schema": SCHEMA,
        "total": len(blocks),
        "kinds": [{"k": k, "n": n, "name": next((nm for _, kind, nm in DIAGRAM_KINDS if kind == k), k)}
                  for k, n in by_kind.most_common()],
        "verdicts": dict(verdicts),
        "byDomain": [{"d": d, "n": sum(c.values()), "kinds": dict(c.most_common())}
                     for d, c in sorted(by_domain.items(), key=lambda kv: (-sum(kv[1].values()), kv[0]))],
        "heaviest": heaviest,
        "pages": {pid: dict(c.most_common()) for pid, c in sorted(by_page.items())},
        "broken": broken,
    }


def build_coverage(pages: list[dict]) -> dict:
    rows = []
    for d in sorted({p["d"] for p in pages}):
        members = [p for p in pages if p["d"] == d]
        cells = []
        for idx, layer in enumerate(LAYERS):
            hit = sum(1 for p in members if p["layers"][idx] == "1")
            cells.append({"layer": layer["id"], "landed": hit, "total": len(members),
                          "status": "full" if hit == len(members) else ("partial" if hit else "gap")})
        rows.append({"d": d, "n": members[0]["dn"], "count": len(members), "cells": cells,
                     "sum": sum(c["landed"] for c in cells),
                     "full": sum(1 for c in cells if c["status"] == "full")})
    rows.sort(key=lambda r: (-r["full"], -r["count"], r["d"]))
    totals = []
    for idx, layer in enumerate(LAYERS):
        landed = sum(1 for p in pages if p["layers"][idx] == "1")
        gaps = [p["id"] for p in pages if p["layers"][idx] != "1"]
        totals.append({"layer": layer["id"], "name": layer["name"], "landed": landed, "total": len(pages),
                       "gapCount": len(gaps), "gaps": gaps[:200]})
    meta = [{"id": layer["id"], "name": layer["name"], "criterion": layer["criterion"]} for layer in LAYERS]
    return {"schema": SCHEMA, "layers": meta, "rows": rows, "totals": totals}


def build_structure(pages: list[dict], domains: dict, blocks: list[dict]) -> dict:
    tiers = Counter(p["tier"] or "(未标注)" for p in pages)
    cats = Counter(p["cat"] or "(未标注)" for p in pages)
    diff = Counter(p["diff"] or "(未标注)" for p in pages)
    orphans = [p["id"] for p in pages if p["in"] == 0]
    hubs = sorted(({"id": p["id"], "t": p["t"], "in": p["in"], "d": p["d"]} for p in pages),
                  key=lambda p: (-p["in"], p["id"]))[:20]
    return {
        "schema": SCHEMA,
        "partitions": domains["partitions"],
        "domains": sum(len(part["domains"]) for part in domains["partitions"]),
        "pages": len(pages),
        "lines": sum(p["n"] for p in pages),
        "diagrams": len(blocks),
        "diagramsOk": sum(1 for b in blocks if b["v"] == "ok"),
        "tables": sum(p["tb"] for p in pages),
        "links": sum(p["links"] for p in pages),
        "tiers": [{"k": k, "n": n} for k, n in sorted(tiers.items(), key=lambda kv: TIER_ORDER.get(kv[0], 9))],
        "categories": [{"k": k, "n": n} for k, n in cats.most_common(12)],
        "difficulty": [{"k": k, "n": n} for k, n in diff.most_common()],
        "orphanCount": len(orphans),
        "orphans": orphans[:300],
        "hubs": hubs,
    }


def build_outputs() -> tuple[dict[Path, str], dict]:
    """Return (file → serialised text, page → segments). Strings stay as built."""
    raw, content = build()
    return ({path: dump_json(path, text) if not isinstance(text, str) else text
             for path, text in raw.items()}, content)


def split_content(outputs: dict[Path, str], content: dict) -> dict[Path, str]:
    """Each page gets its own segment shard file (no monolithic index).

    Segment shape: ["t", text] · ["c", lang, code] · ["m", diagramId, kind, 0] ·
    ["d", diagramId, kind, verdict, source]  (unrenderable Mermaid kept verbatim).
    """
    for page_id, segs in content.items():
        outputs[CONTENT / f"{page_id}.json"] = canonical_json(segs)
    return outputs


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="exit 1 when console/ data is stale")
    parser.add_argument("--summary", action="store_true", help="print derived totals only")
    args = parser.parse_args()

    outputs, content = build_outputs()
    outputs = split_content(outputs, content)
    structure = json.loads(outputs[DATA / "graph" / "structure.json"])
    summary = (f"pages={structure['pages']} domains={structure['domains']} diagrams={structure['diagrams']} "
               f"ok={structure['diagramsOk']} tables={structure['tables']} links={structure['links']}")
    if args.summary:
        print(summary)
        return 0
    if args.check:
        stale = [p for p, text in outputs.items() if not p.exists() or p.read_text(encoding="utf-8") != text]
        if stale:
            print(f"STALE: {len(stale)} file(s) — run make console-sync (first: {stale[0].relative_to(ROOT)})")
            return 1
        print(f"OK: console corpus up to date. {summary}")
        return 0
    CONTENT.mkdir(parents=True, exist_ok=True)
    for old in CONTENT.glob("*.json"):
        old.unlink()
    for path, text in outputs.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
    print(f"wrote {len(outputs)} files")
    print(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
