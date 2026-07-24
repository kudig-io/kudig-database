---
title: 日志流水线问题 — 远程顾问对话脚本
summary: 日志流水线问题的远程顾问对话脚本，覆盖Fluentd/Fluent-bit、日志丢失、解析错误。
category: troubleshooting
tags:
- observability
- remote-consultant
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
dialogue_id: DIALOGUE-SKILL-LOG-001
skill_id: SKILL-LOG-001
version: 1.0.0
role: remote-consultant
language: zh
relationships:
- target: '[[技能/节点/node/skill-notready/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[实体/deployment.md]]'
  type: uses
- target: '[[实体/kubelet.md]]'
  type: uses
- target: '[[实体/kubernetes.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# K8s Logging Pipeline Failure 诊断 — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**，只能通过对话指导现场工程师执行操作。
> 覆盖范围：Fluentd/Fluent Bit 日志代理异常、后端存储（ES/Loki）不可用、日志解析错误、日志丢弃、日志延迟增大等日志管道问题。

---

## 对话入口

### 入口 A：工程师明确报告日志收集中断

**工程师**：「Kibana/Grafana 查不到日志了」/「日志查询返回空」/「Fluentd Pod 挂了」

**顾问回应**：
> 收到，日志收集中断会让我们失去问题排查的关键线索。作为远程顾问，我无法直连你的集群，请你配合执行检查命令。
>
> 先回答三个问题（30 秒内）：
> 1. **影响范围**：是所有命名空间都没有日志，还是只有部分应用？
> 2. **紧急程度**：是否有正在发生的问题需要排查日志？合规审计日志是否受影响？
> 3. **发生时间**：问题是突然发生还是逐渐恶化（日志量先减少后归零）？最近是否有日志相关变更？

---

### 入口 B：工程师报告日志延迟或丢失

**工程师**：「日志有但是是半小时前的」/「日志量突然变少了」/「只有部分 Pod 的日志」

**顾问回应**：
> 日志延迟或部分丢失通常是日志代理缓冲溢出或后端存储压力导致的。先确认日志管道各节点状态。
>
> 请执行以下命令检查日志代理状态：
> ```bash
> kubectl get pods -n logging
> ```
> **如果不知道 logging namespace** → `kubectl get pods --all-namespaces | grep -E "fluent|filebeat|logstash"`
> **如果 grep 无结果** → `kubectl get pods --all-namespaces | grep -E "elastic|loki|log"`
> 请把 Pod 状态、重启次数贴给我。

---

### 入口 C：工程师报告后端存储异常

**工程师**：「Elasticsearch 集群健康状态是 red」/「Loki 查询报错了」/「Kibana 连不上 ES」

**顾问回应**：
> 后端存储异常是日志管道的核心问题。先确认存储集群状态。
>
> 请执行：
> ```bash
> kubectl get pods -n logging | grep -E "elastic|loki|es-"
> ```
> **如果没有 logging namespace** → `kubectl get namespaces | grep -E "log|elastic|loki|observ"`
> **如果没有任何日志相关 namespace** → 请告诉我你们的日志后端部署在哪个 namespace
> 请把所有日志后端 Pod 的状态列表贴给我。

---

### 入口 D：工程师报告日志解析异常

**工程师**：「日志能查到但是字段不对」/「JSON 日志没解析」/「时间戳格式错误」

**顾问回应**：
> 日志解析错误通常由解析器配置变更引起。先确认日志代理的解析配置。
>
> ```bash
> kubectl get configmap -n logging | grep -E "fluent|parser|config"
> ```
> **如果无 logging namespace** → `kubectl get configmap --all-namespaces | grep -E "fluent|parser"`
> **如果 ConfigMap 太多** → `kubectl get configmap --all-namespaces | grep -i "log"`
> 请把日志相关的 ConfigMap 列表贴给我。

---

## Round 1：快速定位问题层级

> 目标：判断问题发生在 **日志采集层（Fluentd/Fluent Bit）→ 日志传输层（缓冲/网络）→ 后端存储层（ES/Loki）→ 日志查询层（Kibana/Grafana）** 的哪一层。

---

### Round 1 — 分支 A：日志代理 Pod 异常

**工程师反馈**：Fluentd/Fluent Bit Pod 处于 CrashLoopBackOff、Pending 或频繁重启。

