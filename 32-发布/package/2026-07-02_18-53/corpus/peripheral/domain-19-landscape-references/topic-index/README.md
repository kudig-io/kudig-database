---
title: topic-index 深度研究入口使用指南
description: '# topic-index 深度研究入口使用指南'
summary: '# topic-index 深度研究入口使用指南'
category: index
tags:
- k8s
- index
- catalog
- etcd
- kubelet
- scheduler
- istio
- envoy
- coredns
- flux
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- topic-index 深度研究入口使用指南 是什么
- 如何 topic-index 深度研究入口使用指南
trigger_keywords:
- topic-index
- 深度研究入口使用指南
- index
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- etcd-basics
- gpu-scheduling-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-index 深度研究入口使用指南

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 说明如何使用 topic-index 作为深度研究的语料库入口

---

## 一、架构概览

```
用户输入
    │
    ▼
┌─────────────────────────────────────────────────────────────────┐
│                    topic-index 深度研究入口                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐     │
│  │  向量索引     │     │  意图路由     │     │  FTA 联动    │     │
│  │              │     │              │     │              │     │
│  │ vector-index │────▶│ hybrid-search │────▶│ domain-10-troubleshooting-diagnostics/topic-fta/  │     │
│  │  .json       │     │  -meta.json   │     │  list/*.md   │     │
│  └──────────────┘     └──────────────┘     └──────────────┘     │
│         │                   │                   │               │
│         ▼                   ▼                   ▼               │
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐     │
│  │  快速定位     │     │  分类匹配     │     │  技能卡片     │     │
│  │  知识域       │     │  Category    │     │  Skills      │     │
│  └──────────────┘     └──────────────┘     └──────────────┘     │
│                                                                  │
│  增强的索引文件: domain-19-landscape-references/topic-index/*.md (17个)                          │
│  新增元数据: YAML frontmatter + search_tags                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 二、增强功能

### 2.1 元数据增强

每个索引文件新增 YAML frontmatter：

```yaml
---
index_metadata:
  keyword: "node"
  category: "TC-INFRA-NODE"
  related_skills:
    - "SKILL-NODE-001"
  fta_codes:
    - "FTA-NODE-023"
  severity_hint: "P0-P1"
  description: "节点生命周期、状态、kubelet、容器运行时相关问题"

deep_research:
  intent_corpus: "../P0-1-intent-corpus-expanded.jsonl"
  tool_schema: "../P0-Tool-Schema-Definition.md"
  knowledge_graph: "../P0-Knowledge-Graph-RDF-Model.md"

search_tags:
    - "node"
    - "kubelet"
    - "runtime"
    - "containerd"
---
```

### 2.2 向量索引

构建了 `domain-19-landscape-references/topic-index/vector-index.json`：

```json
{
  "node-index.md": {
    "keyword": "node",
    "category": "TC-INFRA-NODE",
    "severity_hint": "P0-P1",
    "description": "节点生命周期、状态、kubelet、容器运行时相关问题",
    "related_skills": ["SKILL-NODE-001"],
    "fta_codes": ["FTA-NODE-023"],
    "section_counts": {"troubleshooting": 12, "skills": 3, ...},
    "total_links": 150
  }
}
```

### 2.3 混合搜索元数据

构建了 `domain-19-landscape-references/topic-index/hybrid-search-meta.json`：

```json
{
  "keyword_to_category": {
    "NotReady": ["TC-INFRA-NODE"],
    "OOMKilled": ["TC-APP-POD"],
    "CrashLoopBackOff": ["TC-APP-POD"],
    ...
  },
  "category_to_keywords": {
    "TC-INFRA-NODE": ["NotReady", "Node", "kubelet", ...],
    "TC-APP-POD": ["OOMKilled", "CrashLoop", "Pending", ...]
  },
  "intent_corpus_stats": {
    "total_intents": 729,
    "categories": 18
  }
}
```

---

## 三、使用方法

### 3.1 快速定位知识域

```python
import json

# 加载向量索引
with open('domain-19-landscape-references/topic-index/vector-index.json') as f:
    index_data = json.load(f)

# 搜索
query = "node notready kubelet"
results = semantic_search(query, index_data, top_k=3)

for r in results:
    print(f"[{r['keyword']}] {r['category']} - {r['description']}")
```

**示例输出**：
```
[node] TC-INFRA-NODE - 节点生命周期、状态、kubelet、容器运行时相关问题
```

### 3.2 意图路由分类

```python
import json

# 加载混合搜索元数据
with open('domain-19-landscape-references/topic-index/hybrid-search-meta.json') as f:
    hybrid_meta = json.load(f)

# 意图路由
query = "节点NotReady，Pod被驱逐"
result = intent_route(query, hybrid_meta)

print(f"Category: {result['category']}")  # TC-INFRA-NODE
print(f"Confidence: {result['confidence']:.2f}")  # 0.85
```

### 3.3 FTA 联动检索

```python
# 根据 category 找到对应的 FTA
category_to_fta = {
    "TC-INFRA-NODE": "FTA-NODE-023",
    "TC-APP-POD": "FTA-POD-001",
    "TC-INFRA-NET": "FTA-NET-001",
}

