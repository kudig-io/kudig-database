# opa v0.33 Release Notes

Source: [v0.33.1](https://github.com/open-policy-agent/opa/releases/tag/v0.33.1)

This is a bugfix release addressing an issue in the formatting of rego code that contains
object literals. With the last release, those objects would under some conditions have their
keys re-ordered, with some of them put into a single line.

Thanks to @[iainmcgin](https://github.com/iainmcgin) for reporting.

### Fixes

- format: make groupIterable sort by row ([#3849](https://github.com/open-policy-agent/opa/issues/3849)) 
