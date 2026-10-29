# K8s 安全面试 · 快问快答沉淀

> 来源：`k8s-ops-quick-qa` 快问快答会话安全专项。格式同 `k8s-高级面试快问快答-沉淀.md` 答案卡（一句话 / 30 秒答 / 弹药 / 红线 / 下一问）。
> 主线从"一次请求的三道门"（Q1）顺链路走完：RBAC → SA 凭证 → Secret 落盘 → 逃逸 → 进程硬化 → 网络隔离 → 供应链 → 准入执行 → 审计追责 → 节点暴露面 → 多租户边界，收在文末「安全验收」深挖。
> 交叉引用约定：`高级沉淀 Qxx` 指 `k8s-高级面试快问快答-沉淀.md`（hostNetwork=Q44、PSS=Q45、Terway 策略=Q25）；apiserver 四道闸门加固速答另见 `k8s-基础加固面试问答-沉淀.md` 模块 1。
> 开始日期：2026-10-20（Q1–Q12 基础专项）；Q13–Q24 进阶专项 2026-10-20 追加。

---

## Q1. 一次 kubectl 请求要过几道安检？（安全/架构）

**一句话**｜三道门按序全过：认证（你是谁）→ 鉴权（你能干这个吗）→ 准入（这个写操作合不合规）——401/403 之分是第一判断线。

**30 秒答**｜①**认证 Authentication**：客户端证书、SA Token、OIDC 三选一，全部失败且开了匿名则按 `system:anonymous` 处理；过不了是 **401**。②**鉴权 Authorization**：默认 RBAC（生产 `Node,RBAC`），逐条规则比对，全不匹配**默认拒绝**；过不了是 **403**。③**准入 Admission**：只对**写请求**（create/update/patch/delete/connect）生效，读请求根本不走；顺序是内建插件（ServiceAccount、ResourceQuota、PodSecurity…）→ 内建 mutating → mutating webhook → schema 校验 → 内建 validating → validating webhook——前一步改过的对象后一步看到的是最终形态。判断线：401 查凭证（证书过期 → 高级沉淀 Q20、token 失效），403 查 RBAC；准入在鉴权**之后**——没权限的请求轮不到准入说话。

**弹药**
- `system:masters` 是超级用户组，**绕过全部鉴权**——谁在里面必须能说清
- `kubectl auth whoami`（1.28+）验当前身份；`--as=<user>` 模拟别人查权限（需 impersonate 权限）
- 准入拒绝原文直接在 kubectl 报错里：`admission webhook "xxx" denied the request`
- ValidatingAdmissionPolicy（CEL 表达式，1.30 GA）不用 webhook 就能写内建校验规则

**红线**｜❌ 把 401/403 说反；❌ 准入管 GET——读请求不走准入；❌ 顺序说成"先准入后鉴权"。

**下一问**｜鉴权的主流就是 RBAC——四个对象怎么搭？

---

## Q2. RBAC 四个对象怎么搭配？最小权限怎么落地？（安全/RBAC）

**一句话**｜Role/ClusterRole 是权限集（resources × verbs），Binding 把它发给主体（User/Group/ServiceAccount）——**RoleBinding 可以引用 ClusterRole**，这是"权限复用 + 范围限定"的标准姿势。

**30 秒答**｜关键交叉规则记两条：ClusterRoleBinding **不能**引用 Role；RoleBinding 引用 ClusterRole **合法且推荐**——内置 view/edit/admin 三个 ClusterRole 就是留给各命名空间 RoleBinding 复用的。verbs 常考：get/list/watch/create/update/patch/delete/deletecollection，特殊动词 bind/escalate（谁能发放/提升权限）、impersonate。防提权机制：**你不能 grant 自己没有的权限**（escalate 守门）。最小权限落地三件：CI/巡检给 view 或自定义只读；应用 SA 按需建 Role 绝不绑 cluster-admin；`resources: ["*"]` 通配符生产禁用。内置角色权限阶梯：cluster-admin > admin > edit > view。

**弹药**
- 现场验证手：`kubectl auth can-i delete pods -n prod --as=system:serviceaccount:ci:deployer`
- 反查大权限：`kubectl get clusterrolebindings -o wide` 看谁挂着 cluster-admin；`kubectl auth can-i --list --as=<u>` 列全量
- 内置 ClusterRole 被手改会被 controller-manager 自动还原（`rbac.authorization.kubernetes.io/autoupdate: "true"`）——要定制就复制一份新角色
- 聚合 ClusterRole：aggregationRule 按 label 自动收编子角色，权限组合不打架

**红线**｜❌ 给 CI / 应用 SA 绑 cluster-admin（出一次事全集群买单）；❌ 说 RoleBinding 不能引用 ClusterRole（恰恰是推荐用法）；❌ 通配符授权上生产。

**下一问**｜Binding 最常见的主体是 ServiceAccount——Pod 里的 token 从哪来、会过期吗？

---

## Q3. ServiceAccount 的 token 是永久的吗？1.24 之后变了什么？（安全/SA·Token）

**一句话**｜1.24 起 Pod 不再自动拿永久 Secret token——改为 projected 的 bound token：默认 1 小时过期、绑死 Pod 生命周期（Pod 删 token 失效）、kubelet 自动轮换。

**30 秒答**｜旧行为（≤1.23）：建 SA 自动生成永久 Secret token，泄漏即永久后门。新行为（1.24+，TokenRequest）：kubelet 为挂载的 Pod 申请 bound token，投到 `/var/run/secrets/kubernetes.io/serviceaccount/token`，三特征：①**默认 1h TTL**，kubelet 在超过 80% TTL 或超过 24h 时自动换新；②**绑 Pod**：Pod 删除 token 立即失效；③**受众受限**（aud=apiserver）。临时要 token：`kubectl create token <sa> --duration=10m`；真要长期凭证（老 SDK/CI）手动建 `kubernetes.io/service-account-token` 类型 Secret 并写 service-account 注解（不推荐，属技术债）。收敛攻击面：不访问 apiserver 的 Pod 一律 `automountServiceAccountToken: false`（Pod 级或 SA 级）——默认挂 token 就是白送攻击者一个集群内凭证。

**弹药**
- 容器内 `cat /var/run/secrets/kubernetes.io/serviceaccount/token` 是 JWT，解码看 exp/aud 和 `kubernetes.io.pod.name` 绑定声明
- 存量长期 token 盘点：`kubectl get secret -A | grep service-account-token`
- 配套认人：user 走 kubeconfig 证书（人的身份），workload 走 SA token（机器身份）——别说混
- TokenRequest 颁发的 token 不落 etcd（不是 Secret 对象），天然没有"token 当 Secret 存"的泄露面

**红线**｜❌ 说"SA token 永久有效"（1.24 前才是）；❌ 所有 Pod 默认挂着 SA token 还宣称安全；❌ 不知道 1.24 这个版本分界——这题一半考的就是版本意识。

**下一问**｜token 最常被读的目标就是 Secret——Secret 本身在 etcd 里是明文吗？

---

## Q4. Secret 在 etcd 里是明文吗？静态加密怎么开、存量怎么办？（安全/Secret）

**一句话**｜默认明文（base64 只是编码不是加密，高级沉淀 Q42 的坑）——开 EncryptionConfiguration 才真正落盘加密；**开了也不会自动加密存量数据**，必须全量重写一遍。

