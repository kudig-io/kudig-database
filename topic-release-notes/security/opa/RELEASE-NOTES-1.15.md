# opa v1.15 Release Notes

Source: [v1.15.1](https://github.com/open-policy-agent/opa/releases/tag/v1.15.1)

This patch release fixes a backwards-incompatible change in the `v1/logging.Logger` interface that inadvertently made it into Release v1.15.0.
When using OPA as Go module, and when providing custom `Logger` implementations, this change would break your build.

> [!TIP]
> Users of the binaries or Docker images can ignore this, the code is otherwise the same as v1.15.0.

### Miscellaneous

- logging: make WithContext() optional (authored by @srenatus)

