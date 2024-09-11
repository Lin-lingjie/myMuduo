#pragma once

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   // 记录数据包长度
    static const size_t kInitialSize = 1024; // 初始大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
    {
    }

    // 可读数据量
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    // 可写数据量
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    // 头部预留
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 缓冲区中可读数据的起始地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    // 复位
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // 应用只读取了可读缓冲区数据的一部分，就是len，还剩下readerIndex_ += len -> writerIndex_
        }
        else // len == readableBytes()
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        // 读取数据
        std::string result(peek(), len);
        // 缓冲区复位
        retrieve(len); 
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin()
    {
        // it.operator*()
        return &*buffer_.begin(); // vector底层数组首元素的地址
    }
    const char *begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        // 可读区域那边读了一块能用，那块加上都不够的话
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    // 下个要读取数据的起始位置索引
    size_t readerIndex_;
    // 可写数据的起始位置索引
    size_t writerIndex_;
};