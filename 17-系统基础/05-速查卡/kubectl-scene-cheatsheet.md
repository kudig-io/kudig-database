---
title: kubectl 场景速查卡
description: 'ssh <node-ip> "sudo journalctl -u kubelet --since 30m | tail -50"  #
  Step 3: 查看 kubelet 日志'
summary: 'ssh <node-ip> "sudo journalctl -u kubelet --since 30m | tail -50"  # Step
  3: 查看 kubelet 日志'
category: cheatsheet
tags:
- cheatsheet
- quick-reference
- etcd
- apiserver
- kubelet
- scheduler
- cilium
- flannel
- coredns
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubectl 场景速查卡 是什么
- 如何 kubectl 场景速查卡
trigger_keywords:
- kubectl
- 场景速查卡
- cheat
- sheet
prerequisites:
- kubectl-basics
- cloud-provider-basics
- cilium-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubectl 场景速查卡

> **文档类型**: 速查卡 | **版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: on-call 工程师按问题场景快速查找 kubectl 命令（非按资源类型）

---

## 1. 节点问题场景

### 节点 NotReady / Unknown

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 诊断命令（3 步）
kubectl get nodes -o wide                           # Step 1: 查看节点状态
kubectl describe node <node-name>                   # Step 2: 查看节点详情和 Conditions
ssh <node-ip> "sudo journalctl -u kubelet --since 30m | tail -50"  # Step 3: 查看 kubelet 日志

# 快速修复（低风险）
kubectl uncordon <node-name>                        # 解封节点（故障恢复后）

# 修复（中风险）
ssh <node-ip> "sudo systemctl restart kubelet"     # 重启 kubelet（需审批）
```
### 节点磁盘/内存压力

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 诊断命令
ssh <node-ip> "df -h / /var/lib/kubelet"           # 检查磁盘
ssh <node-ip> "free -h"                             # 检查内存
kubectl top nodes                                   # 查看节点资源使用

# 快速修复
kubectl cordon <node-name> && kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data  # 驱逐 Pod
```
### 批量节点维护

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 维护前
kubectl cordon <node-name>                         # 封禁节点
kubectl drain <node-name> --ignore-daemonsets --grace-period=60 --timeout=300s  # 驱逐 Pod

# 维护后
ssh <node-ip> "sudo reboot"                         # 执行维护
sleep 30 && kubectl get nodes <node-name>           # 等待恢复
kubectl uncordon <node-name>                        # 解封节点
```
---

## 2. Pod 问题场景

### Pod Pending（调度失败）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get pods -o wide                            # 查看 Pod 状态
kubectl describe pod <pod-name> | grep -A20 "Events:"  # 查看调度失败原因

# 常见原因和修复
# 原因 1: 资源不足
kubectl describe nodes | grep -A5 "Allocated resources"  # 确认资源状态
kubectl scale deployment <name> --replicas=0       # 临时减少副本（低风险）

# 原因 2: 污点不容忍
kubectl get nodes -o jsonpath='{.items[*].spec.taints}'  # 查看节点污点
kubectl patch pod <pod-name> -p '{"spec":{"tolerations":[{"key":"node.[[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/not-ready","operator":"Exists","effect":"NoExecute","tolerationSeconds":300}]}}'  # 临时添加容忍

# 原因 3: nodeSelector 不匹配
kubectl label node <node-name> <label-key>=<value>  # 添加标签匹配
```
### Pod CrashLoopBackOff / Error

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get pods -o wide                            # 查看重启次数
kubectl describe pod <pod-name> | grep -A15 "Events:"  # 查看崩溃原因
kubectl logs <pod-name> --previous                  # 查看上一个容器日志
kubectl logs <pod-name> -c <container-name>          # 查看特定容器日志

# 快速修复（低风险）
kubectl rollout restart deployment <deploy-name> -n <namespace>  # 重启 Deployment

# 修复（中风险）
kubectl delete pod <pod-name>                      # 删除 Pod（Deployment 会重建）
```
### Pod OOMKilled（退出码 137）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl describe pod <pod-name> | grep -A10 "Last State"  # 查看上次终止原因
kubectl top pods                                     # 查看内存使用

# 修复
kubectl patch deployment <deploy-name> -n <namespace> --patch '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"limits":{"memory":"2Gi"},"requests":{"memory":"1Gi"}}}]}}}}'  # 增加内存限制
```
### Pod ImagePullBackOff

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl describe pod <pod-name> | grep -A10 "ImagePull"  # 查看拉取错误

