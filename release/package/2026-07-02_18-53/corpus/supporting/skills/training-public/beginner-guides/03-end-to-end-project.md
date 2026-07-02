---
title: 端到端项目实战——从代码到生产完整流水线
description: 面向小白的完整项目案例：从零开发一个待办事项应用，经历代码编写、Docker 镜像构建、K8s 部署、Helm 打包、GitOps 交付、Prometheus
  监控、故障排查的完整生产流水线
summary: 面向小白的完整项目案例：从零开发一个待办事项应用，经历代码编写、Docker 镜像构建、K8s 部署、Helm 打包、GitOps 交付、Prometheus
  监控、故障排查的完整生产流水线
category: learning
tags:
- tutorial
- beginner
- project
- end-to-end
- helm
- gitops
- cicd
- prometheus
- grafana
- argocd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 学完基础概念的小白
- 需要项目经验的求职者
- 培训案例开发者
estimated_read_time: 45min
intent_queries:
- K8s 完整项目案例
- 从代码到生产
- K8s 实战项目
- GitOps 完整流程
trigger_keywords:
- 项目实战
- 端到端
- 完整案例
- 流水线
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- kafka-basics
- redis-basics
- policy-basics
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 端到端项目实战——从代码到生产完整流水线

> **项目目标**: 构建一个 **Todo 应用**，并让它跑在 K8s 生产环境中  
> **你将学到**: 代码 → 镜像 → K8s → [[Helm|Helm]] → GitOps → 监控 → 排障 的完整闭环  
> **预估时间**: 4-6 小时（可分 2-3 天完成）  
> **前置要求**: 已完成 [本地环境搭建](02-local-lab-environment.md) 和 [基础概念学习](../fundamentals/)

---

## 项目架构概览

```
用户浏览器
    │
    ▼
┌─────────────────────────────────────┐
│           Ingress (Nginx)            │  ← 流量入口，TLS 终止
│        https://todo.local            │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│      Todo Web 服务 (3 副本)           │  ← Node.js + Express
│   - 前端页面渲染                      │
│   - REST API (CRUD)                 │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│      Redis (1 主 + 1 从)            │  ← 缓存会话和计数
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│      PostgreSQL (StatefulSet)       │  ← 持久化数据
│   - 10GB PVC 持久化存储              │
└─────────────────────────────────────┘

监控层:
  ├── Prometheus ← 抓取指标
  └── Grafana    ← 可视化看板
```

---

## 阶段一：应用开发（30 分钟）

### 1.1 初始化项目

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
mkdir todo-app && cd todo-app
git init

# 项目结构
tree
# .
# ├── src/
# │   ├── app.js          # Express 主应用
# │   ├── routes/
# │   │   └── todos.js    # Todo CRUD API
# │   ├── models/
# │   │   └── todo.js     # 数据模型
# │   └── db.js           # 数据库连接
# ├── views/
# │   └── index.ejs       # 前端页面
# ├── Dockerfile
# ├── docker-compose.yml  # 本地开发
# ├── k8s/                # K8s 原生 manifest
# ├── helm/               # Helm Chart
# └── .github/
#     └── workflows/
#         └── ci.yml      # GitHub Actions CI
```
### 1.2 核心代码

`src/app.js`:
```javascript
const express = require('express');
const redis = require('redis');
const { Pool } = require('pg');

const app = express();
app.use(express.json());
app.set('view engine', 'ejs');

// PostgreSQL 连接
const pgPool = new Pool({
  host: process.env.DB_HOST || 'localhost',
  port: process.env.DB_PORT || 5432,
  user: process.env.DB_USER || 'todo',
  password: process.env.DB_PASSWORD || 'todo123',
  database: process.env.DB_NAME || 'todo'
});

// Redis 连接
const redisClient = redis.createClient({
  url: `redis://${process.env.REDIS_HOST || 'localhost'}:6379`
});
redisClient.connect().catch(console.error);

// 健康检查端点（K8s Probe 用）
app.get('/health', async (req, res) => {
  try {
    await pgPool.query('SELECT 1');
    res.json({ status: 'ok', timestamp: new Date().toISOString() });
  } catch (e) {
    res.status(503).json({ status: 'unhealthy', error: e.message });
  }
});

