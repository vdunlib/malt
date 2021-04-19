#define _GNU_SOURCE 1

#include <cstring>
#include <string>

#include "vdunlib/unix/SysError.hpp"

namespace vdunlib {

std::string sysError(int sysErrno) {
    char buf[1024];
    return std::string{strerror_r(sysErrno, buf, sizeof(buf))};
}

}
