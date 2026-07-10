---
title: 阿里云专有云 DNS 解析异常：CoreDNS 配置被 ConfigMap 误改 + VPC DNS 转发
description: 跨命名空间服务调用出现间歇性 503，根因是 CoreDNS Corefile 被误改 forward 到专有云中继 DNS，含诊断、修复、验证与话术。
summary: 跨命名空间服务调用出现间歇性 503，根因是 CoreDNS Corefile 被误改 forward 到专有云中继 DNS，含诊断、修复、验证与话术。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- coredns
- dns
- vpc-dns
- configmap
- service-discovery
- network
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-008
priority: P1
severity: high
affected_cluster: ack-prod-vpc01
affected_namespace: logistics-platform
ticket_type: 网络解析故障
skill_ref: K8s DNS 解析异常诊断
fta_ref: 'FTA: DNS 解析失败'
last_updated: 2026-06-26
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 DNS 解析异常：CoreDNS 配置被 ConfigMap 误改 + VPC DNS 转发 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- coredns
- dns
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
- target: '[[系统基础/知识字典/networking/dns-resolution.md]]'
  type: related_to
- target: '[[entities/coredns.md]]'
  type: related_to
- target: '[[系统基础/知识字典/networking/dns.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-013-configmap-secret-update-not-effective.md]]'
  type: related_to
- target: '[[系统基础/知识字典/configuration/configmap.md]]'
  type: related_to
- target: '[[concepts/service-networking.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 008：DNS 解析异常（CoreDNS 配置被 ConfigMap 误改 + VPC DNS 转发）

## 1. 工单描述

**用户原始描述：**

> 从今天下午 16:00 开始，logistics-platform 里的 route-service 调用 inventory-service 经常超时，报错 `java.net.UnknownHostException: inventory-service.logistics-platform.svc.cluster.local`。不是每次都失败，大概 30% 请求会中招。同一 Pod 里 `curl inventory-service` 有时候能通，有时候 503。我们下午 15:30 在 ACK 控制台改了 CoreDNS 的 ConfigMap，加了阿里云内网 DNS 地址，想优化一下外部域名解析。namespace 是 logistics-platform。帮忙看看是不是改错了。

## 2. 分类与优先级判定

- **任务类型：** 服务发现 / DNS 解析异常 / 网络间歇性故障
- **优先级：** P1（生产环境 + 服务间调用受影响 + 用户已自行变更 CoreDNS）
- **严重程度：** high
- **响应时限：** 15 分钟内给出修复方案
- **安全级别：** 中高风险（CoreDNS 为集群级组件，修改影响全局）

## 3. 诊断步骤

### 3.1 在业务 Pod 内复现 DNS 解析异常

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入异常 Pod
kubectl exec -it deploy/route-service -n logistics-platform -- /bin/sh

