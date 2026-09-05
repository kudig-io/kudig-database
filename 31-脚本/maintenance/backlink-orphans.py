#!/usr/bin/env python3
"""
backlink-orphans.py — 孤儿页反向链接修补（合并自 .claude/scripts/backlink_{fix,wave2,wave3,wave4}.py）

四种策略（--strategy，默认 all）：
  emitters       出链 >= --emitter-min-outgoing 且入链 = 0 的页面 → 其链接目标回链它
  index          topic-index / merged-indexes 索引页 → 其链接目标回链它
  release-notes  发布说明孤儿页 → 按标题提取组件名（含别名表）从实体页回链
  hub            其余孤儿页 → 正文中首个 hub 关键词替换为内联 wikilink

相对旧 wave 脚本的改进：
  - 链接写入前校验目标页面存在，不再制造断链（旧 wave3 hub 表指向旧目录树，
    原样运行会触发 quality.yml 的 Broken Wikilink Gate）
  - 自包含：现场构建链接图，不再依赖 .claude/scripts/output/page_stats.json
  - --dry-run 只统计不写盘；--vault-root 脱离硬编码；扫描范围排除冻结/生成目录

用法：
  python3 31-脚本/maintenance/backlink-orphans.py --dry-run
  python3 31-脚本/maintenance/backlink-orphans.py --strategy index --report /tmp/report.json
"""

import argparse
import json
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

try:
    import yaml
except ImportError:  # frontmatter 标题解析退化为文件名
    yaml = None

WIKILINK_RE = re.compile(r"\[\[([^\]|]+)(?:\|[^\]]+)?\]\]")

SKIP_FILES = {"index.md", "log.md", "hot.md"}

# 冻结/生成/工具目录不参与扫描：不给它们写链接，也不把它们当链接目标
EXCLUDE_DIRS = {
    ".git", ".github", ".githooks", ".obsidian", ".claude", ".vscode", ".idea",
    ".venv", ".ruff_cache", "node_modules",
    ".codebuddy", ".comate", ".mimocode", ".qoder", ".understand-anything",
    ".wiki-meta", ".zread", ".impeccable", ".mimosa", ".video_agent",
    "_archives", "_staging", "assets", "web",
    "30-站点", "32-发布", "33-源码", "35-元数据", "36-报告", "37-归档", "GTM",
}

# 发布说明标题 → 组件实体页文件名别名（源自 wave3）
COMPONENT_ALIASES = {
    "gatekeeper": ["opa-gatekeeper", "opa-gatekeeper-policy", "gatekeeper-policy"],
    "calico": ["calico-fta"],
}

# hub 关键词 → 目标页相对路径（源自 wave3，路径属旧版目录树）。
# 仅当目标页在当前库中真实存在时才会写入；命中旧路径的词条自动跳过，
# 待按 22-概念 / 23-实体 现行结构修订后可逐步恢复生效。
HUB_KEYWORDS = {
    "kubernetes": "entities/kubernetes.md",
    "k8s": "entities/kubernetes.md",
    "docker": "entities/docker.md",
    "container": "entities/docker.md",
    "pod": "concepts/pod-lifecycle.md",
    "deployment": "entities/deployment.md",
    "service": "entities/service.md",
    "ingress": "entities/ingress.md",
    "cni": "entities/cni.md",
    "calico": "skills/calico-fta.md",
    "flannel": "entities/flannel.md",
    "cilium": "entities/cilium.md",
    "terway": "entities/terway.md",
    "istio": "entities/istio.md",
    "envoy": "entities/envoy.md",
    "linkerd": "entities/linkerd.md",
    "prometheus": "entities/prometheus.md",
    "grafana": "entities/prometheus-grafana.md",
    "jaeger": "entities/jaeger.md",
    "opentelemetry": "entities/opentelemetry.md",
    "argo": "entities/argo.md",
    "argocd": "entities/argo.md",
    "gitops": "concepts/gitops-principles.md",
    "etcd": "entities/etcd.md",
    "apiserver": "entities/apiserver.md",
    "kubelet": "entities/kubelet.md",
    "kube-proxy": "entities/kube-proxy.md",
    "helm": "entities/helm.md",
    "kustomize": "entities/kustomize.md",
    "operator": "entities/operator.md",
    "vault": "entities/vault.md",
    "cert-manager": "entities/cert-manager.md",
    "rbac": "entities/rbac.md",
    "opa": "entities/opa.md",
    "kyverno": "entities/kyverno.md",
    "pvc": "concepts/pvc.md",
    "pv": "concepts/pv.md",
    "ceph": "entities/ceph.md",
    "rook": "entities/rook.md",
    "longhorn": "entities/longhorn.md",
    "kafka": "entities/kafka.md",
    "redis": "entities/redis.md",
    "postgresql": "entities/postgresql.md",
    "mysql": "entities/mysql.md",
    "terraform": "entities/terraform.md",
    "linux": "entities/linux.md",
    "kernel": "entities/linux.md",
    "cgroup": "concepts/cgroup.md",
    "namespace": "concepts/namespace.md",
    "systemd": "entities/systemd.md",
    "elasticsearch": "entities/elasticsearch.md",
    "loki": "entities/loki.md",
    "fluentd": "entities/fluentd.md",
    "backup": "entities/velero.md",
    "autoscaler": "entities/cluster-autoscaler.md",
    "hpa": "entities/hpa.md",
    "vpa": "entities/vpa.md",
    "karpenter": "entities/karpenter.md",
    "dns": "entities/coredns.md",
    "coredns": "entities/coredns.md",
    "openkruise": "entities/openkruise.md",
    "knative": "entities/knative.md",
    "dapr": "entities/dapr.md",
}


