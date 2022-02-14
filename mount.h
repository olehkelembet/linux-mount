#include <stdbool.h>
#include <libudev.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <stdio.h>
#include <mntent.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <assert.h>


typedef struct{

  struct udev *udev;
  struct udev_monitor *umon;
  struct udev_device *udevice;
  char *mount_point;

  const char* LOGFILE;
  const char* MOUNT_PREFIX;
  const char* NTFS3G_FS_TYPE;
  const char* EXTRA_CHARACTER;
  const char* DEFAULT_MOUNT_PATH;
  const char* DEFAULT_MOUNT_NAME;
  const char* DEFAULT_FS_TYPE;
  const char* SLASH;

  char *mountPointString;
  char *mountPoint;

  bool (*isRoot)(void);
  bool (*isDirectory)(const char*);
  bool (*isMountPoint)(const char*);
  bool (*generateMountPointString)(struct Mount*);
  bool (*isMounted)(struct Mount*, const char*);
  int (*mountDevice)(struct Mount*);
  bool (*unmountDevice)(struct Mount*);
  void (*log)(const char*);
  void (*daemonize)(struct Mount*);
  void (*loop)(struct Mount*);

}Mount;

