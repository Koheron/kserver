/// Utilities to parse binary buffers
///
/// (c) Koheron

#ifndef __BINARY_PARSER_HPP__
#define __BINARY_PARSER_HPP__

#include <tuple>

namespace kserver {

template<typename Tp>
Tp parse(char *buff);

template<typename Tp>
constexpr size_t size_of;

// Specializations
template<>
uint16_t parse<uint16_t>(char *buff)
{
    return buff[1] + (buff[0] << 8);
}

template<>
constexpr size_t size_of<uint16_t> = 2;

template<>
uint32_t parse<uint32_t>(char *buff)
{
    return buff[3] + (buff[2] << 8) + (buff[1] << 16) + (buff[0] << 24);
}

template<>
constexpr size_t size_of<uint32_t> = 4;

template<>
float parse<float>(char *buff)
{
    return static_cast<float>(parse<uint32_t>(buff));
}

template<>
constexpr size_t size_of<float> = 4;

template<>
bool parse<bool>(char *buff)
{
    return buff[0] == 0;
}

template<>
constexpr size_t size_of<bool> = 1;

template<size_t position = 0, typename Tp0, typename... Tp>
inline typename std::enable_if<0 == sizeof...(Tp), std::tuple<Tp0, Tp...>>::type
parse_buffer(char *buff)
{
    return std::make_tuple(parse<Tp0>(&buff[position]));
}

template<size_t position = 0, typename Tp0, typename... Tp>
inline typename std::enable_if<0 < sizeof...(Tp), std::tuple<Tp0, Tp...>>::type
parse_buffer(char *buff)
{
    return std::tuple_cat(std::make_tuple(parse<Tp0>(&buff[position])),
                          parse_buffer<position + size_of<Tp0>, Tp...>(buff));
}

} // namespace kserver

#endif // __BINARY_PARSER_HPP__