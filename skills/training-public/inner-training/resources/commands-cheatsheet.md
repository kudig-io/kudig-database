---
title: ACK/ACR/K8S 命令速查表
description: '**适用场景**: 日常运维快速参考 | **更新日期**: 2024'
category: learning
tags:
- k8s
- training
- hands-on
- statefulset
- daemonset
- ingress
- rbac
- rag
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- All kudig-database users
- ACK operators
- DevOps engineers
estimated_read_time: 5min
intent_queries:
- kubectl commands reference cheat sheet
- aliyun cs CLI commands quick reference
- Kubernetes日常运维命令
- ACK API commands cheat sheet
- kubectl node pod service management
trigger_keywords:
- kubectl
- aliyun
- commands
- cheat sheet
- quick reference
- CLI
- API
- node
- pod
- service
prerequisites:
- kubectl-basics
- gpu-ml-basics
related_domains:
- domain-12-cloud-providers
- domain-01-cluster-fundamentals
related_topics:
- knowledge-map
- reading-sequence
created: "2026-05-23"
---

# ACK/ACR/K8S 命令速查表

> **适用场景**: 日常运维快速参考 | **更新日期**: 2024

---

## 一、aliyun CLI — ACK 集群管理

### 集群操作

```bash
# 查看集群列表
aliyun cs GET /clusters

# 查看集群详情
aliyun cs GET /clusters/<cluster_id>

# 创建集群
aliyun cs POST /clusters --body '{ ... }'

# 删除集群
aliyun cs DELETE /clusters/<cluster_id>

# 获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config

# 查看集群升级状态
aliyun cs GET /clusters/<cluster_id>/upgradestatus

# 升级集群
aliyun cs POST /clusters/<cluster_id>/upgrade --body '{"version": "<ver>"}'
```

### 节点池操作

```bash
# 查看节点池列表
aliyun cs GET /clusters/<cluster_id>/nodepools

# 查看节点池详情
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>

# 创建节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{ ... }'

# 更新节点池
aliyun cs PUT /clusters/<cluster_id>/nodepools/<nodepool_id> --body '{ ... }'

# 删除节点池
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<nodepool_id>

# 扩容节点池
aliyun cs POST /clusters/<cluster_id>/nodepools/<nodepool_id> --body '{"count": N}'

# 移除节点
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<nodepool_id>/nodes \
  --body '{"nodes":["<node-id>"],"release_node":true,"drain_node":true}'
```

### 权限管理

```bash
# 授权 RAM 用户
aliyun cs POST /clusters/<cluster_id>/grant_permissions --body '{ ... }'

# 查看用户权限
aliyun cs GET /clusters/<cluster_id>/grant_permissions
```

### 组件管理

```bash
# 查看集群组件
aliyun cs GET /clusters/<cluster_id>/components

# 升级组件
aliyun cs POST /clusters/<cluster_id>/components/<name>/upgrade
```

---

## 二、aliyun CLI — ACR 镜像管理

```bash
# 查看实例列表
aliyun cr GetInstanceList

# 查看命名空间
aliyun cr GetNamespaceList --InstanceId <instance_id>

# 查看镜像仓库列表
aliyun cr GetRepoList --InstanceId <instance_id>

# 查看镜像 Tag 列表
aliyun cr GetRepoTagList --InstanceId <instance_id> --RepoId <repo_id>
```

---

## 三、kubectl — 集群信息

```bash
# 集群信息
kubectl cluster-info
kubectl version
kubectl get nodes -o wide

# API 健康检查
kubectl get --raw /healthz
kubectl get --raw /readyz

# 资源概览
kubectl api-resources          # 支持的资源类型
kubectl api-versions           # 支持的 API 版本
```

---

## 四、kubectl — 节点管理

