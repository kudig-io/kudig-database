# opa v0.21 Release Notes

Source: [v0.21.1](https://github.com/open-policy-agent/opa/releases/tag/v0.21.1)

This release fixes [#2497](https://github.com/open-policy-agent/opa/issues/2497) where the comprehension indexing optimization produced incorrect results for nested comprehensions that close over variables in the outer scope. This issue only affects policies containing nested comprehensions that are recognized by the indexer (which is a relatively small percentage).

This release also backports the GitHub Actions migration and a fix to the Wasm library build step.