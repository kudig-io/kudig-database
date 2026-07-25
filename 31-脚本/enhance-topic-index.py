#!/usr/bin/env python3
"""
增强 topic-index 目录下的所有索引文件
添加 YAML frontmatter 元数据，支持深度研究语料库入口

使用方法:
    python3 enhance-topic-index.py           # 增强所有索引
    python3 enhance-topic-index.py --dry-run # 仅预览不写入
"""

import os
import re
import json
from pathlib import Path
from typing import Dict, List, Optional

# 元数据映射表
INDEX_METADATA = {
    "node-index.md": {
        "keyword": "node",
        "category": "TC-INFRA-NODE",
        "related_skills": ["SKILL-NODE-001"],
        "fta_codes": ["FTA-NODE-023"],
        "severity_hint": "P0-P1",
        "description": "节点生命周期、状态、kubelet、容器运行时相关问题"
    },
    "pod-index.md": {
        "keyword": "pod",
        "category": "TC-APP-POD",
        "related_skills": ["SKILL-POD-001", "SKILL-POD-002", "SKILL-IMAGE-001"],
        "fta_codes": ["FTA-POD-001"],
        "severity_hint": "P1-P2",
        "description": "Pod 创建、调度、运行、终止相关问题"
    },
    "network-index.md": {
        "keyword": "network",
        "category": "TC-INFRA-NET",
        "related_skills": ["SKILL-NET-001", "SKILL-NET-002", "SKILL-NET-003", "SKILL-NET-004"],
        "fta_codes": ["FTA-NET-001"],
        "severity_hint": "P1-P2",
        "description": "CNI、DNS、Service、Ingress、NetworkPolicy 相关问题"
    },
    "storage-index.md": {
        "keyword": "storage",
        "category": "TC-INFRA-STORE",
        "related_skills": ["SKILL-STORE-001"],
        "fta_codes": ["FTA-STORE-001"],
        "severity_hint": "P1-P2",
        "description": "PV/PVC、CSI、StorageClass、卷挂载相关问题"
    },
    "cert-index.md": {
        "keyword": "certificate",
        "category": "TC-SEC-CERT",
        "related_skills": ["SKILL-SEC-001"],
        "fta_codes": ["FTA-CERT-001"],
        "severity_hint": "P0",
        "description": "证书过期、CSR、TLS 握手相关问题"
    },
    "security-index.md": {
        "keyword": "security",
        "category": "TC-SEC-RBAC",
        "related_skills": ["SKILL-SEC-002", "SKILL-SECURITY-001"],
        "fta_codes": ["FTA-SEC-001"],
        "severity_hint": "P0-P1",
        "description": "RBAC、PSP、PSA、准入控制相关问题"
    },
    "scheduler-index.md": {
        "keyword": "scheduler",
        "category": "TC-INFRA-SCALE",
        "related_skills": ["SKILL-SCALE-001"],
        "fta_codes": ["FTA-SCHEDULER-001"],
        "severity_hint": "P1-P2",
        "description": "调度失败、亲和性、污点、拓扑约束相关问题"
    },
    "etcd-index.md": {
        "keyword": "etcd",
        "category": "TC-INFRA-CP",
        "related_skills": ["SKILL-CP-001"],
        "fta_codes": ["FTA-ETCD-001"],
        "severity_hint": "P0",
        "description": "etcd 存储、空间配额、选主、备份恢复相关问题"
    },
    "dns-index.md": {
        "keyword": "dns",
        "category": "TC-INFRA-NET",
        "related_skills": ["SKILL-NET-001"],
        "fta_codes": ["FTA-DNS-001"],
        "severity_hint": "P1",
        "description": "CoreDNS、域名解析、服务发现相关问题"
    },
    "cluster-index.md": {
        "keyword": "cluster",
        "category": "TC-INFRA-CP",
        "related_skills": ["SKILL-CP-001", "SKILL-NODE-001"],
        "fta_codes": ["FTA-CLUSTER-001"],
        "severity_hint": "P0",
        "description": "集群整体可用性、升级、高可用相关问题"
    },
    "pvc-index.md": {
        "keyword": "pvc",
        "category": "TC-INFRA-STORE",
        "related_skills": ["SKILL-STORE-001"],
        "fta_codes": ["FTA-PVC-001"],
        "severity_hint": "P1",
        "description": "PVC 绑定、存储供给、卷扩容相关问题"
    },
    "observability-index.md": {
        "keyword": "observability",
        "category": "TC-DATA-OBS",
        "related_skills": [],
        "fta_codes": ["FTA-OBS-001"],
        "severity_hint": "P2",
        "description": "监控、告警、日志、可视化相关问题"
    },
    "service-mesh-index.md": {
        "keyword": "service-mesh",
        "category": "TC-APP-SVC",
        "related_skills": [],
        "fta_codes": ["FTA-MESH-001"],
        "severity_hint": "P1-P2",
        "description": "Istio、Envoy sidecar、mTLS、流量管理相关问题"
    },
    "gitops-cicd-index.md": {
        "keyword": "gitops",
        "category": "TC-APP-WORKLOAD",
        "related_skills": [],
        "fta_codes": [],
        "severity_hint": "P2-P3",
        "description": "Argo CD、Flux、Jenkins、GitHub Actions 相关问题"
    },
    "backup-dr-index.md": {
        "keyword": "backup",
        "category": "TC-DATA-BACKUP",
        "related_skills": [],
        "fta_codes": ["FTA-BACKUP-001"],
        "severity_hint": "P1-P2",
        "description": "Velero 备份、快照、灾难恢复相关问题"
    },
    "ai-gpu-index.md": {
        "keyword": "ai-gpu",
        "category": "TC-DATA-AI",
        "related_skills": [],
        "fta_codes": [],
        "severity_hint": "P1-P2",
        "description": "GPU 调度、CUDA、模型训练、AI 工作负载相关问题"
    },
    "terway-index.md": {
        "keyword": "terway",
        "category": "TC-INFRA-NET",
        "related_skills": [],
        "fta_codes": ["FTA-TERWAY-001"],
        "severity_hint": "P1-P2",
        "description": "阿里云 Terway ENI、IPVLAN、网络模式相关问题"
    }
}

