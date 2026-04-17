/*

  WHDArchiveExtractor

  This Amiga CLI program simplifies the extraction of large numbers of
  LHA archives commonly used in WHDLoad. Designed with WHDLoad Download
  Tool users in mind, it automates the process of searching for archives
  in all subfolders and extracting them to a specified target path while
  preserving the original folder structure.

  Key features of this program include:

  * Scanning input folders and subfolders for LHA and LZX archives
  * Extracting the archives using the LHA program to an output folder
  * Preserving the subfolder structure from the input folder during
    extraction
  * Extracting only new or updated files to avoid unnecessary duplication

  To use this program, ensure the LHA software is installed in the C:
  directory. You can download it from aminet.net/package/util/arc/lha

  v1.0.0 - 2023-04-02 - First version released.

  v1.1.0 - 2024-03-14 - Added support for LZX archives.  Download from
                        aminet.net/package/util/arc/lzx121r1
                      - Improved error handling.
                      - If the target folder already exists, it
                        will now be scanned for protected files and the
                        protection bits will be removed.  This is to allow
                        the extraction to replace the files.

  This program is released under the MIT License.
*/

#include <ctype.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <workbench/icon.h>

#define bool int
#define true 1
#define false 0
#define MAX_ERRORS 40
#define MAX_ERROR_LENGTH 256
#define DEBUG 1
#define BUFFER_SIZE 1024
#define MAX_TRACKED_DESTINATIONS 1024
#define MAX_PATH_LENGTH 256
#define MAX_AMIGA_COMPONENT_LENGTH 30
#define ICON_APPLIER_MAX_PATH 512
#define ICON_APPLIER_DEFAULT_ICONS_DIR "PROGDIR:Icons"

typedef struct icon_applier_options
{
  BOOL use_custom_icons;
  const char *icons_dir;
} icon_applier_options;

bool skip_disk_space_check = false, test_archives_only = false, skip_if_dest_exists = false, enable_custom_icons = false;
char *input_file_path;
char *output_file_path;
char single_error_message[MAX_ERROR_LENGTH];
char error_messages_array[MAX_ERRORS][MAX_ERROR_LENGTH];
char version_number[] = "1.1.1";
int  num_archives_found;
int  error_count = 0;
int  num_directories_scanned;
int  should_stop_app = 0; /* used to stop the app if the lha extraction fails */
long start_time;
int  resetProtectionBits = 1;
int  destination_tracking_overflow_warned = 0;

STRPTR input_directory_path;
STRPTR output_directory_path;
icon_applier_options drawer_icon_options = {FALSE, ICON_APPLIER_DEFAULT_ICONS_DIR};

/* Function prototypes */
char *get_file_path(const char *full_path);
char *remove_text(char *input_str, STRPTR text_to_remove);
int   check_disk_space(STRPTR path, int min_space_mb);
int   does_file_exist(char *filename);
int   does_folder_exists(const char *folder_name);
int   ends_with_lha(const char *filename);
void  sanitizeAmigaPath(char *path);
void  get_directory_contents(STRPTR input_directory_path, STRPTR output_directory_path);
void  logError(const char *errorMessage);
void  printErrors(void);
void  remove_trailing_slash(char *str);
char *findFirstDirectory(char *filePath);
char *get_file_extension(const char *filename, char *outputBuffer);
int   get_path_state(const char *path);
int   is_destination_claimed(const char *destination_path, char *claimed_destinations[], int claimed_destinations_count);
int   add_claimed_destination(const char *destination_path, char *claimed_destinations[], int *claimed_destinations_count);
void  free_claimed_destinations(char *claimed_destinations[], int claimed_destinations_count);
int   ensure_directory_exists(const char *path, int *created);
int   ensure_path_directories(const char *base_root, const char *full_target_path, int *created_count);
BOOL  icon_applier_exists(const char *path);
void  icon_applier_sanitize_path(char *path);
const char *icon_applier_get_icons_dir(const icon_applier_options *options);
BOOL  icon_applier_ensure_drawer_icon(const char *dir_path, const icon_applier_options *options);

int num_lzx_archives_found = 0;
int num_lha_archives_found = 0;
int num_archives_skipped_dest_exists = 0;
int num_destination_conflicts = 0;
int num_destination_not_drawer_conflicts = 0;
int num_directories_created = 0;
int num_drawer_icons_attempted = 0;
int num_drawer_icons_applied = 0;
int num_drawer_icons_failed = 0;

/*
 * Function to sanitize an Amiga file path in-place by correcting specific path issues.
 * It ensures no slashes immediately follow a colon, replaces "//" with "/", and removes consecutive colons.
 * Assumes the input buffer is large enough for the sanitized path.
 */
