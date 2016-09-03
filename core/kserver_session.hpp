/// (c) Koheron

#ifndef __KSERVER_SESSION_HPP__
#define __KSERVER_SESSION_HPP__

#include <string>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <unistd.h>
#include <type_traits>

#include "commands.hpp"
#include "devices_manager.hpp"
#include "kserver_defs.hpp"
#include "kserver.hpp"
#include "peer_info.hpp"
#include "session_manager.hpp"
#include "serializer_deserializer.hpp"
#include "socket_interface_defs.hpp"

#if KSERVER_HAS_WEBSOCKET
#include "websocket.hpp"
#include "websocket.tpp"
#endif

namespace kserver {

/// Stores the permissions of a session
struct SessionPermissions
{
    /// True if the session can write into a device
    bool write = DFLT_WRITE_PERM;
    /// True if the session can read from a device
    bool read = DFLT_READ_PERM;
};

class SessionManager;

class SessionAbstract
{
  public:
    SessionAbstract(int sock_type_)
    : kind(sock_type_) {}

    template<typename... Tp> int send_cstr(const char *string, Tp&&... args);
    template<typename... Tp> std::tuple<int, Tp...> deserialize(Command& cmd);
    template<typename T, size_t N> std::tuple<int, const std::array<T, N>&> extract_array(Command& cmd);
    template<typename T> int rcv_vector(std::vector<T>& vec, uint64_t length, Command& cmd);
    template<typename... Tp> int send(const std::tuple<Tp...>& t);
    template<typename T, size_t N> int send(const std::array<T, N>& vect);
    template<typename T> int send(const std::vector<T>& vect);
    template<typename T> int send_array(const T* data, unsigned int len);
    template<class T> int send(const T& data);

    int kind;
};

/// Session
///
/// Receive and parse the client request for execution.
///
/// By calling the appropriate socket interface, it offers
/// an abstract communication layer to the devices. Thus
/// shielding them from the underlying communication protocol.
template<int sock_type>
class Session : public SessionAbstract
{
  public:
    Session(const std::shared_ptr<KServerConfig>& config_,
            int comm_fd, SessID id_, PeerInfo peer_info,
            SessionManager& session_manager_);

    int run();

    unsigned int request_num() const {return requests_num;}
    unsigned int error_num() const {return errors_num;}
    SessID get_id() const {return id;}
    const char* get_client_ip() const {return peer_info.ip_str;}
    int get_client_port() const {return peer_info.port;}
    std::time_t get_start_time() const {return start_time;}
    const SessionPermissions* get_permissions() const {return &permissions;}

    // Receive - Send

    // TODO Move in Session<TCP> specialization
    int rcv_n_bytes(char *buffer, uint64_t n_bytes);

    template<typename... Tp> std::tuple<int, Tp...> deserialize(Command& cmd);
    template<typename T, size_t N> std::tuple<int, const std::array<T, N>&> extract_array(Command& cmd);

    // The command is passed in argument since for the WebSocket the vector data
    // are stored into it. This implies that the whole vector is already stored on
    // the stack which might not be a good thing.
    //
    //The TCP won't use it as it reads directly the TCP buffer.
    template<typename T> int rcv_vector(std::vector<T>& vec, uint64_t length, Command& cmd);

    /// Send scalar data
    template<class T> int send(const T& data);

    template<typename... Tp> int send_string(const std::string& str, Tp&&... args);
    template<typename... Tp> int send_cstr(const char* string, Tp&&... args);

    template<typename T> int send_array(const T* data, unsigned int len);
    template<typename T> int send(const std::vector<T>& vect);
    template<typename T, size_t N> int send(const std::array<T, N>& vect);
    template<typename... Tp> int send(const std::tuple<Tp...>& t);

  private:
    std::shared_ptr<KServerConfig> config;
    int comm_fd;  ///< Socket file descriptor
    SessID id;
    SysLog *syslog_ptr;
    PeerInfo peer_info;
    SessionManager& session_manager;
    SessionPermissions permissions;