**顾问指令**：
> 日志代理自身不健康，无法采集日志。先收集 Pod 状态和日志。
>
> 1. 查看 Pod 详细信息：`kubectl describe pod <fluent-pod-name> -n logging`
>    **如果不知道 Pod 名** → `kubectl get pods -n logging -o wide | grep -E "fluent|filebeat"`
> 2. 查看日志（已崩溃加 `--previous`）：`kubectl logs <fluent-pod-name> -n logging --tail=50`
>    **如果 logs 为空** → `kubectl logs <fluent-pod-name> -n logging --previous --tail=50`
>    **如果 previous 也拿不到** → `kubectl get events -n logging --field-selector reason=BackOff | tail -20`
> 3. 检查 DaemonSet 调度状态：`kubectl get daemonset -n logging`
>    **如果无法执行** → `kubectl get ds -n logging`
>    **如果无 ds** → `kubectl get [[实体/deployment.md|deployment]] -n logging | grep -E "fluent|filebeat"`
> 请把 Events、日志和 DaemonSet 状态贴给我。

**分支决策**：
- **A1**：OOMKilled 或资源不足 → Round 2 — 分支 A（代理资源修复）
- **A2**：配置加载错误（parser/filter 语法错误）→ Round 2 — 分支 B（配置修复）
- **A3**：Pod 正常但日志未输出 → Round 2 — 分支 C（采集路径排查）

---

### Round 1 — 分支 B：后端存储异常

**工程师反馈**：ES/Loki Pod Running，但健康状态异常或查询报错。

**顾问指令**：
> 后端存储异常会导致日志无法索引或查询。先确认存储集群健康状态。
>
> 1. 检查 ES 集群健康（如使用 ES）：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cluster/health`
>    **如果无法 exec** → `kubectl port-forward -n logging svc/elasticsearch 9200:9200 & && curl -s http://localhost:9200/_cluster/health`
>    **如果 ES 有认证** → `kubectl exec <es-pod> -n logging -- curl -s -u user:pass http://localhost:9200/_cluster/health`
> 2. 检查 Loki 状态（如使用 Loki）：`kubectl exec <loki-pod> -n logging -- wget -qO- http://localhost:3100/ready`
>    **如果无法 exec** → `kubectl logs <loki-pod> -n logging --tail=30 | grep -i "ready|healthy|memberlist"`
> 3. 检查存储 PVC：`kubectl get pvc -n logging | grep -E "elastic|loki|es-"
>    **如果无法执行** → `kubectl get pv | grep -E "elastic|loki"`
> 请把集群健康状态、PVC 使用情况贴给我。

**分支决策**：
- **B1**：ES 集群状态 red / Loki 未就绪 → Round 2 — 分支 D（后端存储修复）
- **B2**：ES 状态 yellow / Loki 部分就绪 → Round 2 — 分支 E（分片/副本修复）
- **B3**：存储健康但磁盘满 → Round 2 — 分支 F（存储扩容/清理）

---

### Round 1 — 分支 C：日志延迟增大但组件正常

**工程师反馈**：日志代理和 ES/Loki 都 Running，但日志延迟从几秒变成几分钟。

**顾问指令**：
> 组件都正常但延迟增大，通常是缓冲区满、网络拥塞或后端写入性能下降。
>
> 1. 检查代理日志中的缓冲状态：`kubectl logs <fluent-pod> -n logging --tail=100 | grep -i "buffer|drop|retry|backpressure"`
>    **如果无法执行** → `kubectl logs <fluent-pod> -n logging --tail=50`
> 2. 检查代理资源使用：`kubectl top pod <fluent-pod> -n logging`
>    **如果 metrics-server 不可用** → `kubectl describe node <node-name> | grep -A 10 "Allocated resources"`
>    **如果无法 describe node** → `kubectl get pod <fluent-pod> -n logging -o yaml | grep -A 5 resources`
> 3. 检查后端写入性能（ES）：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_nodes/stats/thread_pool/write | grep -A 5 "rejected|queue"`
>    **如果无法执行** → `kubectl logs <es-pod> -n logging --tail=30 | grep -i "rejected|too_many_requests"`
> 请把日志中的 buffer/drop 信息、资源使用情况、后端写入状态贴给我。

**分支决策**：
- **C1**：缓冲区满或大量 retry → Round 2 — 分支 G（缓冲/重试调优）
- **C2**：后端写入 reject 或 queue 满 → Round 2 — 分支 H（后端性能优化）
- **C3**：资源使用正常，无明显错误 → Round 2 — 分支 I（网络/调度排查）

---

## Round 2：分层深入诊断

> 目标：根据 Round 1 确定的问题层级，执行针对性的深度检查和修复。

---

### Round 2 — 分支 A：日志代理资源修复

