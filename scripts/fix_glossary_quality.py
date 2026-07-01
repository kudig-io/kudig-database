#!/usr/bin/env python3
"""批量修复术语文档质量问题:
1. trigger_keywords 去重 (6 files)
2. 补充缺失的 ## 参考链接 段落 (29 files)
3. 填充空的 ## Related 段落 (69 files)
"""

import os, re, yaml
from pathlib import Path
from collections import defaultdict

BASE = Path(".")
SKIP = {'k8s-glossary.md','_index.md','index.md','MOC.md','GAP-ANALYSIS.md',
        'appendix-a-glossary.md','README.md','_MOC.md','_moc.md'}
SCAN_DIRS = [
    'domain-17-system-foundation/topic-dictionary',
    'domain-10-troubleshooting-diagnostics/topic-fta/glossary',
]

# ── 按 category 分组的 Related 链接池 ──────────────────────
RELATED_POOL = {
    'fundamentals': [
        'domain-17-system-foundation/topic-dictionary/fundamentals/pod',
        'domain-17-system-foundation/topic-dictionary/fundamentals/container',
        'domain-17-system-foundation/topic-dictionary/fundamentals/node',
        'domain-17-system-foundation/topic-dictionary/fundamentals/namespace',
        'domain-17-system-foundation/topic-dictionary/fundamentals/cluster',
        'domain-17-system-foundation/topic-dictionary/fundamentals/control-plane',
        'domain-17-system-foundation/topic-dictionary/fundamentals/kubelet',
        'domain-17-system-foundation/topic-dictionary/fundamentals/kube-apiserver',
        'domain-17-system-foundation/topic-dictionary/fundamentals/kube-scheduler',
        'domain-17-system-foundation/topic-dictionary/fundamentals/controller-manager',
        'domain-17-system-foundation/topic-dictionary/fundamentals/etcd',
        'domain-17-system-foundation/topic-dictionary/fundamentals/worker-node',
        'domain-17-system-foundation/topic-dictionary/fundamentals/master-node',
    ],
    'workloads': [
        'domain-17-system-foundation/topic-dictionary/workloads/pod',
        'domain-17-system-foundation/topic-dictionary/workloads/deployment',
        'domain-17-system-foundation/topic-dictionary/workloads/statefulset',
        'domain-17-system-foundation/topic-dictionary/workloads/daemonset',
        'domain-17-system-foundation/topic-dictionary/workloads/replicaset',
        'domain-17-system-foundation/topic-dictionary/workloads/job',
        'domain-17-system-foundation/topic-dictionary/workloads/cronjob',
        'domain-17-system-foundation/topic-dictionary/workloads/workload',
    ],
    'networking': [
        'domain-17-system-foundation/topic-dictionary/networking/service',
        'domain-17-system-foundation/topic-dictionary/networking/ingress',
        'domain-17-system-foundation/topic-dictionary/networking/clusterip',
        'domain-17-system-foundation/topic-dictionary/networking/nodeport',
        'domain-17-system-foundation/topic-dictionary/networking/loadbalancer',
        'domain-17-system-foundation/topic-dictionary/networking/headless-service',
        'domain-17-system-foundation/topic-dictionary/networking/externalname',
        'domain-17-system-foundation/topic-dictionary/networking/endpoint',
        'domain-17-system-foundation/topic-dictionary/networking/networkpolicy',
        'domain-17-system-foundation/topic-dictionary/networking/cni',
        'domain-17-system-foundation/topic-dictionary/networking/coredns',
    ],
    'storage': [
        'domain-17-system-foundation/topic-dictionary/storage/persistent-volume',
        'domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim',
        'domain-17-system-foundation/topic-dictionary/storage/storage-class',
        'domain-17-system-foundation/topic-dictionary/storage/volume',
        'domain-17-system-foundation/topic-dictionary/storage/emptydir',
        'domain-17-system-foundation/topic-dictionary/storage/hostpath',
        'domain-17-system-foundation/topic-dictionary/storage/configmap',
        'domain-17-system-foundation/topic-dictionary/storage/secret',
        'domain-17-system-foundation/topic-dictionary/storage/csi',
    ],
    'scheduling': [
        'domain-17-system-foundation/topic-dictionary/scheduling/affinity',
        'domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity',
        'domain-17-system-foundation/topic-dictionary/scheduling/taint',
        'domain-17-system-foundation/topic-dictionary/scheduling/toleration',
        'domain-17-system-foundation/topic-dictionary/scheduling/node-selector',
        'domain-17-system-foundation/topic-dictionary/scheduling/resource-request',
        'domain-17-system-foundation/topic-dictionary/scheduling/resource-limit',
        'domain-17-system-foundation/topic-dictionary/scheduling/hpa',
        'domain-17-system-foundation/topic-dictionary/scheduling/vpa',
        'domain-17-system-foundation/topic-dictionary/scheduling/qos',
        'domain-17-system-foundation/topic-dictionary/scheduling/topology',
    ],
    'configuration': [
        'domain-17-system-foundation/topic-dictionary/configuration/configmap',
        'domain-17-system-foundation/topic-dictionary/configuration/secret',
        'domain-17-system-foundation/topic-dictionary/configuration/env',
        'domain-17-system-foundation/topic-dictionary/configuration/configmaps',
        'domain-17-system-foundation/topic-dictionary/configuration/probe',
        'domain-17-system-foundation/topic-dictionary/configuration/liveness-probe',
        'domain-17-system-foundation/topic-dictionary/configuration/readiness-probe',
        'domain-17-system-foundation/topic-dictionary/configuration/startup-probe',
        'domain-17-system-foundation/topic-dictionary/configuration/graceful-shutdown',
    ],
    'security': [
        'domain-17-system-foundation/topic-dictionary/security/rbac',
        'domain-17-system-foundation/topic-dictionary/security/role',
        'domain-17-system-foundation/topic-dictionary/security/clusterrole',
        'domain-17-system-foundation/topic-dictionary/security/rolebinding',
        'domain-17-system-foundation/topic-dictionary/security/clusterrolebinding',
        'domain-17-system-foundation/topic-dictionary/security/service-account',
        'domain-17-system-foundation/topic-dictionary/security/service-account-token',
        'domain-17-system-foundation/topic-dictionary/security/security-context',
        'domain-17-system-foundation/topic-dictionary/security/network-policy',
        'domain-17-system-foundation/topic-dictionary/security/certificate',
    ],
    'observability': [
        'domain-17-system-foundation/topic-dictionary/observability/prometheus',
        'domain-17-system-foundation/topic-dictionary/observability/grafana',
        'domain-17-system-foundation/topic-dictionary/observability/alertmanager',
        'domain-17-system-foundation/topic-dictionary/observability/metrics-server',
        'domain-17-system-foundation/topic-dictionary/observability/kubernetes-events',
        'domain-17-system-foundation/topic-dictionary/observability/logging',
    ],
    'operations': [
        'domain-17-system-foundation/topic-dictionary/operations/kubectl',
        'domain-17-system-foundation/topic-dictionary/operations/helm',
        'domain-17-system-foundation/topic-dictionary/operations/kustomize',
        'domain-17-system-foundation/topic-dictionary/operations/cordon',
        'domain-17-system-foundation/topic-dictionary/operations/uncordon',
        'domain-17-system-foundation/topic-dictionary/operations/drain',
        'domain-17-system-foundation/topic-dictionary/operations/scale',
        'domain-17-system-foundation/topic-dictionary/operations/rolling-update',
        'domain-17-system-foundation/topic-dictionary/operations/rollback',
    ],
    'platform-engineering': [
        'domain-17-system-foundation/topic-dictionary/platform-engineering/api-group',
        'domain-17-system-foundation/topic-dictionary/platform-engineering/api-version',
        'domain-17-system-foundation/topic-dictionary/platform-engineering/kind',
        'domain-17-system-foundation/topic-dictionary/platform-engineering/manifest',
        'domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resource',
        'domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern',
        'domain-17-system-foundation/topic-dictionary/platform-engineering/server-side-apply',
    ],
    'tooling': [
        'domain-17-system-foundation/topic-dictionary/tooling/kubectl',
        'domain-17-system-foundation/topic-dictionary/tooling/kubeadm',
        'domain-17-system-foundation/topic-dictionary/tooling/kubectx',
        'domain-17-system-foundation/topic-dictionary/tooling/kubens',
        'domain-17-system-foundation/topic-dictionary/tooling/k9s',
        'domain-17-system-foundation/topic-dictionary/tooling/stern',
        'domain-17-system-foundation/topic-dictionary/tooling/etcdctl',
        'domain-17-system-foundation/topic-dictionary/tooling/helm',
    ],
    'fta': [
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/fault-tree-analysis',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/top-event',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/basic-event',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/or-gate',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/and-gate',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/minimal-cut-set',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/mtbf',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/mttr',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/availability',
        'domain-10-troubleshooting-diagnostics/topic-fta/glossary/fmea',
        'domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary',
    ],
}

