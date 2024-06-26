#pragma once
#include <stdint.h>
#include <stddef.h>

using SlipHandler = void(*)(char*, size_t);

template<SlipHandler handler, size_t Buff = 256>
struct Slip {
    static constexpr char END = char(0xC0);
    static constexpr char ESC = char(0xDB);
    static constexpr char EscapedEnd = char(0xDC);
    static constexpr char EscapedEsc = char(0xDD);
protected:
    char buff[Buff];
    size_t ptr = 0;
    bool esc = 0;
    bool err = false;

    void handleChar(char ch) noexcept {
        if (ptr >= sizeof(buff)) {
            ptr = 0;
            err = 1;
        } else if (err && ch == END) {
            ptr = 0;
            err = false;
        } else if (esc) {
            esc = false;
            switch (ch) {
            case EscapedEsc:
                buff[ptr++] = ESC;
                break;
            case EscapedEnd: {
                buff[ptr++] = END;
                break;
            }
            default: {
                err = true;
            }
            }
        } else if (ch == END) {
            handler(buff, ptr);
            ptr = 0;
        } else if (ch == ESC) {
            esc = true;
        } else {
            buff[ptr++] = ch;
        }
    }
public:
    Slip() = default;
    void Read() {
        int ch = 0;
        while ((ch = Serial.read()) != -1) {
            handleChar(ch);
        }
    }
    static void Write(const char* data, size_t size) {
        size_t av = Serial.availableForWrite();
        for (size_t i = 0; i < size; ++i) {
            while (av < 4) {
                av = Serial.availableForWrite();
            }
            char ch = data[i];
            switch (ch) {
            case END: {
                Serial.write(ESC);
                Serial.write(EscapedEnd);
                break;
            }
            case ESC: {
                Serial.write(ESC);
                Serial.write(EscapedEsc);
                break;
            }
            default: {
                Serial.write(ch);
                break;
            }
            }
        }
        Serial.write(END);
        Serial.flush();
    }
};
