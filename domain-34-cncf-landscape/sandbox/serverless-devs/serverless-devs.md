# Serverless Devs

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/Serverless-Devs/Serverless-Devs |
| **官网** | https://www.serverless-devs.com/ |
| **许可证** | Apache-2.0 |
| **开发语言** | TypeScript / JavaScript |
| **CNCF 分类** | Serverless / Developer Tools |
| **支持平台** | AWS Lambda / Alibaba FC / Tencent SCF / Huawei FG |

---

## 项目概述

Serverless Devs 是一个开源的 Serverless 开发者平台和命令行工具，致力于为开发者提供强大便捷的 Serverless 应用全生命周期管理能力。项目采用组件化设计，支持多云厂商的 Serverless 服务，让开发者能够使用统一的开发体验在不同云平台上开发、部署和管理 Serverless 应用。

### 核心价值

- **多云统一**: 一套工具支持 AWS、阿里云、腾讯云、华为云等主流云厂商
- **组件化设计**: 可插拔的组件体系，支持自定义扩展
- **应用商店**: 丰富的应用模板和组件生态
- **全生命周期**: 覆盖开发、测试、部署、运维全流程
- **开发者友好**: 简洁的 CLI 和直观的配置文件

---

## 核心特性

### 多云适配

```
┌─────────────────────────────────────────────────────────────┐
│                    Serverless Devs CLI                       │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │   AWS FC    │  │  Alibaba FC │  │ Tencent SCF │          │
│  │  Component  │  │  Component  │  │  Component  │          │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘          │
│         │                │                │                  │
│         ▼                ▼                ▼                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Serverless Devs Core Engine              │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐            │   │
│  │  │ Template │  │Component │  │  Plugin  │            │   │
│  │  │  Engine  │  │  Loader  │  │  System  │            │   │
│  │  └──────────┘  └──────────┘  └──────────┘            │   │
│  └──────────────────────────────────────────────────────┘   │
│         │                │                │                  │
│         ▼                ▼                ▼                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │ AWS Lambda  │  │ Alibaba FC  │  │ Tencent SCF │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 应用生命周期管理

| 阶段 | 功能 | 命令 |
|:---|:---|:---|
| **初始化** | 从模板创建项目 | `s init` |
| **开发** | 本地调试运行 | `s local invoke` |
| **构建** | 依赖安装打包 | `s build` |
| **部署** | 发布到云平台 | `s deploy` |
| **调用** | 远程函数调用 | `s invoke` |
| **日志** | 查看运行日志 | `s logs` |
| **指标** | 监控指标查看 | `s metrics` |
| **清理** | 资源删除清理 | `s remove` |

### 组件生态

- **fc**: 阿里云函数计算组件
- **fc-domain**: 自定义域名管理
- **fc-api**: API 网关配置
- **lambda**: AWS Lambda 组件
- **scf**: 腾讯云云函数组件
- **layer**: 层(依赖)管理组件

---

## 架构设计

```
┌───────────────────────────────────────────────────────────────────┐
│                        Serverless Devs                             │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    Command Line Interface                     │ │
│  │   ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐    │ │
│  │   │ init │ │deploy│ │invoke│ │ logs │ │remove│ │config│    │ │
│  │   └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘    │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                       Core Engine                             │ │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐              │ │
│  │  │   Parser   │  │  Executor  │  │   Logger   │              │ │
│  │  │  (s.yaml)  │  │            │  │            │              │ │
│  │  └────────────┘  └────────────┘  └────────────┘              │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                   Component Registry                          │ │
│  │                                                                │ │
│  │   ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │ │
│  │   │   FC    │    │ Lambda  │    │   SCF   │    │   FG    │  │ │
│  │   │Component│    │Component│    │Component│    │Component│  │ │
│  │   └────┬────┘    └────┬────┘    └────┬────┘    └────┬────┘  │ │
│  └────────│──────────────│──────────────│──────────────│───────┘ │
│           │              │              │              │          │
│           ▼              ▼              ▼              ▼          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │ Alibaba FC  │ │ AWS Lambda  │ │ Tencent SCF │ │ Huawei FG   │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└───────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 npm 安装
npm install @serverless-devs/s -g