void sanitizeAmigaPath(char *path)
{
  ULONG len;
  char *sanitizedPath;
  int i, j;

  if (path == NULL)
  {
    return;
  }

  len = strlen(path) + 1; /* Include null terminator*/
  sanitizedPath = (char *)AllocVec(len, MEMF_ANY | MEMF_CLEAR);
  if (sanitizedPath == NULL)
  {
    /* Memory allocation failed */
    return;
  }

  i = 0;
  j = 0;
  while (path[i] != '\0')
  {
    if (path[i] == ':')
    {
      /* Copy the colon */
      sanitizedPath[j++] = path[i++];
      /* Skip all following slashes */
      while (path[i] == '/')
      {
        i++;
      }
    }
    else if (path[i] == '/' && path[i + 1] == '/')
    {
      /* Skip redundant slashes */
      i++;
    }
    else
    {
      /* Copy other characters */
      sanitizedPath[j++] = path[i++];
    }
  }

  sanitizedPath[j] = '\0'; /* Ensure the result is null-terminated */

  /* Copy back to the original path and free the allocated memory */
  strcpy(path, sanitizedPath);
  FreeVec(sanitizedPath);
}

BOOL icon_applier_exists(const char *path)
{
  BPTR lock;

  if (path == NULL || path[0] == '\0')
  {
    return FALSE;
  }

  lock = Lock((CONST_STRPTR)path, ACCESS_READ);
  if (lock == 0)
  {
    return FALSE;
  }

  UnLock(lock);
  return TRUE;
}

void icon_applier_sanitize_path(char *path)
{
  size_t i;

  if (path == NULL)
  {
    return;
  }

  sanitizeAmigaPath(path);

  for (i = 0; path[i] != '\0'; i++)
  {
    if (((unsigned char)path[i] < 32U) || strchr("*?#|<>\"{}", path[i]) != NULL)
    {
      path[i] = '_';
    }
  }

  remove_trailing_slash(path);
}

const char *icon_applier_get_icons_dir(const icon_applier_options *options)
{
  if (options == NULL || options->icons_dir == NULL || options->icons_dir[0] == '\0')
  {
    return ICON_APPLIER_DEFAULT_ICONS_DIR;
  }

  return options->icons_dir;
}

BOOL icon_applier_ensure_drawer_icon(const char *dir_path, const icon_applier_options *options)
{
  size_t dir_path_length;
  size_t icons_dir_length;
  size_t leaf_name_length;
  char info_path[ICON_APPLIER_MAX_PATH];
  char source_icon_name[ICON_APPLIER_MAX_PATH];
  const char *leaf_name;
  const char *icons_dir;
  const char *slash;
  const char *colon;
  struct DiskObject *diskobj;
  static int icons_dir_checked = 0;
  static BOOL icons_dir_exists = FALSE;

  if (dir_path == NULL || options == NULL)
  {
    return FALSE;
  }

  dir_path_length = strlen(dir_path);
  if (dir_path_length + 5 >= sizeof(info_path))
  {
    return FALSE;
  }

  strcpy(info_path, dir_path);
  strcat(info_path, ".info");
  icon_applier_sanitize_path(info_path);

  /* Preserve existing icon files for drawers that already have one. */
  if (icon_applier_exists(info_path))
  {
    return TRUE;
  }

  slash = strrchr(dir_path, '/');
  if (slash != NULL)
  {
    leaf_name = slash + 1;
  }
  else
  {
    colon = strrchr(dir_path, ':');
    leaf_name = (colon != NULL) ? (colon + 1) : dir_path;
  }

  if (leaf_name[0] == '\0')
  {
    return FALSE;
  }

  diskobj = NULL;
  icons_dir = icon_applier_get_icons_dir(options);

  if (options->use_custom_icons)
  {
    if (!icons_dir_checked)
    {
      icons_dir_exists = icon_applier_exists(icons_dir);
      icons_dir_checked = 1;
    }

    if (icons_dir_exists)
    {
      icons_dir_length = strlen(icons_dir);
      leaf_name_length = strlen(leaf_name);
      if (icons_dir_length + leaf_name_length + 2 < sizeof(source_icon_name))
      {
        strcpy(source_icon_name, icons_dir);
        strcat(source_icon_name, "/");
        strcat(source_icon_name, leaf_name);
        icon_applier_sanitize_path(source_icon_name);
        diskobj = GetDiskObject((CONST_STRPTR)source_icon_name);
      }
    }
  }

  if (diskobj == NULL)
  {
    diskobj = GetDefDiskObject(WBDRAWER);
    if (diskobj == NULL)
    {
      return FALSE;
    }
  }

  if (!PutDiskObject((CONST_STRPTR)dir_path, diskobj))
  {
    FreeDiskObject(diskobj);
    return FALSE;
  }

  FreeDiskObject(diskobj);
  return TRUE;
}

char *remove_text(char *input_str, STRPTR text_to_remove)
{
  int remove_len = strlen(text_to_remove);

  /* Check if the second string exists at the start of the first string */
  if (strncmp(input_str, text_to_remove, remove_len) == 0)
  {
    /* If the second string is found, return a pointer to the rest of the
     * first string*/
    return input_str + remove_len;
  }

  /*If the second string is not found, return the original first string*/
  return input_str;
}

