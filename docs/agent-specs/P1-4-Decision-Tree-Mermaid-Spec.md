---
title: P1-4 决策树 Mermaid 可视化规范与实例
description: '## 1. 可视化标准概述'
summary: '## 1. 可视化标准概述'
category: general
tags:
- k8s
- etcd
- kubelet
- calico
- coredns
- docker
- daemonset
- ingress
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
- P1-4 决策树 Mermaid 可视化规范与实例 是什么
- 如何 P1-4 决策树 Mermaid 可视化规范与实例
trigger_keywords:
- P1-4
- 决策树
- Mermaid
- 可视化规范与实例
prerequisites:
- kubectl-basics
- cni-basics
- etcd-basics
---



# P1-4 决策树 Mermaid 可视化规范与实例

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 定义决策树可视化标准，供 AI Agent 生成可读的问题诊断流程图

---

## 1. 可视化标准概述

### 1.1 设计原则

| 原则 | 说明 |
|------|------|
| **层次清晰** | 父子节点从上到下排列，同级节点左对齐 |
| **色彩语义** | 不同严重程度使用不同颜色 |
| **交互友好** | 支持点击跳转和悬停提示 |
| **可执行** | Mermaid 代码可直接渲染为 SVG/PNG |

### 1.2 节点类型规范

| 节点类型 | 形状 | 颜色 | 用途 |
|----------|------|------|------|
| 顶事件 (TE) | 六边形 | 🔴 `#FF6B6B` | 问题起点 |
| 中间事件 (IE) | 菱形 | 🟠 `#FFE66D` | 复合原因 |
| 基本事件 (BE) | 圆角矩形 | 🟡 `#4ECDC4` | 单一根因 |
| 逻辑门 (OR/AND) | 边框标识 | 灰色 | 条件关系 |
| 诊断步骤 (DS) | 矩形 | 🔵 `#45B7D1` | 操作指引 |
| 修复方案 (REM) | 圆角矩形 | 🟢 `#96CEB4` | 修复动作 |
| 工具调用 (TOOL) | 虚线矩形 | 🟣 `#DDA0DD` | kubectl 命令 |

---

## 2. Mermaid 语法规范

### 2.1 基础语法

```mermaid
flowchart TD
    %% 节点定义
    TE1["顶事件: 集群不可用"]
    IE1["中间事件: 控制平面问题"]
    BE1["基本事件: etcd 空间不足"]
    DS1["诊断步骤: 检查 etcd 磁盘空间"]
    REM1["修复方案: 清理 etcd 碎片"]
    
    %% 关系定义
    TE1 --> IE1
    IE1 --> BE1
    BE1 --> DS1
    DS1 --> REM1
    
    %% 样式定义
    classDef te fill:#FF6B6B,stroke:#333,stroke-width:2px
    classDef ie fill:#FFE66D,stroke:#333,stroke-width:1px
    classDef be fill:#4ECDC4,stroke:#333,stroke-width:1px
    classDef ds fill:#45B7D1,stroke:#333,stroke-width:1px
    classDef rem fill:#96CEB4,stroke:#333,stroke-width:1px
    
    class TE1 te
    class IE1 ie
    class BE1 be
    class DS1 ds
    class REM1 rem
```

### 2.2 逻辑门表示

```mermaid
flowchart TD
    %% OR 门 - 任一子事件触发
    TE1["顶事件: Pod 启动失败"]
    OR1{{"OR"}}
    IE1["镜像拉取失败"]
    IE2["调度失败"]
    IE3["运行时错误"]
    
    TE1 --> OR1
    OR1 --> IE1
    OR1 --> IE2
    OR1 --> IE3
    
    %% AND 门 - 所有子事件同时触发
    TE2["顶事件: 数据丢失"]
    AND1{{"AND"}}
    BE1["备份未执行"]
    BE2["恢复验证失败"]
    BE3["快照损坏"]
    
    TE2 --> AND1
    AND1 --> BE1
    AND1 --> BE2
    AND1 --> BE3
    
    %% 样式
    classDef or_gate fill:#FFF,stroke:#666,stroke-width:2px,stroke-dasharray:5,5
    classDef and_gate fill:#FFF,stroke:#666,stroke-width:3px
    class OR1 or_gate
    class AND1 and_gate
```

---

## 3. 决策树实例

### 3.1 Node NotReady 决策树

