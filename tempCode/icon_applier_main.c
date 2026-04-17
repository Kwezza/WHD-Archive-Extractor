#include "icon_applier.h"

#include <stdio.h>
#include <string.h>

/*
 * Simple standalone front-end.
 *
 * Usage:
 *   icon_applier drawer <dir_path> [icons_dir]
 *   icon_applier game   <target_directory> <game_folder_name> [icons_dir]
 *
 * Notes:
 * - Custom icons are enabled by default in this sample front-end.
 * - Pass an alternate icons_dir to override PROGDIR:Icons.
 */
int main(int argc, char **argv)
{
    icon_applier_options options;
    const char *mode;
    BOOL ok;

    if (argc < 3)
    {
        printf("Usage:\n");
        printf("  %s drawer <dir_path> [icons_dir]\n", argv[0]);
        printf("  %s game   <target_directory> <game_folder_name> [icons_dir]\n", argv[0]);
        return 20;
    }

    options.use_custom_icons = TRUE;
    options.icons_dir = NULL;

    mode = argv[1];

    if (strcmp(mode, "drawer") == 0)
    {
        if (argc >= 4)
        {
            options.icons_dir = argv[3];
        }

        ok = icon_applier_ensure_drawer_icon(argv[2], &options);
        return ok ? 0 : 20;
    }

    if (strcmp(mode, "game") == 0)
    {
        if (argc < 4)
        {
            printf("Missing <game_folder_name> for game mode\n");
            return 20;
        }

        if (argc >= 5)
        {
            options.icons_dir = argv[4];
        }

        ok = icon_applier_apply_game_folder_icon(argv[2], argv[3], &options);
        return ok ? 0 : 20;
    }

    printf("Unknown mode: %s\n", mode);
    return 20;
}
