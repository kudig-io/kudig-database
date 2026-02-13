# Redis Enterprise Cache Operations 深度实践

> **Author**: Cache Platform Architect | **Version**: v1.0 | **Update Time**: 2026-02-07
> **Scenario**: Enterprise-grade Redis cache operations and high availability management | **Complexity**: ⭐⭐⭐⭐

## 🎯 Abstract

This document provides comprehensive exploration of Redis enterprise deployment architecture, cache management strategies, and high availability implementation. Based on large-scale production environment experience, it offers complete technical guidance from cluster setup to advanced persistence and performance tuning, helping enterprises build ultra-fast, reliable caching platforms with integrated monitoring, automatic failover, and disaster recovery capabilities for mission-critical applications.

## 1. Redis Enterprise Architecture

### 1.1 Core Component Architecture

```mermaid
graph TB
    subgraph "Redis Infrastructure"
        A[Redis Instances]
        B[Master-Slave Replication]
        C[Redis Cluster]
        D[Sentinel Monitoring]
        E[Proxy Layer]
    end
    
    subgraph "Data Management"
        F[Data Persistence]
        G[Memory Management]
        H[Eviction Policies]
        I[Data Partitioning]
        J[Cache Warming]
    end
    
    subgraph "High Availability"
        K[Automatic Failover]
        L[Load Balancing]
        M[Health Monitoring]
        N[Backup & Restore]
        O[Disaster Recovery]
    end
    
    subgraph "Security & Access Control"
        P[Authentication]
        Q[Authorization]
        R[Encryption]
        S[Network Security]
        T[Audit Logging]
    end
    
    subgraph "Performance Optimization"
        U[Memory Optimization]
        V[Connection Pooling]
        W[Pipeline Operations]
        X[Batch Processing]
        Y[Compression]
    end
    
    subgraph "Monitoring & Operations"
        Z[Performance Metrics]
        AA[Alerting System]
        AB[Log Management]
        AC[Capacity Planning]
        AD[Troubleshooting]
    end
    
    A --> B
    B --> C
    C --> D
    D --> E
    
    F --> G
    G --> H
    H --> I
    I --> J
    
    K --> L
    L --> M
    M --> N
    N --> O
    
    P --> Q
    Q --> R
    R --> S
    S --> T
    
    U --> V
    V --> W
    W --> X
    X --> Y
    
    Z --> AA
    AA --> AB
    AB --> AC
    AC --> AD
```

### 1.2 Enterprise Deployment Architecture

```yaml
redis_enterprise_deployment:
  cluster_configuration:
    production_cluster:
      name: "redis-prod-cluster"
      port: 6379
      nodes:
        - host: "redis-node-0.redis-svc.production.svc.cluster.local"
          port: 6379
          role: "master"
          memory: "8GB"
          maxmemory: "6GB"
          maxmemory_policy: "allkeys-lru"
        
        - host: "redis-node-1.redis-svc.production.svc.cluster.local"
          port: 6379
          role: "slave"
          memory: "8GB"
          replication:
            master_host: "redis-node-0"
            master_port: 6379
        
        - host: "redis-node-2.redis-svc.production.svc.cluster.local"
          port: 6379
          role: "master"
          memory: "8GB"
          maxmemory: "6GB"
          maxmemory_policy: "allkeys-lfu"
      
      cluster_slots: 16384
      hash_tags: "{}"
      cluster_require_full_coverage: false
  
  sentinel_configuration:
    sentinels:
      - host: "redis-sentinel-0.redis-sentinel-svc.production.svc.cluster.local"
        port: 26379
        quorum: 2
        down_after_milliseconds: 5000
        failover_timeout: 10000
      
      - host: "redis-sentinel-1.redis-sentinel-svc.production.svc.cluster.local"
        port: 26379
        quorum: 2
        down_after_milliseconds: 5000
        failover_timeout: 10000
      
      - host: "redis-sentinel-2.redis-sentinel-svc.production.svc.cluster.local"
        port: 26379
        quorum: 2
        down_after_milliseconds: 5000
        failover_timeout: 10000
  
  persistence_configuration:
    rdb:
      save_intervals:
        - "900 1"    # 15分钟内至少1个key变化
        - "300 10"   # 5分钟内至少10个key变化
        - "60 10000" # 1分钟内至少10000个key变化
      compression: "yes"
      checksum: "yes"
    
    aof:
      enabled: "yes"
      filename: "appendonly.aof"
      fsync: "everysec"
      auto_aof_rewrite_percentage: 100
      auto_aof_rewrite_min_size: "64mb"
  
  security_configuration:
    authentication:
      requirepass: "super_secure_redis_password_2023"
      masterauth: "super_secure_redis_password_2023"
    
    tls_ssl:
      tls_port: 6380
      tls_cert_file: "/etc/redis/tls/redis.crt"
      tls_key_file: "/etc/redis/tls/redis.key"
      tls_ca_cert_file: "/etc/redis/tls/ca.crt"
      tls_auth_clients: "yes"
    
    acl:
      users:
        - username: "default"
          passwords: ["super_secure_redis_password_2023"]
          commands: ["+@all"]
          keys: ["*"]
        
        - username: "application"
          passwords: ["app_password_2023"]
          commands: ["+get", "+set", "+exists", "+expire"]
          keys: ["app:*"]
        
        - username: "monitoring"
          passwords: ["monitor_password_2023"]
          commands: ["+info", "+client", "+ping"]
          keys: [""]
```

