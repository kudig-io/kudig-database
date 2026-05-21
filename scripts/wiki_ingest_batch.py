#!/usr/bin/env python3
"""
Wiki Ingest Script — processes domain-34, domain-17, domain-5 source files
Generates structured Chinese wiki pages in entities/ references/ concepts/
"""
import json, os, re, hashlib, sys
from datetime import datetime, timezone

VAULT = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"
TODAY = "2026-05-21"
TODAY_ISO = "2026-05-21T00:00:00Z"

EXISTING_ENTITIES = set()
EXISTING_CONCEPTS = set()
EXISTING_REFS = set()

import glob as globmod
for f in globmod.glob(os.path.join(VAULT, "entities", "*.md")):
    EXISTING_ENTITIES.add(os.path.basename(f).replace(".md", ""))
for f in globmod.glob(os.path.join(VAULT, "concepts", "*.md")):
    EXISTING_CONCEPTS.add(os.path.basename(f).replace(".md", ""))
for f in globmod.glob(os.path.join(VAULT, "references", "*.md")):
    EXISTING_REFS.add(os.path.basename(f).replace(".md", ""))


def read_file(path):
    with open(os.path.join(VAULT, path), 'r', encoding='utf-8') as f:
        return f.read()


def sha256_content(content):
    return "sha256:" + hashlib.sha256(content.encode('utf-8')).hexdigest()


def file_stat(fpath):
    full = os.path.join(VAULT, fpath)
    st = os.stat(full)
    return st.st_size, datetime.fromtimestamp(st.st_mtime, tz=timezone.utc).isoformat()


def parse_frontmatter(content):
    if not content.startswith('---'):
        return {}, content
    end = content.find('---', 3)
    if end == -1:
        return {}, content
    fm_text = content[3:end].strip()
    body = content[end+3:].strip()
    info = {}
    for line in fm_text.split('\n'):
        line = line.strip()
        if ':' in line and not line.startswith('-'):
            k, _, v = line.partition(':')
            k = k.strip()
            v = v.strip().strip("'\"")
            if k and v:
                info[k] = v
    return info, body


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


def extract_project_name(fpath):
    parts = fpath.split('/')
    if len(parts) >= 3:
        folder = parts[-2]
        fname = parts[-1].replace('.md', '')
        if fname == folder:
            return folder
        return fname
    return parts[-1].replace('.md', '')


def get_cncf_status(fpath):
    if '/graduated/' in fpath:
        return 'Graduated'
    elif '/incubating/' in fpath:
        return 'Incubating'
    elif '/sandbox/' in fpath:
        return 'Sandbox'
    return 'Unknown'


