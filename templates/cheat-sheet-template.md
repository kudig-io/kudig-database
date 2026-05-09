# {{主题名称}} 速查卡

> **模板版本**: 2.0
> **最后更新**: 2026-05
> **适用场景**: {{场景描述}}
> **适用版本**: {{版本范围}}
> **最后更新**: {{日期}}

---

## YAML Front Matter

```yaml
---
title: "{{主题名称}}速查卡"
description: "{{一句话说明本文档用途}}"
category: cheatsheet
tags: [k8s, {{component}}, cheatsheet, quick-reference]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
last_updated: "{{YYYY-MM}}"
authors:
  - name: "{{姓名}}"
    role: "{{角色}}"
difficulty: "beginner"
related_docs:
  - path: "../domain-{{N}}-{{name}}/{{doc}}.md"
    desc: "深度文档"
  - path: "../topic-fta/list/{{component}}-fta.md"
    desc: "FTA 故障树"
---
```

---

## 快速参考索引

> 本速查卡包含的命令，按场景快速查找。

| 场景 | 命令类别 | 页内位置 |
|:---|:---|:---|
| {{场景A}} | {{类别}} | [§1](#section-1) |
| {{场景B}} | {{类别}} | [§2](#section-2) |
| {{场景C}} | {{类别}} | [§3](#section-3) |

---

## 1. {{分类1}}

> {{分类说明：一句话描述这类命令的用途}}

### 1.1 {{子分类A}}

| 命令/语法 | 说明 | 示例 |
|:---|:---|:---|
| `{{command1}}` | {{说明}} | `{{示例}}` |
| `{{command2}}` | {{说明}} | `{{示例}}` |
| `{{command3}}` | {{说明}} | `{{示例}}` |

### 1.2 {{子分类B}}

```bash
# {{场景说明}}
{{命令}}

# {{场景说明}}
{{命令}}

# {{场景说明}}
{{命令}}
```

---

## 2. {{分类2}}

> {{分类说明}}

### 2.1 {{子分类A}}

| 命令/语法 | 说明 | 示例 |
|:---|:---|:---|
| `{{command1}}` | {{说明}} | `{{示例}}` |
| `{{command2}}` | {{说明}} | `{{示例}}` |

### 2.2 {{子分类B}}

```bash
# {{场景说明}}
{{命令}}

# {{场景说明}}
{{命令}}
```

---

## 3. {{分类3}}

> {{分类说明}}

### 3.1 {{子分类A}}

| 命令/语法 | 说明 | 示例 |
|:---|:---|:---|
| `{{command1}}` | {{说明}} | `{{示例}}` |

### 3.2 {{子分类B}}

```bash
# {{场景说明}}
{{命令}}
```

---

## 4. 常见问题速查

> 快速解决常见问题的命令索引。

| 问题 | 快速解决 | 验证命令 |
|:---|:---|:---|
| {{问题1：比如 Pod 无法启动}} | `kubectl describe pod {{name}} -n {{ns}}` | `kubectl get pod {{name}} -n {{ns}}` |
| {{问题2：比如网络不通}} | `kubectl exec -it {{pod}} -n {{ns}} -- curl localhost:{{port}}` | `kubectl logs {{pod}} -n {{ns}} --tail=50` |
| {{问题3：比如证书过期}} | `kubectl get csr | grep -E 'Pending' | awk '{print $1}' | xargs kubectl certificate approve` | `openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates` |
| {{问题4：比如节点 NotReady}} | `kubectl describe node {{node}} | grep -E 'Conditions|Events'` | `kubectl get node {{node}} -o wide` |
| {{问题5：比如存储挂载失败}} | `kubectl describe pvc {{name}} -n {{ns}}` | `kubectl get pvc {{name}} -n {{ns}}` |

---

## 5. 一键脚本集合

> 常用复合操作封装为可执行脚本片段。

### 5.1 诊断脚本

```bash
#!/bin/bash
# {{脚本名称}} - {{用途简述}}
# 用法: ./{{script-name}}.sh [namespace] [resource-name]

NAMESPACE="${1:-default}"
RESOURCE="${2:-all}"

echo "=== 命名空间: $NAMESPACE ==="
echo "--- Pod 状态概览 ---"
kubectl get pods -n "$NAMESPACE" -o wide | head -20

echo "--- 最近 Events ---"
kubectl get events -n "$NAMESPACE" --sort-by='.lastTimestamp' | tail -10

echo "--- 资源使用 ---"
kubectl top pods -n "$NAMESPACE" 2>/dev/null || echo "metrics-server unavailable"

if [ "$RESOURCE" != "all" ]; then
  echo "--- 详情: $RESOURCE ---"
  kubectl describe "$RESOURCE" -n "$NAMESPACE"
fi
```

### 5.2 修复脚本

```bash
#!/bin/bash
# {{脚本名称}} - {{用途简述}}
# ⚠️ 执行前请确认影响范围

echo "即将执行: {{操作描述}}"
read -p "确认执行? (yes/no): " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
  echo "已取消"
  exit 0
fi

{{修复命令}}
```

---

## 6. 云厂商特有命令

> 不同云厂商的特殊诊断和配置命令。

| 云厂商 | 特殊命令 | 用途 |
|:---|:---|:---|
| AWS EKS | `aws eks describe-cluster --name {{cluster}}` | 查看集群配置 |
| AWS EKS | `aws eks update-kubeconfig --name {{cluster}}` | 更新 kubeconfig |
| GCP GKE | `gcloud container clusters describe {{cluster}}` | 查看集群详情 |
| GCP GKE | `gcloud container clusters get-credentials {{cluster}}` | 更新 kubeconfig |
| Azure AKS | `az aks show --name {{cluster}} --resource-group {{rg}}` | 查看集群详情 |
| Azure AKS | `az aks get-credentials --name {{cluster}} --resource-group {{rg}}` | 更新 kubeconfig |
| 阿里云 ACK | `aliyun cs DescribeClusterDetail --clusterId {{id}}` | 查看集群详情 |
| 腾讯云 TKE | `tke cluster describe --cluster-id {{id}}` | 查看集群详情 |

---

## 7. 版本差异速查

> 不同 K8s 版本间命令输出格式和可用参数的差异。

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|:---|:---|:---|:---|:---|:---|
| `kubectl api-resources` | ✅ | ✅ | ✅ | ✅ | ✅ |
| `kubectl get --show-io` | ❌ | ✅ | ✅ | ✅ | ✅ |
| `kubectl rollout status` | ✅ | ✅ | ✅ | ✅ | ✅ |
| `--dry-run=client` | ⚠️ | ✅ | ✅ | ✅ | ✅ |

---

## 8. 参考资料与延伸阅读

- [官方文档]({{URL}})
- [深度文档](../domain-{{N}}-{{name}}/{{doc}}.md)
- [FTA 故障树](../topic-fta/list/{{component}}-fta.md)
- [相关 Skill](../topic-skills/{{NN}}-{{scenario}}.md)

---

## 附录：命令速查表（可打印版）

```
╔══════════════════════════════════════════════════════════════════════╗
║                    {{主题名称}} 命令速查表                              ║
╠══════════════════════════════════════════════════════════════════════╣
║ 场景                          │ 命令                                  ║
╠══════════════════════════════════════════════════════════════════════╣
║ {{场景A}}                    │ {{命令}}                     ║
║ {{场景B}}                    │ {{命令}}                     ║
║ {{场景C}}                    │ {{命令}}                     ║
╠══════════════════════════════════════════════════════════════════════╣
║ 最后更新: {{YYYY-MM}} | 版本: v1.28-v1.32                              ║
╚══════════════════════════════════════════════════════════════════════╝
```