# 验证安装
s -v

# 配置云厂商凭证（阿里云示例）
s config add

# 选择阿里云 FC，输入 AccessKey
? Please select a provider: Alibaba Cloud
? AccessKeyID: <your-access-key-id>
? AccessKeySecret: <your-access-key-secret>
```

### 初始化项目

```bash
# 从应用商店初始化
s init

# 选择模板
? Please select an Serverless-Devs Application:
❯ fc-runtime-starter - Quick start for Chinese cloud FC
  fc-custom-container - Custom container example
  fc-http-nodejs - HTTP trigger Node.js example
  fc-event-python3 - Event trigger Python example

# 直接指定模板
s init fc-runtime-starter --project-name my-serverless-app
```

### 项目配置 (s.yaml)

```yaml
edition: 3.0.0
name: my-serverless-app
access: default

vars:
  region: cn-hangzhou
  service:
    name: serverless-demo
    description: My first serverless app

resources:
  hello-world:
    component: fc3
    props:
      region: ${vars.region}
      functionName: hello-world
      description: Hello World Function
      runtime: nodejs18
      code: ./code
      handler: index.handler
      memorySize: 128
      timeout: 60
      environmentVariables:
        NODE_ENV: production
      triggers:
        - triggerName: http-trigger
          triggerType: http
          triggerConfig:
            authType: anonymous
            methods:
              - GET
              - POST
```

### 函数代码示例

```javascript
// code/index.js
exports.handler = async (event, context) => {
  console.log('Event:', JSON.stringify(event));
  
  return {
    statusCode: 200,
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      message: 'Hello from Serverless Devs!',
      timestamp: new Date().toISOString(),
      requestId: context.requestId
    })
  };
};
```

### 部署和调用

```bash
# 部署应用
s deploy

# 输出:
# hello-world:
#   region:       cn-hangzhou
#   functionName: hello-world
#   runtime:      nodejs18
#   url:          https://xxx.cn-hangzhou.fc.aliyuncs.com/2016-08-15/proxy/serverless-demo/hello-world/

# 调用函数
s invoke -e '{"key": "value"}'

# 查看日志
s logs --tail

# 本地调试
s local invoke -e '{"test": true}'
```

---

## 高级功能

### 多服务编排

```yaml
edition: 3.0.0
name: microservices-app
access: default

vars:
  region: cn-hangzhou

resources:
  # API 网关服务
  api-gateway:
    component: fc3
    props:
      region: ${vars.region}
      functionName: api-gateway
      runtime: nodejs18
      code: ./gateway
      handler: index.handler
      triggers:
        - triggerName: http
          triggerType: http
          triggerConfig:
            authType: jwt
            methods: [GET, POST, PUT, DELETE]

  # 用户服务
  user-service:
    component: fc3
    props:
      region: ${vars.region}
      functionName: user-service
      runtime: nodejs18
      code: ./user
      handler: index.handler
      environmentVariables:
        DB_HOST: ${resources.database.output.host}

  # 订单服务
  order-service:
    component: fc3
    props:
      region: ${vars.region}
      functionName: order-service
      runtime: python3.10
      code: ./order
      handler: index.handler
      asyncInvokeConfig:
        maxAsyncRetryAttempts: 3
        maxAsyncEventAgeInSeconds: 7200

  # 数据库
  database:
    component: rds
    props:
      region: ${vars.region}
      instanceType: mysql.n1.micro.1
