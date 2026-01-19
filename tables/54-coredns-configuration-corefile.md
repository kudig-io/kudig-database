# 54 - CoreDNS Corefile 配置详解 (Corefile Configuration)

> **适用版本**: CoreDNS 1.8.0+ / Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、Corefile 语法基础 (Syntax Fundamentals)

### 1.1 基本结构

```
# 注释以 # 开头

# Server Block 基本格式
<zone>:[port] {
    <plugin> [arguments...]
    <plugin> {
        <option> <value>
    }
}

# 多Zone共享配置
<zone1> <zone2> <zone3>:[port] {
    <plugin>
}
```

### 1.2 语法元素详解

| 元素 | 格式 | 示例 | 说明 |
|:---|:---|:---|:---|
| **Zone** | `<域名>.` | `cluster.local.`, `.` | 必须以`.`结尾(根区域除外) |
| **Port** | `:<端口号>` | `:53`, `:5353` | 可选,默认53 |
| **Plugin** | `<名称>` | `kubernetes`, `forward` | 区分大小写 |
| **Arguments** | 空格分隔 | `forward . 8.8.8.8 8.8.4.4` | 紧跟插件名 |
| **Block** | `{ }` | 见示例 | 多行配置 |
| **Comment** | `#` | `# 这是注释` | 单行注释 |

### 1.3 特殊语法

```
# 1. 导入外部文件
import /etc/coredns/custom/*.conf

# 2. 环境变量替换
forward . {$UPSTREAM_DNS}

# 3. 多区域共享
cluster.local in-addr.arpa ip6.arpa:53 {
    kubernetes
}

# 4. 协议前缀
tls://.:853 {
    tls cert.pem key.pem ca.pem
    forward . 8.8.8.8
}

# 5. 通配符Zone
*.example.com:53 {
    forward . 10.0.0.1
}
```

---

## 二、Kubernetes 默认配置详解 (Default Configuration)

### 2.1 标准Corefile解析

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors                          # [1] 错误日志
        health {                        # [2] 健康检查
           lameduck 5s
        }
        ready                           # [3] 就绪探针
        kubernetes cluster.local in-addr.arpa ip6.arpa {  # [4] K8s插件
           pods insecure
           fallthrough in-addr.arpa ip6.arpa
           ttl 30
        }
        prometheus :9153                # [5] 指标暴露
        forward . /etc/resolv.conf {    # [6] 上游转发
           max_concurrent 1000
        }
        cache 30                        # [7] 缓存
        loop                            # [8] 循环检测
        reload                          # [9] 热重载
        loadbalance                     # [10] 负载均衡
    }