def get_cncf_category(sections, body):
    cats_mapping = {
        'observability': ['prometheus', 'grafana', 'jaeger', 'opentelemetry', 'fluentd',
                          'thanos', 'cortex', 'pixie', 'perses', 'trickster'],
        'security': ['falco', 'trivy', 'opa', 'kyverno', 'spiffe', 'spire', 'tuf',
                     'kubescape', 'kubearmor', 'cert-manager', 'notary', 'ratify',
                     'kubewarden', 'tetragon', 'parsec', 'keylime', 'athenz', 'vault',
                     'dex', 'openfga', 'keycloak', 'paralus', 'bank-vaults',
                     'containerssh', 'oauth2-proxy', 'inclavare-containers',
                     'confidential-containers', 'open-policy-containers'],
        'networking': ['cilium', 'envoy', 'linkerd', 'coredns', 'cni', 'antrea',
                       'metallb', 'submariner', 'contour', 'kube-ovn',
                       'ovn-kubernetes', 'network-service-mesh', 'loxilb', 'bfe',
                       'emissary-ingress', 'kgateway', 'kuma', 'aeraki-mesh',
                       'kmesh', 'kube-vip', 'spiderpool', 'k8gb', 'easegress'],
        'runtime': ['containerd', 'cri-o', 'runc', 'kata', 'youki', 'kuasar',
                    'wasmedge', 'spin', 'spinkube', 'virtual-kubelet', 'podman',
                    'k3s', 'k0s', 'kairos', 'flatcar', 'bootc', 'composefs',
                    'urunc', 'hyperlight', 'container2wasm'],
        'storage': ['rook', 'longhorn', 'openebs', 'cubefs', 'harbor', 'dragonfly',
                    'vitess', 'tikv', 'piraeus-datastore', 'carina', 'hwameistor',
                    'cloudnativepg', 'opengemini', 'k8up', 'kanister'],
        'ci/cd': ['flux', 'argo', 'tekton', 'backstage', 'shipwright', 'pipecd',
                  'werf', 'atlantis', 'carvel', 'devspace', 'devfile', 'konveyor',
                  'buildpacks', 'ko', 'kpt', 'kcl', 'cdk8s', 'dalec'],
        'orchestration': ['kubernetes', 'helm', 'karmada', 'kubefleet',
                          'open-cluster-management', 'clusternet', 'clusterpedia',
                          'kubeedge', 'openyurt', 'kubestellar', 'kcp', 'kubeflow',
                          'volcano', 'keda', 'knative', 'openkruise', 'kubevela',
                          'operator-framework', 'crossplane', 'dapr', 'openfunction',
                          'slimfaas', 'serverless-workflow', 'serverless-devs',
                          'koordinator', 'kudo', 'kusionstack', 'kubefleet',
                          'armada', 'cozystack', 'score'],
        'container': ['containerd', 'cri-o', 'dapr', 'podman-desktop',
                      'podman-container-tools'],
        'platform': ['backstage', 'radius', 'openchoreo', 'cozystack',
                     'kubeclipper', 'headlamp', 'kusionstack', 'kudo', 'score',
                     'opengitops', 'kagent', 'holmesgpt', 'k8sgpt'],
        'policy': ['opa', 'kyverno', 'kubewarden', 'cedar',
                   'open-policy-containers'],
        'chaos': ['litmus', 'chaos-mesh', 'chaosblade', 'krkn'],
        'cost': ['opencost', 'kepler'],
        'image': ['harbor', 'distribution', 'zot', 'oras', 'stacker', 'copa',
                  'eraser', 'slimtoolkit', 'container2wasm', 'modelpack', 'kitops'],
        'edge': ['kubeedge', 'openyurt', 'akri', 'kairos', 'interlink'],
        'streaming': ['nats', 'strimzi', 'tremor', 'cadence'],
        'database': ['vitess', 'tikv', 'cloudnativepg', 'opengemini', 'oxia'],
        'serverless': ['knative', 'openfunction', 'slimfaas', 'fission',
                       'serverless-workflow', 'serverless-devs', 'spin', 'spinkube'],
        'supply-chain': ['in-toto', 'tuf', 'notary-project', 'ratify',
                         'artifact-hub', 'openfeature'],
        'service-mesh': ['istio', 'linkerd', 'envoy', 'kuma', 'aeraki-mesh',
                         'meshery', 'kmesh', 'sermant'],
        'wasm': ['wasmedge', 'wasmcloud', 'spin', 'spinkube', 'container2wasm',
                 'kuasar', 'hyperlight'],
        'metal': ['metal3-io', 'tinkerbell', 'flatcar', 'kairos'],
        'data': ['vineyard', 'drasi', 'tremor', 'oxia'],
        'registry': ['harbor', 'distribution', 'zot', 'oras', 'xregistry'],
        'config': ['kcl', 'kpt', 'carvel', 'cdk8s', 'opentofu', 'porter'],
        'multi-cluster': ['karmada', 'kubefleet', 'open-cluster-management',
                          'clusternet', 'clusterpedia', 'kubestellar', 'submariner'],
    }
    fpath_lower = body.lower()[:500] + ' ' + str(list(sections.keys())).lower()
    for cat, projects in cats_mapping.items():
        for p in projects:
            if p in fpath_lower:
                return cat.replace('-', ' ').title()
    # Try from CNCF category in content
    for line in body.split('\n'):
        if 'cncf 分类' in line.lower():
            parts = line.split('|')
            for pp in parts:
                pp = pp.strip().strip('* ')
                if pp and pp not in ['cncf 分类', 'CNCF 分类', '**CNCF 分类**', '']:
                    return pp
    return 'Ecosystem'


