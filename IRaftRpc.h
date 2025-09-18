#pragma once

#include <string>
#include <unordered_map>

class raft; // 前置声明，避免循环包含

// 抽象 RPC 接口
class IRaftRpc {
    public:
        virtual ~IRaftRpc() = default;
        virtual bool requestVote(int targetId, int term, int candidateId,
                                 int candidateLastLogIndex, int candidateLastLogTerm) = 0;
        // 追加日志/心跳：当 hasEntry=false 时为心跳；leaderCommit 用于推进 follower 提交点
        virtual bool appendEntries(int targetId, int term, int leaderId,
                                   int prevLogIndex, int prevLogTerm,
                                   const std::string& cmd, const std::string& content,
                                   int entryTerm, int entryIndex, bool hasEntry,
                                   int leaderCommit) = 0;
        // 兼容旧心跳接口（可由实现内部转发到 appendEntries）
        virtual bool appendEntriesHeartbeat(int targetId, int leaderTerm, int leaderId, int leaderCommit) = 0;
    };

// 简单内存总线实现（对外可见）
class InMemoryRpc : public IRaftRpc {
public:
    std::unordered_map<int, raft*> id2node;
    bool requestVote(int targetId, int term, int candidateId,
                     int candidateLastLogIndex, int candidateLastLogTerm) override;

    bool appendEntries(int targetId, int term, int leaderId,
                       int prevLogIndex, int prevLogTerm,
                       const std::string& cmd, const std::string& content,
                       int entryTerm, int entryIndex, bool hasEntry,
                       int leaderCommit) override;

    bool appendEntriesHeartbeat(int targetId, int leaderTerm, int leaderId, int leaderCommit) override;
};