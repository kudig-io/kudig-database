---
title: 工单数字人快速问答 - On-Call 速查
description: 'description: 用户来提问时，直接给出快速答案+排查命令+下一步指引。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- coredns
- docker
- opa
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 工单数字人快速问答 - On-Call 速查 是什么
- 如何 工单数字人快速问答 - On-Call 速查
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 工单数字人快速问答
- On-Call
- 速查
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
- redis-basics
- mysql-basics
- policy-basics
---

---
title: 工单数字人快速问答 - On-Call 速查
description: 用户来提问时，直接给出快速答案+排查命令+下一步指引。
category: learning
tags:
- tutorial
- k8s
- training
- lecturer
- etcd
- apiserver
- kubelet
- scheduler
- coredns
- docker
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE 工程师
- 运维工程师
- 值班工程师
estimated_read_time: 5min
intent_queries:
- 工单数字人快速问答 - On-Call 速查 是什么
- 如何 工单数字人快速问答 - On-Call 速查
trigger_keywords:
- 工单数字人快速问答
- On-Call
- 速查
- k8s
- learning
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---

# 工单数字人快速问答 - On-Call 速查

> **章节**: 工单场景 | **难度**: 入门/进阶 | **时长**: 速查

---

## 数字人定位

```
【使用场景】

本速查用于"工单数字人"（Ticket Digital Human）。
用户来提问时，直接给出快速答案+排查命令+下一步指引。

【回复风格】

1. 先确认问题 → "你说的 Pod 一直 Pending，是这个吗？"
2. 直接给命令 → "先执行 kubectl describe pod..."
3. 解释原因 → "这是因为调度器找不到合适的节点"
4. 问"还有其他问题吗？" → 结尾总是确认

【禁止】

✗ 不确定时不要瞎猜 → "这个我不太确定，建议查官方文档"
✗ 不给危险操作建议 → "生产环境执行前一定要备份！"
✗ 不替代人工判断 → "这个需要人工确认"
```

---

## 一、Pod 问题（最高频）

### Q1: Pod 一直 Pending 怎么办？

```
【回复】

"Pod 处于 Pending，说明调度器无法分配节点。排查步骤：

第一步：查看详情
kubectl describe pod <pod-name> -n <namespace>

重点看 Events 部分！

常见报错：
• 'Insufficient cpu' → CPU 资源不足
• 'Insufficient memory' → 内存不足
• 'node(s) had taint' → 节点有污点
• 'no nodes available' → 没有符合条件的节点
• 'pvc not found' → PVC 不存在

第二步：如果资源不足
kubectl describe nodes

查看节点资源使用情况，找出资源充足的节点。

第三步：如果是有污点
kubectl describe node <node-name> | grep Taints

解决方案：给 Pod 添加污点容忍（tolerations）。

【一句话总结】

"Pending 不慌张，describe 看 Events！"

还有问题吗？
"
```

### Q2: Pod 一直 CrashLoopBackOff 怎么办？

```
【回复】

"CrashLoopBackOff 意味着容器持续崩溃重启。排查步骤：

第一步：看日志（最重要！）
kubectl logs <pod-name> -n <namespace> --previous

这会显示上一个（崩溃的）容器的日志。

第二步：检查资源限制
kubectl describe pod <pod-name> | grep -A5 "Limits"

如果内存 limit 太小，容器会被 OOMKilled。

第三步：检查启动命令
kubectl describe pod <pod-name> | grep -A10 "Container Config"

确认 command 和 args 配置正确。
有时候镜像有自己的 Entrypoint，你写的 command 被覆盖了。

第四步：检查依赖服务
如果应用需要连数据库，确认数据库是否可用。

【常见原因汇总】

1. 应用启动命令错误 → command/args 配置问题
2. 依赖服务不可用 → 数据库/Redis/API 连不上
3. 配置文件错误 → ConfigMap/Secret 路径错误
4. 内存不足 OOM → 增加 memory limit
5. 端口被占用 → 检查 port 冲突

【一句话总结】

"CrashLoop 不慌张，logs 看端倪！"

还有问题吗？
"
```

