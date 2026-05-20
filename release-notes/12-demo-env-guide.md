# kudig-database Demo 环境搭建指南

> **用途**: 发布会现场演示环境搭建
> **目标**: 从零搭建一个可演示的 kudig-database + AI Agent 环境
> **预计搭建时间**: 4-6 小时

---

## 一、环境要求

### 硬件要求

| 组件 | 最低配置 | 推荐配置 |
|------|---------|---------|
| CPU | 8 核 | 16 核 |
| 内存 | 32 GB | 64 GB |
| 磁盘 | 100 GB SSD | 200 GB NVMe |
| 网络 | 100 Mbps | 1 Gbps |

### 软件要求

| 组件 | 版本要求 | 说明 |
|------|---------|------|
| Kubernetes | 1.28+ | 用于部署 Agent 和 RAG 系统 |
| kubectl | 与集群版本匹配 | 集群管理 |
| Helm | 3.12+ | 应用部署 |
| Python | 3.10+ | RAG 系统运行环境 |
| Docker | 24.0+ | 容器构建 |

### K8s 集群要求

```
最小集群规模:
- 3 个 Worker Node (每节点 8C32G)
- 1 个 Control Plane Node (4C8G)
- 支持 ReadWriteMany 的 StorageClass (如 NFS/CephFS)
- Ingress Controller (Nginx/Traefik)
- CoreDNS 正常运行
```

### 预装组件

```
- Vector Database: Milvus 2.3+ 或 Qdrant 0.8+
- LLM API: OpenAI 兼容接口 (GPT-4 / Claude / 本地模型)
- Embedding Model: text-embedding-3-small 或 BGE-large-zh
```

---

## 二、搭建步骤

### Step 1: 准备 K8s 集群 (30 分钟)

```bash
# 方案 A: 使用现有集群
kubectl cluster-info
kubectl get nodes

# 方案 B: 使用 kind 创建本地集群
cat <<EOF > kind-config.yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
  - role: control-plane
  - role: worker
  - role: worker
  - role: worker
EOF
kind create cluster --config kind-config.yaml --name kudig-demo

# 方案 C: 使用 k3d 创建轻量集群
k3d cluster create kudig-demo --agents 3 --memory 8192
```

### Step 2: 部署向量数据库 (20 分钟)

```bash
# 方案 A: Milvus (推荐)
helm repo add milvus https://milvus-io.github.io/milvus-helm/
helm install milvus milvus/milvus \
  --set cluster.enabled=false \
  --set etcd.replicaCount=1 \
  --set minio.mode=standalone \
  --set pulsar.enabled=false \
  -n kudig-system --create-namespace

# 方案 B: Qdrant (轻量替代)
helm repo add qdrant https://qdrant.github.io/qdrant-helm/
helm install qdrant qdrant/qdrant \
  --set replicaCount=1 \
  -n kudig-system --create-namespace
```

### Step 3: 导入知识库数据 (60 分钟)

```bash
# 1. 克隆知识库
git clone https://github.com/kudig-io/kudig-database.git
cd kudig-database

# 2. 安装 Python 依赖
pip install -r requirements.txt

# 3. 配置 Embedding 模型
export EMBEDDING_MODEL="text-embedding-3-small"
export EMBEDDING_API_KEY="your-api-key"

# 4. 配置向量数据库连接
export VECTOR_DB_HOST="milvus.kudig-system.svc.cluster.local"
export VECTOR_DB_PORT="19530"

# 5. 执行数据导入 (分批, 每批 500 篇)
python scripts/ingest.py \
  --source ./docs \
  --batch-size 500 \
  --collection kudig_v1

# 6. 验证导入结果
python scripts/verify.py --collection kudig_v1
# 预期输出: 3,346 documents indexed, 40 domains covered
```

### Step 4: 部署 RAG 系统 (30 分钟)

```bash
# 1. 部署 RAG API 服务
kubectl apply -f deploy/rag-api.yaml

# 2. 配置 RAG Profile (使用 SRE Profile)
kubectl apply -f deploy/rag-profile-sre.yaml

# 3. 验证 RAG 服务
kubectl get pods -n kudig-system -l app=rag-api
kubectl port-forward svc/rag-api 8080:8080 -n kudig-system

# 4. 测试 RAG 查询
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{"question": "etcd 备份恢复怎么做", "top_k": 5}'
```

### Step 5: 部署 AI Agent (30 分钟)

```bash
# 1. 部署 Agent 服务
kubectl apply -f deploy/agent.yaml

# 2. 配置 Agent 连接 RAG
kubectl apply -f deploy/agent-config.yaml

# 3. 验证 Agent 服务
kubectl get pods -n kudig-system -l app=kudig-agent

# 4. 测试 Agent 问答
curl -X POST http://localhost:8081/chat \
  -H "Content-Type: application/json" \
  -d '{"message": "Pod CrashLoopBackOff 帮我排查"}'
```

### Step 6: 部署演示前端 (20 分钟)