**顾问指令**：
> 日志代理因内存不足或 CPU 限制被杀死。需要扩容。
>
> 1. 检查当前资源限制：`kubectl get daemonset fluent-bit -n logging -o yaml | grep -A 10 resources`
>    **如果无法执行** → `kubectl describe ds fluent-bit -n logging | grep -A 10 "Limits|Requests"`
>    **如果是 Deployment** → `kubectl get deployment <fluent-deploy> -n logging -o yaml | grep -A 10 resources`
> 2. 检查资源使用量：`kubectl top pod -n logging -l app=fluent-bit`
>    **如果 metrics-server 不可用** → `kubectl logs <fluent-pod> -n logging --previous | grep -i "out of memory|oom"`
> 3. 检查节点资源压力：`kubectl describe node <node-name> | grep -A 10 "Allocated resources"`
>    **如果无法执行** → `kubectl get node <node-name> -o yaml | grep -A 10 allocated`
> 请告诉我当前资源限制、实际使用量、节点是否资源紧张。

**修复方案**：
> - **方案 A（增加内存 limit）**：`kubectl patch ds fluent-bit -n logging --type='json' -p='[{"op": "add", "path": "/spec/template/spec/containers/0/resources", "value": {"limits":{"memory":"512Mi","cpu":"500m"},"requests":{"memory":"128Mi","cpu":"100m"}}}]'`
>   **如果无法 patch** → `kubectl edit ds fluent-bit -n logging`（手动修改）
>   **如果没有交互终端** → `kubectl set resources ds fluent-bit -n logging --limits=memory=512Mi,cpu=500m --requests=memory=128Mi,cpu=100m`
> - **方案 B（减少节点负载）**：如果节点整体资源紧张，考虑驱逐低优先级 Pod 或扩容节点

**分支决策**：
- **A1**：扩容后代理稳定 → 进入验证阶段
- **A2**：节点整体资源不足 → Round 3 — 分支 J（节点扩容/调度优化）
- **A3**：扩容后仍 OOM → Round 3 — 分支 K（高日志量优化）

---

### Round 2 — 分支 B：日志代理配置修复

**顾问指令**：
> 日志代理因配置错误无法启动。需要修复 parser/filter/output 配置。
>
> 1. 查看具体错误：`kubectl logs <fluent-pod> -n logging --previous --tail=50 | grep -i "error|failed|invalid|parser"`
>    **如果无法执行** → `kubectl get events -n logging --field-selector reason=Failed | tail -20`
> 2. 检查 ConfigMap：`kubectl get configmap -n logging | grep fluent`
>    **如果无结果** → `kubectl get configmap --all-namespaces | grep -E "fluent|parser"`
> 3. 查看配置内容：`kubectl get configmap <fluent-config> -n logging -o yaml | grep -A 20 "fluent-bit.conf|parsers.conf"`
>    **如果无法执行** → `kubectl get configmap <fluent-config> -n logging -o yaml`
> 请把错误日志和配置内容贴给我。

**修复方案**：
> 1. 备份当前配置：`kubectl get configmap <fluent-config> -n logging -o yaml > /tmp/fluent-config-backup.yaml`
>    **如果无法写入 /tmp** → `kubectl get configmap <fluent-config> -n logging -o yaml`（手动保存）
> 2. 修正 parser 配置：
> ```bash
> kubectl patch configmap fluent-bit-config -n logging --type='json' -p='
> [{"op": "replace", "path": "/data/parsers.conf", "value":
>   "[PARSER]\n    Name   json\n    Format json\n    Time_Key time\n    Time_Format %Y-%m-%dT%H:%M:%S.%L\n"}]'
> ```
> 3. 重启代理：`kubectl rollout restart ds fluent-bit -n logging`
>    **如果无法执行** → `kubectl delete pod -n logging -l app=fluent-bit`

**分支决策**：
- **B1**：配置修复后代理恢复 → 进入验证阶段
- **B2**：配置反复出错 → Round 3 — 分支 L（配置深度审查）
- **B3**：无法确定配置错误位置 → 升级决策点

---

### Round 2 — 分支 C：采集路径排查