# 常见原因和修复
# 原因 1: 镜像不存在
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'  # 确认镜像名
docker pull <image-name>                                         # 在本地验证

# 原因 2: 私有仓库无凭据
kubectl create secret docker-registry <secret-name> --docker-server=<registry> --docker-username=<user> --docker-password=<pass>  # 创建 registry secret
kubectl patch serviceaccount <sa-name> -n <namespace> -p '{"imagePullSecrets":[{"name":"<secret-name>"}]}'  # 关联到 SA

# 原因 3: 网络问题
kubectl run test --image=curlimages/curl --restart=Never -it -- sh  # 调试网络
```
---

## 3. 网络问题场景

### [[Service|Service]] 无 Endpoints / 503

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get svc <svc-name> -n <namespace>         # 查看 Service 配置
kubectl get endpoints <svc-name> -n <namespace>    # 查看 Endpoints（关键！）
kubectl describe svc <svc-name> -n <namespace>     # 查看详细事件

# 无 Endpoints 时排查
kubectl get pods -n <namespace> -l <selector>      # 确认 selector 能匹配到 Pod
kubectl get pods -n <namespace> | grep Running     # 确认 Pod 正在运行

# 快速修复
kubectl rollout restart deployment <deploy-name> -n <namespace>  # 重启 Deployment
```
### DNS 解析失败

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl run dnsutils --image=tutum/dnsutils --restart=Never -it -- nslookup kubernetes.default  # 测试集群 DNS
kubectl run dnsutils --image=tutum/dnsutils --restart=Never -it -- nslookup <svc-name>          # 测试 Service DNS
kubectl exec -it <pod-name> -- cat /etc/resolv.conf  # 查看 DNS 配置

# 常见原因和修复
# 原因 1: CoreDNS 不健康
kubectl get pods -n kube-system -l k8s-app=kube-dns  # 检查 CoreDNS 状态
kubectl delete pods -n kube-system -l k8s-app=kube-dns  # 重启 CoreDNS（低风险）

# 原因 2: kube-proxy 问题
kubectl get pods -n kube-system -l k8s-app=kube-proxy  # 检查 kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=20  # 查看日志

# 原因 3: NetworkPolicy 阻塞
kubectl get networkpolicy -n <namespace>           # 检查是否有拒绝策略
```
### [[Ingress|Ingress]] 404/502/503

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
kubectl get ingress -n <namespace>                # 查看 Ingress 状态
kubectl describe ingress <ingress-name> -n <namespace>  # 查看路由配置
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50  # 查看 Ingress Controller 日志

# 常见原因和修复
# 原因 1: Backend Service 不存在或无 Endpoints
kubectl get svc <backend-svc> -n <namespace>       # 确认 Service 存在
kubectl get endpoints <backend-svc> -n <namespace>  # 确认有 Endpoints

# 原因 2: 路径匹配错误
# 检查 Ingress 规则的 path 和 pathType

# 原因 3: TLS 证书问题
kubectl get secret <tls-secret> -n <namespace>     # 确认证书 secret 存在
```
### Pod 之间跨节点不通

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl exec -it <pod-a> -- ping -c 3 <pod-b-ip>  # 测试 Pod 间连通性
kubectl exec -it <pod-a> -- ip addr               # 查看 Pod 网络接口
kubectl exec -it <pod-a> -- route -n             # 查看路由表

# CNI 检查（Flannel）
kubectl get pods -n kube-flannel                   # 检查 Flannel 状态
ip addr | grep flannel                            # 检查 Flannel 接口

# CNI 检查（Cilium）
kubectl get pods -n kube-system -l k8s-app=cilium  # 检查 Cilium 状态
cilium status                                      # 查看 Cilium 连接状态
```
---

## 4. 控制平面问题场景

### API Server 无法访问

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
curl -sk https://localhost:6443/healthz            # 检查 API Server 健康（控制平面节点）
kubectl get nodes                                    # 测试 API Server 响应
journalctl -u kube-apiserver --since 30m | tail -50  # 查看 API Server 日志（控制平面节点）

# 常见原因和修复
# 原因 1: etcd 问题
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key endpoint health

# 原因 2: 证书过期
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates  # 检查证书过期
sudo kubeadm certs renew apiserver                 # 续期证书（需审批）

# 原因 3: 内存不足（kubeadm 日志中 OOM）
kubectl get pods -n kube-system                     # 检查 Pod 状态
```
### Scheduler 不工作（Pod 一直 Pending）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get events --sort-by='.lastTimestamp' | grep -i "scheduler"  # 查看调度器事件
kubectl logs -n kube-system kube-scheduler-xxx --tail=50               # 查看调度器日志