**30 秒答**｜机制：apiserver 挂 `--encryption-provider-config`，Secret 按 provider 列表加密后写 etcd；**列表第一个负责新写入**，读取按序尝试。选型：**KMS v2（1.29 GA，生产首选）**对接云 KMS/密管；本地加密选 secretbox（aescbc 无硬件加速偏慢，aesgcm 要频繁换 key 防 nonce 重用，都别首选）；**identity（明文）放列表末尾**做旧数据读回退。开启三步：①重启 apiserver 生效；②**重写存量**：`kubectl get secrets -A -o json | kubectl replace -f -`；③验证：`etcdctl get /registry/secrets/default/<x>` 出现 `k8s:enc:` 前缀才是真加密。key 轮换：新 key 插到列表第一位 → 全量重写 → 确认无解密报错 → 再删旧 key。

**弹药**
- etcd 证书 = 全集群 Secret 的万能钥匙，etcd 访问控制等级必须 ≥ apiserver（呼应基础加固 Q1）
- 集群内 Secret 还有两条泄露路：env 注入会进 `/proc/<pid>/environ`、镜像构建层缓存别塞 secret——最稳是 External Secrets Operator + 云密管，集群里只存"指针"
- audit 口径：secrets 只记 Metadata，别开 RequestResponse（明文进日志 = 白加密，Q10 呼应）

**红线**｜❌ base64 当加密；❌ 开了加密不重写存量就宣布完成；❌ audit 给 secrets 开 RequestResponse 级。

**下一问**｜凭证防住之后，最大的单点风险是容器逃逸——privileged 到底放开了什么？

---

## Q5. 容器逃逸的常见路径？privileged 的真实代价？（安全/逃逸）

**一句话**｜privileged ≈ 把隔离全关（全 capabilities + 全部宿主设备 + 不受 seccomp 限制），拿到它 ≈ 实质拿到宿主机 root；逃逸路线里八成的前提是"Pod 拿到了不该有的宿主机视角"。

**30 秒答**｜路线图：①**privileged + 挂宿主盘**：`fdisk -l` 找到宿主根盘 → `mount /dev/sda1 /host` → 直接改宿主 `/etc/cron*`、ssh authorized_keys——最经典演示；②**挂容器运行时 socket**（`/run/containerd/containerd.sock`）：用 ctr 起一个挂宿主根的 privileged 容器，等于把宿主机交出去；③**CAP_SYS_ADMIN + cgroup v1**：release_agent 逃逸（CVE-2022-0492 家族）；④**hostPID**：`/proc/<宿主进程>/root` 直接浏览宿主文件系统（接高级沉淀 Q44）；⑤**内核/runc 漏洞**：CVE-2019-5736（恶意镜像入口程序覆写 runc 自身）、DirtyPipe 类——这类不靠误配，靠补丁和运行时检测兜底。防线按性价比排：业务 Pod 禁 privileged（PSS baseline 即拦，高级沉淀 Q45）→ 禁 hostPath → drop capabilities + seccomp（下一题）→ 内核/runc 及时升级 → Falco 类运行时检测。

**弹药**
- 特权 Pod 一把梭审计：`kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[].securityContext.privileged==true) | .metadata.namespace'`
- socket 挂载同法查 `.spec.volumes[].hostPath.path` 含 `containerd.sock` / `docker.sock`
- 判断线：系统组件（CNI/CSI/监控 agent）有 privileged 通常合法；**业务命名空间出现 privileged 即事故**
- 讲逃逸先反问"你集群里谁能 privileged"——这句话本身就是加分动作

**红线**｜❌ "容器天然隔离所以安全"——隔离靠内核特性集，误配一开全失效；❌ 只答"privileged 就是 root"——要说全 capabilities/设备/seccomp 三层；❌ 背 CVE 编号说不出触发前提。

**下一问**｜不 privileged 的 Pod 怎么硬化？securityContext 那堆字段各管什么？

---

## Q6. securityContext 硬化清单：各字段管什么？（安全/硬化）

**一句话**｜六件套各挡一条路：runAsNonRoot 挡 root 身份、allowPrivilegeEscalation 挡 suid 提权、capabilities drop ALL 收敛默认权限、seccomp 挡危险 syscall、readOnlyRootFilesystem 挡落盘持久化、privileged 永远 false——PSS restricted 档就是这套的官方组合（高级沉淀 Q45）。

**30 秒答**｜①**runAsNonRoot: true**：kubelet 启动前校验 UID≠0，root 镜像直接拒启——配合镜像 USER 指令或 runAsUser: 1000；②**allowPrivilegeEscalation: false**：设 NO_NEW_PRIVS，容器内 suid/sgid 提不了权；③**capabilities**：容器默认自带 14 个 cap（含 **NET_RAW**——可伪造流量，高安全场景单独 drop），restricted 要求 `drop: [ALL]` 再按需 `add: [NET_BIND_SERVICE]`（绑 80/443）；④**seccompProfile: RuntimeDefault**：containerd 内置 syscall 白名单（挡 mount/kexec/ptrace 类），自定义走 Localhost + 节点 profile，Unconfined 等于裸奔；⑤**readOnlyRootFilesystem: true**：根盘只读，落马后写不了 webshell/cron，需要写的路径单独挂 emptyDir（/tmp）——**它防的是持久化，不是性能**；⑥**fsGroup**：卷挂载属组修正，非 root 容器挂 PVC 必配否则目录写不进。Pod 级 securityContext 是默认值、容器级可覆盖，审计两层都要看。

**弹药**
- 容器内验证三连：`id` / `capsh --print` / `grep Cap /proc/self/status`
- 默认 14 cap：CHOWN、DAC_OVERRIDE、FOWNER、FSETID、KILL、MKNOD、NET_BIND_SERVICE、NET_RAW、SETFCAP、SETGID、SETPCAP、SETUID、SYS_CHROOT、AUDIT_WRITE
- 一个能过 PSS restricted 的业务容器 = 上面四件套 + readOnlyRootFilesystem + emptyDir /tmp

**红线**｜❌ 只配 runAsNonRoot 就宣称"已硬化"（cap/seccomp 全裸）；❌ readOnlyRootFilesystem 后应用写不了 /tmp 就关掉它（挂 emptyDir 才是解）；❌ 不知道 NET_RAW 默认在清单里。

**下一问**｜进程层硬化完，网络层第一刀：default-deny 怎么配？

---

## Q7. NetworkPolicy 怎么做 default-deny？有哪些坑？（安全/NetworkPolicy）

**一句话**｜NetworkPolicy 是白名单加法、没有 deny 规则——`podSelector: {}` 全选 + 空规则 = 全拒；前提 CNI 必须实现（Flannel 不支持且**静默不报错**）。

**30 秒答**｜deny-all 模板（命名空间一把梭）：
```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata: { name: default-deny-all, namespace: app }
spec:
  podSelector: {}
  policyTypes: [ Ingress, Egress ]
```
然后按需白名单放行。五坑：①**静默失效**——CNI 不支持时策略被忽略且无任何报错（Flannel），上线必做双 Pod 探针验证；②**egress 忘放 DNS**——deny-all 后第一个故障就是解析失败，放行 kube-dns 53/UDP+TCP；③peer 语义——podSelector 和 namespaceSelector 写在**同一个 peer 里是 AND，拆成两项是 OR**（面试经典陷阱）；④**hostNetwork Pod 绕过**——策略管 Pod 流量，节点网络流量不受管（接高级沉淀 Q44）；⑤只有 L3/L4，HTTP 路由级策略要 Cilium（L7）。ipBlock 只该用来匹配集群外 IP，别拿它匹配 Pod。

