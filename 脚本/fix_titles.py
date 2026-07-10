#!/usr/bin/env python3
"""Final fix: titles and remaining bad summaries for sandbox projects"""
import os, re, glob

VAULT = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"

def read_file(rel):
    with open(os.path.join(VAULT, rel), 'r', encoding='utf-8') as f:
        return f.read()

def write_file(rel, content):
    with open(os.path.join(VAULT, rel), 'w', encoding='utf-8') as f:
        f.write(content)

def get_title_from_summary(summary, fname):
    """Extract a proper title from the summary text"""
    # Common patterns: "ProjectName 是...", "ProjectName (FullName) 是..."
    m = re.match(r'^([A-Za-z][A-Za-z0-9\s\-()./]+?)\s+(?:是|为|一个|提供|基于|在)', summary)
    if m:
        return m.group(1).strip()
    
    # Try CNCF category-based naming
    name_map = {
        'ovn-kubernetes': 'OVN-Kubernetes',
        'cert-manager': 'cert-manager 证书管理',
        'vscode-kubernetes-tools': 'VS Code Kubernetes Tools',
        'kube-burner': 'Kube-burner 性能测试',
        'external-secrets': 'External Secrets Operator',
        'oscal-compass': 'OSCAL Compass',
        'connect-rpc': 'Connect RPC',
        'inspektor-gadget': 'Inspektor Gadget',
        'runme-notebooks': 'Runme 交互式笔记本',
        'logging-operator': 'Logging Operator',
        'open-cluster-management': 'Open Cluster Management',
        'network-service-mesh': 'Network Service Mesh',
        'kube-ovn': 'Kube-OVN',
        'serverless-devs': 'Serverless Devs',
        'oauth2-proxy': 'OAuth2 Proxy',
        'kube-vip': 'kube-vip',
        'piraeus-datastore': 'Piraeus Datastore',
        'kube-rs': 'kube-rs',
        'virtual-kubelet': 'Virtual Kubelet',
        'aeraki-mesh': 'Aeraki Mesh',
        'akri': 'Akri',
        'antrea': 'Antrea',
        'armada': 'Armada',
        'athenz': 'Athenz',
        'atlantis': 'Atlantis',
        'bank-vaults': 'Bank Vaults',
        'bfe': 'BFE',
        'bootc': 'bootc',
        'bpfman': 'bpfman',
        'cadence': 'Cadence',
        'capsule': 'Capsule',
        'carina': 'Carina',
        'cartography': 'Cartography',
        'carvel': 'Carvel',
        'cdk8s': 'cdk8s',
        'cedar': 'Cedar',
        'chaosblade': 'ChaosBlade',
        'cloudnativepg': 'CloudNativePG',
        'clusternet': 'Clusternet',
        'clusterpedia': 'Clusterpedia',
        'cohdi': 'Cohdi',
        'composefs': 'composefs',
        'confidential-containers': 'Confidential Containers',
        'container2wasm': 'container2wasm',
        'containerssh': 'ContainerSSH',
        'copa': 'Copa',
        'cozystack': 'Cozystack',
        'dalec': 'Dalec',
        'devfile': 'Devfile',
        'devspace': 'DevSpace',
        'dex': 'Dex',
        'distribution': 'Distribution',
        'drasi': 'Drasi',
        'easegress': 'Easegress',
        'eraser': 'Eraser',
        'hami': 'HAMI',
        'headlamp': 'Headlamp',
        'hexa': 'Hexa',
        'holmesgpt': 'HolmesGPT',
        'hwameistor': 'HwameiStor',
        'hyperlight': 'Hyperlight',
        'inclavare-containers': 'Inclavare Containers',
        'interlink': 'interLink',
        'k0s': 'k0s',
        'k8gb': 'k8gb',
        'k8sgpt': 'k8sgpt',
        'k8up': 'K8up',
        'kagent': 'kagent',
        'kairos': 'Kairos',
        'kaito': 'KAITO',
        'kanister': 'Kanister',
        'kcl': 'KCL',
        'kcp': 'kcp',
        'kepler': 'Kepler',
        'keylime': 'Keylime',
        'kgateway': 'kgateway',
        'kitops': 'KitOps',
        'kmesh': 'Kmesh',
        'ko': 'ko',
        'konveyor': 'Konveyor',
        'koordinator': 'Koordinator',
        'kpt': 'kpt',
        'krkn': 'Krkn',
        'kuadrant': 'Kuadrant',
        'kuasar': 'Kuasar',
        'kubean': 'Kubean',
        'kubearmor': 'KubeArmor',
        'kubeclipper': 'KubeClipper',
        'kubeelasti': 'KubeElasti',
        'kubefleet': 'KubeFleet',
        'kuberhealthy': 'Kuberhealthy',
        'kubeslice': 'KubeSlice',
        'kubestellar': 'Kubestellar',
        'kubewarden': 'Kubewarden',
        'kudo': 'KUDO',
        'kuma': 'Kuma',
        'kured': 'Kured',
        'kusionstack': 'KusionStack',
        'loxilb': 'LoxiLB',
        'meshery': 'Meshery',
        'metallb': 'MetalLB',
        'microcks': 'Microcks',
        'modelpack': 'ModelPack',
        'open-policy-containers': 'Open Policy Containers',
        'openchoreo': 'OpenChoreo',
        'openebs': 'OpenEBS',
        'openfunction': 'OpenFunction',
        'opengemini': 'openGemini',
        'opengitops': 'OpenGitOps',
        'opentofu': 'OpenTofu',
        'oras': 'ORAS',
        'paralus': 'Paralus',
        'parsec': 'Parsec',
        'perses': 'Perses',
        'pipecd': 'PipeCD',
        'pixie': 'Pixie',
        'podman-container-tools': 'Podman 容器工具',
        'podman-desktop': 'Podman Desktop',
        'porter': 'Porter',
        'radius': 'Radius',
        'ratify': 'Ratify',
        'schemahero': 'SchemaHero',
        'score': 'Score',
        'sermant': 'Sermant',
        'serverless-workflow': 'Serverless Workflow',
        'shipwright': 'Shipwright',
        'slimfaas': 'SlimFaas',
        'slimtoolkit': 'SlimToolkit',
        'sops': 'SOPS',
        'spiderpool': 'Spiderpool',
        'spin': 'Spin',
        'spinkube': 'SpinKube',
        'stacker': 'Stacker',
        'submariner': 'Submariner',
        'telepresence': 'Telepresence',
        'tinkerbell': 'Tinkerbell',
        'tokenetes': 'Tokenetes',
        'tremor': 'Tremor',
        'trickster': 'Trickster',
        'urunc': 'urunc',
        'vineyard': 'Vineyard',
        'wasmedge': 'WasmEdge',
        'werf': 'werf',
        'xregistry': 'xRegistry',
        'youki': 'youki',
        'zot': 'Zot',
        'kubernetes': 'Kubernetes (CNCF Graduated)',
    }
    return name_map.get(fname, fname)