```

### 2.2 各插件作用详解

| 序号 | 插件 | 作用 | 必要性 | 配置项说明 |
|:---:|:---|:---|:---:|:---|
| 1 | **errors** | 将错误输出到stdout | 必须 | 应放在最前面 |
| 2 | **health** | 暴露/health端点于:8080 | 推荐 | `lameduck`=优雅关闭等待时间 |
| 3 | **ready** | 暴露/ready端点于:8181 | 推荐 | 所有插件就绪后返回200 |
| 4 | **kubernetes** | K8s服务发现核心 | 必须 | 处理cluster.local等区域 |
| 5 | **prometheus** | 暴露/metrics于:9153 | 推荐 | Prometheus格式指标 |
| 6 | **forward** | 转发非K8s查询 | 必须 | 处理外部域名 |
| 7 | **cache** | DNS响应缓存 | 推荐 | 大幅提升性能 |
| 8 | **loop** | 检测转发循环 | 推荐 | 防止无限循环 |
| 9 | **reload** | ConfigMap变更热重载 | 推荐 | 默认30s检查间隔 |
| 10 | **loadbalance** | A/AAAA记录轮询 | 可选 | 随机化响应顺序 |

---

## 三、核心插件配置详解 (Core Plugins Configuration)

### 3.1 kubernetes 插件

```
kubernetes [ZONES...] {
    # 基础配置
    endpoint URL                    # API Server地址(默认自动发现)
    tls CERT KEY CACERT            # TLS证书配置
    kubeconfig KUBECONFIG CONTEXT  # kubeconfig文件路径
    
    # Pod解析模式
    pods POD-MODE                   # disabled|insecure|verified
    
    # 命名空间过滤
    namespaces NAMESPACE...         # 限制解析的命名空间
    
    # 标签选择
    labels EXPRESSION               # 基于标签过滤
    
    # 回退行为
    fallthrough [ZONES...]          # 无法解析时传递给下一个插件
    
    # TTL设置
    ttl SECONDS                     # 响应TTL(默认5秒)
    
    # 否定缓存
    noendpoints                     # 不返回endpoint记录
}
```

**配置示例**:

```
kubernetes cluster.local in-addr.arpa ip6.arpa {
    pods verified                   # 验证Pod存在性
    namespaces production staging   # 只解析这两个命名空间
    labels environment=prod         # 只解析带此标签的资源
    fallthrough in-addr.arpa ip6.arpa
    ttl 60
}
```

**pods 模式对比**:

| 模式 | 说明 | Pod A记录 | 安全性 |
|:---|:---|:---|:---|
| `disabled` | 禁用Pod A记录 | 不创建 | 最高 |
| `insecure` | 无验证创建 | 总是创建 | 最低 |
| `verified` | 验证后创建 | 验证Pod存在后创建 | 中等 |

### 3.2 forward 插件

```
forward FROM TO... {
    # 目标服务器
    except IGNORED_NAMES...        # 排除的域名
    
    # 连接配置
    force_tcp                      # 强制TCP
    prefer_udp                     # 优先UDP
    expire DURATION                # 连接过期时间
    max_fails INTEGER              # 标记不健康前的失败次数
    
    # TLS配置
    tls CERT KEY CA                # TLS证书
    tls_servername NAME            # TLS服务器名
    
    # 健康检查
    health_check DURATION          # 健康检查间隔
    
    # 并发控制
    max_concurrent INTEGER         # 最大并发数
    
    # 策略
    policy random|round_robin|sequential  # 负载均衡策略
}
```

**配置示例**:

```
# 多上游DNS服务器
forward . 8.8.8.8 8.8.4.4 1.1.1.1 {
    max_concurrent 1000
    max_fails 3
    health_check 5s
    policy round_robin
    expire 10s
}

# 基于域名分流
forward example.com 10.0.0.53 {
    max_concurrent 100
}

forward . /etc/resolv.conf {
    except example.com
    max_concurrent 1000
}
```

### 3.3 cache 插件

```
cache [TTL] [ZONES...] {
    # 缓存容量
    success CAPACITY [TTL] [MINTTL]   # 成功响应缓存
    denial CAPACITY [TTL] [MINTTL]    # 否定响应缓存
    
    # 预取
    prefetch AMOUNT DURATION [PERCENTAGE%]  # 缓存预热
    
    # 过滤
    serve_stale [DURATION]           # 上游不可用时服务过期缓存
    
    # 禁用缓存的条件
    disable success|denial           # 禁用特定类型缓存
}
```

**配置示例**:

```
# 基础配置
cache 30

# 高级配置
cache {
    success 10000 3600 300    # 最多10000条,TTL 1小时,最小TTL 5分钟
    denial 1000 60 30         # 否定缓存1000条,TTL 1分钟
    prefetch 10 1h 10%        # 剩余10%TTL时预取
    serve_stale 1h            # 上游故障时服务最多1小时的过期缓存
}
```

### 3.4 rewrite 插件

```
rewrite [continue|stop] FIELD FROM TO [OPTIONS]

# FIELD类型:
# - name: 重写查询名称
# - type: 重写查询类型
# - class: 重写查询类别
# - edns0: 重写EDNS0选项
# - ttl: 重写响应TTL

# FROM格式:
# - exact:name    精确匹配
# - prefix:name   前缀匹配
# - suffix:name   后缀匹配
# - substring:name 子串匹配
# - regex:pattern 正则匹配
```

**配置示例**:

```
# 精确重写
rewrite name exact legacy-db.default.svc.cluster.local new-db.default.svc.cluster.local

# 后缀重写
rewrite name suffix .old.local .new.local

# 正则重写
rewrite name regex (.*)\.old\.example\.com {1}.new.example.com

# 类型重写
rewrite type AAAA A    # 将AAAA查询转为A查询

