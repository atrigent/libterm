#ifndef INTERFACE_H
#define INTERFACE_H

/* if an interface function gets used internally as well,
 * put it in here
 */

extern int ltm_reload_sigchld_handler();
extern int ltm_set_window_dimensions(int, uint, uint);

#endif
