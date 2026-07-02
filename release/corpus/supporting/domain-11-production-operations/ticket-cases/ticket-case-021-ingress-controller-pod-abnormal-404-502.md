---
title: 阿里云专有云 Ingress 控制器 Pod 异常导致业务访问 404/502
description: 电商大促期间 Nginx Ingress Controller Pod 漂移后业务入口出现 404/502，根因为控制器 Pod 配置与节点负载不均导致后端
  upstream 漂移，含诊断、修复与验证。
summary: 电商大促期间 Nginx Ingress Controller Pod 漂移后业务入口出现 404/502，根因为控制器 Pod 配置与节点负载不均导致后端
  upstream 漂移，含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- ingress
- nginx-ingress
- 502
- 404
- loadbalancer
- slb
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-021
priority: P0
severity: critical
affected_cluster: ack-prod-vpc02
affected_namespace: ingress-nginx
ticket_type: 入口网关故障
skill_ref: Ingress 网关诊断
fta_ref: 'FTA: Ingress 404/502'
last_updated: 2026-06-26
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 Ingress 控制器 Pod 异常导致业务访问 404/502 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- ingress
- nginx-ingress
prerequisites:
- kubectl-basics
- k8s-networking
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-046-ingress-controller-404-502.md]]'
  type: related_to
- target: '[[concepts/ingress.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-011-ingress-controller-pod-404-502.md]]'
  type: related_to
---



# 工单 021：Ingress 控制器 Pod 异常导致业务访问 404/502

## 1. 工单描述

**用户原始描述：**

> 我们是阿里云专有云 ACK 集群，入口用的是 Nginx Ingress Controller。今晚 20:15 开始电商大促第一波流量进来后，监控显示很多用户访问我们的商城首页和下单接口出现 502 Bad Gateway，还有少量 404 Not Found。刷新几次有时候又能出来。我们在 ingress-nginx namespace 里看到有 Pod 状态异常，有的 CrashLoopBackOff，有的 Running 但是 Restart 次数很多。SLB 后端应该是 ingress-nginx 的这几个 Pod。业务 namespace 叫 mall-prod。麻烦尽快看一下，现在大促还在进行，订单掉了非常多。

## 2. 分类与优先级判定

- **任务类型：** 入口网关故障 / Ingress 控制器异常 / 流量入口 502/404
- **优先级：** P0（生产环境 + 大促期间 + 入口流量异常 + 业务订单损失）
- **严重程度：** critical
- **响应时限：** 立即响应，5 分钟内给出缓解方案
- **安全级别：** 中风险（涉及入口网关重启与流量切换，需确认变更窗口）

## 3. 诊断步骤

### 3.1 确认 Ingress Controller Pod 状态

```bash
# 查看 ingress-nginx namespace 下所有 Pod
kubectl get pod -n ingress-nginx -o wide

# 重点查看控制器 Pod 的运行状态与重启次数
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o wide

# 查看控制器 Deployment/ReplicaSet 事件
kubectl describe deployment ingress-nginx-controller -n ingress-nginx
kubectl get events -n ingress-nginx --sort-by='.lastTimestamp' | tail -50
```

### 3.2 检查 SLB 与后端 Endpoint 状态

```bash
# 查看 Ingress 控制器 Service 的 EXTERNAL-IP 与后端
kubectl get svc -n ingress-nginx
kubectl describe svc ingress-nginx-controller -n ingress-nginx

# 查看 Endpoint 是否健康
kubectl get endpoints ingress-nginx-controller -n ingress-nginx -o yaml

# 通过阿里云 CLI 查看 SLB 后端服务器健康状态
aliyun slb DescribeLoadBalancerAttribute --LoadBalancerId lb-2zeXXXXXXXXXXXXXX
aliyun slb DescribeHealthStatus --LoadBalancerId lb-2zeXXXXXXXXXXXXXX --ListenerPort 443
```

### 3.3 查看控制器日志定位 502/404 来源

```bash
# 查看异常 Pod 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=500 --previous=false

# 若 Pod 处于 CrashLoopBackOff，查看上次退出日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --previous --tail=500

# 过滤 upstream 连接错误与 502 条目
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=1000 | grep -E "502|upstream|error|failed"
```

### 3.4 检查 Ingress 规则与 Backend Service

```bash
# 列出业务 namespace 的 Ingress
kubectl get ingress -n mall-prod
kubectl describe ingress mall-frontend -n mall-prod
kubectl describe ingress mall-order -n mall-prod

# 检查对应 Service 与 Endpoint
kubectl get svc -n mall-prod
kubectl get endpoints -n mall-prod

# 测试集群内通过 Service 访问后端
kubectl run curl-test --rm -it --restart=Never -n mall-prod --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/curlimages/curl:latest -- \
  curl -s -o /dev/null -w "%{http_code}" http://mall-frontend.mall-prod.svc.cluster.local:8080/health
```

