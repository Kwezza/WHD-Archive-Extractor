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

  This program uses extensive recursive functions to scan the input
  directory and all its subdirectories for LHA and LZX archives.
  It must be compiled with the StackExtend flag to avoid stack overflows.
  As far as I know, this feature is only supported by the SAS/C compiler.

  v1.0.0 - 2023-04-02 - First version released.

  v1.1.0 - 2024-03-14 - Added support for LZX archives.  Download from
                        aminet.net/package/util/arc/lzx121r1
                      - Improved error handling.
                      - If the target folder already exists, it
                        will now be scanned for protected files and the
                        protection bits will be removed.  This is to allow
                        the extraction to replace the files if needed.
  
  v1.1.1 - 2024-03-15 - Fixed some typos and text alignment issues

  v1.2.0 - 2025-05-06 - Added support for UnLZX 2.16 and LZX 1.21. 
                        Both LZX extractors use different commands, which
                        are now properly supported. Previously, the
                        program misleadingly implied support for both.
                        - Special thanks to Ed Brindley for identifying 
                          this issue.

  v1.2.1 - 2025-05-10 - Resolved buffer overflow issues and addressed
                        a memory leak. Upgrading to this version is 
                        recommended.
                      - Added a version cookie for the dos "version"
                        command.

  This program is released under the MIT License.
*/  


#include <ctype.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



#define bool int
#define true 1
#define false 0
#define MAX_ERRORS 40
#define MAX_ERROR_LENGTH 256
#define DEBUG 1
#define BUFFER_SIZE 1024


#define PROGRAM_NAME "WHD Archive Extractor"
#define VERSION_STRING "1.2.1"
#define VERSION_DATE "09.05.2025"

bool skip_disk_space_check = false, test_archives_only = false;
char *input_file_path;
char *output_file_path;
char single_error_message[MAX_ERROR_LENGTH];
char error_messages_array[MAX_ERRORS][MAX_ERROR_LENGTH];

int  num_archives_found;
int  error_count = 0;
int  num_directories_scanned;
int  should_stop_app = 0; /* used to stop the app if the lha extraction fails */
long start_time;
int  reset_protection_bits = 1;
char lzx_extract_command[9];  
char lzx_extract_target_command[4];
const char *version = "$VER: " PROGRAM_NAME " " VERSION_STRING " (" VERSION_DATE ") compiled " __DATE__ " " __TIME__ "";


STRPTR input_directory_path;

STRPTR output_directory_path;

/* Function prototypes */
char *get_file_path(const char *full_path);
char *remove_text(char *input_str, STRPTR text_to_remove);
int   check_disk_space(STRPTR path, int min_space_mb);
int   does_file_exist(char *filename);
int   does_folder_exists(const char *folder_name);

void  sanitize_amiga_path(char *path);
void  get_directory_contents(STRPTR input_directory_path, STRPTR output_directory_path);
void  log_error(const char *errorMessage);
void  print_errors(void);
void  remove_trailing_slash(char *str);
char *find_first_directory(char *filePath);
char *get_file_extension(const char *filename, char *outputBuffer);
char *get_executable_version(const char *filePath);

int num_lzx_archives_found = 0;
int num_lha_archives_found = 0;



/**
 * @brief Sanitizes an Amiga file path in-place by correcting specific path issues.
 *
 * This function modifies the given path string to:
 * - Remove any slashes ('/') that immediately follow a colon (':').
 * - Replace double slashes ('//') with a single slash ('/').
 * - Ensure the result is null-terminated.
 *
 * The function assumes the input buffer is large enough to hold the sanitized path.
 * The sanitization is performed in-place, but a temporary buffer is used internally.
 *
 * @param path The file path string to sanitize. Must be a writable, null-terminated string.
 */
void sanitize_amiga_path(char *path)
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

/**
 * @brief Removes a specified prefix from the beginning of a string, if present.
 *
 * Checks if the string @p text_to_remove appears at the start of @p input_str.
 * If so, returns a pointer to the character in @p input_str immediately after the prefix.
 * If not, returns the original @p input_str pointer.
 *
 * @param input_str The input string to check and potentially modify.
 * @param text_to_remove The prefix to remove from the start of @p input_str.
 * @return A pointer to the resulting string (either after the prefix, or the original string if the prefix is not present).
 */
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



