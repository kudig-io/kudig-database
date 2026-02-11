# Kubernetes 故障排查方法论全栈培训

> **适用版本**: 所有 Kubernetes 版本 | **文档类型**: 实战排障专项
> **目标受众**: SRE、架构师、高级运维
> **核心原则**: 分层排查、证据驱动、快速止损

---

## 🔰 第一阶段：排障心法与标准工具

### 1.1 排障五步走
1.  **确认现象**: 谁坏了？坏到什么程度？
2.  **信息收集**: Logs, Describe, Events, Metrics。
3.  **假设与验证**: 是网络问题吗？还是存储？
4.  **快速止损**: 切流、重启、扩容。
5.  **根因分析 (RCA)**: 防止再次发生。

### 1.2 瑞士军刀 (Command Line)
*   `kubectl get events -w`: 实时监控集群事件。
*   `kubectl debug`: 注入临时容器进行排障。
*   `nsenter`: 进入容器网络/进程命名空间。

---

## 📘 第二阶段：分层排障模型

### 2.1 Pod 层 (常见问题)
*   **Pending**: 调度失败（资源不足、亲和性冲突）。
*   **ImagePullBackOff**: 镜像拉不动（凭证错、地址错、网络墙）。
*   **CrashLoopBackOff**: 程序启动就挂（代码 Bug、环境变量缺失）。

### 2.2 网络层 (复杂问题)
*   **DNS 解析失败**: 检查 CoreDNS。
*   **Service 不通**: 检查 Kube-proxy 规则、EndpointSlice。
*   **网络策略拦截**: 检查 NetworkPolicy。

---

## ⚡ 第三阶段：Node 与 Control Plane 排障

### 3.1 Node 级故障
*   **NotReady**: 检查 kubelet 日志、PIDs 耗尽、磁盘空间满。
*   **CPU Throttle**: 资源 Limit 设置过低导致被限流。

### 3.2 脑裂与 API Server 压力
*   **etcd 响应慢**: 导致整个集群变更变慢。

---

## 🛠️ 第四阶段：应急响应 (SOP)

### 4.1 快速恢复四板斧
1.  **重启**: `kubectl delete pod` (利用自愈机制)。
2.  **扩容**: 增加副本数分摊压力。
3.  **回滚**: `kubectl rollout undo`。
4.  **降级**: 切断非核心功能流量。

### 🏆 SRE 运维红线
*   *红线 1: 严禁在未保留现场（抓包/快照）的情况下盲目重启所有组件。*
*   *红线 2: 任何排障操作必须记录在案。*
*   *红线 3: 核心链路组件（DNS/Ingress）必须有备用紧急方案。*
