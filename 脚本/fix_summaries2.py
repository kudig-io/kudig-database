#!/usr/bin/env python3
"""Fix remaining bad summaries and titles in wiki pages"""
import json, os, re, glob

VAULT = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"

def read_file(rel):
    with open(os.path.join(VAULT, rel), 'r', encoding='utf-8') as f:
        return f.read()

def write_file(rel, content):
    with open(os.path.join(VAULT, rel), 'w', encoding='utf-8') as f:
        f.write(content)

def extract_sections(body):
    sections = {}
    current_key = None
    current_lines = []
    for line in body.split('\n'):
        if line.startswith('## '):
            if current_key:
                sections[current_key] = '\n'.join(current_lines).strip()
            current_key = line[3:].strip()
            current_lines = []
        else:
            current_lines.append(line)
    if current_key:
        sections[current_key] = '\n'.join(current_lines).strip()
    return sections

def get_better_summary(body, sections):
    """Find a better summary from body content"""
    for key in ['简介', '项目概述', '产品简介', '概述', '产品定位', '核心定位']:
        if key in sections:
            text = sections[key].strip()
            for para in text.split('\n\n'):
                para = para.strip()
                # Skip bad content
                if (len(para) > 20 and not para.startswith('#') and
                    not para.startswith('|') and not para.startswith('description') and
                    not para.startswith('path:') and not para.startswith('┌') and
                    not para.startswith('2.') and not para.startswith('A[') and
                    'fta' not in para.lower()[:20]):
                    if len(para) > 200:
                        para = para[:197] + '...'
                    return para
    # Second pass: find any good paragraph
    for line in body.split('\n'):
        line = line.strip()
        if (line and len(line) > 30 and
            not line.startswith('#') and not line.startswith('|') and
            not line.startswith('>') and not line.startswith('---') and
            not line.startswith('-') and not line.startswith('title:') and
            not line.startswith('description:') and not line.startswith('path:') and
            not line.startswith('┌') and not line.startswith('│') and
            not line.startswith('└') and not line.startswith('A[') and
            not line.startswith('*') and not line.startswith('2.') and
            'fta' not in line.lower()[:30]):
            if len(line) > 200:
                line = line[:197] + '...'
            return line
    return None

# CNCF project title mappings
TITLE_MAP = {
    'opentelemetry': 'OpenTelemetry',
    'k3s': 'k3s 轻量级 Kubernetes',
    'containerd': 'containerd',
    'istio': 'Istio',
    'prometheus': 'Prometheus',
    'etcd': 'etcd',
    'helm': 'Helm',
    'cilium': 'Cilium',
    'envoy': 'Envoy',
    'flux': 'Flux',
    'argo': 'Argo Workflows',
    'falco': 'Falco',
    'opa': 'OPA (Open Policy Agent)',
    'coredns': 'CoreDNS',
    'jaeger': 'Jaeger',
    'rook': 'Rook',
    'harbor': 'Harbor',
    'longhorn': 'Longhorn',
    'thanos': 'Thanos',
    'keda': 'KEDA',
    'knative': 'Knative',
    'crossplane': 'Crossplane',
    'dapr': 'Dapr',
    'backstage': 'Backstage',
    'kyverno': 'Kyverno',
    'kubeflow': 'Kubeflow',
    'volcano': 'Volcano',
    'nats': 'NATS',
    'grpc': 'gRPC',
    'cri-o': 'CRI-O',
    'contour': 'Contour',
    'spiffe': 'SPIFFE',
    'spire': 'SPIRE',
    'tuf': 'TUF',
    'cloudevents': 'CloudEvents',
    'in-toto': 'in-toto',
    'tikv': 'TiKV',
    'vitess': 'Vitess',
    'cubefs': 'CubeFS',
    'dragonfly': 'Dragonfly',
    'fluentd': 'Fluentd',
    'kubeedge': 'KubeEdge',
    'linkerd': 'Linkerd',
    'chaos-mesh': 'Chaos Mesh',
    'strimzi': 'Strimzi',
    'artifactory-hub': 'Artifact Hub',
    'karmada': 'Karmada',
    'keycloak': 'Keycloak',
    'openkruise': 'OpenKruise',
    'opencost': 'OpenCost',
    'operator-framework': 'Operator Framework',
    'litmus': 'LitmusChaos',
    'cloud-custodian': 'Cloud Custodian',
    'kubevirt': 'KubeVirt',
    'kubevela': 'KubeVela',
    'kubescape': 'Kubescape',
    'cni': 'CNI (Container Network Interface)',
    'openfeature': 'OpenFeature',
    'openfga': 'OpenFGA',
    'fluid': 'Fluid',
    'kserve': 'KServe',
    'notary-project': 'Notary Project',
    'openyurt': 'OpenYurt',
    'flatcar': 'Flatcar Container Linux',
    'metal3-io': 'Metal3',
    'cortex': 'Cortex',
    'wasmcloud': 'wasmCloud',
    'lima': 'Lima',
    'buildpacks': 'Cloud Native Buildpacks',
    'artifact-hub': 'Artifact Hub',
    'emissary-ingress': 'Emissary-Ingress',
}