### Q3: Pod ImagePullBackOff 怎么办？

```
【回复】

"ImagePullBackOff 说明镜像拉取失败。排查步骤：

第一步：确认镜像名称
kubectl describe pod <pod-name> | grep -A3 "Containers"

看镜像名称是否拼写错误。

第二步：如果镜像是私有仓库
检查是否有 imagePullSecrets：
kubectl describe pod <pod-name> | grep "ImagePull"

如果没有 imagePullSecrets，需要创建：
```bash
# ⚠️ 重要：请替换为真实凭证，不要使用占位符
kubectl create secret docker-registry my-secret \
  --docker-server=<your-registry> \
  --docker-username=<your-username> \
  --docker-password=<your-password> \
  --docker-email=<your-email>
```

然后在 Pod spec 里添加：
imagePullSecrets:
- name: my-secret

第三步：测试拉取
docker pull <image-name>

在能访问registry的机器上手动拉取测试。

第四步：检查网络
如果是非私有仓库，可能是网络问题。
kubectl exec -it <pod-name> -- ping <registry-host>

【常见原因汇总】

1. 镜像名称拼写错误
2. 镜像 tag 不存在（用了 latest 但没有 latest）
3. 私有仓库未授权（没有 imagePullSecrets）
4. 网络不通（无法访问 registry）
5. 证书问题（私有仓库 HTTPS 证书无效）

还有问题吗？
"
```

### Q4: Pod Evicted 怎么办？

```
【回复】

"Pod 被 Evicted，说明节点资源压力太大，调度器把 Pod 赶走了。

【常见原因】

1. 节点磁盘压力（DiskPressure）
2. 节点内存压力（MemoryPressure）
3. 节点 PID 压力（PIDPressure）
4. 节点网络不可用（NetworkUnavailable）

【排查步骤】

第一步：检查节点状态
kubectl get nodes

看节点是不是有以上压力。

第二步：检查驱逐历史
kubectl describe pod <pod-name> | grep -A10 "Events"

通常会显示 "Evicted due to ..."。

第三步：恢复被驱逐的 Pod
kubectl get pods -n <namespace> | grep Evicted
kubectl delete pods -n <namespace> --field-selector=status.phase=Evicted

然后让 Deployment 重新创建 Pod。

【预防措施】

1. 设置合理的 resources requests/limits
2. 配置 Pod 中断预算（PDB）
3. 避免节点资源过度使用
4. 及时扩容增加节点

还有问题吗？
"
```

---

## 二、网络问题（高频）

### Q5: Service 无法访问怎么办？

```
【回复】

"Service 访问不了，按以下步骤排查：

第一步：确认 Service 存在
kubectl get svc -n <namespace>

如果不存在，检查 Service 名称是否正确。

第二步：检查 Endpoints（关键！）
kubectl get endpoints <service-name> -n <namespace>

Endpoints 为空 = 没有 Pod 匹配 Service 的 selector！

第三步：检查 Selector 匹配
kubectl describe svc <service-name> | grep -A5 Selector

看 Service 的 selector 是什么：
  Selector: app=web

对比 Pod 的 labels：
kubectl get pods -n <namespace> --show-labels | grep app

Pod 必须有 app=web 这个 label，Endpoints 才会有值。

第四步：检查 Pod 是否 Running
kubectl get pods -n <namespace>

Pod 必须在 Running 状态才能成为 Endpoints。

第五步：如果以上都正常，检查网络策略
kubectl get networkpolicy -n <namespace>
kubectl describe networkpolicy <name> -n <namespace>

NetworkPolicy 可能阻止了流量。

【Service 访问不了排查三板斧】

1. Service 存在？
2. Endpoints 不为空？
3. Pod 在 Running 状态？

还有问题吗？
"
```

### Q6: DNS 解析失败怎么办？