**弹药**
- 验证探针：两个 netshoot Pod 互 curl，通/不通各验一条；先 `kubectl get netpol -A` 看部署面
- AND/OR 对照：`from: [{namespaceSelector: A, podSelector: B}]`（AND）vs `from: [{namespaceSelector: A}, {podSelector: B}]`（OR）
- Terway 的实现方式接高级沉淀 Q25（ENIIP→iptables、IPvlan→eBPF）
- 巡检项：哪些命名空间还没有 default-deny——脚本对 `kubectl get ns` 求差集

**红线**｜❌ 部署 Flannel 还宣称有 NetworkPolicy 隔离（静默不生效，最爱被钓）；❌ deny-all 不放 DNS（上线即故障）；❌ AND/OR 语义说反。

**下一问**｜Pod 内外都硬化了，还有一条敌人必经之路：镜像供应链。

---

## Q8. 镜像供应链安全：扫描、签名、digest 固定怎么做？（安全/供应链）

**一句话**｜三道闸：CI 扫描（Trivy 拦高危 CVE）→ 准入验签（cosign + Kyverno：无签名拒绝）→ 交付固定 digest（`:latest` 是可变指针，digest 才不可变）。

**30 秒答**｜①**扫描**：Trivy/Grype 进 CI 卡点（CRITICAL 拒绝）+ registry 周期重扫——CVE 库天天更新，已上线镜像会"变坏"；②**签名验签**：cosign 签名（keyless 或 KMS key），Kyverno `verifyImages` 在准入层验，无签名直接拒——防有人往 registry 推同名 tag 投毒；③**digest 固定**：`image: repo/app@sha256:...` 不可变；:latest 的双重罪——不可复现 + 投毒面；④**基础镜像保鲜**：base 镜像出了 CVE 补丁，你构建的镜像**不会自动获得**——要定期重建 + 自动升级 PR（renovate 类）；⑤SBOM（syft 生成）是加分项：漏洞告警才能定位"哪个组件受影响"。配套：私有 registry + imagePullSecrets，registry 匿名拉取 = 供应链裸奔。

**弹药**
- `trivy image --severity CRITICAL,HIGH --exit-code 1 <img>`（CI 卡点）
- `cosign verify --key cosign.pub repo/app:tag`；Kyverno verifyImages + "拒 :latest/未固定 digest"策略模板
- 判断线：扫描只做在 CI 不做在准入 = 任何能推 registry 的人可绕过全部防线

**红线**｜❌ :latest 上生产；❌ "扫过 = 安全"——扫描是快照，CVE 库每天在变；❌ 答不出"为什么基础镜像升级要重新构建我的镜像"。

**下一问**｜供应链策略要在集群内强制执行，靠准入 webhook——顺序和 failurePolicy 怎么选？

---

## Q9. 准入 webhook：调用顺序、failurePolicy、自拦截死锁？（安全/准入）

**一句话**｜写请求专属链条：内建 mutating → mutating webhook → schema 校验 → 内建 validating → validating webhook；安全类策略 failurePolicy 必须 Fail（挂了宁可拒绝写）——但 webhook 故障即全集群写瘫痪，所以要 namespace 隔离、且**别拦自己**。

**30 秒答**｜①**顺序**：准入只碰写请求；mutating 全部跑完才进 validating——validating 看到的是最终形态；同类型 webhook 串行调用、顺序按配置名与 webhook 名排序，业务上别依赖精确顺序；mutating 有 reinvocationPolicy（Never/IfNeeded）应对后续 webhook 又改对象。②**failurePolicy**：API 默认值是 **Ignore**，但验签/PSS 这类安全策略必须显式 **Fail**——Ignore 等于"搞挂你的 webhook 就能绕过策略"；Fail 的代价：Istio/Kyverno 的 webhook 挂了，全集群 kubectl apply 全挂——所以必须 namespaceSelector/objectSelector 排除 kube-system，且 webhook 自己所在命名空间**必须排除自己**：否则 webhook 宕 → 它的 Pod 重建 → 被自己的 Fail 策略拦 → 谁都救不了它，这就是**自拦截死锁**。③**超时**：timeoutSeconds 默认 10s，webhook 慢 = 全集群写请求慢（排查接高级沉淀 Q11）。

**弹药**
- 准入面盘点：`kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations`——逐个核 failurePolicy 与范围选择器
- 死锁自检：webhook 的 namespaceSelector 是否排除了自己部署的 ns（Kyverno 官方模板自带）
- 监控：`apiserver_admission_webhook_rejections_total`、`apiserver_admission_webhook_admission_duration_seconds`
- `kubectl apply --dry-run=server` 会真跑完整准入链（要求策略声明 sideEffects: None）

**红线**｜❌ 安全 webhook 用 Ignore（等于没装还给你安全感）；❌ webhook 全量拦截不排除 kube-system（控制面自杀装置）；❌ 不知道 validating 在 mutating 之后。

**下一问**｜策略在跑、集群在被加固，出了事怎么回答"是谁干的"？——审计。

---

## Q10. 审计日志：怎么回答"谁删了那个 Pod"？（安全/审计）

**一句话**｜audit log 记录每个到达 apiserver 的请求（谁、从哪、干了什么、结果），默认**不开**；查事故先查 audit——`kubectl get events` 只有 1 小时（--event-ttl），不算证据。

**30 秒答**｜配置三件：`--audit-policy-file` 分级策略 + 后端（log 滚动文件 / webhook 送 SIEM——**必须送出集群独立存储**，攻击者进集群第一件事就是抹日志）。level 四档 None < Metadata < Request < RequestResponse；生产口径：读类 None/Metadata、写类 Metadata、secrets 只到 Metadata（Q4 红线）、exec 与 RBAC 变更到 Request。stage：RequestReceived / ResponseComplete（watch 另有 ResponseStarted）。查"谁删了 Pod"：过滤 `objectRef.resource=pods + verb=delete + objectRef.name`，看 user.username、sourceIPs、userAgent、annotations 里的授权决策——一条链就是完整事故现场。边界：audit 只覆盖 apiserver 视角，容器里干了什么是 Falco/runtime 的事（文末深挖）。

**弹药**
- policy 分层逻辑（首条命中生效）：高频系统只读 None → secrets 用 Metadata → 写操作 Metadata → RBAC/exec 用 Request → 兜底 Metadata
- 现场过滤：`jq 'select(.verb=="delete" and .objectRef.resource=="pods") | {user:.user.username, ip:.sourceIPs, name:.objectRef.name}'`
- log 后端：--audit-log-maxage / maxbackup / maxsize；webhook 后端：--audit-webhook-config-file
- 双源分工：events（1h 窗口，排障）vs audit（持久，追责）

**红线**｜❌ "有 events 所以能查"——1 小时就没了；❌ 全量 RequestResponse（日志爆炸 + secret 明文入库）；❌ 日志只存在集群节点本地（一起被抹）。

**下一问**｜控制面看完看节点面——kubelet 的 10250 敞着会出什么事？

---

## Q11. kubelet 10250 / 10255 / 10248 三个端口的安全边界？（安全/节点面）

**一句话**｜10250 带鉴权但内含 exec/run 等 RCE 级接口、10255 是无鉴权只读口（必须关）、10248 只绑 localhost——节点面底线：10250 只许控制面网络可达，"匿名 + AlwaysAllow"组合 = 全节点 RCE。

