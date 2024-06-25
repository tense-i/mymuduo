#pragma once

#include <vector>
#include <string>

#include <algorithm>

class Buffer
{
private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

public:
    static const size_t kCheapPrepend = 8;   // 预留8字节的空间
    static const size_t kInitialSize = 1024; // 初始大小为1KB
    explicit Buffer(size_t initialSize = kInitialSize);
    ~Buffer();

    size_t readableBytes() const;

    size_t writableBytes() const;

    size_t prependableBytes() const;

    const char *peek() const;

    void retrieve(size_t len);

    void retrieveAll();

    std::string retrieveAllAsString();

    std::string retrieveAllAsString(size_t len);

    void ensureWritableBytes(size_t len);

    void append(const char *data, size_t len);

    char *beginWrite();

    ssize_t readFd(int fd, int *savedErrno);

    ssize_t writeFd(int fd, int *savedErrno);

private:
    const char *begin() const;

    char *begin();

    void makeSpace(size_t len);
};
