# 20-网络性能优化

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

网络性能优化是提升Kubernetes集群整体性能的关键环节。本文档详细介绍CNI插件调优、网络策略优化和服务网格性能调优的最佳实践。

## 🌐 CNI插件性能优化

### Calico网络优化

#### 1. Calico配置优化
```yaml
# Calico高性能配置
apiVersion: crd.projectcalico.org/v1
kind: FelixConfiguration
metadata:
  name: default
spec:
  # 性能优化参数
  reportingInterval: 0s
  logSeverityScreen: Info
  ipv6Support: false
  ipipEnabled: false
  vxlanEnabled: true
  vxlanVNI: 4096
  vxlanPort: 4789
  
  # 资源优化
  bpfEnabled: true  # 启用eBPF数据平面
  bpfLogLevel: ""
  bpfDataIfacePattern: ^(en.*|eth.*|tunl0$)
  
  # 连接跟踪优化
  conntrackMax: 524288
  conntrackMaxPerCore: 131072
  conntrackMin: 131072
  
  # 路由优化
  routeRefreshInterval: 90s
  iptablesRefreshInterval: 90s
  xdpRefreshInterval: 90s
  
  # 健康检查优化
  healthPort: 9099
  healthTimeout: 30s
---
apiVersion: crd.projectcalico.org/v1
kind: IPPool
metadata:
  name: default-ipv4-ippool
spec:
  cidr: 10.244.0.0/16
  ipipMode: Never
  vxlanMode: Always
  natOutgoing: true
  blockSize: 24
  nodeSelector: all()
  disableBGPExport: true
---
# Calico Typha配置优化
apiVersion: apps/v1
kind: Deployment
metadata:
  name: calico-typha
  namespace: calico-system
spec:
  replicas: 3
  selector:
    matchLabels:
      k8s-app: calico-typha
  template:
    metadata:
      labels:
        k8s-app: calico-typha
    spec:
      containers:
      - name: calico-typha
        image: docker.io/calico/typha:v3.26.1
        env:
        - name: TYPHA_LOGSEVERITYSCREEN
          value: "info"
        - name: TYPHA_CONNECTIONREBALANCINGMODE
          value: "kubernetes"
        - name: TYPHA_DATASTORETYPE
          value: "kubernetes"
        - name: TYPHA_HEALTHENABLED
          value: "true"
        - name: TYPHA_PROMETHEUSMETRICSENABLED
          value: "true"
        - name: TYPHA_USAGEREPORTINGENABLED
          value: "false"
        - name: TYPHA_SNAPSHOTCACHEMAXBATCHSIZE
          value: "100"
        - name: TYPHA_SNAPSHOTCACHEWORKERS
          value: "10"
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 1Gi
```

#### 2. Calico BPF模式配置
```yaml
# 启用Calico eBPF模式
apiVersion: crd.projectcalico.org/v1
kind: FelixConfiguration
metadata:
  name: default
spec:
  bpfEnabled: true
  bpfLogLevel: ""
  bpfExtToServiceConnmark: 0x80
  bpfDataIfacePattern: ^(en.*|eth.*|tunl0$)
  bpfDisableGROForIfaces: ^(kube-ipvs0$|cali.*)
  
  # BPF连接跟踪优化
  bpfConnectTimeLoadBalancingEnabled: true
  bpfHostNetworkedNATWithoutCTLB: Enabled
  bpfKubeProxyIptablesCleanupEnabled: true
---
# BPF所需内核参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: calico-bpf-sysctl
  namespace: calico-system
data:
  bpf-sysctl.conf: |
    net.core.bpf_jit_enable=1
    net.core.bpf_jit_harden=0
    net.core.bpf_jit_kallsyms=1
    net.core.netdev_max_backlog=5000
    net.core.netdev_budget=600
    net.core.netdev_budget_usecs=8000
    net.ipv4.conf.all.rp_filter=1
    net.ipv4.conf.default.rp_filter=1
    net.ipv4.tcp_congestion_control=bbr
    net.ipv4.tcp_ecn=1
    net.ipv4.tcp_fastopen=3
```

