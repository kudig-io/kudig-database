# 控制平面性能基准测试 (Control Plane Performance Benchmarking)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 性能测试指南

---

## 目录

1. [性能测试框架](#1-性能测试框架)
2. [核心组件基准测试](#2-核心组件基准测试)
3. [负载测试场景](#3-负载测试场景)
4. [性能指标定义](#4-性能指标定义)
5. [测试工具集](#5-测试工具集)
6. [结果分析方法](#6-结果分析方法)
7. [性能优化建议](#7-性能优化建议)
8. [容量规划指南](#8-容量规划指南)
9. [持续性能监控](#9-持续性能监控)
10. [测试最佳实践](#10-测试最佳实践)

---

## 1. 性能测试框架

### 1.1 测试架构设计

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Performance Testing Framework                           │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  测试环境层 (Test Environment Layer)                                            │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Test Infrastructure                            │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Control     │ │ Worker      │ │ Load        │ │ Monitoring          ││    │
│  │  │ Plane Nodes │ │ Nodes       │ │ Generator   │ │ System              ││    │
│  │  │ (3+)        │ │ (N)         │ │             │ │                     ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│          │               │               │               │                      │
│          ▼               ▼               ▼               ▼                      │
│  测试执行层 (Test Execution Layer)                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Test Orchestration                             │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Test Runner │ │ Scenario    │ │ Data        │ │ Result              ││    │
│  │  │             │ │ Executor    │ │ Collector   │ │ Aggregator          ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│          │                                                                      │
│          ▼                                                                      │
│  结果分析层 (Result Analysis Layer)                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                          Analytics Engine                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Statistical │ │ Trend       │ │ Comparison  │ │ Reporting           ││    │
│  │  │ Analysis    │ │ Analysis    │ │ Engine      │ │ Engine              ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 测试分类体系

| 测试类型 | 目标 | 频率 | 复杂度 |
|----------|------|------|--------|
| **基准测试** | 建立性能基线 | 部署后首次 | 中 |
| **回归测试** | 验证性能不变 | 每次升级 | 中 |
| **压力测试** | 确定极限容量 | 季度 | 高 |
| **负载测试** | 验证预期性能 | 月度 | 中 |
| **稳定性测试** | 长期性能表现 | 年度 | 高 |

---

## 2. 核心组件基准测试

### 2.1 API Server性能测试

```bash
#!/bin/bash
# API Server性能基准测试脚本

TEST_DURATION=300  # 5分钟测试
CONCURRENT_USERS=100
API_SERVER_ENDPOINT="https://kubernetes.default.svc:443"

# 1. 基准测试配置
setup_benchmark() {
    echo "Setting up API Server benchmark..."
    
    # 创建测试命名空间
    kubectl create namespace perf-test --dry-run=client -o yaml | kubectl apply -f -
    
    # 部署测试应用
    cat > /tmp/test-deployment.yaml << EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: perf-test-app
  namespace: perf-test
spec:
  replicas: 50
  selector:
    matchLabels:
      app: perf-test
  template:
    metadata:
      labels:
        app: perf-test
    spec:
      containers:
      - name: test-app
        image: nginx:latest
        resources:
          requests:
            memory: "64Mi"
            cpu: "250m"
EOF
    
    kubectl apply -f /tmp/test-deployment.yaml
    kubectl wait --for=condition=available --timeout=300s deployment/perf-test-app -n perf-test
}

# 2. API延迟测试
test_api_latency() {
    echo "Testing API latency..."
    
    # GET请求延迟测试
    echo "Testing GET requests..."
    for i in {1..1000}; do
        start_time=$(date +%s%3N)
        kubectl get pods -n perf-test >/dev/null 2>&1
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/get-latency.txt
    done
    
    # POST请求延迟测试
    echo "Testing POST requests..."
    for i in {1..500}; do
        start_time=$(date +%s%3N)
        cat > /tmp/temp-pod.yaml << EOF
apiVersion: v1
kind: Pod
metadata:
  name: temp-pod-$i
  namespace: perf-test
spec:
  containers:
  - name: temp
    image: busybox
    command: ["sleep", "3600"]
EOF
        kubectl apply -f /tmp/temp-pod.yaml >/dev/null 2>&1
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/post-latency.txt
    done
}

# 3. 并发性能测试
test_concurrent_performance() {
    echo "Testing concurrent performance..."
    
    # 使用hey工具进行并发测试
    hey -z ${TEST_DURATION}s -c ${CONCURRENT_USERS} \
        -H "Authorization: Bearer $(kubectl create token default -n perf-test)" \
        "${API_SERVER_ENDPOINT}/api/v1/namespaces/perf-test/pods" \
        > /tmp/concurrent-results.txt 2>&1
}

# 4. 结果分析
analyze_results() {
    echo "Analyzing results..."
    
    # GET延迟统计
    echo "GET Request Latency Statistics:"
    awk '{sum+=$1; sumsq+=$1*$1} END {printf "Avg: %.2fms, StdDev: %.2fms, Min: %.2fms, Max: %.2fms\n", sum/NR, sqrt(sumsq/NR - (sum/NR)^2), min, max}' \
        min=$(sort -n /tmp/get-latency.txt | head -1) \
        max=$(sort -n /tmp/get-latency.txt | tail -1) \
        /tmp/get-latency.txt
    
    # POST延迟统计
    echo "POST Request Latency Statistics:"
    awk '{sum+=$1; sumsq+=$1*$1} END {printf "Avg: %.2fms, StdDev: %.2fms, Min: %.2fms, Max: %.2fms\n", sum/NR, sqrt(sumsq/NR - (sum/NR)^2), min, max}' \
        min=$(sort -n /tmp/post-latency.txt | head -1) \
        max=$(sort -n /tmp/post-latency.txt | tail -1) \
        /tmp/post-latency.txt
    
    # 并发测试结果
    echo "Concurrent Performance Results:"
    grep "Requests/sec" /tmp/concurrent-results.txt
    grep "Latency" /tmp/concurrent-results.txt
}

# 执行测试
setup_benchmark
test_api_latency
test_concurrent_performance
analyze_results
```

### 2.2 etcd性能基准

```bash
#!/bin/bash
# etcd性能基准测试

ETCD_ENDPOINTS="https://etcd-0.etcd:2379,https://etcd-1.etcd:2379,https://etcd-2.etcd:2379"
ETCD_CERTS="--cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key"

# 1. 写入性能测试
test_etcd_write_performance() {
    echo "Testing etcd write performance..."
    
    # 小对象写入测试 (1KB)
    echo "Testing small object writes (1KB)..."
    for i in {1..10000}; do
        value=$(openssl rand -hex 512)  # 1KB数据
        start_time=$(date +%s%3N)
        ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS put "test/small/$i" "$value" >/dev/null
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/etcd-small-write.txt
    done
    
    # 大对象写入测试 (1MB)
    echo "Testing large object writes (1MB)..."
    for i in {1..1000}; do
        value=$(openssl rand -hex 524288)  # 1MB数据
        start_time=$(date +%s%3N)
        ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS put "test/large/$i" "$value" >/dev/null
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/etcd-large-write.txt
    done
}

# 2. 读取性能测试
test_etcd_read_performance() {
    echo "Testing etcd read performance..."
    
    # 小对象读取测试
    echo "Testing small object reads..."
    for i in {1..10000}; do
        start_time=$(date +%s%3N)
        ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS get "test/small/$i" >/dev/null
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/etcd-small-read.txt
    done
    
    # 大对象读取测试
    echo "Testing large object reads..."
    for i in {1..1000}; do
        start_time=$(date +%s%3N)
        ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS get "test/large/$i" >/dev/null
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/etcd-large-read.txt
    done
}

# 3. 并发性能测试
test_etcd_concurrent_performance() {
    echo "Testing etcd concurrent performance..."
    
    # 使用etcd提供的性能测试工具
    ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS check perf --load="s" > /tmp/etcd-perf-short.txt
    ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS check perf --load="m" > /tmp/etcd-perf-medium.txt
    ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS check perf --load="l" > /tmp/etcd-perf-large.txt
}

# 4. 集群健康测试
test_etcd_cluster_health() {
    echo "Testing etcd cluster health..."
    
    # 延迟测试
    for i in {1..100}; do
        ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS endpoint health --cluster
        sleep 1
    done > /tmp/etcd-health-test.txt
    
    # 状态一致性检查
    ETCDCTL_API=3 etcdctl --endpoints=$ETCD_ENDPOINTS $ETCD_CERTS endpoint status --cluster -w table > /tmp/etcd-status.txt
}

# 5. 结果汇总
summarize_etcd_results() {
    echo "=== etcd Performance Summary ==="
    
    echo "Small Object Write Latency:"
    awk '{sum+=$1} END {print "Average:", sum/NR "ms"}' /tmp/etcd-small-write.txt
    
    echo "Large Object Write Latency:"
    awk '{sum+=$1} END {print "Average:", sum/NR "ms"}' /tmp/etcd-large-write.txt
    
    echo "Small Object Read Latency:"
    awk '{sum+=$1} END {print "Average:", sum/NR "ms"}' /tmp/etcd-small-read.txt
    
    echo "Large Object Read Latency:"
    awk '{sum+=$1} END {print "Average:", sum/NR "ms"}' /tmp/etcd-large-read.txt
    
    echo "Performance Test Results:"
    cat /tmp/etcd-perf-*.txt
}

# 执行测试
test_etcd_write_performance
test_etcd_read_performance
test_etcd_concurrent_performance
test_etcd_cluster_health
summarize_etcd_results
```

### 2.3 控制器性能测试

```python
#!/usr/bin/env python3
# 控制器性能测试脚本

import subprocess
import time
import threading
import statistics
from datetime import datetime

class ControllerPerformanceTester:
    def __init__(self):
        self.results = {
            'deployment_sync': [],
            'replicaset_sync': [],
            'pod_creation': [],
            'event_processing': []
        }
    
    def create_test_deployments(self, count=100):
        """创建测试用Deployment"""
        print(f"Creating {count} test deployments...")
        
        for i in range(count):
            deployment_yaml = f"""
apiVersion: apps/v1
kind: Deployment
metadata:
  name: test-deployment-{i}
  namespace: perf-test
spec:
  replicas: 3
  selector:
    matchLabels:
      app: test-app-{i}
  template:
    metadata:
      labels:
        app: test-app-{i}
    spec:
      containers:
      - name: test-container
        image: nginx:latest
        resources:
          requests:
            memory: "32Mi"
            cpu: "100m"
"""
            with open(f'/tmp/deployment-{i}.yaml', 'w') as f:
                f.write(deployment_yaml)
            
            subprocess.run(['kubectl', 'apply', '-f', f'/tmp/deployment-{i}.yaml'], 
                         capture_output=True)
    
    def measure_deployment_sync_time(self):
        """测量Deployment同步时间"""
        print("Measuring deployment sync time...")
        
        start_time = time.time()
        # 等待所有Deployment可用
        subprocess.run([
            'kubectl', 'wait', '--for=condition=available', 
            '--timeout=300s', 'deployment', '--all', '-n', 'perf-test'
        ])
        end_time = time.time()
        
        sync_time = end_time - start_time
        self.results['deployment_sync'].append(sync_time)
        print(f"Deployment sync time: {sync_time:.2f}s")
    
    def measure_replicaset_sync_time(self):
        """测量ReplicaSet同步时间"""
        print("Measuring replicaset sync time...")
        
        # 获取ReplicaSet数量
        result = subprocess.run([
            'kubectl', 'get', 'rs', '-n', 'perf-test', 
            '-o', 'jsonpath={.items[*].metadata.name}'
        ], capture_output=True, text=True)
        
        rs_count = len(result.stdout.split())
        print(f"Found {rs_count} ReplicaSets")
        
        # 测量同步时间
        start_time = time.time()
        subprocess.run([
            'kubectl', 'wait', '--for=condition=complete', 
            '--timeout=300s', 'rs', '--all', '-n', 'perf-test'
        ])
        end_time = time.time()
        
        sync_time = end_time - start_time
        self.results['replicaset_sync'].append(sync_time)
        print(f"ReplicaSet sync time: {sync_time:.2f}s")
    
    def measure_pod_creation_time(self):
        """测量Pod创建时间"""
        print("Measuring pod creation time...")
        
        # 获取Pod总数
        result = subprocess.run([
            'kubectl', 'get', 'pods', '-n', 'perf-test', 
            '-o', 'jsonpath={.items[*].metadata.name}'
        ], capture_output=True, text=True)
        
        pod_names = result.stdout.split()
        print(f"Found {len(pod_names)} pods")
        
        # 测量每个Pod的创建时间
        for pod_name in pod_names[:10]:  # 采样前10个
            start_time = time.time()
            subprocess.run([
                'kubectl', 'wait', '--for=condition=Ready', 
                f'pod/{pod_name}', '-n', 'perf-test', '--timeout=300s'
            ])
            end_time = time.time()
            
            creation_time = end_time - start_time
            self.results['pod_creation'].append(creation_time)
            print(f"Pod {pod_name} creation time: {creation_time:.2f}s")
    
    def monitor_event_processing(self, duration=300):
        """监控事件处理性能"""
        print("Monitoring event processing...")
        
        start_time = time.time()
        events_processed = 0
        
        while time.time() - start_time < duration:
            # 获取事件计数
            result = subprocess.run([
                'kubectl', 'get', 'events', '-n', 'perf-test', 
                '--no-headers'
            ], capture_output=True, text=True)
            
            current_events = len(result.stdout.split('\n')) - 1
            if current_events > events_processed:
                self.results['event_processing'].append(current_events - events_processed)
                events_processed = current_events
            
            time.sleep(5)  # 每5秒采样一次
    
    def run_comprehensive_test(self):
        """运行综合性能测试"""
        print("Starting comprehensive controller performance test...")
        
        # 创建测试环境
        subprocess.run(['kubectl', 'create', 'namespace', 'perf-test', '--dry-run=client', '-o', 'yaml'], 
                      capture_output=True, text=True)
        subprocess.run(['kubectl', 'apply', '-f', '-'], input='''apiVersion: v1
kind: Namespace
metadata:
  name: perf-test''')
        
        # 并行执行各项测试
        threads = []
        
        # 部署创建和同步测试
        thread1 = threading.Thread(target=self.create_test_deployments, args=(50,))
        thread2 = threading.Thread(target=self.measure_deployment_sync_time)
        thread3 = threading.Thread(target=self.measure_replicaset_sync_time)
        thread4 = threading.Thread(target=self.measure_pod_creation_time)
        thread5 = threading.Thread(target=self.monitor_event_processing, args=(300,))
        
        threads.extend([thread1, thread2, thread3, thread4, thread5])
        
        for thread in threads:
            thread.start()
        
        for thread in threads:
            thread.join()
        
        self.generate_report()
    
    def generate_report(self):
        """生成测试报告"""
        print("\n=== Controller Performance Report ===")
        print(f"Test completed at: {datetime.now()}")
        print()
        
        for test_type, measurements in self.results.items():
            if measurements:
                avg_time = statistics.mean(measurements)
                if len(measurements) > 1:
                    std_dev = statistics.stdev(measurements)
                    print(f"{test_type.replace('_', ' ').title()}:")
                    print(f"  Average: {avg_time:.2f}s")
                    print(f"  Std Dev: {std_dev:.2f}s")
                    print(f"  Samples: {len(measurements)}")
                else:
                    print(f"{test_type.replace('_', ' ').title()}: {avg_time:.2f}s")
                print()

# 运行测试
if __name__ == "__main__":
    tester = ControllerPerformanceTester()
    tester.run_comprehensive_test()
```

---

## 3. 负载测试场景

### 3.1 典型负载模式

```yaml
# 负载测试场景配置
load_test_scenarios:
  steady_state:
    description: "稳定状态下的正常负载"
    concurrent_users: 50
    request_rate: "100 req/s"
    duration: "1h"
    expected_metrics:
      api_latency_p95: "< 100ms"
      error_rate: "< 0.1%"
      throughput: "> 80 req/s"
  
  peak_load:
    description: "高峰期突发负载"
    concurrent_users: 500
    request_rate: "1000 req/s"
    duration: "15m"
    ramp_up_time: "5m"
    expected_metrics:
      api_latency_p95: "< 500ms"
      error_rate: "< 1%"
      throughput: "> 800 req/s"
  
  sustained_high_load:
    description: "持续高负载运行"
    concurrent_users: 200
    request_rate: "500 req/s"
    duration: "8h"
    expected_metrics:
      api_latency_p95: "< 200ms"
      error_rate: "< 0.5%"
      memory_growth: "< 10%"
  
  mixed_workload:
    description: "混合读写操作负载"
    read_ratio: 0.7
    write_ratio: 0.3
    concurrent_users: 100
    request_rate: "200 req/s"
    duration: "2h"
    expected_metrics:
      read_latency_p95: "< 50ms"
      write_latency_p95: "< 200ms"
      error_rate: "< 0.2%"
```

### 3.2 负载生成脚本

```python
#!/usr/bin/env python3
# 负载生成器

import asyncio
import aiohttp
import time
import random
import statistics
from datetime import datetime, timedelta

class LoadGenerator:
    def __init__(self, api_server_url, token, namespace="perf-test"):
        self.api_server_url = api_server_url
        self.token = token
        self.namespace = namespace
        self.metrics = {
            'latencies': [],
            'errors': 0,
            'success': 0,
            'start_time': None,
            'end_time': None
        }
    
    async def make_request(self, session, url, method='GET'):
        """执行单个API请求"""
        start_time = time.time()
        try:
            headers = {'Authorization': f'Bearer {self.token}'}
            
            if method == 'GET':
                async with session.get(url, headers=headers, timeout=30) as response:
                    await response.read()
            elif method == 'POST':
                # 创建简单的Pod
                pod_data = {
                    "apiVersion": "v1",
                    "kind": "Pod",
                    "metadata": {
                        "generateName": "load-test-pod-"
                    },
                    "spec": {
                        "containers": [{
                            "name": "test",
                            "image": "nginx:latest"
                        }]
                    }
                }
                async with session.post(url, json=pod_data, headers=headers, timeout=30) as response:
                    await response.read()
            
            end_time = time.time()
            latency = (end_time - start_time) * 1000  # 转换为毫秒
            self.metrics['latencies'].append(latency)
            
            if response.status < 400:
                self.metrics['success'] += 1
            else:
                self.metrics['errors'] += 1
                
        except Exception as e:
            end_time = time.time()
            latency = (end_time - start_time) * 1000
            self.metrics['latencies'].append(latency)
            self.metrics['errors'] += 1
    
    async def load_test_worker(self, session, duration, requests_per_second):
        """负载测试工作线程"""
        end_time = time.time() + duration
        request_interval = 1.0 / requests_per_second
        
        while time.time() < end_time:
            start = time.time()
            
            # 随机选择读写操作
            if random.random() < 0.7:  # 70% 读操作
                url = f"{self.api_server_url}/api/v1/namespaces/{self.namespace}/pods"
                await self.make_request(session, url, 'GET')
            else:  # 30% 写操作
                url = f"{self.api_server_url}/api/v1/namespaces/{self.namespace}/pods"
                await self.make_request(session, url, 'POST')
            
            # 控制请求频率
            elapsed = time.time() - start
            if elapsed < request_interval:
                await asyncio.sleep(request_interval - elapsed)
    
    async def run_load_test(self, concurrent_users=50, requests_per_second=100, duration=300):
        """运行负载测试"""
        self.metrics['start_time'] = datetime.now()
        
        print(f"Starting load test with {concurrent_users} users at {requests_per_second} req/s for {duration}s")
        
        connector = aiohttp.TCPConnector(limit=concurrent_users, ttl_dns_cache=300)
        timeout = aiohttp.ClientTimeout(total=30)
        
        async with aiohttp.ClientSession(connector=connector, timeout=timeout) as session:
            # 创建并发任务
            tasks = []
            for _ in range(concurrent_users):
                task = asyncio.create_task(
                    self.load_test_worker(session, duration, requests_per_second/concurrent_users)
                )
                tasks.append(task)
            
            # 等待所有任务完成
            await asyncio.gather(*tasks, return_exceptions=True)
        
        self.metrics['end_time'] = datetime.now()
        self.print_results()
    
    def print_results(self):
        """打印测试结果"""
        print("\n=== Load Test Results ===")
        print(f"Start Time: {self.metrics['start_time']}")
        print(f"End Time: {self.metrics['end_time']}")
        print(f"Duration: {self.metrics['end_time'] - self.metrics['start_time']}")
        print(f"Successful Requests: {self.metrics['success']}")
        print(f"Failed Requests: {self.metrics['errors']}")
        
        total_requests = self.metrics['success'] + self.metrics['errors']
        if total_requests > 0:
            error_rate = (self.metrics['errors'] / total_requests) * 100
            print(f"Error Rate: {error_rate:.2f}%")
            
            if self.metrics['latencies']:
                latencies = sorted(self.metrics['latencies'])
                print(f"Latency Statistics:")
                print(f"  Average: {statistics.mean(latencies):.2f}ms")
                print(f"  Median: {statistics.median(latencies):.2f}ms")
                print(f"  95th Percentile: {latencies[int(len(latencies) * 0.95)]:.2f}ms")
                print(f"  99th Percentile: {latencies[int(len(latencies) * 0.99)]:.2f}ms")
                print(f"  Min: {min(latencies):.2f}ms")
                print(f"  Max: {max(latencies):.2f}ms")

# 使用示例
async def main():
    # 获取访问令牌
    import subprocess
    result = subprocess.run(['kubectl', 'create', 'token', 'default', '-n', 'perf-test'], 
                          capture_output=True, text=True)
    token = result.stdout.strip()
    
    generator = LoadGenerator(
        api_server_url="https://kubernetes.default.svc:443",
        token=token
    )
    
    # 运行负载测试
    await generator.run_load_test(
        concurrent_users=100,
        requests_per_second=200,
        duration=300  # 5分钟
    )

if __name__ == "__main__":
    asyncio.run(main())
```

---

## 4. 性能指标定义

### 4.1 关键性能指标(KPIs)

| 指标类别 | 指标名称 | 基准值 | 目标值 | 测量方法 |
|----------|----------|--------|--------|----------|
| **API性能** | P95延迟 | < 100ms | < 50ms | Prometheus直方图 |
| **API性能** | 吞吐量 | > 1000 req/s | > 2000 req/s | 请求计数器 |
| **API性能** | 错误率 | < 0.1% | < 0.01% | 状态码统计 |
| **etcd性能** | 写入延迟 | < 10ms | < 5ms | etcd指标 |
| **etcd性能** | 读取延迟 | < 5ms | < 2ms | etcd指标 |
| **etcd性能** | 集群健康 | 100% | 100% | 健康检查 |
| **控制器性能** | 同步延迟 | < 30s | < 10s | 控制器指标 |
| **控制器性能** | 事件处理率 | > 1000 events/s | > 2000 events/s | 事件计数器 |
| **调度器性能** | 调度延迟 | < 5s | < 2s | 调度器指标 |
| **资源使用** | CPU使用率 | < 70% | < 50% | 资源指标 |
| **资源使用** | 内存使用率 | < 80% | < 60% | 资源指标 |

### 4.2 SLI/SLO定义

```yaml
# 服务水平指标和目标
service_level_objectives:
  api_server:
    availability_slo:
      target: 99.9%
      window: "30d"
      measurement: "uptime / total_time"
    
    latency_sli:
      target: 95%
      threshold: "100ms"
      window: "1h"
      measurement: "requests_with_latency < 100ms / total_requests"
    
    reliability_slo:
      target: 99.95%
      window: "7d"
      measurement: "successful_requests / total_requests"
  
  etcd:
    durability_slo:
      target: 99.99%
      window: "30d"
      measurement: "successful_writes / total_writes"
    
    consistency_slo:
      target: 100%
      window: "1h"
      measurement: "consistent_reads / total_reads"
  
  control_plane:
    recovery_time_slo:
      target: 95%
      threshold: "300s"
      window: "1y"
      measurement: "incidents_resolved_within_300s / total_incidents"
```

---

## 5. 测试工具集

### 5.1 核心测试工具

```bash
#!/bin/bash
# 性能测试工具集安装脚本

# 1. 安装必备工具
install_performance_tools() {
    echo "Installing performance testing tools..."
    
    # 安装hey (HTTP负载测试工具)
    wget https://hey-release.s3.us-east-2.amazonaws.com/hey_linux_amd64
    chmod +x hey_linux_amd64
    sudo mv hey_linux_amd64 /usr/local/bin/hey
    
    # 安装vegeta (HTTP负载测试工具)
    wget https://github.com/tsenart/vegeta/releases/download/v12.8.4/vegeta_12.8.4_linux_amd64.tar.gz
    tar -xzf vegeta_12.8.4_linux_amd64.tar.gz
    sudo mv vegeta /usr/local/bin/
    
    # 安装wrk (HTTP基准测试工具)
    sudo apt-get update
    sudo apt-get install -y wrk
    
    # 安装k6 (现代负载测试工具)
    curl -OL https://github.com/grafana/k6/releases/latest/download/k6-linux-amd64.tar.gz
    tar -xzf k6-linux-amd64.tar.gz
    sudo mv k6-linux-amd64 /usr/local/bin/k6
}

# 2. 安装监控工具
install_monitoring_tools() {
    echo "Installing monitoring tools..."
    
    # 安装Prometheus exporter工具
    wget https://github.com/prometheus/node_exporter/releases/download/v1.6.1/node_exporter-1.6.1.linux-amd64.tar.gz
    tar -xzf node_exporter-1.6.1.linux-amd64.tar.gz
    sudo mv node_exporter-1.6.1.linux-amd64/node_exporter /usr/local/bin/
    
    # 安装kube-state-metrics
    kubectl apply -f https://github.com/kubernetes/kube-state-metrics/releases/latest/download/kube-state-metrics.yaml
}

# 3. 安装分析工具
install_analysis_tools() {
    echo "Installing analysis tools..."
    
    # 安装Python数据分析库
    pip3 install numpy pandas matplotlib seaborn scipy
    
    # 安装Jupyter notebook
    pip3 install jupyter
    
    # 安装Grafana
    sudo apt-get install -y apt-transport-https software-properties-common
    wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
    echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee -a /etc/apt/sources.list.d/grafana.list
    sudo apt-get update
    sudo apt-get install -y grafana
}

# 4. 创建测试环境
setup_test_environment() {
    echo "Setting up test environment..."
    
    # 创建测试命名空间
    kubectl create namespace perf-test --dry-run=client -o yaml | kubectl apply -f -
    
    # 部署监控堆栈
    kubectl apply -f https://raw.githubusercontent.com/prometheus-operator/kube-prometheus/main/manifests/setup
    kubectl apply -f https://raw.githubusercontent.com/prometheus-operator/kube-prometheus/main/manifests/
    
    # 等待组件就绪
    kubectl wait --for=condition=available --timeout=300s deployment/prometheus-k8s -n monitoring
    kubectl wait --for=condition=available --timeout=300s deployment/grafana -n monitoring
}

# 执行安装
install_performance_tools
install_monitoring_tools
install_analysis_tools
setup_test_environment

echo "Performance testing environment setup complete!"
```

### 5.2 自定义测试工具

```python
#!/usr/bin/env python3
# Kubernetes性能测试框架

import subprocess
import time
import json
import statistics
from datetime import datetime
from typing import Dict, List, Any

class K8sPerformanceTester:
    def __init__(self, namespace: str = "perf-test"):
        self.namespace = namespace
        self.test_results: Dict[str, List[float]] = {}
        self.timestamps: List[datetime] = []
    
    def run_api_benchmark(self, iterations: int = 1000) -> Dict[str, Any]:
        """运行API基准测试"""
        print("Running API benchmark...")
        
        latencies = []
        errors = 0
        successes = 0
        
        for i in range(iterations):
            start_time = time.time()
            try:
                result = subprocess.run([
                    'kubectl', 'get', 'pods', '-n', self.namespace
                ], capture_output=True, timeout=30)
                
                end_time = time.time()
                latency = (end_time - start_time) * 1000  # ms
                latencies.append(latency)
                
                if result.returncode == 0:
                    successes += 1
                else:
                    errors += 1
                    
            except subprocess.TimeoutExpired:
                end_time = time.time()
                latency = (end_time - start_time) * 1000
                latencies.append(latency)
                errors += 1
            except Exception as e:
                errors += 1
        
        # 计算统计指标
        stats = {
            'total_requests': iterations,
            'success_rate': successes / iterations * 100,
            'error_rate': errors / iterations * 100,
            'latency_stats': {
                'mean': statistics.mean(latencies),
                'median': statistics.median(latencies),
                'p95': sorted(latencies)[int(len(latencies) * 0.95)],
                'p99': sorted(latencies)[int(len(latencies) * 0.99)],
                'min': min(latencies),
                'max': max(latencies),
                'std_dev': statistics.stdev(latencies) if len(latencies) > 1 else 0
            }
        }
        
        self.test_results['api_benchmark'] = latencies
        return stats
    
    def run_etcd_benchmark(self) -> Dict[str, Any]:
        """运行etcd基准测试"""
        print("Running etcd benchmark...")
        
        # 使用etcd自带的性能测试工具
        try:
            result = subprocess.run([
                'ETCDCTL_API=3', 'etcdctl', 'check', 'perf', '--load=s'
            ], capture_output=True, text=True, shell=True, timeout=300)
            
            # 解析结果
            output = result.stdout
            stats = {}
            
            # 提取关键指标
            for line in output.split('\n'):
                if 'average request latency' in line:
                    stats['avg_latency'] = float(line.split()[-2])
                elif 'requests per second' in line:
                    stats['throughput'] = float(line.split()[-2])
                elif '99th percentile latency' in line:
                    stats['p99_latency'] = float(line.split()[-2])
            
            return stats
            
        except Exception as e:
            return {'error': str(e)}
    
    def run_scheduler_benchmark(self, pod_count: int = 100) -> Dict[str, Any]:
        """运行调度器基准测试"""
        print(f"Running scheduler benchmark with {pod_count} pods...")
        
        # 创建测试Deployment
        deployment_yaml = f"""
apiVersion: apps/v1
kind: Deployment
metadata:
  name: scheduler-benchmark
  namespace: {self.namespace}
spec:
  replicas: {pod_count}
  selector:
    matchLabels:
      app: scheduler-benchmark
  template:
    metadata:
      labels:
        app: scheduler-benchmark
    spec:
      containers:
      - name: bench
        image: nginx:latest
        resources:
          requests:
            memory: "32Mi"
            cpu: "50m"
"""
        
        start_time = time.time()
        
        # 应用Deployment
        with open('/tmp/scheduler-bench.yaml', 'w') as f:
            f.write(deployment_yaml)
        
        subprocess.run(['kubectl', 'apply', '-f', '/tmp/scheduler-bench.yaml'])
        
        # 等待所有Pod就绪
        subprocess.run([
            'kubectl', 'wait', '--for=condition=available', 
            'deployment/scheduler-benchmark', '-n', self.namespace, '--timeout=600s'
        ])
        
        end_time = time.time()
        
        # 清理资源
        subprocess.run(['kubectl', 'delete', 'deployment', 'scheduler-benchmark', '-n', self.namespace])
        
        return {
            'total_pods': pod_count,
            'scheduling_time': end_time - start_time,
            'avg_scheduling_time': (end_time - start_time) / pod_count
        }
    
    def generate_comprehensive_report(self) -> str:
        """生成综合性能报告"""
        report = {
            'timestamp': datetime.now().isoformat(),
            'namespace': self.namespace,
            'tests': {}
        }
        
        # 运行各项测试
        report['tests']['api_benchmark'] = self.run_api_benchmark()
        report['tests']['etcd_benchmark'] = self.run_etcd_benchmark()
        report['tests']['scheduler_benchmark'] = self.run_scheduler_benchmark()
        
        # 保存报告
        report_file = f"/tmp/k8s-performance-report-{datetime.now().strftime('%Y%m%d-%H%M%S')}.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        return report_file

# 使用示例
if __name__ == "__main__":
    tester = K8sPerformanceTester()
    report_file = tester.generate_comprehensive_report()
    print(f"Performance report saved to: {report_file}")
```

---

## 6. 结果分析方法

### 6.1 统计分析方法

```python
#!/usr/bin/env python3
# 性能测试结果分析工具

import json
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats
from datetime import datetime
import pandas as pd

class PerformanceAnalyzer:
    def __init__(self, report_file: str):
        with open(report_file, 'r') as f:
            self.data = json.load(f)
        self.results_df = None
    
    def load_test_data(self):
        """加载测试数据到DataFrame"""
        test_data = []
        
        for test_name, test_results in self.data['tests'].items():
            if 'latency_stats' in test_results:
                stats = test_results['latency_stats']
                test_data.append({
                    'test_name': test_name,
                    'metric': 'latency',
                    'mean': stats['mean'],
                    'median': stats['median'],
                    'p95': stats['p95'],
                    'p99': stats['p99'],
                    'min': stats['min'],
                    'max': stats['max'],
                    'std_dev': stats['std_dev'],
                    'success_rate': test_results['success_rate'],
                    'error_rate': test_results['error_rate']
                })
        
        self.results_df = pd.DataFrame(test_data)
    
    def statistical_analysis(self):
        """统计分析"""
        print("=== Statistical Analysis ===")
        
        if self.results_df is not None:
            # 描述性统计
            print("Descriptive Statistics:")
            print(self.results_df[['mean', 'median', 'p95', 'p99']].describe())
            
            # 正态性检验
            print("\nNormality Tests:")
            for _, row in self.results_df.iterrows():
                if len(self.data['tests'][row['test_name']]['latencies']) > 8:
                    stat, p_value = stats.shapiro(
                        self.data['tests'][row['test_name']]['latencies'][:5000]
                    )
                    print(f"{row['test_name']}: Shapiro-Wilk p-value = {p_value:.6f}")
            
            # 异常值检测
            print("\nOutlier Detection:")
            for _, row in self.results_df.iterrows():
                latencies = self.data['tests'][row['test_name']]['latencies']
                Q1 = np.percentile(latencies, 25)
                Q3 = np.percentile(latencies, 75)
                IQR = Q3 - Q1
                outliers = [x for x in latencies if x < (Q1 - 1.5 * IQR) or x > (Q3 + 1.5 * IQR)]
                print(f"{row['test_name']}: {len(outliers)} outliers detected ({len(outliers)/len(latencies)*100:.2f}%)")
    
    def visualization(self):
        """数据可视化"""
        if self.results_df is None:
            self.load_test_data()
        
        # 设置图表样式
        plt.style.use('seaborn-v0_8')
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        
        # 1. 延迟分布对比
        axes[0, 0].bar(self.results_df['test_name'], self.results_df['mean'], 
                      yerr=self.results_df['std_dev'], capsize=5)
        axes[0, 0].set_title('Average Latency by Test Type')
        axes[0, 0].set_ylabel('Latency (ms)')
        axes[0, 0].tick_params(axis='x', rotation=45)
        
        # 2. 百分位数对比
        percentiles_data = self.results_df[['test_name', 'p95', 'p99']].melt(
            id_vars=['test_name'], var_name='percentile', value_name='latency'
        )
        sns.barplot(data=percentiles_data, x='test_name', y='latency', hue='percentile', ax=axes[0, 1])
        axes[0, 1].set_title('Latency Percentiles')
        axes[0, 1].set_ylabel('Latency (ms)')
        axes[0, 1].tick_params(axis='x', rotation=45)
        
        # 3. 成功率对比
        axes[1, 0].bar(self.results_df['test_name'], self.results_df['success_rate'])
        axes[1, 0].set_title('Success Rate by Test Type')
        axes[1, 0].set_ylabel('Success Rate (%)')
        axes[1, 0].tick_params(axis='x', rotation=45)
        
        # 4. 错误率对比
        axes[1, 1].bar(self.results_df['test_name'], self.results_df['error_rate'])
        axes[1, 1].set_title('Error Rate by Test Type')
        axes[1, 1].set_ylabel('Error Rate (%)')
        axes[1, 1].tick_params(axis='x', rotation=45)
        
        plt.tight_layout()
        plt.savefig(f'/tmp/performance-analysis-{datetime.now().strftime("%Y%m%d-%H%M%S")}.png', 
                   dpi=300, bbox_inches='tight')
        plt.show()
    
    def generate_recommendations(self):
        """生成优化建议"""
        print("\n=== Performance Recommendations ===")
        
        recommendations = []
        
        for _, row in self.results_df.iterrows():
            # 延迟相关建议
            if row['p95'] > 100:
                recommendations.append(f"High {row['test_name']} latency detected ({row['p95']:.1f}ms). Consider scaling API Server.")
            
            if row['p99'] > 500:
                recommendations.append(f"Very high {row['test_name']} tail latency ({row['p99']:.1f}ms). Investigate outliers.")
            
            # 成功率相关建议
            if row['success_rate'] < 99.9:
                recommendations.append(f"Low success rate for {row['test_name']} ({row['success_rate']:.2f}%). Check error logs.")
            
            if row['error_rate'] > 0.1:
                recommendations.append(f"High error rate for {row['test_name']} ({row['error_rate']:.2f}%). Review system health.")
        
        # 通用建议
        avg_success = self.results_df['success_rate'].mean()
        if avg_success < 99.5:
            recommendations.append("Overall system reliability below target. Consider implementing circuit breakers.")
        
        for rec in recommendations:
            print(f"• {rec}")
        
        return recommendations

# 使用示例
if __name__ == "__main__":
    analyzer = PerformanceAnalyzer('/tmp/k8s-performance-report-20240101-120000.json')
    analyzer.load_test_data()
    analyzer.statistical_analysis()
    analyzer.visualization()
    analyzer.generate_recommendations()
```

---

## 7. 性能优化建议

### 7.1 API Server优化

```yaml
# API Server性能优化配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver-optimized
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.30.0
    command:
    - kube-apiserver
    # 性能优化参数
    - --max-requests-inflight=3000          # 增加并发请求数
    - --max-mutating-requests-inflight=1000  # 增加变更请求并发数
    - --request-timeout=2m                  # 增加请求超时时间
    - --min-request-timeout=1800            # 增加最小超时时间
    
    # 缓存优化
    - --default-watch-cache-size=5000       # 增加Watch缓存大小
    - --watch-cache-sizes=                   # 为热点资源设置更大缓存
      pods#10000,
      nodes#2000,
      services#5000,
      endpoints#10000,
      configmaps#5000,
      secrets#5000
    
    # etcd优化
    - --etcd-compaction-interval=5m         # 优化压缩间隔
    - --etcd-count-metric-poll-period=30s   # 优化指标轮询
    - --storage-media-type=application/vnd.kubernetes.protobuf  # 使用Protobuf格式
    
    # 特性门控
    - --feature-gates=                      # 启用性能相关特性
      APIPriorityAndFairness=true,
      ServerSideApply=true,
      WatchBookmark=true
    
    resources:
      requests:
        cpu: "2000m"
        memory: "8Gi"
      limits:
        cpu: "4000m"
        memory: "16Gi"
```

### 7.2 etcd优化

```bash
#!/bin/bash
# etcd性能优化脚本

# 1. 系统级优化
optimize_system_settings() {
    echo "Optimizing system settings..."
    
    # 调整文件描述符限制
    echo "* soft nofile 1048576" >> /etc/security/limits.conf
    echo "* hard nofile 1048576" >> /etc/security/limits.conf
    
    # 调整网络参数
    cat >> /etc/sysctl.conf << EOF
# etcd优化参数
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = bbr
net.ipv4.tcp_slow_start_after_idle = 0
EOF
    
    sysctl -p
}

# 2. etcd配置优化
optimize_etcd_config() {
    echo "Optimizing etcd configuration..."
    
    # 备份原始配置
    cp /etc/kubernetes/manifests/etcd.yaml /etc/kubernetes/manifests/etcd.yaml.backup
    
    # 应用优化配置
    cat > /etc/kubernetes/manifests/etcd-optimized.yaml << EOF
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    image: quay.io/coreos/etcd:v3.5.12
    command:
    - etcd
    # 性能优化参数
    - --quota-backend-bytes=8589934592        # 8GB存储配额
    - --max-request-bytes=10485760            # 10MB最大请求
    - --heartbeat-interval=100                # 优化心跳间隔
    - --election-timeout=1000                 # 优化选举超时
    - --snapshot-count=10000                  # 优化快照触发
    - --auto-compaction-retention=1h          # 优化自动压缩
    - --max-txn-ops=10000                     # 增加事务操作数
    - --max-watcher-per-resource=10000        # 增加每资源watcher数
    
    # 网络优化
    - --grpc-keepalive-min-time=5s
    - --grpc-keepalive-interval=2h
    - --grpc-keepalive-timeout=20s
    
    resources:
      requests:
        cpu: "2000m"
        memory: "8Gi"
      limits:
        cpu: "4000m"
        memory: "16Gi"
EOF
    
    # 重启etcd应用新配置
    mv /etc/kubernetes/manifests/etcd-optimized.yaml /etc/kubernetes/manifests/etcd.yaml
}

# 3. 存储优化
optimize_storage() {
    echo "Optimizing storage..."
    
    # 检查存储类型
    STORAGE_TYPE=$(lsblk -o NAME,ROTA | grep -v ROTA | grep -v NAME | head -1 | awk '{print $2}')
    
    if [ "$STORAGE_TYPE" = "0" ]; then
        echo "NVMe/SSD storage detected, applying optimizations..."
        # SSD优化参数
        echo 'deadline' > /sys/block/nvme0n1/queue/scheduler
        echo 1 > /sys/block/nvme0n1/queue/nomerges
    else
        echo "HDD storage detected, applying different optimizations..."
        # HDD优化参数
        echo 'cfq' > /sys/block/sda/queue/scheduler
    fi
}

# 执行优化
optimize_system_settings
optimize_etcd_config
optimize_storage

echo "etcd performance optimization completed!"
```

---

## 8. 容量规划指南

### 8.1 容量规划模型

```python
#!/usr/bin/env python3
# Kubernetes容量规划工具

import math
from typing import Dict, List
from dataclasses import dataclass

@dataclass
class CapacityRequirements:
    nodes: int
    pods: int
    api_requests_per_second: int
    etcd_storage_gb: float
    control_plane_cpu_cores: int
    control_plane_memory_gb: int

class CapacityPlanner:
    def __init__(self):
        # 基准性能数据 (基于测试结果)
        self.benchmarks = {
            'api_server': {
                'req_per_core': 1000,  # 每核每秒请求数
                'memory_per_req_mb': 5,  # 每请求内存需求
                'latency_target_ms': 50
            },
            'etcd': {
                'writes_per_second_per_gb': 1000,  # 每GB每秒写入数
                'reads_per_second_per_gb': 5000,   # 每GB每秒读取数
                'storage_growth_rate': 0.1  # 每月增长10%
            },
            'controller_manager': {
                'objects_per_core': 10000,  # 每核可管理对象数
                'memory_per_object_mb': 0.1  # 每对象内存需求
            }
        }
    
    def calculate_capacity(self, workload_profile: Dict) -> CapacityRequirements:
        """计算容量需求"""
        
        # 1. 基于Pod数量计算
        pods = workload_profile.get('expected_pods', 1000)
        nodes = math.ceil(pods / workload_profile.get('pods_per_node', 110))
        
        # 2. 计算API请求量
        api_rps = (
            pods * workload_profile.get('api_calls_per_pod_per_hour', 60) / 3600 +
            nodes * workload_profile.get('node_api_calls_per_hour', 120) / 3600 +
            workload_profile.get('external_api_calls_per_hour', 1000) / 3600
        )
        
        # 3. 计算etcd存储需求
        base_storage = workload_profile.get('base_etcd_storage_gb', 2)
        growth_months = workload_profile.get('planning_horizon_months', 12)
        etcd_storage = base_storage * (1 + self.benchmarks['etcd']['storage_growth_rate']) ** growth_months
        
        # 4. 计算控制平面资源需求
        # API Server需求
        api_cores = math.ceil(api_rps / self.benchmarks['api_server']['req_per_core'] * 1.5)  # 50%冗余
        api_memory = math.ceil((api_rps * self.benchmarks['api_server']['memory_per_req_mb'] * 1.5) / 1024)
        
        # Controller Manager需求
        controller_objects = pods + nodes + 1000  # pods + nodes + 其他资源
        controller_cores = math.ceil(controller_objects / self.benchmarks['controller_manager']['objects_per_core'])
        controller_memory = math.ceil(
            (controller_objects * self.benchmarks['controller_manager']['memory_per_object_mb']) / 1024
        )
        
        # 总控制平面需求
        total_cores = max(api_cores, controller_cores) * 3  # 3节点HA
        total_memory = max(api_memory, controller_memory) * 3
        
        return CapacityRequirements(
            nodes=nodes,
            pods=pods,
            api_requests_per_second=int(api_rps),
            etcd_storage_gb=round(etcd_storage, 2),
            control_plane_cpu_cores=total_cores,
            control_plane_memory_gb=total_memory
        )
    
    def generate_scaling_recommendations(self, current_capacity: CapacityRequirements, 
                                       projected_growth: float) -> List[str]:
        """生成扩展建议"""
        recommendations = []
        
        # 计算增长后的需求
        projected_pods = int(current_capacity.pods * (1 + projected_growth))
        projected_nodes = math.ceil(projected_pods / 110)
        projected_rps = int(current_capacity.api_requests_per_second * (1 + projected_growth))
        
        if projected_nodes > current_capacity.nodes * 1.3:
            recommendations.append(f"Plan to add {(projected_nodes - current_capacity.nodes)} more worker nodes")
        
        if projected_rps > current_capacity.api_requests_per_second * 1.5:
            recommendations.append(f"Increase control plane capacity to handle {projected_rps} req/s")
        
        projected_etcd_storage = current_capacity.etcd_storage_gb * (1 + projected_growth * 0.5)
        if projected_etcd_storage > current_capacity.etcd_storage_gb * 1.5:
            recommendations.append(f"Plan etcd storage expansion to {projected_etcd_storage:.1f}GB")
        
        return recommendations

# 使用示例
if __name__ == "__main__":
    planner = CapacityPlanner()
    
    # 定义工作负载配置
    workload = {
        'expected_pods': 5000,
        'pods_per_node': 110,
        'api_calls_per_pod_per_hour': 120,
        'node_api_calls_per_hour': 240,
        'external_api_calls_per_hour': 2000,
        'base_etcd_storage_gb': 5,
        'planning_horizon_months': 12
    }
    
    # 计算容量需求
    capacity = planner.calculate_capacity(workload)
    
    print("=== Capacity Planning Results ===")
    print(f"Required Nodes: {capacity.nodes}")
    print(f"Expected Pods: {capacity.pods}")
    print(f"API Requests/sec: {capacity.api_requests_per_second}")
    print(f"etcd Storage: {capacity.etcd_storage_gb}GB")
    print(f"Control Plane CPU: {capacity.control_plane_cpu_cores} cores")
    print(f"Control Plane Memory: {capacity.control_plane_memory_gb}GB")
    
    # 生成扩展建议
    recommendations = planner.generate_scaling_recommendations(capacity, 0.5)  # 50%增长
    print("\n=== Scaling Recommendations ===")
    for rec in recommendations:
        print(f"• {rec}")
```

---

## 9. 持续性能监控

### 9.1 性能监控仪表板

```yaml
# Grafana性能监控仪表板配置
apiVersion: integreatly.org/v1alpha1
kind: GrafanaDashboard
metadata:
  name: k8s-performance-dashboard
  namespace: monitoring
spec:
  json: |
    {
      "dashboard": {
        "id": null,
        "title": "Kubernetes Performance Monitoring",
        "timezone": "browser",
        "schemaVersion": 16,
        "version": 0,
        "refresh": "30s",
        "panels": [
          {
            "type": "graph",
            "title": "API Server Performance",
            "gridPos": {"h": 8, "w": 12, "x": 0, "y": 0},
            "targets": [
              {
                "expr": "histogram_quantile(0.95, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le))",
                "legendFormat": "95th Percentile"
              },
              {
                "expr": "histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le))",
                "legendFormat": "99th Percentile"
              }
            ],
            "yaxes": [{"format": "s", "label": "Latency"}]
          },
          {
            "type": "stat",
            "title": "Current API Throughput",
            "gridPos": {"h": 4, "w": 6, "x": 12, "y": 0},
            "targets": [
              {
                "expr": "sum(rate(apiserver_request_total[5m]))",
                "instant": true
              }
            ],
            "format": "none"
          },
          {
            "type": "graph",
            "title": "etcd Performance",
            "gridPos": {"h": 8, "w": 12, "x": 0, "y": 8},
            "targets": [
              {
                "expr": "histogram_quantile(0.95, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m]))",
                "legendFormat": "Commit Latency 95th"
              },
              {
                "expr": "histogram_quantile(0.95, rate(etcd_network_peer_round_trip_time_seconds_bucket[5m]))",
                "legendFormat": "Network RTT 95th"
              }
            ],
            "yaxes": [{"format": "s", "label": "Latency"}]
          },
          {
            "type": "graph",
            "title": "Control Plane Resource Usage",
            "gridPos": {"h": 8, "w": 12, "x": 0, "y": 16},
            "targets": [
              {
                "expr": "sum(rate(container_cpu_usage_seconds_total{namespace=\"kube-system\"}[5m])) by (pod)",
                "legendFormat": "{{pod}}"
              }
            ],
            "yaxes": [{"format": "short", "label": "CPU Cores"}]
          }
        ]
      }
    }
```

### 9.2 自动化性能告警

```yaml
# 性能告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: performance-alerts
  namespace: monitoring
spec:
  groups:
  - name: performance.alerts
    rules:
    # API Server性能告警
    - alert: APIServerHighLatency
      expr: histogram_quantile(0.95, rate(apiserver_request_duration_seconds_bucket[5m])) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "API Server latency is high"
        description: "95th percentile latency is above 100ms"
    
    - alert: APILowThroughput
      expr: sum(rate(apiserver_request_total[5m])) < 100
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "API Server throughput is low"
        description: "Request rate dropped below 100 req/s"
    
    # etcd性能告警
    - alert: EtcdHighCommitLatency
      expr: histogram_quantile(0.95, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.05
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "etcd commit latency is high"
        description: "95th percentile commit latency above 50ms"
    
    # 控制平面资源告警
    - alert: ControlPlaneHighCPU
      expr: sum(rate(container_cpu_usage_seconds_total{namespace="kube-system"}[5m])) > 10
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Control plane CPU usage is high"
        description: "Total CPU usage above 10 cores"
    
    - alert: ControlPlaneHighMemory
      expr: sum(container_memory_usage_bytes{namespace="kube-system"}) > 21474836480  # 20GB
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Control plane memory usage is high"
        description: "Total memory usage above 20GB"
```

---

## 10. 测试最佳实践

### 10.1 测试执行清单

```yaml
# 性能测试执行清单
performance_test_checklist:
  pre_test_preparation:
    - [ ] 确认测试环境隔离
    - [ ] 备份生产数据
    - [ ] 验证测试工具安装
    - [ ] 准备监控系统
    - [ ] 定义成功标准
    - [ ] 通知相关团队
    
  test_execution:
    - [ ] 执行基准测试
    - [ ] 运行负载测试
    - [ ] 监控系统指标
    - [ ] 记录异常事件
    - [ ] 收集性能数据
    - [ ] 验证测试结果
    
  post_test_activities:
    - [ ] 分析测试结果
    - [ ] 生成测试报告
    - [ ] 识别性能瓶颈
    - [ ] 提出优化建议
    - [ ] 清理测试资源
    - [ ] 更新文档
```

### 10.2 持续改进循环

```
持续性能改进循环:

┌─────────────────────────────────────────────────────────────────────────┐
│                      Continuous Performance Improvement                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. 基线建立 (Baseline Establishment)                                   │
│     ├── 执行初始性能测试                                                 │
│     ├── 建立性能基线指标                                                 │
│     └── 定义SLI/SLO目标                                                  │
│        │                                                                 │
│        ▼                                                                 │
│  2. 定期测试 (Regular Testing)                                          │
│     ├── 每次发布前性能回归测试                                           │
│     ├── 月度负载测试                                                     │
│     ├── 季度压力测试                                                     │
│     └── 年度容量规划测试                                                 │
│        │                                                                 │
│        ▼                                                                 │
│  3. 监控分析 (Monitoring & Analysis)                                    │
│     ├── 实时性能指标监控                                                 │
│     ├── 趋势分析和预警                                                   │
│     ├── 异常检测和根因分析                                               │
│     └── 性能报告生成                                                     │
│        │                                                                 │
│        ▼                                                                 │
│  4. 优化实施 (Optimization Implementation)                              │
│     ├── 识别性能瓶颈                                                     │
│     ├── 制定优化方案                                                     │
│     ├── 实施性能改进                                                     │
│     └── 验证优化效果                                                     │
│        │                                                                 │
│        └─────────────────────────────────────────────────────────────────┘
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

通过建立完善的性能基准测试体系，可以确保Kubernetes控制平面在各种负载条件下都能提供稳定、高效的服务，为业务系统的可靠运行奠定坚实基础。