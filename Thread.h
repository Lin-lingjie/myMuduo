#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <unistd.h>
#include <atomic>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;  

    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const {return started_;}
    pid_t tid() const {return tid_; }
    const std::string& name() const{return name_; }
    static int numCreated() {return numCreated_; }
    
private:
    void setDefaultName();

    bool started_;
    bool joined_;    //等待进程完成
    std::shared_ptr<std::thread> thread_;   //通过智能指针来控制进程的开启等操作
    pid_t tid_;           //线程的Id
    ThreadFunc func_;    //存储线程函数
    std::string name_;
    static std::atomic_int numCreated_;  //记录线程创建数
};