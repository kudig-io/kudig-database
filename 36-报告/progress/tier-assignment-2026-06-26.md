---
title: Tier 批量分配报告（2026-06-26）
description: 根据入链数和更新时间为核心页面自动分配 tier
category: reports
tags:
- wiki-lint
- tier
- maintenance
created: "2026-06-26"
updated: "2026-06-26"
---

# Tier 批量分配报告

- **扫描页面数**: 4988
- **core 分配**: 1102
- **supporting 分配**: 1372
- **peripheral 分配**: 2511
- **保持不变**: 1
- **权限错误**: 2

## 分配规则

- **core**: 入链 >= 5
- **peripheral**: 入链 <= 1 且 90+ 天未更新
- **supporting**: 其他情况

> ⚠️ 自动分配的 tier 建议人工 review，尤其是原本手动设置过 tier 的页面。