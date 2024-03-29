#ifndef EXAMPLE_UTILS_H
#define EXAMPLE_UTILS_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <stdlib.h>
#include <initializer_list>

#include "dnnl.hpp"
#include "dnnl_debug.h"

// Read from memory, write to handle
inline void read_from_dnnl_memory(void *handle, dnnl::memory &mem) {
    dnnl::engine eng = mem.get_engine();
    size_t size = mem.get_desc().get_size();

    if (!handle) throw std::runtime_error("handle is nullptr.");

    if (eng.get_kind() == dnnl::engine::kind::cpu) {
        uint8_t *src = static_cast<uint8_t *>(mem.get_data_handle());
        if (!src) throw std::runtime_error("get_data_handle returned nullptr.");
        for (size_t i = 0; i < size; ++i)
            ((uint8_t *)handle)[i] = src[i];
        return;
    }

    assert(!"not expected");
}

// Read from handle, write to memory
inline void write_to_dnnl_memory(void *handle, dnnl::memory &mem) {
    dnnl::engine eng = mem.get_engine();
    size_t size = mem.get_desc().get_size();

    if (!handle) throw std::runtime_error("handle is nullptr.");

    if (eng.get_kind() == dnnl::engine::kind::cpu) {
        uint8_t *dst = static_cast<uint8_t *>(mem.get_data_handle());
        if (!dst) throw std::runtime_error("get_data_handle returned nullptr.");
        for (size_t i = 0; i < size; ++i)
            dst[i] = ((uint8_t *)handle)[i];
        return;
    }

    assert(!"not expected");
}

inline const char *engine_kind2str_upper(dnnl::engine::kind kind) {
    if (kind == dnnl::engine::kind::cpu) return "CPU";
    if (kind == dnnl::engine::kind::gpu) return "GPU";
    assert(!"not expected");
    return "<Unknown engine>";
}

dnnl::engine::kind validate_engine_kind(dnnl::engine::kind akind) {
    // Checking if a GPU exists on the machine
    if (akind == dnnl::engine::kind::gpu) {
        if (dnnl::engine::get_count(dnnl::engine::kind::gpu) == 0) {
            std::cout << "Application couldn't find GPU, please run with CPU "
                         "instead.\n";
            exit(0);
        }
    }
    return akind;
}

inline dnnl::engine::kind parse_engine_kind(
        int argc, char **argv, int extra_args = 0) {
    // Returns default engine kind, i.e. CPU, if none given
    if (argc == 1) {
        return validate_engine_kind(dnnl::engine::kind::cpu);
    } else if (argc <= extra_args + 2) {
        std::string engine_kind_str = argv[1];
        // Checking the engine type, i.e. CPU or GPU
        if (engine_kind_str == "cpu") {
            return validate_engine_kind(dnnl::engine::kind::cpu);
        } else if (engine_kind_str == "gpu") {
            return validate_engine_kind(dnnl::engine::kind::gpu);
        }
    }

    // If all above fails, the example should be ran properly
    std::cout << "Inappropriate engine kind." << std::endl
              << "Please run the example like this: " << argv[0] << " [cpu|gpu]"
              << (extra_args ? " [extra arguments]" : "") << "." << std::endl;
    exit(1);
}

// Exception class to indicate that the example uses a feature that is not
// available on the current systems. It is not treated as an error then, but
// just notifies a user.
struct example_allows_unimplemented : public std::exception {
    example_allows_unimplemented(const char *message) noexcept
        : message(message) {}
    const char *what() const noexcept override { return message; }
    const char *message;
};

// Runs example function with signature void() and catches errors.
// Returns `0` on success, `1` or oneDNN error, and `2` on example error.
inline int handle_example_errors(
        std::initializer_list<dnnl::engine::kind> engine_kinds,
        std::function<void()> example) {
    int exit_code = 0;

    try {
        example();
    } catch (example_allows_unimplemented &e) {
        std::cout << e.message << std::endl;
        exit_code = 0;
    } catch (dnnl::error &e) {
        std::cout << "oneDNN error caught: " << std::endl
                  << "\tStatus: " << dnnl_status2str(e.status) << std::endl
                  << "\tMessage: " << e.what() << std::endl;
        exit_code = 1;
    } catch (std::exception &e) {
        std::cout << "Error in the example: " << e.what() << "." << std::endl;
        exit_code = 2;
    }

    std::string engine_kind_str;
    for (auto it = engine_kinds.begin(); it != engine_kinds.end(); ++it) {
        if (it != engine_kinds.begin()) engine_kind_str += "/";
        engine_kind_str += engine_kind2str_upper(*it);
    }

    std::cout << "Example " << (exit_code ? "failed" : "passed") << " on "
              << engine_kind_str << "." << std::endl;
    return exit_code;
}

// Same as above, but for functions with signature void(dnnl::engine::kind).
inline int handle_example_errors(
        std::function<void(dnnl::engine::kind)> example,
        dnnl::engine::kind engine_kind) {
    return handle_example_errors(
            {engine_kind}, [&]() { example(engine_kind); });
}

inline dnnl::memory::dim product(const dnnl::memory::dims &dims) {
    return std::accumulate(dims.begin(), dims.end(), (dnnl::memory::dim)1,
            std::multiplies<dnnl::memory::dim>());
}

#endif