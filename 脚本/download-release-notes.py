#!/usr/bin/env python3
import urllib.request, json, os, time, re, sys

BASE_DIR = "topic-release-notes"


def fetch_json(url, retries=3):
    for attempt in range(retries):
        req = urllib.request.Request(url, headers={"User-Agent": "curl"})
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                return json.loads(resp.read())
        except urllib.error.HTTPError as e:
            if e.code == 403:
                reset = int(e.headers.get("X-RateLimit-Reset", 0))
                wait = max(reset - int(time.time()), 10) + 2
                print(f"  RATE LIMITED, waiting {wait}s...", flush=True)
                time.sleep(wait)
                continue
            if e.code == 404:
                return None
            if attempt < retries - 1:
                time.sleep(3)
                continue
            raise
        except Exception:
            if attempt < retries - 1:
                time.sleep(2)
                continue
            raise
    return None


def get_all_releases(repo, max_pages=30):
    releases = []
    for page in range(1, max_pages + 1):
        url = f"https://api.github.com/repos/{repo}/releases?per_page=100&page={page}"
        data = fetch_json(url)
        if data is None:
            break
        releases.extend(data)
        if len(data) < 100:
            break
    return releases


def parse_version(tag):
    tag = tag.lstrip("vV")
    m = re.match(r"(\d+)\.(\d+)(?:\.(\d+))?", tag)
    if m:
        return (int(m.group(1)), int(m.group(2)), int(m.group(3) or 0))
    return None


SKIP = [
    "alpha",
    "beta",
    "rc",
    "pre",
    "dev",
    "nightly",
    "canary",
    "snapshot",
    "test",
    "experimental",
]


def group_releases(releases):
    groups = {}
    for r in releases:
        tag = r["tag_name"]
        tl = tag.lower()
        if any(x in tl for x in SKIP):
            continue
        ver = parse_version(tag)
        if not ver:
            continue
        key = f"{ver[0]}.{ver[1]}"
        if key not in groups or ver[2] > groups[key][0]:
            groups[key] = (ver[2], r)
    return groups


def download_project(name, repo, category_dir):
    project_dir = (
        os.path.join(BASE_DIR, category_dir, name)
        if category_dir
        else os.path.join(BASE_DIR, name)
    )
    os.makedirs(project_dir, exist_ok=True)
    print(f"[{category_dir or 'kubernetes'}] {name} ({repo})...", flush=True)

    releases = get_all_releases(repo)
    if not releases:
        print(f"  No releases found", flush=True)
        return 0

    groups = group_releases(releases)
    count = 0
    for ver_key in sorted(groups.keys(), key=lambda v: tuple(map(int, v.split(".")))):
        patch, release = groups[ver_key]
        tag = release["tag_name"]
        body = release.get("body", "") or "(No release notes)"
        html_url = release.get(
            "html_url", f"https://github.com/{repo}/releases/tag/{tag}"
        )
        outfile = os.path.join(project_dir, f"RELEASE-NOTES-{ver_key}.md")
        with open(outfile, "w") as f:
            f.write(f"# {name} v{ver_key} Release Notes\n\n")
            f.write(f"Source: [{tag}]({html_url})\n\n")
            f.write(body)
        count += 1
    print(f"  {count} versions", flush=True)
    return count


def download_kubernetes_changelogs():
    versions = [f"1.{i}" for i in range(2, 37)]
    project_dir = os.path.join(BASE_DIR, "kubernetes")
    os.makedirs(project_dir, exist_ok=True)
    count = 0
    for v in versions:
        url = f"https://raw.githubusercontent.com/kubernetes/kubernetes/master/CHANGELOG/CHANGELOG-{v}.md"
        req = urllib.request.Request(url, headers={"User-Agent": "curl"})
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                content = resp.read()
            outfile = os.path.join(project_dir, f"CHANGELOG-{v}.md")
            with open(outfile, "wb") as f:
                f.write(content)
            count += 1
        except Exception as e:
            print(f"  Failed CHANGELOG-{v}.md: {e}", flush=True)
    print(f"[kubernetes] CHANGELOG files: {count}", flush=True)
    return count


