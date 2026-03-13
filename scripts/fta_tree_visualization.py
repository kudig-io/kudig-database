"""
FTA Tree Visualization - Node NotReady & Pod NotReady
High-quality presentation-grade output using matplotlib + networkx
"""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib import rcParams
import numpy as np

# Use a CJK-capable font available on macOS
import matplotlib.font_manager as _fm
_cjk_font = None
for _f in _fm.fontManager.ttflist:
    if _f.name in ("Arial Unicode MS", "Heiti TC", "Hiragino Sans GB"):
        _cjk_font = _f.fname
        rcParams["font.family"] = "sans-serif"
        rcParams["font.sans-serif"] = [_f.name] + rcParams.get("font.sans-serif", [])
        break
rcParams["axes.unicode_minus"] = False

# ─────────────────────────────────────────────
# Color palette
# ─────────────────────────────────────────────
C = {
    "top":      "#C0392B",   # red   – top event
    "or_gate":  "#E67E22",   # orange – OR gate
    "and_gate": "#8E44AD",   # purple – AND gate
    "cat":      "#2980B9",   # blue  – category node
    "leaf":     "#27AE60",   # green – leaf / root cause
    "edge":     "#555555",
    "bg":       "#FAFAFA",
    "title_bg": "#2C3E50",
    "text_light":"#FFFFFF",
    "text_dark": "#1A1A2E",
}

# ─────────────────────────────────────────────
# Tree data definition
# Each node: (id, label, type, parent_id)
# types: top | or | and | cat | leaf
# ─────────────────────────────────────────────

NODE_TREE = [
    ("N_TOP",  "顶事件\nNode NotReady",          "top",  None),
    ("N_OR0",  "OR",                              "or",   "N_TOP"),

    # L2 categories
    ("N_NSTAT","节点状态异常",                    "cat",  "N_OR0"),
    ("N_KLET", "kubelet 异常",                    "cat",  "N_OR0"),
    ("N_RT",   "容器运行时异常",                  "cat",  "N_OR0"),
    ("N_RES",  "资源/容量异常",                   "cat",  "N_OR0"),
    ("N_NET",  "网络/连通性异常",                 "cat",  "N_OR0"),
    ("N_STO",  "本地存储/镜像异常",              "cat",  "N_OR0"),
    ("N_KERN", "内核/系统异常",                   "cat",  "N_OR0"),
    ("N_TIME", "时间/证书异常",                   "cat",  "N_OR0"),
    ("N_CP",   "控制面依赖异常",                  "cat",  "N_OR0"),

    # NSTAT
    ("N_NSOR", "OR",                              "or",   "N_NSTAT"),
    ("N_NS1",  "NotReady /\nUnknown",             "leaf", "N_NSOR"),
    ("N_NS2",  "节点频繁\n重启/不可达",           "leaf", "N_NSOR"),
    ("N_NS3",  "节点被 cordon\n/驱逐",            "leaf", "N_NSOR"),

    # KLET
    ("N_KTOR", "OR",                              "or",   "N_KLET"),
    ("N_KT1",  "kubelet 服务\n异常",              "leaf", "N_KTOR"),
    ("N_KT2",  "心跳上报\n失败",                  "leaf", "N_KTOR"),
    ("N_KT3",  "证书/鉴权\n失败",                 "leaf", "N_KTOR"),
    ("N_KT4",  "驱逐策略\n触发",                  "leaf", "N_KTOR"),
    ("N_KT5",  "PLEG\n不健康",                    "cat",  "N_KTOR"),
    # PLEG AND gate
    ("N_PAND", "AND\nPLEG 不健康触发",            "and",  "N_KT5"),
    ("N_PA1",  "PLEG relist\n超时",               "leaf", "N_PAND"),
    ("N_PA2",  "容器过多/\n运行时慢响应",          "leaf", "N_PAND"),

    # RT
    ("N_RTOR", "OR",                              "or",   "N_RT"),
    ("N_RT1",  "containerd /\ndockerd 异常",      "leaf", "N_RTOR"),
    ("N_RT2",  "CRI socket\n不可用",              "leaf", "N_RTOR"),
    ("N_RT3",  "镜像仓库/\n网络异常",             "leaf", "N_RTOR"),
    ("N_RT4",  "运行时\nhang/无响应",             "leaf", "N_RTOR"),

    # RES
    ("N_RESOR","OR",                              "or",   "N_RES"),
    ("N_RE1",  "内存压力",                        "cat",  "N_RESOR"),
    ("N_RE2",  "磁盘压力",                        "leaf", "N_RESOR"),
    ("N_RE3",  "CPU 过载",                        "leaf", "N_RESOR"),
    ("N_RE4",  "PID/句柄\n耗尽",                  "leaf", "N_RESOR"),
    # MEM AND
    ("N_MAND", "AND\n内存耗尽驱逐",               "and",  "N_RE1"),
    ("N_MA1",  "可用内存低于\n驱逐阈值",           "leaf", "N_MAND"),
    ("N_MA2",  "高密度 Pod\n无 limits",           "leaf", "N_MAND"),

    # NET
    ("N_NETOR","OR",                              "or",   "N_NET"),
    ("N_NE1",  "与 API Server\n不通",             "leaf", "N_NETOR"),
    ("N_NE2",  "CNI 组件\n异常",                  "leaf", "N_NETOR"),
    ("N_NE3",  "路由/iptables/\nipvs 异常",       "leaf", "N_NETOR"),
    ("N_NE4",  "DNS 依赖\n异常",                  "leaf", "N_NETOR"),

    # STO
    ("N_STOOR","OR",                              "or",   "N_STO"),
    ("N_ST1",  "镜像磁盘满/\nGC 失败",            "leaf", "N_STOOR"),
    ("N_ST2",  "本地卷损坏\n/只读",               "leaf", "N_STOOR"),
    ("N_ST3",  "挂载异常",                        "leaf", "N_STOOR"),

    # KERNEL
    ("N_KROR", "OR",                              "or",   "N_KERN"),
    ("N_KR1",  "内核 panic",                      "leaf", "N_KROR"),
    ("N_KR2",  "驱动/模块\n异常",                 "leaf", "N_KROR"),
    ("N_KR3",  "系统日志\n暴涨",                  "leaf", "N_KROR"),

    # TIME
    ("N_TIMOR","OR",                              "or",   "N_TIME"),
    ("N_TI1",  "节点证书\n过期",                  "leaf", "N_TIMOR"),
    ("N_TI2",  "时间同步失败\n→ TLS 失败",        "leaf", "N_TIMOR"),

    # CP
    ("N_CPOR", "OR",                              "or",   "N_CP"),
    ("N_CP1",  "API Server\n异常",                "leaf", "N_CPOR"),
    ("N_CP2",  "网络/安全策略\n阻断",             "leaf", "N_CPOR"),
]