**30 秒答**｜①**10250 kubelet API**：认证走客户端证书 / SA token，鉴权应配 Webhook（转发 apiserver 按 Node/RBAC 判）——但 kubelet 自身默认值是 `--authorization-mode=AlwaysAllow` + `--anonymous-auth=true`，自建集群没收敛就是"摸到 10250 的人 = root"（exec 进容器、读全量 Pod 清单、run 接口）；kubeadm 新版已默认收紧（anonymous=false + Webhook），存量集群 kube-bench 必查。②**10255 只读口**：无鉴权吐 /pods（含镜像、env）、/metrics——信息侦察起点，`--read-only-port=0` 关掉。③**10248**：healthz 只绑 127.0.0.1，无风险但要能说清分工（高级沉淀 Q30 已背）。真正护栏在网络层：安全组只放行控制面与堡垒机访问 10250，业务网段全拒——这层做对了，上层配置烂一点也有缓冲。

**弹药**
- 探测：`curl -k https://<node>:10250/pods` 无凭证应 401/403；返回 JSON 即事故
- `ss -tlnp | grep -E '10248|10250|10255'` 核对监听面
- kubelet config 三项：authentication.anonymous.enabled=false / authorization.mode=Webhook / readOnlyPort=0
- 相邻 CIS 项顺手报：--protect-kernel-defaults、--streaming-connection-idle-timeout、--event-qps 限速

**红线**｜❌ 10250 对业务网段开放（单机失陷 → 横向全节点）；❌ 10255 开着说"只读无所谓"（侦察数据全送）；❌ "反正有 TLS"——TLS 管加密不管鉴权，匿名 + AlwaysAllow 带着加密照样 RCE。

**下一问**｜节点面上最后一个边界问题：多租户到底能不能共用一个集群？

---

## Q12. namespace 是安全边界吗？多租户隔离怎么分？（安全/多租户）

**一句话**｜namespace 是**软隔离**（管理面分权），不是安全边界——同集群租户共享节点内核、etcd 和 registry；不同信任域要么独立集群（或 vCluster），namespace + 全套策略只配给同一信任域的弱隔离。

**30 秒答**｜namespace 能给的：RBAC 按 ns 切分、ResourceQuota/LimitRange 控资源、NetworkPolicy 控互访、PSA label 挡高危 Pod。给不了的：①**内核共享**——任何租户容器逃逸（Q5）即跨租户，hostPath/privileged 有 PSA enforce 兜底但内核 0day 无解；②**节点共享**——默认任何租户的 Pod 可调度到任意节点（要用 taint/toleration 做专属节点池）；③**控制面共享**——能拿到高权限的租户管理员看见全集群。分级方案：同信任域不同团队 → namespace + 五件套；不同信任域（外部租户/合规隔离）→ 独立集群或 vCluster 虚拟控制面。判断线：被问"租户 A 的开发能不能搞挂租户 B"——答"namespace 只防误、不防恶，恶意隔离上集群边界"，这句就是分层意识。

**弹药**
- 租户 ns 标准包五件套：ResourceQuota + LimitRange + default-deny NetworkPolicy（Q7）+ PSA enforce（高级沉淀 Q45）+ 专属节点池（taint + toleration）
- 共享节点用 taint `dedicated=tenant-a:NoSchedule` 拦调度——**label 不拦调度，taint 才拦**
- vCluster/Kamaji：每租户独立轻量 apiserver，宿集群只承载虚拟控制面——SaaS 多租户折中解
- 巡检：`kubectl get resourcequota,limitrange,networkpolicy -n <tenant>` 四件在不在

**红线**｜❌ 把 namespace 当安全边界承诺给安全/合规团队；❌ 多租户共享节点不打 taint（调度一视同仁全混跑）；❌ "网络策略做了就隔离了"——内核逃逸不走路由。

---

## 深挖：安全体系怎么"验收"——四层分工、kube-bench 与运行时检测

**一句话**｜准入管"带病进场"，运行时检测（Falco/Tetragon）管"进场后干坏事"，kube-bench（CIS 基线）管"底座配置合不合规"，audit（Q10）管"事后谁干的"——四层都有答案才叫可验收。

**四层分工表**

| 层 | 手段 | 时机 | 回答的问题 |
|----|------|------|-----------|
| 预防 | RBAC / PSA / NetworkPolicy / 镜像验签（Q2/高级沉淀 Q45/Q7/Q8） | 部署与访问时 | 坏东西能不能进来 |
| 检测 | Falco / Tetragon（eBPF） | 运行中 | 进来的在不在干坏事 |
| 基线 | kube-bench（CIS Benchmark） | 周期巡检 | 底座配置合不合规 |
| 追溯 | audit log（Q10） | 事后 | 谁干的、什么时候 |

**运行时检测典型规则**（说得出三条就是实践过）：容器内起交互 shell、写 /etc/、读 /etc/shadow、非预期出网、setuid 二进制落盘、加载内核模块。Falco：内核事件 + 规则引擎，产出告警；Tetragon（Cilium 系）：eBPF 按 Pod/cgroup 过滤，支持**同步阻断**（enforce），不只告警。

**kube-bench**：CIS Kubernetes Benchmark 自动化扫描，四大块 Control Plane / Etcd / Node / Policies；托管集群（ACK/EKS）部分项由云厂商负责、标 N/A——**共享责任模型**要能说清（apiserver 项对应基础加固 Q1）。扫出 Fail 排优先级：先网络暴露面（10255/匿名）→ 鉴权配置 → 审计开启 → 性能相关项；每项标注"已修/豁免原因/责任人"。

**验收清单（安全评审标准问答顺序）**：RBAC 最小权限 + cluster-admin 归属审计 → PSA enforce 档位 → default-deny 覆盖率 → Secret 静态加密（KMS）→ audit 送出集群 → 镜像准入验签 → kubelet 三口收敛 → kube-bench 周期跑 → Falco 告警接值班。

**面试答法（45 秒版）**｜"我们把安全分四层验收：预防层 RBAC + PSA + NetworkPolicy + 镜像验签，在部署和访问时拦；检测层 Falco/Tetragon 抓运行中异常，比如容器里起 shell、读敏感文件；底座用 kube-bench 对 CIS 基线定期巡检，托管项按云厂商共享责任标 N/A；所有 apiserver 请求进 audit log 送出集群独立存储。四层分别回答——能不能进、在不在干坏事、底座合不合规、谁干的。"

**追问预判**
- Falco 和 NetworkPolicy 什么关系？——互补不替代：netpol 是静态白名单（挡预期外访问），Falco 是动态检测（发现白名单内的恶意行为）；一个在转发层、一个在内核事件层。
- 运行时检测误报怎么治？——按工作负载画像收敛规则（白名单进程/出网目标），规则跟着基线镜像走，不全局一把梭。
- CIS 一堆 Fail 先修哪个？——暴露面 > 鉴权 > 审计 > 性能类；修不动的要求有豁免记录，不允许"知道但不管"。

**下一批预判**｜安全专项的延伸：容器取证细节、Falco 规则编写、合规映射（等保/PCI→K8s 控制项）、供应链 SLSA 等级。

---

> **Q13–Q24：安全进阶专项**（2026-10-20 追加）。基础批次（Q1–Q12）答的是"每道门是什么"，这批答"门后面怎么搭"：PKI 信任链、kubelet 权限边界、人与 CI 的身份接入、策略引擎换代（CEL）、沙箱运行时、GitOps 密管、零信任 mTLS、两个落地准入案例（AlwaysPullImages/distroless），最后是三条实战题——挖矿应急、CVE 应急、接手集群 10 分钟安全摸底（收口卡）。

