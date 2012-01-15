#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

static char runlevel;
static char *progname;

struct item {
 	char *command, *spawn, *level;
	int id;
    struct item *next;
};
struct item *top = NULL;

int main(int argc, char **argv) {
	
	int c, pid, status, err = 0;
	int check = 0;
	progname = argv[0];
	FILE *fp;
	
	extern void process(FILE *fp);
	extern void runcommand();
	
	if (argc == 1) {
		fprintf(stderr, "usage: %s [-r runlevel][filename]\n", progname);
		return(err);	
	}
		

	while ((c = getopt(argc, argv, "r:")) != EOF) {
		switch (c) {
		case 'r':
			sscanf(optarg, "%c", &runlevel);
			check = 1;
			break;
		case '?':
		default:
			err = 1;
	    	break;
		}
	}
	
	if (err || (argc > 3 + check)) {
		fprintf(stderr, "usage: %s [-r runlevel][filename]\n", progname);
		return(err);
    }
	
	if ((fp = fopen(argv[optind], "r")) == NULL) {
	   	perror(argv[optind]);
	   	status = 1;
	} 
	else
		process(fp);
	
	pid = fork();
	fflush(stdout);
	
	switch (pid) {
		case -1:
			perror("fork");
        	return(1);
		case 0:
			runcommand();
			return(1);
		default:
			wait(&status);
			return(0);
	}
}

/*program core, runs all processes and
respawns them as necessary*/
void runcommand() {
	struct item *p;
	int pid, status;
	
	extern void respawn(int x);
	
	for (p = top; p; p = p->next) {
		pid = fork();
		fflush(stdout);
		switch (pid) {
			case -1:
        		perror("fork");
        		exit(126);
			case 0:
				if (runlevel == '\0') {
					execl("/bin/sh", "sh", "-c", p->command, (char *)NULL);
        			perror("/bin/sh");
					exit(125);
				}
				else {
					if(strchr(p->level, runlevel) || p->level[0] == '\0') {
						execl("/bin/sh", "sh", "-c", p->command, (char *)NULL);
        				perror("/bin/sh");
						exit(125);
					}
				}
			default:
				if (p->id == 0)
					p->id = pid;
		}
	}
	while (1) {
		pid = wait(&status);
		respawn(pid);
	}
}

/* respawns process with pid = x*/
void respawn(int x) {
	int pid;
	struct item *temp;
	extern struct item *find(int id);
		
	temp = find(x);
	pid = fork();
	fflush(stdout);
	
	switch(pid) {
		case -1:
			perror("fork");
			exit(124);
		case 0:
			if (strcmp(temp->spawn, "respawn") == 0)
				execl("/bin/sh", "sh", "-c", temp->command, (char *)NULL);
			exit(123);
		default:
			break;
		}
}

/* parses input file*/
void process(FILE *fp) {
	
	char line[500], *p, *c, *level, *spawn, *command;
	extern void insert(char *level, char *spawn, char *command);
	extern char *estrsave(char *s);
	
    while (!feof(fp)) {
		fgets(line, 500, fp);
		if (!feof(fp)){
			if ((p = strchr(line, '#')))
	    		*p = '\0';
			if ((p = strchr(line, '\n')))
	    		*p = '\0';
			if ((p = strchr(line, ':'))){
				*p = '\0';
				level = estrsave(line);
				*p++;
				if ((c = strchr(p, ':'))){
		 			*c = '\0';
					spawn = estrsave(p);
					*c++;
					command = estrsave(c);
					insert(level, spawn, command);
				}
			}
		}
	}
	fclose(fp);
}

char *estrsave(char *s) {
    char *q;
    if (!s)
        return(NULL);
    if ((q = malloc(strlen(s) + 1)) == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(q, s);
    return(q);
}

void insert(char *level, char *spawn, char *command) {
    struct item *p;

    /* create the new item */
    p = (struct item *)malloc(sizeof(struct item));
    if (p == NULL)
        exit(1); 
    p->level = level;
    p->spawn = spawn;
	p->command = command;

    /* link it in */
    p->next = top;
    top = p;
}

/* finds command with pid = id */
struct item *find(int id) {
    struct item *p;
    for (p = top; p; p = p->next)
        if (p->id == id)
            return(p);
    return(NULL);
}