def find_wikilinks(body, source_path):
    links = []
    text_lower = body.lower()
    link_map = {
        'etcd': 'entities/etcd',
        'prometheus': 'entities/prometheus-grafana',
        'grafana': 'entities/prometheus-grafana',
        'istio': 'entities/istio',
        'cilium': 'entities/cilium',
        'containerd': 'entities/containerd',
        'flannel': 'concepts/cilium-ebpf-networking',
        'cni': 'entities/cni-plugins',
        'flux': 'entities/flux',
        'argo': 'entities/argocd',
        'falco': 'entities/falco',
        'trivy': 'entities/trivy',
        'crossplane': 'entities/crossplane',
        'vault': 'entities/vault',
        'kyverno': 'entities/kyverno',
        'networkpolicy': 'entities/networkpolicy',
        'deployment': 'entities/deployment',
        'crd': 'entities/crd-custom-resources',
        'custom resource': 'entities/crd-custom-resources',
        'operator': 'concepts/operator-pattern',
        'controller': 'concepts/controller-pattern',
        'service mesh': 'concepts/service-mesh-architecture',
        'gitops': 'concepts/gitops-principles',
        'observability': 'concepts/observability-pillars',
        'autoscal': 'concepts/autoscaling-strategies',
        'ebpf': 'concepts/cilium-ebpf-networking',
        'container runtime': 'concepts/container-runtime-comparison',
        'storage': 'concepts/storage-model',
        'high availability': 'concepts/high-availability-patterns',
        'microservice': 'concepts/microservice-resilience-patterns',
        'declarative': 'concepts/declarative-api',
        'platform engineering': 'concepts/platform-engineering-idp',
        'secret': 'concepts/secrets-management',
        'pod': 'concepts/pod-lifecycle',
        'kubelet': 'entities/kubelet',
        'kube-apiserver': 'entities/kube-apiserver',
        'scheduler': 'entities/kube-scheduler',
        'csi': 'entities/csi-drivers',
        'kube-proxy': 'concepts/service-networking',
        'security': 'concepts/security-defense-depth',
        'supply chain': 'concepts/supply-chain-security',
        'ci/cd': 'concepts/ci-cd-pipeline-patterns',
        'multi-tenancy': 'concepts/multi-tenancy-isolation',
    }
    seen = set()
    fname = os.path.basename(source_path).replace('.md', '')
    for keyword, wiki_path in link_map.items():
        if keyword in text_lower and wiki_path not in seen:
            if fname not in wiki_path:
                links.append(wiki_path)
                seen.add(wiki_path)
                if len(links) >= 5:
                    break
    if not links:
        links = ['concepts/kubernetes-architecture-overview']
    return links


def extract_summary(body, sections, title):
    for key in ['简介', '项目概述', '产品简介', '概述', 'Introduction', 'Overview', '产品定位']:
        if key in sections:
            text = sections[key].strip()
            for para in text.split('\n\n'):
                para = para.strip()
                if len(para) > 20 and not para.startswith('#') and not para.startswith('|'):
                    if len(para) > 200:
                        para = para[:197] + '...'
                    return para
    for line in body.split('\n'):
        line = line.strip()
        if (line and not line.startswith('#') and not line.startswith('|') and
                not line.startswith('>') and not line.startswith('---') and
                not line.startswith('-') and len(line) > 20):
            if len(line) > 200:
                line = line[:197] + '...'
            return line
    return f'{title} 的技术文档与最佳实践指南'


def extract_key_points(sections):
    points = []
    for key_name in ['核心特性', '核心功能', '核心架构', '核心能力', '核心价值', '关键特性',
                      '主要功能', '主要特性', '核心组件', '组件架构', '关键组件', '整体架构',
                      '架构特点', '技术特点', '技术优势', '产品特色', '设计特点',
                      '产品架构', '产品定位', '核心定位', '关键能力']:
        if key_name in sections:
            text = sections[key_name]
            for line in text.split('\n'):
                line = line.strip()
                if line.startswith('- **') or line.startswith('- '):
                    point = line.lstrip('- ').strip()
                    if len(point) > 5:
                        points.append(point)
            if points:
                break
    return points[:6]


def extract_best_practices(sections):
    practices = []
    for key_name in ['最佳实践', '生产部署', '部署建议', '生产建议', '运维建议',
                      '生产部署要点', '注意事项', '使用建议', '部署最佳实践',
                      '运维最佳实践', '部署运维', '运维指南']:
        if key_name in sections:
            text = sections[key_name]
            for line in text.split('\n'):
                line = line.strip()
                if line.startswith(('- ', '* ')) or re.match(r'^\d+\.', line):
                    point = re.sub(r'^[-*\d.]+\s*', '', line).strip()
                    if len(point) > 5:
                        practices.append(point)
            if practices:
                break
    return practices[:6]