# ── 通用 Related 链接（跨分类） ──────────────────────
GENERIC_RELATED = [
    'domain-17-system-foundation/topic-dictionary/k8s-glossary',
    'domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary',
]


def get_category(filepath):
    """从文件路径提取分类"""
    parts = Path(filepath).parts
    # domain-17: .../topic-dictionary/<category>/file.md
    if 'topic-dictionary' in parts:
        idx = parts.index('topic-dictionary')
        if idx + 1 < len(parts) - 1:
            return parts[idx + 1]
    # domain-10: .../glossary/file.md
    if 'glossary' in parts:
        return 'fta'
    return 'fundamentals'


def get_related_links(filepath, max_links=5):
    """为文件生成 Related 链接"""
    cat = get_category(filepath)
    pool = RELATED_POOL.get(cat, RELATED_POOL['fundamentals'])
    self_path = filepath.replace('.md', '').replace('./', '')

    # 从同分类池中选取，排除自身
    candidates = [p for p in pool if p != self_path and not self_path.endswith(Path(p).name)]

    # 如果候选不足，从其他分类补充
    if len(candidates) < max_links:
        for other_cat, other_pool in RELATED_POOL.items():
            if other_cat == cat:
                continue
            for p in other_pool:
                if p not in candidates and p != self_path:
                    candidates.append(p)
            if len(candidates) >= max_links:
                break

    selected = candidates[:max_links]

    # 生成 wikilink 格式
    lines = []
    for p in selected:
        name = Path(p).name.replace('-', ' ').title()
        lines.append(f"- [[{p}|{name}]]")
    return '\n'.join(lines)


