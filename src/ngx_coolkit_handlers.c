/*
 * Copyright (c) 2010, FRiCKLE Piotr Sikora <info@frickle.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ngx_coolkit_handlers.h"
#include "ngx_coolkit_module.h"


ngx_int_t
ngx_coolkit_override_method_handler(ngx_http_request_t *r)
{
    ngx_coolkit_loc_conf_t  *cklcf;
    ngx_coolkit_ctx_t       *ckctx;
    ngx_str_t                method;
    ngx_conf_bitmask_t      *b;
    ngx_uint_t               original, j;

    cklcf = ngx_http_get_module_loc_conf(r, ngx_coolkit_module);
    // 请求的模块上下文
    ckctx = ngx_http_get_module_ctx(r, ngx_coolkit_module);

    /* always test against original request method */
    if ((ckctx != NULL) && (ckctx->overridden_method != 0)) {
        original = ckctx->overridden_method;
    } else {
        original = r->method;
    }

    if ((cklcf->override_source) && (cklcf->override_methods & original)) {
        // 获得应该使用的 method，存储在 method 变量中
        if (ngx_http_complex_value(r, cklcf->override_source, &method)
            != NGX_OK)
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (method.len == 0) {
            return NGX_DECLINED;
        }

        b = ngx_coolkit_http_methods;
        for (j = 0; b[j].name.len; j++) {
            if ((b[j].name.len - 1 == method.len)
                && (ngx_strncasecmp(b[j].name.data, method.data, method.len)
                     == 0))
            {
                if (ckctx == NULL) {
                    ckctx = ngx_pcalloc(r->pool, sizeof(ngx_coolkit_ctx_t));
                    if (ckctx == NULL) {
                        return NGX_HTTP_INTERNAL_SERVER_ERROR;
                    }

                    /*
                     * set by ngx_pcalloc():
                     *
                     *     ckctx->overridden_method = 0
                     *     ckctx->overridden_method_name = { 0, NULL }
                     */

                    ngx_http_set_ctx(r, ckctx, ngx_coolkit_module);
                }

                // 保存原始 method
                if (ckctx->overridden_method == 0) {
                    ckctx->overridden_method = r->method;
                    ckctx->overridden_method_name = r->method_name;
                }

                // 更新请求方法
                r->method = b[j].mask;
                r->method_name = b[j].name;
                r->method_name.len--; /* "hidden" space */

                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "coolkit override method: %V", &method);

                return NGX_DECLINED;
            }
        }

        return NGX_DECLINED;
    }

    if ((ckctx != NULL) && (ckctx->overridden_method != 0)
        && (cklcf->override_source == NULL))
    {
        /*
         * Bring back original method in location with
         * "override_method off".
         * This mess happens because this handlers runs twice:
         * in server rewrite and (location) rewrite phases.
         */

        r->method = ckctx->overridden_method;
        r->method_name = ckctx->overridden_method_name;
    }

    return NGX_DECLINED;
}
