# AI平台故障排查与性能优化

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Troubleshooting](https://kubernetes.io/docs/tasks/debug/) | [NVIDIA DCGM](https://developer.nvidia.com/dcgm)

## 一、AI平台故障诊断体系

### 1.1 故障分类与诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                          AI Platform Troubleshooting Framework                      │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            问题识别阶段 (Problem Identification)               │  │
│  │                                                                               │  │
│  │  用户报告问题 ──▶ 症状收集 ──▶ 影响范围评估 ──▶ 优先级确定                      │  │
│  │       │              │              │              │                            │  │
│  │       ▼              ▼              ▼              ▼                            │  │
│  │  ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐                         │  │
│  │  │  SLA    │   │  日志    │   │  监控    │   │  用户    │                         │  │
│  │  │  影响   │   │  收集    │   │  数据    │   │  反馈    │                         │  │
│  │  └─────────┘   └─────────┘   └─────────┘   └─────────┘                         │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            根因分析阶段 (Root Cause Analysis)                  │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   基础设施   │  │    模型     │  │    应用     │  │    网络     │          │  │
│  │  │    层面     │  │    层面     │  │    层面     │  │    层面     │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • GPU故障   │  │ • 模型损坏   │  │ • 代码缺陷   │  │ • 网络延迟   │          │  │
│  │  │ • 存储异常   │  │ • 版本不匹配 │  │ • 配置错误   │  │ • 带宽不足   │          │  │
│  │  │ • 节点问题   │  │ • 数据污染   │  │ • 资源竞争   │  │ • DNS解析   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            解决方案阶段 (Solution Implementation)              │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   临时      │  │   永久      │  │   预防      │  │   验证      │          │  │
│  │  │   修复      │  │   修复      │  │   措施      │  │   测试      │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • 快速缓解   │  │ • 根治问题   │  │ • 改进监控   │  │ • 回归测试   │          │  │
│  │  │ • 服务降级   │  │ • 代码修复   │  │ • 更新流程   │  │ • 性能验证   │          │  │
│  │  │ • 容错切换   │  │ • 架构优化   │  │ • 培训文档   │  │ • 用户验收   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            经验总结阶段 (Knowledge Management)                 │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   文档      │  │   培训      │  │   工具      │  │   流程      │          │  │
│  │  │   更新      │  │   分享      │  │   改进      │  │   优化      │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • 故障手册   │  │ • 经验分享   │  │ • 自动化    │  │ • SOP更新   │          │  │
│  │  │ • 最佳实践   │  │ • 团队培训   │  │ • 预警机制   │  │ • 响应提速   │          │  │
│  │  │ • 案例库     │  │ • 知识传递   │  │ • 诊断工具   │  │ • 协作改进   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 常见故障模式矩阵

| 故障类型 | 典型症状 | 影响层级 | MTTR | 诊断工具 |
|----------|----------|----------|------|----------|
| **GPU故障** | CUDA错误、驱动崩溃、温度过高 | 基础设施 | 30min | nvidia-smi, dcgmi |
| **模型加载失败** | 无法启动、OOM、权重损坏 | 应用层 | 15min | model logs, kubectl describe |
| **推理延迟高** | P99>500ms、吞吐量下降 | 服务层 | 45min | Prometheus, Jaeger |
| **训练卡住** | Loss不收敛、梯度消失 | 计算层 | 60min | TensorBoard, profiling |
| **数据访问异常** | 数据读取慢、文件损坏 | 存储层 | 20min | iostat, storage logs |
| **网络通信问题** | 连接超时、丢包严重 | 网络层 | 25min | ping, tcpdump, traceroute |

---

## 二、基础设施层故障排查

### 2.1 GPU相关问题诊断

```bash
#!/bin/bash
# gpu_diagnostics.sh - GPU故障诊断脚本

echo "=== GPU诊断报告 ==="
echo "生成时间: $(date)"
echo "节点名称: $(hostname)"
echo

# 1. 基础信息收集
echo "1. GPU基本信息:"
nvidia-smi --query-gpu=name,driver_version,memory.total,memory.used --format=csv,noheader,nounits
echo

# 2. 温度和功耗检查
echo "2. 温度和功耗状态:"
nvidia-smi --query-gpu=temperature.gpu,power.draw,enforced.power.limit --format=csv,noheader,nounits
echo

# 3. 进程和内存使用
echo "3. GPU进程占用:"
nvidia-smi pmon -i 0 -s um
echo

# 4. ECC错误检查
echo "4. ECC错误统计:"
nvidia-smi --query-gpu=ecc.errors.corrected.volatile.total,ecc.errors.uncorrected.volatile.total --format=csv,noheader,nounits
echo

# 5. PCIe连接状态
echo "5. PCIe连接状态:"
nvidia-smi --query-gpu=pcie.link.gen.current,pcie.link.width.current --format=csv,noheader,nounits
echo

# 6. DCGM健康检查
echo "6. DCGM健康状态:"
if command -v dcgmi &> /dev/null; then
    dcgmi health -g 0 -v
else
    echo "DCGM未安装，跳过健康检查"
fi
echo

# 7. GPU拓扑检查
echo "7. GPU拓扑结构:"
nvidia-smi topo -m
echo

# 8. 性能测试
echo "8. GPU性能基准测试:"
python3 -c "
import torch
if torch.cuda.is_available():
    device = torch.device('cuda')
    # 简单的矩阵乘法测试
    a = torch.randn(1000, 1000).to(device)
    b = torch.randn(1000, 1000).to(device)
    torch.cuda.synchronize()
    import time
    start = time.time()
    c = torch.mm(a, b)
    torch.cuda.synchronize()
    end = time.time()
    print(f'矩阵乘法耗时: {(end-start)*1000:.2f} ms')
    print(f'GPU利用率: {torch.cuda.utilization()}%')
else:
    print('CUDA不可用')
"
```

**常见GPU故障处理**：

```bash
# GPU驱动问题
# 重启nvidia驱动
sudo systemctl restart nvidia-persistenced
sudo modprobe -r nvidia_uvm && sudo modprobe nvidia_uvm

# 清理GPU内存
sudo nvidia-smi --gpu-reset

# 检查GPU是否被独占
lsof /dev/nvidia*

# 温度过高处理
# 检查风扇转速
nvidia-settings -q [gpu:0]/GPUFanControlState
# 设置风扇策略
nvidia-settings -a [gpu:0]/GPUFanControlState=1 -a [fan:0]/GPUTargetFanSpeed=80
```

### 2.2 存储性能问题排查

```bash
#!/bin/bash
# storage_diagnostics.sh - 存储性能诊断

echo "=== 存储诊断报告 ==="
echo "节点: $(hostname)"
echo "时间: $(date)"
echo

# 1. 存储挂载检查
echo "1. 存储挂载状态:"
df -h | grep -E "(models|data|storage)"
mount | grep -E "(nfs|ceph|gluster)"
echo

# 2. I/O性能测试
echo "2. I/O性能基准测试:"
echo "顺序读取测试:"
dd if=/dev/zero of=/tmp/testfile bs=1G count=1 oflag=direct 2>&1 | tail -1
echo "顺序写入测试:"
dd if=/tmp/testfile of=/dev/null bs=1G count=1 iflag=direct 2>&1 | tail -1
rm -f /tmp/testfile
echo

# 3. 存储延迟检查
echo "3. 存储延迟统计:"
iostat -x 1 5 | grep -A 1 "^Device"
echo

# 4. 文件系统检查
echo "4. 文件系统健康状态:"
for fs in $(df -T | awk '/ext4|xfs|zfs/{print $1}'); do
    echo "检查 $fs:"
    tune2fs -l $fs 2>/dev/null | grep -E "(State|Last checked)"
done
echo

# 5. NFS挂载问题排查
echo "5. NFS连接状态:"
showmount -e localhost 2>/dev/null || echo "NFS服务未运行"
rpcinfo -p localhost 2>/dev/null | grep nfs
echo

# 6. Ceph存储检查（如果使用）
if command -v ceph &> /dev/null; then
    echo "6. Ceph集群状态:"
    ceph health detail
    ceph df
    ceph osd pool stats
fi
```

---

## 三、应用层故障诊断

### 3.1 模型服务故障排查

```bash
#!/bin/bash
# model_service_diagnostics.sh - 模型服务诊断脚本

NAMESPACE=${1:-ai-models}
SERVICE_NAME=${2:-llama3-inference}

echo "=== 模型服务诊断报告 ==="
echo "命名空间: $NAMESPACE"
echo "服务名称: $SERVICE_NAME"
echo "时间: $(date)"
echo

# 1. Pod状态检查
echo "1. Pod运行状态:"
kubectl get pods -n $NAMESPACE -l app=$SERVICE_NAME -o wide
echo

# 2. 服务端点检查
echo "2. 服务端点状态:"
kubectl get endpoints -n $NAMESPACE $SERVICE_NAME
echo

# 3. 服务日志分析
echo "3. 最近错误日志:"
kubectl logs -n $NAMESPACE -l app=$SERVICE_NAME --tail=100 | grep -i "error\|exception\|failed" | tail -20
echo

# 4. 资源使用情况
echo "4. 资源使用统计:"
kubectl top pods -n $NAMESPACE -l app=$SERVICE_NAME
echo

# 5. 服务描述信息
echo "5. 服务详细信息:"
kubectl describe service $SERVICE_NAME -n $NAMESPACE
echo

# 6. 部署配置检查
echo "6. 部署配置状态:"
kubectl describe deployment $SERVICE_NAME -n $NAMESPACE
echo

# 7. 网络策略检查
echo "7. 网络策略影响:"
kubectl get networkpolicies -n $NAMESPACE
echo

# 8. 健康检查探针
echo "8. 健康检查状态:"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$SERVICE_NAME --sort-by='.lastTimestamp' | tail -10
```

**模型加载失败常见原因**：

```python
# model_loading_debugger.py
import traceback
import sys
import psutil
import GPUtil

class ModelLoadingDebugger:
    def __init__(self):
        self.checks = []
        
    def diagnose_loading_failure(self, model_path, error_message):
        """诊断模型加载失败的根本原因"""
        
        diagnosis = {
            'error_analysis': self._analyze_error_message(error_message),
            'resource_check': self._check_system_resources(),
            'file_integrity': self._verify_file_integrity(model_path),
            'dependency_check': self._check_dependencies(),
            'recommendations': []
        }
        
        # 根据诊断结果生成建议
        diagnosis['recommendations'] = self._generate_recommendations(diagnosis)
        
        return diagnosis
        
    def _analyze_error_message(self, error_msg):
        """分析错误消息类型"""
        
        error_patterns = {
            'memory_error': ['out of memory', 'oom', 'cuda out of memory'],
            'file_error': ['file not found', 'corrupted', 'checksum'],
            'compatibility_error': ['version mismatch', 'incompatible'],
            'permission_error': ['permission denied', 'access denied'],
            'network_error': ['connection refused', 'timeout', 'unreachable']
        }
        
        error_type = 'unknown'
        for category, patterns in error_patterns.items():
            if any(pattern in error_msg.lower() for pattern in patterns):
                error_type = category
                break
                
        return {
            'type': error_type,
            'message': error_msg,
            'stack_trace': traceback.format_exc()
        }
        
    def _check_system_resources(self):
        """检查系统资源"""
        
        # GPU资源检查
        gpus = GPUtil.getGPUs()
        gpu_info = []
        for gpu in gpus:
            gpu_info.append({
                'id': gpu.id,
                'name': gpu.name,
                'memory_free': gpu.memoryFree,
                'memory_used': gpu.memoryUsed,
                'memory_total': gpu.memoryTotal,
                'utilization': gpu.load
            })
            
        # CPU和内存检查
        cpu_percent = psutil.cpu_percent(interval=1)
        memory = psutil.virtual_memory()
        
        return {
            'cpu_usage': cpu_percent,
            'memory_available_gb': memory.available / (1024**3),
            'memory_total_gb': memory.total / (1024**3),
            'gpus': gpu_info
        }
        
    def _verify_file_integrity(self, model_path):
        """验证模型文件完整性"""
        
        import os
        import hashlib
        
        if not os.path.exists(model_path):
            return {'status': 'missing', 'details': '文件不存在'}
            
        # 检查文件大小
        file_size = os.path.getsize(model_path)
        
        # 计算校验和（如果是已知的模型）
        expected_checksums = {
            'llama3-70b.bin': 'expected_md5_hash_here'
        }
        
        filename = os.path.basename(model_path)
        if filename in expected_checksums:
            actual_hash = self._calculate_md5(model_path)
            expected_hash = expected_checksums[filename]
            
            return {
                'status': 'valid' if actual_hash == expected_hash else 'corrupted',
                'file_size_mb': file_size / (1024*1024),
                'checksum_match': actual_hash == expected_hash
            }
            
        return {
            'status': 'unknown',
            'file_size_mb': file_size / (1024*1024),
            'note': '无校验和信息'
        }
        
    def _calculate_md5(self, filepath):
        """计算文件MD5校验和"""
        import hashlib
        hash_md5 = hashlib.md5()
        with open(filepath, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        return hash_md5.hexdigest()
        
    def _check_dependencies(self):
        """检查依赖项兼容性"""
        
        import torch
        import transformers
        
        return {
            'torch_version': torch.__version__,
            'transformers_version': transformers.__version__,
            'cuda_available': torch.cuda.is_available(),
            'cuda_version': torch.version.cuda if torch.cuda.is_available() else 'N/A',
            'cudnn_version': torch.backends.cudnn.version() if torch.backends.cudnn.is_available() else 'N/A'
        }
        
    def _generate_recommendations(self, diagnosis):
        """生成解决建议"""
        
        recommendations = []
        error_type = diagnosis['error_analysis']['type']
        
        if error_type == 'memory_error':
            recommendations.extend([
                "增加GPU内存分配",
                "启用模型量化减少内存需求",
                "使用模型并行或流水线并行",
                "检查是否有其他进程占用GPU内存"
            ])
            
        elif error_type == 'file_error':
            recommendations.extend([
                "重新下载模型文件",
                "验证存储系统的完整性",
                "检查文件权限设置",
                "确认模型路径配置正确"
            ])
            
        elif error_type == 'compatibility_error':
            recommendations.extend([
                "更新PyTorch和Transformers版本",
                "检查CUDA和cuDNN版本兼容性",
                "确认模型格式与框架版本匹配",
                "查看官方文档的版本要求"
            ])
            
        # 添加通用建议
        recommendations.extend([
            "查看完整的错误堆栈信息",
            "检查系统日志获取更多信息",
            "尝试在不同环境中重现问题",
            "联系模型提供商获取支持"
        ])
        
        return recommendations

# 使用示例
debugger = ModelLoadingDebugger()

try:
    # 尝试加载模型
    model = load_model("path/to/model")
except Exception as e:
    # 诊断失败原因
    diagnosis = debugger.diagnose_loading_failure("path/to/model", str(e))
    
    print("=== 模型加载故障诊断报告 ===")
    print(f"错误类型: {diagnosis['error_analysis']['type']}")
    print(f"错误详情: {diagnosis['error_analysis']['message']}")
    print("\n系统资源:")
    print(f"可用内存: {diagnosis['resource_check']['memory_available_gb']:.2f} GB")
    print(f"GPU状态: {[gpu['name'] for gpu in diagnosis['resource_check']['gpus']]}")
    print("\n解决建议:")
    for i, rec in enumerate(diagnosis['recommendations'], 1):
        print(f"{i}. {rec}")
```

### 3.2 推理性能问题分析

```python
# inference_performance_analyzer.py
import time
import threading
import statistics
from concurrent.futures import ThreadPoolExecutor
import matplotlib.pyplot as plt
import numpy as np

class InferencePerformanceAnalyzer:
    def __init__(self, endpoint_url, model_name):
        self.endpoint_url = endpoint_url
        self.model_name = model_name
        self.metrics = {
            'latencies': [],
            'throughputs': [],
            'errors': [],
            'resource_usage': []
        }
        
    def stress_test(self, duration_seconds=300, concurrent_requests=10, payload=None):
        """执行压力测试"""
        
        print(f"开始压力测试: {duration_seconds}秒, {concurrent_requests}并发")
        
        start_time = time.time()
        end_time = start_time + duration_seconds
        
        # 测试负载
        default_payload = {
            "prompt": "Hello, how are you?",
            "max_tokens": 100,
            "temperature": 0.7
        }
        test_payload = payload or default_payload
        
        # 并发执行测试
        with ThreadPoolExecutor(max_workers=concurrent_requests) as executor:
            futures = []
            
            while time.time() < end_time:
                future = executor.submit(self._single_request, test_payload)
                futures.append(future)
                
                # 控制请求频率
                time.sleep(0.1)
                
        # 收集结果
        for future in futures:
            try:
                result = future.result(timeout=30)
                if result:
                    self.metrics['latencies'].append(result['latency'])
                    self.metrics['throughputs'].append(result['throughput'])
                    if result['error']:
                        self.metrics['errors'].append(result['error'])
            except Exception as e:
                self.metrics['errors'].append(str(e))
                
        self._analyze_results()
        
    def _single_request(self, payload):
        """执行单次推理请求"""
        
        import requests
        import json
        
        start_time = time.time()
        
        try:
            response = requests.post(
                self.endpoint_url,
                json=payload,
                timeout=30
            )
            
            end_time = time.time()
            latency = (end_time - start_time) * 1000  # 转换为毫秒
            
            if response.status_code == 200:
                # 计算吞吐量（tokens/second）
                response_data = response.json()
                output_tokens = len(response_data.get('generated_text', '').split())
                throughput = output_tokens / (latency / 1000) if latency > 0 else 0
                
                return {
                    'latency': latency,
                    'throughput': throughput,
                    'error': None,
                    'response_size': len(json.dumps(response_data))
                }
            else:
                return {
                    'latency': latency,
                    'throughput': 0,
                    'error': f"HTTP {response.status_code}",
                    'response_size': 0
                }
                
        except Exception as e:
            end_time = time.time()
            latency = (end_time - start_time) * 1000
            return {
                'latency': latency,
                'throughput': 0,
                'error': str(e),
                'response_size': 0
            }
            
    def _analyze_results(self):
        """分析测试结果"""
        
        latencies = self.metrics['latencies']
        errors = self.metrics['errors']
        
        if not latencies:
            print("没有成功请求的数据")
            return
            
        # 基本统计
        stats = {
            'total_requests': len(latencies) + len(errors),
            'successful_requests': len(latencies),
            'error_rate': len(errors) / (len(latencies) + len(errors)) * 100,
            'avg_latency': statistics.mean(latencies),
            'median_latency': statistics.median(latencies),
            'p95_latency': np.percentile(latencies, 95),
            'p99_latency': np.percentile(latencies, 99),
            'min_latency': min(latencies),
            'max_latency': max(latencies),
            'std_deviation': statistics.stdev(latencies)
        }
        
        # 吞吐量统计
        throughputs = self.metrics['throughputs']
        if throughputs:
            stats.update({
                'avg_throughput': statistics.mean(throughputs),
                'max_throughput': max(throughputs),
                'min_throughput': min(throughputs)
            })
            
        self._print_analysis(stats)
        self._generate_charts(stats)
        
    def _print_analysis(self, stats):
        """打印分析结果"""
        
        print("\n=== 推理性能分析报告 ===")
        print(f"模型: {self.model_name}")
        print(f"测试时间: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        print("\n--- 性能指标 ---")
        print(f"总请求数: {stats['total_requests']}")
        print(f"成功请求数: {stats['successful_requests']}")
        print(f"错误率: {stats['error_rate']:.2f}%")
        print(f"\n延迟统计 (ms):")
        print(f"  平均延迟: {stats['avg_latency']:.2f}")
        print(f"  中位数延迟: {stats['median_latency']:.2f}")
        print(f"  P95延迟: {stats['p95_latency']:.2f}")
        print(f"  P99延迟: {stats['p99_latency']:.2f}")
        print(f"  最小延迟: {stats['min_latency']:.2f}")
        print(f"  最大延迟: {stats['max_latency']:.2f}")
        print(f"  标准差: {stats['std_deviation']:.2f}")
        
        if 'avg_throughput' in stats:
            print(f"\n吞吐量统计 (tokens/sec):")
            print(f"  平均吞吐量: {stats['avg_throughput']:.2f}")
            print(f"  最大吞吐量: {stats['max_throughput']:.2f}")
            print(f"  最小吞吐量: {stats['min_throughput']:.2f}")
            
        # 性能评级
        self._performance_rating(stats)
        
    def _performance_rating(self, stats):
        """性能评级"""
        
        ratings = []
        
        # 延迟评级
        avg_latency = stats['avg_latency']
        if avg_latency < 100:
            ratings.append(("延迟", "优秀 ⭐⭐⭐⭐⭐"))
        elif avg_latency < 300:
            ratings.append(("延迟", "良好 ⭐⭐⭐⭐"))
        elif avg_latency < 500:
            ratings.append(("延迟", "一般 ⭐⭐⭐"))
        else:
            ratings.append(("延迟", "较差 ⭐⭐"))
            
        # 错误率评级
        error_rate = stats['error_rate']
        if error_rate == 0:
            ratings.append(("稳定性", "优秀 ⭐⭐⭐⭐⭐"))
        elif error_rate < 1:
            ratings.append(("稳定性", "良好 ⭐⭐⭐⭐"))
        elif error_rate < 5:
            ratings.append(("稳定性", "一般 ⭐⭐⭐"))
        else:
            ratings.append(("稳定性", "较差 ⭐⭐"))
            
        print("\n--- 性能评级 ---")
        for metric, rating in ratings:
            print(f"  {metric}: {rating}")
            
    def _generate_charts(self, stats):
        """生成性能图表"""
        
        latencies = sorted(self.metrics['latencies'])
        percentiles = [i/len(latencies)*100 for i in range(len(latencies))]
        
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        
        # 延迟分布直方图
        ax1.hist(latencies, bins=50, alpha=0.7, color='blue')
        ax1.set_xlabel('延迟 (ms)')
        ax1.set_ylabel('频次')
        ax1.set_title('延迟分布')
        ax1.axvline(stats['avg_latency'], color='red', linestyle='--', 
                   label=f'平均: {stats["avg_latency"]:.1f}ms')
        ax1.axvline(stats['p95_latency'], color='orange', linestyle='--',
                   label=f'P95: {stats["p95_latency"]:.1f}ms')
        ax1.legend()
        
        # 累积分布函数
        ax2.plot(percentiles, latencies)
        ax2.set_xlabel('百分位 (%)')
        ax2.set_ylabel('延迟 (ms)')
        ax2.set_title('延迟累积分布')
        ax2.grid(True)
        
        # 吞吐量趋势（如果有时间序列数据）
        if len(self.metrics['throughputs']) > 1:
            throughputs = self.metrics['throughputs']
            time_points = range(len(throughputs))
            ax3.plot(time_points, throughputs, marker='o', markersize=2)
            ax3.set_xlabel('请求序号')
            ax3.set_ylabel('吞吐量 (tokens/sec)')
            ax3.set_title('吞吐量趋势')
            ax3.grid(True)
            
        # 错误分析饼图
        error_categories = {}
        for error in self.metrics['errors']:
            category = error.split(':')[0] if ':' in error else 'Other'
            error_categories[category] = error_categories.get(category, 0) + 1
            
        if error_categories:
            ax4.pie(error_categories.values(), labels=error_categories.keys(), autopct='%1.1f%%')
            ax4.set_title('错误类型分布')
        else:
            ax4.text(0.5, 0.5, '无错误', ha='center', va='center', transform=ax4.transAxes)
            ax4.set_title('错误分析')
            
        plt.tight_layout()
        plt.savefig(f'inference_performance_{int(time.time())}.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print(f"\n性能图表已保存为: inference_performance_{int(time.time())}.png")

# 使用示例
analyzer = InferencePerformanceAnalyzer(
    endpoint_url="http://model-service.ai-models.svc.cluster.local:8000/v1/completions",
    model_name="llama3-70b"
)

# 执行压力测试
analyzer.stress_test(
    duration_seconds=300,  # 5分钟测试
    concurrent_requests=20,  # 20并发
    payload={
        "prompt": "Explain quantum computing in simple terms:",
        "max_tokens": 200,
        "temperature": 0.7
    }
)
```

---

## 四、监控告警体系建设

### 4.1 关键指标告警配置

```yaml
# ai_monitoring_alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-platform-alerts
  namespace: monitoring
spec:
  groups:
  # GPU相关告警
  - name: ai.gpu.alerts
    rules:
    - alert: HighGPUUtilization
      expr: avg by(instance, gpu)(DCGM_FI_DEV_GPU_UTIL) > 95
      for: 5m
      labels:
        severity: warning
        team: ai-platform
      annotations:
        summary: "GPU利用率过高 ({{ $labels.instance }} GPU {{ $labels.gpu }})"
        description: "GPU利用率持续超过95%，可能导致性能瓶颈"
        
    - alert: HighGPUTemperature
      expr: DCGM_FI_DEV_GPU_TEMP > 80
      for: 2m
      labels:
        severity: critical
        team: ai-platform
      annotations:
        summary: "GPU温度过高 ({{ $labels.instance }})"
        description: "GPU温度达到{{ $value }}°C，可能影响稳定性和寿命"
        
    - alert: GPUMemoryPressure
      expr: (DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL) * 100 > 90
      for: 3m
      labels:
        severity: warning
        team: ai-platform
      annotations:
        summary: "GPU内存压力过大"
        description: "GPU内存使用率{{ $value | printf \"%.2f\" }}%，接近上限"
        
  # 模型服务告警
  - name: ai.model.alerts
    rules:
    - alert: HighInferenceLatency
      expr: histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le, service)) > 0.5
      for: 3m
      labels:
        severity: warning
        service: inference
      annotations:
        summary: "推理延迟过高 ({{ $labels.service }})"
        description: "P99延迟超过500ms，当前值: {{ $value | printf \"%.3f\" }}s"
        
    - alert: ModelServiceDown
      expr: up{job=~"model-.*"} == 0
      for: 1m
      labels:
        severity: critical
        team: ai-platform
      annotations:
        summary: "模型服务不可用"
        description: "服务 {{ $labels.job }} 无法访问"
        
    - alert: HighModelErrorRate
      expr: sum(rate(model_errors_total[5m])) / sum(rate(model_requests_total[5m])) > 0.05
      for: 2m
      labels:
        severity: critical
        service: model-serving
      annotations:
        summary: "模型错误率异常"
        description: "错误率超过5%，当前值: {{ $value | printf \"%.2f\" }}%"
        
  # 训练任务告警
  - name: ai.training.alerts
    rules:
    - alert: TrainingJobStuck
      expr: kube_job_status_active{job_name=~"training-.*"} > 0
      for: 30m
      labels:
        severity: warning
        team: ml-engineering
      annotations:
        summary: "训练任务卡住"
        description: "训练任务 {{ $labels.job_name }} 已运行超过30分钟无进展"
        
    - alert: TrainingLossNaN
      expr: training_loss{status="invalid"} > 0
      for: 1m
      labels:
        severity: critical
        team: ml-engineering
      annotations:
        summary: "训练损失出现NaN值"
        description: "训练过程中出现无效的损失值，请检查数据和模型配置"
        
  # 存储告警
  - name: ai.storage.alerts
    rules:
    - alert: LowDiskSpace
      expr: (node_filesystem_avail_bytes / node_filesystem_size_bytes) * 100 < 10
      for: 5m
      labels:
        severity: critical
        team: ai-platform
      annotations:
        summary: "磁盘空间不足"
        description: "节点 {{ $labels.instance }} 可用空间低于10%"
        
    - alert: HighStorageLatency
      expr: rate(node_disk_read_time_seconds_total[1m]) / rate(node_disk_reads_completed_total[1m]) > 0.1
      for: 3m
      labels:
        severity: warning
        team: ai-platform
      annotations:
        summary: "存储延迟过高"
        description: "存储I/O延迟超过100ms"
```

### 4.2 自动化诊断工具

```python
# automated_diagnostic_tool.py
import subprocess
import json
import yaml
from datetime import datetime
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

class AutomatedDiagnosticTool:
    def __init__(self, config_file="diagnostic_config.yaml"):
        self.config = self._load_config(config_file)
        self.results = {}
        
    def _load_config(self, config_file):
        """加载诊断配置"""
        with open(config_file, 'r') as f:
            return yaml.safe_load(f)
            
    def run_full_diagnosis(self):
        """运行完整诊断"""
        
        print("开始AI平台全面诊断...")
        start_time = datetime.now()
        
        # 1. 基础设施检查
        self.results['infrastructure'] = self._check_infrastructure()
        
        # 2. Kubernetes集群状态
        self.results['kubernetes'] = self._check_kubernetes()
        
        # 3. AI服务状态
        self.results['ai_services'] = self._check_ai_services()
        
        # 4. 性能指标分析
        self.results['performance'] = self._analyze_performance()
        
        # 5. 安全检查
        self.results['security'] = self._check_security()
        
        # 6. 生成报告
        self._generate_report()
        
        # 7. 发送告警（如有问题）
        if self._has_critical_issues():
            self._send_alerts()
            
        end_time = datetime.now()
        print(f"诊断完成，耗时: {end_time - start_time}")
        
        return self.results
        
    def _check_infrastructure(self):
        """检查基础设施状态"""
        
        checks = {}
        
        # GPU状态检查
        try:
            gpu_output = subprocess.check_output(['nvidia-smi', '--query-gpu=name,temperature.gpu,utilization.gpu,memory.used,memory.total', '--format=csv,noheader,nounits'], text=True)
            checks['gpus'] = []
            for line in gpu_output.strip().split('\n'):
                parts = line.split(', ')
                checks['gpus'].append({
                    'name': parts[0],
                    'temperature': int(parts[1]),
                    'utilization': int(parts[2]),
                    'memory_used': int(parts[3]),
                    'memory_total': int(parts[4])
                })
        except subprocess.CalledProcessError:
            checks['gpus'] = '无法获取GPU信息'
            
        # 存储状态检查
        try:
            df_output = subprocess.check_output(['df', '-h'], text=True)
            checks['storage'] = df_output
        except subprocess.CalledProcessError:
            checks['storage'] = '无法获取存储信息'
            
        # 网络连通性检查
        checks['network'] = self._check_network_connectivity()
        
        return checks
        
    def _check_kubernetes(self):
        """检查Kubernetes集群状态"""
        
        checks = {}
        
        # 节点状态
        try:
            nodes_output = subprocess.check_output(['kubectl', 'get', 'nodes', '-o', 'wide'], text=True)
            checks['nodes'] = nodes_output
        except subprocess.CalledProcessError:
            checks['nodes'] = '无法获取节点信息'
            
        # Pod状态（AI相关）
        try:
            pods_output = subprocess.check_output(['kubectl', 'get', 'pods', '-n', 'ai-models', '-o', 'wide'], text=True)
            checks['ai_pods'] = pods_output
        except subprocess.CalledProcessError:
            checks['ai_pods'] = '无法获取AI Pods信息'
            
        # 资源使用情况
        try:
            top_output = subprocess.check_output(['kubectl', 'top', 'nodes'], text=True)
            checks['resource_usage'] = top_output
        except subprocess.CalledProcessError:
            checks['resource_usage'] = '无法获取资源使用信息'
            
        return checks
        
    def _check_ai_services(self):
        """检查AI服务状态"""
        
        services = ['model-registry', 'inference-service', 'training-operator']
        checks = {}
        
        for service in services:
            try:
                # 检查服务端点
                endpoints_output = subprocess.check_output(['kubectl', 'get', 'endpoints', '-n', 'ai-models', service], text=True)
                checks[f'{service}_endpoint'] = '正常' if 'ports' in endpoints_output else '异常'
                
                # 检查服务日志中的错误
                logs_output = subprocess.check_output(['kubectl', 'logs', '-n', 'ai-models', f'deployment/{service}', '--tail=100'], text=True)
                error_count = logs_output.count('ERROR') + logs_output.count('Exception')
                checks[f'{service}_errors'] = error_count
                
            except subprocess.CalledProcessError as e:
                checks[f'{service}_endpoint'] = f'检查失败: {str(e)}'
                checks[f'{service}_errors'] = -1
                
        return checks
        
    def _analyze_performance(self):
        """分析性能指标"""
        
        # 从Prometheus获取指标（需要配置Prometheus地址）
        performance_data = {}
        
        # 这里可以集成Prometheus API调用
        # 示例指标检查
        performance_data['recent_issues'] = self._get_recent_performance_issues()
        
        return performance_data
        
    def _check_security(self):
        """安全检查"""
        
        security_checks = {}
        
        # 检查Pod安全策略
        try:
            psp_output = subprocess.check_output(['kubectl', 'get', 'podsecuritypolicies'], text=True)
            security_checks['pod_security_policies'] = '存在' if 'NAME' in psp_output else '缺失'
        except subprocess.CalledProcessError:
            security_checks['pod_security_policies'] = '检查失败'
            
        # 检查网络策略
        try:
            netpol_output = subprocess.check_output(['kubectl', 'get', 'networkpolicies', '--all-namespaces'], text=True)
            security_checks['network_policies'] = '存在' if 'NAME' in netpol_output else '缺失'
        except subprocess.CalledProcessError:
            security_checks['network_policies'] = '检查失败'
            
        return security_checks
        
    def _check_network_connectivity(self):
        """检查网络连通性"""
        
        targets = ['google.com', 'github.com', 'internal-registry.local']
        results = {}
        
        for target in targets:
            try:
                subprocess.check_output(['ping', '-c', '3', target], stderr=subprocess.STDOUT)
                results[target] = '可达'
            except subprocess.CalledProcessError:
                results[target] = '不可达'
                
        return results
        
    def _get_recent_performance_issues(self):
        """获取最近的性能问题"""
        # 模拟数据，实际应该从监控系统获取
        return [
            {'type': 'high_latency', 'timestamp': '2024-01-15T10:30:00Z', 'severity': 'warning'},
            {'type': 'low_throughput', 'timestamp': '2024-01-15T09:15:00Z', 'severity': 'info'}
        ]
        
    def _has_critical_issues(self):
        """检查是否存在严重问题"""
        
        # 简化的严重问题判断逻辑
        critical_indicators = [
            'gpus' in self.results.get('infrastructure', {}) and isinstance(self.results['infrastructure']['gpus'], str),
            'nodes' in self.results.get('kubernetes', {}) and 'NotReady' in self.results['kubernetes']['nodes'],
            any(service.endswith('_endpoint') and self.results['ai_services'][service] != '正常' 
                for service in self.results.get('ai_services', {}))
        ]
        
        return any(critical_indicators)
        
    def _generate_report(self):
        """生成诊断报告"""
        
        report = {
            'timestamp': datetime.now().isoformat(),
            'summary': self._generate_summary(),
            'detailed_results': self.results
        }
        
        # 保存报告
        with open(f'ai_diagnostic_report_{datetime.now().strftime("%Y%m%d_%H%M%S")}.json', 'w') as f:
            json.dump(report, f, indent=2, default=str)
            
        # 生成HTML报告
        self._generate_html_report(report)
        
    def _generate_summary(self):
        """生成摘要"""
        
        issues = []
        
        # 基础设施问题
        infra = self.results.get('infrastructure', {})
        if isinstance(infra.get('gpus'), str):
            issues.append("GPU状态异常")
            
        # Kubernetes问题
        k8s = self.results.get('kubernetes', {})
        if 'NotReady' in k8s.get('nodes', ''):
            issues.append("Kubernetes节点异常")
            
        # AI服务问题
        ai_services = self.results.get('ai_services', {})
        for service, status in ai_services.items():
            if service.endswith('_endpoint') and status != '正常':
                issues.append(f"{service.replace('_endpoint', '')}服务异常")
                
        return {
            'total_checks': len(self.results),
            'issues_found': len(issues),
            'critical_issues': issues,
            'overall_status': 'HEALTHY' if not issues else 'WARNING'
        }
        
    def _generate_html_report(self, report):
        """生成HTML格式报告"""
        
        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>AI平台诊断报告</title>
            <style>
                body {{ font-family: Arial, sans-serif; margin: 20px; }}
                .header {{ background-color: #f0f0f0; padding: 10px; border-radius: 5px; }}
                .section {{ margin: 20px 0; }}
                .issue {{ color: red; font-weight: bold; }}
                .ok {{ color: green; }}
                pre {{ background-color: #f5f5f5; padding: 10px; overflow-x: auto; }}
            </style>
        </head>
        <body>
            <div class="header">
                <h1>AI平台诊断报告</h1>
                <p>生成时间: {report['timestamp']}</p>
                <p>总体状态: <span class="{report['summary']['overall_status'].lower()}">{report['summary']['overall_status']}</span></p>
                <p>发现问题: {report['summary']['issues_found']} 个</p>
            </div>
            
            <div class="section">
                <h2>关键问题</h2>
                <ul>
        """
        
        for issue in report['summary']['critical_issues']:
            html_content += f"<li class='issue'>{issue}</li>"
            
        html_content += """
                </ul>
            </div>
            
            <div class="section">
                <h2>详细结果</h2>
                <pre>
        """
        
        html_content += json.dumps(report['detailed_results'], indent=2, default=str)
        html_content += """
                </pre>
            </div>
        </body>
        </html>
        """
        
        filename = f'ai_diagnostic_report_{datetime.now().strftime("%Y%m%d_%H%M%S")}.html'
        with open(filename, 'w') as f:
            f.write(html_content)
            
        print(f"HTML报告已生成: {filename}")
        
    def _send_alerts(self):
        """发送告警邮件"""
        
        # 邮件配置
        smtp_server = self.config.get('smtp_server', 'localhost')
        smtp_port = self.config.get('smtp_port', 587)
        sender_email = self.config.get('sender_email', 'alerts@company.com')
        recipient_emails = self.config.get('recipient_emails', [])
        
        if not recipient_emails:
            print("未配置收件人邮箱")
            return
            
        # 创建邮件
        msg = MIMEMultipart()
        msg['From'] = sender_email
        msg['To'] = ', '.join(recipient_emails)
        msg['Subject'] = f"AI平台诊断告警 - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
        
        body = f"""
        AI平台诊断发现严重问题：
        
        {chr(10).join(self.results['summary']['critical_issues'])}
        
        请及时处理。
        
        详细报告请查看附件。
        """
        
        msg.attach(MIMEText(body, 'plain'))
        
        # 发送邮件
        try:
            server = smtplib.SMTP(smtp_server, smtp_port)
            server.starttls()
            # 如果需要认证，取消下面注释并填写凭据
            # server.login(sender_email, "your_password")
            server.send_message(msg)
            server.quit()
            print("告警邮件已发送")
        except Exception as e:
            print(f"发送邮件失败: {e}")

# 使用示例
if __name__ == "__main__":
    diagnostic = AutomatedDiagnosticTool("diagnostic_config.yaml")
    results = diagnostic.run_full_diagnosis()
    print("诊断完成，结果已保存")
```

---

## 五、性能优化最佳实践

### 5.1 系统级优化

```bash
#!/bin/bash
# system_optimization.sh - 系统性能优化脚本

echo "开始系统性能优化..."

# 1. 内核参数优化
echo "优化内核参数..."
cat >> /etc/sysctl.conf << EOF
# 网络优化
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = bbr

# 文件系统优化
fs.file-max = 2097152
fs.nr_open = 2097152

# 内存优化
vm.swappiness = 1
vm.dirty_ratio = 15
vm.dirty_background_ratio = 5
EOF

sysctl -p

# 2. GPU驱动优化
echo "优化GPU驱动设置..."
cat > /etc/modprobe.d/nvidia.conf << EOF
options nvidia NVreg_RegistryDwords="PerfLevelSrc=0x2222"
options nvidia NVreg_RestrictProfilingToAdminUsers=0
options nvidia NVreg_EnableGpuFirmware=1
EOF

# 3. systemd服务优化
echo "优化systemd服务..."
mkdir -p /etc/systemd/system/docker.service.d
cat > /etc/systemd/system/docker.service.d/override.conf << EOF
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd --data-root /fast-ssd/docker --log-driver=json-file --log-opt max-size=100m --log-opt max-file=3
EOF

systemctl daemon-reload
systemctl restart docker

# 4. Kubernetes组件优化
echo "优化Kubernetes组件..."
# kubelet配置优化
cat > /var/lib/kubelet/config.yaml << EOF
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
maxPods: 110
podPidsLimit: 4096
serializeImagePulls: false
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
featureGates:
  CPUManager: true
  MemoryManager: true
cpuManagerPolicy: static
memoryManagerPolicy: Static
reservedSystemCPUs: "0,1"
systemReserved:
  cpu: "1"
  memory: "4Gi"
kubeReserved:
  cpu: "1"
  memory: "4Gi"
EOF

echo "系统优化完成！请重启相关服务使配置生效。"
```

### 5.2 应用级优化清单

✅ **模型优化**
- [ ] 启用模型量化（FP16/INT8）
- [ ] 使用模型并行减少单GPU压力
- [ ] 实施动态批处理提高吞吐量
- [ ] 启用KV缓存复用
- [ ] 优化注意力机制（Flash Attention）

✅ **容器优化**
- [ ] 设置合理的资源限制和请求
- [ ] 启用节点亲和性和反亲和性
- [ ] 配置合适的探针超时时间
- [ ] 优化镜像层数和大小
- [ ] 启用镜像预拉取

✅ **网络优化**
- [ ] 启用服务网格sidecar注入
- [ ] 配置合理的连接池参数
- [ ] 启用HTTP/2和gRPC
- [ ] 优化DNS解析配置
- [ ] 启用连接复用

---