## 2. Advanced Cache Management

### 2.1 Redis Cluster Management

```bash
#!/bin/bash
# redis_cluster_management.sh

REDIS_NODES=(
    "redis-node-0:6379"
    "redis-node-1:6379" 
    "redis-node-2:6379"
    "redis-node-3:6379"
    "redis-node-4:6379"
    "redis-node-5:6379"
)

# 1. 创建Redis集群
create_redis_cluster() {
    echo "Creating Redis cluster..."
    
    redis-cli --cluster create \
        ${REDIS_NODES[0]} ${REDIS_NODES[1]} ${REDIS_NODES[2]} \
        ${REDIS_NODES[3]} ${REDIS_NODES[4]} ${REDIS_NODES[5]} \
        --cluster-replicas 1 \
        --cluster-yes
    
    echo "Redis cluster created successfully"
}

# 2. 检查集群状态
check_cluster_status() {
    echo "Checking cluster status..."
    
    for node in "${REDIS_NODES[@]}"; do
        echo "Node: $node"
        redis-cli -h ${node%:*} -p ${node#*:} cluster info
        echo "---"
    done
}

# 3. 添加新节点到集群
add_cluster_node() {
    local new_node=$1
    local master_node=$2
    
    echo "Adding node $new_node to cluster..."
    
    # 添加主节点
    redis-cli --cluster add-node $new_node $master_node --cluster-slave
    
    # 重新分片数据
    redis-cli --cluster reshard $master_node \
        --cluster-from all \
        --cluster-to $new_node \
        --cluster-slots 1000 \
        --cluster-yes
}

# 4. 删除集群节点
remove_cluster_node() {
    local node_to_remove=$1
    
    echo "Removing node $node_to_remove from cluster..."
    
    # 获取节点ID
    node_id=$(redis-cli -h ${node_to_remove%:*} -p ${node_to_remove#*:} cluster myid)
    
    # 删除节点
    redis-cli --cluster del-node ${REDIS_NODES[0]} $node_id
}

# 5. 集群故障转移测试
test_failover() {
    local master_node=${REDIS_NODES[0]}
    
    echo "Testing failover for $master_node..."
    
    # 强制主节点下线
    redis-cli -h ${master_node%:*} -p ${master_node#*:} debug segfault
    
    # 等待故障转移完成
    sleep 30
    
    # 验证新主节点
    check_cluster_status
}
```

### 2.2 Advanced Data Structures and Patterns

```python
# Python Redis高级数据结构使用示例
import redis
import json
import time
from datetime import datetime, timedelta

class RedisAdvancedPatterns:
    def __init__(self, host='localhost', port=6379, db=0):
        self.redis_client = redis.Redis(
            host=host, 
            port=port, 
            db=db,
            decode_responses=True,
            socket_connect_timeout=5,
            socket_timeout=5
        )
    
    # 1. 使用Sorted Set实现排行榜
    def update_leaderboard(self, user_id, score):
        """更新用户排行榜分数"""
        key = "leaderboard:weekly"
        pipeline = self.redis_client.pipeline()
        
        # 更新分数
        pipeline.zadd(key, {user_id: score})
        
        # 设置过期时间（一周）
        pipeline.expire(key, 7 * 24 * 3600)
        
        # 获取排名
        pipeline.zrevrank(key, user_id)
        pipeline.zscore(key, user_id)
        
        results = pipeline.execute()
        rank = results[2]
        current_score = results[3]
        
        return {
            'user_id': user_id,
            'score': current_score,
            'rank': rank + 1 if rank is not None else None
        }
    
    def get_top_players(self, limit=10):
        """获取排行榜前N名"""
        key = "leaderboard:weekly"
        top_players = self.redis_client.zrevrange(
            key, 0, limit-1, withscores=True
        )
        return [(player[0], player[1]) for player in top_players]
    
    # 2. 使用Hash实现用户资料缓存
    def cache_user_profile(self, user_id, profile_data, ttl=3600):
        """缓存用户资料"""
        key = f"user:profile:{user_id}"
        self.redis_client.hset(key, mapping=profile_data)
        self.redis_client.expire(key, ttl)
        return True
    
    def get_user_profile(self, user_id):
        """获取用户资料"""
        key = f"user:profile:{user_id}"
        profile = self.redis_client.hgetall(key)
        return profile if profile else None
    
    # 3. 使用Bitmap实现签到功能
    def user_checkin(self, user_id, date=None):
        """用户签到"""
        if date is None:
            date = datetime.now()
        
        key = f"checkin:{date.strftime('%Y-%m')}"
        offset = int(date.strftime('%d')) - 1
        
        # 设置位图
        result = self.redis_client.setbit(key, offset, 1)
        self.redis_client.expire(key, 31 * 24 * 3600)  # 保留一个月
        
        return result == 0  # 返回True表示首次签到
    
    def get_checkin_stats(self, user_id, year_month):
        """获取用户签到统计"""
        key = f"checkin:{year_month}"
        
        # 获取整个月的签到情况
        bitmap = self.redis_client.get(key)
        if not bitmap:
            return {'total_days': 0, 'checkin_days': 0, 'consecutive_days': 0}
        
        # 统计签到天数
        checkin_days = bin(int.from_bytes(bitmap, 'big')).count('1')
        
        # 计算连续签到天数
        consecutive_days = 0
        current_consecutive = 0
        
        for bit in bin(int.from_bytes(bitmap, 'big'))[2:].zfill(31):
            if bit == '1':
                current_consecutive += 1
                consecutive_days = max(consecutive_days, current_consecutive)
            else:
                current_consecutive = 0
        
        return {
            'total_days': 31,
            'checkin_days': checkin_days,
            'consecutive_days': consecutive_days
        }
    
    # 4. 使用HyperLogLog实现基数统计
    def track_unique_visitors(self, page_id, user_id):
        """跟踪页面唯一访客"""
        key = f"visitors:daily:{page_id}:{datetime.now().strftime('%Y-%m-%d')}"
        self.redis_client.pfadd(key, user_id)
        self.redis_client.expire(key, 25 * 3600)  # 保留25小时
        return True
    
    def get_unique_visitor_count(self, page_id, days=7):
        """获取页面近N天唯一访客数"""
        keys = []
        for i in range(days):
            date = (datetime.now() - timedelta(days=i)).strftime('%Y-%m-%d')
            keys.append(f"visitors:daily:{page_id}:{date}")
        
        # 合并多个HyperLogLog
        merged_key = f"visitors:merged:{page_id}:{int(time.time())}"
        self.redis_client.pfmerge(merged_key, *keys)
        
        count = self.redis_client.pfcount(merged_key)
        
        # 清理临时键
        self.redis_client.delete(merged_key)
        
        return count

# 使用示例
redis_patterns = RedisAdvancedPatterns()

# 排行榜操作
redis_patterns.update_leaderboard("user_123", 1500)
top_players = redis_patterns.get_top_players(5)

# 用户资料缓存
profile_data = {
    "name": "John Doe",
    "email": "john@example.com",
    "avatar": "avatar_url",
    "last_login": str(datetime.now())
}
redis_patterns.cache_user_profile("user_123", profile_data)
user_profile = redis_patterns.get_user_profile("user_123")

# 签到功能
redis_patterns.user_checkin("user_123")
stats = redis_patterns.get_checkin_stats("user_123", "2023-12")

# 唯一访客统计
redis_patterns.track_unique_visitors("homepage", "visitor_456")
unique_count = redis_patterns.get_unique_visitor_count("homepage", 7)
```

