---
title: "监控告警问题 — 远程顾问对话脚本"
category: "troubleshooting"
tags: ["observability", "remote-consultant"]
created: "2026-05-23"
updated: "2026-05-23"
dialogue_id: "DIALOGUE-SKILL-MON-001"
skill_id: "SKILL-MON-001"
version: "1.0.0"
role: "remote-consultant"
language: "zh"
summary: "监控告警问题的远程顾问对话脚本，覆盖Prometheus、Grafana、Alertmanager排查。"
relationships:
  - target: "[[entities/deployment]]"
    type: uses
  - target: "[[entities/kubernetes]]"
    type: uses
  - target: "[[domain-17-system-foundation/topic-dictionary/fundamentals/namespaces]]"
    type: uses
---

# K8s Monitoring & Alerting Failure 诊断 — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**，只能通过对话指导现场工程师执行操作。
> 覆盖范围：Prometheus 采集失败、Grafana 面板无数据、Alertmanager 不发送通知、告警规则错误等监控告警问题。

---

## 对话入口

### 入口 A：工程师明确报告监控异常

**工程师**：「Prometheus target 显示 DOWN」/「Grafana 面板没数据」/「监控告警不触发」

**顾问回应**：
> 收到，监控告警问题会让我们对集群状态"盲飞"，需要尽快恢复。作为远程顾问，我无法直连你的集群，请你配合执行检查命令。
>
> 先回答三个问题（30 秒内）：
> 1. **影响范围**：Prometheus/Grafana/Alertmanager 全部异常，还是只有部分组件？
> 2. **紧急程度**：是否有正在发生的问题因监控缺失而未被及时发现？
> 3. **发生时间**：问题是突然发生还是逐渐恶化？最近是否有监控相关的变更（规则更新、配置修改、扩容）？

---

### 入口 B：工程师报告告警未收到通知

**工程师**：「告警规则触发了但没收到邮件/钉钉/Slack」/「PagerDuty 没收到告警」

**顾问回应**：
> 告警不通知是严重的可观测性缺口。先确认告警链路各节点状态。
>
> 请执行以下命令检查 Alertmanager 状态：
> ```bash
> kubectl get pods -n monitoring -l app=alertmanager
> ```
> **如果无法执行**（不知道 monitoring namespace）→ `kubectl get pods --all-[[domain-17-system-foundation/topic-dictionary/fundamentals/namespaces|namespaces]] | grep -i alertmanager`
> **如果 grep 无结果** → `kubectl get pods --all-namespaces | grep -E "alert|prometheus|grafana"`
> 请把 Pod 状态、重启次数贴给我。

---

### 入口 C：工程师报告 Grafana 可视化异常

**工程师**：「Grafana Dashboard 显示 No Data」/「面板有数据但图表是断线的」

**顾问回应**：
> Grafana 无数据可能是数据源问题，也可能是 Prometheus 采集出了问题。先分层排查。
>
> 请执行：
> ```bash
> kubectl get pods -n monitoring
> ```
> **如果没有 monitoring namespace** → `kubectl get namespaces | grep -E "monitor|observ|prometheus|grafana"`
> **如果没有任何监控相关 namespace** → 请告诉我你们的监控组件部署在哪个 namespace
> 请把所有监控相关 Pod 的状态列表贴给我。

---

### 入口 D：工程师报告 Prometheus 相关告警

**工程师**：「Prometheus Pod 重启了」/「prometheus-k8s-0 处于 Pending」

**顾问回应**：
> Prometheus 本身异常会导致整个监控体系停摆。先确认 Prometheus 组件状态。
>
> ```bash
> kubectl get pods -n monitoring -l app=prometheus
> ```
> **如果没有 app=prometheus 标签** → `kubectl get pods -n monitoring | grep -i prom`
> **如果 monitoring namespace 不存在** → `kubectl get pods --all-namespaces | grep -i prom`
> 请把 Pod 状态、重启次数、AGE 贴给我。

---

## Round 1：快速定位问题组件

> 目标：判断问题发生在 **Prometheus 采集层 → Grafana 展示层 → Alertmanager 通知层 → 告警规则层** 的哪一层。

---

### Round 1 — 分支 A：Prometheus 相关 Pod 异常

**工程师反馈**：Prometheus Pod 处于 CrashLoopBackOff、Pending 或频繁重启。

