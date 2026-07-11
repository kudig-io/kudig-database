---
title: GPU 利用率报告脚本
description: 集群范围内 GPU 利用率报告与空闲检测
summary: GPU 利用率报告脚本 — 跨命名空间 GPU 使用率分析和空闲 GPU 检测
category: automation
tags:
- k8s
- automation
- gpu
- monitoring
- nvidia
- bash
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- AI 平台工程师
- FinOps
estimated_read_time: 8min
intent_queries:
- GPU 利用率报告脚本 是什么
- 如何监控 Kubernetes GPU 使用率
- GPU 空闲检测脚本
- kubernetes gpu utilization report script
trigger_keywords:
- GPU
- 利用率
- utilization
- nvidia
- 报告
- 脚本
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本脚本为只读检查 (🟢)，不修改集群状态。

# GPU 利用率报告脚本

> 脚本 ID: `AUTO-02` | 语言: Bash+Python | 风险: 🟢 只读 | 执行时间: ~15s

## 概述

本脚本对集群中所有 GPU 节点执行利用率采集和报告:

1. **GPU 概览** — 节点级 GPU 数量、型号、驱动版本
2. **利用率采集** — 通过 `nvidia-smi` 采集每张 GPU 的 SM 利用率、显存使用、温度
3. **Pod 映射** — GPU 设备到 Pod 的映射 (谁在用什么 GPU)
4. **空闲检测** — 识别长时间低利用率的 GPU，提供成本优化建议
5. **汇总报告** — 按命名空间/工作负载聚合 GPU 资源使用

## 前置条件

- NVIDIA GPU 节点已安装 `nvidia-smi` (驱动 >= 525)
- 部署了 NVIDIA Device Plugin (`nvidia/k8s-device-plugin`)
- 部署了 DCGM Exporter (推荐，用于 Prometheus 集成)
- 执行脚本的主机具有对 GPU 节点的 SSH 访问权限，或通过 DaemonSet 执行
- `jq` 和 `python3` 已安装

## 使用方法

```bash
# 通过 SSH 远程采集 (需要免密登录)
bash gpu-utilization-report.sh --mode ssh --ssh-user core

# 通过 DaemonSet (在集群内执行)
bash gpu-utilization-report.sh --mode daemonset

# 从 Prometheus/DCGM Exporter 采集
bash gpu-utilization-report.sh --mode prometheus --prom-url http://prometheus:9090

# 指定空闲阈值 (默认: 利用率 < 10% 持续 30 分钟)
bash gpu-utilization-report.sh --idle-threshold 5 --idle-duration 60

# JSON 输出
bash gpu-utilization-report.sh --json
```

## 完整脚本

