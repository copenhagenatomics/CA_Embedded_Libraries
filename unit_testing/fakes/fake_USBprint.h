/*!
** @file   fake_USBprint.h
** @author Luke W
** @date   22/01/2024
*/

#ifndef FAKE_USBPRINT_H_
#define FAKE_USBPRINT_H_

#include <vector>
#include <string>

#include "USBprint.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

using namespace std;

/* Allow a range of single line container tests */
#define EXPECT_READ_USB(x) { \
    vector<string> ss = hostUSBread(); \
    EXPECT_THAT(ss, (x)); \
}

/* Allow a range of single line container tests */
#define EXPECT_FLUSH_USB(x) { \
    vector<string> ss = hostUSBread(true); \
    EXPECT_THAT(ss, (x)); \
}

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void hostUSBprintf(const char * format, ...);
vector<string> hostUSBread(bool flush=false);

void hostUSBConnect();
void hostUSBDisconnect();
void itoa(int n, char* s, int radix);

vector<string> getChannelsFromLine(string& channel_line);

#endif /* FAKE_USBPRINT_H_ */
