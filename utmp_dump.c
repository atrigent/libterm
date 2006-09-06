#include <stdio.h>
#include <utmp.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

char * ident_ut_type(short ut_type) {
	switch(ut_type) {
		case UT_UNKNOWN:
			return "UT_UNKNOWN";
		case RUN_LVL:
			return "RUN_LVL";
		case BOOT_TIME:
			return "BOOT_TIME";
		case NEW_TIME:
			return "NEW_TIME";
		case OLD_TIME:
			return "OLD_TIME";
		case INIT_PROCESS:
			return "INIT_PROCESS";
		case LOGIN_PROCESS:
			return "LOGIN_PROCESS";
		case USER_PROCESS:
			return "USER_PROCESS";
		case DEAD_PROCESS:
			return "DEAD_PROCESS";
		case ACCOUNTING:
			return "ACCOUNTING";
		default:
			return NULL;
	}
}

int main() {
	/* uint32_t to ensure a logical bit shift */
	uint32_t * v6addr;
	struct utmp utmp;
	int fd, count;
	char * tmp;

	fd = open("/var/log/wtmp.1", O_RDONLY);
	if(fd == -1) {
		printf("Could not open utmp/wtmp: %s\n", strerror(errno));
		return 1;
	}

	for(count = 0; read(fd, &utmp, sizeof(struct utmp)); count++) {
		printf("utmp struct number %i:\n", count+1);
		printf("\tut_type:     ");
		tmp = ident_ut_type(utmp.ut_type);
		if(tmp) printf("%s\n", tmp);
		else printf("%i\n", utmp.ut_type);
		printf("\tut_pid:      %i\n", utmp.ut_pid);
		printf("\tut_line:     %.*s\n", UT_LINESIZE, utmp.ut_line);
		printf("\tut_id:       %.4s\n", utmp.ut_id);
		printf("\tut_user:     %.*s\n", UT_NAMESIZE, utmp.ut_user);
		printf("\tut_host:     %.*s\n", UT_HOSTSIZE, utmp.ut_host);
		
		printf("\tut_exit:\n");
		printf("\t\te_termination:  %i\n", utmp.ut_exit.e_termination);
		printf("\t\te_exit:         %i\n", utmp.ut_exit.e_exit);

		printf("\tut_session:  %i\n", utmp.ut_session);
		
		printf("\tut_tv:\n");
		printf("\t\ttv_sec:         %s", ctime(&utmp.ut_tv.tv_sec));
		printf("\t\ttv_usec:        %i\n", utmp.ut_tv.tv_usec);

		v6addr = utmp.ut_addr_v6;
		printf("\tut_addr_v6:  %x:%x:%x:%x:%x:%x:%u.%u.%u.%u\n",
				v6addr[3] >> 16,
				v6addr[3] & 0xffff
				v6addr[2] >> 16,
				v6addr[2] & 0xffff,
				v6addr[1] >> 16,
				v6addr[1] & 0xffff,
				v6addr[0] >> 24,
				(v6addr[0] >> 16) & 0xff,
				(v6addr[0] >> 8) & 0xff,
				v6addr[0] & 0xff);
	}
	
	return 0;
}
