---
title: 5 分钟快速上手指南
description: '# 5 分钟快速上手指南'
summary: '# 5 分钟快速上手指南'
category: general
tags:
- k8s
- docker
- llm
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 5 分钟快速上手指南 是什么
- 如何 5 分钟快速上手指南
trigger_keywords:
- 分钟快速上手指南
prerequisites:
- kubectl-basics
---



# 5 分钟快速上手指南

> **目标**: 让开发者在 5 分钟内完成接入, 并验证 Agent 能正常回答 [[entities/kubernetes.md|k8s]] 问题

---

## 前置条件

- Python 3.10+
- Git
- 至少一个向量数据库 (推荐: Chroma / Milvus / Qdrant)
- (可选) LLM API Key (OpenAI / Claude / 本地模型)

---

## 第一步: 克隆仓库 (1 分钟)

```bash
# 克隆仓库
git clone https://github.com/kudig-io/kudig-database.git
cd kudig-database

# 安装依赖
pip install -r requirements.txt

# 验证
python -c "import kudig; print(f'kudig-database v{kudig.__version__} loaded')"
# 预期输出: kudig-database v1.0.0 loaded
```

### 常见问题

- **git clone 超时**: 使用镜像 `git clone https://gitee.com/kudig-io/kudig-database.git`
- **pip 安装失败**: 升级 pip `pip install --upgrade pip`, 或使用 `pip install -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple/`
- **Python 版本不兼容**: 使用 pyenv 安装 3.10+ `pyenv install 3.11 && pyenv local 3.11`

---

## 第二步: 选择 Profile (1 分钟)

```bash
# 查看可用角色
ls profiles/
# 输出:
# devops-engineer/    — DevOps 工程师, 偏 CI/CD 和自动化
# sre/                — SRE, 偏可靠性和监控
# platform-engineer/  — 平台工程师, 偏基础设施
# cloud-architect/    — 云架构师, 偏多云和架构设计
# k8s-beginner/       — K8s 初学者, 偏基础概念

# 选择你的角色 (以 devops-engineer 为例)
export KUDIG_PROFILE=devops-engineer

# 查看该角色的知识集
python scripts/list_knowledge.py --profile $KUDIG_PROFILE
# 输出: 1,247 documents across 28 domains
```

### 如何选择?

| 你的情况 | 推荐 Profile |
|----------|-------------|
| 负责 CI/CD 流水线和部署自动化 | devops-engineer |
| 负责监控、告警和问题响应 | sre |
| 负责 K8s 集群搭建和维护 | platform-engineer |
| 负责云架构设计和技术选型 | cloud-architect |
| 刚开始学 K8s | k8s-beginner |

---

## 第三步: 导入 RAG (3 分钟)

### 方式 A: 使用内置 Chroma (推荐新手)

```bash
# 一键导入到本地 Chroma 向量数据库
python scripts/import_rag.py \
  --profile $KUDIG_PROFILE \
  --target chroma \
  --output ./vector-db

# 预期输出:
# [1/5] Loading knowledge base... 1,247 documents
# [2/5] Chunking documents... 8,432 chunks
# [3/5] Generating embeddings... (ETA: 2min)
# [4/5] Writing to Chroma... ./vector-db/
# [5/5] Done! 8,432 vectors indexed
```

### 方式 B: 导入到 Milvus

```bash
python scripts/import_rag.py \
  --profile $KUDIG_PROFILE \
  --target milvus \
  --milvus-uri http://localhost:19530 \
  --milvus-collection kudig_knowledge
```

### 方式 C: 导入到 Qdrant

```bash
python scripts/import_rag.py \
  --profile $KUDIG_PROFILE \
  --target qdrant \
  --qdrant-uri http://localhost:6333 \
  --qdrant-collection kudig_knowledge
```

### 方式 D: 导出为 LangChain 格式

```bash
python scripts/export_langchain.py \
  --profile $KUDIG_PROFILE \
  --output ./langchain-docs/

# 然后在你的 LangChain 项目中:
# from langchain.document_loaders import DirectoryLoader
# docs = DirectoryLoader("./langchain-docs/").load()
```

---

## 验证: 问 Agent 一个测试问题

```bash
# 启动本地 Agent 服务
python -m kudig.agent serve --port 8000

# 发送测试问题
curl -X POST http://localhost:8000/api/chat \
  -H "Content-Type: application/json" \
  -d '{
    "message": "Pod 一直处于 CrashLoopBackOff 状态, 怎么排查?"
  }'
```

### 预期响应

```json
{
  "answer": "CrashLoopBackOff 的标准排查流程如下:\n\n1. 查看 Pod 日志: kubectl logs <pod-name> --previous\n2. 检查资源限制: kubectl describe pod <pod-name>\n3. 检查 Liveness/Readiness Probe 配置\n4. 检查挂载卷和 ConfigMap/Secret\n...",
  "sources": [
    "diagnostic/crashloopbackoff.md",
    "knowledge/pod-lifecycle.md"
  ],
  "confidence": 0.92
}
```

### 验证清单

- [ ] 响应时间 < 3 秒
- [ ] answer 包含具体命令和步骤 (不是泛泛而谈)
- [ ] sources 引用了 kudig-database 中的文档
- [ ] confidence > 0.8

---

## 常见问题

### 导入失败

| 错误 | 原因 | 解决 |
|------|------|------|
| `ModuleNotFoundError: No module named 'chromadb'` | 缺少向量数据库依赖 | `pip install chromadb` |
| `FileNotFoundError: profiles/xxx not found` | Profile 名称错误 | `ls profiles/` 确认 |
| `MemoryError during embedding` | 内存不足 | 加 `--batch-size 100` 减小批次 |
| `ConnectionError: Milvus not reachable` | Milvus 未启动 | `docker-compose up -d milvus` |

### 检索不准

| 现象 | 原因 | 解决 |
|------|------|------|
| 返回不相关文档 | 切片太大或太小 | 调整 `--chunk-size 512` |
| 语义匹配差 | 嵌入模型不匹配 | 使用推荐模型 `--embedding-model bge-large-zh` |
| 重复文档 | 去重失败 | 加 `--dedup` 参数重新导入 |

### 响应慢

| 现象 | 原因 | 解决 |
|------|------|------|
| 检索慢 (>3s) | 向量库索引未优化 | 执行 `python scripts/optimize_index.py` |
| 生成慢 (>10s) | LLM 速度慢 | 切换更快的模型或使用 streaming |
| 首次请求慢 | 冷启动 | 预热: `curl http://localhost:8000/api/health` |
