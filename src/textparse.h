#ifndef TEXTPARSE_H
#define TEXTPARSE_H

#define range_a_z "abcdefghijklmnopqrstuvwxyz"
#define range_A_Z "ABCDEFGIHJKLMNOPQRSTUVWXYZ"
#define range_0_9 "0123456789"

extern int parse_ff_past_consec(char **, char *);
extern int parse_ff_past(char **, char *);

extern int parse_read(char **, char **, uint);
extern int parse_read_consec(char **, char **, char *);
extern int parse_read_to(char **, char **, char *);
extern int parse_read_to_without_trailing(char **, char **, char *, char *);

#endif
