#!/usr/bin/env python3
"""
Cross-linker batch script for Kudig Database vault.
Handles multiple orphan types with different strategies.
"""

import os
import json
import re
import yaml
from pathlib import Path
from collections import defaultdict

VAULT_ROOT = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
EXCLUDE_DIRS = {
    ".git", ".obsidian", ".claude", ".venv", "_archives",
    ".understand-anything", ".ruff_cache", ".codebuddy", ".comate",
    ".wiki-meta", ".zread", "_staging", "assets", "web"
}

WIKILINK_RE = re.compile(r'\[\[([^\]|]+)(?:\|[^\]]+)?\]\]')
# Match standard markdown links: [text](./path) or [text](../path) or [text](path)
MD_LINK_RE = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')


def get_frontmatter(content: str) -> dict:
    if not content.startswith("---"):
        return {}
    parts = content.split("---", 2)
    if len(parts) < 3:
        return {}
    try:
        return yaml.safe_load(parts[1]) or {}
    except Exception:
        return {}


def rebuild_frontmatter(fm: dict, content: str) -> str:
    parts = content.split("---", 2)
    if len(parts) < 3:
        return content
    new_fm = yaml.dump(fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
    return f"---\n{new_fm}---{parts[2]}"


def get_body(content: str) -> str:
    if not content.startswith("---"):
        return content
    parts = content.split("---", 2)
    if len(parts) < 3:
        return content
    return parts[2]


class CrossLinker:
    def __init__(self):
        self.all_pages = {}
        self.filename_to_paths = defaultdict(list)
        self.path_to_filename = {}
        self.orphans = []
        self.hub_pages = set()
        self.stats = {"inline_added": 0, "related_added": 0, "md_links_converted": 0,
                      "prereq_linked": 0, "pages_modified": set()}

        self._load_registry()
        self._load_orphans()
        self._identify_hubs()

    def _load_registry(self):
        for root, dirs, files in os.walk(VAULT_ROOT):
            dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
            for f in files:
                if f.endswith(".md"):
                    md_path = Path(root) / f
                    rel_path = str(md_path.relative_to(VAULT_ROOT))
                    if rel_path in ("index.md", "log.md", "hot.md"):
                        continue
                    self.all_pages[rel_path] = md_path
                    filename = md_path.stem.lower()
                    self.filename_to_paths[filename].append(rel_path)
                    self.path_to_filename[rel_path] = md_path.stem

    def _load_orphans(self):
        output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"
        with open(output_dir / "orphans.json", "r", encoding="utf-8") as f:
            self.orphans = json.load(f)

    def _identify_hubs(self):
        """Identify hub pages by title/filename keywords."""
        hub_keywords = {
            "kubernetes", "k8s", "docker", "container", "pod", "deployment",
            "service", "ingress", "cni", "calico", "flannel", "cilium", "terway",
            "istio", "envoy", "linkerd", "service-mesh",
            "prometheus", "grafana", "jaeger", "opentelemetry", "observability",
            "argo", "argocd", "flux", "gitops", "tekton", "cicd",
            "etcd", "apiserver", "kubelet", "kube-proxy", "controller-manager",
            "helm", "kustomize", "operator",
            "vault", "cert-manager", "rbac", "opa", "kyverno", "security",
            "pvc", "pv", "storage", "ceph", "rook", "longhorn",
            "kafka", "redis", "postgresql", "mysql",
            "terraform", "pulumi", "ansible", "iac",
            "linux", "kernel", "cgroup", "namespace", "systemd",
            "elasticsearch", "loki", "fluentd", "logging",
            "backup", "dr", "disaster-recovery", "chaos-engineering",
            "autoscaler", "hpa", "vpa", "karpenter", "cluster-autoscaler",
            "slb", "alb", "nlb", "loadbalancer", "dns", "coredns",
            "openkruise", "knative", "dapr", "wasm", "gatekeeper",
        }
        for rel_path, md_path in self.all_pages.items():
            text = f"{md_path.stem} {rel_path}".lower()
            for kw in hub_keywords:
                if kw in text:
                    self.hub_pages.add(rel_path)
                    break

    def _page_exists_by_name(self, name: str) -> str:
        """Find a page by filename (case insensitive). Returns rel_path or None."""
        name_lower = name.lower().strip()
        # Direct match
        paths = self.filename_to_paths.get(name_lower, [])
        if paths:
            return paths[0]
        # Strip .md if present
        if name_lower.endswith(".md"):
            paths = self.filename_to_paths.get(name_lower[:-3], [])
            if paths:
                return paths[0]
        return None

    def _add_inline_link(self, content: str, mention: str, target_rel: str) -> str:
        """Add an inline wikilink for the first occurrence of mention."""
        # Escape regex special chars in mention
        escaped = re.escape(mention)
        pattern = re.compile(r'(?<!\[)(?<!\[\[)' + escaped + r'(?!\])', re.IGNORECASE)

        def replacer(m):
            return f"[[{target_rel.replace('.md', '')}|{m.group(0)}]]"

        new_content, count = pattern.subn(replacer, content, count=1)
        if count > 0:
            self.stats["inline_added"] += 1
        return new_content

    def _add_related_link(self, content: str, target_rel: str, target_title: str) -> str:
        """Add a link in ## Related section."""
        link_text = f"[[{target_rel.replace('.md', '')}|{target_title}]]"
        if link_text in content:
            return content
        if f"[[{target_rel.replace('.md', '')}" in content:
            return content

        related_line = f"- {link_text}\n"
        if "## Related" in content:
            idx = content.find("## Related")
            section_start = idx + len("## Related")
            next_header = content.find("\n## ", section_start)
            if next_header == -1:
                if not content.endswith("\n"):
                    content += "\n"
                content += related_line
            else:
                content = content[:next_header] + related_line + content[next_header:]
        else:
            if not content.endswith("\n"):
                content += "\n"
            content += "\n## Related\n\n" + related_line
        self.stats["related_added"] += 1
        return content

    def process_index_pages(self):
        """Convert standard markdown links to wikilinks in index pages."""
        index_pages = [p for p in self.orphans if "/topic-index/" in p or "/merged-indexes/" in p]
        print(f"Processing {len(index_pages)} index pages...")

        for rel_path in index_pages:
            md_path = self.all_pages.get(rel_path)
            if not md_path:
                continue
            content = md_path.read_text(encoding="utf-8")
            body = get_body(content)

            links_found = MD_LINK_RE.findall(body)
            modified = False
            for display, url in links_found:
                # Only process relative links (./ or ../)
                if not (url.startswith("./") or url.startswith("../")):
                    continue
                # Convert to wikilink
                # Remove ./ and .md
                target = url.lstrip("./").replace(".md", "")
                wikilink = f"[[{target}|{display}]]"
                old_link = f"[{display}]({url})"
                body = body.replace(old_link, wikilink, 1)
                modified = True
                self.stats["md_links_converted"] += 1

            if modified:
                # Rebuild content
                parts = content.split("---", 2)
                if len(parts) >= 3:
                    new_content = f"---{parts[1]}---{body}"
                    md_path.write_text(new_content, encoding="utf-8")
                    self.stats["pages_modified"].add(rel_path)

    def process_release_notes(self):
        """Link release notes pages to their entity pages via prerequisites."""
        release_notes = [p for p in self.orphans if "/topic-release-notes/" in p]
        print(f"Processing {len(release_notes)} release notes pages...")

        for rel_path in release_notes:
            md_path = self.all_pages.get(rel_path)
            if not md_path:
                continue
            content = md_path.read_text(encoding="utf-8")
            fm = get_frontmatter(content)
            prereqs = fm.get("prerequisites", []) or []

            if isinstance(prereqs, str):
                prereqs = [prereqs] if prereqs else []
            prereqs = [str(p).strip() for p in prereqs if p]

            if not prereqs:
                # Try to infer from title
                title = fm.get("title", "")
                # e.g. "opa v0.68 Release Notes" -> link to opa
                parts = title.split()
                if parts:
                    prereqs = [parts[0].lower()]

            linked = False
            for prereq in prereqs:
                target = self._page_exists_by_name(prereq)
                if not target:
                    continue
                # Add backlink from target to this release note
                target_md = self.all_pages.get(target)
                if not target_md:
                    continue
                target_content = target_md.read_text(encoding="utf-8")
                fm_source = get_frontmatter(content)
                source_title = fm_source.get("title", "") or md_path.stem
                new_target_content = self._add_related_link(target_content, rel_path, source_title)
                if new_target_content != target_content:
                    target_md.write_text(new_target_content, encoding="utf-8")
                    self.stats["pages_modified"].add(target)
                    self.stats["prereq_linked"] += 1
                    linked = True

            if linked:
                self.stats["pages_modified"].add(rel_path)

    def process_generic_orphans(self):
        """Scan generic orphan pages for hub mentions and add inline links."""
        # Exclude already-handled types
        handled_patterns = ("/topic-index/", "/merged-indexes/", "/topic-release-notes/")
        generic_orphans = [p for p in self.orphans if not any(x in p for x in handled_patterns)]
        print(f"Processing {len(generic_orphans)} generic orphan pages...")

        for rel_path in generic_orphans:
            md_path = self.all_pages.get(rel_path)
            if not md_path:
                continue
            content = md_path.read_text(encoding="utf-8")
            body = get_body(content)
            fm = get_frontmatter(content)
            title = fm.get("title", "") or md_path.stem

            modified = False
            # Check if body mentions any hub page name
            for hub_rel in list(self.hub_pages)[:200]:  # Limit to top 200 hubs for speed
                hub_filename = self.path_to_filename.get(hub_rel, "")
                hub_name = Path(hub_rel).stem
                if hub_filename.lower() in title.lower():
                    continue  # Don't link to self
                # Check if hub name appears in body
                pattern = re.compile(r'(?<!\[)(?<!\[\[)\b' + re.escape(hub_name) + r'\b', re.IGNORECASE)
                if pattern.search(body):
                    new_body = self._add_inline_link(body, hub_name, hub_rel)
                    if new_body != body:
                        body = new_body
                        modified = True
                        break  # Only add one link per page for now

            if modified:
                parts = content.split("---", 2)
                if len(parts) >= 3:
                    new_content = f"---{parts[1]}---{body}"
                    md_path.write_text(new_content, encoding="utf-8")
                    self.stats["pages_modified"].add(rel_path)

    def run(self):
        print("=== Cross-Linker Batch ===")
        self.process_index_pages()
        self.process_release_notes()
        self.process_generic_orphans()

        print(f"\n=== Results ===")
        print(f"MD links converted to wikilinks: {self.stats['md_links_converted']}")
        print(f"Prerequisite backlinks added: {self.stats['prereq_linked']}")
        print(f"Inline links added: {self.stats['inline_added']}")
        print(f"Related links added: {self.stats['related_added']}")
        print(f"Total pages modified: {len(self.stats['pages_modified'])}")

        # Save report
        output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"
        report = {
            "md_links_converted": self.stats["md_links_converted"],
            "prereq_linked": self.stats["prereq_linked"],
            "inline_added": self.stats["inline_added"],
            "related_added": self.stats["related_added"],
            "pages_modified": list(self.stats["pages_modified"]),
        }
        with open(output_dir / "cross_link_report.json", "w", encoding="utf-8") as f:
            json.dump(report, f, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    linker = CrossLinker()
    linker.run()
