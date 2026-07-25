---
title: 09-软件物料清单
description: '- software-bill-of-materials的最佳实践'
summary: '- software-bill-of-materials的最佳实践'
category: general
tags:
- k8s
- prometheus
- grafana
- harbor
- opa
- job
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 软件物料清单 是什么
- 如何 软件物料清单
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- 软件物料清单
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- prometheus-basics
- monitoring-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
tags:
- k8s
- production
- best-practice
intent_queries:
- software-bill-of-materials是什么？
- software-bill-of-materials的使用方法
- software-bill-of-materials的最佳实践

tier: peripheral---
title: 09-软件物料清单
description: '<!-- chunk: 📋 概述' -->## 📋 概述'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- [[Prometheus|prometheus]]
- grafana
- [[Harbor|harbor]]
- opa
- job
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- 软件物料清单 是什么
- 如何 软件物料清单
- [[Kubernetes|Kubernetes]] 18 production operations 最佳实践
trigger_keywords:
- 软件物料清单
- production
- operations
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 09-软件物料清单

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 📋 概述 -->## 📋 概述

软件物料清单(SBOM)是现代软件供应链安全管理的基础。本文档详细介绍如何在Kubernetes环境中生成、管理和利用SBOM来提升安全性。

<!-- chunk: 📦 SBOM基础概念 -->## 📦 SBOM基础概念

## SBOM标准格式

## 1. SPDX格式示例
```json
{
  "spdxVersion": "SPDX-2.3",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "name": "myapp-container-image",
  "documentNamespace": "https://example.com/sbom/myapp-1.0.0",
  "creationInfo": {
    "creators": [
      "Tool: syft-0.84.1"
    ],
    "created": "2024-01-15T10:30:00Z"
  },
  "packages": [
    {
      "name": "alpine-baselayout",
      "SPDXID": "SPDXRef-Package-alpine-baselayout-3.2.0-r22",
      "versionInfo": "3.2.0-r22",
      "supplier": "Person: Natanael Copa <ncopa@alpinelinux.org>",
      "downloadLocation": "https://dl-cdn.alpinelinux.org/alpine/v3.18/main/x86_64/",
      "filesAnalyzed": false,
      "licenseConcluded": "GPL-2.0-only",
      "licenseDeclared": "GPL-2.0-only",
      "copyrightText": "Copyright 1999-2003 David I. Bell and others",
      "externalRefs": [
        {
          "referenceCategory": "PACKAGE-MANAGER",
          "referenceType": "purl",
          "referenceLocator": "pkg:apk/alpine/alpine-baselayout@3.2.0-r22?arch=x86_64&upstream=alpine-baselayout&distro=alpine-3.18.4"
        }
      ]
    }
  ]
}
```

## 2. CycloneDX格式示例
```json
{
  "bomFormat": "CycloneDX",
  "specVersion": "1.5",
  "serialNumber": "urn:uuid:3e671687-395b-41f5-a30f-a58921a69b79",
  "version": 1,
  "metadata": {
    "timestamp": "2024-01-15T10:30:00Z",
    "tools": [
      {
        "vendor": "anchore",
        "name": "syft",
        "version": "0.84.1"
      }
    ],
    "component": {
      "type": "container",
      "name": "myapp",
      "version": "1.0.0"
    }
  },
  "components": [
    {
      "type": "library",
      "name": "express",
      "version": "4.18.2",
      "purl": "pkg:npm/express@4.18.2",
      "licenses": [
        {
          "license": {
            "id": "MIT"
          }
        }
      ],
      "externalReferences": [
        {
          "type": "issue-tracker",
          "url": "https://github.com/expressjs/express/issues"
        }
      ]
    }
  ]
}
```

<!-- chunk: 🛠️ SBOM生成工具 -->## 🛠️ SBOM生成工具

## Syft工具配置

## 1. 容器镜像SBOM生成
```yaml
# Syft配置文件
apiVersion: batch/v1
kind: Job
metadata:
  name: sbom-generator
  namespace: security
spec:
  template:
    spec:
      containers:
      - name: syft
        image: anchore/syft:v0.84.1
        args:
        - "registry.example.com/myapp:latest"
        - "-o"
        - "spdx-json"
        - "--file"
        - "/results/sbom-spdx.json"
        volumeMounts:
        - name: results
          mountPath: /results
        env:
        - name: SYFT_REGISTRY_AUTH_AUTHORITY
          valueFrom:
            secretKeyRef:
              name: registry-credentials
              key: authority
        - name: SYFT_REGISTRY_AUTH_USERNAME
          valueFrom:
            secretKeyRef:
              name: registry-credentials
              key: username
        - name: SYFT_REGISTRY_AUTH_PASSWORD
          valueFrom:
            secretKeyRef:
              name: registry-credentials
              key: password
      volumes:
      - name: results
        emptyDir: {}
      restartPolicy: Never
```