### 3.5 检查节点资源与 Pod 调度

```bash
# 查看 ingress-nginx Pod 所在节点
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.nodeName}{"\n"}{end}'

# 查看节点 CPU/内存负载
kubectl top node $(kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[*].spec.nodeName}')

# 查看节点状态与污点
kubectl describe node $(kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].spec.nodeName}')
```

### 3.6 检查 Nginx Ingress 配置与 ConfigMap

```bash
# 查看 ingress-nginx 配置
kubectl get configmap -n ingress-nginx
kubectl get configmap ingress-nginx-controller -n ingress-prod -o yaml

# 查看是否启用强制 SSL 重定向、proxy-body-size 等参数
kubectl get configmap ingress-nginx-controller -n ingress-nginx -o yaml | grep -E "ssl-redirect|proxy-body-size|proxy-connect-timeout|worker-processes"
```

### 3.7 诊断过程补充说明

Ingress 控制器作为集群七层入口网关，其稳定性直接影响所有依赖它的业务。排查 404/502 时需要区分问题是来自 Ingress 控制器本身，还是来自后端业务服务。一个简单的方法是：如果所有 Ingress 域名都返回 502/404，则很可能是控制器或 SLB 后端异常；如果只有部分域名异常，则更可能是对应后端 Service 或 Ingress 规则的问题。

在阿里云 ACK 专有云环境中，Nginx Ingress Controller 通常通过 LoadBalancer 类型 Service 暴露，由阿里云 SLB 作为四层负载均衡。SLB 的健康检查会周期性地探测控制器 Pod 的健康端口，只有当 Pod 被标记为 Ready 且通过健康检查后，SLB 才会将流量转发到该 Pod。如果控制器 Pod 因节点资源压力被驱逐并重新调度，新 Pod 启动期间可能已经被 SLB 挂载，此时进入的流量会返回 502。

Nginx Ingress 的 `worker-processes` 参数控制 Nginx worker 进程数。设置为 `auto` 时，Nginx 会根据 CPU 核心数启动对应数量的 worker。在容器环境中，若 limits.cpu 较大，worker 数会很多，每个 worker 都会占用一定内存，高并发下容易出现 OOM。对于高流量入口网关，建议固定 worker 数量并结合 limits.cpu 调优。`proxy-connect-timeout`、`proxy-send-timeout`、`proxy-read-timeout` 分别控制与后端建立连接、发送请求、读取响应的超时时间，设置过短会导致后端短暂不可达时直接返回 502。

## 4. 根因分析

综合控制器 Pod 状态、SLB 后端健康检查、日志与节点资源，判定根因为 **"Nginx Ingress Controller Pod 因节点 CPU/内存压力被驱逐并漂移至高负载节点，新启动 Pod 未就绪即被 SLB 挂载，导致 upstream 不可用，表现为 502；部分 404 来自旧 Pod 缓存的无效 upstream"**，置信度 **高**。

1. **节点资源压力：** 大促流量导致部分工作节点 CPU 与内存使用率飙升，kubelet 触发 Eviction，将 Ingress Controller Pod 驱逐至其他节点。新节点本身已承载大量业务 Pod，控制器 Pod 启动缓慢且频繁 OOM。
2. **SLB 后端健康检查滞后：** SLB 健康检查间隔为 5 秒，异常 Pod 在未被标记为不健康前继续接收流量，返回 502。
3. **控制器配置不合理：** ConfigMap 中 `worker-processes` 设置为 `auto`，在高并发场景下单 Pod 占用 CPU 过高；`proxy-connect-timeout` 较短，后端短暂不可达即返回 502。
4. **404 来源：** 部分旧 Pod 本地 Nginx 缓存了过期 upstream，漂移后仍尝试连接已下线的业务 Pod，导致少量 404。

### 4.1 风险与影响评估

- **业务影响：** 大促期间入口流量异常直接影响订单转化与用户体验，可能造成 GMV 损失与品牌口碑下降。
- **扩散风险：** 若其他业务入口共用同一套 Ingress Controller，故障范围会进一步扩大，全站入口都可能受影响。
- **数据风险：** 502 不会导致数据丢失，但可能引起重复下单、支付状态不一致等幂等性问题，需要业务侧对账修复。
- **容量风险：** 当前控制器副本数与资源配置无法支撑大促峰值，若不扩容，后续流量高峰仍会触发同样故障。
- **运维风险：** 控制器与业务 Pod 混部在高负载节点，节点资源压力会同时影响入口网关与业务服务，风险叠加。

## 5. 修复命令

### 5.1 临时缓解：扩容 Ingress Controller 副本并添加反亲和性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 立即扩容控制器副本数
kubectl scale deployment ingress-nginx-controller --replicas=6 -n ingress-nginx

# 2. 添加 Pod 反亲和性，避免多个控制器落在同一节点
cat <<'EOF' | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  replicas: 6
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app.kubernetes.io/name
                      operator: In
                      values:
                        - ingress-nginx
                topologyKey: kubernetes.io/hostname
