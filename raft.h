#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

using std::string;

class raft {
    struct Log
    {
        int index = -1;         // 日志记录的索引
        int term = 0;           // 日志记录的任期
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
    int log[1000];              // 简化日志占位
    int lastLogIndex = -1;      // 简化：记录最后日志索引
    int lastLogTerm = 0;        // 简化：记录最后日志任期

    // 简易集群（本地模拟）：保存其他节点指针
    std::vector<raft*> peers;

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
    void addPeer(raft* peer);
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

    // 其余接口（占位）
    void vote();
    void heartbeat();
    void logTransmit();
    void sycnLog();
    void processRequest();
    void test();
};