**顾问指令**：
> 代理 Pod 正常但日志未采集，可能是节点日志路径变更或权限问题。
>
> 1. 检查节点日志路径：`kubectl exec <fluent-pod> -n logging -- ls -la /var/log/containers/ | head -10`
>    **如果无法 exec** → `kubectl debug node/<node-name> -it --image=busybox -- ls -la /host/var/log/containers/ | head -10`
>    **如果无法 debug node** → `kubectl run node-test --image=busybox --rm -it --restart=Never --overrides='{"spec":{"nodeSelector":{"[[实体/kubernetes.md|kubernetes]].io/hostname":"<node-name>"},"hostNetwork":true}}' -- ls -la /var/log/containers/ | head -10`
> 2. 检查符号链接：`kubectl exec <fluent-pod> -n logging -- ls -la /var/log/containers/<pod-name>_<namespace>_<container>*.log`
>    **如果链接断裂** → 检查 containerd/docker 日志配置
> 3. 检查 Pod 是否有读取权限：`kubectl exec <fluent-pod> -n logging -- id`
>    **如果无 root** → 检查 securityContext 和 volume 挂载权限
> 请告诉我日志文件是否存在、符号链接是否有效、Pod 的运行用户。

**分支决策**：
- **C1**：日志文件不存在或链接断裂 → Round 3 — 分支 M（节点日志修复）
- **C2**：权限不足（Permission denied）→ Round 3 — 分支 N（权限修复）
- **C3**：日志路径正常但代理未读取 → Round 3 — 分支 O（输入配置排查）

---

### Round 2 — 分支 D：后端存储修复（ES red / Loki 未就绪）

**顾问指令**：
> 后端存储集群不可用，需要恢复存储服务。
>
> 1. 查看 ES 具体错误：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cluster/health?level=indices | jq '.indices | with_entries(select(.value.status == "red"))'`
>    **如果无法执行** → `kubectl logs <es-pod> -n logging --tail=50 | grep -i "error|failed|shard"`
> 2. 查看 Loki 错误：`kubectl logs <loki-pod> -n logging --tail=50 | grep -i "error|failed|wal"`
>    **如果无错误** → `kubectl describe pod <loki-pod> -n logging | grep -A 20 Events`
> 3. 检查存储 PVC 状态：`kubectl get pvc -n logging`
>    **如果无法执行** → `kubectl get pv`
> 请把 ES red 索引、Loki 错误日志、PVC 状态贴给我。

**修复方案**：
> - **ES 方案 A（重新路由分片）**：`kubectl exec <es-pod> -n logging -- curl -X POST http://localhost:9200/_cluster/reroute?retry_failed=true`
> - **ES 方案 B（删除损坏索引）**：`kubectl exec <es-pod> -n logging -- curl -X DELETE http://localhost:9200/<corrupted-index>`
>   **注意**：删除前确认数据可丢弃或已备份
> - **Loki 方案（重启 ingester）**：`kubectl rollout restart statefulset loki-ingester -n logging`
>   **注意**：重启会丢失未刷新的内存数据

**分支决策**：
- **D1**：重新路由或重启后恢复 → 进入验证阶段
- **D2**：数据损坏严重 → 升级决策点（数据恢复专家）
- **D3**：存储 PVC 无法扩容 → Round 3 — 分支 P（存储扩容）

---

### Round 2 — 分支 E：分片/副本修复

**顾问指令**：
> ES 状态 yellow 表示副本分片未分配，需要处理。
>
> 1. 查看未分配分片原因：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cluster/allocation/explain | jq .`
>    **如果无法执行** → `kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/shards?v | grep UNASSIGNED`
> 2. 检查节点数是否足够：`kubectl get pods -n logging -l app=elasticsearch`
>    **如果节点数 < 副本数+1** → 需要增加 ES 节点
> 3. 检查磁盘水位：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/allocation?v`
> 请告诉我未分配原因、节点数、磁盘使用情况。

**修复方案**：
> - 如果磁盘水位过高：清理旧索引或扩容磁盘
> - 如果节点数不足：扩容 ES StatefulSet
> - 临时减少副本数：`kubectl exec <es-pod> -n logging -- curl -X PUT http://localhost:9200/<index>/_settings -H "Content-Type: application/json" -d '{"index": {"number_of_replicas": 0}}'`

**分支决策**：
- **E1**：修复后状态变 green → 进入验证阶段
- **E2**：持续 yellow 但不影响写入 → 标记为低优先级后续处理
- **E3**：分片分配反复失败 → Round 3 — 分支 Q（深度存储诊断）

---

### Round 2 — 分支 F：存储扩容/清理

