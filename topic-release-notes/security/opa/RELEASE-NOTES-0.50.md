# opa v0.50 Release Notes

Source: [v0.50.2](https://github.com/open-policy-agent/opa/releases/tag/v0.50.2)

This is a bug fix release that addresses a regression in 0.50.1. 
This regression impacts policies with rules that, as its else-value, assign a comprehension containing variables.
Such rules would cause the compilation of the policy to fail with a `rego_unsafe_var_error` error.

E.g. the following policy would fail to compile with a `policy.rego:5: rego_unsafe_var_error: var x is unsafe` error:
```rego
package example

p {
	false
} else := [x | x := 1]
```

### Fixes

- ast: Fixing bug where comprehensions in rule else-heads weren't rewritten correctly ([#5771](https://github.com/open-policy-agent/opa/issues/5771)) authored by @johanfylling reported by @davidmdm