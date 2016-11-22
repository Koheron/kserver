/// Commands received by the server
///
/// (c) Koheron

#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include <array>

#include "session_manager.hpp"
#include "dev_definitions.hpp"
#include "serializer_deserializer.hpp"

namespace kserver {

template<size_t len>
struct Buffer
{
    constexpr Buffer(size_t position_ = 0) noexcept
    : position(position_)
    {};

    constexpr size_t size() const {return len;}

    void set()     {_data.fill(0);}
    char* data()   {return _data.data();}
    char* begin()  {return &(_data.data())[position];}

    // These functions are used by Websocket

    template<typename... Tp>
    std::tuple<Tp...> deserialize() {
        static_assert(required_buffer_size<Tp...>() <= len, "Buffer size too small");

        const auto tup = kserver::deserialize<0, Tp...>(begin());
        position += required_buffer_size<Tp...>();
        return tup;
    }

    template<typename T, size_t N>
    const std::array<T, N>& extract_array() {
        // http://stackoverflow.com/questions/11205186/treat-c-cstyle-array-as-stdarray
        const auto p = reinterpret_cast<const std::array<T, N>*>(begin());
        assert(p->data() == (const T*)begin());
        position += size_of<T, N>;
        return *p;
    }

    template<typename T>
    void to_vector(std::vector<T>& vec, uint64_t length) {
        const auto b = reinterpret_cast<const T*>(begin());
        vec.insert(vec.begin(), std::make_move_iterator(b),
                   std::make_move_iterator(b + length));
        position += length * sizeof(T);
    }

    void to_string(std::string& str, uint64_t length) {
        str.insert(str.begin(), std::make_move_iterator(begin()),
                   std::make_move_iterator(begin() + length));
        position += length;
    }

  private:
    std::array<char, len> _data;
    size_t position; // Current position in the buffer
};

struct Command
{
    Command() noexcept
    : header(HEADER_START)
    {}

    enum Header : uint32_t {
        HEADER_SIZE = 8,
        HEADER_START = 4  // First 4 bytes are reserved
    };

    SessID sess_id = -1;                    ///< ID of the session emitting the command
    SessionAbstract *sess;                  ///< Pointer to the session emitting the command
    device_t device = NO_DEVICE;            ///< The device to control
    int32_t operation = -1;                 ///< Operation ID

    Buffer<HEADER_SIZE> header;             ///< Raw data header
    Buffer<CMD_PAYLOAD_BUFFER_LEN> payload;
};

} // namespace kserver

#endif // __COMMANDS_HPP__

