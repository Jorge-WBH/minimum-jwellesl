#include <stdarg.h>

#include "minemu/platform.h"
#include "minemu/uart.h"

/* Writes the decimal/hex digits of an unsigned value, most significant first. */
static void print_unsigned(unsigned int value, unsigned int base) {
    static const char digits[] = "0123456789abcdef";
    char buffer[32];
    int i = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value != 0) {
        buffer[i++] = digits[value % base];
        value /= base;
    }

    while (i > 0) {
        uart_putc(buffer[--i]);
    }
}

static void print_int(int value) {
    if (value < 0) {
        uart_putc('-');
        /* cast to unsigned before negating to avoid overflow on INT_MIN */
        print_unsigned((unsigned int)(-(long)value), 10);
    } else {
        print_unsigned((unsigned int)value, 10);
    }
}

void mprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    for (const char *p = format; *p != '\0'; ++p) {
        if (*p != '%') {
            uart_putc(*p);
            continue;
        }

        ++p;
        switch (*p) {
        case 's':
            uart_print(va_arg(args, const char *));
            break;
        case 'd':
            print_int(va_arg(args, int));
            break;
        case 'x':
            print_unsigned(va_arg(args, unsigned int), 16);
            break;
        case 'c':
            uart_putc((char)va_arg(args, int));
            break;
        case '%':
            uart_putc('%');
            break;
        case '\0':
            /* trailing '%' at end of format string: print it and stop */
            uart_putc('%');
            va_end(args);
            return;
        default:
            /* unknown specifier: print it literally so mistakes are visible */
            uart_putc('%');
            uart_putc(*p);
            break;
        }
    }

    va_end(args);
}
