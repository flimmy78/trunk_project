/*******************************************************************************
Copyright (c) 2012-2015, Autelan Networks. All rights reserved.
*******************************************************************************/
#include "utils.h"

#define println(fmt, args...)   printf(fmt "\n", ##args)
#define LOG(level, fmt, args...) do { 	\
	syslog(level, fmt, ##args);	\
	println(fmt, ##args);		\
} while(0)
#define D(fmt, args...)         LOG(LOG_DEBUG, fmt, ##args)
#define E(fmt, args...)         LOG(LOG_ERR, fmt, ##args)

#ifndef MAX_SCRIPT
#define MAX_SCRIPT      127
#endif

#ifndef MAX_CLIENTS
#define MAX_CLIENTS     32
#endif

#ifndef MAX_RUNTIME
#define MAX_RUNTIME     300 /* second */
#endif

#ifndef MAX_DELAY
#define MAX_DELAY       (500*1000)
#endif

#ifndef MIN_DELAY
#define MIN_DELAY       (100*1000)
#endif

struct app {
    struct list_head node;

    int pid;
    time_t create;
};

static struct {
    unsigned int ip;
    int port;
    
    int fd;
    int pid;
    char script[1+MAX_SCRIPT];  /* fullname, include path */
    char *script_name;          /* just filename, NOT include path */
    
    int count;
    struct list_head list;
} C = {
    .fd         = -1,
    .list       = LIST_HEAD_INIT(C.list),
};

static struct app *
app_new(int pid)
{
    struct app *app = (struct app *)malloc(sizeof(*app));
    if (NULL==app) {
        E("app_new no memory");
        
        return NULL;
    }
    memset(app, 0, sizeof(*app));
    INIT_LIST_HEAD(&app->node);
    app->pid = pid;
    time(&app->create);
    
    return app;
}

static inline void
app_free(struct app *app)
{
    free(app);
}

static int
app_insert(struct app *app)
{
    if (NULL==app) {
        return -EINVAL;
    }
    
    list_add_tail(&app->node, &C.list);
    C.count++;
}

static int
app_remove(struct app *app)
{
    if (NULL==app) {
        return -EINVAL;
    }
    
    C.count--;
    list_del(&app->node);
}

typedef int app_foreach_t(struct app *app, void *in, void *out, int *go_on);

static int
app_foreach(app_foreach_t *cb, void *in, void *out)
{
    struct app *app, *n;
    int err;
    int go_on;
    
    list_for_each_entry_safe(app, n, &C.list, node) {
        err = (*cb)(app, in, out, &go_on);
        if (err<0) {
            // log
        }
        
        if (go_on) {
            continue;
        } else {
            return err;
        }
    }

    return 0;
}

static int
find_cb(struct app *app, void *in, void *out, int *go_on, int remove)
{
    int pid = (int)in;
    struct app **item = (struct app **)out;

    if (pid==app->pid) {
        if (remove) {
            app_remove(app);
        }
        
        *item = app;
        *go_on = 0;
    } else {
        *go_on = 1;
    }

    return 0;
}


static int
app_find_cb(struct app *app, void *in, void *out, int *go_on)
{
    return find_cb(app, in, out, go_on, 0);
}

static struct app *
app_find(int pid)
{
    struct app *app = NULL;

    app_foreach(app_find_cb, (void *)pid, (void *)&app);

    return app;
}

static int
app_create(int pid)
{
    if (app_find(pid)) {
        return -EEXIST;
    } else {
        return app_insert(app_new(pid));
    }
}

static int
app_delete_cb(struct app *app, void *in, void *out, int *go_on)
{
    return find_cb(app, in, out, go_on, 1);
}

static int
app_delete(int pid)
{
    struct app *app = NULL;
    
    app_foreach(app_delete_cb, (void *)pid, (void *)&app);
    if (NULL==app) {
        return -ENOEXIST;
    }

    return 0;
}

static int
app_timeout_cb(struct app *app, void *in, void *out, int *go_on)
{
    time_t now = *(time_t *)in;

    if ((now - app->create) > MAX_RUNTIME) {
        D("app pid:%d timeout", app->pid);
        
        kill(app->pid, SIGKILL);
        
        app_remove(app);
    }

    *go_on = 1;
    
    return 0;
}


static int
app_timeout(void)
{
    time_t now = time(NULL);
    
    return app_foreach(app_timeout_cb, (void *)&now, NULL);
}

static void
app_wait(void)
{
    int pid;

    while((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        D("waited pid:%d", pid);
        
        app_delete(pid);
    }
}

static int
child(void)
{
    int fd;
    int err = 0;
    
    struct sockaddr_in addr;
    int size = sizeof(addr);
    
    fd = accept(C.fd, (struct sockaddr *)&addr, &size);
    if (fd<0) {
        E("accept error:%d", errno);
        
        return errno;
    }

    int pid = fork();
    if (0==pid) { // child
        close(0);
        close(1);
        
        dup2(fd, 0);
        dup2(fd, 1);
        
        close(fd);
        if (fd!=C.fd) {
            close(C.fd);
        }
        
        execl(C.script, C.script_name, NULL);

        E("execl error:%d", errno);
        err = errno;
    }
    else if (pid>0) { // father
        err = app_create(pid);
    }
    else { // (pid<0), error
        E("fork error:%d", errno);
        
        err = errno;
    }
    
    close(fd);

    return err;
}

static int
is_new(int *new)
{
    fd_set rset;
    struct timeval tv = {
        .tv_sec     = 0,
        .tv_usec    = MAX_DELAY,
    };
    
    FD_ZERO(&rset);
    FD_SET(C.fd, &rset);
    
    int err = select(C.fd+1, &rset, NULL, NULL, &tv);
    switch(err) {
        case -1:/* error */
            if (EINTR==errno) {
                // is breaked
                return 0;
            } else {
                E("select error:%d", errno);
		
                return errno;
            }
        case 0: /* timeout, retry */
            return 0;
        default: /* to accept */
            *new = 1;
            return 0;
    }
}

static void
service(void)
{
    int err;
    int pid;
    
    while(1) {
        int new = 0;

        err = is_new(&new);
        if (err) {
            usleep(MIN_DELAY);
        }
        else if (new) {
            err = child();
        }

        app_wait();
        
        app_timeout();
    }
}

static int
run(void)
{
    int fd = -1;
    int err = 0;
   
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd<0) {
        E("socket error:%d", errno);
        
        err = errno; goto error;
    }
    C.fd = fd;
    
    int opt = 1;
    err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (err<0) {
        err = errno; goto error;
    }
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = C.port,
        .sin_addr   = {
            .s_addr = C.ip,
        },
    };
    err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (err<0) {
        E("bind error:%d", errno);
        
        err = errno; goto error;
    }

    err = listen(fd, MAX_CLIENTS);
    if (err<0) {
        E("listen error:%d", errno);
        
        err = errno; goto error;
    }

    C.pid = getpid();
    
    service();
error:
    if (fd>0) {
        close(fd);
    }
    
    return err;
}