def generate_cncf_wiki(source_path, content):
    info, body = parse_frontmatter(content)
    title = info.get('title', extract_project_name(source_path))
    sections = extract_sections(body)
    project_name = extract_project_name(source_path)
    cncf_status = get_cncf_status(source_path)
    category = get_cncf_category(sections, body)
    summary = extract_summary(body, sections, title)
    key_points = extract_key_points(sections)
    best_practices = extract_best_practices(sections)
    wikilinks = find_wikilinks(body, source_path)

    out_name = project_name
    out_path = f"entities/{out_name}.md"

    # Check if entity already exists
    if out_name in EXISTING_ENTITIES:
        existing_content = read_file(f"entities/{out_name}.md")
        if source_path not in existing_content:
            if 'sources:' in existing_content:
                existing_content = existing_content.replace(
                    'sources:\n', f'sources:\n- {source_path}\n', 1
                )
            existing_content = re.sub(
                r'updated: \d{4}-\d{2}-\d{2}', f'updated: {TODAY}', existing_content
            )
            return out_path, existing_content, 'update'
        return out_path, existing_content, 'skip'

    tier = 'core' if cncf_status == 'Graduated' else ('supporting' if cncf_status == 'Incubating' else 'reference')

    tags = ['k8s', 'cncf']
    cat_slug = category.lower().replace('/', '-').replace(' ', '-')
    if cat_slug and cat_slug != 'ecosystem':
        tags.append(cat_slug)
    tags.append(project_name)

    official_url = ''
    for line in body.split('\n'):
        if '官方网站' in line and 'http' in line:
            match = re.search(r'https?://[^\s|)]+', line)
            if match:
                official_url = match.group(0)

    language = 'Go'
    for line in body.split('\n'):
        if ('主要语言' in line or '开发语言' in line) and '|' in line:
            parts = line.split('|')
            for p in parts:
                p = p.strip()
                if p and '语言' not in p and not p.startswith(':'):
                    language = p.strip('* ')
                    break

    kp_md = '\n'.join(f'- {p}' for p in key_points) if key_points else '- 详见源文档获取完整信息 ^[inferred]'
    bp_md = '\n'.join(f'- {p}' for p in best_practices) if best_practices else '- 建议参考官方文档获取最新部署指南 ^[inferred]'
    links_md = '\n'.join(f'- [[{l}]]' for l in wikilinks)

    wiki = f"""---
title: "{title}"
category: entities
summary: "{summary[:200]}"
tags: [{', '.join(tags)}]
sources: ["{source_path}"]
created: {TODAY}
updated: {TODAY}
lifecycle: draft
lifecycle_changed: "{TODAY}"
tier: {tier}
base_confidence: 0.7
---

# {title}

> **CNCF 状态**: {cncf_status} | **类别**: {category} | **主要语言**: {language}

## 概述

{summary}

## 核心能力

{kp_md}

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

{bp_md}

## 架构定位

在 CNCF 生态中，{title} 属于 **{category}** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

{links_md}
"""
    return out_path, wiki, 'create'


