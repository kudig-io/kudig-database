# 55 - CoreDNS 插件完整参考 (Plugins Reference)

> **适用版本**: CoreDNS 1.8.0+ / Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、插件分类总览 (Plugin Categories Overview)

### 1.1 插件分类表

| 类别 | 插件数量 | 主要用途 | 核心插件 |
|:---|:---:|:---|:---|
| **数据源** | 8 | 提供DNS记录数据 | kubernetes, file, hosts, etcd |
| **缓存** | 2 | 缓存DNS响应 | cache, nsid |
| **转发/代理** | 3 | 转发查询到上游 | forward, proxy, grpc |
| **修改** | 4 | 修改请求或响应 | rewrite, template, header, metadata |
| **监控** | 4 | 日志/指标/追踪 | log, prometheus, trace, debug |
| **安全** | 4 | 访问控制/加密 | acl, dnssec, tls, dnstap |
| **辅助** | 10+ | 健康检查/错误处理等 | errors, health, ready, loop, reload |

### 1.2 插件启用状态检查

```bash
# 查看CoreDNS编译包含的插件
kubectl exec -n kube-system deploy/coredns -- coredns -plugins

# 常见输出
Server types:
  dns

Caddyfile loaders:
  flag
  default

Other plugins:
  dns.acl
  dns.any
  dns.autopath
  dns.bind
  dns.bufsize
  dns.cache
  dns.cancel
  dns.chaos
  dns.clouddns
  dns.debug
  dns.dns64
  dns.dnssec
  dns.dnstap
  dns.erratic
  dns.errors
  dns.etcd
  dns.file
  dns.forward
  dns.grpc
  dns.header
  dns.health
  dns.hosts
  dns.k8s_external
  dns.kubernetes
  dns.loadbalance
  dns.log
  dns.loop
  dns.metadata
  dns.nsid
  dns.pprof
  dns.prometheus
  dns.ready
  dns.reload
  dns.rewrite
  dns.root
  dns.route53
  dns.secondary
  dns.sign
  dns.template
  dns.tls
  dns.trace
  dns.transfer
  dns.whoami
```

---

## 二、数据源插件 (Data Source Plugins)

### 2.1 kubernetes 插件

**功能**: Kubernetes服务发现核心插件

```
kubernetes [ZONES...] {
    endpoint URL                    # API Server URL
    tls CERT KEY CACERT            # 客户端TLS
    kubeconfig KUBECONFIG CONTEXT  # kubeconfig路径
    namespaces NAMESPACE...         # 命名空间过滤
    labels EXPRESSION               # 标签选择器
    pods POD-MODE                   # disabled|insecure|verified
    endpoint_pod_names              # 使用Pod名称作为endpoint名
    ttl SECONDS                     # 响应TTL
    noendpoints                     # 不返回endpoint
    fallthrough [ZONES...]          # 回退到下一个插件
    ignore empty_service            # 忽略没有endpoint的服务
}
```

**完整配置示例**:
```
kubernetes cluster.local in-addr.arpa ip6.arpa {
    pods verified
    namespaces kube-system default production
    labels environment in (production,staging)
    ttl 60
    fallthrough in-addr.arpa ip6.arpa
    ignore empty_service
}
```

**支持的DNS记录类型**:

| 记录类型 | 场景 | 示例查询 |
|:---|:---|:---|
| **A** | Service ClusterIP | nginx.default.svc.cluster.local |
| **AAAA** | IPv6 ClusterIP | nginx.default.svc.cluster.local |
| **SRV** | 服务端口发现 | _http._tcp.nginx.default.svc.cluster.local |
| **PTR** | 反向解析 | 10.96.0.1.in-addr.arpa |
| **CNAME** | ExternalName服务 | ext.default.svc.cluster.local |

### 2.2 file 插件

**功能**: 从区域文件加载DNS记录

```
file DBFILE [ZONES...] {
    transfer to ADDRESS...         # 允许区域传输的地址
    transfer from ADDRESS...       # 从主服务器拉取区域
    reload DURATION                # 重新加载间隔
    upstream [ADDRESS...]          # 上游解析器
}
```

**区域文件示例** (`db.example.com`):
```
$ORIGIN example.com.
$TTL 3600

@       IN      SOA     ns1.example.com. admin.example.com. (
                        2024010101      ; Serial
                        3600            ; Refresh
                        900             ; Retry
                        604800          ; Expire
                        86400 )         ; Minimum TTL

        IN      NS      ns1.example.com.
        IN      NS      ns2.example.com.

ns1     IN      A       10.0.0.1
ns2     IN      A       10.0.0.2

www     IN      A       10.0.0.10
api     IN      A       10.0.0.11
db      IN      A       10.0.0.20

mail    IN      MX      10 mail1.example.com.
mail1   IN      A       10.0.0.30
```

