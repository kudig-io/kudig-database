---
title: opa v0.31 Release Notes
description: opa v0.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- wasm
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- opa v0.31 Release Notes 是什么
- 如何 opa v0.31 Release Notes
trigger_keywords:
- opa
- v0.31
- Release
- Notes
- release
- notes
---


# opa v0.31 Release Notes

Source: [v0.31.0](https://github.com/open-policy-agent/opa/releases/tag/v0.31.0)

This release contains **performance improvements** for evaluating partial sets and objects,
and introduces a new ABI call to OPA's Wasm modules to speed up Wasm evaluations.

It also comes with an improvement for checking policies -- unsafe declared variables are now caught at compile time.
This means that **some policies** that have been working fine with previous versions, because their unsafe variables
had not ever been queried, will fail to compile with OPA 0.31.0.
See below for details and what to do about that.

### Spotlights

#### Partial Sets and Objects Performance

Resolving an issue ([#822](https://github.com/open-policy-agent/opa/issues/822)) created on July 4th 2018,
OPA can now cache the results of partial sets and partial objects.

A benchmark that accesses a partial set of increasing size _twice_ shows a saving of more than 50%:

    name                             old time/op    new time/op    delta
    PartialRuleSetIteration/10-16       230µs ±10%     101µs ± 3%  -56.10%  (p=0.000 n=10+10)
    PartialRuleSetIteration/100-16     13.4ms ± 9%     5.5ms ± 9%  -58.74%  (p=0.000 n=10+9)
    PartialRuleSetIteration/1000-16     1.31s ±10%     0.51s ± 8%  -61.12%  (p=0.000 n=10+9)

    name                             old alloc/op   new alloc/op   delta
    PartialRuleSetIteration/10-16      77.7kB ± 0%    35.8kB ± 0%  -53.94%  (p=0.000 n=10+10)
    PartialRuleSetIteration/100-16     3.72MB ± 0%    1.29MB ± 0%  -65.26%  (p=0.000 n=10+10)
    PartialRuleSetIteration/1000-16     365MB ± 0%     114MB ± 0%  -68.86%  (p=0.000 n=10+10)

    name                             old allocs/op  new allocs/op  delta
    PartialRuleSetIteration/10-16       1.84k ± 0%     0.69k ± 0%  -62.42%  (p=0.000 n=10+10)
    PartialRuleSetIteration/100-16      99.3k ± 0%     14.5k ± 0%  -85.43%  (p=0.000 n=10+9)
    PartialRuleSetIteration/1000-16     10.0M ± 0%      1.0M ± 0%  -89.58%  (p=0.000 n=10+9)

These numbers were gathered querying `fixture[i]; fixture[j]` with a policy of

```rego
fixture[x] {
	x := numbers.range(1, n)[_]
}
```
where `n` is 10, 100, or 1000.

There are multiple access patterns that are accounted for: if a _ground_ scalar is used to
access a previously not-cached partial rule,

```rego
allow {
	managers[input.user] # here
}

managers[x] {
	# some logic here
}
```

the evaluation algorithm will calculate the set membership of `input.user` _only_, and cache the result.

If there is a query that requires evaluating the entire partial, however, the algorithm will also cache the entire partial:
```rego
allow {
	some person
	managers[person]
	# more expressions
}

managers[x] {
	# some logic here
}
```
thus avoiding extra evaluations later on.
The same is true if `managers` was used as a fully materialized set in an execution.


This also means that the question about whether to write

```rego
q = { x | ... } # set comprehension
```

or

```rego
q[x] { ... } # partial set rule
```

becomes much less important for policy evaluation performance.

#### WebAssembly Performance

OPA-generated Wasm modules have gotten a fast-path evaluation method:
By calling the one-off function

    opa_eval(reserved, entrypoint, data_addr, input_addr, input_len, format)

which returns a pointer to the serialized result set (in JSON if format is 0, "value" format if 1),
the number of VM calls needed for evaluating a policy via Wasm is drastically reduced.

The performance benefit is huge:

    name         old time/op    new time/op    delta
    WasmRego-16    84.3µs ± 6%    15.1µs ± 0%  -82.07%  (p=0.008 n=5+5)

The added `opa_eval` export comes with an ABI bump to version 1.2.
See [#3627](https://github.com/open-policy-agent/opa/pull/3627) for all details.

Along the same line, we've examined the processing of query evaluations that are Wasm-backed _through the `rego` package_.
This allowed us to avoid unneccessary work ([#3666](https://github.com/open-policy-agent/opa/issues/3666)).


#### Unsafe declared variables now cause a compile-time error

Before this release, local variables that had been _declared_, i.e. introduced via the `some` keyword, had been able
to slip through the safety checks unnoticed.

For example, a policy like

```rego
package demo

q { 
	input == "open sesame"
}

p[x] {
	some x
}
```

would have _not_ caused any error **if `data.demo.p` wasn't queried**.
Querying `data.demo.p` would return an "var requires evaluation" error.

With this release, the erroneous rule no longer goes unnoticed, but is **caught at compile time**: "var x is unsafe".

The most likely fix is to remove the rule with the unsafe variable, since it cannot have contributed to a successful
evaluation in previous OPA versions.

See [#3580](https://github.com/open-policy-agent/opa/issues/3580) for details.

### Topdown and Rego

- New built-in function: `crypto.x509.parse_and_verify_certificates` ([#3601](https://github.com/open-policy-agent/opa/issues/3601)), authored by @[jalseth](https://github.com/jalseth)

  This function enables you to verify that there is a chain from a leaf certificate back to the trusted root.
- New built-in function: `rand.intn` generates a random number between `0` and `n` ([#3615](https://github.com/open-policy-agent/opa/issues/3615)), authored by @[base698](https://github.com/base698)

  The function takes a string argument to ensure that the same call, within one policy evaluation, returns the same random number.
- `http.send` enhancement: New `caching_mode` parameter to configure if deserialized or serialized response bodies should be cached ([#3599](https://github.com/open-policy-agent/opa/issues/3599)) 
- Custom built-in function enhancement: let custom builtins halt evaluation ([#3534](https://github.com/open-policy-agent/opa/issues/3534))
- Partial evaluation: Fix stack overflow on certain expressions ([#3559](https://github.com/open-policy-agent/opa/issues/3559))

### Tooling

- Query Profiling: `opa eval --profile` now supports a `--count=#` flag to gather metrics and profiling data over multiple runs, and displays aggregate statistics for the results ([#3651](https://github.com/open-policy-agent/opa/issues/3651)).

  This allows you to gather more robust numbers to assess policy performance.

- Docker images: Publish static image ([#3633](https://github.com/open-policy-agent/opa/issues/3633))

  As of this release, you can use the staticly-built Linux binary from a docker image: `openpolicyagent/opa:0.31.0-static`.
  It contains the same binary that has been published since release v0.29.4, statically linked to musl, with evaluating Wasm disabled.

### Fixes

- Built-in `http.send`: ignore `tls_use_system_certs` setting on Windows. Having this set to _true_ (the default as of v0.29.0) would _always_ return an error on Windows.
- The console decision logger is no longer tied to the general log level ([#3654](https://github.com/open-policy-agent/opa/issues/3654))
- Update query compiler to reject empty queries ([#3625](https://github.com/open-policy-agent/opa/issues/3625))
- Partial Evaluation fix: Don't generate comprehension with unsafe variables ([#3557](https://github.com/open-policy-agent/opa/issues/3557))
- Parser: modules containing _only_ tabs and spaces no longer lead to a runtime panic.
- Wasm: ensure that the desired stack space for the C library calls (64KiB) is not reduced by data segments added in the compiler.
   This is achieved by putting the stack first -- stack overflows now become "out of bounds" memory access traps.
   Before, it would silently corrupt the static data.

### Server and Runtime

- New configuration for Management APIs: using `resource`, the request path for sending decision logs can be configured now ([#3618](https://github.com/open-policy-agent/opa/issues/3618)), authored by @[cbuto](https://github.com/cbuto)

  `/logs` is still the default, but can now be overridden.
  With this change, the `partition_name` config becomes deprecated, since its functionality is subsumed by this new configurable.

### Documentation

- How to debug? Clarify how to access `Note` events for debugging via explanations ([#3628](https://github.com/open-policy-agent/opa/issues/3628)) authored by @[enori](https://github.com/enori)
- Clarify special characters for key, i.e. what `x["y"]` is necessary because `x.y` isn't valid ([#3638](https://github.com/open-policy-agent/opa/issues/3638)) authored by @[Hongbo-Miao](https://github.com/Hongbo-Miao)
- Management APIs: Remove deprecated fields from docs
- Policy Reference: add missing backtick; `type_name` builtin is natively implemented in Wasm