**顾问指令**：
> Prometheus 自身不健康是根因。先收集 Pod 状态和日志。
>
> 1. 查看 Pod 详细信息：`kubectl describe pod <prometheus-pod-name> -n monitoring`
>    **如果不知道 Pod 名** → `kubectl get pods -n monitoring -o wide | grep -i prom`
> 2. 查看日志（已崩溃加 `--previous`）：`kubectl logs <prometheus-pod-name> -n monitoring --tail=50`
>    **如果 logs 为空** → `kubectl logs <prometheus-pod-name> -n monitoring --previous --tail=50`
>    **如果 previous 也拿不到** → `kubectl get events -n monitoring --field-selector reason=BackOff | tail -20`
> 3. 检查 Prometheus PVC 和存储：`kubectl get pvc -n monitoring | grep prometheus`
>    **如果无法执行** → `kubectl get pv | grep prometheus`
> 请把 Events、日志和存储状态贴给我。

**分支决策**：
- **A1**：OOMKilled 或存储满 → Round 2 — 分支 A（Prometheus 存储/资源修复）
- **A2**：配置加载错误（scrape config 语法错误）→ Round 2 — 分支 B（配置修复）
- **A3**：Pod 正常但 target 大量 DOWN → Round 2 — 分支 C（采集目标排查）

---

### Round 1 — 分支 B：Grafana 数据源异常

**工程师反馈**：Grafana Pod Running，但面板显示 No Data 或数据源测试失败。

**顾问指令**：
> Grafana 本身正常，问题出在与 Prometheus 的连接或数据源配置上。
>
> 1. 检查 Grafana Pod 状态：`kubectl get pods -n monitoring -l app=grafana`
>    **如果没有 app=grafana 标签** → `kubectl get pods -n monitoring | grep -i grafana`
> 2. 检查 Grafana 数据源 ConfigMap：`kubectl get configmap -n monitoring | grep datasource`
>    **如果无 datasource ConfigMap** → `kubectl get configmap -n monitoring`
> 3. 检查 Prometheus Service：`kubectl get svc -n monitoring | grep prometheus`
>    **如果无法执行** → `kubectl get svc --all-namespaces | grep -E "prometheus|9090"`
> 请告诉我 Grafana Pod 状态、数据源 ConfigMap 是否存在、Prometheus Service 的 CLUSTER-IP 和 PORT。

**分支决策**：
- **B1**：Prometheus Service 不存在或 ClusterIP 为 None → Round 2 — 分支 D（Service/Endpoint 修复）
- **B2**：数据源 ConfigMap 中 URL 配置错误 → Round 2 — 分支 E（数据源配置修复）
- **B3**：Service 和 ConfigMap 都正常 → Round 2 — 分支 F（网络策略/连通性排查）

---

### Round 1 — 分支 C：Alertmanager 不发送通知

**工程师反馈**：Alertmanager Pod Running，但告警触发了却没收到通知。

**顾问指令**：
> Alertmanager 运行正常但不发通知，通常是路由配置或接收器配置问题。
>
> 1. 检查 Alertmanager Pod 状态：`kubectl get pods -n monitoring -l app=alertmanager`
>    **如果没有 app=alertmanager 标签** → `kubectl get pods -n monitoring | grep -i alert`
> 2. 检查 Alertmanager 配置 Secret：`kubectl get secret -n monitoring | grep alertmanager`
>    **如果无 alertmanager Secret** → `kubectl get secret -n monitoring`
> 3. 检查 PrometheusRule 是否存在：`kubectl get prometheusrules --all-namespaces | head -10`
>    **如果无法执行** → `kubectl get prometheusrule --all-namespaces 2>/dev/null || echo "无 prometheusrules CRD"`
> 请告诉我 Alertmanager Pod 状态、配置 Secret 是否存在、以及是否有告警规则。

**分支决策**：
- **C1**：Alertmanager 配置 Secret 不存在 → Round 2 — 分支 G（配置重建）
- **C2**：PrometheusRule 存在但告警规则可能语法错误 → Round 2 — 分支 H（规则排查）
- **C3**：配置都存在，需要检查路由和接收器 → Round 2 — 分支 I（路由/接收器排查）

---

## Round 2：分层深入诊断

> 目标：根据 Round 1 确定的问题层级，执行针对性的深度检查和修复。

---

### Round 2 — 分支 A：Prometheus 存储/资源修复

