---
title: Wasm AI 推理 (Wasm AI Inference)
description: 通过 WebAssembly 在边缘节点和云原生环境中运行 AI/ML 推理，实现安全、高效、可移植的模型部署。
summary: 通过 WebAssembly 在边缘节点和云原生环境中运行 AI/ML 推理，实现安全、高效、可移植的模型部署。
category: webassembly-cloud-native
tags:
- k8s
- wasm
- webassembly
- cloud-native
- prometheus
- docker
- hpa
- gpu
- cuda
- serverless
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
- Wasm AI 推理 (Wasm AI Inference) 是什么
- 如何 Wasm AI 推理 (Wasm AI Inference)
- Kubernetes 38 webassembly cloud native 最佳实践
trigger_keywords:
- Wasm
- AI
- 推理
- Wasm
- AI
- Inference
- webassembly
- cloud
prerequisites:
- kubectl-basics
- prometheus-basics
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



# Wasm AI 推理 (Wasm AI Inference)

> 通过 WebAssembly 在边缘节点和云原生环境中运行 AI/ML 推理，实现安全、高效、可移植的模型部署。

---

<!-- chunk: 目录 -->## 目录

1. [Wasm AI 推理架构概述](#1-wasm-ai-推理架构概述)
2. [WASI-NN 标准接口](#2-wasi-nn-标准接口)
3. [[entities/wasmedge.md|WasmEdge]] WASI-NN 实践](#3-wasmedge-wasi-nn-实践)
4. [ONNX Runtime Wasm](#4-onnx-runtime-wasm)
5. [llama.cpp Wasm 移植](#5-llamacpp-wasm-移植)
6. [模型优化与量化](#6-模型优化与量化)
7. [边缘 AI 推理部署](#7-边缘-ai-推理部署)
8. [Rust AI 推理开发](#8-rust-ai-推理开发)
9. [Python/JS AI 推理集成](#9-pythonjs-ai-推理集成)
10. [多模型服务架构](#10-多模型服务架构)
11. [性能基准与对比](#11-性能基准与对比)
12. [[entities/kubernetes.md|Kubernetes]] AI 推理集成](#12-kubernetes-ai-推理集成)
13. [实战案例：图像分类服务](#13-实战案例图像分类服务)
14. [实战案例：LLM 推理服务](#14-实战案例llm-推理服务)

---

<!-- chunk: 1. Wasm AI 推理架构概述 -->## 1. Wasm AI 推理架构概述

## 1.1 为什么选择 Wasm 进行 AI 推理

```mermaid
graph TB
    subgraph "传统 AI 推理部署"
        Container[Docker 容器<br/>~500MB-2GB]
        GPU[GPU 依赖<br/>驱动版本锁定]
        Arch[架构绑定<br/>x86/ARM 不通用]
    end

    subgraph "Wasm AI 推理优势"
        Size[超轻量<br/>5-50MB]
        Port[跨平台<br/>x86/ARM/RISC-V]
        Sec[沙箱安全<br/>零权限默认]
        Fast[快速启动<br/><10ms]
        CPU_GPU[CPU/GPU<br/>统一接口]
    end

    WasmAI[Wasm AI Runtime] --> Size
    WasmAI --> Port
    WasmAI --> Sec
    WasmAI --> Fast
    WasmAI --> CPU_GPU
```

## 1.2 Wasm AI 推理生态全景

```mermaid
graph LR
    subgraph "模型格式"
        ONNX[ONNX]
        TFLite[TFLite]
        PT[PyTorch/TorchScript]
        GGUF[GGUF/llama.cpp]
        OpenVINO[OpenVINO IR]
    end

    subgraph "推理框架"
        WasmEdgeNN[WasmEdge WASI-NN]
        ONNXRT[ONNX Runtime Web]
        LlamaWasm[llama.cpp.wasm]
        Candle[Candle/Rust]
        Tract[Tract/Rust]
    end

    subgraph "部署目标"
        Browser[浏览器]
        Edge[边缘节点]
        K8s[Kubernetes]
        CDN[CDN/Serverless]
        IoT[IoT 设备]
    end

    ONNX --> WasmEdgeNN
    TFLite --> WasmEdgeNN
    PT --> WasmEdgeNN
    GGUF --> LlamaWasm
    ONNX --> ONNXRT
    PT --> Candle

    WasmEdgeNN --> Edge
    WasmEdgeNN --> K8s
    ONNXRT --> Browser
    LlamaWasm --> Browser
    LlamaWasm --> Edge
    Candle --> K8s
```

## 1.3 关键技术指标

```
Wasm AI 推理性能指标（2025年）：

┌──────────────────────────────────────────────────────────┐
│ 模型类型        │ 框架           │ 延迟   │ 吞吐量       │
├──────────────────────────────────────────────────────────┤
│ ResNet-50       │ WASI-NN/ORT   │ 8ms    │ 120 img/s    │
│ BERT-base       │ WASI-NN/ORT   │ 45ms   │ 22 req/s     │
│ Whisper-tiny    │ WasmEdge      │ 0.3x   │ 实时转录      │
│ LLaMA-7B(Q4)    │ llama.wasm    │ 15t/s  │ 单用户可用    │
│ YOLOv8n         │ WASI-NN       │ 25ms   │ 40 img/s     │
│ Stable Diffusion│ WebGPU+Wasm   │ ~60s   │ 1 img/min     │
└──────────────────────────────────────────────────────────┘

注：延迟数据基于 Apple M2 或等效 x86 CPU
```

---

<!-- chunk: 2. WASI-NN 标准接口 -->## 2. WASI-NN 标准接口

## 2.1 WASI-NN 接口定义

WASI-NN（WebAssembly System Interface for Neural Networks）是标准化的 AI 推理接口：

```wit
// wasi-nn.wit（简化版）
package wasi:nn@0.2.0;

interface graph {
    // 后端类型
    enum graph-encoding {
        openvino,
        onnx,
        tensorflow,
        pytorch,
        tensorflowlite,
        ggml,
        autodetect,
    }
    
    // 执行目标
    enum execution-target {
        cpu,
        gpu,
        tpu,
    }
    
    // 图（模型）句柄
    resource graph {
        // 初始化推理上下文
        init-execution-context: func() -> result<graph-execution-context, error>;
    }
    
    // 加载模型
    load: func(
        builder: list<list<u8>>,
        encoding: graph-encoding,
        target: execution-target,
    ) -> result<graph, error>;
    
    // 从预注册名称加载
    load-by-name: func(name: string) -> result<graph, error>;
}

interface inference {
    use graph.{graph-execution-context};
    
    resource graph-execution-context {
        // 设置输入张量
        set-input: func(
            index: u32,
            tensor: tensor,
        ) -> result<_, error>;
        
        // 执行推理
        compute: func() -> result<_, error>;
        
        // 获取输出张量
        get-output: func(index: u32) -> result<tensor, error>;
    }
}

interface tensor {
    // 张量类型
    enum tensor-type {
        fp16,
        fp32,
        fp64,
        bf16,
        u8,
        i32,
        i64,
    }
    
    record tensor {
        // 张量形状 [batch, channels, height, width]
        dimensions: list<u32>,
        ty: tensor-type,
        data: tensor-data,
    }
    
    type tensor-data = list<u8>;  // 原始字节数据
}
```

## 2.2 WASI-NN 调用流程

```mermaid
sequenceDiagram
    participant App as Wasm App
    participant Host as WASI-NN Host
    participant Backend as Backend (CPU/GPU)

    App->>Host: wasi_nn::load(model_bytes, ONNX, CPU)
    Host->>Backend: 初始化推理引擎
    Backend-->>Host: graph_handle
    Host-->>App: graph_handle

    App->>Host: graph.init_execution_context()
    Host->>Backend: 创建推理上下文
    Backend-->>Host: context_handle
    Host-->>App: context_handle

    loop 每次推理
        App->>Host: context.set_input(0, input_tensor)
        Host->>Backend: 设置输入数据
        App->>Host: context.compute()
        Host->>Backend: 执行推理
        Backend-->>Host: 推理完成
        App->>Host: context.get_output(0)
        Host-->>App: output_tensor
    end
```

## 2.3 WASI-NN 错误类型

```rust
// WASI-NN 错误处理
#[repr(u32)]
pub enum NnErrno {
    Success = 0,
    InvalidArgument = 1,
    InvalidEncoding = 2,
    Timeout = 3,
    RuntimeError = 4,
    UnsupportedOperation = 5,
    TooLarge = 6,
    NotFound = 7,
}

impl std::fmt::Display for NnErrno {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            NnErrno::Success => write!(f, "Success"),
            NnErrno::InvalidArgument => write!(f, "Invalid argument"),
            NnErrno::InvalidEncoding => write!(f, "Invalid model encoding"),
            NnErrno::Timeout => write!(f, "Inference timeout"),
            NnErrno::RuntimeError => write!(f, "Runtime error during inference"),
            NnErrno::UnsupportedOperation => write!(f, "Unsupported operation"),
            NnErrno::TooLarge => write!(f, "Input/output too large"),
            NnErrno::NotFound => write!(f, "Model or resource not found"),
        }
    }
}
```

---

<!-- chunk: 3. WasmEdge WASI-NN 实践 -->## 3. WasmEdge WASI-NN 实践

## 3.1 WasmEdge 安装与配置

```bash
# 安装 WasmEdge（带 WASI-NN 支持）
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | \
  bash -s -- --plugins wasi_nn-ggml wasi_nn-openvino

# 验证安装
wasmedge --version
wasmedge plugin list

# 安装 ONNX 后端
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | \
  bash -s -- --plugins wasi_nn-onnxruntime

# 配置环境
source ~/.bashrc
export WASMEDGE_PLUGIN_PATH=/usr/local/lib/wasmedge
```

## 3.2 完整图像分类示例

```rust
// Cargo.toml
// [dependencies]
// wasi-nn = "0.5.0"
// image = "0.24"
// ndarray = "0.15"

// src/main.rs - ONNX 图像分类
use wasi_nn::{
    ExecutionTarget, GraphBuilder, GraphEncoding,
    GraphExecutionContext, TensorType,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 1. 加载模型
    println!("Loading model...");
    let model_bytes = std::fs::read("resnet50.onnx")?;
    
    let graph = GraphBuilder::new(GraphEncoding::Onnx, ExecutionTarget::Cpu)
        .build_from_bytes([&model_bytes])?;
    
    // 2. 创建执行上下文
    let mut context = graph.init_execution_context()?;
    
    // 3. 处理输入图像
    println!("Processing image...");
    let img = image::open("test-image.jpg")?
        .resize_exact(224, 224, image::imageops::FilterType::Lanczos3)
        .to_rgb8();
    
    // 归一化到 [0, 1] 并排列为 [1, 3, 224, 224] NCHW 格式
    let tensor_data = preprocess_image(&img);
    
    // 4. 设置输入
    context.set_input(
        0,
        TensorType::F32,
        &[1, 3, 224, 224],
        &tensor_data,
    )?;
    
    // 5. 执行推理
    let start = std::time::Instant::now();
    context.compute()?;
    let inference_time = start.elapsed();
    
    // 6. 获取输出
    let output_size = 1000;  // ImageNet 1000 类
    let mut output_data = vec![0f32; output_size];
    
    context.get_output(
        0,
        &mut output_data.iter_mut()
            .flat_map(|v| v.to_le_bytes())
            .collect::<Vec<u8>>()
    )?;
    
    // 7. 解析结果
    let results = postprocess_classification(&output_data);
    
    println!("\n推理时间: {:?}", inference_time);
    println!("\nTop-5 分类结果:");
    for (i, (class_id, confidence)) in results.iter().enumerate().take(5) {
        println!("  {}. Class {} - 置信度: {:.2}%", 
            i + 1, class_id, confidence * 100.0);
    }
    
    Ok(())
}

fn preprocess_image(img: &image::RgbImage) -> Vec<u8> {
    // ImageNet 归一化参数
    let mean = [0.485f32, 0.456, 0.406];
    let std = [0.229f32, 0.224, 0.225];
    
    let width = img.width() as usize;
    let height = img.height() as usize;
    
    // NHWC -> NCHW 转换并归一化
    let mut tensor = vec![0f32; 1 * 3 * height * width];
    
    for y in 0..height {
        for x in 0..width {
            let pixel = img.get_pixel(x as u32, y as u32);
            for c in 0..3 {
                let value = pixel[c] as f32 / 255.0;
                let normalized = (value - mean[c]) / std[c];
                tensor[c * height * width + y * width + x] = normalized;
            }
        }
    }
    
    // f32 -> bytes
    tensor.iter()
        .flat_map(|v| v.to_le_bytes())
        .collect()
}

fn postprocess_classification(logits: &[f32]) -> Vec<(usize, f32)> {
    // Softmax
    let max_val = logits.iter().cloned().fold(f32::NEG_INFINITY, f32::max);
    let exp: Vec<f32> = logits.iter().map(|&x| (x - max_val).exp()).collect();
    let sum: f32 = exp.iter().sum();
    let probs: Vec<f32> = exp.iter().map(|&x| x / sum).collect();
    
    // 排序并返回 Top-K
    let mut indexed: Vec<(usize, f32)> = probs.iter()
        .enumerate()
        .map(|(i, &p)| (i, p))
        .collect();
    indexed.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap());
    indexed
}
```

## 3.3 运行配置

```bash
# 使用 WasmEdge 运行
wasmedge \
  --dir .:. \
  --env "WASMEDGE_PLUGIN_WASI_NN_PRELOAD=default:ONNX:CPU:resnet50.onnx" \
  classifier.wasm

# 或通过命令行指定模型
wasmedge \
  --dir .:. \
  classifier.wasm \
  --model resnet50.onnx \
  --input test-image.jpg

# 批量推理测试
for img in images/*.jpg; do
  wasmedge --dir .:. classifier.wasm "$img"
done
```

## 3.4 GGML 后端（LLM 推理）

```rust
// 使用 WasmEdge GGML 后端运行 LLM
use wasi_nn::{ExecutionTarget, GraphBuilder, GraphEncoding};

fn run_llm_inference(
    model_path: &str,
    prompt: &str,
    max_tokens: u32,
) -> Result<String, Box<dyn std::error::Error>> {
    // 构建 GGML 推理配置
    let config = serde_json::json!({
        "model_alias": "default",
        "n_gpu_layers": 0,  // 0=CPU only
        "n_ctx": 4096,
        "n_batch": 512,
        "n_threads": 4,
        "stream_stdout": false,
    });
    
    // 加载 GGUF 模型
    let model_bytes = std::fs::read(model_path)?;
    
    let graph = GraphBuilder::new(
        GraphEncoding::Ggml,
        ExecutionTarget::Cpu,
    ).config(config.to_string())
     .build_from_bytes([&model_bytes])?;
    
    let mut context = graph.init_execution_context()?;
    
    // 构建完整 prompt（支持聊天模板）
    let full_prompt = format!(
        "<|system|>\nYou are a helpful assistant.\n<|user|>\n{}\n<|assistant|>\n",
        prompt
    );
    
    // 设置输入（文本转字节）
    let prompt_bytes = full_prompt.as_bytes();
    context.set_input(0, wasi_nn::TensorType::U8, &[prompt_bytes.len() as u32], prompt_bytes)?;
    
    // 推理
    context.compute()?;
    
    // 获取生成文本
    let max_output_size = max_tokens as usize * 4;  // 估计字节数
    let mut output = vec![0u8; max_output_size];
    let written = context.get_output(0, &mut output)?;
    
    Ok(String::from_utf8_lossy(&output[..written]).to_string())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let response = run_llm_inference(
        "llama-3-8b-instruct.Q4_K_M.gguf",
        "解释 WebAssembly 的主要优势",
        512,
    )?;
    
    println!("LLM Response:\n{}", response);
    Ok(())
}
```

```bash
# 使用 WasmEdge GGML 插件运行
wasmedge \
  --dir .:. \
  --env "WASMEDGE_PLUGIN_WASI_NN_PRELOAD=default:GGML:CPU:llama-3-8b-instruct.Q4_K_M.gguf" \
  llm-inference.wasm \
  "解释 Kubernetes 的核心架构"
```

---

<!-- chunk: 4. ONNX Runtime Wasm -->## 4. ONNX Runtime Wasm

## 4.1 在浏览器中使用 ONNX Runtime Web

```javascript
// 浏览器端 ONNX 推理
import * as ort from 'onnxruntime-web';

// 配置 ONNX Runtime
ort.env.wasm.wasmPaths = '/dist/';  // WASM 文件路径
ort.env.wasm.numThreads = 4;        // Web Worker 线程数

class OnnxInferenceEngine {
    constructor() {
        this.sessions = new Map();
    }

    async loadModel(modelName, modelPath, options = {}) {
        const sessionOptions = {
            executionProviders: ['wasm'],  // 或 ['webgl', 'webgpu']
            graphOptimizationLevel: 'all',
            enableCpuMemArena: true,
            enableMemPattern: true,
            ...options,
        };

        console.time(`Load ${modelName}`);
        const session = await ort.InferenceSession.create(
            modelPath,
            sessionOptions,
        );
        console.timeEnd(`Load ${modelName}`);

        this.sessions.set(modelName, session);
        console.log(`Loaded model: ${modelName}`);
        console.log('Input names:', session.inputNames);
        console.log('Output names:', session.outputNames);

        return session;
    }

    async runInference(modelName, inputs) {
        const session = this.sessions.get(modelName);
        if (!session) {
            throw new Error(`Model not loaded: ${modelName}`);
        }

        const start = performance.now();
        const results = await session.run(inputs);
        const latency = performance.now() - start;

        console.log(`Inference time: ${latency.toFixed(2)}ms`);
        return results;
    }

    async classifyImage(imageElement) {
        // 预处理图像
        const canvas = document.createElement('canvas');
        canvas.width = 224;
        canvas.height = 224;
        const ctx = canvas.getContext('2d');
        ctx.drawImage(imageElement, 0, 0, 224, 224);

        const imageData = ctx.getImageData(0, 0, 224, 224);
        const inputTensor = this.preprocessImageNet(imageData);

        // 运行推理
        const results = await this.runInference('resnet50', {
            'input': inputTensor,
        });

        // 后处理
        const output = results['output'].data;
        return this.softmaxTopK(output, 5);
    }

    preprocessImageNet(imageData) {
        const { data, width, height } = imageData;
        const mean = [0.485, 0.456, 0.406];
        const std = [0.229, 0.224, 0.225];

        // RGBA -> RGB, HWC -> CHW, 归一化
        const floatData = new Float32Array(3 * height * width);

        for (let h = 0; h < height; h++) {
            for (let w = 0; w < width; w++) {
                const pixelIdx = (h * width + w) * 4;
                for (let c = 0; c < 3; c++) {
                    const value = data[pixelIdx + c] / 255.0;
                    const normalized = (value - mean[c]) / std[c];
                    floatData[c * height * width + h * width + w] = normalized;
                }
            }
        }

        return new ort.Tensor('float32', floatData, [1, 3, height, width]);
    }

    softmaxTopK(logits, k) {
        const maxVal = Math.max(...logits);
        const exp = Array.from(logits).map(x => Math.exp(x - maxVal));
        const sum = exp.reduce((a, b) => a + b, 0);
        const probs = exp.map(x => x / sum);

        return probs
            .map((prob, idx) => ({ classId: idx, probability: prob }))
            .sort((a, b) => b.probability - a.probability)
            .slice(0, k);
    }
}

// 使用示例
const engine = new OnnxInferenceEngine();

async function main() {
    // 加载模型
    await engine.loadModel('resnet50', '/models/resnet50.onnx');
    await engine.loadModel('bert', '/models/bert-base-uncased.onnx', {
        executionProviders: ['wasm'],
    });

    // 图像分类
    const img = document.getElementById('test-image');
    const classifications = await engine.classifyImage(img);
    console.log('Top-5 Classifications:', classifications);

    // 文本分类
    const textInput = prepareTextInput("This is a great product!", 128);
    const textResult = await engine.runInference('bert', textInput);
    console.log('Sentiment:', textResult);
}
```

## 4.2 Node.js ONNX 推理服务

```javascript
// onnx-inference-server.js
const express = require('express');
const ort = require('onnxruntime-node');
const sharp = require('sharp');
const multer = require('multer');

const app = express();
const upload = multer({ storage: multer.memoryStorage() });

class ModelServer {
    constructor() {
        this.models = {};
        this.stats = {
            totalInferences: 0,
            totalLatencyMs: 0,
            errors: 0,
        };
    }

    async initialize() {
        console.log('Loading models...');

        // 加载图像分类模型
        this.models.classifier = await ort.InferenceSession.create(
            './models/resnet50.onnx',
            {
                executionProviders: ['cpu'],
                graphOptimizationLevel: 'all',
                interOpNumThreads: 4,
                intraOpNumThreads: 4,
            }
        );

        // 加载目标检测模型
        this.models.detector = await ort.InferenceSession.create(
            './models/yolov8n.onnx',
            {
                executionProviders: ['cpu'],
                graphOptimizationLevel: 'all',
            }
        );

        console.log('Models loaded successfully');
    }

    async classifyImage(imageBuffer) {
        const start = Date.now();

        // 使用 sharp 预处理
        const processed = await sharp(imageBuffer)
            .resize(224, 224, { fit: 'fill' })
            .removeAlpha()
            .raw()
            .toBuffer();

        // 创建张量
        const tensorData = new Float32Array(3 * 224 * 224);
        const mean = [0.485, 0.456, 0.406];
        const std = [0.229, 0.224, 0.225];

        for (let i = 0; i < 224 * 224; i++) {
            for (let c = 0; c < 3; c++) {
                const val = processed[i * 3 + c] / 255.0;
                tensorData[c * 224 * 224 + i] = (val - mean[c]) / std[c];
            }
        }

        const tensor = new ort.Tensor('float32', tensorData, [1, 3, 224, 224]);

        // 推理
        const results = await this.models.classifier.run({ input: tensor });
        const latency = Date.now() - start;

        // 更新统计
        this.stats.totalInferences++;
        this.stats.totalLatencyMs += latency;

        // 后处理
        const output = Array.from(results.output.data);
        const topK = this.softmaxTopK(output, 5);

        return {
            predictions: topK,
            latency_ms: latency,
            model: 'resnet50',
        };
    }

    async detectObjects(imageBuffer) {
        const start = Date.now();

        const processed = await sharp(imageBuffer)
            .resize(640, 640, { fit: 'fill' })
            .removeAlpha()
            .raw()
            .toBuffer();

        const tensorData = new Float32Array(3 * 640 * 640);
        for (let i = 0; i < 640 * 640; i++) {
            for (let c = 0; c < 3; c++) {
                tensorData[c * 640 * 640 + i] = processed[i * 3 + c] / 255.0;
            }
        }

        const tensor = new ort.Tensor('float32', tensorData, [1, 3, 640, 640]);
        const results = await this.models.detector.run({ images: tensor });
        const latency = Date.now() - start;

        // YOLOv8 后处理
        const detections = this.processYoloOutput(
            results.output0.data,
            results.output0.dims,
            0.5  // confidence threshold
        );

        return {
            detections,
            latency_ms: latency,
            model: 'yolov8n',
        };
    }

    softmaxTopK(logits, k) {
        const maxVal = Math.max(...logits);
        const exp = logits.map(x => Math.exp(x - maxVal));
        const sum = exp.reduce((a, b) => a + b, 0);
        const probs = exp.map(x => x / sum);
        return probs
            .map((p, i) => ({ classId: i, confidence: p }))
            .sort((a, b) => b.confidence - a.confidence)
            .slice(0, k);
    }

    processYoloOutput(data, dims, threshold) {
        const [batch, features, numBoxes] = dims;
        const detections = [];

        for (let i = 0; i < numBoxes; i++) {
            const cx = data[0 * numBoxes + i];
            const cy = data[1 * numBoxes + i];
            const w = data[2 * numBoxes + i];
            const h = data[3 * numBoxes + i];

            // 找到最大类别置信度
            let maxConf = 0;
            let maxClass = 0;
            for (let c = 4; c < features; c++) {
                const conf = data[c * numBoxes + i];
                if (conf > maxConf) {
                    maxConf = conf;
                    maxClass = c - 4;
                }
            }

            if (maxConf > threshold) {
                detections.push({
                    bbox: { cx, cy, w, h },
                    classId: maxClass,
                    confidence: maxConf,
                });
            }
        }

        return detections;
    }

    getStats() {
        return {
            ...this.stats,
            avgLatencyMs: this.stats.totalInferences > 0
                ? this.stats.totalLatencyMs / this.stats.totalInferences
                : 0,
        };
    }
}

// API 路由
const server = new ModelServer();

app.post('/classify', upload.single('image'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ error: 'No image provided' });
        }

        const result = await server.classifyImage(req.file.buffer);
        res.json(result);
    } catch (error) {
        server.stats.errors++;
        console.error('Classification error:', error);
        res.status(500).json({ error: error.message });
    }
});

app.post('/detect', upload.single('image'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ error: 'No image provided' });
        }

        const result = await server.detectObjects(req.file.buffer);
        res.json(result);
    } catch (error) {
        server.stats.errors++;
        res.status(500).json({ error: error.message });
    }
});

app.get('/health', (req, res) => {
    res.json({
        status: 'healthy',
        stats: server.getStats(),
        models: Object.keys(server.models),
    });
});

server.initialize().then(() => {
    app.listen(8080, () => {
        console.log('ONNX Inference Server running on :8080');
    });
});
```

---

<!-- chunk: 5. llama.cpp Wasm 移植 -->## 5. llama.cpp Wasm 移植

## 5.1 llama.cpp.wasm 使用

```bash
# 安装 llama.cpp wasm 版本
npm install @llama-node/llama-cpp

# 或使用预构建的 wasm 版本
git clone https://github.com/ggerganov/llama.cpp
cd llama.cpp

# 编译 Wasm 版本
mkdir build-wasm && cd build-wasm
emcmake cmake .. \
  -DLLAMA_WASM=ON \
  -DLLAMA_WASM_SINGLE_FILE=ON

emmake make -j4
```

```javascript
// llama.cpp wasm 浏览器推理
import createModule from './llama.js';

class LlamaCppWasm {
    constructor() {
        this.module = null;
        this.modelLoaded = false;
    }

    async initialize() {
        this.module = await createModule({
            print: (text) => console.log(text),
            printErr: (text) => console.error(text),
        });
        console.log('llama.cpp WASM initialized');
    }

    async loadModel(modelArrayBuffer) {
        // 将模型写入 Emscripten 虚拟文件系统
        const modelArray = new Uint8Array(modelArrayBuffer);
        this.module.FS.writeFile('/model.gguf', modelArray);

        // 初始化模型
        const result = this.module.ccall(
            'llama_init',
            'number',
            ['string', 'number'],
            ['/model.gguf', 4096]  // 模型路径, 上下文长度
        );

        if (result !== 0) {
            throw new Error(`Failed to load model: ${result}`);
        }

        this.modelLoaded = true;
        console.log('Model loaded successfully');
    }

    async generate(prompt, options = {}) {
        if (!this.modelLoaded) {
            throw new Error('Model not loaded');
        }

        const {
            maxTokens = 256,
            temperature = 0.7,
            topP = 0.9,
            topK = 40,
            repeatPenalty = 1.1,
        } = options;

        return new Promise((resolve, reject) => {
            let output = '';

            // 设置回调函数
            this.module.onTokenGenerated = (token) => {
                output += token;
                options.onToken?.(token);
            };

            // 调用推理
            const result = this.module.ccall(
                'llama_generate',
                'number',
                ['string', 'number', 'number', 'number', 'number', 'number'],
                [prompt, maxTokens, temperature * 100, topP * 100, topK, repeatPenalty * 100]
            );

            if (result !== 0) {
                reject(new Error(`Generation failed: ${result}`));
            } else {
                resolve(output);
            }
        });
    }

    async streamGenerate(prompt, options = {}) {
        // 流式生成（返回 AsyncGenerator）
        const tokens = [];
        let done = false;
        let error = null;

        this.module.onTokenGenerated = (token) => {
            tokens.push(token);
            options.onToken?.(token);
        };

        this.module.onGenerationComplete = () => {
            done = true;
        };

        this.module.onGenerationError = (err) => {
            error = new Error(err);
            done = true;
        };

        // 异步启动推理
        setTimeout(() => {
            this.module.ccall(
                'llama_generate_stream',
                null,
                ['string', 'number', 'number'],
                [prompt, options.maxTokens || 512, options.temperature * 100 || 70]
            );
        }, 0);

        // 返回 AsyncGenerator
        return (async function* () {
            let idx = 0;
            while (!done || idx < tokens.length) {
                if (idx < tokens.length) {
                    yield tokens[idx++];
                } else {
                    await new Promise(resolve => setTimeout(resolve, 10));
                }
            }
            if (error) throw error;
        })();
    }
}

// 使用示例
async function main() {
    const llama = new LlamaCppWasm();
    await llama.initialize();

    // 加载量化模型（Q4_K_M 格式，7B 约 4GB）
    const response = await fetch('/models/llama-3-8b-instruct.Q4_K_M.gguf');
    const modelBuffer = await response.arrayBuffer();
    await llama.loadModel(modelBuffer);

    // 生成文本
    const prompt = `<|system|>
You are a helpful cloud native expert.
<|user|>
What is WebAssembly and why is it important for cloud native?
<|assistant|>`;

    console.log('Generating response...');

    // 流式输出
    const stream = await llama.streamGenerate(prompt, {
        maxTokens: 512,
        temperature: 0.7,
        onToken: (token) => process.stdout.write(token),
    });

    let fullResponse = '';
    for await (const token of stream) {
        fullResponse += token;
    }

    console.log('\n\nFull response:', fullResponse);
}
```

## 5.2 WasmEdge + llama.cpp 服务端部署

```bash
# 使用 WasmEdge 运行 llama.cpp API 服务器
wasmedge \
  --dir .:. \
  --env "WASMEDGE_PLUGIN_WASI_NN_PRELOAD=default:GGML:AUTO:llama-3-8b-instruct.Q4_K_M.gguf" \
  llama-api-server.wasm \
  --model-alias default \
  --model-name llama-3-8b-instruct \
  --prompt-template llama-3-chat \
  --socket-addr 0.0.0.0:8080 \
  --context-size 4096 \
  --batch-size 512

# 测试 API（OpenAI 兼容格式）
curl http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama-3-8b-instruct",
    "messages": [
      {"role": "system", "content": "You are a helpful assistant."},
      {"role": "user", "content": "Explain Wasm in 3 sentences."}
    ],
    "max_tokens": 512,
    "temperature": 0.7,
    "stream": false
  }'
```

---

<!-- chunk: 6. 模型优化与量化 -->## 6. 模型优化与量化

## 6.1 ONNX 模型优化流程

```python
# optimize_model.py - 模型优化工具

import onnx
import onnxruntime as ort
from onnxruntime.transformers import optimizer
from onnxruntime.quantization import (
    quantize_dynamic,
    quantize_static,
    QuantType,
    CalibrationDataReader,
    QuantizationMode,
)
import numpy as np
from pathlib import Path

def optimize_for_inference(model_path: str, output_path: str):
    """基础图优化"""
    model = onnx.load(model_path)
    
    # 简化模型
    from onnxsim import simplify
    simplified_model, check = simplify(model)
    assert check, "Model simplification failed"
    
    # 保存优化后的模型
    onnx.save(simplified_model, output_path)
    
    orig_size = Path(model_path).stat().st_size / 1024 / 1024
    opt_size = Path(output_path).stat().st_size / 1024 / 1024
    print(f"Original size: {orig_size:.1f}MB")
    print(f"Optimized size: {opt_size:.1f}MB")
    print(f"Reduction: {(1 - opt_size/orig_size) * 100:.1f}%")

def quantize_to_int8(model_path: str, output_path: str, calibration_data=None):
    """INT8 量化（最高压缩率）"""
    if calibration_data:
        # 静态量化（需要校准数据）
        class DataReader(CalibrationDataReader):
            def __init__(self, data):
                self.data = data
                self.idx = 0
            
            def get_next(self):
                if self.idx >= len(self.data):
                    return None
                item = self.data[self.idx]
                self.idx += 1
                return item
        
        quantize_static(
            model_path,
            output_path,
            DataReader(calibration_data),
            quant_format=QuantizationMode.QLinearOps,
            per_channel=True,
            reduce_range=True,
            weight_type=QuantType.QInt8,
            activation_type=QuantType.QInt8,
        )
    else:
        # 动态量化（无需校准数据）
        quantize_dynamic(
            model_path,
            output_path,
            weight_type=QuantType.QInt8,
            optimize_model=True,
            per_channel=True,
        )
    
    orig_size = Path(model_path).stat().st_size / 1024 / 1024
    quant_size = Path(output_path).stat().st_size / 1024 / 1024
    print(f"Quantized: {orig_size:.1f}MB -> {quant_size:.1f}MB ({(1-quant_size/orig_size)*100:.0f}% reduction)")

def quantize_to_float16(model_path: str, output_path: str):
    """FP16 量化（平衡精度与大小）"""
    from onnxconverter_common import float16
    
    model = onnx.load(model_path)
    model_fp16 = float16.convert_float_to_float16(model, keep_io_types=True)
    onnx.save(model_fp16, output_path)

def benchmark_model(model_path: str, input_shape: list, n_runs: int = 100):
    """基准测试"""
    sess_options = ort.SessionOptions()
    sess_options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    sess_options.intra_op_num_threads = 4
    
    session = ort.InferenceSession(
        model_path,
        sess_options=sess_options,
        providers=['CPUExecutionProvider'],
    )
    
    # 生成随机输入
    input_name = session.get_inputs()[0].name
    input_data = {input_name: np.random.randn(*input_shape).astype(np.float32)}
    
    # 预热
    for _ in range(10):
        session.run(None, input_data)
    
    # 正式测试
    import time
    latencies = []
    for _ in range(n_runs):
        start = time.perf_counter()
        session.run(None, input_data)
        latencies.append((time.perf_counter() - start) * 1000)
    
    print(f"\nBenchmark Results ({n_runs} runs):")
    print(f"  P50 latency: {np.percentile(latencies, 50):.2f}ms")
    print(f"  P90 latency: {np.percentile(latencies, 90):.2f}ms")
    print(f"  P99 latency: {np.percentile(latencies, 99):.2f}ms")
    print(f"  Throughput: {1000 / np.mean(latencies):.1f} inferences/sec")
    
    return latencies

# 完整优化流程
if __name__ == "__main__":
    model_name = "resnet50"
    
    # 步骤 1: 基础优化
    optimize_for_inference(
        f"{model_name}.onnx",
        f"{model_name}_optimized.onnx",
    )
    
    # 步骤 2: FP16 量化
    quantize_to_float16(
        f"{model_name}_optimized.onnx",
        f"{model_name}_fp16.onnx",
    )
    
    # 步骤 3: INT8 量化（动态）
    quantize_to_int8(
        f"{model_name}_optimized.onnx",
        f"{model_name}_int8.onnx",
    )
    
    # 基准测试比较
    print("\n=== Original Model ===")
    benchmark_model(f"{model_name}.onnx", [1, 3, 224, 224])
    
    print("\n=== Optimized Model ===")
    benchmark_model(f"{model_name}_optimized.onnx", [1, 3, 224, 224])
    
    print("\n=== FP16 Model ===")
    benchmark_model(f"{model_name}_fp16.onnx", [1, 3, 224, 224])
    
    print("\n=== INT8 Model ===")
    benchmark_model(f"{model_name}_int8.onnx", [1, 3, 224, 224])
```

## 6.2 GGUF 量化级别选择

```
GGUF 量化级别对比（以 LLaMA-3-8B 为例）：

量化级别   文件大小   内存占用   生成速度   质量损失
────────────────────────────────────────────────────
F32        30 GB     32 GB      1x        无
F16        15 GB     16 GB      1.2x      极小
Q8_0       8.5 GB    9 GB       1.5x      极小
Q6_K       6.1 GB    6.5 GB     1.8x      很小
Q5_K_M     5.1 GB    5.5 GB     2.0x      小
Q4_K_M     4.4 GB    4.8 GB     2.3x      可接受 ★推荐
Q3_K_M     3.5 GB    3.8 GB     2.8x      中等
Q2_K       2.9 GB    3.2 GB     3.0x      较大

推荐场景：
- 服务器部署（高精度）: Q6_K 或 Q8_0
- 标准边缘部署: Q4_K_M（最佳平衡）
- 内存受限环境: Q3_K_M
- 极限压缩: Q2_K（质量显著下降）
```

---

<!-- chunk: 7. 边缘 AI 推理部署 -->## 7. 边缘 AI 推理部署

## 7.1 KubeEdge + Wasm AI

```yaml
# kubeEdge-wasm-ai.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-ai-inference
  namespace: edge-ai
spec:
  replicas: 1
  selector:
    matchLabels:
      app: ai-inference
  template:
    metadata:
      labels:
        app: ai-inference
    spec:
      # 调度到边缘节点
      nodeSelector:
        node-role.kubernetes.io/edge: "true"
      
      # 使用 Wasm 运行时
      runtimeClassName: wasmedge
      
      containers:
        - name: inference-server
          # 超轻量 AI 推理容器
          image: ghcr.io/my-org/wasm-inference:1.0.0
          
          resources:
            requests:
              cpu: "200m"
              memory: "512Mi"
            limits:
              cpu: "2"
              memory: "2Gi"
          
          env:
            - name: MODEL_PATH
              value: "/models/resnet50_int8.onnx"
            - name: MAX_BATCH_SIZE
              value: "8"
            - name: NUM_THREADS
              value: "4"
          
          volumeMounts:
            - name: models
              mountPath: /models
              readOnly: true
            - name: input-data
              mountPath: /data/input
            - name: output-data
              mountPath: /data/output
          
          ports:
            - containerPort: 8080
              name: http
          
          livenessProbe:
            httpGet:
              path: /health
              port: 8080
            initialDelaySeconds: 10
            periodSeconds: 30
          
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 10
      
      volumes:
        - name: models
          configMap:
            name: ai-models
        - name: input-data
          hostPath:
            path: /opt/edge/ai/input
        - name: output-data
          hostPath:
            path: /opt/edge/ai/output
```

## 7.2 边缘 AI 推理代码

```rust
// edge-inference-server/src/main.rs
use std::sync::Arc;
use tokio::sync::RwLock;
use wasi_nn::{ExecutionTarget, GraphBuilder, GraphEncoding};

struct EdgeInferenceServer {
    models: Arc<RwLock<ModelRegistry>>,
    config: ServerConfig,
    stats: Arc<RwLock<InferenceStats>>,
}

struct ModelRegistry {
    models: std::collections::HashMap<String, LoadedModel>,
}

struct LoadedModel {
    graph: wasi_nn::Graph,
    model_name: String,
    input_shape: Vec<u32>,
    output_shape: Vec<u32>,
    loaded_at: std::time::Instant,
    inference_count: u64,
}

#[derive(serde::Deserialize)]
struct ServerConfig {
    model_dir: String,
    port: u16,
    max_batch_size: usize,
    num_threads: usize,
    preload_models: Vec<PreloadConfig>,
}

#[derive(serde::Deserialize)]
struct PreloadConfig {
    name: String,
    file: String,
    encoding: String,
    input_shape: Vec<u32>,
}

#[derive(Default, serde::Serialize)]
struct InferenceStats {
    total_requests: u64,
    total_latency_ms: f64,
    errors: u64,
    model_stats: std::collections::HashMap<String, ModelStats>,
}

#[derive(Default, serde::Serialize)]
struct ModelStats {
    requests: u64,
    total_latency_ms: f64,
    p50_latency_ms: f64,
    p99_latency_ms: f64,
}

impl EdgeInferenceServer {
    async fn new(config: ServerConfig) -> anyhow::Result<Self> {
        let mut registry = ModelRegistry {
            models: std::collections::HashMap::new(),
        };

        // 预加载模型
        for preload in &config.preload_models {
            println!("Preloading model: {}", preload.name);

            let model_path = format!("{}/{}", config.model_dir, preload.file);
            let model_bytes = tokio::fs::read(&model_path).await
                .map_err(|e| anyhow::anyhow!("Failed to read model {}: {}", model_path, e))?;

            let encoding = match preload.encoding.as_str() {
                "onnx" => GraphEncoding::Onnx,
                "ggml" => GraphEncoding::Ggml,
                "tensorflow" => GraphEncoding::Tensorflow,
                "openvino" => GraphEncoding::OpenVino,
                _ => return Err(anyhow::anyhow!("Unknown encoding: {}", preload.encoding)),
            };

            let graph = GraphBuilder::new(encoding, ExecutionTarget::Cpu)
                .build_from_bytes([&model_bytes])
                .map_err(|e| anyhow::anyhow!("Failed to load model {}: {:?}", preload.name, e))?;

            registry.models.insert(preload.name.clone(), LoadedModel {
                graph,
                model_name: preload.name.clone(),
                input_shape: preload.input_shape.clone(),
                output_shape: vec![],
                loaded_at: std::time::Instant::now(),
                inference_count: 0,
            });

            println!("Model {} loaded successfully", preload.name);
        }

        Ok(Self {
            models: Arc::new(RwLock::new(registry)),
            config,
            stats: Arc::new(RwLock::new(InferenceStats::default())),
        })
    }

    async fn run_inference(
        &self,
        model_name: &str,
        input_data: Vec<u8>,
        input_shape: Vec<u32>,
    ) -> anyhow::Result<(Vec<u8>, std::time::Duration)> {
        let registry = self.models.read().await;
        let model = registry.models.get(model_name)
            .ok_or_else(|| anyhow::anyhow!("Model not found: {}", model_name))?;

        let mut context = model.graph.init_execution_context()
            .map_err(|e| anyhow::anyhow!("Failed to create context: {:?}", e))?;

        // 设置输入
        context.set_input(0, wasi_nn::TensorType::F32, &input_shape, &input_data)
            .map_err(|e| anyhow::anyhow!("Failed to set input: {:?}", e))?;

        // 执行推理
        let start = std::time::Instant::now();
        context.compute()
            .map_err(|e| anyhow::anyhow!("Inference failed: {:?}", e))?;
        let latency = start.elapsed();

        // 获取输出
        let output_size = 1000 * 4;  // 1000 classes * 4 bytes (f32)
        let mut output = vec![0u8; output_size];
        context.get_output(0, &mut output)
            .map_err(|e| anyhow::anyhow!("Failed to get output: {:?}", e))?;

        Ok((output, latency))
    }
}
```

---

<!-- chunk: 8. Rust AI 推理开发 -->## 8. Rust AI 推理开发

## 8.1 使用 Candle 框架

```rust
// Cargo.toml
// [dependencies]
// candle-core = { version = "0.5", features = ["cuda"] }
// candle-nn = "0.5"
// candle-transformers = "0.5"
// tokenizers = "0.15"

use candle_core::{Device, Tensor, DType};
use candle_nn::{VarBuilder, Module};
use candle_transformers::models::bert::{BertModel, Config};
use tokenizers::Tokenizer;

struct BertInferenceEngine {
    model: BertModel,
    tokenizer: Tokenizer,
    device: Device,
}

impl BertInferenceEngine {
    fn new(model_path: &str, tokenizer_path: &str) -> anyhow::Result<Self> {
        let device = Device::Cpu;

        // 加载 tokenizer
        let tokenizer = Tokenizer::from_file(tokenizer_path)
            .map_err(|e| anyhow::anyhow!("Tokenizer load error: {}", e))?;

        // 加载模型配置
        let config_path = format!("{}/config.json", model_path);
        let config: Config = serde_json::from_str(
            &std::fs::read_to_string(&config_path)?
        )?;

        // 加载模型权重
        let weights_path = format!("{}/model.safetensors", model_path);
        let vb = unsafe {
            VarBuilder::from_mmaped_safetensors(&[weights_path], DType::F32, &device)?
        };

        let model = BertModel::load(vb, &config)?;

        Ok(Self { model, tokenizer, device })
    }

    fn encode(&self, texts: &[&str]) -> anyhow::Result<Vec<Vec<f32>>> {
        // Tokenize
        let encodings = self.tokenizer
            .encode_batch(texts.to_vec(), true)
            .map_err(|e| anyhow::anyhow!("Tokenize error: {}", e))?;

        let max_len = encodings.iter()
            .map(|e| e.len())
            .max()
            .unwrap_or(0);

        let batch_size = texts.len();

        // 构建输入张量
        let input_ids: Vec<u32> = encodings.iter()
            .flat_map(|e| {
                let mut ids = e.get_ids().to_vec();
                ids.resize(max_len, 0);  // padding
                ids
            })
            .collect();

        let attention_mask: Vec<u32> = encodings.iter()
            .flat_map(|e| {
                let len = e.len();
                (0..max_len).map(move |i| if i < len { 1 } else { 0 })
            })
            .collect();

        let input_ids_tensor = Tensor::from_vec(
            input_ids,
            (batch_size, max_len),
            &self.device,
        )?.to_dtype(DType::U32)?;

        let attention_mask_tensor = Tensor::from_vec(
            attention_mask,
            (batch_size, max_len),
            &self.device,
        )?.to_dtype(DType::U32)?;

        // 前向传播
        let embeddings = self.model.forward(
            &input_ids_tensor,
            &attention_mask_tensor,
            None,
        )?;

        // 平均池化获得句子向量
        let (_, seq_len, _) = embeddings.dims3()?;
        let mask_expanded = attention_mask_tensor
            .unsqueeze(2)?
            .broadcast_as(embeddings.shape())?
            .to_dtype(DType::F32)?;

        let sum_embeddings = (embeddings * mask_expanded)?.sum(1)?;
        let sum_mask = mask_expanded.sum(1)?;
        let mean_embeddings = (sum_embeddings / sum_mask)?;

        // 归一化
        let norm = mean_embeddings.sqr()?.sum_keepdim(1)?.sqrt()?;
        let normalized = mean_embeddings.broadcast_div(&norm)?;

        // 转换为 Vec<Vec<f32>>
        let result: Vec<Vec<f32>> = normalized.to_vec2()?;
        Ok(result)
    }

    fn similarity(&self, vec1: &[f32], vec2: &[f32]) -> f32 {
        // 余弦相似度
        let dot: f32 = vec1.iter().zip(vec2.iter()).map(|(a, b)| a * b).sum();
        let norm1: f32 = vec1.iter().map(|x| x * x).sum::<f32>().sqrt();
        let norm2: f32 = vec2.iter().map(|x| x * x).sum::<f32>().sqrt();

        if norm1 == 0.0 || norm2 == 0.0 {
            return 0.0;
        }

        dot / (norm1 * norm2)
    }
}

// 编译为 Wasm 的推理服务
#[no_mangle]
pub extern "C" fn compute_similarity(
    text1_ptr: *const u8, text1_len: usize,
    text2_ptr: *const u8, text2_len: usize,
) -> f32 {
    let text1 = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(text1_ptr, text1_len))
    };
    let text2 = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(text2_ptr, text2_len))
    };

    // 使用全局模型实例（实际需要初始化）
    let engine = get_global_engine();
    match engine.encode(&[text1, text2]) {
        Ok(embeddings) => engine.similarity(&embeddings[0], &embeddings[1]),
        Err(_) => -1.0,
    }
}
```

## 8.2 使用 Tract 框架（轻量推理）

```rust
// 使用 tract 进行轻量级推理（纯 Rust，适合 Wasm）
use tract_onnx::prelude::*;
use ndarray::Array;

fn load_and_run_onnx(
    model_path: &str,
    input: Array<f32, ndarray::IxDyn>,
) -> anyhow::Result<Vec<f32>> {
    // 加载 ONNX 模型
    let model = tract_onnx::onnx()
        .model_for_path(model_path)?
        .with_input_fact(
            0,
            f32::fact(input.shape()),
        )?
        .into_optimized()?
        .into_runnable()?;

    // 创建输入张量
    let input_tensor: Tensor = input.into();

    // 执行推理
    let result = model.run(tvec!(input_tensor.into()))?;

    // 提取结果
    let output = result[0]
        .to_array_view::<f32>()?
        .as_slice()
        .unwrap_or_default()
        .to_vec();

    Ok(output)
}

// 批量推理（Wasm 中高效使用内存）
fn batch_inference(
    model_path: &str,
    images: &[Vec<u8>],
    batch_size: usize,
) -> anyhow::Result<Vec<Vec<(usize, f32)>>> {
    let model = tract_onnx::onnx()
        .model_for_path(model_path)?
        .into_optimized()?
        .into_runnable()?;

    let mut all_results = Vec::with_capacity(images.len());

    // 分批处理
    for batch in images.chunks(batch_size) {
        let batch_len = batch.len();

        // 构建 batch 张量 [N, 3, 224, 224]
        let mut batch_data = vec![0f32; batch_len * 3 * 224 * 224];

        for (i, img_bytes) in batch.iter().enumerate() {
            let img = image::load_from_memory(img_bytes)?
                .resize_exact(224, 224, image::imageops::FilterType::Lanczos3)
                .to_rgb8();

            // 归一化并写入 batch
            let mean = [0.485f32, 0.456, 0.406];
            let std = [0.229f32, 0.224, 0.225];

            for (y, row) in img.rows().enumerate() {
                for (x, pixel) in row.enumerate() {
                    for c in 0..3 {
                        let val = pixel[c] as f32 / 255.0;
                        let idx = i * 3 * 224 * 224 + c * 224 * 224 + y * 224 + x;
                        batch_data[idx] = (val - mean[c]) / std[c];
                    }
                }
            }
        }

        let input = Array::from_shape_vec(
            ndarray::IxDyn(&[batch_len, 3, 224, 224]),
            batch_data,
        )?;

        let input_tensor: Tensor = input.into();
        let result = model.run(tvec!(input_tensor.into()))?;

        let output = result[0].to_array_view::<f32>()?;

        // 处理每个样本的结果
        for i in 0..batch_len {
            let sample_output: Vec<f32> = output.slice(ndarray::s![i, ..]).to_vec();
            let top5 = softmax_top_k(&sample_output, 5);
            all_results.push(top5);
        }
    }

    Ok(all_results)
}

fn softmax_top_k(logits: &[f32], k: usize) -> Vec<(usize, f32)> {
    let max_val = logits.iter().cloned().fold(f32::NEG_INFINITY, f32::max);
    let exp: Vec<f32> = logits.iter().map(|&x| (x - max_val).exp()).collect();
    let sum: f32 = exp.iter().sum();
    let probs: Vec<f32> = exp.iter().map(|&x| x / sum).collect();

    let mut indexed: Vec<(usize, f32)> = probs.iter()
        .enumerate()
        .map(|(i, &p)| (i, p))
        .collect();
    indexed.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap());
    indexed.truncate(k);
    indexed
}
```

---

<!-- chunk: 9. Python/JS AI 推理集成 -->## 9. Python/JS AI 推理集成

## 9.1 Python 调用 WasmEdge 推理

```python
# python_wasm_inference.py
import subprocess
import json
import tempfile
import os
import numpy as np
from PIL import Image
import io

class WasmEdgeInference:
    def __init__(self, wasm_binary: str, model_path: str):
        self.wasm_binary = wasm_binary
        self.model_path = model_path
    
    def classify_image(self, image_path: str) -> dict:
        """调用 WasmEdge 进行图像分类"""
        result = subprocess.run(
            [
                "wasmedge",
                "--dir", f".:{os.path.dirname(self.model_path)}",
                "--env", f"WASMEDGE_PLUGIN_WASI_NN_PRELOAD=default:ONNX:CPU:{self.model_path}",
                self.wasm_binary,
                "--image", image_path,
            ],
            capture_output=True,
            text=True,
            timeout=30,
        )
        
        if result.returncode != 0:
            raise RuntimeError(f"WasmEdge error: {result.stderr}")
        
        return json.loads(result.stdout)
    
    def batch_classify(self, image_paths: list) -> list:
        """批量分类"""
        results = []
        for path in image_paths:
            try:
                result = self.classify_image(path)
                results.append(result)
            except Exception as e:
                results.append({"error": str(e), "path": path})
        return results
    
    def preprocess_for_wasm(self, image: Image.Image) -> bytes:
        """将 PIL Image 预处理为 Wasm 可接受的格式"""
        # 调整大小
        img_resized = image.resize((224, 224), Image.LANCZOS)
        img_rgb = img_resized.convert('RGB')
        
        # 保存为临时文件
        with tempfile.NamedTemporaryFile(suffix='.jpg', delete=False) as f:
            img_rgb.save(f.name, 'JPEG', quality=95)
            return f.name


# 使用 wasmedge-sdk Python 绑定
try:
    from wasmedge_sdk import WasmEdge, Config, Module
    
    class WasmEdgePythonSDK:
        def __init__(self, wasm_path: str, model_path: str):
            config = Config()
            config.wasm_component_model = False
            
            self.vm = WasmEdge(config=config)
            
            # 注册 WASI-NN 插件
            self.vm.plugin_manager.load_with_default_config("wasi_nn")
            
            # 加载模块
            with open(wasm_path, 'rb') as f:
                module_bytes = f.read()
            
            self.module = Module.from_bytes(module_bytes)
            self.instance = self.vm.run_module(self.module)
        
        def classify(self, image_array: np.ndarray) -> dict:
            # 分配 Wasm 内存
            ptr = self.instance.call("malloc", [len(image_array.tobytes())])
            
            # 写入数据
            mem = self.instance.memory("memory")
            mem.write(ptr, image_array.astype(np.float32).tobytes())
            
            # 调用推理函数
            result_ptr, result_len = self.instance.call(
                "classify",
                [ptr, *image_array.shape]
            )
            
            # 读取结果
            result_bytes = mem.read(result_ptr, result_len)
            result = json.loads(result_bytes.decode('utf-8'))
            
            # 释放内存
            self.instance.call("free", [ptr])
            self.instance.call("free", [result_ptr])
            
            return result

except ImportError:
    pass  # wasmedge-sdk 不可用时降级

# 高级 API 包装
class AIInferenceClient:
    def __init__(self, backend: str = "wasmedge", **kwargs):
        if backend == "wasmedge":
            self.engine = WasmEdgeInference(**kwargs)
        elif backend == "onnxruntime":
            import onnxruntime as ort
            self.session = ort.InferenceSession(kwargs.get("model_path"))
        
        self.backend = backend
    
    def infer(self, input_data, **kwargs):
        if self.backend == "wasmedge":
            if isinstance(input_data, str):
                return self.engine.classify_image(input_data)
            elif isinstance(input_data, Image.Image):
                tmp_path = self.engine.preprocess_for_wasm(input_data)
                return self.engine.classify_image(tmp_path)
        elif self.backend == "onnxruntime":
            # 直接使用 OnnxRuntime
            input_name = self.session.get_inputs()[0].name
            outputs = self.session.run(None, {input_name: input_data})
            return {"output": outputs[0].tolist()}
```

---

<!-- chunk: 10. 多模型服务架构 -->## 10. 多模型服务架构

## 10.1 模型服务调度器

```mermaid
graph TB
    subgraph "Model Serving Architecture"
        LB[负载均衡器]
        
        subgraph "模型实例池"
            subgraph "ResNet-50 Pool"
                R1[ResNet Worker 1<br/>Wasm]
                R2[ResNet Worker 2<br/>Wasm]
                R3[ResNet Worker 3<br/>Wasm]
            end
            
            subgraph "BERT Pool"
                B1[BERT Worker 1<br/>Wasm]
                B2[BERT Worker 2<br/>Wasm]
            end
            
            subgraph "LLaMA Pool"
                L1[LLaMA Worker<br/>Wasm+GPU]
            end
        end
        
        ModelRegistry[模型注册中心]
        ScaleManager[弹性伸缩]
        Metrics[指标收集]
    end
    
    Client --> LB
    LB --> R1
    LB --> B1
    LB --> L1
    ScaleManager --> |根据负载调整| ModelRegistry
```

```rust
// 多模型服务调度器
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::{RwLock, Semaphore};

struct ModelServingSystem {
    models: Arc<RwLock<HashMap<String, ModelPool>>>,
    global_limiter: Arc<Semaphore>,
}

struct ModelPool {
    workers: Vec<Arc<ModelWorker>>,
    limiter: Arc<Semaphore>,
    queue_depth: u32,
}

struct ModelWorker {
    id: String,
    model_name: String,
    context: Arc<tokio::sync::Mutex<wasi_nn::GraphExecutionContext>>,
    stats: Arc<WorkerStats>,
}

#[derive(Default)]
struct WorkerStats {
    requests: std::sync::atomic::AtomicU64,
    total_latency_ns: std::sync::atomic::AtomicU64,
    errors: std::sync::atomic::AtomicU64,
}

impl ModelServingSystem {
    async fn infer(
        &self,
        model_name: &str,
        input: Vec<u8>,
        input_shape: Vec<u32>,
    ) -> anyhow::Result<Vec<u8>> {
        // 获取全局并发限制
        let _global_permit = self.global_limiter.acquire().await?;

        let models = self.models.read().await;
        let pool = models.get(model_name)
            .ok_or_else(|| anyhow::anyhow!("Model not found: {}", model_name))?;

        // 获取模型级并发限制
        let _pool_permit = pool.limiter.acquire().await?;

        // 轮询选择最空闲的 worker
        let worker = self.select_worker(&pool.workers);

        // 执行推理
        let start = std::time::Instant::now();
        let result = {
            let mut ctx = worker.context.lock().await;
            ctx.set_input(0, wasi_nn::TensorType::F32, &input_shape, &input)?;
            ctx.compute()?;

            let mut output = vec![0u8; 4000];  // 1000 * 4 bytes
            ctx.get_output(0, &mut output)?;
            output
        };
        let latency = start.elapsed();

        // 更新统计
        worker.stats.requests.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
        worker.stats.total_latency_ns.fetch_add(
            latency.as_nanos() as u64,
            std::sync::atomic::Ordering::Relaxed,
        );

        Ok(result)
    }

    fn select_worker(&self, workers: &[Arc<ModelWorker>]) -> Arc<ModelWorker> {
        // 选择请求数最少的 worker（最小连接算法）
        workers.iter()
            .min_by_key(|w| w.stats.requests.load(std::sync::atomic::Ordering::Relaxed))
            .cloned()
            .unwrap_or_else(|| workers[0].clone())
    }
}
```

---

<!-- chunk: 11. 性能基准与对比 -->## 11. 性能基准与对比

## 11.1 详细性能基准测试

```python
# benchmark_wasm_inference.py
import time
import statistics
import subprocess
import json
import numpy as np
import requests
from concurrent.futures import ThreadPoolExecutor

class InferenceBenchmark:
    def __init__(self, endpoint: str):
        self.endpoint = endpoint
        self.results = {}
    
    def run_single_benchmark(
        self,
        model_name: str,
        input_generator,
        n_warmup: int = 20,
        n_runs: int = 200,
    ) -> dict:
        # 预热
        for _ in range(n_warmup):
            input_data = input_generator()
            requests.post(f"{self.endpoint}/infer/{model_name}", 
                         data=input_data, timeout=10)
        
        # 正式测试
        latencies = []
        for _ in range(n_runs):
            input_data = input_generator()
            start = time.perf_counter()
            resp = requests.post(
                f"{self.endpoint}/infer/{model_name}",
                data=input_data,
                timeout=10,
            )
            latency = (time.perf_counter() - start) * 1000
            if resp.ok:
                latencies.append(latency)
        
        return {
            "model": model_name,
            "n_runs": len(latencies),
            "p50_ms": statistics.median(latencies),
            "p90_ms": np.percentile(latencies, 90),
            "p99_ms": np.percentile(latencies, 99),
            "min_ms": min(latencies),
            "max_ms": max(latencies),
            "throughput_rps": 1000 / statistics.mean(latencies),
        }
    
    def run_concurrent_benchmark(
        self,
        model_name: str,
        input_generator,
        concurrency: int,
        n_requests: int,
    ) -> dict:
        latencies = []
        errors = 0
        
        def make_request():
            nonlocal errors
            try:
                input_data = input_generator()
                start = time.perf_counter()
                resp = requests.post(
                    f"{self.endpoint}/infer/{model_name}",
                    data=input_data,
                    timeout=10,
                )
                latency = (time.perf_counter() - start) * 1000
                if resp.ok:
                    return latency
                else:
                    errors += 1
                    return None
            except Exception:
                errors += 1
                return None
        
        wall_start = time.perf_counter()
        with ThreadPoolExecutor(max_workers=concurrency) as executor:
            futures = [executor.submit(make_request) for _ in range(n_requests)]
            results = [f.result() for f in futures]
        wall_time = time.perf_counter() - wall_start
        
        latencies = [r for r in results if r is not None]
        
        return {
            "model": model_name,
            "concurrency": concurrency,
            "total_requests": n_requests,
            "successful": len(latencies),
            "errors": errors,
            "p50_ms": statistics.median(latencies) if latencies else 0,
            "p99_ms": np.percentile(latencies, 99) if latencies else 0,
            "throughput_rps": n_requests / wall_time,
        }
    
    def print_report(self, results: list):
        print("\n" + "="*70)
        print("Wasm AI Inference Benchmark Report")
        print("="*70)
        
        for r in results:
            print(f"\nModel: {r['model']}")
            print(f"  P50 Latency:  {r['p50_ms']:.2f}ms")
            print(f"  P90 Latency:  {r['p90_ms']:.2f}ms")
            print(f"  P99 Latency:  {r['p99_ms']:.2f}ms")
            print(f"  Throughput:   {r['throughput_rps']:.1f} req/s")


# 运行基准测试
if __name__ == "__main__":
    bench = InferenceBenchmark("http://localhost:8080")
    
    results = []
    
    # ResNet-50 分类
    results.append(bench.run_single_benchmark(
        "resnet50",
        lambda: open("test-image.jpg", "rb").read(),
        n_warmup=20,
        n_runs=200,
    ))
    
    # BERT 文本分类
    results.append(bench.run_single_benchmark(
        "bert-classification",
        lambda: json.dumps({"text": "This is a sample input for BERT classification."}).encode(),
        n_warmup=10,
        n_runs=100,
    ))
    
    bench.print_report(results)
    
    # 并发测试
    print("\n" + "="*70)
    print("Concurrent Throughput Test")
    print("="*70)
    
    for concurrency in [1, 4, 8, 16]:
        result = bench.run_concurrent_benchmark(
            "resnet50",
            lambda: open("test-image.jpg", "rb").read(),
            concurrency=concurrency,
            n_requests=200,
        )
        print(f"Concurrency {concurrency:2d}: {result['throughput_rps']:.1f} req/s, "
              f"P99={result['p99_ms']:.1f}ms, errors={result['errors']}")
```

## 11.2 Wasm vs 原生对比

```
AI 推理性能对比（Apple M2 MacBook Pro）：

ResNet-50 图像分类（224x224 输入，batch=1）：
──────────────────────────────────────────────────────────────
方式                     P50    P99    吞吐量    内存    大小
──────────────────────────────────────────────────────────────
Python (PyTorch)         15ms   25ms   65/s     850MB   -
Python (ONNX Runtime)    8ms    14ms   120/s    450MB   -
Rust (Tract)             6ms    10ms   160/s    180MB   原生
Rust (Tract) → Wasm      7ms    12ms   140/s    60MB    9MB
WasmEdge WASI-NN (ONNX)  9ms    15ms   110/s    120MB   15MB
Node.js (ONNX RT Web)    12ms   20ms   80/s     250MB   -
Browser (ONNX RT Web)    18ms   35ms   55/s     -       wasm

BERT-base 文本编码（128 tokens）：
──────────────────────────────────────────────────────────────
Python (PyTorch)         45ms   80ms   22/s     1.2GB   -
Python (ONNX Runtime)    25ms   40ms   40/s     500MB   -
Rust (Candle)            20ms   35ms   50/s     350MB   原生
WasmEdge WASI-NN         30ms   55ms   33/s     200MB   25MB

结论：Wasm 推理性能约为原生的 85-90%，但具有
- 🔒 完整沙箱隔离
- 📦 极小的部署包（10-25MB vs 几百MB）
- 🌐 跨平台无需重新编译
- ⚡ 毫秒级冷启动
```

---

<!-- chunk: 12. Kubernetes AI 推理集成 -->## 12. Kubernetes AI 推理集成

## 12.1 Kubernetes 推理服务部署

```yaml
# k8s-ai-inference.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-ai-inference-service
  namespace: ai-inference
  labels:
    app: ai-inference
    version: v1
spec:
  replicas: 3
  selector:
    matchLabels:
      app: ai-inference
  template:
    metadata:
      labels:
        app: ai-inference
        version: v1
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
        prometheus.io/path: "/metrics"
    spec:
      # 使用 WasmEdge 运行时
      runtimeClassName: wasmedge
      
      initContainers:
        # 下载模型文件
        - name: model-downloader
          image: curlimages/curl:8.0.0
          command:
            - sh
            - -c
            - |
              if [ ! -f /models/resnet50_int8.onnx ]; then
                echo "Downloading models..."
                curl -sL https://models.example.com/resnet50_int8.onnx \
                     -o /models/resnet50_int8.onnx
                curl -sL https://models.example.com/bert_int8.onnx \
                     -o /models/bert_int8.onnx
                echo "Models downloaded"
              fi
          volumeMounts:
            - name: models
              mountPath: /models
      
      containers:
        - name: inference-server
          image: ghcr.io/my-org/wasm-inference-server:1.0.0
          command: ["wasmedge"]
          args:
            - "--dir=.:/"
            - "--env=WASMEDGE_PLUGIN_WASI_NN_PRELOAD=resnet50:ONNX:CPU:/models/resnet50_int8.onnx,bert:ONNX:CPU:/models/bert_int8.onnx"
            - "/app/inference-server.wasm"
            - "--port=8080"
            - "--workers=4"
          
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "4"
              memory: "4Gi"
          
          env:
            - name: MAX_BATCH_SIZE
              value: "8"
            - name: REQUEST_TIMEOUT_MS
              value: "5000"
            - name: LOG_LEVEL
              value: "info"
          
          ports:
            - containerPort: 8080
              name: http
          
          volumeMounts:
            - name: models
              mountPath: /models
              readOnly: true
          
          livenessProbe:
            httpGet:
              path: /health
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 10
            failureThreshold: 3
          
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 10
            periodSeconds: 5
      
      volumes:
        - name: models
          persistentVolumeClaim:
            claimName: ai-models-pvc

---
apiVersion: v1
kind: Service
metadata:
  name: ai-inference-service
  namespace: ai-inference
spec:
  selector:
    app: ai-inference
  ports:
    - name: http
      port: 80
      targetPort: 8080
  type: ClusterIP

---
# HPA 自动伸缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: ai-inference-hpa
  namespace: ai-inference
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: wasm-ai-inference-service
  minReplicas: 2
  maxReplicas: 20
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
    - type: Pods
      pods:
        metric:
          name: inference_queue_depth
        target:
          type: AverageValue
          averageValue: "10"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Pods
          value: 4
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Pods
          value: 1
          periodSeconds: 120
```

---

<!-- chunk: 13. 实战案例：图像分类服务 -->## 13. 实战案例：图像分类服务

## 13.1 完整图像分类服务

```rust
// complete-image-classifier/src/main.rs
use axum::{
    extract::{Multipart, State},
    http::StatusCode,
    response::Json,
    routing::{get, post},
    Router,
};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use tokio::sync::Mutex;

#[derive(Serialize)]
struct ClassificationResult {
    predictions: Vec<Prediction>,
    inference_time_ms: f64,
    model_name: String,
}

#[derive(Serialize)]
struct Prediction {
    class_id: usize,
    class_name: String,
    confidence: f32,
}

#[derive(Clone)]
struct AppState {
    classifier: Arc<Mutex<ImageClassifier>>,
    imagenet_labels: Arc<Vec<String>>,
}

struct ImageClassifier {
    graph: wasi_nn::Graph,
}

impl ImageClassifier {
    fn new(model_path: &str) -> anyhow::Result<Self> {
        let model_bytes = std::fs::read(model_path)?;
        let graph = wasi_nn::GraphBuilder::new(
            wasi_nn::GraphEncoding::Onnx,
            wasi_nn::ExecutionTarget::Cpu,
        ).build_from_bytes([&model_bytes])?;

        Ok(Self { graph })
    }

    fn classify(&self, image_bytes: &[u8]) -> anyhow::Result<Vec<(usize, f32)>> {
        let img = image::load_from_memory(image_bytes)?
            .resize_exact(224, 224, image::imageops::FilterType::Lanczos3)
            .to_rgb8();

        let tensor_data = preprocess_image(&img);

        let mut context = self.graph.init_execution_context()?;
        context.set_input(0, wasi_nn::TensorType::F32, &[1, 3, 224, 224], &tensor_data)?;
        context.compute()?;

        let mut output = vec![0u8; 1000 * 4];
        context.get_output(0, &mut output)?;

        let logits: Vec<f32> = output.chunks(4)
            .map(|b| f32::from_le_bytes([b[0], b[1], b[2], b[3]]))
            .collect();

        Ok(softmax_top_k(&logits, 5))
    }
}

async fn classify_handler(
    State(state): State<AppState>,
    mut multipart: Multipart,
) -> Result<Json<ClassificationResult>, StatusCode> {
    let mut image_bytes = None;

    while let Some(field) = multipart.next_field().await
        .map_err(|_| StatusCode::BAD_REQUEST)?
    {
        if field.name() == Some("image") {
            image_bytes = Some(
                field.bytes().await
                    .map_err(|_| StatusCode::BAD_REQUEST)?
            );
        }
    }

    let bytes = image_bytes.ok_or(StatusCode::BAD_REQUEST)?;

    let start = std::time::Instant::now();
    let classifier = state.classifier.lock().await;
    let predictions = classifier.classify(&bytes)
        .map_err(|e| {
            eprintln!("Classification error: {}", e);
            StatusCode::INTERNAL_SERVER_ERROR
        })?;
    let inference_time = start.elapsed();

    let labels = &state.imagenet_labels;
    let result = ClassificationResult {
        predictions: predictions.iter().map(|(class_id, confidence)| {
            Prediction {
                class_id: *class_id,
                class_name: labels.get(*class_id)
                    .cloned()
                    .unwrap_or_else(|| format!("class_{}", class_id)),
                confidence: *confidence,
            }
        }).collect(),
        inference_time_ms: inference_time.as_secs_f64() * 1000.0,
        model_name: "resnet50-int8".to_string(),
    };

    Ok(Json(result))
}

async fn health_handler() -> Json<serde_json::Value> {
    Json(serde_json::json!({"status": "healthy", "version": "1.0.0"}))
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let classifier = ImageClassifier::new("resnet50_int8.onnx")?;
    let labels: Vec<String> = serde_json::from_str(
        &std::fs::read_to_string("imagenet_labels.json")?
    )?;

    let state = AppState {
        classifier: Arc::new(Mutex::new(classifier)),
        imagenet_labels: Arc::new(labels),
    };

    let app = Router::new()
        .route("/classify", post(classify_handler))
        .route("/health", get(health_handler))
        .with_state(state);

    println!("Image Classification Service running on :8080");
    axum::Server::bind(&"0.0.0.0:8080".parse()?)
        .serve(app.into_make_service())
        .await?;

    Ok(())
}

fn preprocess_image(img: &image::RgbImage) -> Vec<u8> {
    let mean = [0.485f32, 0.456, 0.406];
    let std = [0.229f32, 0.224, 0.225];
    let (w, h) = (img.width() as usize, img.height() as usize);
    let mut tensor = vec![0f32; 3 * h * w];

    for y in 0..h {
        for x in 0..w {
            let p = img.get_pixel(x as u32, y as u32);
            for c in 0..3 {
                let v = p[c] as f32 / 255.0;
                tensor[c * h * w + y * w + x] = (v - mean[c]) / std[c];
            }
        }
    }
    tensor.iter().flat_map(|v| v.to_le_bytes()).collect()
}

fn softmax_top_k(logits: &[f32], k: usize) -> Vec<(usize, f32)> {
    let max = logits.iter().cloned().fold(f32::NEG_INFINITY, f32::max);
    let exp: Vec<f32> = logits.iter().map(|&x| (x - max).exp()).collect();
    let sum: f32 = exp.iter().sum();
    let probs: Vec<f32> = exp.iter().map(|&x| x / sum).collect();
    let mut indexed: Vec<(usize, f32)> = probs.iter().enumerate().map(|(i, &p)| (i, p)).collect();
    indexed.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap());
    indexed.truncate(k);
    indexed
}
```

---

<!-- chunk: 14. 实战案例：LLM 推理服务 -->## 14. 实战案例：LLM 推理服务

## 14.1 OpenAI 兼容 LLM API

```rust
// llm-service/src/main.rs - OpenAI 兼容 API
use axum::{
    extract::State,
    response::{Json, Sse, sse::Event},
    routing::post,
    Router,
};
use serde::{Deserialize, Serialize};
use futures::stream::{self, Stream};
use std::sync::Arc;

#[derive(Deserialize)]
struct ChatRequest {
    model: String,
    messages: Vec<ChatMessage>,
    max_tokens: Option<u32>,
    temperature: Option<f32>,
    stream: Option<bool>,
}

#[derive(Deserialize, Serialize)]
struct ChatMessage {
    role: String,
    content: String,
}

#[derive(Serialize)]
struct ChatResponse {
    id: String,
    object: String,
    created: u64,
    model: String,
    choices: Vec<ChatChoice>,
    usage: Option<UsageInfo>,
}

#[derive(Serialize)]
struct ChatChoice {
    index: u32,
    message: Option<ChatMessage>,
    delta: Option<ChatMessageDelta>,
    finish_reason: Option<String>,
}

#[derive(Serialize)]
struct ChatMessageDelta {
    role: Option<String>,
    content: Option<String>,
}

#[derive(Serialize)]
struct UsageInfo {
    prompt_tokens: u32,
    completion_tokens: u32,
    total_tokens: u32,
}

struct LlmService {
    graph: wasi_nn::Graph,
    model_name: String,
    context_size: usize,
}

impl LlmService {
    fn format_prompt(&self, messages: &[ChatMessage]) -> String {
        // LLaMA-3 聊天模板
        let mut prompt = String::from("<|begin_of_text|>");

        for msg in messages {
            match msg.role.as_str() {
                "system" => prompt.push_str(&format!(
                    "<|start_header_id|>system<|end_header_id|>\n\n{}<|eot_id|>",
                    msg.content
                )),
                "user" => prompt.push_str(&format!(
                    "<|start_header_id|>user<|end_header_id|>\n\n{}<|eot_id|>",
                    msg.content
                )),
                "assistant" => prompt.push_str(&format!(
                    "<|start_header_id|>assistant<|end_header_id|>\n\n{}<|eot_id|>",
                    msg.content
                )),
                _ => {}
            }
        }

        // 开始助手回复
        prompt.push_str("<|start_header_id|>assistant<|end_header_id|>\n\n");
        prompt
    }

    fn generate(&self, prompt: &str, max_tokens: u32, temperature: f32) -> anyhow::Result<String> {
        let mut context = self.graph.init_execution_context()?;

        let config = serde_json::json!({
            "max_new_tokens": max_tokens,
            "temperature": temperature,
            "top_p": 0.9,
            "repeat_penalty": 1.1,
        });

        let prompt_with_config = serde_json::json!({
            "prompt": prompt,
            "config": config,
        }).to_string();

        context.set_input(
            0,
            wasi_nn::TensorType::U8,
            &[prompt_with_config.len() as u32],
            prompt_with_config.as_bytes(),
        )?;

        context.compute()?;

        let mut output = vec![0u8; max_tokens as usize * 8];
        let written = context.get_output(0, &mut output)?;

        Ok(String::from_utf8_lossy(&output[..written]).to_string())
    }
}

async fn chat_completions(
    State(service): State<Arc<LlmService>>,
    Json(req): Json<ChatRequest>,
) -> Result<Json<ChatResponse>, axum::http::StatusCode> {
    let prompt = service.format_prompt(&req.messages);
    let max_tokens = req.max_tokens.unwrap_or(512);
    let temperature = req.temperature.unwrap_or(0.7);

    let response_text = service.generate(&prompt, max_tokens, temperature)
        .map_err(|_| axum::http::StatusCode::INTERNAL_SERVER_ERROR)?;

    let response = ChatResponse {
        id: format!("chatcmpl-{}", uuid_v4()),
        object: "chat.completion".to_string(),
        created: unix_timestamp(),
        model: req.model.clone(),
        choices: vec![ChatChoice {
            index: 0,
            message: Some(ChatMessage {
                role: "assistant".to_string(),
                content: response_text,
            }),
            delta: None,
            finish_reason: Some("stop".to_string()),
        }],
        usage: Some(UsageInfo {
            prompt_tokens: prompt.len() as u32 / 4,
            completion_tokens: max_tokens,
            total_tokens: (prompt.len() as u32 / 4) + max_tokens,
        }),
    };

    Ok(Json(response))
}

fn uuid_v4() -> String {
    format!("{:032x}", std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_nanos())
}

fn unix_timestamp() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs()
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let model_bytes = std::fs::read("llama-3-8b-instruct.Q4_K_M.gguf")?;
    let graph = wasi_nn::GraphBuilder::new(
        wasi_nn::GraphEncoding::Ggml,
        wasi_nn::ExecutionTarget::Cpu,
    ).config(serde_json::json!({
        "n_ctx": 4096,
        "n_threads": 8,
        "n_gpu_layers": 0,
    }).to_string())
     .build_from_bytes([&model_bytes])?;

    let service = Arc::new(LlmService {
        graph,
        model_name: "llama-3-8b-instruct".to_string(),
        context_size: 4096,
    });

    let app = Router::new()
        .route("/v1/chat/completions", post(chat_completions))
        .with_state(service);

    println!("LLM Service (OpenAI compatible) running on :8080");
    axum::Server::bind(&"0.0.0.0:8080".parse()?)
        .serve(app.into_make_service())
        .await?;

    Ok(())
}
```

---

<!-- chunk: 总结 -->## 总结

Wasm AI 推理通过 **WASI-NN** 标准接口实现了跨平台、安全的 AI 模型部署：

**技术选型建议**：

| 场景 | 推荐方案 | 理由 |
|------|----------|------|
| 边缘 CV 推理 | WasmEdge + ONNX INT8 | 低内存，CPU 高效 |
| 浏览器 AI | ONNX Runtime Web | 成熟，WebGL 加速 |
| 边缘 LLM | WasmEdge + GGML Q4_K_M | 平衡质量与速度 |
| 高性能推理 | Rust Candle/Tract | 最优性能 |
| 嵌入式 IoT | WAMR + TFLite | 最低资源占用 |

**最佳实践**：
1. 使用 INT8/FP16 量化，大幅减少模型大小和推理时间
2. 通过 WASI-NN 标准接口解耦模型格式与运行时
3. 预加载模型至共享内存，避免重复加载
4. 对 LLM 使用 Q4_K_M 量化，平衡质量与资源
5. 利用 Kubernetes HPA 根据推理队列深度自动伸缩

---

*参考资料：*
- [WASI-NN Specification](https://github.com/WebAssembly/wasi-nn)
- [WasmEdge AI Inference Documentation](https://wasmedge.org/book/en/write_wasm/rust/wasinn.html)
- [ONNX Runtime Web](https://onnxruntime.ai/docs/get-started/with-javascript/web.html)
- [llama.cpp](https://github.com/ggerganov/llama.cpp)
- [Candle ML Framework](https://github.com/huggingface/candle)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-38-webassembly-cloud-native KUDIG Database — Global MOC
- [[domain-15-specialized-tech/README.md|[[Domain 38: WebAssembly 云原生 (WebAssembly Cloud Native)|Domain 38: WebAssembly 云原生 (WebAssembly Cloud Native)]]]]
- Domain-38 WebAssembly 云原生 — 开源项目索引
- WebAssembly 云原生基础
- containerd Wasm 运行时
- SpinKube 框架实践
- wasmCloud 平台
- WasmEdge 运行时
- Wasm 组件模型 (Wasm Component Model)
- Wasm 插件系统 (Wasm Plugin System)
- Wasm Serverless (Wasm Serverless)
- Wasm 安全与沙箱 (Wasm Security and Sandbox)

## See Also

- 06-wasm-component-model
- 07-wasm-plugin-system
- 09-wasm-serverless
- 10-wasm-security-sandbox
