/*

  WHDArchiveExtractor

  This Amiga CLI program simplifies the extraction of large numbers of
  LHA archives commonly used in WHDLoad. Designed with WHDLoad Download
  Tool users in mind, it automates the process of searching for archives
  in all subfolders and extracting them to a specified target path while
  preserving the original folder structure.

  Key features of this program include:

  * Scanning input folders and subfolders for LHA archives
  * Extracting the archives using the LHA program to an output folder
  * Preserving the subfolder structure from the input folder during
    extraction
  * Extracting only new or updated files to avoid unnecessary duplication

  To use this program, ensure the LHA software is installed in the C:
  directory. You can download it from aminet.net/package/util/arc/lha.
  unlzx is also supported for LZX archives, and can be downloaded from
  aminet.net/package/util/arc/lzx121r1

  Created 02/04/2023 and compiled on the PC using VBCC.
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

bool skip_disk_space_check = false, test_archives_only = false;
char *input_file_path;
char *output_file_path;
char single_error_message[MAX_ERROR_LENGTH];
char error_messages_array[MAX_ERRORS][MAX_ERROR_LENGTH];
char version_number[] = "1.1.0";
int num_archives_found;
int error_count = 0;
int num_directories_scanned;
int should_stop_app = 0; /* used to stop the app if the lha extraction fails */
long start_time;
int resetProtectionBits = 1;

STRPTR input_directory_path;
STRPTR output_directory_path;

/* Function prototypes */
char *get_file_path(const char *full_path);
char *remove_text(char *input_str, STRPTR text_to_remove);
int check_disk_space(STRPTR path, int min_space_mb);
int does_file_exist(char *filename);
int does_folder_exists(const char *folder_name);
int ends_with_lha(const char *filename);
void sanitizeAmigaPath(char *path);
void get_directory_contents(STRPTR input_directory_path, STRPTR output_directory_path);
void logError(const char *errorMessage);
void printErrors(void);
void remove_trailing_slash(char *str);
char *findFirstDirectory(char *filePath);

int num_lzx_archives_found = 0;
int num_lha_archives_found = 0;


/*
 * Sanitizes an Amiga file path in-place by removing or replacing specific invalid character sequences.
 * Assumes the input buffer is large enough for the sanitized path.
 * It replaces "//" with "/", ":/" with ":", and removes consecutive colons.
 * Parameters:
 *     char *path - Pointer to the original file path string to be modified in place.
 */

/*
 * Function to sanitize an Amiga file path in-place by correcting specific path issues.
 * It ensures no slashes immediately follow a colon, replaces "//" with "/", and removes consecutive colons.
 * Assumes the input buffer is large enough for the sanitized path.
 */
