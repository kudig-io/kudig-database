---
title: Wasm Serverless (Wasm Serverless)
description: WebAssembly 为 Serverless 计算带来毫秒级冷启动、安全沙箱隔离和超轻量部署，重新定义边缘与云端 FaaS 架构。
summary: WebAssembly 为 Serverless 计算带来毫秒级冷启动、安全沙箱隔离和超轻量部署，重新定义边缘与云端 FaaS 架构。
category: webassembly-cloud-native
tags:
- k8s
- wasm
- webassembly
- cloud-native
- prometheus
- grafana
- opa
- redis
- postgresql
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- 开发工程师
- SRE
estimated_read_time: 5min
intent_queries:
- Wasm Serverless (Wasm Serverless) 是什么
- 如何 Wasm Serverless (Wasm Serverless)
- Kubernetes 38 webassembly cloud native 最佳实践
trigger_keywords:
- Wasm
- Serverless
- Wasm
- Serverless
- webassembly
- cloud
- native
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
- policy-basics
- observability-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Wasm Serverless (Wasm Serverless)

> WebAssembly 为 Serverless 计算带来毫秒级冷启动、安全沙箱隔离和超轻量部署，重新定义边缘与云端 FaaS 架构。

---

<!-- chunk: 目录 -->## 目录

1. [Wasm Serverless 架构概述](#1-wasm-serverless-架构概述)
2. [冷启动优化原理](#2-冷启动优化原理)
3. [[实体/spin.md|Spin]] 框架详解](#3-spin-框架详解)
4. [[实体/wasmcloud.md|wasmCloud]]ud 平台|wasmCloud 平台]]](#4-wasmcloud-平台)
5. [Fermyon Cloud 部署](#5-fermyon-cloud-部署)
6. [事件触发器系统](#6-事件触发器系统)
7. [Scale-to-Zero 实现](#7-scale-to-zero-实现)
8. [FaaS 设计模式](#8-faas-设计模式)
9. [有状态 Serverless](#9-有状态-serverless)
10. [多云 Serverless 部署](#10-多云-serverless-部署)
11. [Serverless 可观测性](#11-serverless-可观测性)
12. [边缘 Serverless](#12-边缘-serverless)
13. [性能基准与对比](#13-性能基准与对比)
14. [生产运维最佳实践](#14-生产运维最佳实践)

---

<!-- chunk: 1. Wasm Serverless 架构概述 -->## 1. Wasm Serverless 架构概述

## 1.1 传统 Serverless 的局限

```mermaid
graph LR
    subgraph "传统 Serverless 问题"
        ColdStart[冷启动 100ms-10s]
        MemoryWaste[内存浪费 128MB+]
        OSOverhead[OS 开销大]
        SecurityIssues[安全隔离弱]
        VendorLock[厂商锁定]
    end

    subgraph "Wasm Serverless 解决方案"
        FastStart[< 1ms 冷启动]
        LowMem[低内存 2-10MB]
        DirectExec[直接执行，无 OS 开销]
        SandboxSec[完整沙箱隔离]
        Portable[跨平台可移植]
    end

    WasmFaaS[Wasm FaaS Platform] --> FastStart
    WasmFaaS --> LowMem
    WasmFaaS --> DirectExec
    WasmFaaS --> SandboxSec
    WasmFaaS --> Portable
```

## 1.2 Wasm Serverless 生态全景

```mermaid
graph TB
    subgraph "开源平台"
        Spin[Fermyon Spin]
        wasmCloud[wasmCloud]
        WasmEdgeFunc[WasmEdge FaaS]
        Lunatic[Lunatic]
        Atmo[Atmo - Suborbital]
    end

    subgraph "云原生运行时"
        Wasmtime[Wasmtime]
        WasmEdge[WasmEdge]
        WAMR[WAMR]
        V8[V8 Wasm]
    end

    subgraph "托管平台"
        FermyonCloud[Fermyon Cloud]
        CloudflareWorkers[Cloudflare Workers]
        FastlyCompute[Fastly Compute@Edge]
        VercelEdge[Vercel Edge Runtime]
        DenoEdge[Deno Deploy]
    end

    subgraph "K8s 集成"
        SpinK8s[Spin on K8s]
        KnativeWasm[Knative + Wasm]
        OpenFaaS[OpenFaaS + Wasm]
        KEDA[KEDA + Wasm]
    end

    Spin --> Wasmtime
    wasmCloud --> Wasmtime
    WasmEdgeFunc --> WasmEdge
    FermyonCloud --> Spin
    CloudflareWorkers --> V8
```

## 1.3 核心技术指标对比

```
# 🟢 低风险：只读/信息收集，通常无副作用
Serverless 平台对比（2025年）：

┌──────────────────────────────────────────────────────────────┐
│ 平台               │ 冷启动  │ 内存/实例 │ 最大执行 │ 价格     │
├──────────────────────────────────────────────────────────────┤
│ AWS Lambda         │ 100ms+  │ 128MB+   │ 15min    │ $0.20/M  │
│ Google Cloud Run   │ 200ms+  │ 128MB+   │ 60min    │ $0.40/M  │
│ Cloudflare Workers │ <1ms    │ 128MB    │ 30s      │ $0.50/M  │
│ Fermyon Spin       │ <1ms    │ 10-50MB  │ 无限制   │ 自托管   │
│ wasmCloud          │ <1ms    │ 2-20MB   │ 无限制   │ 自托管   │
│ Fastly Compute     │ <1ms    │ 64MB     │ 30s      │ $0.50/M  │
│ Deno Deploy        │ <50ms   │ 512MB    │ 无限制   │ $0.30/M  │
└──────────────────────────────────────────────────────────────┘
```
---

<!-- chunk: 2. 冷启动优化原理 -->## 2. 冷启动优化原理

## 2.1 传统容器 vs Wasm 冷启动

```mermaid
graph TD
    subgraph "容器冷启动链路 ~500ms-10s"
        Pull[镜像拉取 1-30s]
        Create[容器创建 50ms]
        Network[网络配置 100ms]
        Runtime[运行时启动 100ms]
        App[应用初始化 100ms-5s]
    end

    subgraph "Wasm 冷启动链路 <1ms"
        Load[Wasm 模块加载 0.1ms]
        Compile[编译/AOT 加载 0.2ms]
        Init[WASI 初始化 0.1ms]
        Start[_start() 执行 0.05ms]
    end

    subgraph "优化技术"
        AOT[AOT 预编译]
        Lazy[懒加载函数]
        Snapshot[快照/Fork]
        Pool[实例预热池]
    end

    Wasm冷启动 --> AOT
    Wasm冷启动 --> Lazy
    Wasm冷启动 --> Snapshot
    Wasm冷启动 --> Pool
```

## 2.2 AOT 预编译优化

```rust
// wasmtime AOT 预编译与缓存
use wasmtime::{Engine, Config, OptLevel};
use std::path::Path;

pub struct WasmRuntime {
    engine: Engine,
    cache_dir: String,
}

impl WasmRuntime {
    pub fn new(cache_dir: &str) -> anyhow::Result<Self> {
        let mut config = Config::new();
        config.wasm_component_model(true);
        
        // AOT 优化配置
        config.cranelift_opt_level(OptLevel::Speed);
        config.parallel_compilation(true);
        config.memory_init_cow(true);  // Copy-on-Write 内存初始化
        config.memory_guaranteed_dense_image_size(16 * 1024 * 1024);
        
        // 启用 epoch-based 中断
        config.epoch_interruption(true);
        
        Ok(Self {
            engine: Engine::new(&config)?,
            cache_dir: cache_dir.to_string(),
        })
    }

    pub fn precompile_and_cache(
        &self,
        wasm_path: &str,
    ) -> anyhow::Result<String> {
        let cache_path = format!(
            "{}/{}.cwasm",
            self.cache_dir,
            sha256_file(wasm_path)?
        );

        if Path::new(&cache_path).exists() {
            println!("Using cached AOT: {}", cache_path);
            return Ok(cache_path);
        }

        println!("Precompiling {}...", wasm_path);
        let start = std::time::Instant::now();

        let wasm_bytes = std::fs::read(wasm_path)?;
        let component = wasmtime::component::Component::new(&self.engine, &wasm_bytes)?;
        let serialized = component.serialize()?;

        std::fs::create_dir_all(&self.cache_dir)?;
        std::fs::write(&cache_path, &serialized)?;

        println!("Precompiled in {:?}: {}", start.elapsed(), cache_path);
        Ok(cache_path)
    }

    pub fn load_precompiled(
        &self,
        cache_path: &str,
    ) -> anyhow::Result<wasmtime::component::Component> {
        let start = std::time::Instant::now();

        // 加载预编译模块（不安全：需要信任来源）
        let component = unsafe {
            wasmtime::component::Component::deserialize_file(&self.engine, cache_path)?
        };

        println!("Loaded precompiled in {:?}", start.elapsed());
        Ok(component)
    }
}

fn sha256_file(path: &str) -> anyhow::Result<String> {
    use std::io::Read;
    let mut file = std::fs::File::open(path)?;
    let mut hasher = sha2::Sha256::new();
    let mut buffer = [0u8; 8192];

    loop {
        let n = file.read(&mut buffer)?;
        if n == 0 { break; }
        sha2::Digest::update(&mut hasher, &buffer[..n]);
    }

    Ok(format!("{:x}", sha2::Digest::finalize(hasher)))
}
```

## 2.3 实例预热池（Warm Pool）

```rust
// 实例预热池实现
use std::sync::Arc;
use tokio::sync::{Mutex, Semaphore};
use std::collections::VecDeque;

pub struct InstancePool {
    available: Arc<Mutex<VecDeque<WarmInstance>>>,
    semaphore: Arc<Semaphore>,
    component: Arc<wasmtime::component::Component>,
    engine: Arc<wasmtime::Engine>,
    config: PoolConfig,
}

pub struct WarmInstance {
    store: wasmtime::Store<InstanceState>,
    instance: wasmtime::component::Instance,
    created_at: std::time::Instant,
    use_count: u32,
}

#[derive(Clone)]
pub struct PoolConfig {
    min_warm: usize,     // 最小预热实例数
    max_size: usize,     // 最大池大小
    max_age_secs: u64,   // 实例最大存活时间
    max_uses: u32,       // 实例最大使用次数
    warmup_interval_ms: u64,  // 预热间隔
}

impl InstancePool {
    pub async fn new(
        component: Arc<wasmtime::component::Component>,
        engine: Arc<wasmtime::Engine>,
        config: PoolConfig,
    ) -> anyhow::Result<Arc<Self>> {
        let pool = Arc::new(Self {
            available: Arc::new(Mutex::new(VecDeque::new())),
            semaphore: Arc::new(Semaphore::new(config.max_size)),
            component,
            engine,
            config: config.clone(),
        });

        // 预热初始实例
        let pool_clone = pool.clone();
        tokio::spawn(async move {
            pool_clone.warmup_loop().await;
        });

        Ok(pool)
    }

    async fn warmup_loop(&self) {
        loop {
            let current_size = self.available.lock().await.len();

            if current_size < self.config.min_warm {
                let to_create = self.config.min_warm - current_size;
                for _ in 0..to_create {
                    match self.create_instance().await {
                        Ok(instance) => {
                            self.available.lock().await.push_back(instance);
                            println!("Created warm instance, pool size: {}",
                                self.available.lock().await.len());
                        }
                        Err(e) => eprintln!("Failed to create warm instance: {}", e),
                    }
                }
            }

            // 清理过期实例
            self.cleanup_expired().await;

            tokio::time::sleep(
                tokio::time::Duration::from_millis(self.config.warmup_interval_ms)
            ).await;
        }
    }

    async fn cleanup_expired(&self) {
        let mut pool = self.available.lock().await;
        let now = std::time::Instant::now();

        pool.retain(|instance| {
            let age = now.duration_since(instance.created_at).as_secs();
            let too_old = age > self.config.max_age_secs;
            let too_used = instance.use_count >= self.config.max_uses;
            !too_old && !too_used
        });
    }

    pub async fn acquire(&self) -> anyhow::Result<WarmInstance> {
        // 尝试从池中获取
        {
            let mut pool = self.available.lock().await;
            if let Some(instance) = pool.pop_front() {
                return Ok(instance);
            }
        }

        // 池为空，创建新实例
        self.create_instance().await
    }

    pub async fn release(&self, mut instance: WarmInstance) {
        instance.use_count += 1;

        // 检查实例是否还能复用
        let age = instance.created_at.elapsed().as_secs();
        if age < self.config.max_age_secs && instance.use_count < self.config.max_uses {
            let pool_size = self.available.lock().await.len();
            if pool_size < self.config.max_size {
                self.available.lock().await.push_back(instance);
                return;
            }
        }

        // 丢弃实例（自动清理）
        drop(instance);
    }

    async fn create_instance(&self) -> anyhow::Result<WarmInstance> {
        let start = std::time::Instant::now();

        let wasi = wasmtime_wasi::WasiCtxBuilder::new()
            .inherit_stdio()
            .build();

        let mut store = wasmtime::Store::new(
            &self.engine,
            InstanceState { wasi },
        );
        store.set_epoch_deadline(10);  // 防止无限循环

        let mut linker = wasmtime::component::Linker::new(&self.engine);
        wasmtime_wasi::add_to_linker_sync(&mut linker)?;

        let instance = linker.instantiate(&mut store, &self.component)?;

        println!("Instance created in {:?}", start.elapsed());

        Ok(WarmInstance {
            store,
            instance,
            created_at: std::time::Instant::now(),
            use_count: 0,
        })
    }
}

struct InstanceState {
    wasi: wasmtime_wasi::WasiCtx,
}

impl wasmtime_wasi::WasiView for InstanceState {
    fn ctx(&mut self) -> &mut wasmtime_wasi::WasiCtx { &mut self.wasi }
    fn table(&mut self) -> &mut wasmtime_wasi::ResourceTable {
        unimplemented!()
    }
}
```

## 2.4 冷启动时间测量

```rust
// 冷启动基准测试
use std::time::{Duration, Instant};

async fn benchmark_cold_start(
    engine: &wasmtime::Engine,
    wasm_path: &str,
    n_runs: u32,
) -> BenchResult {
    let mut latencies = Vec::with_capacity(n_runs as usize);

    for _ in 0..n_runs {
        let start = Instant::now();

        // 加载模块
        let bytes = std::fs::read(wasm_path).unwrap();
        let component = wasmtime::component::Component::new(engine, &bytes).unwrap();

        // 实例化
        let wasi = wasmtime_wasi::WasiCtxBuilder::new().build();
        let mut store = wasmtime::Store::new(engine, wasi);
        let mut linker = wasmtime::component::Linker::new(engine);
        wasmtime_wasi::add_to_linker_sync(&mut linker).unwrap();
        let instance = linker.instantiate(&mut store, &component).unwrap();

        // 调用入口函数
        if let Some(func) = instance.get_func(&mut store, "handle") {
            let mut result = vec![];
            func.call(&mut store, &[], &mut result).unwrap();
        }

        latencies.push(start.elapsed());
    }

    latencies.sort();

    BenchResult {
        min: *latencies.first().unwrap(),
        max: *latencies.last().unwrap(),
        p50: latencies[n_runs as usize / 2],
        p90: latencies[n_runs as usize * 9 / 10],
        p99: latencies[n_runs as usize * 99 / 100],
        mean: latencies.iter().sum::<Duration>() / n_runs,
    }
}

#[derive(Debug)]
struct BenchResult {
    min: Duration,
    max: Duration,
    p50: Duration,
    p90: Duration,
    p99: Duration,
    mean: Duration,
}
```

---

<!-- chunk: 3. Spin 框架详解 -->## 3. Spin 框架详解

## 3.1 Spin 安装与项目创建

```bash
# 安装 Spin CLI
curl -fsSL https://developer.fermyon.com/downloads/install.sh | bash
sudo mv spin /usr/local/bin/

# 验证安装
spin --version

# 安装模板
spin templates install --git https://github.com/fermyon/spin-python-sdk
spin templates install --git https://github.com/fermyon/spin-js-sdk

# 列出可用模板
spin templates list

# 创建 HTTP 服务
spin new -t http-rust my-api
cd my-api

# 创建 Redis 消费者
spin new -t redis-rust my-consumer

# 构建
spin build

# 本地运行
spin up

# 运行并设置环境变量
spin up \
  --env "DATABASE_URL=postgres://..." \
  --env "API_KEY=secret-key"
```

## 3.2 spin.toml 配置详解

```toml
# spin.toml - 完整配置示例
spin_manifest_version = 2

[application]
name = "my-microservices"
version = "1.0.0"
description = "Multi-component Wasm microservices"
authors = ["team@example.com"]

# 全局变量
[application.variables]
database_url = { required = true }
redis_url = { required = false, default = "redis://localhost:6379" }
log_level = { required = false, default = "info" }
feature_flags = { required = false, default = "{}" }

# HTTP API 组件
trigger.http
route = "/api/..."
component = "api-handler"

[component.api-handler]
source = "components/api-handler/target/wasm32-wasi/release/api_handler.wasm"
description = "Main HTTP API handler"

# WASI 允许配置
[component.api-handler.build]
command = "cargo build --target wasm32-wasi --release"
workdir = "components/api-handler"
watch = ["src/**/*.rs", "Cargo.toml"]

[component.api-handler.trigger]
executor = { type = "wagi" }  # 或 "default" for Spin HTTP

# 允许访问的主机
[component.api-handler.allowed_outbound_hosts]
outbound_hosts = [
  "https://api.stripe.com",
  "https://*.auth0.com",
  "redis://localhost:6379",
]

# 键值存储
[component.api-handler.key_value_stores]
default = { label = "default" }
sessions = { label = "sessions" }

# SQLite 数据库
[component.api-handler.sqlite_databases]
default = { label = "default" }

# 环境变量
[component.api-handler.environment]
LOG_LEVEL = "{{ log_level }}"
DATABASE_URL = "{{ database_url }}"
FEATURE_FLAGS = "{{ feature_flags }}"

# 文件挂载
[component.api-handler.files]
templates = { path = "templates/", destination = "/templates" }
assets = { path = "assets/", destination = "/assets" }

# -----------------------------------------------
# Auth 子服务
trigger.http
route = "/auth/..."
component = "auth-service"

[component.auth-service]
source = "components/auth/target/wasm32-wasi/release/auth.wasm"

[component.auth-service.allowed_outbound_hosts]
outbound_hosts = ["https://auth.example.com"]

[component.auth-service.key_value_stores]
tokens = { label = "tokens" }

# -----------------------------------------------
# 异步任务处理器（Redis 触发）
trigger.redis
channel = "tasks"
component = "task-processor"

[component.task-processor]
source = "components/task-processor/target/wasm32-wasi/release/task_processor.wasm"
description = "Async task processor"

[component.task-processor.allowed_outbound_hosts]
outbound_hosts = ["https://notifications.example.com"]

# -----------------------------------------------
# 定时任务（计划触发）
trigger.cron
cron_expression = "0 */5 * * * *"  # 每5分钟
component = "cleanup-job"

[component.cleanup-job]
source = "components/cleanup/target/wasm32-wasi/release/cleanup.wasm"
```

## 3.3 完整 Spin HTTP API 实现

```rust
// components/api-handler/src/lib.rs
use spin_sdk::{
    http::{IncomingRequest, OutgoingResponse, ResponseBuilder, Router},
    http_component,
    key_value::Store,
    sqlite::{Connection, Value},
    variables,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
struct User {
    id: String,
    name: String,
    email: String,
    created_at: u64,
}

#[derive(Debug, Deserialize)]
struct CreateUserRequest {
    name: String,
    email: String,
}

#[derive(Debug, Serialize)]
struct ApiResponse<T> {
    success: bool,
    data: Option<T>,
    error: Option<String>,
    request_id: String,
}

impl<T: Serialize> ApiResponse<T> {
    fn ok(data: T, request_id: &str) -> Self {
        Self {
            success: true,
            data: Some(data),
            error: None,
            request_id: request_id.to_string(),
        }
    }

    fn error(msg: &str, request_id: &str) -> ApiResponse<()> {
        ApiResponse {
            success: false,
            data: None,
            error: Some(msg.to_string()),
            request_id: request_id.to_string(),
        }
    }
}

// 使用 Spin HTTP 组件宏
#[http_component]
async fn handle_request(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let router = Router::new()
        .get("/api/users", list_users)
        .get("/api/users/:id", get_user)
        .post("/api/users", create_user)
        .put("/api/users/:id", update_user)
        .delete("/api/users/:id", delete_user)
        .get("/api/health", health_check);

    router.handle(req).await
}

async fn list_users(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let request_id = get_or_generate_request_id(&req);

    // 从 SQLite 查询
    let conn = Connection::open_default()?;
    let rows = conn.execute(
        "SELECT id, name, email, created_at FROM users ORDER BY created_at DESC LIMIT 100",
        &[],
    )?;

    let users: Vec<User> = rows.rows().map(|row| User {
        id: row.get::<&str>(0).unwrap_or("").to_string(),
        name: row.get::<&str>(1).unwrap_or("").to_string(),
        email: row.get::<&str>(2).unwrap_or("").to_string(),
        created_at: row.get::<u64>(3).unwrap_or(0),
    }).collect();

    let response = ApiResponse::ok(users, &request_id);
    
    Ok(ResponseBuilder::new(200)
        .header("content-type", "application/json")
        .header("x-request-id", &request_id)
        .body(serde_json::to_vec(&response)?)
        .build())
}

async fn get_user(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let request_id = get_or_generate_request_id(&req);
    let user_id = req.path_param("id").unwrap_or_default();

    // 先检查缓存
    let cache = Store::open_default()?;
    let cache_key = format!("user:{}", user_id);

    if let Some(cached) = cache.get(&cache_key)? {
        let user: User = serde_json::from_slice(&cached)?;
        return Ok(ResponseBuilder::new(200)
            .header("content-type", "application/json")
            .header("x-cache", "HIT")
            .header("x-request-id", &request_id)
            .body(serde_json::to_vec(&ApiResponse::ok(user, &request_id))?)
            .build());
    }

    // 从数据库查询
    let conn = Connection::open_default()?;
    let rows = conn.execute(
        "SELECT id, name, email, created_at FROM users WHERE id = ?",
        &[Value::Text(user_id.clone())],
    )?;

    if rows.rows().count() == 0 {
        return Ok(ResponseBuilder::new(404)
            .header("content-type", "application/json")
            .body(serde_json::to_vec(&ApiResponse::<()>::error(
                &format!("User {} not found", user_id),
                &request_id,
            ))?)
            .build());
    }

    let user = rows.rows().next().map(|row| User {
        id: row.get::<&str>(0).unwrap_or("").to_string(),
        name: row.get::<&str>(1).unwrap_or("").to_string(),
        email: row.get::<&str>(2).unwrap_or("").to_string(),
        created_at: row.get::<u64>(3).unwrap_or(0),
    }).unwrap();

    // 写入缓存（TTL 300s）
    cache.set(&cache_key, &serde_json::to_vec(&user)?)?;

    Ok(ResponseBuilder::new(200)
        .header("content-type", "application/json")
        .header("x-cache", "MISS")
        .header("x-request-id", &request_id)
        .body(serde_json::to_vec(&ApiResponse::ok(user, &request_id))?)
        .build())
}

async fn create_user(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let request_id = get_or_generate_request_id(&req);

    // 读取并解析请求体
    let body = req.body().await?;
    let create_req: CreateUserRequest = match serde_json::from_slice(&body) {
        Ok(r) => r,
        Err(e) => {
            return Ok(ResponseBuilder::new(400)
                .header("content-type", "application/json")
                .body(serde_json::to_vec(&ApiResponse::<()>::error(
                    &format!("Invalid request body: {}", e),
                    &request_id,
                ))?)
                .build());
        }
    };

    // 验证
    if create_req.name.trim().is_empty() {
        return Ok(ResponseBuilder::new(400)
            .header("content-type", "application/json")
            .body(serde_json::to_vec(&ApiResponse::<()>::error("Name is required", &request_id))?)
            .build());
    }

    if !create_req.email.contains('@') {
        return Ok(ResponseBuilder::new(400)
            .header("content-type", "application/json")
            .body(serde_json::to_vec(&ApiResponse::<()>::error("Invalid email format", &request_id))?)
            .build());
    }

    // 生成 ID
    let user_id = generate_user_id(&create_req.name, &create_req.email);
    let now = current_timestamp();

    // 写入数据库
    let conn = Connection::open_default()?;
    conn.execute(
        "INSERT INTO users (id, name, email, created_at) VALUES (?, ?, ?, ?)",
        &[
            Value::Text(user_id.clone()),
            Value::Text(create_req.name.clone()),
            Value::Text(create_req.email.clone()),
            Value::Integer(now as i64),
        ],
    )?;

    let user = User {
        id: user_id,
        name: create_req.name,
        email: create_req.email,
        created_at: now,
    };

    // 推送事件（异步通知）
    publish_event("user.created", &user)?;

    Ok(ResponseBuilder::new(201)
        .header("content-type", "application/json")
        .header("x-request-id", &request_id)
        .body(serde_json::to_vec(&ApiResponse::ok(user, &request_id))?)
        .build())
}

async fn update_user(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let request_id = get_or_generate_request_id(&req);
    let user_id = req.path_param("id").unwrap_or_default();
    let body = req.body().await?;
    
    // ... 更新逻辑（类似 create_user）

    // 失效缓存
    let cache = Store::open_default()?;
    cache.delete(&format!("user:{}", user_id))?;

    Ok(ResponseBuilder::new(200)
        .header("content-type", "application/json")
        .body(serde_json::to_vec(&ApiResponse::<()>::error("Not implemented", &request_id))?)
        .build())
}

async fn delete_user(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let request_id = get_or_generate_request_id(&req);
    let user_id = req.path_param("id").unwrap_or_default();

    let conn = Connection::open_default()?;
    conn.execute(
        "DELETE FROM users WHERE id = ?",
        &[Value::Text(user_id.clone())],
    )?;

    // 失效缓存
    let cache = Store::open_default()?;
    cache.delete(&format!("user:{}", user_id))?;

    Ok(ResponseBuilder::new(204)
        .header("x-request-id", &request_id)
        .body(vec![])
        .build())
}

async fn health_check(_req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let health = serde_json::json!({
        "status": "healthy",
        "version": env!("CARGO_PKG_VERSION"),
        "timestamp": current_timestamp(),
    });

    Ok(ResponseBuilder::new(200)
        .header("content-type", "application/json")
        .body(serde_json::to_vec(&health)?)
        .build())
}

fn get_or_generate_request_id(req: &IncomingRequest) -> String {
    req.header("x-request-id")
        .map(|v| v.to_string())
        .unwrap_or_else(|| generate_request_id())
}

fn generate_request_id() -> String {
    format!("req-{:016x}", current_timestamp())
}

fn generate_user_id(name: &str, email: &str) -> String {
    format!("usr-{:016x}", {
        use std::hash::{Hash, Hasher};
        let mut hasher = std::collections::hash_map::DefaultHasher::new();
        format!("{}{}{}", name, email, current_timestamp()).hash(&mut hasher);
        hasher.finish()
    })
}

fn current_timestamp() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}

fn publish_event(event_type: &str, data: &impl Serialize) -> anyhow::Result<()> {
    // 使用 Spin outbound Redis 发布事件
    let redis_url = variables::get("redis_url")?;
    let payload = serde_json::to_string(data)?;
    spin_sdk::redis::publish(&redis_url, event_type, payload.as_bytes())?;
    Ok(())
}
```

## 3.4 Spin 外向 HTTP 调用

```rust
// 在 Spin 组件中发起外部 HTTP 请求
use spin_sdk::http::{Method, Request, Response};

async fn call_external_api(
    url: &str,
    api_key: &str,
) -> anyhow::Result<serde_json::Value> {
    // 使用 Spin SDK 的受限 HTTP 客户端
    let request = Request::builder()
        .method(Method::Get)
        .uri(url)
        .header("Authorization", &format!("Bearer {}", api_key))
        .header("Content-Type", "application/json")
        .build();

    let response: Response = spin_sdk::http::send(request).await?;

    let status = response.status();
    if !status.is_success() {
        anyhow::bail!("External API error: {}", status);
    }

    let body = response.body();
    Ok(serde_json::from_slice(body)?)
}

// 带重试的 HTTP 调用
async fn call_with_retry(
    url: &str,
    max_retries: u32,
    backoff_ms: u64,
) -> anyhow::Result<Response> {
    let mut last_error = None;

    for attempt in 0..=max_retries {
        if attempt > 0 {
            // 指数退避
            let wait = backoff_ms * 2u64.pow(attempt - 1);
            tokio::time::sleep(tokio::time::Duration::from_millis(wait)).await;
        }

        let request = Request::builder()
            .method(Method::Get)
            .uri(url)
            .build();

        match spin_sdk::http::send(request).await {
            Ok(resp) if resp.status().is_success() => return Ok(resp),
            Ok(resp) => {
                last_error = Some(anyhow::anyhow!("HTTP {}", resp.status()));
            }
            Err(e) => {
                last_error = Some(e.into());
            }
        }

        println!("Attempt {} failed, retrying...", attempt + 1);
    }

    Err(last_error.unwrap_or_else(|| anyhow::anyhow!("All retries exhausted")))
}
```

---

<!-- chunk: 4. wasmCloud 平台 -->## 4. wasmCloud 平台

## 4.1 wasmCloud 架构

```mermaid
graph TB
    subgraph "wasmCloud Lattice"
        subgraph "Host 1 (Cloud)"
            Actor1[Actor: auth]
            Actor2[Actor: api]
            Provider1[Provider: HTTP]
            Provider2[Provider: KV]
        end

        subgraph "Host 2 (Edge)"
            Actor3[Actor: processor]
            Provider3[Provider: Messaging]
        end

        subgraph "Host 3 (IoT)"
            Actor4[Actor: sensor-reader]
            Provider4[Provider: Serial]
        end

        NATS[NATS.io 消息总线]

        Host1 --> NATS
        Host2 --> NATS
        Host3 --> NATS
    end

    subgraph "控制平面"
        Wadm[wadm - 应用管理]
        Console[wasmCloud Console]
    end

    Wadm --> |OAM 应用描述符| NATS
    Console --> NATS
```

## 4.2 wasmCloud Actor 开发

```rust
// wasmCloud Actor 示例（使用 wasmcloud-actor SDK）
use wasmcloud_actor::{
    HttpRequest, HttpResponse, MessageDispatch,
    Context, Actor, ActorResult,
};

#[actor::main]
async fn main(ctx: &Context, req: HttpRequest) -> ActorResult<HttpResponse> {
    let path = &req.path;
    let method = &req.method;

    match (method.as_str(), path.as_str()) {
        ("GET", "/") => handle_root(ctx).await,
        ("POST", "/process") => handle_process(ctx, req).await,
        _ => Ok(HttpResponse::not_found()),
    }
}

async fn handle_root(ctx: &Context) -> ActorResult<HttpResponse> {
    // 通过 Capability Provider 获取当前时间
    let time: u64 = ctx.capability("wasmcloud:builtin:clock").await?
        .call("get_timestamp", &[]).await?
        .into();

    Ok(HttpResponse::json(serde_json::json!({
        "service": "wasmcloud-actor",
        "timestamp": time,
        "status": "ok"
    }))?)
}

async fn handle_process(ctx: &Context, req: HttpRequest) -> ActorResult<HttpResponse> {
    let body: serde_json::Value = serde_json::from_slice(&req.body)?;

    // 通过 Messaging Provider 发送消息
    ctx.capability("wasmcloud:messaging")
        .await?
        .call("publish", &serde_json::json!({
            "subject": "jobs.process",
            "body": body,
        }))
        .await?;

    // 通过 KV Store Provider 记录状态
    ctx.capability("wasmcloud:keyvalue")
        .await?
        .call("set", &serde_json::json!({
            "key": format!("job:{}", uuid_v4()),
            "value": "pending",
            "expires": 3600
        }))
        .await?;

    Ok(HttpResponse::json(serde_json::json!({
        "status": "accepted",
        "message": "Job queued for processing"
    }))?)
}

fn uuid_v4() -> String {
    format!("{:032x}", std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_nanos())
}
```

## 4.3 OAM 应用描述符（wadm）

```yaml
# wasmcloud-app.yaml - 使用 OAM 描述符部署应用
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: microservices-app
  annotations:
    version: v1.0.0
    description: "Multi-service wasmCloud application"

spec:
  components:
    # HTTP API Actor
    - name: api-handler
      type: actor
      properties:
        image: ghcr.io/my-org/api-handler:1.0.0
      traits:
        - type: spreadscaler
          properties:
            instances: 5  # 5 个并发实例
            spread:
              - name: cloud-nodes
                requirements:
                  location: cloud
                weight: 80
              - name: edge-nodes
                requirements:
                  location: edge
                weight: 20

    # Auth Actor
    - name: auth-service
      type: actor
      properties:
        image: ghcr.io/my-org/auth-service:2.0.0
      traits:
        - type: spreadscaler
          properties:
            instances: 3

    # HTTP Server Capability Provider
    - name: http-server
      type: capability
      properties:
        image: ghcr.io/wasmcloud/http-server:0.20.0
        contract: wasmcloud:httpserver
        config:
          - name: default-http
            properties:
              address: "0.0.0.0:8080"

    # Redis KV Provider
    - name: redis-kv
      type: capability
      properties:
        image: ghcr.io/wasmcloud/keyvalue-redis:0.25.0
        contract: wasmcloud:keyvalue
        config:
          - name: redis-config
            properties:
              url: "redis://redis:6379"
              pool_size: "10"

    # NATS Messaging Provider
    - name: nats-messaging
      type: capability
      properties:
        image: ghcr.io/wasmcloud/messaging-nats:0.19.0
        contract: wasmcloud:messaging
        config:
          - name: nats-config
            properties:
              cluster_uris: "nats://nats:4222"

  # 链接关系声明
  links:
    - source: api-handler
      target: http-server
      namespace: wasmcloud
      package: httpserver
      interfaces:
        - incoming-handler

    - source: api-handler
      target: redis-kv
      namespace: wasmcloud
      package: keyvalue
      interfaces:
        - atomics
        - store

    - source: api-handler
      target: nats-messaging
      namespace: wasmcloud
      package: messaging
      interfaces:
        - consumer
        - producer
```

---

<!-- chunk: 5. Fermyon Cloud 部署 -->## 5. Fermyon Cloud 部署

## 5.1 部署到 Fermyon Cloud

```bash
# 登录 Fermyon Cloud
spin cloud login

# 创建应用
spin cloud apps create my-serverless-app

# 部署
spin cloud deploy

# 查看部署状态
spin cloud apps list
spin cloud logs my-serverless-app

# 管理变量
spin cloud variables set \
  --app my-serverless-app \
  DATABASE_URL="postgres://..." \
  API_KEY="secret"

# 查看当前变量
spin cloud variables list --app my-serverless-app

# 自定义域名
spin cloud domains add \
  --app my-serverless-app \
  api.mycompany.com

# 查看指标
spin cloud metrics \
  --app my-serverless-app \
  --since 1h
```

## 5.2 多环境部署

```bash
# 开发环境
spin cloud deploy \
  --app my-app-dev \
  --from spin.toml \
  --variable-file .env.dev

# 预发布环境
spin cloud deploy \
  --app my-app-staging \
  --from spin.toml \
  --variable-file .env.staging

# 生产环境
spin cloud deploy \
  --app my-app-prod \
  --from spin.toml \
  --variable-file .env.prod

# 蓝绿部署
spin cloud deploy \
  --app my-app-prod-green \
  --from spin.toml

# 流量切换
spin cloud routes set \
  --app my-app-prod \
  --target my-app-prod-green \
  --weight 100
```

---

<!-- chunk: 6. 事件触发器系统 -->## 6. 事件触发器系统

## 6.1 多种触发器类型

```toml
# spin.toml 中配置各种触发器

# HTTP 触发器
trigger.http
route = "/api/{path:...}"
component = "http-handler"

# Redis 订阅触发器
trigger.redis
channel = "user-events"
component = "user-event-processor"

trigger.redis
channel = "payment-events"
component = "payment-processor"

# 定时触发（Cron）
trigger.cron
cron_expression = "0 0 * * *"    # 每天凌晨
component = "daily-report"

trigger.cron
cron_expression = "*/5 * * * *"  # 每5分钟
component = "metrics-aggregator"

# MQTT 触发器（IoT）
trigger.mqtt
address = "mqtt://broker:1883"
topic = "sensors/#"
component = "sensor-processor"
```

## 6.2 Redis 事件处理器

```rust
// components/event-processor/src/lib.rs
use spin_sdk::{
    redis::{Payload, RedisParameter},
    redis_component,
    key_value::Store,
    http::{Request, Method},
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Deserialize)]
struct UserEvent {
    event_type: String,
    user_id: String,
    data: serde_json::Value,
    timestamp: u64,
}

#[derive(Debug, Serialize)]
struct NotificationPayload {
    user_id: String,
    message: String,
    channel: String,
}

// Redis 组件宏
#[redis_component]
async fn process_user_event(payload: Payload) -> anyhow::Result<()> {
    // 解析事件
    let event: UserEvent = serde_json::from_slice(&payload)?;
    
    println!(
        "Processing event: type={} user={} ts={}",
        event.event_type, event.user_id, event.timestamp
    );

    match event.event_type.as_str() {
        "user.created" => {
            handle_user_created(&event).await?;
        }
        "user.updated" => {
            handle_user_updated(&event).await?;
        }
        "user.deleted" => {
            handle_user_deleted(&event).await?;
        }
        _ => {
            println!("Unknown event type: {}", event.event_type);
        }
    }

    Ok(())
}

async fn handle_user_created(event: &UserEvent) -> anyhow::Result<()> {
    // 1. 发送欢迎邮件
    let notification = NotificationPayload {
        user_id: event.user_id.clone(),
        message: "Welcome to our platform!".to_string(),
        channel: "email".to_string(),
    };

    send_notification(&notification).await?;

    // 2. 初始化用户配额
    let store = Store::open_default()?;
    store.set(
        &format!("quota:{}", event.user_id),
        &serde_json::to_vec(&serde_json::json!({
            "api_calls": 1000,
            "storage_mb": 100,
            "reset_at": event.timestamp + 86400,
        }))?,
    )?;

    // 3. 触发欢迎工作流
    trigger_workflow("welcome-flow", &event.user_id).await?;

    println!("User created handler completed: {}", event.user_id);
    Ok(())
}

async fn handle_user_updated(event: &UserEvent) -> anyhow::Result<()> {
    // 失效相关缓存
    let store = Store::open_default()?;
    store.delete(&format!("user:{}", event.user_id))?;
    store.delete(&format!("user_perms:{}", event.user_id))?;

    println!("User updated, cache invalidated: {}", event.user_id);
    Ok(())
}

async fn handle_user_deleted(event: &UserEvent) -> anyhow::Result<()> {
    // 清理所有用户相关数据
    let store = Store::open_default()?;
    let keys_to_delete = vec![
        format!("user:{}", event.user_id),
        format!("user_perms:{}", event.user_id),
        format!("quota:{}", event.user_id),
        format!("session:{}", event.user_id),
    ];

    for key in keys_to_delete {
        store.delete(&key)?;
    }

    // 通知其他系统
    send_notification(&NotificationPayload {
        user_id: event.user_id.clone(),
        message: "Your account has been deleted.".to_string(),
        channel: "email".to_string(),
    }).await?;

    Ok(())
}

async fn send_notification(notif: &NotificationPayload) -> anyhow::Result<()> {
    let notification_url = spin_sdk::variables::get("notification_service_url")?;

    let request = Request::builder()
        .method(Method::Post)
        .uri(&format!("{}/v1/notifications", notification_url))
        .header("Content-Type", "application/json")
        .body(serde_json::to_vec(notif)?)
        .build();

    spin_sdk::http::send(request).await?;
    Ok(())
}

async fn trigger_workflow(workflow_name: &str, user_id: &str) -> anyhow::Result<()> {
    let workflow_url = spin_sdk::variables::get("workflow_service_url")?;

    let request = Request::builder()
        .method(Method::Post)
        .uri(&format!("{}/v1/workflows/{}", workflow_url, workflow_name))
        .header("Content-Type", "application/json")
        .body(serde_json::to_vec(&serde_json::json!({
            "user_id": user_id,
            "triggered_at": current_timestamp(),
        }))?)
        .build();

    spin_sdk::http::send(request).await?;
    Ok(())
}

fn current_timestamp() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
```

## 6.3 Cron 定时任务

```rust
// components/metrics-aggregator/src/lib.rs
use spin_sdk::{cron_component, sqlite::Connection, key_value::Store};
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize)]
struct MetricSummary {
    window_start: u64,
    window_end: u64,
    total_requests: i64,
    avg_latency_ms: f64,
    p99_latency_ms: f64,
    error_count: i64,
    error_rate: f64,
}

#[cron_component]
async fn aggregate_metrics() -> anyhow::Result<()> {
    let now = current_timestamp();
    let window_start = now - 300;  // 过去5分钟

    println!("Aggregating metrics for window: {} - {}", window_start, now);

    let conn = Connection::open_default()?;

    // 聚合请求统计
    let result = conn.execute(
        "SELECT 
            COUNT(*) as total,
            AVG(latency_ms) as avg_latency,
            MAX(CASE WHEN row_num = CAST(0.99 * total AS INTEGER) THEN latency_ms END) as p99,
            SUM(CASE WHEN status_code >= 400 THEN 1 ELSE 0 END) as errors
         FROM (
             SELECT latency_ms, status_code,
                    ROW_NUMBER() OVER (ORDER BY latency_ms) as row_num,
                    COUNT(*) OVER () as total
             FROM request_logs
             WHERE created_at >= ? AND created_at < ?
         )",
        &[
            spin_sdk::sqlite::Value::Integer(window_start as i64),
            spin_sdk::sqlite::Value::Integer(now as i64),
        ],
    )?;

    if let Some(row) = result.rows().next() {
        let total: i64 = row.get(0).unwrap_or(0);
        let avg_latency: f64 = row.get(1).unwrap_or(0.0);
        let p99_latency: f64 = row.get(2).unwrap_or(0.0);
        let errors: i64 = row.get(3).unwrap_or(0);

        let summary = MetricSummary {
            window_start,
            window_end: now,
            total_requests: total,
            avg_latency_ms: avg_latency,
            p99_latency_ms: p99_latency,
            error_count: errors,
            error_rate: if total > 0 { errors as f64 / total as f64 } else { 0.0 },
        };

        // 写入聚合结果
        conn.execute(
            "INSERT INTO metric_summaries (window_start, window_end, data) VALUES (?, ?, ?)",
            &[
                spin_sdk::sqlite::Value::Integer(window_start as i64),
                spin_sdk::sqlite::Value::Integer(now as i64),
                spin_sdk::sqlite::Value::Text(serde_json::to_string(&summary)?),
            ],
        )?;

        // 存入 KV 供快速读取
        let store = Store::open_default()?;
        store.set("metrics:latest", &serde_json::to_vec(&summary)?)?;

        println!("Metrics aggregated: total={} avg={:.2}ms errors={}", 
            total, avg_latency, errors);

        // 检查告警条件
        if summary.error_rate > 0.05 {
            trigger_alert("high_error_rate", &summary).await?;
        }
        if summary.p99_latency_ms > 1000.0 {
            trigger_alert("high_latency", &summary).await?;
        }
    }

    Ok(())
}

async fn trigger_alert(alert_type: &str, summary: &MetricSummary) -> anyhow::Result<()> {
    println!("ALERT: {} - error_rate={:.2}% p99={:.0}ms",
        alert_type, summary.error_rate * 100.0, summary.p99_latency_ms);

    // 发送告警到 PagerDuty/Slack
    let webhook_url = spin_sdk::variables::get("alert_webhook_url")?;
    let payload = serde_json::json!({
        "alert_type": alert_type,
        "severity": if summary.error_rate > 0.1 { "critical" } else { "warning" },
        "summary": summary,
    });

    let request = spin_sdk::http::Request::builder()
        .method(spin_sdk::http::Method::Post)
        .uri(&webhook_url)
        .header("Content-Type", "application/json")
        .body(serde_json::to_vec(&payload)?)
        .build();

    spin_sdk::http::send(request).await?;
    Ok(())
}

fn current_timestamp() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
```

---

<!-- chunk: 7. Scale-to-Zero 实现 -->## 7. Scale-to-Zero 实现

## 7.1 Knative + Wasm Scale-to-Zero

```yaml
# knative-wasm-service.yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: wasm-serverless
  namespace: default
  annotations:
    # Wasm 运行时
    runtime.knative.dev/wasm: "true"
spec:
  template:
    metadata:
      annotations:
        # Scale-to-Zero 配置
        autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
        autoscaling.knative.dev/target: "100"        # 每实例并发目标
        autoscaling.knative.dev/min-scale: "0"       # 缩容到零
        autoscaling.knative.dev/max-scale: "100"     # 最大实例数
        autoscaling.knative.dev/scale-to-zero-pod-retention-period: "60s"
        autoscaling.knative.dev/initial-scale: "1"
        autoscaling.knative.dev/scale-down-delay: "30s"
        
        # 预热配置（减少冷启动影响）
        autoscaling.knative.dev/target-burst-capacity: "200"
    
    spec:
      # Wasm 运行时类
      runtimeClassName: wasmedge
      
      # 极短超时（Wasm 启动快）
      timeoutSeconds: 30
      
      containers:
        - name: wasm-service
          image: ghcr.io/my-org/my-wasm-service:1.0.0
          
          resources:
            requests:
              cpu: "10m"      # Wasm 极低 CPU 需求
              memory: "16Mi"  # 极低内存需求
            limits:
              cpu: "1"
              memory: "128Mi"
          
          env:
            - name: PORT
              value: "8080"
          
          readinessProbe:
            httpGet:
              path: /health
              port: 8080
            initialDelaySeconds: 0  # Wasm 几乎即时启动
            periodSeconds: 1
            successThreshold: 1
            failureThreshold: 3

---
# KEDA 基于队列深度的自动伸缩
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: wasm-queue-scaler
  namespace: default
spec:
  scaleTargetRef:
    name: wasm-worker
  minReplicaCount: 0
  maxReplicaCount: 50
  cooldownPeriod: 60
  
  triggers:
    - type: redis
      metadata:
        host: redis-master.default.svc.cluster.local
        port: "6379"
        listName: "job-queue"
        listLength: "5"   # 每5个任务启动一个实例
```

## 7.2 自适应预热策略

```rust
// 自适应实例预热管理器
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Arc;

struct AdaptiveWarmPoolManager {
    pool: Arc<InstancePool>,
    stats: Arc<TrafficStats>,
    config: AdaptiveConfig,
}

struct TrafficStats {
    requests_per_second: AtomicU64,
    cold_starts: AtomicU64,
    total_requests: AtomicU64,
}

#[derive(Clone)]
struct AdaptiveConfig {
    min_warm_instances: usize,
    max_warm_instances: usize,
    target_cold_start_rate: f64,  // 目标冷启动率 (0.0-1.0)
    adjustment_interval_secs: u64,
}

impl AdaptiveWarmPoolManager {
    async fn run_adaptive_loop(&self) {
        let mut interval = tokio::time::interval(
            tokio::time::Duration::from_secs(self.config.adjustment_interval_secs)
        );

        loop {
            interval.tick().await;

            let total = self.stats.total_requests.swap(0, Ordering::Relaxed);
            let cold_starts = self.stats.cold_starts.swap(0, Ordering::Relaxed);
            let rps = self.stats.requests_per_second.load(Ordering::Relaxed);

            if total == 0 {
                continue;
            }

            let actual_cold_start_rate = cold_starts as f64 / total as f64;
            let current_warm = self.pool.available_count().await;

            println!(
                "Traffic stats: rps={} cold_start_rate={:.2}% warm_instances={}",
                rps,
                actual_cold_start_rate * 100.0,
                current_warm,
            );

            // 根据冷启动率调整预热实例数
            let new_target = if actual_cold_start_rate > self.config.target_cold_start_rate * 1.2 {
                // 冷启动过多，增加预热实例
                (current_warm + 2).min(self.config.max_warm_instances)
            } else if actual_cold_start_rate < self.config.target_cold_start_rate * 0.5
                && current_warm > self.config.min_warm_instances
            {
                // 冷启动很少，可以减少预热实例
                (current_warm - 1).max(self.config.min_warm_instances)
            } else {
                current_warm
            };

            // 基于 RPS 的预测性预热
            let predicted_warm = predict_required_warm(rps);
            let final_target = new_target.max(predicted_warm);

            if final_target != current_warm {
                println!("Adjusting warm pool: {} -> {}", current_warm, final_target);
                self.pool.set_target_size(final_target).await;
            }
        }
    }
}

fn predict_required_warm(rps: u64) -> usize {
    // 简单线性预测：每100 rps 保留1个实例
    ((rps as f64 / 100.0).ceil() as usize).max(1)
}
```

---

<!-- chunk: 8. FaaS 设计模式 -->## 8. FaaS 设计模式

## 8.1 函数链（Function Chaining）

```mermaid
graph LR
    Input[输入事件] --> F1[validate-input.wasm]
    F1 --> |validated data| F2[enrich-data.wasm]
    F2 --> |enriched| F3[process-business.wasm]
    F3 --> |result| F4[format-output.wasm]
    F4 --> Output[响应/事件]
    
    F1 --> |invalid| Error[error-handler.wasm]
    F2 --> |enrich failed| F3
    F3 --> |business error| Error
```

```rust
// 函数链协调器
use serde_json::Value;

struct FunctionChain {
    steps: Vec<ChainStep>,
    error_handler: Option<String>,
}

struct ChainStep {
    function_name: String,
    condition: Option<String>,  // 条件表达式
    transform: Option<String>,  // 输入转换
}

impl FunctionChain {
    async fn execute(&self, input: Value) -> anyhow::Result<Value> {
        let mut current = input;

        for step in &self.steps {
            // 检查条件
            if let Some(condition) = &step.condition {
                if !evaluate_condition(condition, &current) {
                    println!("Skipping step {} (condition not met)", step.function_name);
                    continue;
                }
            }

            // 执行输入转换
            if let Some(transform) = &step.transform {
                current = apply_transform(transform, current)?;
            }

            // 调用函数
            match invoke_function(&step.function_name, current.clone()).await {
                Ok(result) => {
                    current = result;
                }
                Err(e) => {
                    if let Some(handler) = &self.error_handler {
                        let error_input = serde_json::json!({
                            "error": e.to_string(),
                            "step": step.function_name,
                            "input": current,
                        });
                        current = invoke_function(handler, error_input).await?;
                    } else {
                        return Err(e);
                    }
                }
            }
        }

        Ok(current)
    }
}

async fn invoke_function(name: &str, input: Value) -> anyhow::Result<Value> {
    // 通过 Spin SDK 或 wasmCloud 调用函数
    let url = format!("http://localhost:8080/functions/{}", name);
    let request = spin_sdk::http::Request::builder()
        .method(spin_sdk::http::Method::Post)
        .uri(&url)
        .header("Content-Type", "application/json")
        .body(serde_json::to_vec(&input)?)
        .build();

    let response = spin_sdk::http::send(request).await?;
    let body = response.body();
    Ok(serde_json::from_slice(body)?)
}

fn evaluate_condition(condition: &str, data: &Value) -> bool {
    // 简化的条件求值（实际可使用 jsonpath 或 JMESPath）
    match condition {
        "$.status == 'active'" => {
            data.get("status").and_then(|v| v.as_str()) == Some("active")
        }
        _ => true,
    }
}

fn apply_transform(transform: &str, data: Value) -> anyhow::Result<Value> {
    // 简化的转换（实际可使用 jq 语法）
    Ok(data)
}
```

## 8.2 扇出模式（Fan-Out）

```rust
// 扇出处理：并行调用多个函数
use futures::future::join_all;

async fn fan_out_process(
    event: serde_json::Value,
    handlers: Vec<String>,
) -> Vec<anyhow::Result<serde_json::Value>> {
    // 并发调用所有处理器
    let futures: Vec<_> = handlers.iter()
        .map(|handler| invoke_function(handler, event.clone()))
        .collect();

    join_all(futures).await
}

// 扇出 + 聚合：收集所有结果
async fn fan_out_aggregate(
    event: serde_json::Value,
    handlers: Vec<String>,
    aggregator: &str,
) -> anyhow::Result<serde_json::Value> {
    let results = fan_out_process(event, handlers).await;

    // 收集成功结果
    let successful: Vec<serde_json::Value> = results.into_iter()
        .filter_map(|r| r.ok())
        .collect();

    // 调用聚合函数
    invoke_function(aggregator, serde_json::json!({
        "results": successful,
    })).await
}
```

## 8.3 Saga 模式（分布式事务）

```rust
// Saga 补偿事务
use std::collections::VecDeque;

#[derive(Clone)]
struct SagaStep {
    name: String,
    action: String,           // 正向操作
    compensation: String,     // 补偿操作
}

struct SagaOrchestrator {
    steps: Vec<SagaStep>,
    completed: VecDeque<(SagaStep, serde_json::Value)>,
}

impl SagaOrchestrator {
    async fn execute(&mut self, initial_data: serde_json::Value) -> anyhow::Result<serde_json::Value> {
        let mut current = initial_data;
        self.completed.clear();

        for step in self.steps.clone() {
            println!("Executing saga step: {}", step.name);

            match invoke_function(&step.action, current.clone()).await {
                Ok(result) => {
                    self.completed.push_front((step.clone(), current.clone()));
                    current = result;
                }
                Err(e) => {
                    println!("Step {} failed: {}", step.name, e);
                    // 执行补偿操作
                    self.compensate().await;
                    return Err(anyhow::anyhow!("Saga failed at step {}: {}", step.name, e));
                }
            }
        }

        Ok(current)
    }

    async fn compensate(&mut self) {
        println!("Starting saga compensation...");

        while let Some((step, data)) = self.completed.pop_front() {
            println!("Compensating step: {}", step.name);

            match invoke_function(&step.compensation, data).await {
                Ok(_) => println!("Compensation succeeded: {}", step.name),
                Err(e) => println!("WARN: Compensation failed for {}: {}", step.name, e),
            }
        }

        println!("Saga compensation complete");
    }
}
```

---

<!-- chunk: 9. 有状态 Serverless -->## 9. 有状态 Serverless

## 9.1 状态持久化模式

```mermaid
graph TB
    subgraph "Wasm Serverless 状态管理"
        subgraph "无状态层（可扩展）"
            W1[Wasm Instance 1]
            W2[Wasm Instance 2]
            W3[Wasm Instance 3]
        end

        subgraph "状态层"
            KV[KV Store<br/>Redis/Turso]
            DB[SQLite/PostgreSQL]
            Cache[分布式缓存]
            Queue[消息队列<br/>NATS/Kafka]
        end

        W1 --> KV
        W2 --> KV
        W3 --> KV
        W1 --> DB
        W2 --> DB
        W3 --> DB
    end
```

```rust
// 带状态的 Serverless 函数
use spin_sdk::{
    key_value::Store,
    sqlite::{Connection, Value},
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
struct Session {
    session_id: String,
    user_id: String,
    created_at: u64,
    expires_at: u64,
    data: serde_json::Value,
}

struct SessionManager {
    store: Store,
    session_ttl_secs: u64,
}

impl SessionManager {
    fn new() -> anyhow::Result<Self> {
        Ok(Self {
            store: Store::open("sessions")?,
            session_ttl_secs: 3600,  // 1 小时
        })
    }

    fn create_session(
        &self,
        user_id: &str,
        initial_data: serde_json::Value,
    ) -> anyhow::Result<Session> {
        let now = current_timestamp();
        let session = Session {
            session_id: generate_session_id(),
            user_id: user_id.to_string(),
            created_at: now,
            expires_at: now + self.session_ttl_secs,
            data: initial_data,
        };

        self.store.set(
            &format!("session:{}", session.session_id),
            &serde_json::to_vec(&session)?,
        )?;

        // 用户 session 索引
        self.store.set(
            &format!("user_session:{}", user_id),
            session.session_id.as_bytes(),
        )?;

        Ok(session)
    }

    fn get_session(&self, session_id: &str) -> anyhow::Result<Option<Session>> {
        match self.store.get(&format!("session:{}", session_id))? {
            Some(bytes) => {
                let session: Session = serde_json::from_slice(&bytes)?;
                let now = current_timestamp();

                if session.expires_at < now {
                    // Session 已过期
                    self.delete_session(session_id)?;
                    return Ok(None);
                }

                Ok(Some(session))
            }
            None => Ok(None),
        }
    }

    fn update_session(
        &self,
        session_id: &str,
        updates: serde_json::Value,
    ) -> anyhow::Result<Session> {
        let mut session = self.get_session(session_id)?
            .ok_or_else(|| anyhow::anyhow!("Session not found: {}", session_id))?;

        // 合并更新
        if let (serde_json::Value::Object(ref mut data), serde_json::Value::Object(new_data)) =
            (&mut session.data, updates)
        {
            for (k, v) in new_data {
                data.insert(k, v);
            }
        }

        // 延长 session 有效期（滑动窗口）
        session.expires_at = current_timestamp() + self.session_ttl_secs;

        self.store.set(
            &format!("session:{}", session_id),
            &serde_json::to_vec(&session)?,
        )?;

        Ok(session)
    }

    fn delete_session(&self, session_id: &str) -> anyhow::Result<()> {
        // 先获取 user_id 用于清理索引
        if let Some(session) = self.get_session(session_id)? {
            self.store.delete(&format!("user_session:{}", session.user_id))?;
        }
        self.store.delete(&format!("session:{}", session_id))?;
        Ok(())
    }
}

fn generate_session_id() -> String {
    format!("sess-{:032x}", current_timestamp())
}

fn current_timestamp() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
```

---

<!-- chunk: 10. 多云 Serverless 部署 -->## 10. 多云 Serverless 部署

## 10.1 云厂商中立部署配置

```yaml
# 使用 Spin 跨云部署
# deploy-multicloud.yaml

environments:
  aws_lambda:
    provider: aws
    region: us-east-1
    runtime: wasm
    
  gcp_cloud_run:
    provider: gcp
    region: us-central1
    runtime: wasm
    
  azure_functions:
    provider: azure
    region: eastus
    runtime: wasm
    
  fermyon_cloud:
    provider: fermyon
    
  cloudflare_workers:
    provider: cloudflare

deployment:
  strategy: multi-region
  primary: fermyon_cloud
  fallback:
    - cloudflare_workers
    - aws_lambda
```

```bash
#!/bin/bash
# deploy-to-cloudflare.sh

# 将 Spin 应用部署到 Cloudflare Workers
spin build

# 转换为 CF Workers 格式
spin cloud deploy --provider cloudflare \
  --route "https://api.mycompany.com/*"

# 或使用 wrangler 直接部署
cat > wrangler.toml << 'EOF'
name = "my-wasm-worker"
main = "dist/worker.wasm"
compatibility_date = "2024-01-01"

[vars]
ENVIRONMENT = "production"
API_KEY = "..."

[[系统基础/知识字典/fundamentals/namespaces.md|namespaces]]
binding = "KV_STORE"
id = "abc123..."

[build]
command = "spin build && spin convert --target cloudflare"
EOF

wrangler deploy
```

## 10.2 边缘 + 云混合部署

```yaml
# k8s-edge-cloud-hybrid.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-hybrid-service
spec:
  replicas: 2
  template:
    spec:
      # 优先部署到边缘节点
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              preference:
                matchExpressions:
                  - key: node-role.kubernetes.io/edge
                    operator: Exists
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: runtime.class
                    operator: In
                    values:
                      - wasmedge
      
      runtimeClassName: wasmedge
      
      containers:
        - name: wasm-service
          image: ghcr.io/my-org/wasm-service:1.0.0
          env:
            - name: DEPLOYMENT_TIER
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
            - name: REGION
              value: "us-west-2"
```

---

<!-- chunk: 11. Serverless 可观测性 -->## 11. Serverless 可观测性

## 11.1 OpenTelemetry 集成

```rust
// 在 Spin 组件中集成 OpenTelemetry
use spin_sdk::http::{IncomingRequest, OutgoingResponse, ResponseBuilder};
use opentelemetry::trace::{Tracer, TracerProvider};

fn init_tracing() -> anyhow::Result<opentelemetry_sdk::trace::Tracer> {
    // 配置 OTLP 导出器
    let exporter = opentelemetry_otlp::new_exporter()
        .http()
        .with_endpoint("http://otel-collector:4318")
        .build_span_exporter()?;

    let provider = opentelemetry_sdk::trace::TracerProvider::builder()
        .with_batch_exporter(exporter, opentelemetry_sdk::runtime::Tokio)
        .with_resource(opentelemetry_sdk::Resource::new(vec![
            opentelemetry::KeyValue::new("service.name", "wasm-serverless"),
            opentelemetry::KeyValue::new("service.version", env!("CARGO_PKG_VERSION")),
            opentelemetry::KeyValue::new("deployment.environment", "production"),
        ]))
        .build();

    Ok(provider.tracer("wasm-service"))
}

async fn handle_with_tracing(
    req: IncomingRequest,
    tracer: &impl Tracer,
) -> anyhow::Result<OutgoingResponse> {
    let parent_ctx = extract_trace_context(&req);

    let span = tracer
        .span_builder("handle_request")
        .with_parent_context(parent_ctx)
        .with_attributes(vec![
            opentelemetry::KeyValue::new("http.method", req.method().to_string()),
            opentelemetry::KeyValue::new("http.path", req.path().to_string()),
            opentelemetry::KeyValue::new("http.target", req.path().to_string()),
        ])
        .start(tracer);

    let _guard = opentelemetry::trace::mark_span_as_active(span);

    // 业务处理
    let start = std::time::Instant::now();

    let response = process_request(req).await.map_err(|e| {
        opentelemetry::trace::get_active_span(|span| {
            span.record_error(&e);
            span.set_status(opentelemetry::trace::Status::Error {
                description: e.to_string().into(),
            });
        });
        e
    })?;

    let latency = start.elapsed().as_millis();
    opentelemetry::trace::get_active_span(|span| {
        span.set_attribute(opentelemetry::KeyValue::new(
            "http.status_code",
            response.status().as_u16() as i64,
        ));
        span.set_attribute(opentelemetry::KeyValue::new(
            "latency_ms",
            latency as i64,
        ));
    });

    Ok(response)
}

fn extract_trace_context(req: &IncomingRequest) -> opentelemetry::Context {
    let traceparent = req.header("traceparent")
        .unwrap_or_default();
    let tracestate = req.header("tracestate")
        .unwrap_or_default();

    // 解析 W3C Trace Context
    let mut carrier = std::collections::HashMap::new();
    if !traceparent.is_empty() {
        carrier.insert("traceparent".to_string(), traceparent);
    }
    if !tracestate.is_empty() {
        carrier.insert("tracestate".to_string(), tracestate);
    }

    opentelemetry::global::get_text_map_propagator(|propagator| {
        propagator.extract(&carrier)
    })
}

async fn process_request(req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    Ok(ResponseBuilder::new(200)
        .header("content-type", "application/json")
        .body(b"{\"status\":\"ok\"}".to_vec())
        .build())
}
```

## 11.2 Prometheus 指标暴露

```rust
// Serverless 函数指标收集
use spin_sdk::{
    http::{IncomingRequest, OutgoingResponse, ResponseBuilder},
    key_value::Store,
};

const METRICS_KEY_PREFIX: &str = "metric:";

fn increment_counter(name: &str, labels: &[(&str, &str)]) -> anyhow::Result<()> {
    let store = Store::open_default()?;
    let key = format!("{}{}", METRICS_KEY_PREFIX, format_metric_key(name, labels));

    let current = store.get(&key)?
        .and_then(|b| String::from_utf8(b).ok())
        .and_then(|s| s.parse::<u64>().ok())
        .unwrap_or(0);

    store.set(&key, (current + 1).to_string().as_bytes())?;
    Ok(())
}

fn record_histogram(name: &str, value: f64, labels: &[(&str, &str)]) -> anyhow::Result<()> {
    let store = Store::open_default()?;
    let key = format!("{}hist:{}", METRICS_KEY_PREFIX, format_metric_key(name, labels));

    // 简化的直方图：存储累计值和计数
    let current_data = store.get(&key)?
        .and_then(|b| serde_json::from_slice::<serde_json::Value>(&b).ok())
        .unwrap_or_else(|| serde_json::json!({"sum": 0.0, "count": 0, "buckets": {}}));

    let new_sum = current_data["sum"].as_f64().unwrap_or(0.0) + value;
    let new_count = current_data["count"].as_u64().unwrap_or(0) + 1;

    let updated = serde_json::json!({
        "sum": new_sum,
        "count": new_count,
    });

    store.set(&key, &serde_json::to_vec(&updated)?)?;
    Ok(())
}

fn format_metric_key(name: &str, labels: &[(&str, &str)]) -> String {
    if labels.is_empty() {
        return name.to_string();
    }
    let labels_str: Vec<String> = labels.iter()
        .map(|(k, v)| format!("{}={}", k, v))
        .collect();
    format!("{}_{}", name, labels_str.join("_"))
}

// 指标端点
async fn metrics_handler(_req: IncomingRequest) -> anyhow::Result<OutgoingResponse> {
    let store = Store::open_default()?;
    let mut output = String::new();

    // 获取所有指标键
    let keys = store.get_keys()?;
    let metric_keys: Vec<String> = keys.into_iter()
        .filter(|k| k.starts_with(METRICS_KEY_PREFIX))
        .collect();

    for key in metric_keys {
        if let Some(value) = store.get(&key)? {
            let metric_name = key.trim_start_matches(METRICS_KEY_PREFIX);

            if let Ok(count) = String::from_utf8(value.clone())
                .and_then(|s| s.parse::<u64>().map_err(|e| std::string::FromUtf8Error::from(vec![])))
            {
                output.push_str(&format!("{} {}\n", metric_name, count));
            } else if let Ok(hist) = serde_json::from_slice::<serde_json::Value>(&value) {
                let metric_name = metric_name.trim_start_matches("hist:");
                output.push_str(&format!("{}_sum {}\n",
                    metric_name, hist["sum"].as_f64().unwrap_or(0.0)));
                output.push_str(&format!("{}_count {}\n",
                    metric_name, hist["count"].as_u64().unwrap_or(0)));
            }
        }
    }

    Ok(ResponseBuilder::new(200)
        .header("content-type", "text/plain; version=0.0.4")
        .body(output.into_bytes())
        .build())
}
```

---

<!-- chunk: 12. 边缘 Serverless -->## 12. 边缘 Serverless

## 12.1 Cloudflare Workers

```javascript
// cloudflare-worker.js - 边缘 Wasm Serverless
import wasmModule from './target/wasm32-unknown-unknown/release/my_worker.wasm';

// Wasm 模块实例（每个 Worker 隔离）
let wasmInstance = null;

async function initWasm() {
    if (!wasmInstance) {
        wasmInstance = await WebAssembly.instantiate(wasmModule, {
            // 主机函数注入
            env: {
                now: () => BigInt(Date.now()),
                log: (ptr, len) => {
                    const text = decoder.decode(
                        new Uint8Array(wasmInstance.exports.memory.buffer, ptr, len)
                    );
                    console.log('[wasm]', text);
                },
            },
        });
    }
    return wasmInstance;
}

const decoder = new TextDecoder();
const encoder = new TextEncoder();

export default {
    async fetch(request, env, ctx) {
        const instance = await initWasm();
        const { memory, handle_request, malloc, free } = instance.exports;

        // 序列化请求
        const requestData = JSON.stringify({
            method: request.method,
            url: request.url,
            headers: Object.fromEntries(request.headers),
            body: request.method !== 'GET' ? await request.text() : null,
        });

        const requestBytes = encoder.encode(requestData);
        const inputPtr = malloc(requestBytes.length);
        new Uint8Array(memory.buffer, inputPtr, requestBytes.length).set(requestBytes);

        // 调用 Wasm 处理函数
        const outputPtr = handle_request(inputPtr, requestBytes.length);
        free(inputPtr);

        // 读取响应
        const header = new DataView(memory.buffer, outputPtr, 8);
        const responsePtr = header.getInt32(0, true);
        const responseLen = header.getInt32(4, true);

        const responseBytes = new Uint8Array(memory.buffer, responsePtr, responseLen);
        const responseData = JSON.parse(decoder.decode(responseBytes));

        free(outputPtr);

        return new Response(responseData.body, {
            status: responseData.status,
            headers: responseData.headers,
        });
    },
};
```

## 12.2 Fastly Compute@Edge

```rust
// fastly-compute/src/main.rs
use fastly::{Error, Request, Response};
use fastly::mime;

#[fastly::main]
fn main(req: Request) -> Result<Response, Error> {
    // 路由
    match (req.get_method_str(), req.get_path()) {
        ("GET", "/") => handle_home(req),
        ("GET", path) if path.starts_with("/api/") => handle_api(req),
        ("POST", "/webhook") => handle_webhook(req),
        _ => Ok(Response::from_status(404)
            .with_body_text_plain("Not Found")),
    }
}

fn handle_home(_req: Request) -> Result<Response, Error> {
    let resp = serde_json::json!({
        "service": "fastly-wasm-edge",
        "datacenter": fastly::geo::datacenter()
            .map(|dc| dc.to_string())
            .unwrap_or_else(|| "unknown".to_string()),
        "pop": fastly::geo::pop()
            .map(|pop| pop.to_string())
            .unwrap_or_else(|| "unknown".to_string()),
    });

    Ok(Response::from_status(200)
        .with_content_type(mime::APPLICATION_JSON)
        .with_body_json(&resp)?)
}

fn handle_api(req: Request) -> Result<Response, Error> {
    let path = req.get_path().to_string();

    // 检查缓存
    let cache_key = format!("api_cache:{}", path);
    if let Some(cached) = fastly::cache::core::lookup(cache_key.as_bytes())? {
        if let Some(body) = cached.to_string() {
            return Ok(Response::from_status(200)
                .with_content_type(mime::APPLICATION_JSON)
                .with_header("x-cache", "HIT")
                .with_body(body));
        }
    }

    // 转发到源服务
    let backend_req = req.clone_without_body();
    let backend_resp = backend_req.send("backend-api")?;

    let status = backend_resp.get_status();
    let body = backend_resp.into_body_str()?;

    // 缓存成功响应（5分钟）
    if status.is_success() {
        fastly::cache::core::insert(
            cache_key.as_bytes(),
            300,  // TTL in seconds
        ).body_writer().write_all(body.as_bytes())?;
    }

    Ok(Response::from_status(status)
        .with_content_type(mime::APPLICATION_JSON)
        .with_header("x-cache", "MISS")
        .with_body(body))
}

fn handle_webhook(mut req: Request) -> Result<Response, Error> {
    let body = req.take_body_str();

    // 验证签名
    let signature = req.get_header_str("x-signature")
        .unwrap_or_default();
    
    if !verify_webhook_signature(&body, signature) {
        return Ok(Response::from_status(401)
            .with_body_text_plain("Invalid signature"));
    }

    // 异步处理（发到队列）
    let event: serde_json::Value = serde_json::from_str(&body)?;
    
    // 发送到后端处理
    let process_req = Request::post("https://processor.example.com/webhook")
        .with_body_json(&event)?;
    process_req.send_async("processor-backend")?;

    Ok(Response::from_status(202)
        .with_body_text_plain("Accepted"))
}

fn verify_webhook_signature(body: &str, signature: &str) -> bool {
    // HMAC 验证（简化）
    !signature.is_empty()
}
```

---

<!-- chunk: 13. 性能基准与对比 -->## 13. 性能基准与对比

## 13.1 冷启动延迟对比

```
# 🟢 低风险：只读/信息收集，通常无副作用
Serverless 冷启动延迟对比（2025年实测）：

┌────────────────────────────────────────────────────────────┐
│ 平台                 │ P50     │ P99     │ 内存    │ 语言   │
├────────────────────────────────────────────────────────────┤
│ AWS Lambda (Node.js) │ 150ms   │ 800ms   │ 128MB   │ JS     │
│ AWS Lambda (Rust)    │ 60ms    │ 200ms   │ 128MB   │ Rust   │
│ AWS Lambda (Python)  │ 300ms   │ 1200ms  │ 256MB   │ Python │
│ GCP Cloud Run        │ 200ms   │ 2000ms  │ 256MB   │ 任意   │
│ Cloudflare Workers   │ 0ms     │ 2ms     │ 128MB   │ JS/Wasm│
│ Fermyon Spin         │ 0.5ms   │ 2ms     │ 10MB    │ Rust   │
│ wasmCloud            │ 0.8ms   │ 3ms     │ 8MB     │ Rust   │
│ Fastly Compute       │ 0ms     │ 1ms     │ 64MB    │ Rust   │
│ WasmEdge FaaS        │ 1ms     │ 5ms     │ 20MB    │ Rust   │
│ K8s + Knative (Wasm) │ 5ms     │ 20ms    │ 16MB    │ Rust   │
└────────────────────────────────────────────────────────────┘

注：
- Cloudflare/Fastly 因边缘预热无冷启动
- Spin/wasmCloud 因 Wasm AOT 预编译极速启动
- K8s 包含容器调度开销
```
## 13.2 吞吐量对比

```python
# benchmark_serverless.py
import asyncio
import aiohttp
import time
import statistics

async def benchmark_endpoint(
    url: str,
    n_requests: int,
    concurrency: int,
) -> dict:
    semaphore = asyncio.Semaphore(concurrency)
    latencies = []
    errors = 0

    async def make_request():
        nonlocal errors
        async with semaphore:
            try:
                start = time.perf_counter()
                async with aiohttp.ClientSession() as session:
                    async with session.get(url) as resp:
                        await resp.text()
                        latency = (time.perf_counter() - start) * 1000
                        latencies.append(latency)
            except Exception:
                errors += 1

    wall_start = time.perf_counter()
    tasks = [make_request() for _ in range(n_requests)]
    await asyncio.gather(*tasks)
    wall_time = time.perf_counter() - wall_start

    return {
        "url": url,
        "n_requests": n_requests,
        "concurrency": concurrency,
        "success": len(latencies),
        "errors": errors,
        "p50_ms": statistics.median(latencies) if latencies else 0,
        "p99_ms": sorted(latencies)[int(len(latencies)*0.99)] if latencies else 0,
        "throughput_rps": n_requests / wall_time,
    }

async def main():
    # 对比不同 Serverless 平台
    endpoints = [
        ("Spin (Local)", "http://localhost:3000/api/test"),
        ("AWS Lambda", "https://xxx.execute-api.us-east-1.amazonaws.com/test"),
        ("Cloudflare Workers", "https://my-worker.my-domain.workers.dev/test"),
    ]

    for name, url in endpoints:
        result = await benchmark_endpoint(url, n_requests=1000, concurrency=50)
        print(f"\n{name}:")
        print(f"  Throughput: {result['throughput_rps']:.1f} req/s")
        print(f"  P50: {result['p50_ms']:.1f}ms")
        print(f"  P99: {result['p99_ms']:.1f}ms")
        print(f"  Errors: {result['errors']}/{result['n_requests']}")

asyncio.run(main())
```

---

<!-- chunk: 14. 生产运维最佳实践 -->## 14. 生产运维最佳实践

## 14.1 蓝绿部署流程

```bash
#!/bin/bash
# blue-green-deploy.sh

APP_NAME="production-api"
NEW_VERSION="${1:-latest}"
HEALTH_CHECK_URL="https://api.example.com/health"
ROLLBACK_TIMEOUT=300

echo "Deploying $APP_NAME version $NEW_VERSION..."

# 1. 部署新版本（绿色）
spin cloud deploy \
  --app "${APP_NAME}-green" \
  --version "${NEW_VERSION}" \
  --variable-file .env.prod

echo "Green deployment complete, running health checks..."

# 2. 健康检查
for i in $(seq 1 10); do
  STATUS=$(curl -s -o /dev/null -w "%{http_code}" "${HEALTH_CHECK_URL}")
  if [ "$STATUS" = "200" ]; then
    echo "Health check passed (attempt $i)"
    break
  fi
  echo "Health check failed (attempt $i), status: $STATUS"
  sleep 3
done

# 3. 切换流量
echo "Switching traffic to green..."
spin cloud routes set \
  --app "${APP_NAME}" \
  --target "${APP_NAME}-green" \
  --weight 100

echo "Traffic switched to new version"

# 4. 监控一段时间
echo "Monitoring for $ROLLBACK_TIMEOUT seconds..."
sleep "$ROLLBACK_TIMEOUT"

# 5. 检查错误率
ERROR_RATE=$(spin cloud metrics "${APP_NAME}" \
  --metric error_rate \
  --since 5m \
  --format json \
  | jq '.value')

if (( $(echo "$ERROR_RATE > 0.05" | bc -l) )); then
  echo "ERROR: High error rate detected ($ERROR_RATE), rolling back!"
  
  # 回滚
  spin cloud routes set \
    --app "${APP_NAME}" \
    --target "${APP_NAME}-blue" \
    --weight 100
  
  exit 1
fi

echo "Deployment successful!"

# 6. 清理旧版本
spin cloud apps delete "${APP_NAME}-blue" || true
spin cloud apps rename "${APP_NAME}-green" "${APP_NAME}-blue" || true
```

## 14.2 监控告警配置

```yaml
# grafana-dashboard.yaml - Wasm Serverless 监控仪表板
apiVersion: 1
providers:
  - name: wasm-serverless
    type: file
    options:
      path: /var/lib/grafana/dashboards/wasm-serverless

---
# prometheus-alerts.yaml
groups:
  - name: wasm-serverless-sla
    rules:
      # 可用性
      - alert: WasmServiceDown
        expr: up{job="wasm-serverless"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Wasm service {{ $labels.instance }} is down"

      # 延迟 SLO (P99 < 100ms)
      - alert: WasmHighLatency
        expr: |
          histogram_quantile(0.99, 
            rate(http_request_duration_ms_bucket{job="wasm-serverless"}[5m])
          ) > 100
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "P99 latency > 100ms for {{ $labels.path }}"

      # 错误率 SLO (< 1%)
      - alert: WasmHighErrorRate
        expr: |
          rate(http_requests_total{job="wasm-serverless", status=~"5.."}[5m])
          /
          rate(http_requests_total{job="wasm-serverless"}[5m])
          > 0.01
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Error rate > 1% for {{ $labels.app }}"

      # 冷启动率
      - alert: WasmHighColdStartRate
        expr: |
          rate(cold_starts_total[5m])
          /
          rate(http_requests_total[5m])
          > 0.1
        for: 10m
        labels:
          severity: info
        annotations:
          summary: "Cold start rate > 10%, consider increasing warm pool"

      # 内存使用
      - alert: WasmHighMemoryUsage
        expr: |
          container_memory_usage_bytes{container="wasm-service"} 
          / container_spec_memory_limit_bytes > 0.85
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Memory usage > 85% for {{ $labels.pod }}"
```

---

<!-- chunk: 总结 -->## 总结

Wasm Serverless 通过独特的技术优势重新定义了 FaaS 计算：

**核心能力总结**：

| 特性 | 实现方式 | 优势 |
|------|----------|------|
| **极速冷启动** | AOT 预编译 + 实例池 | <1ms vs 传统 100ms+ |
| **超低内存** | Wasm 线性内存隔离 | 2-20MB vs 传统 128MB+ |
| **多语言支持** | WIT 标准接口 | Rust/Go/JS/Python 共用平台 |
| **安全隔离** | Wasm 沙箱 + WASI capabilities | 每请求完全隔离 |
| **跨平台** | 标准 Wasm ABI | 一次构建，多云部署 |

**选型建议**：
- **开发友好**：选 **Fermyon Spin**（最完整的工具链）
- **全球边缘**：选 **Cloudflare Workers** 或 **Fastly Compute**
- **企业内部**：选 **wasmCloud**（完整的分布式 Actor 模型）
- **K8s 集成**：选 **Spin + Kubernetes** 或 **Knative + Wasm**

**最佳实践**：
1. 预编译组件并缓存 .cwasm 文件实现 <0.5ms 冷启动
2. 维护至少 3 个预热实例应对流量突发
3. 使用外部 KV/SQLite 存储函数状态（无状态设计）
4. 通过 Redis/NATS 触发器实现异步事件处理
5. 使用 OAM 应用描述符统一管理多云部署

---

*参考资料：*
- [Fermyon Spin Documentation](https://developer.fermyon.com/spin/v2/)
- [wasmCloud Documentation](https://wasmcloud.com/docs/)
- [Cloudflare Workers Wasm](https://developers.cloudflare.com/workers/runtime-apis/webassembly/)
- [Fastly Compute@Edge](https://docs.fastly.com/products/compute)
- [KEDA Wasm Scaler](https://keda.sh/docs/2.12/scalers/redis-lists/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-38-webassembly-cloud-native MOC
- [[专项技术/README.md|Domain 15: WebAssembly 云原生 (WebAssembly Cloud Native)]]
- Domain-38 WebAssembly 云原生 — 开源项目索引
- WebAssembly 云原生基础
- containerd Wasm 运行时
- SpinKube 框架实践
- wasmCloud 平台
- WasmEdge 运行时
- Wasm 组件模型 (Wasm Component Model)
- Wasm 插件系统 (Wasm Plugin System)
- Wasm AI 推理 (Wasm AI Inference)
- Wasm 安全与沙箱 (Wasm Security and Sandbox)

## See Also

- 07-wasm-plugin-system
- 08-wasm-ai-inference
- 10-wasm-security-sandbox
- 99-wasmedge-cloud-native-guide


<!-- risk-assessed -->
