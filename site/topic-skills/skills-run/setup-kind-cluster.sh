#!/usr/bin/env bash
# ============================================================================
# setup-kind-cluster.sh — 创建本地多节点 Kind 集群用于 Skills Demo
# Create a local multi-node Kind cluster for Skills Demo
# ============================================================================
# 用法 / Usage:
#   bash setup-kind-cluster.sh
#
# 前置条件 / Prerequisites:
#   - Docker Desktop 已运行 / Docker Desktop is running
#   - kind 已安装 / kind is installed (https://kind.sigs.k8s.io/)
#   - kubectl 已安装 / kubectl is installed
# ============================================================================

set -euo pipefail

# ---- 颜色定义 / Color definitions ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

CLUSTER_NAME="${CLUSTER_NAME:-skill-demo}"
KIND_IMAGE="${KIND_IMAGE:-kindest/node:v1.31.4}"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║      🚀 Skills Demo — Kind Cluster Setup                   ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ---- Step 1: 前置检查 / Prerequisite checks ----
echo -e "${BLUE}[1/5] 检查前置条件 / Checking prerequisites...${NC}"

check_tool() {
    if ! command -v "$1" &>/dev/null; then
        echo -e "${RED}✗ $1 未安装 / $1 is not installed${NC}"
        echo -e "  安装方式 / Install: $2"
        exit 1
    fi
    echo -e "  ${GREEN}✓${NC} $1 $(command -v "$1")"
}

check_tool "docker" "https://docs.docker.com/get-docker/"
check_tool "kind"   "brew install kind  /  go install sigs.k8s.io/kind@latest"
check_tool "kubectl" "brew install kubectl  /  https://kubernetes.io/docs/tasks/tools/"

# 检查 Docker 是否运行
if ! docker info &>/dev/null; then
    echo -e "${RED}✗ Docker 未运行，请先启动 Docker Desktop${NC}"
    exit 1
fi
echo -e "  ${GREEN}✓${NC} Docker is running"
echo ""

# ---- Step 2: 检查是否已存在同名集群 / Check existing cluster ----
echo -e "${BLUE}[2/5] 检查已有集群 / Checking existing clusters...${NC}"

if kind get clusters 2>/dev/null | grep -q "^${CLUSTER_NAME}$"; then
    echo -e "  ${YELLOW}⚠ 集群 '${CLUSTER_NAME}' 已存在 / Cluster already exists${NC}"
    if [[ ! -t 0 ]]; then
        echo -e "  ${YELLOW}非交互式环境 detected，使用已有集群 / Non-interactive mode, using existing cluster${NC}"
        kubectl cluster-info --context "kind-${CLUSTER_NAME}" 2>/dev/null || true
        exit 0
    fi
    read -rp "  删除并重新创建? / Delete and recreate? [y/N] " answer
    if [[ "${answer,,}" == "y" ]]; then
        echo -e "  正在删除 / Deleting..."
        kind delete cluster --name "${CLUSTER_NAME}"
        echo -e "  ${GREEN}✓${NC} 已删除 / Deleted"
    else
        echo -e "  ${GREEN}使用已有集群 / Using existing cluster${NC}"
        kubectl cluster-info --context "kind-${CLUSTER_NAME}" 2>/dev/null || true
        exit 0
    fi
fi
echo ""

# ---- Step 3: 创建 Kind 配置 / Create Kind config ----
echo -e "${BLUE}[3/5] 生成集群配置 / Generating cluster config...${NC}"

# mktemp 模板需要兼容 BSD(macOS) 和 GNU(Linux)
# BSD mktemp 要求 XXXXXX 在末尾，因此使用纯后缀格式，然后重命名
KIND_CONFIG=$(mktemp "${TMPDIR:-/tmp}/kind-config-XXXXXXXX") || exit 1
trap 'rm -f "${KIND_CONFIG}"' EXIT
cat > "${KIND_CONFIG}" <<EOF
# Kind multi-node cluster for Skills Demo
# 1 control-plane + 2 workers — 模拟生产环境多节点场景
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: ${CLUSTER_NAME}
nodes:
  - role: control-plane
    image: ${KIND_IMAGE}
    kubeadmConfigPatches:
      - |
        kind: ClusterConfiguration
        apiServer:
          extraArgs:
            "audit-log-path": "/var/log/kubernetes/audit.log"
            "audit-log-maxage": "7"
    extraPortMappings:
      - containerPort: 30000
        hostPort: 30000
        protocol: TCP
      - containerPort: 30001
        hostPort: 30001
        protocol: TCP
  - role: worker
    image: ${KIND_IMAGE}
    labels:
      node-role.kubernetes.io/app: ""
      skill-demo/role: worker-1
  - role: worker
    image: ${KIND_IMAGE}
    labels:
      node-role.kubernetes.io/app: ""
      skill-demo/role: worker-2