```bash
#!/bin/bash
# gpu-utilization-report.sh — GPU 利用率报告
# 风险等级: 🟢 只读，无副作用

set -euo pipefail

MODE="ssh"
SSH_USER="core"
PROM_URL=""
IDLE_THRESHOLD=10      # 利用率百分比
IDLE_DURATION=30       # 分钟
OUTPUT="text"
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

while [[ $# -gt 0 ]]; do
    case $1 in
        --mode) MODE="$2"; shift 2 ;;
        --ssh-user) SSH_USER="$2"; shift 2 ;;
        --prom-url) PROM_URL="$2"; shift 2 ;;
        --idle-threshold) IDLE_THRESHOLD="$2"; shift 2 ;;
        --idle-duration) IDLE_DURATION="$2"; shift 2 ;;
        --json) OUTPUT="json"; shift ;;
        *) echo "Unknown: $1"; exit 1 ;;
    esac
done

echo "============================================"
echo " GPU Utilization Report — $TIMESTAMP"
echo "============================================"

# ── 获取 GPU 节点列表 ──
GPU_NODES=$(kubectl get nodes -l nvidia.com/gpu.present=true \
    -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' 2>/dev/null)

if [ -z "$GPU_NODES" ]; then
    echo "No GPU nodes found in cluster."
    exit 0
fi

TOTAL_GPUS=0
IDLE_GPUS=0
GPU_DATA="[]"

echo -e "\n[1/4] GPU Node Inventory"
echo "--------------------------------------------"
echo "$GPU_NODES" | while read -r node; do
    [ -z "$node" ] && return
    local gpu_count gpu_model driver
    gpu_count=$(kubectl get node "$node" -o jsonpath='{.status.capacity.nvidia\.com/gpu}' 2>/dev/null)
    gpu_model=$(kubectl get node "$node" -o jsonpath='{.metadata.labels.nvidia\.com/gpu\.product}' 2>/dev/null)
    driver=$(kubectl get node "$node" -o jsonpath='{.status.nodeInfo.kernel}' 2>/dev/null)
    echo "  Node: $node"
    echo "    GPU Model : ${gpu_model:-unknown}"
    echo "    GPU Count : ${gpu_count:-0}"
    echo "    Driver    : ${driver:-unknown}"
    TOTAL_GPUS=$((TOTAL_GPUS + gpu_count))
done

# ── 采集 GPU 利用率 ──
collect_gpu_utilization() {
    echo -e "\n[2/4] GPU Utilization (per device)"
    echo "--------------------------------------------"
    
    if [ "$MODE" = "prometheus" ] && [ -n "$PROM_URL" ]; then
        # 通过 DCGM Exporter / Prometheus 查询
        echo "  (via Prometheus: $PROM_URL)"
        
        # 查询 GPU 利用率
        curl -s "${PROM_URL}/api/v1/query" \
            --data-urlencode 'query=DCGM_FI_DEV_GPU_UTIL' | \
            jq -r '.data.result[] | "  \(.metric.node)/GPU\(.metric.gpu): \(.value[1])% utilization"' 2>/dev/null
        
        # 查询显存使用
        echo ""
        curl -s "${PROM_URL}/api/v1/query" \
            --data-urlencode 'query=DCGM_FI_DEV_FB_USED_MEM / DCGM_FI_DEV_FB_TOTAL_MEM * 100' | \
            jq -r '.data.result[] | "  \(.metric.node)/GPU\(.metric.gpu): \(.value[1])% memory"' 2>/dev/null
        
        # 识别空闲 GPU
        echo ""
        local idle_gpus
        idle_gpus=$(curl -s "${PROM_URL}/api/v1/query" \
            --data-urlencode "query=avg_over_time(DCGM_FI_DEV_GPU_UTIL[${IDLE_DURATION}m]) < ${IDLE_THRESHOLD}" | \
            jq -r '.data.result[] | "  \(.metric.node)/GPU\(.metric.gpu)"' 2>/dev/null)
        
        if [ -n "$idle_gpus" ]; then
            echo "  ⚠️  Idle GPUs (< ${IDLE_THRESHOLD}% for ${IDLE_DURATION}m):"
            echo "$idle_gpus"
        fi
    
    elif [ "$MODE" = "daemonset" ]; then
        # 直接在 GPU 节点上运行
        echo "  (local nvidia-smi)"
        nvidia-smi --query-gpu=index,name,utilization.gpu,utilization.memory,memory.total,memory.used,temperature.gpu,power.draw \
            --format=csv,noheader | while IFS=',' read -r idx name gpu_util mem_util mem_total mem_used temp power; do
            echo "  GPU${idx} (${name}): util=${gpu_util}, mem=${mem_used}/${mem_total}, temp=${temp}C, power=${power}"
        done
    
    else
        # SSH 模式
        echo "  (via SSH to GPU nodes)"
        echo "$GPU_NODES" | while read -r node; do
            [ -z "$node" ] && return
            echo "  --- Node: $node ---"
            ssh "${SSH_USER}@${node}" \
                "nvidia-smi --query-gpu=index,name,utilization.gpu,utilization.memory,memory.total,memory.used,temperature.gpu,power.draw --format=csv,noheader" 2>/dev/null | \
                while IFS=',' read -r idx name gpu_util mem_util mem_total mem_used temp power; do
                    echo "    GPU${idx} (${name}): util=${gpu_util}, mem=${mem_used}/${mem_total}, temp=${temp}C, power=${power}"
                done
        done
    fi
}

collect_gpu_utilization

# ── Pod → GPU 映射 ──
echo -e "\n[3/4] Pod → GPU Allocation"
echo "--------------------------------------------"
kubectl get pods -A -o json 2>/dev/null | \
    python3 -c "
import json, sys
data = json.load(sys.stdin)
gpu_pods = []
for pod in data.get('items', []):
    ns = pod['metadata']['namespace']
    name = pod['metadata']['name']
    node = pod.get('spec', {}).get('nodeName', 'N/A')
    for c in pod.get('spec', {}).get('containers', []):
        req = c.get('resources', {}).get('limits', {})
        gpu_count = req.get('nvidia.com/gpu', 0)
        if gpu_count:
            gpu_pods.append({
                'namespace': ns,
                'pod': name,
                'container': c['name'],
                'node': node,
                'gpu_count': gpu_count
            })

if gpu_pods:
    for p in gpu_pods:
        print(f\"  {p['namespace']}/{p['pod']} [{p['container']}] → {p['gpu_count']} GPU on {p['node']}\")
    print(f\"\n  Total GPU allocated: {sum(p['gpu_count'] for p in gpu_pods)}\")
else:
    print('  No pods currently using GPUs')
" 2>/dev/null || echo "  ⚠️  Failed to parse pod allocations"

# ── 汇总报告 ──
echo -e "\n[4/4] Summary"
echo "--------------------------------------------"
TOTAL_NODES=$(echo "$GPU_NODES" | wc -w)
echo "  GPU Nodes       : $TOTAL_NODES"
echo "  Idle Threshold  : < ${IDLE_THRESHOLD}% utilization for ${IDLE_DURATION}min"
echo ""
echo "  💡 Optimization Tips:"
echo "    - Idle GPUs can be scheduled for low-priority jobs (time-slicing)"
echo "    - Consider GPU MIG for underutilized A100/H100 GPUs"
echo "    - Use Volcano/Kueue for GPU job queueing"
echo "    - Review [[脚本/prompts/cost-analysis|cost analysis]] for GPU cost optimization"

echo -e "\n============================================"
echo " GPU Report Complete"
echo "============================================"
```

## 输出示例

```
[2/4] GPU Utilization (per device)
--------------------------------------------
  --- Node: gpu-node-01 ---
    GPU0 (NVIDIA A100-SXM4-80GB): util= 85 %, mem= 65000/81920 MiB, temp=68C, power=310W
    GPU1 (NVIDIA A100-SXM4-80GB): util= 3 %, mem= 1200/81920 MiB, temp=35C, power=65W
  ⚠️  Idle GPUs (< 10% for 30m):
    gpu-node-01/GPU1

[3/4] Pod → GPU Allocation
--------------------------------------------
  ai-training/distributed-training-worker-0 [trainer] → 1 GPU on gpu-node-01

[4/4] Summary
  💡 GPU1 on gpu-node-01 is idle — consider MIG or time-slicing
```

## 集成建议

- 配合 [[脚本/prompts/cost-analysis|成本优化 Prompt]] 分析 GPU 空闲时段的成本浪费
- 结合 [[脚本/prompts/capacity-review|容量规划 Prompt]] 评估 GPU 右 Sizing
- 建议每 5 分钟通过 Prometheus 采集一次，本脚本用于每日汇总报告
- 空闲 GPU 超过 2 小时建议触发告警，通知相关负责人

<!-- risk-assessed -->