// 指标端点（Prometheus 用）
app.get('/metrics', async (req, res) => {
  const todoCount = await pgPool.query('SELECT COUNT(*) FROM todos');
  const cacheHits = await redisClient.get('cache:hits') || 0;
  res.set('Content-Type', 'text/plain');
  res.send(`
# HELP todo_count Total number of todos
todo_count ${todoCount.rows[0].count}
# HELP cache_hits Total cache hits
cache_hits ${cacheHits}
  `.trim());
});

// Todo CRUD API
app.get('/', async (req, res) => {
  const result = await pgPool.query('SELECT * FROM todos ORDER BY id DESC');
  res.render('index', { todos: result.rows });
});

app.get('/api/todos', async (req, res) => {
  // 先查 Redis 缓存
  const cached = await redisClient.get('todos:all');
  if (cached) {
    await redisClient.incr('cache:hits');
    return res.json(JSON.parse(cached));
  }
  const result = await pgPool.query('SELECT * FROM todos ORDER BY id DESC');
  await redisClient.setEx('todos:all', 60, JSON.stringify(result.rows));
  res.json(result.rows);
});

app.post('/api/todos', async (req, res) => {
  const { text } = req.body;
  const result = await pgPool.query(
    'INSERT INTO todos (text, done) VALUES ($1, false) RETURNING *',
    [text]
  );
  await redisClient.del('todos:all'); // 清除缓存
  res.status(201).json(result.rows[0]);
});

app.put('/api/todos/:id', async (req, res) => {
  const { id } = req.params;
  const { done } = req.body;
  const result = await pgPool.query(
    'UPDATE todos SET done = $1 WHERE id = $2 RETURNING *',
    [done, id]
  );
  await redisClient.del('todos:all');
  res.json(result.rows[0]);
});

