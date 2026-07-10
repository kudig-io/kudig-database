---
title: oauth2-proxy 认证代理
description: oauth2-proxy 是一个反向代理，为后端应用提供 OAuth2/OIDC 认证层。常用于为没有内置认证功能的 Kubernetes
  Dashboard、...
summary: oauth2-proxy 是一个反向代理，为后端应用提供 OAuth2/OIDC 认证层。常用于为没有内置认证功能的 Kubernetes Dashboard、...
category: dictionary
tags:
- k8s
- glossary
- security
- authentication
- proxy
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- oauth2-proxy 认证代理 是什么
- oauth2-proxy 详解
trigger_keywords:
- oauth2-proxy 认证代理
- oauth2-proxy
- dictionary
prerequisites:
- kubernetes
---



# oauth2-proxy 认证代理（oauth2-proxy）

## 概述

oauth2-proxy 是一个反向代理，为后端应用提供 OAuth2/OIDC 认证层。常用于为没有内置认证功能的 Kubernetes Dashboard、Prometheus、Grafana 等服务添加登录保护。

## 核心概念/原理

- **认证代理**：在应用前端拦截请求，验证 OAuth2/OIDC Token
- **多 Provider**：支持 Google、GitHub、GitLab、OIDC、Azure AD 等
- **Kubernetes 友好**：以 Sidecar 或独立 Ingress 方式部署
- **Cookie 管理**：加密 Cookie 存储认证状态，支持刷新

## 关键机制或特性

- 基于 Cookie 的会话管理（支持 Redis 后端存储会话）
- 邮件域名白名单、邮箱验证等访问控制
- 配合 nginx-ingress 的 `auth-url` / `auth-signin` 注解使用
- 支持 htpasswd 文件作为后备认证
- 请求头注入用户信息（X-Auth-Request-User/Email）

## 使用场景与最佳实践

- 为 Prometheus/Grafana/K8s Dashboard 添加 SSO 登录
- 内部服务的统一认证网关
- 基于邮箱域名的简单访问控制
- 与 Dex 配合实现企业级 SSO

## 参考链接

- https://oauth2-proxy.github.io/oauth2-proxy/
- https://github.com/oauth2-proxy/oauth2-proxy

## Related

- [[domain-17-system-foundation/知识字典/security/dex.md|Dex]]
- [[domain-17-system-foundation/知识字典/networking/traefik.md|Traefik]]
- [[domain-17-system-foundation/知识字典/security/rbac.md|RBAC]]
