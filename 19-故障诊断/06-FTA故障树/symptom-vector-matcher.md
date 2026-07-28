---
title: 症状向量匹配引擎 (Symptom Vector Matching Engine)
description: '## 一、设计目标'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- kubelet
- coredns
- ingress
- networkpolicy
- gpu
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 症状向量匹配引擎 (Symptom Vector Matching Engine) 是什么
- 如何 症状向量匹配引擎 (Symptom Vector Matching Engine)
- 症状向量匹配引擎 (Symptom Vector Matching Engine) 根因分析
- 症状向量匹配引擎 (Symptom Vector Matching Engine) 故障树
trigger_keywords:
- 症状向量匹配引擎
- Symptom
- Vector
- Matching
- Engine
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
tier: supporting
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 症状向量匹配引擎 (Symptom Vector Matching Engine)

> **版本**: v1.0
> **定位**: 将问题现象转化为可计算向量的智能匹配引擎
> **更新日期**: 2026-05-18

---

## 一、设计目标

### 1.1 问题定义

```
传统方式:
  输入: "[[22-概念/02-工作负载/pod-lifecycle|pod]] CrashLoopBackOff + OOMKilled + Exit 137"
  匹配: 精确匹配症状表
  输出: 固定映射 → BE-2.3
  
改进方式:
  输入: "Pod CrashLoopBackOff + OOMKilled + Exit 137"
  向量化: [0.9, 0.85, 0.95, 0.0, 0.0, ...] (12维特征向量)
  相似度: 与已知症状模式计算余弦相似度
  输出: [
    { pattern: "OOMKilled", similarity: 0.92, path: "BE-2.3" },
    { pattern: "CrashLoop", similarity: 0.78, path: "BE-2.1" },
    { pattern: "Evicted",   similarity: 0.35, path: "BE-3.2" }
  ]
```

### 1.2 核心能力

| 能力 | 说明 |
|:---|:---|
| **语义理解** | 理解"容器挂了"、"Pod 反复重启"等口语化表述 |
| **模糊匹配** | 未精确命中的症状也能找到相似模式 |
| **置信度排序** | 返回 Top-K 候选，含相似度得分 |
| **增量学习** | 新症状模式可追加到向量库 |
| **跨语言** | 支持中文/英文/混合输入 |

---

## 二、向量化设计

### 2.1 特征空间定义

