#!/usr/bin/env python3
"""Fix CNCF category misclassifications and minor issues"""
import os, re, glob

VAULT = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"

# Correct category mappings (project_name -> correct_category)
CATEGORY_FIXES = {
    'metallb': 'Networking',
    'k3s': 'Runtime',
    'k0s': 'Runtime',
    'telepresence': 'Networking',
    'kubewarden': 'Policy',
    'kubearmor': 'Security',
    'paralus': 'Security',
    'containerssh': 'Security',
    'oauth2-proxy': 'Security',
    'bank-vaults': 'Security',
    'athenz': 'Security',
    'hexa': 'Security',
    'inclavare-containers': 'Security',
    'confidential-containers': 'Security',
    'open-policy-containers': 'Policy',
    'capsule': 'Policy',
    'koordinator': 'Orchestration',
    'kubefleet': 'Orchestration',
    'kubestellar': 'Orchestration',
    'clusternet': 'Orchestration',
    'clusterpedia': 'Orchestration',
    'open-cluster-management': 'Orchestration',
    'kudo': 'Orchestration',
    'score': 'Orchestration',
    'cozystack': 'Platform',
    'headlamp': 'Platform',
    'kubeclipper': 'Platform',
    'kusionstack': 'Platform',
    'openchoreo': 'Platform',
    'backstage': 'Platform',
    'kagent': 'Platform',
    'holmesgpt': 'Platform',
    'k8sgpt': 'Platform',
    'radius': 'Platform',
    'opengitops': 'Platform',
    'schemahero': 'Database',
    'cloudnativepg': 'Database',
    'opengemini': 'Database',
    'oxia': 'Database',
    'cadence': 'Streaming',
    'tremor': 'Streaming',
    'drasi': 'Streaming',
    'vineyard': 'Data',
    'serverless-workflow': 'Serverless',
    'serverless-devs': 'Serverless',
    'slimfaas': 'Serverless',
    'openfunction': 'Serverless',
    'kaito': 'AI/ML',
    'kubeflow': 'AI/ML',
    'kepler': 'Cost',
    'opencost': 'Cost',
    'devfile': 'CI/CD',
    'devspace': 'CI/CD',
    'werf': 'CI/CD',
    'atlantis': 'CI/CD',
    'carvel': 'CI/CD',
    'dalec': 'CI/CD',
    'kitops': 'Image',
    'copa': 'Image',
    'eraser': 'Image',
    'slimtoolkit': 'Image',
    'modelpack': 'Image',
    'container2wasm': 'Image',
    'stacker': 'Image',
    'oras': 'Image',
    'zot': 'Image',
    'xregistry': 'Image',
    'ratify': 'Supply Chain',
    'in-toto': 'Supply Chain',
    'tuf': 'Supply Chain',
    'notary-project': 'Supply Chain',
    'artifact-hub': 'Supply Chain',
    'openfeature': 'Supply Chain',
    'sops': 'Supply Chain',
    'external-secrets': 'Supply Chain',
    'porter': 'Config',
    'kpt': 'Config',
    'kcl': 'Config',
    'cdk8s': 'Config',
    'opentofu': 'Config',
    'submariner': 'Networking',
    'kube-vip': 'Networking',
    'spiderpool': 'Networking',
    'k8gb': 'Networking',
    'easegress': 'Networking',
    'loxilb': 'Networking',
    'bfe': 'Networking',
    'aeraki-mesh': 'Networking',
    'kmesh': 'Networking',
    'meshery': 'Networking',
    'sermant': 'Service Mesh',
    'kuma': 'Service Mesh',
    'kubefleet': 'Orchestration',
    'kairos': 'Edge',
    'akri': 'Edge',
    'interlink': 'Edge',
    'flatcar': 'Runtime',
    'bootc': 'Runtime',
    'composefs': 'Runtime',
    'urunc': 'Runtime',
    'hyperlight': 'Runtime',
    'kuasar': 'Runtime',
    'container2wasm': 'Runtime',
    'wasmedge': 'Runtime',
    'spin': 'Runtime',
    'spinkube': 'Runtime',
    'virtual-kubelet': 'Runtime',
    'podman-desktop': 'Runtime',
    'podman-container-tools': 'Runtime',
    'rook': 'Storage',
    'longhorn': 'Storage',
    'openebs': 'Storage',
    'piraeus-datastore': 'Storage',
    'carina': 'Storage',
    'hwameistor': 'Storage',
    'k8up': 'Storage',
    'kanister': 'Storage',
    'openyurt': 'Edge',
    'kubeedge': 'Edge',
    'tinkerbell': 'Metal',
    'metal3-io': 'Metal',
    'strimzi': 'Streaming',
    'nats': 'Streaming',
    'wasmcloud': 'Runtime',
    'lima': 'Runtime',
    'runme-notebooks': 'Platform',
    'keylime': 'Security',
    'parsec': 'Security',
    'cartography': 'Security',
    'cloud-custodian': 'Policy',
    'oscal-compass': 'Security',
    'pipecd': 'CI/CD',
    'shipwright': 'CI/CD',
    'konveyor': 'CI/CD',
    'kured': 'Orchestration',
    'kuberhealthy': 'Observability',
    'perses': 'Observability',
    'trickster': 'Observability',
    'pixie': 'Observability',
    'logging-operator': 'Observability',
    'kube-burner': 'Orchestration',
    'krkn': 'Chaos',
    'chaosblade': 'Chaos',
    'tokenetes': 'Security',
    'vscode-kubernetes-tools': 'Platform',
    'kube-rs': 'Platform',
    'connect-rpc': 'Networking',
    'openkruise': 'Orchestration',
    'kubevela': 'Orchestration',
    'karmada': 'Orchestration',
    'volcano': 'Orchestration',
    'keda': 'Orchestration',
    'knative': 'Orchestration',
    'operator-framework': 'Orchestration',
    'crossplane': 'Orchestration',
    'dapr': 'Orchestration',
    'kubeslice': 'Networking',
    'kube-ovn': 'Networking',
    'ovn-kubernetes': 'Networking',
    'network-service-mesh': 'Networking',
    'antrea': 'Networking',
    'inspektor-gadget': 'Observability',
    'kuadrant': 'Networking',
    'emissary-ingress': 'Networking',
    'contour': 'Networking',
    'kgateway': 'Networking',
    'hamei': 'Storage',
    'openebs': 'Storage',
}