int ends_with_lha(const char *filename)
{
  size_t len = strlen(filename);
  return len > 4 && strcmp(filename + len - 4, ".lha") == 0;
}

/*
 * Extracts the file extension from a given filename and converts it to uppercase.
 * Assumes the extension is exactly 4 characters long, including the dot.
 *
 * Parameters:
 *     filename - the name of the file from which to extract the extension.
 *     outputBuffer - a buffer to hold the uppercase extension, must be at least 5 characters long.
 *
 * Returns:
 *     A pointer to the outputBuffer containing the uppercase extension, or NULL if the operation fails.
 */
char *get_file_extension(const char *filename, char *outputBuffer)
{
  const char *extensionStart;
  int i;
  size_t len = strlen(filename);

  if (len < 4 || outputBuffer == NULL)
  {
    return NULL; /* Return NULL if the string is too short or outputBuffer is NULL */
  }

  extensionStart = filename + len - 4;
  for (i = 0; i < 4; i++)
  {
    outputBuffer[i] = toupper((unsigned char)extensionStart[i]);
  }
  outputBuffer[4] = '\0';

  return outputBuffer;
}

char *get_file_path(const char *full_path)
{
  char *file_path = NULL;

  /* Find the last occurrence of the path separator character */
  const char *last_slash = strrchr(full_path, '/');
  const char *last_back_slash = strrchr(full_path, '\\');
  const char *last_path_separator = last_slash > last_back_slash ? last_slash : last_back_slash;

  if (last_path_separator != NULL)
  {
    /* Calculate the length of the file path */
    size_t file_path_length = last_path_separator - full_path + 1;

    /* Allocate memory for the file path string */
    file_path = malloc(file_path_length + 1);

    if (file_path != NULL)
    {
      /* Copy the file path string to the newly allocated memory */
      strncpy(file_path, full_path, file_path_length);
      file_path[file_path_length] = '\0';
    }
  }

  return file_path;
}

int does_file_exist(char *filename)
{
  FILE *file;
  /* Try to open the file for reading*/
  if ((file = fopen(filename, "r")))
  {
    /* If successful, close the file and return 1*/
    fclose(file);
    return 1;
  }
  else
  {
    /* If not successful, return 0*/
    return 0;
  }
}

int does_folder_exists(const char *folder_name)
{
  BPTR lock = Lock((CONST_STRPTR)folder_name, ACCESS_READ);
  if (lock != 0)
  {
    UnLock(lock);
    return 1; /* Folder exists*/
  }
  else
  {
    return 0; /* Folder does not exist */
  }
}

/*
 * Returns destination path state:
 *  1 = exists and is a directory
 *  0 = does not exist or cannot be examined
 * -1 = exists but is not a directory
 */
int get_path_state(const char *path)
{
  BPTR lock;
  int path_state;
  struct FileInfoBlock *file_info_block;

  lock = Lock((CONST_STRPTR)path, ACCESS_READ);
  if (lock == 0)
  {
    return 0;
  }

  path_state = 0;
  file_info_block = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);
  if (file_info_block)
  {
    if (Examine(lock, file_info_block))
    {
      if (file_info_block->fib_DirEntryType > 0)
      {
        path_state = 1;
      }
      else
      {
        path_state = -1;
      }
    }
    FreeMem(file_info_block, sizeof(struct FileInfoBlock));
  }

  UnLock(lock);
  return path_state;
}

int ensure_directory_exists(const char *path, int *created)
{
  BPTR created_lock;
  int path_state;

  if (created != NULL)
  {
    *created = 0;
  }

  path_state = get_path_state(path);
  if (path_state == 1)
  {
    return 1;
  }

  if (path_state == -1)
  {
    return -1;
  }

  created_lock = CreateDir((CONST_STRPTR)path);
  if (created_lock == 0)
  {
    return 0;
  }

  UnLock(created_lock);

  if (get_path_state(path) == 1)
  {
    if (created != NULL)
    {
      *created = 1;
    }
    return 1;
  }

  return 0;
}

