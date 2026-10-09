#include <cstddef>
#include <string>
#include <vector>
#include <unistd.h>
#include <type_traits>
#include <cstdint>

namespace
{

template<typename T>
struct is_bool
{
    static constexpr bool value = std::is_same<typename std::decay<T>::type, bool>::value;
};

template<typename T>
struct is_float
{
    static constexpr bool value = std::is_same<typename std::decay<T>::type, float>::value ||
                                  std::is_same<typename std::decay<T>::type, double>::value ||
                                  std::is_same<typename std::decay<T>::type, long double>::value;
};

template<typename T>
struct is_signed_integral
{
    static constexpr bool value = std::is_same<typename std::decay<T>::type, short>::value || std::is_same<typename std::decay<T>::type, int>::value ||
                                  std::is_same<typename std::decay<T>::type, long long>::value || std::is_same<typename std::decay<T>::type, long>::value;
};

template<typename T>
struct is_unsigned_integral
{
    static constexpr bool value = std::is_same<typename std::decay<T>::type, unsigned short>::value || std::is_same<typename std::decay<T>::type, unsigned int>::value ||
                                  std::is_same<typename std::decay<T>::type, unsigned long>::value || std::is_same<typename std::decay<T>::type, unsigned long long>::value;
};

template<typename T>
struct is_integral
{
    static constexpr bool value = is_signed_integral<T>::value || is_unsigned_integral<T>::value;
};

template<typename T>
struct is_charcter
{
    static constexpr bool value = std::is_same<typename std::decay<T>::type, char>::value || std::is_same<typename std::decay<T>::type, wchar_t>::value ||
                                  std::is_same<typename std::decay<T>::type, char16_t>::value || std::is_same<typename std::decay<T>::type, char32_t>::value ||
                                  std::is_same<typename std::decay<T>::type, signed char>::value || std::is_same<typename std::decay<T>::type, unsigned char>::value;
};

template<typename T>
struct is_cstr {
    static constexpr bool value = std::is_same<typename std::decay<T>::type, const char*>::value || std::is_same<typename std::decay<T>::type, char*>::value;
};

template<typename T>
struct is_pointer
{
    static constexpr bool value = false;
};

template<typename T>
struct is_pointer<T*>
{
    static constexpr bool value = !std::is_same<typename std::remove_cv<T>::type, char>::value;
};

template<typename T>
typename std::enable_if<is_cstr<T>::value, std::string>::type
convert_to_string(T x)
{
    return x ? std::string(x) : "(null)";
}

template<typename T, typename = void>
struct is_range
{
    static constexpr bool value = false;
};

template<typename T>
typename std::enable_if<std::is_same<typename std::decay<T>::type, std::string>::value, std::string>::type
convert_to_string(T x)
{
    return std::string(x);
}

template<typename T>
struct is_range<T,typename std::enable_if<!std::is_same<typename std::decay<T>::type,
                                           std::string>::value, decltype(std::declval<T>().begin(),
                                                                         std::declval<T>().end(),
                                                                         void())>::type>
{
    static constexpr bool value = true;
};

template <typename T>
struct is_convertable
{
    static constexpr bool value = is_integral<T>::value || is_charcter<T>::value ||
                                  is_float<T>::value || is_bool<T>::value;
};

template<typename T>
constexpr typename std::enable_if<is_bool<T>::value,std::string>::type
convert_to_string(T x)
{
    return x ? "true" : "false";
}

// This impl is C-style from Ester lib, i will change it latter
template<typename T>
constexpr typename std::enable_if<is_signed_integral<T>::value,std::string>::type
convert_to_string(T x)
{
    std::string str{};
    char temp[32];
    size_t i = 0;
    size_t value;

    if (x < 0)
    {
        str += '-';
        value = (size_t)(-(x + 1)) + 1;
    }
    else
    {
        value = (size_t)x;
    }

    do
    {
        temp[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (value);

    while (i)
        str += temp[--i];

    return str;
}

template<typename T>
constexpr typename std::enable_if<is_unsigned_integral<T>::value,std::string>::type
convert_to_string(T x)
{
    std::string str{};
    char temp[32];
    size_t i = 0;

    do
    {
        temp[i++] = (char)('0' + (x % 10));
        x /= 10;
    }
    while (x);

    while (i)
        str += temp[--i];

    return str;
}

template<typename T>
constexpr typename std::enable_if<is_charcter<T>::value,std::string>::type
convert_to_string(T x)
{
    return std::string(1, x);
}

template<typename T>
constexpr typename std::enable_if<is_float<T>::value,std::string>::type
convert_to_string(T x)
{
    if (x != x)
        return "nan";

    if (x > 1.0L / 0.0L)
        return "inf";

    if (x < -1.0L / 0.0L)
        return "-inf";

    std::string result;

    if (x < 0.0L) {
        result += '-';
        x = -x;
    }

    uint64_t integer = (uint64_t)x;
    long double fractional = x - (long double)integer;

    result += std::to_string(integer);
    result += '.';

    for (int i = 0; i < 18; ++i) {
        fractional *= 10.0L;

        uint32_t digit = (uint32_t)fractional;

        result += char('0' + digit);

        fractional -= (long double)digit;
    }

    while (!result.empty() && result.back() == '0')
        result.pop_back();

    if (!result.empty() && result.back() == '.')
        result.pop_back();

    return result;
}

template<typename T>
constexpr typename std::enable_if<is_pointer<T>::value,std::string>::type
convert_to_string(T x)
{
    uintptr_t value = reinterpret_cast<uintptr_t>(x);

    constexpr char hex[] = "0123456789abcdef";

    std::string str{"0x"};

    char temp[sizeof(uintptr_t) * 2];
    size_t i = 0;

    do {
        temp[i++] = hex[value & 0xF];
        value >>= 4;
    } while (value);

    while (i)
        str += temp[--i];

    return str;
}

template<typename T>
typename std::enable_if<is_range<T>::value, std::string>::type
convert_to_string(T x)
{
    std::string str{"["};
    bool first = true;

    for(auto it = x.begin();it != x.end(); it++)
    {
        if (!first) { str += ", "; }
        str += convert_to_string(*it);
        first = false;
    }
    return str += "]";
}

}//end namespace

template<typename... T>
std::string format(std::string str,T&&... args)
{
    std::string fmt {};
    std::vector<std::string> pack = {(convert_to_string(args))...};
    auto index = 0;

    for (auto const &x : str)
    {
        if (x == '@')
        {
            fmt += pack[index++];
        }else
        {
            fmt += x;
        }
    }

    fmt += "\n";

    return fmt;
}

template<typename... T>
void print(std::string str,T&&...args)
{
    std::string fmt = format(str,args...);
    [&](std::string x)
    {
        write(1, x.data(), x.size());
    }(fmt);
}

template<typename T>
void print(T str)
{
    std::string fmt = convert_to_string(str);
    [&](std::string x)
    {
        write(1, x.data(), x.size());
    }(fmt);
}

int main ()
{

    int i = -42;
    unsigned int ui = 123456789U;
    size_t sz = 987654321ULL;
    float f = 3.14159f;
    double d = 2.718281828459045;
    char c = 'X';
    const char *str = "Ahmed";
    const void *ptr = (const void *)&i;
    bool b = true;
    std::vector<std::string> vec{"my name is ","ahmed"};
    print("int=@ uint=@ size=@ float=@ double=@ char=@ string=@ ptr=@ bool=@ vec =@",i, ui, sz, f, d, c, str, ptr, b,vec);
}