EOF
```

### 5.2 调整控制器资源配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 修改控制器 Deployment 的资源请求与限制
cat <<'EOF' | kubectl patch deployment ingress-nginx-controller -n ingress-nginx --patch-file=/dev/stdin
spec:
  template:
    spec:
      containers:
        - name: controller
          resources:
            requests:
              cpu: "2000m"
              memory: "4Gi"
            limits:
              cpu: "4000m"
              memory: "8Gi"
EOF
```

### 5.3 优化 Nginx Ingress ConfigMap

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  worker-processes: "4"
  worker-connections: "16384"
  proxy-connect-timeout: "30"
  proxy-send-timeout: "60"
  proxy-read-timeout: "60"
  proxy-body-size: "200m"
  use-proxy-protocol: "false"
  enable-access-log: "true"
EOF
```

### 5.4 重启控制器使配置生效

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
# 滚动重启
kubectl rollout restart deployment ingress-nginx-controller -n ingress-nginx

# 等待滚动完成
kubectl rollout status deployment ingress-nginx-controller -n ingress-nginx --timeout=300s
```

## 6. 验证命令

```bash
# 1. 确认所有控制器 Pod 运行正常且分布均匀
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o wide

# 2. 确认节点分布
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.nodeName}{"\n"}{end}' | sort -k2

# 3. 确认 SLB 后端全部健康
aliyun slb DescribeHealthStatus --LoadBalancerId lb-2zeXXXXXXXXXXXXXX --ListenerPort 443

# 4. 模拟外部访问测试
for i in $(seq 1 50); do
  curl -s -o /dev/null -w "%{http_code}\n" https://mall.example.com/health
done | sort | uniq -c

# 5. 检查控制器日志无 502/404
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=500 | grep -E "502|404" | tail -20 || echo "无异常状态码"

# 6. 检查业务后端 Service 可达
kubectl run curl-test --rm -it --restart=Never -n mall-prod --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/curlimages/curl:latest -- \
  curl -s -o /dev/null -w "%{http_code}" http://mall-order.mall-prod.svc.cluster.local:8080/health
```

## 7. 回复客户话术

> 您好，工单 TC-2026-021 已紧急处理完成。
>
> **现象确认：** 20:15 起大促流量高峰期间，ingress-nginx namespace 下 Nginx Ingress Controller Pod 被节点资源压力驱逐并漂移，新启动 Pod 未完全就绪即被 SLB 挂载，导致商城入口出现 502 Bad Gateway 与少量 404 Not Found。
>
> **根因：** 控制器 Pod 与工作负载混部在高负载节点上，节点 CPU/内存压力触发 Eviction；控制器副本数不足、资源配置偏低，且 `worker-processes` 与超时参数未针对大促流量调优，导致 upstream 不可用和缓存过期 upstream。
>
> **已执行修复：**
> 1. 将 Ingress Controller 副本数从 2 扩容至 6，提升入口网关冗余度；
> 2. 添加 Pod 反亲和性，避免多个控制器 Pod 落在同一节点；
> 3. 提升控制器 CPU/内存请求与限制（2C4G 请求，4C8G 限制）；
> 4. 优化 ConfigMap：固定 worker-processes=4、增大连接数、调整超时参数、放开 body-size；
> 5. 滚动重启控制器并确认所有 Pod 健康、SLB 后端全部正常。
>
> **当前状态：** 50 次外部探测全部返回 200，控制器日志无新增 502/404，SLB 后端健康检查全部通过。
>
> **后续建议：**
> - 大促前对入口网关进行独立节点池部署，避免与业务 Pod 混部；
> - 建立 Ingress Controller HPA，基于 CPU/连接数自动扩容；
> - 优化 SLB 健康检查参数，缩短异常后端摘除时间；
> - 在 GitOps 中固化 Ingress 控制器资源配置与反亲和性策略；
> - 建议大促后复盘并评估是否需要引入多可用区 SLB + 多副本 Ingress 的高可用架构。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（已闭环，但属于 P0 生产故障，需提交事后复盘报告）
- **是否需要变更审批：** 是（入口网关配置与资源变更已记录变更台账）
- **交接信息：**
  - 已将优化后的 Deployment 与 ConfigMap 提交至 GitOps 仓库；
  - 建议架构团队评估独立 Ingress 节点池与 HPA 方案；
  - 若 24 小时内大促流量再次触发入口异常，需启动 P0 升级并协调阿里云 TAM 介入；
  - 本案例已沉淀至入口网关故障知识库，供后续大促保障参考。

---

*更新时间：2026-06-26 | 责任域：domain-11-production-operations/ticket-cases*

## Related

- Ingress 控制器 Pod 异常导致业务访问 404/502
- Ingress
- Ingress 控制器 Pod 异常导致 404/502
- Ingress 控制器 Pod 异常导致业务访问 404/502
