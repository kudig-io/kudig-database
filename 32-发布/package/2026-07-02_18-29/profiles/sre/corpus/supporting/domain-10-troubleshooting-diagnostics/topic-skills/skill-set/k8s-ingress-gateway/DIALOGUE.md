---
title: Ingress网关问题 — 远程顾问对话脚本
summary: Ingress网关问题的远程顾问对话脚本，覆盖Nginx、Traefik、证书配置排查。
category: troubleshooting
tags:
- networking
- remote-consultant
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
dialogue_id: DIALOGUE-K8S_INGRESS_GATEWAY
skill_id: k8s-ingress-gateway
version: 1.0.0
role: remote-consultant
language: zh
relationships:
- target: '[[domain-17-system-foundation/知识字典/networking/ingress.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]]/网关问题 — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

## 对话入口
### 入口 A
**工程师**：外部用户报告无法访问服务，返回502/503

**顾问回应**：先确认三个问题：
1. **影响范围**：哪些服务/应用受到影响？
2. **紧急程度**：是否影响生产流量？
3. **发生时间**：何时开始出现问题？

### 入口 B
**工程师**：HTTPS证书错误，浏览器显示不安全

**顾问回应**：先确认三个问题：
1. **影响范围**：哪些服务/应用受到影响？
2. **紧急程度**：是否影响生产流量？
3. **发生时间**：何时开始出现问题？

### 入口 C
**工程师**：Ingress规则未生效，流量路由错误

**顾问回应**：先确认三个问题：
1. **影响范围**：哪些服务/应用受到影响？
2. **紧急程度**：是否影响生产流量？
3. **发生时间**：何时开始出现问题？

## Round 1
### 分支 1：Ingress状态
- `kubectl get ingress -n <ns>`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl describe ingress <ing> -n <ns>`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl get ingress <ing> -n <ns> -o yaml`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 2：后端服务
- `kubectl get endpoints <svc> -n <ns>`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl get svc <svc> -n <ns>`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl get pods -n <ns> -l <selector>`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 3：证书检查
- `kubectl get secret <tls-secret> -n <ns> -o jsonpath={.data.\"tls.crt\"} | base64 -d | openssl x509 -noout -dates`
  > 💬 **顾问确认**：如输出与预期不符，请停止操作并立即反馈。
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl get ingress <ing> -n <ns> -o jsonpath={.spec.tls}`
  - 如无法执行：请提供当前可执行的环境信息
- `curl -v -k https://<host> 2>&1 | grep -i cert`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 4：阿里云ACK Ingress/SLB特有排查

**顾问**：如果集群运行在阿里云 ACK 上，Ingress 和外部访问涉及 **ACK Ingress Controller**、**SLB负载均衡**、**ALB应用型负载均衡** 等特有组件。请按以下步骤排查：

**步骤 1：ACK Ingress Controller状态检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看ACK Ingress Controller（nginx-ingress-controller或ALB Ingress Controller）
kubectl get pods -n kube-system | grep -E 'ingress|alb'
kubectl get deployment -n kube-system | grep -E 'ingress|alb'

# 查看Ingress Controller日志
kubectl logs -n kube-system -l app=ingress-nginx --tail=50
# 或ALB Ingress Controller
kubectl logs -n kube-system -l app=alb-ingress-controller --tail=50
```
> **如果无法执行 kubectl**：请登录 **ACK 控制台 > 集群 > 组件管理**，确认：
> 1. **Nginx Ingress Controller** 或 **ALB Ingress Controller** 组件是否正常运行？
> 2. 组件版本是否与 ACK 集群版本兼容？
> 3. 组件是否有异常事件或重启记录？

**步骤 2：阿里云SLB与Ingress关联检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看Ingress注解中的SLB配置
kubectl get ingress <ing> -n <ns> -o yaml | grep -E "slb-id|alibabacloud.com|ack.aliyun.com"

# 查看Ingress Controller的Service（类型为LoadBalancer）
kubectl get svc -n kube-system | grep ingress
kubectl describe svc <ingress-svc> -n kube-system | grep -A10 Events
```
> **如果无法执行**：请登录 **阿里云控制台 > 负载均衡 SLB**，告诉我：
> 1. 与 Ingress Controller 关联的 SLB 实例状态是否为 **运行中**？
> 2. SLB 监听配置是否与 Ingress 规则中的端口一致？
> 3. SLB 后端服务器组中的 ECS 是否全部健康？

**步骤 3：ALB Ingress特有排查（如使用ALB Ingress Controller）**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看ALB Ingress的AlbConfig
kubectl get albconfig -n <ns>
kubectl get albconfig <alb-name> -n <ns> -o yaml