int ensure_path_directories(const char *base_root, const char *full_target_path, int *created_count)
{
  int status;
  int created;
  size_t base_length;
  size_t current_length;
  size_t component_length;
  size_t new_length;
  char current_path[MAX_PATH_LENGTH];
  const char *relative_path;
  const char *component_start;
  const char *separator;

  if (created_count != NULL)
  {
    *created_count = 0;
  }

  if (base_root == NULL || full_target_path == NULL)
  {
    return 0;
  }

  if (strlen(base_root) >= sizeof(current_path) || strlen(full_target_path) >= sizeof(current_path))
  {
    return 0;
  }

  strcpy(current_path, base_root);
  sanitizeAmigaPath(current_path);
  icon_applier_sanitize_path(current_path);
  remove_trailing_slash(current_path);

  if (get_path_state(current_path) != 1)
  {
    return 0;
  }

  base_length = strlen(current_path);
  if (strncmp(full_target_path, current_path, base_length) != 0)
  {
    return 0;
  }

  relative_path = full_target_path + base_length;
  while (*relative_path == '/')
  {
    relative_path++;
  }

  if (*relative_path == '\0')
  {
    return 1;
  }

  component_start = relative_path;
  while (*component_start != '\0')
  {
    separator = strchr(component_start, '/');
    if (separator != NULL)
    {
      component_length = (size_t)(separator - component_start);
    }
    else
    {
      component_length = strlen(component_start);
    }

    if (component_length == 0 || component_length > MAX_AMIGA_COMPONENT_LENGTH)
    {
      return 0;
    }

    if ((component_length == 1 && component_start[0] == '.') ||
        (component_length == 2 && component_start[0] == '.' && component_start[1] == '.'))
    {
      return 0;
    }

    current_length = strlen(current_path);
    new_length = current_length + 1 + component_length;
    if (new_length >= sizeof(current_path))
    {
      return 0;
    }

    current_path[current_length] = '/';
    memcpy(current_path + current_length + 1, component_start, component_length);
    current_path[new_length] = '\0';

    created = 0;
    status = ensure_directory_exists(current_path, &created);
    if (status <= 0)
    {
      return 0;
    }

    if (created && created_count != NULL)
    {
      (*created_count)++;
    }

    if (created && enable_custom_icons)
    {
      num_drawer_icons_attempted++;
      if (icon_applier_ensure_drawer_icon(current_path, &drawer_icon_options))
      {
        num_drawer_icons_applied++;
      }
      else
      {
        num_drawer_icons_failed++;
      }
    }

    if (separator == NULL)
    {
      break;
    }

    component_start = separator + 1;
  }

  return 1;
}

int is_destination_claimed(const char *destination_path, char *claimed_destinations[], int claimed_destinations_count)
{
  int i;

  for (i = 0; i < claimed_destinations_count; i++)
  {
    if (strcmp(claimed_destinations[i], destination_path) == 0)
    {
      return 1;
    }
  }

  return 0;
}

int add_claimed_destination(const char *destination_path, char *claimed_destinations[], int *claimed_destinations_count)
{
  size_t destination_length;

  if (*claimed_destinations_count >= MAX_TRACKED_DESTINATIONS)
  {
    return 0;
  }

  destination_length = strlen(destination_path) + 1;
  claimed_destinations[*claimed_destinations_count] = (char *)AllocVec(destination_length, MEMF_ANY);
  if (claimed_destinations[*claimed_destinations_count] == NULL)
  {
    return 0;
  }

  strcpy(claimed_destinations[*claimed_destinations_count], destination_path);
  (*claimed_destinations_count)++;
  return 1;
}

void free_claimed_destinations(char *claimed_destinations[], int claimed_destinations_count)
{
  int i;

  for (i = 0; i < claimed_destinations_count; i++)
  {
    if (claimed_destinations[i] != NULL)
    {
      FreeVec(claimed_destinations[i]);
      claimed_destinations[i] = NULL;
    }
  }
}

void remove_trailing_slash(char *str)
{
  if (str != NULL && strlen(str) > 0 && str[strlen(str) - 1] == '/')
  {
    str[strlen(str) - 1] = '\0';
  }
}

void logError(const char *errorMessage)
{
  strncpy(error_messages_array[error_count], errorMessage, MAX_ERROR_LENGTH);
  error_messages_array[error_count][MAX_ERROR_LENGTH - 1] = '\0'; /* Ensure null-termination */
  error_count++;
}

void printErrors()
{
  int i;
  if (error_count > 0)
  {
    printf("\n\x1B[1mErrors encountered during execution:\x1B[0m\n");
    for (i = 0; i < error_count; i++)
    {
      printf("\x1B[1mError:\x1B[0m %d: %s\n", i + 1, error_messages_array[i]);
    }
  }
  else
  {
    printf("\nNo errors encountered.\n");
  }
}

char *findFirstDirectory(char *filePath)
{
  static char directoryName[256]; /* Static buffer to hold the directory name */
  FILE *file;
  char line[256]; /* Buffer to read each line into */

  if (does_file_exist(filePath) == 0)
  {
    printf("File does not exist: %s\n", filePath);
    return NULL; /* Return NULL if file can't be opened */
  }

  /* Open the file for reading */
  if ((file = fopen(filePath, "r")) == NULL)
  {
    return NULL; /* Return NULL if file can't be opened */
  }

  while (fgets(line, sizeof(line), file) != NULL)
  {                                          /* Read each line */
    char *slashPosition = strchr(line, '/'); /* Find the first '/' */
    if (slashPosition != NULL)
    {
      /* Calculate the directory name length */
      int dirLength = slashPosition - line;
      /* Copy the directory name to the static buffer */
      strncpy(directoryName, line, dirLength);
      directoryName[dirLength] = '\0'; /* Null-terminate the string */
      fclose(file);                    /* Close the file */
      return directoryName;            /* Return the directory name */
    }
  }

  fclose(file); /* Close the file if no directory is found */
  return NULL;  /* Return NULL if no directory is found */
}