POD_TREE = [
    ("P_TOP",  "顶事件\nPod NotReady",            "top",  None),
    ("P_OR0",  "OR",                              "or",   "P_TOP"),

    # L2
    ("P_SCH",  "调度失败\n/挂起",                 "cat",  "P_OR0"),
    ("P_IMG",  "镜像相关\n异常",                  "cat",  "P_OR0"),
    ("P_RT",   "运行时/启动\n异常",               "cat",  "P_OR0"),
    ("P_HC",   "健康检查\n失败",                  "cat",  "P_OR0"),
    ("P_NET",  "网络异常",                        "cat",  "P_OR0"),
    ("P_STO",  "存储异常",                        "cat",  "P_OR0"),
    ("P_RES",  "资源/配额\n异常",                 "cat",  "P_OR0"),
    ("P_SEC",  "安全/策略\n异常",                 "cat",  "P_OR0"),
    ("P_NODE", "节点/基础设施\n异常",             "cat",  "P_OR0"),
    ("P_CP",   "控制面/集群\n异常",               "cat",  "P_OR0"),
    ("P_CFG",  "配置/依赖\n异常",                 "cat",  "P_OR0"),

    # SCH
    ("P_SCHOR","OR",                              "or",   "P_SCH"),
    ("P_SC1",  "节点不可用\n/污点无法容忍",       "leaf", "P_SCHOR"),
    ("P_SC2",  "资源不足\n无法调度",              "leaf", "P_SCHOR"),
    ("P_SC3",  "亲和/反亲和\n冲突",               "leaf", "P_SCHOR"),
    ("P_SC4",  "调度器\n异常/不可达",             "leaf", "P_SCHOR"),
    ("P_SC5",  "配额/命名空间\n限制",             "leaf", "P_SCHOR"),

    # IMG
    ("P_IMGOR","OR",                              "or",   "P_IMG"),
    ("P_IM1",  "镜像不存在\n/标签错误",           "leaf", "P_IMGOR"),
    ("P_IM2",  "镜像仓库\n认证失败",              "leaf", "P_IMGOR"),
    ("P_IM3",  "镜像拉取\n网络失败",              "leaf", "P_IMGOR"),
    ("P_IM4",  "镜像格式/\n架构不匹配",           "leaf", "P_IMGOR"),

    # RT
    ("P_RTOR", "OR",                              "or",   "P_RT"),
    ("P_RT1",  "容器启动\n命令错误",              "leaf", "P_RTOR"),
    ("P_RT2",  "容器依赖/\n配置缺失",             "leaf", "P_RTOR"),
    ("P_RT3",  "CrashLoop\nBackOff",              "cat",  "P_RTOR"),
    ("P_RT4",  "OOMKilled",                       "cat",  "P_RTOR"),
    ("P_RT5",  "Init 容器\n失败",                 "leaf", "P_RTOR"),
    # CrashLoop AND
    ("P_CAND", "AND\nCrashLoop",                  "and",  "P_RT3"),
    ("P_CA1",  "容器进程\n异常退出",               "leaf", "P_CAND"),
    ("P_CA2",  "重启策略\nAlways/OnFailure",      "leaf", "P_CAND"),
    # OOM AND
    ("P_OAND", "AND\nOOM",                        "and",  "P_RT4"),
    ("P_OA1",  "内存上限\n过低",                  "leaf", "P_OAND"),
    ("P_OA2",  "内存峰值\n增长/泄漏",             "leaf", "P_OAND"),

    # HC
    ("P_HCOR", "OR",                              "or",   "P_HC"),
    ("P_HC1",  "探针配置\n错误",                  "leaf", "P_HCOR"),
    ("P_HC2",  "应用启动\n耗时过长",              "cat",  "P_HCOR"),
    ("P_HC3",  "依赖服务\n不可用",                "leaf", "P_HCOR"),
    ("P_HC4",  "探针端口/\n协议不一致",           "leaf", "P_HCOR"),
    # HC AND
    ("P_HAND", "AND\n启动超时",                   "and",  "P_HC2"),
    ("P_HA1",  "启动耗时\n过长",                  "leaf", "P_HAND"),
    ("P_HA2",  "启动探针/超时\n设置过短",          "leaf", "P_HAND"),

    # NET
    ("P_NETOR","OR",                              "or",   "P_NET"),
    ("P_NE1",  "DNS 解析\n失败",                  "leaf", "P_NETOR"),
    ("P_NE2",  "CNI 插件\n异常",                  "leaf", "P_NETOR"),
    ("P_NE3",  "网络策略\n阻断",                  "leaf", "P_NETOR"),
    ("P_NE4",  "Service/Endpoint\n配置错误",      "leaf", "P_NETOR"),

    # STO
    ("P_STOOR","OR",                              "or",   "P_STO"),
    ("P_ST1",  "PVC 未绑定\n/绑定失败",           "leaf", "P_STOOR"),
    ("P_ST2",  "存储类/CSI\n驱动异常",            "leaf", "P_STOOR"),
    ("P_ST3",  "挂载权限/\n路径错误",             "leaf", "P_STOOR"),
    ("P_ST4",  "卷只读/\n卷损坏",                 "leaf", "P_STOOR"),

    # RES
    ("P_RESOR","OR",                              "or",   "P_RES"),
    ("P_RE1",  "Requests/Limits\n配置不合理",     "leaf", "P_RESOR"),
    ("P_RE2",  "命名空间配额\n不足",              "leaf", "P_RESOR"),
    ("P_RE3",  "节点资源压力\n触发驱逐",          "cat",  "P_RESOR"),
    # Eviction AND
    ("P_EAND", "AND\n节点驱逐",                   "and",  "P_RE3"),
    ("P_EA1",  "节点资源\n压力",                  "leaf", "P_EAND"),
    ("P_EA2",  "Pod 优先级/\nQoS 低",             "leaf", "P_EAND"),

    # SEC
    ("P_SECOR","OR",                              "or",   "P_SEC"),
    ("P_SE1",  "RBAC 权限\n不足",                 "leaf", "P_SECOR"),
    ("P_SE2",  "Pod 安全策略\n阻断",              "leaf", "P_SECOR"),
    ("P_SE3",  "Seccomp/\nAppArmor 拦截",         "leaf", "P_SECOR"),
    ("P_SE4",  "准入 Webhook\n超时/失败",          "leaf", "P_SECOR"),

    # NODE
    ("P_NODOR","OR",                              "or",   "P_NODE"),
    ("P_NO1",  "节点\nNotReady",                  "leaf", "P_NODOR"),
    ("P_NO2",  "kubelet\n异常/驱逐",              "leaf", "P_NODOR"),
    ("P_NO3",  "容器运行时\n异常",                "leaf", "P_NODOR"),
    ("P_NO4",  "磁盘满/GC\n失败",                 "leaf", "P_NODOR"),

    # CP
    ("P_CPOR", "OR",                              "or",   "P_CP"),
    ("P_CP1",  "API Server\n不可用/超时",          "leaf", "P_CPOR"),
    ("P_CP2",  "调度器\n异常",                    "leaf", "P_CPOR"),
    ("P_CP3",  "etcd 异常",                       "leaf", "P_CPOR"),

    # CFG
    ("P_CFGOR","OR",                              "or",   "P_CFG"),
    ("P_CF1",  "ConfigMap\n缺失/未挂载",          "leaf", "P_CFGOR"),
    ("P_CF2",  "Secret 缺失\n/无权限",            "leaf", "P_CFGOR"),
    ("P_CF3",  "环境变量\n配置错误",              "leaf", "P_CFGOR"),
    ("P_CF4",  "ServiceAccount\n/Token 异常",     "leaf", "P_CFGOR"),
]