# 连续解析内部服务域名
for i in $(seq 1 20); do nslookup inventory-service.logistics-platform.svc.cluster.local; sleep 1; done
# 或使用 dig
dig @$(awk '/nameserver/{print $2}' /etc/resolv.conf) inventory-service.logistics-platform.svc.cluster.local
```
### 3.2 检查 CoreDNS Pod 与 ConfigMap

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod -n kube-system -l k8s-app=kube-dns
kubectl get cm coredns -n kube-system -o yaml
kubectl get cm coredns-custom -n kube-system -o yaml
```
### 3.3 检查 CoreDNS 日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=300
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=300 | grep -i "SERVFAIL|NXDOMAIN|timeout"
```
### 3.4 检查 VPC DNS 与节点 /etc/resolv.conf

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看节点默认 DNS 配置
kubectl run node-debug --rm -it --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/busybox:latest --restart=Never --overrides='{"spec":{"hostNetwork":true}}' -- /bin/sh
cat /etc/resolv.conf

# 检查阿里云专有云中继 DNS 可达性
nc -vz 100.100.2.136 53
nc -vz 100.100.2.138 53
```
### 3.5 检查 EndpointSlice 与 Service 状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get svc inventory-service -n logistics-platform -o yaml
kubectl get endpointslice -n logistics-platform -l kubernetes.io/service-name=inventory-service
kubectl get endpoints inventory-service -n logistics-platform
```
### 3.6 诊断过程补充说明

DNS 间歇性失败是最具迷惑性的网络故障之一。排障时建议同时关注 CoreDNS 缓存、UDP 53 端口负载均衡以及节点 `/etc/resolv.conf` 中的 `ndots` 与 `search` 配置。ACK 专有云中，如果节点 `/etc/resolv.conf` 本身指向了 VPC DNS，而 CoreDNS 又将根域 forward 到同一 VPC DNS，可能形成循环或重复转发，进一步放大延迟。

另外，Java 应用的 DNS 缓存默认有 30 秒 TTL，失败结果可能被 JVM 缓存，导致即使 CoreDNS 已恢复，业务仍持续报错。此时需要重启业务 Pod 才能清除 JVM 缓存。排障时可通过 `dig +short` 与 `nslookup` 交叉验证，区分是 DNS 服务器问题还是应用层缓存问题。

## 4. 根因分析

综合解析时好时坏的现象、ConfigMap 变更时间与 CoreDNS 日志，判定根因为 **"CoreDNS Corefile 中的 forward 插件被误配置为专有云中继 DNS，导致集群内部域名被错误转发到外部 DNS"**，置信度 **高**。

1. **配置错误：** 用户在 ACK 控制台修改 CoreDNS ConfigMap 时，将 `.`（根域）的 forward 目标改成了阿里云专有云中继 DNS（100.100.2.136/100.100.2.138）。这导致 `cluster.local`、`svc.cluster.local`、`in-addr.arpa` 等集群内部域名也被转发到外部 VPC DNS。
2. **VPC DNS 行为：** 外部 DNS 服务器不理解集群内部服务域名，返回 NXDOMAIN 或 SERVFAIL；但由于 CoreDNS 缓存机制，部分解析结果恰好命中缓存或 fallback，因此表现为间歇性失败。
3. **影响范围：** 全局所有依赖 DNS 服务发现的 Pod，尤其是跨命名空间调用场景。

### 4.1 风险与影响评估

- **业务影响：** logistics-platform 下微服务间调用成功率下降至约 70%，物流履约链路存在延迟与失败风险。
- **扩散风险：** CoreDNS 为集群级组件，配置错误会影响所有命名空间，若不及时修复可能引发更大范围服务抖动。
- **数据风险：** 不涉及数据丢失，但 DNS 失败结果可能被客户端缓存，修复后需重建 Pod 才能完全消除影响。

## 5. 修复命令

### 5.1 备份当前 CoreDNS ConfigMap

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get cm coredns -n kube-system -o yaml > /tmp/coredns-backup-$(date +%Y%m%d-%H%M%S).yaml
```
### 5.2 恢复标准 Corefile，仅对外部域名 forward 到 VPC DNS

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<'EOF'
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
           lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
           pods insecure
           fallthrough in-addr.arpa ip6.arpa
           ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
           max_concurrent 1000
        }
        cache 30
        loop
        reload
        loadbalance
    }
    # 专有云外部域名可单独配置 stub domain，避免污染根域
    aliyun.com:53 {
        errors
        cache 30
        forward . 100.100.2.136 100.100.2.138
    }
EOF
```
### 5.3 使用 coredns-custom ConfigMap 进行更安全的增量修改

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 推荐：不直接改 coredns ConfigMap，而是通过 coredns-custom 覆盖
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-custom
  namespace: kube-system
data:
  aliyun.server: |
    aliyun.com:53 {
        errors
        cache 30
        forward . 100.100.2.136 100.100.2.138
    }
  vpc.server: |
    alicloud.com:53 {
        errors
        cache 30
        forward . 100.100.2.136 100.100.2.138
    }
EOF
```
### 5.4 重新加载 CoreDNS

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deploy coredns -n kube-system
kubectl rollout status deploy coredns -n kube-system --timeout=120s
```
### 5.5 清空 Pod DNS 缓存

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除 route-service 与 inventory-service Pod，强制重新建立连接与 DNS 缓存
kubectl delete pod -n logistics-platform -l app=route-service
kubectl delete pod -n logistics-platform -l app=inventory-service
```
### 5.6 回滚方案（如修复失败）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若修改后问题加剧，立即从备份恢复 CoreDNS ConfigMap
kubectl apply -f /tmp/coredns-backup-<timestamp>.yaml
kubectl rollout restart deploy coredns -n kube-system