def generate_cloud_provider_wiki(source_path, content):
    info, body = parse_frontmatter(content)
    title = info.get('title', os.path.basename(source_path).replace('.md', ''))
    sections = extract_sections(body)
    summary = extract_summary(body, sections, title)
    wikilinks = find_wikilinks(body, source_path)

    fname = os.path.basename(source_path).replace('.md', '')
    out_name = fname
    out_path = f"references/{out_name}.md"

    if out_name in EXISTING_REFS:
        existing_content = read_file(f"references/{out_name}.md")
        if source_path not in existing_content:
            existing_content = existing_content.replace(
                'sources:\n', f'sources:\n- {source_path}\n', 1
            )
            existing_content = re.sub(
                r'updated: \d{4}-\d{2}-\d{2}', f'updated: {TODAY}', existing_content
            )
            return out_path, existing_content, 'update'
        return out_path, existing_content, 'skip'

    provider = '云平台'
    provider_map = {
        'aws': 'Amazon Web Services (AWS)',
        'gke': 'Google Cloud (GCP)',
        'azure': 'Microsoft Azure',
        'alicloud': '阿里云 (Alibaba Cloud)',
        'ack': '阿里云容器服务 ACK',
        'tke': '腾讯云容器服务 TKE',
        'huawei': '华为云 CCE',
        'cce': '华为云 CCE',
        'ucloud': 'UCloud UK8S',
        'ibm': 'IBM IKS',
        'oracle': 'Oracle OKE',
        'volcengine': '火山引擎 VEK',
        'ctyun': '天翼云 TKE',
        'ecloud': '移动云 CKE',
        'apsara': '阿里云飞天云',
    }
    for key, val in provider_map.items():
        if key in source_path.lower() or key in title.lower():
            provider = val
            break

    key_sections_md = ''
    for sname in ['产品架构与核心组件', '核心功能', '产品特色', '核心优势', '关键特性',
                   '产品架构', '网络架构', '存储架构', '安全架构', '核心组件',
                   '集成方案', '网络方案', '存储方案', '安全方案']:
        if sname in sections:
            text = sections[sname][:500]
            key_sections_md += f"### {sname}\n\n{text}\n\n"
            break
    if not key_sections_md:
        key_sections_md = '详见源文档获取完整产品架构信息。^[inferred]\n'

    links_md = '\n'.join(f'- [[{l}]]' for l in wikilinks[:4])

    tags = ['k8s', 'cloud', 'managed-k8s']
    for kw in ['aws', 'eks', 'gke', 'aks', 'ack', 'tke', 'cce', 'alicloud',
               'huawei', 'tencent', 'oracle', 'ibm', 'volcengine', 'ctyun', 'ecloud']:
        if kw in source_path.lower() or kw in title.lower():
            tags.append(kw)

    wiki = f"""---
title: "{title}"
category: references
summary: "{summary[:200]}"
tags: [{', '.join(tags)}]
sources: ["{source_path}"]
created: {TODAY}
updated: {TODAY}
lifecycle: draft
lifecycle_changed: "{TODAY}"
tier: supporting
base_confidence: 0.7
---

# {title}

> **云厂商**: {provider}

## 产品概述

{summary}

## 核心架构

{key_sections_md}

## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

{links_md}
"""
    return out_path, wiki, 'create'


def generate_networking_wiki(source_path, content):
    info, body = parse_frontmatter(content)
    title = info.get('title', os.path.basename(source_path).replace('.md', ''))
    sections = extract_sections(body)
    summary = extract_summary(body, sections, title)
    wikilinks = find_wikilinks(body, source_path)

    fname = os.path.basename(source_path).replace('.md', '')
    out_name = fname
    out_path = f"entities/{out_name}.md"

    if out_name in EXISTING_ENTITIES:
        existing_content = read_file(f"entities/{out_name}.md")
        if source_path not in existing_content:
            existing_content = existing_content.replace(
                'sources:\n', f'sources:\n- {source_path}\n', 1
            )
            return out_path, existing_content, 'update'
        return out_path, existing_content, 'skip'

    key_sections_md = ''
    for sname in sections:
        if any(kw in sname for kw in ['核心', '架构', '组件', '功能', '特性', '原理',
                                        '模式', '设计', '机制', '流程', '方案',
                                        '配置', '部署', '运维', '优化', '排查',
                                        'CRD', '操作', '使用', '测试', '验证']):
            key_sections_md += f"### {sname}\n\n{sections[sname][:400]}\n\n"
            if len(key_sections_md) > 800:
                break
    if not key_sections_md:
        key_sections_md = '详见源文档获取完整技术细节。^[inferred]\n'

    links_md = '\n'.join(f'- [[{l}]]' for l in wikilinks[:4])

    wiki = f"""---
title: "{title}"
category: entities
summary: "{summary[:200]}"
tags: [k8s, networking, terway, cni, alicloud]
sources: ["{source_path}"]
created: {TODAY}
updated: {TODAY}
lifecycle: draft
lifecycle_changed: "{TODAY}"
tier: supporting
base_confidence: 0.7
---

# {title}

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

{summary}

## 技术细节

{key_sections_md}

## 与 K8s 网络模型的关系

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[entities/cilium|Cilium]] 类似的高性能网络方案。与 [[concepts/cilium-ebpf-networking|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 NetworkPolicy 实现 Pod 间访问控制 ^[inferred]

## 参考链接

{links_md}
"""
    return out_path, wiki, 'create'


