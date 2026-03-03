# Week 4 自测: 网络与存储

> **满分**: 50 分 | **建议用时**: 60 分钟

---

## 一、概念理解 (5 题, 每题 2 分, 共 10 分)

1. **ClusterIP、NodePort、LoadBalancer 三种 Service 类型的区别是什么？在 ACK 中 LoadBalancer 类型会自动创建什么资源？**

   > 你的回答:

2. **Ingress 和 Service LoadBalancer 在功能上有什么区别？什么情况下应该使用 Ingress？**

   > 你的回答:

3. **Terway ENIIP 模式和 Flannel VxLAN 模式的核心区别是什么？各自的优缺点？**

   > 你的回答:

4. **PV 的 Retain 和 Delete 回收策略有什么区别？生产环境推荐哪种？为什么？**

   > 你的回答:

5. **StatefulSet 的 volumeClaimTemplates 和 Deployment 使用 PVC 有什么区别？**

   > 你的回答:

---

## 二、命令实操 (5 题, 每题 2 分, 共 10 分)

1. **写出创建 LoadBalancer Service 并指定为内网 SLB 的 YAML:**

   ```yaml
   # 你的 YAML:
   ```

2. **写出创建基于域名路由的 Ingress 规则 YAML:**

   ```yaml
   # 你的 YAML:
   ```

3. **写出查看节点 Pod CIDR 分配的命令:**

   ```bash
   # 你的命令:
   ```

4. **写出动态创建 20Gi 云盘 PVC 的 YAML:**

   ```yaml
   # 你的 YAML:
   ```

5. **写出扩容 PVC 到 40Gi 的命令:**

   ```bash
   # 你的命令:
   ```

---

## 三、场景分析 (4 题, 每题 5 分, 共 20 分)

### 场景 1: Service 无法访问

**现象**: 创建了 ClusterIP Service，但从其他 Pod 访问时连接超时。

**分析步骤**:

> 你的分析:

**参考方向**:
- 检查 Service selector 是否匹配 Pod 标签
- 检查 Endpoints 是否有后端 Pod
- 检查 Pod 是否处于 Running 状态且通过 readinessProbe
- 检查 kube-proxy 是否正常
- 检查 NetworkPolicy 是否阻止了流量

---

### 场景 2: Ingress 路由不生效

**现象**: 创建了 Ingress 规则，但访问 Ingress IP 返回 404。

**分析步骤**:

> 你的分析:

**参考方向**:
- 确认 Ingress Controller Pod 正常运行
- 检查 IngressClass 是否正确
- 检查 Host 和 Path 配置
- 检查后端 Service 是否正常
- 查看 Ingress Controller 日志

---

### 场景 3: Pod IP 分配失败 (Terway)

**现象**: Terway 集群中新建 Pod 一直处于 ContainerCreating，Events 显示 "failed to allocate IP"。

**分析步骤**:

> 你的分析:

**参考方向**:
- 检查 Pod vSwitch 的可用 IP 数量
- 检查节点 ENI 配额是否用尽
- 查看 Terway Pod 日志
- 确认安全组规则是否正确
- 考虑扩展 Pod vSwitch CIDR

---

### 场景 4: PVC 一直处于 Pending

**现象**: 创建 PVC 后状态一直是 Pending，Pod 无法启动。

**分析步骤**:

> 你的分析:

**参考方向**:
- `kubectl describe pvc` 查看 Events
- 检查 StorageClass 是否存在
- 检查云盘配额和地域可用区限制
- 确认 CSI 插件是否正常运行
- 检查 PVC 的 accessModes 是否与 StorageClass 兼容

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
- 完成了 Service + Ingress 实操 +2
- 实践了 Terway 或 Flannel 排障 +3
- 完成了 PV/PVC 全流程操作 +3

---

## 五、薄弱点记录

| 薄弱点 | 对应 Day | 补强计划 |
|--------|---------|---------|
| | | |
| | | |
| | | |

---

## 培训完成建议

恭喜完成 4 周培训！接下来建议:

1. **完成毕业项目**: [P5: 毕业综合项目](../projects/p5-graduation-project.md)
2. **定期回顾**: 利用 [知识图谱](../resources/knowledge-map.md) 进行周期性复习
3. **持续实践**: 在实际工作中运用所学知识
4. **社区交流**: 参与团队知识分享，教是最好的学
