# kops v1.9 Release Notes

Source: [1.9.2](https://github.com/kubernetes/kops/releases/tag/1.9.2)

Cherry-picks of important fixes:

* Add AuthenticationTokenWebhook flag #5231
* Don't repeatedly download nodeup #5462
* Introduce a global backoff to rate limit failed image downloads #5464
* Fix containerRegistry for Kubernetes < 1.10 #5353
* set GracePeriodSeconds to -1 #5143 
