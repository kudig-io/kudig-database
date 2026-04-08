# opa v0.23 Release Notes

Source: [v0.23.2](https://github.com/open-policy-agent/opa/releases/tag/v0.23.2)

This release contains a fix for a regression in v0.23.1 around bundle downloading. The bug caused OPA to cancel bundle downloads prematurely. Users affected by this issue would see the following error message in the OPA logs:

```
[ERROR] Bundle download failed: bundle read failed: archive read failed: context canceled
  plugin = "bundle"
  name = <bundle name>
```