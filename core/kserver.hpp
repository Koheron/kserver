/// Server main class
///
/// (c) Koheron

#ifndef __KSERVER_HPP__
#define __KSERVER_HPP__

#include "kserver_defs.hpp"
#include "socket_interface_defs.hpp"

#if KSERVER_HAS_THREADS
#include <thread>
#include <mutex>
#endif

#include <array>
#include <vector>
#include <string>
#include <atomic>
#include <ctime>
#include <utility>

// #include "kdevice.hpp"
#include "devices_manager.hpp"
#include "syslog.hpp"
#include "signal_handler.hpp"
// #include "peer_info.hpp"
#include "session_manager.hpp"
#include "pubsub.hpp"
#include <context.hpp>

namespace kserver {

template<int sock_type> class Session;

////////////////////////////////////////////////////////////////////////////
/////// ListeningChannel

template<int sock_type>
struct ListenerStats
{
    int opened_sessions_num = 0; ///< Number of currently opened sessions
    int total_sessions_num = 0;  ///< Total number of sessions
    int total_requests_num = 0;  ///< Total number of requests
};

/// Implementation in listening_channel.cpp
template<int sock_type>
class ListeningChannel
{
  public:
    ListeningChannel(KServer *kserver_)
    : listen_fd(-1),
      kserver(kserver_)
    {
        num_threads.store(-1);
    }

    int init();
    void shutdown();

    /// True if the maximum of threads set by the config is reached
    bool is_max_threads();

    inline void inc_thread_num() { num_threads++; }
    inline void dec_thread_num() { num_threads--; }

    int start_worker();
#if KSERVER_HAS_THREADS
    void join_worker();
#endif

    int open_communication();

    /// Listening socket ID
    int listen_fd;

    /// Number of sessions using the channel
    std::atomic<int> num_threads;

#if KSERVER_HAS_THREADS
    std::thread comm_thread; ///< Listening thread
#endif

    KServer *kserver;
    ListenerStats<sock_type> stats;

  private:  
    int __start_worker();
}; // ListeningChannel

#if KSERVER_HAS_THREADS
template<int sock_type>
void ListeningChannel<sock_type>::join_worker()
{
    if (listen_fd >= 0)
        comm_thread.join();
}
#endif // KSERVER_HAS_THREADS

////////////////////////////////////////////////////////////////////////////
/////// KServer

/// Main class of the server. It initializes the 
/// connections and start the sessions.
///
/// Derived from KDevice, therefore it is seen as 
/// a device from the client stand point.

class KServer
{
  public:
    KServer(std::shared_ptr<kserver::KServerConfig> config_);

    int run();

    /// Operations associated to the device
    enum Operation {
        GET_VERSION = 0,            ///< Send th version of the server
        GET_CMDS = 1,               ///< Send the commands numbers
        GET_STATS = 2,              ///< Get KServer listeners statistics
        GET_DEV_STATUS = 3,         ///< Send the devices status
        GET_RUNNING_SESSIONS = 4,   ///< Send the running sessions
        SUBSCRIBE_PUBSUB = 5,       ///< Subscribe to a broadcast channel
        PUBSUB_PING = 6,            ///< Emit a ping to server broadcast subscribers
        kserver_op_num
    };

    std::shared_ptr<kserver::KServerConfig> config;
    SignalHandler sig_handler;

    std::atomic<bool> exit_comm;

    // Listeners
#if KSERVER_HAS_TCP
    ListeningChannel<TCP> tcp_listener;
#endif
#if KSERVER_HAS_WEBSOCKET
    ListeningChannel<WEBSOCK> websock_listener;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    ListeningChannel<UNIX> unix_listener;
#endif

    // Managers
    DeviceManager dev_manager;
    SessionManager session_manager;

    // Logs
    SysLog syslog;
    std::time_t start_time;

    PubSub pubsub;

#if KSERVER_HAS_THREADS
    std::mutex ks_mutex;
#endif

    Context ct;

    int execute(Command& cmd);
    template<int op> int execute_op(Command& cmd);

  private:
    // Internal functions
    int start_listeners_workers();
    void detach_listeners_workers();
    void join_listeners_workers();
    void close_listeners();

template<int sock_type> friend class ListeningChannel;
};

} // namespace kserver

#endif // __KSERVER_HPP__