### Cilium网络优化

#### 1. Cilium高性能配置
```yaml
# Cilium优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  # 基础配置
  enable-ipv4: "true"
  enable-ipv6: "false"
  enable-bpf-clock-probe: "true"
  enable-bpf-masquerade: "true"
  enable-endpoint-health-checking: "true"
  enable-health-checking: "true"
  enable-well-known-identities: "false"
  enable-remote-node-identity: "true"
  
  # 性能优化
  bpf-ct-global-tcp-max: "524288"
  bpf-ct-global-any-max: "262144"
  bpf-nat-global-max: "841429"
  bpf-neigh-global-max: "524288"
  bpf-policy-map-max: "16384"
  
  # 负载均衡优化
  enable-session-affinity: "true"
  enable-l7-proxy: "false"  # 如不需要L7策略可关闭提升性能
  enable-bandwidth-manager: "true"
  enable-bpf-tproxy: "false"
  
  # 监控和调试
  monitor-aggregation: "medium"
  monitor-aggregation-flags: "all"
  monitor-aggregation-interval: "5s"
  preallocate-bpf-maps: "false"
  
  # 资源限制
  bpf-map-dynamic-size-ratio: "0.0025"
  disable-cnp-status-updates: "true"
  enable-k8s-event-handover: "true"
---
# Cilium DaemonSet优化
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: cilium
  namespace: kube-system
spec:
  selector:
    matchLabels:
      k8s-app: cilium
  template:
    metadata:
      labels:
        k8s-app: cilium
    spec:
      containers:
      - name: cilium-agent
        image: quay.io/cilium/cilium:v1.14.1
        args:
        - --config-dir=/tmp/cilium/config-map
        resources:
          requests:
            cpu: 100m
            memory: 512Mi
          limits:
            cpu: 1000m
            memory: 2Gi
        env:
        - name: K8S_NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        - name: CILIUM_K8S_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        - name: CILIUM_CLUSTERMESH_CONFIG
          value: /var/lib/cilium/clustermesh/
        volumeMounts:
        - name: bpf-maps
          mountPath: /sys/fs/bpf
          mountPropagation: Bidirectional
        - name: cilium-run
          mountPath: /var/run/cilium
        - name: cni-path
          mountPath: /host/opt/cni/bin
        - name: etc-cni-netd
          mountPath: /host/etc/cni/net.d
        - name: clustermesh-secrets
          mountPath: /var/lib/cilium/clustermesh
          readOnly: true
        - name: cilium-config-path
          mountPath: /tmp/cilium/config-map
          readOnly: true
        securityContext:
          privileged: true
      volumes:
      - name: bpf-maps
        hostPath:
          path: /sys/fs/bpf
          type: DirectoryOrCreate
      - name: cilium-run
        hostPath:
          path: /var/run/cilium
          type: DirectoryOrCreate
      - name: cni-path
        hostPath:
          path: /opt/cni/bin
          type: DirectoryOrCreate
      - name: etc-cni-netd
        hostPath:
          path: /etc/cni/net.d
          type: DirectoryOrCreate
      - name: clustermesh-secrets
        secret:
          secretName: cilium-clustermesh
          optional: true
      - name: cilium-config-path
        configMap:
          name: cilium-config
```

## 🎯 网络策略优化

### 策略性能调优

#### 1. 高效网络策略配置
```yaml
# 网络策略优化示例
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: optimized-app-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web-app
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 优化的标签选择器
  - from:
    - namespaceSelector:
        matchLabels:
          name: frontend
      podSelector:
        matchLabels:
          tier: frontend
    - namespaceSelector:
        matchLabels:
          name: monitoring
      podSelector:
        matchLabels:
          app: prometheus
    ports:
    - protocol: TCP
      port: 8080
    - protocol: TCP
      port: 8443
  egress:
  # 优化的目标选择
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
      podSelector:
        matchLabels:
          app: postgresql
    ports:
    - protocol: TCP
      port: 5432
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8
    ports:
    - protocol: UDP
      port: 53
---
# 全局默认拒绝策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
---
# 允许DNS查询的策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-access
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

#### 2. 策略管理工具
```python
#!/usr/bin/env python3
# 网络策略优化和管理工具

