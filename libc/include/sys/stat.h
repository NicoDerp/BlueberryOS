
#ifndef _SYS_STAT_H
#define _SYS_STAT_H 1

#include <sys/cdefs.h>
#include <stdint.h>

struct stat {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;
    uint32_t st_blksize;
    uint32_t st_blocks;
};



#define	S_IFMT   0170000   /* These bits determine file type.  */

/* File types.  */
#define	S_IFDIR  0040000   /* Directory.  */
#define	S_IFCHR  0020000   /* Character device.  */
#define	S_IFBLK  0060000   /* Block device.  */
#define	S_IFREG  0100000   /* Regular file.  */
#define	S_IFIFO  0010000   /* FIFO.  */
#define	S_IFLNK  0120000   /* Symbolic link.  */
#define	S_IFSOCK 0140000   /* Socket.  */


#define	S_ISDIR(mode)   (((mode) & S_IFMT) == S_IFDIR)
#define	S_ISCHR(mode)   (((mode) & S_IFMT) == S_IFCHR)
#define	S_ISBLK(mode)   (((mode) & S_IFMT) == S_IFBLK)
#define	S_ISREG(mode)   (((mode) & S_IFMT) == S_IFREG)
#define S_ISFIFO(mode)  (((mode) & S_IFMT) == S_IFIFO)
#define S_ISLNK(mode)   (((mode) & S_IFMT) == S_IFLNK)



#define S_IRWXU 0000700    /* RWX mask for owner */
#define S_IRUSR 0000400    /* R for owner */
#define S_IWUSR 0000200    /* W for owner */
#define S_IXUSR 0000100    /* X for owner */

#define S_IRWXG 0000070    /* RWX mask for group */
#define S_IRGRP 0000040    /* R for group */
#define S_IWGRP 0000020    /* W for group */
#define S_IXGRP 0000010    /* X for group */

#define S_IRWXO 0000007    /* RWX mask for other */
#define S_IROTH 0000004    /* R for other */
#define S_IWOTH 0000002    /* W for other */
#define S_IXOTH 0000001    /* X for other */

//#define S_ISUID 0004000    /* set user id on execution */
//#define S_ISGID 0002000    /* set group id on execution */
//#define S_ISVTX 0001000    /* save swapped text even after use */

#ifdef __cplusplus
extern "C" {
#endif

int stat(const char* __restrict, struct stat* __restrict);
int lstat(const char* __restrict, struct stat* __restrict);

#ifdef __cplusplus
}
#endif

#endif

