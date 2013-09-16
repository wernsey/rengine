/*1 pak.h
 *# Utility library for manipulating id Software's Quake-style PAK files. 
 *2 References
 *{
 ** http://debian.fmi.uni-sofia.bg/~sergei/cgsr/docs/pak.txt
 ** http://en.wikipedia.org/wiki/PAK_%28file_format%29
 *}
 *2 API
 */

/*@ struct pak_file
 *# Internal datastructure representing a {*PAK*} file.
 */
struct pak_file;

/*@ extern int pak_verbose
 *# Sets verbose mode: \n
 *{
 ** 0 - disabled.
 ** 1 - displays errors and warnings.\n
 ** 2 - everything - intended for debugging.\n
 *}
 *# Defaults to 0.
 */
extern int pak_verbose;

/*@ struct pak_file *pak_open(const char *name)
 *# Opens an existing archive for reading and appending.
 *# Returns {{NULL}} if the file does not exits or on error.
 */
struct pak_file *pak_open(const char *name);

/*@ struct pak_file *pak_create(const char *name)
 *# Creates a new PAK file on disk. It it already exists its contents
 *# will be erased.\n
 *# Returns {{NULL}} on error.
 */
struct pak_file *pak_create(const char *name);

/*@ int pak_append_file(struct pak_file *p, const char *filename)
 *# Appends the contents of a file to the archive.\n
 *# Returns 1 on success, 0 on failure.
 */
int pak_append_file(struct pak_file *p, const char *filename);

/*@ int pak_append_blob(struct pak_file *p, const char *filename, const char *blob, int len)
 *# Appends a {{blob}} of data of length {{len}} to the archive under the name {{filename}}.\n
 *# Returns 1 on success, 0 on failure.
 */
int pak_append_blob(struct pak_file *p, const char *filename, const char *blob, int len);

/*@ void pak_close(struct pak_file * p)
 *# Writes all changes to disk and closes the file.\n
 *# It also deallocates all memory allocated to the {{pak_file}} structure.
 */
int pak_close(struct pak_file * p);

/*@ int pak_num_files(struct pak_file * p)
 *# Returns the number of files in the archive.
 */
int pak_num_files(struct pak_file * p);

/*@ const char *pak_nth_file(struct pak_file * p, int n)
 *# Returns the name of the n-th file in the archive.
 */
const char *pak_nth_file(struct pak_file * p, int n);

/*@ char *pak_get_blob(struct pak_file * p, const char *filename, int *len)
 *# Retrieves the contents of a file within the archive as a blob of bytes.\n
 *# {{len}} will be set to the number of bytes in the blob.\n
 *# The blob needs to be {{free()}}ed afterwards.
 */
char *pak_get_blob(struct pak_file * p, const char *filename, size_t *len);

/*@ char *pak_get_text(struct pak_file * p, const char *filename)
 *# Retrieves the contents of a file within the archive as an array of characters.\n
 *# It assumes a text file and so returns a null-terminated string.
 *# The string needs to be {{free()}}ed afterwards.
 */
char *pak_get_text(struct pak_file * p, const char *filename);

/*@ FILE *pak_get_file(struct pak_file * p, const char *filename)
 *# Retrieves the location of a file within the archive as a {{FILE*}}.\n
 *# It returns {{NULL}} if the file could not be found.\n
 *N Don't write to this {{FILE*}}
 */
FILE *pak_get_file(struct pak_file * p, const char *filename);

/*@ int pak_extract_file(struct pak_file * p, const char *filename, const char *to)
 *# Extracts a file named {{filename}} from the archive to a file named {{to}}.\n
 *# Returns 1 on success, 0 on failure.
 */
int pak_extract_file(struct pak_file * p, const char *filename, const char *to);
