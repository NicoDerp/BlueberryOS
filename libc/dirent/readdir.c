
#include <dirent.h>
#include <bits/types/struct_DIR.h>
#include <stdio.h>

__attribute__((__nonnull__))
struct dirent* readdir(DIR* dir) {

    if (dir->dd_off == 0 || dir->dd_off >= dir->dd_size) {

        dir->dd_off = 0;
        dir->dd_size = getdirentries(dir->dd_fd, (char*) dir->dd_buf, sizeof(dir->dd_buf), &dir->dd_loc);

        if ((int) dir->dd_size <= 0)
            return (struct dirent*) 0;
    }

    struct dirent* ent = (struct dirent*) ((char*) dir->dd_buf + dir->dd_off);
    dir->dd_off += sizeof(struct dirent);
    return ent;
}

