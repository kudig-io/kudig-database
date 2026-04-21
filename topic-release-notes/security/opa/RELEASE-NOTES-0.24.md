# opa v0.24 Release Notes

Source: [v0.24.0](https://github.com/open-policy-agent/opa/releases/tag/v0.24.0)

This release contains a number of small enhancements and bug fixes.

### Bundle Persistence

This release adds support for persisting bundles for recovery purposes. When persistence is enabled, OPA will save activated bundles to disk. On startup, OPA checks for persisted bundles and activates them immediately. This allows OPA to startup if the bundle server is unavailable ([#2097](https://github.com/open-policy-agent/opa/issues/2097)). For more information see the [Bundle](https://www.openpolicyagent.org/docs/latest/management/#bundles) documentation.

### Built-in Functions

This release includes a few new built-in functions:

- `base64.is_valid` for testing if strings are valid base64 encodings ([#2690](https://github.com/open-policy-agent/opa/issues/2690)) authored by @[carlpett](https://github.com/carlpett)
- `net.cidr_merge function` for merging sets of IPs and CIDRs ([#2692](https://github.com/open-policy-agent/opa/issues/2692))
- `urlquery.decode_object` for parsing URL query parameters into objects ([#2647](https://github.com/open-policy-agent/opa/issues/2647)) authored by @[GBrawl](https://github.com/GBrawl)

In addition, `http.send` has been enhanced to support caching overrides and in-band error handling ([#2666](https://github.com/open-policy-agent/opa/issues/2666) and [#2187](https://github.com/open-policy-agent/opa/issues/2187)).

### Fixes

- Fix `opa build` to support custom built-in functions ([#2738](https://github.com/open-policy-agent/opa/issues/2738)) authored by @[gshively11](https://github.com/gshively11)
- Fix for file watching volume mounted configmaps ([#2588](https://github.com/open-policy-agent/opa/issues/2588)) authored by @[drewwells](https://github.com/drewwells)
- Fix discovery plugin to set last request and last successful request timestamps in status updates ([#2630](https://github.com/open-policy-agent/opa/issues/2630))
- Fix planner crash on virtual document iteration ([#2601](https://github.com/open-policy-agent/opa/issues/2601))
- Fix decision logger to requeue failed chunks ([#2724](https://github.com/open-policy-agent/opa/pull/2724) authored by @[anderseknert](https://github.com/anderseknert))
- Fix object/set implementation in WASM-C library to avoid resizing.
- Fix JSON parser in WASM-C library to copy memory for strings and numbers.
- Improve WASM-C library to recycle object and set element structures while growing.

In addition, this release contains several fixes for panics identified by fuzzing:

- ast: Fix compiler to expand exprs in rule args ([#2649](https://github.com/open-policy-agent/opa/issues/2649))
- ast: Fix output var analysis to accept refs with non-var heads ([#2678](https://github.com/open-policy-agent/opa/issues/2678))
- ast: Fix panic during local var rewriting ([#2720](https://github.com/open-policy-agent/opa/issues/2720))
- ast: Fix panic in local var rewriting caused by object corruption ([#2661](https://github.com/open-policy-agent/opa/issues/2661))
- ast: Fix panic in parser post-processing of expressions ([#2714](https://github.com/open-policy-agent/opa/issues/2714))
- ast: Fix parser to ignore rules with args and key in head ([#2662](https://github.com/open-policy-agent/opa/issues/2662))
- ast: Fix object corruption during safety reordering
- types: Fix panic on reference to object with composite key ([#2648](https://github.com/open-policy-agent/opa/issues/2648))

### Backwards Compatibility

- Renamed `timer_rego_builtin_http.send_ns` to `timer_rego_builtin_http_send_ns` to avoid issues with periods in metric keys.
- Removed deprecated `watch` package ([#2265](https://github.com/open-policy-agent/opa/issues/2265))

### Miscellaneous

- Add support for H2C on HTTP listener ([#2739](https://github.com/open-policy-agent/opa/issues/2739) thanks @[srenatus](http://github.com/srenatus)!).
- Add Go version information to `opa version` output (thanks @[srenatus](http://github.com/srenatus)!)
- The official OPA build has been updated to Go v1.14.9. Previously it was using v1.13.7 which is no longer supported (thanks @[srenatus](http://github.com/srenatus)!)