def build_tree(nodes_data):
    """Build parent→children mapping and node metadata."""
    meta = {}   # id → (label, type)
    children = {}  # id → [child_id, ...]
    root = None
    for nid, label, ntype, parent in nodes_data:
        meta[nid] = (label, ntype)
        children.setdefault(nid, [])
        if parent is None:
            root = nid
        else:
            children.setdefault(parent, [])
            children[parent].append(nid)
    return root, meta, children


def compute_positions(root, children, meta, x_start=0, y_start=0,
                       x_spacing=2.4, y_spacing=2.2):
    """Compute (x, y) positions using recursive subtree layout."""
    pos = {}
    subtree_width = {}

    def _width(nid):
        if not children[nid]:
            subtree_width[nid] = 1
            return 1
        w = sum(_width(c) for c in children[nid])
        subtree_width[nid] = max(w, 1)
        return subtree_width[nid]

    _width(root)

    def _place(nid, x, y):
        pos[nid] = (x, y)
        if not children[nid]:
            return
        total_w = sum(subtree_width[c] for c in children[nid])
        cx = x - (total_w - 1) * x_spacing / 2
        for c in children[nid]:
            child_cx = cx + (subtree_width[c] - 1) * x_spacing / 2
            _place(c, child_cx, y - y_spacing)
            cx += subtree_width[c] * x_spacing

    _place(root, x_start, y_start)
    return pos