def page_body(content: str) -> str:
    if not content.startswith("---"):
        return content
    parts = content.split("---", 2)
    return parts[2] if len(parts) == 3 else content


def page_title(content: str, path: Path) -> str:
    if content.startswith("---") and yaml is not None:
        parts = content.split("---", 2)
        if len(parts) == 3:
            try:
                fm = yaml.safe_load(parts[1])
            except yaml.YAMLError:
                fm = None
            if isinstance(fm, dict) and fm.get("title"):
                return str(fm["title"])
    return path.stem


def normalize_link(link_text: str) -> str:
    link = link_text.split("|")[0].strip()
    return link.split("/")[-1].strip().lower()


def scan_vault(vault_root: Path):
    """返回 (rel路径 → Path 映射, 文件名小写 → rel路径列表)。"""
    all_pages = {}
    filename_to_paths = defaultdict(list)
    for root, dirs, files in os.walk(vault_root):
        dirs[:] = sorted(d for d in dirs if d not in EXCLUDE_DIRS)
        for name in files:
            if not name.endswith(".md"):
                continue
            path = Path(root) / name
            rel = path.relative_to(vault_root).as_posix()
            if Path(rel).name in SKIP_FILES:
                continue
            all_pages[rel] = path
            filename_to_paths[path.stem.lower()].append(rel)
    return all_pages, filename_to_paths


def build_graph(all_pages, filename_to_paths):
    """统计每页去重出链数与入链数（仅计入真实存在的目标）。"""
    outgoing = {}
    incoming = defaultdict(int)
    for rel, path in all_pages.items():
        try:
            body = page_body(path.read_text(encoding="utf-8"))
        except OSError:
            outgoing[rel] = 0
            continue
        names = {normalize_link(link) for link in WIKILINK_RE.findall(body)}
        outgoing[rel] = len(names)
        for name in names:
            for target in filename_to_paths.get(name, ()):
                if target != rel:
                    incoming[target] += 1
    return outgoing, incoming


def add_related_link(target_path: Path, source_rel: str, source_title: str, dry_run: bool) -> bool:
    """在目标页 ## Related 区补一条反向链接；目标已含该链接时不重复写。"""
    try:
        content = target_path.read_text(encoding="utf-8")
    except OSError:
        return False

    stem = Path(source_rel).stem
    already = (
        f"[[{source_rel[:-3]}" in content
        or f"[[{stem}" in content
        or f"[[{source_title}" in content
    )
    if already:
        return False

    link_line = f"- [[{source_rel[:-3]}|{source_title}]]\n"
    if "## Related" in content:
        # 插到节标题后（保持与 Related 节相邻；旧 wave 逻辑在节后还有附录内容时
        # 会把链接追加到文件末尾，脱离节外）
        heading_end = content.find("## Related") + len("## Related")
        insert_at = heading_end + 1  # 越过标题行换行符
        if content[insert_at:insert_at + 1] == "\n":
            insert_at += 1
        content = content[:insert_at] + link_line + content[insert_at:]
    else:
        if not content.endswith("\n"):
            content += "\n"
        content += "\n## Related\n\n" + link_line

    if not dry_run:
        target_path.write_text(content, encoding="utf-8")
    return True


def backfill_from_pages(source_pages, all_pages, filename_to_paths, dry_run):
    """让 source_pages 的链接目标回链它们（emitters / index 共用）。"""
    added = 0
    modified = set()
    for rel in source_pages:
        path = all_pages.get(rel)
        if path is None:
            continue
        try:
            content = path.read_text(encoding="utf-8")
        except OSError:
            continue
        title = page_title(content, path)
        for link in WIKILINK_RE.findall(page_body(content)):
            for target in filename_to_paths.get(normalize_link(link), ()):
                if target != rel and add_related_link(all_pages[target], rel, title, dry_run):
                    added += 1
                    modified.add(target)
    return added, modified


def strategy_emitters(ctx):
    min_out = ctx["min_outgoing"]
    sources = [
        rel for rel, out in ctx["outgoing"].items()
        if out >= min_out and ctx["incoming"].get(rel, 0) == 0
    ]
    return backfill_from_pages(sources, ctx["all_pages"], ctx["name_map"], ctx["dry_run"])