**顾问指令**：
> Prometheus 因存储满或内存不足被杀死。需要扩容或清理。
>
> 1. 检查当前 PVC 用量：`kubectl exec <prometheus-pod-name> -n monitoring -- df -h /prometheus`
>    **如果 Pod 已崩溃无法 exec** → `kubectl get pvc <prometheus-pvc-name> -n monitoring -o yaml | grep -A 5 resources`
>    **如果不知道 PVC 名** → `kubectl get pvc -n monitoring`
> 2. 检查 Prometheus 资源限制：`kubectl get prometheus -n monitoring -o yaml | grep -A 10 resources`
>    **如果无 prometheus CRD** → `kubectl get [[entities/deployment|deployment]] -n monitoring | grep prometheus`
>    **如果是 Deployment 部署** → `kubectl get deployment <prometheus-deploy> -n monitoring -o yaml | grep -A 10 resources`
> 3. 检查 TSDB 状态（如果 Pod 能启动）：`kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &`
>    **如果无法 port-forward** → `kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/status/tsdb`
>    **如果 wget 也没有** → `kubectl logs <prometheus-pod> -n monitoring --tail=30 | grep -i "tsdb\|compaction\|retention"`
> 请告诉我存储使用百分比、资源限制值、TSDB 是否有异常。

**修复方案**：
> - **方案 A（存储扩容）**：`kubectl patch pvc <prometheus-pvc> -n monitoring --type='json' -p='[{"op": "replace", "path": "/spec/resources/requests/storage", "value": "100Gi"}]'`
>   **如果无法 patch** → `kubectl edit pvc <prometheus-pvc> -n monitoring`（手动修改）
>   **如果没有交互终端** → 请管理员通过存储类扩容
> - **方案 B（缩短 retention）**：`kubectl patch prometheus <name> -n monitoring --type='json' -p='[{"op": "replace", "path": "/spec/retention", "value": "7d"}]'`
> - **方案 C（增加内存限制）**：`kubectl patch prometheus <name> -n monitoring --type='json' -p='[{"op": "add", "path": "/spec/resources", "value": {"limits":{"memory":"16Gi"},"requests":{"memory":"8Gi"}}}]'`

**分支决策**：
- **A1**：扩容/清理后 Prometheus 恢复 → 进入验证阶段
- **A2**：存储类不支持扩容 → 升级决策点（联系存储管理员）
- **A3**：扩容后仍 OOM → Round 3 — 分支 J（高基数指标排查）

---

### Round 2 — 分支 B：Prometheus 配置修复

**顾问指令**：
> Prometheus 因配置错误无法启动。需要修复 scrape 配置或规则配置。
>
> 1. 查看具体错误：`kubectl logs <prometheus-pod> -n monitoring --previous --tail=50 | grep -i "error\|failed\|invalid"
>    **如果无法执行** → `kubectl get events -n monitoring --field-selector reason=Failed | tail -20`
> 2. 检查 Prometheus 配置 ConfigMap/Secret：`kubectl get configmap -n monitoring | grep prom`
>    **如果无 configmap** → `kubectl get secret -n monitoring | grep prom`
> 3. 查看配置内容：`kubectl get configmap <prometheus-config> -n monitoring -o yaml | grep -A 20 prometheus.yml`
>    **如果无法执行** → `kubectl get secret <prometheus-config> -n monitoring -o jsonpath='{.data.*}' | base64 -d | head -50`
> 请把错误日志和配置内容贴给我。

**修复方案**：
> 1. 备份当前配置：`kubectl get configmap <prometheus-config> -n monitoring -o yaml > /tmp/prometheus-config-backup.yaml`
>    **如果无法写入 /tmp** → `kubectl get configmap <prometheus-config> -n monitoring -o yaml`（手动保存）
> 2. 修正配置后 apply：`kubectl apply -f <fixed-config.yaml> -n monitoring`
>    **如果无法 apply** → `kubectl edit configmap <prometheus-config> -n monitoring`
> 3. 重启 Prometheus：`kubectl rollout restart deployment <prometheus-deploy> -n monitoring`
>    **如果是 StatefulSet** → `kubectl rollout restart statefulset <prometheus-sts> -n monitoring`

**分支决策**：
- **B1**：配置修复后恢复 → 进入验证阶段
- **B2**：配置反复出错 → Round 3 — 分支 K（配置深度审查）
- **B3**：无法确定配置错误位置 → 升级决策点

---

### Round 2 — 分支 C：采集目标排查

**顾问指令**：
> Prometheus Pod 正常但大量 target DOWN，问题在 ServiceMonitor/PodMonitor 或网络层。
>
> 1. 查看 target 状态：`kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &`
>    `curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | select(.health!="up")'`
>    **如果无法 port-forward** → `kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/targets`
>    **如果 wget 也没有** → 在浏览器中访问 Prometheus UI（如果有 Ingress/NodePort）
> 2. 检查 ServiceMonitor：`kubectl get servicemonitor --all-namespaces`
>    **如果无法执行** → `kubectl get servicemonitors --all-namespaces 2>/dev/null || echo "无 servicemonitor CRD"`
> 3. 检查目标 Service 标签：`kubectl get svc -n <target-namespace> --show-labels`
> 请告诉我哪些 target DOWN、对应的错误信息（connection refused、timeout、403 等）。

