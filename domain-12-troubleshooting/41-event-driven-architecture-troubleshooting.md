# 41 - 事件驱动架构故障排查 (Event-Driven Architecture Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **专家级别**: ⭐⭐⭐⭐⭐ | **参考**: [CloudEvents Specification](https://cloudevents.io/), [Knative Eventing](https://knative.dev/docs/eventing/), [Apache Kafka Documentation](https://kafka.apache.org/documentation/)

---

## 相关文档交叉引用

### 🔗 关联故障排查文档
- **[25-网络连通性故障排查](./25-network-connectivity-troubleshooting.md)** - 事件传输网络问题
- **[30-监控告警故障排查](./30-monitoring-alerting-troubleshooting.md)** - 事件系统监控告警
- **[36-Helm Chart故障排查](./36-helm-chart-troubleshooting.md)** - 事件系统部署问题
- **[37-多集群管理故障排查](./37-multi-cluster-management-troubleshooting.md)** - 跨集群事件路由

### 📚 扩展学习资料
- **[CloudEvents规范](https://github.com/cloudevents/spec)** - 云原生事件标准
- **[Knative Serving](https://knative.dev/docs/serving/)** - 无服务器事件处理
- **[Strimzi Kafka Operator](https://strimzi.io/)** - Kubernetes上的Kafka管理

---

## 目录

1. [事件驱动架构概述](#1-事件驱动架构概述)
2. [核心组件故障排查](#2-核心组件故障排查)
3. [事件流问题诊断](#3-事件流问题诊断)
4. [性能瓶颈分析](#4-性能瓶颈分析)
5. [可靠性问题排查](#5-可靠性问题排查)
6. [监控告警配置](#6-监控告警配置)
7. [最佳实践与优化](#7-最佳实践与优化)

---

## 1. 事件驱动架构概述

### 1.1 架构模式分析

```
事件驱动架构核心组件:

┌─────────────────────────────────────────────────────────────────────────────┐
│                          事件驱动架构全景图                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐        │
│  │   事件生产者     │    │   事件代理层     │    │   事件消费者     │        │
│  │ Event Producers │    │ Event Brokers   │    │ Event Consumers │        │
│  │                 │    │                 │    │                 │        │
│  │ • 应用服务       │    │ • Kafka/Redis   │    │ • 微服务         │        │
│  │ • IoT设备       │    │ • NATS Streaming│    │ • 无服务器函数    │        │
│  │ • 数据库变更     │    │ • RabbitMQ      │    │ • 批处理作业     │        │
│  │ • 用户操作       │    │ • Pulsar        │    │ • 实时分析       │        │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘        │
│           │                         │                         │            │
│           ▼                         ▼                         ▼            │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐        │
│  │   事件格式化     │    │   事件路由       │    │   事件处理       │        │
│  │ Event Formatting│    │ Event Routing   │    │ Event Processing│        │
│  │                 │    │                 │    │                 │        │
│  │ • CloudEvents   │    │ • TriggerMesh   │    │ • Knative       │        │
│  │ • Schema Registry│    │ • EventBridge   │    │ • Functions     │        │
│  │ • Serialization │    │ • Filters       │    │ • Stream Proc   │        │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘        │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **事件丢失** | 消息未送达消费者 | 数据完整性受损 | P0 - 紧急 |
| **事件重复** | 同一消息多次处理 | 数据一致性问题 | P1 - 高 |
| **处理延迟** | 事件积压、响应慢 | 业务实时性受损 | P1 - 高 |
| **消费者失败** | 处理程序崩溃 | 业务逻辑中断 | P0 - 紧急 |
| **背压问题** | 生产者阻塞 | 系统吞吐量下降 | P1 - 高 |
| **死信队列满** | 无法处理的消息堆积 | 系统资源耗尽 | P0 - 紧急 |

---

## 2. 核心组件故障排查

### 2.1 Kafka集群故障排查

#### 2.1.1 Broker状态检查
```bash
# ========== 1. Kafka集群健康检查 ==========
# 检查Kafka Pod状态
kubectl get pods -n kafka -l app=kafka

# 验证Kafka服务端点
kubectl get svc -n kafka

# 检查Kafka控制器状态
kubectl exec -n kafka kafka-0 -- kafka-topics.sh --bootstrap-server localhost:9092 --describe --under-replicated-partitions

# ========== 2. Zookeeper连接检查 ==========
# 检查Zookeeper状态
kubectl exec -n kafka zookeeper-0 -- zkCli.sh ls /

# 验证Kafka在Zookeeper中的注册
kubectl exec -n kafka zookeeper-0 -- zkCli.sh get /brokers/ids/0

# ========== 3. Topic和分区状态 ==========
# 列出所有Topic
kubectl exec -n kafka kafka-0 -- kafka-topics.sh --bootstrap-server localhost:9092 --list

# 检查特定Topic详情
kubectl exec -n kafka kafka-0 -- kafka-topics.sh --bootstrap-server localhost:9092 --describe --topic my-topic

# 查看消费者组状态
kubectl exec -n kafka kafka-0 -- kafka-consumer-groups.sh --bootstrap-server localhost:9092 --list
kubectl exec -n kafka kafka-0 -- kafka-consumer-groups.sh --bootstrap-server localhost:9092 --describe --group my-consumer-group
```

#### 2.1.2 性能指标监控
```yaml
# kafka_monitoring_rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kafka-alerts
  namespace: monitoring
spec:
  groups:
  - name: kafka.rules
    rules:
    # Broker健康检查
    - alert: KafkaBrokerDown
      expr: kafka_broker_info == 0
      for: 2m
      labels:
        severity: critical
        category: kafka
      annotations:
        summary: "Kafka broker {{ $labels.instance }} is down"
        
    # 分区离线告警
    - alert: KafkaOfflinePartitions
      expr: kafka_topic_partitions{state="offline"} > 0
      for: 1m
      labels:
        severity: critical
        category: kafka
      annotations:
        summary: "{{ $value }} offline partitions detected"
        
    # 消费延迟告警
    - alert: KafkaConsumerLagHigh
      expr: kafka_consumergroup_lag > 10000
      for: 5m
      labels:
        severity: warning
        category: kafka
      annotations:
        summary: "Consumer lag high for group {{ $labels.consumergroup }}"
        
    # 磁盘使用率告警
    - alert: KafkaDiskUsageHigh
      expr: (kafka_log_size_bytes / kafka_log_capacity_bytes) * 100 > 85
      for: 10m
      labels:
        severity: warning
        category: kafka
      annotations:
        summary: "Kafka disk usage {{ $value | printf \"%.1f\" }}% high"
```

### 2.2 Knative Eventing故障排查

#### 2.2.1 事件网格状态检查
```bash
# ========== 1. Knative组件状态 ==========
# 检查Knative Serving
kubectl get pods -n knative-serving

# 检查Knative Eventing
kubectl get pods -n knative-eventing

# 验证Broker状态
kubectl get brokers -A

# 检查Trigger状态
kubectl get triggers -A

# ========== 2. 事件交付检查 ==========
# 查看事件源状态
kubectl get apiserversources -A
kubectl get pingsources -A
kubectl get kafkasources -A

# 检查事件路由
kubectl get subscriptions -A

# 验证SinkBinding
kubectl get sinkbindings -A

# ========== 3. 事件追踪诊断 ==========
# 启用事件追踪
kubectl patch configmap config-tracing -n knative-eventing --type merge \
  -p '{"data":{"backend":"zipkin","zipkin-endpoint":"http://zipkin.istio-system.svc.cluster.local:9411/api/v2/spans","debug":"true"}}'

# 查看事件轨迹
kubectl port-forward -n istio-system svc/tracing 16686:16686
# 访问 http://localhost:16686 查看事件追踪
```

### 2.3 Redis Streams故障排查

#### 2.3.1 Redis实例健康检查
```bash
# ========== 1. Redis连接和状态检查 ==========
# 检查Redis Pod状态
kubectl get pods -n redis -l app=redis

# 验证Redis连接
REDIS_POD=$(kubectl get pods -n redis -l app=redis -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n redis $REDIS_POD -- redis-cli ping

# 检查Redis内存使用
kubectl exec -n redis $REDIS_POD -- redis-cli info memory

# 查看Redis配置
kubectl exec -n redis $REDIS_POD -- redis-cli config get maxmemory*

# ========== 2. Stream状态监控 ==========
# 列出所有Streams
kubectl exec -n redis $REDIS_POD -- redis-cli xinfo streams

# 检查特定Stream信息
kubectl exec -n redis $REDIS_POD -- redis-cli xinfo stream mystream

# 查看消费者组状态
kubectl exec -n redis $REDIS_POD -- redis-cli xinfo groups mystream

# 检查待处理消息
kubectl exec -n redis $REDIS_POD -- redis-cli xpending mystream mygroup
```

---

## 3. 事件流问题诊断

### 3.1 事件丢失问题排查

#### 3.1.1 诊断流程
```bash
# ========== 事件丢失诊断脚本 ==========
cat <<'EOF' > event-loss-diagnostic.sh
#!/bin/bash

NAMESPACE=${1:-default}
TOPIC_NAME=${2:-my-topic}

echo "=== Event Loss Diagnostic Report ==="
echo "Namespace: $NAMESPACE"
echo "Topic: $TOPIC_NAME"
echo "Time: $(date)"
echo

# 1. 检查生产者状态
echo "1. Producer Status Check:"
kubectl get pods -n $NAMESPACE | grep producer
PRODUCER_POD=$(kubectl get pods -n $NAMESPACE -l app=event-producer -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ ! -z "$PRODUCER_POD" ]; then
    echo "Producer Pod: $PRODUCER_POD"
    kubectl logs $PRODUCER_POD -n $NAMESPACE --tail=50 | grep -i "error\|exception\|failed"
else
    echo "No producer pods found"
fi
echo

# 2. 检查Broker状态
echo "2. Broker Status Check:"
kubectl get pods -n kafka | grep kafka
kubectl exec -n kafka kafka-0 -- kafka-topics.sh --bootstrap-server localhost:9092 --describe --topic $TOPIC_NAME
echo

# 3. 检查消费者状态
echo "3. Consumer Status Check:"
kubectl get pods -n $NAMESPACE | grep consumer
CONSUMER_POD=$(kubectl get pods -n $NAMESPACE -l app=event-consumer -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ ! -z "$CONSUMER_POD" ]; then
    echo "Consumer Pod: $CONSUMER_POD"
    kubectl logs $CONSUMER_POD -n $NAMESPACE --tail=50 | grep -i "error\|exception\|offset"
else
    echo "No consumer pods found"
fi
echo

# 4. 检查消息积压
echo "4. Message Backlog Check:"
kubectl exec -n kafka kafka-0 -- kafka-run-class.sh kafka.tools.GetOffsetShell \
    --broker-list localhost:9092 --topic $TOPIC_NAME --time -1
echo

# 5. 检查网络连接
echo "5. Network Connectivity Check:"
kubectl exec -n kafka kafka-0 -- netstat -an | grep :9092
EOF

chmod +x event-loss-diagnostic.sh
```

#### 3.1.2 常见根本原因
| 原因类别 | 具体原因 | 解决方案 |
|---------|---------|---------|
| **配置问题** | Acknowledgment设置不当 | 调整acks=all，启用幂等性 |
| **网络问题** | 网络分区、连接超时 | 检查网络策略，增加超时配置 |
| **资源不足** | 内存溢出、磁盘满 | 扩容Broker，清理旧数据 |
| **代码缺陷** | 异常处理不当 | 完善错误处理，实现重试机制 |
| **消费者问题** | 消费偏移量异常 | 重置消费位点，检查消费者逻辑 |

### 3.2 事件重复问题处理

#### 3.2.1 幂等性设计模式
```yaml
# 幂等性消费者配置示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: idempotent-consumer
spec:
  replicas: 3
  selector:
    matchLabels:
      app: idempotent-consumer
  template:
    metadata:
      labels:
        app: idempotent-consumer
    spec:
      containers:
      - name: consumer
        image: my-consumer:latest
        env:
        # 启用幂等性配置
        - name: ENABLE_IDEMPOTENCE
          value: "true"
        - name: MAX_IN_FLIGHT
          value: "1"  # 确保顺序处理
        - name: RETRY_ATTEMPTS
          value: "3"
        - name: DEDUPLICATION_WINDOW
          value: "3600"  # 1小时去重窗口
```

#### 3.2.2 去重策略实现
```python
# python_consumer_with_deduplication.py
import hashlib
import time
from typing import Set
import redis

class DeduplicatingConsumer:
    def __init__(self, redis_host: str = 'localhost', redis_port: int = 6379):
        self.redis_client = redis.Redis(host=redis_host, port=redis_port, decode_responses=True)
        self.processed_events: Set[str] = set()
        self.dedup_window = 3600  # 1小时去重窗口
        
    def generate_event_id(self, event_data: dict) -> str:
        """生成事件唯一标识"""
        # 基于业务关键字段生成哈希
        key_fields = [event_data.get('id'), event_data.get('timestamp')]
        key_string = '|'.join(str(field) for field in key_fields if field)
        return hashlib.sha256(key_string.encode()).hexdigest()
        
    def is_duplicate(self, event_id: str) -> bool:
        """检查是否为重复事件"""
        # 检查内存缓存
        if event_id in self.processed_events:
            return True
            
        # 检查Redis持久化记录
        redis_key = f"processed_events:{event_id}"
        if self.redis_client.exists(redis_key):
            # 刷新过期时间
            self.redis_client.expire(redis_key, self.dedup_window)
            return True
            
        return False
        
    def mark_processed(self, event_id: str):
        """标记事件已处理"""
        # 添加到内存缓存
        self.processed_events.add(event_id)
        
        # 添加到Redis持久化存储
        redis_key = f"processed_events:{event_id}"
        self.redis_client.setex(redis_key, self.dedup_window, "1")
        
        # 维护内存缓存大小
        if len(self.processed_events) > 10000:
            oldest_keys = list(self.processed_events)[:1000]
            self.processed_events = self.processed_events.difference(set(oldest_keys))
            
    def process_event(self, event_data: dict):
        """处理事件（幂等性保证）"""
        event_id = self.generate_event_id(event_data)
        
        if self.is_duplicate(event_id):
            print(f"Skipping duplicate event: {event_id}")
            return
            
        try:
            # 执行业务逻辑
            self.handle_business_logic(event_data)
            
            # 标记为已处理
            self.mark_processed(event_id)
            print(f"Processed event: {event_id}")
            
        except Exception as e:
            print(f"Error processing event {event_id}: {e}")
            # 不标记为已处理，允许重试
            raise
            
    def handle_business_logic(self, event_data: dict):
        """具体的业务处理逻辑"""
        # 这里实现具体的业务逻辑
        pass

# 使用示例
consumer = DeduplicatingConsumer()
# consumer.process_event(event_data)
```

---

## 4. 性能瓶颈分析

### 4.1 吞吐量优化

#### 4.1.1 Kafka性能调优
```yaml
# kafka_performance_tuning.yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: high-throughput-cluster
spec:
  kafka:
    version: 3.4.0
    replicas: 3
    listeners:
      - name: plain
        port: 9092
        type: internal
        tls: false
      - name: tls
        port: 9093
        type: internal
        tls: true
    config:
      # 性能优化配置
      num.network.threads: 8
      num.io.threads: 16
      socket.send.buffer.bytes: 102400
      socket.receive.buffer.bytes: 102400
      socket.request.max.bytes: 104857600
      num.replica.fetchers: 4
      replica.fetch.max.bytes: 10485760
      replica.fetch.wait.max.ms: 500
      
      # 存储优化
      log.flush.interval.messages: 10000
      log.flush.interval.ms: 1000
      log.retention.hours: 168
      log.segment.bytes: 1073741824
      log.retention.check.interval.ms: 300000
      
      # 压缩配置
      compression.type: snappy
      message.max.bytes: 10485760
      
    storage:
      type: jbod
      volumes:
      - id: 0
        type: persistent-claim
        size: 200Gi
        deleteClaim: false
```

#### 4.1.2 消费者性能优化
```yaml
# consumer_performance_config.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: optimized-consumer
spec:
  replicas: 6  # 根据分区数调整
  template:
    spec:
      containers:
      - name: consumer
        image: my-consumer:latest
        env:
        # 批量处理配置
        - name: FETCH_MIN_BYTES
          value: "1048576"  # 1MB
        - name: FETCH_MAX_WAIT_MS
          value: "500"
        - name: MAX_POLL_RECORDS
          value: "500"
        - name: MAX_POLL_INTERVAL_MS
          value: "300000"  # 5分钟
          
        # 内存优化
        - name: HEAP_OPTS
          value: "-Xmx2g -Xms2g"
        - name: GC_TUNING
          value: "-XX:+UseG1GC -XX:MaxGCPauseMillis=20"
          
        resources:
          requests:
            memory: "2Gi"
            cpu: "1"
          limits:
            memory: "4Gi"
            cpu: "2"
```

### 4.2 延迟分析工具

#### 4.2.1 端到端延迟测量
```bash
# ========== 事件延迟测量脚本 ==========
cat <<'EOF' > event-latency-measurement.sh
#!/bin/bash

TOPIC_NAME=${1:-latency-test}
MESSAGE_COUNT=${2:-1000}
INTERVAL=${3:-0.1}

echo "Starting latency measurement for topic: $TOPIC_NAME"
echo "Messages to send: $MESSAGE_COUNT"
echo "Interval: ${INTERVAL}s"
echo

# 创建测试Topic
kubectl exec -n kafka kafka-0 -- kafka-topics.sh --bootstrap-server localhost:9092 \
    --create --topic $TOPIC_NAME --partitions 6 --replication-factor 3 2>/dev/null

# 启动消费者（后台）
kubectl run kafka-consumer --image=strimzi/kafka:latest-kafka-3.4.0 \
    --restart=Never --attach --rm \
    -- kafka-console-consumer.sh --bootstrap-server kafka-kafka-bootstrap:9092 \
    --topic $TOPIC_NAME --from-beginning --timeout-ms 60000 > /tmp/consumer_output.txt &

CONSUMER_PID=$!

# 发送带时间戳的消息
echo "Sending messages with timestamps..."
START_TIME=$(date +%s.%N)

for i in $(seq 1 $MESSAGE_COUNT); do
    TIMESTAMP=$(date +%s%3N)
    MESSAGE="msg-$i-$TIMESTAMP"
    
    kubectl exec -n kafka kafka-0 -- kafka-console-producer.sh \
        --bootstrap-server localhost:9092 \
        --topic $TOPIC_NAME <<< "$MESSAGE"
    
    sleep $INTERVAL
done

END_TIME=$(date +%s.%N)
echo "All messages sent. Waiting for consumer..."

# 等待消费者完成
wait $CONSUMER_PID

# 分析延迟
echo
echo "=== Latency Analysis Results ==="
python3 <<PY_END
import re
import statistics
from datetime import datetime

with open('/tmp/consumer_output.txt', 'r') as f:
    lines = f.readlines()

latencies = []
for line in lines:
    match = re.search(r'msg-(\d+)-(\d+)', line.strip())
    if match:
        msg_num, timestamp = match.groups()
        receive_time = datetime.now().timestamp() * 1000
        send_time = int(timestamp)
        latency = receive_time - send_time
        latencies.append(latency)
        print(f"Message {msg_num}: {latency:.2f}ms")

if latencies:
    print(f"\nLatency Statistics:")
    print(f"  Average: {statistics.mean(latencies):.2f}ms")
    print(f"  Median: {statistics.median(latencies):.2f}ms")
    print(f"  95th Percentile: {sorted(latencies)[int(len(latencies)*0.95)]:.2f}ms")
    print(f"  Max: {max(latencies):.2f}ms")
    print(f"  Min: {min(latencies):.2f}ms")
else:
    print("No latency data collected")
PY_END

# 清理测试Topic
kubectl exec -n kafka kafka-0 -- kafka-topics.sh --bootstrap-server localhost:9092 \
    --delete --topic $TOPIC_NAME 2>/dev/null

echo "Test completed."
EOF

chmod +x event-latency-measurement.sh
```

---

## 5. 可靠性问题排查

### 5.1 死信队列管理

#### 5.1.1 DLQ配置和监控
```yaml
# dead_letter_queue_config.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dlq-processor
spec:
  replicas: 2
  selector:
    matchLabels:
      app: dlq-processor
  template:
    metadata:
      labels:
        app: dlq-processor
    spec:
      containers:
      - name: processor
        image: dlq-processor:latest
        env:
        - name: DLQ_TOPIC
          value: "dead-letter-queue"
        - name: RETRY_TOPIC
          value: "retry-queue"
        - name: MAX_RETRY_ATTEMPTS
          value: "3"
        - name: RETRY_DELAY_SECONDS
          value: "300"  # 5分钟重试间隔
        - name: ALERT_THRESHOLD
          value: "100"  # 积压100条告警
        
        ports:
        - containerPort: 8080
          name: metrics
        
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
          
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

#### 5.1.2 DLQ监控告警
```yaml
# dlq_monitoring_rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dlq-alerts
  namespace: monitoring
spec:
  groups:
  - name: dlq.rules
    rules:
    # DLQ积压告警
    - alert: DLQBacklogHigh
      expr: kafka_topic_highwater{topic="dead-letter-queue"} > 100
      for: 5m
      labels:
        severity: warning
        category: reliability
      annotations:
        summary: "DLQ backlog high: {{ $value }} messages"
        description: "Dead letter queue has accumulated {{ $value }} unprocessed messages"
        
    # DLQ增长率告警
    - alert: DLQGrowthRateHigh
      expr: rate(kafka_topic_highwater{topic="dead-letter-queue"}[5m]) > 10
      for: 2m
      labels:
        severity: critical
        category: reliability
      annotations:
        summary: "DLQ growing rapidly: {{ $value | printf \"%.1f\" }} msgs/sec"
        
    # 重试队列堵塞
    - alert: RetryQueueBlocked
      expr: kafka_topic_highwater{topic="retry-queue"} > 1000
      for: 10m
      labels:
        severity: warning
        category: reliability
      annotations:
        summary: "Retry queue blocked: {{ $value }} messages pending"
```

### 5.2 事务一致性保障

#### 5.2.1 分布式事务模式
```python
# saga_pattern_implementation.py
from typing import List, Callable, Any
import uuid
import time

class SagaStep:
    def __init__(self, action: Callable, compensation: Callable):
        self.action = action
        self.compensation = compensation
        self.step_id = str(uuid.uuid4())

class SagaOrchestrator:
    def __init__(self):
        self.completed_steps: List[SagaStep] = []
        
    def execute_step(self, step: SagaStep, *args, **kwargs) -> bool:
        """执行Saga步骤"""
        try:
            result = step.action(*args, **kwargs)
            self.completed_steps.append(step)
            return True
        except Exception as e:
            print(f"Step {step.step_id} failed: {e}")
            self.compensate_failed_steps()
            return False
            
    def compensate_failed_steps(self):
        """补偿已执行的步骤"""
        print("Initiating compensation...")
        for step in reversed(self.completed_steps):
            try:
                step.compensation()
                print(f"Compensated step: {step.step_id}")
            except Exception as e:
                print(f"Compensation failed for {step.step_id}: {e}")
                
    def execute_saga(self, steps: List[SagaStep], *args, **kwargs) -> bool:
        """执行完整的Saga事务"""
        self.completed_steps = []
        
        for i, step in enumerate(steps):
            print(f"Executing step {i+1}/{len(steps)}: {step.step_id}")
            if not self.execute_step(step, *args, **kwargs):
                return False
                
        print("Saga completed successfully")
        return True

# 使用示例
def book_hotel():
    print("Booking hotel...")
    # 模拟酒店预订
    time.sleep(0.1)
    return {"booking_id": "hotel_123"}

def cancel_hotel_booking():
    print("Canceling hotel booking...")
    # 模拟取消预订

def book_flight():
    print("Booking flight...")
    # 模拟航班预订，可能失败
    if time.time() % 2 > 1:  # 模拟50%失败率
        raise Exception("Flight booking failed - no seats available")
    return {"booking_id": "flight_456"}

def cancel_flight_booking():
    print("Canceling flight booking...")

# 创建Saga编排器
orchestrator = SagaOrchestrator()

# 定义Saga步骤
hotel_step = SagaStep(book_hotel, cancel_hotel_booking)
flight_step = SagaStep(book_flight, cancel_flight_booking)

# 执行Saga事务
success = orchestrator.execute_saga([hotel_step, flight_step])

if success:
    print("Travel booking completed!")
else:
    print("Travel booking failed - all reservations cancelled")
```

---

## 6. 监控告警配置

### 6.1 核心指标监控

#### 6.1.1 事件系统关键指标
```yaml
# event_system_monitoring.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: event-system-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: event-processing
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    
---
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: event-system-alerts
  namespace: monitoring
spec:
  groups:
  - name: event-system.rules
    rules:
    # 事件生产速率异常
    - alert: EventProductionRateAnomaly
      expr: |
        abs(
          rate(event_produced_total[5m]) - 
          avg_over_time(rate(event_produced_total[5m])[1h:])
        ) / avg_over_time(rate(event_produced_total[5m])[1h:]) > 0.5
      for: 5m
      labels:
        severity: warning
        category: event-flow
      annotations:
        summary: "Event production rate anomaly detected"
        
    # 事件消费延迟
    - alert: EventProcessingLatencyHigh
      expr: histogram_quantile(0.95, rate(event_processing_duration_seconds_bucket[5m])) > 5
      for: 2m
      labels:
        severity: critical
        category: performance
      annotations:
        summary: "Event processing latency high: {{ $value | printf \"%.2f\" }}s"
        
    # 消费者失败率
    - alert: EventConsumerFailureRateHigh
      expr: rate(event_consumer_failures_total[5m]) / rate(event_received_total[5m]) > 0.1
      for: 1m
      labels:
        severity: critical
        category: reliability
      annotations:
        summary: "Consumer failure rate: {{ $value | printf \"%.2f\" }}%"
        
    # 事件积压告警
    - alert: EventBacklogHigh
      expr: event_backlog_size > 10000
      for: 10m
      labels:
        severity: warning
        category: performance
      annotations:
        summary: "Event backlog high: {{ $value }} messages"
```

### 6.2 仪表板配置

#### 6.2.1 Grafana仪表板JSON
```json
{
  "dashboard": {
    "title": "Event-Driven Architecture Monitoring",
    "panels": [
      {
        "title": "Event Production Rate",
        "type": "graph",
        "datasource": "Prometheus",
        "targets": [
          {
            "expr": "rate(event_produced_total[5m])",
            "legendFormat": "{{topic}}"
          }
        ]
      },
      {
        "title": "Event Consumption Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(event_consumed_total[5m])",
            "legendFormat": "{{consumer_group}}"
          }
        ]
      },
      {
        "title": "Processing Latency (95th Percentile)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(event_processing_duration_seconds_bucket[5m]))",
            "legendFormat": "{{handler}}"
          }
        ]
      },
      {
        "title": "Event Backlog Size",
        "type": "stat",
        "targets": [
          {
            "expr": "event_backlog_size",
            "legendFormat": "Current Backlog"
          }
        ]
      }
    ]
  }
}
```

---

## 7. 最佳实践与优化

### 7.1 架构设计原则

#### 7.1.1 高可用性设计
```yaml
# high_availability_design.yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: ha-event-bus
spec:
  kafka:
    replicas: 5  # 奇数个实例确保选举
    version: 3.4.0
    config:
      # 高可用配置
      min.insync.replicas: 3
      default.replication.factor: 3
      offsets.topic.replication.factor: 3
      transaction.state.log.replication.factor: 3
      transaction.state.log.min.isr: 2
      
    # 机架感知配置
    rack:
      topologyKey: topology.kubernetes.io/zone
      
  zookeeper:
    replicas: 3
    storage:
      type: persistent-claim
      size: 100Gi
      
  entityOperator:
    topicOperator: {}
    userOperator: {}
```

#### 7.1.2 容错机制
```python
# circuit_breaker_pattern.py
import time
from enum import Enum
from typing import Callable, Any, Optional

class CircuitState(Enum):
    CLOSED = "closed"      # 正常状态
    OPEN = "open"          # 断路状态
    HALF_OPEN = "half_open" # 半开状态

class CircuitBreaker:
    def __init__(self, 
                 failure_threshold: int = 5,
                 timeout: int = 60,
                 expected_exception: type = Exception):
        self.failure_threshold = failure_threshold
        self.timeout = timeout
        self.expected_exception = expected_exception
        self.failure_count = 0
        self.last_failure_time = None
        self.state = CircuitState.CLOSED
        
    def call(self, func: Callable, *args, **kwargs) -> Any:
        """通过断路器调用函数"""
        if self.state == CircuitState.OPEN:
            if time.time() - self.last_failure_time > self.timeout:
                self.state = CircuitState.HALF_OPEN
                print("Circuit breaker half-open - trying one request")
            else:
                raise Exception("Circuit breaker is OPEN - service unavailable")
                
        try:
            result = func(*args, **kwargs)
            self._on_success()
            return result
        except self.expected_exception as e:
            self._on_failure()
            raise e
            
    def _on_success(self):
        """成功时的处理"""
        if self.state == CircuitState.HALF_OPEN:
            print("Circuit breaker closed - service restored")
            self.state = CircuitState.CLOSED
        self.failure_count = 0
        
    def _on_failure(self):
        """失败时的处理"""
        self.failure_count += 1
        self.last_failure_time = time.time()
        
        if self.failure_count >= self.failure_threshold:
            print(f"Circuit breaker opened after {self.failure_count} failures")
            self.state = CircuitState.OPEN

# 使用示例
breaker = CircuitBreaker(failure_threshold=3, timeout=30)

def unreliable_service():
    """模拟不稳定的服务"""
    import random
    if random.random() < 0.7:  # 70%失败率
        raise Exception("Service temporarily unavailable")
    return "Success"

# 保护不稳定的调用
try:
    result = breaker.call(unreliable_service)
    print(f"Result: {result}")
except Exception as e:
    print(f"Call failed: {e}")
```

### 7.2 运维检查清单

#### 7.2.1 日常运维检查项
```markdown
## 事件驱动架构运维检查清单

### 🔍 日常监控检查
- [ ] 事件生产速率是否正常
- [ ] 消费者组延迟是否在合理范围内
- [ ] 死信队列是否有积压
- [ ] 系统资源使用率（CPU、内存、磁盘）
- [ ] 网络连接状态和延迟

### 🛠️ 定期维护任务
- [ ] 清理过期的Topic数据
- [ ] 优化消费者组分配
- [ ] 更新安全证书和密钥
- [ ] 执行灾难恢复演练
- [ ] 审查和优化配置参数

### 🚨 紧急响应流程
- [ ] 事件丢失应急处理
- [ ] 系统性能急剧下降处理
- [ ] 大规模事件积压处理
- [ ] 关键组件故障切换
- [ ] 数据一致性问题修复
```

---

**文档状态**: ✅ 完成 | **专家评审**: 已通过 | **最后更新**: 2026-02 | **适用场景**: 云原生事件驱动架构生产环境