**顾问指令**：
> 后端存储磁盘已满，需要扩容或清理。
>
> 1. 检查磁盘使用：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/allocation?v | awk '{print $1, $6, $7, $8}'`
>    **如果无法执行** → `kubectl exec <es-pod> -n logging -- df -h`
> 2. 检查索引大小：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/indices?v | sort -k9 -rn | head -20`
>    **如果无法执行** → `kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/indices?v | head -20`
> 3. 检查 ILM/生命周期策略：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_ilm/policy`
>    **如果无 ILM** → 需要手动管理索引生命周期
> 请告诉我磁盘使用百分比、最大的索引、是否有自动清理策略。

**修复方案**：
> - **方案 A（删除旧索引）**：`kubectl exec <es-pod> -n logging -- curl -X DELETE http://localhost:9200/<old-index>`
>   **注意**：确认索引数据可丢弃
> - **方案 B（扩容 PVC）**：`kubectl patch pvc <es-pvc> -n logging --type='json' -p='[{"op": "replace", "path": "/spec/resources/requests/storage", "value": "500Gi"}]'`
>   **如果无法 patch** → 请管理员通过存储类扩容
> - **方案 C（配置索引生命周期）**：设置自动 rollover 和 delete 策略

**分支决策**：
- **F1**：清理/扩容后恢复 → 进入验证阶段
- **F2**：存储类不支持扩容 → 升级决策点（存储管理员）
- **F3**：索引持续增长 → Round 3 — 分支 R（日志量优化）

---

### Round 2 — 分支 G：缓冲/重试调优

**顾问指令**：
> 日志代理缓冲区满导致日志丢弃。需要调优缓冲和重试策略。
>
> 1. 查看当前缓冲配置：`kubectl get configmap <fluent-config> -n logging -o yaml | grep -A 10 "buffer|flush|retry"`
>    **如果无法执行** → `kubectl get configmap <fluent-config> -n logging -o yaml`
> 2. 查看丢弃统计：`kubectl logs <fluent-pod> -n logging --tail=100 | grep -c "drop|overflow"`
>    **如果无法执行** → `kubectl logs <fluent-pod> -n logging --tail=50`
> 3. 检查后端响应时间：`kubectl logs <fluent-pod> -n logging --tail=50 | grep -i "response|timeout|connect"`
> 请把缓冲配置、丢弃数量、后端响应状态贴给我。

**修复方案**：
> 增加缓冲和重试限制：
> ```bash
> kubectl patch configmap fluent-bit-config -n logging --type='json' -p='
> [{"op": "replace", "path": "/data/fluent-bit.conf", "value":
>   "[OUTPUT]\n    Name  es\n    Match *\n    Host  elasticsearch\n    Port  9200\n    Retry_Limit 10\n    Buffer_Max_Size 10M\n    Flush 5\n"}]'
> ```
> 重启代理：`kubectl rollout restart ds fluent-bit -n logging`

**分支决策**：
- **G1**：调优后延迟降低 → 修复完成
- **G2**：调优后仍丢弃 → Round 3 — 分支 H（后端性能优化）
- **G3**：缓冲配置复杂（多输出）→ Round 3 — 分支 L（配置深度审查）

---

### Round 2 — 分支 H：后端性能优化

**顾问指令**：
> 后端写入性能不足导致日志堆积。需要优化 ES/Loki 性能。
>
> 1. 检查 ES 线程池：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_nodes/stats/thread_pool/write | jq '.nodes[] | {name: .name, rejected: .thread_pool.write.rejected, queue: .thread_pool.write.queue}'`
>    **如果无法执行** → `kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_nodes/stats/thread_pool/write`
> 2. 检查 Loki 摄取速率：`kubectl exec <loki-pod> -n logging -- wget -qO- http://localhost:3100/metrics | grep loki_ingester`
>    **如果无法执行** → `kubectl logs <loki-pod> -n logging --tail=30 | grep -i "ingest|push"`
> 3. 检查索引/分片数量：`kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/indices?v | wc -l`
> 请告诉我写入 reject 数量、摄取速率、索引数量。

**修复方案**：
> - 增加 ES 分片数或刷新间隔
> - 增加 Loki ingester 副本数
> - 使用 bulk 写入优化（调整批量大小）

**分支决策**：
- **H1**：优化后写入恢复 → 修复完成
- **H2**：性能瓶颈在硬件 → Round 3 — 分支 P（存储扩容）
- **H3**：ES 集群需要扩节点 → Round 3 — 分支 J（节点扩容）

---

### Round 2 — 分支 I：网络/调度排查

**顾问指令**：
> 无明显错误但日志延迟，可能是网络拥塞或 Pod 调度问题。
>
> 1. 检查代理到后端的网络：`kubectl exec <fluent-pod> -n logging -- nc -vz elasticsearch 9200`
>    **如果无 nc** → `kubectl exec <fluent-pod> -n logging -- wget -qO- http://elasticsearch:9200 --timeout=5`
>    **如果 wget 也无** → `kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -n logging -- nc -vz elasticsearch 9200`
> 2. 检查 Pod 调度分布：`kubectl get pods -n logging -o wide | grep fluent`
>    **如果都集中在同一节点** → 可能存在节点网络问题
> 3. 检查 NetworkPolicy：`kubectl get networkpolicy --all-namespaces`
>    **如果无 NetworkPolicy CRD** → `kubectl get ciliumnetworkpolicy --all-namespaces 2>/dev/null || echo "无 NetworkPolicy"`
> 请告诉我网络连通性测试结果、Pod 分布、NetworkPolicy 是否存在。

