# TUI 定制：快捷键、主题与界面

> **文档类型**: 定制指南 | **最后更新**: 2026-03 | **关键词**: OpenCode, TUI, Keybinds, Themes, Leader Key, Bubble Tea, tui.json, Custom Theme

---

## 概述

OpenCode 的 TUI 基于 Go Bubble Tea 框架构建，提供 Vim 风格的交互体验。本文覆盖 Leader Key 体系、完整快捷键配置、内置主题选择和自定义主题创建。TUI 配置独立于主配置文件，使用 `tui.json` 管理。

---

## 1. TUI 配置文件

TUI 配置使用独立的 `tui.json`（或 `tui.jsonc`）：

```json
{
  "$schema": "https://opencode.ai/tui.json",
  "theme": "tokyonight",
  "scroll_speed": 3,
  "scroll_acceleration": { "enabled": true },
  "diff_style": "auto"
}
```

**位置**：
- 全局：`~/.config/opencode/tui.json`
- 项目级：`./tui.json`（与 `opencode.json` 并列）
- 自定义：`OPENCODE_TUI_CONFIG` 环境变量

> 旧版 `opencode.json` 中的 `theme`、`keybinds`、`tui` 字段已废弃，会自动迁移。

---

## 2. Leader Key 体系

OpenCode 使用 **Leader Key** 机制避免与终端/tmux 快捷键冲突。

默认 Leader Key：**`Ctrl+X`**

操作流程：先按 Leader Key → 再按功能键。例如新建会话：`Ctrl+X` → `N`。

```json
{
  "$schema": "https://opencode.ai/tui.json",
  "keybinds": {
    "leader": "ctrl+x"
  }
}
```

> 不强制使用 Leader Key，可直接绑定组合键，但推荐使用以避免冲突。

---

## 3. 核心快捷键

### 3.1 应用全局

| 快捷键 | 操作 | 配置键 |
|--------|------|--------|
| `<Leader>q` / `Ctrl+C` / `Ctrl+D` | 退出应用 | `app_exit` |
| `<Leader>n` | 新建会话 | `session_new` |
| `<Leader>l` | 会话列表 | `session_list` |
| `<Leader>g` | 会话时间线 | `session_timeline` |
| `<Leader>m` | 模型列表 | `model_list` |
| `<Leader>a` | Agent 列表 | `agent_list` |
| `Tab` | 切换 Agent | `agent_cycle` |
| `Shift+Tab` | 反向切换 Agent | `agent_cycle_reverse` |
| `<Leader>e` | 打开外部编辑器 | `editor_open` |
| `<Leader>t` | 主题列表 | `theme_list` |
| `<Leader>b` | 侧边栏切换 | `sidebar_toggle` |
| `<Leader>s` | 状态视图 | `status_view` |
| `<Leader>u` | 撤销 | `messages_undo` |
| `<Leader>r` | 重做 | `messages_redo` |
| `<Leader>y` | 复制消息 | `messages_copy` |
| `<Leader>h` | 切换隐藏内容/提示 | `messages_toggle_conceal` / `tips_toggle` |
| `<Leader>c` | 手动 Compact | `session_compact` |
| `<Leader>x` | 导出会话 | `session_export` |
| `Escape` | 中断会话 | `session_interrupt` |
| `Ctrl+P` | 命令列表 | `command_list` |
| `F2` | 最近模型切换 | `model_cycle_recent` |
| `Ctrl+T` | 变体切换 | `variant_cycle` |

### 3.2 输入编辑

| 快捷键 | 操作 | 配置键 |
|--------|------|--------|
| `Return` | 发送消息 | `input_submit` |
| `Shift+Return` / `Ctrl+Return` / `Alt+Return` | 换行 | `input_newline` |
| `Ctrl+V` | 粘贴 | `input_paste` |
| `Ctrl+C` | 清除输入 | `input_clear` |
| `Ctrl+A` | 行首 | `input_line_home` |
| `Ctrl+E` | 行尾 | `input_line_end` |
| `Ctrl+K` | 删除到行尾 | `input_delete_to_line_end` |
| `Ctrl+U` | 删除到行首 | `input_delete_to_line_start` |
| `Ctrl+W` / `Ctrl+Backspace` | 删除前一个词 | `input_delete_word_backward` |
| `Alt+D` | 删除后一个词 | `input_delete_word_forward` |
| `Alt+F` / `Alt+Right` | 前移一个词 | `input_word_forward` |
| `Alt+B` / `Alt+Left` | 后移一个词 | `input_word_backward` |
| `Ctrl+D` / `Delete` | 删除光标下字符 | `input_delete` |
| `Ctrl+-` / `Super+Z` | 撤销编辑 | `input_undo` |
| `Ctrl+.` / `Super+Shift+Z` | 重做编辑 | `input_redo` |