fixed = 0
for fpath in glob.glob(os.path.join(VAULT, 'entities', '*.md')):
    rel = os.path.relpath(fpath, VAULT)
    content = open(fpath, 'r', encoding='utf-8').read()
    fname = os.path.basename(fpath).replace('.md', '')
    
    if fname not in CATEGORY_FIXES:
        continue
    
    correct_cat = CATEGORY_FIXES[fname]
    
    # Find current category line in the > **CNCF 状态** line
    old_match = re.search(r'> \*\*CNCF 状态\*\*: (\w+) \| \*\*类别\*\*: (\w[\w/]*)', content)
    if old_match and old_match.group(2) != correct_cat:
        old_line = old_match.group(0)
        new_line = old_line.replace(f'**类别**: {old_match.group(2)}', f'**类别**: {correct_cat}')
        content = content.replace(old_line, new_line)
        
        # Also update the 架构定位 section
        old_arch = f'属于 **{old_match.group(2)}** 类别'
        new_arch = f'属于 **{correct_cat}** 类别'
        content = content.replace(old_arch, new_arch)
        
        # Also update tags if needed
        old_tag = old_match.group(2).lower().replace('/', '-')
        new_tag = correct_cat.lower().replace('/', '-').replace(' ', '-')
        if old_tag in content and old_tag != new_tag:
            content = content.replace(f'tags: [k8s, cncf, {old_tag}, {fname}]',
                                       f'tags: [k8s, cncf, {new_tag}, {fname}]')
        
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(content)
        fixed += 1

print(f"Fixed {fixed} category classifications")