# 测试调度器
kubectl create -f test-pod.yaml --dry-run=client -o yaml | kubectl apply -f -

# 快速修复
kubectl delete pod <pod-name>                      # 让 Deployment 重新创建
kubectl rollout restart deployment <deploy-name>   # 重启 Deployment
```
### etcd Leader 频繁切换

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key endpoint status | grep -i "leader"

ping -c 20 <other-etcd-node-ip>                   # 检查网络延迟（目标 < 5ms）
iostat -x 1 10                                     # 检查磁盘 I/O（目标 < 10ms）

# 快速修复
# 如果网络问题：检查防火墙和节点间网络
# 如果磁盘问题：考虑将 etcd 数据目录迁移到 SSD
```
### etcd 空间不足（quota exceeded）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
du -sh /var/lib/etcd/                             # 检查 etcd 数据目录大小
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key check datascale --write-out=table

# 紧急修复
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 defrag  # 在线 defrag（不影响集群）
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 alarm disarm  # 解除告警
```
---

## 5. 存储问题场景

### PVC Pending（绑定失败）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
kubectl describe pvc <pvc-name>                    # 查看 PVC 事件
kubectl get storageclass                           # 确认 StorageClass 存在
kubectl get pv                                      # 查看 PV 状态

# 常见原因和修复
# 原因 1: StorageClass 不存在
kubectl get storageclass | grep <sc-name>          # 确认存在

# 原因 2: 云厂商存储配额用尽
# 在云控制台查看存储卷配额

# 原因 3: volumeBindingMode=WaitForFirstConsumer 导致延迟绑定
# 等待 Pod 调度后再绑定
```
### PVC 挂载失败（MountVolume failed）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
kubectl describe pod <pod-name> | grep -A15 "Events:"  # 查看挂载错误
kubectl describe pvc <pvc-name>                    # 查看 PVC 事件

# CSI 驱动问题
kubectl get pods -n kube-system | grep csi        # 检查 CSI 驱动状态
kubectl logs -n kube-system csi-driver-xxx --tail=50  # 查看 CSI 日志

# 快速修复
kubectl delete pod <pod-name>                      # 删除 Pod 让 Deployment 重建
```
### 数据只读（ReadOnly）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl exec -it <pod-name> -- df -h              # 确认文件系统状态
kubectl exec -it <pod-name> -- touch /data/test.txt  # 测试写入

# 云盘问题
# AWS: 检查 EBS volume 状态
aws ec2 describe-volumes --volume-ids <vol-id> | grep State

# 修复（高风险）
# 可能需要 remount 或重启节点
```
---

## 6. 证书与认证场景

### kubelet 证书过期

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 诊断命令
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates  # 检查过期时间
journalctl -u kubelet | grep -i "certificate"      # 查看错误日志

# 修复
sudo systemctl restart kubelet                    # 重启 kubelet 触发自动轮换（需审批）
```
### kubeconfig 过期

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
kubectl config view                                 # 查看上下文
kubectl get pods                                    # 测试是否报认证错误

# 修复
kubeadm kubeconfig user --org team --cluster <cluster-name> > kube.conf  # 重新生成（需审批）
```
---

## 7. 安全与权限场景

### RBAC Forbidden / 权限不足

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl auth can-i <verb> <resource>              # 测试当前用户权限
kubectl auth can-i <verb> <resource> --as=<user>    # 模拟他人权限

