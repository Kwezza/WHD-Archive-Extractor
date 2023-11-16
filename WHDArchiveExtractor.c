/*

  WHDArchiveExtractor

  This Amiga CLI program simplifies the extraction of large numbers of
  LHA archivescommonly used in WHDLoad. Designed with WHDLoad Download
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
// #include <stdbool.h>

#define bool int
#define true 1
#define false 0
#define MAX_ERRORS 40
#define MAX_ERROR_LENGTH 256

#define DEBUG

long start_time;
int archives_found;
int num_dir_scanned;
char version_number[] = "1.1.0";
char *source_file_path;
char *target_file_path;
char errorMessages[MAX_ERRORS][MAX_ERROR_LENGTH];
int errorCount = 0;
bool skip_disk_space_check = false;
char errorMessage[MAX_ERROR_LENGTH];
int stop_app = 0; /* used to stop the app if the lha extraction fails */

STRPTR dir_path;
STRPTR output_directory;

void fix_dos_formatting(char *str);
char *remove_text(char *input_str, STRPTR text_to_remove);
int ends_with_lha(const char *filename);
char *get_file_path(const char *full_path);
void get_directory_contents(STRPTR dir_path, STRPTR output_directory);
int does_file_exist(char *filename);
int does_folder_exists(const char *folder_name);
void remove_trailing_slash(char *str);
void logError(const char *errorMessage);
void printErrors(void);
int check_disk_space(STRPTR path, int min_space_mb);


/* Function to replace "//" with "/" and ":/" with ":" in input string */
void fix_dos_formatting(char *str)
{
    /* Variables to iterate over input string and updated string */
    char *p = str;
    char *q = str;

    /* Replace "//" with "/", ":/" with ":", and remove leading '/' */
    while (*p != '\0')
    {
        if (p == str && *p == '/')
        {
            /* Skip leading '/' */
            p++;
        }
        else if (*p == '/' && *(p + 1) == '/')
        {
            /* Replace '//' with '/' */
            *q = '/';
            p += 2;
        }
        else if (*p == ':' && *(p + 1) == '/')
        {
            /* Replace ':/' with ':' */
            *q = ':';
            p += 2;
        }
        else
        {
            /* Copy character from input to updated string */
            *q = *p;
            p++;
        }
        q++;
    }
    *q = '\0'; /* Add null terminator to end of updated string */
}