```
【回复】

"DNS 解析失败是 K8s 里很常见的问题。排查步骤：

第一步：测试集群 DNS 是否可用
kubectl run -it --rm dnsutils --image=tutum/dnsutils -- nslookup kubernetes.default

如果这个失败，说明集群 DNS 有问题。

第二步：检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

CoreDNS Pod 应该在 Running 状态。
如果不在 Running，说明 CoreDNS 出了问题。

第三步：查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

看看有没有错误信息。

第四步：检查 Pod 的 DNS 配置
kubectl exec -it <pod-name> -- cat /etc/resolv.conf

nameserver 应该是集群 DNS 的 IP（通常是 10.96.0.10）。

【常见原因汇总】

1. CoreDNS 挂了 → 重启 CoreDNS（kubectl rollout restart deployment/coredns -n kube-system）
2. 网络插件问题 → 检查 CNI 配置
3. /etc/resolv.conf 配置错误 → 检查 kubelet 的 --cluster-dns 参数
4. 节点 DNS 配置错误 → 检查节点的 /etc/resolv.conf

【一句话总结】

"DNS 故障不要慌，CoreDNS 先检查，再看 /etc/resolv.conf！"

还有问题吗？
"
```

### Q7: Ingress 返回 404 怎么办？

```
【回复】

"Ingress 404 的排查步骤：

第一步：检查 Ingress 资源
kubectl get ingress -n <namespace>
kubectl describe ingress <name> -n <namespace>

看 Rules 部分，确认 host 和 path 配置正确。

第二步：检查 Ingress Class
kubectl get ingressclass

确认 Ingress 里有 ingressClassName: <class-name>

如果没有配置 IngressClass，集群可能没有默认的 Ingress Controller。

第三步：检查后端 Service
kubectl get svc -n <namespace>

确认 Ingress 里的 service.name 存在。

第四步：检查 Endpoints
kubectl get endpoints <service-name> -n <namespace>

Service 有 Endpoints 才能响应请求。
如果 Endpoints 为空，说明没有 Pod 匹配 Service。

第五步：检查域名解析
ping <ingress-host>

域名应该解析到 Ingress Controller 的 IP。
如果是本地测试，可以用 curl -H "Host: myapp.example.com" <Ingress-Controller-IP>

第六步：检查 Ingress Controller
kubectl get pods -n ingress-nginx

Ingress Controller 必须在 Running 状态。
如果不在 Running，重启 Ingress Controller。

【一句话总结】

"Ingress 404 先查五点：Ingress、Class、Service、Endpoints、DNS！"

还有问题吗？
"
```

### Q8: NetworkPolicy 导致无法访问怎么办？

```
【回复】

"如果确认网络策略是问题所在，排查步骤：

第一步：查看 Namespace 的网络策略
kubectl get networkpolicy -n <namespace>

第二步：查看策略详情
kubectl describe networkpolicy <name> -n <namespace>

看 ingress 和 egress 规则。

第三步：临时测试（删除策略前备份）
kubectl get networkpolicy <name> -n <namespace> -o yaml > backup.yaml

如果需要临时放行，可以删除策略：
kubectl delete networkpolicy <name> -n <namespace>

验证网络是否恢复，然后重新应用正确策略。

第四步：创建正确的网络策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-specific
  namespace: <namespace>
spec:
  podSelector:
    matchLabels:
      app: my-app
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080

【注意】

删除网络策略要谨慎，确保不是因为其他原因导致无法访问。
删除前先备份，验证后重新配置正确的策略。

还有问题吗？
"
```

---

## 三、资源问题（高频）

### Q9: 资源配额超限怎么办？