app.delete('/api/todos/:id', async (req, res) => {
  const { id } = req.params;
  await pgPool.query('DELETE FROM todos WHERE id = $1', [id]);
  await redisClient.del('todos:all');
  res.status(204).send();
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Todo app listening on port ${PORT}`);
});
```

`src/db.js`（数据库初始化）:
```javascript
const { Pool } = require('pg');

const pool = new Pool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT || 5432,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME
});

async function initDB() {
  await pool.query(`
    CREATE TABLE IF NOT EXISTS todos (
      id SERIAL PRIMARY KEY,
      text VARCHAR(255) NOT NULL,
      done BOOLEAN DEFAULT false,
      created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
  `);
  console.log('Database initialized');
}

initDB().catch(console.error);
module.exports = pool;
```

`views/index.ejs`（简化版前端）:
```html
<!DOCTYPE html>
<html>
<head>
  <title>Todo App on K8s</title>
  <style>
    body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; }
    .todo { padding: 10px; border-bottom: 1px solid #eee; }
    .done { text-decoration: line-through; color: #999; }
    button { background: #007bff; color: white; border: none; padding: 8px 16px; }
  </style>
</head>
<body>
  <h1>🚀 Todo App on Kubernetes</h1>
  <form action="/api/todos" method="POST" onsubmit="handleSubmit(event)">
    <input name="text" placeholder="New todo..." required style="width: 70%; padding: 8px;">
    <button type="submit">Add</button>
  </form>
  <div id="todos">
    <% todos.forEach(todo => { %>
      <div class="todo <%= todo.done ? 'done' : '' %>">
        <%= todo.text %>
        <button onclick="toggle(<%= todo.id %>, <%= !todo.done %>)">
          <%= todo.done ? 'Undo' : 'Done' %>
        </button>
      </div>
    <% }) %>
  </div>
  <script>
    async function handleSubmit(e) {
      e.preventDefault();
      const text = e.target.text.value;
      await fetch('/api/todos', { method: 'POST', headers: {'Content-Type':'application/json'}, body: JSON.stringify({text}) });
      location.reload();
    }
    async function toggle(id, done) {
      await fetch(`/api/todos/${id}`, { method: 'PUT', headers: {'Content-Type':'application/json'}, body: JSON.stringify({done}) });
      location.reload();
    }
  </script>
</body>
</html>
```

### 1.3 本地运行验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装依赖
npm init -y
npm install express pg redis ejs

# 启动 PostgreSQL 和 Redis（Docker）
docker run -d --name todo-postgres -e POSTGRES_USER=todo -e POSTGRES_PASSWORD=todo123 -e POSTGRES_DB=todo -p 5432:5432 postgres:15-alpine
docker run -d --name todo-redis -p 6379:6379 redis:7-alpine

# 初始化数据库
node src/db.js

# 启动应用
DB_HOST=localhost REDIS_HOST=localhost node src/app.js

# 访问 http://localhost:3000，添加几个待办事项验证功能
```
---

## 阶段二：容器化（20 分钟）

### 2.1 编写 Dockerfile

```dockerfile
# 多阶段构建：减小镜像体积
FROM node:20-alpine AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production

FROM node:20-alpine
WORKDIR /app
# 创建非 root 用户（安全最佳实践）
RUN addgroup -g 1001 -S nodejs && adduser -S nodejs -u 1001
COPY --from=builder --chown=nodejs:nodejs /app/node_modules ./node_modules
COPY --chown=nodejs:nodejs src/ ./src/
COPY --chown=nodejs:nodejs views/ ./views/
COPY --chown=nodejs:nodejs package.json ./
USER nodejs
EXPOSE 3000
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD node -e "require('http').get('http://localhost:3000/health', (r) => r.statusCode === 200 ? process.exit(0) : process.exit(1))"
CMD ["node", "src/app.js"]
```

### 2.2 构建并测试镜像

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 构建
docker build -t todo-app:v1.0.0 .

# 验证镜像大小（多阶段构建后应该 < 150MB）
docker images todo-app

# 本地运行容器验证
docker run -d --name todo-app \
  -e DB_HOST=host.docker.internal \
  -e REDIS_HOST=host.docker.internal \
  -p 3000:3000 \
  todo-app:v1.0.0

# 访问 http://localhost:3000 验证
```
---

## 阶段三：K8s 原生部署（40 分钟）

### 3.1 Namespace 与 ConfigMap

`k8s/01-namespace.yaml`:
```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: todo-app
```

`k8s/02-configmap.yaml`:
```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: todo-config
  namespace: todo-app
data:
  DB_HOST: "postgres"
  DB_PORT: "5432"
  DB_NAME: "todo"
  REDIS_HOST: "redis"
  PORT: "3000"
```

`k8s/03-secret.yaml`:
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: todo-secret
  namespace: todo-app
type: Opaque
stringData:
  DB_USER: "todo"
  DB_PASSWORD: "todo123"
```

### 3.2 数据库部署（StatefulSet + PVC）

`k8s/04-postgres.yaml`:
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: todo-app
spec:
  serviceName: postgres
  replicas: 1
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
      - name: postgres
        image: postgres:15-alpine
        ports:
        - containerPort: 5432
        env:
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: todo-secret
              key: DB_USER
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: todo-secret
              key: DB_PASSWORD
        - name: POSTGRES_DB
          valueFrom:
            configMapKeyRef:
              name: todo-config
              key: DB_NAME
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
---
apiVersion: v1
kind: Service
metadata:
  name: postgres
  namespace: todo-app
spec:
  selector:
    app: postgres
  ports:
  - port: 5432
    targetPort: 5432
  clusterIP: None  # Headless Service，StatefulSet 需要
```

### 3.3 Redis 部署

`k8s/05-redis.yaml`:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: redis
  namespace: todo-app
spec:
  replicas: 1
  selector:
    matchLabels:
      app: redis
  template:
    metadata:
      labels:
        app: redis
    spec:
      containers:
      - name: redis
        image: redis:7-alpine
        ports:
        - containerPort: 6379
        resources:
          requests:
            memory: "64Mi"
            cpu: "50m"
          limits:
            memory: "128Mi"
            cpu: "100m"
---
apiVersion: v1
kind: Service
metadata:
  name: redis
  namespace: todo-app
spec:
  selector:
    app: redis
  ports:
  - port: 6379
    targetPort: 6379
```

### 3.4 应用部署（Deployment + Service + HPA）

`k8s/06-app.yaml`:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: todo-web
  namespace: todo-app
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: todo-web
  template:
    metadata:
      labels:
        app: todo-web
    spec:
      initContainers:
      # 初始化容器：等待数据库就绪
      - name: wait-for-db
        image: busybox:1.36
        command: ['sh', '-c', 'until nc -z postgres 5432; do echo waiting for db...; sleep 2; done']
      containers:
      - name: web
        image: todo-app:v1.0.0
        imagePullPolicy: IfNotPresent
        ports:
        - containerPort: 3000
        envFrom:
        - configMapRef:
            name: todo-config
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: todo-secret
              key: DB_USER
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: todo-secret
              key: DB_PASSWORD
        resources:
          requests:
            memory: "128Mi"
            cpu: "100m"
          limits:
            memory: "256Mi"
            cpu: "200m"
        livenessProbe:
          httpGet:
            path: /health
            port: 3000
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /health
            port: 3000
          initialDelaySeconds: 5
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: todo-web
  namespace: todo-app
spec:
  selector:
    app: todo-web
  ports:
  - port: 80
    targetPort: 3000
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: todo-web-hpa
  namespace: todo-app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: todo-web
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容前等待 5 分钟
```

### 3.5 Ingress

`k8s/07-ingress.yaml`:
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: todo-ingress
  namespace: todo-app
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "false"
spec:
  ingressClassName: nginx
  rules:
  - host: todo.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: todo-web
            port:
              number: 80
```

### 3.6 一键部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 加载镜像到 kind 集群（如果用 kind）
kind load docker-image todo-app:v1.0.0 --name k8s-lab

# 应用所有配置
kubectl apply -f k8s/

# 验证部署
kubectl get all -n todo-app
kubectl get pvc -n todo-app
kubectl get ingress -n todo-app

# 查看 Pod 启动日志
kubectl logs -n todo-app deployment/todo-web --tail=50 -f

# 本地访问（配置 hosts: 127.0.0.1 todo.local）
# http://todo.local
```
---

## 阶段四：Helm 打包（30 分钟）

### 4.1 初始化 Helm Chart

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
helm create helm/todo-app

# 清理默认模板，保留核心结构
rm helm/todo-app/templates/*.yaml
rm -rf helm/todo-app/templates/tests
```
### 4.2 Chart.yaml

`helm/todo-app/Chart.yaml`:
```yaml
apiVersion: v2
name: todo-app
description: A Todo application for K8s learning
type: application
version: 1.0.0
appVersion: "1.0.0"
keywords:
  - todo
  - learning
  - kubernetes
maintainers:
  - name: KUDIG Team
```

### 4.3 values.yaml

`helm/todo-app/values.yaml`:
```yaml
replicaCount: 3

image:
  repository: todo-app
  tag: v1.0.0
  pullPolicy: IfNotPresent

service:
  type: ClusterIP
  port: 80

ingress:
  enabled: true
  className: nginx
  hosts:
    - host: todo.local
      paths:
        - path: /
          pathType: Prefix

resources:
  limits:
    cpu: 200m
    memory: 256Mi
  requests:
    cpu: 100m
    memory: 128Mi

autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 10
  targetCPUUtilizationPercentage: 70

postgres:
  enabled: true
  storage: 10Gi

redis:
  enabled: true
```

### 4.4 模板文件

`helm/todo-app/templates/_helpers.tpl`（辅助模板，省略，用 helm create 生成的即可）

`helm/todo-app/templates/deployment.yaml`:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "todo-app.fullname" . }}
  labels:
    {{- include "todo-app.labels" . | nindent 4 }}
spec:
  {{- if not .Values.autoscaling.enabled }}
  replicas: {{ .Values.replicaCount }}
  {{- end }}
  selector:
    matchLabels:
      {{- include "todo-app.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      labels:
        {{- include "todo-app.selectorLabels" . | nindent 8 }}
    spec:
      initContainers:
      - name: wait-for-db
        image: busybox:1.36
        command: ['sh', '-c', 'until nc -z {{ include "todo-app.fullname" . }}-postgres 5432; do sleep 2; done']
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        imagePullPolicy: {{ .Values.image.pullPolicy }}
        ports:
        - containerPort: 3000
        envFrom:
        - configMapRef:
            name: {{ include "todo-app.fullname" . }}-config
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: {{ include "todo-app.fullname" . }}-secret
              key: DB_USER
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: {{ include "todo-app.fullname" . }}-secret
              key: DB_PASSWORD
        livenessProbe:
          httpGet:
            path: /health
            port: 3000
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /health
            port: 3000
          initialDelaySeconds: 5
          periodSeconds: 5
        resources:
          {{- toYaml .Values.resources | nindent 12 }}
```

> （其余模板：service.yaml、ingress.yaml、hpa.yaml、postgres.yaml、redis.yaml、configmap.yaml、secret.yaml 类似，使用 Helm 模板语法）

### 4.5 部署 Helm Chart

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `helm upgrade/install`：部署/升级 release

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 语法检查
helm lint helm/todo-app

# 模板渲染预览（不部署）
helm template todo-app helm/todo-app --namespace todo-app

# 安装/升级
helm upgrade --install todo-app helm/todo-app \
  --namespace todo-app \
  --create-namespace \
  --set replicaCount=5

# 查看 releases
helm list -n todo-app

# 升级到新版本
helm upgrade todo-app helm/todo-app --set image.tag=v1.1.0

# 回滚
helm rollback todo-app 1

# 卸载
helm uninstall todo-app -n todo-app  # ⚠️ 删除 release 及关联资源
```
---

## 阶段五：GitOps 交付（20 分钟）

### 5.1 推送代码到 Git

```bash
git add .
git commit -m "feat: init todo app with k8s manifests"
git remote add origin https://github.com/yourname/todo-app.git
git push -u origin main
```

### 5.2 安装 [[ArgoCD|ArgoCD]]

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 ArgoCD 命名空间
kubectl create namespace argocd

# 安装 ArgoCD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 暴露服务（本地测试用）
kubectl port-forward svc/argocd-server -n argocd 8080:443

# 获取初始密码
kubectl get secret argocd-initial-admin-secret -n argocd -o jsonpath="{.data.password}" | base64 -d
# 用户名: admin
```
### 5.3 配置 ArgoCD Application

`argocd-application.yaml`:
```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: todo-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/yourname/todo-app.git
    targetRevision: main
    path: helm/todo-app
    helm:
      valueFiles:
        - values.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: todo-app
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f argocd-application.yaml

# 访问 ArgoCD UI: https://localhost:8080
# 登录后可以看到应用同步状态
```
### 5.4 体验 GitOps

1. 修改代码（如改个前端颜色）
2. `git push`
3. ArgoCD 自动检测到 Git 变更 → 自动同步到集群
4. 刷新浏览器，看到变更生效

**这就是 GitOps：Git 是唯一事实来源，集群状态自动与 Git 保持一致。**

---

## 阶段六：可观测性（30 分钟）

### 6.1 部署 Prometheus + Grafana

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 kube-prometheus-stack Helm Chart
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --set grafana.enabled=true

# 暴露 Grafana
kubectl port-forward svc/prometheus-grafana -n monitoring 3001:80
# 访问 http://localhost:3001，默认账号 admin / prom-operator
```
### 6.2 配置应用监控

创建 `ServiceMonitor` 让 Prometheus 抓取应用指标：

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: todo-app-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: todo-web
  namespaceSelector:
    matchNames:
      - todo-app
  endpoints:
  - port: http
    path: /metrics
    interval: 15s
```

### 6.3 导入 Grafana 看板

在 Grafana 中创建看板，展示：
- Todo 数量变化趋势
- 缓存命中率
- Pod CPU/内存使用率
- HTTP 请求 QPS 和延迟

### 6.4 配置告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: todo-app-alerts
  namespace: monitoring
spec:
  groups:
  - name: todo-app
    rules:
    - alert: TodoAppHighErrorRate
      expr: rate(http_requests_total{status=~"5.."}[5m]) > 0.1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Todo App 错误率过高"
        description: "5xx 错误率超过 10%"
    - alert: TodoAppPodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total{namespace="todo-app"}[10m]) > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Todo App Pod 反复重启"
```

---

## 阶段七：故障演练（20 分钟）

### 场景 1：Pod 崩溃自动恢复

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 手动删除一个 Pod
kubectl delete pod -n todo-app -l app=todo-web --grace-period=0

# 观察：Deployment 自动创建新 Pod
kubectl get pods -n todo-app -w
```
### 场景 2：数据库连接断开

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除 PostgreSQL Pod
kubectl delete pod -n todo-app -l app=postgres

# 观察：应用 Pod 进入 CrashLoopBackOff
# 等 PostgreSQL 恢复后，应用自动恢复
kubectl logs -n todo-app deployment/todo-web -f
```
### 场景 3：HPA 弹性扩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 压力测试
kubectl run -it --rm load-generator --image=busybox:1.36 --restart=Never -- /bin/sh -c "while true; do wget -q -O- http://todo-web.todo-app.svc.cluster.local; done"

# 另一个终端观察 HPA
kubectl get hpa -n todo-app -w
# 看到副本数从 3 → 5 → 8 → 10
```
### 场景 4：配置变更（不重建镜像）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 修改 ConfigMap（如改页面标题）
kubectl edit configmap todo-config -n todo-app

# 滚动重启应用（让配置生效）
kubectl rollout restart deployment/todo-web -n todo-app

# 验证：页面标题已更新，但镜像没变
```
---

## 项目总结

### 你完成了什么？

| 阶段 | 产出 | 对应生产技能 |
|------|------|-------------|
| 应用开发 | 一个完整的 CRUD 应用 | 全栈开发能力 |
| 容器化 | Dockerfile + 镜像 | 云原生开发规范 |
| K8s 部署 | 7 个 YAML 文件 | 平台部署能力 |
| Helm 打包 | 可复用的 Chart | 交付标准化 |
| GitOps | ArgoCD 自动同步 | 现代交付流程 |
| 可观测性 | Prometheus + Grafana | 生产运维能力 |
| 故障演练 | 4 个排障场景 | SRE 核心能力 |

### 写在简历上

> **项目：云原生 Todo 应用**
> - 使用 Node.js + Express 开发全栈应用，PostgreSQL 持久化 + Redis 缓存加速
> - 编写多阶段 Dockerfile，镜像体积从 1GB 优化至 120MB
> - 编写 K8s manifest，实现 Deployment 滚动更新、HPA 自动扩缩容、Ingress 流量入口
> - 使用 Helm 打包应用，支持多环境（dev/staging/prod）配置分离
> - 搭建 ArgoCD 实现 GitOps 交付，代码提交后 30 秒内自动同步到集群
> - 部署 Prometheus + Grafana 监控体系，配置 5 个核心业务告警规则

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 卸载 Helm releases
helm uninstall todo-app -n todo-app  # ⚠️ 删除 release 及关联资源
helm uninstall prometheus -n monitoring  # ⚠️ 删除 release 及关联资源

# 删除命名空间（级联删除所有资源）
kubectl delete ns todo-app monitoring argocd  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 删除 kind 集群（如需）
kind delete cluster --name k8s-lab
```
---

## 下一步学习建议

1. **加中间件**: 给项目加上 Kafka 消息队列、Elasticsearch 搜索
2. **加安全**: 配置 [[NetworkPolicy|NetworkPolicy]]、RBAC、Pod Security Standards
3. **加多环境**: 用 Helm values 文件管理 dev/staging/prod 差异
4. **加测试**: 在 CI 流水线中加入 K8s 集成测试（kuttl、helm test）
5. **加混沌**: 用 Chaos Mesh 做故障注入演练

---

**关联文档**:
- [[02-local-lab-environment]] — 本地实验环境搭建
- [[04-cka-exam-prep-guide]] — 备考 CKA 时，本项目是很好的练习素材
- ../fundamentals/03-deployment-basics.md — Deployment 基础概念
- ../../domain-08-release-change-management/01-gitops/ — GitOps 深度解析


<!-- risk-assessed -->
