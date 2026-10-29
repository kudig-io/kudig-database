# K8s 集群核心组件 · 快问快答（白话版）

> 配套 `maintaining/04-报告/k8s-高级面试快问快答-沉淀.md` 的 Q13–Q20。
> 那边是"面试口径"的压缩答案卡，这版用人话把每个概念讲明白。
> **建议用法**：先看这版建立理解，再去背沉淀版的「30 秒答」。
> 日期：2026-10-16

---

## 〇、先建全局：一个集群就是一家公司

| 组件 | 相当于 | 干什么 |
|------|--------|--------|
| apiserver | 前台总服务台 | 所有事都要经过它登记，是唯一能改"档案"的窗口 |
| etcd | 档案室 | 集群全部状态只存这一份，唯一的真相 |
| scheduler | 分工位的 HR | 看新员工（Pod）该坐哪个工位（节点），只做决定、不动手 |
| kube-controller-manager | 盯 KPI 的部门经理 | 不停对比"应该几个"和"实际几个"，有差距就去补 |
| kubelet | 车间主任 | 拿着本工位名单，实际安排人上岗、盯健康 |
| kube-proxy | 门牌分机号设置员 | 让"找 Service"的电话能转到真实 Pod |
| containerd | 真正的搬砖工 | kubelet 喊"起容器"，它去干 |
| CoreDNS | 查号台 | 把服务名翻译成 IP |

---

## 一、kube-scheduler

**Q1：Pod 提交到跑起来，中间发生了什么？**

四步：①提交的 Pod 先被记进档案（etcd），这时它没有节点，状态 Pending；②调度器看到"没节点的 Pod"，先把明显不行的节点划掉（内存不够的、明确说不要去的）——**过滤**；③剩下的候选打分：谁资源最宽裕、谁上边已有相同镜像就加分，选最高分——**打分**；④把节点名写进 Pod 档案（**绑定**），kubelet 看到名单上有自己才真正拉容器。

一句话：**调度器只决定"坐哪个工位"，真正安排上岗的是 kubelet。**

**Q2：scheduler 挂了会怎样？**

已经在跑的 Pod 完全不受影响——它们早绑好节点了，调度器只服务"还没节点的 Pod"。后果只有一个：新 Pod 一直 Pending。所以面试里说"调度器挂了集群就崩"是错的。

**Q3：Pod 一直 Pending，第一眼看哪里？**

`kubectl describe pod` 拉到最后看 Events。调度器会把拒绝原因写在那，比如 `0/6 nodes are available: Insufficient cpu`（6 个节点没有一个 CPU 够）。这是调度器亲口说的，比猜快得多。

**Q4：几千个节点，调度会不会太慢？**

有开关 `--percentage-of-nodes-to-score`：不用每个节点都打分，抽一部分就行，集群越大抽得越少（最低 5%）。牺牲一点"最优"，换大集群的调度速度。

**Q5：写死 spec.nodeName 会怎样？**

等于绕过 HR 自己占工位：资源够不够、污点让不让去，所有检查全部跳过，可能坐上一个根本坐不下的工位。生产别这么干。

**Q6：高级别的 Pod 抢不到资源怎么办？**

抢占机制：给 Pod 分优先级（PriorityClass），高级 Pod 调度不上时，调度器挑"代价最小"的低优先级 Pod 赶走，腾出资源。

---

## 二、kube-controller-manager（KCM）

**Q1：KCM 和 scheduler 有什么区别？**

scheduler 只回答一个问题："这个 Pod 放哪个节点？"KCM 管除此之外所有的"期望 vs 现实"：你说要 3 个副本现在只有 2 个，它去补。两者都不直接碰容器——碰容器的是节点上的 kubelet。

**Q2：Deployment / ReplicaSet / Pod 三层，副本数归谁管？**

**ReplicaSet 管**。Deployment 只管"有几个版本的 RS、怎么滚动"。改 Deployment 的 replicas，实际是 Deployment 让下面的 RS 去改。

**Q3：KCM 里都有哪些控制器？**

面试常问的：副本控制（Deployment/RS）、节点管理（失联打标记、超时赶 Pod）、Endpoints（记录 Service 该连哪些 Pod IP）、垃圾回收（删 Deployment 连带删干净 RS 和 Pod）、ServiceAccount 和证书审批。

**Q4：为什么 KCM 全集群只能有一个"活的"？**

两个控制器同时调和同一个对象会打架——都去创建 Pod 就重复了。所以多副本部署用"抢椅子"（Lease 租约对象）选一个领导，其他待命。默认节奏：租约 15 秒、10 秒内必须续、每 2 秒试一次；领导挂了最多 15 秒后备用顶上。

**Q5：节点失联后，为什么打"污点"而不直接删 Pod？**

打污点 = 贴张条子"这节点失联了"，每个 Pod 自己决定容忍多久（tolerationSeconds）。控制逻辑和业务需求解耦：有的业务愿意多等，有的想赶紧走。

---

## 三、kubelet

**Q1：PLEG 是什么？**