# Terway title mappings
TERWAY_TITLES = {
    '40-terway-product-overview': 'Terway 产品概览',
    '41-terway-architecture-deep-dive': 'Terway 架构深度解析',
    '42-terway-usage-guide': 'Terway 使用指南',
    '43-terway-crd-operations': 'Terway CRD 资源操作',
    '44-terway-operations-manual': 'Terway 运维手册',
    '45-terway-testing-validation': 'Terway 测试验证',
    '46-terway-performance-tuning': 'Terway 性能调优',
    '47-terway-troubleshooting-fta': 'Terway 故障排查',
}

# Containerd supplementary titles
CONTAINERD_TITLES = {
    '02-containerd-v2-features': 'containerd 2.0 新特性',
    '03-containerd-security-hardening': 'containerd 安全加固',
    '04-containerd-upgrade-migration': 'containerd 升级迁移',
    '05-containerd-windows-support': 'containerd Windows 支持',
    '06-containerd-observability': 'containerd 可观测性',
    '07-containerd-disaster-recovery': 'containerd 灾难恢复',
    '08-containerd-multi-tenant': 'containerd 多租户',
}

# Istio supplementary titles
ISTIO_TITLES = {
    '02-istio-advanced-traffic-management': 'Istio 高级流量管理',
    '03-istio-security-hardening': 'Istio 安全加固',
}

# Prometheus supplementary titles
PROM_TITLES = {
    '02-prometheus-promql-advanced': 'PromQL 高级查询',
    '03-prometheus-ha-deployment': 'Prometheus 高可用部署',
}

fixed = 0
for subdir in ['entities', 'references']:
    for fpath in glob.glob(os.path.join(VAULT, subdir, '*.md')):
        rel = os.path.relpath(fpath, VAULT)
        content = read_file(rel)
        fname = os.path.basename(fpath).replace('.md', '')
        changed = False
        
        # Fix title
        title_match = re.search(r'^title: "(.+?)"$', content, re.MULTILINE)
        if title_match:
            old_title = title_match.group(1)
            new_title = None
            
            if fname in TITLE_MAP:
                new_title = TITLE_MAP[fname]
            elif fname in TERWAY_TITLES:
                new_title = TERWAY_TITLES[fname]
            elif fname in CONTAINERD_TITLES:
                new_title = CONTAINERD_TITLES[fname]
            elif fname in ISTIO_TITLES:
                new_title = ISTIO_TITLES[fname]
            elif fname in PROM_TITLES:
                new_title = PROM_TITLES[fname]
            
            if new_title and new_title != old_title:
                content = content.replace(f'title: "{old_title}"', f'title: "{new_title}"')
                content = content.replace(f'# {old_title}', f'# {new_title}')
                changed = True
        
        # Fix bad summaries
        sum_match = re.search(r'summary: "([^"]*)"', content)
        if sum_match:
            summary = sum_match.group(1)
            is_bad = (summary.startswith('path:') or summary.startswith('┌') or
                     summary.startswith('2.') or summary.startswith('A[') or
                     summary.startswith('title:') or summary.startswith('description:') or
                     summary.startswith('node_cpu') or summary.startswith('service.beta') or
                     (summary.startswith('所需') and 'IP' in summary[:20]) or
                     len(summary) < 15)
            
            if is_bad:
                # Try to get from source
                src_match = re.search(r'sources: \["([^"]*)"', content)
                if src_match:
                    try:
                        src_content = read_file(src_match.group(1))
                        if src_content.startswith('---'):
                            end = src_content.find('---', 3)
                            body = src_content[end+3:].strip() if end != -1 else src_content
                        else:
                            body = src_content
                        sections = extract_sections(body)
                        better = get_better_summary(body, sections)
                        if better and better != summary:
                            content = content.replace(f'summary: "{summary}"', f'summary: "{better[:200]}"')
                            changed = True
                    except:
                        pass
        
        if changed:
            write_file(rel, content)
            fixed += 1
            print(f"  FIXED {fname}")

print(f"\nTotal fixed: {fixed}")
