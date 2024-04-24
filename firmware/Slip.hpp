#pragma once

namespace SLIP
{

static constexpr char END = char(0xC0);
static constexpr char ESC = char(0xDB);
static constexpr char EscapedEnd = char(0xDC);
static constexpr char EscapedEsc = char(0xDD);

struct Result {
    enum Status {
        Ok = 0,
        NoTerminator,
        DoubleEscape,
        InvalidEscape,
        UnterminatedEscape,
    };
    Status status = {};
    const char *begin = nullptr;
    const char *end = nullptr;
    
    Result(Status s, const char* b, const char* p) : 
      status(s), begin(b), end(p) 
    {}

    explicit operator bool() const noexcept {
      return status == Ok;
    }
};

inline Result DeEscape(char* begin, const char* end) {
    auto ptr = begin;
    bool esc = false;
    for(auto curr = begin; curr != end; ++curr) {
        char ch = *curr;
        switch(ch) {
          case ESC: {
            if (esc) {
                return Result{Result::DoubleEscape, begin, begin};
            }
            esc = true;
            continue;
          }
          case END: {
            if (esc) {
              return Result{Result::InvalidEscape, begin, begin};
            }
            return Result{Result::Ok, begin, ptr};
          }
        }
        if (esc) {
            esc = false;
            switch (ch) {
            case EscapedEnd: {
                *ptr++ = END;
                break;
            }
            case EscapedEsc: {
                *ptr++ = ESC;
                break;
            }
            default: {
                return Result{Result::InvalidEscape, begin, begin};
            }
            }
        } else {
            *ptr++ = ch;
        }
    }
    if (esc) {
        return Result{Result::UnterminatedEscape, begin, begin};
    }
    return Result{Result::NoTerminator, begin, begin};
}

}
