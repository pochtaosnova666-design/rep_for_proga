#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <cstdlib>
#include <cerrno>
using namespace std;
void printSizes() {
    cout << "bool: " << sizeof(bool) << "\nchar: " << sizeof(char) << endl;
    cout << "short int: " << sizeof(short) << "\nint: " << sizeof(int) << endl;
    cout << "long int: " << sizeof(long) << "\nlong long: " << sizeof(long long) << endl;
    cout << "float: " << sizeof(float) << "\ndouble: " << sizeof(double) << endl;
    cout << "long double: " << sizeof(long double) << endl;
}
// Порядок бпйтов x86/x64. Для bool показываем только логический бит.
void printBits(const void* number, int width, int type) {
    unsigned char bytes[16] = {};
    memcpy(bytes, number, (width + 7) / 8);
    bool real = type == 7 || type == 8;
    bool sign = type != 1 && (type != 2 || numeric_limits<char>::is_signed);
    int mantBits = type == 7 ? 23 : 52;
    for (int i = width - 1; i >= 0; --i) {
        if (i == width - 1) cout << (sign ? "[sign] " : "[bits] ");
        else if (real && i == width - 2) cout << " [exp] ";
        else if (real && i == mantBits - 1) cout << " [mant] ";
        else if (!real && sign && i == width - 2) cout << " [bits] ";
        cout << ((bytes[i / 8] >> (i % 8)) & 1);
        if (!real && i % 8 == 0 && i != 0) cout << ' ';
    }
    cout << endl;
    if (!real) return;
    uint64_t bits = 0;
    memcpy(&bits, bytes, width / 8);
    int bias = type == 7 ? 127 : 1023;
    int exponent = (int)((bits >> mantBits) & (2 * bias + 1));
    uint64_t mantissa = bits & ((1ull << mantBits) - 1);
    cout << "exponent=" << exponent << " mantissa=" << mantissa;
    if (exponent == 0 && mantissa == 0) cout << "  ноль";
    else if (exponent == 0) cout << "  денорм., E=" << 1 - bias;
    else if (exponent == 2 * bias + 1)
        cout << (mantissa ? "  NaN" : ((bits >> (width - 1)) ? "  -inf" : "  +inf"));
    else cout << "  E=" << exponent - bias;
    cout << endl;
}
void printValue(const void* number, int type) {
    cout.precision(type == 7 ? 9 : (type == 8 ? 17 : numeric_limits<long double>::max_digits10));
    cout << "Значение: ";
    switch (type) {
    case 1: cout << *(const bool*)number; break;
    case 2: cout << *(const char*)number << ", код: " << (int)*(const unsigned char*)number; break;
    case 3: cout << *(const short*)number; break;
    case 4: cout << *(const int*)number; break;
    case 5: cout << *(const long*)number; break;
    case 6: cout << *(const long long*)number; break;
    case 7: cout << *(const float*)number; break;
    case 8: cout << *(const double*)number; break;
    case 9: cout << *(const long double*)number; break;
    }
    cout << endl;
}
void printThreeCodes() {
    int32_t value; int width;
    cout << "Введите целое число (int32_t): ";
    if (!(cin >> value)) return;
    cout << "Разрядность от 2 до 32: ";
    if (!(cin >> width)) return;
    if (width < 2 || width > 32) { cout << "Неверная разрядность.\n"; return; }
    int64_t limit = (1LL << (width - 1)) - 1;
    if (value < -limit || value > limit) {
        cout << "Число не помещается в прямой и обратный коды.\n";
        return;
    }
    uint32_t mag = value < 0 ? (uint32_t)(-(int64_t)value) : (uint32_t)value;
    uint32_t mask = width == 32 ? 0xFFFFFFFFu : (1u << width) - 1;
    uint32_t sign = value < 0 ? 1u << (width - 1) : 0;
    uint32_t codes[] = {sign | mag, value < 0 ? sign | (~mag & (mask >> 1)) : mag, (uint32_t)value & mask};
    const char* names[] = {"Прямой: ", "Обратный: ", "Дополнительный: "};
    for (int i = 0; i < 3; ++i) {
        cout << names[i]; printBits(&codes[i], width, 4);
    }
}
void readReal(void* number, int type) {
    string text;
    if (!(cin >> text)) return;
    char* end;
    long double value;
    errno = 0;
    // strtof/strtod позволяют вводить денормплизованные числа и в MSVC.
    if (type == 7) {
        float f = strtof(text.c_str(), &end);
        value = f; memcpy(number, &f, sizeof(f));
    } else if (type == 8) {
        double d = strtod(text.c_str(), &end);
        value = d; memcpy(number, &d, sizeof(d));
    } else {
        value = strtold(text.c_str(), &end); memcpy(number, &value, sizeof(value));
    }
    if (end == text.c_str() || *end != '\0' ||
        (errno == ERANGE && (value == 0 || value > 1 || value < -1))) {
        cin.setstate(ios::failbit);
    }
}
void changeBit(void* number, int width, int type, int k, int action) {
    unsigned char bytes[16];
    memcpy(bytes, number, (width + 7) / 8);
    unsigned char mask = (unsigned char)(1u << (k % 8));
    if (action == 1) bytes[k / 8] |= mask;
    else if (action == 2) bytes[k / 8] &= (unsigned char)~mask;
    else bytes[k / 8] ^= mask;
    if (type == 1) *(bool*)number = bytes[0] != 0;
    else memcpy(number, bytes, (width + 7) / 8);
}
void invertSign(void* number, int width, int type) { changeBit(number, width, type, width - 1, 3); }
int main() {
    printSizes();
    cout << "Дробные числа вводите через точку.\n";
    while (true) {
        if (!cin) {
            if (cin.eof()) break;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Ошибка ввода. Повторите выбор.\n";
        }
        cout << "\n1 - биты, 2 - изменить бит, 3 - три кода, 4 - инверсия знака, 0 - выход\nВыбор: ";
        int action, type;
        if (!(cin >> action)) continue;
        if (action == 0) break;
        if (action < 1 || action > 4) { cout << "Неверный пункт.\n"; continue; }
        if (action == 3) { printThreeCodes(); continue; }
        if (action == 4) cout << "Инверсия знака: 1 - float, 2 - double: ";
        else cout << "1 - bool, 2 - char, 3 - short, 4 - int, 5 - long,\n6 - long long, 7 - float, 8 - double, 9 - long double: ";
        if (!(cin >> type)) continue;
        if (action == 4) {
            if (type < 1 || type > 2) { cout << "Неверный тип.\n"; continue; }
            type += 6;
        }
        if (type < 1 || type > 9) { cout << "Неверный тип.\n"; continue; }
        bool b; char c; short s; int n; long l; long long ll;
        float f; double d; long double ld;
        void* number = nullptr;
        int width = 0;
        cout << (type == 1 ? "Введите 0 или 1: " : type == 2 ? "Введите один символ ASCII: " : "Введите число: ");
        switch (type) {
        case 1:
            if (cin >> n) {
                if (n != 0 && n != 1) cin.setstate(ios::failbit);
                else b = n != 0;
            }
            number = &b; width = 1; break;
        case 2: cin >> c; number = &c; width = 8; break;
        case 3: cin >> s; number = &s; width = sizeof(s) * 8; break;
        case 4: cin >> n; number = &n; width = sizeof(n) * 8; break;
        case 5: cin >> l; number = &l; width = sizeof(l) * 8; break;
        case 6: cin >> ll; number = &ll; width = sizeof(ll) * 8; break;
        case 7: readReal(&f, type); number = &f; width = 32; break;
        case 8: readReal(&d, type); number = &d; width = 64; break;
        case 9: readReal(&ld, type); number = &ld; break;
        }
        if (!cin) continue;
        if (type == 9) {
            int digits = numeric_limits<long double>::digits;
            int exponent = numeric_limits<long double>::max_exponent;
            if (digits == 53 && exponent == 1024) width = 64;
            else if (digits == 64 && exponent == 16384) width = 80;
            else if (digits == 113 && exponent == 16384) width = 128;
            else { cout << "Этот формат long double не поддерживается.\n"; continue; }
        }
        printValue(number, type); printBits(number, width, type);
        if (action == 1) continue;
        if (action == 4) invertSign(number, width, type);
        else {
            int k, operation;
            cout << "Номер бита (0 - младший), от 0 до " << width << ": ";
            if (!(cin >> k)) continue;
            if (k < 0 || k >= width) { cout << "Неверный номер бита.\n"; continue; }
            cout << "1 - установить, 2 - сбросить, 3 - инвертировать: ";
            if (!(cin >> operation)) continue;
            if (operation < 1 || operation > 3) { cout << "Неверная операция.\n"; continue; }
            changeBit(number, width, type, k, operation);
        }
        cout << "После изменения:";
        printValue(number, type); printBits(number, width, type);
    }
    return 0;
}