# 自动应答
rewrite stop {
    name regex (.*)\.internal\.example\.com
    answer name (.*)\.internal\.example\.com {1}.external.example.com
    answer value 10\.0\.0\.(\d+) 192.168.0.{1}
}
```

### 3.5 hosts 插件

```
hosts [FILE [ZONES...]] {
    # 回退
    fallthrough [ZONES...]
    
    # 内联记录
    INLINE_ENTRY...
    
    # 重载间隔
    reload DURATION
    
    # TTL
    ttl SECONDS
}
```

**配置示例**:

```
# 文件方式
hosts /etc/coredns/custom-hosts {
    fallthrough
    reload 10s
    ttl 60
}

# 内联方式
hosts {
    10.0.0.1 internal-api.company.local
    10.0.0.2 internal-db.company.local
    192.168.1.100 legacy-server.company.local
    
    fallthrough
    ttl 3600
}
```

### 3.6 log 插件

```
log [NAME] [FORMAT]

# FORMAT变量:
# {type}      - 查询类型(A,AAAA,SRV等)
# {name}      - 查询域名
# {class}     - 查询类别
# {proto}     - 协议(udp/tcp)
# {remote}    - 客户端IP
# {port}      - 客户端端口
# {size}      - 请求大小
# {rcode}     - 响应码
# {rsize}     - 响应大小
# {duration}  - 处理时长
# {>id}       - 请求ID
# {>opcode}   - 操作码
```

**配置示例**:

```
# 默认格式
log

# 自定义格式
log . "{remote}:{port} - [{time}] {>id} \"{type} {class} {name} {proto} {size}\" {rcode} {rsize} {duration}"

# 仅记录特定类型
log . {
    class denial    # 只记录否定响应
}

# 按域名过滤
log cluster.local {
    class all
}
```

---

## 四、高级配置场景 (Advanced Configuration Scenarios)

### 4.1 存根域 (Stub Domains)

企业内部DNS解析分流:

```
# Corefile
.:53 {
    errors
    health
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    prometheus :9153
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}

# 公司内部域名转发到内部DNS
internal.company.com:53 {
    errors
    cache 30
    forward . 10.0.0.53 10.0.0.54
}

