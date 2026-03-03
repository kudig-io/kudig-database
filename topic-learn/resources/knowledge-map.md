# 知识图谱模板

使用此模板记录你的学习成果，构建个人知识图谱。

---

## Week 1: 地基建设期

### Docker

**核心概念:**
- [ ] Docker Engine 架构
- [ ] 镜像 vs 容器
- [ ] Union Filesystem
- [ ] 网络模式 (bridge/host/overlay)
- [ ] 存储 (Volume/Bind Mount)

**我的理解:**
```
(用自己的语言总结)

```

**还需加强:**
```

```

---

### Linux

**核心概念:**
- [ ] namespace (7种类型)
- [ ] cgroup (资源限制)
- [ ] 进程管理
- [ ] 网络配置 (ip/iptables)

**我的理解:**
```

```

**还需加强:**
```

```

---

### K8s 架构

**核心概念:**
- [ ] 控制平面: etcd, API Server, Scheduler, Controller Manager
- [ ] 数据平面: kubelet, kube-proxy, Container Runtime
- [ ] 声明式管理
- [ ] 控制器模式

**架构图:**
```
(画出你理解的架构图)




```

**我的理解:**
```

```

---

## Week 2: 核心技术构建期

### 控制平面

**核心概念:**
- [ ] etcd Raft 协议
- [ ] API Server 请求链
- [ ] Scheduler Filter/Score
- [ ] Controller Reconcile 循环

**我的理解:**
```

```

---

### 工作负载

**核心概念:**
- [ ] Deployment 滚动更新
- [ ] StatefulSet 有序部署
- [ ] DaemonSet 每节点运行
- [ ] Pod 生命周期和探针
- [ ] 资源管理 (QoS)
- [ ] HPA 自动扩缩容

**对比总结:**
| 类型 | 特点 | 使用场景 |
|------|------|----------|
| Deployment | | |
| StatefulSet | | |
| DaemonSet | | |

---

### 网络

**核心概念:**
- [ ] K8s 网络模型
- [ ] CNI 插件机制
- [ ] Service 四种类型
- [ ] CoreDNS 服务发现
- [ ] Ingress 路由
- [ ] NetworkPolicy

**网络流程图:**
```
(画出请求从外部到 Pod 的流程)




```

---

### 存储

**核心概念:**
- [ ] PV/PVC 绑定
- [ ] StorageClass 动态供应
- [ ] 访问模式 (RWO/ROX/RWX)
- [ ] Reclaim Policy

---

## Week 3: 运维作战能力期

### 安全

**核心概念:**
- [ ] RBAC 四种资源
- [ ] ServiceAccount
- [ ] Pod Security Standards
- [ ] Secret 管理

**RBAC 权限设计:**
```

```

---

### 可观测性

**核心概念:**
- [ ] Metrics/Logs/Traces 三支柱
- [ ] Prometheus 数据模型
- [ ] PromQL 查询
- [ ] Alertmanager 路由
- [ ] Loki 日志查询

**监控架构图:**
```




```

---

### 故障排查

**核心概念:**
- [ ] FTA 故障树分析
- [ ] FEBM 取证循证
- [ ] 结构化排障流程

**常见故障速查:**
| 现象 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pending | | |
| CrashLoopBackOff | | |
| OOMKilled | | |
| ImagePullBackOff | | |

---

## Week 4: 企业级进阶期

### GitOps

**核心概念:**
- [ ] GitOps 原则
- [ ] ArgoCD 工作流
- [ ] 多环境管理
- [ ] Kustomize/Helm

---

### 生产运维

**核心概念:**
- [ ] SLO/SLI 体系
- [ ] 变更管理
- [ ] 事故响应
- [ ] 容量规划

---

## 总结

### 最有价值的知识点

1. 
2. 
3. 

### 仍需深入的领域

1. 
2. 
3. 

### 下一步学习计划

1. 
2. 
3. 
