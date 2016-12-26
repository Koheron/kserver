/// PubSub
///
/// (c) Koheron

#ifndef __PUBSUB_HPP__
#define __PUBSUB_HPP__

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <type_traits>

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

#include "kserver_defs.hpp"
#include "signal_handler.hpp"

namespace kserver {

template<size_t channels_count>
struct Subscribers
{
    template<uint16_t channel>
    const auto& get() const {
        return std::get<channel>(_subscribers);
    }

    template<uint16_t channel>
    int count() const {
        return get<channel>().size();
    }

    int subscribe(uint16_t channel, SessID sid) {
        if (channel >= channels_count)
            return -1;

        // Check whether session hasn't already
        // subscribed before adding to subscribers.

        auto& s = _subscribers[channel];
        auto it = std::find(s.begin(), s.end(), sid);

        if (it == s.end())
            s.push_back(sid);

        return 0;
    }

    void unsubscribe(SessID sid) {
        for (auto& s : _subscribers) {
            // http://stackoverflow.com/questions/39912/how-do-i-remove-an-item-from-a-stl-vector-with-a-certain-value
            auto it = std::find(s.begin(), s.end(), sid);

            if (it != s.end()) {
                using std::swap;
                swap(*it, s.back());
                s.pop_back();
            }
        }
    }

  private:
    std::array<std::vector<SessID>, channels_count> _subscribers;
};

class SessionManager;

class PubSub
{
  public:
    PubSub(SessionManager& session_manager_,
           SignalHandler& sig_handler_)
    : session_manager(session_manager_)
    , sig_handler(sig_handler_)
    {
        memset(fmt_buffer, 0, FMT_BUFF_LEN);
    }

    // Session sid subscribes to a channel
    int subscribe(uint16_t channel, SessID sid) {
        return subscribers.subscribe(channel, sid);
    }

    // Must be called when a session is closed
    void unsubscribe(SessID sid) {
        subscribers.unsubscribe(sid);
    }

    // Event message structure
    // |      RESERVED     | CHANNEL |  EVENT  |   Arguments
    // |  0 |  1 |  2 |  3 |  4 |  5 |  8 |  9 | 12 | 13 | 14 | ...
    template<uint16_t channel, uint16_t event, typename... Args>
    int emit(Args&&... args);

    // This specialization emits a formated string
    template<uint16_t channel, uint16_t event, typename... Args>
    int emit(const char *str, Args&&... args);

    enum Channels {
        SERVER_CHANNEL,        ///< Server events
        SYSLOG_CHANNEL,        ///< Syslog events
        DEVICES_CHANNEL,       ///< Device notifications
        channels_count
    };

    enum ServerChanEvents {
        PING,                   ///< For tests
        PING_TEXT,              ///< For tests
        server_chan_events_num
    };

  private:
    SessionManager& session_manager;
    SignalHandler& sig_handler;
    Subscribers<channels_count> subscribers;

    static constexpr int32_t FMT_BUFF_LEN = 1024;
    char fmt_buffer[FMT_BUFF_LEN];
};

} // namespace kserver

#endif // __PUBSUB_HPP__