## Q13. K8s PKI 信任链：组件之间到底谁信谁？（安全/PKI）

**一句话**｜一张 cluster CA 管所有"→apiserver"方向的客户端证书，etcd 自带独立 CA，扩展 apiserver（metrics-server）走 front proxy CA——三套 CA 说清，这题就赢了。

**30 秒答**｜面试官在考"你调过证书还是只背过命令"。逐行过信任表：

| 通信 | 凭证 | 签发者 |
|------|------|--------|
| kubectl → apiserver | 客户端证书 / OIDC / SA token | cluster CA |
| KCM/SCH/kube-proxy → apiserver | kubeconfig 内客户端证书 | cluster CA |
| kubelet → apiserver | CSR 签发的客户端证书（signerName=kubelet） | cluster CA |
| apiserver → etcd | apiserver-etcd-client | **etcd CA（独立一套）** |
| etcd 成员互信 | peer 证书 | etcd CA |
| apiserver → kubelet（exec/logs） | apiserver-kubelet-client 证书 | cluster CA；kubelet serving 证书靠 `--kubelet-certificate-authority` 校验 |
| apiserver → 扩展 apiserver（metrics-server） | front-proxy-client | **front proxy CA**（requestheader-client-ca） |

配套事实：kubeadm 一套 cluster CA（10 年）+ 组件证书 1 年（高级沉淀 Q20 的续期题）；admin.conf 的用户在 `system:masters` 组——绕过 RBAC 的超级身份（Q1 弹药）；etcd CA 证书 = 万能钥匙（Q4）。判断线：能说出"三套 CA 各管一段 + front-proxy 最容易漏"就是调过。

**弹药**
- metrics-server 报 x509 → 八成是 front proxy CA / `--kubelet-certificate-authority` 没配（很多人用 --kubelet-insecure-tls 蒙混）
- apiserver 证书 SAN 必须含：kubernetes、kubernetes.default(.svc)、service IP、LB IP、节点名——SAN 缺了 kubectl 直接 x509 报错
- `kubeadm certs check-expiration` 一屏看完；`openssl x509 -text` 核 SAN 是排障基本功
- kubelet serving 证书默认自签——apiserver 侧不配 CA 校验 = 不验证节点身份

**红线**｜❌ 说"就一张 CA"（etcd/front-proxy 两套独立的最容易漏）；❌ etcd 证书和集群 CA 混为一谈；❌ 说不出 front-proxy 是干什么的。

**下一问**｜信任链里最特殊的一环是 kubelet——它的权限为什么单独设计？

---

## Q14. Node authorizer / NodeRestriction：kubelet 的权限为什么特殊？（安全/鉴权）

**一句话**｜kubelet 天然需要读它节点上所有 Pod 的 Secret——给 RBAC 就是全集群泄密，所以单独造了 Node authorizer（只看自己节点的东西）+ NodeRestriction 准入（只准动自己的 Node/Pod）。

**30 秒答**｜kubelet 的正当需求：读本节点 Pod、Pod 引用的 Secret/ConfigMap、PVC/PV 状态，写 Node 状态和 Pod status。若用普通 RBAC 发这些权限，任何节点的 kubelet 可读**全集群** Secret——横向移动直通车。解法两层：①**Node authorizer**（authorization-mode 里第一个 `Node`）：身份在 `system:nodes` 组且带合法节点名的 kubelet，权限被自动限定为"仅本节点绑定的对象"——读 A 节点 Pod 的 Secret 到 B 节点？直接拒。②**NodeRestriction** 准入插件：再收一刀——kubelet 只能改**自己的** Node 对象（label 只许 `kubernetes.io/` 白名单前缀）、只能创建/修改调度到自己节点的 Pod、不能给自己加 taint 乱打标。判断线：kubelet 证书被偷 ≠ 集群沦陷——拿到的是"本节点 Pod 的 Secret + 自己 Node 的操控权"，这就是为什么 kubelet 凭证要和 admin 严格分开、且 authorization-mode 必须带 Node。

**弹药**
- `kubectl auth can-i get secrets -n <ns> --as=system:node:<node-a>` 在 node-b 的语境下试——被拒即 Node authorizer 生效
- 自注册链路：kubelet 用 bootstrap token 提 CSR（高级沉淀 Q38）→ 自动审批 → `system:nodes` 组 → Node authorizer 接管
- apiserver 参数口径：`--authorization-mode=Node,RBAC`——Node 必须在 RBAC 前面；漏了 Node 用 RBAC 给 nodes 组放权 = 退化成"全集群 Secret 互读"
- NodeRestriction 默认在准入插件列表里，1.x 一直默认开——但自建二进制部署的人会漏

**红线**｜❌ 给 `system:nodes` 绑普通 ClusterRole 放行 Secret（正是这套机制要防的）；❌ 节点间复用同一张 kubelet 证书（节点名身份失效，Node authorizer 形同虚设）；❌ 说不清 kubelet 被攻陷的实际爆炸半径。

**下一问**｜kubelet 是机器身份，那人和 CI 的身份怎么接进来？

---

## Q15. 人与 CI 的身份接入：OIDC、kubeconfig 与凭证治理（安全/身份）

**一句话**｜人的身份接 OIDC（组映射 RBAC），CI 用短时 SA token / 云角色联合（ACK 的 RRSA）——**共同大忌是共享 admin.conf**：K8s 客户端证书没有吊销机制，泄漏只能等过期。

**30 秒答**｜①**人**：生产标准姿势是 apiserver 挂 OIDC（`--oidc-issuer-url`、`--oidc-client-id`、`--oidc-username-claim`、`--oidc-groups-claim`），IdP 的 group claim 直接映射 RBAC——人在 IdP 离职即失效。反面教材：X.509 客户端证书**没有任何吊销机制**（不支持 CRL/OCSP 强校验），泄漏的 admin 证书只有两个出路——等 1 年过期，或换 CA（全集群重签）。②**本地凭证纪律**：admin.conf 只存密码库/堡垒机，本地 kubeconfig 用 exec 凭证插件（oidc-login、`aliyun cs GET /k8s-token`、`aws eks get-token`）现取短时 token——磁盘上不留长期凭证。③**CI**：最佳是云角色联合——ACK 叫 **RRSA**（RAM Roles for Service Accounts，对应 AWS 的 IRSA），CI Pod 凭 SA 换 RAM 临时凭证，全程无 AK；退一步用 `kubectl create token --duration` 短时 SA token，绝不落长期 Secret。④**审计增强**：审计类 CI 工具用 impersonation 头（`--as=<真实用户>`）透传操作者身份，audit log 里看到的是人而不是"ci-bot"。

**弹药**
- `kubectl config view --raw` 查本地有没有躺证书/token——面试可以演示"我的 kubeconfig 里只有 exec 插件"
- 共享 admin.conf 的代价：所有人操作在 audit 里都叫 `kubernetes-admin`——出事查不了人（Q10 直接废掉）
- OIDC 组 → RBAC 的桥：ClusterRoleBinding subjects 写 group，不写一个个 user
- kubectl 执行插件示例：`users: [{name: me, user: {exec: {apiVersion: client.authentication.k8s.io/v1, command: kubectl, args: [oidc-login, get-token]}}}]`

**红线**｜❌ 团队共享 admin.conf（无审计 + 无吊销双重雷）；❌ CI 里放永久 AK/证书——RRSA/短时 token 就是解；❌ 说"证书丢了重签一张就行"——重签不吊销旧的。

**下一问**｜身份接进来之后，策略执行引擎本身也在换代——CEL 和 webhook 怎么选？