### 3.3 消息浏览

| 快捷键 | 操作 |
|--------|------|
| `PageUp` / `Ctrl+Alt+B` | 上翻页 |
| `PageDown` / `Ctrl+Alt+F` | 下翻页 |
| `Ctrl+Alt+U` | 上半页 |
| `Ctrl+Alt+D` | 下半页 |
| `Ctrl+G` / `Home` | 跳到顶部 |
| `Ctrl+Alt+G` / `End` | 跳到底部 |

### 3.4 子会话导航

| 快捷键 | 操作 |
|--------|------|
| `<Leader>+Down` | 进入第一个子会话 |
| `Right` | 下一个子会话 |
| `Left` | 上一个子会话 |
| `Up` | 返回父会话 |

### 3.5 禁用快捷键

```json
{
  "$schema": "https://opencode.ai/tui.json",
  "keybinds": {
    "session_compact": "none",
    "scrollbar_toggle": "none",
    "username_toggle": "none"
  }
}
```

---

## 4. 内置主题

| 主题 | 说明 |
|------|------|
| `opencode` | 默认主题 |
| `system` | 自适应终端背景色，使用 ANSI 色 |
| `tokyonight` | Tokyo Night 风格 |
| `catppuccin` | Catppuccin 风格 |
| `catppuccin-macchiato` | Catppuccin Macchiato 变体 |
| `gruvbox` | Gruvbox 风格 |
| `nord` | Nord 风格 |
| `one-dark` | Atom One Dark |
| `kanagawa` | Kanagawa 风格 |
| `everforest` | Everforest 风格 |
| `ayu` | Ayu Dark |
| `matrix` | 黑客风格绿色主题 |

切换主题：`/theme` 命令或在 `tui.json` 中设置：

```json
{ "theme": "tokyonight" }
```

### 4.1 System 主题特性

`system` 主题自动适配终端色彩方案：
- 根据终端背景色生成灰阶，确保最佳对比度
- 使用 ANSI 标准色（0-15）进行语法高亮
- 文本和背景使用 `"none"` 保持终端默认外观

---

## 5. 自定义主题

### 5.1 主题文件位置（按优先级）

1. 项目级：`.opencode/themes/*.json`
2. 用户级：`~/.config/opencode/themes/*.json`
3. 内置主题

### 5.2 创建自定义主题

```bash
mkdir -p ~/.config/opencode/themes
```

`~/.config/opencode/themes/my-theme.json`：

```json
{
  "$schema": "https://opencode.ai/theme.json",
  "defs": {
    "nord0": "#2E3440",
    "nord4": "#D8DEE9",
    "nord8": "#88C0D0",
    "nord11": "#BF616A",
    "nord14": "#A3BE8C"
  },
  "theme": {
    "primary": { "dark": "nord8", "light": "#5E81AC" },
    "error": { "dark": "nord11", "light": "nord11" },
    "success": { "dark": "nord14", "light": "nord14" },
    "text": { "dark": "nord4", "light": "nord0" },
    "background": { "dark": "nord0", "light": "#ECEFF4" },
    "border": { "dark": "#434C5E", "light": "#4C566A" }
  }
}
```

### 5.3 颜色格式

| 格式 | 示例 | 说明 |
|------|------|------|
| Hex | `"#ffffff"` | 标准 6 位十六进制 |
| ANSI | `3` | 0-255 ANSI 色号 |
| 引用 | `"primary"` | 引用 `defs` 或主题中的颜色名 |
| Dark/Light | `{"dark": "#000", "light": "#fff"}` | 按终端模式自动选择 |
| None | `"none"` | 使用终端默认色/透明 |

---

## 6. Shift+Enter 终端配置

部分终端不默认发送 Shift+Enter 修饰键。需要手动配置：

### Windows Terminal

在 `settings.json` 的 `actions` 中添加：

```json
{
  "command": { "action": "sendInput", "input": "\u001b[13;2u" },
  "id": "User.sendInput.ShiftEnterCustom"
}
```

在 `keybindings` 中绑定：

```json
{ "keys": "shift+enter", "id": "User.sendInput.ShiftEnterCustom" }
```

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [02 - 安装部署](./02-opencode-installation-quickstart.md) | 终端要求与 Truecolor |
| [01 - 概述与架构](./01-opencode-overview-architecture.md) | TUI 在架构中的位置 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/keybinds、opencode.ai/docs/themes）整理。*
