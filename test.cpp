#include "raft.h"
#include <thread>
#include <chrono>
#include <unordered_map>
#include "IRaftRpc.h"

int main(){
   InMemoryRpc bus;
   raft n1; n1.setId(1); n1.setRpc(&bus);
   raft n2; n2.setId(2); n2.setRpc(&bus);
   raft n3; n3.setId(3); n3.setRpc(&bus);
   raft n4; n4.setId(4); n4.setRpc(&bus);
   raft n5; n5.setId(5); n5.setRpc(&bus);

   bus.id2node[1] = &n1; bus.id2node[2] = &n2; bus.id2node[3] = &n3;bus.id2node[4] = &n4; bus.id2node[5] = &n5;
   n1.addPeerId(2); n1.addPeerId(3);n1.addPeerId(4); n1.addPeerId(5);
   n2.addPeerId(1); n2.addPeerId(3);n2.addPeerId(4); n2.addPeerId(5);
   n3.addPeerId(1); n3.addPeerId(2);n3.addPeerId(4); n3.addPeerId(5);
   n4.addPeerId(1); n4.addPeerId(2); n4.addPeerId(3);n4.addPeerId(5);
   n5.addPeerId(1); n5.addPeerId(2); n5.addPeerId(3);n5.addPeerId(4);
  n1.start(); n2.start(); n3.start();n4.start(); n5.start();
  // 等待选主
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // 当前 Leader
  raft* leader = nullptr;
  if(n1.getRole() == "leader") leader = &n1;
  else if(n2.getRole() == "leader") leader = &n2;
  else if(n3.getRole() == "leader") leader = &n3;
  else if(n4.getRole() == "leader") leader = &n4;
  else if(n5.getRole() == "leader") leader = &n5;

  // 在 Leader 上提交两条日志，观察各节点 apply
  if(leader){
    leader->clientPropose("set", "x=1");
    leader->clientPropose("set", "y=2");
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(800));

  // 宕机当前 Leader，触发重新选主
  if(leader) leader->setAlive(false);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // 新的 Leader（存活且为 leader）
  raft* leader2 = nullptr;
  if(n1.isAlive() && n1.getRole() == "leader") leader2 = &n1;
  else if(n2.isAlive() && n2.getRole() == "leader") leader2 = &n2;
  else if(n3.isAlive() && n3.getRole() == "leader") leader2 = &n3;
  else if(n4.isAlive() && n4.getRole() == "leader") leader2 = &n4;
  else if(n5.isAlive() && n5.getRole() == "leader") leader2 = &n5;

  // 新 Leader 再提交一条，验证已提交不丢、继续复制
  if(leader2){
    leader2->clientPropose("set", "z=3");
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1200));

   n1.stop(); n2.stop(); n3.stop(); n4.stop(); n5.stop();
   return 0;
}