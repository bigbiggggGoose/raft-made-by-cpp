#include "raft.h"
#include <thread>
#include <chrono>
#include <random>
#include <cstdint>
using namespace std::chrono;
std::mutex g_log_mtx;

raft::raft(){
    role = "follower";
    term = 0;
    leaderId = -1;
    heartbeatTimeout = 150;
    electionTimeout = 150;
    votedFor = -1;
    state = 0;
}
raft::~raft(){
    role = "follower";
    term = 0;
    leaderId = -1;
    heartbeatTimeout = 150;
    electionTimeout = 150;
    votedFor = -1;
    state = 0;
}
string raft::getRole(){
    return role;
}
int raft::getTerm(){
    return term;
}
int raft::getLeaderId(){
    return leaderId;
}
int raft::getHeartbeatTimeout(){
    return heartbeatTimeout;
}
int raft::getElectionTimeout(){
    return electionTimeout;
}
int raft::getVotedFor(){
    return votedFor;
}
int raft::getState(){
    return state;
}
int raft::getLog(int index){
    if(index >= 0 && index <= lastLogIndex) return logs[index].term;
    return 0;
}
void raft::setRole(string role){
    this->role = role;
}
void raft::setTerm(int term){
    this->term = term;
}
void raft::setLeaderId(int leaderId){
    this->leaderId = leaderId;
}
void raft::setHeartbeatTimeout(int heartbeatTimeout){
    this->heartbeatTimeout = heartbeatTimeout;
}
void raft::setElectionTimeout(int electionTimeout){
    this->electionTimeout = electionTimeout;
}
void raft::setVotedFor(int votedFor){
    this->votedFor = votedFor;
}
void raft::setState(int state){
    this->state = state;
}
void raft::setLog(int index, int log){
    if(index < 0 || index >= 1024) return;
    logs[index].index = index;
    logs[index].term = log;
    if(index > lastLogIndex){
        lastLogIndex = index;
        lastLogTerm = logs[index].term;
    }
}



void raft::init(){
    role = "follower";
    term = 0;
    leaderId = -1;
    heartbeatTimeout = 150;
    electionTimeout = 150;
    votedFor = -1;
    state = 0;
    lastLogIndex = -1;
    lastLogTerm = 0;
    commitIndex = -1;
    lastApplied = -1;
}

// ============ 新增：节点/集群辅助 ============
void raft::setId(int nid){ id = nid; }
int raft::getId() const { return id; }
void raft::addPeerId(int peerId){ if(peerId != id) peerIds.push_back(peerId); }
void raft::setRpc(IRaftRpc* r){ rpc = r; }

void raft::setAlive(bool ok){ alive.store(ok); }

void raft::start(){
    running.store(true);
    resetElectionTimer();
    electionThread = std::thread([this]{ this->startElectionLoop(); });
}

void raft::stop(){
    running.store(false);
    if(electionThread.joinable()) electionThread.join();
    if(heartbeatThread.joinable()) heartbeatThread.join();
}

void raft::resetElectionTimer(){  //
    using namespace std::chrono;
    static thread_local std::mt19937_64 rng(
        static_cast<uint64_t>(steady_clock::now().time_since_epoch().count())
        ^ (static_cast<uint64_t>(id) * 0x9e37ULL));
    std::uniform_int_distribution<int> dist(150, 450);
    lastHeartbeatTime = steady_clock::now();
    randomizedElectionTimeoutMs = electionTimeout + dist(rng);
}

void raft::startElectionLoop(){
    while(running.load()){
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if(!alive.load()) continue;

        auto now = steady_clock::now();
        auto msSince = duration_cast<milliseconds>(now - lastHeartbeatTime).count();
        if(msSince >= randomizedElectionTimeoutMs){
            this->election();
            resetElectionTimer();
        }
    }
}

void raft::startHeartbeatLoop(){
    while(running.load() && role == "leader" && alive.load()){
        for(int pid : peerIds){ if(rpc) rpc->appendEntriesHeartbeat(pid, term, id, commitIndex); }
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatTimeout));
    }
}

bool raft::onHeartbeat(int leaderTerm, int fromLeaderId){
    if(!alive.load()) return false;
    if(leaderTerm < term) return false;
    if(leaderTerm > term){
        term = leaderTerm;
        votedFor = -1;
    }
    role = "follower";
    leaderId = fromLeaderId;
    lastHeartbeatTime = steady_clock::now();
    return true;
}