void sanitizeAmigaPath(char *path) {
    ULONG len;
    char *sanitizedPath;
    int i, j;

    if (path == NULL) {
        return;
    }

    len = strlen(path) + 1; // Include null terminator
    sanitizedPath = (char *)AllocVec(len, MEMF_ANY | MEMF_CLEAR);
    if (sanitizedPath == NULL) {
        // Memory allocation failed
        return;
    }

    i = 0;
    j = 0;
    while (path[i] != '\0') {
        if (path[i] == ':') {
            // Copy the colon
            sanitizedPath[j++] = path[i++];
            // Skip all following slashes
            while (path[i] == '/') {
                i++;
            }
        } else if (path[i] == '/' && path[i + 1] == '/') {
            // Skip redundant slashes
            i++;
        } else {
            // Copy other characters
            sanitizedPath[j++] = path[i++];
        }
    }

    sanitizedPath[j] = '\0'; // Ensure the result is null-terminated

    // Copy back to the original path and free the allocated memory
    strcpy(path, sanitizedPath);
    FreeVec(sanitizedPath);
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

char *get_file_extension(const char *filename)
{
  const char *extensionStart;
  char *uppercaseExtension;
  int i = 0;
  size_t len = strlen(filename);
  if (len < 4)
  {
    return NULL; /* Return NULL if the string is too short */
  }

  extensionStart = filename + len - 4;
  uppercaseExtension = malloc(5 * sizeof(char)); /* 4 characters + null terminator */
  if (!uppercaseExtension)
  {
    return NULL; /* Return NULL if memory allocation fails */
  }
  for (i = 0; i < 4; i++)
  {
    uppercaseExtension[i] = toupper(extensionStart[i]);
  }
  uppercaseExtension[4] = '\0';

  return uppercaseExtension;
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
  char *file_extension;
  char current_file_path[256];
  char ExtractCommand[20];
  char extraction_command[256];
  char *directoryName;
  char program_name[6];
  char fileCommandStore[256];
  LONG command_result;

  struct FileInfoBlock *file_info_block;
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

              //printf("current_file_path: %s\n", current_file_path);
              //printf("input_file_path: %s\n", input_file_path);
              file_extension = get_file_extension(file_info_block->fib_FileName);
              //printf("File extension: %s\n", file_extension);
              if (strcmp(file_extension, ".LHA") == 0 || strcmp(file_extension, ".LZX") == 0)
              {
                sprintf(fileCommandStore, "%s/%s", output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)));
                sanitizeAmigaPath(fileCommandStore);
                printf("Extracting \x1B[1m%s\x1B[0m to \x1B[1m%s\x1B[0m\n",file_info_block->fib_FileName, fileCommandStore);
                if (strcmp(file_extension, ".LHA") == 0)
                {
                  num_lha_archives_found++;
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

                      sprintf(extraction_command, "lha vq \"%s\" >ram:listing.txt", current_file_path);
                      sanitizeAmigaPath(extraction_command);
                      SystemTagList(extraction_command, NULL);
                      directoryName = findFirstDirectory("ram:listing.txt");
                      if (directoryName != NULL)
                      {
                        sprintf(fileCommandStore, "%s/%s/%s", output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)), directoryName);
                        sanitizeAmigaPath(fileCommandStore);
                        if (does_folder_exists(fileCommandStore) ==1)
                        {
                          sprintf(fileCommandStore, "protect %s/%s/%s/#? ALL rwed >NIL:", output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)), directoryName);
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
                    DeleteFile("ram:listing.txt");
                    ExtractCommand[0] = '\0';
                    strcpy(ExtractCommand, "-T -M -N -m x\0");
                  }
                }
                else
                {
                  num_lzx_archives_found++;
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

                //printf("Target path: %s\n", get_file_path(remove_text(current_file_path, input_file_path)));
                /* Check for disk space before extracting */
                // printf("Skip disk space check: %d\n", skip_disk_space_check);
                should_stop_app = 0;
                if (skip_disk_space_check == false)
                {
                  int disk_check_result = check_disk_space(output_directory_path, 20);
                  // printf("Disk check result: %d\n", disk_check_result);
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
                  sprintf(extraction_command, "%s %s \"%s\" \"%s/%s\"", program_name, ExtractCommand, current_file_path, output_directory_path, get_file_path(remove_text(current_file_path, input_file_path)));
                  sanitizeAmigaPath(extraction_command);
                  printf("ExtractCommand: %s\n", ExtractCommand);
                  //

                  printf("Full command: %s\n", extraction_command);

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
      "\x1B[32mThis program is designed to automatically locate LHA and "
      "LZX archive files within\nnested subdirectories, extract their contents "
      "to a specified destination, \nand preserve the original directory "
      "hierarchy in which the archives\nwere located.\x1B[0m \n\n");

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
        "File c:unlzx does not exist. There are a few LZX compressed"
        "archives for WHDLoad.  This program will continue and ignore these"
        "archives until UnLZX is installed.  Please install the latest version"
        "of UnLZX2.lha from www.aminet.org\n");
  }

  if (argc < 2)
  {
    printf(
        "\x1B[1mUsage:\x1B[0m WHDArchiveExtractor <source_directory> "
        "<output_directory_path> [-enablespacecheck] [-testarchivesonly]\n\n");
    return 1;
  }

  input_directory_path = argv[1];
  output_directory_path = argv[2];

#ifdef DEBUG
  // input_directory_path = "WHD:";
  // output_directory_path = "PC:WHDTarget";
#endif

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
  printErrors();
  printf("\nWHDArchiveExtractor V%s\n\n", version_number);
  return 0;
}