/**
 * @brief Extracts the file extension from a given filename and converts it to uppercase.
 *
 * Assumes the extension is exactly 4 characters long, including the dot.
 *
 * @param filename The name of the file from which to extract the extension.
 * @param outputBuffer A buffer to hold the uppercase extension, must be at least 5 characters long.
 * @return A pointer to the outputBuffer containing the uppercase extension, or NULL if the operation fails.
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
    /* old version: file_path = malloc(file_path_length + 1);*/
    file_path = (char *)AllocVec(file_path_length + 1, MEMF_CLEAR);

    if (file_path != NULL)
    {
      /* Copy the file path string to the newly allocated memory */
      strncpy(file_path, full_path, file_path_length);
      file_path[file_path_length] = '\0';
    }
  }

  return file_path;
}

/**
 * @brief Checks if a file exists by attempting to open it for reading.
 *
 * Tries to open the specified file in read mode. If successful, the file exists.
 *
 * @param filename The name (and path) of the file to check.
 * @return 1 if the file exists, 0 otherwise.
 */
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

/**
 * @brief Checks if a folder (directory) exists.
 *
 * Attempts to lock the specified folder for reading. If successful, the folder exists.
 *
 * @param folder_name The name (and path) of the folder to check.
 * @return 1 if the folder exists, 0 otherwise.
 */
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

/**
 * @brief Removes a trailing slash from a string, if present.
 *
 * Modifies the input string in-place to remove a trailing '/' character, if it exists.
 *
 * @param str The string to modify. Must be a writable, null-terminated string.
 */
void remove_trailing_slash(char *str)
{
  if (str != NULL && strlen(str) > 0 && str[strlen(str) - 1] == '/')
  {
    str[strlen(str) - 1] = '\0';
  }
}

/**
 * @brief Logs an error message to the error message array.
 *
 * Copies the provided error message into the global error message array and increments the error count.
 * Ensures the message is null-terminated and does not exceed the maximum allowed length.
 *
 * @param errorMessage The error message to log.
 */
void log_error(const char *errorMessage)
{
  strncpy(error_messages_array[error_count], errorMessage, MAX_ERROR_LENGTH);
  error_messages_array[error_count][MAX_ERROR_LENGTH - 1] = '\0'; /* Ensure null-termination */
  error_count++;
}

/**
 * @brief Prints all logged error messages to the console.
 *
 * If any errors have been logged, prints them in a formatted list. Otherwise, prints a message indicating no errors.
 */
void print_errors()
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

