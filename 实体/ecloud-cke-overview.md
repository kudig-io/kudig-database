---
title: 移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南
description: 'title: 移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南'
summary: 'title: 移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南'
category: general
tags:
- cloud
- multi-cloud
- deep-dive
- etcd
- statefulset
- daemonset
- job
- ingress
- rbac
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- 移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南 是什么
- 如何 移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南
- Kubernetes 12 cloud providers 最佳实践
trigger_keywords:
- 移动云
- CKE
- China
- Mobile
- Cloud
- Kubernetes
- Engine
- 企业级深度实战指南
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南

## 产品概述与战略定位

移动云Kubernetes引擎是中国移动云基于其全球领先的网络基础设施和丰富的运营商运维经验打造的企业级容器服务。作为运营商背景的云计算服务商，CKE充分发挥了中国移动在CDN网络、5G技术、边缘计算等方面的独特优势，为企业客户提供网络性能卓越、安全性强、成本优化的容器化解决方案。

> **官方文档**: [移动云容器服务文档](https://www.cmecloud.cn/document/10026730)
> **技术基础**: 中国移动超大规模网络基础设施
> **服务特色**: CDN网络加速、5G边缘计算、专属宿主机、政企安全合规
> **性能指标**: 单集群支持3000节点，网络延迟<1ms，CDN加速提升300%

## 运营商级架构深度解析

### 控制平面运营商级设计

**多地域高可用架构**
- 控制平面跨三大运营商网络区域部署
- 与中国移动骨干网深度融合，网络延迟优化50%
- 采用电信级etcd集群配置，数据持久性99.999%
- 支持控制平面的灰度发布和秒级故障切换

**CDN网络集成优势**
```yaml
# 移动云CKE CDN优化配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cdn-optimized-application
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app: cdn-app
  template:
    metadata:
      labels:
        app: cdn-app
    spec:
      containers:
      - name: web-app
        image: cmecloud/web-app:v2.0
        ports:
        - containerPort: 80
        env:
        - name: CDN_ENDPOINT
          value: "cdn.cmecloud.cn"
        - name: NETWORK_OPTIMIZATION
          value: "enabled"
        
        # CDN就近访问优化
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
            
      # CDN节点亲和性调度
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: topology.cmecloud.cn/region
                operator: In
                values:
                - "china-mobile-cdn"
              - key: network.performance
                operator: In
                values:
                - "high-speed"
```

### 节点管理运营商特色

**专属宿主机架构**
- **物理隔离**: 完全独占物理服务器资源
- **性能保障**: 无虚拟化开销，性能提升20-30%
- **安全合规**: 满足金融、政务等高安全要求
- **灵活计费**: 按物理服务器计费，成本透明

**CDN边缘节点优化**
```yaml
# 移动云CKE边缘计算节点配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: edge-caching-agent
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: edge-caching
  template:
    metadata:
      labels:
        app: edge-caching
    spec:
      tolerations:
      - key: node-role.kubernetes.io/edge
        operator: Exists
        effect: NoSchedule
      
      containers:
      - name: caching-agent
        image: cmecloud/edge-cache:v1.5
        env:
        - name: CDN_NODE_ID
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        - name: CACHE_STORAGE_PATH
          value: "/mnt/cache-storage"
        volumeMounts:
        - name: cache-storage
          mountPath: /mnt/cache-storage
          
      volumes:
      - name: cache-storage
        hostPath:
          path: /data/cache
          type: DirectoryOrCreate
          
      # 边缘节点选择器
      nodeSelector:
        cmecloud.cn/node-type: edge
        topology.cmecloud.cn/network-latency: low
```

### 存储架构CDN优化

**CDN缓存存储方案**
- **高速缓存层**: 基于NVMe SSD的本地缓存，访问延迟<0.1ms
- **分布式存储**: 移动云分布式文件系统，支持海量小文件存储
- **对象存储集成**: 与移动云OBS深度集成，支持静态资源加速
- **智能缓存策略**: 基于访问热度的自动缓存优化

## 生产环境CDN优化部署方案

### 内容分发网络架构
```
├── 源站集群 (origin-cluster)
│   ├── 高性能计算节点池
│   ├── 多可用区部署保障
│   ├── 动态内容实时同步
│   └── 源站安全防护
├── CDN边缘节点 (cdn-edge-nodes)
│   ├── 全国2000+边缘节点
│   ├── 智能路由调度
│   ├── 动静分离优化
│   └── DDoS防护能力
└── 终端用户接入层
    ├── 最近边缘节点接入
    ├── HTTP/2协议优化
    ├── 智能压缩传输
    └── 移动端适配优化
```

### 节点规格CDN优化选型

| 应用场景 | 推荐规格 | CDN优化配置 | 网络优势 | 适用行业 |
|---------|---------|------------|----------|----------|
| 静态网站 | ecs.s3.large | 本地SSD缓存 + CDN加速 | 全国节点覆盖 | 电商、媒体 |
| 视频直播 | ecs.c3.xlarge + GPU | 边缘转码 + 实时缓存 | 低延迟传输 | 直播、教育 |
| 移动应用 | ecs.s3.medium | 智能压缩 + 协议优化 | 移动网络适配 | APP、游戏 |
| 企业门户 | 专属宿主机 | 物理隔离 + 安全加固 | 政企专线接入 | 政府、金融 |
| IoT边缘 | ecs.t3.micro | 轻量级容器 + 本地计算 | 超低延迟 | 工业物联网 |

### CDN安全加固配置

**网络访问控制策略**
```yaml
# 移动云CKE CDN安全策略配置
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: cdn-security-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: cdn-service
  policyTypes:
  - Ingress
  - Egress
  
  # CDN节点访问控制
  ingress:
  - from:
    - ipBlock:
        cidr: 117.135.0.0/16  # 移动网络IP段
    - ipBlock:
        cidr: 211.136.0.0/16  # 移动骨干网
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
      
  # 源站回源安全控制
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: origin-backend
    ports:
    - protocol: TCP
      port: 8080
    - protocol: TCP
      port: 3306
      
  # DDoS防护配置
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 80
      rateLimit: "10000req/min"  # 请求速率限制
```

**专属宿主机安全配置**
```yaml
# 移动云CKE专属宿主机RBAC配置
apiVersion: v1
kind: ServiceAccount
metadata:
  name: dedicated-host-sa
  namespace: production
  annotations:
    cmecloud.dedicated/host: "required"
    security.isolation: "physical"
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: dedicated-host-role
rules:
- apiGroups: [""]
  resources: ["pods", "services", "persistentvolumes"]
  verbs: ["get", "list", "watch", "create", "delete"]
- apiGroups: ["storage.k8s.io"]
  resources: ["storageclasses"]
  verbs: ["get", "list"]  # 专属存储类访问
- apiGroups: ["cmecloud.cn"]
  resources: ["dedicatedhosts"]
  verbs: ["get", "list", "watch"]  # 专属宿主机管理
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dedicated-host-rolebinding
  namespace: production
subjects:
- kind: ServiceAccount
  name: dedicated-host-sa
roleRef:
  kind: Role
  name: dedicated-host-role
  apiGroup: rbac.authorization.k8s.io
```

### CDN监控告警体系

**CDN性能监控配置**
```yaml
# 移动云CKE CDN监控集成配置
global:
  scrape_interval: 10s
  evaluation_interval: 10s

rule_files:
  - "cdn-performance-alerts.yaml"
  - "edge-node-health-alerts.yaml"
  - "cache-hit-ratio-alerts.yaml"

scrape_configs:
  # CDN节点监控
  - job_name: 'cdn-edge-nodes'
    static_configs:
    - targets:
      - 'cdn-monitor-1.cmecloud.cn:9100'
      - 'cdn-monitor-2.cmecloud.cn:9100'
      - 'cdn-monitor-3.cmecloud.cn:9100'
    metrics_path: '/metrics/cdn'
    
  # 缓存命中率监控
  - job_name: 'cache-performance'
    static_configs:
    - targets: ['cache-analyzer:8080']
    metrics:
    - cache_hit_ratio
    - cache_latency_ms
    - bandwidth_saving_percent
    
  # 用户体验监控
  - job_name: 'user-experience-metrics'
    kubernetes_sd_configs:
    - role: pod
      selectors:
      - role: "pod"
        label: "app=user-experience-monitor"
```

**关键CDN告警规则**
```yaml
# 移动云CKE CDN告警规则配置
groups:
- name: cmecloud.cdn.production.alerts
  rules:
  # CDN缓存命中率告警
  - alert: CacheHitRatioLow
    expr: cache_hit_ratio < 0.8
    for: 5m
    labels:
      severity: warning
      service: cdn
      team: operations
    annotations:
      summary: "CDN缓存命中率偏低"
      description: "缓存命中率 {{ $value | humanizePercentage }} 低于阈值80%"
      
  # 边缘节点健康度告警
  - alert: EdgeNodeUnhealthy
    expr: edge_node_health_status == 0
    for: 2m
    labels:
      severity: critical
      location: edge
      team: noc
    annotations:
      summary: "CDN边缘节点异常"
      description: "边缘节点 {{ $labels.node_name }} 健康状态异常"
      
  # 网络延迟告警
  - alert: NetworkLatencyHigh
    expr: network_latency_ms > 50
    for: 3m
    labels:
      severity: warning
      network: mobile
      team: network
    annotations:
      summary: "网络延迟过高"
      description: "网络延迟 {{ $value }}ms 超过移动网络标准"
      
  # 带宽利用率告警
  - alert: BandwidthUtilizationHigh
    expr: bandwidth_utilization > 90
    for: 2m
    labels:
      severity: warning
      resource: bandwidth
      team: operations
    annotations:
      summary: "带宽利用率过高"
      description: "带宽利用率 {{ $value | humanizePercentage }} 超过阈值90%"
```

## 运营商级成本优化策略

### CDN成本管理方案
```yaml
# 移动云CKE CDN成本优化配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cdn-cost-optimizer
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cdn-cost-optimizer
  template:
    metadata:
      labels:
        app: cdn-cost-optimizer
    spec:
      containers:
      - name: optimizer
        image: cmecloud/cdn-cost-optimizer:v1.2
        env:
        - name: CLUSTER_ID
          value: "cls-cdn-prod"
        - name: CDN_OPTIMIZATION_STRATEGY
          value: "mobile-network"
        - name: COST_SAVING_TARGET
          value: "0.3"  # 目标节省30%成本
        volumeMounts:
        - name: config
          mountPath: /etc/cdn-cost
      volumes:
      - name: config
        configMap:
          name: cdn-cost-optimization-config
```

**专属宿主机成本优化**
```bash
#!/bin/bash
# 移动云CKE专属宿主机成本分析脚本

CLUSTER_ID="cls-dedicated-prod"
ANALYSIS_TIME=$(date '+%Y%m%d_%H%M%S')

echo "=== 移动云CKE专属宿主机成本分析 ==="
echo "集群ID: $CLUSTER_ID"
echo "分析时间: $ANALYSIS_TIME"
echo

# 1. 专属宿主机资源使用分析
echo "1. 专属宿主机资源使用情况..."
DEDICATED_HOSTS=$(cmecloud_cli ecs DescribeDedicatedHosts \
    --RegionId "cn-beijing" \
    --DedicatedHostIds.1 $CLUSTER_ID \
    --query "DedicatedHosts[*].{ID:DedicatedHostId,CPU:Capacity.Cpu,Memory:Capacity.Memory,UsedCPU:CapacityUsed.Cpu,UsedMemory:CapacityUsed.Memory}" \
    --output table)

echo "$DEDICATED_HOSTS"

# 2. 成本效益分析
echo "2. 成本效益分析..."
for host in $DEDICATED_HOSTS; do
    HOST_ID=$(echo $host | awk '{print $1}')
    TOTAL_CPU=$(echo $host | awk '{print $2}')
    USED_CPU=$(echo $host | awk '{print $4}')
    UTILIZATION=$(echo "scale=2; $USED_CPU/$TOTAL_CPU*100" | bc)
    
    echo "宿主机 $HOST_ID: CPU利用率 ${UTILIZATION}%"
    
    if (( $(echo "$UTILIZATION < 30" | bc -l) )); then
        echo "  ⚠️  利用率偏低，建议合并或释放"
    elif (( $(echo "$UTILIZATION > 80" | bc -l) )); then
        echo "  ✅ 利用率良好"
    fi
done

# 3. CDN流量成本分析
echo "3. CDN流量成本优化建议..."
MONTHLY_TRAFFIC=$(cmecloud_cli cdn DescribeDomainFlowData \
    --DomainName "*.example.com" \
    --StartTime "$(date -d '1 month ago' +%Y-%m-%d)" \
    --EndTime "$(date +%Y-%m-%d)" \
    --query "FlowDataPerInterval[*].Value" \
    --output text | awk '{sum+=$1} END {print sum}')

echo "月流量消耗: $MONTHLY_TRAFFIC GB"

# CDN成本计算 (示例费率)
CDN_COST=$(echo "scale=2; $MONTHLY_TRAFFIC * 0.15" | bc)  # 0.15元/GB
echo "预估CDN成本: ¥$CDN_COST"

# 优化建议
if (( $(echo "$MONTHLY_TRAFFIC > 10000" | bc -l) )); then
    echo "💡 建议启用CDN智能压缩，预计节省20-40%流量成本"
    echo "💡 考虑使用移动云专属流量包，可节省30%费用"
fi
```

## 故障排查与应急响应

### CDN故障诊断工具
```bash
#!/bin/bash
# 移动云CKE CDN故障诊断脚本

DOMAIN_NAME=$1
DIAGNOSIS_TIME=$(date '+%Y%m%d_%H%M%S')
REPORT_FILE="/tmp/cdn-diagnosis-${DIAGNOSIS_TIME}.md"

exec > >(tee -a "$REPORT_FILE") 2>&1

echo "# 移动云CDN故障诊断报告"
echo "域名: $DOMAIN_NAME"
echo "诊断时间: $(date)"
echo "========================"

# 1. CDN节点状态检查
echo "## 1. CDN节点状态检查"
cmecloud_cli cdn DescribeCdnUserDomains \
    --DomainName $DOMAIN_NAME \
    --query "Domains[*].{Name:DomainName,Status:DomainStatus,CNAME:Cname}" \
    --output table

# 2. 缓存配置检查
echo "## 2. 缓存配置检查"
cmecloud_cli cdn DescribeDomainConfigs \
    --DomainName $DOMAIN_NAME \
    --query "DomainConfigs.CacheConfigs" \
    --output json

# 3. 网络连通性测试
echo "## 3. 网络连通性测试"
for i in {1..5}; do
    echo "测试 #$i:"
    curl -w "@curl-format.txt" -o /dev/null -s "http://$DOMAIN_NAME/test.html"
    echo
done

# 4. 边缘节点健康检查
echo "## 4. 边缘节点健康检查"
EDGE_NODES=$(dig +short $DOMAIN_NAME | head -5)
for node in $EDGE_NODES; do
    echo "检查节点: $node"
    ping -c 3 $node
    echo
done

echo "诊断报告已保存到: $REPORT_FILE"
```

### 运营商级应急响应预案

**一级问题响应 (Critical - CDN服务中断)**
```markdown
## 一级CDN问题响应流程

**响应时间要求**: < 5分钟 (运营商级标准)
**影响范围**: CDN服务全局中断，影响所有用户访问

### 响应步骤:

1. **立即响应 (0-1分钟)**
   - NOC自动告警触发
   - 值班工程师立即响应
   - 同时通知:
     * 运营总监
     * 技术负责人
     * 客户服务团队
   - 启动运营商级应急指挥系统

2. **快速诊断 (1-5分钟)**
   - 并行执行多路径诊断:
     * 源站可用性检查
     * CDN节点状态验证
     * 网络连通性测试
     * DNS解析检查
   - 利用移动云智能运维平台快速定位
   - 确定问题根本原因和影响范围

3. **应急处置 (5-15分钟)**
   - 执行预设的CDN应急预案
   - 启用备用源站或降级服务
   - 实施流量切换和负载重定向
   - 激活容灾备份系统
   - 持续监控服务恢复情况

4. **服务恢复 (15分钟-1小时)**
   - 验证CDN服务恢复正常
   - 逐步恢复完整服务能力
   - 监控关键性能指标
   - 确认用户体验达标
   - 向相关部门报告恢复状态

5. **事后总结**
   - 召开问题复盘会议
   - 编写运营商级事故报告
   - 分析根本原因和改进措施
   - 更新应急预案和操作手册
   - 向监管部门提交报告
```

## 运营商级特性与优势

### 网络性能优势
- **全国节点覆盖**: 2000+CDN节点，覆盖全国主要城市
- **移动网络优化**: 针对中国移动网络深度优化
- **智能路由调度**: 基于网络质量和用户位置的智能调度
- **协议优化**: HTTP/2、QUIC等新一代协议支持

### 安全合规优势
- **运营商级安全**: 符合电信行业安全标准
- **DDoS防护**: 100Gbps以上DDoS攻击防护能力
- **WAF防护**: Web应用防火墙，防止常见Web攻击
- **政企安全合规**: 满足等保三级、ISO 27001等合规要求

### 成本优化优势
- **专属流量包**: 移动云专属CDN流量优惠包
- **阶梯计费**: 流量越大单价越低
- **智能压缩**: 自动内容压缩节省30-50%流量
- **缓存优化**: 智能缓存策略减少源站回源

## 行业解决方案

### 电商平台场景
- **静态资源加速**: 图片、CSS、JS等静态资源CDN加速
- **动态内容优化**: 商品详情页、搜索结果等动态内容优化
- **移动端适配**: 针对移动APP和H5页面的专项优化
- **促销活动保障**: 大促期间的流量峰值应对方案

### 视频直播场景
- **直播推流优化**: 低延迟直播推流协议优化
- **边缘转码**: 在CDN边缘节点进行实时视频转码
- **多码率适配**: 自动适配不同网络环境的视频码率
- **全球加速**: 海外用户访问优化

### 企业官网场景
- **政企网站加速**: 政府、国企官网访问加速
- **安全防护**: 网站安全加固和攻击防护
- **专属宿主机**: 满足特殊合规要求的物理隔离
- **访问统计**: 详细的访问数据统计分析

## 客户案例

**大型电商平台CDN优化**
- **客户需求**: 双十一购物节期间的海量用户访问压力
- **解决方案**: 移动云CKE+CDN全站加速方案
- **实施效果**: 访问速度提升300%，系统稳定性达99.99%

**省级政务云平台建设**
- **客户需求**: 建设安全合规的政务门户网站
- **解决方案**: 专属宿主机+CDN安全加速方案
- **实施效果**: 通过等保三级测评，访问响应时间<100ms

**在线教育平台优化**
- **客户需求**: 在线直播课程的流畅观看体验
- **解决方案**: 移动云CKE边缘计算+CDN直播优化
- **实施效果**: 直播延迟降低至1.5秒，用户满意度提升40%

## 总结

移动云CKE凭借中国移动强大的网络基础设施和丰富的运营商运维经验，为企业客户提供了网络性能卓越、安全性强、成本优化的容器化解决方案。通过深度集成CDN网络优势、专属宿主机架构、运营商级安全防护等特色能力，成为电商、视频、政企等行业客户的理想选择。

## 生产环境部署最佳实践

### 集群规划与设计

**政企客户分层架构**
```
├── 开发测试环境 (dev-cke)
│   ├── 单可用区部署，节约成本
│   ├── 通用型实例 (ecs.ic5.large)
│   ├── 基础监控告警配置
│   └── 公网访问便于调试
├── 预生产环境 (staging-cke)
│   ├── 双可用区部署
│   ├── 计算优化型实例 (ecs.c5.xlarge)
│   ├── 增强安全配置
│   ├── 完整监控体系
│   └── 自动化测试集成
└── 生产环境 (prod-cke)
    ├── 三可用区高可用架构
    ├── 异构节点池(计算+内存优化)
    ├── 运营商级安全加固
    ├── 全链路监控告警
    ├── 灾备容灾配置
    └── 政企合规审计支持
```

**节点规格选型指南**

| 应用场景 | 推荐规格 | 配置详情 | 网络优势 | 适用行业 |
|---------|---------|---------|---------|---------|
| 电商平台 | ecs.c5.2xlarge | 8核16GB内存 + 大带宽 | CDN加速 | 电商、零售 |
| 视频直播 | ecs.g5.xlarge + GPU | 4核16GB + T4 GPU | 边缘节点部署 | 媒体、娱乐 |
| 金融科技 | ecs.r5.2xlarge | 8核64GB内存 | 专线接入 | 银行、证券 |
| 政务服务 | ecs.ic5.xlarge | 4核8GB + 安全加固 | 专属宿主机 | 政府、事业单位 |
| 工业互联网 | ecs.i3.xlarge | 4核32GB + 本地SSD | 边缘计算 | 制造、能源 |

### 安全加固配置

**运营商级网络安全策略**
```yaml
# 移动云CKE网络安全策略 - 零信任架构
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: cmcc-security-policy
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  
  # 默认拒绝所有流量
  ingress: []
  egress: []
---
# 允许必要的管理流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-management-traffic
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: management
  policyTypes:
  - Ingress
  ingress:
  # 只允许来自运维管理网络的流量
  - from:
    - ipBlock:
        cidr: 10.100.0.0/16  # 运维管理网络段
    ports:
    - protocol: TCP
      port: 22    # SSH
    - protocol: TCP
      port: 443   # HTTPS管理接口
---
# 业务系统间通信策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: business-communication-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      system: business
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 只允许同系统内的服务访问
  - from:
    - podSelector:
        matchLabels:
          system: business
    ports:
    - protocol: TCP
      port: 8080
  egress:
  # 限制对外访问
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 3306
```

**RBAC精细化权限管理**
```yaml
# 移动云CKE RBAC最佳实践配置
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cmcc-app-sa
  namespace: production
  annotations:
    ecloud.role/arn: "acs:ram::1234567890123456:role/CMCCAppRole"
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: cmcc-app-role
rules:
# 最小必要权限原则
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch", "patch"]  # 限制为只读+更新状态
- apiGroups: ["batch"]
  resources: ["jobs"]
  verbs: ["create", "get", "list", "delete"]  # 限制批处理操作
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies"]
  verbs: ["get", "list"]  # 网络策略只读
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: cmcc-app-rolebinding
  namespace: production
subjects:
- kind: ServiceAccount
  name: cmcc-app-sa
roleRef:
  kind: Role
  name: cmcc-app-role
  apiGroup: rbac.authorization.k8s.io
```

**安全组和访问控制配置**
```bash
#!/bin/bash
# 移动云CKE安全加固配置脚本

# 基础环境变量
REGION="cn-north-1"
VPC_ID="vpc-xxxxxxxx"
CLUSTER_ID="cls-xxxxxxxx"

echo "开始配置移动云CKE安全加固..."

# 1. 创建安全组
echo "1. 创建运营商级安全组..."
ecloud_cli vpc CreateSecurityGroup \
    --RegionId $REGION \
    --VpcId $VPC_ID \
    --SecurityGroupName "cmcc-cke-prod-sg" \
    --Description "移动云CKE生产环境安全组"

SECURITY_GROUP_ID=$(ecloud_cli vpc DescribeSecurityGroups \
    --RegionId $REGION \
    --VpcId $VPC_ID \
    --SecurityGroupIds.1 "cmcc-cke-prod-sg" \
    --query "SecurityGroups[0].SecurityGroupId" \
    --output text)

# 2. 配置入站规则 - 最小化开放原则
echo "2. 配置入站安全规则..."

# 只允许HTTPS管理端口
ecloud_cli vpc AuthorizeSecurityGroup \
    --RegionId $REGION \
    --SecurityGroupId $SECURITY_GROUP_ID \
    --IpProtocol tcp \
    --PortRange 443/443 \
    --SourceCidrIp "10.100.0.0/16" \
    --Policy accept \
    --Priority 100 \
    --Description "Kubernetes API Server访问"

# SSH管理端口(仅限运维网络)
ecloud_cli vpc AuthorizeSecurityGroup \
    --RegionId $REGION \
    --SecurityGroupId $SECURITY_GROUP_ID \
    --IpProtocol tcp \
    --PortRange 22/22 \
    --SourceCidrIp "10.200.0.0/16" \
    --Policy accept \
    --Priority 110 \
    --Description "运维SSH访问"

# 3. 配置出站规则 - 精细化控制
echo "3. 配置出站安全规则..."

# 允许访问内部服务
ecloud_cli vpc AuthorizeSecurityGroupEgress \
    --RegionId $REGION \
    --SecurityGroupId $SECURITY_GROUP_ID \
    --IpProtocol tcp \
    --PortRange 3306/3306 \
    --DestCidrIp "10.0.0.0/8" \
    --Policy accept \
    --Priority 100 \
    --Description "数据库访问"

# 4. 配置网络ACL
echo "4. 配置网络访问控制列表..."
ecloud_cli vpc CreateNetworkAcl \
    --RegionId $REGION \
    --VpcId $VPC_ID \
    --NetworkAclName "cmcc-cke-nacl"

NETWORK_ACL_ID=$(ecloud_cli vpc DescribeNetworkAcls \
    --RegionId $REGION \
    --VpcId $VPC_ID \
    --NetworkAclName "cmcc-cke-nacl" \
    --query "NetworkAcls[0].NetworkAclId" \
    --output text)

# 绑定到子网
SUBNET_IDS=$(ecloud_cli vpc DescribeSubnets \
    --RegionId $REGION \
    --VpcId $VPC_ID \
    --query "Subnets[*].SubnetId" \
    --output text)

for subnet_id in $SUBNET_IDS; do
    ecloud_cli vpc AssociateNetworkAcl \
        --RegionId $REGION \
        --NetworkAclId $NETWORK_ACL_ID \
        --SubnetId $subnet_id
done

echo "安全加固配置完成！"
```

### 监控告警体系

**运营商级监控指标体系**
```yaml
# 移动云CKE监控配置 - 运营商级标准
global:
  scrape_interval: 15s
  evaluation_interval: 15s

# 告警规则文件
rule_files:
  - "cmcc-cke-alerts.yaml"
  - "network-quality-alerts.yaml"
  - "cdn-performance-alerts.yaml"

# 监控目标配置
scrape_configs:
  # 核心组件监控
  - job_name: 'kubernetes-control-plane'
    static_configs:
    - targets: ['localhost:8080']  # API Server
    metrics_path: '/metrics'
    scheme: https
    tls_config:
      ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
    bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token

  # 节点性能监控
  - job_name: 'kubernetes-nodes'
    kubernetes_sd_configs:
    - role: node
    relabel_configs:
    - source_labels: [__address__]
      regex: '(.*):10250'
      target_label: __address__
      replacement: '${1}:9100'  # Node Exporter端口

  # CDN性能监控(移动云特色)
  - job_name: 'cdn-performance-monitoring'
    static_configs:
    - targets: 
      - '10.0.1.10:9091'  # CDN节点监控探针
      - '10.0.2.10:9091'  # 多地域CDN监测点
    metrics:
    - cdn_hit_rate
    - cdn_response_time_ms
    - cdn_bandwidth_utilization
```

**关键告警规则配置**
```yaml
# 移动云CKE核心告警规则 - 运营商级标准
groups:
- name: cmcc.cke.production.alerts
  rules:
  # 运营商级可用性告警
  - alert: CKEControlPlaneUnavailable
    expr: up{job="kubernetes-control-plane"} == 0
    for: 30s
    labels:
      severity: critical
      service_level: carrier-grade
      team: noc
    annotations:
      summary: "CKE控制平面不可用"
      description: "集群 {{ $labels.cluster }} 控制平面已宕机，影响运营商级服务可用性"

  # CDN性能告警(移动云特色)
  - alert: CDNPerformanceDegraded
    expr: cdn_response_time_ms > 100
    for: 2m
    labels:
      severity: warning
      service_level: carrier-grade
      team: cdn
    annotations:
      summary: "CDN性能下降"
      description: "CDN响应时间 {{ $value }}ms 超过标准(100ms)"

  # 网络质量告警
  - alert: NetworkLatencyDegraded
    expr: network_latency_ms > 30
    for: 1m
    labels:
      severity: critical
      service_level: carrier-grade
      team: network
    annotations:
      summary: "网络延迟异常"
      description: "网络延迟 {{ $value }}ms 超过运营商级标准(30ms)"

  # 边缘节点告警
  - alert: EdgeNodeOffline
    expr: edge_node_status == 0
    for: 1m
    labels:
      severity: critical
      location: edge
      team: edge
    annotations:
      summary: "边缘计算节点离线"
      description: "边缘节点 {{ $labels.node_name }} 已离线，影响就近服务"
```

**移动云监控集成配置**
```bash
#!/bin/bash
# 移动云CKE监控告警配置脚本

CLUSTER_ID="cls-xxxxxxxx"
PROJECT_ID="project-cmcc-prod"

echo "=== 移动云CKE监控告警配置 ==="

# 1. 创建监控告警组
echo "1. 创建运营商级告警组..."
ecloud_cli cms CreateAlarmGroup \
    --GroupName "CMCC-CKE-Production" \
    --Contacts.1.Name "NOC值班组" \
    --Contacts.1.Phone "138****8888" \
    --Contacts.1.Email "noc@cmcc.com" \
    --Contacts.2.Name "运维负责人" \
    --Contacts.2.Phone "139****9999" \
    --Contacts.2.Email "ops@cmcc.com"

ALARM_GROUP_ID=$(ecloud_cli cms DescribeAlarmGroups \
    --GroupName "CMCC-CKE-Production" \
    --query "AlarmGroups[0].GroupId" \
    --output text)

# 2. 配置核心指标告警
echo "2. 配置核心指标告警策略..."

# API Server可用性告警
ecloud_cli cms CreateAlarmRule \
    --RuleName "CKE-API-Server-Availability" \
    --Namespace "acs_kubernetes" \
    --MetricName "api_server_up" \
    --Dimensions.1.Key "clusterId" \
    --Dimensions.1.Value "$CLUSTER_ID" \
    --ComparisonOperator "<" \
    --Threshold 1 \
    --EvaluationCount 2 \
    --Statistics "Average" \
    --Period 60 \
    --ContactGroups.1 "$ALARM_GROUP_ID" \
    --SilenceTime 300 \
    --Escalations.1.Severity "critical" \
    --Escalations.1.Threshold 1

# 节点CPU使用率告警
ecloud_cli cms CreateAlarmRule \
    --RuleName "CKE-Node-CPU-Usage" \
    --Namespace "acs_ecs" \
    --MetricName "cpu utilization" \
    --ComparisonOperator ">" \
    --Threshold 85 \
    --EvaluationCount 3 \
    --Statistics "Average" \
    --Period 300 \
    --ContactGroups.1 "$ALARM_GROUP_ID" \
    --SilenceTime 600 \
    --Escalations.1.Severity "warning" \
    --Escalations.1.Threshold 85

# 3. 配置CDN性能告警(移动云特色)
echo "3. 配置CDN性能监控..."

# CDN命中率告警
ecloud_cli cms CreateAlarmRule \
    --RuleName "CDN-Hit-Rate-Quality" \
    --Namespace "cmcc_cdn" \
    --MetricName "hit_rate" \
    --ComparisonOperator "<" \
    --Threshold 95 \
    --EvaluationCount 2 \
    --Statistics "Average" \
    --Period 300 \
    --ContactGroups.1 "$ALARM_GROUP_ID" \
    --SilenceTime 300 \
    --Escalations.1.Severity "warning" \
    --Escalations.1.Threshold 95

echo "监控告警配置完成！"
```

### 成本优化策略

**运营商级成本管理方案**
```yaml
# 移动云CKE成本优化配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cost-optimizer
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cost-optimizer
  template:
    metadata:
      labels:
        app: cost-optimizer
    spec:
      containers:
      - name: optimizer
        image: ecloud/cost-optimizer:v1.0
        env:
        - name: CLUSTER_ID
          value: "cls-xxxxxxxx"
        - name: OPTIMIZATION_STRATEGY
          value: "carrier-enterprise"
        - name: COST_THRESHOLD
          value: "0.8"  # 成本阈值80%
        volumeMounts:
        - name: config
          mountPath: /etc/cost-optimize
      volumes:
      - name: config
        configMap:
          name: cost-optimization-config

---
# 成本优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cost-optimization-config
  namespace: kube-system
data:
  optimization-rules.yaml: |
    # 移动云CKE成本优化规则
    
    # 1. 资源规格优化
    instance_optimization:
      - workload_type: web_application
        recommended_instance: ecs.ic5.large
        cost_saving: "25%"
      - workload_type: data_processing
        recommended_instance: ecs.c5.xlarge
        cost_saving: "30%"
    
    # 2. 混合付费模式
    payment_mix:
      reserved_instances: "40%"    # 预留实例比例
      spot_instances: "30%"        # 竞价实例比例
      on_demand: "30%"             # 按量付费比例
    
    # 3. 自动扩缩容配置
    autoscaling:
      enabled: true
      min_nodes: 3
      max_nodes: 50
      scale_down_utilization_threshold: "50%"
      scale_down_unneeded_time: "10m"
```

**资源配额精细化管理**
```yaml
# 移动云CKE资源配额管理 - 运营商级标准
apiVersion: v1
kind: ResourceQuota
metadata:
  name: cmcc-production-quota
  namespace: production
spec:
  hard:
    # CPU资源配额(运营商级标准)
    requests.cpu: "200"           # 请求200核CPU
    limits.cpu: "400"             # 限制400核CPU
    
    # 内存资源配额
    requests.memory: 500Gi        # 请求500GB内存
    limits.memory: 1000Gi         # 限制1TB内存
    
    # 存储资源配额
    requests.storage: 10Ti        # 请求10TB存储
    persistentvolumeclaims: "500" # PVC数量限制
    
    # 网络资源配额
    services.loadbalancers: "100" # 负载均衡器数量
    services.nodeports: "50"      # NodePort服务数量
    
    # 对象数量配额
    pods: "10000"                 # Pod数量限制
    services: "2000"              # Service数量限制
    configmaps: "1000"            # ConfigMap数量限制
    secrets: "1000"               # Secret数量限制

---
# LimitRange配置 - 默认资源限制
apiVersion: v1
kind: LimitRange
metadata:
  name: cmcc-limit-range
  namespace: production
spec:
  limits:
  - type: Container
    default:
      cpu: "2"                    # 默认2核CPU
      memory: 4Gi                 # 默认4GB内存
    defaultRequest:
      cpu: "200m"                 # 默认请求200m CPU
      memory: 512Mi               # 默认请求512MB内存
    max:
      cpu: "16"                   # 最大16核CPU
      memory: 64Gi                # 最大64GB内存
    min:
      cpu: "10m"                  # 最小10m CPU
      memory: 4Mi                 # 最小4MB内存
```

**成本分析和优化脚本**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 移动云CKE成本分析和优化工具

CLUSTER_ID="cls-xxxxxxxx"
BILLING_CYCLE="2024-01"

echo "=== 移动云CKE成本分析报告 ==="
echo "集群ID: $CLUSTER_ID"
echo "计费周期: $BILLING_CYCLE"
echo

# 1. 获取集群成本数据
echo "1. 集群成本概览..."
TOTAL_COST=$(ecloud_cli billing QueryAccountBill \
    --BillingCycle $BILLING_CYCLE \
    --ProductCode "kubernetes" \
    --query "Data.BillingItems[?InstanceId=='$CLUSTER_ID'].RoundDownBillingAmount" \
    --output text)

echo "本月总成本: ¥$TOTAL_COST"

# 2. 按资源类型分析成本
echo "2. 资源成本明细..."
ecloud_cli billing QueryAccountBill \
    --BillingCycle $BILLING_CYCLE \
    --ProductCode "kubernetes" \
    --query "Data.BillingItems[*].{Resource:ItemName,Cost:RoundDownBillingAmount}" \
    --output table

# 3. 节点成本分析
echo "3. 节点成本分析..."
NODE_COST_DATA=$(ecloud_cli ecs DescribeInstances \
    --InstanceIds.1 $CLUSTER_ID \
    --query "Instances[*].{InstanceId:InstanceId,InstanceType:InstanceType,Status:Status}" \
    --output json)

echo "$NODE_COST_DATA" | jq -r '
[
  "实例ID\t规格\t状态\t月成本(估算)",
  (.[] | "\(.InstanceId)\t\(.InstanceType)\t\(.Status)\t¥\(calculate_cost(.InstanceType))")
] | .[]' | column -t

# 4. 成本优化建议
echo
echo "=== 成本优化建议 ==="

# 检查闲置资源
IDLE_RESOURCES=$(kubectl get pods --all-namespaces --no-headers | grep -c "Evicted|Completed")
if [ $IDLE_RESOURCES -gt 0 ]; then
    echo "⚠️  发现 $IDLE_RESOURCES 个闲置资源，建议清理"
fi

# 检查资源使用率
LOW_UTILIZATION=$(kubectl top nodes | awk 'NR>1 {if($3+0 < 30) print $1}')
if [ -n "$LOW_UTILIZATION" ]; then
    echo "⚠️  发现低利用率节点: $LOW_UTILIZATION"
    echo "   建议合并或缩小节点规格"
fi

# 生成优化配置
cat > cmcc-cost-optimization-plan.yaml << EOF
# 移动云CKE成本优化实施方案

# 1. 启用Cluster Autoscaler
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  replicas: 1
  template:
    spec:
      containers:
      - name: cluster-autoscaler
        image: ecloud/cluster-autoscaler:v1.20
        command:
        - ./cluster-autoscaler
        - --cloud-provider=ecloud
        - --nodes=3:50:node-pool-standard
        - --scale-down-utilization-threshold=0.5
        - --scale-down-unneeded-time=10m

# 2. 配置混合实例策略
apiVersion: ecloud.com/v1
kind: NodePool
metadata:
  name: cost-optimized-pool
spec:
  instanceTypes:
  - ecs.ic5.large     # 60% 按量实例
  - ecs.c5.xlarge     # 30% 预留实例  
  - spot.ecs.ic5.small # 10% 竞价实例
  scaling:
    minSize: 5
    maxSize: 30
    desiredSize: 10

# 3. 启用资源配额管理
apiVersion: v1
kind: ResourceQuota
metadata:
  name: cost-control-quota
  namespace: production
spec:
  hard:
    requests.cpu: "100"
    requests.memory: 200Gi
    limits.cpu: "200"  
    limits.memory: 400Gi
EOF

echo "优化方案已生成: cmcc-cost-optimization-plan.yaml"
echo "预计可节省成本: 20-30%"
```
## 故障排查与应急响应

### 常见问题诊断流程

**运营商级故障诊断脚本**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 移动云CKE故障诊断工具 - 运营商级标准

CLUSTER_ID="cls-xxxxxxxx"
DIAGNOSIS_TIME=$(date '+%Y%m%d_%H%M%S')
REPORT_FILE="/tmp/cmcc-cke-diagnosis-${DIAGNOSIS_TIME}.md"

exec > >(tee -a "$REPORT_FILE") 2>&1

echo "# 移动云CKE故障诊断报告"
echo "诊断时间: $(date)"
echo "集群ID: $CLUSTER_ID"
echo

# 1. 集群状态检查
echo "## 1. 集群状态检查"
CLUSTER_STATUS=$(ecloud_cli cs DescribeClusterDetail \
    --ClusterId $CLUSTER_ID \
    --query "Cluster.Status" \
    --output text)

echo "集群状态: $CLUSTER_STATUS"

if [ "$CLUSTER_STATUS" != "running" ]; then
    echo "❌ 集群状态异常"
    exit 1
fi

# 2. 节点健康检查
echo "## 2. 节点健康检查"
kubectl get nodes -o wide
echo

NOT_READY_NODES=$(kubectl get nodes | grep -v Ready | wc -l)
if [ $NOT_READY_NODES -gt 0 ]; then
    echo "❌ 发现 $NOT_READY_NODES 个NotReady节点"
    kubectl get nodes | grep -v Ready
else
    echo "✅ 所有节点状态正常"
fi

# 3. 网络质量检查(运营商特色)
echo "## 3. 网络质量检查"
NETWORK_QUALITY=$(kubectl exec -it netshoot-pod -- ping -c 5 8.8.8.8 2>/dev/null)
if [ $? -eq 0 ]; then
    echo "网络连通性: 正常"
    echo "$NETWORK_QUALITY"
else
    echo "❌ 网络连通性异常"
fi

# 4. CDN性能检查
echo "## 4. CDN性能检查"
CDN_STATUS=$(ecloud_cli cdn DescribeCdnService \
    --ClusterId $CLUSTER_ID \
    --query "Services[*].{Domain:Domain,Status:Status,HitRate:HitRate}" \
    --output table)

echo "$CDN_STATUS"

# 5. 安全合规检查
echo "## 5. 安全合规检查"
SECURITY_VIOLATIONS=$(kubectl get events --all-namespaces | grep -c "SecurityViolation")
echo "安全违规事件: $SECURITY_VIOLATIONS"

if [ $SECURITY_VIOLATIONS -gt 0 ]; then
    echo "❌ 发现安全合规问题"
    kubectl get events --all-namespaces | grep "SecurityViolation"
else
    echo "✅ 安全合规检查通过"
fi

echo
echo "诊断报告已保存到: $REPORT_FILE"
```
### 应急响应预案

**一级问题响应流程 (Critical - 运营商级服务中断)**
```markdown
## 一级问题响应 (P1 - Critical)

**响应时间要求**: < 10分钟 (运营商级标准)
**影响范围**: 核心运营商服务中断，影响大量用户

### 响应流程:

1. **立即响应阶段 (0-2分钟)**
   - NOC(网络运营中心)自动告警触发
   - 值班工程师立即响应
   - 同时通知:
     * 运维总监
     * 业务部门负责人  
     * 客户服务团队
   - 启动运营商级应急指挥系统

2. **快速诊断阶段 (2-10分钟)**
   - 并行执行多路径诊断:
     * 控制平面可用性检查
     * 核心网络连通性验证
     * CDN服务质量检测
     * 边缘节点状态确认
   - 利用移动云智能运维平台快速定位
   - 确定问题根本原因和影响范围

3. **应急处置阶段 (10-30分钟)**
   - 执行预设的运营商级应急预案
   - 启用备用集群或降级服务
   - 实施流量切换和负载重定向
   - 激活容灾备份系统
   - 持续监控服务恢复情况

4. **服务恢复阶段 (30分钟-2小时)**
   - 验证核心运营商服务恢复正常
   - 逐步恢复完整服务能力
   - 监控关键性能指标(KPI)
   - 确认用户体验达标
   - 向相关部门报告恢复状态

5. **事后总结阶段**
   - 召开问题复盘会议
   - 编写运营商级事故报告
   - 分析根本原因和改进措施
   - 更新应急预案和操作手册
   - 向监管部门提交报告
```

## 核心特性与优势

### 运营商级技术优势

**网络性能优势**
- 与中国移动骨干网络深度融合
- CDN网络节点就近接入优化
- 毫秒级网络延迟保障
- 运营商级QoS服务质量保证

**边缘计算优势**
- 全国CDN节点广泛覆盖
- 边缘计算节点低延迟部署
- 5G网络切片技术支持
- 就近服务和内容分发

**可靠性优势**
- 99.95%运营商级SLA保障
- 多地域容灾备份能力
- 秒级故障检测和切换
- 7×24小时专业运维支持

### 行业解决方案

**电商平台场景**
- CDN加速和全球内容分发
- 高并发访问和弹性扩缩容
- 智能调度和负载均衡
- 满足电商旺季流量需求

**视频直播场景**
- 边缘节点就近部署
- 低延迟直播推流和播放
- 大带宽网络支持
- 实时转码和内容分发

**政企服务场景**
- 专属宿主机和物理隔离
- 符合政企安全合规要求
- 专线接入和网络保障
- 定制化解决方案支持

## 客户案例

**大型电商平台**
- **客户需求**: 支撑双十一等大促活动的高并发访问
- **解决方案**: 利用移动云CKE的CDN集成和弹性扩缩容能力
- **实施效果**: 支撑千万级并发访问，响应时间降低40%

**视频直播平台**
- **客户需求**: 提供低延迟、高质量的直播服务
- **解决方案**: 部署边缘计算节点，结合CDN内容分发
- **实施效果**: 直播延迟降低至1秒以内，用户体验显著提升

**政务服务平台**
- **客户需求**: 建设符合政府安全要求的在线服务平台
- **解决方案**: 采用专属宿主机部署，配合安全加固方案
- **实施效果**: 通过等保三级测评，服务千万级市民用户

## 总结

移动云CKE凭借中国移动强大的网络基础设施和丰富的运营商经验，为客户提供了高性能、高可靠的容器化解决方案。通过深度融合CDN网络、边缘计算等运营商特色能力，以及完善的合规性保障，成为电商、媒体、政企等行业的理想选择。

## Related

- [[deep-dive|#deep-dive Hub]] — tag hub

- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- 06-monitoring-alerting-system
- [[实体/kubernetes.md|kubernetes]]
- [[系统基础/知识字典/workloads/daemonset.md|daemonset]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
