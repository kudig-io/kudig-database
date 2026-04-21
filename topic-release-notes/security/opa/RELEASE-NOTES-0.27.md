# opa v0.27 Release Notes

Source: [v0.27.1](https://github.com/open-policy-agent/opa/releases/tag/v0.27.1)

This release contains a fix for crashes experienced when configuring OPA to use S3 signing as service credentials ([#3255](https://github.com/open-policy-agent/opa/issues/3255)).

In addition to that, we have a small number of enhancements and fixes:

### Tooling

- The `eval` subcommand now allows using `--import` without using `--package`. Authored by @[onelittlenightmusic](https://github.com/onelittlenightmusic), [#3240](https://github.com/open-policy-agent/opa/pull/3240).

### Compiler

- The `ast` package now exports another method for JSON conversion, `ast.JSONWithOpts`, that allows further options to be set ([#3244](https://github.com/open-policy-agent/opa/pull/3244).

### Server

- REST plugins using `s3_signing` as credentials method can now include the specified service in the signature (SigV4). Authored by @[cogwirrel](https://github.com/cogwirrel), [#3210](https://github.com/open-policy-agent/opa/pull/3210).

### Documentation

- Remove soon-to-be deprecated `any` and `all` from the [Policy Reference](https://www.openpolicyagent.org/docs/v0.27.1/policy-reference/#aggregates) ([#3241](https://github.com/open-policy-agent/opa/pull/3241)) -- see also [#2437](https://github.com/open-policy-agent/opa/issues/2437).
- Add missing `discovery.service` field to [Discovery configuration](https://www.openpolicyagent.org/docs/v0.27.1/configuration/#discovery) table ([#3237](https://github.com/open-policy-agent/opa/pull/3237)).
- Fix dead links to the Envoy pages ([#3248](https://github.com/open-policy-agent/opa/pull/3248)).

### WebAssembly

- Executions using the internal Wasm SDK will now be interrupted when the provided context is done (cancelled or deadline reached).
- The generated Wasm modules could become much smaller: unused functions are replaced by `unreachable` stubs, and the heavyweight runtime components related to regular expressions are excluded when none of the regex-related builtins are used: `glob.match`, `regex.is_valid`, `regex.match`, `regex.is_valid`, and `regex.find_all_string_submatch_n`.
- The Wasm runtime now allows passing in the time to be used for evaluation, enabling callers to control the time-of-day observed by Wasm compiled policies.
- Wasmtime runtime has been updated to the latest version (v0.24.0).