EOF

echo -e "  ${GREEN}✓${NC} 配置文件: ${KIND_CONFIG}"
echo -e "  节点拓扑 / Node topology:"
echo -e "    ┌─────────────────────────────┐"
echo -e "    │  control-plane (1 node)     │"
echo -e "    ├─────────────────────────────┤"
echo -e "    │  worker-1  │  worker-2      │"
echo -e "    └─────────────────────────────┘"
echo ""

# ---- Step 4: 创建集群 / Create cluster ----
echo -e "${BLUE}[4/5] 创建 Kind 集群 / Creating Kind cluster...${NC}"
echo -e "  镜像 / Image: ${KIND_IMAGE}"
echo -e "  名称 / Name:  ${CLUSTER_NAME}"
echo -e "  这可能需要 2-5 分钟 / This may take 2-5 minutes..."
echo ""

kind create cluster --config "${KIND_CONFIG}" --wait 120s

echo ""
echo -e "  ${GREEN}✓${NC} 集群创建成功 / Cluster created successfully"
echo ""

# ---- Step 5: 验证集群 / Verify cluster ----
echo -e "${BLUE}[5/5] 验证集群状态 / Verifying cluster status...${NC}"

# 切换 context
kubectl cluster-info --context "kind-${CLUSTER_NAME}"
echo ""

# 显示节点
echo -e "  ${CYAN}节点状态 / Node status:${NC}"
kubectl get nodes -o wide
echo ""

# 等待所有节点 Ready
echo -e "  等待所有节点 Ready / Waiting for all nodes to be Ready..."
if ! kubectl wait --for=condition=Ready nodes --all --timeout=300s; then
    echo -e "  ${YELLOW}⚠ 部分节点未在 300s 内就绪，继续执行...${NC}"
fi
echo ""

# 显示系统 Pod
echo -e "  ${CYAN}系统组件 / System components:${NC}"
kubectl get pods -n kube-system --no-headers | head -10
echo ""

# 部署一些基础工作负载用于 demo
echo -e "  ${CYAN}部署 demo 工作负载 / Deploying demo workloads...${NC}"
kubectl create namespace skill-demo --dry-run=client -o yaml | kubectl apply -f -

# 部署一个简单的 nginx deployment
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: demo-nginx
  namespace: skill-demo
  labels:
    app: demo-nginx
    purpose: skill-demo
spec:
  replicas: 3
  selector:
    matchLabels:
      app: demo-nginx
  template:
    metadata:
      labels:
        app: demo-nginx
    spec:
      securityContext:
        runAsNonRoot: true
        seccompProfile:
          type: RuntimeDefault
      containers:
        - name: nginx
          image: nginx:1.27-alpine
          imagePullPolicy: IfNotPresent
          ports:
            - containerPort: 80
              protocol: TCP
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 100m
              memory: 128Mi
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            capabilities:
              drop:
                - ALL
          livenessProbe:
            httpGet:
              path: /
              port: 80
            initialDelaySeconds: 10
            periodSeconds: 10
            timeoutSeconds: 5
            failureThreshold: 3
          readinessProbe:
            httpGet:
              path: /
              port: 80
            initialDelaySeconds: 5
            periodSeconds: 5
            timeoutSeconds: 3
            failureThreshold: 3
---
apiVersion: v1
kind: Service
metadata:
  name: demo-nginx
  namespace: skill-demo
spec:
  selector:
    app: demo-nginx
  ports:
    - port: 80
      targetPort: 80
EOF

echo -e "  等待工作负载就绪 / Waiting for workloads to be ready..."
if ! kubectl rollout status deployment/demo-nginx -n skill-demo --timeout=120s; then
    echo -e "  ${YELLOW}⚠ Deployment 未在 120s 内就绪，请手动检查${NC}"
fi
echo ""

# ---- 完成 ----
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║      ✅ Skills Demo 集群就绪 / Cluster Ready!               ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  集群名称 / Cluster: ${CLUSTER_NAME}                              ║${NC}"
echo -e "${GREEN}║  Context: kind-${CLUSTER_NAME}                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  下一步 / Next steps:                                        ║${NC}"
echo -e "${GREEN}║    1. bash run-skill-demo.sh          # 交互式 demo          ║${NC}"
echo -e "${GREEN}║    2. bash scenarios/01-*.sh           # 运行单个场景         ║${NC}"
echo -e "${GREEN}║    3. bash teardown.sh                 # 清理集群             ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