## 3. Performance Optimization

### 3.1 Memory Optimization Strategies

```bash
#!/bin/bash
# redis_memory_optimization.sh

REDIS_HOST="localhost"
REDIS_PORT=6379

# 1. 内存使用分析
analyze_memory_usage() {
    echo "=== Redis Memory Analysis ==="
    
    # 基本内存信息
    redis-cli -h $REDIS_HOST -p $REDIS_PORT info memory
    
    # 内存碎片率
    fragmentation=$(redis-cli -h $REDIS_HOST -p $REDIS_PORT info memory | grep mem_fragmentation_ratio | cut -d: -f2)
    echo "Memory Fragmentation Ratio: $fragmentation"
    
    # 最大内存配置
    maxmemory=$(redis-cli -h $REDIS_HOST -p $REDIS_PORT config get maxmemory | tail -1)
    echo "Max Memory: $maxmemory bytes"
    
    # 当前使用内存
    used_memory=$(redis-cli -h $REDIS_HOST -p $REDIS_PORT info memory | grep used_memory: | cut -d: -f2)
    echo "Used Memory: $used_memory bytes"
}

# 2. 大Key检测
find_large_keys() {
    echo "Finding large keys..."
    
    redis-cli -h $REDIS_HOST -p $REDIS_PORT --bigkeys
    
    # 详细的大Key分析
    echo "Detailed large key analysis:"
    redis-cli -h $REDIS_HOST -p $REDIS_PORT scan 0 | while read cursor keys; do
        for key in $keys; do
            if [ "$key" != "0" ]; then
                type=$(redis-cli -h $REDIS_HOST -p $REDIS_PORT type $key)
                mem_usage=$(redis-cli -h $REDIS_HOST -p $REDIS_PORT memory usage $key)
                if [ "$mem_usage" -gt 1048576 ]; then  # 大于1MB
                    echo "Large Key: $key ($type) - Size: $mem_usage bytes"
                fi
            fi
        done
    done
}

# 3. 内存优化建议
optimize_memory() {
    echo "Applying memory optimizations..."
    
    # 设置合理的最大内存
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set maxmemory 2gb
    
    # 设置内存淘汰策略
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set maxmemory-policy allkeys-lru
    
    # 启用内存压缩（Redis 7.0+）
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set activedefrag yes
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set active-defrag-ignore-bytes 100mb
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set active-defrag-threshold-lower 10
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set active-defrag-threshold-upper 100
    
    # 优化哈希结构
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set hash-max-ziplist-entries 512
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set hash-max-ziplist-value 64
    
    # 优化集合结构
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set set-max-intset-entries 512
    
    # 优化有序集合
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set zset-max-ziplist-entries 128
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set zset-max-ziplist-value 64
}

# 4. 连接优化
optimize_connections() {
    echo "Optimizing connections..."
    
    # 增加最大连接数
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set maxclients 10000
    
    # 启用TCP keepalive
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set tcp-keepalive 300
    
    # 优化TCP backlog
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set tcp-backlog 511
    
    # 启用pipeline批处理
    echo "Consider using pipeline operations for better performance"
}

# 5. 持久化优化
optimize_persistence() {
    echo "Optimizing persistence..."
    
    # RDB优化
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set save "900 1 300 10 60 10000"
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set rdbcompression yes
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set rdbchecksum yes
    
    # AOF优化
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set appendonly yes
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set appendfsync everysec
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set auto-aof-rewrite-percentage 100
    redis-cli -h $REDIS_HOST -p $REDIS_PORT config set auto-aof-rewrite-min-size 64mb
}

# 执行所有优化
main() {
    analyze_memory_usage
    echo ""
    find_large_keys
    echo ""
    optimize_memory
    echo ""
    optimize_connections
    echo ""
    optimize_persistence
    echo ""
    echo "Memory optimization completed!"
}

main
```