def strategy_index(ctx):
    sources = [
        rel for rel in ctx["all_pages"]
        if "/topic-index/" in rel or "/merged-indexes/" in rel
    ]
    return backfill_from_pages(sources, ctx["all_pages"], ctx["name_map"], ctx["dry_run"])


def extract_component(title: str) -> str:
    """'envoy v1.36 Release Notes' → 'envoy'（截断于版本号前）。"""
    match = re.match(r"^(.+?)\s+v?\d", title.strip())
    if not match:
        return ""
    return match.group(1).strip().lower().replace(" ", "-")


def strategy_release_notes(ctx):
    added = 0
    modified = set()
    orphans = ctx["orphans"]
    for rel in orphans:
        if "/topic-release-notes/" not in rel:
            continue
        path = ctx["all_pages"][rel]
        try:
            content = path.read_text(encoding="utf-8")
        except OSError:
            continue
        title = page_title(content, path)
        component = extract_component(title)
        if not component:
            continue

        targets = ctx["name_map"].get(component, ())
        if not targets and component in COMPONENT_ALIASES:
            for alias in COMPONENT_ALIASES[component]:
                targets = ctx["name_map"].get(alias, ())
                if targets:
                    break
        if not targets:
            targets = next(
                (
                    paths for fname, paths in ctx["name_map"].items()
                    if component in fname or fname in component
                ),
                (),
            )
        for target in targets:
            if target != rel and add_related_link(ctx["all_pages"][target], rel, title, ctx["dry_run"]):
                added += 1
                modified.add(target)
                break
    return added, modified


def strategy_hub(ctx):
    added = 0
    modified = set()
    for rel in ctx["orphans"]:
        if "/topic-release-notes/" in rel:
            continue
        path = ctx["all_pages"][rel]
        try:
            content = path.read_text(encoding="utf-8")
        except OSError:
            continue
        body = page_body(content)
        title_lower = page_title(content, path).lower()

        for keyword, target_rel in HUB_KEYWORDS.items():
            if keyword in title_lower or keyword not in body.lower():
                continue
            if target_rel not in ctx["all_pages"]:
                continue  # 旧目录树路径，目标已不存在则跳过
            pattern = re.compile(r"(?<!\[)\b" + re.escape(keyword) + r"\b", re.IGNORECASE)
            new_body = pattern.sub(
                f"[[{target_rel[:-3]}|{keyword}]]", body, count=1
            )
            if new_body == body:
                continue
            if not ctx["dry_run"]:
                parts = content.split("---", 2)
                if len(parts) == 3:
                    path.write_text(f"---{parts[1]}---{new_body}", encoding="utf-8")
                else:
                    path.write_text(new_body, encoding="utf-8")
            added += 1
            modified.add(rel)
            break
    return added, modified


STRATEGIES = {
    "emitters": strategy_emitters,
    "index": strategy_index,
    "release-notes": strategy_release_notes,
    "hub": strategy_hub,
}


def main() -> int:
    default_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--vault-root", type=Path, default=default_root)
    parser.add_argument(
        "--strategy", choices=[*STRATEGIES, "all"], default="all",
        help="要执行的回链策略",
    )
    parser.add_argument("--dry-run", action="store_true", help="只统计不写盘")
    parser.add_argument(
        "--emitter-min-outgoing", type=int, default=5,
        help="emitters 策略的最小出链数（默认 5）",
    )
    parser.add_argument("--report", type=Path, help="JSON 报告输出路径")
    args = parser.parse_args()

    vault_root = args.vault_root.resolve()
    if not vault_root.is_dir():
        print(f"vault root 不存在: {vault_root}", file=sys.stderr)
        return 2

    all_pages, filename_to_paths = scan_vault(vault_root)
    outgoing, incoming = build_graph(all_pages, filename_to_paths)
    orphans = sorted(rel for rel in all_pages if incoming.get(rel, 0) == 0)

    selected = list(STRATEGIES) if args.strategy == "all" else [args.strategy]
    ctx = {
        "all_pages": all_pages,
        "name_map": filename_to_paths,
        "outgoing": outgoing,
        "incoming": incoming,
        "orphans": orphans,
        "dry_run": args.dry_run,
        "min_outgoing": args.emitter_min_outgoing,
    }

    report = {
        "vault_root": str(vault_root),
        "pages_scanned": len(all_pages),
        "orphans": len(orphans),
        "dry_run": args.dry_run,
        "strategies": {},
    }
    pages_modified = set()
    for name in selected:
        added, modified = STRATEGIES[name](ctx)
        report["strategies"][name] = {"links_added": added, "pages_touched": len(modified)}
        pages_modified |= modified

    report["pages_modified_total"] = len(pages_modified)
    payload = json.dumps(report, ensure_ascii=False, indent=2)
    print(payload)
    if args.report:
        args.report.write_text(payload + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