def main():
    manifest_path = os.path.join(VAULT, '.manifest.json')
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)

    if 'sources' not in manifest:
        manifest['sources'] = {}

    all_files = []

    # domain-34-cncf-landscape
    for root, dirs, fnames in os.walk(os.path.join(VAULT, 'domain-34-cncf-landscape')):
        for fn in fnames:
            if fn.endswith('.md') and fn not in ('README.md', 'MOC.md'):
                rel = os.path.relpath(os.path.join(root, fn), VAULT)
                if rel not in manifest['sources']:
                    all_files.append(('cncf', rel))

    # domain-17-cloud-provider
    for root, dirs, fnames in os.walk(os.path.join(VAULT, 'domain-17-cloud-provider')):
        for fn in fnames:
            if fn.endswith('.md') and fn not in ('README.md', 'MOC.md'):
                rel = os.path.relpath(os.path.join(root, fn), VAULT)
                if rel not in manifest['sources']:
                    all_files.append(('cloud', rel))

    # domain-5-networking (only uningested)
    for root, dirs, fnames in os.walk(os.path.join(VAULT, 'domain-5-networking')):
        for fn in fnames:
            if fn.endswith('.md') and fn not in ('README.md', 'MOC.md'):
                rel = os.path.relpath(os.path.join(root, fn), VAULT)
                if rel not in manifest['sources']:
                    all_files.append(('networking', rel))

    print(f"Total files to process: {len(all_files)}")

    stats = {'created': 0, 'updated': 0, 'skipped': 0, 'errors': 0}
    log_entries = []

    for idx, (domain, source_path) in enumerate(all_files):
        try:
            content = read_file(source_path)
            size_bytes, modified_at = file_stat(source_path)

            if domain == 'cncf':
                out_path, wiki_content, action = generate_cncf_wiki(source_path, content)
            elif domain == 'cloud':
                out_path, wiki_content, action = generate_cloud_provider_wiki(source_path, content)
            else:
                out_path, wiki_content, action = generate_networking_wiki(source_path, content)

            if action == 'skip':
                stats['skipped'] += 1
            else:
                full_out = os.path.join(VAULT, out_path)
                os.makedirs(os.path.dirname(full_out), exist_ok=True)
                with open(full_out, 'w', encoding='utf-8') as f:
                    f.write(wiki_content)
                if action == 'create':
                    stats['created'] += 1
                    out_name = os.path.basename(out_path).replace('.md', '')
                    EXISTING_ENTITIES.add(out_name)
                    EXISTING_REFS.add(out_name)
                else:
                    stats['updated'] += 1

            manifest['sources'][source_path] = {
                'ingested_at': TODAY_ISO,
                'size_bytes': size_bytes,
                'modified_at': modified_at,
                'content_hash': sha256_content(content),
                'source_type': 'document',
                'project': None
            }
            log_entries.append(f"[{action.upper():7s}] {source_path} -> {out_path}")

            if (idx + 1) % 50 == 0:
                print(f"  Processed {idx+1}/{len(all_files)}...")

        except Exception as e:
            stats['errors'] += 1
            log_entries.append(f"[ERROR  ] {source_path}: {str(e)}")
            print(f"  ERROR on {source_path}: {e}")

    # Save manifest
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    # Write log
    log_path = os.path.join(VAULT, 'log.md')
    log_header = f"\n\n## Wiki Ingest Batch — {TODAY}\n\n"
    log_summary = f"**统计**: 创建 {stats['created']} | 更新 {stats['updated']} | 跳过 {stats['skipped']} | 错误 {stats['errors']}\n\n"
    log_content = log_header + log_summary + "```\n" + '\n'.join(log_entries) + "\n```\n"

    with open(log_path, 'a', encoding='utf-8') as f:
        f.write(log_content)

    print(f"\n=== DONE ===")
    print(f"Created: {stats['created']}")
    print(f"Updated: {stats['updated']}")
    print(f"Skipped: {stats['skipped']}")
    print(f"Errors:  {stats['errors']}")
    print(f"Manifest entries: {len(manifest['sources'])}")
    print(f"Log written to: {log_path}")


if __name__ == '__main__':
    main()