**Corefile配置**:
```
example.com:53 {
    file /etc/coredns/db.example.com example.com {
        reload 10s
        transfer to *
    }
    errors
    log
}
```

### 2.3 hosts 插件

**功能**: /etc/hosts格式的静态记录

```
hosts [FILE [ZONES...]] {
    INLINE_HOSTS                   # 内联主机记录
    fallthrough [ZONES...]         # 回退
    no_reverse                     # 禁用反向记录
    ttl SECONDS                    # TTL
    reload DURATION                # 重载间隔
}
```

**配置示例**:
```
hosts {
    # 内部服务
    10.0.0.100 api.internal.local api
    10.0.0.101 db.internal.local db
    10.0.0.102 cache.internal.local cache
    
    # 外部服务映射
    203.0.113.50 partner-api.external.local
    
    # IPv6
    2001:db8::1 ipv6-service.internal.local
    
    fallthrough
    ttl 60
    reload 5s
}
```

### 2.4 etcd 插件

**功能**: 从etcd v3存储读取DNS记录

```
etcd [ZONES...] {
    stubzones                      # 启用存根区域
    fallthrough [ZONES...]         # 回退
    path PATH                      # etcd路径前缀
    endpoint ENDPOINT...           # etcd端点
    credentials USERNAME PASSWORD  # 认证
    tls CERT KEY CACERT           # TLS配置
    ttl SECONDS                    # 默认TTL
}
```

**配置示例**:
```
etcd example.com {
    path /skydns
    endpoint https://etcd1:2379 https://etcd2:2379 https://etcd3:2379
    tls /etc/coredns/etcd-client.crt /etc/coredns/etcd-client.key /etc/coredns/etcd-ca.crt
    fallthrough
    ttl 300
}
```

**etcd中的数据格式**:
```json
// Key: /skydns/com/example/www
{
    "host": "10.0.0.10",
    "ttl": 300
}

// Key: /skydns/com/example/api
{
    "host": "10.0.0.11",
    "port": 8080,
    "priority": 10
}
```

### 2.5 auto 插件

**功能**: 自动加载目录中的区域文件

```
auto [ZONES...] {
    directory DIR [REGEXP ORIGIN_TEMPLATE]
    reload DURATION
    no_reload
    upstream [ADDRESS...]
}
```

**配置示例**:
```
auto {
    directory /etc/coredns/zones (.*).db {1}
    reload 10s
}
```

### 2.6 secondary 插件

**功能**: 作为辅助DNS服务器，从主服务器传输区域

```
secondary [ZONES...] {
    transfer from ADDRESS
    transfer to ADDRESS
}
```

### 2.7 route53 插件

**功能**: 从AWS Route 53读取DNS记录

```
route53 [ZONE:HOSTED_ZONE_ID...] {
    credentials PROFILE [FILENAME]
    refresh DURATION
    fallthrough [ZONES...]
    upstream [ADDRESS...]
}
```

### 2.8 clouddns 插件

**功能**: 从Google Cloud DNS读取记录

```
clouddns [ZONE:PROJECT:HOSTED_ZONE_NAME...] {
    credentials FILE
    fallthrough [ZONES...]
    upstream [ADDRESS...]
}
```

---

## 三、缓存插件 (Cache Plugins)

### 3.1 cache 插件

**功能**: DNS响应缓存

```
cache [TTL] [ZONES...] {
    success CAPACITY [TTL] [MINTTL]   # 成功响应缓存
    denial CAPACITY [TTL] [MINTTL]    # 否定响应缓存 (NXDOMAIN)
    prefetch AMOUNT DURATION [PERCENTAGE%]
    serve_stale [DURATION]
    servfail DURATION
    disable success|denial
    keepttl
}
```

**参数详解**:

| 参数 | 默认值 | 说明 | 推荐值 |
|:---|:---|:---|:---|
| TTL | 3600 | 最大缓存时间 | 30-300 |
| success CAPACITY | 9984 | 成功响应缓存条数 | 10000-100000 |
| denial CAPACITY | 9984 | 否定响应缓存条数 | 1000-10000 |
| MINTTL | 5 | 最小TTL | 5-30 |
| prefetch | 禁用 | 预取设置 | `10 1h 10%` |
| serve_stale | 禁用 | 过期缓存服务时间 | `1h` |
| servfail | 5s | SERVFAIL缓存时间 | 5s |