def fix_trigger_keywords_dedup(filepath, text):
    """修复 trigger_keywords 中的重复条目"""
    if not text.startswith('---'):
        return text, False
    end = text.find('---', 3)
    if end < 0:
        return text, False

    fm_text = text[3:end].strip()
    try:
        meta = yaml.safe_load(fm_text)
    except:
        return text, False

    tks = meta.get('trigger_keywords', [])
    if not isinstance(tks, list) or len(tks) == len(set(tks)):
        return text, False

    # 去重保序
    seen = set()
    unique = []
    for tk in tks:
        if tk not in seen:
            seen.add(tk)
            unique.append(tk)

    # 在原始文本中替换 trigger_keywords 区块
    lines = text.split('\n')
    new_lines = []
    in_tk = False
    tk_done = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('trigger_keywords:'):
            in_tk = True
            new_lines.append(line)
            for kw in unique:
                new_lines.append(f"- {kw}")
            continue
        if in_tk:
            if stripped.startswith('- '):
                continue  # skip old entries
            else:
                in_tk = False
        new_lines.append(line)

    return '\n'.join(new_lines), True


def fix_missing_refs(filepath, text):
    """补充缺失的 ## 参考链接 段落"""
    if '## 参考链接' in text:
        return text, False

    title = Path(filepath).stem.replace('-', ' ').title()
    refs_section = f"\n## 参考链接\n\n- [{title}]()\n"

    # 在 ## Related 前插入，或追加到末尾
    if '## Related' in text:
        text = text.replace('\n## Related', refs_section + '\n## Related')
    else:
        text = text.rstrip() + '\n' + refs_section + '\n'

    return text, True


def fix_empty_related(filepath, text):
    """填充空的 ## Related 段落"""
    if '## Related' not in text:
        return text, False

    ridx = text.index('## Related')
    after = text[ridx + 10:].strip()
    if after and not after.startswith('## '):
        return text, False  # 已有内容

    related_content = get_related_links(filepath)
    if not related_content:
        return text, False

    # 替换空的 Related 段落
    old_section = text[ridx:]
    # 找到 Related 后到下一个 ## 或文件末尾
    rest_after_related = text[ridx + 10:]
    next_h2 = rest_after_related.find('\n## ')
    if next_h2 >= 0:
        trailing = rest_after_related[next_h2:]
    else:
        trailing = ''

    new_section = f"## Related\n\n{related_content}\n{trailing}"
    text = text[:ridx] + new_section

    return text, True


def main():
    stats = {'dedup': 0, 'refs': 0, 'related': 0, 'scanned': 0}

    for scan_dir in SCAN_DIRS:
        for root, dirs, files in os.walk(scan_dir):
            for fn in files:
                if not fn.endswith('.md') or fn in SKIP:
                    continue
                fp = os.path.join(root, fn)
                with open(fp) as f:
                    text = f.read()
                if not text.startswith('---'):
                    continue

                # 检查是否是术语文档
                end = text.find('---', 3)
                if end < 0:
                    continue
                try:
                    meta = yaml.safe_load(text[3:end].strip())
                except:
                    continue
                if not meta or 'trigger_keywords' not in meta:
                    continue

                stats['scanned'] += 1
                modified = False

                # Fix 1: trigger_keywords dedup
                text, changed = fix_trigger_keywords_dedup(fp, text)
                if changed:
                    stats['dedup'] += 1
                    modified = True

                # Fix 2: missing 参考链接
                text, changed = fix_missing_refs(fp, text)
                if changed:
                    stats['refs'] += 1
                    modified = True

                # Fix 3: empty Related
                text, changed = fix_empty_related(fp, text)
                if changed:
                    stats['related'] += 1
                    modified = True

                if modified:
                    with open(fp, 'w') as f:
                        f.write(text)

    print(f"扫描文件: {stats['scanned']}")
    print(f"修复 trigger_keywords 去重: {stats['dedup']}")
    print(f"补充 参考链接 段落: {stats['refs']}")
    print(f"填充 Related 段落: {stats['related']}")
    print(f"总计修复: {stats['dedup'] + stats['refs'] + stats['related']}")


if __name__ == '__main__':
    main()
