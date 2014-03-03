#include "private.h"

#include <Elementary.h>

#ifdef HAVE_AVAHI
# include <Ecore_Avahi.h>
# include <avahi-client/client.h>
# include <avahi-client/lookup.h>
# include <avahi-common/error.h>
#endif

#include "options_ssh.h"

#define FREE_CLEAN(h, cb) do { if (h) cb(h); h = NULL; } while (0);

typedef struct _SSH_Genlist SSH_Genlist;
struct _SSH_Genlist
{
   Evas_Object *gl;
   Elm_Object_Item *announced_servers;
   Elm_Object_Item *ww_servers;
   Eina_List *items;
};

struct _SSH_Server
{
   const char *domain;
   const char *name;
   const char *ip;
   unsigned int port;

   unsigned char announced : 1;
};

static Eina_List *announced_servers = NULL;
static Eina_List *ww_servers = NULL;
static Eina_List *genlists = NULL;

static Elm_Genlist_Item_Class *itc_group = NULL;
static Elm_Genlist_Item_Class *itc = NULL;

static void
_option_genlist_ssh_add(SSH_Genlist *genlist, unsigned char announced, const SSH_Server *server)
{
   Elm_Object_Item *parent;
   Elm_Object_Item *it;

   if (announced)
     {
        if (!genlist->announced_servers)
          {
             genlist->announced_servers = elm_genlist_item_append(genlist->gl, itc_group,
                                                              (void *)(uintptr_t)1 /* item data */,
                                                              NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
             elm_genlist_item_select_mode_set(genlist->announced_servers, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
          }
        parent = genlist->announced_servers;
     }
   else
     {
        if (!genlist->ww_servers)
          {
             genlist->ww_servers = elm_genlist_item_append(genlist->gl, itc_group, NULL,
                                                           NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
             elm_genlist_item_select_mode_set(genlist->ww_servers, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
          }
        parent = genlist->ww_servers;
     }

   it = elm_genlist_item_append(genlist->gl, itc, server /* item data */,
                                parent /* parent */,
                                ELM_GENLIST_ITEM_NONE,
                                NULL /* func */, // FIXME: there should the action come
                                NULL);

   genlist->items = eina_list_append(genlist->items, it);
}

static void
_option_genlist_ssh_del(SSH_Genlist *genlist, const SSH_Server *server)
{
   Elm_Object_Item *it;
   Eina_List *l, *ll;

   EINA_LIST_FOREACH_SAFE(genlist->items, l, ll, it)
     if (elm_object_item_data_get(it) == server)
       {
          elm_object_item_del(it);
          genlist->items = eina_list_remove_list(genlist->items, l);
       }
}

void
options_ssh_server_add(unsigned char announced, const char *domain, const char *name, const char *ip, unsigned int port)
{
   SSH_Server *srv;
   SSH_Genlist *genlist;
   Eina_List* l;

   srv = calloc(1, sizeof (SSH_Server));
   if (!srv) return ;

   srv->domain = eina_stringshare_add(domain);
   srv->name = eina_stringshare_add(name);
   srv->ip = eina_stringshare_add(ip);
   srv->port = port;
   srv->announced = announced;

   INF("Avahi discovered new host '%s' (%s:%i) from domain '%s'.",
       name, ip, port, domain);

   if (announced)
     announced_servers = eina_list_append(announced_servers, srv);
   else
     ww_servers = eina_list_append(ww_servers, srv);

   EINA_LIST_FOREACH(genlists, l, genlist)
     _option_genlist_ssh_add(genlist, announced, srv);
}

void
options_ssh_server_data_del(SSH_Server *server)
{
   SSH_Genlist *genlist;
   Eina_List *l;

   INF("Avahi discovered host disapeared '%s' (%s:%i) from domain '%s'.",
       server->name, server->ip, server->port, server->domain);

   EINA_LIST_FOREACH(genlists, l, genlist)
     _option_genlist_ssh_del(genlist, server);

   eina_stringshare_del(server->name);
   eina_stringshare_del(server->domain);
   eina_stringshare_del(server->ip);

   if (server->announced)
     announced_servers = eina_list_remove(announced_servers, server);
   else
     ww_servers = eina_list_remove(ww_servers, server);

   free(server);
}

void
options_ssh_server_del(unsigned char announced, const char *domain, const char *name)
{
   SSH_Server *srv;
   Eina_List *l;

   domain = eina_stringshare_add(domain);
   name = eina_stringshare_add(name);

   l = announced ? announced_servers : ww_servers;

   EINA_LIST_FOREACH(l, l, srv)
     if (srv->name == name &&
         srv->domain == domain)
       {
          options_ssh_server_data_del(srv);
          break;
       }

   eina_stringshare_del(domain);
   eina_stringshare_del(name);
}

static Eina_Bool
_options_genlist_death(void *data, Eo *obj EINA_UNUSED,
                       const Eo_Event_Description *desc EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   SSH_Genlist *genlist = data;

   eina_list_free(genlist->items);
   free(genlist);

   return EINA_TRUE;
}

void
options_genlist_add(Evas_Object *gl)
{
   SSH_Genlist *genlist;
   SSH_Server *srv;
   Eina_List *l;

   genlist = calloc(1, sizeof(SSH_Genlist));
   if (!genlist) return ;

   genlist->gl = gl;

   EINA_LIST_FOREACH(announced_servers, l, srv)
     _option_genlist_ssh_add(genlist, 1, srv);
   EINA_LIST_FOREACH(ww_servers, l, srv)
     _option_genlist_ssh_add(genlist, 0, srv);

   eo_do(gl, eo_event_callback_add(EO_EV_DEL, _options_genlist_death, genlist));
}

static char *
gl_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   SSH_Server *srv = data;
   Eina_Strbuf *buf;
   char *r;

   buf = eina_strbuf_new();

   if (srv->port != 22)
     eina_strbuf_append_printf(buf, "%s:%i", srv->name, srv->port);
   else
     eina_strbuf_append_printf(buf, "%s", srv->name);

   r = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   return r;
}

static char *
gl_group_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   if (data) return strdup("Announced server");
   return strdup("World wide server");
}

#ifdef HAVE_AVAHI
static Ecore_Avahi *context = NULL;
static AvahiClient *client = NULL;
static AvahiServiceBrowser *sb = NULL;
static int error = 0;

// FIXME: Check what it does
static void
_resolve_callback(AvahiServiceResolver *r,
                  AvahiIfIndex interface EINA_UNUSED,
                  AvahiProtocol protocol EINA_UNUSED,
                  AvahiResolverEvent event,
                  const char *name,
                  const char *type,
                  const char *domain,
                  const char *host_name EINA_UNUSED,
                  const AvahiAddress *address,
                  uint16_t port,
                  AvahiStringList *txt EINA_UNUSED,
                  AvahiLookupResultFlags flags EINA_UNUSED,
                  void* userdata EINA_UNUSED)
{
   /* Called whenever a service has been resolved successfully or timed out */
   switch (event)
     {
      case AVAHI_RESOLVER_FAILURE:
         ERR("Avahi failed to resolve service '%s' of type '%s' in domain '%s': %s\n",
             name, type, domain,
             avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
         break;

      case AVAHI_RESOLVER_FOUND:
        {
           char a[AVAHI_ADDRESS_STR_MAX];

           avahi_address_snprint(a, sizeof(a), address);
           options_ssh_server_add(1, domain, name, a, port);
        }
     }

   avahi_service_resolver_free(r);
}

static void
_browse_callback(AvahiServiceBrowser *b EINA_UNUSED,
                 AvahiIfIndex interface,
                 AvahiProtocol protocol,
                 AvahiBrowserEvent event,
                 const char *name,
                 const char *type,
                 const char *domain,
                 AvahiLookupResultFlags flags EINA_UNUSED,
                 void* userdata)
{
   AvahiClient *c = userdata;

   /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
   switch (event)
     {
      case AVAHI_BROWSER_FAILURE:
         // FIXME: What to do in case of failure ? Schedule another browser ?
         break;

      case AVAHI_BROWSER_NEW:
         /* We ignore the returned resolver object. In the callback
            function we free it. If the server is terminated before
            the callback function is called the server will free
            the resolver for us. */

         avahi_service_resolver_new(c, interface,
                                    protocol, name, type, domain,
                                    AVAHI_PROTO_UNSPEC, 0, _resolve_callback, c);

         break;

      case AVAHI_BROWSER_REMOVE:
         options_ssh_server_del(1, domain, name);
         break;

      case AVAHI_BROWSER_ALL_FOR_NOW:
      case AVAHI_BROWSER_CACHE_EXHAUSTED:
         /* Nothing to do in our case */
         break;
     }
}

static void
_client_callback(AvahiClient *c, AvahiClientState state, void * userdata EINA_UNUSED)
{
   switch (state)
     {
      case AVAHI_CLIENT_S_REGISTERING:
      case AVAHI_CLIENT_S_COLLISION:
      case AVAHI_CLIENT_S_RUNNING:
         if (!sb)
           {
              sb = avahi_service_browser_new(c, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_ssh._tcp", NULL, 0, _browse_callback, c);
              if (!sb)
                {
                   WRN("Failed to setup an Avahi Service Browser.");
                   return ;
                }
           }
         break;

      case AVAHI_CLIENT_FAILURE:
         WRN("Server connection failure: %s.", avahi_strerror(avahi_client_errno(c)));
         if (avahi_client_errno(c) == AVAHI_ERR_DISCONNECTED)
           {
              FREE_CLEAN(sb, avahi_service_browser_free);
              FREE_CLEAN(client, avahi_client_free);

              client = avahi_client_new(ecore_avahi_poll_get(context), AVAHI_CLIENT_NO_FAIL, _client_callback, NULL, &error);
              if (!client)
                {
                   WRN("Unable to setup an AvahiClient context.");
                   return ;
                }
              break;
           }

      case AVAHI_CLIENT_CONNECTING:
         FREE_CLEAN(sb, avahi_service_browser_free);
         break;
     }
}
#endif

void
options_ssh_init(void)
{
   if (!itc)
     {
        itc = elm_genlist_item_class_new();
        itc->item_style = "default";
        itc->func.text_get = gl_text_get;
        itc->func.content_get  = NULL;
        itc->func.state_get = NULL;
        itc->func.del = NULL;
     }
   if (!itc_group)
     {
        itc_group = elm_genlist_item_class_new();
        itc_group->item_style = "group_index";
        itc_group->func.text_get = gl_group_get;
        itc_group->func.content_get  = NULL;
        itc_group->func.state_get = NULL;
        itc_group->func.del = NULL;
     }

#ifdef HAVE_AVAHI
   if (context) return ;

   context = ecore_avahi_add();
   if (!context)
     {
        WRN("Unable to setup an Ecore_Avahi context.");
        return ;
     }

   client = avahi_client_new(ecore_avahi_poll_get(context), AVAHI_CLIENT_NO_FAIL, _client_callback, NULL, &error);
   if (!client)
     {
        WRN("Unable to setup an AvahiClient context.");
        options_ssh_shutdown();
        return ;
     }
#endif
}

void
options_ssh_shutdown(void)
{
   FREE_CLEAN(itc, elm_genlist_item_class_free);
   FREE_CLEAN(itc_group, elm_genlist_item_class_free);
#ifdef HAVE_AVAHI
   FREE_CLEAN(sb, avahi_service_browser_free);
   FREE_CLEAN(client, avahi_client_free);
   FREE_CLEAN(context, ecore_avahi_del);
#endif
}