### 3.2 Pipeline and Batch Operations

```python
# Redis Pipeline and Batch Operations
import redis
import time
import json
from typing import List, Dict, Any

class RedisPipelineOperations:
    def __init__(self, host='localhost', port=6379, db=0):
        self.redis_client = redis.Redis(
            host=host,
            port=port,
            db=db,
            decode_responses=True,
            socket_connect_timeout=5,
            socket_timeout=5
        )
    
    def batch_set_with_pipeline(self, key_value_pairs: Dict[str, Any], ttl: int = None) -> bool:
        """使用Pipeline批量设置键值对"""
        pipeline = self.redis_client.pipeline(transaction=False)
        
        for key, value in key_value_pairs.items():
            if isinstance(value, (dict, list)):
                value = json.dumps(value)
            pipeline.set(key, value)
            if ttl:
                pipeline.expire(key, ttl)
        
        results = pipeline.execute()
        return all(results)
    
    def batch_get_with_pipeline(self, keys: List[str]) -> Dict[str, Any]:
        """使用Pipeline批量获取键值"""
        pipeline = self.redis_client.pipeline(transaction=False)
        
        for key in keys:
            pipeline.get(key)
        
        results = pipeline.execute()
        
        return_dict = {}
        for i, key in enumerate(keys):
            value = results[i]
            if value:
                try:
                    # 尝试解析JSON
                    return_dict[key] = json.loads(value)
                except json.JSONDecodeError:
                    return_dict[key] = value
            else:
                return_dict[key] = None
        
        return return_dict
    
    def atomic_counter_operations(self, counter_keys: List[str], increment_values: List[int]) -> List[int]:
        """原子计数器操作"""
        pipeline = self.redis_client.pipeline(transaction=True)
        
        for key, incr_value in zip(counter_keys, increment_values):
            pipeline.incrby(key, incr_value)
        
        results = pipeline.execute()
        return results
    
    def cache_warming_pipeline(self, warmup_data: List[Dict]) -> int:
        """缓存预热Pipeline"""
        pipeline = self.redis_client.pipeline(transaction=False)
        success_count = 0
        
        for item in warmup_data:
            key = item.get('key')
            value = item.get('value')
            ttl = item.get('ttl', 3600)
            
            if key and value:
                if isinstance(value, (dict, list)):
                    value = json.dumps(value)
                
                pipeline.setex(key, ttl, value)
                success_count += 1
        
        pipeline.execute()
        return success_count
    
    def multi_operation_transaction(self, operations: List[Dict]) -> List[Any]:
        """多操作事务处理"""
        pipeline = self.redis_client.pipeline(transaction=True)
        
        for op in operations:
            op_type = op.get('type')
            key = op.get('key')
            value = op.get('value')
            args = op.get('args', [])
            
            if op_type == 'set':
                pipeline.set(key, value, *args)
            elif op_type == 'get':
                pipeline.get(key)
            elif op_type == 'incr':
                pipeline.incr(key)
            elif op_type == 'hset':
                pipeline.hset(key, *args)
            elif op_type == 'zadd':
                pipeline.zadd(key, *args)
            # 可以添加更多操作类型
        
        try:
            results = pipeline.execute()
            return results
        except redis.WatchError:
            # 处理乐观锁冲突
            return None

# 使用示例
pipeline_ops = RedisPipelineOperations()

# 批量设置操作
batch_data = {
    'user:1001:name': 'Alice',
    'user:1001:email': 'alice@example.com',
    'user:1002:name': 'Bob',
    'user:1002:email': 'bob@example.com'
}
pipeline_ops.batch_set_with_pipeline(batch_data, ttl=7200)

# 批量获取操作
keys_to_fetch = ['user:1001:name', 'user:1001:email', 'user:1002:name']
results = pipeline_ops.batch_get_with_pipeline(keys_to_fetch)

# 原子计数器操作
counter_keys = ['page_views:home', 'page_views:products', 'page_views:about']
increments = [1, 3, 1]
new_counts = pipeline_ops.atomic_counter_operations(counter_keys, increments)

# 缓存预热
warmup_items = [
    {'key': 'popular_products', 'value': ['prod_1', 'prod_2', 'prod_3'], 'ttl': 1800},
    {'key': 'featured_articles', 'value': ['art_1', 'art_2'], 'ttl': 3600},
    {'key': 'site_config', 'value': {'theme': 'dark', 'language': 'en'}, 'ttl': 86400}
]
warmed_items = pipeline_ops.cache_warming_pipeline(warmup_items)
```

## 4. High Availability and Disaster Recovery

### 4.1 Sentinel-based Failover Setup

