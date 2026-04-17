#ifndef ICON_APPLIER_H
#define ICON_APPLIER_H

#include <exec/types.h>

#define ICON_APPLIER_MAX_PATH 512
#define ICON_APPLIER_DEFAULT_ICONS_DIR "PROGDIR:Icons"

/*
 * Runtime options for icon application.
 * - use_custom_icons=FALSE disables all custom icon reads from icons_dir.
 * - icons_dir can be NULL to use ICON_APPLIER_DEFAULT_ICONS_DIR.
 */
typedef struct icon_applier_options
{
    BOOL use_custom_icons;
    const char *icons_dir;
} icon_applier_options;

/*
 * Ensures <dir_path>.info exists.
 * Order of preference:
 * 1) Custom icon from <icons_dir>/<leaf_name>.info when enabled and present.
 * 2) System default drawer icon (WBDRAWER).
 *
 * Returns TRUE if an icon exists already or creation succeeded.
 * Returns FALSE only on invalid input or write failure.
 */
BOOL icon_applier_ensure_drawer_icon(const char *dir_path,
                                     const icon_applier_options *options);

/*
 * Applies the template icon "WHD folder.info" to:
 * <target_directory>/<game_folder_name>.info
 *
 * Existing destination .info is unprotected and replaced.
 * Returns TRUE on success, FALSE on validation or write failure.
 */
BOOL icon_applier_apply_game_folder_icon(const char *target_directory,
                                         const char *game_folder_name,
                                         const icon_applier_options *options);

#endif /* ICON_APPLIER_H */
