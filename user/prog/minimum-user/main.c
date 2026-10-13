#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "minemu/user_abi.h"

static size_t msh_strlen(const char *s) {
    size_t count = 0;
    for (; s[count] != '\0'; count++) {}
    return count;
}

static void msh_write(const char *buf, size_t n) {
    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)buf, (uint32_t)n);
}

static void msh_puts(const char *s) {
    msh_write(s, msh_strlen(s));
}

/* Waits until one character has been typed and returns it. UART read never
 * blocks: it returns 1 when it copied a byte into x, 0 when nothing was
 * waiting yet, and a negative value on error, so keep asking until it is 1. */
static char msh_getc(void) {
    char x;

    for (;;) { //-ever
        int32_t got = ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)&x, 1U);

        if (got == 1) {
            return x;
        }
    }
}

/* The HW2 page says lines can be up to 20 bytes, but tests/hw2/msh-echo.toml sends a
 * 25-byte line ("echo hello from user mode") and expects all of it echoed, while
 * msh-overlong.toml sends 136 bytes and expects the line discarded. 64 satisfies both. */
#define MSH_LINE_MAX 64
#define MSH_MAX_ARGS 10

/* Splits line in place on runs of spaces, like strtok but with no hidden
 * state. Each returned token pointer points into line itself (now
 * NUL-terminated per token). Returns the number of tokens found (0 if line
 * was empty or all spaces). */
static size_t tokenize(char *line, char *tokens[], size_t max_tokens) {
    size_t count = 0;
    char *p = line;

    while (*p != '\0' && count < max_tokens) {
        while (*p == ' ') {
            p++; /* skip leading / repeated spaces */
        }
        if (*p == '\0') {
            break;
        }

        tokens[count++] = p;

        while (*p != ' ' && *p != '\0') {
            p++;
        }
        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }

    return count;
}

/* No <string.h> available, so a minimal strcmp-alike for exact matches. */
static int streq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/* Blocks until a full line (terminated by '\n') has been read into buffer,
 * which must have room for at least MSH_LINE_MAX + 1 bytes. Returns the
 * line length, not counting the terminating '\0'.
 *
 * Overlong rule: once the buffer holds MSH_LINE_MAX bytes, extra spaces are
 * dropped (trailing spaces change nothing), but any other extra character
 * marks the line as overlong and the whole line is discarded (returned as an
 * empty line, so the shell just prompts again). */
static size_t read_line(char *buffer) {
    size_t length = 0;
    bool overflowed = false;

    for (;;) {
        char c = msh_getc(); /* waits until a character arrives */

        if (c == '\n') {
            break;
        }

        if (c == 0x08 || c == 0x7f) {
            if (length > 0) {
                length--; /* backspace; ignored on an empty line */
            }
            continue;
        }

        if (length < MSH_LINE_MAX) {
            buffer[length++] = c;
        } else if (c != ' ') {
            overflowed = true; /* a real character past the limit */
        }
    }

    if (overflowed) {
        length = 0; /* discard the overlong line */
    }
    buffer[length] = '\0';
    return length;
}

/* Parses one already-read, NUL-terminated line and runs the matching
 * command, or reports it as unknown. Mutates line (tokenize splits it in
 * place). */
static void dispatch(char *line) {
    char *tokens[MSH_MAX_ARGS];
    size_t count = tokenize(line, tokens, MSH_MAX_ARGS);

    if (count == 0) {
        return; /* blank line (or all spaces): nothing to do */
    }

    if (streq(tokens[0], "echo")) {
        for (size_t i = 1; i < count; i++) {
            if (i > 1) {
                msh_puts(" ");
            }
            msh_puts(tokens[i]);
        }
        msh_puts("\n");
    } else {
        /* no printf in user space: write the message in three pieces */
        msh_puts("command not found: ");
        msh_puts(tokens[0]);
        msh_puts("\n");
    }
}

void minemu_user_main(void) {
    char line[MSH_LINE_MAX + 1];

    /* Never returns: returning would call _exit, which this kernel does not support. */
    for (;;) {
        msh_puts("msh> ");
        read_line(line);
        dispatch(line);
    }
}