```bash
#!/bin/bash
# redis_sentinel_setup.sh

SENTINEL_CONFIG_DIR="/etc/redis/sentinel"
MASTER_NAME="mymaster"
MASTER_IP="192.168.1.100"
MASTER_PORT=6379
QUORUM=2

# 1. 创建Sentinel配置文件
create_sentinel_config() {
    local sentinel_port=$1
    local config_file="$SENTINEL_CONFIG_DIR/sentinel_$sentinel_port.conf"
    
    cat > $config_file << EOF
port $sentinel_port
bind 0.0.0.0
daemonize yes
pidfile /var/run/redis-sentinel-$sentinel_port.pid
logfile /var/log/redis/sentinel_$sentinel_port.log
dir /tmp

sentinel monitor $MASTER_NAME $MASTER_IP $MASTER_PORT $QUORUM
sentinel down-after-milliseconds $MASTER_NAME 5000
sentinel failover-timeout $MASTER_NAME 10000
sentinel parallel-syncs $MASTER_NAME 1

sentinel auth-pass $MASTER_NAME "super_secure_redis_password_2023"
sentinel auth-user $MASTER_NAME "sentinel"

# 通知脚本
sentinel notification-script $MASTER_NAME /etc/redis/scripts/sentinel_notify.sh
sentinel client-reconfig-script $MASTER_NAME /etc/redis/scripts/sentinel_reconfig.sh
EOF

    echo "Created sentinel config: $config_file"
}

# 2. 启动Sentinel实例
start_sentinel() {
    local sentinel_port=$1
    local config_file="$SENTINEL_CONFIG_DIR/sentinel_$sentinel_port.conf"
    
    redis-sentinel $config_file
    echo "Started Sentinel on port $sentinel_port"
}

# 3. 验证Sentinel状态
verify_sentinel() {
    local sentinel_port=$1
    
    echo "Verifying Sentinel on port $sentinel_port:"
    redis-cli -p $sentinel_port sentinel masters
    redis-cli -p $sentinel_port sentinel slaves $MASTER_NAME
}

# 4. 故障转移测试脚本
failover_test() {
    echo "Testing failover..."
    
    # 获取当前主节点
    current_master=$(redis-cli -p 26379 sentinel get-master-addr-by-name $MASTER_NAME)
    echo "Current master: $current_master"
    
    # 强制主节点下线
    redis-cli -h $MASTER_IP -p $MASTER_PORT debug segfault
    
    # 等待故障转移
    sleep 30
    
    # 验证新主节点
    new_master=$(redis-cli -p 26379 sentinel get-master-addr-by-name $MASTER_NAME)
    echo "New master: $new_master"
    
    if [ "$current_master" != "$new_master" ]; then
        echo "Failover successful!"
    else
        echo "Failover failed!"
    fi
}

# 5. 主从切换通知脚本
cat > /etc/redis/scripts/sentinel_notify.sh << 'EOF'
#!/bin/bash

EVENT_TYPE=$1
MASTER_NAME=$2
MASTER_IP=$3
MASTER_PORT=$4

LOG_FILE="/var/log/redis/sentinel_notifications.log"

echo "$(date): Event $EVENT_TYPE for master $MASTER_NAME at $MASTER_IP:$MASTER_PORT" >> $LOG_FILE

case $EVENT_TYPE in
    +reset-master)
        echo "Master has been reset" >> $LOG_FILE
        ;;
    +slave)
        SLAVE_IP=$5
        SLAVE_PORT=$6
        echo "New slave added: $SLAVE_IP:$SLAVE_PORT" >> $LOG_FILE
        ;;
    +failover-state-reconf-slaves)
        echo "Failover in progress - reconfiguring slaves" >> $LOG_FILE
        ;;
    +failover-end)
        echo "Failover completed successfully" >> $LOG_FILE
        # 这里可以添加应用层的通知逻辑
        ;;
esac
EOF

chmod +x /etc/redis/scripts/sentinel_notify.sh

# 主配置
create_sentinel_config 26379
create_sentinel_config 26380
create_sentinel_config 26381

# 启动实例
start_sentinel 26379
start_sentinel 26380
start_sentinel 26381

# 验证配置
sleep 5
verify_sentinel 26379
```

### 4.2 Backup and Recovery Procedures

