#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include "grail.h"

#define DEBUG
#define TEMP_BUFSIZE 256

#define let static
#define global static


global InternCmd *interns;
global char lookahead;
global char *stream;
global char expr[TEMP_BUFSIZE] = "";


void *prealloc(void *ptr, size_t num_bytes)
{
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("prealloc failed");
        exit(1);
    }

    return ptr;
}

void *pmalloc(size_t num_bytes)
{
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("pmalloc failed");
        exit(1);
    }

    return ptr;
}

void *buf__grow(const void *buf, size_t new_len, size_t elem_size)
{    
    size_t new_cap = MAX(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
    BufHdr *new_hdr;
    if (buf) {
        new_hdr = prealloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = pmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}


void push_cmd_intern(const char *str, CmdTerminals term, void (*builtin_fn)(char *))
{
    size_t len = str + strlen(str) - str;
    char *cmd = pmalloc(len + 1);
    memcpy(cmd, str, len);
    cmd[len] = 0; // null terminate

    buf_push(interns, ((InternCmd){len, cmd, term, builtin_fn}));
}

InternCmd get_cmd_intern(CmdTerminals term)
{
    for (size_t i = 0; i < buf_len(interns); i++) {
        bool is_main = (interns[i].term & MAIN) == 0;
        bool is_required = (interns[i].term & REQUIRE) == 0;
        if (interns[i].term == term || is_main || is_required) {
            return interns[i];
        }
    }

    perror("Cmd undefined");
    buf_free(interns);
    interns = NULL;
    exit(1);
}

const InternCmd *find_cmd_intern(char *cmd)
{
    for (size_t i = 0; i < buf_len(interns); i++) {
        if (strcmp(interns[i].cmd,cmd) == 0) {
            return &interns[i];
        }
    }    

    return NULL;
}

void init_cmds() 
{
    //push_cmd_intern("@declare", DECLARE);
    push_cmd_intern("@main", MAIN | REQUIRE, &require_cmd);
    push_cmd_intern("@include", INCLUDE, &include_cmd);
    // TODO(loffenje): add more cmds
}

static inline void match_token(int token)
{
    if (lookahead == token) {
        lookahead = *(++stream);
        return;
    }

    perror("Syntax match error");
    buf_free(interns);
    interns = NULL;
    exit(1);
}

static inline void expr_forward()
{
    while (isalnum(lookahead) || lookahead == '.' || lookahead == '/') { // TODO: extend checking
        if (strlen(expr) > TEMP_BUFSIZE) {
            perror("Overflow");
            buf_free(interns);
            interns = NULL;
            exit(1);
        }
        strcat(expr, &lookahead);
        lookahead = *(++stream);
    }

}

static inline FILE* create_fd_or_fail(char *name)
{
    FILE *fd;

    fd = fopen(name, "r");

    if (!fd) {
        perror("File not found");
        buf_free(interns);
        interns = NULL;
        exit(1);
    }

    return fd;
}

void lookup_cmd(char *line) 
{
    char *cmd = get_splitted_cmd(line);
    if (cmd == NULL) {
        return;
    }

    char *cmd_s = pmalloc(strlen(cmd)); //TODO: solve segfault
    strcpy(cmd_s, cmd);

    const char delim[2] = "(";
    char *token = strtok(cmd, delim);
    if (token == NULL) {
        return;
    }

    const InternCmd *intern_cmd = find_cmd_intern(token);
    if (intern_cmd == NULL) {
        return;
    }

    stream = &cmd_s[intern_cmd->len];
    lookahead = *stream;
    match_token('(');
    expr_forward();
    match_token(')');
    intern_cmd->builtin_fn(expr);
    free(cmd_s);

}

void require_cmd(char *name)
{
    char bufline[LINE_SIZE];
    let FILE *fd;
    
    fd = create_fd_or_fail(name);
    while (fgets(bufline, sizeof(bufline), fd) != NULL) {
        bufline[strlen(bufline) - 1] = '\0';    
        expr[0] = '\0';
        lookup_cmd(bufline);
    }

    fclose(fd);
}

void include_cmd(char *name) 
{
    char bufline[LINE_SIZE];
    let FILE *fd;

    fd = fopen(name, "r");
    if (!fd) {
        return;
    }

    while (fgets(bufline, sizeof(bufline), fd) != NULL) {
        bufline[strlen(bufline) - 1] = '\0';   
        expr[0] = '\0';
        lookup_cmd(bufline);
    }

    fclose(fd);
}

void run_grail(char *name)
{    
    char bufline[LINE_SIZE];

    FILE *fd;
    
    fd = create_fd_or_fail(name);
    int c;

    InternCmd intern_cmd = get_cmd(MAIN);

    while (fgets(bufline, sizeof(bufline), fd) != NULL) {
        bufline[strlen(bufline) - 1] = '\0';
        char *cmd = get_splitted_cmd(bufline);
        if (cmd == NULL) {
            continue;
        }

        if (strncmp(cmd, intern_cmd.cmd, intern_cmd.len) == 0) {
            stream = &cmd[intern_cmd.len];
            lookahead = *stream;
            match_token('(');
            expr_forward();
            match_token(')');
            intern_cmd.builtin_fn(expr);
        }

        free(cmd);
        cmd = NULL;
    }

    fclose(fd);
}

char *get_splitted_cmd(char *bufline)
{
    let char *cmd;
    char *cmdchr = strchr(bufline, '@');
    if (!cmdchr) {
        printf("%s\n",bufline);
        return NULL;
    } 

    cmd = pmalloc(LINE_SIZE);
    char raw_bufline[LINE_SIZE] = "";
    int offset = cmdchr - bufline;
    
    strncpy(cmd, bufline + offset, strlen(bufline + offset));
    strncpy(raw_bufline, bufline, offset);
    raw_bufline[strlen(bufline) - 1] = '\0';
    printf("%s\n", raw_bufline);

    cmd[strlen(cmd)] = 0;

    return cmd;
}

void process_templates(char *entry_file)
{
    init_cmds();

    run_grail(entry_file);

    buf_free(interns);
}