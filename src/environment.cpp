/*****************************************************************************
 *
 * X testing environment - Google Test environment feat. dummy x server
 *
 * Copyright (C) 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include "xorg/gtest/environment.h"
#include "xorg/gtest/process.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include <X11/Xlib.h>

struct xorg::testing::Environment::Private {
  std::string path_to_conf;
  int display;
  Process process;
};

xorg::testing::Environment::Environment(const std::string& path, int display)
    : d_(new Private) {
  d_->path_to_conf = path;
  d_->display = display;
}

xorg::testing::Environment::~Environment() {
  delete d_;
}

void xorg::testing::Environment::SetUp() {
  static char display_string[6];
  snprintf(display_string, 6, ":%d", d_->display);

  if (d_->process.Start("Xorg", display_string, d_->path_to_conf.c_str())) {
    setenv("DISPLAY", display_string, true);

    for (int i = 0; i < 10; ++i) {
      Display* display = XOpenDisplay(NULL);

      if (display) {
        XCloseDisplay(display);
        return;
      }

      int status;
      int pid = d_->process.Wait(&status, WNOHANG);
      if (pid == d_->process.pid()) {
        FAIL() << "Dummy X server failed to start, did you run as root?";
        return;
      } else if (pid == 0) {
        sleep(1); /* Give the dummy X server some time to start */
        continue;
      } else if (pid == -1) {
        FAIL() << "Could not get status of dummy X server process: "
            << std::strerror(errno);
        return;
      } else {
        FAIL() << "Invalid child PID returned by waitpid()";
        return;
      }
    }

    FAIL() << "Unable to open connection to dummy X server";
  }
}

void xorg::testing::Environment::TearDown() {
  if (!d_->process.Terminate()) {
    FAIL() << "Warning: Failed to terminate dummy Xorg server: "
        << std::strerror(errno);
    if (!d_->process.Kill())
      FAIL() << "Warning: Failed to kill dummy Xorg server: "
          << std::strerror(errno);
  }
}
