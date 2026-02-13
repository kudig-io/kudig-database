# Kubernetes 安全与 RBAC 权限管理全栈培训

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 安全治理专项
> **目标受众**: 安全工程师、SRE、系统管理员
> **核心原则**: 最小权限原则、零信任架构、多层防御

---

## 🔰 第一阶段：安全基础与 4C 模型

### 1.1 安全 4C 模型
*   **Cloud**: 云基础设施安全。
*   **Cluster**: Kubernetes 集群安全。
*   **Container**: 镜像与运行时安全。
*   **Code**: 代码与应用安全。

### 1.2 认证与鉴权 (AuthN & AuthZ)
*   **谁在访问？** (Authentication): 证书、Token、OIDC。
*   **能做什么？** (Authorization): **RBAC** (基于角色的访问控制)。

---

## 📘 第二阶段：RBAC 深度解析

### 2.1 四个核心对象
1.  **Role**: 定义在 Namespace 内的权限（“能读 Pod”）。
2.  **ClusterRole**: 定义在集群维度的权限（“能管 Node”）。
3.  **RoleBinding**: 将 Role 绑定到主体。
4.  **ClusterRoleBinding**: 将 ClusterRole 绑定到主体。

### 2.2 主体 (Subject)
*   **User**: 外部人员。
*   **Group**: 用户组。
*   **ServiceAccount (SA)**: Pod 内进程访问 API 的身份。

---

## ⚡ 第三阶段：生产级安全加固

### 3.1 准入控制 (Admission Control)
*   **MutatingWebhook**: 修改请求（如自动注入 Sidecar）。
*   **ValidatingWebhook**: 校验请求（如禁止使用 `latest` 标签）。

### 3.2 网络安全与策略
*   **NetworkPolicy**: Pod 级别的防火墙，默认应开启“全拒绝”并按需放行。

---

## 🛠️ 第四阶段：审计与应急响应

### 4.1 审计日志 (Audit Log)
*   记录“谁在什么时间对什么资源做了什么操作”。
*   **SRE 建议**: 生产环境必须开启并持久化审计日志以备溯源。

### 🏆 SRE 运维红线
*   *红线 1: 严禁为 ServiceAccount 赋予 `cluster-admin` 权限。*
*   *红线 2: 严禁将 apiserver 暴露在公网且不加 IP 白名单。*
*   *红线 3: 定期清理无用的 RBAC 绑定。*