void get_directory_contents(STRPTR input_directory_path, STRPTR output_directory_path)
{
  BPTR dir_lock;
  int folder_claimed_destinations_count;
  int has_lha_destination_name;
  int directories_created_for_archive;
  int path_prepare_result;
  size_t archive_filename_length;
  int destination_path_state;
  int destination_conflict_detected;
  char *folder_claimed_destinations[MAX_TRACKED_DESTINATIONS];
  char destination_claim_key[MAX_ERROR_LENGTH];
  char file_extension[5];
  char current_file_path[256];
  char destination_drawer_path[256];
  char lha_destination_name[256];
  char archive_name_without_extension[256];
  char ExtractCommand[20];
  char extraction_command[256];
  char *relative_destination_path;
  char *directoryName;
  char program_name[6];
  char fileCommandStore[256];
  LONG command_result;

  struct FileInfoBlock *file_info_block;
  folder_claimed_destinations_count = 0;
  resetProtectionBits = 1;

  printf("Scanning directory: %s\n", input_directory_path);

  dir_lock = Lock((CONST_STRPTR)input_directory_path, ACCESS_READ);
  if (dir_lock)
  {
    file_info_block = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);
    if (file_info_block)
    {
      if (Examine(dir_lock, file_info_block))
      {
        while (ExNext(dir_lock, file_info_block) && should_stop_app == 0)
        {
          if (strcmp(file_info_block->fib_FileName, ".") != 0 && strcmp(file_info_block->fib_FileName, "..") != 0)
          {
            strcpy(current_file_path, input_directory_path);
            strcat(current_file_path, "/");
            strcat(current_file_path, file_info_block->fib_FileName);
            sanitizeAmigaPath(current_file_path);

            if (file_info_block->fib_DirEntryType > 0)
            {
              num_directories_scanned++;

              get_directory_contents(current_file_path, output_directory_path);
            }
            else
            {
              get_file_extension(file_info_block->fib_FileName, file_extension);

              if (strcmp(file_extension, ".LHA") == 0 || strcmp(file_extension, ".LZX") == 0)
              {
                if (strcmp(file_extension, ".LHA") == 0)
                {
                  num_lha_archives_found++;
                }
                else
                {
                  num_lzx_archives_found++;
                }

                sprintf(fileCommandStore, "%s/%s", output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)));
                sanitizeAmigaPath(fileCommandStore);

                strcpy(archive_name_without_extension, file_info_block->fib_FileName);
                archive_filename_length = strlen(archive_name_without_extension);
                if (archive_filename_length > 4)
                {
                  archive_name_without_extension[archive_filename_length - 4] = '\0';
                }

                sprintf(destination_drawer_path, "%s/%s", fileCommandStore, archive_name_without_extension);
                sanitizeAmigaPath(destination_drawer_path);

                has_lha_destination_name = 0;
                lha_destination_name[0] = '\0';
                if (strcmp(file_extension, ".LHA") == 0)
                {
                  sprintf(extraction_command, "lha vq \"%s\" >ram:listing.txt", current_file_path);
                  sanitizeAmigaPath(extraction_command);
                  SystemTagList(extraction_command, NULL);
                  directoryName = findFirstDirectory("ram:listing.txt");
                  DeleteFile("ram:listing.txt");

                  if (directoryName != NULL && directoryName[0] != '\0')
                  {
                    strncpy(lha_destination_name, directoryName, sizeof(lha_destination_name) - 1);
                    lha_destination_name[sizeof(lha_destination_name) - 1] = '\0';
                    has_lha_destination_name = 1;

                    sprintf(destination_drawer_path, "%s/%s", fileCommandStore, lha_destination_name);
                    sanitizeAmigaPath(destination_drawer_path);
                  }
                }

                if (has_lha_destination_name)
                {
                  strncpy(destination_claim_key, lha_destination_name, MAX_ERROR_LENGTH - 1);
                }
                else
                {
                  strncpy(destination_claim_key, archive_name_without_extension, MAX_ERROR_LENGTH - 1);
                }
                destination_claim_key[MAX_ERROR_LENGTH - 1] = '\0';

                destination_conflict_detected = 0;
                destination_path_state = get_path_state(destination_drawer_path);
                if (destination_path_state == -1)
                {
                  printf("Conflict: destination path exists but is not a drawer: \x1B[1m%s\x1B[0m\n", destination_drawer_path);
                  num_destination_conflicts++;
                  num_destination_not_drawer_conflicts++;
                  destination_conflict_detected = 1;
                }

                if (is_destination_claimed(destination_claim_key, folder_claimed_destinations, folder_claimed_destinations_count))
                {
                  printf("Conflict: two archives map to the same destination drawer: \x1B[1m%s\x1B[0m\n", destination_drawer_path);
                  num_destination_conflicts++;
                  destination_conflict_detected = 1;
                }
                else
                {
                  if (!add_claimed_destination(destination_claim_key, folder_claimed_destinations, &folder_claimed_destinations_count) && destination_tracking_overflow_warned == 0)
                  {
                    printf("Warning: destination conflict tracking limit reached. Further destination conflicts may not be detected.\n");
                    destination_tracking_overflow_warned = 1;
                  }
                }

                if (destination_conflict_detected)
                {
                  continue;
                }

                if (skip_if_dest_exists && !test_archives_only && destination_path_state == 1)
                {
                  relative_destination_path = remove_text(destination_drawer_path, output_directory_path);
                  printf("Skipping \x1B[1m%s\x1B[0m (already exists: \x1B[1m%s\x1B[0m)\n", file_info_block->fib_FileName, relative_destination_path);
                  num_archives_skipped_dest_exists++;
                  continue;
                }

                printf("Extracting \x1B[1m%s\x1B[0m to \x1B[1m%s\x1B[0m\n", file_info_block->fib_FileName, fileCommandStore);
                if (strcmp(file_extension, ".LHA") == 0)
                {
                  strcpy(program_name, "lha");
                  if (test_archives_only)
                  {
                    strcpy(ExtractCommand, "t\0");
                  }
                  else
                  {
                    strcpy(ExtractCommand, "-T -M -N -m x\0");

                    if (resetProtectionBits == 1)
                    {
                      if (has_lha_destination_name)
                      {
                        sprintf(fileCommandStore, "%s/%s/%s", output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)), lha_destination_name);
                        sanitizeAmigaPath(fileCommandStore);
                        if (does_folder_exists(fileCommandStore) == 1)
                        {
                          sprintf(fileCommandStore, "protect %s/%s/%s/#? ALL rwed >NIL:", output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)), lha_destination_name);
                          sanitizeAmigaPath(fileCommandStore);
                          printf("Prepping any protected files for potential replacement...\n");
                          SystemTagList(fileCommandStore, NULL);
                        }
                      }
                      else
                      {
                        printf("Unable to get the file path from the LHA output for file %s.\n", current_file_path);
                      }
                    }
                    ExtractCommand[0] = '\0';
                    strcpy(ExtractCommand, "-T -M -N -m x\0");
                  }
                }
                else
                {
                  strcpy(program_name, "unlzx");
                  if (test_archives_only)
                  {
                    strcpy(ExtractCommand, "-v\0");
                  }
                  else
                  {
                    strcpy(ExtractCommand, "-x\0");
                  }
                }

                
                /* Check for disk space before extracting */
                should_stop_app = 0;
                if (skip_disk_space_check == false)
                {
                  int disk_check_result = check_disk_space(output_directory_path, 20);
                  if (disk_check_result < 0)
                  {
                    /* To do: handle various error cases based
                       on the result code */
                    printf(
                        "\x1B[1mError:\x1B[0m Not enough "
                        "space on the target drive or cannot "
                        "check space.\n20MB minimum checked "
                        "for.  To disable this check, launch "
                        "the program\nwithout the "
                        "'-enablediskcheck' command.\n");
                    should_stop_app = 1;
                  }
                }

                if (should_stop_app == 0 && !test_archives_only)
                {
                  directories_created_for_archive = 0;
                  path_prepare_result = ensure_path_directories(output_directory_path, destination_drawer_path, &directories_created_for_archive);
                  if (!path_prepare_result)
                  {
                    printf(
                        "\n\x1B[1mError:\x1B[0m "
                        "Unable to prepare destination drawers "
                        "for %s at %s\n",
                        current_file_path,
                        destination_drawer_path);

                    strncpy(single_error_message, current_file_path, MAX_ERROR_LENGTH - 1);
                    single_error_message[MAX_ERROR_LENGTH - 1] = '\0';
                    if (strlen(single_error_message) + strlen(" failed to prepare destination path") < MAX_ERROR_LENGTH)
                    {
                      strcat(single_error_message, " failed to prepare destination path");
                    }
                    logError(single_error_message);
                    continue;
                  }

                  num_directories_created += directories_created_for_archive;
                }

                if (should_stop_app == 0)
                {
                  num_archives_found++;

                  /* Combine the extraction command, source
                   * path, and output path */
                  sprintf(extraction_command, "%s %s \"%s\" \"%s/%s\"", program_name, ExtractCommand, current_file_path, output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)));
                  sanitizeAmigaPath(extraction_command);

                  /* Execute the command*/
                  command_result = SystemTagList(extraction_command, NULL);

                  /* Check for error */
                  if (command_result != 0)
                  {
                    if (command_result == 10)
                    {
                      printf(
                          "\n\x1B[1mError:\x1B[0m "
                          "Corrupt archive %s\n",
                          current_file_path);
                      /* Copy the first part of the
                         message */
                      strncpy(single_error_message, current_file_path, MAX_ERROR_LENGTH - 1);
                      single_error_message[MAX_ERROR_LENGTH - 1] = '\0'; /* Ensure null-termination */

                      /* Concatenate the error message if there's space */
                      if (strlen(single_error_message) + strlen(" is corrupt") < MAX_ERROR_LENGTH)
                      {
                        strcat(single_error_message, " is corrupt");
                      }
                      logError(single_error_message);
                    }
                    else
                    {
                      printf(
                          "\n\x1B[1mError:\x1B[0m "
                          "Failed to execute command "
                          "lha for file %s.\nPlease "
                          "check the archive is not "
                          "damaged, and there is "
                          "enough space in the\ntarget "
                          "directory.\n",
                          current_file_path);
                      /* Copy the first part of the message */

                      strncpy(single_error_message, current_file_path, MAX_ERROR_LENGTH - 1);
                      single_error_message[MAX_ERROR_LENGTH - 1] = '\0'; /* Ensure null-termination

                     /* Concatenate the error message if there's space */
                      if (strlen(single_error_message) + strlen(" failed to extract. "
                                                                "Unknown error") <
                          MAX_ERROR_LENGTH)
                      {
                        strcat(single_error_message,
                               " failed to extract. "
                               "Unknown error");
                      }
                      logError(single_error_message);
                    }
                  }
                  /* if the number of errors is greater then MAX_ERRORS, then quit */
                  if (error_count >= MAX_ERRORS)
                  {
                    printf(
                        "Maximum number of errors "
                        "reached. Aborting.\n");
                    should_stop_app = 1;
                  }
                }
              }
            }
          }
        }
      }
      FreeMem(file_info_block, sizeof(struct FileInfoBlock));
    }
    UnLock(dir_lock);
  }

  free_claimed_destinations(folder_claimed_destinations, folder_claimed_destinations_count);
}