```
【回复】

"报错 'exceeded quota' 说明 Namespace 的资源配额用完了。排查步骤：

第一步：查看配额使用情况
kubectl describe resourcequota -n <namespace>

输出示例：
Resource     Used  Hard
--------     ---   ---
pods         50    50    ← 这里满了

第二步：找出可以清理的资源

查看非 Running 的 Pod：
kubectl get pods -n <namespace> | grep -v Running

清理已完成的 Job：
kubectl get jobs -n <namespace>
kubectl delete jobs --field-selector=status.successful=1 -n <namespace>

清理已完成的 CronJob 的 Pod：
kubectl get pods -n <namespace> | grep Completed

第三步：如果确实需要更多配额
kubectl edit resourcequota <name> -n <namespace>

或者联系集群管理员增加 Namespace 配额。

【配额计算说明】

ResourceQuota 限制的是 requests.sum 和 limits.sum。
如果你有 10 个 Pod，每个 limits.memory=1Gi，总共就是 10Gi。

第四步：设置默认资源限制（LimitRange）
kubectl describe limitrange -n <namespace>

如果没有 LimitRange，Pod 可能无限使用资源。
可以创建 LimitRange 限制默认资源使用。

还有问题吗？
"
```

### Q10: Pod OOMKilled 怎么办？

```
【回复】

"Pod 被 OOMKilled 说明内存超限了。排查步骤：

第一步：确认 OOM
kubectl describe pod <pod-name> | grep -A10 "Last State"

如果 Last State 显示 OOMKilled，就是内存问题。

第二步：检查内存限制
kubectl describe pod <pod-name> | grep -A5 "Limits"

memory: 这个值是不是太小了？

第三步：查看应用的实际内存使用
kubectl top pods -n <namespace>

看实际使用的内存和 limits 对比。

第四步：增加内存限制

方式一：kubectl edit
kubectl edit deployment <name> -n <namespace>
# 修改 resources.limits.memory

方式二：apply YAML
kubectl apply -f deployment.yaml

第五步：如果内存持续增长
可能是应用内存泄漏，需要检查应用代码。

【内存单位说明】

Ki = Kibibyte (1024)
Mi = Mebibyte (1024^2)
Gi = Gibibyte (1024^3)

1Gi = 1024Mi = 约 10.7 亿字节

【一句话总结】

"OOM 了别慌，增加 memory limits 先保障，再查泄漏。"

还有问题吗？
"
```

### Q11: HPA 不工作怎么办？

```
【回复】

"HPA 不触发扩容，按以下步骤排查：

第一步：检查 HPA 状态
kubectl get hpa -n <namespace>
kubectl describe hpa <name> -n <namespace>

看 Conditions 部分：
• AbleToScale = True → 可以伸缩
• ScalingActive = True → 伸缩功能正常
• ScalingRestricted = True → 有条件限制扩缩容

第二步：检查 Metrics Server（关键！）
kubectl get pods -n kube-system -l k8s-app=k8s-dashboard-metrics-server

Metrics Server 必须正常运行！
如果 Metrics Server 没运行，HPA 无法获取 CPU/内存数据。

如果 Metrics Server 有问题：
kubectl logs -n kube-system -l k8s-app=k8s-dashboard-metrics-server --tail=50

第三步：检查 Pod 资源请求
kubectl describe pod <pod-name> | grep -A5 "Requests"

HPA 需要 Pod 设置了 resources.requests 才能计算使用率！
如果 Pod 没有设置 requests，HPA 无法判断是否需要扩容。

第四步：检查当前负载
kubectl get hpa <name> -n <namespace>

看 Current 和 Target：
Current CPU: 50% | Target CPU: 80%
说明当前负载还没达到扩容阈值。

【HPA 扩容公式】

desiredReplicas = ceil(currentReplicas * currentMetricValue / targetMetricValue)

如果当前 CPU 80%，目标 80%，副本 2 → 需要的副本 = 2
如果当前 CPU 160%，目标 80%，副本 2 → 需要的副本 = 4

【一句话总结】

"HPA 不工作，先检查 Metrics Server，再看 Pod 的资源请求！"

还有问题吗？
"
```

### Q12: 调度失败怎么办？