**生产配置示例**:
```
cache {
    success 50000 3600 60      # 5万条成功缓存,最大1小时,最小1分钟
    denial 5000 600 30         # 5千条否定缓存,最大10分钟
    prefetch 10 1h 20%         # 剩余20%TTL时预取
    serve_stale 2h             # 上游故障时服务2小时过期缓存
    servfail 5s                # SERVFAIL缓存5秒
}
```

### 3.2 nsid 插件

**功能**: 添加NSID (Name Server Identifier) EDNS0选项

```
nsid [DATA]
```

**配置示例**:
```
nsid coredns-pod-1
```

---

## 四、转发插件 (Forwarding Plugins)

### 4.1 forward 插件

**功能**: 转发DNS查询到上游服务器

```
forward FROM TO... {
    except IGNORED_NAMES...
    force_tcp
    prefer_udp
    expire DURATION
    max_fails INTEGER
    tls CERT KEY CA
    tls_servername NAME
    policy random|round_robin|sequential
    health_check DURATION [no_rec]
    max_concurrent INTEGER
}
```

**策略对比**:

| 策略 | 说明 | 适用场景 |
|:---|:---|:---|
| random | 随机选择 | 通用场景 |
| round_robin | 轮询 | 负载均衡 |
| sequential | 顺序(失败才下一个) | 主备模式 |

**多上游配置示例**:
```
# 公共DNS (负载均衡)
forward . 8.8.8.8 8.8.4.4 1.1.1.1 {
    max_concurrent 1000
    policy round_robin
    health_check 5s
    max_fails 3
}

# DoT上游
forward . tls://8.8.8.8 tls://8.8.4.4 {
    tls_servername dns.google
    health_check 10s
}

# 内部DNS (顺序)
forward internal.company.com 10.0.0.53 10.0.0.54 {
    policy sequential
    max_fails 2
}
```

### 4.2 grpc 插件

**功能**: 通过gRPC转发DNS查询

```
grpc FROM TO... {
    tls CERT KEY CA
    tls_servername NAME
    policy random|round_robin|sequential
}
```

### 4.3 alternate 插件

**功能**: 基于响应码切换上游

```
alternate RCODE FROM TO... {
    same options as forward
}
```

**配置示例**:
```
# 当主DNS返回SERVFAIL或REFUSED时使用备用DNS
forward . 10.0.0.53 {
    max_concurrent 1000
}
alternate SERVFAIL,REFUSED . 8.8.8.8 8.8.4.4
```

---

## 五、修改插件 (Modification Plugins)

### 5.1 rewrite 插件

**功能**: 重写DNS请求或响应

```
rewrite [continue|stop] FIELD FROM TO [OPTIONS]
```

**FIELD类型**:

| FIELD | 说明 | 示例 |
|:---|:---|:---|
| name | 查询名称 | `rewrite name exact old.com new.com` |
| type | 查询类型 | `rewrite type AAAA A` |
| class | 查询类别 | `rewrite class CH IN` |
| edns0 | EDNS0选项 | 复杂配置 |
| ttl | 响应TTL | `rewrite ttl exact . 60` |

**FROM匹配模式**:

| 模式 | 语法 | 示例 |
|:---|:---|:---|
| exact | `exact:name` | `exact:old.example.com` |
| prefix | `prefix:name` | `prefix:old.` |
| suffix | `suffix:name` | `suffix:.old.com` |
| substring | `substring:name` | `substring:old` |
| regex | `regex:pattern` | `regex:(.*)\.old\.com$` |

**完整示例集**:
```
# 1. 精确域名重写
rewrite name exact legacy-db.default.svc.cluster.local new-db.default.svc.cluster.local

# 2. 后缀重写
rewrite name suffix .old.internal .new.internal

# 3. 正则重写 (保留子域名)
rewrite name regex (.*)\.old\.example\.com {1}.new.example.com

# 4. 类型重写 (AAAA转A)
rewrite type AAAA A

# 5. 响应重写
rewrite stop {
    name regex (.*)\.internal\.local
    answer name (.*)\.internal\.local {1}.external.local
    answer value 10\.0\.0\.(\d+) 192.168.0.{1}
}

# 6. TTL重写
rewrite ttl regex .* 60
```

### 5.2 template 插件

**功能**: 基于模板生成DNS响应

