/*!
** @file   fake_usbd_cdc.cpp
** @author Luke W
** @date   03/05/2024
*/

#include <fstream>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <sstream>

#include "fake_usbd_cdc.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define TX_RX_BUFFER_LENGTH 1024
#define TEST_OUT_FILENAME   "testout.txt"

/***************************************************************************************************
** NAMESPACES
***************************************************************************************************/

using namespace std;

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static ofstream test_out, test_in;
static stringstream test_ss;

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/


uint8_t USBD_CDC_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CDC_ItfTypeDef *fops) {
    return pdev->ret_val;
}

uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint32_t length) {
    pdev->tx_buf   = pbuff;
    pdev->tx_count = length;
    return pdev->ret_val;
}

uint8_t USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff) {
    /* Does nothing */
    return pdev->ret_val;
}

uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev) {
    /* Does nothing */
    return pdev->ret_val;
}

uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev) {
    /* For first time, open a new file */
    if(!test_out.is_open()) {
        test_out.open(TEST_OUT_FILENAME);
    }

    test_out.write((const char *)pdev->tx_buf, pdev->tx_count);
    test_out.flush();

    /* Also write the string to the internal copy buffer */
    test_ss.clear();
    test_ss << string((const char *)pdev->tx_buf, pdev->tx_count);
    
    return pdev->ret_val;
}

vector<string>* host_USBD_CDC_Receive(bool flush) {
    vector<string>* result = new vector<string>;
    string str;
    while(getline(test_ss, str)) {
        result->push_back(str);
    }

    if(!flush) {
        /* Put everything back into the stream */
        test_ss.clear();
        for(vector<string>::iterator it = result->begin(); it != result->end(); it++) {
            test_ss << *it;
        }
    }

    return result;
}