# 13 - ConfigMap YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02  
> **相关领域**: [域3-存储与配置](../domain-3-storage/) | **前置知识**: Pod, Volume  
> **关联配置**: [14-Secret参考](./14-secret-all-types.md) | [Pod配置](./01-pod-all-fields.md)

---

## 📋 目录

1. [API 概述与版本](#api-概述与版本)
2. [字段规格表](#字段规格表)
3. [基础配置示例](#基础配置示例)
4. [环境变量注入](#环境变量注入)
5. [Volume 挂载](#volume-挂载)
6. [高级使用技巧](#高级使用技巧)
7. [内部实现原理](#内部实现原理)
8. [生产实战案例](#生产实战案例)
9. [版本兼容性与最佳实践](#版本兼容性与最佳实践)

---

## API 概述与版本

### 基本信息

| 属性 | 值 |
|------|-----|
| **API Group** | `` (core) |
| **API Version** | `v1` |
| **Kind** | `ConfigMap` |
| **命名空间作用域** | ✅ 是 |
| **缩写** | `cm` |

### 核心特性

```yaml
# ConfigMap 用途
# 1. 应用配置文件 (如 nginx.conf, application.yaml)
# 2. 环境变量值
# 3. 命令行参数
# 4. 启动脚本
# 5. 多环境配置 (开发/测试/生产)
```

### 与 Secret 的差异

| 维度 | ConfigMap | Secret |
|------|-----------|--------|
| **存储内容** | 非敏感配置 | 敏感信息(密码, 证书) |
| **数据编码** | 明文 | Base64 编码 |
| **etcd 加密** | 否 | 可选(需 EncryptionConfiguration) |
| **挂载方式** | tmpfs (内存) | tmpfs (内存) |
| **大小限制** | 1MB | 1MB |
| **不可变性** | 支持 (v1.21+) | 支持 (v1.21+) |

---

## 字段规格表

### 核心字段

| 字段路径 | 类型 | 必填 | 版本 | 说明 |
|----------|------|------|------|------|
| `apiVersion` | string | ✅ | v1 | 固定为 `v1` |
| `kind` | string | ✅ | v1 | 固定为 `ConfigMap` |
| `metadata.name` | string | ✅ | v1 | ConfigMap 名称 |
| `metadata.namespace` | string | ❌ | v1 | 命名空间(默认 default) |
| `data` | map[string]string | ❌ | v1 | UTF-8 文本键值对 |
| `binaryData` | map[string][]byte | ❌ | v1 | 二进制数据(Base64) |
| `immutable` | bool | ❌ | v1.21+ | 不可变标记 |

### 限制说明

| 限制项 | 值 | 说明 |
|--------|-----|------|
| **总大小** | 1 MiB (1048576 字节) | `data` + `binaryData` 总和 |
| **键名规范** | `[-._a-zA-Z0-9]+` | 可包含字母、数字、`-`、`_`、`.` |
| **键名保留** | 不能以 `..` 开头 | 避免路径遍历攻击 |
| **etcd 限制** | etcd 单值最大 1.5 MiB | ConfigMap 包含 metadata 开销 |

---

## 基础配置示例

### 简单键值对

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: default
data:
  # 简单文本配置
  database_host: "mysql.default.svc.cluster.local"
  database_port: "3306"
  log_level: "info"
  feature_flag: "true"
```

### 多行配置文件

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-config
  namespace: web
data:
  # 使用 | 保留换行符
  nginx.conf: |
    user nginx;
    worker_processes auto;
    error_log /var/log/nginx/error.log warn;
    pid /var/run/nginx.pid;
    
    events {
        worker_connections 1024;
    }
    
    http {
        include /etc/nginx/mime.types;
        default_type application/octet-stream;
        
        log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                        '$status $body_bytes_sent "$http_referer" '
                        '"$http_user_agent" "$http_x_forwarded_for"';
        
        access_log /var/log/nginx/access.log main;
        
        sendfile on;
        keepalive_timeout 65;
        
        server {
            listen 80;
            server_name localhost;
            
            location / {
                root /usr/share/nginx/html;
                index index.html;
            }
        }
    }
  
  # YAML 配置示例
  app-config.yaml: |
    server:
      port: 8080
      host: 0.0.0.0
    
    database:
      driver: mysql
      host: mysql.default.svc
      port: 3306
      name: myapp_db
    
    logging:
      level: info
      format: json
```

### 二进制数据 (binaryData)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: binary-config
  namespace: default
binaryData:
  # Base64 编码的二进制数据 (如图片、证书)
  # echo "Hello Binary" | base64
  sample.bin: SGVsbG8gQmluYXJ5Cg==

data:
  # 文本数据仍使用 data
  text-file.txt: "This is plain text"
```

### 不可变 ConfigMap (v1.21+)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: immutable-config
  namespace: production
# 标记为不可变(创建后无法修改)
immutable: true
data:
  app_version: "v1.0.0"
  release_date: "2026-02-10"
  # 优势:
  # 1. kubelet 不再监听变更, 减少 API Server 负载
  # 2. 防止意外修改生产配置
  # 3. 提升集群性能(大量 Pod 使用时)
```

---

## 环境变量注入

### 单个键作为环境变量

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    env:
    # 方式1: 单个键注入
    - name: DATABASE_HOST
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database_host
    
    - name: DATABASE_PORT
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database_port
    
    # 可选: 设置 optional=true, ConfigMap 不存在时不报错
    - name: OPTIONAL_CONFIG
      valueFrom:
        configMapKeyRef:
          name: optional-config
          key: some_key
          optional: true
```

### 所有键作为环境变量 (envFrom)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: env-config
data:
  LOG_LEVEL: info
  MAX_CONNECTIONS: "100"
  ENABLE_CACHE: "true"

---
apiVersion: v1
kind: Pod
metadata:
  name: envfrom-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    # 将 ConfigMap 所有键作为环境变量
    envFrom:
    - configMapRef:
        name: env-config
    
    # 结果: 容器中自动创建环境变量
    # LOG_LEVEL=info
    # MAX_CONNECTIONS=100
    # ENABLE_CACHE=true
```

### 添加前缀 (envFrom + prefix)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: prefix-env-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    envFrom:
    # 为所有环境变量添加前缀
    - prefix: APP_
      configMapRef:
        name: env-config
    
    # 结果:
    # APP_LOG_LEVEL=info
    # APP_MAX_CONNECTIONS=100
    # APP_ENABLE_CACHE=true
```

### 处理无效键名

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: special-keys
data:
  # 有效键名
  valid-key: "value1"
  
  # 无效键名(不符合环境变量命名规范)
  "invalid.key": "value2"  # 包含 .
  "123-start": "value3"     # 以数字开头

---
apiVersion: v1
kind: Pod
metadata:
  name: special-env-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    envFrom:
    - configMapRef:
        name: special-keys
    
    # 结果:
    # valid-key=value1 (自动转换为 VALID_KEY)
    # invalid.key 和 123-start 会被跳过(无效环境变量名)
```

---

## Volume 挂载

### 完整挂载 (所有键)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-files
data:
  config.json: |
    {
      "server": {
        "port": 8080
      }
    }
  settings.ini: |
    [database]
    host=mysql.default.svc
    port=3306

---
apiVersion: v1
kind: Pod
metadata:
  name: volume-mount-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    # 挂载整个 ConfigMap
    - name: config-volume
      mountPath: /etc/config
      readOnly: true
    
    # 结果: 在容器中生成文件
    # /etc/config/config.json
    # /etc/config/settings.ini
  
  volumes:
  - name: config-volume
    configMap:
      name: app-files
```

### 选择性挂载 (items)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: selective-mount-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: config-volume
      mountPath: /etc/config
  
  volumes:
  - name: config-volume
    configMap:
      name: app-files
      # 仅挂载指定的键
      items:
      - key: config.json
        path: app-config.json  # 自定义文件名
      - key: settings.ini
        path: db/settings.ini  # 可包含子目录
      
      # 结果:
      # /etc/config/app-config.json
      # /etc/config/db/settings.ini
```

### 设置文件权限 (defaultMode)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: file-mode-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: script-volume
      mountPath: /scripts
  
  volumes:
  - name: script-volume
    configMap:
      name: startup-scripts
      # 设置文件权限 (八进制)
      defaultMode: 0755  # rwxr-xr-x (可执行脚本)
      
      items:
      - key: startup.sh
        path: startup.sh
        mode: 0644  # rw-r--r-- (覆盖 defaultMode)
```

### subPath 挂载 (单文件)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-config
data:
  nginx.conf: |
    # Nginx 配置内容
    server {
        listen 80;
    }

---
apiVersion: v1
kind: Pod
metadata:
  name: subpath-pod
spec:
  containers:
  - name: nginx
    image: nginx:1.25
    volumeMounts:
    # 使用 subPath 仅挂载单个文件
    - name: config-volume
      mountPath: /etc/nginx/nginx.conf
      subPath: nginx.conf  # 仅挂载此文件
      readOnly: true
    
    # 注意: subPath 挂载不会自动更新!
    # ConfigMap 更新后需要重启 Pod
  
  volumes:
  - name: config-volume
    configMap:
      name: nginx-config
```

### 避免覆盖现有目录

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: no-overwrite-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    # 错误示例: 会覆盖 /etc 目录的所有内容
    # - name: config-volume
    #   mountPath: /etc
    
    # 正确示例1: 挂载到子目录
    - name: config-volume
      mountPath: /etc/myapp
      readOnly: true
    
    # 正确示例2: 使用 subPath 挂载单个文件
    - name: config-volume
      mountPath: /etc/myapp/config.json
      subPath: config.json
      readOnly: true
  
  volumes:
  - name: config-volume
    configMap:
      name: app-files
```

---

## 高级使用技巧

### 组合多个 ConfigMap

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-configmap-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    
    # 环境变量: 从多个 ConfigMap 注入
    env:
    - name: DB_HOST
      valueFrom:
        configMapKeyRef:
          name: database-config
          key: host
    - name: REDIS_HOST
      valueFrom:
        configMapKeyRef:
          name: cache-config
          key: host
    
    volumeMounts:
    # Volume 1: 应用配置
    - name: app-config
      mountPath: /etc/app
    # Volume 2: Nginx 配置
    - name: nginx-config
      mountPath: /etc/nginx
  
  volumes:
  - name: app-config
    configMap:
      name: app-files
  - name: nginx-config
    configMap:
      name: nginx-files
```

### Projected Volume (合并多个配置)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: projected-volume-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: all-config
      mountPath: /etc/config
  
  volumes:
  # Projected Volume: 将多个 ConfigMap 合并到同一目录
  - name: all-config
    projected:
      sources:
      # 来源1: ConfigMap
      - configMap:
          name: app-config
          items:
          - key: app.yaml
            path: app.yaml
      
      # 来源2: 另一个 ConfigMap
      - configMap:
          name: database-config
          items:
          - key: database.yaml
            path: database.yaml
      
      # 来源3: Secret (可混合)
      - secret:
          name: api-keys
          items:
          - key: api_key
            path: secrets/api_key
      
      # 结果:
      # /etc/config/app.yaml
      # /etc/config/database.yaml
      # /etc/config/secrets/api_key
```

### 动态配置注入 (InitContainer)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: dynamic-config-pod
spec:
  # InitContainer: 处理配置模板
  initContainers:
  - name: config-renderer
    image: busybox:1.36
    command:
    - sh
    - -c
    - |
      # 替换配置模板中的变量
      sed "s/\${POD_NAME}/${POD_NAME}/g" /templates/config.tpl > /config/config.yaml
      sed -i "s/\${POD_IP}/${POD_IP}/g" /config/config.yaml
    env:
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    volumeMounts:
    - name: config-template
      mountPath: /templates
    - name: rendered-config
      mountPath: /config
  
  # 主容器: 使用渲染后的配置
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: rendered-config
      mountPath: /etc/config
  
  volumes:
  # ConfigMap: 配置模板
  - name: config-template
    configMap:
      name: config-templates
  # EmptyDir: 存储渲染后的配置
  - name: rendered-config
    emptyDir: {}
```

### kubectl 命令行创建

```bash
# 从字面量创建
kubectl create configmap literal-config \
  --from-literal=key1=value1 \
  --from-literal=key2=value2

# 从文件创建
kubectl create configmap file-config \
  --from-file=config.yaml \
  --from-file=settings.ini

# 从目录创建 (目录下所有文件)
kubectl create configmap dir-config \
  --from-file=./config-dir/

# 从环境文件创建 (.env 格式)
kubectl create configmap env-config \
  --from-env-file=.env

# 指定命名空间
kubectl create configmap app-config \
  --from-file=config.yaml \
  --namespace=production

# 生成 YAML 而不创建 (dry-run)
kubectl create configmap test-config \
  --from-literal=key=value \
  --dry-run=client -o yaml > configmap.yaml
```

---

## 内部实现原理

### ConfigMap 更新传播机制

```yaml
# ConfigMap 更新到 Pod 的时间线

# 1. 更新 ConfigMap
kubectl edit configmap app-config

# 2. kubelet 同步周期
# - 默认同步周期: --sync-frequency=1m (每分钟检查一次)
# - 实际更新时间: 1m ~ 2m (取决于 kubelet 缓存刷新)

# 3. Volume 挂载更新
# - 完整挂载 (volume): 自动更新 (1-2分钟)
# - subPath 挂载: 永不更新 (需要重启 Pod)

# 4. 环境变量注入
# - env/envFrom: 永不更新 (需要重启 Pod)
```

### 为什么 subPath 不自动更新?

```yaml
# 原理解析:
# 1. 完整挂载: kubelet 创建符号链接
#    /etc/config -> /var/lib/kubelet/pods/{uid}/volumes/kubernetes.io~configmap/{name}
#    当 ConfigMap 更新时, kubelet 更新目标目录, 符号链接自动指向新内容

# 2. subPath 挂载: kubelet 创建硬绑定
#    直接 bind mount 单个文件到容器, 无符号链接层
#    更新需要重新 bind mount, 但这会导致容器重启

# 示例: 查看挂载方式
apiVersion: v1
kind: Pod
metadata:
  name: mount-test
spec:
  containers:
  - name: app
    image: busybox
    command: ["sh", "-c", "sleep 3600"]
    volumeMounts:
    - name: config
      mountPath: /etc/config
  volumes:
  - name: config
    configMap:
      name: app-config

# 进入容器查看
# kubectl exec mount-test -- ls -la /etc/config
# lrwxrwxrwx 1 root root 31 Feb 10 10:00 /etc/config/config.yaml -> ..data/config.yaml
# lrwxrwxrwx 1 root root 25 Feb 10 10:00 ..data -> ..2026_02_10_10_00_12_345
```

### kubelet 同步周期调优

```yaml
# kubelet 配置参数
# --sync-frequency=1m            # ConfigMap/Secret 同步周期 (默认 1分钟)
# --config-map-and-secret-change-detection-strategy=Watch  # 监听模式 (推荐)

# 模式对比:
# 1. Poll (轮询): kubelet 定期向 API Server 查询所有 ConfigMap
#    - 延迟: --sync-frequency 时间
#    - API Server 负载: 高

# 2. Watch (监听): kubelet 通过 Watch API 实时接收变更
#    - 延迟: 秒级
#    - API Server 负载: 低 (推荐)

# 注意: 即使使用 Watch, 挂载到 Pod 仍需等待 kubelet 缓存刷新
```

### 不可变 ConfigMap 的性能优势

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: immutable-config
immutable: true
data:
  key: value

# 性能优势:
# 1. kubelet 跳过监听: 标记为 immutable 后, kubelet 不再监听此 ConfigMap 变更
# 2. 减少 API Server 负载: 大规模集群中, 数千个 Pod 使用同一 ConfigMap 时显著降低 Watch 连接数
# 3. 内存优化: kubelet 不缓存更新检查任务

# 权衡:
# - 无法修改: 需要创建新 ConfigMap + 更新 Pod 引用
# - 适用场景: 静态配置, 版本化配置 (如 app-config-v1, app-config-v2)
```

---

## 生产实战案例

### 案例1: Nginx 配置热更新

```yaml
# 场景: Nginx 配置变更自动热加载, 无需重启 Pod

# 1. ConfigMap 存储 Nginx 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-config
  namespace: web
data:
  nginx.conf: |
    events {
        worker_connections 1024;
    }
    http {
        server {
            listen 80;
            location / {
                return 200 "Version 1.0\n";
            }
        }
    }

---
# 2. Deployment + Sidecar 实现热更新
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-hot-reload
  namespace: web
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      # 主容器: Nginx
      - name: nginx
        image: nginx:1.25
        ports:
        - containerPort: 80
        volumeMounts:
        - name: nginx-config
          mountPath: /etc/nginx/nginx.conf
          subPath: nginx.conf
          readOnly: true
      
      # Sidecar: 监听配置变更并热加载
      - name: config-reloader
        image: busybox:1.36
        command:
        - sh
        - -c
        - |
          # 计算配置文件的初始哈希
          OLD_HASH=$(md5sum /etc/nginx/nginx.conf | awk '{print $1}')
          echo "Initial config hash: $OLD_HASH"
          
          while true; do
            sleep 10
            
            # 检查配置是否变更
            NEW_HASH=$(md5sum /etc/nginx/nginx.conf | awk '{print $1}')
            if [ "$NEW_HASH" != "$OLD_HASH" ]; then
              echo "Config changed! Reloading Nginx..."
              
              # 向 Nginx 发送 reload 信号
              nginx -s reload 2>/dev/null || true
              
              OLD_HASH=$NEW_HASH
            fi
          done
        volumeMounts:
        - name: nginx-config
          mountPath: /etc/nginx
          readOnly: true
      
      volumes:
      - name: nginx-config
        configMap:
          name: nginx-config

# 3. 更新配置验证热加载
# kubectl edit configmap nginx-config -n web
# (修改 location / return 内容为 "Version 2.0")
# curl http://<nginx-service-ip>
# 输出: Version 2.0 (10-120秒后生效, 无需重启 Pod)
```

### 案例2: 多环境配置管理

```yaml
# 场景: 同一应用在开发/测试/生产环境使用不同配置

# 开发环境 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: development
data:
  environment: "development"
  log_level: "debug"
  database_host: "mysql.development.svc.cluster.local"
  enable_debug: "true"

---
# 测试环境 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: testing
data:
  environment: "testing"
  log_level: "info"
  database_host: "mysql.testing.svc.cluster.local"
  enable_debug: "false"

---
# 生产环境 ConfigMap (不可变)
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-v1
  namespace: production
  labels:
    version: "v1"
immutable: true  # 生产环境强制不可变
data:
  environment: "production"
  log_level: "warn"
  database_host: "mysql-primary.production.svc.cluster.local"
  enable_debug: "false"

---
# 统一 Deployment 模板 (使用 Kustomize/Helm 管理)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: myapp:latest
        envFrom:
        # 引用当前命名空间的同名 ConfigMap
        - configMapRef:
            name: app-config  # 开发/测试环境
            # name: app-config-v1  # 生产环境
```

### 案例3: 应用配置文件注入

```yaml
# 场景: Spring Boot 应用使用 application.yaml 配置

# ConfigMap 存储完整配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: springboot-config
  namespace: default
data:
  application.yaml: |
    server:
      port: 8080
      servlet:
        context-path: /api
    
    spring:
      application:
        name: my-springboot-app
      
      datasource:
        url: jdbc:mysql://mysql.default.svc:3306/mydb
        username: ${DB_USERNAME}
        password: ${DB_PASSWORD}
        driver-class-name: com.mysql.cj.jdbc.Driver
      
      jpa:
        hibernate:
          ddl-auto: validate
        show-sql: false
    
    logging:
      level:
        root: INFO
        com.mycompany: DEBUG
  
  logback-spring.xml: |
    <?xml version="1.0" encoding="UTF-8"?>
    <configuration>
      <appender name="CONSOLE" class="ch.qos.logback.core.ConsoleAppender">
        <encoder>
          <pattern>%d{yyyy-MM-dd HH:mm:ss} - %msg%n</pattern>
        </encoder>
      </appender>
      
      <root level="INFO">
        <appender-ref ref="CONSOLE" />
      </root>
    </configuration>

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: springboot-app
spec:
  replicas: 2
  selector:
    matchLabels:
      app: springboot
  template:
    metadata:
      labels:
        app: springboot
    spec:
      containers:
      - name: app
        image: mycompany/springboot-app:latest
        ports:
        - containerPort: 8080
        
        # 环境变量: 注入敏感信息
        env:
        - name: DB_USERNAME
          valueFrom:
            secretKeyRef:
              name: database-secret
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: database-secret
              key: password
        
        # Volume 挂载: 配置文件
        volumeMounts:
        - name: config
          mountPath: /config
          readOnly: true
        
        # Spring Boot 指定配置文件位置
        args:
        - --spring.config.location=classpath:/application.yaml,file:/config/application.yaml
        - --logging.config=/config/logback-spring.xml
      
      volumes:
      - name: config
        configMap:
          name: springboot-config
```

### 案例4: 启动脚本注入

```yaml
# 场景: 数据库初始化脚本通过 ConfigMap 注入

# ConfigMap 存储初始化脚本
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-init-scripts
  namespace: database
data:
  01-create-database.sql: |
    CREATE DATABASE IF NOT EXISTS myapp_db
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;
    
    USE myapp_db;
  
  02-create-tables.sql: |
    CREATE TABLE IF NOT EXISTS users (
      id BIGINT AUTO_INCREMENT PRIMARY KEY,
      username VARCHAR(255) NOT NULL UNIQUE,
      email VARCHAR(255) NOT NULL,
      created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
    
    CREATE INDEX idx_username ON users(username);
  
  03-seed-data.sql: |
    INSERT INTO users (username, email) VALUES
    ('admin', 'admin@example.com'),
    ('test_user', 'test@example.com')
    ON DUPLICATE KEY UPDATE username=username;

---
apiVersion: v1
kind: Pod
metadata:
  name: mysql-with-init
  namespace: database
spec:
  containers:
  - name: mysql
    image: mysql:8.0
    env:
    - name: MYSQL_ROOT_PASSWORD
      valueFrom:
        secretKeyRef:
          name: mysql-secret
          key: root-password
    ports:
    - containerPort: 3306
    volumeMounts:
    # 挂载初始化脚本到 MySQL 的 docker-entrypoint-initdb.d
    - name: init-scripts
      mountPath: /docker-entrypoint-initdb.d
      readOnly: true
    # MySQL 数据持久化
    - name: data
      mountPath: /var/lib/mysql
  
  volumes:
  - name: init-scripts
    configMap:
      name: mysql-init-scripts
  - name: data
    persistentVolumeClaim:
      claimName: mysql-pvc

# MySQL 容器启动时会自动执行 /docker-entrypoint-initdb.d 中的脚本(按字母顺序)
```

---

## 版本兼容性与最佳实践

### 版本特性

| Kubernetes 版本 | ConfigMap 新特性 |
|-----------------|------------------|
| v1.19+ | `immutable` 字段 (Alpha) |
| v1.21+ | `immutable` 字段 (GA) |
| v1.25+ | 无重大变更 |
| v1.32+ | 无重大变更 |

### 最佳实践

#### 1. 版本化管理

```yaml
# 推荐: 为 ConfigMap 添加版本后缀
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-v1
  labels:
    version: "v1"
    app: myapp
data:
  config: "version 1"

---
# 新版本
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-v2
  labels:
    version: "v2"
    app: myapp
immutable: true
data:
  config: "version 2"

---
# Deployment 引用特定版本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      containers:
      - name: app
        image: myapp:latest
        envFrom:
        - configMapRef:
            name: app-config-v2  # 明确指定版本
```

#### 2. 使用 immutable 提升性能

```yaml
# 适用场景:
# - 静态配置 (如版本号, 常量)
# - 大规模集群 (>100 节点)
# - 高频访问的配置 (>1000 Pod 引用)

apiVersion: v1
kind: ConfigMap
metadata:
  name: static-config-2026-02-10
immutable: true
data:
  app_version: "1.0.0"
  build_date: "2026-02-10"
  release_notes_url: "https://example.com/releases/v1.0.0"
```

#### 3. 分离敏感与非敏感配置

```yaml
# ConfigMap: 非敏感配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  database_host: "mysql.default.svc"
  database_port: "3306"
  log_level: "info"

---
# Secret: 敏感信息
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets
type: Opaque
data:
  database_password: cGFzc3dvcmQxMjM=  # base64: password123

---
# Pod 同时引用
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  containers:
  - name: app
    image: myapp:latest
    env:
    # 非敏感配置
    - name: DB_HOST
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database_host
    # 敏感信息
    - name: DB_PASSWORD
      valueFrom:
        secretKeyRef:
          name: app-secrets
          key: database_password
```

#### 4. 避免过大的 ConfigMap

```yaml
# 反模式: 单个 ConfigMap 接近 1MB 限制
# apiVersion: v1
# kind: ConfigMap
# metadata:
#   name: huge-config
# data:
#   large_file: |
#     ... 900KB 内容 ...

# 推荐: 拆分为多个 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-part1
data:
  config1: "..."
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-part2
data:
  config2: "..."

# 或者: 使用外部存储 (如 S3, NFS, Git)
```

#### 5. 配置热更新策略

```yaml
# 场景1: 需要热更新 -> 使用 Volume 挂载 (非 subPath)
apiVersion: v1
kind: Pod
metadata:
  name: hot-reload-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: config
      mountPath: /etc/config  # 完整挂载
  volumes:
  - name: config
    configMap:
      name: app-config

# 场景2: 无需热更新 -> 使用环境变量 (更高效)
apiVersion: v1
kind: Pod
metadata:
  name: static-config-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    envFrom:
    - configMapRef:
        name: app-config
```

### FAQ

#### Q1: ConfigMap 更新后多久生效?

**A:** 时间线分析:
- **Volume 挂载**: 1-2 分钟 (kubelet 同步周期 + 缓存刷新)
- **subPath 挂载**: 永不生效 (需重启 Pod)
- **环境变量**: 永不生效 (需重启 Pod)

```bash
# 加速更新方法:
# 1. 重启 Pod (立即生效)
kubectl rollout restart deployment/myapp

# 2. 使用不可变 ConfigMap + 版本化
# (创建新 ConfigMap, 更新 Deployment 引用)
```

#### Q2: 如何强制 Pod 使用最新配置?

**A:** 三种方案:
```yaml
# 方案1: Deployment annotation 触发滚动更新
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    metadata:
      annotations:
        configmap-version: "v2"  # 修改此值触发更新
    spec:
      containers:
      - name: app
        image: myapp:latest
        envFrom:
        - configMapRef:
            name: app-config

# 方案2: ConfigMap 哈希后缀
# (Helm/Kustomize 自动生成)
# app-config-v1-abc123
# app-config-v2-def456

# 方案3: Reloader (第三方工具)
# https://github.com/stakater/Reloader
# 自动监听 ConfigMap 变更并重启 Pod
```

#### Q3: subPath 挂载的文件能否更新?

**A:** 不能自动更新, 原因:
```yaml
# subPath 使用 bind mount, 绑定到特定 inode
# ConfigMap 更新创建新文件, 但 bind mount 仍指向旧 inode

# 解决方案:
# 1. 避免使用 subPath (推荐)
volumeMounts:
- name: config
  mountPath: /etc/config
  # 不使用 subPath

# 2. 必须使用 subPath 时, 配合 Sidecar 重启容器
# (复杂, 不推荐)
```

#### Q4: ConfigMap 能否跨命名空间引用?

**A:** 不能, ConfigMap 必须与 Pod 在同一命名空间:
```yaml
# 错误示例:
apiVersion: v1
kind: Pod
metadata:
  name: app
  namespace: team-a
spec:
  containers:
  - name: app
    image: myapp:latest
    envFrom:
    - configMapRef:
        name: shared-config
        namespace: shared  # 不支持! Pod 会启动失败

# 解决方案:
# 1. 复制 ConfigMap 到目标命名空间
# 2. 使用外部配置中心 (如 Consul, etcd)
# 3. 使用 Secret + ReferenceGrant (仅适用于 Secret)
```

#### Q5: 如何验证 ConfigMap 是否生效?

**A:** 诊断步骤:
```bash
# 1. 检查 ConfigMap 内容
kubectl get configmap app-config -o yaml

# 2. 验证 Pod 环境变量
kubectl exec <pod-name> -- env | grep <CONFIG_KEY>

# 3. 验证 Volume 挂载内容
kubectl exec <pod-name> -- cat /etc/config/config.yaml

# 4. 查看 Pod 事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 5. 检查 ConfigMap 引用错误
kubectl describe pod <pod-name> | grep "ConfigMap.*not found"
```

---

## 相关资源

### 官方文档
- ConfigMap 概念: https://kubernetes.io/docs/concepts/configuration/configmap/
- 配置 Pod 使用 ConfigMap: https://kubernetes.io/docs/tasks/configure-pod-container/configure-pod-configmap/

### 工具推荐
- **Reloader**: 自动重启使用 ConfigMap 的 Pod (https://github.com/stakater/Reloader)
- **Kustomize**: ConfigMap 生成器 (自动添加哈希后缀)
- **Helm**: 模板化 ConfigMap 管理

### 本知识库相关文档
- [14 - Secret 全类型参考](./14-secret-all-types.md)
- [01 - Pod 完整字段参考](./01-pod-all-fields.md)
- [Volume 类型参考](./06-volume-types.md)

---

**最后更新**: 2026-02 | **维护者**: Kudig.io 社区 | **反馈**: [GitHub Issues](https://github.com/kudig-io/kudig-database)