ALL_PROJECTS = [
    ("", "kubernetes", "kubernetes/kubernetes"),
    ("core-deps", "etcd", "etcd-io/etcd"),
    ("core-deps", "containerd", "containerd/containerd"),
    ("core-deps", "cri-o", "cri-o/cri-o"),
    ("core-deps", "runc", "opencontainers/runc"),
    ("core-deps", "coredns", "coredns/coredns"),
    ("cli-tools", "minikube", "kubernetes/minikube"),
    ("cli-tools", "kind", "kubernetes-sigs/kind"),
    ("cli-tools", "kops", "kubernetes/kops"),
    ("cli-tools", "kustomize", "kubernetes-sigs/kustomize"),
    ("cli-tools", "helm", "helm/helm"),
    ("networking", "istio", "istio/istio"),
    ("networking", "envoy", "envoyproxy/envoy"),
    ("networking", "cni-plugins", "containernetworking/plugins"),
    ("networking", "calico", "projectcalico/calico"),
    ("networking", "cilium", "cilium/cilium"),
    ("networking", "linkerd", "linkerd/linkerd2"),
    ("observability", "prometheus", "prometheus/prometheus"),
    ("observability", "grafana", "grafana/grafana"),
    (
        "observability",
        "opentelemetry-collector",
        "open-telemetry/opentelemetry-collector",
    ),
    ("observability", "thanos", "thanos-io/thanos"),
    ("observability", "loki", "grafana/loki"),
    ("cicd-gitops", "argo-cd", "argoproj/argo-cd"),
    ("cicd-gitops", "flux", "fluxcd/flux2"),
    ("cicd-gitops", "tekton", "tektoncd/pipeline"),
    ("security", "falco", "falcosecurity/falco"),
    ("security", "opa", "open-policy-agent/opa"),
    ("security", "gatekeeper", "open-policy-agent/gatekeeper"),
    ("security", "trivy", "aquasecurity/trivy"),
    ("security", "cert-manager", "cert-manager/cert-manager"),
    ("storage", "rook", "rook/rook"),
    ("storage", "velero", "vmware-tanzu/velero"),
    ("storage", "longhorn", "longhorn/longhorn"),
]


def main():
    total = 0

    # Kubernetes from CHANGELOG directory
    k8s_dir = os.path.join(BASE_DIR, "kubernetes")
    os.makedirs(k8s_dir, exist_ok=True)
    count = download_kubernetes_changelogs()
    total += count

    # Kubernetes v1.0 and v1.1 from GitHub Releases
    print("[kubernetes] v1.0, v1.1 from GitHub Releases...", flush=True)
    k8s_releases = get_all_releases("kubernetes/kubernetes")
    k8s_groups = group_releases(k8s_releases)
    for ver in [
        "0.4",
        "0.5",
        "0.6",
        "0.7",
        "0.8",
        "0.9",
        "0.10",
        "0.11",
        "0.12",
        "0.13",
        "0.14",
        "0.15",
        "0.16",
        "0.17",
        "0.18",
        "0.19",
        "0.20",
        "0.21",
        "1.0",
        "1.1",
    ]:
        if ver in k8s_groups:
            _, release = k8s_groups[ver]
            tag = release["tag_name"]
            body = release.get("body", "") or "(No release notes)"
            html_url = release.get("html_url", "")
            outfile = os.path.join(k8s_dir, f"RELEASE-NOTES-{ver}.md")
            with open(outfile, "w") as f:
                f.write(f"# Kubernetes v{ver} Release Notes\n\n")
                f.write(f"Source: [{tag}]({html_url})\n\n")
                f.write(body)
            total += 1
    print(f"  Kubernetes pre-GA + v1.0/v1.1 done", flush=True)

    # All other projects
    for category, name, repo in ALL_PROJECTS[1:]:
        try:
            count = download_project(name, repo, category)
            total += count
        except Exception as e:
            print(f"  ERROR {name}: {e}", flush=True)

    print(f"\nTOTAL: {total} release note files", flush=True)


if __name__ == "__main__":
    main()
