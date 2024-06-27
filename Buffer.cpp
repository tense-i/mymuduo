#include "Buffer.h"
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

Buffer::~Buffer()
{
}

/**
 * @brief 返回可读的字节数
 */
size_t Buffer::readableBytes() const
{
    return writerIndex_ - readerIndex_;
}

/**
 * @brief 返回可写的字节数。空闲块2

*/
size_t Buffer::writableBytes() const
{
    return buffer_.size() - writerIndex_;
}

/**
 * @brief 返回预留的字节数、大小为空闲块1+kCheapPrepend

*/
size_t Buffer::prependableBytes() const
{
    // 为什么返回readerIndex_，而不是kCheapPrepend？
    // 因为readerIndex_是真正的读指针，kCheapPrepend是预留的空间，不是真正的读指针
    return readerIndex_;
}

/**
 * @brief 返回缓冲区中可读数据的起始地址
 */
const char *Buffer::peek() const
{
    return begin() + readerIndex_;
}

/**
 *@brief 从缓冲区中读取部分数据,不读取真实数据，只是移动readerIndex_指针，读数据由应用层自己调用readFd()函数或retrieveAllAsString()函数
 */
void Buffer::retrieve(size_t len)
{
    if (len < readableBytes())
    {
        readerIndex_ += len; // 应用只读取了刻度缓冲区数据的一部分，就是len，还剩下readerIndex_ += len -> writerIndex_
    }
    else
    {
        retrieveAll();
    }
}
/**
 *@brief 从缓冲区中读取全部数据,不读取真实数据，只是移动readerIndex_指针，读数据由应用层自己调用readFd()函数或retrieveAllAsString()函数
 */
void Buffer::retrieveAll()
{
    readerIndex_ = writerIndex_ = kCheapPrepend;
}

/**
 * @brief 从缓冲区中读取全部数据,并返回读取的数据以string形式
 */
std::string Buffer::retrieveAllAsString()
{
    return retrieveAllAsString(readableBytes());
}

/**
 *@brief 从缓冲区中读取部分数据,并返回读取的数据以string形式

*/
std::string Buffer::retrieveAllAsString(size_t len)
{
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

/**
 * @brief 确保缓冲区中有足够的空间
 */
void Buffer::ensureWritableBytes(size_t len)
{
    // 空闲块2 < 待写入的数据长度
    if (writableBytes() < len)
    {
        makeSpace(len); // 扩容
    }
}

/**
 * @brief 将数据追加到缓冲区中,[data, data+len]内存上的数据，添加到writable缓冲区当中

*/
void Buffer::append(const char *data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

/**
 * @brief 返回可写的缓冲区的起始地址

*/
char *Buffer::beginWrite()
{
    return begin() + writerIndex_;
}

/**
 * @brief 从fd上读取数据  Poller工作在LT模式,Buffer缓冲区是有大小的！ 但是从fd上读数据的时候却不知道tcp数据最终的大小
 * @param saveErrno 保存错误码
 */
ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    char extrabuf[65536]; // 64KB栈空间
    bzero(extrabuf, sizeof(extrabuf));

    struct iovec vec[2];                     // 两块缓冲区,iovector
    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小

    // 第一块缓冲区、指向Buffer缓冲区
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    // 第二块缓冲区、指向extrabuf
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // 当前Buffer缓冲区的可写缓冲区大小足够存储读出来的数据、不使用extrabuf iovcnt=1
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;

    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (n <= writable) //// Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else // 使用了两块缓存区、extrabuf里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // writerIndex_开始写 n - writable大小的数据
    }
    return n;
}

/**
 * @brief 通过fd发送数据
 */
ssize_t Buffer::writeFd(int fd, int *savedErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *savedErrno = errno;
    }
    return n;
}

const char *Buffer::begin() const
{
    return &*buffer_.begin();
}

char *Buffer::begin()
{
    return &*buffer_.begin();
}

/**
 * @brief 扩容操作
 * @param len 待写入的数据长度
 */
void Buffer::makeSpace(size_t len)
{

    // 空闲块2+空闲块1+预留的空间 < 待写入的数据长度 + 预留的空间
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
        // 扩容
        buffer_.resize(writerIndex_ + len);
    }
    else // 空闲块2+空闲块1+预留的空间 >= 待写入的数据长度 + 预留的空间
    {    // 如果缓冲区有足够的空间但布局不合理（即，存在未使用的前置空间），则将可读数据前移
        // 将可读数据前移、合并空闲块1和空闲块2
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                  begin() + writerIndex_,
                  begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}