```bash
# 1. 部署 Web 前端
kubectl apply -f deploy/frontend.yaml

# 2. 配置 Ingress
kubectl apply -f deploy/ingress.yaml

# 3. 验证前端访问
# 浏览器打开 https://demo.kudig.io
```

---

## 三、演示数据准备

### 预加载文档清单

以下文档必须在演示前确认已导入向量数据库:

| 优先级 | 文档 | 用途 | 对应演示场景 |
|--------|------|------|------------|
| P0 | domain-3 etcd 深度文档 | 深度研究演示 | 场景 1 |
| P0 | topic-skills SOP (全部 18 个) | 问题排查演示 | 场景 2 |
| P0 | topic-fta 故障树 | 问题排查演示 | 场景 2 |
| P0 | topic-application-architecture/06-fintech | 架构设计演示 | 场景 3 |
| P0 | command-output-diagnosis.md | 命令解读演示 | 场景 4 |
| P1 | diagnose-pod-crashloop.sh | 问题排查演示 | 场景 2 |
| P1 | topic-cheat-sheet (全部 13 张) | 速查演示 (备用) | - |
| P1 | topic-qa-corpus (抽检 100 组) | 验证 QA 质量 | - |

### 数据验证脚本

```bash
# 验证关键文档是否已导入
python scripts/verify.py --check-p0

# 预期输出:
# ✅ domain-3 etcd deep doc: FOUND (1,042 lines)
# ✅ topic-skills sop: FOUND (18 documents)
# ✅ topic-fta fault-tree: FOUND
# ✅ fintech-architecture: FOUND
# ✅ command-output-diagnosis: FOUND
# ✅ diagnose-pod-crashloop.sh: FOUND
# ✅ All P0 documents verified: 6/6
```

### 演示问题预设

```yaml
# demo-questions.yaml
scenarios:
  - name: "深度研究"
    question: "我要深度研究 etcd 的生产运维, 包括架构原理、Raft 共识、备份恢复和性能调优"
    max_response_time: 30s
    expected_keywords: ["Raft", "MVCC", "Watch", "备份恢复", "性能调优"]

  - name: "问题排查"
    question: "线上 Pod CrashLoopBackOff, RESTARTS 一直在涨, 帮我排查"
    max_response_time: 60s
    expected_keywords: ["崩溃日志", "OOMKilled", "livenessProbe", "修复建议"]

  - name: "架构设计"
    question: "帮我设计一套金融支付系统的 Kubernetes 生产架构, 要满足 PCI-DSS 合规要求"
    max_response_time: 120s
    expected_keywords: ["微服务", "HSM", "KMS", "多可用区", "YAML"]

  - name: "命令解读"
    question: "kubectl describe pod 显示 OOMKilled, Exit Code 137, 这是什么意思? 怎么处理?"
    max_response_time: 30s
    expected_keywords: ["128 + 9", "SIGKILL", "OOM Killer", "limits.memory"]
```

---

## 四、翻车预案 (Plan B)

### 场景 1: Agent 响应超时 (>60 秒)

| 级别 | 症状 | 处理 |
|------|------|------|
| 黄色 | 响应 30-60 秒 | 边演示边说 "知识库比较大, 让 Agent 再想想" |
| 红色 | 响应 >60 秒 | 切换到预录制屏 (录屏文件: demo/backup/scene1.mp4) |

**预防措施**:
- 演示前 30 分钟预热系统, 发送 3-5 个测试查询
- 使用 SRE Profile (精简版) 而非全量 Profile
- 确保向量数据库索引已构建完成

### 场景 2: Agent 输出质量不佳

| 级别 | 症状 | 处理 |
|------|------|------|
| 黄色 | 输出部分正确, 缺少关键信息 | 主持人补充说明: "Agent 命中了知识库, 让我补充几个关键点" |
| 红色 | 输出完全不相关 | 切换到预录制屏, 同时说 "网络波动, 我们用录屏展示" |

**预防措施**:
- 演示前用预设问题测试, 确认输出质量
- 准备好每个场景的 "理想输出" 文本, 用于主持人口述补充
- 在 RAG Profile 中调整 top_k 和相似度阈值

### 场景 3: K8s 集群故障

| 级别 | 症状 | 处理 |
|------|------|------|
| 黄色 | 单个 Pod 重启 | kubectl 自动恢复, 无需处理 |
| 红色 | 集群不可用 | 切换到备用集群 (提前部署好, 域名 DNS 切换) |

**预防措施**:
- 演示集群至少 3 个 Worker Node
- 关键服务设置 PodDisruptionBudget
- 提前准备好备用集群 (可用轻量 k3d 集群)
- 配置 readinessProbe 确保服务就绪

### 场景 4: 网络中断

| 级别 | 症状 | 处理 |
|------|------|------|
| 黄色 | 网络延迟高 | 降低演示节奏, 主持人多讲解 |
| 红色 | 网络完全中断 | 切换到本地离线演示环境 (提前准备) |

**预防措施**:
- 使用有线网络, 禁用 WiFi
- 提前测试网络带宽和延迟
- 准备本地离线演示环境 (kind 集群 + 本地 LLM)

