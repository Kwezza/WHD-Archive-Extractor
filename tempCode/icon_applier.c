#include "icon_applier.h"

#include <stdio.h>
#include <string.h>

static BOOL icon_applier_exists(const char *path)
{
    BPTR lock;

    if (path == NULL || path[0] == '\0')
    {
        return FALSE;
    }

    lock = Lock(path, ACCESS_READ);
    if (lock == ZERO)
    {
        return FALSE;
    }

    UnLock(lock);
    return TRUE;
}

static void icon_applier_sanitize_path(char *path)
{
    char temp[ICON_APPLIER_MAX_PATH];
    size_t src;
    size_t dst;

    if (path == NULL)
    {
        return;
    }

    temp[0] = '\0';
    src = 0;
    dst = 0;

    while (path[src] != '\0' && dst < (sizeof(temp) - 1U))
    {
        if (path[src] == ':')
        {
            temp[dst++] = path[src++];

            while (path[src] == '/')
            {
                src++;
            }
        }
        else if (path[src] == '/' && path[src + 1U] == '/')
        {
            src++;
        }
        else if (strchr("*?#|<>\"{}", path[src]) != NULL)
        {
            temp[dst++] = '_';
            src++;
        }
        else if ((unsigned char)path[src] < 32U)
        {
            src++;
        }
        else
        {
            temp[dst++] = path[src++];
        }
    }

    while (dst > 0U && temp[dst - 1U] == '/')
    {
        dst--;
    }

    temp[dst] = '\0';

    if (dst < ICON_APPLIER_MAX_PATH)
    {
        strcpy(path, temp);
    }
}

static const char *icon_applier_get_icons_dir(const icon_applier_options *options)
{
    if (options == NULL || options->icons_dir == NULL || options->icons_dir[0] == '\0')
    {
        return ICON_APPLIER_DEFAULT_ICONS_DIR;
    }

    return options->icons_dir;
}

BOOL icon_applier_ensure_drawer_icon(const char *dir_path,
                                     const icon_applier_options *options)
{
    char info_path[ICON_APPLIER_MAX_PATH];
    char source_icon_name[ICON_APPLIER_MAX_PATH];
    const char *leaf_name;
    const char *icons_dir;
    struct DiskObject *diskobj;

    static int icons_dir_checked = 0;
    static BOOL icons_dir_exists = FALSE;

    if (dir_path == NULL || options == NULL)
    {
        return FALSE;
    }

    if (snprintf(info_path, sizeof(info_path), "%s.info", dir_path) >= (int)sizeof(info_path))
    {
        return FALSE;
    }

    icon_applier_sanitize_path(info_path);

    /* If an icon already exists, do not overwrite it. */
    if (icon_applier_exists(info_path))
    {
        return TRUE;
    }

    leaf_name = strrchr(dir_path, '/');
    if (leaf_name != NULL)
    {
        leaf_name++;
    }
    else
    {
        const char *colon;

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
            if (snprintf(source_icon_name,
                         sizeof(source_icon_name),
                         "%s/%s",
                         icons_dir,
                         leaf_name) < (int)sizeof(source_icon_name))
            {
                icon_applier_sanitize_path(source_icon_name);
                diskobj = GetDiskObject(source_icon_name);
            }
        }
    }

    /* Fallback keeps behavior predictable even without custom icon files. */
    if (diskobj == NULL)
    {
        diskobj = GetDefDiskObject(WBDRAWER);
        if (diskobj == NULL)
        {
            return FALSE;
        }
    }

    if (!PutDiskObject(dir_path, diskobj))
    {
        FreeDiskObject(diskobj);
        return FALSE;
    }

    FreeDiskObject(diskobj);
    return TRUE;
}

BOOL icon_applier_apply_game_folder_icon(const char *target_directory,
                                         const char *game_folder_name,
                                         const icon_applier_options *options)
{
    char template_icon_name[ICON_APPLIER_MAX_PATH];
    char dest_icon_name[ICON_APPLIER_MAX_PATH];
    char dest_info_path[ICON_APPLIER_MAX_PATH + 5];
    const char *icons_dir;
    struct DiskObject *diskobj;

    static int template_checked = 0;
    static BOOL template_exists = FALSE;

    if (target_directory == NULL || game_folder_name == NULL || options == NULL)
    {
        return FALSE;
    }

    if (!options->use_custom_icons)
    {
        return TRUE;
    }

    icons_dir = icon_applier_get_icons_dir(options);

    if (snprintf(template_icon_name,
                 sizeof(template_icon_name),
                 "%s/WHD folder",
                 icons_dir) >= (int)sizeof(template_icon_name))
    {
        return FALSE;
    }

    icon_applier_sanitize_path(template_icon_name);

    /* Cache template availability to avoid repeated filesystem checks. */
    if (!template_checked)
    {
        struct DiskObject *probe;

        probe = GetDiskObject(template_icon_name);
        if (probe != NULL)
        {
            FreeDiskObject(probe);
            template_exists = TRUE;
        }
        else
        {
            template_exists = FALSE;
        }

        template_checked = 1;
    }

    if (!template_exists)
    {
        return TRUE;
    }

    if (snprintf(dest_icon_name,
                 sizeof(dest_icon_name),
                 "%s/%s",
                 target_directory,
                 game_folder_name) >= (int)sizeof(dest_icon_name))
    {
        return FALSE;
    }

    icon_applier_sanitize_path(dest_icon_name);

    if (snprintf(dest_info_path,
                 sizeof(dest_info_path),
                 "%s.info",
                 dest_icon_name) >= (int)sizeof(dest_info_path))
    {
        return FALSE;
    }

    /* Replace destination icon atomically from the template where possible. */
    if (icon_applier_exists(dest_info_path))
    {
        if (!SetProtection(dest_info_path, 0))
        {
            return FALSE;
        }

        if (!DeleteFile(dest_info_path))
        {
            return FALSE;
        }
    }

    diskobj = GetDiskObject(template_icon_name);
    if (diskobj == NULL)
    {
        return FALSE;
    }

    if (!PutDiskObject(dest_icon_name, diskobj))
    {
        FreeDiskObject(diskobj);
        return FALSE;
    }

    FreeDiskObject(diskobj);
    return TRUE;
}
