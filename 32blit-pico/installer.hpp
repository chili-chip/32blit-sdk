#pragma once

// Installs .blit files copied onto the USB drive (AUTO_INSTALL, loader only).
// Runs on the home screen once the computer has ejected the drive or gone away,
// then removes the installed copies from the drive.

// the computer wrote to the drive, look for new files once it lets go
void installer_request_scan();
void update_installer();

// true while installing, the drive reports "not ready" meanwhile
bool installer_active();