```
template CLASS TYPE [ZONE...] {
    match REGEX...
    answer RR
    additional RR
    authority RR
    rcode CODE
    fallthrough [ZONES...]
}
```

**配置示例**:
```
# 1. 阻止域名 (返回0.0.0.0)
template IN A blocked.local {
    match .*\.blocked\.local$
    answer "{{ .Name }} 60 IN A 0.0.0.0"
}

# 2. 动态生成PTR记录
template IN PTR 10.in-addr.arpa {
    match ^(\d+)\.(\d+)\.(\d+)\.10\.in-addr\.arpa$
    answer "{{ .Name }} 60 IN PTR host-{{ .Group.3 }}-{{ .Group.2 }}-{{ .Group.1 }}.internal.local."
}

# 3. 返回NXDOMAIN
template ANY ANY forbidden.example.com {
    rcode NXDOMAIN
    authority "forbidden.example.com. 60 IN SOA ns.example.com. admin.example.com. 1 3600 300 86400 60"
}
```

### 5.3 header 插件

**功能**: 修改DNS消息头部

```
header {
    response FIELD VALUE
    request FIELD VALUE
}
```

### 5.4 metadata 插件

**功能**: 在请求上下文中添加元数据

```
metadata
```

---

## 六、监控插件 (Monitoring Plugins)

### 6.1 log 插件

**功能**: 查询日志记录

```
log [NAME] [FORMAT]
```

**格式变量**:

| 变量 | 说明 | 示例值 |
|:---|:---|:---|
| `{type}` | 查询类型 | A, AAAA, SRV |
| `{name}` | 查询域名 | nginx.default.svc.cluster.local |
| `{class}` | 查询类别 | IN |
| `{proto}` | 协议 | udp, tcp |
| `{remote}` | 客户端IP | 10.244.1.5 |
| `{port}` | 客户端端口 | 53482 |
| `{size}` | 请求大小 | 45 |
| `{rcode}` | 响应码 | NOERROR, NXDOMAIN |
| `{rsize}` | 响应大小 | 128 |
| `{duration}` | 处理时长 | 0.5ms |
| `{>id}` | 请求ID | 12345 |
| `{>opcode}` | 操作码 | QUERY |
| `{server_ip}` | 服务器IP | 10.96.0.10 |

**配置示例**:
```
# 默认格式
log

# 详细格式
log . "{remote}:{port} - [{time}] {>id} \"{type} {class} {name} {proto} {size}\" {rcode} {rsize} {duration}"

# 仅记录错误
log . {
    class error
}

# 特定域名
log cluster.local
```

### 6.2 prometheus 插件

**功能**: 暴露Prometheus指标

```
prometheus [ADDRESS]
```

**暴露的指标**:

| 指标 | 类型 | 说明 |
|:---|:---|:---|
| coredns_dns_requests_total | Counter | 总请求数 |
| coredns_dns_responses_total | Counter | 总响应数(按rcode) |
| coredns_dns_request_duration_seconds | Histogram | 请求延迟 |
| coredns_dns_request_size_bytes | Histogram | 请求大小 |
| coredns_dns_response_size_bytes | Histogram | 响应大小 |
| coredns_cache_hits_total | Counter | 缓存命中 |
| coredns_cache_misses_total | Counter | 缓存未命中 |
| coredns_cache_size | Gauge | 缓存条目数 |
| coredns_forward_requests_total | Counter | 转发请求数 |
| coredns_forward_request_duration_seconds | Histogram | 转发延迟 |

### 6.3 trace 插件

**功能**: 分布式追踪(OpenTelemetry)

```
trace [ENDPOINT] {
    every FRACTION
    service NAME
    client_server
    datadog_analytics_rate RATE
}
```

**配置示例**:
```
trace zipkin:9411 {
    every 100        # 每100个请求采样1个
    service coredns
    client_server
}
```

### 6.4 debug 插件

**功能**: 启用调试模式

```
debug
```

### 6.5 pprof 插件

**功能**: 暴露Go pprof端点

```
pprof [ADDRESS]
```

---

## 七、安全插件 (Security Plugins)

### 7.1 acl 插件

**功能**: 访问控制列表

```
acl [ZONES...] {
    allow|deny|block|filter net CIDR...
    allow|deny|block|filter net CIDR... {
        CLASS TYPE [CLASS TYPE...]
    }
}
```

**动作说明**:

| 动作 | 说明 |
|:---|:---|
| allow | 允许查询 |
| deny | 拒绝(返回REFUSED) |
| block | 阻止(返回NXDOMAIN) |
| filter | 过滤响应中的记录 |