```python
# 症状特征向量 (32维)
SYMPTOM_FEATURE_DIMENSIONS = [
    # 基础症状特征 (8维)
    "pod_restart",           # Pod 重启行为
    "pod_pending",          # Pod 调度失败
    "pod_evicted",          # Pod 驱逐
    "oom_killed",           # OOM 杀死
    "not_ready",            # 节点/服务未就绪
    "connection_fail",     # 连接失败
    "timeout",              # 超时
    "error_log",            # 错误日志存在
    
    # 资源特征 (6维)
    "memory_high",          # 内存使用率高
    "cpu_high",             # CPU 使用率高
    "disk_full",             # 磁盘空间不足
    "network_latency",       # 网络延迟
    "storage_io_high",       # 存储 IO 高
    "gpu_memory_high",       # GPU 内存高
    
    # 错误码特征 (4维)
    "exit_137",             # Exit Code 137 (OOM)
    "exit_1",               # Exit Code 1 (通用错误)
    "exit_143",             # Exit Code 143 (SIGTERM)
    "exit_125",             # Exit Code 125 (容器运行时错误)
    
    # 云厂商特征 (4维)
    "ack_specific",          # ACK 特有症状
    "aws_specific",          # AWS EKS 特有
    "gcp_specific",          # GCP GKE 特有
    "on_premise",            # 私有化部署
    
    # 时间特征 (4维)
    "startup_phase",        # 启动阶段
    "runtime_phase",         # 运行阶段
    "scale_phase",           # 扩缩容阶段
    "drain_phase",           # 节点排空阶段
    
    # 严重程度 (6维)
    "p0_critical",           # P0 紧急
    "p1_major",              # P1 重要
    "p2_minor",              # P2 一般
    "user_impact_high",      # 影响大量用户
    "service_down",          # 服务完全不可用
    "degraded",              # 服务降级
]

class SymptomVectorizer:
    """症状向量器"""
    
    def __init__(self):
        self.feature_dimensions = SYMPTOM_FEATURE_DIMENSIONS
        self.dimension_count = len(self.feature_dimensions)
        
    def vectorize(self, symptom_input):
        """
        将症状输入转换为特征向量
        """
        vector = [0.0] * self.dimension_count
        
        # 基础症状解析
        if "CrashLoopBackOff" in symptom_input or "反复重启" in symptom_input:
            vector[0] = 1.0  # pod_restart
        if "Pending" in symptom_input or "调度失败" in symptom_input:
            vector[1] = 1.0  # pod_pending
        if "Evicted" in symptom_input or "驱逐" in symptom_input:
            vector[2] = 1.0  # pod_evicted
        if "OOMKilled" in symptom_input or "OOM" in symptom_input:
            vector[3] = 1.0  # oom_killed
        if "NotReady" in symptom_input or "未就绪" in symptom_input:
            vector[4] = 1.0  # not_ready
        if "无法访问" in symptom_input or "connection" in symptom_input.lower():
            vector[5] = 1.0  # connection_fail
        if "超时" in symptom_input or "timeout" in symptom_input.lower():
            vector[6] = 1.0  # timeout
        if "error" in symptom_input.lower() or "错误" in symptom_input:
            vector[7] = 0.8  # error_log
            
        # Exit Code 解析
        exit_code = self.extract_exit_code(symptom_input)
        if exit_code == 137:
            vector[12] = 1.0  # exit_137
            vector[3] = max(vector[3], 0.95)  # OOMKilled 增强
        elif exit_code == 1:
            vector[13] = 0.7  # exit_1
        elif exit_code == 143:
            vector[14] = 0.8  # exit_143
            
        # 资源指标解析
        if "内存" in symptom_input or "memory" in symptom_input.lower():
            vector[8] = 0.9  # memory_high
        if "CPU" in symptom_input or "cpu" in symptom_input.lower():
            vector[9] = 0.8  # cpu_high
            
        # 严重程度
        if "P0" in symptom_input or "紧急" in symptom_input:
            vector[24] = 1.0
            vector[27] = 1.0  # service_down
            
        # 归一化
        vector = self.normalize(vector)
        
        return vector
    
    def extract_exit_code(self, symptom_input):
        """提取 Exit Code"""
        import re
        match = re.search(r'(?:exit|code|退出)[^\d]*(\d+)', symptom_input, re.I)
        if match:
            return int(match.group(1))
        return None
```

### 2.2 已知症状模式库

