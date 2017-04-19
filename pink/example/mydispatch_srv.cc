#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <atomic>

#include "slash/include/xdebug.h"
#include "pink/include/pink_thread.h"
#include "pink/include/server_thread.h"

#include "myproto.pb.h"
#include "pink/include/pb_conn.h"

#include <google/protobuf/message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace pink;

class MyConn: public PbConn {
 public:
  MyConn(int fd, std::string ip_port, Thread *thread);
  virtual ~MyConn();
 protected:
  virtual int DealMessage();

 private:
  myproto::Ping ping_;
  myproto::PingRes ping_res_;
};

MyConn::MyConn(int fd, ::std::string ip_port, Thread *thread)
    : PbConn(fd, ip_port, thread) {
}

MyConn::~MyConn() {
}

int MyConn::DealMessage() {
  printf("In the myconn DealMessage branch\n");
  ping_.ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);
  ping_res_.Clear();
  ping_res_.set_res(11234);
  ping_res_.set_mess("heiheidfdfdf");
  printf ("DealMessage receive (%s)\n", ping_res_.mess().c_str());
  res_ = &ping_res_;
  set_is_reply(true);
  return 0;
}

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn *NewPinkConn(int connfd, const std::string &ip_port, Thread *thread) const {
    return new MyConn(connfd, ip_port, thread);
  }
};

static std::atomic<bool> running(false);

static void IntSigHandle(const int sig) {
  printf("Catch Signal %d, cleanup...\n", sig);
  running.store(false);
  printf("server Exit");
}

static void SignalSetup() {
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, &IntSigHandle);
  signal(SIGQUIT, &IntSigHandle);
  signal(SIGTERM, &IntSigHandle);
}

int main() {
  SignalSetup();
  ConnFactory *my_conn_factory = new MyConnFactory();
  ServerThread *st = NewDispatchThread(9211, 10, my_conn_factory, 1000);

  if (st->StartThread() != 0) {
    printf("StartThread error happened!\n");
    exit(-1);
  }
  running.store(true);
  while (running.load()) {
    sleep(1);
  }
  st->StopThread();

  delete st;
  delete my_conn_factory;

  return 0;
}
