# Ingress 网关故障诊断

## 概述

Ingress Controller（Nginx Ingress、Higress）路由失败、TLS 终止异常、后端健康检查、注解配置错误的故障树诊断。

## 文件索引

| 文件 | 覆盖场景 |
|:---|:---|
| [ingress-fta.md](ingress-fta.md) | Ingress 通用 FTA 故障树（Controller/TLS/路由/后端/注解） |
| [nginx-ingress-fta.md](nginx-ingress-fta.md) | Nginx Ingress Controller 专项故障树 |
| [higress-fta.md](higress-fta.md) | Higress 云原生网关专项故障树 |

## 相关链接

- [[技能/故障诊断-网络/gateway-api/gateway-api-fta.md|Gateway API 故障树]]
- [[技能/故障诊断-网络/service/service-fta.md|Service 故障树]]