```yaml
symptom_patterns:
  # 模式 1: OOMKilled 经典模式
  - id: "PATTERN-OOM-001"
    name: "OOMKilled 经典模式"
    vector: [0.8, 0.0, 0.3, 1.0, 0.2, 0.1, 0.0, 0.9, 0.95, 0.3, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.6, 0.3, 0.0, 0.5, 0.0, 0.0, 0.0, 0.3]
    keywords:
      - "OOMKilled"
      - "Exit 137"
      - "memory"
      - "heap"
    fta_path: "TE-2 → IE-2.1 → BE-2.3"
    confidence: 0.95
    auto_fix_actions:
      - "HA-2.3.1: increase_memory_limit"
      - "HA-2.3.2: tune_jvm_heap"
      
  # 模式 2: Pod Pending 调度失败
  - id: "PATTERN-PEND-001"
    name: "Pod Pending 调度失败"
    vector: [0.3, 1.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.3, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.4, 0.6, 0.0, 0.4, 0.0, 0.0, 0.0, 0.2]
    keywords:
      - "Pending"
      - "调度失败"
      - "调度异常"
    fta_path: "TE-3 → IE-3.1 → BE-3.1"
    confidence: 0.90
    auto_fix_actions:
      - "HA-3.1.1: check_resource_quota"
      - "HA-3.1.2: adjust_affinity"
      
  # 模式 3: CrashLoopBackOff
  - id: "PATTERN-CL-001"
    name: "CrashLoopBackOff 循环崩溃"
    vector: [1.0, 0.2, 0.2, 0.5, 0.2, 0.3, 0.2, 0.7, 0.4, 0.3, 0.0, 0.0, 0.5, 0.6, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.5, 0.0, 0.6, 0.0, 0.0, 0.0, 0.3]
    keywords:
      - "CrashLoopBackOff"
      - "反复重启"
      - "容器启动失败"
    fta_path: "TE-2 → IE-2.1 → BE-2.1"
    confidence: 0.88
    auto_fix_actions:
      - "HA-2.1.1: check_liveness_probe"
      - "HA-2.1.2: verify_image"
      
  # 模式 4: DNS 解析失败
  - id: "PATTERN-DNS-001"
    name: "DNS 解析失败"
    vector: [0.2, 0.0, 0.0, 0.0, 0.1, 0.9, 0.8, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.0, 0.0, 0.3, 0.4, 0.0, 0.3, 0.0, 0.0, 0.0, 0.2]
    keywords:
      - "DNS"
      - "域名解析"
      - "nameserver"
      - "nslookup"
    fta_path: "TE-4 → IE-4.1 → BE-4.1"
    confidence: 0.92
    auto_fix_actions:
      - "HA-4.1.1: check_coredns"
      - "HA-4.1.2: verify_resolv_conf"
      
  # 模式 5: Node NotReady
  - id: "PATTERN-NR-001"
    name: "Node NotReady 节点异常"
    vector: [0.3, 0.1, 0.5, 0.1, 1.0, 0.2, 0.1, 0.2, 0.4, 0.3, 0.2, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.3, 0.0, 0.7, 0.0, 0.0, 0.0, 0.4]
    keywords:
      - "NotReady"
      - "节点异常"
      - "kubelet"
    fta_path: "TE-1 → IE-1.2 → BE-1.5"
    confidence: 0.90
    auto_fix_actions:
      - "HA-1.5.1: check_kubelet"
      - "HA-1.5.2: verify_node_disk"
      
  # 模式 6: Service 无法访问
  - id: "PATTERN-SVC-001"
    name: "Service 无法访问"
    vector: [0.3, 0.1, 0.0, 0.0, 0.2, 1.0, 0.5, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.4, 0.5, 0.0, 0.6, 0.0, 0.0, 0.0, 0.3]
    keywords:
      - "Service 无法访问"
      - "连接被拒绝"
      - "connection refused"
    fta_path: "TE-2 → IE-2.2 → BE-2.5"
    confidence: 0.85
    auto_fix_actions:
      - "HA-2.5.1: check_endpoints"
      - "HA-2.5.2: verify_selector"
      
  # 模式 7: PVC 挂载失败
  - id: "PATTERN-PVC-001"
    name: "PVC 挂载失败"
    vector: [0.4, 0.3, 0.0, 0.0, 0.1, 0.2, 0.1, 0.5, 0.0, 0.0, 0.3, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.4, 0.0, 0.4, 0.0, 0.0, 0.0, 0.3]
    keywords:
      - "PVC"
      - "挂载失败"
      - "mount"
      - "storage"
    fta_path: "TE-5 → IE-5.2 → BE-5.1"
    confidence: 0.88
    auto_fix_actions:
      - "HA-5.1.1: check_pvc_status"
      - "HA-5.1.2: verify_storage_class"
      
  # 模式 8: 证书过期
  - id: "PATTERN-CERT-001"
    name: "证书过期异常"
    vector: [0.1, 0.0, 0.0, 0.0, 0.2, 0.3, 0.2, 0.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.3, 0.8, 0.3, 0.0, 0.0, 0.0, 0.2]
    keywords:
      - "证书"
      - "certificate"
      - "过期"
      - "expired"
    fta_path: "TE-7 → IE-7.1 → BE-7.1"
    confidence: 0.93
    auto_fix_actions:
      - "HA-7.1.1: renew_certificate"
      - "HA-7.1.2: verify_cert_manager"
```

---

## 三、匹配算法实现

### 3.1 余弦相似度匹配