**分支决策**：
- **C1**：ServiceMonitor 选择器与 Service 标签不匹配 → Round 3 — 分支 L（ServiceMonitor 修复）
- **C2**：target 连接超时或拒绝 → Round 3 — 分支 M（网络/安全策略排查）
- **C3**：target 返回 403/401 → Round 3 — 分支 N（认证/授权排查）

---

### Round 2 — 分支 D：Service/Endpoint 修复

**顾问指令**：
> Prometheus Service 不存在或没有 Endpoint，导致 Grafana 无法连接。
>
> 1. 检查 Service 详情：`kubectl get svc -n monitoring <prometheus-svc-name> -o yaml`
>    **如果无法执行** → `kubectl describe svc -n monitoring <prometheus-svc-name>`
> 2. 检查 Endpoints：`kubectl get endpoints -n monitoring <prometheus-svc-name>`
>    **如果为空** → `kubectl get pods -n monitoring -l app=prometheus -o wide`
> 3. 检查 selector 是否匹配：`kubectl get svc -n monitoring <prometheus-svc-name> -o jsonpath='{.spec.selector}'`
>    **如果无法执行 jsonpath** → `kubectl get svc -n monitoring <prometheus-svc-name> -o yaml | grep -A 5 selector`
> 请告诉我 Service 的 selector、Endpoints 是否有 IP、Pod 标签是否匹配。

**修复方案**：
> - 如果 selector 错误：`kubectl patch svc <prometheus-svc-name> -n monitoring --type='json' -p='[{"op": "replace", "path": "/spec/selector", "value": {"app":"prometheus"}}]'`
> - 如果 Endpoints 为空但 Pod 正常：检查 Pod 的 readinessProbe 是否通过
> - 如果 Service 不存在：创建 Service YAML 并 apply

**分支决策**：
- **D1**：修复 selector 后恢复 → 进入验证阶段
- **D2**：Pod 未通过 readinessProbe → Round 3 — 分支 O（Pod 健康排查）
- **D3**：Service 存在但网络不通 → Round 3 — 分支 M（网络排查）

---

### Round 2 — 分支 E：Grafana 数据源配置修复

**顾问指令**：
> Grafana 数据源 URL 或认证配置错误。
>
> 1. 查看当前数据源配置：`kubectl get configmap -n monitoring <datasource-config> -o yaml`
>    **如果 ConfigMap 中无 datasource** → `kubectl get secret -n monitoring | grep grafana`
> 2. 检查 Prometheus Service 地址：`kubectl get svc -n monitoring prometheus-k8s`
>    **如果 svc 名不同** → `kubectl get svc -n monitoring | grep prom`
> 3. 测试从 Grafana Pod 连接 Prometheus：`kubectl exec <grafana-pod> -n monitoring -- wget -qO- http://prometheus-k8s:9090/api/v1/status/targets | head -5`
>    **如果 wget 无** → `kubectl exec <grafana-pod> -n monitoring -- curl -s http://prometheus-k8s:9090/api/v1/status/targets | head -5`
> 请告诉我数据源中配置的 URL、Prometheus Service 的实际地址。

**修复方案**：
> 1. 修正 datasource URL：更新 ConfigMap 中 `url: http://prometheus-k8s:9090`
> 2. 重启 Grafana：`kubectl rollout restart deployment grafana -n monitoring`
>    **如果无法 rollout restart** → `kubectl delete pod <grafana-pod> -n monitoring`
> 3. 在 Grafana UI 中测试数据源连接

**分支决策**：
- **E1**：URL 修正后数据源测试通过 → 进入验证阶段
- **E2**：URL 正确但测试仍失败 → Round 3 — 分支 M（网络连通性排查）
- **E3**：需要配置认证（basic auth/tls）→ Round 3 — 分支 N（认证配置修复）

---

### Round 2 — 分支 F：网络策略/连通性排查

**顾问指令**：
> Grafana 和 Prometheus 都正常，但之间无法通信，可能是 NetworkPolicy 或 CNI 问题。
>
> 1. 检查 NetworkPolicy：`kubectl get networkpolicy --all-namespaces`
>    **如果无 NetworkPolicy CRD** → `kubectl get ciliumnetworkpolicy --all-namespaces 2>/dev/null || kubectl get caliconetworkpolicy --all-namespaces 2>/dev/null || echo "无 NetworkPolicy"`
> 2. 从 Grafana Pod 测试连通性：`kubectl exec <grafana-pod> -n monitoring -- nc -vz prometheus-k8s 9090`
>    **如果无 nc** → `kubectl exec <grafana-pod> -n monitoring -- sh -c "echo '' > /dev/tcp/prometheus-k8s/9090"`
>    **如果 /dev/tcp 不支持** → `kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -n monitoring -- nc -vz prometheus-k8s 9090`
> 3. 检查 CNI Pod 状态：`kubectl get pods -n kube-system | grep -E 'calico|cilium|flannel|weave'`
> 请告诉我 NetworkPolicy 是否存在、连通性测试结果、CNI 状态。

