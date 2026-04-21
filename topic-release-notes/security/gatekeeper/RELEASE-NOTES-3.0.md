# gatekeeper v3.0 Release Notes

Source: [v3.0.3](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.0.3)

This alpha release includes breaking changes and bug fixes.

## Breaking Changes ⚠️ 
* Rename deny rule to violation (#169)
* Change to HA-Compatible Status Schemas (#159)
* Fix CT name validation (https://github.com/open-policy-agent/frameworks/pull/27)
* Only require kind for Constraint Templates (https://github.com/open-policy-agent/frameworks/pull/29)
* Handle namespaceselector and empty namespaces (https://github.com/open-policy-agent/frameworks/pull/26)

## Bug Fixes 🐞
* Detect/handle invalid syntax in k8scontainerlimits (#167)
* Handle namespaceselector failure (#155)

Please report any issues here: https://github.com/open-policy-agent/gatekeeper/issues/new