```bash
#!/bin/bash
# redis_backup_restore.sh

BACKUP_DIR="/backup/redis"
RETENTION_DAYS=30
REDIS_HOST="localhost"
REDIS_PORT=6379

# 1. RDB备份
backup_rdb() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_file="$BACKUP_DIR/rdb_backup_$timestamp.rdb"
    
    echo "Starting RDB backup..."
    
    # 触发BGSAVE
    redis-cli -h $REDIS_HOST -p $REDIS_PORT bgsave
    
    # 等待备份完成
    while [ "$(redis-cli -h $REDIS_HOST -p $REDIS_PORT lastsave)" = "0" ]; do
        sleep 1
    done
    
    # 复制RDB文件
    cp /var/lib/redis/dump.rdb $backup_file
    
    # 验证备份文件
    if [ -f "$backup_file" ] && [ -s "$backup_file" ]; then
        # 创建校验和
        md5sum $backup_file > "$backup_file.md5"
        echo "RDB backup completed: $backup_file"
        return 0
    else
        echo "RDB backup failed"
        return 1
    fi
}

# 2. AOF备份
backup_aof() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_file="$BACKUP_DIR/aof_backup_$timestamp.aof"
    
    echo "Starting AOF backup..."
    
    # BGREWRITEAOF重写AOF文件
    redis-cli -h $REDIS_HOST -p $REDIS_PORT bgrewriteaof
    
    # 等待重写完成
    sleep 10
    
    # 复制AOF文件
    cp /var/lib/redis/appendonly.aof $backup_file
    
    # 验证备份文件
    if [ -f "$backup_file" ] && [ -s "$backup_file" ]; then
        md5sum $backup_file > "$backup_file.md5"
        echo "AOF backup completed: $backup_file"
        return 0
    else
        echo "AOF backup failed"
        return 1
    fi
}

# 3. 增量备份
incremental_backup() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_file="$BACKUP_DIR/incr_backup_$timestamp.aof"
    
    echo "Starting incremental backup..."
    
    # 获取当前AOF文件
    cp /var/lib/redis/appendonly.aof $backup_file
    
    # 记录备份时间点
    redis-cli -h $REDIS_HOST -p $REDIS_PORT time | head -1 > "$BACKUP_DIR/last_backup_time"
    
    if [ -f "$backup_file" ]; then
        echo "Incremental backup completed: $backup_file"
        return 0
    else
        echo "Incremental backup failed"
        return 1
    fi
}

# 4. 备份验证
verify_backup() {
    local backup_file=$1
    
    echo "Verifying backup: $backup_file"
    
    # 验证校验和
    if [ -f "$backup_file.md5" ]; then
        md5sum -c "$backup_file.md5"
        if [ $? -ne 0 ]; then
            echo "Backup verification failed - checksum mismatch"
            return 1
        fi
    fi
    
    # 测试恢复到临时实例
    local temp_port=6380
    redis-server --port $temp_port --dir /tmp --dbfilename temp_dump.rdb &
    temp_pid=$!
    
    sleep 2
    
    # 加载备份数据
    redis-cli -p $temp_port shutdown nosave
    cp $backup_file /tmp/temp_dump.rdb
    redis-server --port $temp_port --dir /tmp --dbfilename temp_dump.rdb &
    
    sleep 2
    
    # 验证数据完整性
    local key_count=$(redis-cli -p $temp_port dbsize)
    echo "Recovered database contains $key_count keys"
    
    # 清理临时实例
    redis-cli -p $temp_port shutdown nosave
    kill $temp_pid 2>/dev/null
    
    echo "Backup verification completed"
    return 0
}

# 5. 恢复操作
restore_backup() {
    local backup_file=$1
    local target_host=${2:-localhost}
    local target_port=${3:-6379}
    
    echo "Restoring backup: $backup_file to $target_host:$target_port"
    
    # 停止目标Redis实例
    redis-cli -h $target_host -p $target_port shutdown nosave
    
    # 备份现有数据
    local existing_backup="/tmp/existing_data_$(date +%Y%m%d_%H%M%S).rdb"
    cp /var/lib/redis/dump.rdb $existing_backup
    
    # 恢复备份文件
    cp $backup_file /var/lib/redis/dump.rdb
    
    # 启动Redis实例
    systemctl start redis
    
    # 验证恢复
    sleep 5
    local recovered_keys=$(redis-cli -h $target_host -p $target_port dbsize)
    echo "Recovery completed. Database contains $recovered_keys keys"
}

# 6. 备份清理
cleanup_old_backups() {
    echo "Cleaning up old backups..."
    find "$BACKUP_DIR" -name "rdb_backup_*.rdb" -mtime +$RETENTION_DAYS -delete
    find "$BACKUP_DIR" -name "aof_backup_*.aof" -mtime +$RETENTION_DAYS -delete
    find "$BACKUP_DIR" -name "incr_backup_*.aof" -mtime +7 -delete
}

# 7. 自动备份调度
setup_cron_jobs() {
    echo "Setting up automatic backup schedule..."
    
    # 每天凌晨2点进行RDB备份
    echo "0 2 * * * $0 backup_rdb" | crontab -
    
    # 每小时进行增量备份
    echo "0 * * * * $0 incremental_backup" | crontab -
    
    # 每周日凌晨3点进行AOF备份
    echo "0 3 * * 0 $0 backup_aof" | crontab -
    
    # 每天凌晨4点清理旧备份
    echo "0 4 * * * $0 cleanup_old_backups" | crontab -
}

# 主函数
main() {
    case "$1" in
        backup_rdb)
            backup_rdb
            ;;
        backup_aof)
            backup_aof
            ;;
        incremental_backup)
            incremental_backup
            ;;
        restore)
            restore_backup "$2" "$3" "$4"
            ;;
        verify)
            verify_backup "$2"
            ;;
        cleanup)
            cleanup_old_backups
            ;;
        schedule)
            setup_cron_jobs
            ;;
        *)
            echo "Usage: $0 {backup_rdb|backup_aof|incremental_backup|restore|verify|cleanup|schedule}"
            echo "Examples:"
            echo "  $0 backup_rdb"
            echo "  $0 restore /backup/redis/rdb_backup_20231201_020000.rdb"
            echo "  $0 verify /backup/redis/rdb_backup_20231201_020000.rdb"
            echo "  $0 schedule"
            ;;
    esac
}

main "$@"
```

## 5. Monitoring and Alerting

### 5.1 Comprehensive Monitoring Setup

