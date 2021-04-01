void dieWithError(char *msg);
void dieWithSystemError(char *msg);  // also prints errno

#define MAXMSGLEN  512

/* fixed-length message header */
typedef struct {
  uint16_t magic;
  uint16_t length;
  uint32_t xactionid;
  uint8_t  flags;
  uint8_t  result;
  uint16_t port;
} header_t;