# 合作伙伴域名转发到专用DNS
partner.example.com:53 {
    errors
    cache 60
    forward . 192.168.100.53
}
```

### 4.2 上游DNS高可用

```
.:53 {
    errors
    health
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    # 主DNS组
    forward . 8.8.8.8 8.8.4.4 {
        max_concurrent 1000
        max_fails 3
        health_check 5s
        policy round_robin
    }
    
    # 备用DNS (仅在主DNS全部失败时使用)
    alternate SERVFAIL,REFUSED,NXDOMAIN . 1.1.1.1 1.0.0.1
    
    cache 30
    loop
    reload
    loadbalance
}
```

### 4.3 ExternalName服务支持

```
.:53 {
    errors
    health
    
    # 重写ExternalName到实际CNAME
    rewrite continue {
        name regex (.*)\.external\.svc\.cluster\.local$ {1}.example.com
        answer name (.*)\.example\.com$ {1}.external.svc.cluster.local
    }
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

### 4.4 DNS-over-TLS (DoT) 配置

```
# DoT服务 (端口853)
tls://.:853 {
    tls /etc/coredns/cert.pem /etc/coredns/key.pem /etc/coredns/ca.pem
    
    errors
    health
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    forward . tls://8.8.8.8 tls://8.8.4.4 {
        tls_servername dns.google
        health_check 5s
    }
    
    cache 30
    loop
    reload
    loadbalance
}

# 同时保留普通DNS (端口53)
.:53 {
    errors
    health
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

### 4.5 多集群DNS联邦

```
# 主集群配置
.:53 {
    errors
    health
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    # 其他集群域名转发
    forward cluster-b.local 10.100.0.10 {
        max_concurrent 100
    }
    
    forward cluster-c.local 10.200.0.10 {
        max_concurrent 100
    }
    
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

### 4.6 自定义错误响应

```
.:53 {
    errors
    health
    
    # 对不存在的域名返回自定义响应
    template IN A blocked.domain.com {
        rcode NXDOMAIN
        authority "blocked.domain.com. 60 IN SOA ns.blocked.domain.com. admin.blocked.domain.com. 2021010100 3600 300 86400 60"
    }
    
    # 所有blocked子域返回0.0.0.0
    template IN A blocked {
        match .*\.blocked\.local$
        answer "{{ .Name }} 60 IN A 0.0.0.0"
        fallthrough
    }
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

### 4.7 ACL访问控制

```
.:53 {
    errors
    health
    
    # 访问控制
    acl {
        # 允许内部网段
        allow net 10.0.0.0/8
        allow net 172.16.0.0/12
        allow net 192.168.0.0/16
        
        # 拒绝其他
        block net *
    }
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
    }
    
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

---

## 五、ConfigMap 管理实践 (ConfigMap Management)

### 5.1 安全更新流程

```bash
# 1. 导出当前配置
kubectl get configmap coredns -n kube-system -o yaml > coredns-backup.yaml

# 2. 编辑配置
kubectl edit configmap coredns -n kube-system

# 3. 验证配置语法 (使用coredns -validate)
kubectl run coredns-validate --rm -it --image=coredns/coredns:1.11.1 \
  --restart=Never -- -conf /dev/stdin -validate << 'EOF'
.:53 {
    errors
    health
    kubernetes cluster.local in-addr.arpa ip6.arpa
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
}
EOF

# 4. 等待reload (默认30秒) 或手动触发
kubectl rollout restart deployment/coredns -n kube-system

# 5. 验证生效
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50
```

### 5.2 配置版本管理

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
  annotations:
    # 版本追踪
    config.version: "1.2.3"
    config.updated-at: "2026-01-15T10:30:00Z"
    config.updated-by: "ops-team"
    # 变更说明
    config.changelog: |
      v1.2.3: 添加internal.company.com存根域
      v1.2.2: 调整缓存TTL到60秒
      v1.2.1: 初始配置
data:
  Corefile: |
    # ... 配置内容 ...
```

### 5.3 使用Kustomize管理

```yaml
# kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - base/coredns-configmap.yaml

configMapGenerator:
  - name: coredns-custom
    namespace: kube-system
    files:
      - Corefile=overlays/production/Corefile

generatorOptions:
  disableNameSuffixHash: true
```

---

## 六、插件配置速查表 (Plugin Quick Reference)

### 6.1 常用插件配置

| 插件 | 最小配置 | 生产推荐配置 |
|:---|:---|:---|
| **errors** | `errors` | `errors` |
| **health** | `health` | `health { lameduck 5s }` |
| **ready** | `ready` | `ready` |
| **kubernetes** | `kubernetes cluster.local` | `kubernetes cluster.local in-addr.arpa ip6.arpa { pods insecure; fallthrough in-addr.arpa ip6.arpa; ttl 30 }` |
| **forward** | `forward . 8.8.8.8` | `forward . 8.8.8.8 8.8.4.4 { max_concurrent 1000; health_check 5s }` |
| **cache** | `cache 30` | `cache { success 10000 3600 300; denial 1000 60; prefetch 10 1h 10% }` |
| **log** | `log` | `log . "{remote}:{port} {type} {name} {rcode} {duration}"` |
| **prometheus** | `prometheus` | `prometheus :9153` |
| **reload** | `reload` | `reload 30s` |
| **loop** | `loop` | `loop` |
| **loadbalance** | `loadbalance` | `loadbalance round_robin` |

### 6.2 条件配置决策树

```
需要记录DNS查询日志?
├─ 是 → 添加 log 插件
│       └─ 需要详细日志? → 自定义格式
└─ 否 → 跳过

需要监控指标?
├─ 是 → 添加 prometheus 插件
└─ 否 → 跳过

需要服务内部DNS?
├─ 是 → 添加 kubernetes 插件
│       ├─ 需要Pod A记录? → pods insecure/verified
│       ├─ 需要反向解析? → 添加 in-addr.arpa ip6.arpa
│       └─ 需要限制命名空间? → namespaces 选项
└─ 否 → 跳过

需要外部DNS解析?
├─ 是 → 添加 forward 插件
│       ├─ 需要DoT? → 使用 tls:// 前缀
│       ├─ 需要多上游? → 添加多个地址
│       └─ 需要高可用? → 配置health_check
└─ 否 → 跳过

需要缓存?
├─ 是 → 添加 cache 插件
│       ├─ 需要预取? → prefetch选项
│       └─ 需要过期服务? → serve_stale选项
└─ 否 → 跳过

需要自定义域名映射?
├─ 是 → 使用 hosts 或 rewrite 插件
│       ├─ 静态映射 → hosts插件
│       └─ 动态重写 → rewrite插件
└─ 否 → 跳过
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
