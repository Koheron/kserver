/// @file {{ device.class_name|lower }}.hpp
///
/// (c) Koheron 2014-2015 

#ifndef __{{ device.class_name|upper }}_HPP__
#define __{{ device.class_name|upper }}_HPP__

{% for include in device.includes -%}
#include <{{ include }}>
{% endfor -%}
#include <drivers/core/dev_mem.hpp>

#if KSERVER_HAS_THREADS
#include <mutex>
#endif

#include "../core/kdevice.hpp"
#include "../core/devices_manager.hpp"

namespace kserver {

class {{ device.class_name }} : public KDevice<{{ device.class_name }},{{ device.name }}>
{
  public:
    const device_t kind = {{ device.name }};
    enum { __kind = {{ device.name }} };

  public:
    {{ device.class_name }}(KServer* kserver, Klib::DevMem& dev_mem_)
    : KDevice<{{ device.class_name }},{{ device.name }}>(kserver)
    , dev_mem(dev_mem_)
    {% if device.objects|length -%}
    {% for object in device.objects -%}
    , {{ object["name"] }}(dev_mem_)
    {% endfor -%}
    {% endif -%}
    {}

    enum Operation {
        {% for operation in device.operations -%}
        {{ operation["name"] }},
        {% endfor -%}        
        {{ device.name|lower }}_op_num
    };

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif

    Klib::DevMem& dev_mem;
    {% if device.objects|length -%}
    {% for object in device.objects -%}
    {{ object["type"] }} {{ object["name"] }};
    {% endfor -%}
    {% endif %}
}; // class KS_{{ device.name|capitalize }}

{% for operation in device.operations -%}
template<>
template<>
struct KDevice<{{ device.class_name }},{{ device.name }}>::
            Argument<{{ device.class_name }}::{{ operation["name"] }}>
{
{%- macro print_param_line(arg) %}
        {{ arg["type"] }} {{ arg["name"]}}; ///< {{ arg["description"] }}
{%- endmacro -%}
{% for arg in operation["arguments"] -%}
{% if not ("flag" in arg and arg["flag"] == 'CLIENT_ONLY') -%}
  {#- TODO : implement elementary type check -#}
  {% if not "cast" in arg -%}
    {% if ("description" in arg) and (arg["description"] != None) -%}
    {{ print_param_line(arg) }}
    {% else -%}
    {{ arg["type"] }} {{ arg["name"]}};
    {% endif -%}
  {% else -%}
    {% if ("description" in arg) and (arg["description"] != None)-%}
    {{ arg["cast"] }} {{ arg["name"]}}; ///< {{ arg["description"] }}
    {% else -%}
    {{ arg["cast"] }} {{ arg["name"]}};
    {% endif -%}
  {% endif -%}
{% endif -%}
{% endfor -%}
};

{% endfor -%}

} // namespace kserver

#endif //__{{ device.class_name|upper }}_HPP__