char *remove_text(char *input_str, STRPTR text_to_remove)
{
    // int input_len = strlen(input_str);
    int remove_len = strlen(text_to_remove);

    /* Check if the second string exists at the start of the first string */
    if (strncmp(input_str, text_to_remove, remove_len) == 0)
    {
        /* If the second string is found, return a pointer to the rest of the first string*/
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

    strncpy(errorMessages[errorCount], errorMessage, MAX_ERROR_LENGTH);
    errorMessages[errorCount][MAX_ERROR_LENGTH - 1] = '\0'; // Ensure null-termination
    errorCount++;
}


void printErrors()
{
    int i;
    if (errorCount > 0)
    {
        printf("\nErrors encountered during execution:\n");
        for (i = 0; i < errorCount; i++)
        {
            printf("Error %d: %s\n", i + 1, errorMessages[i]);
        }
    }
    else
    {
        printf("\nNo errors encountered.\n");
    }
}

void get_directory_contents(STRPTR dir_path, STRPTR output_directory)
{
    struct FileInfoBlock *file_info_block;
    BPTR dir_lock;
    char current_file_path[256];
    char extraction_command[256];
    LONG command_result;

    dir_lock = Lock((CONST_STRPTR)dir_path, ACCESS_READ);
    if (dir_lock)
    {
        file_info_block = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);
        if (file_info_block)
        {
            if (Examine(dir_lock, file_info_block))
            {
                while (ExNext(dir_lock, file_info_block) && stop_app == 0)
                {
                    if (strcmp(file_info_block->fib_FileName, ".") != 0 && strcmp(file_info_block->fib_FileName, "..") != 0)
                    {
                        strcpy(current_file_path, dir_path);
                        strcat(current_file_path, "/");
                        strcat(current_file_path, file_info_block->fib_FileName);

                        if (file_info_block->fib_DirEntryType > 0)
                        {
                            num_dir_scanned++;

                            get_directory_contents(current_file_path, output_directory);
                        }
                        else
                        {
                            if (ends_with_lha(file_info_block->fib_FileName))
                            {

                                // Check for disk space before extracting
                                if (!skip_disk_space_check)
                                {
                                    int disk_check_result = check_disk_space(output_directory, 20);
                                    if (disk_check_result < 0)
                                    {
                                        // To do: handle various error cases based on the result code
                                        printf("Error: Not enough space on the target drive or cannot check space.\n20MB miniumum checked for.  To disable this check, launch the program\nwith the '-skipdiskcheck' command.\n");
                                        stop_app = 1;
                                    }
                                    else
                                    {
                                        archives_found++;

                                        /* Combine the extraction command, source path, and output path */
                                        sprintf(extraction_command, "lha  -T x \"%s\" \"%s/%s\"", current_file_path, output_directory, get_file_path(remove_text(current_file_path, source_file_path)));
                                        fix_dos_formatting(extraction_command);
                                        printf("%s\n", extraction_command);
                                        /* Execute the command */
                                        command_result = SystemTagList(extraction_command, NULL);

                                        /* Check for error */

                                        if (command_result != 0)
                                        {
                                            if (command_result == 10)
                                            {
                                                printf("\nError: Corrupt archive %s\n", current_file_path);
                                                // Copy the first part of the message
                                                strncpy(errorMessage, current_file_path, MAX_ERROR_LENGTH - 1);
                                                errorMessage[MAX_ERROR_LENGTH - 1] = '\0'; // Ensure null-termination

                                                // Concatenate the error message if there's space
                                                if (strlen(errorMessage) + strlen(" is corrupt") < MAX_ERROR_LENGTH)
                                                {
                                                    strcat(errorMessage, " is corrupt");
                                                }
                                                logError(errorMessage);
                                            }
                                            else
                                            {
                                                printf("\nError: Failed to execute command lha for file %s\nPlease check the archive is not damaged, and there is enough space in the\ntarget directory.  The program will now quit to prevent possible\nflood of popup messages.\n", current_file_path);
                                                // Copy the first part of the message
                                                strncpy(errorMessage, current_file_path, MAX_ERROR_LENGTH - 1);
                                                errorMessage[MAX_ERROR_LENGTH - 1] = '\0'; // Ensure null-termination

                                                // Concatenate the error message if there's space
                                                if (strlen(errorMessage) + strlen(" failed to extract. Unknown error") < MAX_ERROR_LENGTH)
                                                {
                                                    strcat(errorMessage, " failed to extract. Unknown error");
                                                }
                                                logError(errorMessage);
                                            }
                                        }
                                        // if the number of errors is greater then MAX_ERRORS, then quit
                                        if (errorCount >= MAX_ERRORS)
                                        {
                                            printf("Maximum number of errors reached. Aborting.\n");
                                            stop_app = 1;
                                        }
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
        return -1; // Allocation failed, can't check disk space


    
    if (!lock)
    {
        FreeMem(info, sizeof(struct InfoData));
        return -2; // Unable to lock the path, can't check disk space
    }

    result = 0; // Default to 0, meaning there's enough space

    if (Info(lock, info))
    {
        // Convert available blocks to bytes and then to megabytes
        free_space = ((long )info->id_NumBlocks - (long )info->id_NumBlocksUsed) * (long )info->id_BytesPerBlock / 1024 / 1024;
        if (free_space < min_space_mb)
        {
            result = -3; // Not enough space
        }
    }
    else
    {
        result = -4; // Info call failed
    }

    UnLock(lock);
    FreeMem(info, sizeof(struct InfoData));
    return result;
}

int main(int argc, char *argv[])
{

    int  i, disk_check_result;
    long elapsed_seconds, hours, minutes, seconds;

    printf("\n");

    if (argc < 2)
    {
        printf("Usage: WHDArchiveExtractor <source_directory> <output_directory>\nWHD Archive Extractor V%s\n", version_number);
        return 1;
    }

    if (!does_file_exist("c:lha"))
    {
        printf("File c:lha does not exist. As this program requires it to extract the archives, it will now quit. Please install the latest version of lha.run from www.aminet.org\n");
        return 0;
    }

    dir_path = argv[1];

    output_directory = argv[2];
#ifdef DEBUG
    dir_path = "PC0:WHDLoad/Beta";
    output_directory = "DH1:";
#endif
    for (i = 3; i < argc; i++)
    {
        if (strcmp(argv[i], "-skipdiskcheck") == 0)
        {
            skip_disk_space_check = true;
        }
    }




    remove_trailing_slash(dir_path);
    remove_trailing_slash(output_directory);

    source_file_path = dir_path;


    printf("Scanning directory: %s\n", dir_path);
    printf("Extracting archives to: %s\n", output_directory);

    if (does_folder_exists(dir_path)==0)
    {
        printf("\nUnable to find the source folder %s\n\n", dir_path);
        return 0;
    }
    if (does_folder_exists(output_directory)==0)
    {
        printf("\nUnable to find the target folder %s\n\n", output_directory);
        return 0;
    }


    if (!skip_disk_space_check)
    {
        disk_check_result = check_disk_space(output_directory, 20);
        if (disk_check_result < 0)
        {
            // To do: handle various error cases based on the result code
            printf("\nError: Not enough space on the target drive or cannot check space.\n20MB miniumum checked for.  To disable this check, launch the\nprogram with the '-skipdiskcheck' command.\n");
            return 0;
        }
    }
    /* Start timer */
    start_time = time(NULL);

    get_directory_contents(dir_path, output_directory);

    /* Calculate elapsed time */
    elapsed_seconds = time(NULL) - start_time;
    hours = elapsed_seconds / 3600;
    minutes = (elapsed_seconds % 3600) / 60;
    seconds = elapsed_seconds % 60;
    printf("Scanned %d directories and found %d archives\n", num_dir_scanned, archives_found);
    printf("Elapsed time: %ld:%02ld:%02ld\n", hours, minutes, seconds);
    printErrors();
    printf("\nWHDArchiveExtractor V%s\n", version_number);
    return 0;
}