## 2. 多格式输出配置
```bash
#!/bin/bash
# 多格式SBOM生成脚本

IMAGE_NAME="myapp:latest"
OUTPUT_DIR="/sbom-output"

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 生成不同格式的SBOM
syft "$IMAGE_NAME" -o spdx-json > "$OUTPUT_DIR/sbom-spdx.json"
syft "$IMAGE_NAME" -o cyclonedx-json > "$OUTPUT_DIR/sbom-cyclonedx.json"
syft "$IMAGE_NAME" -o table > "$OUTPUT_DIR/sbom-table.txt"
syft "$IMAGE_NAME" -o json > "$OUTPUT_DIR/sbom-raw.json"

# 验证SBOM格式
echo "Validating SBOM formats..."
sbom-tool validate --input-file "$OUTPUT_DIR/sbom-spdx.json" --spec-version SPDX-2.3
sbom-tool validate --input-file "$OUTPUT_DIR/sbom-cyclonedx.json" --spec-version CycloneDX-1.5

echo "SBOM generation completed:"
ls -la "$OUTPUT_DIR"
```

## Grype漏洞扫描集成

## 1. 漏洞扫描配置
```yaml
# Grype扫描Job
apiVersion: batch/v1
kind: Job
metadata:
  name: vulnerability-scan
  namespace: security
spec:
  template:
    spec:
      containers:
      - name: grype
        image: anchore/grype:latest
        args:
        - "registry.example.com/myapp:latest"
        - "-o"
        - "json"
        - "--file"
        - "/results/vulnerabilities.json"
        volumeMounts:
        - name: results
          mountPath: /results
        env:
        - name: GRYPE_DB_AUTO_UPDATE
          value: "true"
        - name: GRYPE_DB_CACHE_DIR
          value: "/tmp/grype-db"
      volumes:
      - name: results
        emptyDir: {}
      restartPolicy: Never
---
# 扫描结果分析
apiVersion: v1
kind: ConfigMap
metadata:
  name: vulnerability-analysis
  namespace: security
data:
  analyze-vulns.sh: |
    #!/bin/bash
    VULN_FILE="/results/vulnerabilities.json"
    
    # 提取严重漏洞
    jq '.matches[] | select(.vulnerability.severity == "Critical" or .vulnerability.severity == "High")' "$VULN_FILE" > /results/critical-vulns.json
    
    # 统计漏洞分布
    jq '.matches | group_by(.vulnerability.severity) | map({severity: .[0].vulnerability.severity, count: length})' "$VULN_FILE" > /results/vuln-summary.json
    
    # 检查已知漏洞
    if [ -s /results/critical-vulns.json ]; then
      echo "CRITICAL VULNERABILITIES DETECTED"
      exit 1
    fi
```

<!-- chunk: 📊 SBOM管理平台 -->## 📊 SBOM管理平台

## Harbor SBOM集成

## 1. Harbor配置
```yaml
# Harbor SBOM插件配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: harbor-sbom-config
  namespace: harbor
data:
  sbom_scanner.yaml: |
    scanner:
      name: "Trivy"
      vendor: "Aqua Security"
      version: "0.40.0"
    
    capabilities:
      - type: "sbom"
        consumes_mime_types:
          - "application/vnd.oci.image.manifest.v1+json"
        produces_mime_types:
          - "application/spdx+json"
          - "application/vnd.cyclonedx+json"
    
    properties:
      - name: "harbor.scanner-adapter/scanner-type"
        value: "os-package-vulnerability"
```

## 2. 自动化SBOM生成
```yaml
# 镜像推送时自动生成SBOM
apiVersion: tekton.dev/v1beta1
kind: Pipeline
metadata:
  name: build-and-scan-pipeline
  namespace: ci-cd
spec:
  workspaces:
  - name: shared-data
  tasks:
  - name: build-image
    taskRef:
      name: buildah
    workspaces:
    - name: source
      workspace: shared-data
    params:
    - name: IMAGE
      value: "$(params.image-url)"
      
  - name: generate-sbom
    taskRef:
      name: syft-sbom-generator
    runAfter:
    - build-image
    workspaces:
    - name: source
      workspace: shared-data
    params:
    - name: IMAGE
      value: "$(params.image-url)"
    - name: OUTPUT_FORMAT
      value: "spdx-json"
      
  - name: vulnerability-scan
    taskRef:
      name: grype-scanner
    runAfter:
    - generate-sbom
    workspaces:
    - name: source
      workspace: shared-data
    params:
    - name: SBOM_FILE
      value: "$(workspaces.source.path)/sbom.json"
```

<!-- chunk: 🔍 依赖关系分析 -->## 🔍 依赖关系分析

## 依赖树可视化

