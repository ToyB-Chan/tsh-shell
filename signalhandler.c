#include "signalhandler.h"

bool g_abortRequested = false;

void SignalHandlerAbort(int signal)
{
	g_abortRequested = true;
}