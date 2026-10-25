#pragma once

// libVLC's headers use the POSIX ssize_t, which MSVC doesn't define.
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

#include <vlc/vlc.h>
