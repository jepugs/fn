// base.hpp -- common error handling code for fn

#ifndef __FN_BASE_HPP
#define __FN_BASE_HPP

#include <cstdint>
#include <forward_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace fn {

/// aliases imported from std
template<class T> using forward_list = std::forward_list<T>;
template<class T> using optional = std::optional<T>;
template<class T> using vector = std::vector<T>;
using string = std::string;

template<class T> using shared_ptr = std::shared_ptr<T>;
template<class T> using unique_ptr = std::unique_ptr<T>;

/// integer/float typedefs by bitwidth
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// since we're using u64 for our pointers, we better have 64-bit pointers
static_assert(sizeof(uintptr_t) == 8);

// we also assume we have 64-bit double and 32-bit float
static_assert(sizeof(float) == 4);
typedef float f32;
static_assert(sizeof(double) == 8);
typedef double f64;

// this is implemented as needed
template<typename T> u32 hash(const T& v);

/// semantic typedefs
// addresses on the stack
typedef u16 stack_addr;
// addresses in the current call frame (i.e. arguments and local variables)
typedef u8 local_addr;
// 32-bit integers represent addresses in the bytecode
typedef u32 bc_addr;
// used to identify local variables and upvalues
typedef u8 local_id;
// used to identify bytecode constant
typedef u16 const_id;
// used to identify symbols
typedef u32 symbol_id;

// debugging information
struct source_loc {
    const std::shared_ptr<string> filename;
    const int line;
    const int col;

    source_loc(const string& filename, int line=1, int col=1)
        : filename{new string{filename}}
        , line{line}
        , col{col} {
    }
    source_loc(const char* filename, int line=1, int col=1)
        : filename{new string(filename)}
        , line{line}
        , col{col} {
    }
    source_loc(const std::shared_ptr<string>& filename, int line, int col)
        : filename{filename}
        , line{line}
        , col{col} {
    }
    source_loc(const source_loc& loc)
        : filename{loc.filename}
        , line{loc.line}
        , col{loc.col} {
    }
};

class fn_error : public std::exception {
    // pointer to the formatted error message. need this to ensure that the return value of what()
    // is properly cleaned up when the object is destroyed.
    string *formatted;

    public:
    const string subsystem;
    const string message;
    const source_loc origin;

    // t_od_o: move this to a .cpp file so we don't need to include sstream
    fn_error(const string& subsystem, const string& message, const source_loc& origin)
        : subsystem(subsystem)
        , message(message)
        , origin(origin) {
        // build formatted error message
        std::ostringstream ss;
        ss << "[" + subsystem + "] error at line " << origin.line << ",col " << origin.col
           << " in " << *origin.filename << ":\n\t" << message;
        formatted = new string(ss.str());
        
    }
    ~fn_error() {
        delete formatted;
    }

    const char* what() const noexcept override {
        return formatted->c_str();
    }

};

    
}

#endif