static int
usage(int argc, char *argv[])
{
    char *self = argv[0];
    
    println("%s", self);
    println("\t%s ip port script", self);
    
    return -EINVAL;
}

/*
* sinetd ip port script
*/
int main(int argc, char *argv[])
{
    char *ip    = argv[1];
    char *port  = argv[2];
    char *script= argv[3];
    int err = 0;

    if (4!=argc) {
        return usage(argc, argv);
    }
    
    /*
    * check script
    */
    struct stat st;
    err = stat(script, &st);
    if (err<0) {
        println("bad script %s", script);
        
        return -EINVAL;
    }
    else if (0x100 != (0x100 & st.st_mode)) {
        return -EINVAL;
    }
    
    int len = strlen(script);
    if (len > MAX_SCRIPT) {
        println("script too big, NOT more than %d", MAX_SCRIPT);
        
        return -EINVAL;
    }
    memcpy(C.script, script, len);
    C.script_name = basename(C.script);
    openlog(C.script_name, 0, LOG_USER);

    struct in_addr addr;
    if (0==inet_aton(ip, &addr)) {
        println("bad ip:%s", ip);
        
        return -EINVAL;
    }
    C.ip = addr.s_addr;
    
    int iport = atoi(port);
    if (0==iport) {
        println("bad port:%s", port);
        
        return -EINVAL;
    }
    C.port = htons(iport);
    
    err = run();
    if (err<0) {
        return err;
    }
    
    return 0;
}

/******************************************************************************/