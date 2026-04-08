# opa v0.38 Release Notes

Source: [v0.38.1](https://github.com/open-policy-agent/opa/releases/tag/v0.38.1)

This is a bug fix release that addresses one issue when using `opa test` with the
`--bundle` (`-b`) flag, and a policy that uses the `every` keyword.

There are no other code changes in this release.

### Fixes

- Compiler: don't raise an error with unused declared+generated vars (every) ([#4420](https://github.com/open-policy-agent/opa/issues/4420)), reported by @kristiansvalland