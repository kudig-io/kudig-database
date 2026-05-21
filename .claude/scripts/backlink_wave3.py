#!/usr/bin/env python3
"""
Wave 3: Final cleanup for remaining orphans.
- Fix component extraction for release notes (envoy issue)
- Generic orphans: add inline links to hub concepts
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


def get_body(content: str) -> str:
    if not content.startswith("---"):
        return content
    parts = content.split("---", 2)
    if len(parts) < 3:
        return content
    return parts[2]


def add_related_link(target_path: Path, source_rel: str, source_title: str) -> bool:
    try:
        content = target_path.read_text(encoding="utf-8")
    except Exception:
        return False

    source_filename = Path(source_rel).stem
    checks = [
        f"[[{source_rel.replace('.md', '')}",
        f"[[{source_filename}",
        f"[[{source_title}",
    ]
    for check in checks:
        if check in content:
            return False

    link_line = f"- [[{source_rel.replace('.md', '')}|{source_title}]]\n"

    if "## Related" in content:
        idx = content.find("## Related")
        section_start = idx + len("## Related")
        next_header = content.find("\n## ", section_start)
        if next_header == -1:
            if not content.endswith("\n"):
                content += "\n"
            content += link_line
        else:
            content = content[:next_header] + link_line + content[next_header:]
    else:
        if not content.endswith("\n"):
            content += "\n"
        content += "\n## Related\n\n" + link_line

    target_path.write_text(content, encoding="utf-8")
    return True


def extract_component(title: str) -> str:
    """Extract component name from release note title."""
    # "envoy v1.36 Release Notes" -> "envoy"
    # "opa v0.68 Release Notes" -> "opa"
    # "opentelemetry-collector v0.101 Release Notes" -> "opentelemetry-collector"
    # Strategy: match everything up to a version pattern like v1.2, 1.2.3, v0.x
    m = re.match(r'^(.+?)\s+v?\d', title.strip())
    if m:
        return m.group(1).strip().lower().replace(' ', '-')
    return ""


def main():
    output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"

    with open(output_dir / "orphans.json", "r", encoding="utf-8") as f:
        orphans = json.load(f)

    # Build mappings
    all_pages = {}
    filename_to_paths = defaultdict(list)
    path_to_filename = {}

    for root, dirs, files in os.walk(VAULT_ROOT):
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for f in files:
            if f.endswith(".md"):
                md_path = Path(root) / f
                rel_path = str(md_path.relative_to(VAULT_ROOT))
                if rel_path in ("index.md", "log.md", "hot.md"):
                    continue
                all_pages[rel_path] = md_path
                filename = md_path.stem.lower()
                filename_to_paths[filename].append(rel_path)
                path_to_filename[rel_path] = md_path.stem

    backlinks_added = 0
    pages_modified = set()

    # --- Fix 1: Remaining release notes ---
    release_note_orphans = [p for p in orphans if "/topic-release-notes/" in p]
    print(f"Release note orphans to fix: {len(release_note_orphans)}")

    # Component name aliases for matching
    component_aliases = {
        "gatekeeper": ["opa-gatekeeper", "opa-gatekeeper-policy", "gatekeeper-policy"],
        "calico": ["calico-fta"],
        "envoy": ["envoy"],
        "cilium": ["cilium"],
        "istio": ["istio"],
        "linkerd": ["linkerd"],
        "vault": ["vault"],
        "consul": ["consul"],
        "traefik": ["traefik"],
        "nginx": ["nginx"],
    }

    for rel_path in release_note_orphans:
        md_path = all_pages.get(rel_path)
        if not md_path:
            continue
        content = md_path.read_text(encoding="utf-8")
        fm = get_frontmatter(content)
        title = fm.get("title", "") or md_path.stem

        component = extract_component(title)
        if not component:
            continue

        # Try direct match
        targets = filename_to_paths.get(component, [])
        # Try aliases
        if not targets and component in component_aliases:
            for alias in component_aliases[component]:
                targets = filename_to_paths.get(alias, [])
                if targets:
                    break
        # Try partial match
        if not targets:
            for fname, paths in filename_to_paths.items():
                if component in fname or fname in component:
                    targets = paths
                    break

        if not targets:
            continue

        for target_rel in targets:
            if target_rel == rel_path:
                continue
            target_md = all_pages.get(target_rel)
            if not target_md:
                continue
            if add_related_link(target_md, rel_path, title):
                backlinks_added += 1
                pages_modified.add(target_rel)
                break

    print(f"Release note backlinks added: {backlinks_added}")

    # --- Fix 2: Generic orphans - inline link to hub concepts ---
    generic_orphans = [p for p in orphans if "/topic-release-notes/" not in p]
    print(f"Generic orphans to fix: {len(generic_orphans)}")

    # Build hub keyword -> page mapping
    hub_keywords = {
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
        "service-mesh": "entities/istio.md",
        "prometheus": "entities/prometheus.md",
        "grafana": "entities/prometheus-grafana.md",
        "jaeger": "entities/jaeger.md",
        "opentelemetry": "entities/opentelemetry.md",
        "observability": "domain-06-observability/README.md",
        "argo": "entities/argo.md",
        "argocd": "entities/argo.md",
        "flux": "entities/flux.md",
        "gitops": "concepts/gitops-principles.md",
        "tekton": "entities/tekton.md",
        "cicd": "domain-08-release-change-management/README.md",
        "etcd": "entities/etcd.md",
        "apiserver": "entities/apiserver.md",
        "kubelet": "entities/kubelet.md",
        "kube-proxy": "entities/kube-proxy.md",
        "controller-manager": "entities/controller-manager.md",
        "helm": "entities/helm.md",
        "kustomize": "entities/kustomize.md",
        "operator": "entities/operator.md",
        "vault": "entities/vault.md",
        "cert-manager": "entities/cert-manager.md",
        "rbac": "entities/rbac.md",
        "opa": "entities/opa.md",
        "kyverno": "entities/kyverno.md",
        "security": "domain-05-security-compliance/README.md",
        "pvc": "concepts/pvc.md",
        "pv": "concepts/pv.md",
        "storage": "domain-04-storage-data/README.md",
        "ceph": "entities/ceph.md",
        "rook": "entities/rook.md",
        "longhorn": "entities/longhorn.md",
        "kafka": "entities/kafka.md",
        "redis": "entities/redis.md",
        "postgresql": "entities/postgresql.md",
        "mysql": "entities/mysql.md",
        "terraform": "entities/terraform.md",
        "pulumi": "entities/pulumi.md",
        "iac": "entities/terraform.md",
        "linux": "entities/linux.md",
        "kernel": "entities/linux.md",
        "cgroup": "concepts/cgroup.md",
        "namespace": "concepts/namespace.md",
        "systemd": "entities/systemd.md",
        "elasticsearch": "entities/elasticsearch.md",
        "loki": "entities/loki.md",
        "fluentd": "entities/fluentd.md",
        "logging": "domain-06-observability/03-logging/README.md",
        "backup": "entities/velero.md",
        "dr": "domain-09-reliability-engineering/02-disaster-recovery/README.md",
        "autoscaler": "entities/cluster-autoscaler.md",
        "hpa": "entities/hpa.md",
        "vpa": "entities/vpa.md",
        "karpenter": "entities/karpenter.md",
        "dns": "entities/coredns.md",
        "coredns": "entities/coredns.md",
        "openkruise": "entities/openkruise.md",
        "knative": "entities/knative.md",
        "dapr": "entities/dapr.md",
        "wasm": "entities/wasm.md",
        "gatekeeper": "domain-05-security-compliance/04-policy-governance/09-opa-gatekeeper-policy.md",
    }

    inline_added = 0
    for rel_path in generic_orphans:
        md_path = all_pages.get(rel_path)
        if not md_path:
            continue
        content = md_path.read_text(encoding="utf-8")
        body = get_body(content)
        fm = get_frontmatter(content)
        title = fm.get("title", "") or md_path.stem
        title_lower = title.lower()

        modified = False
        for keyword, target_rel in hub_keywords.items():
            if keyword in title_lower:
                continue  # Don't link if it's in title (likely self)
            if keyword not in body.lower():
                continue

            # Find first occurrence and wrap in wikilink
            escaped = re.escape(keyword)
            pattern = re.compile(r'(?<!\[)(?<!\[\[)\b' + escaped + r'\b', re.IGNORECASE)
            if pattern.search(body):
                new_body = pattern.sub(f"[[{target_rel.replace('.md', '')}|{keyword}]]", body, count=1)
                if new_body != body:
                    body = new_body
                    modified = True
                    inline_added += 1
                    break  # Only one link per page

        if modified:
            parts = content.split("---", 2)
            if len(parts) >= 3:
                new_content = f"---{parts[1]}---{body}"
                md_path.write_text(new_content, encoding="utf-8")
                pages_modified.add(rel_path)

    print(f"Inline links added to generic orphans: {inline_added}")
    print(f"Total pages modified in wave 3: {len(pages_modified)}")

    report = {
        "release_note_backlinks": backlinks_added,
        "generic_inline_links": inline_added,
        "pages_modified": len(pages_modified),
    }
    with open(output_dir / "backlink_wave3_report.json", "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    main()