import asyncio
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import json
from datetime import datetime
import networkx as nx

class NetworkPolicyOptimizer:
    def __init__(self):
        config.load_kube_config()
        self.networking_v1 = client.NetworkingV1Api()
        self.core_v1 = client.CoreV1Api()
        self.apps_v1 = client.AppsV1Api()
        
        self.optimization_rules = {
            'redundancy_threshold': 3,  # 允许的最大重复规则数
            'complexity_threshold': 10, # 最大选择器复杂度
            'overlap_detection': True,
            'performance_weight': 0.7,
            'security_weight': 0.3
        }
    
    async def analyze_network_policies(self, namespace=None):
        """分析网络策略"""
        try:
            if namespace:
                policies = self.networking_v1.list_namespaced_network_policy(namespace)
            else:
                policies = self.networking_v1.list_network_policy_for_all_namespaces()
            
            analysis_results = {
                'timestamp': datetime.now().isoformat(),
                'total_policies': len(policies.items),
                'namespaces': {},
                'optimization_opportunities': [],
                'performance_metrics': {},
                'security_gaps': []
            }
            
            # 按命名空间分组分析
            for policy in policies.items:
                ns = policy.metadata.namespace
                if ns not in analysis_results['namespaces']:
                    analysis_results['namespaces'][ns] = {
                        'policy_count': 0,
                        'policies': [],
                        'complexity_score': 0
                    }
                
                policy_analysis = await self.analyze_single_policy(policy)
                analysis_results['namespaces'][ns]['policies'].append(policy_analysis)
                analysis_results['namespaces'][ns]['policy_count'] += 1
                analysis_results['namespaces'][ns]['complexity_score'] += policy_analysis['complexity_score']
            
            # 识别优化机会
            analysis_results['optimization_opportunities'] = await self.identify_optimization_opportunities(
                analysis_results['namespaces']
            )
            
            # 评估性能指标
            analysis_results['performance_metrics'] = await self.calculate_performance_metrics(
                analysis_results['namespaces']
            )
            
            # 检测安全缺口
            analysis_results['security_gaps'] = await self.detect_security_gaps(
                analysis_results['namespaces']
            )
            
            return analysis_results
            
        except ApiException as e:
            return {'error': f"API Error: {e}"}
    
    async def analyze_single_policy(self, policy):
        """分析单个网络策略"""
        analysis = {
            'name': policy.metadata.name,
            'namespace': policy.metadata.namespace,
            'creation_timestamp': policy.metadata.creation_timestamp.isoformat(),
            'ingress_rules': len(getattr(policy.spec, 'ingress', []) or []),
            'egress_rules': len(getattr(policy.spec, 'egress', []) or []),
            'selectors_complexity': 0,
            'complexity_score': 0,
            'overlap_potential': [],
            'recommendations': []
        }
        
        # 计算选择器复杂度
        selectors = []
        if hasattr(policy.spec, 'pod_selector') and policy.spec.pod_selector:
            selectors.extend(self.extract_selectors(policy.spec.pod_selector))
        
        if hasattr(policy.spec, 'ingress'):
            for rule in policy.spec.ingress or []:
                if hasattr(rule, 'from') and rule.from:
                    for peer in rule.from:
                        if hasattr(peer, 'pod_selector') and peer.pod_selector:
                            selectors.extend(self.extract_selectors(peer.pod_selector))
                        if hasattr(peer, 'namespace_selector') and peer.namespace_selector:
                            selectors.extend(self.extract_selectors(peer.namespace_selector))
        
        analysis['selectors_complexity'] = len(selectors)
        analysis['complexity_score'] = self.calculate_complexity_score(analysis)
        
        # 检查重叠可能性
        analysis['overlap_potential'] = await self.check_overlap_potential(policy)
        
        # 生成优化建议
        analysis['recommendations'] = self.generate_policy_recommendations(analysis)
        
        return analysis
    
    def extract_selectors(self, selector_obj):
        """提取选择器"""
        selectors = []
        if hasattr(selector_obj, 'match_labels') and selector_obj.match_labels:
            selectors.extend(selector_obj.match_labels.keys())
        if hasattr(selector_obj, 'match_expressions') and selector_obj.match_expressions:
            selectors.extend([expr.key for expr in selector_obj.match_expressions])
        return selectors
    
    def calculate_complexity_score(self, policy_analysis):
        """计算策略复杂度分数"""
        base_score = (
            policy_analysis['ingress_rules'] * 2 +
            policy_analysis['egress_rules'] * 2 +
            policy_analysis['selectors_complexity'] * 3
        )
        
        # 根据规则数量调整分数
        if base_score > 50:
            return min(100, base_score * 1.5)
        elif base_score < 10:
            return max(1, base_score * 0.5)
        else:
            return base_score
    
    async def check_overlap_potential(self, policy):
        """检查重叠可能性"""
        overlaps = []
        
        # 简化的重叠检测逻辑
        if policy.spec.pod_selector and hasattr(policy.spec.pod_selector, 'match_labels'):
            labels = policy.spec.pod_selector.match_labels or {}
            if len(labels) == 0:
                overlaps.append('Empty pod selector - matches all pods')
            
            # 检查过于宽泛的选择器
            broad_selectors = ['app', 'tier', 'version']
            if any(key in labels for key in broad_selectors) and len(labels) == 1:
                overlaps.append('Broad selector may overlap with other policies')
        
        return overlaps
    
    def generate_policy_recommendations(self, policy_analysis):
        """生成策略建议"""
        recommendations = []
        
        if policy_analysis['complexity_score'] > 50:
            recommendations.append({
                'type': 'simplification',
                'priority': 'high',
                'description': 'Policy is overly complex',
                'actions': [
                    'Consolidate similar rules',
                    'Use more specific selectors',
                    'Split into multiple focused policies'
                ]
            })
        
        if policy_analysis['selectors_complexity'] > self.optimization_rules['complexity_threshold']:
            recommendations.append({
                'type': 'selector_optimization',
                'priority': 'medium',
                'description': 'Selectors are too complex',
                'actions': [
                    'Reduce match expression complexity',
                    'Use simpler label combinations',
                    'Consider using namespace isolation'
                ]
            })
        
        if policy_analysis['overlap_potential']:
            recommendations.append({
                'type': 'overlap_prevention',
                'priority': 'medium',
                'description': 'Potential policy overlaps detected',
                'actions': policy_analysis['overlap_potential']
            })
        
        return recommendations
    
    async def identify_optimization_opportunities(self, namespaces_data):
        """识别优化机会"""
        opportunities = []
        
        for namespace, data in namespaces_data.items():
            # 检查策略冗余
            if data['policy_count'] > 10:
                opportunities.append({
                    'namespace': namespace,
                    'type': 'policy_consolidation',
                    'description': f'Too many policies ({data["policy_count"]}) in namespace',
                    'impact': 'high',
                    'recommendation': 'Consolidate overlapping policies'
                })
            
            # 检查复杂度过高
            avg_complexity = data['complexity_score'] / max(data['policy_count'], 1)
            if avg_complexity > 30:
                opportunities.append({
                    'namespace': namespace,
                    'type': 'complexity_reduction',
                    'description': f'High average policy complexity ({avg_complexity:.1f})',
                    'impact': 'medium',
                    'recommendation': 'Simplify complex policies'
                })
        
        return opportunities
    
    async def calculate_performance_metrics(self, namespaces_data):
        """计算性能指标"""
        total_policies = sum(data['policy_count'] for data in namespaces_data.values())
        total_complexity = sum(data['complexity_score'] for data in namespaces_data.values())
        
        return {
            'total_policies': total_policies,
            'average_complexity': total_complexity / max(total_policies, 1),
            'complexity_distribution': self.analyze_complexity_distribution(namespaces_data),
            'performance_score': self.calculate_performance_score(namespaces_data)
        }
    
    def analyze_complexity_distribution(self, namespaces_data):
        """分析复杂度分布"""
        complexities = []
        for data in namespaces_data.values():
            for policy in data['policies']:
                complexities.append(policy['complexity_score'])
        
        if not complexities:
            return {'min': 0, 'max': 0, 'avg': 0, 'median': 0}
        
        return {
            'min': min(complexities),
            'max': max(complexities),
            'avg': sum(complexities) / len(complexities),
            'median': sorted(complexities)[len(complexities) // 2]
        }
    
    def calculate_performance_score(self, namespaces_data):
        """计算性能分数"""
        scores = []
        for data in namespaces_data.values():
            namespace_score = 100 - (data['complexity_score'] / max(data['policy_count'], 1))
            scores.append(max(0, min(100, namespace_score)))
        
        return sum(scores) / max(len(scores), 1) if scores else 100
    
    async def detect_security_gaps(self, namespaces_data):
        """检测安全缺口"""
        gaps = []
        
        for namespace, data in namespaces_data.items():
            # 检查是否缺少默认拒绝策略
            has_default_deny = any(
                'default-deny' in policy['name'].lower() or 
                (policy['ingress_rules'] == 0 and policy['egress_rules'] == 0)
                for policy in data['policies']
            )
            
            if not has_default_deny:
                gaps.append({
                    'namespace': namespace,
                    'type': 'missing_default_deny',
                    'severity': 'high',
                    'description': 'Missing default deny policy',
                    'recommendation': 'Implement default deny-all network policy'
                })
            
            # 检查过度宽松的策略
            for policy in data['policies']:
                if any('overlap_potential' in rec['description'].lower() 
                       for rec in policy['recommendations']):
                    gaps.append({
                        'namespace': namespace,
                        'type': 'overly_permissive',
                        'severity': 'medium',
                        'description': f'Policy {policy["name"]} may be overly permissive',
                        'recommendation': 'Review and tighten policy rules'
                    })
        
        return gaps
    
    async def optimize_network_policies(self, analysis_results):
        """优化网络策略"""
        optimizations = []
        
        for opportunity in analysis_results['optimization_opportunities']:
            if opportunity['type'] == 'policy_consolidation':
                optimization = await self.consolidate_policies(opportunity['namespace'])
                optimizations.append(optimization)
            elif opportunity['type'] == 'complexity_reduction':
                optimization = await self.simplify_policies(opportunity['namespace'])
                optimizations.append(optimization)
        
        return optimizations
    
    async def consolidate_policies(self, namespace):
        """合并策略"""
        # 获取命名空间中的所有策略
        policies = self.networking_v1.list_namespaced_network_policy(namespace)
        
        # 分析相似策略
        policy_groups = self.group_similar_policies(policies.items)
        
        consolidated_policies = []
        for group in policy_groups:
            if len(group) > 1:
                consolidated = await self.create_consolidated_policy(group)
                consolidated_policies.append(consolidated)
        
        return {
            'namespace': namespace,
            'type': 'consolidation',
            'original_count': len(policies.items),
            'consolidated_count': len(consolidated_policies),
            'policies': consolidated_policies
        }
    
    def group_similar_policies(self, policies):
        """将相似策略分组"""
        # 简化的分组逻辑
        groups = []
        processed = set()
        
        for i, policy1 in enumerate(policies):
            if i in processed:
                continue
                
            group = [policy1]
            processed.add(i)
            
            for j, policy2 in enumerate(policies[i+1:], i+1):
                if j not in processed and self.policies_are_similar(policy1, policy2):
                    group.append(policy2)
                    processed.add(j)
            
            if len(group) > 1:
                groups.append(group)
        
        return groups
    
    def policies_are_similar(self, policy1, policy2):
        """判断策略是否相似"""
        # 简化的相似性判断
        try:
            # 比较Pod选择器
            sel1 = getattr(policy1.spec, 'pod_selector', None)
            sel2 = getattr(policy2.spec, 'pod_selector', None)
            
            if sel1 and sel2:
                labels1 = getattr(sel1, 'match_labels', {}) or {}
                labels2 = getattr(sel2, 'match_labels', {}) or {}
                if labels1 == labels2:
                    return True
            
            # 比较规则类型
            ingress1 = len(getattr(policy1.spec, 'ingress', []) or [])
            ingress2 = len(getattr(policy2.spec, 'ingress', []) or [])
            egress1 = len(getattr(policy1.spec, 'egress', []) or [])
            egress2 = len(getattr(policy2.spec, 'egress', []) or [])
            
            return (ingress1 > 0) == (ingress2 > 0) and (egress1 > 0) == (egress2 > 0)
            
        except Exception:
            return False
    
    async def create_consolidated_policy(self, policy_group):
        """创建合并后的策略"""
        # 简化的合并逻辑
        base_policy = policy_group[0]
        
        consolidated_spec = {
            'pod_selector': base_policy.spec.pod_selector,
            'policy_types': list(set(
                getattr(base_policy.spec, 'policy_types', []) or []
            )),
            'ingress': [],
            'egress': []
        }
        
        # 合并规则
        for policy in policy_group:
            if hasattr(policy.spec, 'ingress') and policy.spec.ingress:
                consolidated_spec['ingress'].extend(policy.spec.ingress)
            if hasattr(policy.spec, 'egress') and policy.spec.egress:
                consolidated_spec['egress'].extend(policy.spec.egress)
        
        return {
            'name': f"consolidated-{base_policy.metadata.name}",
            'spec': consolidated_spec,
            'merged_policies': [p.metadata.name for p in policy_group]
        }

# 使用示例
async def main():
    optimizer = NetworkPolicyOptimizer()
    
    # 分析网络策略
    analysis = await optimizer.analyze_network_policies()
    print("Network Policy Analysis:")
    print(json.dumps(analysis, indent=2, ensure_ascii=False))
    
    # 执行优化
    if analysis.get('optimization_opportunities'):
        optimizations = await optimizer.optimize_network_policies(analysis)
        print("\nOptimization Results:")
        print(json.dumps(optimizations, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    asyncio.run(main())
```

## 🚀 服务网格性能优化

### Istio性能调优

#### 1. Istio Sidecar优化
```yaml
# Istio Sidecar资源优化
apiVersion: v1
kind: ConfigMap
metadata:
  name: istio-sidecar-injector
  namespace: istio-system
data:
  config: |
    policy: enabled
    alwaysInjectSelector:
      []
    neverInjectSelector:
      []
    injectedAnnotations:
      sidecar.istio.io/rewriteAppHTTPProbers: "true"
    template: |
      initContainers:
      - name: istio-init
        image: docker.io/istio/proxyv2:1.18.2
        args:
        - istio-iptables
        - -p
        - "15001"
        - -z
        - "15006"
        - -u
        - "1337"
        - -m
        - REDIRECT
        - -i
        - "*"
        - -x
        - ""
        - -b
        - "*"
        - -d
        - "15090,15021,15020"
        resources:
          requests:
            cpu: 10m
            memory: 10Mi
          limits:
            cpu: 100m
            memory: 50Mi
---
# Sidecar资源配置
apiVersion: networking.istio.io/v1beta1
kind: Sidecar
metadata:
  name: default
  namespace: production
spec:
  egress:
  - hosts:
    - "./*"
    - "istio-system/*"
  ingress:
  - port:
      number: 8080
      protocol: HTTP
      name: http
    defaultEndpoint: 127.0.0.1:8080
  outboundTrafficPolicy:
    mode: REGISTRY_ONLY
---
# Envoy资源配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: envoy-config
  namespace: istio-system
data:
  envoy.yaml: |
    static_resources:
      listeners:
      - name: virtualInbound
        address:
          socket_address:
            address: 0.0.0.0
            port_value: 15006
        filter_chains:
        - filters:
          - name: envoy.filters.network.http_connection_manager
            typed_config:
              "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
              stat_prefix: inbound_http
              route_config:
                name: local_route
              http_filters:
              - name: envoy.filters.http.router
                typed_config:
                  "@type": type.googleapis.com/envoy.extensions.filters.http.router.v3.Router
              stream_idle_timeout: 0s
              common_http_protocol_options:
                idle_timeout: 1h
```

#### 2. Istio性能配置
```yaml
# Istio Pilot优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: istio
  namespace: istio-system
data:
  mesh: |-
    accessLogFile: /dev/stdout
    accessLogEncoding: JSON
    enableTracing: false  # 生产环境可关闭以提升性能
    outboundTrafficPolicy:
      mode: REGISTRY_ONLY
    proxyMetadata:
      ISTIO_META_IDLE_TIMEOUT: 1h
      ISTIO_META_STATSd_ENABLED: "false"
    defaultConfig:
      tracing:
        zipkin:
          address: zipkin.istio-system:9411
      proxyMetadata:
        ISTIO_META_DNS_CAPTURE: "true"
        ISTIO_META_DNS_AUTO_ALLOCATE: "true"
      terminationDrainDuration: 5s
      extraStatTags:
      - request_operation
      - grpc_status
      drainDuration: 45s
      parentShutdownDuration: 1m0s
      concurrency: 2
---
# Istio Ingress Gateway优化
apiVersion: apps/v1
kind: Deployment
metadata:
  name: istio-ingressgateway
  namespace: istio-system
spec:
  replicas: 3
  selector:
    matchLabels:
      app: istio-ingressgateway
  template:
    metadata:
      annotations:
        prometheus.io/port: "15020"
        prometheus.io/scrape: "true"
        sidecar.istio.io/inject: "false"
      labels:
        app: istio-ingressgateway
        istio: ingressgateway
    spec:
      containers:
      - name: istio-proxy
        image: docker.io/istio/proxyv2:1.18.2
        ports:
        - containerPort: 80
        - containerPort: 443
        - containerPort: 15021
        - containerPort: 15443
        - containerPort: 15090
        args:
        - proxy
        - router
        - --domain
        - $(POD_NAMESPACE).svc.cluster.local
        - --proxyLogLevel=warning
        - --proxyComponentLogLevel=misc:error
        - --log_output_level=default:info
        env:
        - name: JWT_POLICY
          value: third-party-jwt
        - name: PILOT_CERT_PROVIDER
          value: istiod
        - name: CA_ADDR
          value: istiod.istio-system.svc:15012
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              apiVersion: v1
              fieldPath: spec.nodeName
        - name: POD_NAME
          valueFrom:
            fieldRef:
              apiVersion: v1
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              apiVersion: v1
              fieldPath: metadata.namespace
        - name: INSTANCE_IP
          valueFrom:
            fieldRef:
              apiVersion: v1
              fieldPath: status.podIP
        - name: HOST_IP
          valueFrom:
            fieldRef:
              apiVersion: v1
              fieldPath: status.hostIP
        - name: SERVICE_ACCOUNT
          valueFrom:
            fieldRef:
              fieldPath: spec.serviceAccountName
        - name: ISTIO_META_WORKLOAD_NAME
          value: istio-ingressgateway
        - name: ISTIO_META_OWNER
          value: kubernetes://apis/apps/v1/namespaces/istio-system/deployments/istio-ingressgateway
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 2000m
            memory: 1Gi
```

## 📊 网络性能监控

### 网络指标收集

#### 1. Prometheus网络监控规则
```yaml
# 网络性能监控规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: network-performance-rules
  namespace: monitoring
spec:
  groups:
  - name: network.performance.rules
    rules:
    # 网络延迟指标
    - record: network:rtt:milliseconds
      expr: histogram_quantile(0.95, rate(container_network_receive_packets_rtt_seconds_bucket[5m])) * 1000
      
    - record: network:latency:p99
      expr: histogram_quantile(0.99, rate(container_network_transmit_packets_rtt_seconds_bucket[5m])) * 1000
      
    # 带宽利用率
    - record: network:bandwidth:utilization
      expr: (rate(container_network_receive_bytes_total[5m]) + rate(container_network_transmit_bytes_total[5m])) / 1e9 * 100
      
    # 数据包丢失率
    - record: network:packet_loss:rate
      expr: rate(container_network_receive_packets_dropped_total[5m]) / (rate(container_network_receive_packets_total[5m]) > 0)
      
    # 连接数统计
    - record: network:connections:established
      expr: sum(net_conntrack_connections{state="established"}) by (instance)
      
    # 网络错误统计
    - record: network:errors:total
      expr: sum(rate(container_network_receive_errors_total[5m]) + rate(container_network_transmit_errors_total[5m])) by (namespace, pod)
      
    # TCP连接统计
    - record: tcp:connections:active
      expr: sum(node_netstat_Tcp_ActiveOpens) by (instance)
      
    - record: tcp:connections:passive
      expr: sum(node_netstat_Tcp_PassiveOpens) by (instance)
```

#### 2. 网络性能仪表板
```json
{
  "dashboard": {
    "title": "Network Performance Dashboard",
    "panels": [
      {
        "title": "Network Throughput",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(container_network_receive_bytes_total[5m])) by (namespace)",
            "legendFormat": "RX {{namespace}}"
          },
          {
            "expr": "sum(rate(container_network_transmit_bytes_total[5m])) by (namespace)",
            "legendFormat": "TX {{namespace}}"
          }
        ]
      },
      {
        "title": "Network Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(container_network_receive_packets_rtt_seconds_bucket[5m])) * 1000",
            "legendFormat": "p95 Latency (ms)"
          },
          {
            "expr": "histogram_quantile(0.99, rate(container_network_receive_packets_rtt_seconds_bucket[5m])) * 1000",
            "legendFormat": "p99 Latency (ms)"
          }
        ]
      },
      {
        "title": "Packet Loss Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(container_network_receive_packets_dropped_total[5m]) / (rate(container_network_receive_packets_total[5m]) > 0) * 100",
            "legendFormat": "Loss Rate %"
          }
        ]
      },
      {
        "title": "TCP Connections",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(node_netstat_Tcp_CurrEstab)",
            "legendFormat": "Established Connections"
          }
        ]
      }
    ]
  }
}
```

## 🔧 实施检查清单

### 网络基础设施优化
- [ ] 选择和配置高性能CNI插件
- [ ] 优化网络插件配置参数
- [ ] 实施网络策略最佳实践
- [ ] 配置服务网格性能优化
- [ ] 优化DNS和负载均衡配置
- [ ] 实施网络监控和告警机制

### 性能调优实施
- [ ] 分析现有网络性能瓶颈
- [ ] 实施CNI插件性能优化
- [ ] 优化网络策略复杂度
- [ ] 调整服务网格资源配置
- [ ] 实施网络QoS策略
- [ ] 配置网络故障排除工具

### 监控和维护
- [ ] 部署网络性能监控系统
- [ ] 建立网络性能基线
- [ ] 实施自动化网络诊断
- [ ] 定期进行网络性能评估
- [ ] 维护网络优化文档
- [ ] 建立网络性能改进流程

---

*本文档为企业级Kubernetes网络性能优化提供完整的调优方案和实施指导*