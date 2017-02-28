#ifndef XSERIAL_PORT_H
#define XSERIAL_PORT_H

#ifdef WIN32
#include "xwin_comm.h"
#else
#include "xyc_linux_com.h"
#endif

void test_serial_port(char * b);

#endif // XSERIAL_PORT_H