```

### 自定义域名

```yaml
resources:
  custom-domain:
    component: fc3-domain
    props:
      region: ${vars.region}
      domainName: api.example.com
      protocol: HTTPS
      certConfig:
        certName: my-cert
        certificate: ${file(./certs/cert.pem)}
        privateKey: ${file(./certs/key.pem)}
      routeConfig:
        routes:
          - functionName: api-gateway
            path: /*
            methods: [GET, POST]
```

### Layer 层管理

```yaml
resources:
  common-layer:
    component: fc3-layer
    props:
      region: ${vars.region}
      layerName: common-dependencies
      code: ./layers/common
      compatibleRuntime:
        - nodejs18
        - nodejs16

  my-function:
    component: fc3
    props:
      region: ${vars.region}
      functionName: my-function
      runtime: nodejs18
      code: ./code
      handler: index.handler
      layers:
        - ${resources.common-layer.output.layerArn}
```

### CI/CD 集成

```yaml
# GitHub Actions 示例
# .github/workflows/deploy.yml
name: Deploy Serverless App

on:
  push:
    branches: [main]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: '18'
      
      - name: Install Serverless Devs
        run: npm install @serverless-devs/s -g
      
      - name: Configure Credentials
        run: |
          s config add --AccessKeyID ${{ secrets.ALIYUN_ACCESS_KEY_ID }} \
                       --AccessKeySecret ${{ secrets.ALIYUN_ACCESS_KEY_SECRET }} \
                       -a production
      
      - name: Deploy
        run: s deploy -a production
```

---

## 本地开发调试

### 本地调试模式

```bash
# 启动本地调试服务器
s local start

# 本地调用函数
s local invoke -e '{"httpMethod": "GET", "path": "/api/users"}'

# 断点调试 (Node.js)
s local invoke --debug-port 9229 -e '{}'
# 然后使用 Chrome DevTools 或 VS Code 连接 9229 端口

# 环境变量注入
s local invoke --env-file .env.local -e '{}'
```

### VS Code 调试配置

```json
// .vscode/launch.json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "node",
      "request": "attach",
      "name": "Attach to Serverless Devs",
      "port": 9229,
      "restart": true,
      "sourceMaps": true,
      "localRoot": "${workspaceFolder}/code",
      "remoteRoot": "/code"
    }
  ]
}
```

---

## 监控与日志

### 日志查看

```bash
# 实时日志流
s logs --tail

# 查看指定时间范围日志
s logs --start-time "2026-03-01 00:00:00" --end-time "2026-03-01 23:59:59"

# 关键词搜索
s logs --keyword "ERROR"

# 指定函数日志
s logs --function-name hello-world
```

### 指标监控

```bash
# 查看函数指标
s metrics

# 输出:
# Function: hello-world
# ├── Invocations: 1,234
# ├── Errors: 12 (0.97%)
# ├── Duration (avg): 45ms
# ├── Duration (p99): 120ms
# ├── Concurrent Executions: 5
# └── Throttles: 0
```

---

## 最佳实践

### 项目结构

```
my-serverless-app/
├── s.yaml                    # 主配置文件
├── s.local.yaml              # 本地开发配置
├── .env                      # 环境变量
├── functions/
│   ├── api/
│   │   ├── index.js
│   │   └── package.json
│   ├── worker/
│   │   ├── index.py
│   │   └── requirements.txt
│   └── scheduler/
│       └── index.js
├── layers/
│   └── common/
│       └── nodejs/
│           └── node_modules/
└── tests/
    └── api.test.js
```

### 环境管理

```yaml
# s.yaml - 多环境配置
edition: 3.0.0
name: my-app
access: ${env.SERVERLESS_ACCESS, 'default'}

vars:
  region: ${env.REGION, 'cn-hangzhou'}
  env: ${env.DEPLOY_ENV, 'dev'}

resources:
  api:
    component: fc3
    props:
      region: ${vars.region}
      functionName: api-${vars.env}
      environmentVariables:
        NODE_ENV: ${vars.env}
        LOG_LEVEL: ${vars.env == 'production' ? 'info' : 'debug'}
```

```bash
# 部署到不同环境
DEPLOY_ENV=dev s deploy
DEPLOY_ENV=staging s deploy
DEPLOY_ENV=production s deploy -a production
```

---

## 参考资源

- [GitHub 仓库](https://github.com/Serverless-Devs/Serverless-Devs)
- [官方文档](https://docs.serverless-devs.com/)
- [应用商店](https://registry.serverless-devs.com/)
- [组件开发指南](https://docs.serverless-devs.com/dev-guide/component)
- [CNCF Serverless WG](https://github.com/cncf/wg-serverless)
- [阿里云函数计算](https://www.alibabacloud.com/product/function-compute)

---

**维护者**: Kudig Team | **许可证**: MIT