```mermaid
flowchart TD
    %% ========== 顶事件 ==========
    TE["Node 异常<br/>🔴 P0"]
    
    %% ========== 第一层: 症状分类 ==========
    OR1{{"OR"}}
    TE --> OR1
    
    IE1["节点状态异常"]
    IE2["kubelet 异常"]
    IE3["容器运行时异常"]
    IE4["资源压力异常"]
    IE5["网络连通性异常"]
    
    OR1 --> IE1
    OR1 --> IE2
    OR1 --> IE3
    OR1 --> IE4
    OR1 --> IE5
    
    %% ========== 第二层: 节点状态 ==========
    IE1 --> IE1_A["Node NotReady/Unknown"]
    IE1 --> IE1_B["节点频繁重启"]
    IE1 --> IE1_C["节点被 Cordon"]
    
    %% ========== 第二层: kubelet 异常 ==========
    IE2 --> IE2_A["kubelet 服务停止"]
    IE2 --> IE2_B["心跳上报失败"]
    IE2 --> IE2_C["证书过期"]
    IE2 --> IE2_D["PLEG 不健康"]
    IE2 --> IE2_E["驱逐策略触发"]
    
    %% ========== 第三层: 详细诊断 ==========
    %% kubelet 服务
    DS1["📋 执行: kubectl get node ${NODE} -o json<br/>查看 status.conditions"]
    IE2_A --> DS1
    
    DS2["📋 执行: ssh ${NODE} 'systemctl status kubelet'<br/>查看服务状态"]
    DS1 --> DS2
    
    %% 心跳失败
    DS3["📋 执行: kubectl get lease -n kube-node-lease ${NODE}<br/>检查 Lease 更新时间"]
    IE2_B --> DS3
    
    %% 证书过期
    DS4["📋 执行: ssh ${NODE} 'openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates'<br/>检查证书有效期"]
    IE2_C --> DS4
    
    %% PLEG
    AND_PLEG{{"AND"}}
    IE2_D --> AND_PLEG
    AND_PLEG --> BE_PLEG1["PLEG relist 超时"]
    AND_PLEG --> BE_PLEG2["容器数量过多"]
    
    %% ========== 修复方案 ==========
    REM1["🛠️ 证书修复: kubectl certificate approve <csr-name><br/>⚠️ 风险: HIGH"]
    IE2_C --> REM1
    
    REM2["🛠️ 重启 kubelet: systemctl restart kubelet<br/>⚠️ 风险: MEDIUM"]
    IE2_A --> REM2
    
    REM3["🛠️ 驱逐策略调整: 降低 eviction threshold<br/>⚠️ 风险: MEDIUM"]
    IE2_E --> REM3
    
    %% ========== 样式 ==========
    classDef te fill:#FF6B6B,stroke:#333,stroke-width:3px,color:#fff
    classDef ie fill:#FFE66D,stroke:#333,stroke-width:2px
    classDef be fill:#4ECDC4,stroke:#333,stroke-width:1px
    classDef ds fill:#45B7D1,stroke:#333,stroke-width:1px,color:#fff
    classDef rem fill:#96CEB4,stroke:#333,stroke-width:2px
    classDef or_gate fill:#FFF,stroke:#666,stroke-width:2px
    classDef and_gate fill:#FFF,stroke:#666,stroke-width:3px
    
    class TE te
    class OR1,IE1,IE2,IE3,IE4,IE5 ie
    class IE1_A,IE1_B,IE1_C,IE2_A,IE2_B,IE2_C,IE2_D,IE2_E ie
    class BE_PLEG1,BE_PLEG2 be
    class DS1,DS2,DS3,DS4 ds
    class REM1,REM2,REM3 rem
    class AND_PLEG,and_gate or_gate
```

### 3.2 Pod Pending 决策树

