# raft
</br>
*Raft
</br>
Raft 是一种分布式一致性算法（共识算法），用于在多台不可靠机器上就一串指令（日志）达成“强一致”的顺序与结果。它把分布式系统抽象为“复制状态机”：只要所有副本按同样顺序执行相同指令，最终状态就一致。
设计目标: 与 Paxos 等价的安全性与可用性，但更易理解与实现。
核心角色: Follower（跟随者）、Candidate（候选人）、Leader（领导者）。
关键术语: Term（任期）、Log Index（日志序号）、Commit Index（已提交位置）、多数派投票。
</br>
*核心机制
</br>
领导者选举: 节点在超时未收到心跳时发起选举；获得多数票者成为 Leader。投票遵循“日志新者优先”，避免旧 Leader 覆盖新日志。
日志复制: 客户端写请求交给 Leader，Leader 将日志条目复制到多数节点；多数确认后“提交”，再对外生效。
安全性: 只要多数节点存活，不会出现已提交数据回滚；Leader 只能提交当前任期自己产生的条目，防止乱序。
成员变更: 采用“联合共识”（Joint Consensus）两阶段过渡，保证变更期间仍需多数派。
日志压缩: 通过快照（Snapshot）截断旧日志，降低存储与恢复成本。
读一致性: 常用两种方式
线性一致读：ReadIndex/心跳确认或强制走 Leader。
高性能读：Leader 租约（Lease Read），依赖时钟假设。
</br>
*优缺点
</br>
优点: 简单清晰、工程实现成本低、强一致（CP）、可恢复性好。
限制:
需要多数派存活（> N/2），分区下偏向一致性而牺牲可用性。
Leader 可能成为吞吐瓶颈（可通过分片/多 Raft 组缓解）。
跨机房高延迟场景下提交延迟受限于最慢多数副本。
</br>
*典型应用与产品
</br>
配置/注册中心: etcd、Consul（Kubernetes 通过 etcd 实现一致存储）。
分布式数据库/KV: TiKV/TiDB、CockroachDB、RocksDB 生态中的复制方案、RedisRaft 模块。
消息与日志系统: Kafka 的 KRaft 模式（移除 ZooKeeper）。
搜索与协调: Elasticsearch Zen2（Raft 思想改造）、NATS JetStream（基于 Raft 的流副本）。
元数据/控制面: 文件系统元数据服务、服务发现、分布式锁、Leader 选举。
