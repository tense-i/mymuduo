private:
    using EventList = std::vector<epoll_event>;
    static const int kInitEventListSize = 16;
    // epoll句柄
    int epollfd_;
    // epoll的事件数组
    EventList events_;
    // 初始化事件数组的长度