    struct EmptyBuffer {};
    std::conditional_t<sock_type == TCP || sock_type == UNIX,
            Buffer<KSERVER_RECV_DATA_BUFF_LEN>, EmptyBuffer> recv_data_buff;

#if KSERVER_HAS_WEBSOCKET
    struct EmptyWebsock {
        EmptyWebsock(std::shared_ptr<KServerConfig> config_, KServer *kserver_) {}
    };

    std::conditional_t<sock_type == WEBSOCK, WebSocket, EmptyWebsock> websock;
#endif

    // Monitoring
    unsigned int requests_num; ///< Number of requests received during the current session
    unsigned int errors_num;   ///< Number of requests errors during the current session
    std::time_t start_time;    ///< Starting time of the session

    // Internal functions
    int init_socket();
    int exit_socket();
    int init_session();
    void exit_session();
    int read_command(Command& cmd);

friend class SessionManager;
};

template<int sock_type>
Session<sock_type>::Session(const std::shared_ptr<KServerConfig>& config_,
                            int comm_fd_, SessID id_, PeerInfo peer_info_,
                            SessionManager& session_manager_)
: SessionAbstract(sock_type)
, config(config_)
, comm_fd(comm_fd_)
, id(id_)
, syslog_ptr(&session_manager_.kserver.syslog)
, peer_info(peer_info_)
, session_manager(session_manager_)
, permissions()
#if KSERVER_HAS_WEBSOCKET
, websock(config_, &session_manager_.kserver)
#endif
, requests_num(0)
, errors_num(0)
, start_time(0)
{}

template<int sock_type>
int Session<sock_type>::init_session()
{
    errors_num = 0;
    requests_num = 0;
    start_time = std::time(nullptr);
    return init_socket();
}

template<int sock_type>
void Session<sock_type>::exit_session()
{
    if (exit_socket() < 0)
        session_manager.kserver.syslog.print<SysLog::WARNING>(
        "An error occured during session exit\n");
}

template<int sock_type>
int Session<sock_type>::run()
{
    if (init_session() < 0)
        return -1;

    while (!session_manager.kserver.exit_comm.load()) {
        Command cmd;
        int nb_bytes_rcvd = read_command(cmd);

        if (session_manager.kserver.exit_comm.load())
            break;

        if (nb_bytes_rcvd <= 0) {

            // We don't call exit_session() here because the
            // socket is already closed.

            return nb_bytes_rcvd;
        }

        requests_num++;

        if (unlikely(session_manager.dev_manager.Execute(cmd) < 0))
            errors_num++;
    }

    exit_session();
    return 0;
}

template<int sock_type>
template<typename T>
int Session<sock_type>::send(const std::vector<T>& vect)
{
    // Length in front of vector data:
    // RESERVED | LENGTH_BYTES | DATA ...
    const auto& array = serialize<uint32_t, uint64_t>(0U, vect.size() * sizeof(T));
    std::vector<unsigned char> data(array.begin(), array.end());
    data.resize(required_buffer_size<uint32_t, uint64_t>() + vect.size() * sizeof(T));
    memcpy(data.data() + array.size(), vect.data(), vect.size() * sizeof(T));
    return send_array<unsigned char>(data.data(), data.size());
}

template<int sock_type>
template<typename T, size_t N>
int Session<sock_type>::send(const std::array<T, N>& vect)
{
    return send_array<T>(vect.data(), N);
}

// http://stackoverflow.com/questions/1374468/stringstream-string-and-char-conversion-confusion
template<int sock_type>
template<typename... Tp>
int Session<sock_type>::send(const std::tuple<Tp...>& t)
{
    const auto& arr = serialize(t);
    return send_array<unsigned char>(arr.data(), arr.size());
}

template<int sock_type>
template<typename... Tp>
int Session<sock_type>::send_cstr(const char *string, Tp&&... args)
{
    return send_string(std::move(std::string(string)), args...);
}

template<int sock_type>
template<typename... Tp>
int Session<sock_type>::send_string(const std::string& str, Tp&&... args)
{
    uint32_t len = str.size() + 1; // Including '\0'
    auto array = serialize(0U, args..., len);
    std::vector<unsigned char> data(array.begin(), array.end());
    std::copy(str.begin(), str.end(), std::back_inserter(data));
    data.push_back('\0');
    return send_array<unsigned char>(data.data(), data.size());
}

#define SEND_SPECIALIZE_IMPL(session_kind)                                            \
    template<> template<>                                                             \
    inline int session_kind::send<std::string>(const std::string& str)                \
    {                                                                                 \
        return send_string(str);                                                      \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<uint32_t>(const uint32_t& val)                      \
    {                                                                                 \
        return send_array<uint32_t>(&val, 1);                                         \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<bool>(const bool& val)                              \
    {                                                                                 \
        return send<uint32_t>(val);                                                   \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<int>(const int& val)                                \
    {                                                                                 \
        return send<uint32_t>(val);                                                   \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<uint64_t>(const uint64_t& val)                      \
    {                                                                                 \
        return send_array<uint64_t>(&val, 1);                                         \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<float>(const float& val)                            \
    {                                                                                 \
        return send_array<float>(&val, 1);                                            \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<double>(const double& val)                          \
    {                                                                                 \
        return send_array<double>(&val, 1);                                           \
    }

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP || KSERVER_HAS_UNIX_SOCKET

template<>
int Session<TCP>::rcv_n_bytes(char *buffer, uint64_t n_bytes);

template<>
template<typename T>
int Session<TCP>::rcv_vector(std::vector<T>& vec, uint64_t length, Command& cmd)
{
    vec.resize(length);
    return rcv_n_bytes(reinterpret_cast<char *>(vec.data()), length * sizeof(T));
}

template<>
template<typename... Tp>
std::tuple<int, Tp...> Session<TCP>::deserialize(Command& cmd)
{
    Buffer<required_buffer_size<Tp...>()> buff;
    int err = rcv_n_bytes(buff.data(), required_buffer_size<Tp...>());
    return std::tuple_cat(std::make_tuple(err), buff.deserialize<Tp...>());
}

template<>
template<typename T, size_t N>
std::tuple<int, const std::array<T, N>&> Session<TCP>::extract_array(Command& cmd)
{
    Buffer<size_of<T, N>> buff;
    int err = rcv_n_bytes(buff.data(), size_of<T, N>);
    return std::tuple_cat(std::make_tuple(err), std::forward_as_tuple(buff.extract_array<T, N>()));
}

template<>
template<class T>
int Session<TCP>::send_array(const T *data, unsigned int len)
{
    int bytes_send = sizeof(T) * len;
    int n_bytes_send = write(comm_fd, (void*)data, bytes_send);

    if (unlikely(n_bytes_send < 0)) {
       session_manager.kserver.syslog.print<SysLog::ERROR>(
          "TCPSocket::send_array: Can't write to client\n");
       return -1;
    }

    if (unlikely(n_bytes_send != bytes_send)) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "TCPSocket::send_array: Some bytes have not been sent\n");
        return -1;
    }

    if (config->verbose)
        session_manager.kserver.syslog.print_dbg("[S] [%u bytes]\n", bytes_send);

    return bytes_send;
}

SEND_SPECIALIZE_IMPL(Session<TCP>)

#endif // KSERVER_HAS_TCP

// -----------------------------------------------
// Unix socket
// -----------------------------------------------

#if KSERVER_HAS_UNIX_SOCKET
// Unix socket has the same interface than TCP socket
template<>
class Session<UNIX> : public Session<TCP>
{
  public:
    Session<UNIX>(const std::shared_ptr<KServerConfig>& config_,
                  int comm_fd_, SessID id_, PeerInfo peer_info_,
                  SessionManager& session_manager_)
    : Session<TCP>(config_, comm_fd_, id_, peer_info_, session_manager_) {}
};
#endif // KSERVER_HAS_UNIX_SOCKET

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET

template<>
template<typename T>
int Session<WEBSOCK>::rcv_vector(std::vector<T>& vec, uint64_t length, Command& cmd)
{
    if (length * sizeof(T) > CMD_PAYLOAD_BUFFER_LEN) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "WebSocket::rcv_vector: Payload size overflow\n");
        return -1;
    }

    cmd.payload.copy_to_vector(vec, length);
    return 0;
}

template<>
template<typename... Tp>
std::tuple<int, Tp...> Session<WEBSOCK>::deserialize(Command& cmd)
{
    return std::tuple_cat(std::make_tuple(0), cmd.payload.deserialize<Tp...>());
}

template<>
template<typename T, size_t N>
std::tuple<int, const std::array<T, N>&> Session<WEBSOCK>::extract_array(Command& cmd)
{
    return std::tuple_cat(std::make_tuple(0), std::forward_as_tuple(cmd.payload.extract_array<T, N>()));
}

template<>
template<class T>
inline int Session<WEBSOCK>::send_array(const T *data, unsigned int len)
{
    return websock.send(data, len);
}

SEND_SPECIALIZE_IMPL(Session<WEBSOCK>)

#endif // KSERVER_HAS_WEBSOCKET

// -----------------------------------------------
// Select session kind
// -----------------------------------------------

#if KSERVER_HAS_TCP
  #define CASE_TCP(...)                                             \
    case TCP:                                                       \
        return static_cast<Session<TCP>*>(this)-> __VA_ARGS__;
#else
  #define CASE_TCP(...)
#endif

#if KSERVER_HAS_UNIX_SOCKET
  #define CASE_UNIX(...)                                            \
    case UNIX:                                                      \
        return static_cast<Session<UNIX>*>(this)-> __VA_ARGS__;
#else
  #define CASE_UNIX(...)
#endif

#if KSERVER_HAS_WEBSOCKET
  #define CASE_WEBSOCK(...)                                         \
    case WEBSOCK:                                                   \
        return static_cast<Session<WEBSOCK>*>(this)-> __VA_ARGS__;
#else
  #define CASE_WEBSOCK(...)
#endif

#define SWITCH_SOCK_TYPE(...)     \
    switch (this->kind) {         \
      CASE_TCP(__VA_ARGS__)       \
      CASE_UNIX(__VA_ARGS__)      \
      CASE_WEBSOCK(__VA_ARGS__)   \
      default: assert(false);     \
    }

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885

template<class T>
int SessionAbstract::send(const T& data)
{
    SWITCH_SOCK_TYPE(template send<T>(data))
    return -1;
}

template<typename T> 
int SessionAbstract::send_array(const T* data, unsigned int len)
{
    SWITCH_SOCK_TYPE(template send_array<T>(data, len))
    return -1;
}

template<typename T>
int SessionAbstract::send(const std::vector<T>& vect)
{
    SWITCH_SOCK_TYPE(template send<T>(vect));
    return -1;
}

template<typename T, size_t N>
int SessionAbstract::send(const std::array<T, N>& vect)
{
    SWITCH_SOCK_TYPE(template send<T, N>(vect))
    return -1;
}

template<typename... Tp>
int SessionAbstract::send(const std::tuple<Tp...>& t)
{
    SWITCH_SOCK_TYPE(template send<Tp...>(t))
    return -1;
}

template<typename... Tp>
inline std::tuple<int, Tp...> SessionAbstract::deserialize(Command& cmd)
{
    SWITCH_SOCK_TYPE(template deserialize<Tp...>(cmd))
    return std::tuple_cat(std::make_tuple(-1), std::tuple<Tp...>());
}

template<typename T, size_t N>
inline std::tuple<int, const std::array<T, N>&> SessionAbstract::extract_array(Command& cmd)
{
    SWITCH_SOCK_TYPE(template extract_array<T, N>(cmd))
    return std::tuple_cat(std::make_tuple(-1), std::forward_as_tuple(std::array<T, N>()));
}

template<typename T>
inline int SessionAbstract::rcv_vector(std::vector<T>& vec, uint64_t length, Command& cmd)
{
    SWITCH_SOCK_TYPE(rcv_vector(vec, length, cmd))
    return -1;
}

template<typename... Tp>
inline int SessionAbstract::send_cstr(const char *string, Tp&&... args)
{
    SWITCH_SOCK_TYPE(send_cstr(string, args...))
    return -1;
}

// Cast abstract session unique_ptr
template<int sock_type>
Session<sock_type>*
cast_to_session(const std::unique_ptr<SessionAbstract>& sess_abstract)
{
    return static_cast<Session<sock_type>*>(sess_abstract.get());
}

} // namespace kserver

#endif // __KSERVER_SESSION_HPP__
