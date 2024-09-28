#include <caml/alloc.h>
#include <caml/bigarray.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iconv.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

enum class ErrorHandler
{
    interrupte,
    ignore,
    embed
};

struct CD
{
    iconv_t cd;
    CD(const char* from, const char* to)
    {
        iconv_t cd = iconv_open(to, from);
        if (cd == (iconv_t)-1) {
            throw std::runtime_error{"unsupported encoding"};
        }
        this->cd = cd;
    }
    operator iconv_t() const noexcept { return this->cd; }
};

class Converter
{
    CD cd;
    mutable std::string result;
    mutable char* in_ptr;
    mutable size_t in_left;
    static constexpr auto BUF_SIZE = 128;

public:
    Converter(const char* from, const char* to) : cd(from, to) {}

public:
    std::string conv(const char* input, size_t len, ErrorHandler eh) const
    {
        this->in_ptr = (char*)input;
        this->in_left = len;
        while (in_left != 0) {
            char buf[BUF_SIZE]{};
            char* out_ptr = buf;
            size_t out_left = BUF_SIZE;
            if (iconv(cd, &this->in_ptr, &this->in_left, &out_ptr, &out_left) == (size_t)-1 && errno != E2BIG) {
                this->errorHandler(eh);
            }
            this->append(buf, BUF_SIZE - out_left, eh);
        }
        auto res = std::move(this->result);
        new (&this->result) std::string{};
        return res;
    }

private:
    void errorHandler(ErrorHandler eh) const
    {
        switch (eh) {
        case ErrorHandler::ignore: {
            this->skipInvalidBytes();
        }
        case ErrorHandler::embed: {
            const size_t skip_len = this->skipInvalidBytes();
            this->result.append(1, '\0').append(this->in_ptr - skip_len, skip_len).append(1, '\0');
        }
        default:
            throw std::runtime_error{"invalid byte sequence"};
        }
    }

    void append(char* buf, size_t len, ErrorHandler eh) const
    {
        if (eh == ErrorHandler::embed) {
            for (size_t i = 0; i < len; i++) {
                if (buf[i] == 0) {
                    this->result.append(1, '\0');
                }
                this->result.append(1, buf[i]);
            }
        } else {
            this->result.append(buf, len);
        }
    }

    size_t skipInvalidBytes() const
    {
        // restore to initial state (for stateful conversion)
        iconv(this->cd, nullptr, nullptr, nullptr, nullptr);
        char buf[128];
        char* out_ptr = buf;
        size_t count = 0;
        while (this->in_left != 0) {
            const auto ptr = this->in_ptr;
            const auto left = this->in_left;
            size_t out_left = 128;
            if (iconv(cd, &this->in_ptr, &this->in_left, &out_ptr, &out_left) == (size_t)-1 && errno != E2BIG) {
                count++;
                this->in_ptr++;
                this->in_left--;
            } else {
                this->in_ptr = ptr;
                this->in_left = left;
                break;
            }
        }
        iconv(this->cd, nullptr, nullptr, nullptr, nullptr);
        return count;
    }
};
} // namespace

CAMLprim extern "C" value conv(value from, value to, value bytes, value error_handler)
{
    CAMLparam4(from, to, bytes, error_handler);
    CAMLlocal1(result);
    try {
        Converter cv{String_val(from), String_val(to)};
        auto output =
            cv.conv(String_val(bytes), caml_string_length(bytes), static_cast<ErrorHandler>(Int_val(error_handler)));
        result = caml_alloc_initialized_string(output.size(), output.data());
    } catch (std::runtime_error& e) {
        caml_invalid_argument(e.what());
    } catch (std::bad_alloc&) {
        caml_failwith("out of memory");
    }

    CAMLreturn(result);
}
