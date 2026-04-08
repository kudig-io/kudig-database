# argo-cd v1.0 Release Notes

Source: [v1.0.2](https://github.com/argoproj/argo-cd/releases/tag/v1.0.2)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.0.2/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.0.2/manifests/ha/install.yaml
```

### Changes since v1.0.1

* Cluster registration was unintentionally persisting client-cert auth credentials (#1742)

## Checking if Argo CD is storing unneccesry credentials.

The following instructions explain if any action is needed to remove certificate-authentication data from Argo CD:

1. Run the following command and see if you see key data. If there is non-null output (e.g. output like the following), then Argo CD is storing unnecessary credentials and the cluster credentials should be re-added.

```shell
kubectl get secret cluster-192.168.64.66-892108214 -o json | jq -r '.data.config' | base64 --decode | jq -r '.tlsClientConfig.keyData'
LS0tLS1CRUdJTiBSU0EgUFJJVkFURSBLRVktLS0tLQpNSUlFcEFJQkFBS0NBUUVBeU54R1U1R2lPdEsydHpqWjVTMlVJUHJ6c0VyYktTdFlBakc2V3RqQUhjSHFQWmVzClkyTy96ekNNK3c5SFdSOEVCdkE3NjROdHdVVHFCVkFiQU05a3kvbEZLVjdmSUFkZVVDWmQ2QzZ6OXpDc242eTcKTnlBcWJETDR6b0xjZHRsUEJmL3JuSENKRXgwTEhiRWJscHNVSUpoNHZVWUNDSkRnbGh1NVNzM3ZGRmxNdkZBbQpZdis3QlZ3aE5YU2RwdEU1amg4WU1VTUJMTHFHQzAwUXpsKzVUZmdEVk9qd3U5Rzdub1pvanhZSlZScnJvaDQ2CjY4Z05hdVJkUzNaY2dxVkRUMWVrTmlsQjFKZW83Q0ZiaWdlRW5FUGVUenF5T3BjWG1lbkh5dDZ6L1BNZGN3LzgKbFpLWWkzaHpJcTBKY1lGWTBranBZME05bE95d2dveUd6YTcxRXdJREFRQUJBb0lCQVFDVHNwNG9EMXZxdzAxRwpONURLWEJTamw4VWZxanV6NzBKZEFySVU0WE9Mcmk4UHNYczY3bnQ1NENxYTVtWkJtM1A3b2lWOWpmeGo5TWZjCnRrWFU5NndYN1NrMVBhVDJ5VlJKdlp5cUFjV21DKzJ6MEhFdUhRSDA1QnBleUkxUys0S0hWK09wK25waFNxY0UKNDFuMUNmM241aFpLbjdNWkYyZCtHYzdMdWRpRzdjSG8walZmdXQ1eVNONitHeC91VTlraGV3ZERMQXRzOFVMeQp6NnVlYm9uTUhkQlVZNHNoNEtWZktGRmRrV2FRNlB5YU1DNktUOGNFbWJBY1RPT01yNVc5cjRXVHNxSXZ0ckNhClpzSWQ2YUhmUFRaWTZuMjdqNXNBd2tGZ0tSZXFmZ0NwZGdtbUM1bGxCRS9sTkhUbXFhNkRJc25MbXhzQUEvYjgKYWcvZVRaM2hBb0dCQU9NVGJaaDM5SlJtdkNTK2NWcnN2eHZ1V3FMNEdnbnRQUUhEUzhPZXBNcnR2V0dvV2hDSwpaWVRwU3dxZHh6dFArWU5iRFNzUCsyZ09TRnV2bEFSWFE3dzRqbWdNZHZVSDFqb0FSSDhuN3M0MFFKUkU1STJkCnROSU9pd0FLeTNpdDJxNy81UXlnemtQNVpTWktwSkhZWWk3QTRhaDFZZ3lQcmpmaXB0V3dYTnpqQW9HQkFPSnkKQVFvK3ZjQlJDZVV4T2lWajkzRFZJMkNvb1NhcmlBa1RQRnJhcTZXUm02S2FSTlpBYjZlWk0zc2JiMDNYSm9mbgp6ZC92UmJBWWYzeVV1K1BsL0s4Y0VnQUVYSHRxOVhBZ2NPQ0xYTzExRHlTV3RrMEowYnRYSlFrM01DQXBHVnc2CkFUbXpGMGNuYjBQdXZhZXNRTStUWVdwMzd2cU56S3hmdmRsUXRxNFJBb0dCQU5pNExoMGFQMTl6UFpXRC9RUGUKZC9iY1lieXdOWW5MMWpIY2htN0k5bGFHMS94Z2hMVE1vVjljbUxZbEo0VEFLMDdtazRiSjFoUFZyZEZ6blQwWApYQnBEa0FaVi95S1V2Q3pYSElpUFFDZWxUdzB6UXo2MWlXSUJaMEEvRFRxOEVyNTZrOHlkbkw3YlEySnNVdXl2CksrV2JTTU5TWktYQWEzSUM2MTkrMXVJcEFvR0FZSDlrb2hFS202SHRMWlpFeVJwSW4vUzBGc1RGcDgwQk01elcKNDRDOEZOcHdFR0xkWXRBaXhMRXNseEdoNVBJQ29YZk82OWJ6UTQrdEJGSDluNmlxZlpUZ3R0RWsrQk1rZEp2ZQpmbEhsVCt2S2dEVVppc3JjYlpFOVh5ZjlnamNCYjZQb1VjWlg3U0tJNzlJVlVCYS9wN1dPbGVoMkZwL0cwTTRjCkFUZThJWUVDZ1lCSWoweVZ3aGlmNm1FcjVrUmIveFJRb2V4aGdrSW1CME9LVnlFZUU3VTFBR1RjYXRQSWlSZFIKdWpJSFpOdzVNRWZ1b24vWFZkdXl1dytqakFVcUZ5c05aNTdQT2tXdjUweXMyTkVSM0RXUFJReS9MN0M2ZWNOcQpFNDhiWGt3V0ExVXR2QUlMckRDa0s5ZVd3cEpLNTdFQms4d2NvdkU4YlRRV0hUTEJndHEzWnc9PQotLS0tLUVORCBSU0EgUFJJVkFURSBLRVktLS0tLQo=
```

If the secret does not contain key data (i.e. output like the following), then no action is necessary:
```shell
$ kubectl get secret cluster-192.168.64.66-892108214 -o json | jq -r '.data.config' | base64 --decode | jq -r '.tlsClientConfig.keyData'
null
```

2. If key data is stored in the secret, run the following commands to re-add the cluster to Argo CD. **_IMPORTANT_**: ensure you are using argocd v1.0.2 CLI or greater.

```shell
$ argocd cluster rm CLUSTERURL
$ argocd cluster add CONTEXTNAME
```
