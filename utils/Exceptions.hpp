#pragma once

#include <sstream>
#include <system_error>

#if DEBUG
#define THROW_LINE __func__ << "[" << __LINE__ << "]: "
#else
#define THROW_LINE ""
#endif

#define THROW_IF($cond, $mess) if ($cond) { THROW($mess); }
#define THROW($mess) {                                  \
    std::stringstream $st;                              \
    $st << THROW_LINE << $mess;                         \
    throw std::runtime_error($st.str());                \
}

#define THROW_ERRNO_IF($cond, $mess) if ($cond) { THROW_ERRNO($mess); }
#define THROW_ERRNO($mess) THROW_ERRNO_ERROR(errno, $mess);
#define THROW_ERRNO_ERROR($errno, $mess) {                      \
    std::error_code $ec($errno, std::generic_category());       \
    std::stringstream $st;                                      \
    $st << THROW_LINE << $mess;                                 \
    throw std::system_error($ec, $st.str());                    \
}