```python
import math

class SymptomVectorMatcher:
    """症状向量匹配器"""
    
    def __init__(self, pattern_library):
        self.patterns = pattern_library  # 已知症状模式库
        
    def match(self, symptom_input, top_k=5):
        """
        匹配症状输入，返回 Top-K 候选
        """
        
        # 1. 向量化输入
        input_vector = self.vectorizer.vectorize(symptom_input)
        
        # 2. 计算与每个模式的相似度
        candidates = []
        for pattern in self.patterns:
            similarity = self.cosine_similarity(input_vector, pattern.vector)
            
            # 加权计算综合得分
            keyword_score = self.keyword_match_score(symptom_input, pattern.keywords)
            final_score = similarity * 0.7 + keyword_score * 0.3
            
            candidates.append({
                "pattern_id": pattern.id,
                "pattern_name": pattern.name,
                "fta_path": pattern.fta_path,
                "vector_similarity": similarity,
                "keyword_score": keyword_score,
                "final_score": final_score,
                "confidence": pattern.confidence,
                "auto_fix_actions": pattern.auto_fix_actions
            })
        
        # 3. 排序并返回 Top-K
        candidates.sort(key=lambda x: x["final_score"], reverse=True)
        return candidates[:top_k]
    
    def cosine_similarity(self, vec1, vec2):
        """计算余弦相似度"""
        
        dot_product = sum(v1 * v2 for v1, v2 in zip(vec1, vec2))
        magnitude1 = math.sqrt(sum(v * v for v in vec1))
        magnitude2 = math.sqrt(sum(v * v for v in vec2))
        
        if magnitude1 == 0 or magnitude2 == 0:
            return 0.0
            
        return dot_product / (magnitude1 * magnitude2)
    
    def keyword_match_score(self, symptom_input, keywords):
        """计算关键词匹配得分"""
        
        input_lower = symptom_input.lower()
        matches = sum(1 for kw in keywords if kw.lower() in input_lower)
        
        return matches / len(keywords) if keywords else 0.0
```

### 3.2 语义扩展匹配

```python
class SemanticExpander:
    """语义扩展器 - 处理同义词和口语化表达"""
    
    SEMANTIC_MAP = {
        # Pod 状态相关
        "容器挂了": ["CrashLoopBackOff", "容器启动失败", "Pod 崩溃"],
        "Pod 挂了": ["CrashLoopBackOff", "Pod 不可用"],
        "Pod 反复重启": ["CrashLoopBackOff", "RestartLoop"],
        "Pod 起不来": ["Pending", "CrashLoopBackOff", "容器启动失败"],
        
        # 内存相关
        "内存泄漏": ["OOMKilled", "memory leak", "heap overflow"],
        "OOM": ["OOMKilled", "OutOfMemory"],
        "内存不够": ["OOMKilled", "memory limit"],
        
        # 网络相关
        "网络不通": ["connection fail", "网络异常", "NetworkPolicy"],
        "访问不了": ["connection fail", "无法访问"],
        "DNS 坏了": ["DNS 解析失败", "nameserver error"],
        "域名解析失败": ["DNS 解析失败", "nslookup failed"],
        
        # 节点相关
        "节点挂了": ["Node NotReady", "kubelet down"],
        "机器挂了": ["Node NotReady", "节点异常"],
        
        # 服务相关
        "服务挂了": ["Service 不可用", "Endpoint 异常"],
        "服务起不来": ["Service 不可用", "Ingress 503"],
        
        # 存储相关
        "存储挂了": ["PVC 挂载失败", "storage unavailable"],
        "盘满了": ["disk full", "存储空间不足"],
    }
    
    def expand(self, symptom_input):
        """语义扩展，将口语化表达转为标准术语"""
        
        expanded = symptom_input
        
        for informal, standards in self.SEMANTIC_MAP.items():
            if informal in symptom_input:
                # 替换为第一个标准术语
                expanded = expanded.replace(informal, standards[0])
                # 追加其他标准术语作为参考
                for std in standards[1:]:
                    if std not in expanded:
                        expanded += " " + std
                        
        return expanded
```

### 3.3 未知症状检测

```python
class UnknownSymptomDetector:
    """未知症状检测器"""
    
    def __init__(self, min_similarity_threshold=0.4):
        self.min_threshold = min_similarity_threshold
        
    def is_unknown(self, match_results):
        """
        判断是否为未知症状
        """
        
        if not match_results:
            return True, "无任何匹配模式"
            
        top_score = match_results[0]["final_score"]
        
        if top_score < self.min_threshold:
            return True, f"最高相似度 {top_score:.2f} < 阈值 {self.min_threshold}"
            
        # 检查是否有多个候选得分接近
        if len(match_results) >= 2:
            score_gap = match_results[0]["final_score"] - match_results[1]["final_score"]
            if score_gap < 0.1:
                return True, f"候选得分接近 (差距 {score_gap:.2f})，无法确认单一模式"
                
        return False, ""
    
    def generate_escalation(self, symptom_input, match_results):
        """生成升级建议"""
        
        is_unknown, reason = self.is_unknown(match_results)
        
        if not is_unknown:
            return None
            
        return {
            "type": "unknown_symptom",
            "reason": reason,
            "input": symptom_input,
            "recommended_actions": [
                "手动检查 kubectl get events",
                "查看最近日志 kubectl logs",
                "触发 FEBM 完整取证流程",
                "通知 SRE 团队人工介入"
            ],
            "escalate_to": "human_expert",
            "priority": "P1"
        }
```