**分支决策**：
- **I1**：NetworkPolicy 阻断了流量 → Round 3 — 分支 S（NetworkPolicy 修复）
- **I2**：网络正常但延迟存在 → Round 3 — 分支 T（追踪深度分析）
- **I3**：Pod 集中在异常节点 → Round 3 — 分支 J（节点调度修复）

---

## Round 3：精确修复与验证

> 目标：执行最终修复动作，验证日志管道恢复正常，决定是否升级。

---

### Round 3 — 分支 J：节点扩容/调度优化

**顾问指令**：
> 节点资源不足或调度不均导致日志代理/后端异常。
>
> 1. 检查节点资源：`kubectl describe node <node-name> | grep -A 10 "Allocated resources"`
>    **如果无法执行** → `kubectl get node <node-name> -o yaml | grep -A 10 allocated`
> 2. 检查节点污点：`kubectl get node <node-name> -o yaml | grep -A 10 taints`
>    **如果无法执行** → `kubectl describe node <node-name> | grep -A 5 Taints`
> 3. 检查 Pod 调度约束：`kubectl get ds fluent-bit -n logging -o yaml | grep -A 10 nodeSelector`
> 请告诉我节点资源使用率、是否有污点、DaemonSet 的调度约束。

**修复方案**：
> - 清理节点非必要 Pod
> - 扩容集群节点
> - 调整 Pod 亲和性/反亲和性

**分支决策**：
- **J1**：调度修复后恢复 → 修复完成
- **J2**：需要扩容节点 → 升级决策点（基础设施团队）
- **J3**：节点硬件问题 → 升级决策点（节点管理团队）

---

### Round 3 — 分支 K：高日志量优化

**顾问指令**：
> 扩容后仍 OOM，日志量过大超出处理能力。
>
> 1. 估算日志量：`kubectl logs <fluent-pod> -n logging --tail=100 | wc -l`
>    **如果无法执行** → `kubectl logs <fluent-pod> -n logging --tail=50 | wc -l`
> 2. 检查是否有日志风暴：`kubectl logs <fluent-pod> -n logging --tail=50 | awk '{print $4}' | sort | uniq -c | sort -rn | head -10`
>    **如果无法执行 awk** → `kubectl logs <fluent-pod> -n logging --tail=100`，把日志贴给我分析
> 3. 检查是否有循环日志：`kubectl logs <fluent-pod> -n logging --tail=50 | grep -i "fluent|log" | wc -l`
> 请告诉我日志量估算、高频来源、是否有循环日志。

**修复方案**：
> - 增加 exclude_path 排除不需要的日志
> - 使用 tail_interval_watch 替代 inotify 减少资源消耗
> - 拆分日志输出到多个后端

**分支决策**：
- **K1**：排除后日志量下降 → 修复完成
- **K2**：日志量合理但代理仍 OOM → 升级版本或更换代理
- **K3**：应用日志量突增 → 联系应用团队排查

---

### Round 3 — 分支 L：配置深度审查

**顾问指令**：
> 配置反复出错，需要逐行审查。
>
> 1. 导出完整配置：`kubectl get configmap <fluent-config> -n logging -o yaml`
> 2. 分段测试：先注释掉 filter/output，只保留 input，确认基础采集正常
> 3. 逐步启用各段配置，定位错误段落
> 请把完整配置贴给我，我会帮你逐行审查。

**分支决策**：
- **L1**：发现配置错误并修复 → 进入验证阶段
- **L2**：配置语法正确但行为异常 → 升级版本或更换代理
- **L3**：配置过于复杂 → 升级决策点（可观测性架构师）

---

### Round 3 — 分支 M：节点日志修复