def node_style(ntype):
    styles = {
        "top":  dict(boxstyle="round,pad=0.35", fc=C["top"],      ec="#922B21", lw=2.5),
        "or":   dict(boxstyle="round,pad=0.28", fc=C["or_gate"],  ec="#CA6F1E", lw=1.8),
        "and":  dict(boxstyle="round,pad=0.28", fc=C["and_gate"], ec="#6C3483", lw=1.8),
        "cat":  dict(boxstyle="round,pad=0.32", fc=C["cat"],      ec="#1A5276", lw=1.8),
        "leaf": dict(boxstyle="round,pad=0.28", fc=C["leaf"],     ec="#1E8449", lw=1.5),
    }
    return styles.get(ntype, styles["leaf"])


def draw_tree(ax, tree_data, title, x_spacing=1.6, y_spacing=1.6, font_size=6.0):
    root, meta, children = build_tree(tree_data)
    pos = compute_positions(root, children, meta,
                             x_start=0, y_start=0,
                             x_spacing=x_spacing, y_spacing=y_spacing)

    # Compute bounds and set axis limits with padding
    xs = [p[0] for p in pos.values()]
    ys = [p[1] for p in pos.values()]
    pad_x = x_spacing * 1.5
    pad_y = y_spacing * 1.2
    ax.set_xlim(min(xs) - pad_x, max(xs) + pad_x)
    ax.set_ylim(min(ys) - pad_y, max(ys) + pad_y + 1.5)  # +1.5 for title

    # Draw edges
    for nid, clist in children.items():
        if nid not in pos:
            continue
        px, py = pos[nid]
        for c in clist:
            cx, cy = pos[c]
            ax.annotate("",
                xy=(cx, cy + 0.30),
                xytext=(px, py - 0.30),
                arrowprops=dict(
                    arrowstyle="-|>",
                    color=C["edge"],
                    lw=0.8,
                    mutation_scale=8,
                    connectionstyle="arc3,rad=0.0"
                ))

    # Draw nodes
    for nid, (label, ntype) in meta.items():
        if nid not in pos:
            continue
        x, y = pos[nid]
        style = node_style(ntype)
        fsize = font_size + 1.0 if ntype == "top" else font_size
        fw = "bold" if ntype in ("top", "cat") else "normal"
        color = C["text_light"] if ntype in ("top", "and", "or") else C["text_dark"]
        ax.text(x, y, label,
                ha="center", va="center",
                fontsize=fsize, fontweight=fw, color=color,
                bbox=style, zorder=5,
                multialignment="center")

    # Title at top
    ax.text(0, max(ys) + 1.0, title,
            ha="center", va="center", fontsize=10, fontweight="bold",
            color=C["title_bg"],
            bbox=dict(boxstyle="round,pad=0.4", fc="#D5EAF7", ec=C["title_bg"], lw=2))