---

## Q16. ValidatingAdmissionPolicy（CEL）和准入 webhook 怎么选？（安全/策略引擎）

**一句话**｜VAP 把校验策略写成集群内 CEL 资源（1.30 GA）：进程内执行、没有外部服务依赖——webhook 的"Fail 拒写 / Ignore 绕过"两难直接消失；需要改写对象或查外部上下文时才回 webhook。

**30 秒答**｜**VAP**：`ValidatingAdmissionPolicy`（规则，CEL 表达式）+ `ValidatingAdmissionPolicyBinding`（绑定范围与参数）两个对象，`validationActions: [Deny, Warn, Audit]` 三种动作可叠加。对比 webhook 的本质优势：**没有独立服务就没有独立故障面**——Q9 的 failurePolicy 两难、TLS 证书、超时放大、自拦截死锁，全都不存在；策略随 GitOps 版本化，dry-run/audit 模式渐进放量天生支持。局限也说清：只做校验（不能 mutate——MutatingAdmissionPolicy 1.32 才 alpha）、CEL 表达式复杂逻辑费劲、**不能访问外部服务**（这也是优点：策略结果确定性）。**仍需 webhook** 的三个场景：需要改写对象（Istio sidecar 注入）、需要外部上下文（cosign 拉验签、查 CMDB）、需要第三方生态（Kyverno/OPA 的策略库）。判断线：能校验的用 VAP（新集群默认选项），要 mutate/要外呼的用 webhook，混用不冲突。

**弹药**
- K8s 官方 ValidatingAdmissionPolicyLibrary 有一套现成策略模板（require-ro-rootfs、限制 hostNetwork 等），起步直接抄
- `kubectl get validatingadmissionpolicies,validatingadmissionpolicybindings`
- 放量姿势：先 `validationActions: [Audit]` 观察两周 → 加 Warn → 最后 Deny（和 PSA 迁移同一套节奏，高级沉淀 Q45）
- CEL 也能做条件：`request.operation == "CREATE" && object.spec.securityContext.privileged` 这种一行就是一条策略

**红线**｜❌ "VAP 会取代 webhook 所以直接迁"——mutate 和外呼场景留 webhook，别一刀切；❌ 把 CEL 写成百行天书（可读性就是策略的可维护性）；❌ 不知道 Deny/Warn/Audit 可以并存。

**下一问**｜策略挡的是误配，更强的隔离靠换运行时——什么时候上沙箱？

---

## Q17. 什么时候需要 gVisor / Kata 沙箱运行时？（安全/沙箱）

**一句话**｜runc 和宿主机共享内核——内核 0day 一炸全炸；跑不可信代码或多租户 SaaS 就要沙箱：gVisor 用用户态内核拦 syscall，Kata 给每个 Pod 一个轻量 VM 真 kernel。

**30 秒答**｜判断标准一条：**容器里跑的代码可不可信**。可信（自家服务）→ runc + 硬化（Q6）足够；不可信（CI runner、FaaS、多租户运行用户代码）→ 沙箱。两条路线：①**gVisor**：用户态实现 Linux ABI（Sentry 进程拦截 syscall），攻击面变成"实现层漏洞"而非内核本体——优点轻快，缺点 syscall 密集型负载性能掉、ABI 兼容有缺口（fancy netlink/io_uring 类可能不支持）；②**Kata Containers**：每 Pod 一个轻量 VM（真 kernel + 硬件虚拟化），隔离最强——代价：每 Pod 多 ~100MB 内存、启动慢、云上常需支持嵌套虚拟化的机型（裸金属/特定规格）。K8s 接入统一走 **RuntimeClass**：集群注册 runtimeClass（gvisor/kata），Pod 写 `runtimeClassName: kata`，RuntimeClass 自带 scheduling 字段把这类 Pod 引到装了沙箱的节点池——经典形态是"专用节点池跑不可信负载"，与 Q12 的 taint 隔离天然组合。判断线：沙箱是纵深一层不是免死金牌——PSS/最小权限照配，别因为上了 Kata 就放开策略。

**弹药**
- `kubectl get runtimeclass`；Pod 里 `runtimeClassName` 一行切换
- containerd 侧注册：config.toml 的 `[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]`（gVisor 的 runtime type）
- CI runner 场景标准答案：专用节点池 + taint + Kata + 每次 Job 用完销毁节点（无状态 = 无取证负担）
- 性能口径：gVisor 对 syscall 重负载（频繁 fork/IO）损耗最明显，网络转发类中等；上线前拿真实负载压测

**红线**｜❌ "上了沙箱可以不用 PSS/硬化"——层与层不互斥；❌ 不测兼容性全量切换（io_uring、特定 syscall 一撞就碎）；❌ 说 gVisor 是"VM"（它是用户态内核，Kata 才是 VM）。

**下一问**｜从运行时回到镜像本身——GitOps 时代 Secret 怎么不进 Git 还能部署？

---

## Q18. GitOps 下的 Secret 管理：SOPS / Sealed Secrets / ESO 怎么选？（安全/密管）

**一句话**｜三档：Sealed Secrets（集群公钥加密进 Git）、SOPS+KMS/age（文件级加密、部署时解）、External Secrets Operator（Git 里只有"指针"，运行时从云密管拉）——云原生主力选 ESO，共同底线是**明文 Secret 进过 Git 就当已泄露**。

**30 秒答**｜先立判断线：Git 历史删不掉，任何明文 secret 提交过 = 已泄露，处置是轮换不是删提交。三方案对比：

| 方案 | 原理 | Git 里存什么 | 适用 |
|------|------|-------------|------|
| Sealed Secrets（bitnami） | 集群侧控制器持私钥，本地用其公钥加密 | 加密 SealedSecret | 小团队简单起步 |
| SOPS + age/KMS | 按 key 加密 yaml 文件，部署时解（ArgoCD 走 KSOPS 插件） | 加密原文 | 要求"一切进 Git"的团队 |
| **External Secrets Operator** | 集群只存 ExternalSecret CR（引用关系），控制器从云密管/Vault 同步成 Secret | **连密文都没有** | 云原生主力（推荐） |

ESO 为什么是主力：密钥生命周期（轮换/权限/审计）完全留在密管侧，集群只存引用；换环境只换 backend。配套细节：ESO 的 SecretStore 本身要用 IRSA/RRSA（Q15）绑定云权限，不落 AK；secret 刷新有 `refreshInterval`，轮换后自动同步。ArgoCD 生态对应物：argocd-vault-plugin / KSOPS。判断线：被问"GitOps 怎么管密码"时先答"目标状态是 Git 里没有 Secret"——这句话定调，再讲方案。

**弹药**
- Sealed Secrets 私钥丢失 = 全部 SealedSecret 解不开——私钥必须备份（它落在集群 Secret 里，etcd 备份一起管）
- SOPS 加密粒度可以到"只加密 data 字段"——diff 仍可读结构，评审友好
- ESO 巡检：`kubectl get externalsecrets -A` 看 SecretSynced 状态；同步失败 Secret 会保留旧值（不致故障但会陈旧）
- 密管侧审计 + 集群 audit（Q10）两层一起看：谁改了密钥、谁拉了 Secret

**红线**｜❌ 用 kustomize secretGenerator / Helm values 明文管生产 secret；❌ GitOps 仓库权限开放到"公司全员可读"还塞密文（SOPS 密文泄露密钥即裸奔）；❌ 轮换只轮密管不轮集群（ESO refreshInterval 没配，Secret 永远是旧的）。