# 加载 FTA
fta_code = category_to_fta.get(result['category'])
fta_path = f"domain-10-troubleshooting-diagnostics/topic-fta/list/{fta_code.lower().replace('_', '-')}.md"

# 读取 FTA 内容
with open(fta_path) as f:
    fta_content = f.read()
```

### 3.4 完整深度研究流程

```python
def deep_research(query: str):
    """
    完整深度研究流程
    """

    # 1. 意图路由
    hybrid_meta = load_hybrid_meta()
    intent = intent_route(query, hybrid_meta)

    # 2. 知识域定位
    index_data = load_vector_index()
    knowledge_domains = semantic_search(query, index_data, top_k=5)

    # 3. FTA 加载
    fta_codes = knowledge_domains[0]['fta_codes'] if knowledge_domains else []
    fta_trees = []
    for fta_code in fta_codes:
        fta_trees.append(load_fta(fta_code))

    # 4. Skills 加载
    skills = knowledge_domains[0].get('related_skills', [])
    skill_docs = [load_skill(skill_id) for skill_id in skills]

    # 5. 工具 Schema 加载
    tool_schema = load_tool_schema()

    return {
        "intent": intent,
        "knowledge_domains": knowledge_domains,
        "fta_trees": fta_trees,
        "skills": skill_docs,
        "tool_schema": tool_schema
    }
```

---

## 四、脚本工具

### 4.1 增强索引脚本

```bash
# 预览增强效果（不写入）
python3 scripts/enhance-topic-index.py --dry-run

# 增强所有索引
python3 scripts/enhance-topic-index.py

# 增强指定文件
python3 scripts/enhance-topic-index.py --file node-index.md
```

### 4.2 向量索引构建脚本

```bash
# 构建向量索引
python3 scripts/build-index-vector.py

# 测试搜索
python3 scripts/build-index-vector.py --search "node notready kubelet"

# 测试意图路由
python3 scripts/build-index-vector.py --route "节点NotReady"
```

---

## 五、索引文件清单

| 索引文件 | Category | 严重程度 | 描述 |
|----------|----------|----------|------|
| node-index.md | TC-INFRA-NODE | P0-P1 | 节点生命周期、状态、kubelet |
| pod-index.md | TC-APP-POD | P1-P2 | Pod 创建、调度、运行、终止 |
| network-index.md | TC-INFRA-NET | P1-P2 | CNI、DNS、[[domain-17-system-foundation/知识字典/networking/service.md|Service]]、[[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]] |
| storage-index.md | TC-INFRA-STORE | P1-P2 | PV/PVC、CSI、StorageClass |
| cert-index.md | TC-SEC-CERT | P0 | 证书过期、CSR、TLS |
| security-index.md | TC-SEC-RBAC | P0-P1 | RBAC、PSP、PSA |
| scheduler-index.md | TC-INFRA-SCALE | P1-P2 | 调度失败、亲和性、污点 |
| etcd-index.md | TC-INFRA-CP | P0 | [[entities/etcd.md|etcd]] 存储、空间、配额 |
| dns-index.md | TC-INFRA-NET | P1 | CoreDNS、域名解析 |
| cluster-index.md | TC-INFRA-CP | P0 | 集群整体、高可用 |
| pvc-index.md | TC-INFRA-STORE | P1 | PVC 绑定、存储供给 |
| observability-index.md | TC-DATA-OBS | P2 | 监控、告警、日志 |
| service-mesh-index.md | TC-APP-SVC | P1-P2 | Istio、Envoy、mTLS |
| gitops-cicd-index.md | TC-APP-WORKLOAD | P2-P3 | Argo CD、Flux、Jenkins |
| backup-dr-index.md | TC-DATA-BACKUP | P1-P2 | Velero、快照、灾难恢复 |
| ai-gpu-index.md | TC-DATA-AI | P1-P2 | GPU、CUDA、模型训练 |
| terway-index.md | TC-INFRA-NET | P1-P2 | 阿里云 Terway、ENI |

---

## 六、与其他模块的联动

```
topic-index
    │
    ├───▶ P0-1-intent-corpus-expanded.jsonl  (意图语料)
    │
    ├───▶ P0-Tool-Schema-Definition.md        (工具Schema)
    │
    ├───▶ P0-Knowledge-Graph-RDF-Model.md     (知识图谱)
    │
    ├───▶ domain-10-troubleshooting-diagnostics/topic-fta/list/*.md                  (故障树)
    │
    ├───▶ domain-10-troubleshooting-diagnostics/topic-skills/*.md                   (技能卡片)
    │
    └───▶ domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/  (结构化排查)
```

---

**下一步行动**: 将此入口文档添加到 README.md 的"AI 语料库场景"章节，实现与主 README 的联动。

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- KUDIG Knowledge Base Architecture — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Kubernetes 灾难恢复最佳实践 & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