fixed = 0
for subdir in ['entities', 'references']:
    for fpath in glob.glob(os.path.join(VAULT, subdir, '*.md')):
        rel = os.path.relpath(fpath, VAULT)
        content = read_file(rel)
        fname = os.path.basename(fpath).replace('.md', '')
        changed = False
        
        # Get current title
        tm = re.search(r'^title: "(.+?)"$', content, re.MULTILINE)
        if not tm:
            continue
        old_title = tm.group(1)
        
        # Get summary
        sm = re.search(r'^summary: "(.+?)"$', content, re.MULTILINE)
        summary = sm.group(1) if sm else ''
        
        # Fix bad summary for promql
        if fname == '02-prometheus-promql-advanced':
            new_summary = 'PromQL 高级查询技巧，涵盖子查询、聚合函数、向量匹配、多步计算等高级用法，帮助构建复杂的监控告警规则。'
            content = content.replace(f'summary: "{summary}"', f'summary: "{new_summary}"')
            changed = True
            summary = new_summary
        
        # Fix generic titles (title == filename)
        if old_title == fname:
            new_title = get_title_from_summary(summary, fname)
            if new_title != old_title:
                content = content.replace(f'title: "{old_title}"', f'title: "{new_title}"')
                content = content.replace(f'# {old_title}', f'# {new_title}')
                changed = True
        
        if changed:
            write_file(rel, content)
            fixed += 1

print(f"Fixed {fixed} files")

# Also check references for bad titles
for fpath in glob.glob(os.path.join(VAULT, 'references', '*.md')):
    rel = os.path.relpath(fpath, VAULT)
    content = read_file(rel)
    fname = os.path.basename(fpath).replace('.md', '')
    tm = re.search(r'^title: "(.+?)"$', content, re.MULTILINE)
    if tm and tm.group(1) == fname:
        # These should be fine for references (e.g. aws-eks-overview)
        pass