def make_legend(ax):
    handles = [
        mpatches.Patch(fc=C["top"],      ec="#922B21", lw=1.5, label="顶事件 (Top Event)"),
        mpatches.Patch(fc=C["or_gate"],  ec="#CA6F1E", lw=1.5, label="OR 门 – 任一子事件触发"),
        mpatches.Patch(fc=C["and_gate"], ec="#6C3483", lw=1.5, label="AND 门 – 全部子事件触发"),
        mpatches.Patch(fc=C["cat"],      ec="#1A5276", lw=1.5, label="分类节点 (Category)"),
        mpatches.Patch(fc=C["leaf"],     ec="#1E8449", lw=1.5, label="根因叶节点 (Root Cause)"),
    ]
    legend = ax.legend(handles=handles, loc="lower center",
                       ncol=5, fontsize=8.5,
                       frameon=True, framealpha=0.95,
                       edgecolor="#AAAAAA",
                       bbox_to_anchor=(0.5, 0.005))
    legend.get_frame().set_linewidth(1.2)


def main():
    fig = plt.figure(figsize=(38, 24), dpi=180, facecolor=C["bg"])

    # Two sub-axes side by side
    ax_node = fig.add_axes([0.01, 0.05, 0.47, 0.92], facecolor=C["bg"])
    ax_pod  = fig.add_axes([0.52, 0.05, 0.47, 0.92], facecolor=C["bg"])

    for ax in (ax_node, ax_pod):
        ax.axis("off")

    # ── Node NotReady Tree ──
    draw_tree(ax_node, NODE_TREE,
              "Node NotReady – FTA 故障树",
              x_spacing=1.4, y_spacing=1.5,
              font_size=7.0)

    # ── Pod NotReady Tree ──
    draw_tree(ax_pod, POD_TREE,
              "Pod NotReady – FTA 故障树",
              x_spacing=1.4, y_spacing=1.5,
              font_size=7.0)

    # ── Main title ──
    fig.text(0.5, 0.985,
             "Kubernetes FTA 故障树分析  ·  Node NotReady  &  Pod NotReady",
             ha="center", va="top",
             fontsize=20, fontweight="bold", color=C["title_bg"],
             fontfamily="sans-serif")
    fig.text(0.5, 0.972,
             "Fault Tree Analysis  |  OR Gate: any child triggers parent  |  AND Gate: all children required",
             ha="center", va="top",
             fontsize=10, color="#555555",
             fontfamily="sans-serif")

    # Legend
    make_legend(ax_node)

    out = "/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/assets/fta_node_pod_notready.png"
    plt.savefig(out, dpi=180, bbox_inches=None,
                facecolor=C["bg"], format="png")
    print(f"Saved → {out}")
    plt.close()


if __name__ == "__main__":
    main()