```mermaid
flowchart TD
    %% ========== 顶事件 ==========
    TE["Pod Pending<br/>🟠 P1"]
    
    %% ========== 调度失败分类 ==========
    OR1{{"OR"}}
    TE --> OR1
    
    IE1["调度器无法分配节点"]
    IE2["镜像拉取失败"]
    IE3["资源配置问题"]
    IE4["权限/策略问题"]
    
    OR1 --> IE1
    OR1 --> IE2
    OR1 --> IE3
    OR1 --> IE4
    
    %% ========== 调度器问题 ==========
    IE1 --> IE1_A["资源不足"]
    IE1 --> IE1_B["亲和性/反亲和性冲突"]
    IE1 --> IE1_C["污点不容忍"]
    IE1 --> IE1_D["拓扑约束不满足"]
    
    %% 资源不足详情
    DS1["📋 执行: kubectl describe pod ${POD} -n ${NS}<br/>查看 'Events' 中的 'FailedScheduling'"]
    IE1_A --> DS1
    
    DS2["📋 执行: kubectl describe node<br/>查看 'Allocated resources'"]
    DS1 --> DS2
    
    REM1["🛠️ 扩容节点 或 调整 resource limits<br/>⚠️ 风险: MEDIUM"]
    IE1_A --> REM1
    
    %% 亲和性冲突
    DS3["📋 执行: kubectl get pod ${POD} -o jsonpath='{.spec.affinity}'<br/>查看亲和性配置"]
    IE1_B --> DS3
    
    DS4["📋 执行: kubectl get pods -l ${selector} -o wide<br/>检查现有 Pod 分布"]
    DS3 --> DS4
    
    REM2["🛠️ 调整 affinity rules 或 remove conflicting pods<br/>⚠️ 风险: MEDIUM"]
    IE1_B --> REM2
    
    %% ========== 镜像拉取 ==========
    IE2 --> IE2_A["镜像不存在"]
    IE2 --> IE2_B["认证凭据错误"]
    IE2 --> IE2_C["网络访问限制"]
    
    DS5["📋 执行: kubectl describe pod ${POD}<br/>查看 'Events' 中的 'ImagePullBackOff'"]
    IE2 --> DS5
    
    REM3["🛠️ 修复镜像标签 或 更新 ImagePullSecrets<br/>⚠️ 风险: LOW"]
    IE2_A --> REM3
    
    REM4["🛠️ 创建 docker-registry secret<br/>kubectl create secret docker-registry reg-secret<br/>--docker-server=${REGISTRY} --docker-username=${USER} --docker-password=${PASS}<br/>⚠️ 风险: LOW"]
    IE2_B --> REM4
    
    %% ========== 样式 ==========
    classDef te fill:#FFE66D,stroke:#333,stroke-width:3px,color:#333
    classDef ie fill:#FFF,stroke:#333,stroke-width:1px
    classDef ds fill:#45B7D1,stroke:#333,stroke-width:1px,color:#fff
    classDef rem fill:#96CEB4,stroke:#333,stroke-width:2px
    classDef or_gate fill:#FFF,stroke:#666,stroke-width:2px
    
    class TE te
    class OR1,IE1,IE2,IE3,IE4 ie
    class IE1_A,IE1_B,IE1_C,IE1_D,IE2_A,IE2_B,IE2_C ie
    class DS1,DS2,DS3,DS4,DS5 ds
    class REM1,REM2,REM3,REM4 rem
    class OR1 or_gate
```

### 3.3 网络连通性决策树

```mermaid
flowchart TD
    %% ========== 顶事件 ==========
    TE["网络连通性异常<br/>🟠 P1"]
    
    %% ========== 症状分类 ==========
    OR1{{"OR"}}
    TE --> OR1
    
    IE1["DNS 解析异常"]
    IE2["Pod 间通信异常"]
    IE3["外部访问异常"]
    IE4["Service/Ingress 异常"]
    
    OR1 --> IE1
    OR1 --> IE2
    OR1 --> IE3
    OR1 --> IE4
    
    %% ========== DNS 异常 ==========
    IE1 --> IE1_A["CoreDNS Pod 不健康"]
    IE1 --> IE1_B["DNS 配置错误"]
    IE1 --> IE1_C["网络策略阻止"]
    
    DS1["📋 执行: kubectl get pods -n kube-system -l k8s-app=kube-dns<br/>检查 CoreDNS Pod 状态"]
    IE1_A --> DS1
    
    DS2["📋 执行: kubectl logs -n kube-system -l k8s-app=kube-dns<br/>查看 DNS 日志错误"]
    DS1 --> DS2
    
    REM1["🛠️ 重启 CoreDNS: kubectl rollout restart deployment/coredns -n kube-system<br/>⚠️ 风险: LOW"]
    IE1_A --> REM1
    
    %% ========== Pod 间通信 ==========
    IE2 --> IE2_A["CNI 组件异常"]
    IE2 --> IE2_B["iptables/ipvs 规则错误"]
    IE2 --> IE2_C["网络命名空间问题"]
    
    DS3["📋 执行: kubectl exec ${POD} -- nslookup [[entities/kubernetes.md|kubernetes]].default<br/>测试集群 DNS 解析"]
    IE2 --> DS3
    
    DS4["📋 执行: kubectl exec ${POD} -- ping ${TARGET_POD_IP}<br/>测试 Pod 间直接通信"]
    DS3 --> DS4
    
    REM2["🛠️ 重启 CNI: kubectl rollout restart daemonset/calico-node -n kube-system<br/>⚠️ 风险: MEDIUM"]
    IE2_A --> REM2
    
    %% ========== Service/Ingress ==========
    IE4 --> IE4_A["Service 无 Endpoints"]
    IE4 --> IE4_B["Ingress 路由错误"]
    IE4 --> IE4_C["LoadBalancer 配置问题"]
    
    DS5["📋 执行: kubectl get endpoints ${SVC} -n ${NS}<br/>检查 Endpoints 是否存在"]
    IE4_A --> DS5
    
    DS6["📋 执行: kubectl describe ingress ${ING} -n ${NS}<br/>查看 Ingress 配置和事件"]
    IE4_B --> DS6
    
    REM3["🛠️ 修复 Pod selector 匹配 或 重建 Service<br/>⚠️ 风险: MEDIUM"]
    IE4_A --> REM3
    
    %% ========== 样式 ==========
    classDef te fill:#FFE66D,stroke:#333,stroke-width:3px,color:#333
    classDef ie fill:#FFF,stroke:#333,stroke-width:1px
    classDef ds fill:#45B7D1,stroke:#333,stroke-width:1px,color:#fff
    classDef rem fill:#96CEB4,stroke:#333,stroke-width:2px
    classDef or_gate fill:#FFF,stroke:#666,stroke-width:2px
    
    class TE te
    class OR1,IE1,IE2,IE3,IE4 ie
    class IE1_A,IE1_B,IE1_C,IE2_A,IE2_B,IE2_C,IE4_A,IE4_B,IE4_C ie
    class DS1,DS2,DS3,DS4,DS5,DS6 ds
    class REM1,REM2,REM3 rem
    class OR1 or_gate
```

