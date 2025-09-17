#include "raft.h"
#include <thread>
#include <chrono>


int main(){
   raft n1; n1.setId(1);
   raft n2; n2.setId(2);
   raft n3; n3.setId(3);

   n1.addPeer(&n2); n1.addPeer(&n3);
   n2.addPeer(&n1); n2.addPeer(&n3);
   n3.addPeer(&n1); n3.addPeer(&n2);

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