```
【回复】

"Pod 调度失败（Pending 且无 Events），排查步骤：

第一步：检查调度器是否正常
kubectl get pods -n kube-system -l component=kube-scheduler

调度器应该在 Running 状态。

第二步：检查节点是否有问题
kubectl get nodes

如果节点都是 NotReady，调度器无法工作。

第三步：检查污点和容忍
kubectl describe node <node-name> | grep Taints

如果节点有 NoSchedule 污点，Pod 没有匹配容忍就无法调度。

第四步：检查资源
kubectl describe nodes

如果所有节点资源都耗尽了，Pod 只能 Pending。

第五步：检查亲和性规则
kubectl describe pod <pod-name> | grep -A10 "Affinity"

如果 Pod 有复杂的亲和性/反亲和性规则，可能找不到满足条件的节点。

【常见原因汇总】

1. 资源不足 → 增加节点或减少 Pod
2. 污点问题 → 添加对应的 tolerations
3. 亲和性问题 → 调整 affinity 规则
4. 调度器挂了 → 重启调度器
5. 节点全部 NotReady → 先恢复节点

还有问题吗？
"
```

---

## 四、节点问题（中频）

### Q13: 节点 NotReady 怎么办？

```
【回复】

"节点显示 NotReady，按以下步骤排查：

第一步：检查节点状态
kubectl get nodes

看看是 NotReady 还是 Unknown。
Unknown 通常是节点失联（网络问题或节点宕机）。

第二步：检查节点详情
kubectl describe node <node-name>

看 Conditions 部分：
• MemoryPressure = True → 内存不足
• DiskPressure = True → 磁盘不足
• PIDPressure = True → PID 不足（进程数太多）
• NetworkUnavailable = True → 网络配置问题
• KubeletNotReady → kubelet 无法和 API Server 通信

第三步：SSH 到节点上检查

检查 kubelet 状态：
systemctl status kubelet

查看 kubelet 日志：
journalctl -u kubelet --since "10 minutes ago" -n 50

检查证书：
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

【常见原因和解决方案】

1. kubelet 没运行 → systemctl restart kubelet
2. 证书过期 → 更新证书（kubeadm alpha certificates renew）
3. 磁盘不足 → docker system prune 清理磁盘
4. 内存不足 → 重启一些低优先级 Pod
5. 网络不通 → 检查网络配置和防火墙

【Node NotReady 快速恢复】

systemctl restart kubelet
systemctl status kubelet
kubectl get nodes

还有问题吗？
"
```

### Q14: 节点上有个 Pod 一直无法删除怎么办？

```
【回复】

"Pod 卡住无法删除，按以下步骤：

第一步：强制删除
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force

如果还是不行，暴力删除：
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force --dry-run=server

第二步：检查是否有 Finalizers
kubectl get pod <pod-name> -n <namespace> -o yaml | grep finalizers

如果有自定义 Finalizers，可能阻塞删除。需要移除 Finalizers：
kubectl patch pod <pod-name> -n <namespace> -p '{"metadata":{"finalizers":null}}'

第三步：检查 API Server 和 kubelet 通信
如果 kubelet 无法和 API Server 通信，删除请求可能无法执行。

第四步：检查 etcd
kubectl exec -it <pod-name> -n kube-system -- etcdctl endpoint health

如果 etcd 有问题，需要修复 etcd。

【⚠️ 高危命令警告】

"Pod 删除不了，grace-period=0 加 --force 是最后的手段！

⚠️ 风险提示：
• 可能导致数据丢失（如果 Pod 有持久化存储）
• 可能导致服务中断
• 应该先尝试正常删除（不加 --force）
• 只有在 Pod 卡住无法正常删除时才使用

使用前请确认：
1. 已备份重要数据
2. 目标集群是否为测试环境
3. 是否已尝试正常删除

强制删除命令：
```bash
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force
```

如果还不行再查 Finalizers："

还有问题吗？
"
```

---

## 五、存储问题（中频）

### Q15: PVC Pending 怎么办？

