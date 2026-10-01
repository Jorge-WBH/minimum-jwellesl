#include <stddef.h>

#include "minemu/console.h"
#include "minemu/uart.h"

#define MSH_LINE_MAX 20
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
 * line length, not counting the terminating '\0'. */
static size_t read_line(char *buffer) {
    size_t length = 0;

    for (;;) {
        char c;
        if (!uart_rx_pop(&c)) {
            continue; /* nothing waiting yet; keep polling the ring buffer */
        }

        if (c == '\n') {
            break;
        }

        if ((c == 0x08 || c == 0x7f) && length > 0) {
            length--;
            continue;
        }

        if (length < MSH_LINE_MAX) {
            buffer[length++] = c;
        } else {
            continue;
        }
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
                uart_putc(' ');
            }
            uart_print(tokens[i]);
        }
        uart_putc('\n');
    } else {
        mprintf("command not found: %s\n", tokens[0]);
    }
}

void msh_run(void) {
    char line[MSH_LINE_MAX + 1];

    for (;;) {
        mprintf("msh> ");
        read_line(line);
        dispatch(line);
    }
}