**分支决策**：
- **F1**：NetworkPolicy 阻断了流量 → Round 3 — 分支 P（NetworkPolicy 修复）
- **F2**：CNI Pod 异常 → 升级决策点（网络深度诊断 SKILL-NET-003）
- **F3**：无 NetworkPolicy 且 CNI 正常 → Round 3 — 分支 Q（DNS/Service 深度排查）

---

### Round 2 — 分支 G：Alertmanager 配置重建

**顾问指令**：
> Alertmanager 配置 Secret 缺失，需要重建。
>
> 1. 确认 Secret 确实不存在：`kubectl get secret -n monitoring | grep alertmanager`
> 2. 检查 Prometheus Operator 日志（如果使用）：`kubectl logs -n monitoring -l app=prometheus-operator --tail=30`
>    **如果无 operator** → `kubectl get deployment -n monitoring`
> 3. 检查 Alertmanager CR：`kubectl get alertmanager -n monitoring`
>    **如果无法执行** → `kubectl get alertmanagers -n monitoring 2>/dev/null || echo "无 alertmanager CRD"`
> 请告诉我是否使用了 Prometheus Operator、Alertmanager CR 是否存在。

**修复方案**：
> 创建 Alertmanager 配置 Secret：
> ```bash
> cat <<EOF | kubectl apply -f -
> apiVersion: v1
> kind: Secret
> metadata:
>   name: alertmanager-<name>
>   namespace: monitoring
> stringData:
>   alertmanager.yaml: |
>     global:
>       smtp_smarthost: 'localhost:587'
>       smtp_from: 'alertmanager@example.com'
>     route:
>       receiver: 'default'
>     receivers:
>     - name: 'default'
>       email_configs:
>       - to: 'oncall@example.com'
> EOF
> ```
> **如果无法 apply** → 把 YAML 保存为文件后请管理员执行

**分支决策**：
- **G1**：配置重建后 Alertmanager 恢复 → 进入验证阶段
- **G2**：使用了 Operator 但配置不生效 → Round 3 — 分支 R（Operator 配置深度排查）
- **G3**：无 Operator 且手动部署复杂 → 升级决策点

---

### Round 2 — 分支 H：告警规则排查

**顾问指令**：
> PrometheusRule 存在但告警不触发，可能是规则语法错误或条件不满足。
>
> 1. 查看规则内容：`kubectl get prometheusrules -n monitoring <rule-name> -o yaml`
>    **如果无法执行** → `kubectl get prometheusrule -n monitoring <rule-name> -o yaml`
> 2. 在 Prometheus UI 查看规则状态：`kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &`
>    访问 http://localhost:9090/rules
>    **如果无法 port-forward** → `kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/rules | grep -A 5 <rule-name>`
> 3. 手动测试 PromQL：`kubectl exec <prometheus-pod> -n monitoring -- wget -qO- "http://localhost:9090/api/v1/query?query=up%7Bjob%3D%27kubernetes-pods%27%7D%3D%3D0"`
> 请告诉我规则列表中是否有错误标记、PromQL 在 Prometheus 中是否能正常查询。

**修复方案**：
> 修正规则中的 PromQL：`kubectl patch prometheusrules <rule-name> -n monitoring --type='json' -p='[{"op": "replace", "path": "/spec/groups/0/rules/0/expr", "value": "up{job=\"[[entities/kubernetes|kubernetes]]-pods\"} == 0"}]'`
> **如果 patch 复杂** → `kubectl edit prometheusrules <rule-name> -n monitoring`

**分支决策**：
- **H1**：修正 PromQL 后规则正常 → 进入验证阶段
- **H2**：规则语法正确但不触发（条件不满足）→ 调整阈值或确认业务确实异常
- **H3**：规则反复报错 → Round 3 — 分支 K（配置深度审查）

---

### Round 2 — 分支 I：路由/接收器排查