**配置示例**:
```
acl {
    # 允许内部网络
    allow net 10.0.0.0/8
    allow net 172.16.0.0/12
    allow net 192.168.0.0/16
    
    # 允许Pod CIDR
    allow net 10.244.0.0/16
    
    # 拒绝其他
    block net *
}

# 按类型控制
acl cluster.local {
    allow net 10.0.0.0/8 {
        IN A
        IN AAAA
        IN SRV
    }
    deny net * {
        IN AXFR    # 禁止区域传输
        IN ANY     # 禁止ANY查询
    }
}
```

### 7.2 dnssec 插件

**功能**: DNSSEC签名

```
dnssec [ZONES...] {
    key file KEY...
    cache_capacity CAPACITY
}
```

### 7.3 dnstap 插件

**功能**: DNS tap日志(详细流量记录)

```
dnstap ENDPOINT {
    identity IDENTITY
    version VERSION
    extra EXTRA
}
```

**配置示例**:
```
dnstap tcp://dnstap-collector:6000 {
    identity coredns-cluster-1
    version 1.11.1
}
```

---

## 八、辅助插件 (Helper Plugins)

### 8.1 errors 插件

**功能**: 错误日志输出

```
errors [FILE]
```

### 8.2 health 插件

**功能**: 健康检查端点

```
health [ADDRESS] {
    lameduck DURATION
}
```

**配置示例**:
```
health :8080 {
    lameduck 5s    # 关闭前等待5秒
}
```

### 8.3 ready 插件

**功能**: 就绪探针端点

```
ready [ADDRESS]
```

### 8.4 loop 插件

**功能**: 转发循环检测

```
loop
```

### 8.5 reload 插件

**功能**: 配置热重载

```
reload [INTERVAL] [JITTER]
```

**配置示例**:
```
reload 30s 15s    # 每30秒检查,随机延迟0-15秒
```

### 8.6 loadbalance 插件

**功能**: A/AAAA记录轮询

```
loadbalance [round_robin|weighted]
```

### 8.7 bufsize 插件

**功能**: 设置EDNS0缓冲区大小

```
bufsize SIZE
```

### 8.8 bind 插件

**功能**: 绑定特定IP地址

```
bind ADDRESS...
```

**配置示例**:
```
bind 10.0.0.10 127.0.0.1
```

### 8.9 chaos 插件

**功能**: 响应CH类查询(版本信息等)

```
chaos [VERSION] [AUTHORS...]
```

### 8.10 any 插件

**功能**: 处理ANY类型查询

```
any
```

---

## 九、插件优先级与顺序 (Plugin Order)

### 9.1 编译时顺序

CoreDNS插件按照编译时定义的顺序执行，以下是官方推荐顺序：

```
1.  metadata
2.  cancel
3.  tls
4.  reload
5.  nsid
6.  bufsize
7.  root
8.  bind
9.  debug
10. trace
11. ready
12. health
13. pprof
14. prometheus
15. errors
16. log
17. dnstap
18. local
19. dns64
20. acl
21. any
22. chaos
23. loadbalance
24. tsig
25. cache
26. rewrite
27. header
28. dnssec
29. autopath
30. minimal
31. template
32. transfer
33. hosts
34. route53
35. clouddns
36. k8s_external
37. kubernetes
38. file
39. auto
40. secondary
41. etcd
42. loop
43. forward
44. grpc
45. erratic
46. whoami
47. on
48. sign
49. view
50. geoip
```

### 9.2 实践中的推荐顺序

```
.:53 {
    # 1. 辅助插件 (最先)
    errors
    log
    debug              # 仅调试时
    
    # 2. 监控插件
    prometheus :9153
    
    # 3. 健康检查
    health {
        lameduck 5s
    }
    ready
    
    # 4. 安全插件
    acl {
        allow net 10.0.0.0/8
        block net *
    }
    
    # 5. 循环检测
    loop
    
    # 6. 修改插件 (在缓存之前)
    rewrite name suffix .old.local .new.local
    
    # 7. 缓存 (在数据源之前!)
    cache 30
    
    # 8. 数据源插件
    hosts /etc/coredns/hosts {
        fallthrough
    }
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
        pods insecure
        fallthrough in-addr.arpa ip6.arpa
    }
    
    # 9. 转发插件 (最后)
    forward . /etc/resolv.conf {
        max_concurrent 1000
    }
    
    # 10. 配置管理
    reload
    loadbalance
}
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
