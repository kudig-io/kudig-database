# opa v0.57 Release Notes

Source: [v0.57.1](https://github.com/open-policy-agent/opa/releases/tag/v0.57.1)

This is a bug fix release addressing the following security issues:

### Golang security fix GO-2023-2102

> A malicious HTTP/2 client which rapidly creates requests and immediately resets them can cause excessive server resource consumption.

### OpenTelemetry-Go Contrib security fix CVE-2023-45142

> Denial of service in otelhttp due to unbound cardinality metrics.