#pragma once

#include "fatfs_blit_api.hpp"

void init_fs();
void update_fs();

// called when the USB host writes to storage
void storage_host_written();
// storage availability as seen by games (false until fatfs has caught up with host writes)
bool is_fs_available();

const char *get_save_path();