**顾问指令**：
> Alertmanager 配置存在但通知未发送，需要检查路由匹配和接收器配置。
>
> 1. 查看当前配置：`kubectl get secret alertmanager-<name> -n monitoring -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d`
>    **如果无法执行 jsonpath** → `kubectl get secret alertmanager-<name> -n monitoring -o yaml | grep alertmanager.yaml | head -5`
>    **如果 base64 解码失败** → `kubectl get secret alertmanager-<name> -n monitoring -o yaml`，手动提取并解码
> 2. 测试告警发送：`curl -X POST http://alertmanager:9093/api/v2/alerts -H "Content-Type: application/json" -d '[{"labels":{"alertname":"TestAlert","severity":"warning"}}]'`
>    **如果无法 curl** → 从集群内 Pod 执行：`kubectl run test-alert --image=curlimages/curl --rm -it --restart=Never -- -X POST http://alertmanager.monitoring:9093/api/v2/alerts -H "Content-Type: application/json" -d '[{"labels":{"alertname":"TestAlert","severity":"warning"}}]'`
> 3. 检查 Alertmanager 日志：`kubectl logs <alertmanager-pod> -n monitoring --tail=50`
> 请把 alertmanager.yaml 配置、测试结果、日志贴给我。

**修复方案**：
> - 修正 route 的 matcher：确保 `match` 或 `match_re` 能匹配到实际告警标签
> - 修正 receiver 配置：确认 webhook URL、邮箱地址、Slack token 正确
> - 检查 silence：确认没有全局 silence 抑制了通知

**分支决策**：
- **I1**：路由/接收器修复后通知恢复 → 进入验证阶段
- **I2**：配置正确但网络不通（webhook 超时）→ Round 3 — 分支 M（网络排查）
- **I3**：接收器服务端问题（Slack/邮件服务器问题）→ 升级决策点（外部服务）

---

## Round 3：精确修复与验证

> 目标：执行最终修复动作，验证监控告警恢复正常，决定是否升级。

---

### Round 3 — 分支 J：高基数指标排查

**顾问指令**：
> Prometheus 扩容后仍内存过高，可能存在高基数指标（high cardinality）。
>
> 1. 检查 TSDB  head series：`kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/status/tsdb | jq '.data.headStats'`
>    **如果无法执行** → `kubectl logs <prometheus-pod> -n monitoring --tail=50 | grep -i "head\|series\|memory"`
> 2. 查看高基数指标：`kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &`
>    访问 http://localhost:9090/tsdb-status
>    **如果无法 port-forward** → `kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/label/__name__/values | jq '.data[]' | wc -l`
> 3. 检查 target 的 label 数量：`kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/targets | jq '.data.activeTargets[].labels | length'`
> 请告诉我 head series 数量、指标总数、是否有异常高基数的 label。

**修复方案**：
> - 使用 recording rules 聚合高基数指标
> - 通过 `metric_relabel_configs` drop 不需要的 label
> - 升级 Prometheus 到支持内存优化的版本

**分支决策**：
- **J1**：优化后内存稳定 → 修复完成
- **J2**：高基数来自系统级指标（如 cadvisor）→ 联系基础设施团队优化采集
- **J3**：无法定位高基数来源 → 升级决策点

---

### Round 3 — 分支 K：配置深度审查

**顾问指令**：
> 配置反复出错，需要逐行审查。
>
> 1. 导出完整配置：`kubectl get configmap <prometheus-config> -n monitoring -o jsonpath='{.data.*}'`
>    **如果无法执行** → `kubectl get configmap <prometheus-config> -n monitoring -o yaml`
> 2. 使用 promtool 验证（如果可用）：`kubectl exec <prometheus-pod> -n monitoring -- promtool check config /etc/prometheus/prometheus.yml`
>    **如果无 promtool** → 在本地安装 prometheus 后验证
> 3. 分段测试：注释掉部分 scrape_configs 逐个启用，定位错误段落
> 请把完整配置贴给我，我会帮你逐行审查。

**分支决策**：
- **K1**：发现配置错误并修复 → 进入验证阶段
- **K2**：配置语法正确但语义错误 → 提供参考配置模板
- **K3**：配置过于复杂（如大量 relabel）→ 升级决策点

---

### Round 3 — 分支 L：ServiceMonitor 修复

**顾问指令**：
> ServiceMonitor 选择器与目标 Service 不匹配。
>
> 1. 查看 ServiceMonitor 选择器：`kubectl get servicemonitor <name> -n <namespace> -o yaml | grep -A 10 selector`
> 2. 查看目标 Service 标签：`kubectl get svc <svc-name> -n <target-namespace> --show-labels`
> 3. 检查 namespaceSelector：`kubectl get servicemonitor <name> -n <namespace> -o yaml | grep -A 5 namespaceSelector`
> 请把 ServiceMonitor 的 selector、Service 的 labels、namespaceSelector 贴给我。