# Category 到 Intent Corpus 的映射
CATEGORY_TO_INTENT = {
    "TC-INFRA-NODE": "TC-INFRA-NODE",
    "TC-APP-POD": "TC-APP-POD",
    "TC-INFRA-NET": "TC-INFRA-NET",
    "TC-INFRA-STORE": "TC-INFRA-STORE",
    "TC-SEC-CERT": "TC-SEC-CERT",
    "TC-SEC-RBAC": "TC-SEC-RBAC",
    "TC-INFRA-SCALE": "TC-INFRA-SCALE",
    "TC-INFRA-CP": "TC-INFRA-CP",
    "TC-APP-SVC": "TC-APP-SVC",
    "TC-DATA-DB": "TC-DATA-DB",
    "TC-DATA-CACHE": "TC-DATA-CACHE",
    "TC-DATA-MQ": "TC-DATA-MQ",
}

def generate_frontmatter(metadata: Dict) -> str:
    """生成 YAML frontmatter"""
    lines = ["---"]
    lines.append("index_metadata:")
    lines.append(f"  keyword: \"{metadata['keyword']}\"")
    lines.append(f"  category: \"{metadata['category']}\"")

    # related_skills
    if metadata.get("related_skills"):
        lines.append("  related_skills:")
        for skill in metadata["related_skills"]:
            lines.append(f"    - \"{skill}\"")
    else:
        lines.append("  related_skills: []")

    # fta_codes
    if metadata.get("fta_codes"):
        lines.append("  fta_codes:")
        for fta in metadata["fta_codes"]:
            lines.append(f"    - \"{fta}\"")
    else:
        lines.append("  fta_codes: []")

    lines.append(f"  severity_hint: \"{metadata['severity_hint']}\"")
    lines.append(f"  description: \"{metadata['description']}\"")

    # 深度研究字段
    lines.append("")
    lines.append("deep_research:")
    lines.append("  intent_corpus: \"../P0-1-intent-corpus-expanded.jsonl\"")
    lines.append("  tool_schema: \"../P0-Tool-Schema-Definition.md\"")
    lines.append("  knowledge_graph: \"../P0-Knowledge-Graph-RDF-Model.md\"")

    # 搜索相关性
    lines.append("")
    lines.append("search_tags:")
    # 添加同义词和变体
    keyword = metadata["keyword"]
    tags = [
        keyword,
        keyword.replace("-", " "),
        keyword.upper(),
        keyword.lower(),
    ]
    if keyword == "network":
        tags.extend(["networking", "cni", "dns", "service", "ingress"])
    elif keyword == "pod":
        tags.extend(["container", "workload", "deployment", "statefulset"])
    elif keyword == "node":
        tags.extend(["kubelet", "runtime", "containerd"])
    elif keyword == "storage":
        tags.extend(["pvc", "pv", "csi", "volume"])
    elif keyword == "certificate":
        tags.extend(["tls", "csr", "x509", "cert-manager"])
    elif keyword == "security":
        tags.extend(["rbac", "psp", "psa", "admission"])

    for tag in tags:
        lines.append(f"    - \"{tag}\"")

    lines.append("---")
    lines.append("")

    return "\n".join(lines)

