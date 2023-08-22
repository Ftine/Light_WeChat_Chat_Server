#include "chatserver.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <json.hpp>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop) {
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(12);

    /* // 使用连接池
     * // 获取连接池实例
     * SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
     * 需要在回调函数中传递连接池的MYSQL*
     * 还需要对model层的所有方法进行重构，其实也就是在相关的操作中，加一个参加 MYSQL* mysql
     * 也就是使用 msgHandler(conn, js, time,sqlCon); 回调提供的sqlCon
     */

}

// 启动服务
void ChatServer::start() {
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    // 客户端断开链接
    if (!conn->connected()) {
        // 断开聊天 客户端网络退出退出群组
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time) {
    // 获取所有的数据TCP 缓冲数据
    string buf = buffer->retrieveAllAsString();

    // 测试，添加json打印代码
    cout << buf << endl;

    // 数据的反序列化
    json js = json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=》业务handler=》conn  js  time
    // ChatService 通过单例模式设计了处理不同的消息类型的方法
    // 我们获取到 js["msgid"] 获取到消息类型 然后调用ChatService::instance获取到了回调相关类的方法
    // 然后就可以解析消息
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);


    /* // 使用sqlRAII获取一个sql连接
     * // 获取连接池实例
     *
     * {
            MYSQL* sqlCon = nullptr;
        {
            SqlConnRAII sqlConn(&sql, connpool);
            // 回调消息绑定好的事件处理器，来执行相应的业务处理, 并传递连接池中sql连接
            msgHandler(conn, js, time,sqlCon);
        }
            // 在此处连接已自动释放
        }
     */


}