#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <linux/limits.h>
#define    CHAR_BACK   500

// * File handler structure
struct file_followed { long last_position; char filename[NAME_MAX]; struct file_followed * next; };
struct file_followed * file_list = NULL;

// * To quit peacefully
int cycle = 1;
void stopCycle(int u) { cycle = 0; }

// * Last tailed filename
char last_tailed[NAME_MAX];

void fileAdd(char * file) {
    struct file_followed ** list = &file_list;
    struct stat statdesc;

    if(stat(file, &statdesc) || !S_ISREG(statdesc.st_mode)) { return; }
    while(*list) { list = &((*list)->next); }
    *list = (struct file_followed*)malloc(sizeof(struct file_followed));
    (*list)->last_position = -1;
    strcpy((*list)->filename, file);
    (*list)->next = NULL;
}

void fileMod(char* fileName, struct file_followed* file_list) {
    struct file_followed* item = file_list;
    while(item) { 
        if(strcmp(item->filename, fileName) == 0) {
            FILE* fp = fopen(item->filename, "r");
            fseek(fp, 0, SEEK_END);
            long end_position = ftell(fp);
            fclose(fp);
            if (end_position <= item->last_position) {
                printf("\n** %s truncated **\n", fileName);
                item->last_position = -1;
            }
            usleep(100);
            return;
        }
        item = item->next;
    }
}

int fileTail(struct file_followed * item) {
    int ret = 0;
    FILE * fp = fopen(item->filename, "r");
    fseek(fp, 0, SEEK_END);
    long end_position = ftell(fp);

    if( end_position != item->last_position ) {
        if(strcmp(item->filename, last_tailed)) { strcpy(last_tailed, item->filename); printf("\n** %s **:\n", item->filename); }

        int start_position = item->last_position == -1 || item->last_position > end_position ? (end_position-CHAR_BACK > 0 ? end_position-CHAR_BACK : 0) : item->last_position;
                    fseek(fp, start_position, SEEK_SET);

        int len = end_position - start_position;
        char * buf = (char*)malloc(len+1);
        fread(buf, len, 1, fp);
        buf[len] = '\0';
        printf("%s%s", len == CHAR_BACK ? "[...]" : "", buf);
        free(buf);

        item->last_position = end_position;
        ret = 1;
    }

    fclose(fp);
    return ret;
}

void fileRem(char * file) {
    struct file_followed ** list = &file_list;
    while(*list && strcmp((*list)->filename, file)) { list = &((*list)->next); }
    if(*list) { struct file_followed * todel = *list; *list = (*list)->next; free(todel); }
}

int main(int argc, char ** argv) {

    struct dirent **namelist;
    struct stat statdesc;
    struct timeval tv;
    fd_set set;
    int fd;
    int wd;
    int r;

    // * Help
    if(stat(argv[1], &statdesc) || !S_ISDIR(statdesc.st_mode)) { printf("[usage] %s dir-to-monitor\n", argv[0]); exit(EXIT_FAILURE); }

    // * Init
    chdir(argv[1]);
    memset(last_tailed, 0, sizeof(last_tailed));
    signal(SIGINT, stopCycle);
    signal(SIGTERM, stopCycle);

    // * Inotify
    if( (fd = inotify_init()) < 0) { perror("inotify_init"); }
    if( (wd = inotify_add_watch( fd, ".", IN_CREATE | IN_MODIFY |IN_DELETE ) < 0)) { perror("inotify_add_watch"); }

    // * File add recursively on dirscan
    if( (r = scandir(".", &namelist, 0, alphasort)) < 0) { perror("scandir"); }
    while (r--) { fileAdd(namelist[r]->d_name); free(namelist[r]); }
    free(namelist);

    // * Neverending cycle
    while(cycle) {
        // * Select on inotify
        FD_ZERO(&set);
        FD_SET(fd, &set);
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
        if( (r = select(fd+1, &set, NULL, NULL, &tv)) == -1) { perror("select"); }

        // * New add or del on inotify
        if(r) {
            struct inotify_event * event;
            char buf[1024];
            if(read(fd, buf, 1024) <= 0) { perror("read"); }
            event = (struct inotify_event *) buf;
            if(event->mask & IN_MODIFY) { fileMod(event->name, file_list);} 
            else if(event->mask & IN_CREATE) { fileAdd(event->name); } 
            else if(event->mask & IN_DELETE) { fileRem(event->name); }
        }

        // * Check for new tails
        struct file_followed * list = file_list;
        int tailers = 0;
        while(list) { tailers += fileTail(list); list = list->next; }
        if(!tailers) { usleep(500000); }
    }

    // * Stop inotify
    inotify_rm_watch( fd, wd );
    close(fd);

    return EXIT_SUCCESS;
}