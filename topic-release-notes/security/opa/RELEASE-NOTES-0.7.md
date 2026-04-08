# opa v0.7 Release Notes

Source: [v0.7.1](https://github.com/open-policy-agent/opa/releases/tag/v0.7.1)

### Fixes

- Use rego.ParsedInput to provide input from form ([#571](https://github.com/open-policy-agent/opa/issues/571))

### Miscellaneous

- Add omitempty tag for ad-hoc query result field
- Fix rego package to check capture vars
- Fix root document assignment in REPL
- Update query compiler to deep copy parsed query