---
title: 第十课：常见问题排查 [09-troubleshooting]
description: '# 第十课：常见问题排查'
summary: 'kubectl describe pod <pod-name> | grep -A3 "Containers"'
category: k8s-lecturer
tags:
- k8s
- training
- lecturer
- coredns
- docker
- hpa
- job
- ingress
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第十课：常见问题排查 是什么
- 如何 第十课：常见问题排查
- 第十课：常见问题排查 故障排查
- 第十课：常见问题排查 排障步骤
trigger_keywords:
- 第十课：常见问题排查
- k8s
- lecturer
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第十课：常见问题排查

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 25 分钟

---

## 学习目标

1. 掌握常见 K8s 问题的排查思路
2. 学会使用诊断命令快速定位问题
3. 了解常见错误的解决方案
4. 建立故障排查的系统方法论

---

## 1. 问题排查方法论

### 1.1 排查三板斧

```
# 🟢 低风险：只读/信息收集，通常无副作用
【场景】

"遇到 K8s 问题不要慌，记住排查三板斧：

第一斧：看状态
kubectl get pods -n <namespace>

第二斧：看详情
kubectl describe pod <pod-name> -n <namespace>

第三斧：看日志
kubectl logs <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --previous

按照这个顺序，80% 的问题都能定位！"
```
### 1.2 问题分类速查

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【Pod 问题】

| 状态 | 原因 | 排查命令 |
|------|------|---------|
| Pending | 调度失败/资源不足 | kubectl describe pod |
| CrashLoopBackOff | 应用持续崩溃 | kubectl logs --previous |
| ImagePullBackOff | 镜像拉取失败 | kubectl describe pod |
| Error | 容器内部错误 | kubectl logs |
| Running 但不可用 | 健康检查失败 | kubectl describe pod |

【网络问题】

| 问题 | 排查命令 |
|------|---------|
| Service 无法访问 | kubectl get endpoints |
| DNS 解析失败 | kubectl run dnsutils --image=tutum/dnsutils -- nslookup |
| Ingress 404 | kubectl get ingress; kubectl describe ingress |

【资源问题】

| 问题 | 排查命令 |
|------|---------|
| 配额超限 | kubectl describe resourcequota |
| OOMKilled | kubectl describe pod |
| CPU Throttling | kubectl top pod |
```
---

## 2. Pod 状态问题

### 2.1 Pod 处于 Pending

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

"Pod 一直处于 Pending 状态，说明调度器无法把它调度到节点上。"

【常见原因】

1. 资源不足（CPU/内存不够）
2. 节点有污点（taints），Pod 没有对应的 toleration
3. 亲和性/反亲和性规则不满足
4. 存储卷无法挂载

【排查步骤】

第一步：看详情
kubectl describe pod <pod-name>

重点看 Events 部分！

常见报错：
• "Insufficient cpu" → CPU 资源不足
• "Insufficient memory" → 内存资源不足
• "node(s) had taint" → 节点有污点
• "no nodes available" → 没有匹配的节点

第二步：如果资源不足
kubectl describe nodes

查看各节点已分配的资源。

第三步：如果是污点问题
kubectl describe node <node-name> | grep Taints

解决方案：给 Pod 添加污点容忍。
```
### 2.2 Pod 处于 CrashLoopBackOff

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

"CrashLoopBackOff 意味着容器一直在崩溃、重启、崩溃..."

【常见原因】

1. 应用启动命令错误
2. 应用依赖的服务不可用
3. 配置文件错误
4. 内存不足被 OOMKilled
5. 端口被占用

【排查步骤】

第一步：看日志（最重要！）
kubectl logs <pod-name> -n <namespace> --previous

这会显示上一个（崩溃的）容器的日志。

第二步：检查资源限制
kubectl describe pod <pod-name> | grep -A5 "Limits"

如果内存 limit 太小，可能 OOM。

第三步：检查启动命令
确认 command 和 args 配置是否正确。
有时候镜像有自己的 Entrypoint，你写的 command 被覆盖了。