## 1. 依赖关系图生成
```python
#!/usr/bin/env python3
# 依赖关系分析脚本

import json
import networkx as nx
import matplotlib.pyplot as plt
from networkx.drawing.nx_agraph import graphviz_layout

def analyze_dependencies(sbom_file):
    """分析SBOM中的依赖关系"""
    with open(sbom_file, 'r') as f:
        sbom_data = json.load(f)
    
    # 构建依赖图
    G = nx.DiGraph()
    
    # 添加组件节点
    for component in sbom_data.get('components', []):
        pkg_id = component.get('bom-ref', component.get('SPDXID'))
        name = component.get('name')
        version = component.get('version')
        G.add_node(pkg_id, name=name, version=version)
    
    # 添加依赖关系
    for dep in sbom_data.get('dependencies', []):
        ref = dep.get('ref')
        depends_on = dep.get('dependsOn', [])
        for dependency in depends_on:
            G.add_edge(ref, dependency)
    
    return G

def visualize_dependencies(graph, output_file):
    """可视化依赖关系图"""
    plt.figure(figsize=(15, 10))
    
    # 使用层次布局
    pos = graphviz_layout(graph, prog='dot')
    
    # 绘制节点
    nx.draw_networkx_nodes(graph, pos, node_size=1000, 
                          node_color='lightblue', alpha=0.7)
    
    # 绘制边
    nx.draw_networkx_edges(graph, pos, edge_color='gray', 
                          arrows=True, arrowstyle='->', arrowsize=20)
    
    # 添加标签
    labels = nx.get_node_attributes(graph, 'name')
    versions = nx.get_node_attributes(graph, 'version')
    
    combined_labels = {}
    for node in graph.nodes():
        name = labels.get(node, node)
        version = versions.get(node, '')
        combined_labels[node] = f"{name}\n{version}" if version else name
    
    nx.draw_networkx_labels(graph, pos, labels=combined_labels, 
                           font_size=8, font_weight='bold')
    
    plt.title("Software Dependency Graph")
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.show()

# 使用示例
if __name__ == "__main__":
    sbom_file = "sbom-cyclonedx.json"
    graph = analyze_dependencies(sbom_file)
    visualize_dependencies(graph, "dependency-graph.png")
    
    # 输出统计信息
    print(f"Total components: {graph.number_of_nodes()}")
    print(f"Total dependencies: {graph.number_of_edges()}")
    print(f"Connected components: {nx.number_weakly_connected_components(graph)}")
```

## 2. 许可证合规检查
```yaml
# 许可证合规检查配置
apiVersion: batch/v1
kind: Job
metadata:
  name: license-compliance-check
  namespace: security
spec:
  template:
    spec:
      containers:
      - name: license-checker
        image: fossa/fossa-cli:latest
        command:
        - /bin/sh
        - -c
        - |
          # 分析SBOM中的许可证
          fossa analyze --sbom-file=/sbom/sbom-spdx.json
          
          # 检查禁止的许可证
          fossa report attribution --format json > /results/attribution.json
          
          # 生成合规报告
          python3 /scripts/check-licenses.py /results/attribution.json
        volumeMounts:
        - name: sbom-volume
          mountPath: /sbom
        - name: results
          mountPath: /results
        - name: scripts
          mountPath: /scripts
      volumes:
      - name: sbom-volume
        configMap:
          name: generated-sbom
      - name: results
        emptyDir: {}
      - name: scripts
        configMap:
          name: license-scripts
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: license-scripts
  namespace: security
data:
  check-licenses.py: |
    #!/usr/bin/env python3
    import json
    import sys
    
    # 禁止的许可证列表
    PROHIBITED_LICENSES = [
        'GPL-2.0',
        'GPL-3.0',
        'AGPL-3.0',
        'LGPL-3.0'
    ]
    
    def check_licenses(attribution_file):
        with open(attribution_file, 'r') as f:
            data = json.load(f)
        
        violations = []
        for component in data.get('components', []):
            licenses = component.get('licenses', [])
            for license_info in licenses:
                license_id = license_info.get('license', {}).get('id', '')
                if license_id in PROHIBITED_LICENSES:
                    violations.append({
                        'component': component.get('name'),
                        'version': component.get('version'),
                        'license': license_id,
                        'reason': 'Prohibited license detected'
                    })
        
        if violations:
            print("LICENSE VIOLATIONS FOUND:")
            for violation in violations:
                print(f"  - {violation['component']} {violation['version']}: {violation['license']}")
            return False
        else:
            print("All licenses compliant")
            return True
    
    if __name__ == "__main__":
        if len(sys.argv) != 2:
            print("Usage: python3 check-licenses.py <attribution-file>")
            sys.exit(1)
        
        if not check_licenses(sys.argv[1]):
            sys.exit(1)
```