**下一问**｜GitOps 管配置，集群内流量呢——零信任的 mTLS 解决 NetworkPolicy 解决不了的什么？

---

## Q19. 零信任：Istio mTLS / SPIFFE 身份解决 NetworkPolicy 的什么盲区？（安全/零信任）

**一句话**｜NetworkPolicy 管得到"IP 包让不让过"，管不了三件事：流量明文、身份不可认证（IP 会变可被滥用）、L7 授权——mTLS 补身份+加密，SPIFFE 给每个工作负载一个可验证的名字。

**30 秒答**｜netpol 的三个盲区：①**明文**——同 ns 白名单内的东西互访全是裸奔；②**IP 信任模型**——放行是按 IP/label，攻陷一个白名单内 Pod 就能代表它说话（IP 不验证对端身份）；③**无 L7**——"允许调 /api/payments 但不许 /admin"做不了。mTLS（Istio/Linkerd/ACK 的 ASM）：两端互相验证书再通信，流量加密；身份从 SA 派生 **SPIFFE ID**（`spiffe://cluster/ns/prod/sa/web`），证书短周期自动轮换——攻陷 Pod 拿不到别人身份。L7 授权（AuthorizationPolicy）按 SPIFFE 身份放行，不再看 IP。部署节奏：mTLS 先 PERMISSIVE（明文也收）观察 → 全量切 STRICT，避免一刀切断流量。与 netpol 的关系是**纵深不是替代**：netpol 在转发层挡边界（还管 hostNetwork/外部流量，mesh 管不到），mTLS 在身份层管加密与认证。成本也答得出：sidecar 资源与延迟、ambient mesh（ztunnel 做无 sidecar 的 L4 mTLS）是当前降本方向。

**弹药**
- `istioctl authz check <pod>` / Kiali 的 graph 安全模式——面试能报出排障工具是实践信号
- PeerAuthentication（命名空间级 STRICT）+ AuthorizationPolicy（按 principal 放行）两件套
- SPIFFE 通用化：跨集群/跨信任域身份互认走 SPIRE 联邦——答出这个词是加分
- 与 SA 的关系：mesh 证书签给 SA（Q3 的 workload 身份），所以"最小权限 SA"是零信任的地基

**红线**｜❌ "上了 mesh 可以删 NetworkPolicy"——hostNetwork/节点流量/北向入口 mesh 管不到；❌ mTLS STRICT 一刀切上线（老客户端直接断）；❌ 说不出 SPIFFE ID 长什么样。

**下一问**｜策略与身份都齐了，看两个具体落地案例——先从最便宜的开始：AlwaysPullImages。

---

## Q20. 生产为什么建议开 AlwaysPullImages 准入？（安全/准入·案例）

**一句话**｜默认 IfNotPresent 信任节点本地缓存——节点失陷或有人能往 registry 推同名 tag，后续 Pod 会"不拉新"直接跑毒镜像；AlwaysPullImages 强制每次拉取，把镜像来源拉回 registry 的认证与审计之下。

**30 秒答**｜攻击场景两条：①**缓存投毒**：攻陷节点的人（或有节点执行权限的进程）把本地缓存镜像改掉——IfNotPresent 下，同节点重建的 Pod 不拉取，直接用毒镜像；②**tag 变脸**：有 registry 写权限的人推一个同名 `:prod` tag——其他节点下次起新 Pod 时才拉到毒镜像。AlwaysPullImages 准入插件把 imagePullPolicy 强制改成 Always：每次创建容器必走 registry——认证、镜像来源、拉取行为全在 registry 审计里可见。代价要主动说：registry 变成硬依赖（registry 挂了本地有镜像也起不来）+ 拉取延迟与仓库压力——解法是 pull-through mirror / registry HA，而不是关掉这个插件。和 Q8 是组合拳：**AlwaysPullImages 防缓存投毒，digest 固定防 tag 变脸**——两道一起才闭环。注意它是非默认准入插件，要显式 `--enable-admission-plugins=AlwaysPullImages`。

**弹药**
- 全集群 :latest 面盘点：`kubectl get pods -A -o json | jq -r '.items[].spec.containers[].image' | grep -c ':latest'`
- 审查现网 effective pullPolicy：`kubectl get pod <p> -o jsonpath='{.spec.containers[*].imagePullPolicy}'`
- registry 侧配套：私有 + 认证 + 只读账号分发；写权限收敛到 CI 身份
- 判断线：面试官追问"代价是什么"时，registry 可用性耦合必答——只背优点会露馅

**红线**｜❌ 开了 AlwaysPullImages 却没配 mirror（大规模滚动时 registry 被打爆，复刻 Q7 的风暴剧本）；❌ 以为它防 tag 变脸（那是 digest 固定的活）；❌ 不知道它是非默认插件。

**下一问**｜镜像来源可信之后，镜像本身越小越安全——distroless 的账怎么算？

---

## Q21. distroless / 最小化镜像的安全价值是什么？（安全/镜像·硬化）

**一句话**｜攻击面正比于包数量：distroless 砍掉 shell、包管理器和一切非运行必需物——落马难持久化、CVE 面变小、镜像小拉取快；代价是没有 shell 好调试，用 ephemeral debug 容器补。

**30 秒答**｜安全账三笔：①**落盘面**：没有 shell / apk / busybox，攻击者进了容器也写不了 webshell 跑不了脚本——Q6 里的"改 cron、起反弹 shell"路线在镜像层就断了；②**CVE 面**：基础镜像从 Ubuntu（几百个包）到 distroless/static（几个 so），扫描器报的高危项数量级下降——Q8 的扫描卡点误报率跟着降；③**意外面**：没有 curl/wget，应用被注入后外传数据的顺手工具少一半。构建纪律配三件：**多阶段构建**（编译工具链留在 builder 阶段，final 只拷产物）、镜像里显式 `USER` 非 root（配 Q6 的 runAsNonRoot 才成立）、base 用 digest 固定 + 定期重建（Q8 的保鲜）。调试方案要主动答：没有 shell 不是绝路，`kubectl debug -it --image=busybox --target=<c>` 注入临时容器共享进程/网络命名空间（高级沉淀 Q33），比"为了 debug 给镜像塞个 bash"健康得多。alpine 提一句：比 distroless 大、有 shell/apk，musl 还偶有 DNS 兼容坑——"小"和"安全"不是一回事，distroless/Chainguard 才是安全取向。

**弹药**
- 模板骨架：`FROM golang:1.x AS build` → 编译 → `FROM gcr.io/distroless/static` + `COPY --from=build` + `USER nonroot:nonroot`
- `dive <img>` 看每层大小与文件；Trivy 扫 base 与 final 的 CVE 数对比——汇报用
- distroless 各 tag 差异：static（纯静态）/ base / cc（带 glibc）——Go 静态编译用 static 最小
- Chainguard Images 走"near-zero CVE + 定期重建"路线——答出说明追踪过生态

**红线**｜❌ 把 alpine 当"安全镜像"（有 shell 有包管理器，只是小）；❌ debug 需求又把 bash/工具塞回生产镜像（那 hardened 白做）；❌ 只讲"镜像小"不讲"落盘面/CVE 面"——这题考的是安全账。

**下一问**｜防线终有穿透时——发现可疑/挖矿 Pod，应急动作顺序？

---

## Q22. 发现可疑 / 挖矿 Pod，应急动作顺序？（安全/应急响应）