int check_disk_space(STRPTR path, int min_space_mb)
{
  struct InfoData *info = AllocMem(sizeof(struct InfoData), MEMF_CLEAR);
  BPTR lock = Lock(path, ACCESS_READ);
  long free_space;
  int result;

  if (!info)
    return -1; /* Allocation failed, can't check disk space */
  if (!lock)
  {
    FreeMem(info, sizeof(struct InfoData));
    return -2; /* Unable to lock the path, can't check disk space */
  }

  result = 0; /* Default to 0, meaning there's enough space */

  if (Info(lock, info))
  {
    /* Convert available blocks to bytes and then to megabytes */
    free_space = ((long)info->id_NumBlocks - (long)info->id_NumBlocksUsed) * (long)info->id_BytesPerBlock / 1024 / 1024;

#ifdef DEBUG
    printf("Free space: %ld\n", free_space);
#endif

    if (free_space < 0)
    {
      result = 0; /* Assume very large disk, so return 0 */
    }
    else if (free_space < min_space_mb)
    {
      result = -3; /* Not enough space */
    }
  }
  else
  {
    result = -4; /* Info call failed */
  }

  UnLock(lock);
  FreeMem(info, sizeof(struct InfoData));
  return result;
}

int main(int argc, char *argv[])
{
  int i, disk_check_result;
  long elapsed_seconds, hours, minutes, seconds;

  /* Black text:  printf("\x1B[30m 30:\x1B[0m \n"); */
  /* White text:  printf("\x1B[31m 31:\x1B[0m \n"); */
  /* Blue text:   printf("\x1B[32m 32:\x1B[0m \n"); */
  /* Grey text:   printf("\x1B[33m 33:\x1B[0m \n"); */

  printf("\n");
  printf("\x1B[1m\x1B[32mWHDArchiveExtractor V%s\x1B[0m\x1B[0m  \n", version_number);

  printf(
      "\x1B[32mThis program is designed to automatically locate "
      "LHA and LZX archive files\nwithin nested subdirectories, "
      "extract their contents to a specified\ndestination, and preserve the original directory "
      "hierarchy in which the \narchives were located.\x1B[0m \n\n");

  if (!does_file_exist("c:lha"))
  {
    printf(
        "File c:lha does not exist. As this program requires it to "
        "extract the archives, it will now quit. Please install the "
        "latest version of lha.run from www.aminet.org\n");
    return 0;
  }

  if (!does_file_exist("c:unlzx"))
  {
    printf(
        "File c:unlzx does not exist. There are a few LZX compressed "
        "archives for WHDLoad.  This program will continue and ignore these "
        "archives until UnLZX is installed.  Please install the latest version "
        "of UnLZX2.lha from www.aminet.org\n");
  }

  if (argc < 3)
  {
    printf(
        "\x1B[1mUsage:\x1B[0m WHDArchiveExtractor <source_directory> "
        "<output_directory_path> [-enablespacecheck (experimental)] "
        "[-testarchivesonly] [-skipifdestexists] [-enablecustomicons] \n\n");
    return 1;
  }

  input_directory_path = argv[1];
  output_directory_path = argv[2];

  skip_disk_space_check = true;
  for (i = 3; i < argc; i++)
  {
    if (strcmp(argv[i], "-enablespacecheck") == 0)
    {
      skip_disk_space_check = false;
    }
    if (strcmp(argv[i], "-testarchivesonly") == 0)
    {
      test_archives_only = true;
    }
    if (strcmp(argv[i], "-skipifdestexists") == 0)
    {
      skip_if_dest_exists = true;
    }
    if (strcmp(argv[i], "-enablecustomicons") == 0)
    {
      enable_custom_icons = true;
    }
  }

  drawer_icon_options.use_custom_icons = enable_custom_icons ? TRUE : FALSE;

  remove_trailing_slash(input_directory_path);
  remove_trailing_slash(output_directory_path);

  input_file_path = input_directory_path;

  printf("\x1B[1mScanning directory:    \x1B[0m %s\n", input_directory_path);
  printf("\x1B[1mExtracting archives to:\x1B[0m %s\n", output_directory_path);
  if (enable_custom_icons)
  {
    printf("\x1B[1mCustom drawer icons:\x1B[0m enabled (source: %s)\n", drawer_icon_options.icons_dir);
  }
  else
  {
    printf("\x1B[1mCustom drawer icons:\x1B[0m disabled\n");
  }

  if (does_folder_exists(input_directory_path) == 0)
  {
    printf("\nUnable to find the source folder %s\n\n", input_directory_path);
    return 0;
  }
  if (does_folder_exists(output_directory_path) == 0)
  {
    printf("\nUnable to find the target folder %s\n\n", output_directory_path);
    return 0;
  }

  if (!skip_disk_space_check)
  {
    disk_check_result = check_disk_space(output_directory_path, 20);
    if (disk_check_result < 0)
    {
      /* To do: handle various error cases based on the result code*/
      printf(
          "\n\x1B[1mError:\x1B[0m Not enough space on the target drive "
          "or cannot check space.\n20MB minimum checked for.  To "
          "disable this check, do not launch the\nprogram with the "
          "\x1B[3m-enablespacecheck\x1B[23m command.\n\n");
      return 0;
    }
  }

  /* Start timer */
  start_time = time(NULL);

  get_directory_contents(input_directory_path, output_directory_path);

  /* Calculate elapsed time */
  elapsed_seconds = time(NULL) - start_time;
  hours = elapsed_seconds / 3600;
  minutes = (elapsed_seconds % 3600) / 60;
  seconds = elapsed_seconds % 60;
  printf(
      "Scanned \x1B[1m%d\x1B[0m directories and found \x1B[1m%d\x1B[0m "
      "archives.\n",
      num_directories_scanned, num_lha_archives_found + num_lzx_archives_found);
  printf(
      "Archives composed of \x1B[1m%d\x1B[0m LHA and \x1B[1m%d\x1B[0m "
      "LZX archives.\n",
      num_lha_archives_found, num_lzx_archives_found);

  if (num_archives_skipped_dest_exists > 0)
  {
    printf(
        "Skipped because destination existed: \x1B[1m%d\x1B[0m\n",
        num_archives_skipped_dest_exists);
  }

  if (num_destination_conflicts > 0)
  {
    printf(
        "Destination conflicts detected: \x1B[1m%d\x1B[0m\n",
        num_destination_conflicts);
  }

  if (num_destination_not_drawer_conflicts > 0)
  {
    printf(
        "Conflicts where destination was not a drawer: \x1B[1m%d\x1B[0m\n",
        num_destination_not_drawer_conflicts);
  }

  if (num_directories_created > 0)
  {
    printf(
        "Created destination drawers during prep: \x1B[1m%d\x1B[0m\n",
        num_directories_created);
  }

  if (enable_custom_icons)
  {
    printf(
        "Drawer icons attempted: \x1B[1m%d\x1B[0m, applied: \x1B[1m%d\x1B[0m\n",
        num_drawer_icons_attempted,
        num_drawer_icons_applied);

    if (num_drawer_icons_failed > 0)
    {
      printf(
          "Drawer icon warnings (non-fatal): \x1B[1m%d\x1B[0m\n",
          num_drawer_icons_failed);
    }
  }

  if (num_lzx_archives_found > 0)
  {
    if (!does_file_exist("c:unlzx"))
    {

      printf(
          "UnLZX is not installed.  \x1B[1m%d\x1B[0m LZX archives were found but not expanded.\n",
          num_lzx_archives_found);
    }
  }

  printf("\nElapsed time: \x1B[1m%ld:%02ld:%02ld\x1B[0m\n", hours, minutes, seconds);
  printErrors();
  printf("\nWHDArchiveExtractor V%s\n\n", version_number);
  return 0;
}
