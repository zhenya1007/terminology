#ifndef _OPTIONS_SSH_H__
#define _OPTIONS_SSH_H__

typedef struct _SSH_Server SSH_Server;

void options_ssh_server_add(unsigned char local, const char *domain, const char *name, const char *ip, unsigned int port);
void options_ssh_server_data_del(SSH_Server *server);
void options_ssh_server_del(unsigned char local, const char *domain, const char *name);
void options_genlist_add(Evas_Object *gl);

void options_ssh_init(void);
void options_ssh_shutdown(void);

#endif