# 查看ALB关联的监听和虚拟服务器组
kubectl get ingress <ing> -n <ns> -o yaml | grep -i "alb"
```
> **如果无法执行**：请登录 **阿里云控制台 > 应用型负载均衡 ALB**，确认：
> 1. ALB 实例状态是否为 **运行中**？
> 2. ALB 监听配置是否与 Ingress 规则匹配？
> 3. ALB 虚拟服务器组中的后端 Pod IP 是否正确？

**步骤 4：ACK专有网络与安全组检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查Ingress Controller Pod所在节点的安全组
kubectl get pod <ingress-pod> -n kube-system -o wide
# 然后到控制台检查该节点ECS的安全组规则
```
> **如果无法执行**：请登录 **阿里云控制台 > 安全组**，确认：
> 1. 节点安全组的 **入方向** 是否允许 SLB/ALB 的网段访问 Ingress Controller 端口（80/443）？
> 2. 节点安全组的 **入方向** 是否允许健康检查源 IP 访问？
> 3. 集群 **网络策略（NetworkPolicy）** 是否阻断了 Ingress Controller 到后端 Pod 的流量？

**阿里云ACK Ingress特有诊断矩阵**：

| ACK特有场景 | 诊断方法 | 修复方案 |
|:---|:---|:---|
| SLB监听端口与Ingress规则不匹配 | SLB控制台查看监听配置 / Ingress YAML | 修正Ingress注解中的端口配置，或调整SLB监听 |
| SLB证书与Ingress TLS Secret不一致 | SLB控制台查看证书 / `kubectl get secret` | 重新上传证书到SLB，或更新Ingress TLS Secret |
| ALB Ingress虚拟服务器组IP不更新 | ALB控制台查看后端服务器组 | 重启ALB Ingress Controller Pod |
| ACK Ingress Controller与CCM版本冲突 | ACK控制台查看组件版本兼容性 | 通过ACK控制台统一升级Ingress Controller和CCM |
| 专有云中ApsaraLB对接异常 | 专有云ASO/天基控制台查看LB服务 | 联系阿里云驻场工程师修复底座负载均衡服务 |
| Ingress注解配置导致SLB重复创建 | `kubectl get ingress -o yaml` 查看 `slb-id` 注解 | 固定使用已有SLB ID，避免自动创建新SLB |
| SLB带宽超限导致访问缓慢 | 云监控查看SLB出入带宽 | 升级SLB带宽规格，或启用按量计费 |
| WAF/CDN回源到ACK Ingress异常 | WAF/CDN控制台查看回源配置 | 确认回源地址为SLB IP，且安全组允许WAF/CDN网段 |

> **远程顾问无法直连时的阿里云控制台排查**：
> 1. **ACK 控制台 > 集群 > 网络 > Ingress**：查看 Ingress 列表、状态和关联的 SLB/ALB
> 2. **阿里云控制台 > SLB/ALB**：查看负载均衡实例状态、监听配置、后端健康状态
> 3. **云监控 > 负载均衡**：查看 QPS、延迟、后端健康状态趋势
> 4. **ACK 控制台 > 集群 > 运维管理 > 网络诊断**：使用 ACK 内置网络诊断工具
> 5. 如果是 **专有云**，请通过 **ASO/天基控制台** 查看 **Apsara LoadBalancer** 服务状态

**分支决策**：
- **ACK-I1**：SLB/ALB监听或健康检查配置异常 → 修正SLB监听配置或Ingress注解
- **ACK-I2**：ACK Ingress Controller组件异常 → 重启或升级Ingress Controller
- **ACK-I3**：安全组/网络策略阻断 → 修正安全组规则或NetworkPolicy
- **ACK-I4**：专有云平台底座LB异常 → 升级至阿里云TAM/驻场工程师

## Round 2
### 分支 1：Ingress修复
- `检查backend serviceName和servicePort`
  - 如无法执行：请提供当前可执行的环境信息
- `如路径错误: kubectl edit ingress <ing> -n <ns>`
  - 如无法执行：请提供当前可执行的环境信息
- `如host不匹配: 确认DNS解析正确`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 2：后端修复
- `如Endpoint为空: 检查Pod标签选择器`
  - 如无法执行：请提供当前可执行的环境信息
- `如Pod未就绪: 检查健康检查配置`
  - 如无法执行：请提供当前可执行的环境信息
- `如Service异常: 重新创建Service`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 3：证书修复
- `如证书过期: kubectl create secret tls <secret> --cert=<cert> --key=<key> -n <ns>`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。
  - 如无法执行：请提供当前可执行的环境信息
- `如自签名: 配置浏览器/客户端信任`
  - 如无法执行：请提供当前可执行的环境信息
- `如使用cert-manager: 检查Certificate和ClusterIssuer状态`
  - 如无法执行：请提供当前可执行的环境信息

## Round 3
### 分支 1：路由验证
- `curl -H "Host:<host>" http://<ingress-ip>/path`
  - 如无法执行：请提供当前可执行的环境信息
- `检查返回状态码和内容`
  - 如无法执行：请提供当前可执行的环境信息
- `验证HTTPS: curl -k https://<host>`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 2：性能验证
- `ab -n 1000 -c 10 http://<host>/`
  - 如无法执行：请提供当前可执行的环境信息
- `检查Ingress Controller资源使用`
  - 如无法执行：请提供当前可执行的环境信息
- `如需要: 扩容Ingress Controller副本`
  - 如无法执行：请提供当前可执行的环境信息