char *find_first_directory(char *filePath)
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
  char file_extension[5];
  char current_file_path[256];
  char ExtractCommand[20];
  char extraction_command[256];
  char ExtractTargetCommand[4];
  char *directoryName;
  char program_name[6];
  char fileCommandStore[256];
  LONG command_result;
  char *file_path_tmp = NULL;
  size_t needed = 0;
  struct FileInfoBlock *file_info_block;
  reset_protection_bits = 1;

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
            num_directories_scanned++;

            /* Safe construction of current_file_path */
            needed = strlen(input_directory_path) + 1 + strlen(file_info_block->fib_FileName) + 1;
            if (needed <= sizeof(current_file_path)) {
                strcpy(current_file_path, input_directory_path);
                strcat(current_file_path, "/");
                strcat(current_file_path, file_info_block->fib_FileName);
            } else {
                printf("Error: Path too long for current_file_path buffer.\n");
                log_error("Path too long for current_file_path buffer.");
                continue;
            }
            sanitize_amiga_path(current_file_path);

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
                file_path_tmp = get_file_path(remove_text(current_file_path, input_file_path));
                if (file_path_tmp) {
                    needed = strlen(output_directory_path) + 1 + strlen(file_path_tmp) + 1;
                    if (needed <= sizeof(fileCommandStore)) {
                        strcpy(fileCommandStore, output_directory_path);
                        strcat(fileCommandStore, "/");
                        strcat(fileCommandStore, file_path_tmp);
                    } else {
                        printf("Error: Path too long for fileCommandStore buffer.\n");
                        log_error("Path too long for fileCommandStore buffer.");
                        FreeVec(file_path_tmp);
                        file_path_tmp = NULL;
                        continue;
                    }
                } else {
                    printf("Error: get_file_path returned NULL.\n");
                    continue;
                }
                sanitize_amiga_path(fileCommandStore);
                printf("Extracting \x1B[1m%s\x1B[0m to \x1B[1m%s\x1B[0m\n", file_info_block->fib_FileName, fileCommandStore);
                FreeVec(file_path_tmp);
                file_path_tmp = NULL;
                if (strcmp(file_extension, ".LHA") == 0)
                {
                  num_lha_archives_found++;
                  strcpy(ExtractTargetCommand, "  \0");
                  strcpy(program_name, "lha");
                  if (test_archives_only)
                  {
                    strcpy(ExtractCommand, "t\0");
                  }
                  else
                  {
                    strcpy(ExtractCommand, "-T -M -N -m x\0");

                    if (reset_protection_bits == 1)
                    {

                      sprintf(extraction_command, "c:lha vq \"%s\" >ram:listing.txt", current_file_path);
                      sanitize_amiga_path(extraction_command);
                      SystemTagList(extraction_command, NULL);
                      directoryName = find_first_directory("ram:listing.txt");
                      if (directoryName != NULL)
                      {
                        file_path_tmp = get_file_path(remove_text(current_file_path, input_file_path));
                        if (file_path_tmp) {
                            needed = strlen(output_directory_path) + 1 + strlen(file_path_tmp) + 1 + strlen(directoryName) + 1;
                            if (needed <= sizeof(fileCommandStore)) {
                                strcpy(fileCommandStore, output_directory_path);
                                strcat(fileCommandStore, "/");
                                strcat(fileCommandStore, file_path_tmp);
                                strcat(fileCommandStore, "/");
                                strcat(fileCommandStore, directoryName);
                            } else {
                                printf("Error: Path too long for fileCommandStore buffer.\n");
                                log_error("Path too long for fileCommandStore buffer.");
                                FreeVec(file_path_tmp);
                                file_path_tmp = NULL;
                                continue;
                            }
                        } else {
                            printf("Error: get_file_path returned NULL.\n");
                            continue;
                        }
                        if (directoryName && strlen(fileCommandStore) + 1 < sizeof(fileCommandStore)) {
                          strncat(fileCommandStore, "/", sizeof(fileCommandStore) - strlen(fileCommandStore) - 1);
                        }
                        if (directoryName && strlen(fileCommandStore) + strlen(directoryName) < sizeof(fileCommandStore)) {
                          strncat(fileCommandStore, directoryName, sizeof(fileCommandStore) - strlen(fileCommandStore) - 1);
                        }
                        sanitize_amiga_path(fileCommandStore);
                        if (does_folder_exists(fileCommandStore) == 1)
                        {
                          file_path_tmp = get_file_path(remove_text(current_file_path, input_file_path));
                          if (file_path_tmp) {
                              needed = strlen(output_directory_path) + 1 + strlen(file_path_tmp) + 1 + strlen(directoryName) + strlen("/#? ALL rwed >NIL:") + 1;
                              if (needed <= sizeof(fileCommandStore)) {
                                  strcpy(fileCommandStore, output_directory_path);
                                  strcat(fileCommandStore, "/");
                                  strcat(fileCommandStore, file_path_tmp);
                                  strcat(fileCommandStore, "/");
                                  strcat(fileCommandStore, directoryName);
                                  strcat(fileCommandStore, "/#? ALL rwed >NIL:");
                              } else {
                                  printf("Error: Path too long for fileCommandStore buffer (protect).\n");
                                  log_error("Error: Path too long for extraction_command buffer.");
                                  FreeVec(file_path_tmp);
                                  file_path_tmp = NULL;
                                  continue;
                              }
                          } else {
                              printf("Error: get_file_path returned NULL.\n");
                              continue;
                          }
                          sanitize_amiga_path(fileCommandStore);
                          printf("Prepping any protected files for potential replacement...\n");
                          SystemTagList(fileCommandStore, NULL);
                          FreeVec(file_path_tmp);
                          file_path_tmp = NULL;
                        }
                        FreeVec(file_path_tmp);
                        file_path_tmp = NULL;
                      }
                      else
                      {
                        printf("Unable to get the file path from the LHA output for file %s.\n", current_file_path);
                      }
                    }
                    DeleteFile("ram:listing.txt");
                    ExtractCommand[0] = '\0';
                    strcpy(ExtractCommand, "-T -M -N -m x\0");
                  }
                }
                else
                {
                  num_lzx_archives_found++;
                  strcpy(ExtractTargetCommand, lzx_extract_target_command);
                  strcpy(program_name, "c:unlzx");
                  if (test_archives_only)
                  {
                    strcpy(ExtractCommand, "-v\0");
                  }
                  else
                  {
                    strcpy(ExtractCommand, lzx_extract_command);
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
                if (should_stop_app == 0)
                {
                  num_archives_found++;

                  /* Combine the extraction command, source
                   * path, and output path */
                  file_path_tmp = get_file_path(remove_text(current_file_path, input_file_path));
                  if (file_path_tmp) {
                      needed = strlen(program_name) + 1 + strlen(ExtractCommand) + 3 + strlen(current_file_path) + 3 + strlen(ExtractTargetCommand) + 3 + strlen(output_directory_path) + 1 + strlen(file_path_tmp) + 2 + 1;
                      if (needed <= sizeof(extraction_command)) {
                          strcpy(extraction_command, program_name);
                          strcat(extraction_command, " ");
                          strcat(extraction_command, ExtractCommand);
                          strcat(extraction_command, " \"");
                          strcat(extraction_command, current_file_path);
                          strcat(extraction_command, "\" ");
                          strcat(extraction_command, ExtractTargetCommand);
                          strcat(extraction_command, " \"");
                          strcat(extraction_command, output_directory_path);
                          strcat(extraction_command, "/");
                          strcat(extraction_command, file_path_tmp);
                          strcat(extraction_command, "\"");
                      } else {
                          printf("Error: Path too long for extraction_command buffer.\n");
                          log_error("Error: Path too long for extraction_command buffer.");
                          FreeVec(file_path_tmp);
                          file_path_tmp = NULL;
                          continue;
                      }
                  } else {
                      printf("Error: get_file_path returned NULL.\n");
                      continue;
                  }
                  sanitize_amiga_path(extraction_command);
                  FreeVec(file_path_tmp);
                  file_path_tmp = NULL;



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
                      log_error(single_error_message);
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
                      log_error(single_error_message);
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
  char *versionInfo;

  /* Black text:  printf("\x1B[30m 30:\x1B[0m \n"); */
  /* White text:  printf("\x1B[31m 31:\x1B[0m \n"); */
  /* Blue text:   printf("\x1B[32m 32:\x1B[0m \n"); */
  /* Grey text:   printf("\x1B[33m 33:\x1B[0m \n"); */

  printf("\n");
  printf("\x1B[1m\x1B[32mWHDArchiveExtractor V%s\x1B[0m\x1B[0m  \n", VERSION_STRING);

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
        "of lzx121r1.lha from www.aminet.org\n");

  }
  else
  {
    versionInfo = get_executable_version("c:unlzx");
    //printf("UnLZX version: %s", versionInfo);
    
    if (strcmp(versionInfo, "UnLZX 2.16") == 0)
    {
      strcpy(lzx_extract_command, "-x");
      strcpy(lzx_extract_target_command, "-o");
      printf("UnLZX version recognised as UnLZX 2.16.\n");
    }
    else if (strcmp(versionInfo, "LZX 1.21") == 0)
    {
      strcpy(lzx_extract_command, "-q -x e");
      strcpy(lzx_extract_target_command, "  ");
      printf("UnLZX version recognised as LZX 1.21 \n");
    }
    else
    {
      /* default to " e" for now */
      strcpy(lzx_extract_command, " e");
      printf("Unknown UnLZX version.  defaulting extraction command to %s\n", lzx_extract_command);
    }
    
    FreeVec(versionInfo);
  }

  if (argc < 2)
  {
    printf(
        "\x1B[1mUsage:\x1B[0m WHDArchiveExtractor <source_directory> "
        "<output_directory_path> [-enablespacecheck (experimental)] \n\n");
    return 1;
  }

  input_directory_path = argv[1];
  output_directory_path = argv[2];

  skip_disk_space_check = true;
  for (i = 4; i < argc; i++)
  {
    if (strcmp(argv[i], "-enablespacecheck") == 0)
    {
      skip_disk_space_check = false;
    }
    if (strcmp(argv[i], "-testarchivesonly") == 0)
    {
      test_archives_only = true;
    }
  }

  remove_trailing_slash(input_directory_path);
  remove_trailing_slash(output_directory_path);

  input_file_path = input_directory_path;

  printf("\x1B[1mScanning directory:    \x1B[0m %s\n", input_directory_path);
  printf("\x1B[1mExtracting archives to:\x1B[0m %s\n", output_directory_path);

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
  print_errors();
  printf("\nWHDArchiveExtractor V%s\n\n", VERSION_STRING);
  return 0;
}

char *get_executable_version(const char *filePath)
{
    char command[256];
    char *versionBuffer = NULL;
    FILE *versionFile;
    char line[256];
    int len;

    /* Create and execute the version command */
    sprintf(command, "version %s >ram:v.txt", filePath);
    SystemTagList((CONST_STRPTR)command, NULL);

    /* Open the version file */
    versionFile = fopen("ram:v.txt", "r");
    if (versionFile != NULL)
    {
        /* Read the first line */
        if (fgets(line, sizeof(line), versionFile) != NULL)
        {
            /* Remove trailing whitespace */
            len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' '))
            {
                line[--len] = '\0';
            }

            /* Allocate memory for the version string */
            versionBuffer = AllocVec(len + 1, MEMF_CLEAR);
            if (versionBuffer != NULL)
            {
                strcpy(versionBuffer, line);
            }
        }
        fclose(versionFile);
    }

    /* Delete the temporary file */
    DeleteFile("ram:v.txt");

    return versionBuffer;
}

