# Week 3 自测: 节点与工作负载管理

> **满分**: 50 分 | **建议用时**: 60 分钟

---

## 一、概念理解 (5 题, 每题 2 分, 共 10 分)

1. **ACK 节点的三种状态分别是什么？节点处于 `NotReady` 状态时，调度器会如何处理？**

   > 你的回答:

2. **解释 Taint 和 Toleration 的协作关系。一个节点设置了 `gpu=true:NoSchedule`，什么样的 Pod 可以调度上去？**

   > 你的回答:

3. **托管节点池和自管理节点池有什么区别？各适用于什么场景？**

   > 你的回答:

4. **Pod 的 livenessProbe 和 readinessProbe 失败后的行为有什么不同？**

   > 你的回答:

5. **ACK 托管版集群中，哪些组件由阿里云维护，哪些需要用户自行运维？**

   > 你的回答:

---

## 二、命令实操 (5 题, 每题 2 分, 共 10 分)

1. **写出将节点 `node-1` 标记为不可调度并驱逐 Pod 的命令序列:**

   ```bash
   # 你的命令:
   ```

2. **写出查看节点池列表和指定节点池详情的 aliyun CLI 命令:**

   ```bash
   # 你的命令:
   ```

3. **写出创建带有 nodeSelector 调度约束的 Pod YAML (调度到 `env=production` 的节点):**

   ```yaml
   # 你的 YAML:
   ```

4. **写出检查 CoreDNS 运行状态并测试 DNS 解析的命令:**

   ```bash
   # 你的命令:
   ```

5. **写出为 Pod 配置 resources.requests 和 resources.limits 的 YAML 片段:**

   ```yaml
   # 你的 YAML:
   ```

---

## 三、场景分析 (4 题, 每题 5 分, 共 20 分)

### 场景 1: 节点池扩容失败

**现象**: 配置了 Cluster Autoscaler，Pod 一直处于 Pending 状态，但节点池没有自动扩容。

**分析步骤** (请写出排查思路和相关命令):

> 你的分析:

**参考方向**:
- 检查 Cluster Autoscaler Pod 日志
- 确认节点池是否启用了自动伸缩
- 检查节点池 max_instances 是否已达上限
- 检查 ECS 库存和配额

---

### 场景 2: Pod 反复 CrashLoopBackOff

**现象**: Deployment 创建后，Pod 反复重启，状态显示 CrashLoopBackOff。

**分析步骤**:

> 你的分析:

**参考方向**:
- `kubectl describe pod` 查看 Events 和 Last State
- `kubectl logs <pod> --previous` 查看上次退出日志
- 检查 livenessProbe 配置是否合理
- 检查容器资源限制是否过低导致 OOM

---

### 场景 3: 节点资源不足

**现象**: 新 Pod 无法调度，报 `Insufficient cpu` 或 `Insufficient memory`。

**分析步骤**:

> 你的分析:

**参考方向**:
- `kubectl describe node` 查看 Allocated resources
- 检查是否有 Pod 设置了过大的 requests
- 检查 ResourceQuota 和 LimitRange
- 考虑扩容节点池或优化资源配置

---

### 场景 4: kube-system 组件异常

**现象**: 集群内 Pod 之间无法通过 Service 名称访问，但 IP 直连正常。

**分析步骤**:

> 你的分析:

**参考方向**:
- DNS 解析问题: 检查 CoreDNS 状态和配置
- kube-proxy 问题: 检查 iptables/ipvs 规则
- CNI 插件问题: 检查 Terway/Flannel DaemonSet 状态
- 分层排查: DNS → kube-proxy → CNI

---

## 四、评分统计

| 部分 | 满分 | 得分 |
|------|------|------|
| 概念理解 | 10 | |
| 命令实操 | 10 | |
| 场景分析 | 20 | |
| **自评加分** | 10 | |
| **合计** | **50** | |

**自评加分标准** (最高 10 分):
- 本周每日教案按时完成 +2
- 独立排查了节点/Pod 问题 +3
- 实践了多节点池架构设计 +3
- 整理了组件运维手册 +2

---

## 五、薄弱点记录

| 薄弱点 | 对应 Day | 补强计划 |
|--------|---------|---------|
| | | |
| | | |
| | | |

---

## 下周计划调整

基于本周自测结果，调整 Week 4 学习重点:

- [ ] 需要加强: ___
- [ ] 可以快速过: ___
- [ ] 特别关注: ___
