# opa v0.66 Release Notes

Source: [v0.66.0](https://github.com/open-policy-agent/opa/releases/tag/v0.66.0)

This release contains a mix of features, performance improvements, and bugfixes.

### Improved Test Reports ([#2546](https://github.com/open-policy-agent/opa/issues/2546))

The `opa test` command now includes a new `--var-values` flag that enriches reporting of failed tests with the values and locations for variables in the failing expression.
E.g.:

```
FAILURES
--------------------------------------------------------------------------------
data.test.test_my_policy: FAIL (0ms)

  test.rego:8:
    	x == y + z
    	|    |   |
    	|    |   3
    	|    y + z: 5
    	|    y: 2
    	1

SUMMARY
--------------------------------------------------------------------------------
test.rego:
data.test.test_foo: FAIL (0ms)
--------------------------------------------------------------------------------
FAIL: 1/1
```

Authored by @johanfylling, reported by @grosser.

### Reading stdin in `opa exec` ([#6538](https://github.com/open-policy-agent/opa/issues/6538))

The `opa exec` command now supports reading `input` documents from stdin with the `--stdin-input` (`-I`) flag.
E.g.:

```shell
$ echo '{"user": "alice"}' | opa exec --stdin-input --bundle my_bundle
```

Authored by @colinjlacy, reported by @humbertoc-silva.

### Topdown and Rego

- ast: Fix blanket "unexpected assign token" error message / usability issue ([#6563](https://github.com/open-policy-agent/opa/issues/6563)) authored by @anderseknert
- ast: Fix wrong location on metadata parse errors on first line ([#6587](https://github.com/open-policy-agent/opa/issues/6587)) authored by @anderseknert
- ast: Fix/inspect unknowns in with stmt ([#6812](https://github.com/open-policy-agent/opa/issues/6812)) authored by @johanfylling reported by @surajupadhyay01
- ast: Include original text in annotation location text attribute ([#6779](https://github.com/open-policy-agent/opa/issues/6779)) authored by @anderseknert
- ast: Expanding nested expressions in `every` domain ([#6790](https://github.com/open-policy-agent/opa/issues/6790)) authored by @johanfylling reported by @anakrish
- topdown: Add http.send request attribute to ignore headers for caching key ([#6642](https://github.com/open-policy-agent/opa/issues/6642)) authored and reported by @rudrakhp

### Runtime, Tooling, SDK

- build: Use chainguard images from dockerhub ([#6830](https://github.com/open-policy-agent/opa/pull/6830)) authored by @srenatus
- bundle: Preallocate buffers for file contents. ([#6818](https://github.com/open-policy-agent/opa/pull/6818)) authored by @philipaconrad
- plugins: Reduce locks during decision logging ([#6797](https://github.com/open-policy-agent/opa/pull/6797)) authored by @mjungsbluth
- plugins/rest: Do local map modification in OAuth2 client credentials flow ([#6769](https://github.com/open-policy-agent/opa/issues/6769)) authored and reported by @eubaranov
- loader: Use a better error message when trying to merge non-objects ([#6803](https://github.com/open-policy-agent/opa/issues/6803)) authored by @anderseknert
- server/authorizer: Fix gzip payload handling ([#6804](https://github.com/open-policy-agent/opa/issues/6804)) authored by @philipaconrad reported by @nevumx

### Docs, Website, Ecosystem

- docs: Remove missing prometheus metric `go_memstats_gc_cpu_fraction` ([#6783](https://github.com/open-policy-agent/opa/issues/6783)) authored by @philipaconrad
- docs: Mention that default functions may not evaluate ([#6265](https://github.com/open-policy-agent/opa/issues/6265)) authored by @anderseknert
- docs: Fix spelling and grammar of `an HTTP` ([#6786](https://github.com/open-policy-agent/opa/pull/6786)) authored by @jdbaldry
- docs/website: Add vs code and zed to ecosystem page ([#6788](https://github.com/open-policy-agent/opa/pull/6788)) authored by @charlieegan3
- docs/website: Add Flipt to the OPA ecosystem ([#6781](https://github.com/open-policy-agent/opa/pull/6781)) authored by @markphelps
- docs/website: Add Flipt blog to their ecosystem page ([#6789](https://github.com/open-policy-agent/opa/pull/6789)) authored by @charlieegan3
- docs/website: Revise language SDK content ([#6811](https://github.com/open-policy-agent/opa/pull/6811)) authored by @charlieegan3

### Miscellaneous

- Dependency updates; notably:
  - build(go): bump golang from 1.22.3 to 1.22.4
  - build(deps): bump github.com/containerd/containerd from 1.7.17 to 1.7.18
  - build(deps): bump golang.org/x/net from 0.25.0 to 0.26.0