第四步：检查依赖
如果应用需要连数据库，确认数据库是否可用。
```
### 2.3 Pod 处于 ImagePullBackOff

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【问题】

"ImagePullBackOff 说明拉取镜像失败了。"

【常见原因】

1. 镜像名称拼写错误
2. 镜像不存在
3. 私有仓库未授权（没有 imagePullSecrets）
4. 网络问题（无法访问仓库）
5. 镜像 tag 错误（用了 latest 但没有 latest）

【排查步骤】

第一步：确认镜像名称
kubectl describe pod <pod-name> | grep -A3 "Containers"

第二步：如果是私有仓库
检查是否有 imagePullSecrets：

imagePullSecrets:
- name: my-registry-secret

创建私有仓库 secret：
kubectl create secret docker-registry my-registry-secret \
  --docker-server=registry.example.com \
  --docker-username=user \
  --docker-password=password
```
---

## 3. 网络问题

### 3.1 Service 无法访问

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

"Service 无法访问，但 Pod 是 Running 的。"

【排查四步曲】

第一步：确认 Service 存在
kubectl get svc -n <namespace>

第二步：确认 Endpoints 不为空
kubectl get endpoints <service-name> -n <namespace>

Endpoints 为空 = 没有 Pod 匹配 Service 的 selector！

第三步：检查 Selector 匹配
kubectl describe svc <service-name> | grep -A5 Selector

确认 Pod 的 labels 和 Service 的 selector 一致。

第四步：检查 Pod 是否 Running
kubectl get pods -n <namespace>

【常见错误】

1. Service selector 写错了
2. Pod 没有匹配的 labels
3. Pod 还没 Ready（ ReadinessProbe 失败）
```
### 3.2 DNS 解析失败

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【问题】

"应用无法解析 Service 名称，比如 ping my-service 失败。"

【排查步骤】

第一步：测试 DNS
kubectl run -it --rm dnsutils --image=tutum/dnsutils -- nslookup [[entities/kubernetes.md|kubernetes]].default

如果这个失败，说明集群 DNS 有问题。

第二步：检查 CoreDNS
kubectl get pods -n kube-system -l k8s-app=kube-dns

CoreDNS Pod 应该在 Running 状态。

第三步：查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

第四步：检查 Pod 的 DNS 配置
kubectl exec -it <pod-name> -- cat /etc/resolv.conf

确认 nameserver 指向集群 DNS。

【常见原因】

1. CoreDNS 挂了 → 重启 CoreDNS
2. 网络插件问题 → 检查 CNI 配置
3. /etc/resolv.conf 配置错误
```
### 3.3 Ingress 404 错误

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

"配置了 Ingress，但访问返回 404。"

【排查步骤】

第一步：检查 Ingress 资源
kubectl get ingress -n <namespace>
kubectl describe ingress <name> -n <namespace>

第二步：检查 Ingress Class
确认 ingressClassName 配置正确。

第三步：检查后端 Service
kubectl get svc -n <namespace>

确认 Ingress 引用的 Service 存在。

第四步：检查 Endpoints
kubectl get endpoints <service-name> -n <namespace>

Service 有 Endpoints 才能响应请求。

第五步：检查域名解析
ping myapp.example.com

确认域名解析到 Ingress Controller 的 IP。
```
---

## 4. 资源问题

### 4.1 配额超限

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【问题】

"报错 'exceeded quota' 或 'Cannot create resource'。"

【排查】

kubectl describe resourcequota -n <namespace>

查看 Used 和 Hard 的对比：
Resource     Used  Hard
--------     ---   ---
pods         50    50    ← 这里满了

【解决方案】

1. 删除不需要的 Pod
   kubectl delete pod <pod-name>

2. 清理已完成的 Job
   kubectl delete jobs --field-selector=status.successful=1 -n <namespace>

3. 申请增加配额
   kubectl edit resourcequota <name> -n <namespace>
```
### 4.2 OOMKilled

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

