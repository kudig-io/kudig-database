#!/usr/bin/env python3
"""
Build Page Registry for the Kudig Database vault.
Extracts frontmatter from all markdown files and produces:
1. registry.json — full page registry
2. empty_tags.json — pages with empty/missing tags
3. hub_candidates.json — candidate hub pages (core concepts)
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
    ".wiki-meta", ".zread", "_staging", "assets"
}

# Domain path → tag mapping
DOMAIN_TAG_MAP = {
    "domain-01-cluster-fundamentals": "cluster-fundamentals",
    "domain-02-workloads-applications": "workloads",
    "domain-03-networking-traffic": "networking",
    "domain-04-storage-data": "storage",
    "domain-05-security-compliance": "security",
    "domain-06-observability": "observability",
    "domain-07-platform-engineering": "platform-engineering",
    "domain-08-release-change-management": "release-management",
    "domain-09-reliability-engineering": "reliability",
    "domain-10-troubleshooting-diagnostics": "troubleshooting",
    "domain-11-cloud-providers": "cloud-providers",
    "domain-12-cost-optimization": "cost-optimization",
    "domain-13-container-runtime": "container-runtime",
    "domain-14-ai-ml-workloads": "ai-ml",
    "domain-15-edge-iot": "edge-iot",
    "domain-16-data-pipelines": "data-pipelines",
    "domain-17-service-mesh": "service-mesh",
    "domain-18-gitops-cicd": "gitops-cicd",
    "domain-19-landscape-references": "landscape-references",
    "domain-20-application-patterns": "application-patterns",
    "domain-java-kubernetes": "java-kubernetes",
    "concepts": "concepts",
    "entities": "entities",
    "skills": "skills",
    "best-practices": "best-practices",
    "references": "references",
    "journal": "journal",
    "docs": "docs",
    "man": "man",
    "corpus-config": "corpus-config",
}

# Known core concept / hub keywords
HUB_KEYWORDS = {
    "kubernetes", "k8s", "docker", "container", "pod", "deployment",
    "service", "ingress", "cni", "calico", "flannel", "cilium", "terway",
    "istio", "envoy", "linkerd", "service-mesh",
    "prometheus", "grafana", "jaeger", "opentelemetry", "observability",
    "argo", "argocd", "flux", "gitops", "tekton", "cicd",
    "etcd", "apiserver", "kubelet", "kube-proxy", "controller-manager",
    "helm", "kustomize", "operator",
    "vault", "cert-manager", "rbac", "opa", "kyverno", "security",
    "pvc", "pv", "storage", "ceph", "rook", "longhorn",
    "kafka", "redis", "postgresql", "mysql", "database",
    "terraform", "pulumi", "ansible", "iac",
    "linux", "kernel", "cgroup", "namespace", "systemd",
    "elasticsearch", "loki", "fluentd", "logging",
    "backup", "dr", "disaster-recovery", "chaos-engineering",
    "autoscaler", "hpa", "vpa", "karpenter", "cluster-autoscaler",
    "slb", "alb", "nlb", "loadbalancer", "dns", "coredns",
    "openkruise", "knative", "dapr",
}


def extract_frontmatter(content: str) -> dict:
    """Extract YAML frontmatter from markdown content."""
    if not content.startswith("---"):
        return {}
    parts = content.split("---", 2)
    if len(parts) < 3:
        return {}
    try:
        return yaml.safe_load(parts[1]) or {}
    except Exception:
        return {}


def infer_domain_tag(rel_path: str) -> str:
    """Infer domain tag from file path."""
    parts = rel_path.split(os.sep)
    for part in parts:
        for prefix, tag in DOMAIN_TAG_MAP.items():
            if part.lower().startswith(prefix.lower()):
                return tag
    return ""


def is_hub_candidate(title: str, filename: str, tags: list) -> bool:
    """Determine if a page is a candidate hub page."""
    text = f"{title} {filename}".lower()
    for kw in HUB_KEYWORDS:
        if kw in text:
            return True
    if "index" in text or "overview" in text or "architecture" in text:
        return True
    if "entities/" in text or "concepts/" in text:
        return True
    return False


def main():
    registry = {}
    empty_tags = []
    hub_candidates = []
    all_files = []

    for root, dirs, files in os.walk(VAULT_ROOT):
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for f in files:
            if f.endswith(".md"):
                all_files.append(Path(root) / f)

    print(f"Total markdown files found: {len(all_files)}")

    for md_path in all_files:
        rel_path = str(md_path.relative_to(VAULT_ROOT))
        if rel_path in ("index.md", "log.md", "hot.md"):
            continue

        try:
            content = md_path.read_text(encoding="utf-8")
        except Exception as e:
            print(f"  SKIP (read error): {rel_path} — {e}")
            continue

        fm = extract_frontmatter(content)
        title = fm.get("title", "") or md_path.stem
        tags = fm.get("tags", []) or []
        aliases = fm.get("aliases", []) or []
        category = fm.get("category", "") or ""

        if isinstance(tags, str):
            tags = [tags] if tags else []
        tags = [str(t).strip().lstrip("#") for t in tags if t]

        if isinstance(aliases, str):
            aliases = [aliases] if aliases else []
        aliases = [str(a).strip() for a in aliases if a]

        entry = {
            "path": rel_path,
            "filename": md_path.stem,
            "title": title,
            "tags": tags,
            "aliases": aliases,
            "category": category,
            "word_count": len(content.split()),
            "has_wikilinks": "[[" in content,
        }
        registry[rel_path] = entry

        if not tags:
            inferred = infer_domain_tag(rel_path)
            empty_tags.append({
                "path": rel_path,
                "title": title,
                "inferred_tag": inferred,
            })

        if is_hub_candidate(title, md_path.stem, tags):
            hub_candidates.append({
                "path": rel_path,
                "title": title,
                "tags": tags,
            })

    output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"
    output_dir.mkdir(parents=True, exist_ok=True)

    with open(output_dir / "registry.json", "w", encoding="utf-8") as f:
        json.dump(registry, f, ensure_ascii=False, indent=2)

    with open(output_dir / "empty_tags.json", "w", encoding="utf-8") as f:
        json.dump(empty_tags, f, ensure_ascii=False, indent=2)

    with open(output_dir / "hub_candidates.json", "w", encoding="utf-8") as f:
        json.dump(hub_candidates, f, ensure_ascii=False, indent=2)

    tag_counts = defaultdict(int)
    for e in registry.values():
        for t in e["tags"]:
            tag_counts[t] += 1

    print(f"\n=== Registry Built ===")
    print(f"Total pages: {len(registry)}")
    print(f"Pages with empty tags: {len(empty_tags)}")
    print(f"Hub candidates: {len(hub_candidates)}")
    print(f"Pages with wikilinks: {sum(1 for e in registry.values() if e['has_wikilinks'])}")
    print(f"\nTop 20 tags:")
    for tag, count in sorted(tag_counts.items(), key=lambda x: -x[1])[:20]:
        print(f"  #{tag}: {count}")


if __name__ == "__main__":
    main()