```
【回复】

"PVC 一直 Pending，按以下步骤排查：

第一步：看 PVC 详情
kubectl describe pvc <pvc-name> -n <namespace>

重点看 Events 部分！

常见报错：
• "no persistent volumes available" → 没有符合条件的 PV
• "StorageClass <name> not found" → StorageClass 不存在
• "waiting for first consumer to be bound" → 等待 PVC 动态制备
• "cannot find storage provisioner" → 存储制备器问题

第二步：检查 StorageClass
kubectl get storageclass

如果 StorageClass 不存在，需要创建：
kubectl get sc

第三步：如果是云存储（阿里云/AWS/腾讯云）
检查 CSI driver 是否正常运行：
kubectl get pods -n kube-system | grep csi

CSI driver 应该在 Running 状态。

第四步：检查云厂商控制台
• 阿里云：查看云盘是否配额不足
• AWS：查看 EBS 配额是否充足
• 腾讯云：查看 CBS 配额

【PVC Pending 常见原因汇总】

1. StorageClass 不存在 → 创建 StorageClass
2. 云盘配额不足 → 联系云厂商增加配额
3. CSI driver 问题 → 重启 CSI driver
4. 节点没有匹配的 PV → 检查 PV 绑定情况

【一句话总结】

"PVC Pending 不慌张，describe pvc 看 Events！"

还有问题吗？
"
```

### Q16: Pod 无法挂载 Volume 怎么办？

```
【回复】

"Volume 挂载失败，排查步骤：

第一步：确认 PVC 已绑定
kubectl get pvc -n <namespace>

PVC 应该是 Bound 状态，不是 Pending。

第二步：检查 Pod 的 volumes 配置
kubectl describe pod <pod-name> | grep -A15 "Volumes"

确认 volumes 和 volumeMounts 配置正确。

第三步：检查挂载路径
volumeMounts:
- name: data
  mountPath: /var/lib/mysql

确认 mountPath 正确。

第四步：进入容器测试
kubectl exec -it <pod-name> -n <namespace> -- /bin/bash

测试挂载：
df -h                    # 查看挂载情况
ls -la /var/lib/mysql    # 查看目录内容

第五步：如果挂载路径是空的
可能是 PVC 问题，检查：
kubectl describe pvc <pvc-name> | grep -A5 "Mounted By"

第六步：检查存储类型兼容性
不同存储类型（nfs、cephfs、clouddisk）需要不同的挂载选项。

还有问题吗？
"
```

---

## 六、安全问题（中频）

### Q17: RBAC Forbidden 怎么办？

```
【回复】

"Forbidden 错误是权限问题，排查步骤：

第一步：确认当前用户
kubectl auth whoami

看当前是谁在操作。

第二步：查看当前权限
kubectl auth can-i --list

列出当前用户的所有权限。

第三步：测试具体操作
kubectl auth can-i <verb> <resource> --namespace=<namespace>

例如：
kubectl auth can-i get pods --namespace=default
kubectl auth can-i create pods --namespace=default

第四步：创建必要权限
如果缺少某个权限，创建 Role 并绑定：

方式一：使用 kubectl create role
kubectl create role pod-reader \
  --verb=get,list \
  --resource=pods \
  -n <namespace>

方式二：使用 YAML
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: <namespace>
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list"]

第五步：绑定到用户/组/ServiceAccount
kubectl create rolebinding pod-reader-binding \
  --role=pod-reader \
  --user=<username> \
  -n <namespace>

【RBAC 三要素】

1. Subject（谁）→ User、Group、ServiceAccount
2. Verb（做什么）→ get、list、create、update、delete
3. Resource（作用于谁）→ pods、services、deployments

【集群级别权限】

如果是集群级别的权限（不是某个 namespace），需要用 ClusterRole 和 ClusterRoleBinding：

kubectl create clusterrole pod-reader-cluster \
  --verb=get,list \
  --resource=pods

kubectl create clusterrolebinding pod-reader-binding \
  --clusterrole=pod-reader-cluster \
  --user=<username>

还有问题吗？
"
```

