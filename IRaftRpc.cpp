#include "IRaftRpc.h"
#include "raft.h"

bool InMemoryRpc::requestVote(int targetId, int term, int candidateId,
                              int candidateLastLogIndex, int candidateLastLogTerm){
    auto it = id2node.find(targetId);
    if (it == id2node.end()) return false;
    return it->second->onRequestVote(term, candidateId, candidateLastLogIndex, candidateLastLogTerm);
}

bool InMemoryRpc::appendEntries(int targetId, int term, int leaderId,
                                int prevLogIndex, int prevLogTerm,
                                const std::string& cmd, const std::string& content,
                                int entryTerm, int entryIndex, bool hasEntry,
                                int leaderCommit){
    auto it = id2node.find(targetId);
    if (it == id2node.end()) return false;
    return it->second->onAppendEntries(term, leaderId, prevLogIndex, prevLogTerm,
                                       cmd, content, entryTerm, entryIndex, hasEntry, leaderCommit);
}

bool InMemoryRpc::appendEntriesHeartbeat(int targetId, int leaderTerm, int leaderId, int leaderCommit){
    auto it = id2node.find(targetId);
    if (it == id2node.end()) return false;
    // 转为无日志的 appendEntries（心跳）
    return it->second->onAppendEntries(leaderTerm, leaderId, -1, 0,
                                       std::string(), std::string(), 0, -1, false, leaderCommit);
}