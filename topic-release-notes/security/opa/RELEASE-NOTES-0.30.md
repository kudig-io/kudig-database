# opa v0.30 Release Notes

Source: [v0.30.2](https://github.com/open-policy-agent/opa/releases/tag/v0.30.2)

This is a bugfix release that modifies the AWS credential provider to use POST
instead of GET for retrieving AWS STS tokens. The GET method can leak
credentials into the debug log if the AWS STS endpoint is unavailable.