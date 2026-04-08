# opa v0.37 Release Notes

Source: [v0.37.2](https://github.com/open-policy-agent/opa/releases/tag/v0.37.2)

This is a bugfix release addressing two bugs:

1. A regression introduced in the formatter fix for CVE-2022-23628.
2. Support indices for appending to an array, conforming to JSON Patch (RFC6902)
   for patch bundles.

### Miscellaneous

- format: generated vars may have a proper location
- storage: Support index for array appends