bool raft::onAppendEntries(int rterm, int leaderIdIn,
                           int prevLogIndex, int prevLogTerm,
                           const std::string& cmd, const std::string& content,
                           int entryTerm, int entryIndex, bool hasEntry,
                           int leaderCommit){
    if(!alive.load()) return false;
    if(rterm < term) return false;
    if(rterm > term){ term = rterm; votedFor = -1; }
    role = "follower"; leaderId = leaderIdIn; lastHeartbeatTime = steady_clock::now();

    if(prevLogIndex >= 0){
        if(prevLogIndex > lastLogIndex) return false;
        if(logs[prevLogIndex].term != prevLogTerm) return false;
    }
    if(hasEntry){
        if(entryIndex <= lastLogIndex && logs[entryIndex].term != entryTerm){
            lastLogIndex = entryIndex - 1;
        }
        logs[entryIndex] = {entryIndex, entryTerm, cmd, content};
        if(entryIndex > lastLogIndex) lastLogIndex = entryIndex;
        lastLogTerm = logs[lastLogIndex].term;
    }
    if(leaderCommit > commitIndex){
        commitIndex = std::min(leaderCommit, lastLogIndex);
        applyCommitted();
    }
    return true;
}

void raft::applyCommitted(){
    while(lastApplied < commitIndex){
        lastApplied++;
        std::lock_guard<std::mutex> lk(g_log_mtx);
        std::cout << "Node " << id << " apply index=" << lastApplied
                  << " term=" << logs[lastApplied].term
                  << " cmd=" << logs[lastApplied].cmd
                  << " content=" << logs[lastApplied].content << std::endl;
    }
}

void raft::clientPropose(const std::string& cmd, const std::string& content){
    if(role != "leader") return;
    int newIndex = lastLogIndex + 1;
    logs[newIndex] = {newIndex, term, cmd, content};
    lastLogIndex = newIndex; lastLogTerm = term;

    int acks = 1;
    for(int pid : peerIds){
        if(!rpc) continue;
        bool ok = rpc->appendEntries(pid, term, id,
                                     newIndex-1, (newIndex-1>=0?logs[newIndex-1].term:0),
                                     cmd, content, term, newIndex, true,
                                     commitIndex);
        if(ok) acks++;
    }
    int clusterSize = static_cast<int>(peerIds.size()) + 1;
    if(acks > clusterSize/2){
        commitIndex = newIndex;
        applyCommitted();
    }
}
 
 
// ============ 选举与投票 ============
// 投票请求处理：返回是否同意投票
bool raft::onRequestVote(int candidateTerm, int candidateId,
                         int candidateLastLogIndex, int candidateLastLogTerm){
    // 如果候选人 term 比我小，拒绝
    if(candidateTerm < term) return false;
    // 如果候选人 term 更大，更新本地任期并转为 follower
    if(candidateTerm > term){
        term = candidateTerm;
        role = "follower";
        votedFor = -1;
        leaderId = -1;
    }

    // 日志新旧比较：先比 lastLogTerm，再比 lastLogIndex
    bool upToDate = (candidateLastLogTerm > lastLogTerm) ||
                    (candidateLastLogTerm == lastLogTerm && candidateLastLogIndex >= lastLogIndex);

    if((votedFor == -1 || votedFor == candidateId) && upToDate){
        votedFor = candidateId; // 记录投票
        return true;
    }
    return false;
}

// 主动发起选举
void raft::election(){
    // 转为 candidate，提升任期并给自己投票
    role = "candidate";
    int myTerm = term + 1; 
    term =myTerm;
    votedFor = id;
    int votes = 1; // 自投一票

    // 广播 RequestVote 给 peers
    for(int pid : peerIds){
        if(rpc && rpc->requestVote(pid, myTerm, id, lastLogIndex, lastLogTerm)) votes += 1;
    }

    // 多数票当选
    int clusterSize = static_cast<int>(peerIds.size()) + 1;
    if(votes > clusterSize/2&& role == "candidate" && term == myTerm){
        term = myTerm;
        becomeLeader();
    }else{
        // 失败则回到 follower（简化）
        role = "follower";
        // 重置计时等待下一轮
    }
}

void raft::becomeLeader(){
    role = "leader";
    leaderId = id;
    lastHeartbeatTime = std::chrono::steady_clock::now();   // 晋升为 Leader 时重置自身计时器
    // 打印并立即发送一次心跳
    {
        std::lock_guard<std::mutex> lock(g_log_mtx);
        std::cout << "Node " << id << " becomes leader at term " << term << std::endl;
    }
    for(int pid : peerIds){ if(rpc) rpc->appendEntriesHeartbeat(pid, term, id, commitIndex); }
    // 开启心跳线程
    if(heartbeatThread.joinable()) heartbeatThread.join();
    heartbeatThread = std::thread([this]{ this->startHeartbeatLoop(); });
}