---

## 4. 决策树生成模板

### 4.1 自动化生成脚本

```python
#!/usr/bin/env python3
"""
决策树 Mermaid 生成器
根据 FTA JSON 输入生成 Mermaid 格式的决策树
"""

import json
from typing import Dict, List

class FTAMermaidGenerator:
    def __init__(self, fta_data: Dict):
        self.fta = fta_data
        self.nodes = []
        self.edges = []
        self.classes = []
        
    def generate(self) -> str:
        """生成完整的 Mermaid 代码"""
        self._add_header()
        self._process_events()
        self._add_styles()
        return self._build_mermaid()
    
    def _add_header(self):
        """添加 Mermaid 头部"""
        self.nodes.append("flowchart TD")
        self.nodes.append("    %% ========== 顶事件 ==========")
        te = self.fta['top_event']
        self.nodes.append(f'    TE["{te["title"]}<br/>🔴 {te["severity"]}"]')
        self.edges.append("    TE --> OR1")
        
    def _process_events(self):
        """处理事件树"""
        # ... 递归处理事件
        
    def _add_styles(self):
        """添加样式定义"""
        self.classes.append("    classDef te fill:#FF6B6B,stroke:#333,stroke-width:3px,color:#fff")
        self.classes.append("    classDef ie fill:#FFE66D,stroke:#333,stroke-width:1px")
        self.classes.append("    classDef be fill:#4ECDC4,stroke:#333,stroke-width:1px")
        self.classes.append("    classDef ds fill:#45B7D1,stroke:#333,stroke-width:1px,color:#fff")
        self.classes.append("    classDef rem fill:#96CEB4,stroke:#333,stroke-width:2px")
        
    def _build_mermaid(self) -> str:
        """构建最终 Mermaid 代码"""
        sections = [
            "\n".join(self.nodes),
            "\n".join(self.edges),
            "\n".join(self.classes)
        ]
        return "\n".join(sections)

# 使用示例
if __name__ == "__main__":
    sample_fta = {
        "top_event": {
            "id": "TE-001",
            "title": "集群不可用",
            "severity": "P0"
        }
    }
    generator = FTAMermaidGenerator(sample_fta)
    print(generator.generate())
```

### 4.2 Mermaid 配置

```yaml
mermaid:
  theme: default
  flowchart:
    curve: linear
    padding: 20
    nodeSpacing: 50
    rankSpacing: 80
  securityLevel: loose
  startOnLoad: true
```

---

## 5. 渲染与交互

### 5.1 渲染方式

| 方式 | 命令 | 适用场景 |
|------|------|---------|
| Mermaid Live | https://mermaid.live | 实时预览 |
| mkdocs-mermaid | 文档站点 | 静态生成 |
| Docusaurus | 文档站点 | 支持 Mermaid |
| VuePress | 文档站点 | Mermaid 插件 |

### 5.2 mkdocs.yml 配置

```yaml
markdown_extensions:
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:pymdownx.superfences.fence_code_format
```

---

## 6. 质量检查清单

- [ ] 顶事件必须标注严重程度 (P0/P1/P2)
- [ ] 所有逻辑门 (OR/AND) 必须清晰标注
- [ ] 每个基本事件必须有对应的诊断步骤
- [ ] 每个诊断步骤必须包含实际可执行的命令
- [ ] 每个修复方案必须标注风险等级
- [ ] 修复方案必须包含回滚方法
- [ ] Mermaid 代码可通过 https://mermaid.live 渲染验证

---

**下一步行动**: 在 domain-10-troubleshooting-diagnostics 目录下所有文档中应用此规范，将现有问题排查流程转换为 Mermaid 决策树格式。