### 场景 5: LLM API 不可用

| 级别 | 症状 | 处理 |
|------|------|------|
| 黄色 | API 偶尔超时 | 重试一次, 通常可恢复 |
| 红色 | API 完全不可用 | 切换到本地模型 (提前部署 Ollama + Qwen2.5) |

**预防措施**:
- 准备两个 LLM API Key (主 + 备)
- 本地部署一个备用模型 (Ollama + Qwen2.5-72B 或 DeepSeek-V3)
- 配置 Agent 的 fallback 机制, 自动切换到本地模型

### 场景 6: 投影/显示故障

| 级别 | 症状 | 处理 |
|------|------|------|
| 黄色 | 分辨率不匹配 | 调整显示器设置, 切换到 1080p |
| 红色 | 无信号输出 | 使用备用笔记本 + HDMI 线 |

**预防措施**:
- 带两台笔记本 (主 + 备)
- 带两根 HDMI 线 + 一个 USB-C 转 HDMI 转接头
- 提前到会场测试投影

---

## 五、彩排检查清单

### 彩排前 (T-24h)

- [ ] K8s 集群正常运行, 所有节点 Ready
- [ ] 向量数据库服务正常, 索引已构建
- [ ] 3,346 篇文档全部导入, verify.py 通过
- [ ] RAG API 服务正常, 端口可达
- [ ] Agent 服务正常, 能正常问答
- [ ] 前端页面可访问, UI 正常
- [ ] 4 个演示场景预设问题全部测试通过
- [ ] 响应时间全部在预期范围内
- [ ] 预录制屏文件准备完毕 (4 个场景各一份)
- [ ] 备用集群部署完成, DNS 可切换
- [ ] 本地离线环境准备完毕

### 彩排中 (T-12h)

- [ ] 完整走一遍 4 个演示场景
- [ ] 确认每个场景的响应时间
- [ ] 确认 Agent 输出质量 (对照 expected_keywords)
- [ ] 测试网络中断场景 (断网后切换到离线环境)
- [ ] 测试 LLM API 不可用场景 (切换到本地模型)
- [ ] 确认投屏/投影效果
- [ ] 确认主持人操作流程 (何时输入, 何时讲解)
- [ ] 计时: 确认演示总时长在 10-12 分钟

### 发布会当天 (T-0)

- [ ] 提前 2 小时到达会场
- [ ] 测试会场网络 (有线连接)
- [ ] 测试投影/大屏显示
- [ ] 预热系统: 发送 3-5 个测试查询
- [ ] 确认所有服务 Pod 运行正常
- [ ] 确认录屏文件可正常播放
- [ ] 确认备用笔记本就绪
- [ ] 确认 LLM API Key 余额充足
- [ ] 与主持人最后确认操作流程
- [ ] 准备一瓶水放台上

### 发布会期间

- [ ] 技术人员在后台监控系统状态
- [ ] 准备好一键切换到录屏的快捷方式
- [ ] 准备好一键切换到备用集群的脚本
- [ ] 保持手机静音, 但后台监控告警通道畅通

---

## 六、快速恢复脚本

```bash
#!/bin/bash
# demo-recovery.sh — 一键恢复演示环境

echo "=== kudig-database Demo Recovery ==="

# 1. 检查集群状态
echo "[1/5] Checking cluster..."
kubectl cluster-info || { echo "FATAL: Cluster unreachable"; exit 1; }

# 2. 检查关键 Pod
echo "[2/5] Checking pods..."
kubectl get pods -n kudig-system
NOT_READY=$(kubectl get pods -n kudig-system --field-selector=status.phase!=Running -o name)
if [ -n "$NOT_READY" ]; then
  echo "WARNING: Some pods not running, restarting..."
  kubectl delete pods -n kudig-system $NOT_READY
  sleep 30
fi

# 3. 检查 RAG API
echo "[3/5] Checking RAG API..."
curl -s http://rag-api.kudig-system.svc.cluster.local:8080/health || {
  echo "WARNING: RAG API unhealthy, restarting..."
  kubectl rollout restart deployment/rag-api -n kudig-system
  sleep 30
}

# 4. 检查 Agent
echo "[4/5] Checking Agent..."
curl -s http://agent.kudig-system.svc.cluster.local:8081/health || {
  echo "WARNING: Agent unhealthy, restarting..."
  kubectl rollout restart deployment/kudig-agent -n kudig-system
  sleep 30
}

# 5. 快速测试
echo "[5/5] Running quick test..."
RESPONSE=$(curl -s -X POST http://localhost:8081/chat \
  -H "Content-Type: application/json" \
  -d '{"message": "etcd 是什么"}')
echo "Test response: $RESPONSE"

echo "=== Recovery complete ==="
```

---

## 七、联系人

| 角色 | 负责人 | 联系方式 |
|------|--------|---------|
| 技术负责人 | [待填写] | [待填写] |
| 前端负责人 | [待填写] | [待填写] |
| 运维负责人 | [待填写] | [待填写] |
| 主持人 | [待填写] | [待填写] |