<!-- chunk: 🛡️ 安全合规集成 -->## 🛡️ 安全合规集成

## 供应链安全框架

## 1. SLSA合规检查
```yaml
# SLSA合规验证
apiVersion: batch/v1
kind: Job
metadata:
  name: slsa-compliance-check
  namespace: security
spec:
  template:
    spec:
      containers:
      - name: slsa-verifier
        image: slsa-framework/slsa-verifier:v2.4.1
        args:
        - "verify-artifact"
        - "--provenance-repository"
        - "https://github.com/example/myapp"
        - "--source-uri"
        - "github.com/example/myapp"
        - "--source-tag"
        - "v1.0.0"
        - "/artifacts/myapp-linux-amd64"
        volumeMounts:
        - name: artifacts
          mountPath: /artifacts
      volumes:
      - name: artifacts
        persistentVolumeClaim:
          claimName: build-artifacts-pvc
```

## 2. 签名验证配置
```yaml
# Cosign签名验证
apiVersion: batch/v1
kind: Job
metadata:
  name: image-signature-verification
  namespace: security
spec:
  template:
    spec:
      containers:
      - name: cosign
        image: sigstore/cosign:v2.0.0
        command:
        - /bin/sh
        - -c
        - |
          # 验证镜像签名
          cosign verify \
            --certificate-identity-regexp "https://github.com/example/myapp/.*" \
            --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
            registry.example.com/myapp:latest
          
          # 验证SBOM签名
          cosign verify-blob \
            --signature sbom.sig \
            --certificate sbom.pem \
            sbom-spdx.json
```

<!-- chunk: 📈 监控与报告 -->## 📈 监控与报告

## SBOM质量指标

## 1. Prometheus指标收集
```yaml
# SBOM质量监控指标
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: sbom-quality-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: sbom-analyzer
  endpoints:
  - port: metrics
    path: /metrics
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sbom-analyzer
  namespace: security
spec:
  replicas: 1
  selector:
    matchLabels:
      app: sbom-analyzer
  template:
    metadata:
      labels:
        app: sbom-analyzer
    spec:
      containers:
      - name: analyzer
        image: custom/sbom-analyzer:latest
        ports:
        - containerPort: 8080
          name: metrics
        env:
        - name: SBOM_STORAGE_PATH
          value: "/sbom-storage"
        volumeMounts:
        - name: sbom-storage
          mountPath: /sbom-storage
      volumes:
      - name: sbom-storage
        persistentVolumeClaim:
          claimName: sbom-storage-pvc
```

## 2. Grafana仪表板
```json
{
  "dashboard": {
    "title": "SBOM Quality Metrics",
    "panels": [
      {
        "title": "SBOM Coverage Rate",
        "type": "gauge",
        "targets": [
          {
            "expr": "sbom_coverage_rate",
            "legendFormat": "Coverage %"
          }
        ]
      },
      {
        "title": "Vulnerability Trends",
        "type": "graph",
        "targets": [
          {
            "expr": "sbom_vulnerabilities_by_severity",
            "legendFormat": "{{severity}}"
          }
        ]
      },
      {
        "title": "License Compliance",
        "type": "stat",
        "targets": [
          {
            "expr": "sbom_license_violations",
            "legendFormat": "Violations"
          }
        ]
      }
    ]
  }
}
```

<!-- chunk: 🔧 实施检查清单 -->## 🔧 实施检查清单

## SBOM生成配置
- [ ] 选择合适的SBOM生成工具(Syft、Tern等)
- [ ] 配置自动化SBOM生成流程
- [ ] 建立SBOM存储和版本管理机制
- [ ] 实施多格式SBOM输出支持
- [ ] 集成漏洞扫描和许可证检查
- [ ] 配置SBOM签名和验证机制

## 合规性管理
- [ ] 建立软件供应链安全策略
- [ ] 实施SLSA合规性验证
- [ ] 配置许可证合规检查
- [ ] 建立依赖关系风险评估
- [ ] 实施安全漏洞跟踪机制
- [ ] 建立第三方组件审批流程

## 监控与报告
- [ ] 部署SBOM质量监控系统
- [ ] 建立合规性报告机制
- [ ] 配置告警和通知系统
- [ ] 维护SBOM分析仪表板
- [ ] 定期审查和优化流程
- [ ] 建立持续改进机制

---

*本文档为企业级软件物料清单管理提供完整的技术实施方案和最佳实践指导*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生产运维 MOC
- [[13-生产运维/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[01-集群基础/02-设计原则/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 10-GitOps流水线实践

## See Also

- 07-zero-trust-security-architecture
- 08-cis-benchmark-compliance-audit
- 10-gitops-pipeline-practices
- 11-infrastructure-as-code

- [[08-安全/README.md|返回目录]]

<!-- risk-assessed -->