**修复方案**：
> 修正选择器标签：`kubectl patch servicemonitor <name> -n <namespace> --type='json' -p='[{"op": "replace", "path": "/spec/selector/matchLabels", "value": {"app":"<correct-label>"}}]'`
> 修正 namespaceSelector：`kubectl patch servicemonitor <name> -n <namespace> --type='json' -p='[{"op": "replace", "path": "/spec/namespaceSelector", "value": {"matchNames":["<target-namespace>"]}}]'`

**分支决策**：
- **L1**：修正后 target UP → 修复完成
- **L2**：标签匹配但仍 DOWN → Round 3 — 分支 M（网络排查）
- **L3**：无 ServiceMonitor CRD（手动配置 prometheus.yml）→ 修改 prometheus 配置并重启

---

### Round 3 — 分支 M：网络/安全策略排查

**顾问指令**：
> target 连接超时或拒绝，需要检查网络和防火墙。
>
> 1. 从 Prometheus Pod 测试目标：`kubectl exec <prometheus-pod> -n monitoring -- nc -vz <target-ip> <target-port>`
>    **如果无 nc** → `kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://<target-ip>:<target-port>/metrics --timeout=5`
> 2. 检查目标 Pod 是否就绪：`kubectl get pod <target-pod> -n <target-namespace> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'`
>    **如果无法执行 jsonpath** → `kubectl get pod <target-pod> -n <target-namespace>`（看 READY 列）
> 3. 检查 NetworkPolicy：`kubectl get networkpolicy -n <target-namespace>`
> 请告诉我连通性测试结果、目标 Pod 是否 Ready、NetworkPolicy 是否存在。

**修复方案**：
> 添加 allow-prometheus NetworkPolicy：
> ```yaml
> apiVersion: networking.k8s.io/v1
> kind: NetworkPolicy
> metadata:
>   name: allow-prometheus-scrape
>   namespace: <target-namespace>
> spec:
>   podSelector: {}
>   policyTypes:
>   - Ingress
>   ingress:
>   - from:
>     - namespaceSelector:
>         matchLabels:
>           kubernetes.io/metadata.name: monitoring
>     ports:
>     - protocol: TCP
>       port: 8080
>     - protocol: TCP
>       port: 9090
>     - protocol: TCP
>       port: 9100
> ```

**分支决策**：
- **M1**：添加 NetworkPolicy 后 target UP → 修复完成
- **M2**：无 NetworkPolicy 但仍不通 → 检查节点防火墙/安全组
- **M3**：目标 Pod 未 Ready → 排查目标应用健康

---

### Round 3 — 分支 N：认证/授权排查

**顾问指令**：
> target 返回 403/401，需要检查认证配置。
>
> 1. 检查目标是否启用了认证：`kubectl get pod <target-pod> -n <target-namespace> -o yaml | grep -A 5 args`
>    **如果无法执行** → `kubectl describe pod <target-pod> -n <target-namespace> | grep -A 10 "Containers:"`
> 2. 检查 Prometheus scrape 配置中的认证：`kubectl get configmap <prometheus-config> -n monitoring -o yaml | grep -A 10 "bearer_token\|basic_auth\|tls_config"`
> 3. 检查 RBAC：目标是否有 ServiceAccount 和权限
> 请把目标 Pod 的启动参数、Prometheus 的认证配置贴给我。

**修复方案**：
> - 添加 bearer_token_file 到 scrape_config
> - 创建 ServiceAccount 和 RoleBinding 供 Prometheus 使用
> - 如果目标不需要认证，临时关闭认证验证

**分支决策**：
- **N1**：认证配置修正后恢复 → 修复完成
- **N2**：RBAC 问题复杂 → 升级决策点（安全团队）
- **N3**：TLS 证书问题 → Round 3 — 分支 S（证书修复）

---

## 验证修复

**顾问指令**：
> 修复已应用，验证监控告警是否恢复正常。
>
> 1. 验证 Prometheus Running：`kubectl get pods -n monitoring | grep prometheus`
>    **如果无法执行** → 通过 Dashboard 查看 Prometheus Pod 状态
> 2. 验证 Grafana Running：`kubectl get pods -n monitoring | grep grafana`
> 3. 验证 Alertmanager Running：`kubectl get pods -n monitoring | grep alertmanager`
> 4. 验证 target 状态：`kubectl exec <prometheus-pod> -n monitoring -- wget -qO- http://localhost:9090/api/v1/targets | jq '.data.activeTargets | length'`
>    **如果无法 exec** → port-forward 后在浏览器访问 /targets
> 5. 验证告警规则无错误：`kubectl logs <prometheus-pod> -n monitoring --tail=20 | grep -i "rule evaluation"
>    **如果无错误日志** → 规则评估正常
> 6. 验证 Alertmanager 通知：发送测试告警，确认收到通知
> 7. 验证 Grafana 面板有数据：在 Grafana UI 中查看关键面板
> 请告诉我以上验证结果。如果全部通过，问题已修复。

---


### 分支 1.4：阿里云ACK/专有云监控告警排查

工程师："我们在阿里云ACK/专有云环境，监控告警有问题"

顾问："阿里云环境有额外的监控维度，请按以下顺序排查：

**步骤 1：阿里云ARMS/云监控检查**
```bash
# 检查是否接入阿里云ARMS
kubectl get pods -n kube-system | grep arms