---

## 四、完整匹配流程

```python
class SymptomMatchingPipeline:
    """症状匹配完整流程"""
    
    def __init__(self):
        self.expander = SemanticExpander()
        self.vectorizer = SymptomVectorizer()
        self.matcher = SymptomVectorMatcher(PATTERN_LIBRARY)
        self.detector = UnknownSymptomDetector()
        
    def match(self, symptom_input, context=None):
        """
        完整匹配流程
        """
        
        # Step 1: 语义扩展
        expanded = self.expander.expand(symptom_input)
        
        # Step 2: 向量化
        vector = self.vectorizer.vectorize(expanded)
        
        # Step 3: 模式匹配
        candidates = self.matcher.match(expanded, top_k=5)
        
        # Step 4: 未知症状检测
        is_unknown, reason = self.detector.is_unknown(candidates)
        
        # Step 5: 构建输出
        result = {
            "input": symptom_input,
            "expanded_input": expanded,
            "vector_representation": vector,
            "matches": candidates,
            "is_unknown": is_unknown,
            "unknown_reason": reason
        }
        
        # 如果是未知症状，生成升级建议
        if is_unknown:
            result["escalation"] = self.detector.generate_escalation(
                symptom_input, 
                candidates
            )
        else:
            # 返回最佳匹配
            result["best_match"] = candidates[0]
            
        # 添加上下文感知推荐
        if context:
            result["context_aware_recommendations"] = self.apply_context(
                candidates, 
                context
            )
            
        return result
    
    def apply_context(self, candidates, context):
        """应用上下文信息调整推荐"""
        
        recommendations = []
        
        for candidate in candidates[:3]:
            # 根据上下文调整置信度
            adjusted_confidence = candidate["confidence"]
            
            # ACK 特有症状
            if context.get("cloud_provider") == "ACK":
                if any(kw in candidate["pattern_name"] for kw in ["Terway", "ASM", "ACK-One"]):
                    adjusted_confidence *= 1.2
                    
            # 高负载环境
            if context.get("is_high_load"):
                if "OOM" in candidate["pattern_name"]:
                    adjusted_confidence *= 1.15
                    
            recommendations.append({
                "pattern": candidate["pattern_name"],
                "adjusted_confidence": min(1.0, adjusted_confidence),
                "fta_path": candidate["fta_path"],
                "actions": candidate["auto_fix_actions"]
            })
            
        return recommendations
```

---

## 五、与 FTA 集成

### 5.1 匹配结果 → FTA 路径

```python
class FTARouter:
    """FTA 路由 - 将匹配结果转为 FTA 执行路径"""
    
    def route(self, match_result):
        """
        将症状匹配结果路由到 FTA 执行引擎
        """
        
        if match_result["is_unknown"]:
            # 未知症状：进入 FEBM 取证流程
            return {
                "mode": "FEBM",
                "reason": "未知症状模式",
                "input": match_result["input"],
                "escalation": match_result.get("escalation")
            }
            
        # 已知症状：构建 FTA 执行请求
        best_match = match_result["best_match"]
        
        fta_request = {
            "mode": "FTA",
            "top_event": self.extract_top_event(best_match["fta_path"]),
            "target_path": best_match["fta_path"],
            "initial_evidence": self.extract_evidence_from_input(match_result["input"]),
            "confidence_threshold": 0.7,
            "context": {
                 "pattern_id": best_match["pattern_id"],
                 "match_score": best_match["final_score"],
                 "auto_actions": best_match["auto_fix_actions"]
             }
        }
        
        return fta_request
    
    def extract_top_event(self, fta_path):
        """从 FTA 路径提取顶事件"""
        # 例如: "TE-2 → IE-2.1 → BE-2.3" → "TE-2"
        return fta_path.split("→")[0].strip()
    
    def extract_evidence_from_input(self, symptom_input):
        """从症状输入提取初始证据"""
        
        evidence = []
        
        if "OOMKilled" in symptom_input or "OOM" in symptom_input:
            evidence.append({"type": "event", "message": "OOMKilled"})
        if "Exit 137" in symptom_input or "137" in symptom_input:
            evidence.append({"type": "exit_code", "value": 137})
            
        return evidence
```