```python
# Redis监控和告警系统
import redis
import time
import json
import smtplib
from email.mime.text import MIMEText
from datetime import datetime, timedelta
from typing import Dict, List, Optional

class RedisMonitor:
    def __init__(self, host='localhost', port=6379, password=None):
        self.redis_client = redis.Redis(
            host=host,
            port=port,
            password=password,
            decode_responses=True,
            socket_connect_timeout=5,
            socket_timeout=5
        )
        self.alert_thresholds = {
            'memory_usage_percent': 85,
            'connected_clients': 1000,
            'blocked_clients': 10,
            'rejected_connections': 5,
            'evicted_keys': 100,
            'keyspace_hits_ratio': 0.8,
            'latency_ms': 10
        }
        self.alert_recipients = ['admin@company.com', 'ops@company.com']
    
    def get_server_info(self) -> Dict:
        """获取服务器基本信息"""
        try:
            info = self.redis_client.info()
            return {
                'version': info.get('redis_version'),
                'mode': info.get('redis_mode'),
                'uptime': info.get('uptime_in_seconds'),
                'connected_clients': info.get('connected_clients'),
                'blocked_clients': info.get('blocked_clients'),
                'used_memory': info.get('used_memory'),
                'used_memory_human': info.get('used_memory_human'),
                'used_memory_peak': info.get('used_memory_peak'),
                'mem_fragmentation_ratio': info.get('mem_fragmentation_ratio'),
                'total_commands_processed': info.get('total_commands_processed'),
                'instantaneous_ops_per_sec': info.get('instantaneous_ops_per_sec'),
                'keyspace_hits': info.get('keyspace_hits'),
                'keyspace_misses': info.get('keyspace_misses')
            }
        except Exception as e:
            return {'error': str(e)}
    
    def get_memory_stats(self) -> Dict:
        """获取内存统计信息"""
        try:
            info = self.redis_client.info('memory')
            memory_stats = self.redis_client.memory_stats()
            
            maxmemory = info.get('maxmemory', 0)
            used_memory = info.get('used_memory', 0)
            memory_usage_percent = (used_memory / maxmemory * 100) if maxmemory > 0 else 0
            
            return {
                'used_memory': used_memory,
                'used_memory_human': info.get('used_memory_human'),
                'used_memory_rss': info.get('used_memory_rss'),
                'used_memory_peak': info.get('used_memory_peak'),
                'maxmemory': maxmemory,
                'maxmemory_human': info.get('maxmemory_human'),
                'memory_usage_percent': round(memory_usage_percent, 2),
                'mem_fragmentation_ratio': info.get('mem_fragmentation_ratio'),
                'allocator_active': memory_stats.get('allocator_active', 0),
                'allocator_resident': memory_stats.get('allocator_resident', 0)
            }
        except Exception as e:
            return {'error': str(e)}
    
    def get_performance_metrics(self) -> Dict:
        """获取性能指标"""
        try:
            # 获取延迟信息
            latency_info = self.redis_client.latency_latest()
            
            # 获取慢查询日志
            slowlog = self.redis_client.slowlog_get(10)
            
            # 计算命中率
            info = self.redis_client.info()
            keyspace_hits = info.get('keyspace_hits', 0)
            keyspace_misses = info.get('keyspace_misses', 0)
            total_lookups = keyspace_hits + keyspace_misses
            hit_ratio = (keyspace_hits / total_lookups) if total_lookups > 0 else 0
            
            return {
                'instantaneous_ops_per_sec': info.get('instantaneous_ops_per_sec'),
                'total_commands_processed': info.get('total_commands_processed'),
                'keyspace_hits': keyspace_hits,
                'keyspace_misses': keyspace_misses,
                'hit_ratio': round(hit_ratio, 4),
                'rejected_connections': info.get('rejected_connections'),
                'sync_full': info.get('sync_full'),
                'sync_partial_ok': info.get('sync_partial_ok'),
                'expired_keys': info.get('expired_keys'),
                'evicted_keys': info.get('evicted_keys'),
                'latency_latest': dict(latency_info) if latency_info else {}
            }
        except Exception as e:
            return {'error': str(e)}
    
    def check_alerts(self) -> List[Dict]:
        """检查告警条件"""
        alerts = []
        
        try:
            # 检查内存使用率
            memory_stats = self.get_memory_stats()
            if 'memory_usage_percent' in memory_stats:
                usage = memory_stats['memory_usage_percent']
                if usage > self.alert_thresholds['memory_usage_percent']:
                    alerts.append({
                        'type': 'HIGH_MEMORY_USAGE',
                        'severity': 'WARNING',
                        'message': f'Memory usage is {usage}% (threshold: {self.alert_thresholds["memory_usage_percent"]}%)',
                        'current_value': usage,
                        'threshold': self.alert_thresholds['memory_usage_percent']
                    })
            
            # 检查连接数
            server_info = self.get_server_info()
            if 'connected_clients' in server_info:
                clients = server_info['connected_clients']
                if clients > self.alert_thresholds['connected_clients']:
                    alerts.append({
                        'type': 'HIGH_CLIENT_CONNECTIONS',
                        'severity': 'WARNING',
                        'message': f'Connected clients: {clients} (threshold: {self.alert_thresholds["connected_clients"]})',
                        'current_value': clients,
                        'threshold': self.alert_thresholds['connected_clients']
                    })
            
            # 检查被阻塞客户端
            if 'blocked_clients' in server_info:
                blocked = server_info['blocked_clients']
                if blocked > self.alert_thresholds['blocked_clients']:
                    alerts.append({
                        'type': 'BLOCKED_CLIENTS',
                        'severity': 'CRITICAL',
                        'message': f'Blocked clients: {blocked} (threshold: {self.alert_thresholds["blocked_clients"]})',
                        'current_value': blocked,
                        'threshold': self.alert_thresholds['blocked_clients']
                    })
            
            # 检查驱逐键数量
            perf_metrics = self.get_performance_metrics()
            if 'evicted_keys' in perf_metrics:
                evicted = perf_metrics['evicted_keys']
                if evicted > self.alert_thresholds['evicted_keys']:
                    alerts.append({
                        'type': 'KEYS_EVICTED',
                        'severity': 'WARNING',
                        'message': f'Evicted keys: {evicted} (threshold: {self.alert_thresholds["evicted_keys"]})',
                        'current_value': evicted,
                        'threshold': self.alert_thresholds['evicted_keys']
                    })
            
            # 检查命中率
            if 'hit_ratio' in perf_metrics:
                hit_ratio = perf_metrics['hit_ratio']
                if hit_ratio < self.alert_thresholds['keyspace_hits_ratio']:
                    alerts.append({
                        'type': 'LOW_HIT_RATIO',
                        'severity': 'WARNING',
                        'message': f'Keyspace hit ratio: {hit_ratio} (threshold: {self.alert_thresholds["keyspace_hits_ratio"]})',
                        'current_value': hit_ratio,
                        'threshold': self.alert_thresholds['keyspace_hits_ratio']
                    })
        
        except Exception as e:
            alerts.append({
                'type': 'MONITORING_ERROR',
                'severity': 'CRITICAL',
                'message': f'Monitoring error: {str(e)}'
            })
        
        return alerts
    
    def send_alert(self, alert: Dict):
        """发送告警通知"""
        subject = f"Redis Alert - {alert['type']} ({alert['severity']})"
        body = f"""
Redis Alert Notification
========================
Time: {datetime.now().isoformat()}
Type: {alert['type']}
Severity: {alert['severity']}
Message: {alert['message']}

Details:
- Current Value: {alert.get('current_value', 'N/A')}
- Threshold: {alert.get('threshold', 'N/A')}
        """
        
        # 这里可以集成邮件、Slack、钉钉等通知方式
        print(f"ALERT: {subject}")
        print(body)
        print("-" * 50)
    
    def monitor_loop(self, interval: int = 60):
        """持续监控循环"""
        print(f"Starting Redis monitoring (interval: {interval}s)")
        
        while True:
            try:
                alerts = self.check_alerts()
                for alert in alerts:
                    self.send_alert(alert)
                
                # 记录监控数据
                timestamp = datetime.now().isoformat()
                metrics = {
                    'timestamp': timestamp,
                    'server_info': self.get_server_info(),
                    'memory_stats': self.get_memory_stats(),
                    'performance_metrics': self.get_performance_metrics(),
                    'alerts': len(alerts)
                }
                
                # 可以将指标存储到时序数据库
                print(f"[{timestamp}] Monitoring cycle completed - {len(alerts)} alerts")
                
            except Exception as e:
                print(f"Monitoring error: {e}")
            
            time.sleep(interval)

# 使用示例
if __name__ == "__main__":
    monitor = RedisMonitor(password="your_redis_password")
    
    # 单次检查
    alerts = monitor.check_alerts()
    for alert in alerts:
        monitor.send_alert(alert)
    
    # 或启动持续监控
    # monitor.monitor_loop(interval=30)
```

