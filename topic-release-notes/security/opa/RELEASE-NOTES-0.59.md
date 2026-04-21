# opa v0.59 Release Notes

Source: [v0.59.0](https://github.com/open-policy-agent/opa/releases/tag/v0.59.0)

v0.59.0


This release adds tooling to help prepare existing policies for the upcoming OPA 1.0 release.
It also contains a mix of improvements, bugfixes and security fixes for third-party libraries.

> **_NOTES:_**
>
> * All published OPA images now run with a non-root uid/gid. The `uid:gid` is set to `1000:1000` for all images. As a result
    there is no longer a need for the `-rootless` image variant and hence it will not be published as part of future releases.
    This change is in line with container security best practices. OPA can still be run with root privileges by explicitly setting the user,
    either with the `--user` argument for `docker run`, or by specifying the `securityContext` in the Kubernetes Pod specification.

### Rego v1

The upcoming release of OPA 1.0, which will be released at a future date, will introduce breaking changes to the Rego language. Most notably:

* the keywords that currently must be imported through `import future.keywords` into a module before use will be part of the Rego language by default, without the need to first import them.
* the `if` keyword will be required before the body of a rule.
* the `contains` keyword will be required when declaring a multi-value rule (partial set rule).
* deprecated built-in functions will be removed.

This current release (`0.59.0`) introduces a new `--rego-v1` flag to the `opa fmt` and `opa check` commands to facilitate the transition of existing policies to be compatible with the 1.0 syntax.

When used with `opa fmt`, the `--rego-v1` flag will format the module(s) according to the new Rego syntax in OPA 1.0. 
Formatted modules are compatible with both the current version of OPA and 1.0. 
Modules using deprecated built-ins will terminate formatting with an error. Future versions of OPA will support rewriting applicable function calls with equivalent Rego compatible with 1.0. 

When used with `opa check`, the `--rego-v1` flag will check that the modules are compatible with both the current version of OPA and 1.0.

#### Relevant Changes

- cmd: Adding `--rego-v1` flag to `check` cmd ([#6429](https://github.com/open-policy-agent/opa/issues/6429)) authored by @johanfylling
- cmd & format: Adding rego-v1 mode to `opa fmt` ([#6297](https://github.com/open-policy-agent/opa/issues/6297)) authored by @johanfylling
- ast: Adding capability feature for the `rego.v1` import (#6375) (authored by @johanfylling)
- ast: Skip if keyword requirement for default rule (`rego.v1`) ([#6356](https://github.com/open-policy-agent/opa/pull/6356)) authored by @ashutosh-narkar
- rego.v1: Fixing erroneous missing value assignment error ([#6364](https://github.com/open-policy-agent/opa/issues/6364)) authored by @johanfylling
- rego.v1: Improving support for rules with chained bodies ([#6370](https://github.com/open-policy-agent/opa/issues/6370)) authored by @johanfylling
- ast: Add `rego.v1` import ([#6247](https://github.com/open-policy-agent/opa/issues/6247)) introduced in OPA 0.58.0, authored by @johanfylling

### Runtime, Tooling, SDK

- ast: Adding `rule_head_refs` capabilities feature flag ([#6334](https://github.com/open-policy-agent/opa/issues/6334)) authored by @johanfylling
- build: Remove rootless image variant ([#4295](https://github.com/open-policy-agent/opa/issues/4295)) authored by @ashutosh-narkar
- discovery: Make status updates non blocking (#6345) ([#6343](https://github.com/open-policy-agent/opa/issues/6343)) authored by @charlieegan3
- plugins/rest: Masks X-AMZ-SECURITY-TOKEN header in decision logs ([#5848](https://github.com/open-policy-agent/opa/issues/5848)) authored by @colinjlacy reported by @jwineinger
- wasm: Fix re2 bug ([#6376](https://github.com/open-policy-agent/opa/issues/6376)) authored by @srenatus reported by @sandhose
- ast: Add ExcludeLocationFile JSON marshalling option ([#6398](https://github.com/open-policy-agent/opa/pull/6398)) (authored by @anderseknert)
- cmd: Add options to the filter to only load rego files ([#6317](https://github.com/open-policy-agent/opa/issues/6317)) authored by @tjons
- ast: Add minimum compatible version computation to compiler ([#6348](https://github.com/open-policy-agent/opa/pull/6348)) authored by @tsandall 
- internal/planner: Insert general ref head objects starting from the leaves, not root. ([#6401](https://github.com/open-policy-agent/opa/pull/6401)) authored by @srenatus
- internal/planner: Don't plan superfluous Equal/NotEqualStmts ([#6386](https://github.com/open-policy-agent/opa/pull/6386)) authored by @srenatus

### Topdown and Rego

- ast: Allowing packages to be declared within the dynamic extent of a rule ([#6387](https://github.com/open-policy-agent/opa/issues/6387)) authored by @johanfylling
- ast: Disallow root document shadowing in leading term of rule refs ([#6291](https://github.com/open-policy-agent/opa/issues/6291)) authored by @johanfylling
- topdown: Add a new builtin function `strings.render_template` to render templated strings ([#6371](https://github.com/open-policy-agent/opa/issues/6371)) authored by @RDVasavada
- topdown/crypto: Add URIStrings field to JSON certs ([#6416](https://github.com/open-policy-agent/opa/issues/6416)) authored by @charlieegan3 reported by @kenjenkins
- ast: change ident token string ([#6435](https://github.com/open-policy-agent/opa/pull/6435)) authored by @tsandall

### Miscellaneous

- chore: Fix IDE warnings and remove usage of several deprecated fields. ([#6397](https://github.com/open-policy-agent/opa/pull/6397)) authored by @willbeason
- chore: Disable verbose output in wasm-sdk-e2e-test ([#6434](https://github.com/open-policy-agent/opa/pull/6434)) authored by @tsandall
- deps: group otel deps ([#6407](https://github.com/open-policy-agent/opa/pull/6407/files)) authored by @srenatus
- test: add environment variable tests ([#6420](https://github.com/open-policy-agent/opa/pull/6420)) authored by @robhafner
- Docs & Website:
  - docs: Add dependency-management-data to the Ecosystem ([#6436](https://github.com/open-policy-agent/opa/pull/6436)) authored by @jamietanna
  - docs: Add docs for dynamic_metadata feature in opa-envoy-plugin ([#6389](https://github.com/open-policy-agent/opa/pull/6389)) authored by @tjons
  - docs: Fixed XACML Policy in documentation (Comparing to Other Systems) to be XACML 3.0 compliant ([#6438](https://github.com/open-policy-agent/opa/pull/6438)) authored by @cdanger
  - docs: Update docs on rego.v1 / OPA 1.0 ([#6365](https://github.com/open-policy-agent/opa/pull/6365)) authored by @anderseknert
  - docs: Update spinnaker integration ([#6414](https://github.com/open-policy-agent/opa/pull/6414)) authored by @charlieegan3
  - docs: Add legitify to ecosystem ([#6369](https://github.com/open-policy-agent/opa/pull/6369)) authored by @charlieegan3
  - docs: add cheat sheet link ([#6362](https://github.com/open-policy-agent/opa/pull/6362)) authored by @charlieegan3
  - docs: add newstack blog to regal ([#6372](https://github.com/open-policy-agent/opa/pull/6372)) authored by @charlieegan3
  - docs: Disk storage broken link ([#6425](https://github.com/open-policy-agent/opa/pull/6425)) authored by @francoisauclair911
  - docs: Update istio envoy tutorial to use AuthorizationPolicy ([#6426](https://github.com/open-policy-agent/opa/pull/6426)) authored by @tjons
- Dependency updates; notably:
  - golang from 1.21.3 to 1.21.4
  - OpenTelemetry (contrib) 1.21.0/0.46.1