def enhance_index_file(filepath: Path, dry_run: bool = False) -> Dict:
    """增强单个索引文件"""
    filename = filepath.name

    if filename not in INDEX_METADATA:
        print(f"⚠️  未知索引文件: {filename}, 跳过")
        return {"status": "skipped", "reason": "unknown"}

    metadata = INDEX_METADATA[filename]

    # 读取原文件
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # 检查是否已有 frontmatter
    if content.startswith("---"):
        frontmatter_end = content.find("---", 3)
        if frontmatter_end != -1 and frontmatter_end < 200:
            print(f"⏭️  已包含 frontmatter: {filename}")
            return {"status": "skipped", "reason": "already_enhanced"}

    # 生成新内容
    new_frontmatter = generate_frontmatter(metadata)
    new_content = new_frontmatter + content

    if dry_run:
        print(f"\n📝 预览增强: {filename}")
        print(new_frontmatter[:500] + "...")
        return {"status": "dry_run", "filename": filename}

    # 写入文件
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(new_content)

    print(f"✅ 增强完成: {filename}")
    return {"status": "success", "filename": filename}

def main():
    import argparse
    parser = argparse.ArgumentParser(description="增强 topic-index 索引文件")
    parser.add_argument("--dry-run", action="store_true", help="仅预览不写入")
    parser.add_argument("--file", help="仅处理指定文件")
    args = parser.parse_args()

    index_dir = Path("topic-index")
    if not index_dir.exists():
        print("❌ topic-index 目录不存在")
        return

    results = []

    if args.file:
        # 仅处理指定文件
        filepath = index_dir / args.file
        if filepath.exists():
            results.append(enhance_index_file(filepath, args.dry_run))
        else:
            print(f"❌ 文件不存在: {filepath}")
    else:
        # 处理所有索引文件
        for filepath in sorted(index_dir.glob("*-index.md")):
            results.append(enhance_index_file(filepath, args.dry_run))

    # 生成统计
    success = sum(1 for r in results if r["status"] == "success")
    skipped = sum(1 for r in results if r["status"] == "skipped")
    dry_run = sum(1 for r in results if r["status"] == "dry_run")

    print(f"\n📊 统计: 成功 {success}, 跳过 {skipped}, 预览 {dry_run}")

if __name__ == "__main__":
    main()