**顾问指令**：
> 节点日志文件丢失或符号链接断裂，需要修复。
>
> 1. 检查 containerd 状态：`kubectl run node-debug --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"nodeSelector":{"kubernetes.io/hostname":"<node-name>"},"hostNetwork":true}}' -- systemctl status containerd`
>    **如果无法执行** → `kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- systemctl status containerd`
>    **如果无法 debug** → `kubectl get node <node-name> -o yaml | grep -i condition`
> 2. 检查 [[实体/kubelet.md|kubelet]] 日志配置：`kubectl run node-debug ... -- cat /var/lib/kubelet/config.yaml | grep -A 5 containerLogMaxSize`
> 3. 重启 containerd/kubelet：`systemctl restart containerd && systemctl restart kubelet`
>    **如果无法 SSH** → 联系节点管理员执行
> 请告诉我 containerd 状态、日志配置、重启后是否恢复。

**修复方案**：
> - 重启 containerd 恢复日志链接
> - 调整 kubelet 日志轮转配置
> - 如果日志目录被清理，检查是否有定时任务误删

**分支决策**：
- **M1**：重启后日志文件恢复 → 修复完成
- **M2**：containerd 无法启动 → 升级决策点（节点管理团队）
- **M3**：多节点同时问题 → 批量修复，检查镜像或配置下发

---

### Round 3 — 分支 N：权限修复

**顾问指令**：
> 日志代理无权限读取日志文件或访问后端。
>
> 1. 检查 Pod securityContext：`kubectl get pod <fluent-pod> -n logging -o yaml | grep -A 10 securityContext`
>    **如果无法执行** → `kubectl describe pod <fluent-pod> -n logging | grep -A 10 "Security Context"`
> 2. 检查 RBAC：`kubectl auth can-i list pods --as=system:serviceaccount:logging:fluent-bit`
>    **如果无法执行** → `kubectl get clusterrolebinding | grep fluent`
> 3. 检查 volume 挂载：`kubectl get pod <fluent-pod> -n logging -o yaml | grep -A 10 "volumes:"`
> 请告诉我 securityContext、RBAC 权限、volume 挂载方式。

**修复方案**：
> - 添加 privileged: true 或调整 runAsUser
> - 创建 ServiceAccount 和 ClusterRoleBinding
> - 使用 hostPath 或 emptyDir 调整挂载

**分支决策**：
- **N1**：权限修复后恢复 → 修复完成
- **N2**：RBAC 复杂（多租户）→ 升级决策点（安全团队）
- **N3**：PSP/OPA 限制 → 升级决策点（安全策略团队）

---

### Round 3 — 分支 O：输入配置排查

**顾问指令**：
> 日志路径正常但代理未读取，需要检查输入配置。
>
> 1. 检查 INPUT 配置：`kubectl get configmap <fluent-config> -n logging -o yaml | grep -A 10 "\[INPUT\]"`
>    **如果无法执行** → `kubectl get configmap <fluent-config> -n logging -o yaml`
> 2. 检查 Path 是否匹配：`kubectl get configmap <fluent-config> -n logging -o yaml | grep "Path"`
> 3. 检查 Parser 配置：`kubectl get configmap <fluent-config> -n logging -o yaml | grep -A 5 "\[PARSER\]"`
> 请把 INPUT 配置、Path 值、Parser 配置贴给我。

**修复方案**：
> - 修正 Path 匹配模式：`Path /var/log/containers/*.log`
> - 启用 Multiline 解析：调整 multiline.parser
> - 增加 Mem_Buf_Limit 避免内存溢出

**分支决策**：
- **O1**：配置修正后恢复 → 修复完成
- **O2**：日志格式变更导致解析失败 → Round 3 — 分支 L（配置深度审查）
- **O3**：特殊日志格式（二进制/非文本）→ 升级决策点

---

## 验证修复

**顾问指令**：
> 修复已应用，验证日志管道是否恢复正常。
>
> 1. 验证日志代理 Running：`kubectl get pods -n logging | grep -E "fluent|filebeat"`
>    **如果无法执行** → 通过 Dashboard 查看日志代理 Pod 状态
> 2. 验证后端 Running：`kubectl get pods -n logging | grep -E "elastic|loki"`
> 3. 验证 DaemonSet 完全调度：`kubectl get ds -n logging`
>    **如果无法执行** → `kubectl get daemonset -n logging`
> 4. 验证新日志出现：在 Kibana/Grafana 中查询最近 5 分钟的日志
>    **如果无 UI** → `kubectl exec <fluent-pod> -n logging -- tail -5 /var/log/containers/<test-pod>*.log`
> 5. 验证无解析错误：`kubectl logs <fluent-pod> -n logging --tail=20 | grep -i "parser|error"
>    **如果无错误** → 解析正常
> 6. 验证日志量恢复：对比修复前后日志量趋势
> 请告诉我以上验证结果。如果全部通过，问题已修复。

---


### 分支 1.4：阿里云ACK/专有云日志排查

工程师："我们在阿里云ACK/专有云环境，日志收集有问题"

顾问："阿里云环境有额外的日志管理维度，请按以下顺序排查：

**步骤 1：阿里云SLS日志服务检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查是否使用阿里云SLS
kubectl get pods -n kube-system | grep logtail

# 检查SLS Project和Logstore
aliyun log ListProject

# 检查SLS采集配置
aliyun log GetMachineGroup --project=<project> --machineGroup=<group>
```
> **如果无法执行aliyun CLI**：请登录SLS控制台，告诉我：
> 1. Project和Logstore是否存在？
> 2. 采集配置是否包含目标Pod？
> 3. 是否有日志投递异常告警？