kubelet 的"眼睛"。每 1 秒把本机所有容器列一遍，跟上一次对比，发现"谁起了、谁死了"就生成事件交给 kubelet。容器运行时卡住、列不动时，kubelet 报 `PLEG is not healthy`，节点转 NotReady。排障分岔口：PLEG 挂是**本机**问题；日志说"上报节点状态失败"才是**控制面**问题。

**Q2：静态 Pod 是什么？为什么删不掉？**

kubelet 直接管的一种 Pod：YAML 文件放进节点 `/etc/kubernetes/manifests` 目录，kubelet 看到文件就起，不看 apiserver 脸色。`kubectl delete` 删不掉——kubelet 一看文件还在，马上又拉起来；要删就删节点上的文件。kubeadm 装的集群里，etcd/apiserver 这些控制面组件全是静态 Pod，好处是"控制面能自己把自己拉起来"。

**Q3：cgroup driver 不一致是什么事故？**

kubelet 和 containerd 都要设置"用哪种方式管资源限额"（systemd 或 cgroupfs），两边必须选一样的。不一样的话资源账本对不上：莫名其妙的 OOMKilled、驱逐行为异常。装集群前先对齐，别等出事再查。

**Q4：kubelet 什么时候开始驱逐 Pod？**

三条默认线：节点内存剩不到 100Mi、根盘可用不到 10%、镜像盘可用不到 15%。碰到线就按优先级从低到高赶 Pod。

**Q5：pause 容器是干什么的？**

每个 Pod 里第一个被拉起的小容器，作用是"占坑"：把网络栈（IP、端口空间）握在手里，业务容器都加入它的网络。所以 pause 一死，整个 Pod 的网络没了，所有业务容器跟着退出重建。

**Q6：kubelet 的证书要手动换吗？**

不用。`--rotate-certificates` 默认开着，到期自动换。注意它跟 kubeadm 的 `certs renew` 是两套，互不相关。

---

## 四、kube-proxy

**Q1：kube-proxy 是代理吗？**

名字骗人，它不碰任何流量。它是"规则写入员"：盯着 Service 和 Pod 的变化，把"访问 Service IP 该转到哪些 Pod IP"写成内核里的规则，真正搬包的是内核。kube-proxy 进程挂了，写好的规则照样生效，只是不再更新。

**Q2：iptables 和 IPVS 模式差在哪？**

iptables 模式：规则是一条长清单，每个包进来要从头核对到尾——Service 少没事，几百上千个 Service 后每个包都在对长清单，越来越慢。IPVS 模式：内核用哈希表一查就中，还自带轮询、最少连接这些负载均衡算法。**Service 上千条是分界线，大集群直接选 IPVS。**

**Q3：Cilium 凭什么能替代 kube-proxy？**

用 eBPF 把"Service 转发"做到了更靠前的位置（socket 层）：程序发起连接那一刻就转好了，包根本不走那一长串内核转发规则。规则少了、路径短了，这就是"绕开 kube-proxy"。

**Q4：IPVS 上线前要检查什么？**

内核模块（ip_vs、ip_vs_rr 这些）加载了没有。没加载的话 kube-proxy 不报错，**悄悄退回 iptables**——你以为在用 IPVS，其实不是。大坑。

**Q5：Pod 都换了一波，为什么还有流量打到旧 Pod？**

两个原因叠加：①内核的连接跟踪表（conntrack）还记着"这个 Service → 旧 Pod"的旧账，已有连接继续发旧地方；②Pod 删除时有宽限期（preStop、terminationGracePeriodSeconds），还在收流量。把"从 Service 摘除"和"进程退出"的时间错开就平滑了。

---

## 五、控制面高可用

**Q1：哪些组件能多副本，哪些只能一个活？**

apiserver 随便多开——它自己不存东西（全在 etcd），开 3 个挂负载均衡后面就行。KCM 和 scheduler 只能一个干活：靠 Lease 抢主，抢到的干活，其他待命。etcd 用 3 台或 5台——**必须奇数**，偶数在分区时谁也凑不够多数派。

**Q2：etcd 跟控制面放一起还是分开？**

stacked（放一起，kubeadm 默认）：省机器，但一台挂了同时损失一个 apiserver 和一个 etcd 成员。external（分开）：故障隔离干净，代价机器翻倍。中小集群 stacked 够用，大规模或强合规选 external。

**Q3：控制面全挂了，线上业务会断吗？**

**不会**。业务流量走"节点上的转发规则 → Pod IP"，这条路上没有控制面的事。断掉的是：新 Pod 调不了、配置改不了、挂掉的 Pod 不重建。"控制面挂 = 全站瘫"是错的——是**管理瘫痪，运行继续**。

**Q4：KCM 出现两个领导会怎样？**

两个控制器同时调和，重复创建对象，比没领导还糟。出现双活说明 Lease 参数配错了（比如续租超时比租约还长），要修配置，不是重启了事。

---

## 六、containerd / CRI

**Q1：从 kubelet 到容器，中间经过谁？**

kubelet 不自己起容器：它喊一声标准化接口 CRI → containerd 接活 → 派一个 shim 当"包工头"盯着 → 真正干活的 runc 把容器进程拉起来。

