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

// Function prototypes 
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


// testing prototypes



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
        printf("\n\x1B[1mErrors encountered during execution:\x1B[0m\n");
        for (i = 0; i < errorCount; i++)
        {
            printf("\x1B[1mError:\x1B[0m %d: %s\n", i + 1, errorMessages[i]);
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
                                    printf("Disk check result: %d\n", disk_check_result);
                                    if (disk_check_result < 0)
                                    {
                                        // To do: handle various error cases based on the result code
                                        printf("\x1B[1mError:\x1B[0m Not enough space on the target drive or cannot check space.\n20MB miniumum checked for.  To disable this check, launch the program\nwith the '-skipdiskcheck' command.\n");
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
                                                printf("\n\x1B[1mError:\x1B[0m Corrupt archive %s\n", current_file_path);
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
                                                printf("\n\x1B[1mError:\x1B[0m Failed to execute command lha for file %s\nPlease check the archive is not damaged, and there is enough space in the\ntarget directory.  The program will now quit to prevent possible\nflood of popup messages.\n", current_file_path);
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

        #ifdef DEBUG
            printf("Free space: %ld\n", free_space);
        #endif

        if (free_space < 0)
        {
            result = 0; // Assume very large disk, so return 0
        }
        else if (free_space < min_space_mb)
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


//********************************************
//***  unit tests

// void test_fix_dos_formatting() {
//     char path1[] = "//test/path//";
//     char path2[] = "C:/folder/:/file";
//     fix_dos_formatting(path1);
//     fix_dos_formatting(path2);
//     printf("Path1: %s\n",path1);
//     printf("Test fix_dos_formatting: %s\n", strcmp(path1, "/test/path/") == 0 ? "Passed" : "Failed");
//     printf("Path2: %s\n",path2);
//     printf("Test fix_dos_formatting: %s\n", strcmp(path2, "C/folder/file") == 0 ? "Passed" : "Failed");
// }

// void test_remove_text() {
//     char testStr1[] = "HelloWorld";
//     char testStr2[] = "TestString";
//     char *result1 = remove_text(testStr1, "Hello");
//     char *result2 = remove_text(testStr2, "String");
//     printf("Test remove_text: %s\n", strcmp(result1, "World") == 0 ? "Passed" : "Failed");
//     printf("Test remove_text: %s\n", strcmp(result2, "Test") == 0 ? "Passed" : "Failed");
// }

// void test_ends_with_lha() {
//     const char *filename1 = "game.lha";
//     const char *filename2 = "image.png";
//     printf("Test ends_with_lha: %s\n", ends_with_lha(filename1) ? "Passed" : "Failed");
//     printf("Test ends_with_lha: %s\n", !ends_with_lha(filename2) ? "Passed" : "Failed");
// }

// void test_get_file_path() {
//     const char *fullPath = "/folder/subfolder/file.txt";
//     char *path = get_file_path(fullPath);
//     printf("Test get_file_path: %s\n", strcmp(path, "/folder/subfolder/") == 0 ? "Passed" : "Failed");
//     free(path); // Assuming get_file_path uses dynamic memory allocation
// }

// void test_does_file_exist() {
//     const char *existingFile = "dh0:test";
//     const char *nonExistingFile = "dh0:nonexistentfile.txt";
//     printf("Test does_file_exist: %s\n", does_file_exist(existingFile) ? "Passed" : "Failed");
//     printf("Test does_file_exist: %s\n", !does_file_exist(nonExistingFile) ? "Passed" : "Failed");
// }

// void test_does_folder_exists() {
//     const char *existingFolder = "dh0:testfolder";
//     const char *nonExistingFolder = "dh0:nonexistentfolder";
//     printf("Test does_folder_exists: %s\n", does_folder_exists(existingFolder) ? "Passed" : "Failed");
//     printf("Test does_folder_exists: %s\n", !does_folder_exists(nonExistingFolder) ? "Passed" : "Failed");
// }

// void test_remove_trailing_slash() {
//     char pathWithSlash[] = "dh0:testfolder/";
//     char pathWithoutSlash[] = "dh0:testfolder";
//     remove_trailing_slash(pathWithSlash);
//     remove_trailing_slash(pathWithoutSlash);
//     printf("Test remove_trailing_slash: %s\n", strcmp(pathWithSlash, "dh0:testfolder") == 0 ? "Passed" : "Failed");
//     printf("Test remove_trailing_slash: %s\n", strcmp(pathWithoutSlash, "dh0:testfolder") == 0 ? "Passed" : "Failed");
// }

// void test_log_and_print_errors() {
//     // Clear previous logs if any
//     errorCount = 0;
//     memset(errorMessages, 0, sizeof(errorMessages));
    
//     // Log some test errors
//     logError("Error 1: File not found");
//     logError("Error 2: Access denied");
//     logError("Error 3: Read error on disk");
    
//     // Now print all logged errors
//     printf("Printing logged errors:\n");
//     printErrors();
//     printf("Test log_and_print_errors: %s\n", errorCount == 3 ? "Passed" : "Failed");
// }

//     // test_fix_dos_formatting();
//     // test_remove_text();
//     // test_ends_with_lha();
//     // test_get_file_path();
//     // test_does_file_exist();
//     // test_does_folder_exists();
//     // test_remove_trailing_slash();
//     // test_log_and_print_errors();

int main(int argc, char *argv[])
{

    int  i, disk_check_result;
    long elapsed_seconds, hours, minutes, seconds;


    // test_fix_dos_formatting();
    // test_remove_text();
    // test_ends_with_lha();
    // test_get_file_path();
    // test_does_file_exist();
    // test_does_folder_exists();
    // test_remove_trailing_slash();
    // test_log_and_print_errors();

    // printf("\x1B[30m 30:\x1B[0m \n");// black
    // printf("\x1B[31m 31:\x1B[0m \n");//white
    // printf("\x1B[32m 32:\x1B[0m \n");// Blue 
    // printf("\x1B[33m 33:\x1B[0m \n");// Grey

    printf("\n");
    printf("\x1B[1m\x1B[32mWHDArchiveExtractor V%s\x1B[0m\x1B[0m  \n", version_number);
    //printf("\x1B[32m Created to seach subfolders for LHA archives and extract to a new location\n and maintaining the folder structure the archive was found in.\x1B[0m \n\n");
    printf("\x1B[32mThis program is designed to automatically locate LHA archive files within\nnested subdirectories, extract their contents to a specified destination, \nand preserve the original directory hierarchy in which the archives\nwere located.\x1B[0m \n\n");

    if (argc < 2)
    {
        printf("\x1B[1mUsage:\x1B[0m WHDArchiveExtractor <source_directory> <output_directory>\n");
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
    dir_path = "WHD:";
    output_directory = "PC:WHDTarget";
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

    printf("\x1B[1mScanning directory:    \x1B[0m %s\n", dir_path);
    printf("\x1B[1mExtracting archives to:\x1B[0m %s\n", output_directory);

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
            printf("Disk check result: %d\n", disk_check_result);
            // To do: handle various error cases based on the result code
            printf("\n\x1B[1mError:\x1B[0m Not enough space on the target drive or cannot check space.\n20MB miniumum checked for.  To disable this check, launch the\nprogram with the \x1B[3m-skipdiskcheck\x1B[23m command.\n\n");
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
    printf("Scanned \x1B[1m%d\x1B[0m directories and found \x1B[1m%d\x1B[0m archives\n", num_dir_scanned, archives_found);
    printf("Elapsed time: \x1B[1m%ld:%02ld:%02ld\x1B[0m\n", hours, minutes, seconds);
    printErrors();
    printf("\nWHDArchiveExtractor V%s\n", version_number);
    return 0;
}



