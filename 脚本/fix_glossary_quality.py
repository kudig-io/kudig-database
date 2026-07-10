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
    '系统基础/topic-dictionary',
    '故障诊断/topic-fta/glossary',
]

# ── 按 category 分组的 Related 链接池 ──────────────────────
RELATED_POOL = {
    'fundamentals': [
        '系统基础/topic-dictionary/fundamentals/pod',
        '系统基础/topic-dictionary/fundamentals/container',
        '系统基础/topic-dictionary/fundamentals/node',
        '系统基础/topic-dictionary/fundamentals/namespace',
        '系统基础/topic-dictionary/fundamentals/cluster',
        '系统基础/topic-dictionary/fundamentals/control-plane',
        '系统基础/topic-dictionary/fundamentals/kubelet',
        '系统基础/topic-dictionary/fundamentals/kube-apiserver',
        '系统基础/topic-dictionary/fundamentals/kube-scheduler',
        '系统基础/topic-dictionary/fundamentals/controller-manager',
        '系统基础/topic-dictionary/fundamentals/etcd',
        '系统基础/topic-dictionary/fundamentals/worker-node',
        '系统基础/topic-dictionary/fundamentals/master-node',
    ],
    'workloads': [
        '系统基础/topic-dictionary/workloads/pod',
        '系统基础/topic-dictionary/workloads/deployment',
        '系统基础/topic-dictionary/workloads/statefulset',
        '系统基础/topic-dictionary/workloads/daemonset',
        '系统基础/topic-dictionary/workloads/replicaset',
        '系统基础/topic-dictionary/workloads/job',
        '系统基础/topic-dictionary/workloads/cronjob',
        '系统基础/topic-dictionary/workloads/workload',
    ],
    'networking': [
        '系统基础/topic-dictionary/networking/service',
        '系统基础/topic-dictionary/networking/ingress',
        '系统基础/topic-dictionary/networking/clusterip',
        '系统基础/topic-dictionary/networking/nodeport',
        '系统基础/topic-dictionary/networking/loadbalancer',
        '系统基础/topic-dictionary/networking/headless-service',
        '系统基础/topic-dictionary/networking/externalname',
        '系统基础/topic-dictionary/networking/endpoint',
        '系统基础/topic-dictionary/networking/networkpolicy',
        '系统基础/topic-dictionary/networking/cni',
        '系统基础/topic-dictionary/networking/coredns',
    ],
    'storage': [
        '系统基础/topic-dictionary/storage/persistent-volume',
        '系统基础/topic-dictionary/storage/persistent-volume-claim',
        '系统基础/topic-dictionary/storage/storage-class',
        '系统基础/topic-dictionary/storage/volume',
        '系统基础/topic-dictionary/storage/emptydir',
        '系统基础/topic-dictionary/storage/hostpath',
        '系统基础/topic-dictionary/storage/configmap',
        '系统基础/topic-dictionary/storage/secret',
        '系统基础/topic-dictionary/storage/csi',
    ],
    'scheduling': [
        '系统基础/topic-dictionary/scheduling/affinity',
        '系统基础/topic-dictionary/scheduling/anti-affinity',
        '系统基础/topic-dictionary/scheduling/taint',
        '系统基础/topic-dictionary/scheduling/toleration',
        '系统基础/topic-dictionary/scheduling/node-selector',
        '系统基础/topic-dictionary/scheduling/resource-request',
        '系统基础/topic-dictionary/scheduling/resource-limit',
        '系统基础/topic-dictionary/scheduling/hpa',
        '系统基础/topic-dictionary/scheduling/vpa',
        '系统基础/topic-dictionary/scheduling/qos',
        '系统基础/topic-dictionary/scheduling/topology',
    ],
    'configuration': [
        '系统基础/topic-dictionary/configuration/configmap',
        '系统基础/topic-dictionary/configuration/secret',
        '系统基础/topic-dictionary/configuration/env',
        '系统基础/topic-dictionary/configuration/configmaps',
        '系统基础/topic-dictionary/configuration/probe',
        '系统基础/topic-dictionary/configuration/liveness-probe',
        '系统基础/topic-dictionary/configuration/readiness-probe',
        '系统基础/topic-dictionary/configuration/startup-probe',
        '系统基础/topic-dictionary/configuration/graceful-shutdown',
    ],
    'security': [
        '系统基础/topic-dictionary/security/rbac',
        '系统基础/topic-dictionary/security/role',
        '系统基础/topic-dictionary/security/clusterrole',
        '系统基础/topic-dictionary/security/rolebinding',
        '系统基础/topic-dictionary/security/clusterrolebinding',
        '系统基础/topic-dictionary/security/service-account',
        '系统基础/topic-dictionary/security/service-account-token',
        '系统基础/topic-dictionary/security/security-context',
        '系统基础/topic-dictionary/security/network-policy',
        '系统基础/topic-dictionary/security/certificate',
    ],
    'observability': [
        '系统基础/topic-dictionary/observability/prometheus',
        '系统基础/topic-dictionary/observability/grafana',
        '系统基础/topic-dictionary/observability/alertmanager',
        '系统基础/topic-dictionary/observability/metrics-server',
        '系统基础/topic-dictionary/observability/kubernetes-events',
        '系统基础/topic-dictionary/observability/logging',
    ],
    'operations': [
        '系统基础/topic-dictionary/operations/kubectl',
        '系统基础/topic-dictionary/operations/helm',
        '系统基础/topic-dictionary/operations/kustomize',
        '系统基础/topic-dictionary/operations/cordon',
        '系统基础/topic-dictionary/operations/uncordon',
        '系统基础/topic-dictionary/operations/drain',
        '系统基础/topic-dictionary/operations/scale',
        '系统基础/topic-dictionary/operations/rolling-update',
        '系统基础/topic-dictionary/operations/rollback',
    ],
    'platform-engineering': [
        '系统基础/topic-dictionary/platform-engineering/api-group',
        '系统基础/topic-dictionary/platform-engineering/api-version',
        '系统基础/topic-dictionary/platform-engineering/kind',
        '系统基础/topic-dictionary/platform-engineering/manifest',
        '系统基础/topic-dictionary/platform-engineering/custom-resource',
        '系统基础/topic-dictionary/platform-engineering/operator-pattern',
        '系统基础/topic-dictionary/platform-engineering/server-side-apply',
    ],
    'tooling': [
        '系统基础/topic-dictionary/tooling/kubectl',
        '系统基础/topic-dictionary/tooling/kubeadm',
        '系统基础/topic-dictionary/tooling/kubectx',
        '系统基础/topic-dictionary/tooling/kubens',
        '系统基础/topic-dictionary/tooling/k9s',
        '系统基础/topic-dictionary/tooling/stern',
        '系统基础/topic-dictionary/tooling/etcdctl',
        '系统基础/topic-dictionary/tooling/helm',
    ],
    'fta': [
        '故障诊断/topic-fta/glossary/fault-tree-analysis',
        '故障诊断/topic-fta/glossary/top-event',
        '故障诊断/topic-fta/glossary/basic-event',
        '故障诊断/topic-fta/glossary/or-gate',
        '故障诊断/topic-fta/glossary/and-gate',
        '故障诊断/topic-fta/glossary/minimal-cut-set',
        '故障诊断/topic-fta/glossary/mtbf',
        '故障诊断/topic-fta/glossary/mttr',
        '故障诊断/topic-fta/glossary/availability',
        '故障诊断/topic-fta/glossary/fmea',
        '故障诊断/topic-fta/appendix-a-glossary',
    ],
}

# ── 通用 Related 链接（跨分类） ──────────────────────
GENERIC_RELATED = [
    '系统基础/topic-dictionary/k8s-glossary',
    '故障诊断/topic-fta/appendix-a-glossary',
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