"Pod 被 OOMKilled，说明内存超限了。"

【排查】

kubectl describe pod <pod-name> | grep -A5 "Last State"

如果 Last State 是 OOMKilled，就能看到。

【解决方案】

1. 增加内存 limit
   resources:
     limits:
       memory: "1Gi"

2. 检查应用是否有内存泄漏
   查看应用的内存使用趋势
```
### 4.3 Pod 无法调度

```
# 🟢 低风险：只读/信息收集，通常无副作用
【问题】

"Pod 一直处于 Pending，无法调度到节点。"

【排查步骤】

kubectl describe pod <pod-name>

看 Events 部分：
• "Insufficient cpu" → CPU 不足
• "Insufficient memory" → 内存不足
• "node(s) had taints" → 污点问题
• "node(s) didn't match Pod's node affinity" → 亲和性问题

【解决方案】

1. 资源不足：
   - 减少 Pod 的 requests
   - 增加集群节点
   - 清理不需要的 Pod

2. 污点问题：
   kubectl describe node <node-name> | grep Taints
   添加对应的 toleration
```
---

## 5. 快速诊断命令汇总

### 5.1 一站式诊断

```
# 🟢 低风险：只读/信息收集，通常无副作用
【查看集群整体状态】

kubectl get nodes                              # 节点状态
kubectl get pods -A                            # 所有 Pod 状态
kubectl get events -A --sort-by=.lastTimestamp # 最近事件

【查看资源详情】

kubectl describe <resource> <name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --tail=100
kubectl top nodes                              # 资源使用
kubectl top pods -n <namespace>

【导出诊断信息】

# 导出 Pod 日志
kubectl logs <pod-name> -n <namespace> > pod.log

# 导出 Pod 详情
kubectl describe pod <pod-name> -n <namespace> > pod-desc.txt

# 导出 Events
kubectl get events -A -o yaml > events.yaml
```
### 5.2 网络诊断

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【测试连通性】

kubectl run -it --rm debug --image=busybox -- /bin/sh
# 在容器内
wget -q -O- http://<service-name>
nslookup <service-name>

【查看网络策略】

kubectl get networkpolicy -n <namespace>
kubectl describe networkpolicy <name> -n <namespace>
```
### 5.3 存储诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【查看 PV/PVC】

kubectl get pv,pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>

【进入容器检查存储】

kubectl exec -it <pod-name> -n <namespace> -- /bin/bash
df -h                  # 查看挂载
ls -la <mount-path>    # 查看文件
```
---

## 6. 总结

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查口诀】

Pod 不起来，三斧定生死：
describe 看详情，logs 看日志，events 看事件。

Service 不通，先看 Endpoints：
为空就是 selector 配错了，
不空就是网络或健康检查。

配额超限了，describe quota 看用量，
清理资源或申请加配额。

【常用命令速查】

kubectl get pods -n <namespace>
kubectl describe pod <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --previous
kubectl get events -n <namespace> --sort-by=.lastTimestamp
kubectl top nodes
kubectl top pods -n <namespace>

【下节课预告】

恭喜你完成了 K8s 入门系列课程！
你已经学会了：
• K8s 核心概念（Pod、Deployment、Service）
• 配置管理（ConfigMap、Secret）
• 资源隔离（Namespace、ResourceQuota）
• 存储和伸缩（PV/PVC、HPA）
• 常见问题排查

继续深入学习，推荐：
• Kubernetes 官方文档
• 故障排查文档（domain-12-troubleshooting）
• 18 个 GA Skill（topic-skills）

有问题随时来找我！"
```
---

**关联文档**:
- [../../故障诊断/](../../故障诊断/) — 故障排查文档
- [../../故障诊断/topic-skills/](../../故障诊断/技能体系/) — 18 个 GA Skill
- [../01-introduction/01-what-is-kubernetes.md](../01-introduction/01-what-is-kubernetes.md) — K8s 概念入门


<!-- risk-assessed -->
