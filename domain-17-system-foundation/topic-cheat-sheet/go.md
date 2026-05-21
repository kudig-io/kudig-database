---
title: Go 生产环境速查卡
description: 涵盖 Go 1.20-1.22 生产环境 90% 以上常用语法和工具，支持快速开发和故障排查
category: cheatsheet
tags:
- go
- golang
- programming
- cheatsheet
- quick-reference
- docker
- opa
- redis
- mysql
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Go 生产环境速查卡 是什么
- 如何 Go 生产环境速查卡
trigger_keywords:
- Go
- 生产环境速查卡
- cheat
- sheet
prerequisites:
- kubectl-basics
- cloud-provider-basics
- redis-basics
- mysql-basics
- policy-basics
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: ../domain-14-ai-ml-infra/
  desc: AI 基础设施文档
- path: ../domain-17-system-foundation/topic-cheat-sheet/linux.md
  desc: Linux 速查卡
---

# Go 生产环境速查卡

> **适用版本**: Go 1.20 - 1.22 | **最后更新**: 2026-05
> **目标**: 涵盖生产环境 90% 以上常用语法和工具，支持快速开发和故障排查

---

## 📋 目录

- [环境配置](#环境配置)
- [基础语法](#基础语法)
- [数据结构](#数据结构)
- [函数与方法](#函数与方法)
- [并发编程](#并发编程)
- [错误处理](#错误处理)
- [包管理](#包管理)
- [文件操作](#文件操作)
- [网络编程](#网络编程)
- [数据库操作](#数据库操作)
- [测试与基准](#测试与基准)
- [性能优化](#性能优化)
- [常用标准库](#常用标准库)
- [生产最佳实践](#生产最佳实践)

---

## 环境配置

### Go 版本管理

```bash
# 查看 Go 版本
go version

# 查看环境变量
go env

# 重要环境变量
export GOROOT=/usr/local/go      # Go 安装路径
export GOPATH=$HOME/go           # 工作空间 (Go 1.11+ 可选)
export GOBIN=$GOPATH/bin         # 二进制安装路径
export GOPROXY=https://proxy.golang.org,direct  # 模块代理
export GOSUMDB=sum.golang.org    # 校验和数据库
export GOPRIVATE=github.com/myorg/*  # 私有模块

# 国内代理 (加速下载)
export GOPROXY=https://goproxy.cn,direct
export GOPROXY=https://goproxy.io,direct

# 设置环境变量 (永久)
go env -w GOPROXY=https://goproxy.cn,direct
go env -w GO111MODULE=on
```

**版本兼容性**:
- **Go 1.20** (2023-02): 泛型改进、标准库增强
- **Go 1.21** (2023-08): 内置工具链管理、性能优化
- **Go 1.22** (2024-02): for 循环变量作用域变更、路由模式增强

### 项目初始化

```bash
# 创建新项目 (Go 1.11+ 使用 Go Modules)
mkdir myproject && cd myproject
go mod init github.com/username/myproject

# 项目结构
myproject/
├── go.mod           # 依赖声明
├── go.sum           # 依赖校验和
├── main.go          # 主程序
├── internal/        # 私有代码 (不可被外部导入)
│   └── service/
├── pkg/             # 公共库
│   └── utils/
├── cmd/             # 命令行工具
│   └── cli/
├── api/             # API 定义 (OpenAPI/protobuf)
├── web/             # 静态文件
└── configs/         # 配置文件

# 添加依赖
go get github.com/gin-gonic/gin@v1.9.1
go get -u github.com/gin-gonic/gin  # 更新到最新

# 整理依赖
go mod tidy  # 清理未使用依赖
go mod download  # 下载依赖到缓存
go mod verify  # 验证依赖完整性

# 查看依赖
go list -m all  # 所有依赖
go list -m -u all  # 显示可更新依赖
go mod graph  # 依赖关系图
```

### 编译与运行

```bash
# 运行程序
go run main.go
go run .

# 编译
go build  # 编译为当前平台二进制
go build -o myapp  # 指定输出文件名
go build -ldflags="-s -w"  # 减小二进制体积 (去除符号表)

# 交叉编译
GOOS=linux GOARCH=amd64 go build -o myapp-linux
GOOS=windows GOARCH=amd64 go build -o myapp.exe
GOOS=darwin GOARCH=arm64 go build -o myapp-mac-arm64

# 支持的平台
go tool dist list

# 安装到 $GOBIN
go install

# 清理缓存
go clean -cache
go clean -modcache  # 清理模块缓存
```

**交叉编译目标**:
- `GOOS=linux GOARCH=amd64` - Linux x86-64
- `GOOS=linux GOARCH=arm64` - Linux ARM64 (服务器、树莓派)
- `GOOS=darwin GOARCH=amd64` - macOS Intel
- `GOOS=darwin GOARCH=arm64` - macOS Apple Silicon
- `GOOS=windows GOARCH=amd64` - Windows x86-64

---

## 基础语法

### 变量与常量

```go
package main

import "fmt"

func main() {
    // 变量声明
    var name string = "John"
    var age int = 30
    var isActive bool = true
    
    // 类型推断
    var city = "New York"
    
    // 短声明 (仅在函数内)
    country := "USA"
    
    // 多变量声明
    var x, y, z int = 1, 2, 3
    a, b := 10, "hello"
    
    // 零值
    var i int       // 0
    var f float64   // 0.0
    var s string    // ""
    var ptr *int    // nil
    var arr [3]int  // [0 0 0]
    
    // 常量
    const PI = 3.14159
    const (
        StatusOK = 200
        StatusNotFound = 404
    )
    
    // iota (自增常量)
    const (
        Sunday = iota    // 0
        Monday           // 1
        Tuesday          // 2
        Wednesday        // 3
    )
    
    // 类型转换
    var a int = 10
    var b float64 = float64(a)
    var c string = fmt.Sprintf("%d", a)
}
```

### 数据类型

```go
// 基础类型
bool
string

// 整数类型
int  int8  int16  int32  int64    // 有符号
uint uint8 uint16 uint32 uint64   // 无符号
byte    // uint8 的别名
rune    // int32 的别名 (Unicode 码点)

// 浮点数
float32 float64

// 复数
complex64 complex128

// 指针
*T

// 数组 (固定长度)
[n]T

// 切片 (动态数组)
[]T

// 映射
map[K]V

// 通道
chan T

// 结构体
struct { ... }

// 接口
interface { ... }

// 函数
func(T1, T2) T3
```

### 流程控制

```go
// if-else
if x > 0 {
    fmt.Println("Positive")
} else if x < 0 {
    fmt.Println("Negative")
} else {
    fmt.Println("Zero")
}

// if 简短语句
if val, err := someFunc(); err != nil {
    fmt.Println("Error:", err)
} else {
    fmt.Println("Value:", val)
}

// switch
switch day := time.Now().Weekday(); day {
case time.Saturday, time.Sunday:
    fmt.Println("Weekend")
default:
    fmt.Println("Weekday")
}

// switch 无条件 (替代长 if-else)
score := 85
switch {
case score >= 90:
    fmt.Println("A")
case score >= 80:
    fmt.Println("B")
default:
    fmt.Println("C")
}

// for 循环 (唯一循环语句)
for i := 0; i < 10; i++ {
    fmt.Println(i)
}

// while 风格
i := 0
for i < 10 {
    fmt.Println(i)
    i++
}

// 无限循环
for {
    // break 退出
    // continue 继续下一轮
}

// range 遍历
nums := []int{1, 2, 3, 4, 5}
for i, v := range nums {
    fmt.Printf("Index: %d, Value: %d\n", i, v)
}

// 仅需值
for _, v := range nums {
    fmt.Println(v)
}

// 遍历 map
m := map[string]int{"a": 1, "b": 2}
for k, v := range m {
    fmt.Printf("%s: %d\n", k, v)
}

// 遍历字符串 (按 rune)
for i, r := range "Hello, 世界" {
    fmt.Printf("%d: %c\n", i, r)
}
```

**Go 1.22 变更**: for 循环变量作用域
```go
// Go 1.21 及之前 (可能导致闭包问题)
for i := 0; i < 3; i++ {
    go func() {
        fmt.Println(i)  // 可能打印 3 3 3
    }()
}

// Go 1.22+ (每次迭代新变量)
for i := 0; i < 3; i++ {
    go func() {
        fmt.Println(i)  // 打印 0 1 2 (顺序不定)
    }()
}

// 或使用闭包参数 (兼容所有版本)
for i := 0; i < 3; i++ {
    go func(i int) {
        fmt.Println(i)
    }(i)
}
```

---

## 数据结构

### 数组与切片

```go
// 数组 (固定长度)
var arr1 [5]int
arr2 := [5]int{1, 2, 3, 4, 5}
arr3 := [...]int{1, 2, 3}  // 自动计算长度

// 切片 (动态数组)
var slice1 []int
slice2 := []int{1, 2, 3}
slice3 := make([]int, 5)       // 长度 5, 容量 5
slice4 := make([]int, 5, 10)   // 长度 5, 容量 10

// 切片操作
nums := []int{0, 1, 2, 3, 4, 5}
fmt.Println(nums[1:4])   // [1 2 3]
fmt.Println(nums[:3])    // [0 1 2]
fmt.Println(nums[3:])    // [3 4 5]
fmt.Println(nums[:])     // [0 1 2 3 4 5]

// append (追加元素)
slice := []int{1, 2, 3}
slice = append(slice, 4)           // [1 2 3 4]
slice = append(slice, 5, 6)        // [1 2 3 4 5 6]
slice = append(slice, []int{7, 8}...)  // [1 2 3 4 5 6 7 8]

// copy (复制切片)
src := []int{1, 2, 3}
dst := make([]int, len(src))
copy(dst, src)

// len 和 cap
fmt.Println(len(slice))  // 长度
fmt.Println(cap(slice))  // 容量

// 删除元素 (无内置方法)
// 删除索引 i
slice = append(slice[:i], slice[i+1:]...)

// 二维切片
matrix := [][]int{
    {1, 2, 3},
    {4, 5, 6},
}
```

### 映射 (Map)

```go
// 声明
var m1 map[string]int
m2 := map[string]int{}
m3 := make(map[string]int)

// 初始化
ages := map[string]int{
    "Alice": 25,
    "Bob":   30,
}

// 赋值
ages["Charlie"] = 35

// 取值
age := ages["Alice"]

// 检查键是否存在
age, ok := ages["Dave"]
if !ok {
    fmt.Println("Dave not found")
}

// 删除
delete(ages, "Bob")

// 遍历
for name, age := range ages {
    fmt.Printf("%s: %d\n", name, age)
}

// 长度
fmt.Println(len(ages))

// 注意: map 不是并发安全的
// 并发使用需要 sync.Mutex 或 sync.Map
```

### 结构体

```go
// 定义结构体
type Person struct {
    Name string
    Age  int
    City string
}

// 创建实例
p1 := Person{"Alice", 25, "NYC"}
p2 := Person{Name: "Bob", Age: 30}  // 部分字段
p3 := Person{}  // 零值

// 指针
p4 := &Person{"Charlie", 35, "LA"}

// 访问字段
fmt.Println(p1.Name)
p1.Age = 26

// 匿名字段 (嵌入)
type Employee struct {
    Person     // 嵌入 Person
    ID     int
    Salary float64
}

e := Employee{
    Person: Person{Name: "Dave", Age: 28},
    ID:     1001,
    Salary: 50000,
}
fmt.Println(e.Name)  // 直接访问嵌入字段

// 结构体标签 (用于 JSON/XML 等)
type User struct {
    ID       int    `json:"id"`
    Username string `json:"username"`
    Email    string `json:"email,omitempty"`
    Password string `json:"-"`  // 忽略字段
}

// 比较
// 如果所有字段可比较，结构体可以用 == 比较
p5 := Person{"Alice", 25, "NYC"}
p6 := Person{"Alice", 25, "NYC"}
fmt.Println(p5 == p6)  // true
```

---

## 函数与方法

### 函数

```go
// 基础函数
func add(a int, b int) int {
    return a + b
}

// 简化参数类型
func add(a, b int) int {
    return a + b
}

// 多返回值
func divmod(a, b int) (int, int) {
    return a / b, a % b
}

// 命名返回值
func split(sum int) (x, y int) {
    x = sum * 4 / 9
    y = sum - x
    return  // 裸返回
}

// 可变参数
func sum(nums ...int) int {
    total := 0
    for _, n := range nums {
        total += n
    }
    return total
}

// 调用
sum(1, 2, 3, 4, 5)
nums := []int{1, 2, 3}
sum(nums...)  // 展开切片

// 函数作为值
func compute(fn func(int, int) int) int {
    return fn(3, 4)
}

add := func(a, b int) int {
    return a + b
}
result := compute(add)

// 闭包
func counter() func() int {
    count := 0
    return func() int {
        count++
        return count
    }
}

c := counter()
fmt.Println(c())  // 1
fmt.Println(c())  // 2

// defer (延迟执行)
func openFile() {
    f, err := os.Open("file.txt")
    if err != nil {
        return
    }
    defer f.Close()  // 函数返回前执行
    
    // 处理文件...
}

// defer 栈 (LIFO)
defer fmt.Println("1")
defer fmt.Println("2")
defer fmt.Println("3")
// 输出: 3 2 1
```

### 方法

```go
// 值接收者
type Rectangle struct {
    Width, Height float64
}

func (r Rectangle) Area() float64 {
    return r.Width * r.Height
}

// 指针接收者 (可以修改接收者)
func (r *Rectangle) Scale(factor float64) {
    r.Width *= factor
    r.Height *= factor
}

// 使用
rect := Rectangle{Width: 10, Height: 5}
fmt.Println(rect.Area())  // 50

rect.Scale(2)
fmt.Println(rect.Area())  // 200

// 指针接收者 vs 值接收者
// 1. 方法需要修改接收者 → 指针接收者
// 2. 接收者是大结构体 → 指针接收者 (避免复制)
// 3. 一致性: 如果某些方法需要指针接收者，其他方法也应使用
```

### 接口

```go
// 定义接口
type Shape interface {
    Area() float64
    Perimeter() float64
}

// 实现接口 (隐式)
type Circle struct {
    Radius float64
}

func (c Circle) Area() float64 {
    return math.Pi * c.Radius * c.Radius
}

func (c Circle) Perimeter() float64 {
    return 2 * math.Pi * c.Radius
}

// 使用接口
func printShapeInfo(s Shape) {
    fmt.Printf("Area: %.2f, Perimeter: %.2f\n", s.Area(), s.Perimeter())
}

circle := Circle{Radius: 5}
printShapeInfo(circle)

// 空接口 (任意类型)
var i interface{}
i = 42
i = "hello"
i = Circle{Radius: 3}

// 类型断言
var x interface{} = "hello"
s := x.(string)  // 成功
n := x.(int)     // panic

// 类型断言 (安全)
s, ok := x.(string)
if ok {
    fmt.Println(s)
}

// 类型switch
func do(i interface{}) {
    switch v := i.(type) {
    case int:
        fmt.Printf("Integer: %d\n", v)
    case string:
        fmt.Printf("String: %s\n", v)
    case Circle:
        fmt.Printf("Circle: %+v\n", v)
    default:
        fmt.Printf("Unknown type: %T\n", v)
    }
}

// 常用接口
// io.Reader
type Reader interface {
    Read(p []byte) (n int, err error)
}

// io.Writer
type Writer interface {
    Write(p []byte) (n int, err error)
}

// error
type error interface {
    Error() string
}

// Stringer (fmt.Println 使用)
type Stringer interface {
    String() string
}
```

---

## 并发编程

### Goroutine

```go
// 启动 goroutine
go func() {
    fmt.Println("Hello from goroutine")
}()

// 带参数
go func(msg string) {
    fmt.Println(msg)
}("Hello")

// 等待 goroutine (使用 channel 或 sync.WaitGroup)
// 方法 1: WaitGroup
var wg sync.WaitGroup

for i := 0; i < 5; i++ {
    wg.Add(1)
    go func(id int) {
        defer wg.Done()
        fmt.Printf("Goroutine %d\n", id)
    }(i)
}

wg.Wait()

// 方法 2: Channel
done := make(chan bool)

go func() {
    fmt.Println("Working...")
    time.Sleep(1 * time.Second)
    done <- true
}()

<-done  // 等待
```

### Channel

```go
// 创建 channel
ch := make(chan int)         // 无缓冲
ch := make(chan int, 10)     // 缓冲大小 10

// 发送和接收
ch <- 42    // 发送
v := <-ch   // 接收

// 关闭 channel
close(ch)

// 检查 channel 是否关闭
v, ok := <-ch
if !ok {
    fmt.Println("Channel closed")
}

// 遍历 channel (直到关闭)
for v := range ch {
    fmt.Println(v)
}

// 单向 channel
func send(ch chan<- int) {  // 仅发送
    ch <- 42
}

func receive(ch <-chan int) {  // 仅接收
    v := <-ch
}

// select (多路复用)
ch1 := make(chan int)
ch2 := make(chan int)

select {
case v := <-ch1:
    fmt.Println("Received from ch1:", v)
case v := <-ch2:
    fmt.Println("Received from ch2:", v)
case <-time.After(1 * time.Second):
    fmt.Println("Timeout")
default:
    fmt.Println("No data")
}

// 常见模式: Worker Pool
func worker(id int, jobs <-chan int, results chan<- int) {
    for j := range jobs {
        fmt.Printf("Worker %d processing job %d\n", id, j)
        time.Sleep(time.Second)
        results <- j * 2
    }
}

func main() {
    jobs := make(chan int, 100)
    results := make(chan int, 100)
    
    // 启动 3 个 worker
    for w := 1; w <= 3; w++ {
        go worker(w, jobs, results)
    }
    
    // 发送 5 个任务
    for j := 1; j <= 5; j++ {
        jobs <- j
    }
    close(jobs)
    
    // 收集结果
    for a := 1; a <= 5; a++ {
        <-results
    }
}
```

### 同步原语 (sync)

```go
// Mutex (互斥锁)
var (
    mu      sync.Mutex
    counter int
)

func increment() {
    mu.Lock()
    defer mu.Unlock()
    counter++
}

// RWMutex (读写锁)
var (
    rwMu sync.RWMutex
    data map[string]string
)

func read(key string) string {
    rwMu.RLock()
    defer rwMu.RUnlock()
    return data[key]
}

func write(key, value string) {
    rwMu.Lock()
    defer rwMu.Unlock()
    data[key] = value
}

// WaitGroup (等待组)
var wg sync.WaitGroup

for i := 0; i < 10; i++ {
    wg.Add(1)
    go func(id int) {
        defer wg.Done()
        // 工作...
    }(i)
}

wg.Wait()

// Once (仅执行一次)
var once sync.Once

func initialize() {
    once.Do(func() {
        fmt.Println("Initialization")
    })
}

// Cond (条件变量)
var (
    cond   = sync.NewCond(&sync.Mutex{})
    ready  bool
)

func wait() {
    cond.L.Lock()
    for !ready {
        cond.Wait()
    }
    cond.L.Unlock()
}

func signal() {
    cond.L.Lock()
    ready = true
    cond.L.Unlock()
    cond.Broadcast()
}

// Atomic (原子操作)
var counter int64

atomic.AddInt64(&counter, 1)
atomic.LoadInt64(&counter)
atomic.StoreInt64(&counter, 10)
atomic.SwapInt64(&counter, 20)
atomic.CompareAndSwapInt64(&counter, 20, 30)
```

### Context (上下文)

```go
import "context"

// 创建 context
ctx := context.Background()  // 根 context
ctx := context.TODO()        // 待定 context

// WithCancel (手动取消)
ctx, cancel := context.WithCancel(context.Background())
defer cancel()

go func() {
    select {
    case <-ctx.Done():
        fmt.Println("Cancelled:", ctx.Err())
        return
    }
}()

cancel()  // 取消

// WithTimeout (超时取消)
ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
defer cancel()

select {
case <-time.After(3 * time.Second):
    fmt.Println("Done")
case <-ctx.Done():
    fmt.Println("Timeout:", ctx.Err())
}

// WithDeadline (截止时间)
deadline := time.Now().Add(5 * time.Second)
ctx, cancel := context.WithDeadline(context.Background(), deadline)
defer cancel()

// WithValue (传递值)
ctx := context.WithValue(context.Background(), "userID", 123)
userID := ctx.Value("userID").(int)

// 实际应用: HTTP 请求
func handler(w http.ResponseWriter, r *http.Request) {
    ctx := r.Context()
    
    // 模拟长时间操作
    select {
    case <-time.After(5 * time.Second):
        fmt.Fprintln(w, "Done")
    case <-ctx.Done():
        fmt.Println("Request cancelled:", ctx.Err())
        http.Error(w, "Request cancelled", 499)
    }
}
```

---

## 错误处理

### 错误基础

```go
// 返回错误
func divide(a, b float64) (float64, error) {
    if b == 0 {
        return 0, errors.New("division by zero")
    }
    return a / b, nil
}

// 使用 fmt.Errorf (格式化错误)
func divide(a, b float64) (float64, error) {
    if b == 0 {
        return 0, fmt.Errorf("cannot divide %f by zero", a)
    }
    return a / b, nil
}

// 检查错误
result, err := divide(10, 0)
if err != nil {
    log.Fatal(err)
}

// 自定义错误类型
type DivisionError struct {
    Dividend float64
    Divisor  float64
}

func (e *DivisionError) Error() string {
    return fmt.Sprintf("cannot divide %f by %f", e.Dividend, e.Divisor)
}

func divide(a, b float64) (float64, error) {
    if b == 0 {
        return 0, &DivisionError{Dividend: a, Divisor: b}
    }
    return a / b, nil
}

// 错误包装 (Go 1.13+)
if err != nil {
    return fmt.Errorf("failed to open file: %w", err)
}

// 错误解包
var pathErr *os.PathError
if errors.As(err, &pathErr) {
    fmt.Println("Path:", pathErr.Path)
}

// 错误判断
if errors.Is(err, os.ErrNotExist) {
    fmt.Println("File does not exist")
}
```

### Panic 和 Recover

```go
// panic (不可恢复错误)
func mustOpen(filename string) *os.File {
    f, err := os.Open(filename)
    if err != nil {
        panic(err)  // 抛出 panic
    }
    return f
}

// recover (恢复 panic)
func safeExecute(fn func()) {
    defer func() {
        if r := recover(); r != nil {
            fmt.Println("Recovered from panic:", r)
        }
    }()
    
    fn()
}

// 使用
safeExecute(func() {
    panic("something went wrong")
})
fmt.Println("Program continues")

// 注意: panic/recover 仅用于真正不可恢复的错误
// 常规错误处理应使用 error 返回值
```

---

## 包管理

### Go Modules

```bash
# 初始化模块
go mod init github.com/username/myproject

# 添加依赖 (自动添加到 go.mod)
go get github.com/gin-gonic/gin@v1.9.1
go get github.com/gin-gonic/gin@latest
go get -u github.com/gin-gonic/gin  # 更新

# 整理依赖
go mod tidy

# 下载依赖
go mod download

# 查看依赖
go list -m all
go list -m -u all  # 显示可更新

# 查看依赖原因
go mod why -m github.com/pkg/errors

# 依赖关系图
go mod graph

# 验证依赖
go mod verify

# vendor (将依赖复制到项目)
go mod vendor

# 编辑 go.mod
go mod edit -require=github.com/pkg/errors@v0.9.1
go mod edit -droprequire=github.com/old/package
```

### go.mod 文件

```go
module github.com/username/myproject

go 1.22

require (
    github.com/gin-gonic/gin v1.9.1
    github.com/pkg/errors v0.9.1
)

require (
    // 间接依赖 (indirect)
    github.com/gin-contrib/sse v0.1.0 // indirect
)

replace (
    // 替换依赖 (用于本地开发或 fork)
    github.com/old/package => github.com/new/package v1.0.0
    github.com/example/lib => ../lib
)

exclude (
    // 排除特定版本
    github.com/bad/package v1.2.3
)
```

### 私有仓库

```bash
# 配置私有仓库
export GOPRIVATE="github.com/myorg/*,gitlab.com/myteam/*"
export GONOPROXY="github.com/myorg/*"
export GONOSUMDB="github.com/myorg/*"

# 配置 Git 凭证
git config --global url."https://username:token@github.com/".insteadOf "https://github.com/"

# 或使用 SSH
git config --global url."git@github.com:".insteadOf "https://github.com/"
```

---

## 文件操作

### 文件读写

```go
import (
    "io"
    "os"
)

// 读取整个文件
data, err := os.ReadFile("file.txt")  // Go 1.16+
if err != nil {
    log.Fatal(err)
}
fmt.Println(string(data))

// 写入文件
err := os.WriteFile("file.txt", []byte("Hello"), 0644)

// 打开文件
f, err := os.Open("file.txt")  // 只读
if err != nil {
    log.Fatal(err)
}
defer f.Close()

// 创建/覆盖文件
f, err := os.Create("new.txt")

// 追加写入
f, err := os.OpenFile("file.txt", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)

// 读取文件 (逐行)
scanner := bufio.NewScanner(f)
for scanner.Scan() {
    fmt.Println(scanner.Text())
}

// 写入文件 (带缓冲)
writer := bufio.NewWriter(f)
writer.WriteString("Hello, World!\n")
writer.Flush()

// 复制文件
src, _ := os.Open("source.txt")
defer src.Close()

dst, _ := os.Create("dest.txt")
defer dst.Close()

io.Copy(dst, src)

// 获取文件信息
info, err := os.Stat("file.txt")
if err != nil {
    if os.IsNotExist(err) {
        fmt.Println("File does not exist")
    }
}
fmt.Println("Size:", info.Size())
fmt.Println("Mode:", info.Mode())
fmt.Println("ModTime:", info.ModTime())
```

### 目录操作

```go
// 创建目录
os.Mkdir("mydir", 0755)
os.MkdirAll("path/to/dir", 0755)  // 递归创建

// 删除
os.Remove("file.txt")  // 删除文件
os.RemoveAll("mydir")  // 递归删除目录

// 重命名/移动
os.Rename("old.txt", "new.txt")

// 列出目录
entries, err := os.ReadDir(".")
for _, entry := range entries {
    fmt.Println(entry.Name(), entry.IsDir())
}

// 遍历目录树
filepath.Walk(".", func(path string, info os.FileInfo, err error) error {
    if err != nil {
        return err
    }
    fmt.Println(path)
    return nil
})

// Go 1.16+ WalkDir (更快)
filepath.WalkDir(".", func(path string, d fs.DirEntry, err error) error {
    if err != nil {
        return err
    }
    fmt.Println(path)
    return nil
})

// 当前工作目录
dir, _ := os.Getwd()

// 改变工作目录
os.Chdir("/path/to/dir")

// 临时目录
tempDir, _ := os.MkdirTemp("", "myapp-*")
defer os.RemoveAll(tempDir)

// 临时文件
tempFile, _ := os.CreateTemp("", "myfile-*.txt")
defer os.Remove(tempFile.Name())
```

### JSON 操作

```go
import "encoding/json"

// 结构体
type Person struct {
    Name string `json:"name"`
    Age  int    `json:"age"`
    City string `json:"city,omitempty"`
}

// 编码 (序列化)
p := Person{Name: "Alice", Age: 25}
data, err := json.Marshal(p)
fmt.Println(string(data))  // {"name":"Alice","age":25}

// 格式化编码
data, err := json.MarshalIndent(p, "", "  ")

// 解码 (反序列化)
jsonStr := `{"name":"Bob","age":30}`
var p Person
err := json.Unmarshal([]byte(jsonStr), &p)

// 编码到文件
f, _ := os.Create("data.json")
defer f.Close()
encoder := json.NewEncoder(f)
encoder.SetIndent("", "  ")
encoder.Encode(p)

// 从文件解码
f, _ := os.Open("data.json")
defer f.Close()
var p Person
decoder := json.NewDecoder(f)
decoder.Decode(&p)

// 处理未知结构 (interface{})
var result map[string]interface{}
json.Unmarshal([]byte(jsonStr), &result)
name := result["name"].(string)
```

---

## 网络编程

### HTTP 客户端

```go
import "net/http"

// GET 请求
resp, err := http.Get("https://api.example.com/users")
if err != nil {
    log.Fatal(err)
}
defer resp.Body.Close()

body, _ := io.ReadAll(resp.Body)
fmt.Println(string(body))

// POST 请求
data := []byte(`{"name":"John"}`)
resp, err := http.Post("https://api.example.com/users",
    "application/json", bytes.NewBuffer(data))

// 自定义请求
req, _ := http.NewRequest("PUT", "https://api.example.com/users/1", 
    bytes.NewBuffer(data))
req.Header.Set("Content-Type", "application/json")
req.Header.Set("Authorization", "Bearer token")

client := &http.Client{Timeout: 10 * time.Second}
resp, err := client.Do(req)

// 带 Context
ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
defer cancel()

req, _ := http.NewRequestWithContext(ctx, "GET", "https://api.example.com/users", nil)
resp, err := client.Do(req)

// 表单提交
data := url.Values{}
data.Set("name", "John")
data.Set("email", "john@example.com")

resp, err := http.PostForm("https://api.example.com/users", data)

// 下载文件
resp, _ := http.Get("https://example.com/file.zip")
defer resp.Body.Close()

out, _ := os.Create("file.zip")
defer out.Close()

io.Copy(out, resp.Body)
```

### HTTP 服务器

```go
// 简单服务器
http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
    fmt.Fprintf(w, "Hello, World!")
})

http.HandleFunc("/api/users", func(w http.ResponseWriter, r *http.Request) {
    switch r.Method {
    case "GET":
        // 处理 GET
    case "POST":
        // 处理 POST
    default:
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
    }
})

log.Fatal(http.ListenAndServe(":8080", nil))

// 自定义 Handler
type MyHandler struct{}

func (h *MyHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
    fmt.Fprintf(w, "Hello from MyHandler")
}

http.Handle("/custom", &MyHandler{})

// ServeMux (路由器)
mux := http.NewServeMux()
mux.HandleFunc("/", homeHandler)
mux.HandleFunc("/api/users", usersHandler)

server := &http.Server{
    Addr:         ":8080",
    Handler:      mux,
    ReadTimeout:  10 * time.Second,
    WriteTimeout: 10 * time.Second,
}

log.Fatal(server.ListenAndServe())

// 中间件
func logging(next http.Handler) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        log.Printf("%s %s", r.Method, r.URL.Path)
        next.ServeHTTP(w, r)
    })
}

mux := http.NewServeMux()
mux.Handle("/", logging(http.HandlerFunc(homeHandler)))

// JSON 响应
func apiHandler(w http.ResponseWriter, r *http.Request) {
    w.Header().Set("Content-Type", "application/json")
    
    data := map[string]interface{}{
        "message": "success",
        "data":    []string{"item1", "item2"},
    }
    
    json.NewEncoder(w).Encode(data)
}

// 文件服务器
http.Handle("/static/", http.StripPrefix("/static/", 
    http.FileServer(http.Dir("./static"))))

// HTTPS 服务器
log.Fatal(http.ListenAndServeTLS(":443", "cert.pem", "key.pem", nil))
```

**Go 1.22 增强路由** (net/http):
```go
// Go 1.22+ 支持路径参数和方法匹配
mux := http.NewServeMux()

// 方法匹配
mux.HandleFunc("GET /users", listUsers)
mux.HandleFunc("POST /users", createUser)

// 路径参数
mux.HandleFunc("GET /users/{id}", func(w http.ResponseWriter, r *http.Request) {
    id := r.PathValue("id")  // Go 1.22+
    fmt.Fprintf(w, "User ID: %s", id)
})

// 通配符
mux.HandleFunc("/files/{path...}", serveFiles)
```

### TCP 网络编程

```go
// TCP 服务器
listener, err := net.Listen("tcp", ":8080")
if err != nil {
    log.Fatal(err)
}
defer listener.Close()

for {
    conn, err := listener.Accept()
    if err != nil {
        log.Println(err)
        continue
    }
    
    go handleConnection(conn)
}

func handleConnection(conn net.Conn) {
    defer conn.Close()
    
    buffer := make([]byte, 1024)
    n, err := conn.Read(buffer)
    if err != nil {
        return
    }
    
    fmt.Println("Received:", string(buffer[:n]))
    conn.Write([]byte("Hello from server\n"))
}

// TCP 客户端
conn, err := net.Dial("tcp", "localhost:8080")
if err != nil {
    log.Fatal(err)
}
defer conn.Close()

conn.Write([]byte("Hello from client\n"))

buffer := make([]byte, 1024)
n, _ := conn.Read(buffer)
fmt.Println("Received:", string(buffer[:n]))
```

---

## 数据库操作

### database/sql (标准库)

```go
import (
    "database/sql"
    _ "github.com/lib/pq"  // PostgreSQL
    // _ "github.com/go-sql-driver/mysql"  // MySQL
)

// 连接数据库
db, err := sql.Open("postgres", 
    "host=localhost port=5432 user=postgres password=secret dbname=mydb sslmode=disable")
if err != nil {
    log.Fatal(err)
}
defer db.Close()

// 测试连接
err = db.Ping()

// 设置连接池
db.SetMaxOpenConns(25)
db.SetMaxIdleConns(5)
db.SetConnMaxLifetime(5 * time.Minute)

// 查询单行
var name string
var age int
err = db.QueryRow("SELECT name, age FROM users WHERE id = $1", 1).Scan(&name, &age)
if err == sql.ErrNoRows {
    fmt.Println("No rows found")
}

// 查询多行
rows, err := db.Query("SELECT id, name, age FROM users WHERE age > $1", 18)
if err != nil {
    log.Fatal(err)
}
defer rows.Close()

for rows.Next() {
    var id int
    var name string
    var age int
    
    err = rows.Scan(&id, &name, &age)
    if err != nil {
        log.Fatal(err)
    }
    
    fmt.Printf("ID: %d, Name: %s, Age: %d\n", id, name, age)
}

err = rows.Err()

// 插入
result, err := db.Exec("INSERT INTO users (name, age) VALUES ($1, $2)", "Alice", 25)
if err != nil {
    log.Fatal(err)
}

lastID, _ := result.LastInsertId()
rowsAffected, _ := result.RowsAffected()

// 更新
result, err := db.Exec("UPDATE users SET age = $1 WHERE id = $2", 26, 1)

// 删除
result, err := db.Exec("DELETE FROM users WHERE id = $1", 1)

// 事务
tx, err := db.Begin()
if err != nil {
    log.Fatal(err)
}

_, err = tx.Exec("INSERT INTO users (name, age) VALUES ($1, $2)", "Bob", 30)
if err != nil {
    tx.Rollback()
    log.Fatal(err)
}

_, err = tx.Exec("UPDATE accounts SET balance = balance - 100 WHERE user_id = $1", 1)
if err != nil {
    tx.Rollback()
    log.Fatal(err)
}

err = tx.Commit()

// 预处理语句
stmt, err := db.Prepare("SELECT name, age FROM users WHERE id = $1")
defer stmt.Close()

var name string
var age int
err = stmt.QueryRow(1).Scan(&name, &age)
```

**驱动版本**:
- **PostgreSQL**: `github.com/lib/pq` v1.10+
- **MySQL**: `github.com/go-sql-driver/mysql` v1.7+
- **SQLite**: `github.com/mattn/go-sqlite3` v1.14+ (需要 CGO)

### ORM (GORM)

```go
import "gorm.io/gorm"
import "gorm.io/driver/postgres"

// 定义模型
type User struct {
    ID        uint           `gorm:"primaryKey"`
    Name      string         `gorm:"size:100;not null"`
    Age       int
    Email     string         `gorm:"uniqueIndex"`
    CreatedAt time.Time
    UpdatedAt time.Time
}

// 连接数据库
dsn := "host=localhost user=postgres password=secret dbname=mydb port=5432"
db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{})

// 自动迁移
db.AutoMigrate(&User{})

// 创建
user := User{Name: "Alice", Age: 25, Email: "alice@example.com"}
db.Create(&user)

// 查询
var user User
db.First(&user, 1)  // 主键查询
db.First(&user, "name = ?", "Alice")  // 条件查询

var users []User
db.Find(&users)  // 查询所有
db.Where("age > ?", 18).Find(&users)  // 条件查询

// 更新
db.Model(&user).Update("age", 26)
db.Model(&user).Updates(User{Name: "Alice Smith", Age: 26})
db.Model(&user).Updates(map[string]interface{}{"age": 26, "name": "Alice"})

// 删除
db.Delete(&user, 1)
db.Where("age < ?", 18).Delete(&User{})

// 事务
db.Transaction(func(tx *gorm.DB) error {
    if err := tx.Create(&user1).Error; err != nil {
        return err
    }
    
    if err := tx.Create(&user2).Error; err != nil {
        return err
    }
    
    return nil
})

// 关联 (一对多)
type Company struct {
    ID    uint
    Name  string
    Users []User
}

// 预加载
var company Company
db.Preload("Users").First(&company, 1)
```

**GORM 版本**: v1.25+ (兼容 Go 1.20+)

---

## 测试与基准

### 单元测试

```go
// math.go
package math

func Add(a, b int) int {
    return a + b
}

// math_test.go
package math

import "testing"

func TestAdd(t *testing.T) {
    result := Add(2, 3)
    expected := 5
    
    if result != expected {
        t.Errorf("Add(2, 3) = %d; want %d", result, expected)
    }
}

// 表驱动测试
func TestAdd(t *testing.T) {
    tests := []struct {
        name string
        a, b int
        want int
    }{
        {"positive", 2, 3, 5},
        {"negative", -1, -2, -3},
        {"zero", 0, 0, 0},
    }
    
    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            got := Add(tt.a, tt.b)
            if got != tt.want {
                t.Errorf("Add(%d, %d) = %d; want %d", tt.a, tt.b, got, tt.want)
            }
        })
    }
}

// 子测试
func TestMath(t *testing.T) {
    t.Run("Add", func(t *testing.T) {
        // 测试 Add
    })
    
    t.Run("Subtract", func(t *testing.T) {
        // 测试 Subtract
    })
}

// 辅助函数
func assertEqual(t *testing.T, got, want int) {
    t.Helper()
    if got != want {
        t.Errorf("got %d; want %d", got, want)
    }
}

// 跳过测试
func TestSlow(t *testing.T) {
    if testing.Short() {
        t.Skip("skipping test in short mode")
    }
    // 长时间测试...
}

// 并行测试
func TestParallel(t *testing.T) {
    t.Parallel()
    // 测试代码...
}

// Setup 和 Teardown
func TestMain(m *testing.M) {
    // Setup
    fmt.Println("Setup")
    
    code := m.Run()
    
    // Teardown
    fmt.Println("Teardown")
    
    os.Exit(code)
}
```

### 运行测试

```bash
# 运行所有测试
go test

# 运行特定包
go test ./...  # 所有包
go test ./pkg/math

# 运行特定测试
go test -run TestAdd
go test -run TestAdd/positive

# 详细输出
go test -v

# 显示覆盖率
go test -cover
go test -coverprofile=coverage.out
go tool cover -html=coverage.out

# 短测试 (跳过慢测试)
go test -short

# 并行测试
go test -parallel 4

# 运行 N 次
go test -count=10

# 竞态检测
go test -race

# 输出到文件
go test -v > test.log
```

### 基准测试

```go
// math_test.go
func BenchmarkAdd(b *testing.B) {
    for i := 0; i < b.N; i++ {
        Add(2, 3)
    }
}

// 带输入的基准测试
func BenchmarkFibonacci(b *testing.B) {
    for i := 0; i < b.N; i++ {
        Fibonacci(10)
    }
}

// 重置计时器
func BenchmarkSetup(b *testing.B) {
    // 耗时的 setup
    data := generateData()
    
    b.ResetTimer()  // 重置计时器
    
    for i := 0; i < b.N; i++ {
        process(data)
    }
}

// 并行基准测试
func BenchmarkAddParallel(b *testing.B) {
    b.RunParallel(func(pb *testing.PB) {
        for pb.Next() {
            Add(2, 3)
        }
    })
}

// 子基准测试
func BenchmarkMath(b *testing.B) {
    b.Run("Add", func(b *testing.B) {
        for i := 0; i < b.N; i++ {
            Add(2, 3)
        }
    })
    
    b.Run("Multiply", func(b *testing.B) {
        for i := 0; i < b.N; i++ {
            Multiply(2, 3)
        }
    })
}
```

### 运行基准测试

```bash
# 运行基准测试
go test -bench=.
go test -bench=BenchmarkAdd

# 指定时间
go test -bench=. -benchtime=10s
go test -bench=. -benchtime=1000000x  # 运行 N 次

# 内存分配统计
go test -bench=. -benchmem

# CPU profile
go test -bench=. -cpuprofile=cpu.prof
go tool pprof cpu.prof

# 内存 profile
go test -bench=. -memprofile=mem.prof
go tool pprof mem.prof

# 对比基准测试
go test -bench=. > old.txt
# 修改代码...
go test -bench=. > new.txt
benchcmp old.txt new.txt  # 需要安装 benchcmp
```

### Mock 测试

```go
// 使用接口实现 mock
type Database interface {
    GetUser(id int) (*User, error)
}

type MockDatabase struct {
    Users map[int]*User
}

func (m *MockDatabase) GetUser(id int) (*User, error) {
    user, ok := m.Users[id]
    if !ok {
        return nil, errors.New("user not found")
    }
    return user, nil
}

// 测试
func TestGetUserService(t *testing.T) {
    mockDB := &MockDatabase{
        Users: map[int]*User{
            1: {ID: 1, Name: "Alice"},
        },
    }
    
    service := NewUserService(mockDB)
    user, err := service.GetUser(1)
    
    if err != nil {
        t.Fatal(err)
    }
    
    if user.Name != "Alice" {
        t.Errorf("expected Alice, got %s", user.Name)
    }
}

// 使用 testify/mock (需要安装)
import "github.com/stretchr/testify/mock"

type MockDatabase struct {
    mock.Mock
}

func (m *MockDatabase) GetUser(id int) (*User, error) {
    args := m.Called(id)
    return args.Get(0).(*User), args.Error(1)
}

func TestWithMock(t *testing.T) {
    mockDB := new(MockDatabase)
    mockDB.On("GetUser", 1).Return(&User{ID: 1, Name: "Alice"}, nil)
    
    user, _ := mockDB.GetUser(1)
    fmt.Println(user.Name)
    
    mockDB.AssertExpectations(t)
}
```

---

## 性能优化

### Profiling (性能分析)

```go
import (
    "runtime/pprof"
    _ "net/http/pprof"  // HTTP profiling
)

// CPU profiling
f, _ := os.Create("cpu.prof")
defer f.Close()

pprof.StartCPUProfile(f)
defer pprof.StopCPUProfile()

// 运行需要分析的代码...

// 内存 profiling
f, _ := os.Create("mem.prof")
defer f.Close()

pprof.WriteHeapProfile(f)

// HTTP profiling (实时查看)
import _ "net/http/pprof"

go func() {
    log.Println(http.ListenAndServe("localhost:6060", nil))
}()

// 访问 http://localhost:6060/debug/pprof/
```

### 分析 Profile

```bash
# CPU profile
go tool pprof cpu.prof
# (pprof) top  # 查看耗时最多的函数
# (pprof) list funcName  # 查看函数详情
# (pprof) web  # 可视化 (需要 graphviz)

# 内存 profile
go tool pprof mem.prof
# (pprof) top
# (pprof) list funcName

# HTTP profiling
go tool pprof http://localhost:6060/debug/pprof/profile?seconds=30  # CPU
go tool pprof http://localhost:6060/debug/pprof/heap  # 内存

# Goroutine 泄漏检查
go tool pprof http://localhost:6060/debug/pprof/goroutine

# Block profile (阻塞)
go tool pprof http://localhost:6060/debug/pprof/block

# Mutex profile (锁竞争)
go tool pprof http://localhost:6060/debug/pprof/mutex
```

### 性能优化技巧

```go
// 1. 使用 strings.Builder 拼接字符串
var sb strings.Builder
for i := 0; i < 1000; i++ {
    sb.WriteString("hello")
}
result := sb.String()

// 2. 预分配切片容量
slice := make([]int, 0, 1000)

// 3. 使用 sync.Pool 复用对象
var pool = sync.Pool{
    New: func() interface{} {
        return new(bytes.Buffer)
    },
}

buf := pool.Get().(*bytes.Buffer)
defer func() {
    buf.Reset()
    pool.Put(buf)
}()

// 4. 避免不必要的内存分配
// ❌ 每次都分配
for i := 0; i < 1000; i++ {
    data := []byte("hello")
    process(data)
}

// ✅ 复用 buffer
data := []byte("hello")
for i := 0; i < 1000; i++ {
    process(data)
}

// 5. 使用 atomic 操作代替 Mutex (简单场景)
var counter int64
atomic.AddInt64(&counter, 1)

// 6. 减少接口类型断言
// ❌
if val, ok := i.(int); ok {
    // 每次都断言
}

// ✅
val, ok := i.(int)
if ok {
    // 仅断言一次
}

// 7. 使用 map 预分配容量
m := make(map[string]int, 1000)

// 8. 并发限流 (控制 goroutine 数量)
sem := make(chan struct{}, 10)  // 最多 10 个并发

for i := 0; i < 100; i++ {
    sem <- struct{}{}
    go func() {
        defer func() { <-sem }()
        // 工作...
    }()
}

// 9. 使用 buffered channel
ch := make(chan int, 100)  // 减少阻塞

// 10. 使用 context 超时控制
ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
defer cancel()
```

---

## 常用标准库

### time (时间)

```go
// 当前时间
now := time.Now()
fmt.Println(now)  // 2026-02-11 10:30:00 +0800 CST

// 格式化 (Go 特有格式字符串: 2006-01-02 15:04:05)
formatted := now.Format("2006-01-02 15:04:05")
formatted := now.Format(time.RFC3339)  // 2026-02-11T10:30:00+08:00

// 解析
t, _ := time.Parse("2006-01-02", "2026-02-11")
t, _ := time.Parse(time.RFC3339, "2026-02-11T10:30:00+08:00")

// 时间操作
tomorrow := now.Add(24 * time.Hour)
yesterday := now.Add(-24 * time.Hour)

// 时间差
duration := time.Since(now)  // 从 now 到现在
duration := time.Until(tomorrow)  // 现在到 tomorrow

// 时间比较
if now.After(yesterday) {
    fmt.Println("now is after yesterday")
}

// 休眠
time.Sleep(2 * time.Second)

// 定时器
timer := time.NewTimer(5 * time.Second)
<-timer.C
fmt.Println("Timer expired")

// Ticker (周期性触发)
ticker := time.NewTicker(1 * time.Second)
defer ticker.Stop()

for {
    select {
    case <-ticker.C:
        fmt.Println("Tick")
    }
}

// Unix 时间戳
timestamp := now.Unix()  // 秒
timestamp := now.UnixMilli()  // 毫秒 (Go 1.17+)
timestamp := now.UnixNano()  // 纳秒

// 从时间戳创建
t := time.Unix(1707628800, 0)
```

### flag (命令行参数)

```go
import "flag"

// 定义参数
var (
    host    = flag.String("host", "localhost", "server host")
    port    = flag.Int("port", 8080, "server port")
    verbose = flag.Bool("verbose", false, "enable verbose logging")
    config  = flag.String("config", "", "config file path")
)

func main() {
    flag.Parse()
    
    fmt.Printf("Host: %s\n", *host)
    fmt.Printf("Port: %d\n", *port)
    fmt.Printf("Verbose: %t\n", *verbose)
    
    // 非标志参数
    args := flag.Args()
    fmt.Println("Args:", args)
}

// 使用:
// ./app -host=example.com -port=9000 -verbose file1.txt file2.txt
```

### log (日志)

```go
import "log"

// 基础日志
log.Println("Info message")
log.Printf("User %s logged in", username)

// 致命错误 (会调用 os.Exit(1))
log.Fatal("Fatal error")
log.Fatalf("Fatal: %s", err)

// Panic (会触发 panic)
log.Panic("Panic error")

// 自定义 Logger
logger := log.New(os.Stdout, "[INFO] ", log.Ldate|log.Ltime|log.Lshortfile)
logger.Println("Custom log message")

// 日志标志
log.SetFlags(log.Ldate | log.Ltime | log.Lmicroseconds | log.Llongfile)

// 输出到文件
f, _ := os.OpenFile("app.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
log.SetOutput(f)

// 结构化日志 (推荐使用第三方库)
// - github.com/sirupsen/logrus
// - go.uber.org/zap
// - github.com/rs/zerolog
```

### os (操作系统)

```go
// 环境变量
value := os.Getenv("PATH")
os.Setenv("MY_VAR", "value")
os.Unsetenv("MY_VAR")

// 所有环境变量
for _, env := range os.Environ() {
    fmt.Println(env)
}

// 命令行参数
args := os.Args  // []string, 第一个是程序名
fmt.Println(os.Args[1:])

// 当前工作目录
dir, _ := os.Getwd()
os.Chdir("/path/to/dir")

// 主机名
hostname, _ := os.Hostname()

// 用户信息
user, _ := user.Current()
fmt.Println(user.Username, user.HomeDir)

// 进程信息
pid := os.Getpid()
ppid := os.Getppid()

// 退出
os.Exit(0)  // 正常退出
os.Exit(1)  // 错误退出

// 信号处理
sigChan := make(chan os.Signal, 1)
signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

go func() {
    sig := <-sigChan
    fmt.Println("Received signal:", sig)
    os.Exit(0)
}()
```

### crypto (加密)

```go
import (
    "crypto/md5"
    "crypto/sha256"
    "crypto/hmac"
    "crypto/rand"
    "encoding/hex"
)

// MD5 (不推荐用于安全场景)
data := []byte("hello")
hash := md5.Sum(data)
fmt.Println(hex.EncodeToString(hash[:]))

// SHA-256 (推荐)
hash := sha256.Sum256(data)
fmt.Println(hex.EncodeToString(hash[:]))

// HMAC
key := []byte("secret")
h := hmac.New(sha256.New, key)
h.Write(data)
signature := h.Sum(nil)

// 生成随机数
b := make([]byte, 16)
rand.Read(b)
fmt.Println(hex.EncodeToString(b))

// AES 加密 (需要 crypto/aes 和 crypto/cipher)
// Base64 编码
import "encoding/base64"

encoded := base64.StdEncoding.EncodeToString(data)
decoded, _ := base64.StdEncoding.DecodeString(encoded)
```

---

## 生产最佳实践

### 项目结构

```
myproject/
├── cmd/
│   └── myapp/
│       └── main.go          # 主程序入口
├── internal/                # 私有代码 (不可被外部导入)
│   ├── config/              # 配置管理
│   ├── handler/             # HTTP handlers
│   ├── service/             # 业务逻辑
│   └── repository/          # 数据访问层
├── pkg/                     # 公共库 (可被外部导入)
│   └── utils/
├── api/                     # API 定义
│   └── openapi.yaml
├── web/                     # 静态文件
│   ├── static/
│   └── templates/
├── configs/                 # 配置文件
│   └── config.yaml
├── scripts/                 # 脚本
│   └── deploy.sh
├── deployments/             # 部署配置
│   ├── docker/
│   │   └── Dockerfile
│   └── k8s/
│       └── deployment.yaml
├── test/                    # 测试数据
├── docs/                    # 文档
├── go.mod
├── go.sum
├── Makefile
└── [[domain-07-platform-engineering/topic-code-analysis/deployment-create/README|README]].md
```

### 配置管理

```go
// 使用 viper (推荐)
import "github.com/spf13/viper"

func initConfig() {
    viper.SetConfigName("config")
    viper.SetConfigType("yaml")
    viper.AddConfigPath("./configs")
    viper.AddConfigPath(".")
    
    // 环境变量
    viper.AutomaticEnv()
    viper.SetEnvPrefix("MYAPP")
    
    // 默认值
    viper.SetDefault("server.port", 8080)
    
    if err := viper.ReadInConfig(); err != nil {
        log.Fatal(err)
    }
}

// 使用配置
port := viper.GetInt("server.port")
host := viper.GetString("server.host")

// 配置文件示例 (config.yaml)
server:
  host: localhost
  port: 8080
  timeout: 30s

database:
  host: localhost
  port: 5432
  user: postgres
  password: ${DB_PASSWORD}  # 从环境变量读取
  dbname: mydb
```

### 日志管理

```go
// 使用 zap (高性能日志库)
import "go.uber.org/zap"

// 开发环境
logger, _ := zap.NewDevelopment()
defer logger.Sync()

logger.Info("Info message",
    zap.String("user", "alice"),
    zap.Int("attempt", 3),
)

// 生产环境
logger, _ := zap.NewProduction()
defer logger.Sync()

// 自定义配置
config := zap.Config{
    Level:    zap.NewAtomicLevelAt(zap.InfoLevel),
    Encoding: "json",
    OutputPaths: []string{"stdout", "/var/log/myapp.log"},
    EncoderConfig: zapcore.EncoderConfig{
        MessageKey:  "message",
        LevelKey:    "level",
        TimeKey:     "time",
        EncodeLevel: zapcore.LowercaseLevelEncoder,
        EncodeTime:  zapcore.ISO8601TimeEncoder,
    },
}

logger, _ := config.Build()
```

### 优雅关闭

```go
func main() {
    server := &http.Server{Addr: ":8080"}
    
    // 启动服务器
    go func() {
        if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            log.Fatal(err)
        }
    }()
    
    // 等待中断信号
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, os.Interrupt, syscall.SIGTERM)
    <-quit
    
    log.Println("Shutting down server...")
    
    // 优雅关闭 (5 秒超时)
    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
    defer cancel()
    
    if err := server.Shutdown(ctx); err != nil {
        log.Fatal("Server forced to shutdown:", err)
    }
    
    log.Println("Server exited")
}
```

### 健康检查

```go
// 健康检查端点
func healthHandler(w http.ResponseWriter, r *http.Request) {
    w.Header().Set("Content-Type", "application/json")
    
    health := map[string]interface{}{
        "status": "healthy",
        "timestamp": time.Now().Unix(),
        "checks": map[string]bool{
            "database": checkDatabase(),
            "redis":    checkRedis(),
        },
    }
    
    json.NewEncoder(w).Encode(health)
}

// 就绪检查
func readinessHandler(w http.ResponseWriter, r *http.Request) {
    if !isReady() {
        http.Error(w, "Not ready", http.StatusServiceUnavailable)
        return
    }
    
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("Ready"))
}

// 注册端点
http.HandleFunc("/health", healthHandler)
http.HandleFunc("/ready", readinessHandler)
```

### Dockerfile

```dockerfile
# 多阶段构建
FROM golang:1.22-alpine AS builder

WORKDIR /app

# 复制依赖文件
COPY go.mod go.sum ./
RUN go mod download

# 复制源代码
COPY . .

# 编译
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -ldflags="-s -w" -o myapp ./cmd/myapp

# 运行阶段
FROM alpine:latest

RUN apk --no-cache add ca-certificates

WORKDIR /root/

COPY --from=builder /app/myapp .
COPY --from=builder /app/configs ./configs

EXPOSE 8080

CMD ["./myapp"]
```

### Makefile

```makefile
.PHONY: build test clean run docker

# 变量
APP_NAME=myapp
VERSION=$(shell git describe --tags --always --dirty)
BUILD_TIME=$(shell date -u '+%Y-%m-%d_%H:%M:%S')
LDFLAGS=-ldflags "-X main.Version=$(VERSION) -X main.BuildTime=$(BUILD_TIME)"

# 构建
build:
	go build $(LDFLAGS) -o bin/$(APP_NAME) ./cmd/myapp

# 运行
run:
	go run ./cmd/myapp

# 测试
test:
	go test -v -cover ./...

# 基准测试
bench:
	go test -bench=. -benchmem ./...

# 代码检查
lint:
	golangci-lint run

# 格式化
fmt:
	go fmt ./...

# 清理
clean:
	rm -rf bin/

# Docker 镜像
docker:
	docker build -t $(APP_NAME):$(VERSION) .

# 交叉编译
build-linux:
	GOOS=linux GOARCH=amd64 go build $(LDFLAGS) -o bin/$(APP_NAME)-linux ./cmd/myapp

build-windows:
	GOOS=windows GOARCH=amd64 go build $(LDFLAGS) -o bin/$(APP_NAME).exe ./cmd/myapp

build-darwin:
	GOOS=darwin GOARCH=amd64 go build $(LDFLAGS) -o bin/$(APP_NAME)-darwin ./cmd/myapp
```

---

## 附录: 常用第三方库

### Web 框架

| 库 | 版本 | 用途 | 特点 |
|-----|------|------|------|
| `github.com/gin-gonic/gin` | v1.9+ | HTTP Web 框架 | 高性能，中间件支持 |
| `github.com/labstack/echo` | v4.11+ | HTTP Web 框架 | 轻量级，快速 |
| `github.com/gofiber/fiber` | v2.52+ | HTTP Web 框架 | Express 风格 |
| `github.com/gorilla/mux` | v1.8+ | HTTP 路由器 | 强大路由功能 |

### 数据库

| 库 | 版本 | 用途 |
|-----|------|------|
| `gorm.io/gorm` | v1.25+ | ORM |
| `github.com/jmoiron/sqlx` | v1.3+ | SQL 扩展 |
| `github.com/go-redis/redis` | v9.4+ | Redis 客户端 |
| `go.mongodb.org/mongo-driver` | v1.13+ | MongoDB 驱动 |

### 工具

| 库 | 版本 | 用途 |
|-----|------|------|
| `github.com/spf13/viper` | v1.18+ | 配置管理 |
| `go.uber.org/zap` | v1.26+ | 高性能日志 |
| `github.com/stretchr/testify` | v1.8+ | 测试工具 |
| `github.com/google/uuid` | v1.5+ | UUID 生成 |

---

**文档维护**: 建议每季度更新一次  
**兼容性**: 代码已在 Go 1.20-1.22 上测试  
**反馈渠道**: 如有错误或建议，请提交 Issue