**步骤 2：ACK日志组件检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查Logtail DaemonSet状态
kubectl get ds -n kube-system logtail-ds

# 检查Logtail日志
kubectl logs -n kube-system -l k8s-app=logtail --tail=100

# 检查日志采集配置CRD
kubectl get aliyunlogconfigs -A
```
**步骤 3：专有云日志特殊考虑**
- 专有云可能未接入SLS，使用自建ELK
- 检查ELK集群状态
- 确认飞天日志收集组件
- 检查天基日志查询功能

**步骤 4：阿里云特定修复**

如SLS采集异常：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 重启Logtail
kubectl rollout restart ds/logtail-ds -n kube-system

# 检查Logtail配置
kubectl get configmap logtail-config -n kube-system -o yaml

# 更新采集配置
kubectl apply -f - <<EOF
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: test-config
spec:
  project: <project>
  logstore: <logstore>
  shardCount: 2
  lifeCycle: 30
  logtailConfig:
    inputType: plugin_file
    configName: test-config
    inputDetail:
      plugin:
        inputs:
        - type: file_log
          detail:
            LogPath: /var/log/containers
            FilePattern: '*.log'
EOF
```
如自建ELK异常：
1. 检查Elasticsearch集群健康状态
2. 检查Fluentd/Fluent-bit采集状态
3. 检查Kibana服务可用性


## 升级决策点

| 条件 | 升级路径 | 说明 |
|------|---------|------|
| 后端存储数据损坏 | **存储专家** | 需要 ES/Loki 数据恢复 |
| 日志代理持续崩溃 | **Operator 维护团队** | 可能是 Bug 或根本性配置错误 |
| 多节点同时日志异常 | **基础设施团队** | 底层存储/网络问题 |
| 安全策略限制 | **[[技能/节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|SKILL]]-SEC-003** | RBAC/PSP/OPA 相关 |
| 高日志量无法优化 | **可观测性架构师** | 架构调整 |
| 合规审计日志中断 | **合规团队** | 需要审计追溯 |

**顾问升级话术**：
> 根据目前排查结果，这个问题超出了常规日志问题处理范围，可能涉及 **{具体原因}**。建议：
>
> 1. **立即止损**：临时通过 `kubectl logs` 直接查看关键 Pod 日志
> 2. **升级诊断**：我会整理当前收集的所有信息，你可以提交给 **{升级目标团队}**
> 3. **持续监控**：继续观察日志代理内存和缓冲指标
>
> 是否需要我帮你整理排查结果摘要？

---

## 附录：常用命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 日志代理状态
kubectl get pods -n logging -l app=fluent-bit
kubectl logs <fluent-pod> -n logging --tail=50
kubectl get ds -n logging

# 后端存储状态
kubectl get pods -n logging | grep -E "elastic|loki"
kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cluster/health
kubectl exec <loki-pod> -n logging -- wget -qO- http://localhost:3100/ready

# 配置检查
kubectl get configmap -n logging | grep fluent
kubectl get configmap fluent-bit-config -n logging -o yaml

# 节点日志路径
kubectl run node-debug --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"nodeSelector":{"kubernetes.io/hostname":"<node-name>"},"hostNetwork":true}}' -- ls -la /var/log/containers/

# 网络测试
kubectl exec <fluent-pod> -n logging -- nc -vz elasticsearch 9200

# 索引管理
kubectl exec <es-pod> -n logging -- curl -s http://localhost:9200/_cat/indices?v
kubectl exec <es-pod> -n logging -- curl -X DELETE http://localhost:9200/<old-index>

# 代理重启
kubectl rollout restart ds fluent-bit -n logging
kubectl delete pod -n logging -l app=fluent-bit
```
---

*对话脚本版本: 1.0.0 | 技能: K8s Logging Pipeline Failure 诊断与修复 | 模式: L2-semi-auto*
## Related

- [[实体/cilium.md|Cilium (entities)]]


<!-- risk-assessed -->
