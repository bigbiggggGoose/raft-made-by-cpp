#include "raft.h"
#include <thread>
#include <chrono>
#include <unordered_map>

// 简单的内存总线 RPC 实现
class InMemoryRpc : public IRaftRpc {
public:
  std::unordered_map<int, raft*> id2node;
  bool requestVote(int targetId, int term, int candidateId,
                   int candidateLastLogIndex, int candidateLastLogTerm) override {
    auto it = id2node.find(targetId);
    if (it == id2node.end()) return false;
    return it->second->onRequestVote(term, candidateId, candidateLastLogIndex, candidateLastLogTerm);
  }
  bool appendEntriesHeartbeat(int targetId, int leaderTerm, int leaderId) override {
    auto it = id2node.find(targetId);
    if (it == id2node.end()) return false;
    return it->second->onHeartbeat(leaderTerm, leaderId);
  }
};


int main(){
   InMemoryRpc bus;
   raft n1; n1.setId(1); n1.setRpc(&bus);
   raft n2; n2.setId(2); n2.setRpc(&bus);
   raft n3; n3.setId(3); n3.setRpc(&bus);

   bus.id2node[1] = &n1; bus.id2node[2] = &n2; bus.id2node[3] = &n3;

   n1.addPeerId(2); n1.addPeerId(3);
   n2.addPeerId(1); n2.addPeerId(3);
   n3.addPeerId(1); n3.addPeerId(2);

   n1.start(); n2.start(); n3.start();

   // 运行一段时间，观察谁成为 leader
   std::this_thread::sleep_for(std::chrono::milliseconds(1500));

   // 模拟 leader 宕机：找到 leader 并置为 false
   if(n1.getRole() == "leader") n1.setAlive(false);
   else if(n2.getRole() == "leader") n2.setAlive(false);
   else n3.setAlive(false);

   std::this_thread::sleep_for(std::chrono::milliseconds(1200));

   n1.stop(); n2.stop(); n3.stop();
   return 0;
}