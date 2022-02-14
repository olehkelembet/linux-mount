#include"mount.h"



static bool isRoot(void)
{
    return (geteuid() == 0);
}


static bool isDirectory(const char *path)
{
    struct stat s;
    int result = !(stat(path, &s)) ? S_ISDIR(s.st_mode): 0;
    return (bool)result;
}


static bool isMountPoint(const char *path)
{
    struct mntent *partition;
    FILE *mtab = setmntent("/etc/mtab", "r");
    if(!mtab)
    {
      //log(err);
      return false;
    }
    for(; partition; partition = getmntent(mtab))
    {
	if (strcmp(partition->mnt_dir, path) == 0) break;
    }
	endmntent(mtab);

	return true;
}


static bool  generateMountPointString(Mount* self)
{
    const char *propertyValue;
    char mountPoint[PATH_MAX] = {0};
    size_t len;

    if(!(propertyValue = udev_device_get_property_value(self->udevice, "ID_FS_LABEL")))
    {
      if(!(propertyValue = udev_device_get_property_value(self->udevice, "ID_MODEL")))
      {
	//log(can't get usb name)
	propertyValue = self->DEFAULT_MOUNT_NAME;
      }
    }

    struct passwd *pw = NULL;

    if (!(pw = getpwuid (geteuid())))
    {
      return false;
    }

    len = snprintf(mountPoint, PATH_MAX - (strlen(mountPoint) - 1), "%s%s%s%s", self->DEFAULT_MOUNT_PATH, pw->pw_name, self->SLASH, propertyValue );
	//len = strlen(mountPoint);

    if(len > PATH_MAX)
    {
      return false;
    }
    else
    {
      if(self->mountPointString)
      {
	 free(self->mountPointString)
	 self->mountPointString = strdup(mountPoint);
	 return true;
      }
    }

    return false;
}


static bool isMounted(Mount* self, const char* devnode)
{
	FILE *mtab;
	struct mntent *partition;
	bool ret = false;

	if (self->mountPoint)
    {
		self->mountPoint = NULL;
    }

	if ((mtab = setmntent("/etc/mtab", "r")))
    {
	for(
	    partition = getmntent(mtab);
	    partition && !ret;
	    partition = getmntent(mtab)
	   )
	{
	    if (strcmp(partition->mnt_fsname, devnode) == 0)
	    {
			ret = true;
			if (self->mountPoint)
	      {
				self->mountPoint = strdup(partition->mnt_dir);
	      }
	    }
	}

		endmntent(mtab);
	}

	return ret;
}


static int mountDevice(Mount* self)
{
	const char *devnode = udev_device_get_devnode(self->udevice);
	const char *fsType = udev_device_get_property_value(self->udevice, "ID_FS_TYPE");
	int status = 0;
	pid_t p;

	if (mkdir(self->mountPoint, S_IRWXU | S_IRGRP | S_IXGRP) == -1 &&
			errno != EEXIST) {
		error(0, errno, "Failed to create mount point %s\n", self->mountPoint);
	//log err
	//pop message "Failed to create mount point"
	}
	else {
		p = fork();

		if (p == -1) {
			perror("fork syscall error");
			fprintf(stderr, "unplug and plug you device again to retry\n");
	    //log err
	    //pop "reinsert device"
			return 1;
		}
		else if (p == 0)
	{
		  setuid(0);
		  perror("/sbin/mount");
	  if( devnode && self->mountPoint)
	  {
	    if (mount(devnode, self->mountPoint, (fsType) ? fsType : self->DEFAULT_FS_TYPE, MS_NOATIME, NULL))
	    {
		if (errno == EBUSY)
		{
		  //log()
		  //pop msg
		    printf("Mountpoint is busy!");
		}
		else
		{
		  //log()
		  //pop msg
		    printf("Mount error: %s", strerror(errno));
		}
	    }
	    else
	    {
	      //pop msg
		printf("Successfully mounted to %s!", self->mountPoint);
	    }
	  }
		  //exit(1);
	  return 1;
		}

		wait(&status);

		if (status != 0)
	{
			fprintf(stderr, "Failed to mount %s on %s\n", devnode, self->mountPoint);
	}
		else
	{
			printf("Device %s successfuly mounted on %s\n", devnode, self->mountPoint);
	}
	}

    return 0;
}