### Q18: Secret 无法查看怎么办？

```
【回复】

"Secret 默认是 base64 编码的，不能直接看到明文。查看步骤：

第一步：base64 解码
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.<key>}' | base64 -d

例如：
kubectl get secret my-secret -n default -o jsonpath='{.data.password}' | base64 -d

第二步：查看所有 Secret
kubectl get secret -n <namespace>

第三步：查看 Secret 详情
kubectl describe secret <secret-name> -n <namespace>

这只会显示类型，不会显示内容。

【创建 base64 编码的 Secret】

```bash
# ⚠️ 重要：请替换为实际密码的 base64 编码
echo -n "your-actual-password" | base64
# 输出：<your-base64-output>
```

YAML 示例：
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: my-secret
type: Opaque
data:
  password: <your-base64-encoded-password>  # TODO: 替换为实际的 base64 编码
```

【注意】

Secret 只是 base64 编码，不是加密！
任何有权限的人都可以解码查看。
生产环境建议配合加密解决方案（如 Vault）。

还有问题吗？
"
```

---

## 七、应用问题（中频）

### Q19: Deployment 滚动更新卡住怎么办？

```
【回复】

"Deployment 滚动更新卡住，按以下步骤排查：

第一步：检查 Deployment 状态
kubectl get deployment <name> -n <namespace>
kubectl describe deployment <name> -n <namespace>

看 Conditions 部分有没有异常。

第二步：检查 ReplicaSet
kubectl get rs -n <namespace>
kubectl describe rs -n <namespace>

看新旧 ReplicaSet 的状态。

第三步：检查 Pod
kubectl get pods -n <namespace>
kubectl describe pod <pod-name> -n <namespace>

看新 Pod 的状态：
• Pending → 调度失败
• ImagePullBackOff → 镜像拉取失败
• CrashLoopBackOff → 应用启动失败
• Running 但 NotReady → ReadinessProbe 失败

第四步：常见原因和解决方案

原因一：镜像拉取失败
kubectl describe pod <new-pod> | grep -A5 "Events"
→ 解决方案：检查镜像名称、凭证

原因二：健康检查失败
kubectl describe pod <new-pod> | grep -A10 "Readiness"
→ 解决方案：增加 initialDelaySeconds 或修复 Probe

原因三：资源不足
kubectl describe nodes
→ 解决方案：减少 replicas 或增加资源

第五步：回滚
如果无法修复，回滚到上一个版本：
kubectl rollout undo deployment/<name> -n <namespace>

【滚动更新参数调整】

如果更新太慢，可以调整策略：
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0

maxSurge: 最多超出多少 Pod（可以设大加快更新）
maxUnavailable: 最多有多少不可用（设为 0 保持全量服务）

【一句话总结】

"滚动更新卡住了，describe pod 看 Events，或者直接 rollout undo！"

还有问题吗？
"
```

### Q20: Pod 无法启动但没有明显错误怎么办？

```
【回复】

"Pod 看起来正常但无法启动，按以下步骤排查：

第一步：检查所有条件
kubectl describe pod <pod-name> | grep -A20 "Conditions"

看 Init 和 Ready 状态。

第二步：检查 Init Containers
kubectl describe pod <pod-name> | grep -A30 "Init Containers"

Init Containers 失败会阻塞主容器启动。
kubectl logs <pod-name> -n <namespace> -c <init-container-name> --previous

第三步：检查资源配额
kubectl describe resourcequota -n <namespace>
kubectl describe limitrange -n <namespace>

可能是 LimitRange 限制了资源。

第四步：检查污点和容忍
kubectl describe pod <pod-name> | grep -A10 "Tolerations"

如果 Pod 没有匹配节点的污点容忍，调度会失败但不一定显示在 Events 里。

第五步：检查 PVC 是否挂载成功
kubectl describe pod <pod-name> | grep -A5 "Volumes"

