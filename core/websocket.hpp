/// Websocket protocol interface
///
/// (c) Koheron

#ifndef __WEBSOCKET_HPP__
#define __WEBSOCKET_HPP__

#include <string>

#include "kserver_defs.hpp"
#include "config.hpp"
#include "commands.hpp"

namespace kserver {

class SysLog;

class WebSocket
{
  public:
    WebSocket(std::shared_ptr<KServerConfig> config_, SysLog& syslog_);

    void set_id(int comm_fd_);
    int authenticate();
    int receive();                  // General purpose data reception
    int receive_cmd(Command& cmd);  // Specialization to receive a command

    /// Send binary blob
    template<class T> int send(const T *data, unsigned int len);

    char* get_payload_no_copy() {return payload;}
    int64_t payload_size() const {return header.payload_size;}
    
    bool is_closed() const {return connection_closed;}

    int exit();

  private:
    std::shared_ptr<KServerConfig> config;
    SysLog& syslog;

    int comm_fd;

    // Buffers
    int read_str_len;
    char read_str[WEBSOCK_READ_STR_LEN];
    char *payload;
    unsigned char sha_str[21];
    unsigned char send_buf[WEBSOCK_SEND_BUF_LEN];

    void reset_read_buff();

    std::string http_packet;

    struct {
        unsigned int header_size;
        int mask_offset;
        int64_t payload_size;
        bool fin;
        bool masked;
        unsigned char opcode;
        unsigned char res[3];
    } header;

    bool connection_closed;

    enum OpCode {
        CONTINUATION_FRAME = 0x0,
        TEXT_FRAME         = 0x1,
        BINARY_FRAME       = 0x2,
        CONNECTION_CLOSE   = 0x8,
        PING               = 0x9,
        PONG               = 0xA
    };

    enum StreamSize {
        SMALL_STREAM  = 125,
        MEDIUM_STREAM = 126,
        BIG_STREAM    = 127
    };

    enum HeaderSize {
        SMALL_HEADER  = 6,
        MEDIUM_HEADER = 8,
        BIG_HEADER    = 14
    };

    enum MaskOffset {
        SMALL_OFFSET  = 2,
        MEDIUM_OFFSET = 4,
        BIG_OFFSET    = 10
    };

    // Internal functions
    int read_http_packet();
    int decode_raw_stream();
    int decode_raw_stream_cmd(Command& cmd);
    int read_stream();
    int read_header();
    int check_opcode(unsigned int opcode);
    int read_n_bytes(int64_t bytes, int64_t expected);

    int set_send_header(unsigned char *bits, long long data_len,
                        unsigned int format);
    int send_request(const std::string& request);
    int send_request(const unsigned char *bits, long long len);
};

template<class T>
inline int WebSocket::send(const T *data, unsigned int len)
{
    if (connection_closed)
        return 0;

    long unsigned int char_data_len = len * sizeof(T) / sizeof(char);

    if (char_data_len + 10 > WEBSOCK_SEND_BUF_LEN)
        return -1;

    int mask_offset = set_send_header(send_buf, char_data_len, (1 << 7) + BINARY_FRAME);
    memcpy(&send_buf[mask_offset], data, char_data_len);
    return send_request(send_buf, mask_offset + char_data_len);
}

} // namespace kserver

#endif // __WEBSOCKET_HPP__
