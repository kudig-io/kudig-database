#!/usr/bin/env python3
"""
Fix empty tags by inferring from file path and inserting into frontmatter.
"""

import json
import yaml
from pathlib import Path

VAULT_ROOT = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
OUTPUT_DIR = VAULT_ROOT / ".claude" / "scripts" / "output"

# Domain path → tag mapping (expanded)
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
    "_meta": "meta",
    "_reports": "reports",
}


def infer_tags(rel_path: str, title: str) -> list:
    """Infer tags from path and title."""
    tags = set()
    parts = rel_path.split("/")

    # Path-based inference
    for part in parts:
        for prefix, tag in DOMAIN_TAG_MAP.items():
            if part.lower().startswith(prefix.lower()):
                tags.add(tag)

    # Title-based inference for common concepts
    title_lower = title.lower()
    concept_tags = {
        "kubernetes": "k8s", "k8s": "k8s", "docker": "docker",
        "prometheus": "prometheus", "grafana": "grafana",
        "etcd": "etcd", "helm": "helm", "argo": "argo",
        "istio": "istio", "envoy": "envoy", "cni": "cni",
        "vault": "vault", "opa": "opa", "rbac": "rbac",
        "ingress": "ingress", "dns": "dns", "coredns": "dns",
        "backup": "backup", "dr": "disaster-recovery",
        "observability": "observability", "logging": "logging",
        "tracing": "tracing", "monitoring": "monitoring",
        "gitops": "gitops", "cicd": "cicd", "tekton": "tekton",
        "security": "security", "network": "networking",
        "storage": "storage", "pvc": "storage", "pv": "storage",
        "operator": "operator", "crd": "crd",
        "autoscaler": "autoscaler", "hpa": "autoscaler",
        "chaos": "chaos-engineering", "sre": "sre",
        "linux": "linux", "kernel": "linux",
        "java": "java", "jvm": "java", "spring": "spring-boot",
        "terraform": "iac", "pulumi": "iac",
        "kafka": "kafka", "redis": "redis",
        "service mesh": "service-mesh",
    }
    for kw, tag in concept_tags.items():
        if kw in title_lower:
            tags.add(tag)

    return sorted(tags)


def update_frontmatter_tags(md_path: Path, tags: list):
    """Update frontmatter tags for a markdown file."""
    content = md_path.read_text(encoding="utf-8")
    if not content.startswith("---"):
        return False

    parts = content.split("---", 2)
    if len(parts) < 3:
        return False

    try:
        fm = yaml.safe_load(parts[1]) or {}
    except Exception:
        return False

    if fm.get("tags"):
        return False  # Already has tags

    fm["tags"] = tags

    # Re-serialize frontmatter
    new_fm = yaml.dump(fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
    new_content = f"---\n{new_fm}---{parts[2]}"
    md_path.write_text(new_content, encoding="utf-8")
    return True


def main():
    with open(OUTPUT_DIR / "empty_tags.json", "r", encoding="utf-8") as f:
        empty_tags = json.load(f)

    fixed = 0
    skipped = 0

    for item in empty_tags:
        rel_path = item["path"]
        title = item["title"]
        inferred = item.get("inferred_tag", "")

        md_path = VAULT_ROOT / rel_path
        if not md_path.exists():
            skipped += 1
            continue

        tags = infer_tags(rel_path, title)
        if not tags and inferred:
            tags = [inferred]
        if not tags:
            tags = ["general"]

        if update_frontmatter_tags(md_path, tags):
            fixed += 1
        else:
            skipped += 1

    print(f"Fixed: {fixed}, Skipped: {skipped}")


if __name__ == "__main__":
    main()
