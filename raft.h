#pragma once
#include "IRaftRpc.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

extern std::mutex g_log_mtx;
using std::string;



class raft {
    struct Log
    {
        int index = -1;         // 日志记录的索引
        int term = 0;           // 日志记录的任期
        string cmd; 
        std::string content;    // 内容
    };

    // 节点元数据
    int id = -1;                // 节点 ID
    string role;                // follower/candidate/leader
    int term;                   // currentTerm
    int leaderId;               // 已知的 leaderId
    int heartbeatTimeout;       // 心跳超时（ms）
    int electionTimeout;        // 选举超时（ms）
    int votedFor;               // 当前任期投给谁
    int state;                  // 自定义状态位
    // 日志与提交状态（简化）
    Log logs[1024];
    int lastLogIndex = -1;      // 最后日志索引
    int lastLogTerm = 0;        // 最后日志任期
    int commitIndex = -1;       // 已提交最大索引
    int lastApplied = -1;       // 已应用最大索引

    // 集群：保存其他节点的 id，通过 RPC 调用
    std::vector<int> peerIds;
    IRaftRpc* rpc = nullptr;

    // 并发/定时
    std::atomic<bool> running{false};
    std::atomic<bool> alive{true};
    std::thread electionThread;
    std::thread heartbeatThread;
    std::mutex mtx;
    std::chrono::steady_clock::time_point lastHeartbeatTime;
    int randomizedElectionTimeoutMs = 400; // 运行时随机化

    void startElectionLoop();
    void startHeartbeatLoop();
    void resetElectionTimer();

public:
    raft();
    ~raft();

    // 基础 getter/setter
    string getRole();
    int getTerm();
    int getLeaderId();
    int getHeartbeatTimeout();
    int getElectionTimeout();
    int getVotedFor();
    int getState();
    int getLog(int index);
    void setRole(string role);
    void setTerm(int term);
    void setLeaderId(int leaderId);
    void setHeartbeatTimeout(int heartbeatTimeout);
    void setElectionTimeout(int electionTimeout);
    void setVotedFor(int votedFor);
    void setState(int state);
    void setLog(int index, int log);

    // 节点/集群辅助
    void setId(int nid);
    int getId() const;
    void addPeerId(int peerId);
    void setRpc(IRaftRpc* r);
    void setAlive(bool ok);
    bool isAlive() const { return alive.load(); }
    void start();
    void stop();

    // 生命周期
    void init();

    // 选举与投票
    void election();                 // 主动发起选举
    bool onRequestVote(int candidateTerm, int candidateId,
                       int candidateLastLogIndex, int candidateLastLogTerm);
    void becomeLeader();
    bool onHeartbeat(int leaderTerm, int fromLeaderId);
    // 追加日志处理（由 RPC 调用）
    bool onAppendEntries(int term, int leaderId,
                         int prevLogIndex, int prevLogTerm,
                         const std::string& cmd, const std::string& content,
                         int entryTerm, int entryIndex, bool hasEntry,
                         int leaderCommit);
    // 客户端在 leader 上提交命令（简化：单条）
    void clientPropose(const std::string& cmd, const std::string& content);
    void applyCommitted();

    // 其余接口（占位）
    void vote();
    void heartbeat();
    void logTransmit();
    void sycnLog();
    void processRequest();
    void test();
};