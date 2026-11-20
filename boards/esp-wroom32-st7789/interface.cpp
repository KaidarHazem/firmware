#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/device.h"
#include <interface.h>

#define BTN_UP   32
#define BTN_DOWN 33
#define BTN_OK   12
#define BTN_BACK 21

void _setup_gpio() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_OK, INPUT_PULLUP);
    pinMode(BTN_BACK, INPUT_PULLUP);

    bruceConfigPins.rotation = 2;
    bruceConfigPins.irTx = 4;
    bruceConfigPins.i2c_bus = {(gpio_num_t)22, (gpio_num_t)-1};
    bruceConfigPins.rfTx = 4;
    bruceConfigPins.rfRx = 2;
    bruceConfigPins.uart_bus = {(gpio_num_t)3, (gpio_num_t)1};
    bruceConfigPins.gps_bus = {(gpio_num_t)3, (gpio_num_t)1};
    bruceConfigPins.badusb_bus = {(gpio_num_t)3, (gpio_num_t)1};

    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15};
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15, (gpio_num_t)2
    };
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15, (gpio_num_t)4
    };
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)5
    };

    bruceConfig.colorInverted = 1;
}

void _post_setup_gpio() {}

void _setBrightness(uint8_t brightval) {}

void InputHandler(void) {
    checkPowerSaveTime();

    if (digitalRead(BTN_UP) == LOW) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        PrevPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 180 && digitalRead(BTN_UP) == LOW);
    }

    if (digitalRead(BTN_DOWN) == LOW) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        NextPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 180 && digitalRead(BTN_DOWN) == LOW);
    }

    if (digitalRead(BTN_OK) == LOW) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        SelPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 180 && digitalRead(BTN_OK) == LOW);
    }

    if (digitalRead(BTN_BACK) == LOW) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        EscPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 180 && digitalRead(BTN_BACK) == LOW);
    }
}

void powerOff() { esp_deep_sleep_start(); }
void goToDeepSleep() { esp_deep_sleep_start(); }
void checkReboot() {}
