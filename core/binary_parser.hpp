/// Utilities to parse binary buffers
///
/// (c) Koheron 

#ifndef __BINARY_PARSER_HPP__
#define __BINARY_PARSER_HPP__

#include <cstring>
#include <tuple>

#include "commands.hpp"

namespace kserver {

// ------------------------
// Type parsing
// ------------------------

template<typename Tp>
Tp parse(const char *buff);

template<typename Tp>
constexpr size_t size_of;

// http://stackoverflow.com/questions/17789928/whats-a-proper-way-of-type-punning-a-float-to-an-int-and-vice-versa
template <typename T, typename U>
inline T pseudo_cast(const U &x)
{
    T to = T(0);
    std::memcpy(&to, &x, (sizeof(T) < sizeof(U)) ? sizeof(T) : sizeof(U));
    return to;
}

// -- Specializations

template<>
inline uint16_t parse<uint16_t>(const char *buff)
{
    return (unsigned char)buff[1] + ((unsigned char)buff[0] << 8);
}

template<> constexpr size_t size_of<uint16_t> = 2;

template<>
inline uint32_t parse<uint32_t>(const char *buff)
{
    return (unsigned char)buff[3] + ((unsigned char)buff[2] << 8) 
           + ((unsigned char)buff[1] << 16) + ((unsigned char)buff[0] << 24);
}

template<> constexpr size_t size_of<uint32_t> = 4;

template<>
inline float parse<float>(const char *buff)
{
    static_assert(sizeof(float) == size_of<uint32_t>,
                  "Invalid float size");
    return pseudo_cast<float, uint32_t>(parse<uint32_t>(buff));
}

template<> constexpr size_t size_of<float> = 4;

template<>
inline bool parse<bool>(const char *buff)
{
    return (unsigned char)buff[0] == 1;
}

template<> constexpr size_t size_of<bool> = 1;

// ------------------------
// Buffer parsing
// ------------------------

// -- into tuple

namespace detail {

// Parse
template<size_t position, typename Tp0, typename... Tp>
inline std::enable_if_t<0 == sizeof...(Tp), std::tuple<Tp0, Tp...>>
parse_buffer(const char *buff)
{
    return std::make_tuple(parse<Tp0>(&buff[position]));
}

template<size_t position, typename Tp0, typename... Tp>
inline std::enable_if_t<0 < sizeof...(Tp), std::tuple<Tp0, Tp...>>
parse_buffer(const char *buff)
{
    return std::tuple_cat(std::make_tuple(parse<Tp0>(&buff[position])),
                          parse_buffer<position + size_of<Tp0>, Tp...>(buff));
}

// Required buffer size
template<typename Tp0, typename... Tp>
constexpr std::enable_if_t<0 == sizeof...(Tp), size_t>
required_buffer_size()
{
    return size_of<Tp0>;
}

template<typename Tp0, typename... Tp>
constexpr std::enable_if_t<0 < sizeof...(Tp), size_t>
required_buffer_size()
{
    return size_of<Tp0> + required_buffer_size<Tp...>();
}

} // namespace detail

template<typename... Tp>
constexpr size_t required_buffer_size()
{
    return detail::required_buffer_size<Tp...>();
}

template<size_t position, size_t len, typename... Tp>
inline std::tuple<Tp...> parse_buffer(const Buffer<len>& buff)
{
    static_assert(required_buffer_size<Tp...>() <= len - position, 
                  "Buffer size too small");

    return detail::parse_buffer<position, Tp...>(buff.data);
}

template<size_t position, typename... Tp>
inline std::tuple<Tp...> parse_buffer(const char *buff)
{
    return detail::parse_buffer<position, Tp...>(buff);
}

} // namespace kserver

#endif // __BINARY_PARSER_HPP__