# 快速修复（低风险）
kubectl create role <role-name> --verb=get,list --resource=pods  # 创建只读 Role
kubectl create rolebinding <name> --role=<role-name> --user=<user>  # 绑定到用户
```
### ServiceAccount 无法访问 API

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get sa <sa-name> -n <namespace> -o yaml    # 查看 SA 和关联的 secret
kubectl exec -it <pod-name> -- cat /var/run/secrets/kubernetes.io/serviceaccount/token  # 查看 token
kubectl exec -it <pod-name> -- cat /var/run/secrets/kubernetes.io/serviceaccount/namespace  # 查看 namespace

# 测试 token
kubectl exec -it <pod-name> -- curl -sk https://kubernetes.default.svc.cluster.local/api --header "Authorization: Bearer $(cat /var/run/secrets/kubernetes.io/serviceaccount/token)"

# 修复
kubectl create rolebinding <name> --clusterrole=system:node --serviceaccount=<namespace>:<sa-name>  # 授予权限
```
---

## 8. 升级与维护场景

### 集群升级失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令
kubectl get nodes -o wide                          # 查看节点版本
kubectl logs -n kube-system kube-apiserver-xxx --tail=50  # 查看 API Server 日志

# 回滚（高风险）
sudo kubeadm upgrade apply --allow-release-migration=false --force  # 强制回滚（需审批）

# 查看 etcd 兼容性
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 version
```
### 节点升级失败（kubelet 不升级）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 诊断命令
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}'

# SSH 到节点执行
ssh <node-ip> "apt-get update && apt-get install -y kubelet=1.30.0-*"  # 指定版本升级
ssh <node-ip> "sudo systemctl restart kubelet"                         # 重启 kubelet

# 验证
kubectl get nodes | grep <node-name>
```
---

## 9. HPA / 扩缩容场景

### HPA 不触发

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get hpa -n <namespace>                     # 查看 HPA 状态
kubectl describe hpa <hpa-name> -n <namespace>     # 查看 HPA 详情和事件
kubectl top pods -n <namespace>                     # 查看 CPU 使用

# 常见原因和修复
# 原因 1: 资源未设置 requests
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].resources.requests.cpu}'

# 修复：更新 Deployment 添加 resource requests
kubectl patch deployment <name> -n <namespace> --patch '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"requests":{"cpu":"100m"}}}]}}}}'

# 原因 2: metrics-server 问题
kubectl get pods -n kube-system | grep metrics-server
kubectl logs -n kube-system metrics-server-xxx --tail=20
```
### HPA 达到最大副本但 CPU 仍高

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断命令
kubectl get hpa -n <namespace>                    # 查看 HPA 状态
kubectl top pods -n <namespace>                    # 查看资源使用
kubectl describe hpa <hpa-name> -n <namespace>    # 查看扩展条件

# 修复
kubectl patch hpa <hpa-name> -n <namespace> --patch '{"spec":{"maxReplicas":20}}'  # 临时增加上限
```
---

## 10. 快速命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# === 节点状态 ===
kubectl get nodes -o wide | grep -v Ready         # 非 Ready 节点
kubectl top nodes                                  # 节点资源使用

# === Pod 状态 ===
kubectl get pods -A | grep -v Running | grep -v Completed  # 异常 Pod
kubectl get events -A --sort-by='.lastTimestamp' | tail -20 | grep -i "error|failed"  # 最近错误事件

# === 快速修复 ===
kubectl rollout restart deployment <name> -n <ns>  # 重启 Deployment（低风险）
kubectl delete pod <name> -n <ns>                  # 删除 Pod（Deployment 会重建）
kubectl uncordon <node>                            # 解封节点
kubectl logs -f <pod> -n <ns>                      # 实时日志

# === 快速验证 ===
kubectl auth whoami                                # 当前用户
kubectl get <resource> -o yaml | head -30         # 查看资源 YAML
kubectl explain <resource> | head -20             # 查看资源字段说明
```
---

```yaml
---
id: KUBECTL-SCENE-CHEATSHEET-001
domain: cheatsheet
type: quick-reference
tags: [kubectl, cheatsheet, scenario-based, quick-reference, oncall, hands-on, k8s-1.28-1.33]
intent_queries:
  - "Pod 问题怎么快速排查"
  - "Service 无 Endpoints 怎么处理"
  - "节点 NotReady 怎么修复"
  - "PVC 挂载失败怎么办"
  - "Ingress 502 怎么排查"
difficulty: intermediate
target_roles: [sre, ops-engineer, developer]
related:
  - 系统基础/topic-cheat-sheet/k8s.md
  - 故障诊断/00-troubleshooting-overview.md
  - P1-5-oncall-quick-reference-card.md
---
```
```

<!-- risk-assessed -->