# 紧急情况下，可临时在业务 Pod 所在节点上修改 /etc/resolv.conf 指向备用 DNS（不推荐长期）
```
## 6. 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 CoreDNS Pod 全部 Running
kubectl get pod -n kube-system -l k8s-app=kube-dns

# 在业务 Pod 内连续解析内部服务域名，确认成功率 100%
kubectl exec -it deploy/route-service -n logistics-platform -- /bin/sh -c '
ok=0; fail=0
for i in $(seq 1 50); do
  if nslookup inventory-service.logistics-platform.svc.cluster.local >/dev/null 2>&1; then
    ok=$((ok+1))
  else
    fail=$((fail+1))
  fi
  sleep 0.5
done
echo "OK: $ok, FAIL: $fail"
'

# 验证外部域名仍可通过 VPC DNS 解析
kubectl exec -it deploy/route-service -n logistics-platform -- nslookup aliyun.com

# 确认 CoreDNS 日志无大量 NXDOMAIN/SERVFAIL
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100 | grep -c "NXDOMAIN|SERVFAIL" || true

# 业务接口调用验证
kubectl exec -it deploy/route-service -n logistics-platform -- \
  curl -s -o /dev/null -w "%{http_code}" http://inventory-service.logistics-platform.svc.cluster.local:8080/health
```
## 7. 回复客户话术

> 您好，工单 TC-2026-008 已处理完成。
>
> **现象确认：** logistics-platform 下 route-service 调用 inventory-service 出现间歇性 `UnknownHostException`，成功率约 70%。
>
> **根因：** 15:30 修改 CoreDNS ConfigMap 时，将 `.`（根域）forward 目标直接指向了阿里云专有云中继 DNS（100.100.2.136/100.100.2.138）。这导致 `cluster.local` 等集群内部域名被错误转发到外部 DNS，外部 DNS 无法解析内部服务名，从而出现间歇性解析失败。
>
> **已执行修复：**
> 1. 恢复 CoreDNS Corefile 标准配置，根域仍走节点 `/etc/resolv.conf`；
> 2. 将外部域名（如 aliyun.com、alicloud.com）单独配置为 stub domain，forward 到 VPC DNS；
> 3. 重启 CoreDNS 并重建业务 Pod，清除旧 DNS 缓存；
> 4. 使用 `coredns-custom` ConfigMap 管理外部域名转发，避免未来直接修改主配置。
>
> **当前状态：** 内部服务域名 50 次解析全部成功，外部域名解析正常，业务接口返回 200。
>
> **后续建议：**
> - 后续修改 CoreDNS 请走变更审批，建议先在测试集群验证；
> - 将 CoreDNS NXDOMAIN/SERVFAIL 增长率纳入告警；
> - 对关键服务调用增加客户端重试与熔断，降低 DNS 抖动影响；
- 建议将 CoreDNS ConfigMap 变更纳入 GitOps 管理，禁止直接通过控制台修改生产配置；
- 在预发环境配置与生产一致的 CoreDNS 规则，发布前进行全量域名解析回归测试。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（已闭环）
- **是否需要变更审批：** 是（CoreDNS 为集群级组件，已记录变更台账）
- **交接信息：**
  - 已通知网络团队确认 VPC DNS 在该时段无异常；
  - 若其他命名空间仍报 DNS 异常，需检查是否受同一 ConfigMap 影响；
  - 已将恢复后的 Corefile 与推荐 stub domain 配置沉淀至知识库；
- 建议对所有命名空间执行一次 DNS 解析抽样测试，确认无残余影响；
- 若再次出现 CoreDNS 配置被误改，将启动变更流程审计并升级至平台架构师；
  - 建议为 CoreDNS ConfigMap 启用 OPA/Gatekeeper 策略保护，禁止直接修改根域 forward 目标；
  - 本次恢复后的配置已作为 baseline 提交，后续任何 CoreDNS 变更需与此 baseline 进行 diff 评审。

---

*更新时间：2026-06-26 | 责任域：生产运维/ticket-cases*

## Related

- DNS 解析
- CoreDNS (entities)
- 域名服务
- [[生产运维/工单案例/ticket-case-013-configmap-secret-update-not-effective.md|ConfigMap/Secret 更新后应用未生效]]
- 配置映射
- [[concepts/service-networking.md|Service Networking]]


<!-- risk-assessed -->
