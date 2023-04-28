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
#include <stdbool.h>

/*#define DEBUG*/

long start_time;
int archives_found;
int num_dir_scanned;
char version_number[] = "1.0.0";
char *source_file_path;
char *target_file_path;

STRPTR dir_path;
STRPTR output_directory;

void fix_dos_formatting(char *str);
char *remove_text(char *input_str, STRPTR text_to_remove);
int ends_with_lha(const char *filename);
char *get_file_path(const char *full_path);
void get_directory_contents(STRPTR dir_path, STRPTR output_directory);
int stop_app = 0; /* used to stop the app if the lha extraction fails */

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
    int input_len = strlen(input_str);
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

void remove_trailing_slash(char *str) {
    if (str != NULL && strlen(str) > 0 && str[strlen(str) - 1] == '/') {
        str[strlen(str) - 1] = '\0';
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
                while (ExNext(dir_lock, file_info_block) && stop_app ==0)
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
                                archives_found++;

                                /* Combine the extraction command, source path, and output path */
                                snprintf(extraction_command, sizeof(extraction_command), "lha  -T x \"%s\" \"%s/%s\"", current_file_path, output_directory, get_file_path(remove_text(current_file_path, source_file_path)));
                                fix_dos_formatting(extraction_command);
                                printf("%s\n", extraction_command);
                                /* Execute the command */
                                command_result = SystemTagList(extraction_command, NULL);

                                /* Check for error */
                                if (command_result != 0)
                                {
                                    printf("\nError: Failed to execute command lha for file %s\nPlease check the archive is not damaged, and there is enough space in the\ntarget directory.  The program will now quit to prevent possible\nflood of popup messages.\n", current_file_path);
                                    /* If the target directory is full, every single file lha attempts to extract will display a warning.  I've decided to stop the app here and allow it to exit gracefully.*/
                                    stop_app =1;
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

int main(int argc, char *argv[])
{

    int recurse, i;
    long elapsed_seconds, hours, minutes, seconds;

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
    output_directory = "DH0:WHDLoad/Beta";
#endif

    remove_trailing_slash(dir_path);
    remove_trailing_slash(output_directory);

    source_file_path = dir_path;

    if (!does_folder_exists(dir_path))
    {
        printf("Unable to find the source folder %s.\n", dir_path);
        return 0;
    }
    if (!does_folder_exists(output_directory))
    {
        printf("Unable to find the target folder %s.\n", output_directory);
        return 0;
    }    

    printf("Scanning directory: %s\n", dir_path);
    printf("Extracting archives to: %s\n", output_directory);

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
    printf("\nWHDArchiveExtractor V%s\n", version_number);
    return 0;
}