**Q2：Pod 和容器什么关系？pause 干嘛的？**

一个 Pod ≠ 一个容器，是"一组共享网络的容器"。先起一个叫 pause 的小容器占住网络（IP 挂在它身上），业务容器再一个个加进来共用这张网卡。

**Q3：shim 是干嘛的？为什么升级 containerd 不用赶 Pod？**

shim 是每个容器的"直属包工头"，containerd 是总部。总部重启装修时，包工头带着工人（容器）继续干活，完工后重新向总部报到。所以升级 containerd 不影响在跑的容器。

**Q4：1.24 为什么把 Docker 踢了？Docker 镜像还能用吗？**

踢的不是镜像标准，是"转接头"：K8s 定了统一的 CRI 接口，Docker 的适配层（dockershim）维护成本太高就砍了。Docker build 的镜像符合 OCI 标准，推到镜像仓库后 containerd 照拉照跑——只是 containerd 不认节点上 `docker images` 里的本地缓存，两个存储。

**Q5：上了 K8s，在节点上怎么查容器？**

用 crictl，别再用 docker ps（看不到）：`crictl pods` 看 Pod 这层（sandbox），`crictl ps` 看容器这层，`crictl logs --previous` 看上一次崩溃的日志。

---

## 七、CoreDNS

**Q1：集群里域名怎么解析的？**

每个 Pod 的 DNS 配置指向 CoreDNS。查集群内名字（`nginx.default.svc.cluster.local`），CoreDNS 直接答；查外网域名（baidu.com），CoreDNS 转发给上游真正的 DNS。

**Q2：ndots:5 是什么坑？**

Pod 的 DNS 配置里 ndots:5 的意思是：域名里点号少于 5 个，就先当"短名"——把集群后缀一个个拼上去试（试满 5 次全失败），最后才当完整域名查。你查 baidu.com（1 个点）被迫白跑 5 趟。访问外网多的应用，DNS 请求量直接 **×6**。

**Q3：DNS 超时为什么总是整整 5 秒？**

一个经典组合 bug：DNS 走 UDP（不发握手），内核的网络转发层（conntrack）记连接账本时偶发把回包的端口记错，应答包丢了。客户端傻等 5 秒超时重试，第二次就成功——现象就是"偶发、正好 5 秒"。根治：NodeLocal 缓存 + TCP 兜底。

**Q4：NodeLocal DNSCache 是什么？**

在每个节点上放一个 DNS 缓存（DaemonSet），Pod 先问本节点，命中就不出节点。判据：Pod 的 resolv.conf 里 nameserver 变成 169.254.20.10。它还用 TCP 转发上游，顺带绕开上面那个 UDP 丢包 bug。

**Q5：dnsPolicy 三个值什么意思？**

ClusterFirst（默认，先问集群 DNS）/ Default（直接用节点的 DNS 配置，等于不做隔离）/ None（完全自定义）。

---

## 八、证书 / PKI

**Q1：kubeadm 集群的证书多久过期？**

两层：根 CA 证书 10 年；各组件证书（apiserver、etcd 互相认证用的）默认 **1 年**。所以经典事故叫"上线一周年那天集群炸了"。

**Q2：renew 之后就完事了吗？**

**没有！** 续期只是写了新文件，运行中的组件不会自动加载。必须让它们重启一遍——控制面组件是静态 Pod，把 manifests 文件挪走再挪回来（或删容器），kubelet 就会带新证书重建。只 renew 不重启 = 白干。

**Q3：CA 到期了能 renew 吗？**

不能。`certs renew all` 只续组件证书，不碰 CA。CA 到期要全部重签一遍，是大动作，必须提前一两年排期。

**Q4：证书过期的故障长什么样？**

辨识度很高：节点本身好好的，但所有 kubelet 连不上 apiserver，日志一堆 `x509: certificate has expired`，整个集群 NotReady。看到 x509 + 一周年时间点，基本就是它。

**Q5：怎么防止被炸？**

两道防线：①监控证书**剩余有效期**，低于 30 天就告警（注意：探测 /readyz 通只说明"现在没过期"，不等于"快过期会提醒你"）；②例行维护窗口提前续，别等最后一天。

---

## 🔗 串联题（把组件连成故事）

1. **Pod 一直 Pending**：describe pod 看 Events → 调度器说"节点不行"是调度问题（资源/亲和/污点）；说绑定了却没起来，去节点查 kubelet。
2. **节点失联时间线**：kubelet 每 10 秒报一次到 → 40 秒没消息控制面标"未知" → 打"失联"污点 → 300 秒后开始赶 Pod。中途先分辨是 PLEG（本机）还是控制面的问题。
3. **Service 不通**：先看 Endpoints 有没有内容（空 = Service 侧问题，别碰网络）→ 有内容查 kube-proxy 规则 → 再查 conntrack 旧账 → 最后才查 DNS。
4. **控制面抖动四部曲**：etcd 慢 → apiserver 限流 429 → 重启引发 LIST 风暴 → 一周年证书到期全线崩。
