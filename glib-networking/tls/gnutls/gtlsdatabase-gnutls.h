/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2010 Collabora, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the licence or (at
 * your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#ifndef __G_TLS_DATABASE_GNUTLS_H__
#define __G_TLS_DATABASE_GNUTLS_H__

#include <gio/gio.h>

#include "gtlscertificate-gnutls.h"

G_BEGIN_DECLS

typedef enum {
  G_TLS_DATABASE_GNUTLS_PINNED_CERTIFICATE = 1,
  G_TLS_DATABASE_GNUTLS_ANCHORED_CERTIFICATE = 2,
} GTlsDatabaseGnutlsAssertion;

#define G_TYPE_TLS_DATABASE_GNUTLS            (g_tls_database_gnutls_get_type ())
#define G_TLS_DATABASE_GNUTLS(inst)           (G_TYPE_CHECK_INSTANCE_CAST ((inst), G_TYPE_TLS_DATABASE_GNUTLS, GTlsDatabaseGnutls))
#define G_TLS_DATABASE_GNUTLS_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_TLS_DATABASE_GNUTLS, GTlsDatabaseGnutlsClass))
#define G_IS_TLS_DATABASE_GNUTLS(inst)        (G_TYPE_CHECK_INSTANCE_TYPE ((inst), G_TYPE_TLS_DATABASE_GNUTLS))
#define G_IS_TLS_DATABASE_GNUTLS_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), G_TYPE_TLS_DATABASE_GNUTLS))
#define G_TLS_DATABASE_GNUTLS_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS ((inst), G_TYPE_TLS_DATABASE_GNUTLS, GTlsDatabaseGnutlsClass))

typedef struct _GTlsDatabaseGnutlsPrivate                   GTlsDatabaseGnutlsPrivate;
typedef struct _GTlsDatabaseGnutlsClass                     GTlsDatabaseGnutlsClass;
typedef struct _GTlsDatabaseGnutls                          GTlsDatabaseGnutls;

struct _GTlsDatabaseGnutlsClass
{
  GTlsDatabaseClass parent_class;

  gboolean       (*lookup_assertion)      (GTlsDatabaseGnutls          *self,
                                           GTlsCertificateGnutls       *certificate,
                                           GTlsDatabaseGnutlsAssertion  assertion,
                                           const gchar                 *purpose,
                                           GSocketConnectable          *identity,
                                           GCancellable                *cancellable,
                                           GError                     **error);
};

struct _GTlsDatabaseGnutls
{
  GTlsDatabase parent_instance;
  GTlsDatabaseGnutlsPrivate *priv;
};

GType          g_tls_database_gnutls_get_type              (void) G_GNUC_CONST;

gboolean       g_tls_database_gnutls_lookup_assertion      (GTlsDatabaseGnutls          *self,
                                                            GTlsCertificateGnutls       *certificate,
                                                            GTlsDatabaseGnutlsAssertion  assertion,
                                                            const gchar                 *purpose,
                                                            GSocketConnectable          *identity,
                                                            GCancellable                *cancellable,
                                                            GError                     **error);

G_END_DECLS

#endif /* __G_TLS_DATABASE_GNUTLS_H___ */
