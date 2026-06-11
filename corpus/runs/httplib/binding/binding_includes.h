// Plain C++ includes for the httplib run (both compilers parse this).
// httptest.h pulls in the library single header httplib.h plus the EchoServer
// loopback fixture (its own <atomic>/<string>/<thread>). No ^^, no [[=...]].
#pragma once
#include "httptest.h"
