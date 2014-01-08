#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define PATH_DELIMS "\\/"

/* Simplified function to get a relative path from 'from' to 'to' */
char *relpath(const char *from, const char *to, char *rel, size_t rellen) {	
	/* Assumes its inputs are directories. */
	char *rv = NULL;
	char *from_path = NULL, *to_path = NULL;
	char *from_start = NULL, *to_start = NULL;	
	char *from_ptr, *to_ptr;
	
	if(rellen < 1)
		goto error;
	rel[0] = '\0';
	
	from_path = strdup(from);
	from_start = from_path;
	to_path = strdup(to);
	to_start = to_path;
	
	/* Eliminate the common prefixes in the paths */
	from_path = my_strtok_r(from_path, PATH_DELIMS, &from_ptr);
	to_path = my_strtok_r(to_path, PATH_DELIMS, &to_ptr);	
	while(from_path && to_path && !strcmp(from_path, to_path)) {
		from_path = my_strtok_r(NULL, PATH_DELIMS, &from_ptr);
		to_path = my_strtok_r(NULL, PATH_DELIMS, &to_ptr);
	}	
	
	/* add ../ for every directory remaining in the 'from' path */
	while(from_path) {
		strncat(rel, "../", rellen - 1);
		rellen -= 3;
		from_path = my_strtok_r(NULL, PATH_DELIMS, &from_ptr);
	}
	
	/* append the remaining parts of the the 'to' path */
	while(to_path) {
		strncat(rel, to_path, rellen - 1);
		rellen -= strlen(to_path);
		strncat(rel, "/", rellen - 1);
		rellen--;
		to_path = my_strtok_r(NULL, PATH_DELIMS, &to_ptr);
	}
	
	rv = rel;
	
error:
	free(from_start);
	free(to_start);
	
	return rv;
}
