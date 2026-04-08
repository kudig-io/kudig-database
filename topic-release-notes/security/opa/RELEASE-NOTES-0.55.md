# opa v0.55 Release Notes

Source: [v0.55.0](https://github.com/open-policy-agent/opa/releases/tag/v0.55.0)

> **_NOTES:_**
>
> * All published OPA images now run with a non-root uid/gid. The `uid:gid` is set to `1000:1000` for all images. As a result
> there is no longer a need for the `-rootless` image variant and hence it will be not be published as part of future releases.
> This change is in line with container security best practices. OPA can still be run with root privileges by explicitly setting the user,
> either with the `--user` argument for `docker run`, or by specifying the `securityContext` in the Kubernetes Pod specification.
>
> * The minimum version of Go required to build the OPA module is **1.19**

This release contains a mix of new features, bugfixes and a new builtin function.

### Honor `default` keyword on functions

Previously if a function was defined with a `default` value, OPA would ignore it. Now the `default` function is honored
if all functions with the same name are undefined. For example,

```rego
package example

default clamp_positive(_) := 0

clamp_positive(x) = x {
    x > 0
}
```

```
$ opa eval -d example.rego 'data.example.clamp_positive(1)' -f pretty
1
```

```
$ opa eval -d example.rego 'data.example.clamp_positive(-1)' -f pretty
0
```

The value of a `default` function follows the same conditions as that of a `default` rule. In addition, a `default`
function satisfies the following properties:

- same arity as other functions with the same name
- arguments should only be plain variables ie. no composite values
- argument names should not be repeated

> **_NOTE:_**  
> 
> `default` functions used to be previously ignored. If existing policies contain `default` functions, ensure that they conform
> to the properties mentioned above. Otherwise, those policies will fail to evaluate.

Authored by @ashutosh-narkar.

### New Built-In Function: crypto.parse_private_keys

`crypto.parse_private_keys` returns zero or more private keys from the given encoded string containing DER certificate data.
If the input contains a list of one or more concatenated PEM blocks, then the built-in will output the parsed private keys
represented as objects.

See [the documentation on the new built-in](https://www.openpolicyagent.org/docs/v0.55.0/policy-reference/#builtin-crypto-cryptoparse_private_keys)
for all the details.

Authored by @volck.

### Runtime, Tooling, SDK

- plugins/rest: Add AWS KMS support for OAuth2 Client Credentials JWT authentication ([#5942](https://github.com/open-policy-agent/opa/pull/5942)) authored by @prasanthu
- sdk: Update input object to conform to the format expected by decision log masking ([#6090](https://github.com/open-policy-agent/opa/pull/6090)) authored by @epaulson10
- sdk: Add option for specifying decision ID to SDK. Users can use this to control the ID that gets included in the decision logs ([#6101](https://github.com/open-policy-agent/opa/pull/6101)) authored by @brianchhun-chime
- cmd: Add `discard` output format to `opa eval` which discards the result while still showing the output of eval flags like `--profile` ([#6103](https://github.com/open-policy-agent/opa/pull/6103)) authored by @26tanishabanik
- Make rootless deprecation messages more explicit as all published OPA images now run with non-root uid/gid ([#6091](https://github.com/open-policy-agent/opa/pull/6091)) authored by @charlieegan3
- download/oci: Add support for Docker Registry v2 authentication scheme ([#6045](https://github.com/open-policy-agent/opa/pull/6045)) authored by @gitu and @DerGut
- plugins/discovery: Ensure discovery plugin doesn't erase its own config on the plugin manager ([#6070](https://github.com/open-policy-agent/opa/pull/6070)) authored by @blacksails

### Topdown and Rego

- ast: Add `WithRoots` compiler option that allows callers to set the roots to include in the output bundle manifest ([#6088](https://github.com/open-policy-agent/opa/pull/6088)) authored by @kubaj
- rego: Parse store modules iff modules set on the Rego object. This change assumes that while using the Rego package, the compiler and store are kept in-sync, and thereby attempts to avoid a race during the compilation process ([#6081](https://github.com/open-policy-agent/opa/pull/6081)) authored by @ashutosh-narkar

### Docs

- docs/envoy: Update the standalone Envoy tutorial to use [kind](https://kind.sigs.k8s.io/), updated Envoy version etc. ([#6105](https://github.com/open-policy-agent/opa/pull/6105)) authored by @charlieegan3

### Website + Ecosystem

- Ecosystem:
  - Carbonetes BrainIAC ([#6073](https://github.com/open-policy-agent/opa/pull/6073)) authored by @jaysonsantos05

- Website:
  - Reorganize relevant doc sections and OPA Ecosystem projects to have a closer integration between them ([#6064](https://github.com/open-policy-agent/opa/issues/6064)) authored by @charlieegan3

### Miscellaneous
- chore: Update comments on some exported functions and clean up instances where the same package was imported multiple times (authored by @testwill)
- Fix issue in the OPA release patch scripts related to `CRLF` line terminations in the patch output ([#6069](https://github.com/open-policy-agent/opa/pull/6069)) authored by @johanfylling
- Dependency bumps, notably:
  - golang from 1.20.5 to 1.20.6
  - oras.land/oras-go/v2 from 2.2.0 to 2.2.1
  - google.golang.org/grpc from 1.56.1 to 1.56.2
  - github.com/containerd/containerd from 1.6.19 to 1.7.2
  - golang.org/x/net from 0.11.0 to 0.12.0
  - go.uber.org/automaxprocs from 1.5.2 to 1.5.3
  - go.opentelemetry.io/otel from v1.14.0 to v1.16.0 ([#6062](https://github.com/open-policy-agent/opa/pull/6062)) authored by @srenatus with feedback from @ghaskins and @zregvart