**一句话**｜六步：**围而不杀**（隔离保留证据）→ 取证 → 定入侵路径 → 斩断凭证 → 清剿恢复 → 复盘补策略——第一反应如果是 `kubectl delete pod`，这题就挂了（控制器会重建，证据同时蒸发）。

**30 秒答**｜①**围**：单 Pod 可疑 → 先打 label 标记 + NetworkPolicy deny 该 ns 出入站（Q7 模板反用）；节点可疑 → cordon + 评估网段隔离。**不 delete**——控制器会重建，etcd 对象和容器现场都没了。②**取证**：`kubectl get pod -o yaml` 固化对象（谁创建的、什么镜像 digest、挂了什么 SA）；`kubectl debug` 进容器拷进程表/定时任务/临时文件；拉 audit log 还原创建链（Q10：user、sourceIP、谁 approve 的 RBAC）。③**定路径**：顺着 audit 找上游——CI 凭证泄漏？SA 被绑了高权限（Q2）？GitOps 仓库被改（Q18）？registry 被推毒（Q20）？路径不清就轮换凭证是盲目的。④**斩断凭证**：删重建涉事 SA（token 即失效）、轮换 imagePullSecrets 和应用 Secret（Q4 的重写）、回收多余 RBAC。⑤**清剿恢复**：全集群扫持久化后门位——**CronJob、DaemonSet、admission webhook 都是攻击者爱用的驻留点**（恶意 webhook 能拦所有后续部署）；用验证过的镜像 digest 重建。⑥**复盘**：这颗雷该被哪层拦住（PSS？验签？AlwaysPullImages？）——补上，否则第二次只是换个镜像名。

**弹药**
- 快速圈可疑面：`kubectl get pods -A -o wide | grep -v Running`之外，按镜像仓库名聚类找陌生源
- 后门巡检三连：`kubectl get cronjob,daemonset -A`、`kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations`（看有没有陌生 webhook）
- 取证镜像快照：`kubectl debug <pod> --image=busybox` 共享进程空间后 `ps aux` / `cat /proc/*/cmdline`
- 汇报口径：RTO（隔离用时）/ 影响面（哪些凭证轮换）/ 根因（哪层策略缺失）

**红线**｜❌ 第一动作 delete pod（现场全毁 + 控制器重建）；❌ 只杀 Pod 不轮换凭证（攻击链还在，换名重来）；❌ 清剿不查 CronJob/DaemonSet/webhook 驻留位。

**下一问**｜挖矿是"人打进来了"，另一类是"漏洞送上门"——高危 CVE 爆出来怎么响应？

---

## Q23. runc / 内核高危 CVE 爆出来，响应流程？（安全/漏洞应急）

**一句话**｜四步：定性（影响版本+我方暴露条件→可不可利用）→ 盘点（全集群版本清单）→ 分级处置（可利用先临时缓解、补丁跟进）→ 滚动升级闭环（drain 批次+PDB，控制面也要升）。

**30 秒答**｜①**定性**：读公告拿两条——影响版本范围、触发条件。经典对例：CVE-2019-5736（runc）需要容器内 root 执行不可信镜像才触发；DirtyPipe 需要内核在 5.8–5.16.11 区间。"是否可利用"必须能说出依据：如果集群跑了不可信负载且内核在区间 → 可利用；全自家可信镜像 + PSS restricted → 暴露条件大减。②**盘点**：`kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kernelVersion}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}'` 一把梭出内核/kubelet 清单，runc/containerd 版本从 `crictl info` 取——没有版本清单就没有响应起点。③**分级处置**：可利用 → **临时缓解先行**（暂停 CI runner 等不可信负载、收紧 PSS 禁 privileged/exec 类权限、必要时 admission 拦新调度），不等补丁；不可利用 → 排期升级，但留缓解记录。④**滚动升级闭环**：staging 先行 → 生产按节点池分批（drain + PDB，高级沉淀 Q29），**控制面节点也要升**（master 上同样跑 kubelet/containerd，静态 Pod 一样中招）——全集群一把梭会复刻 Q7 的重启风暴；升级后全量版本清单复核归零。

**弹药**
- 版本盘点一条龙命令（上面 jsonpath）+ `crictl info | grep -iE "runc|containerd"` 节点侧核对
- 订阅源：K8s 官方 security-advisories、containerd/runc 的 GHSA、阿里云安全公告（ACK 用户）
- 临时缓解层位选择：能 admission 层拦的别到运行时，能运行时拦的别等节点重刷
- 响应文档三要素：暴露面结论（含依据）、升级批次表、验证记录——审计与复盘都要

**红线**｜❌ 全集群同时升级（Q7 风暴 + 无回滚点）；❌ 只升 worker 不升控制面（master 的 kubelet/containerd 是漏网之鱼）；❌ 宣称"不可利用"给不出触发条件对照。

**下一问**｜事件驱动之外还有日常体检——接手一个集群，10 分钟安全摸底看什么？

---

## Q24. 接手一个陌生集群，10 分钟安全摸底看什么？（安全/体检·收口）

**一句话**｜十个动作按"暴露面→权限面→数据面→可追溯→版本面"的顺序过一遍——这题考的是你有没有自己的 checklist，脱口而出 8 项才算做过安全治理。

**30 秒答**（清单式）：
```
1 匿名探测    curl -k https://<apiserver>:6443/version      → 应 401/403
2 授权底座    --authorization-mode=Node,RBAC；盘 system:masters 与 cluster-admin bindings
3 节点口子    curl -k https://<node>:10250/pods 应 401；10255 应拒绝（Q11）
4 特权面      jq 盘 privileged / hostNetwork / hostPath / hostPID Pod（应只剩系统组件，Q5）
5 PSA 覆盖    kubectl get ns --show-labels | grep pod-security → 数 enforce 缺失的 ns（Q45）
6 网络覆盖    ns 总数 vs 有 default-deny 的 ns 数（Q7）
7 数据落盘    apiserver 有没有 encryption-provider-config（Q4）
8 可追溯      有没有 --audit-policy-file、日志送不送出集群（Q10）
9 镜像面      :latest Pod 数；有没有验签/AlwaysPullImages（Q8/Q20）
10 版本面     apiserver↔kubelet 版本 skew、内核、runc（Q23 的盘点命令）
```
判断线：顺序本身就是方法论——先看"外面能不能进来"（1/3），再看"进来后权限多大"（2/4/5），再看"数据裸不裸"（7），再看"出事能不能查"（8）。只会答"跑个 kube-bench"减分——工具代替不了判断，kube-bench 只是第 3/8 步的自动化。

**弹药**
- 把十步写成脚本/Job 定期跑，输出 diff——"体检"变"持续体检"是下一个加分点
- `kubectl get clusterrolebindings -o json | jq -r '.items[] | select(.roleRef.name=="cluster-admin") | .subjects[]?'` 盘神权一步出
- 体检报告四象限：立即修（暴露面）/ 排期修（策略面）/ 记录豁免 / 委托云厂商（共享责任，文末深挖）
- 对接 Q22：体检清单 = 应急时的"攻击面快照"基线，平时扫的每一条都是战时排查路径

**红线**｜❌ 上来就 `kube-bench` 跑半小时然后照单全修（没有优先级观）；❌ 十个动作说不全、或说不出顺序逻辑；❌ 摸完不出报告——体检没有结论等于没做。

---

**下一批预判**｜安全进阶的延伸：容器取证实操（coredump/CRIU/日志固化）、Falco 规则编写、合规映射（等保 2.0/PCI→K8s 控制项对照）、供应链 SLSA 等级与 in-toto——面试被追问时按这四条接。