static void unmountDevice(Mount* self)
{
	const char *dev_node = udev_device_get_devnode(self->udevice);

	if (isMounted(self, dev_node))
    {
		if (self->mountPoint)
	{
			if (umount(self->mountPoint) == -1)
	    {
				printf("Failed to unmount device %s (mount point: %s)\n", dev_node, self->mountPoint);
			}
			else
	    {
				printf("Device %s successfuly unmounted!\nMount point was: %s)\n", dev_node, self->mountPoint);

				if (rmdir(self->mountPoint) == -1)
		{
					printf("Failed to delete: %s!\n", self->mountPoint);
		}
			}
		}
	}
}


static void log(const char* message)
{
	struct tm *t = gmtime(time(NULL));
	printf("[%.2d/%.2d/%.2d %.2d:%.2d:%.2d] : %s\n", t->tm_mday, t->tm_mon,
			1900 + t->tm_year, t->tm_hour, t->tm_min, t->tm_sec);
}


static void daemonize(Mount* self)
{
	int logfd;
	pid_t pid;

	pid = fork();
	if (pid == -1) {
		perror("fork() function problem.");
		exit(EXIT_FAILURE);
	}
	else if (pid != 0) {
		_exit(0);
	}

	if (setsid() == -1) {
		perror("setsid() function failure.");
		exit(EXIT_FAILURE);
	}

	umask(0);

	close(STDIN_FILENO);
	if (open("/dev/null", O_RDONLY) == -1) {
		perror("Stdin redirect issue.");
		exit(EXIT_FAILURE);
	}

	if ((logfd = open(self->LOGFILE, O_CREAT | O_RDWR | O_APPEND,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
		perror("Logfile opening problem.");
		exit(EXIT_FAILURE);
	}

	dup2(logfd, STDERR_FILENO);
	dup2(logfd, STDOUT_FILENO);
	close(logfd);
}


void loop(Mount* self)
{
    int fd = udev_monitor_get_fd(self->umon);

    while (1)
    {
	fd_set fds;
	struct timeval tv;
	int ret;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ret = select(fd+1, &fds, NULL, NULL, &tv);
	if (ret > 0 && FD_ISSET(fd, &fds))
	{
	    self->udevice = udev_monitor_receive_device(self->umon);
	    char*  devtype = udev_device_get_devtype(self->udevice);
	    char*  action = udev_device_get_action(self->udevice);
	    char*  devnode = udev_device_get_devnode(self->udevice);
	    if(strcmp(devtype, "partition")==0 && strcmp(action, "add") == 0)
	    {
		//printf("A new partition detected at: %s.\nTrying to mount to: %s.",devnode,self->DEFAULT_MOUNT_PATH);
		//log
		if(!isMounted(self, devnode))
		{
		  if(!generateMountPointString(self))
		  {
		    if(mountDevice(self) != 0)
		    {
		      //log fail
		      fprintf(stderr, "Failed to mount %s on %s\n", devnode, self->mountPoint);
		    }
		  }
		}
	    }
	    if(strcmp(devtype, "partition")==0 && strcmp(action, "remove") == 0)
	    {
		//printf("Partition removing.\nTrying to unmount.\n");
		//log
		unmountDevice(self);
	    }
	}
	else
	{
	    printf("No Device from call to receive_device() function!\n");
	}
	}
	usleep(250*1000);
	fflush(stdout);
}


void create(Mount* self)
{
  self->LOGFILE = "/var/log/mount_daemon.log";
  self->DEFAULT_MOUNT_NAME = "usb_label";
  self->DEFAULT_MOUNT_PATH = "/media/";
  self->DEFAULT_FS_TYPE = "vfat";
  self->SLASH = "/";

  self->mountPointString = NULL;
  self->mountPoint = NULL;

  self->udev = udev_new();
  if(self->udev == 0)
  {
    return;
  }

  self->umon = udev_monitor_new_from_netlink(self->udev, "udev");
  if(self->umon == 0)
  {
    return;
  }

  assert(udev_monitor_filter_add_match_subsystem_devtype(self->umon, "block", NULL) >=0);
  udev_monitor_enable_receiving(self->umon);

  self->isRoot = &isRoot;
  self->isDirectory = &isDirectory;
  self->isMountPoint = &isMountPoint;
  self->generateMountPointString = &generateMountPointString;
  self->isMounted = &isMounted;
  self->mountDevice = &mountDevice;
  self->unmountDevice = &unmountDevice;
  self->log = &log;
  self->daemonize = &daemonize;
  self->loop = &loop;
}


void destroy(Mount* self)
{
  if(self->mountPointString)
  {
    free(self->mountPointString);
    self->mountPointString = NULL;
  }

  if(self->mountPoint)
  {
    free(self->mountPoint);
    self->mountPoint = NULL;
  }

  udev_monitor_unref(self->umon);
  udev_device_unref(self->udevice);
  udev_unref(self->udev);
}