# 检查云监控插件
kubectl get pods -n kube-system | grep cloud-monitor

# 检查ARMS应用监控
aliyun arms SearchTraceAppByName --AppName <app>
```

> **如果无法执行aliyun CLI**：请登录ARMS控制台，告诉我：
> 1. 应用是否已接入ARMS？
> 2. 告警规则是否配置正确？
> 3. 告警通知渠道是否正常？

**步骤 2：ACK监控组件检查**
```bash
# 检查Prometheus状态
kubectl get pods -n monitoring

# 检查Grafana状态
kubectl get svc -n monitoring grafana

# 检查告警管理器
kubectl get pods -n monitoring alertmanager

# 检查metrics-server
kubectl top nodes
```

**步骤 3：专有云监控特殊考虑**
- 专有云可能未接入ARMS，使用自建Prometheus
- 检查天基监控告警配置
- 确认ASO控制台告警状态
- 检查飞天组件监控数据

**步骤 4：阿里云特定修复**

如ARMS探针异常：
```bash
# 重启ARMS探针
kubectl delete pod -n kube-system -l app=arms-pilot

# 检查ARMS配置
kubectl get configmap arms-config -n kube-system

# 重新接入ARMS
aliyun armsx QueryAppMetadata --AppName <app>
```

如自建Prometheus异常：
1. 检查Prometheus存储空间
2. 检查Target状态
3. 检查Rule配置
4. 检查Alertmanager通知配置

**阿里云控制台路径**：
- ARMS控制台：阿里云首页 → 应用实时监控服务ARMS
- 云监控控制台：阿里云首页 → 云监控
- ACK监控：ACK控制台 → 集群详情 → 监控


## 升级决策点

| 条件 | 升级路径 | 说明 |
|------|---------|------|
| Prometheus 数据损坏 | **存储专家** | 需要 TSDB 数据恢复 |
| 监控基础设施被入侵 | **安全团队 SKILL-SEC-003** | 安全事件响应 |
| CNI/网络策略复杂场景 | **SKILL-NET-003** | 网络深度诊断 |
| 高基数指标无法定位 | **可观测性架构师** | 指标设计优化 |
| TLS/证书问题 | **SKILL-CERT-001** | 证书管理 |
| Prometheus Operator Bug | **Operator 维护团队** | 升级或降级 Operator |

**顾问升级话术**：
> 根据目前排查结果，这个问题超出了常规监控问题处理范围，可能涉及 **{具体原因}**。建议：
>
> 1. **立即止损**：临时通过其他渠道（如直接 kubectl top）获取关键指标
> 2. **升级诊断**：我会整理当前收集的所有信息，你可以提交给 **{升级目标团队}**
> 3. **持续监控**：继续观察 Prometheus 内存和存储指标
>
> 是否需要我帮你整理排查结果摘要？

---

## 附录：常用命令速查

```bash
# Prometheus 状态
kubectl get pods -n monitoring -l app=prometheus
kubectl logs <prometheus-pod> -n monitoring --tail=50
kubectl exec <prometheus-pod> -n monitoring -- df -h /prometheus

# Target 状态
kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | select(.health!="up")'

# Grafana 数据源
kubectl get configmap -n monitoring | grep datasource
kubectl rollout restart deployment grafana -n monitoring

# Alertmanager 配置
kubectl get secret alertmanager-<name> -n monitoring -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d

# ServiceMonitor
kubectl get servicemonitor --all-namespaces
kubectl patch servicemonitor <name> -n <namespace> --type='json' -p='[{"op": "replace", "path": "/spec/selector/matchLabels", "value": {"app":"<label>"}}]'

# 告警规则
kubectl get prometheusrules --all-namespaces
kubectl patch prometheusrules <name> -n <namespace> --type='json' -p='[{"op": "replace", "path": "/spec/groups/0/rules/0/expr", "value": "<promql>"}]'
```

---

*对话脚本版本: 1.0.0 | 技能: K8s Monitoring & Alerting Failure 诊断与修复 | 模式: L2-semi-auto*
## Related

- [[entities/cilium|Cilium (entities)]]
- [[domain-17-system-foundation/topic-dictionary/workloads/pods|Pods]]
