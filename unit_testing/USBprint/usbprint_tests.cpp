/*!
** @file   arraymath_tests.cpp
** @author Luke W
** @date   17/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/* Fakes */
#include "fake_usbd_cdc.h"
#include "fake_stm32xxxx_hal.h"

/* Real supporting units */
#include "usb_cdc_fops.c"

/* UUT */
#include "USBprint.c"

using ::testing::AnyOf;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::IsSubsetOf;
using ::testing::IsSupersetOf;
using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

USBD_HandleTypeDef hUsbDeviceFS;

class UsbPrintTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        UsbPrintTest() {
            /* Minimal setup required for usb_cdc_fops.c */
            hUsbDeviceFS.pClassData = (void*) malloc(sizeof(USBD_CDC_HandleTypeDef));
            ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState = 0;

            /* Only required to prevent asserts from triggering */
            hUsbDeviceFS.classId = 0;
            hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId] = (void*)0x00000004U;

            /* For manipulating lower level fake/mock - pass by defult */
            hUsbDeviceFS.ret_val = 0;
        }

        void sendUsbData(const char * data) {
            uint32_t len = strlen(data);
            char buf[20];
            memcpy(buf, data, len);
            usb_cdc_fops.Receive((uint8_t*)buf, &len);

            /* Buffer should be cleared after receive */
            for(int i = 0; i < len; i++) {
                ASSERT_EQ(0, buf[i]);
            }
        }
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(UsbPrintTest, test_init) {
    EXPECT_EQ(-1, USBnprintf("Hello world!"));
    usb_cdc_fops.Init();
    EXPECT_NE(-1, USBnprintf("Hello world!"));
    EXPECT_READ_USB(Contains("Hello world!"));
}

TEST_F(UsbPrintTest, test_usbformatting) {
    usb_cdc_fops.Init();

    EXPECT_EQ(txAvailable(), 1024);
    EXPECT_EQ(7, USBnprintf("Testing"));
    
    /* For a handful of used format specifiers, check them */
    EXPECT_EQ(10, USBnprintf("Testing %u", 11));
    EXPECT_READ_USB(Contains("Testing 11"));

    EXPECT_EQ(12, USBnprintf("Testing %lu", 1122));
    EXPECT_READ_USB(Contains("Testing 1122"));

    EXPECT_EQ(11, USBnprintf("Testing %d", -12));
    EXPECT_READ_USB(Contains("Testing -12"));

    EXPECT_EQ(12, USBnprintf("Testing %x", 0x1234));
    EXPECT_READ_USB(Contains("Testing 1234"));

    EXPECT_EQ(8, USBnprintf("%8x", 0x1234));
    EXPECT_READ_USB(Contains("    1234"));

    EXPECT_EQ(6, USBnprintf("%06x", 0x1234));
    EXPECT_READ_USB(Contains("001234"));

    EXPECT_EQ(10, USBnprintf("%f", 392.65));
    EXPECT_READ_USB(Contains("392.650000"));

    EXPECT_EQ(8, USBnprintf("%.4f", 392.65));
    EXPECT_READ_USB(Contains("392.6500"));
    
    EXPECT_EQ(9, USBnprintf("%09.4f", 987.654));
    EXPECT_READ_USB(Contains("0987.6540"));

    EXPECT_EQ(16, USBnprintf("Testing: %s", "Success"));
    EXPECT_READ_USB(Contains("Testing: Success"));

    EXPECT_EQ(txAvailable(), 1024);
}

TEST_F(UsbPrintTest, test_usbRecv) {
    usb_cdc_fops.Init();

    sendUsbData("Test String\n12345");

    uint8_t buf[20] = {0};
    int i = 0;
    while(usbRx(&buf[i++]) == 0) {}
    EXPECT_EQ(i - 1, 17);
    EXPECT_STREQ((char*) buf, "Test String\n12345");
}

TEST_F(UsbPrintTest, test_delayedSend) {
    usb_cdc_fops.Init();

    /* Make the USB "busy" */
    ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState = 1;

    EXPECT_EQ(txAvailable(), 1024);
    USBnprintf("test_delayed_send");
    /* Note additional 2 bytes are from CRLF USBnprintf adds */
    EXPECT_EQ(txAvailable(), 1024 - 17 - 2);
    USBnprintf("second message");
    EXPECT_EQ(txAvailable(), 1024 - 17 - 14 - 2 * 2);

    /* Free up the USB */
    ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState = 0;

    uint32_t len = 10;
    uint8_t buf[len];
    usb_cdc_fops.TransmitCplt(buf, &len, 0);

    EXPECT_EQ(txAvailable(), 1024);
    EXPECT_READ_USB(IsSupersetOf({"test_delayed_send\r", "second message"}));
}

TEST_F(UsbPrintTest, test_usb_connection) {
    forceTick(0);
    EXPECT_FALSE(isUsbPortOpen());
    USBD_SetupReqTypedef cmd = {
        .wValue = 0x0001
    };
    usb_cdc_fops.Control(CDC_SET_CONTROL_LINE_STATE, (uint8_t*)&cmd, sizeof(cmd));
    
    /* Note: it should take a few milliseconds before the port opens for "stabilisation" */
    forceTick(10);
    EXPECT_FALSE(isUsbPortOpen());
    forceTick(11);
    EXPECT_TRUE(isUsbPortOpen());
    
    cmd.wValue = 0;
    usb_cdc_fops.Control(CDC_SET_CONTROL_LINE_STATE, (uint8_t*)&cmd, sizeof(cmd));
    EXPECT_FALSE(isUsbPortOpen());
}

TEST_F(UsbPrintTest, test_usbFlush) {
    usb_cdc_fops.Init();

    sendUsbData("testFlush");
    char test;
    EXPECT_EQ(0, usbRx((uint8_t*)&test));
    EXPECT_EQ('t', test);

    usbFlush();
    EXPECT_EQ(-1, usbRx((uint8_t*)&test));
}

TEST_F(UsbPrintTest, test_usbErrors) {
    usb_cdc_fops.Init();

    /* Error 1 - Not enough space in send buffer */
    char buf[CIRCULAR_BUFFER_SIZE + 1] = {0};
    EXPECT_EQ(CIRCULAR_BUFFER_SIZE, writeUSB(buf, CIRCULAR_BUFFER_SIZE + 1));

    EXPECT_EQ(isUsbError(), CDC_ERROR_CROPPED_TRANSMIT);
    USBnprintf("clear");
    EXPECT_EQ(isUsbError(), CDC_ERROR_NONE);

    /* Error 2 - Something went wrong during transmit */
    hUsbDeviceFS.ret_val = 3;
    USBnprintf("err2");
    EXPECT_EQ(isUsbError(), CDC_ERROR_TRANSMIT);
    hUsbDeviceFS.ret_val = 0;
    USBnprintf("clear2");
    EXPECT_EQ(isUsbError(), CDC_ERROR_NONE);

    /* Error 3 - A transmit was delayed, but then there was a problem at a later stage */
    ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState = 1;
    USBnprintf("err3");
    EXPECT_EQ(isUsbError(), CDC_ERROR_NONE);
    
    hUsbDeviceFS.ret_val = 3;
    uint32_t len = 10;
    uint8_t buf2[len];
    usb_cdc_fops.TransmitCplt(buf2, &len, 0);
    EXPECT_EQ(isUsbError(), CDC_ERROR_DELAYED_TRANSMIT);
    ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState = 0;
    hUsbDeviceFS.ret_val = 0;
    USBnprintf("clear3");
    EXPECT_EQ(isUsbError(), CDC_ERROR_NONE);
}
