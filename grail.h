
#define get_cmd(d) get_cmd_intern(d)
#define MAX(x,y) ((x) >= (y) ? (x) : (y))

#define LINE_SIZE 1024

// inspired by Sean Barrett
typedef struct BufHdr
{
    size_t len;
    size_t cap;
    char buf[0];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits(b, n) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))
#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, x) (buf__fit(b, 1), b[buf_len(b)] = (x), buf__hdr(b)->len++)
#define buf_free(b) ((b) ? free(buf__hdr(b)) : 0)

typedef enum CmdTerminals {
    MAIN,
    INCLUDE,
    REQUIRE,
    DECLARE
} CmdTerminals;

typedef struct InternCmd {
    size_t len;
    const char *cmd;
    CmdTerminals term;
    void (*builtin_fn) (char *);
} InternCmd;


void process_templates(char *entry_file);

void run_grail(char *name);

void init_cmds();

char* get_splitted_cmd(char *bufline);

void lookup_cmd(char *bufline);

void require_cmd(char *name);

void include_cmd(char *name);

const InternCmd *find_cmd_intern(char *cmd);