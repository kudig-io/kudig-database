---
title: WebAssembly 云原生基础
description: 1. [WebAssembly 概述](#1-webassembly-概述)
summary: 1. [WebAssembly 概述](#1-webassembly-概述)
category: webassembly-cloud-native
tags:
- k8s
- wasm
- webassembly
- cloud-native
- scheduler
- containerd
- docker
- redis
- hpa
- job
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
- WebAssembly 云原生基础 是什么
- 如何 WebAssembly 云原生基础
- Kubernetes 38 webassembly cloud native 最佳实践
trigger_keywords:
- WebAssembly
- 云原生基础
- webassembly
- cloud
- native
prerequisites:
- kubectl-basics
- redis-basics
- gpu-scheduling-basics
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




# WebAssembly 云原生基础
# WebAssembly Cloud Native Fundamentals

<!-- chunk: 目录 / Table of Contents -->## 目录 / Table of Contents

1. [WebAssembly 概述](#1-webassembly-概述)
2. [Wasm 二进制格式与架构](#2-wasm-二进制格式与架构)
3. [线性内存模型](#3-线性内存模型)
4. [WASI - WebAssembly 系统接口](#4-wasi---webassembly-系统接口)
5. [Wasm vs 容器对比](#5-wasm-vs-容器对比)
6. [云原生用例](#6-云原生用例)
7. [工具链与编译](#7-工具链与编译)
8. [组件模型](#8-组件模型)
9. [安全模型](#9-安全模型)
10. [性能分析与优化](#10-性能分析与优化)
11. [生态系统与运行时](#11-生态系统与运行时)
12. [实践示例](#12-实践示例)

---

<!-- chunk: 1. WebAssembly 概述 -->## 1. WebAssembly 概述

## 1.1 什么是 WebAssembly / What is WebAssembly

WebAssembly（缩写 Wasm）是一种基于栈式虚拟机的二进制指令格式。它被设计为编程语言的可移植编译目标，可以在 Web 上部署高性能客户端和服务端应用。

```
WebAssembly 核心特性：
┌─────────────────────────────────────────────────────┐
│  快速 (Fast)          │  接近原生的执行速度            │
│  安全 (Safe)          │  内存安全的沙箱执行环境         │
│  开放 (Open)          │  W3C 标准，厂商中立            │
│  可移植 (Portable)    │  跨平台、跨语言                │
└─────────────────────────────────────────────────────┘
```

WebAssembly 于 2019 年 12 月成为 W3C 官方标准，并被所有主流浏览器所支持。自 2020 年起，其在服务端和云原生领域的应用快速增长。

## 1.2 历史演进 / History & Evolution

```mermaid
timeline
    title WebAssembly 发展历程
    2015 : WebAssembly 概念提出
         : Mozilla、Google、Apple、Microsoft 联合开发
    2017 : MVP (Minimum Viable Product) 发布
         : 所有主流浏览器支持
    2019 : W3C 官方标准
         : WASI 0.1 草案发布
    2020 : Wasmtime 1.0
         : 服务端 Wasm 兴起
    2022 : 组件模型提案 (Component Model)
         : WIT (Wasm Interface Types)
    2023 : WASI 0.2 Preview 发布
         : containerd wasm shim 生产就绪
    2024 : WASI 0.2 稳定版
         : Kubernetes Wasm 集成成熟
    2025 : 云原生 Wasm 标准化
         : AI/ML 推理场景爆发
```

## 1.3 为什么关注云原生 Wasm / Why Cloud Native Wasm

```mermaid
graph TD
    A[传统容器痛点] --> B[启动时间 100ms-1s]
    A --> C[镜像大小 MB-GB]
    A --> D[内存占用 MB-GB]
    A --> E[OS 依赖]
    
    F[WebAssembly 优势] --> G[启动时间 < 1ms]
    F --> H[模块大小 KB-MB]
    F --> I[内存占用 KB-MB]
    F --> J[平台无关]
    
    K[云原生场景] --> L[Serverless/FaaS]
    K --> M[边缘计算]
    K --> N[插件系统]
    K --> O[AI 推理]
    K --> P[微服务]
```

**核心优势数据对比：**

| 指标 | 传统容器 | Wasm 模块 | 提升比例 |
|------|----------|-----------|----------|
| 冷启动时间 | 100ms ~ 1s | < 1ms | 100x ~ 1000x |
| 镜像/模块大小 | 50MB ~ 1GB | 100KB ~ 10MB | 10x ~ 100x |
| 内存占用 | 50MB ~ 512MB | 1MB ~ 50MB | 10x ~ 50x |
| CPU overhead | 较高 | 接近原生 | ~20% 差距 |
| 安全隔离 | cgroup/namespace | 沙箱 + 能力模型 | 更细粒度 |

---

<!-- chunk: 2. Wasm 二进制格式与架构 -->## 2. Wasm 二进制格式与架构

## 2.1 模块结构 / Module Structure

WebAssembly 模块是二进制编码的，由多个 Section（节）组成：

```
WebAssembly 二进制格式结构
┌─────────────────────────────────────────────────────────┐
│  Magic Number: 0x00 0x61 0x73 0x6D  (\0asm)             │
│  Version:      0x01 0x00 0x00 0x00  (1)                 │
├─────────────────────────────────────────────────────────┤
│  Section 1:  Type Section    (函数签名/类型定义)           │
│  Section 2:  Import Section  (导入声明)                  │
│  Section 3:  Function Section(函数索引)                  │
│  Section 4:  Table Section   (函数引用表)                │
│  Section 5:  Memory Section  (线性内存定义)              │
│  Section 6:  Global Section  (全局变量)                  │
│  Section 7:  Export Section  (导出声明)                  │
│  Section 8:  Start Section   (启动函数)                  │
│  Section 9:  Element Section (表元素初始化)              │
│  Section 10: Code Section    (函数代码体)                │
│  Section 11: Data Section    (内存数据初始化)            │
│  Section 12: Custom Section  (自定义扩展数据)            │
└─────────────────────────────────────────────────────────┘
```

## 2.2 WAT - WebAssembly 文本格式 / Text Format

WAT (WebAssembly Text Format) 是 Wasm 二进制的人类可读表示：

```wat
;; 简单的加法函数
(module
  ;; 类型定义
  (type $add_type (func (param i32 i32) (result i32)))
  
  ;; 函数实现
  (func $add (type $add_type)
    local.get 0    ;; 获取参数 0
    local.get 1    ;; 获取参数 1
    i32.add        ;; 整数加法
  )
  
  ;; 导出函数
  (export "add" (func $add))
)
```

```wat
;; 内存操作示例
(module
  ;; 声明 1 页内存 (64KB)
  (memory $mem 1)
  
  ;; 写入字符串到内存
  (data (i32.const 0) "Hello, WebAssembly!\00")
  
  ;; 函数：返回字符串指针和长度
  (func $get_string (result i32 i32)
    i32.const 0    ;; 指针
    i32.const 19   ;; 长度
  )
  
  (export "memory" (memory $mem))
  (export "get_string" (func $get_string))
)
```

## 2.3 栈式虚拟机 / Stack-based Virtual Machine

```mermaid
graph LR
    subgraph "栈式 VM 执行过程"
        A[指令流] --> B[解码]
        B --> C[执行引擎]
        C --> D[操作数栈]
        D --> E[结果]
    end
    
    subgraph "执行示例: i32.add"
        F["栈: [3, 4]"] --> G["i32.add"]
        G --> H["栈: [7]"]
    end
```

**类型系统 / Type System：**

```
WebAssembly 值类型
├── 数值类型 (Number Types)
│   ├── i32  - 32位整数
│   ├── i64  - 64位整数
│   ├── f32  - 32位浮点数
│   └── f64  - 64位浮点数
├── 向量类型 (Vector Types)
│   └── v128 - 128位 SIMD 向量
└── 引用类型 (Reference Types)
    ├── funcref  - 函数引用
    └── externref - 外部引用
```

## 2.4 指令集架构 / Instruction Set

```wat
;; 控制流指令
(module
  (func $fibonacci (param $n i32) (result i32)
    ;; if-else 示例
    (if (result i32) (i32.le_s (local.get $n) (i32.const 1))
      (then
        local.get $n
      )
      (else
        ;; 递归调用
        (i32.add
          (call $fibonacci (i32.sub (local.get $n) (i32.const 1)))
          (call $fibonacci (i32.sub (local.get $n) (i32.const 2)))
        )
      )
    )
  )
  (export "fibonacci" (func $fibonacci))
)
```

```wat
;; 循环指令
(module
  (func $sum (param $n i32) (result i32)
    (local $i i32)
    (local $result i32)
    
    ;; 初始化
    (local.set $i (i32.const 0))
    (local.set $result (i32.const 0))
    
    ;; loop 块
    (block $break
      (loop $continue
        ;; 条件判断
        (br_if $break (i32.ge_s (local.get $i) (local.get $n)))
        
        ;; result += i
        (local.set $result
          (i32.add (local.get $result) (local.get $i))
        )
        
        ;; i++
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        
        ;; 继续循环
        (br $continue)
      )
    )
    
    local.get $result
  )
  (export "sum" (func $sum))
)
```

---

<!-- chunk: 3. 线性内存模型 -->## 3. 线性内存模型

## 3.1 内存概念 / Memory Concepts

WebAssembly 的内存模型基于线性内存（Linear Memory），是一块连续的字节数组：

```
线性内存布局
┌──────────────────────────────────────────────────────────┐
│  地址 0                                                   │
│  ┌────────────────────────────────────────────────────┐  │
│  │  数据段 (Data Segment)                              │  │
│  │  - 字符串常量                                       │  │
│  │  - 全局数据                                         │  │
│  ├────────────────────────────────────────────────────┤  │
│  │  堆 (Heap)                                          │  │
│  │  - 动态分配的内存                                   │  │
│  │  - malloc/free 管理                                 │  │
│  ├────────────────────────────────────────────────────┤  │
│  │  栈 (Stack)                                         │  │
│  │  - 函数调用栈                                       │  │
│  │  - 局部变量                                         │  │
│  └────────────────────────────────────────────────────┘  │
│  地址 N * 64KB (N 页)                                     │
└──────────────────────────────────────────────────────────┘

注意：
- 每页大小固定为 64KB (65536 字节)
- 最大内存 4GB (2^32 字节) - 32位地址空间
- Memory64 提案支持 64 位地址空间
```

## 3.2 内存操作 / Memory Operations

```rust
// Rust 示例：通过 wasm-bindgen 与 JS 共享内存
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn process_bytes(input: &[u8]) -> Vec<u8> {
    // Rust 直接操作 Wasm 线性内存中的字节切片
    input.iter().map(|&b| b.wrapping_add(1)).collect()
}

#[wasm_bindgen]
pub fn allocate_buffer(size: usize) -> *mut u8 {
    // 分配内存并返回指针
    let mut buf = Vec::with_capacity(size);
    let ptr = buf.as_mut_ptr();
    std::mem::forget(buf); // 防止 Rust 自动释放
    ptr
}

#[wasm_bindgen]
pub fn free_buffer(ptr: *mut u8, size: usize) {
    unsafe {
        // 重新获取所有权并让 Rust 自动释放
        let _ = Vec::from_raw_parts(ptr, 0, size);
    }
}
```

```c
// C 示例：手动内存管理
#include <stdint.h>
#include <string.h>

// 简单内存分配器
static uint8_t heap[65536];
static uint32_t heap_top = 0;

void* wasm_malloc(uint32_t size) {
    if (heap_top + size > sizeof(heap)) {
        return 0; // OOM
    }
    void* ptr = &heap[heap_top];
    heap_top += size;
    return ptr;
}

// 字符串复制操作
__attribute__((export_name("copy_string")))
uint32_t copy_string(const char* src, uint32_t len) {
    char* dst = (char*)wasm_malloc(len + 1);
    if (!dst) return 0;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return (uint32_t)(uintptr_t)dst;
}
```

## 3.3 内存增长 / Memory Growth

```wat
;; 动态内存增长
(module
  (memory $mem 1 10)  ;; 初始 1 页，最大 10 页
  
  (func $grow_memory (param $pages i32) (result i32)
    ;; memory.grow 返回旧的页数，失败返回 -1
    (memory.grow (local.get $pages))
  )
  
  (func $current_memory (result i32)
    memory.size
  )
  
  (export "grow" (func $grow_memory))
  (export "size" (func $current_memory))
  (export "memory" (memory $mem))
)
```

## 3.4 共享内存与线程 / Shared Memory & Threads

```javascript
// JavaScript 中创建共享内存
const sharedMemory = new WebAssembly.Memory({
  initial: 10,
  maximum: 100,
  shared: true  // SharedArrayBuffer
});

// 在多个 Worker 中共享
const worker = new Worker('wasm-worker.js');
worker.postMessage({ memory: sharedMemory });

// 原子操作
const i32 = new Int32Array(sharedMemory.buffer);
Atomics.add(i32, 0, 1);      // 原子加法
Atomics.store(i32, 1, 42);   // 原子存储
Atomics.load(i32, 1);         // 原子读取
```

---

<!-- chunk: 4. WASI - WebAssembly 系统接口 -->## 4. WASI - WebAssembly 系统接口

## 4.1 WASI 概述 / WASI Overview

WASI (WebAssembly System Interface) 是 WebAssembly 的系统级 API 标准，让 Wasm 模块能够以安全、可移植的方式访问系统资源：

```mermaid
graph TD
    subgraph "WASI 架构"
        A[Wasm 模块] --> B[WASI API]
        B --> C{能力检查}
        C -->|授权| D[系统资源访问]
        C -->|拒绝| E[权限错误]
        
        D --> F[文件系统]
        D --> G[网络 Socket]
        D --> H[时钟/时间]
        D --> I[随机数]
        D --> J[环境变量]
        D --> K[进程管理]
    end
    
    subgraph "WASI 版本"
        L[WASI Preview 1] --> M[稳定文件系统 API]
        N[WASI Preview 2] --> O[组件模型集成]
        N --> P[HTTP/网络 API]
        N --> Q[键值存储 API]
    end
```

## 4.2 WASI Preview 1 核心 API / Core APIs

```rust
// Rust 使用 WASI 文件系统操作
use std::fs;
use std::io::{Read, Write};

fn main() {
    // WASI 文件读取
    let mut file = fs::File::open("/data/input.txt")
        .expect("无法打开文件");
    
    let mut content = String::new();
    file.read_to_string(&mut content)
        .expect("读取失败");
    
    println!("文件内容: {}", content);
    
    // WASI 文件写入
    let mut output = fs::File::create("/data/output.txt")
        .expect("无法创建文件");
    
    output.write_all(b"Hello from WASI!\n")
        .expect("写入失败");
    
    // WASI 环境变量
    if let Ok(val) = std::env::var("MY_CONFIG") {
        println!("配置值: {}", val);
    }
    
    // WASI 命令行参数
    let args: Vec<String> = std::env::args().collect();
    println!("参数: {:?}", args);
}
```

```toml
# Cargo.toml - 编译目标配置
[package]
name = "wasi-example"
version = "0.1.0"
edition = "2021"

[dependencies]
# WASI 绑定
wasi = "0.11"

bin
name = "wasi-example"

# 构建命令: cargo build --target wasm32-wasi
```

## 4.3 WASI Preview 2 与组件模型 / Component Model

```
WASI Preview 2 核心接口 (WIT 格式)

package wasi:io@0.2.0;

interface streams {
  type input-stream = resource;
  type output-stream = resource;
  
  read: func(
    self: borrow<input-stream>,
    len: u64
  ) -> result<list<u8>, stream-error>;
  
  write: func(
    self: borrow<output-stream>,
    contents: list<u8>
  ) -> result<_, stream-error>;
}
```

```rust
// 使用 wit-bindgen 生成的 WASI Preview 2 代码
use wasi::http::types::*;

// HTTP Handler 实现
struct HttpHandler;

impl wasi::exports::http::incoming_handler::Guest for HttpHandler {
    fn handle(request: IncomingRequest, response_out: ResponseOutparam) {
        let response = OutgoingResponse::new(Fields::new());
        response.set_status_code(200).unwrap();
        
        let body = response.body().unwrap();
        let stream = body.write().unwrap();
        stream.write(b"Hello from WASI!").unwrap();
        drop(stream);
        OutgoingBody::finish(body, None).unwrap();
        
        ResponseOutparam::set(response_out, Ok(response));
    }
}

wasi::http::proxy::export!(HttpHandler);
```

## 4.4 能力安全模型 / Capability Security Model

```
WASI 能力安全 (Capability-based Security)

传统 POSIX 系统：
  进程默认可访问任何文件（受 UID/GID 限制）
  
WASI 能力模型：
  模块默认无任何权限
  只有被显式传递的资源描述符才能被访问
  
示例：
  # 运行时显式授权目录访问
  wasmtime run \
    --dir /data/input::/ \      # 挂载输入目录
    --dir /data/output::/out \  # 挂载输出目录
    --env FOO=bar \             # 传递环境变量
    my_module.wasm
```

```go
// Go 使用 wazero 运行 WASI 模块（服务端）
package main

import (
    "context"
    "os"
    
    "github.com/tetratelabs/wazero"
    "github.com/tetratelabs/wazero/imports/wasi_snapshot_preview1"
)

func main() {
    ctx := context.Background()
    
    // 创建运行时
    rt := wazero.NewRuntime(ctx)
    defer rt.Close(ctx)
    
    // 加载 WASI Preview 1
    wasi_snapshot_preview1.MustInstantiate(ctx, rt)
    
    // 配置模块
    config := wazero.NewModuleConfig().
        WithStdout(os.Stdout).
        WithStderr(os.Stderr).
        WithArgs("wasm-program", "--verbose").
        WithEnv("LOG_LEVEL", "debug").
        WithFSConfig(
            wazero.NewFSConfig().
                WithDirMount("/host/data", "/"),  // 目录挂载
        )
    
    // 加载并运行 Wasm 模块
    wasmBytes, _ := os.ReadFile("program.wasm")
    module, _ := rt.InstantiateWithConfig(ctx, wasmBytes, config)
    defer module.Close(ctx)
}
```

## 4.5 WASI 网络接口 / Network Interface

```rust
// WASI HTTP 客户端 (Preview 2)
use wasi::http::outgoing_handler;
use wasi::http::types::*;

pub fn fetch_url(url: &str) -> Result<String, String> {
    let request = OutgoingRequest::new(Fields::new());
    request.set_method(&Method::Get).map_err(|_| "设置方法失败")?;
    
    let uri = url.parse::<Uri>().map_err(|e| e.to_string())?;
    request.set_scheme(Some(&Scheme::Https))
        .map_err(|_| "设置 scheme 失败")?;
    request.set_authority(Some(uri.authority()))
        .map_err(|_| "设置 authority 失败")?;
    request.set_path_with_query(uri.path_and_query().map(|pq| pq.as_str()))
        .map_err(|_| "设置路径失败")?;
    
    // 发送请求
    let future = outgoing_handler::handle(request, None)
        .map_err(|e| format!("发送请求失败: {:?}", e))?;
    
    // 等待响应
    let response = future.get()
        .ok_or("无响应")?
        .map_err(|e| format!("响应错误: {:?}", e))?
        .map_err(|e| format!("HTTP 错误: {:?}", e))?;
    
    // 读取响应体
    let body = response.consume().map_err(|_| "消费响应体失败")?;
    let stream = body.stream().map_err(|_| "获取流失败")?;
    
    let mut bytes = Vec::new();
    loop {
        match stream.read(8192) {
            Ok(chunk) if chunk.is_empty() => break,
            Ok(chunk) => bytes.extend_from_slice(&chunk),
            Err(_) => break,
        }
    }
    
    String::from_utf8(bytes).map_err(|e| e.to_string())
}
```

---

<!-- chunk: 5. Wasm vs 容器对比 -->## 5. Wasm vs 容器对比

## 5.1 架构对比 / Architecture Comparison

```mermaid
graph TD
    subgraph "传统容器架构"
        A1[应用代码] --> B1[容器镜像]
        B1 --> C1[容器运行时 containerd/runc]
        C1 --> D1[Linux 内核]
        C1 --> E1[cgroup/namespace]
        D1 --> F1[硬件]
    end
    
    subgraph "WebAssembly 架构"
        A2[应用代码] --> B2[Wasm 模块]
        B2 --> C2[Wasm 运行时 Wasmtime/WasmEdge]
        C2 --> D2[操作系统 API]
        D2 --> F2[硬件]
    end
```

## 5.2 详细对比表 / Detailed Comparison

| 维度 | Docker 容器 | Wasm 模块 | 备注 |
|------|------------|-----------|------|
| **启动时间** | 100ms ~ 1s | < 1ms | Wasm 快 100-1000x |
| **冷启动** | 慢（需拉取镜像） | 快（模块小） | Serverless 关键指标 |
| **镜像/模块大小** | 50MB ~ 2GB | 100KB ~ 10MB | Wasm 小 10-100x |
| **内存占用** | 50MB ~ 512MB | 1MB ~ 50MB | 显著降低 |
| **CPU 性能** | 接近原生 | 接近原生（有轻微 overhead） | < 20% 差距 |
| **安全隔离** | Namespace + Seccomp | 沙箱 + 能力模型 | Wasm 更细粒度 |
| **可移植性** | 跨 Linux 架构 | 跨所有平台/架构 | Wasm 更强 |
| **语言支持** | 任何语言 | C/C++/Rust/Go/... | 容器支持更广 |
| **系统调用** | 直接系统调用 | 通过 WASI 接口 | Wasm 有限制 |
| **网络** | 完整网络栈 | WASI Socket（有限） | 容器更完整 |
| **存储** | 完整文件系统 | 受限文件系统 | 容器更灵活 |
| **有状态应用** | 支持 | 有限支持 | 容器更适合 |
| **调试工具** | 成熟 | 发展中 | 容器生态更完善 |
| **生态系统** | 极其成熟 | 快速发展 | 容器生态更大 |
| **OCI 兼容** | 是 | 是（OCI Wasm Artifact）| 统一分发 |

## 5.3 使用场景选择 / Use Case Selection

```
何时使用 Wasm（而非容器）：

✅ 适合 Wasm 的场景：
  - Serverless/FaaS 函数（关注冷启动）
  - 边缘计算（资源受限节点）
  - 插件/扩展系统（动态加载，沙箱安全）
  - 多租户代码执行（强隔离需求）
  - AI 推理（轻量、可移植）
  - 短生命周期任务（快速启停）

❌ 不适合 Wasm 的场景：
  - 数据库（需完整 IO 访问）
  - 完整 Linux 应用（系统调用依赖）
  - GUI 应用
  - 需要完整网络栈的应用
  - 长运行的有状态服务（容器更成熟）
```

## 5.4 混合部署模式 / Hybrid Deployment

```yaml
# Kubernetes 中混合部署：容器 + Wasm
apiVersion: v1
kind: Pod
metadata:
  name: hybrid-app
spec:
  containers:
  # 传统容器：数据库、消息队列等
  - name: redis
    image: redis:7-alpine
    ports:
    - containerPort: 6379
  
  # Wasm 容器：轻量业务逻辑
  - name: handler
    image: ghcr.io/myorg/handler:latest
    runtimeClassName: wasmtime  # 使用 Wasm 运行时
    resources:
      limits:
        memory: "64Mi"
        cpu: "200m"
```

---

<!-- chunk: 6. 云原生用例 -->## 6. 云原生用例

## 6.1 Serverless / FaaS

```mermaid
sequenceDiagram
    participant Client as 客户端
    participant Gateway as API Gateway
    participant Scheduler as 调度器
    participant WasmRT as Wasm 运行时
    participant Func as Wasm 函数
    
    Client->>Gateway: HTTP 请求
    Gateway->>Scheduler: 路由到函数
    
    alt 冷启动 (< 1ms)
        Scheduler->>WasmRT: 加载 Wasm 模块
        WasmRT->>Func: 实例化函数
    else 热启动 (缓存)
        Scheduler->>Func: 直接调用
    end
    
    Func->>Func: 执行业务逻辑
    Func->>Gateway: 返回响应
    Gateway->>Client: HTTP 响应
    
    Note over WasmRT,Func: 执行后可立即销毁（scale-to-zero）
```

```rust
// Serverless Wasm 函数示例 (Spin Framework)
use spin_sdk::http::{IntoResponse, Request, Response};
use spin_sdk::http_component;

#[http_component]
fn handle_request(req: Request) -> anyhow::Result<impl IntoResponse> {
    println!("收到请求: {} {}", req.method(), req.uri());
    
    // 处理请求体
    let body = req.body();
    let response_body = format!("Echo: {}", 
        String::from_utf8_lossy(body));
    
    Ok(Response::builder()
        .status(200)
        .header("Content-Type", "text/plain")
        .body(response_body)
        .build())
}
```

## 6.2 边缘计算 / Edge Computing

```
边缘计算 Wasm 部署架构
                                                    
  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
  │  中心云      │    │  边缘节点1   │    │  边缘节点2   │
  │             │    │             │    │             │
  │  ┌────────┐ │    │  ┌────────┐ │    │  ┌────────┐ │
  │  │ 镜像   │ │───▶│  │  Wasm  │ │    │  │  Wasm  │ │
  │  │ 仓库   │ │    │  │ 模块   │ │    │  │ 模块   │ │
  │  └────────┘ │    │  └────────┘ │    │  └────────┘ │
  │             │    │             │    │             │
  │  配置管理   │    │  ARM/x86    │    │  RISC-V     │
  └─────────────┘    └─────────────┘    └─────────────┘
  
  Wasm 优势：
  - 同一模块文件运行在不同 CPU 架构
  - 模块小，适合低带宽分发
  - 低内存占用，适合资源受限设备
```

## 6.3 插件系统 / Plugin System

```go
// Go 实现 Wasm 插件系统（使用 wazero）
package main

import (
    "context"
    "fmt"
    "os"
    
    "github.com/tetratelabs/wazero"
    "github.com/tetratelabs/wazero/api"
)

// PluginManager 管理 Wasm 插件
type PluginManager struct {
    runtime wazero.Runtime
    plugins map[string]api.Module
}

func NewPluginManager() *PluginManager {
    ctx := context.Background()
    rt := wazero.NewRuntimeWithConfig(ctx,
        wazero.NewRuntimeConfig().WithCompilationCache(
            wazero.NewCompilationCache(),
        ),
    )
    return &PluginManager{
        runtime: rt,
        plugins: make(map[string]api.Module),
    }
}

func (pm *PluginManager) LoadPlugin(name, path string) error {
    ctx := context.Background()
    
    // 注册主机函数（Host Functions）
    _, err := pm.runtime.NewHostModuleBuilder("env").
        NewFunctionBuilder().
        WithFunc(func(ctx context.Context, msg uint32) {
            // 读取 Wasm 内存中的字符串
            module := ctx.Value("module").(api.Module)
            mem := module.Memory()
            bytes, _ := mem.Read(msg, 256)
            fmt.Printf("[插件日志] %s\n", bytes)
        }).
        Export("log").
        Instantiate(ctx)
    
    if err != nil {
        return fmt.Errorf("注册主机模块失败: %w", err)
    }
    
    // 加载插件
    wasmBytes, err := os.ReadFile(path)
    if err != nil {
        return fmt.Errorf("读取插件文件失败: %w", err)
    }
    
    module, err := pm.runtime.InstantiateWithConfig(ctx, wasmBytes,
        wazero.NewModuleConfig().WithName(name),
    )
    if err != nil {
        return fmt.Errorf("实例化插件失败: %w", err)
    }
    
    pm.plugins[name] = module
    return nil
}

func (pm *PluginManager) CallPlugin(name, funcName string, args ...uint64) ([]uint64, error) {
    ctx := context.Background()
    
    plugin, ok := pm.plugins[name]
    if !ok {
        return nil, fmt.Errorf("插件 %s 未找到", name)
    }
    
    fn := plugin.ExportedFunction(funcName)
    if fn == nil {
        return nil, fmt.Errorf("函数 %s 未导出", funcName)
    }
    
    return fn.Call(ctx, args...)
}
```

## 6.4 AI 推理 / AI Inference

```
Wasm AI 推理架构
                                                        
  ┌──────────────────────────────────────────────────┐
  │                  AI 应用层                        │
  │  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
  │  │  图像识别 │  │  NLP 处理 │  │  推荐系统    │   │
  │  └──────────┘  └──────────┘  └──────────────┘   │
  ├──────────────────────────────────────────────────┤
  │               Wasm AI 运行时层                    │
  │  ┌─────────┐ ┌─────────┐ ┌──────────────────┐   │
  │  │  ONNX   │ │TensorFlow│ │  WasmEdge NN     │   │
  │  │  Runtime│ │  Lite    │ │  (WASI-NN)       │   │
  │  └─────────┘ └─────────┘ └──────────────────┘   │
  ├──────────────────────────────────────────────────┤
  │               Wasm 运行时层                       │
  │  WasmEdge / Wasmtime / Wasmer                    │
  ├──────────────────────────────────────────────────┤
  │               硬件加速层                          │
  │  CPU / GPU / NPU / FPGA                          │
  └──────────────────────────────────────────────────┘
```

```rust
// WASI-NN AI 推理示例（WasmEdge）
use wasi_nn::{Graph, GraphEncoding, ExecutionTarget, TensorType};

fn run_inference(model_path: &str, input: &[f32]) -> Vec<f32> {
    // 加载 ONNX 模型
    let graph = Graph::load(
        &[std::fs::read(model_path).expect("读取模型失败")],
        GraphEncoding::Onnx,
        ExecutionTarget::CPU,
    ).expect("加载图失败");
    
    // 创建执行上下文
    let mut ctx = graph.init_execution_context()
        .expect("初始化上下文失败");
    
    // 设置输入张量
    ctx.set_input(
        0,
        TensorType::F32,
        &[1, 3, 224, 224],  // batch, channel, height, width
        input,
    ).expect("设置输入失败");
    
    // 执行推理
    ctx.compute().expect("推理失败");
    
    // 获取输出
    let output_size = 1000; // ImageNet 1000 类
    let mut output = vec![0f32; output_size];
    ctx.get_output(0, &mut output).expect("获取输出失败");
    
    output
}
```

## 6.5 数据处理管道 / Data Processing Pipeline

```yaml
# Knative + Wasm 数据处理管道
apiVersion: flows.knative.dev/v1
kind: Sequence
metadata:
  name: wasm-data-pipeline
spec:
  steps:
  - ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: wasm-parser       # Wasm: 数据解析
  - ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: wasm-transformer  # Wasm: 数据转换
  - ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: wasm-validator    # Wasm: 数据验证
  channelTemplate:
    apiVersion: messaging.knative.dev/v1
    kind: InMemoryChannel
```

---

<!-- chunk: 7. 工具链与编译 -->## 7. 工具链与编译

## 7.1 编译目标 / Compilation Targets

```
主要 Wasm 编译目标
                                
  wasm32-unknown-unknown   - 纯 Wasm（浏览器）
  wasm32-wasi              - WASI Preview 1
  wasm32-wasip1            - WASI Preview 1（新别名）
  wasm32-wasip2            - WASI Preview 2（组件模型）
  wasm32-unknown-emscripten - Emscripten（浏览器 + POSIX）
```

## 7.2 Rust 工具链 / Rust Toolchain

```bash
# 安装 Rust Wasm 工具链
rustup target add wasm32-wasi
rustup target add wasm32-unknown-unknown

# 安装 wasm-pack（Web 应用）
cargo install wasm-pack

# 安装 cargo-component（组件模型）
cargo install cargo-component

# 安装 wit-bindgen（接口绑定生成）
cargo install wit-bindgen-cli

# 编译 WASI 模块
cargo build --target wasm32-wasi --release

# 优化 Wasm 大小
cargo install wasm-opt
wasm-opt -Os target/wasm32-wasi/release/app.wasm -o app.wasm
```

```toml
# Cargo.toml - Wasm 组件配置
[package]
name = "my-wasm-component"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
wit-bindgen = "0.20"
wasi = "0.12"

[profile.release]
opt-level = "s"       # 优化大小
lto = true            # 链接时优化
codegen-units = 1     # 减少代码大小
panic = "abort"       # 避免 panic 处理代码
strip = true          # 剥离符号表
```

## 7.3 Go 工具链 / Go Toolchain

```bash
# TinyGo - 面向嵌入式和 Wasm 的 Go 编译器
# 安装 TinyGo
brew install tinygo  # macOS
# 或下载二进制

# 编译为 WASI
tinygo build -o app.wasm -target=wasi ./main.go

# 标准 Go 编译（实验性 WASI 支持）
GOOS=wasip1 GOARCH=wasm go build -o app.wasm .
```

```go
// Go WASI 示例
//go:build wasip1

package main

import (
    "fmt"
    "os"
)

func main() {
    // WASI 环境变量
    fmt.Println("WASI Go 程序启动")
    
    // 读取命令行参数
    for i, arg := range os.Args {
        fmt.Printf("参数 %d: %s\n", i, arg)
    }
    
    // 读取文件（需要 WASI 目录权限）
    data, err := os.ReadFile("/data/config.json")
    if err != nil {
        fmt.Fprintf(os.Stderr, "读取文件错误: %v\n", err)
        os.Exit(1)
    }
    
    fmt.Printf("配置文件内容: %s\n", data)
}
```

## 7.4 AssemblyScript / TypeScript-like

```typescript
// AssemblyScript - TypeScript 子集，编译为 Wasm
// assembly/index.ts

export function fibonacci(n: i32): i32 {
  if (n <= 1) return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}

export function add(a: i32, b: i32): i32 {
  return a + b;
}

// 内存操作
export function allocate(size: i32): i32 {
  return heap.alloc(size) as i32;
}

export function deallocate(ptr: i32): void {
  heap.free(changetype<usize>(ptr));
}
```

```json
// package.json
{
  "scripts": {
    "build": "asc assembly/index.ts -o build/release.wasm --optimize",
    "build:debug": "asc assembly/index.ts -o build/debug.wasm --debug"
  },
  "devDependencies": {
    "assemblyscript": "^0.27"
  }
}
```

## 7.5 WASM 工具 / Tools

```bash
# wabt - WebAssembly 二进制工具包
brew install wabt

# wat2wasm: 文本格式转二进制
wat2wasm example.wat -o example.wasm

# wasm2wat: 二进制转文本格式（反汇编）
wasm2wat example.wasm -o example.wat

# wasm-objdump: 模块信息查看
wasm-objdump -x example.wasm

# wasm-validate: 验证模块合法性
wasm-validate example.wasm

# wasm-strip: 剥离调试信息
wasm-strip example.wasm

# wasm-opt (binaryen): 优化 Wasm 模块
wasm-opt -O3 -o optimized.wasm input.wasm
```

---

<!-- chunk: 8. 组件模型 -->## 8. 组件模型

## 8.1 组件模型概述 / Component Model Overview

```mermaid
graph TD
    subgraph "Wasm 核心模块 (Core Module)"
        A[线性内存]
        B[函数]
        C[表]
    end
    
    subgraph "Wasm 组件 (Component)"
        D[组件接口 WIT]
        E[核心模块实例]
        F[类型适配器]
    end
    
    subgraph "组合层 (Composition Layer)"
        G[组件 A] --> H[导入/导出绑定]
        I[组件 B] --> H
        H --> J[组合应用]
    end
    
    A --> E
    B --> E
    E --> D
    D --> G
    D --> I
```

## 8.2 WIT - Wasm 接口类型 / Interface Types

```wit
// world.wit - 定义组件世界（接口）
package my:app@1.0.0;

// 定义接口
interface types {
  // 资源类型
  resource user {
    constructor(id: u64, name: string);
    get-id: func() -> u64;
    get-name: func() -> string;
    set-name: func(name: string);
  }
  
  // 枚举
  enum status {
    active,
    inactive,
    pending,
  }
  
  // 变体（Tagged Union）
  variant result {
    ok(string),
    err(string),
  }
  
  // Record（结构体）
  record request {
    path: string,
    method: string,
    headers: list<tuple<string, string>>,
    body: option<list<u8>>,
  }
}

// 定义 HTTP 接口
interface http-handler {
  use types.{request, result};
  handle: func(req: request) -> result;
}

// 世界定义
world http-service {
  // 导入系统接口
  import wasi:http/incoming-handler@0.2.0;
  
  // 导出业务接口
  export http-handler;
}
```

## 8.3 组件组合 / Component Composition

```bash
# 使用 wasm-tools 组合组件
cargo install wasm-tools

# 构建多个组件
cargo component build --release

# 查看组件接口
wasm-tools component wit my-component.wasm

# 组合两个组件
wasm-tools compose \
  -d database-component.wasm \
  -d cache-component.wasm \
  app-component.wasm \
  -o composed-app.wasm
```

---

<!-- chunk: 9. 安全模型 -->## 9. 安全模型

## 9.1 Wasm 沙箱 / Wasm Sandbox

```
WebAssembly 安全层次

┌─────────────────────────────────────────────┐
│  应用层安全                                  │
│  - 类型安全 (Type Safety)                   │
│  - 无未定义行为                              │
├─────────────────────────────────────────────┤
│  内存安全                                    │
│  - 线性内存访问边界检查                      │
│  - 无指针算术越界                            │
│  - 无悬挂指针                               │
├─────────────────────────────────────────────┤
│  沙箱隔离                                    │
│  - 默认无主机访问                            │
│  - 能力驱动的资源访问                        │
│  - 函数调用表隔离                            │
├─────────────────────────────────────────────┤
│  运行时安全                                  │
│  - JIT 代码验证                              │
│  - Spectre 缓解措施                         │
│  - 栈溢出保护                               │
└─────────────────────────────────────────────┘
```

## 9.2 多租户安全 / Multi-tenant Security

```rust
// 多租户 Wasm 执行引擎
use wasmtime::*;
use std::collections::HashMap;

struct MultiTenantRuntime {
    engine: Engine,
    tenants: HashMap<String, (Store<()>, Instance)>,
}

impl MultiTenantRuntime {
    fn new() -> Self {
        let mut config = Config::new();
        // 安全配置
        config.cranelift_opt_level(OptLevel::Speed);
        config.epoch_interruption(true);  // 支持中断
        config.consume_fuel(true);        // 资源限制
        
        Self {
            engine: Engine::new(&config).unwrap(),
            tenants: HashMap::new(),
        }
    }
    
    fn add_tenant(&mut self, id: &str, wasm_bytes: &[u8]) -> Result<()> {
        let module = Module::new(&self.engine, wasm_bytes)?;
        let mut store = Store::new(&self.engine, ());
        
        // 设置燃料限制（CPU 配额）
        store.set_fuel(1_000_000)?;
        
        // 设置内存限制
        store.limiter(|_| {
            let mut limiter = StoreLimitsBuilder::new();
            limiter.memory_size(64 * 1024 * 1024);  // 64MB
            limiter.build()
        });
        
        let instance = Instance::new(&mut store, &module, &[])?;
        self.tenants.insert(id.to_string(), (store, instance));
        
        Ok(())
    }
    
    fn call_tenant(&mut self, id: &str, func: &str, args: &[Val]) 
        -> Result<Vec<Val>> 
    {
        let (store, instance) = self.tenants.get_mut(id)
            .ok_or_else(|| anyhow::anyhow!("租户 {} 不存在", id))?;
        
        let f = instance.get_func(&mut *store, func)
            .ok_or_else(|| anyhow::anyhow!("函数 {} 不存在", func))?;
        
        let mut results = vec![Val::I32(0); f.ty(&*store).results().len()];
        f.call(&mut *store, args, &mut results)?;
        
        Ok(results)
    }
}
```

## 9.3 Spectre 防护 / Spectre Mitigation

```
Wasmtime Spectre 防护措施：

1. 线性内存访问防护
   - 使用虚拟内存保护（guard pages）
   - 在 x86 上利用 4GB 地址空间限制

2. 分支目标预测防护
   - 使用 retpoline 技术
   - 限制间接调用目标

3. 内存访问时序防护
   - 加载时边界检查
   - 避免推测性越界访问
```

---

<!-- chunk: 10. 性能分析与优化 -->## 10. 性能分析与优化

## 10.1 编译策略 / Compilation Strategies

```
Wasm 执行引擎编译策略
                                              
  解释执行 (Interpreter)
  - 最快启动
  - 最慢运行
  - 适合：短生命周期、一次性任务
  
  单遍编译 (Single-pass JIT)
  - 较快启动（ms 级）
  - 较慢运行（比 AOT 慢 2-3x）
  - 适合：Serverless 函数
  
  优化编译 (Optimizing JIT/AOT)
  - 慢启动（10ms-1s）
  - 接近原生性能
  - 适合：长运行服务
  
  AOT 预编译 (Ahead-of-Time)
  - 镜像时编译
  - 最快启动 + 最快运行
  - 适合：已知负载的生产环境
```

## 10.2 性能基准 / Performance Benchmarks

```
WebAssembly vs Native 性能对比（近似值）

数值计算：
  Native C:          1.0x
  Wasm (Wasmtime):   1.05 ~ 1.2x (略慢)
  
内存密集：
  Native C:          1.0x
  Wasm:              1.1 ~ 1.5x (有额外边界检查)
  
IO 密集：
  Native:            1.0x
  Wasm + WASI:       1.2 ~ 2.0x (有 WASI 调用开销)
  
启动时间（对比 JVM）：
  JVM (JIT 预热后):  基准
  Wasm (AOT):        100x 更快启动
  Wasm (JIT):        10x 更快启动
```

## 10.3 优化技巧 / Optimization Tips

```rust
// Rust Wasm 优化技巧

// 1. 避免 Box/Vec 频繁分配
// 不好的做法
fn bad_process(data: Vec<u8>) -> Vec<u8> {
    data.into_iter().map(|b| b + 1).collect() // 多次分配
}

// 好的做法
fn good_process(data: &mut [u8]) {
    data.iter_mut().for_each(|b| *b += 1); // 原地修改
}

// 2. 使用 SIMD 加速（需要 simd128 特性）
#[target_feature(enable = "simd128")]
unsafe fn simd_add(a: &[f32], b: &[f32], out: &mut [f32]) {
    use std::arch::wasm32::*;
    
    let chunks = a.len() / 4;
    for i in 0..chunks {
        let va = v128_load(a[i*4..].as_ptr() as *const v128);
        let vb = v128_load(b[i*4..].as_ptr() as *const v128);
        let vc = f32x4_add(va, vb);
        v128_store(out[i*4..].as_mut_ptr() as *mut v128, vc);
    }
}

// 3. 减少 JS-Wasm 互调用
// 每次 JS↔Wasm 调用都有开销，应该批量处理
#[no_mangle]
pub extern "C" fn batch_process(ptr: *mut u8, len: usize) -> usize {
    let slice = unsafe { std::slice::from_raw_parts_mut(ptr, len) };
    // 一次调用处理整个缓冲区
    let count = slice.iter_mut()
        .filter(|&&mut b| b > 0)
        .map(|b| { *b *= 2; *b })
        .count();
    count
}
```

---

<!-- chunk: 11. 生态系统与运行时 -->## 11. 生态系统与运行时

## 11.1 主要 Wasm 运行时对比 / Runtime Comparison

| 运行时 | 语言 | 许可证 | 特点 | 主要用途 |
|--------|------|--------|------|----------|
| **Wasmtime** | Rust | Apache-2.0 | Cranelift JIT, 安全优先 | 服务端、Kubernetes |
| **WasmEdge** | C++ | Apache-2.0 | AI/ML 支持, WASI-NN | 边缘、AI 推理 |
| **Wasmer** | Rust | MIT | 多后端 (LLVM/Cranelift) | 通用、嵌入式 |
| **wazero** | Go | Apache-2.0 | 纯 Go, 零依赖 | Go 应用嵌入 |
| **V8** | C++ | BSD | 最成熟, JS 引擎 | 浏览器、Deno |
| **SpiderMonkey** | C++ | MPL-2.0 | Firefox 引擎 | 浏览器 |

## 11.2 云原生 Wasm 项目 / Cloud Native Projects

```mermaid
graph TD
    subgraph "CNCF Wasm 项目"
        A[WasmEdge] --> B[CNCF Sandbox]
        C[Spin/SpinKube] --> D[CNCF Sandbox]
        E[Krustlet] --> F[已归档]
    end
    
    subgraph "相关工具"
        G[runwasi] --> H[containerd shim]
        I[wasm-workers-server] --> J[边缘部署]
        K[wasmCloud] --> L[分布式 Actor]
        M[Fermyon Spin] --> N[Serverless 框架]
    end
    
    subgraph "标准组织"
        O[W3C WebAssembly WG] --> P[核心规范]
        Q[WASI Subgroup] --> R[系统接口规范]
        S[Component Model WG] --> T[组件模型规范]
    end
```

## 11.3 OCI Wasm 工件标准 / OCI Wasm Artifact

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 将 Wasm 模块打包为 OCI 镜像
# 使用 wasm-to-oci 工具
wasm-to-oci push myapp.wasm ghcr.io/myorg/myapp:latest

# 查看 OCI 镜像层
docker manifest inspect ghcr.io/myorg/myapp:latest

# OCI Artifact 格式
# {
#   "mediaType": "application/vnd.oci.image.manifest.v1+json",
#   "layers": [{
#     "mediaType": "application/vnd.wasm.content.layer.v1+wasm",
#     "digest": "sha256:...",
#     "size": 1234
#   }],
#   "annotations": {
#     "org.opencontainers.image.created": "2024-01-01T00:00:00Z"
#   }
# }
```
---

<!-- chunk: 12. 实践示例 -->## 12. 实践示例

## 12.1 完整 Rust WASI 应用 / Complete Rust WASI App

```rust
// src/main.rs - 完整 WASI Web 服务器（使用 Spin）
use anyhow::Result;
use spin_sdk::{
    http::{IntoResponse, Method, Params, Request, Response, Router},
    http_component,
    key_value::Store,
};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug)]
struct User {
    id: u64,
    name: String,
    email: String,
}

// 注册 HTTP 路由
#[http_component]
fn handle_request(req: Request) -> Result<impl IntoResponse> {
    let mut router = Router::new();
    
    router.get("/users/:id", get_user);
    router.post("/users", create_user);
    router.delete("/users/:id", delete_user);
    
    Ok(router.handle(req))
}

// GET /users/:id
fn get_user(req: Request, params: Params) -> Result<impl IntoResponse> {
    let id = params.get("id").unwrap_or("0");
    
    // 从 KV Store 获取用户
    let store = Store::open_default()?;
    
    match store.get(&format!("user:{}", id))? {
        Some(data) => {
            let user: User = serde_json::from_slice(&data)?;
            Ok(Response::builder()
                .status(200)
                .header("Content-Type", "application/json")
                .body(serde_json::to_vec(&user)?)
                .build())
        }
        None => Ok(Response::builder()
            .status(404)
            .body("用户未找到")
            .build()),
    }
}

// POST /users
fn create_user(req: Request, _params: Params) -> Result<impl IntoResponse> {
    let user: User = serde_json::from_slice(req.body())?;
    
    // 存储到 KV Store
    let store = Store::open_default()?;
    store.set(
        &format!("user:{}", user.id),
        &serde_json::to_vec(&user)?,
    )?;
    
    Ok(Response::builder()
        .status(201)
        .header("Content-Type", "application/json")
        .header("Location", format!("/users/{}", user.id))
        .body(serde_json::to_vec(&user)?)
        .build())
}

// DELETE /users/:id
fn delete_user(_req: Request, params: Params) -> Result<impl IntoResponse> {
    let id = params.get("id").unwrap_or("0");
    
    let store = Store::open_default()?;
    store.delete(&format!("user:{}", id))?;
    
    Ok(Response::builder()
        .status(204)
        .body(())
        .build())
}
```

## 12.2 Kubernetes 部署配置 / Kubernetes Deployment

```yaml
# RuntimeClass 配置（需要 containerd wasm shim）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime-spin
handler: spin
scheduling:
  nodeClassification:
    nodeSelector:
      matchLabels:
        kubernetes.io/arch: wasm32

---
# Wasm 应用 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-user-service
  namespace: default
  labels:
    app: wasm-user-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: wasm-user-service
  template:
    metadata:
      labels:
        app: wasm-user-service
    spec:
      runtimeClassName: wasmtime-spin  # 使用 Wasm 运行时
      containers:
      - name: user-service
        image: ghcr.io/myorg/user-service:v1.0.0
        # Wasm 模块不需要 command/args
        env:
        - name: SPIN_APP_KV_STORE
          value: redis://redis-service:6379
        resources:
          requests:
            memory: "16Mi"
            cpu: "50m"
          limits:
            memory: "64Mi"
            cpu: "200m"
        ports:
        - containerPort: 80
          name: http

---
# Service
apiVersion: v1
kind: Service
metadata:
  name: wasm-user-service
spec:
  selector:
    app: wasm-user-service
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP

---
# HPA 自动伸缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: wasm-user-service-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: wasm-user-service
  minReplicas: 1
  maxReplicas: 50
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

## 12.3 完整构建流水线 / Complete CI/CD Pipeline

```yaml
# .github/workflows/wasm-build.yml
name: Build and Deploy Wasm

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

env:
  REGISTRY: ghcr.io
  IMAGE_NAME: ${{ github.repository }}

jobs:
  build-wasm:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    
    - name: 安装 Rust 工具链
      uses: dtolnay/rust-toolchain@stable
      with:
        targets: wasm32-wasi
    
    - name: 安装 Spin CLI
      uses: fermyon/actions/spin/setup@v1
      with:
        version: "v3.0.0"
    
    - name: 缓存 Rust 编译缓存
      uses: Swatinem/rust-cache@v2
    
    - name: 编译 Wasm 模块
      run: |
        cargo build --target wasm32-wasi --release
        ls -lh target/wasm32-wasi/release/*.wasm
    
    - name: 优化 Wasm 大小
      run: |
        cargo install wasm-opt
        wasm-opt -Os \
          target/wasm32-wasi/release/app.wasm \
          -o dist/app.wasm
        echo "优化后大小: $(du -sh dist/app.wasm)"
    
    - name: 运行 Wasm 测试
      run: |
        spin test
    
    - name: 构建并推送 OCI 镜像
      uses: docker/build-push-action@v5
      with:
        context: .
        push: ${{ github.event_name == 'push' }}
        tags: |
          ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}:latest
          ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}:${{ github.sha }}
    
    - name: 部署到 Kubernetes
      if: github.ref == 'refs/heads/main'
      run: |
        kubectl set image deployment/wasm-app \
          app=${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}:${{ github.sha }}
        kubectl rollout status deployment/wasm-app
```

## 12.4 性能测试 / Performance Testing

```go
// 性能测试：Wasm vs Native
package benchmark_test

import (
    "context"
    "testing"
    "os"
    
    "github.com/tetratelabs/wazero"
    "github.com/tetratelabs/wazero/imports/wasi_snapshot_preview1"
)

// 测试 Wasm 函数调用开销
func BenchmarkWasmFibonacci(b *testing.B) {
    ctx := context.Background()
    rt := wazero.NewRuntime(ctx)
    defer rt.Close(ctx)
    
    wasi_snapshot_preview1.MustInstantiate(ctx, rt)
    
    wasmBytes, _ := os.ReadFile("fibonacci.wasm")
    module, _ := rt.InstantiateWithConfig(ctx, wasmBytes,
        wazero.NewModuleConfig().WithName("fib"))
    defer module.Close(ctx)
    
    fn := module.ExportedFunction("fibonacci")
    
    b.ResetTimer()
    b.RunParallel(func(pb *testing.PB) {
        for pb.Next() {
            fn.Call(ctx, 30)
        }
    })
}

// 对比：Native Go 函数
func fibonacci(n uint64) uint64 {
    if n <= 1 {
        return n
    }
    return fibonacci(n-1) + fibonacci(n-2)
}

func BenchmarkNativeFibonacci(b *testing.B) {
    b.RunParallel(func(pb *testing.PB) {
        for pb.Next() {
            fibonacci(30)
        }
    })
}
```

---

<!-- chunk: 参考资料 / References -->## 参考资料 / References

## 官方规范 / Official Specifications
- [WebAssembly 核心规范](https://webassembly.github.io/spec/core/)
- [WASI Preview 2 规范](https://github.com/WebAssembly/WASI)
- [WebAssembly 组件模型](https://github.com/WebAssembly/component-model)
- [W3C WebAssembly 标准](https://www.w3.org/TR/wasm-core-2/)

## 运行时文档 / Runtime Documentation
- [Wasmtime 官方文档](https://docs.wasmtime.dev/)
- [WasmEdge 官方文档](https://wasmedge.org/docs/)
- [Wasmer 官方文档](https://docs.wasmer.io/)
- [wazero 官方文档](https://wazero.io/)

## 云原生集成 / Cloud Native Integration
- [containerd runwasi](https://github.com/containerd/runwasi)
- [Fermyon Spin 文档](https://developer.fermyon.com/spin/)
- [wasmCloud 文档](https://wasmcloud.com/docs/)

## 学习资源 / Learning Resources
- [Rust Wasm 书籍](https://rustwasm.github.io/docs/book/)
- [WASI Tutorial](https://github.com/bytecodealliance/wasmtime/blob/main/docs/WASI-tutorial.md)
- [WebAssembly.org](https://webassembly.org/)

---

*最后更新 / Last Updated: 2025-03-04*
*版本 / Version: 1.0.0*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-38-webassembly-cloud-native KUDIG Database — Global MOC
- [[16-专项技术/README.md|Domain 15: WebAssembly 云原生 (WebAssembly Cloud Native)]]
- Domain-38 WebAssembly 云原生 — 开源项目索引
- containerd Wasm 运行时
- SpinKube 框架实践
- wasmCloud 平台
- WasmEdge 运行时
- Wasm 组件模型 (Wasm Component Model)
- Wasm 插件系统 (Wasm Plugin System)
- Wasm AI 推理 (Wasm AI Inference)
- Wasm Serverless (Wasm Serverless)
- Wasm 安全与沙箱 (Wasm Security and Sandbox)

## See Also

- 10-wasm-security-sandbox
- 99-wasmedge-cloud-native-guide
- 02-containerd-wasm-shim
- 03-spinkube-framework


<!-- risk-assessed -->
