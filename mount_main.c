#include "mount.h"

int main(int argc, char** argv)
{
  Mount mount;

  create(&mount);
  mount.loop(&mount);
  destroy(&mount);
}

