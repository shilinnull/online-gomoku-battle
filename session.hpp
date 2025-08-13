#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include "Util.hpp"
#include <unordered_map>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;

class session
{
public:
    session(uint64_t ssid) : _ssid(ssid) { DLOG("SESSION %p 被创建！！", this); }
    ~session() { DLOG("SESSION %p 被释放！！", this); }
    uint64_t ssid() { return _ssid; }
    void set_statu(ss_statu statu) { _statu = statu; }
    void set_user(uint64_t uid) { _uid = uid; }
    uint64_t get_user() { return _uid; }
    bool is_login() { return (_statu == LOGIN); }
    void set_timer(const wsserver_t::timer_ptr &tp) { _tp = tp; }
    wsserver_t::timer_ptr &get_timer() { return _tp; }

private:
    uint64_t _ssid;            // 唯一标识符
    uint64_t _uid;             // session对应的用户id
    ss_statu _statu;           // 用户状态：未登录，已登录
    wsserver_t::timer_ptr _tp; // session关联的定时器
};

#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
using session_ptr = std::shared_ptr<session>;

class session_manager
{
public:
    session_manager(wsserver_t *srv)
        : _next_ssid(1), _server(srv)
    {
        DLOG("session管理器初始化完毕！");
    }
    ~session_manager() { DLOG("session管理器即将销毁！"); }

    session_ptr create_session(uint64_t uid, ss_statu statu)
    {
        std::unique_lock<std::mutex> _lock;
        session_ptr ssp(new session(_next_ssid++)); // 创建一个session
        ssp->set_statu(statu);
        ssp->set_user(uid);

        _session.insert(std::make_pair(ssp->ssid(), ssp)); // 添加进行管理
        return ssp;
    }

    void append_session(const session_ptr &ssp)
    {
        std::unique_lock<std::mutex> _lock;
        _session.insert(std::make_pair(ssp->ssid(), ssp));
    }

    session_ptr get_session_by_ssid(uint64_t ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _session.find(ssid);
        if (it == _session.end())
            return session_ptr();
        return it->second;
    }
    void remove_session(uint64_t ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _session.erase(ssid);
    }
    void set_session_expire_time(uint64_t ssid, int ms)
    {
        session_ptr ssp = get_session_by_ssid(ssid);
        if (ssp.get() == nullptr)
            return;
        wsserver_t::timer_ptr tp = ssp->get_timer();
        if (tp.get() == nullptr && ms == SESSION_FOREVER)
        {
            // 1. 在session永久存在的情况下，设置永久存在
            return;
        }
        else if (tp.get() == nullptr && ms != SESSION_FOREVER)
        {
            // 2. 在session永久存在的情况下，设置指定时间之后被删除的定时任务
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
                std::bind(&session_manager::remove_session, this, ssid));
            ssp->set_timer(tmp_tp);
        }
        else if (tp.get() != nullptr && ms == SESSION_FOREVER)
        {
            // 3. 在session设置了定时删除的情况下，将session设置为永久存在
            // 删除定时任务--- stready_timer删除定时任务会导致任务直接被执行
            tp->cancel(); // 不会立即取消

            // 需要重新给session管理器中，添加一个session信息, 且添加的时候需要使用定时器，而不是立即添加
            ssp->set_timer(wsserver_t::timer_ptr()); // 将session关联的定时器设置为空
            _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));
        }
        else if (tp.get() != nullptr && ms != SESSION_FOREVER)
        {
            // 4. 在session设置了定时删除的情况下，将session重置删除时间
            tp->cancel(); // 因为这个取消定时任务并不是立即取消的
            ssp->set_timer(wsserver_t::timer_ptr());
            _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));

            // 重新给session添加定时销毁任务
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
                std::bind(&session_manager::remove_session, this, ssp->ssid()));
            // 重新设置session关联的定时器
            ssp->set_timer(tmp_tp);
        }
        else
        {
            ELOG("未知错误!");
        }
    }

private:
    uint64_t _next_ssid;
    std::mutex _mutex;
    std::unordered_map<uint64_t, session_ptr> _session;
    wsserver_t *_server;
};

#endif