## 升级决策点
- **P0（立即升级）**：集群核心功能受损，多服务中断
- **P1（建议升级）**：单服务中断，有 workaround
- **P2（观察）**：非关键路径，可稍后处理

## 附录：常用命令速查
| 场景 | 命令 |
|:---|:---|
| 查看资源 | `kubectl get <resource> -n <ns>` |
| 查看详情 | `kubectl describe <resource> <name> -n <ns>` |
| 查看日志 | `kubectl logs <pod> -n <ns>` |
| 进入容器 | `kubectl exec -it <pod> -n <ns> -- /bin/sh` |

## Round 1 补充 — Ingress Controller状态

### 分支 4：Controller Pod状态
- `kubectl get pods -n <ingress-ns> -l app.kubernetes.io/name=<ingress-controller>`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。
  - 如无法执行：请提供Ingress Controller Pod状态
- `kubectl logs -n <ingress-ns> -l app.kubernetes.io/name=<ingress-controller> --tail=100`
  - 如无法执行：请提供Controller日志
- `kubectl top pod -n <ingress-ns> -l app.kubernetes.io/name=<ingress-controller>`
  - 如无法执行：请描述资源使用情况

### 分支 5：负载均衡器状态
- `kubectl get svc -n <ingress-ns> <ingress-svc>`
  - 如无法执行：请提供LB服务状态
- `kubectl describe svc -n <ingress-ns> <ingress-svc> | grep -A10 Events`
  > 💬 **顾问确认**：请检查输出是否符合预期，确认无误后再继续下一步。
  - 如无法执行：请提供LB事件
- `检查云提供商控制台中的LB健康状态`
  - 如无法执行：请描述LB状态

## Round 2 补充 — 高级修复

### 分支 4：Rate Limiting配置
- `kubectl get configmap -n <ingress-ns> | grep <ingress-config>`
  - 如无法执行：请提供配置信息
- `检查nginx.conf或类似配置中的limit_req_zone`
  > 💬 **顾问确认**：如果命令执行失败，请提供错误信息，我会调整方案。
  - 如无法执行：请提供限速配置
- `调整或禁用限速: kubectl edit configmap <cm> -n <ingress-ns>`
  - 如无法执行：请描述限速策略

### 分支 5：CORS和重写规则
- `kubectl get ingress <ing> -n <ns> -o json | jq '.metadata.annotations'`
  - 如无法执行：请提供Ingress注解
- `检查rewrite-target、ssl-redirect等注解`
  - 如无法执行：请描述当前注解配置
- `如CORS问题: 添加cors-allow-origin等注解`
  > 💬 **顾问确认**：在执行危险操作前，请再次确认当前备份状态。
  - 如无法执行：请描述CORS需求

## Round 3 补充 — 验证与监控

### 分支 3：性能基准测试
- `ab -n 10000 -c 100 -H "Host:<host>" http://<ingress-ip>/`
  - 如无法执行：请提供性能测试结果
- `检查Ingress Controller CPU/内存使用`
  - 如无法执行：请描述资源瓶颈
- `如需要: 水平扩容Ingress Controller副本数`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。
  - 如无法执行：请描述扩容计划

### 分支 4：日志和监控
- `kubectl logs -n <ingress-ns> <ingress-pod> | grep <host>`
  - 如无法执行：请提供访问日志
- `配置Ingress访问日志输出到ELK/Loki`
  - 如无法执行：请描述日志收集方案
- `设置Ingress错误率告警（5xx比例>1%）`
  - 如无法执行：请描述告警配置

## 升级决策点（补充）

- **P0**：Ingress Controller全部崩溃，所有外部流量中断
- **P1**：部分路由失效，影响特定服务
- **P2**：配置优化需求，非问题场景

## 附录：Ingress问题排查流程

```
外部访问失败
    ├── DNS问题 → 检查DNS解析/CNAME记录
    ├── LB问题 → 检查云LB/NodePort/健康检查
    ├── Ingress配置 → 检查host/path/backend
    ├── TLS问题 → 检查证书/Secret
    └── 后端服务 → 检查Pod/Service/Endpoint
```

| 限制场景 | 替代方案 | 降级策略 |
|:---|:---|:---|
| 无法修改Ingress | 使用kubectl patch | 通过GitOps/ArgoCD更新 |
| 证书无法更新 | 使用cert-manager自动管理 | 手动创建临时证书 |
| Ingress Controller资源不足 | 垂直或水平扩容 | 减少非必要路由 |
| 配置冲突 | 分离Ingress资源 | 使用不同的IngressClass |

## 相关案例

- [[concepts/case-studies/2026-04-10-ingress-502-bad-gateway.md|2026-04-10-ingress-502-bad-gateway]]
## Related

- [[domain-12-cloud-providers/阿里云/05-阿里云SLB与Ingress.md|阿里云SLB与Ingress]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-17-system-foundation/02-kubernetes-events/01-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[32-发布/package/2026-07-02_18-29/profiles/sre/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/07-ingress-troubleshooting|15 - Ingress 故障排查 (Ingress Troubleshooting)]]
- [[entities/argo.md|Argo Workflows]]


<!-- risk-assessed -->