如果是持久化 workload，PVC 挂载失败会导致启动阻塞。

第六步：查看所有 Events
kubectl get events -n <namespace> --sort-by=.lastTimestamp | tail -50

按照时间排序，看最近的 Events。

第七步：尝试手动调度
如果怀疑调度问题：
kubectl get nodes -o jsonpath='{.items[*].status.conditions[?(@.type=="Ready")].status}'

确认节点都是 Ready。

还有问题吗？
"
```

---

## 八、命令速查

### 一键诊断命令

```bash
# 集群整体状态
kubectl get nodes
kubectl get pods -A
kubectl get events -A --sort-by=.lastTimestamp | tail -20

# Pod 问题
kubectl describe pod <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --previous

# 网络问题
kubectl get endpoints <service-name> -n <namespace>
kubectl run -it --rm dnsutils --image=tutum/dnsutils -- nslookup <service>

# 资源使用
kubectl top nodes
kubectl top pods -n <namespace>

# 配额查看
kubectl describe resourcequota -n <namespace>
kubectl describe limitrange -n <namespace>
```

### 快速修复命令

```bash
# 重启 Deployment
kubectl rollout restart deployment <name> -n <namespace>

# 回滚 Deployment
kubectl rollout undo deployment <name> -n <namespace>

# 删除卡住的 Pod
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force

# 扩缩容
kubectl scale deployment <name> --replicas=3 -n <namespace>

# 强制删除 Evicted Pod
kubectl delete pods -n <namespace> --field-selector=status.phase=Evicted

# 重启 CoreDNS
kubectl rollout restart deployment/coredns -n kube-system
```

---

## 九、升级人工触发条件

```
【需要升级人工的情况】

1. 生产环境故障（P0/P1 级别）
2. 需要执行危险操作（删除资源、修改配置）
3. 涉及数据丢失风险
4. 问题超过 3 轮对话仍未解决
5. 用户明确要求人工处理

【升级话术】

"这个问题比较复杂，我先帮你记录工单，
人工专家会在 30 分钟内联系你。
紧急问题可以拨打：xxx-xxxx-xxxx"

【升级前记录的信息】

• 集群版本：kubectl version --server
• 资源类型：Pod/Service/Deployment 等
• 错误信息：kubectl describe 的 Events
• 复现步骤：什么时候开始出问题
• 已尝试的解决方案：什么命令/方法
```

---

## 十、场景化对眸

| 场景 | 用户问法 | 数字人回复要点 |
|------|---------|---------------|
| Pod 卡住 | "Pod 一直 Pending" | describe 看 Events |
| 应用崩了 | "容器一直重启" | logs 看日志 |
| 服务不通 | "访问不了我的服务" | 检查 Endpoints |
| 网络慢 | "DNS 解析失败" | 检查 CoreDNS 状态 |
| 资源不足 | "配额超限了" | describe quota 看使用量 |
| 版本回滚 | "想回滚到上一个版本" | kubectl rollout undo |
| 扩缩容 | "想增加 Pod 数量" | kubectl scale |
| On-Call 值班 | "有个告警过来了" | 快速诊断→修复→验证 |

---

**关联文档**:
- [../README.md](../README.md) — 讲师完整台词设计
- [../troubleshooting/decision-tree-mermaid.md](../troubleshooting/decision-tree-mermaid.md) — Mermaid 决策树
- [../../domain-10-troubleshooting-diagnostics/topic-skills/](../../domain-10-troubleshooting-diagnostics/topic-skills/) — 18 个 GA Skill
- [../../domain-10-troubleshooting-diagnostics/](../../domain-10-troubleshooting-diagnostics/) — 故障排查文档

## See Also

- [[domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-27-storage-mount.md|day-27-storage-mount]]
- [[domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-28-comprehensive-review.md|day-28-comprehensive-review]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup.md|p1-k8s-cluster-setup]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/projects/p2-production-app-orchestration.md|p2-production-app-orchestration]]
