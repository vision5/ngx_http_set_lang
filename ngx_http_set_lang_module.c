
/*
 * Copyright (c) 2009 Marcus Clyne
 */


#include    <ndk.h>


// TODO : add setting a variable that dictates the method - $set_lang_method


static ngx_int_t    ngx_http_set_lang_cookie = 0;
static ngx_int_t    ngx_set_lang_method_var_index;

typedef struct      ngx_http_set_lang_loc_conf_s        ngx_http_set_lang_loc_conf_t;
typedef ngx_int_t   (*ngx_http_set_lang_func_pt)        (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);

static ngx_int_t    ngx_http_set_lang_from_accept_lang  (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_get          (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_post         (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_cookie       (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_geoip        (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_host         (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_referer      (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);
static ngx_int_t    ngx_http_set_lang_from_var          (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v);

static char *       ngx_http_set_lang                   (ngx_conf_t *cf, ngx_command_t *cmd, void *cnf);
static ngx_int_t    ngx_http_set_lang_from_methods      (ngx_http_request_t *r, ngx_str_t *v);

static ngx_int_t    ngx_http_set_lang_init              (ngx_conf_t *cf);
static void *       ngx_http_set_lang_create_loc_conf   (ngx_conf_t *cf);
static char *       ngx_http_set_lang_merge_loc_conf    (ngx_conf_t *cf, void *parent, void *child);


static  ndk_set_var_t      ngx_http_var_set_lang_def = {
    NDK_SET_VAR_BASIC,
    ngx_http_set_lang_from_methods,
    0,
    NULL
};




typedef struct {
    ngx_http_set_lang_func_pt       func;   // put func first because it's more efficient for calling
    ngx_str_t                       name;
} ngx_http_set_lang_method_t;


struct ngx_http_set_lang_loc_conf_s {
    ngx_int_t                       enable;
    ngx_flag_t                      set_cookie;
    ngx_flag_t                      set_method;
    ngx_int_t                       var_index;

    ngx_array_t                    *langs;
    ngx_array_t                    *indexes;
    ngx_array_t                    *hosts;
    ngx_array_t                    *referers;
    ngx_http_set_lang_method_t     *methods;

    ngx_str_t                       get_var;
    ngx_str_t                       post_var;
    ngx_str_t                       cookie;
};





static ngx_http_set_lang_method_t   ngx_http_set_lang_methods [] = {

    {ngx_http_set_lang_from_accept_lang,    ngx_string ("accept_lang")},
    {ngx_http_set_lang_from_get,            ngx_string ("get")},
    {ngx_http_set_lang_from_post,           ngx_string ("post")},
    {ngx_http_set_lang_from_cookie,         ngx_string ("cookie")},
    {ngx_http_set_lang_from_geoip,          ngx_string ("geoip")},
    {ngx_http_set_lang_from_host,           ngx_string ("host")},
    {ngx_http_set_lang_from_referer,        ngx_string ("referer")},
    {NULL,                                  ngx_null_string}
};



static ngx_command_t  ngx_http_set_lang_commands[] = {
    {
        ngx_string ("set_lang"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_1MORE,
        ngx_http_set_lang,
        0,
        0,
        &ngx_http_var_set_lang_def
    },
    {
        ngx_string ("set_lang_method"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, set_method),
        NULL
    },
    {
        ngx_string ("lang_cookie"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, cookie),
        NULL
    },
    {
        ngx_string ("lang_get_var"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, get_var),
        NULL
    },
    {
        ngx_string ("lang_list"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_1MORE,
        ndk_conf_set_str_array_multi_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, langs),
        NULL
    },
    {
        ngx_string ("lang_post_var"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, post_var),
        NULL
    },
    {
        ngx_string ("lang_host"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE2,
        ngx_conf_set_keyval_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, hosts),
        NULL
    },
    {
        ngx_string ("lang_referer"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE2,
        ngx_conf_set_keyval_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof (ngx_http_set_lang_loc_conf_t, referers),
        NULL
    },
    ngx_null_command
};


ngx_http_module_t  ngx_http_set_lang_module_ctx = {

    ngx_http_set_lang_init,                 // preconfig
    NULL,                                   // postconfig
    NULL,                                   // create main config
    NULL,                                   // init main config
    NULL,                                   // create server config
    NULL,                                   // merge server config
    ngx_http_set_lang_create_loc_conf,      // create location config
    ngx_http_set_lang_merge_loc_conf        // merge location config
};


ngx_module_t    ngx_http_set_lang_module = {

    NGX_MODULE_V1,
    &ngx_http_set_lang_module_ctx,          // module context
    ngx_http_set_lang_commands,             // module directives
    NGX_HTTP_MODULE,                        // module type
    NULL,                                   // init master
    NULL,                                   // init module
    NULL,                                   // init process
    NULL,                                   // init thread
    NULL,                                   // exit thread
    NULL,                                   // exit process
    NULL,                                   // exit master
    NGX_MODULE_V1_PADDING
};




static ngx_int_t
ngx_http_set_lang_from_methods (ngx_http_request_t *r, ngx_str_t *v)
{
    ngx_http_set_lang_loc_conf_t        *llcf;
    ngx_http_set_lang_method_t          *method;
    size_t                               len;
    u_char                              *cookie, *p;
    ngx_table_elt_t                     *cookie_hdr;
    ngx_int_t                            rc;
    ngx_http_variable_value_t           *mv;
    ngx_http_set_lang_func_pt            func;

    llcf = ngx_http_get_module_loc_conf (r, ngx_http_set_lang_module);

    if (!llcf->enable)
        return  NGX_DECLINED;

    // reset var index - NOTE : if a method that issues subrequests is added, then this won't always work;

    llcf->var_index = 0;


    // iterate over methods of getting lang value

    for (method = llcf->methods ; (func = method->func) ; method++) {

        rc = func (r, llcf, v);

        switch (rc) {

        case    NGX_OK :

            if (method->func != ngx_http_set_lang_from_cookie && llcf->set_cookie) {


                // TODO : add parsing of path and exipires on cookie

                // create lang cookie

                len = llcf->cookie.len + v->len + sizeof ("; path=/");

                cookie = ngx_pnalloc (r->pool, len + 1);
                if (cookie == NULL) {
                    return  NGX_ERROR;
                }

                p = ngx_copy (cookie, llcf->cookie.data, llcf->cookie.len);
                *p++ = '=';

                p = ngx_copy (p, v->data, v->len);
                p = ngx_copy (p, "; path=/", sizeof ("; path=/"));


                // add lang cookie to main response

                cookie_hdr = ngx_list_push (&r->main->headers_out.headers);
                if (cookie_hdr == NULL) {
                    return  NGX_ERROR;
                }

                cookie_hdr->hash = 1;
                cookie_hdr->key.len = sizeof ("Set-Cookie") - 1;
                cookie_hdr->key.data = (u_char *) "Set-Cookie";
                cookie_hdr->value.len = len;
                cookie_hdr->value.data = cookie;
            }

            if (llcf->set_method) {

                mv = ngx_http_get_indexed_variable (r, ngx_set_lang_method_var_index);

                mv->data = method->name.data;
                mv->len = method->name.len;
                mv->valid = 1;
                mv->no_cacheable = 0;
                mv->not_found = 0;
            }

            return  NGX_OK;

        case    NGX_DECLINED :

            continue;
        }

        return  NGX_ERROR;
    }   

    return  NGX_DECLINED;
}




static ngx_int_t
ngx_http_set_lang_from_accept_lang (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    ngx_str_t           accept_lang, *lang;
    u_char              *s, *m, *e;
    size_t              len;
    ngx_uint_t          i;

    if (r->headers_in.accept_language == NULL)
        return  NGX_DECLINED;

    accept_lang = r->headers_in.accept_language->value;

    s = accept_lang.data;
    e = accept_lang.data + accept_lang.len;

    do {

        // strip spaces

        while (*s == ' ') {
            s++;

            if (s == e)
                return  NGX_DECLINED;
        };

        m = s;


        // parse between ',' and ';'

        while (m < e && *m != ',' && *m != ';')  m++;            

        len = m-s;


        // compare to lang list

        lang = conf->langs->elts;

        for (i=conf->langs->nelts; i; i--,lang++) {

            if (len == lang->len && !ngx_strncasecmp (s, lang->data, len)) {

                v->data = s;
                v->len = len;

                return  NGX_OK;
            }
        }

        // strip weighting, ',' and ';'

        if (*m == ';')  while (m < e && *m != ',')  m++;
        if (*m == ',')  m++;

        s = m;

    } while (m < e);


    return  NGX_DECLINED;
}



static ngx_int_t
ngx_http_set_lang_from_get (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    ngx_str_t   value;

    if (ngx_http_arg (r, conf->get_var.data, conf->get_var.len, &value) == NGX_DECLINED)
        return  NGX_DECLINED;

    v->data = value.data;
    v->len = value.len;

    return  NGX_OK;
}



static ngx_int_t
ngx_http_set_lang_from_post (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    // TODO

    return  NGX_DECLINED;
}



static ngx_int_t
ngx_http_set_lang_from_cookie (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    ngx_str_t       cookie;

    if (ngx_http_parse_multi_header_lines (&r->headers_in.cookies, &conf->cookie, &cookie) == NGX_DECLINED)
        return  NGX_DECLINED;

    v->data = cookie.data;
    v->len = cookie.len;

    return  NGX_OK;
}




static ngx_int_t
ngx_http_set_lang_from_geoip (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    // TODO

    return  NGX_DECLINED;
}



static ngx_int_t
ngx_http_set_lang_from_host (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    ngx_keyval_t        *kv;
    ngx_uint_t           i;
    ngx_str_t            host;

    // TODO : change to a hashed version

    host = r->headers_in.host->value;
    kv = conf->hosts->elts;

    for (i=conf->hosts->nelts; i; i--, kv++) {

        if (kv->key.data[0] == '.') {

            // check wildcard domain suffixes (e.g. .example.fr ->fr)

            if (host.len > kv->key.len && 
                host.data [host.len - kv->key.len] == '.' &&
                !ngx_strncasecmp (host.data + host.len - kv->key.len, kv->key.data, kv->key.len - 1)) {

                v->data = kv->value.data;
                v->len = kv->value.len;

                return  NGX_OK;
            }

        // check static domains

        } else if (host.len == kv->key.len && !ngx_strncasecmp (host.data, kv->key.data, host.len)) {

            v->data = kv->value.data;
            v->len = kv->value.len;

            return  NGX_OK;
        }

        // TODO : regexes
    }

    return  NGX_DECLINED;
}



static ngx_int_t
ngx_http_set_lang_from_referer (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    ngx_keyval_t        *kv;
    ngx_uint_t           i;
    ngx_str_t            referer;

    // TODO : change to a hashed version

    if (r->headers_in.referer == NULL)
        return  NGX_DECLINED;

    referer = r->headers_in.referer->value;

    kv = conf->referers->elts;

    for (i=conf->referers->nelts; i; i--, kv++) {

        if (kv->key.data[0] == '.') {

            // check wildcard domain suffixes (e.g. .example.fr ->fr)

            if (referer.len > kv->key.len && 
                referer.data [referer.len - kv->key.len] == '.' &&
                !ngx_strncasecmp (referer.data + referer.len - kv->key.len, kv->key.data, kv->key.len - 1)) {

                v->data = kv->value.data;
                v->len = kv->value.len;

                return  NGX_OK;
            }

        // check static domains

        } else if (referer.len == kv->key.len && !ngx_strncasecmp (referer.data, kv->key.data, referer.len)) {

            v->data = kv->value.data;
            v->len = kv->value.len;

            return  NGX_OK;
        }

        // TODO : regexes
    }

    return  NGX_DECLINED;
}



static ngx_int_t
ngx_http_set_lang_from_var (ngx_http_request_t *r, ngx_http_set_lang_loc_conf_t *conf, ngx_str_t *v)
{
    ngx_http_variable_value_t   *value;
    ngx_int_t   *indexes;

    indexes = conf->indexes->elts;

    value = ngx_http_get_indexed_variable (r, indexes [conf->var_index]);
    if (value == NULL) {
        conf->var_index++;
        return  NGX_DECLINED;
    }

    v->data = value->data;
    v->len = value->len;

    return  NGX_OK;
}




static char *
ngx_http_set_lang (ngx_conf_t *cf, ngx_command_t *cmd, void *cnf)
{
    ngx_http_variable_t             *var;
    ngx_str_t                       *value;
    ngx_uint_t                       i;
    ngx_int_t                       *index;
    ngx_http_set_lang_loc_conf_t    *conf;
    ngx_http_set_lang_method_t      *method, *next_method;
    char                            *p;

    conf = ngx_http_conf_get_module_loc_conf (cf, ngx_http_set_lang_module);


    // check for duplicates

    if (conf->enable != NGX_CONF_UNSET)
        return  "is duplicate";


    value = cf->args->elts;
    value++;


    // check for validitiy

    if (value->data[0] != '$') {

        if (value->len == 2 && !ngx_strncasecmp (value->data, (u_char *) "on", 2)) {
            conf->enable = 1;
            return  NGX_CONF_OK;

        } else if (value->len == 3 && !ngx_strncasecmp (value->data, (u_char *) "off", 3)) {
            conf->enable = 0;
            return  NGX_CONF_OK;
        }

        ngx_conf_log_error (NGX_LOG_EMERG, cf, 0, "\"%V\" variable name does not start with '$' and not on/off", value);
        return  NGX_CONF_ERROR;
    }

    if (cf->args->nelts < 3) {
        ngx_conf_log_error (NGX_LOG_EMERG, cf, 0, "no set_lang methods defined");
        return  NGX_CONF_ERROR;
    }

    conf->enable = 1;


    // add the variable to the rewrite processes

    p = ndk_set_var (cf, cmd, cnf);
    if (p != NGX_CONF_OK)
        return  p;



    // parse set_from methods (adding blank one on end)

    method = ngx_pcalloc (cf->pool, (cf->args->nelts - 1) * sizeof (ngx_http_set_lang_method_t));
    if (method == NULL)
        return  NGX_CONF_ERROR;

    conf->methods = method;

    value++;

    for (i=cf->args->nelts-2; i; i--, value++, method++) {

        next_method = ngx_http_set_lang_methods;

        do {

            if (next_method->func == NULL) {
                ngx_conf_log_error (NGX_LOG_EMERG, cf, 0, "set from \"%V\" is not a valid method", value);
                return  NGX_CONF_ERROR;
            }

            if (value->data[0] == '$') {

                value->data++;
                value->len--;


                // add variable

                var = ngx_http_add_variable (cf, value, NGX_HTTP_VAR_CHANGEABLE);
                if (var == NULL)
                    return  NGX_CONF_ERROR;

                if (conf->indexes == NGX_CONF_UNSET_PTR) {

                    conf->indexes = ngx_array_create (cf->pool, 3, sizeof (ngx_int_t));
                    if (conf->indexes == NULL)
                        return  NGX_CONF_ERROR;
                }


                // add var index to list

                index = ngx_array_push (conf->indexes);
                if (index == NULL)
                    return  NGX_CONF_ERROR;

                *index = ngx_http_get_variable_index (cf, value);
                if (*index == NGX_ERROR)
                    return  NGX_CONF_ERROR;

                
                // add method to list

                method->func = ngx_http_set_lang_from_var;
                method->name.data = (u_char *) "variable";
                method->name.len = sizeof ("variable") - 1;     // TODO : include the variable name
                break;

            } else if (value->len == next_method->name.len && !ngx_strncasecmp (value->data, next_method->name.data, value->len)) {

                *method = *next_method;

                if (method->func == ngx_http_set_lang_from_cookie && conf->set_cookie == NGX_CONF_UNSET)
                    conf->set_cookie = 1;

                break;
            }

            next_method++;

        } while (1);
    }


    return  NGX_CONF_OK;
}


static ngx_int_t
ngx_http_set_lang_init (ngx_conf_t *cf)
{
    ngx_http_variable_t     *v;
    ngx_str_t                var_name = ngx_string ("set_lang_method");
    ngx_int_t                index;

    v = ngx_http_add_variable (cf, &var_name, 0);
    if (v == NULL) {
        return NGX_ERROR;
    }

    v->get_handler = ndk_http_rewrite_var;


    index = ngx_http_get_variable_index (cf, &var_name);
    if (index == NGX_ERROR) {
        return  NGX_ERROR;
    }

    ngx_set_lang_method_var_index = index;

    return  NGX_OK;
}



static void *
ngx_http_set_lang_create_loc_conf (ngx_conf_t *cf)
{
    ngx_http_set_lang_loc_conf_t    *conf;

    ndk_pcallocp_rn (conf, cf->pool);

    conf->enable        = NGX_CONF_UNSET;
    conf->set_cookie    = NGX_CONF_UNSET;
    conf->set_method    = NGX_CONF_UNSET;

    conf->langs         = NGX_CONF_UNSET_PTR;
    conf->methods       = NGX_CONF_UNSET_PTR;
    conf->indexes       = NGX_CONF_UNSET_PTR;

    return  conf;
}



static char *
ngx_http_set_lang_merge_loc_conf (ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_set_lang_loc_conf_t    *prev, *conf;
    ngx_http_set_lang_method_t      *method;
    ngx_http_set_lang_func_pt        func;

    prev = parent;
    conf = child;

    ngx_conf_merge_value        (conf->enable, prev->enable, 0);
    ngx_conf_merge_value        (conf->set_cookie, prev->set_cookie, 0);
    ngx_conf_merge_value        (conf->set_method, prev->set_method, 0);

    ngx_conf_merge_ptr_value    (conf->langs, prev->langs, NULL);
    ngx_conf_merge_ptr_value    (conf->methods, prev->methods, NULL);
    ngx_conf_merge_ptr_value    (conf->indexes, prev->indexes, NULL);

    if (conf->hosts == NULL)
        conf->hosts = prev->hosts;

    if (conf->referers == NULL)
        conf->referers = prev->referers;

    ngx_conf_merge_str_value    (conf->post_var, prev->get_var, "lang");
    ngx_conf_merge_str_value    (conf->get_var, prev->post_var, "lang");
    ngx_conf_merge_str_value    (conf->cookie, prev->cookie, "lang");


    if (conf->enable) {

        method = conf->methods;

        for ( ; (func = method->func) ; method++) {

            if (func == ngx_http_set_lang_from_host && conf->hosts == NULL) {
                ngx_conf_log_error (NGX_LOG_EMERG, cf, 0, "no hosts have been specified for set_lang (e.g. lang_host example.de de;)");
                return  NGX_CONF_ERROR;
            }

            if (func == ngx_http_set_lang_from_referer && conf->referers == NULL) {
                ngx_conf_log_error (NGX_LOG_EMERG, cf, 0, "no referers have been specified for set_lang (e.g. lang_referer example.de de;)");
                return  NGX_CONF_ERROR;
            }

            if (func == ngx_http_set_lang_from_accept_lang && conf->langs == NULL) {
                ngx_conf_log_error (NGX_LOG_EMERG, cf, 0, "no languages have been specified for set_lang (e.g.: lang_list en fr de;)");
                return  NGX_CONF_ERROR;
            }

            if (func == ngx_http_set_lang_from_cookie) {
                ngx_http_set_lang_cookie = 1;
            }
        }
    }

    return  NGX_CONF_OK;
}