```bash
# 查看节点
kubectl get nodes -o wide
kubectl describe node <node>
kubectl top node

# 标签管理
kubectl label nodes <node> key=value
kubectl label nodes <node> key-              # 删除标签
kubectl get nodes --show-labels
kubectl get nodes -l key=value

# 污点管理
kubectl taint nodes <node> key=value:NoSchedule
kubectl taint nodes <node> key=value:NoSchedule-   # 删除

# 维护操作
kubectl cordon <node>                        # 标记不可调度
kubectl uncordon <node>                      # 恢复调度
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
```

---

## 五、kubectl — Pod 操作

```bash
# 查看 Pod
kubectl get pods -o wide
kubectl get pods -A                          # 所有 Namespace
kubectl get pods -l app=<name>               # 按标签
kubectl get pods --sort-by='.status.phase'
kubectl describe pod <pod>
kubectl get pod <pod> -o yaml

# 日志
kubectl logs <pod>
kubectl logs <pod> -c <container>            # 多容器
kubectl logs <pod> --previous                # 上次退出
kubectl logs <pod> -f                        # 实时
kubectl logs -l app=<name> --tail=20

# 调试
kubectl exec -it <pod> -- /bin/sh
kubectl exec -it <pod> -c <container> -- /bin/sh
kubectl port-forward pod/<pod> 8080:80
kubectl cp <pod>:/path/file ./local-file

# 生命周期
kubectl run <name> --image=<image>
kubectl delete pod <pod>
kubectl delete pod <pod> --force --grace-period=0
```

---

## 六、kubectl — Deployment / [[StatefulSet|StatefulSet]]

```bash
# Deployment
kubectl get deploy
kubectl describe deploy <name>
kubectl scale deploy <name> --replicas=5
kubectl rollout status deploy <name>
kubectl rollout history deploy <name>
kubectl rollout undo deploy <name>
kubectl rollout restart deploy <name>

# StatefulSet
kubectl get sts
kubectl scale sts <name> --replicas=3
```

---

## 七、kubectl — [[Service|Service]] / [[Ingress|Ingress]]

```bash
# Service
kubectl get svc
kubectl describe svc <name>
kubectl get endpoints <name>

# Ingress
kubectl get ingress
kubectl describe ingress <name>
```

---

## 八、kubectl — 存储

```bash
# StorageClass
kubectl get sc
kubectl describe sc <name>

# PV / PVC
kubectl get pv
kubectl get pvc
kubectl describe pvc <name>

# 扩容 PVC
kubectl patch pvc <name> -p '{"spec":{"resources":{"requests":{"storage":"40Gi"}}}}'
```

---

## 九、kubectl — RBAC

```bash
# 查看权限
kubectl auth can-i <verb> <resource>
kubectl auth can-i create pods --as=<user>
kubectl auth can-i --list

# 角色
kubectl get role,rolebinding -n <ns>
kubectl get clusterrole,clusterrolebinding
kubectl describe role <name> -n <ns>
```

---

## 十、kubectl — 配额与限制

```bash
# ResourceQuota
kubectl get quota -n <ns>
kubectl describe quota <name> -n <ns>

# LimitRange
kubectl get limitrange -n <ns>
kubectl describe limitrange <name> -n <ns>
```

---

## 十一、kubectl — 故障排查

```bash
# 事件
kubectl get events --sort-by='.lastTimestamp'
kubectl get events -n <ns> --field-selector reason=Failed

# DNS 测试
kubectl run dns-test --rm -it --restart=Never \
  --image=busybox:1.36 -- nslookup kubernetes.default

# 网络测试
kubectl run net-test --rm -it --restart=Never \
  --image=busybox:1.36 -- wget -qO- http://<svc-name>

# 组件检查
kubectl get pods -n kube-system
kubectl logs -n kube-system <component-pod> --tail=30
```

---

## 十二、常用 JSON Path

```bash
# 获取所有 Pod IP
kubectl get pods -o jsonpath='{.items[*].status.podIP}'

# 获取节点 Pod CIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# 获取 Service External IP
kubectl get svc <name> -o jsonpath='{.status.loadBalancer.ingress[0].ip}'

# 获取 PVC 绑定的 PV
kubectl get pvc <name> -o jsonpath='{.spec.volumeName}'
```

## Related

- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