### 5.2 集成示例

```python
# 使用示例
pipeline = SymptomMatchingPipeline()

# 输入: 口语化症状
symptom = "Pod 反复重启，OOMKilled，exit code 137"

# 执行匹配
result = pipeline.match(symptom, context={"cloud_provider": "ACK", "is_high_load": True})

print(f"""
输入: {result['input']}
扩展: {result['expanded_input']}

最佳匹配: {result['best_match']['pattern_name']}
FTA 路径: {result['best_match']['fta_path']}
相似度: {result['best_match']['final_score']:.2f}

推荐修复:
""")

for action in result['best_match']['auto_fix_actions']:
    print(f"  - {action}")

# 生成 FTA 执行请求
router = FTARouter()
fta_request = router.route(result)
print(f"\nFTA 执行请求: {fta_request}")
```

---

## 六、模式库管理

### 6.1 模式更新 API

```python
class PatternLibraryManager:
    """模式库管理器"""
    
    def __init__(self, storage_path):
        self.storage_path = storage_path
        self.patterns = self.load_patterns()
        
    def add_pattern(self, pattern):
        """添加新模式"""
        
        # 向量化新模式
        pattern.vector = self.vectorizer.vectorize(
            " ".join(pattern.keywords)
        )
        
        self.patterns.append(pattern)
        self.save_patterns()
        
    def update_pattern_confidence(self, pattern_id, delta):
        """更新模式置信度 (来自学习反馈)"""
        
        for pattern in self.patterns:
            if pattern.id == pattern_id:
                pattern.confidence = min(1.0, pattern.confidence + delta)
                self.save_patterns()
                return
                
    def suggest_new_pattern(self, symptoms, fta_path, evidence):
        """
        从实际问题中学习，提议新模式
        (由 FTA 学习引擎调用)
        """
        
        new_pattern = {
            "id": f"PROPOSED-{len(self.patterns) + 1}",
            "name": self.generate_name(symptoms),
            "keywords": symptoms,
            "fta_path": fta_path,
            "vector": self.vectorizer.vectorize(" ".join(symptoms)),
            "confidence": 0.5,  # 新模式初始置信度
            "status": "proposed",  # 待评审
            "evidence": evidence
        }
        
        return new_pattern
```

### 6.2 模式库持久化

```yaml
# pattern-library.yaml
version: "1.0"
last_updated: "2026-05-18"
patterns:
  - id: "PATTERN-OOM-001"
    name: "OOMKilled 经典模式"
    keywords: ["OOMKilled", "Exit 137", "memory", "heap"]
    vector_dimensions: 32
    confidence: 0.95
    status: "active"
    
  - id: "PATTERN-PEND-001"
    name: "Pod Pending 调度失败"
    keywords: ["Pending", "调度失败", "调度异常"]
    vector_dimensions: 32
    confidence: 0.90
    status: "active"

proposed_patterns:
  # 待评审的新模式
  - id: "PROPOSED-001"
    name: "Terway ENI 多队列压力"
    keywords: ["ENI", "队列压力", "网络抖动"]
    fta_path: "TE-9 → IE-9.1 → BE-9.1.1"
    proposed_by: "learning_engine"
    proposed_at: "2026-05-18"
    evidence_count: 5
```

---

## 七、性能指标

| 指标 | 目标值 | 说明 |
|:---|:---:|:---|
| **匹配延迟** | < 50ms | 从输入到返回候选的延迟 |
| **Top-1 准确率** | ≥ 90% | 最佳匹配正确的比例 |
| **Top-3 召回率** | ≥ 95% | 正确结果在前 3 候选中的比例 |
| **未知症状检测率** | ≥ 85% | 未知症状被正确识别的比例 |
| **向量库规模** | 支持 1000+ 模式 | 单实例可管理的模式数量 |

---

> **版本**: v1.0
> **维护团队**: Platform Team / AI Team
> **下一步**: 集成到 K8sOpsAgent 实现

<!-- risk-assessed -->
