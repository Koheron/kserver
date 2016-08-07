/// Broadcasting
///
/// (c) Koheron

#include "kserver_session.hpp"

namespace kserver {

template<uint32_t channel, uint32_t event, typename... Tp>
void Broadcast::emit(Tp&&... args)
{
    for (auto const& sid: subscribers)
        session_manager.get_session(sid).send(
            std::make_tuple(0U,   // RESERVED
                            channel,
                            event,
                            args...)
        );
}

} // namespace kserver