### 5.2 Prometheus Exporter Configuration

```yaml
# redis_exporter_config.yaml
redis_exporter:
  redis_addr: "localhost:6379"
  redis_user: ""
  redis_password: "your_redis_password"
  namespace: "redis"
  check_keys: "db0=user:*,db0=session:*"
  check_single_keys: "db0:stats:total_users,db0:stats:active_sessions"
  script_path: "/etc/redis/scripts/metrics.lua"

scrape_configs:
  - job_name: 'redis'
    static_configs:
      - targets: ['redis-exporter:9121']
    scrape_interval: 15s
    scrape_timeout: 10s

# Alerting Rules
groups:
  - name: redis.rules
    rules:
      - alert: RedisDown
        expr: redis_up == 0
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "Redis instance is down"
          description: "Redis instance {{ $labels.instance }} is not responding"
      
      - alert: RedisMemoryHigh
        expr: (redis_memory_used_bytes / redis_memory_max_bytes * 100) > 85
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Redis memory usage high"
          description: "Redis memory usage is {{ $value | humanizePercentage }}"
      
      - alert: RedisRejectedConnections
        expr: irate(redis_rejected_connections_total[5m]) > 5
        for: 2m
        labels:
          severity: warning
        annotations:
          summary: "Redis rejecting connections"
          description: "Redis is rejecting connections at a rate of {{ $value }} per second"
      
      - alert: RedisEvictionsHigh
        expr: irate(redis_evicted_keys_total[5m]) > 100
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High Redis key evictions"
          description: "Redis is evicting keys at a rate of {{ $value }} per second"
      
      - alert: RedisHitRateLow
        expr: (irate(redis_keyspace_hits_total[5m]) / (irate(redis_keyspace_hits_total[5m]) + irate(redis_keyspace_misses_total[5m]))) < 0.8
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Low Redis hit rate"
          description: "Redis keyspace hit rate is below 80%: {{ $value | humanizePercentage }}"
```

---
*This document is based on enterprise-level Redis practice experience and continuously updated with the latest technologies and best practices.*