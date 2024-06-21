#pragma once

#include <signal.h>
#include <stdbool.h>

extern bool g_abortRequested;

void SignalHandlerAbort(int signal);
