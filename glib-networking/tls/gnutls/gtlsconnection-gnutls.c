/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2009 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "glib.h"

#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "gtlsconnection-gnutls.h"
#include "gtlsbackend-gnutls.h"
#include "gtlscertificate-gnutls.h"
#include "gtlsinputstream-gnutls.h"
#include "gtlsoutputstream-gnutls.h"
#include "gtlsserverconnection-gnutls.h"

#ifdef HAVE_PKCS11
#include <p11-kit/pin.h>
#include "pkcs11/gpkcs11pin.h"
#endif

#include <glib/gi18n-lib.h>
#include <TIZEN.h>

static ssize_t g_tls_connection_gnutls_push_func (gnutls_transport_ptr_t  transport_data,
						  const void             *buf,
						  size_t                  buflen);
static ssize_t g_tls_connection_gnutls_pull_func (gnutls_transport_ptr_t  transport_data,
						  void                   *buf,
						  size_t                  buflen);

static void     g_tls_connection_gnutls_initable_iface_init (GInitableIface  *iface);
static gboolean g_tls_connection_gnutls_initable_init       (GInitable       *initable,
							     GCancellable    *cancellable,
							     GError         **error);

#ifdef HAVE_PKCS11
static P11KitPin*    on_pin_prompt_callback  (const char     *pinfile,
                                              P11KitUri      *pin_uri,
                                              const char     *pin_description,
                                              P11KitPinFlags  pin_flags,
                                              void           *callback_data);
#endif

static void g_tls_connection_gnutls_init_priorities (void);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GTlsConnectionGnutls, g_tls_connection_gnutls, G_TYPE_TLS_CONNECTION,
				  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
							 g_tls_connection_gnutls_initable_iface_init);
				  g_tls_connection_gnutls_init_priorities ();
				  );


enum
{
  PROP_0,
  PROP_BASE_IO_STREAM,
  PROP_REQUIRE_CLOSE_NOTIFY,
  PROP_REHANDSHAKE_MODE,
  PROP_USE_SYSTEM_CERTDB,
  PROP_DATABASE,
  PROP_CERTIFICATE,
  PROP_INTERACTION,
  PROP_PEER_CERTIFICATE,
  PROP_PEER_CERTIFICATE_ERRORS
};

struct _GTlsConnectionGnutlsPrivate
{
  GIOStream *base_io_stream;
  GPollableInputStream *base_istream;
  GPollableOutputStream *base_ostream;

  gnutls_certificate_credentials creds;
  gnutls_session session;

  GTlsCertificate *certificate, *peer_certificate;
  GTlsCertificateFlags peer_certificate_errors;
  gboolean require_close_notify;
  GTlsRehandshakeMode rehandshake_mode;
  gboolean is_system_certdb;
  GTlsDatabase *database;
  gboolean database_is_unset;
  gboolean need_handshake, handshaking, ever_handshaked;
  gboolean closing;

  GInputStream *tls_istream;
  GOutputStream *tls_ostream;

  GTlsInteraction *interaction;
  gchar *interaction_id;

  GError *error;
  GCancellable *cancellable;
  gboolean blocking;
#ifndef GNUTLS_E_PREMATURE_TERMINATION
  gboolean eof;
#endif
  GIOCondition internal_direction;
};

static gint unique_interaction_id = 0;

static void
g_tls_connection_gnutls_init (GTlsConnectionGnutls *gnutls)
{
  gint unique_id;

  gnutls->priv = G_TYPE_INSTANCE_GET_PRIVATE (gnutls, G_TYPE_TLS_CONNECTION_GNUTLS, GTlsConnectionGnutlsPrivate);

  gnutls_certificate_allocate_credentials (&gnutls->priv->creds);
  gnutls_certificate_set_verify_flags (gnutls->priv->creds,
				       GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT);

  gnutls->priv->need_handshake = TRUE;

  gnutls->priv->database_is_unset = TRUE;
  gnutls->priv->is_system_certdb = TRUE;

  unique_id = g_atomic_int_add (&unique_interaction_id, 1);
  gnutls->priv->interaction_id = g_strdup_printf ("gtls:%d", unique_id);

#ifdef HAVE_PKCS11
  p11_kit_pin_register_callback (gnutls->priv->interaction_id,
                                 on_pin_prompt_callback, gnutls, NULL);
#endif
}

static gnutls_priority_t priorities[2][2];

static void
g_tls_connection_gnutls_init_priorities (void)
{
  /* First field is "ssl3 only", second is "allow unsafe rehandshaking" */
#if ENABLE(TIZEN_TLS11_AND_TLS12_SUPPORT_DISABLE)
  gnutls_priority_init (&priorities[FALSE][FALSE],
			"NORMAL:%COMPAT:!VERS-TLS1.2:!VERS-TLS1.1",
			NULL);
#else
  gnutls_priority_init (&priorities[FALSE][FALSE],
			"NORMAL:%COMPAT",
			NULL);
#endif
  gnutls_priority_init (&priorities[TRUE][FALSE],
			"NORMAL:%COMPAT:!VERS-TLS1.2:!VERS-TLS1.1:!VERS-TLS1.0",
			NULL);
#if ENABLE(TIZEN_TLS11_AND_TLS12_SUPPORT_DISABLE)
  gnutls_priority_init (&priorities[FALSE][TRUE],
			"NORMAL:%COMPAT:!VERS-TLS1.2:!VERS-TLS1.1:%UNSAFE_RENEGOTIATION",
			NULL);
#else
  gnutls_priority_init (&priorities[FALSE][TRUE],
			"NORMAL:%COMPAT:%UNSAFE_RENEGOTIATION",
			NULL);
#endif
  gnutls_priority_init (&priorities[TRUE][TRUE],
			"NORMAL:%COMPAT:!VERS-TLS1.2:!VERS-TLS1.1:!VERS-TLS1.0:%UNSAFE_RENEGOTIATION",
			NULL);
}

static void
g_tls_connection_gnutls_set_handshake_priority (GTlsConnectionGnutls *gnutls)
{
  gboolean use_ssl3, unsafe_rehandshake;

  if (G_IS_TLS_CLIENT_CONNECTION (gnutls))
    use_ssl3 = g_tls_client_connection_get_use_ssl3 (G_TLS_CLIENT_CONNECTION (gnutls));
  else
    use_ssl3 = FALSE;
  unsafe_rehandshake = (gnutls->priv->rehandshake_mode == G_TLS_REHANDSHAKE_UNSAFELY);
  gnutls_priority_set (gnutls->priv->session,
		       priorities[use_ssl3][unsafe_rehandshake]);
}

static gboolean
g_tls_connection_gnutls_initable_init (GInitable     *initable,
				       GCancellable  *cancellable,
				       GError       **error)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (initable);
  int status;

  g_return_val_if_fail (gnutls->priv->base_istream != NULL &&
			gnutls->priv->base_ostream != NULL, FALSE);

  /* Make sure gnutls->priv->session has been initialized (it may have
   * already been initialized by a construct-time property setter).
   */
  g_tls_connection_gnutls_get_session (gnutls);

  status = gnutls_credentials_set (gnutls->priv->session,
				   GNUTLS_CRD_CERTIFICATE,
				   gnutls->priv->creds);
  if (status != 0)
    {
      g_set_error (error, G_TLS_ERROR, G_TLS_ERROR_MISC,
		   _("Could not create TLS connection: %s"),
		   gnutls_strerror (status));
      return FALSE;
    }

  /* Some servers (especially on embedded devices) use tiny keys that
   * gnutls will reject by default. We want it to accept them.
   */
  gnutls_dh_set_prime_bits (gnutls->priv->session, 256);

  gnutls_transport_set_push_function (gnutls->priv->session,
				      g_tls_connection_gnutls_push_func);
  gnutls_transport_set_pull_function (gnutls->priv->session,
				      g_tls_connection_gnutls_pull_func);
  gnutls_transport_set_ptr (gnutls->priv->session, gnutls);

  gnutls->priv->tls_istream = g_tls_input_stream_gnutls_new (gnutls);
  gnutls->priv->tls_ostream = g_tls_output_stream_gnutls_new (gnutls);

  return TRUE;
}

static void
g_tls_connection_gnutls_finalize (GObject *object)
{
  GTlsConnectionGnutls *connection = G_TLS_CONNECTION_GNUTLS (object);

  if (connection->priv->base_io_stream)
    g_object_unref (connection->priv->base_io_stream);

  if (connection->priv->session)
    gnutls_deinit (connection->priv->session);

  if (connection->priv->tls_istream)
    g_object_unref (connection->priv->tls_istream);
  if (connection->priv->tls_ostream) 
    g_object_unref (connection->priv->tls_ostream);

  if (connection->priv->creds)
    gnutls_certificate_free_credentials (connection->priv->creds);

  if (connection->priv->database)
    g_object_unref (connection->priv->database);
  if (connection->priv->certificate)
    g_object_unref (connection->priv->certificate);
  if (connection->priv->peer_certificate)
    g_object_unref (connection->priv->peer_certificate);

  g_clear_object (&connection->priv->interaction);

  if (connection->priv->error)
    g_error_free (connection->priv->error);

#ifdef HAVE_PKCS11
  p11_kit_pin_unregister_callback (connection->priv->interaction_id,
                                   on_pin_prompt_callback, connection);
#endif
  g_free (connection->priv->interaction_id);

  G_OBJECT_CLASS (g_tls_connection_gnutls_parent_class)->finalize (object);
}

static void
g_tls_connection_gnutls_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (object);
  GTlsBackend *backend;

  switch (prop_id)
    {
    case PROP_BASE_IO_STREAM:
      g_value_set_object (value, gnutls->priv->base_io_stream);
      break;

    case PROP_REQUIRE_CLOSE_NOTIFY:
      g_value_set_boolean (value, gnutls->priv->require_close_notify);
      break;

    case PROP_REHANDSHAKE_MODE:
      g_value_set_enum (value, gnutls->priv->rehandshake_mode);
      break;

    case PROP_USE_SYSTEM_CERTDB:
      g_value_set_boolean (value, gnutls->priv->is_system_certdb);
      break;

    case PROP_DATABASE:
      if (gnutls->priv->database_is_unset)
        {
          backend = g_tls_backend_get_default ();
          gnutls->priv->database =  g_tls_backend_get_default_database (backend);
          gnutls->priv->database_is_unset = FALSE;
        }
      g_value_set_object (value, gnutls->priv->database);
      break;

    case PROP_CERTIFICATE:
      g_value_set_object (value, gnutls->priv->certificate);
      break;

    case PROP_INTERACTION:
      g_value_set_object (value, gnutls->priv->interaction);
      break;

    case PROP_PEER_CERTIFICATE:
      g_value_set_object (value, gnutls->priv->peer_certificate);
      break;

    case PROP_PEER_CERTIFICATE_ERRORS:
      g_value_set_flags (value, gnutls->priv->peer_certificate_errors);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_connection_gnutls_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (object);
  GInputStream *istream;
  GOutputStream *ostream;
  gboolean system_certdb;
  GTlsBackend *backend;

  switch (prop_id)
    {
    case PROP_BASE_IO_STREAM:
      if (gnutls->priv->base_io_stream)
	{
	  g_object_unref (gnutls->priv->base_io_stream);
	  gnutls->priv->base_istream = NULL;
	  gnutls->priv->base_ostream = NULL;
	}
      gnutls->priv->base_io_stream = g_value_dup_object (value);
      if (!gnutls->priv->base_io_stream)
	return;

      istream = g_io_stream_get_input_stream (gnutls->priv->base_io_stream);
      ostream = g_io_stream_get_output_stream (gnutls->priv->base_io_stream);

      if (G_IS_POLLABLE_INPUT_STREAM (istream) &&
	  g_pollable_input_stream_can_poll (G_POLLABLE_INPUT_STREAM (istream)))
	gnutls->priv->base_istream = G_POLLABLE_INPUT_STREAM (istream);
      if (G_IS_POLLABLE_OUTPUT_STREAM (ostream) &&
	  g_pollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (ostream)))
	gnutls->priv->base_ostream = G_POLLABLE_OUTPUT_STREAM (ostream);
      break;

    case PROP_REQUIRE_CLOSE_NOTIFY:
      gnutls->priv->require_close_notify = g_value_get_boolean (value);
      break;

    case PROP_REHANDSHAKE_MODE:
      gnutls->priv->rehandshake_mode = g_value_get_enum (value);
      break;

    case PROP_USE_SYSTEM_CERTDB:
      system_certdb = g_value_get_boolean (value);
      if (system_certdb != gnutls->priv->is_system_certdb)
        {
          g_clear_object (&gnutls->priv->database);
          if (system_certdb)
            {
              backend = g_tls_backend_get_default ();
              gnutls->priv->database = g_tls_backend_get_default_database (backend);
            }
          gnutls->priv->is_system_certdb = system_certdb;
        }
      break;

    case PROP_DATABASE:
      g_clear_object (&gnutls->priv->database);
      gnutls->priv->database = g_value_dup_object (value);
      gnutls->priv->is_system_certdb = FALSE;
      gnutls->priv->database_is_unset = FALSE;
      break;

    case PROP_CERTIFICATE:
      if (gnutls->priv->certificate)
	g_object_unref (gnutls->priv->certificate);
      gnutls->priv->certificate = g_value_dup_object (value);
      break;

    case PROP_INTERACTION:
      g_clear_object (&gnutls->priv->interaction);
      gnutls->priv->interaction = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

gnutls_certificate_credentials
g_tls_connection_gnutls_get_credentials (GTlsConnectionGnutls *gnutls)
{
  return gnutls->priv->creds;
}

gnutls_session
g_tls_connection_gnutls_get_session (GTlsConnectionGnutls *gnutls)
{
  /* Ideally we would initialize gnutls->priv->session from
   * g_tls_connection_gnutls_init(), but we can't tell if it's a
   * client or server connection at that point... And
   * g_tls_connection_gnutls_initiable_init() is too late, because
   * construct-time property setters may need to modify it.
   */
  if (!gnutls->priv->session)
    {
      gboolean client = G_IS_TLS_CLIENT_CONNECTION (gnutls);
      gnutls_init (&gnutls->priv->session, client ? GNUTLS_CLIENT : GNUTLS_SERVER);
    }

  return gnutls->priv->session;
}

void
g_tls_connection_gnutls_get_certificate (GTlsConnectionGnutls *gnutls,
                                         gnutls_retr2_st      *st)
{
  GTlsCertificate *cert;

  cert = g_tls_connection_get_certificate (G_TLS_CONNECTION (gnutls));

  st->cert_type = GNUTLS_CRT_X509;
  st->ncerts = 0;

  if (cert)
      g_tls_certificate_gnutls_copy (G_TLS_CERTIFICATE_GNUTLS (cert),
                                     gnutls->priv->interaction_id, st);
}

static void
begin_gnutls_io (GTlsConnectionGnutls  *gnutls,
		 gboolean               blocking,
		 GCancellable          *cancellable)
{
  gnutls->priv->blocking = blocking;
  gnutls->priv->cancellable = cancellable;
  gnutls->priv->internal_direction = 0;
  if (cancellable)
    g_cancellable_push_current (cancellable);
  g_clear_error (&gnutls->priv->error);
}

static int
end_gnutls_io (GTlsConnectionGnutls  *gnutls,
	       int                    status,
	       GError               **error)
{
  if (gnutls->priv->cancellable)
    g_cancellable_pop_current (gnutls->priv->cancellable);
  gnutls->priv->cancellable = NULL;

  if (status >= 0)
    {
      g_clear_error (&gnutls->priv->error);
      return status;
    }

  if (gnutls->priv->handshaking && !gnutls->priv->ever_handshaked)
    {
      if (g_error_matches (gnutls->priv->error, G_IO_ERROR, G_IO_ERROR_FAILED) ||
	  status == GNUTLS_E_UNEXPECTED_PACKET_LENGTH ||
	  status == GNUTLS_E_FATAL_ALERT_RECEIVED ||
	  status == GNUTLS_E_DECRYPTION_FAILED ||
	  status == GNUTLS_E_UNSUPPORTED_VERSION_PACKET)
	{
	  g_clear_error (&gnutls->priv->error);
	  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_NOT_TLS,
			       _("Peer failed to perform TLS handshake"));
	  return GNUTLS_E_PULL_ERROR;
	}
    }

  if (gnutls->priv->error)
    {
      if (g_error_matches (gnutls->priv->error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
	status = GNUTLS_E_AGAIN;
      else
	G_TLS_CONNECTION_GNUTLS_GET_CLASS (gnutls)->failed (gnutls);
      g_propagate_error (error, gnutls->priv->error);
      gnutls->priv->error = NULL;
      return status;
    }
  else if (status == GNUTLS_E_REHANDSHAKE)
    {
      if (gnutls->priv->rehandshake_mode == G_TLS_REHANDSHAKE_NEVER)
	{
	  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_MISC,
			       _("Peer requested illegal TLS rehandshake"));
	  return GNUTLS_E_PULL_ERROR;
	}

      gnutls->priv->need_handshake = TRUE;
      return status;
    }
  else if (
#ifdef GNUTLS_E_PREMATURE_TERMINATION
	   status == GNUTLS_E_PREMATURE_TERMINATION
#else
	   status == GNUTLS_E_UNEXPECTED_PACKET_LENGTH && gnutls->priv->eof
#endif
	   )
    {
      if (gnutls->priv->require_close_notify)
	{
	  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_EOF,
			       _("TLS connection closed unexpectedly"));
	  G_TLS_CONNECTION_GNUTLS_GET_CLASS (gnutls)->failed (gnutls);
	  return status;
	}
      else
	return 0;
    }

  return status;
}

#define BEGIN_GNUTLS_IO(gnutls, blocking, cancellable)	\
  begin_gnutls_io (gnutls, blocking, cancellable);	\
  do {

#define END_GNUTLS_IO(gnutls, ret, errmsg, error)	\
  } while ((ret == GNUTLS_E_AGAIN ||			\
            ret == GNUTLS_E_WARNING_ALERT_RECEIVED) &&	\
           !gnutls->priv->error);			\
  ret = end_gnutls_io (gnutls, ret, error);		\
  if (ret < 0 && ret != GNUTLS_E_REHANDSHAKE && error && !*error) \
    {							\
      g_set_error (error, G_TLS_ERROR, G_TLS_ERROR_MISC,\
                   errmsg, gnutls_strerror (ret));	\
    }							\
  ;

gboolean
g_tls_connection_gnutls_check (GTlsConnectionGnutls  *gnutls,
			       GIOCondition           condition)
{
  if (!gnutls->priv->internal_direction)
    return TRUE;

  if (gnutls->priv->handshaking || gnutls->priv->closing)
    condition = gnutls->priv->internal_direction;

  if (condition & G_IO_IN)
    return g_pollable_input_stream_is_readable (gnutls->priv->base_istream);
  else
    return g_pollable_output_stream_is_writable (gnutls->priv->base_ostream);
}

typedef struct {
  GSource source;

  GTlsConnectionGnutls *gnutls;
  GObject              *stream;

  GSource              *child_source;
  GIOCondition          current_direction;
} GTlsConnectionGnutlsSource;

static gboolean
gnutls_source_prepare (GSource *source,
		       gint    *timeout)
{
  *timeout = -1;
  return FALSE;
}

static gboolean
gnutls_source_check (GSource *source)
{
  return FALSE;
}

static gboolean
gnutls_source_sync_child_source (GTlsConnectionGnutlsSource *gnutls_source)
{
  GTlsConnectionGnutls *gnutls = gnutls_source->gnutls;
  GSource *source = (GSource *)gnutls_source;
  GIOCondition direction;

  if (gnutls->priv->handshaking || gnutls->priv->closing)
    direction = gnutls->priv->internal_direction;
  else if (!gnutls_source->stream)
    return FALSE;
  else if (G_IS_TLS_INPUT_STREAM_GNUTLS (gnutls_source->stream))
    direction = G_IO_IN;
  else
    direction = G_IO_OUT;

  if (direction == gnutls_source->current_direction)
    return TRUE;

  if (gnutls_source->child_source)
    {
      g_source_remove_child_source (source, gnutls_source->child_source);
      g_source_unref (gnutls_source->child_source);
    }

  if (direction & G_IO_IN)
    gnutls_source->child_source = g_pollable_input_stream_create_source (gnutls->priv->base_istream, NULL);
  else
    gnutls_source->child_source = g_pollable_output_stream_create_source (gnutls->priv->base_ostream, NULL);

  g_source_set_dummy_callback (gnutls_source->child_source);
  g_source_add_child_source (source, gnutls_source->child_source);
  gnutls_source->current_direction = direction;
  return TRUE;
}

static gboolean
gnutls_source_dispatch (GSource     *source,
			GSourceFunc  callback,
			gpointer     user_data)
{
  GPollableSourceFunc func = (GPollableSourceFunc)callback;
  GTlsConnectionGnutlsSource *gnutls_source = (GTlsConnectionGnutlsSource *)source;
  gboolean ret;

  ret = (*func) (gnutls_source->stream, user_data);
  if (ret)
    ret = gnutls_source_sync_child_source (gnutls_source);

  return ret;
}

static void
gnutls_source_finalize (GSource *source)
{
  GTlsConnectionGnutlsSource *gnutls_source = (GTlsConnectionGnutlsSource *)source;

  g_object_unref (gnutls_source->gnutls);

  if (gnutls_source->child_source)
    g_source_unref (gnutls_source->child_source);
}

static gboolean
g_tls_connection_gnutls_source_closure_callback (GObject  *stream,
						 gpointer  data)
{
  GClosure *closure = data;

  GValue param = { 0, };
  GValue result_value = { 0, };
  gboolean result;

  g_value_init (&result_value, G_TYPE_BOOLEAN);

  g_value_init (&param, G_TYPE_OBJECT);
  g_value_set_object (&param, stream);

  g_closure_invoke (closure, &result_value, 1, &param, NULL);

  result = g_value_get_boolean (&result_value);
  g_value_unset (&result_value);
  g_value_unset (&param);

  return result;
}

static GSourceFuncs gnutls_source_funcs =
{
  gnutls_source_prepare,
  gnutls_source_check,
  gnutls_source_dispatch,
  gnutls_source_finalize,
  (GSourceFunc)g_tls_connection_gnutls_source_closure_callback,
  (GSourceDummyMarshal)g_cclosure_marshal_generic
};

GSource *
g_tls_connection_gnutls_create_source (GTlsConnectionGnutls  *gnutls,
				       GIOCondition           condition,
				       GCancellable          *cancellable)
{
  GSource *source, *cancellable_source;
  GTlsConnectionGnutlsSource *gnutls_source;

  source = g_source_new (&gnutls_source_funcs, sizeof (GTlsConnectionGnutlsSource));
  g_source_set_name (source, "GTlsConnectionGnutlsSource");
  gnutls_source = (GTlsConnectionGnutlsSource *)source;
  gnutls_source->gnutls = g_object_ref (gnutls);
  if (condition & G_IO_IN)
    gnutls_source->stream = G_OBJECT (gnutls->priv->tls_istream);
  else if (condition & G_IO_OUT)
    gnutls_source->stream = G_OBJECT (gnutls->priv->tls_ostream);
  gnutls_source_sync_child_source (gnutls_source);

  if (cancellable)
    {
      cancellable_source = g_cancellable_source_new (cancellable);
      g_source_set_dummy_callback (cancellable_source);
      g_source_add_child_source (source, cancellable_source);
      g_source_unref (cancellable_source);
    }

  return source;
}

static void
set_gnutls_error (GTlsConnectionGnutls *gnutls, GIOCondition direction)
{
  if (g_error_matches (gnutls->priv->error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    gnutls_transport_set_errno (gnutls->priv->session, EINTR);
  else if (g_error_matches (gnutls->priv->error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      gnutls_transport_set_errno (gnutls->priv->session, EAGAIN);
      gnutls->priv->internal_direction = direction;
    }
  else
    gnutls_transport_set_errno (gnutls->priv->session, EIO);
}

static ssize_t
g_tls_connection_gnutls_pull_func (gnutls_transport_ptr_t  transport_data,
				   void                   *buf,
				   size_t                  buflen)
{
  GTlsConnectionGnutls *gnutls = transport_data;
  ssize_t ret;

  /* If gnutls->priv->error is non-%NULL when we're called, it means
   * that an error previously occurred, but gnutls decided not to
   * propagate it. So it's correct for us to just clear it. (Usually
   * this means it ignored an EAGAIN after a short read, and now
   * we'll return EAGAIN again, which it will obey this time.)
   */
  g_clear_error (&gnutls->priv->error);

  if (gnutls->priv->blocking)
    {
      ret = g_input_stream_read (G_INPUT_STREAM (gnutls->priv->base_istream),
				 buf, buflen,
				 gnutls->priv->cancellable,
				 &gnutls->priv->error);
    }
  else
    {
      ret = g_pollable_input_stream_read_nonblocking (gnutls->priv->base_istream,
						      buf, buflen,
						      gnutls->priv->cancellable,
						      &gnutls->priv->error);
    }

  if (ret < 0)
    set_gnutls_error (gnutls, G_IO_IN);
#ifndef GNUTLS_E_PREMATURE_TERMINATION
  else if (ret == 0)
    gnutls->priv->eof = TRUE;
#endif

  return ret;
}

static ssize_t
g_tls_connection_gnutls_push_func (gnutls_transport_ptr_t  transport_data,
				   const void             *buf,
				   size_t                  buflen)
{
  GTlsConnectionGnutls *gnutls = transport_data;
  ssize_t ret;

  /* See comment in pull_func. */
  g_clear_error (&gnutls->priv->error);

  if (gnutls->priv->blocking)
    {
      ret = g_output_stream_write (G_OUTPUT_STREAM (gnutls->priv->base_ostream),
				   buf, buflen,
				   gnutls->priv->cancellable,
				   &gnutls->priv->error);
    }
  else
    {
      ret = g_pollable_output_stream_write_nonblocking (gnutls->priv->base_ostream,
							buf, buflen,
							gnutls->priv->cancellable,
							&gnutls->priv->error);
    }
  if (ret < 0)
    set_gnutls_error (gnutls, G_IO_OUT);

  return ret;
}

static gboolean
handshake_internal (GTlsConnectionGnutls  *gnutls,
		    gboolean               blocking,
		    GCancellable          *cancellable,
		    GError               **error)
{
  GTlsCertificate *peer_certificate = NULL;
  GTlsCertificateFlags peer_certificate_errors = 0;
  int ret;

  if (G_IS_TLS_SERVER_CONNECTION_GNUTLS (gnutls) &&
      gnutls->priv->ever_handshaked && !gnutls->priv->handshaking &&
      !gnutls->priv->need_handshake)
    {
      BEGIN_GNUTLS_IO (gnutls, blocking, cancellable);
      ret = gnutls_rehandshake (gnutls->priv->session);
      END_GNUTLS_IO (gnutls, ret, _("Error performing TLS handshake: %s"), error);

      if (ret != 0)
	return FALSE;
    }

  if (!gnutls->priv->handshaking)
    {
      gnutls->priv->handshaking = TRUE;

      if (gnutls->priv->peer_certificate)
	{
	  g_object_unref (gnutls->priv->peer_certificate);
	  gnutls->priv->peer_certificate = NULL;
	  gnutls->priv->peer_certificate_errors = 0;

	  g_object_notify (G_OBJECT (gnutls), "peer-certificate");
	  g_object_notify (G_OBJECT (gnutls), "peer-certificate-errors");
	}

      g_tls_connection_gnutls_set_handshake_priority (gnutls);
      G_TLS_CONNECTION_GNUTLS_GET_CLASS (gnutls)->begin_handshake (gnutls);
    }

  BEGIN_GNUTLS_IO (gnutls, blocking, cancellable);
  ret = gnutls_handshake (gnutls->priv->session);
  END_GNUTLS_IO (gnutls, ret, _("Error performing TLS handshake: %s"), error);

  if (ret == GNUTLS_E_AGAIN)
    return FALSE;

  gnutls->priv->handshaking = FALSE;
  gnutls->priv->need_handshake = FALSE;
  gnutls->priv->ever_handshaked = TRUE;

  if (ret == 0 &&
      gnutls_certificate_type_get (gnutls->priv->session) == GNUTLS_CRT_X509)
    {
      GTlsCertificate *chain, *cert;
      const gnutls_datum_t *certs;
      unsigned int num_certs;
      int i;

      certs = gnutls_certificate_get_peers (gnutls->priv->session, &num_certs);
      chain = NULL;
      if (certs)
	{
	  for (i = num_certs - 1; i >= 0; i--)
	    {
	      cert = g_tls_certificate_gnutls_new (&certs[i], chain);
	      if (chain)
		g_object_unref (chain);
	      chain = cert;
	    }
	}

      peer_certificate = chain;
    }

  if (peer_certificate)
    {
      gboolean accepted;

      accepted = G_TLS_CONNECTION_GNUTLS_GET_CLASS (gnutls)->verify_peer (gnutls, peer_certificate, &peer_certificate_errors);

      gnutls->priv->peer_certificate = peer_certificate;
      gnutls->priv->peer_certificate_errors = peer_certificate_errors;

      g_object_notify (G_OBJECT (gnutls), "peer-certificate");
      g_object_notify (G_OBJECT (gnutls), "peer-certificate-errors");

      if (!accepted)
	{
	  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			       _("Unacceptable TLS certificate"));
	  return FALSE;
	}
    }

  G_TLS_CONNECTION_GNUTLS_GET_CLASS (gnutls)->finish_handshake (gnutls, ret == 0, error);
  return (ret == 0);
}

static gboolean
handshake_in_progress_or_failed (GTlsConnectionGnutls  *gnutls,
				 gboolean               blocking,
				 GCancellable          *cancellable,
				 GError               **error)
{
  if (!(gnutls->priv->need_handshake || gnutls->priv->handshaking))
    return FALSE;

  return !handshake_internal (gnutls, blocking, cancellable, error);
}

static gboolean
g_tls_connection_gnutls_handshake (GTlsConnection   *conn,
				   GCancellable     *cancellable,
				   GError          **error)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (conn);

  return handshake_internal (gnutls, TRUE, cancellable, error);
}

static gboolean
g_tls_connection_gnutls_handshake_ready (GObject  *pollable_stream,
					 gpointer  user_data)
{
  GTlsConnectionGnutls *gnutls;
  GSimpleAsyncResult *simple = user_data;
  gboolean success;
  GError *error = NULL;

  gnutls = G_TLS_CONNECTION_GNUTLS (g_async_result_get_source_object (G_ASYNC_RESULT (simple)));
  g_object_unref (gnutls);

  success = handshake_internal (gnutls, FALSE, NULL, &error);
  if (!success && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      g_error_free (error);
      return TRUE;
    }

  if (error)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }
  else
    g_simple_async_result_set_op_res_gboolean (simple, success);
  g_simple_async_result_complete (simple);
  g_object_unref (simple);

  return FALSE;
}

static void
g_tls_connection_gnutls_handshake_async (GTlsConnection       *conn,
					 int                   io_priority,
					 GCancellable         *cancellable,
					 GAsyncReadyCallback   callback,
					 gpointer              user_data)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (conn);
  GSimpleAsyncResult *simple;
  gboolean success;
  GError *error = NULL;
  GSource *source;

  simple = g_simple_async_result_new (G_OBJECT (conn), callback, user_data,
				      g_tls_connection_gnutls_handshake_async);
  success = handshake_internal (gnutls, FALSE, cancellable, &error);
  if (success)
    {
      g_simple_async_result_set_op_res_gboolean (simple, TRUE);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }
  else if (error)
    g_error_free (error);

  source = g_tls_connection_gnutls_create_source (gnutls, 0, cancellable);
  g_source_set_callback (source,
			 (GSourceFunc) g_tls_connection_gnutls_handshake_ready,
			 simple, NULL);
  g_source_set_priority (source, io_priority);
  g_source_attach (source, g_main_context_get_thread_default ());
  g_source_unref (source);
}

static gboolean
g_tls_connection_gnutls_handshake_finish (GTlsConnection       *conn,
					  GAsyncResult         *result,
					  GError              **error)
{
  GSimpleAsyncResult *simple;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (conn), g_tls_connection_gnutls_handshake_async), FALSE);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  return g_simple_async_result_get_op_res_gboolean (simple);
}

/* #if ENABLE(TIZEN_NPN) */
/* Following two functions MUST be declared.
 * Because glib2.0 already has following functions named set_next_protocols/get_next_protocol. */
/*
  available protocols as it is on Dec. 2012:
    http/1.1
    spdy/2
    spdy/3
*/
#define __PROTOCOL_NAME_MAXLEN    8       /* http/1.1 */

static void
g_tls_connection_gnutls_set_next_protocols (GTlsConnectionGnutls  *gnutls,
            const gchar           *protocols)
{
  g_return_if_fail (gnutls);
  g_return_if_fail (protocols);

#if ENABLE(TIZEN_NPN)
  gnutls_negotiate_next_protocol (gnutls->priv->session, protocols, strlen(protocols));
#endif
}

static const gchar *
g_tls_connection_gnutls_get_next_protocol (GTlsConnectionGnutls *gnutls)
{
#if ENABLE(TIZEN_NPN)
  int gnutls_ret;
  size_t size;
  gchar buf[__PROTOCOL_NAME_MAXLEN];
  gchar *ret_buf;
  unsigned int supported;

  g_return_val_if_fail (gnutls, NULL);

  size = __PROTOCOL_NAME_MAXLEN;
  gnutls_ret = gnutls_get_next_protocol (gnutls->priv->session, buf, &size, &supported);
  if (!gnutls_ret)
  {
    ret_buf = g_new0 (gchar, size + 1);
    memcpy (ret_buf, buf, size);

    return ret_buf;
  }
  else
  {
    return NULL;
  }
#else
  return NULL;
#endif
}
/* #endif */

gssize
g_tls_connection_gnutls_read (GTlsConnectionGnutls  *gnutls,
			      void                  *buffer,
			      gsize                  count,
			      gboolean               blocking,
			      GCancellable          *cancellable,
			      GError               **error)
{
  gssize ret;

 again:
  if (handshake_in_progress_or_failed (gnutls, blocking, cancellable, error))
    return -1;

  BEGIN_GNUTLS_IO (gnutls, blocking, cancellable);
  ret = gnutls_record_recv (gnutls->priv->session, buffer, count);
  END_GNUTLS_IO (gnutls, ret, _("Error reading data from TLS socket: %s"), error);

  if (ret >= 0)
    return ret;
  else if (ret == GNUTLS_E_REHANDSHAKE)
    goto again;
  else
    return -1;
}

gssize
g_tls_connection_gnutls_write (GTlsConnectionGnutls  *gnutls,
			       const void            *buffer,
			       gsize                  count,
			       gboolean               blocking,
			       GCancellable          *cancellable,
			       GError               **error)
{
  gssize ret;

 again:
  if (handshake_in_progress_or_failed (gnutls, blocking, cancellable, error))
    return -1;

  BEGIN_GNUTLS_IO (gnutls, blocking, cancellable);
  ret = gnutls_record_send (gnutls->priv->session, buffer, count);
  END_GNUTLS_IO (gnutls, ret, _("Error writing data to TLS socket: %s"), error);

  if (ret >= 0)
    return ret;
  else if (ret == GNUTLS_E_REHANDSHAKE)
    goto again;
  else
    return -1;
}

static GInputStream  *
g_tls_connection_gnutls_get_input_stream (GIOStream *stream)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (stream);

  return gnutls->priv->tls_istream;
}

static GOutputStream *
g_tls_connection_gnutls_get_output_stream (GIOStream *stream)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (stream);

  return gnutls->priv->tls_ostream;
}

static gboolean
close_internal (GTlsConnectionGnutls  *gnutls,
		gboolean               blocking,
		GCancellable          *cancellable,
		GError               **error)
{
  int ret;

  /* If we haven't finished the initial handshake yet, there's no
   * reason to finish it just so we can close.
   */
  if (!gnutls->priv->ever_handshaked)
    return TRUE;

  if (handshake_in_progress_or_failed (gnutls, blocking, cancellable, error))
    return FALSE;

  gnutls->priv->closing = TRUE;
  BEGIN_GNUTLS_IO (gnutls, blocking, cancellable);
  ret = gnutls_bye (gnutls->priv->session, GNUTLS_SHUT_WR);
  END_GNUTLS_IO (gnutls, ret, _("Error performing TLS close: %s"), error);
  if (ret == 0 || !error || !g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    gnutls->priv->closing = FALSE;

  return ret == 0;
}

static gboolean
g_tls_connection_gnutls_close (GIOStream     *stream,
			       GCancellable  *cancellable,
			       GError       **error)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (stream);

  if (!close_internal (gnutls, TRUE, cancellable, error))
    return FALSE;
  return g_io_stream_close (gnutls->priv->base_io_stream,
			    cancellable, error);
}

typedef struct {
  GSimpleAsyncResult *simple;
  GCancellable *cancellable;
  int io_priority;
} AsyncCloseData;

static void
close_base_stream_cb (GObject      *base_stream,
		      GAsyncResult *result,
		      gpointer      user_data)
{
  gboolean success;
  GError *error = NULL;
  AsyncCloseData *acd = user_data;

  success = g_io_stream_close_finish (G_IO_STREAM (base_stream),
				      result, &error);
  if (success)
    g_simple_async_result_set_op_res_gboolean (acd->simple, TRUE);
  else
    {
      g_simple_async_result_set_from_error (acd->simple, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (acd->simple);
  g_object_unref (acd->simple);
  if (acd->cancellable)
    g_object_unref (acd->cancellable);
  g_slice_free (AsyncCloseData, acd);
}

static gboolean
g_tls_connection_gnutls_close_ready (GObject  *pollable_stream,
				     gpointer  user_data)
{
  GTlsConnectionGnutls *gnutls;
  AsyncCloseData *acd = user_data;
  gboolean success;
  GError *error = NULL;

  gnutls = G_TLS_CONNECTION_GNUTLS (g_async_result_get_source_object (G_ASYNC_RESULT (acd->simple)));
  g_object_unref (gnutls);

  success = close_internal (gnutls, FALSE, NULL, &error);
  if (!success && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      g_error_free (error);
      return TRUE;
    }

  if (error)
    {
      g_simple_async_result_set_from_error (acd->simple, error);
      g_simple_async_result_complete (acd->simple);
      g_error_free (error);
      g_object_unref (acd->simple);
      if (acd->cancellable)
	g_object_unref (acd->cancellable);
      g_slice_free (AsyncCloseData, acd);
    }
  else
    {
      g_io_stream_close_async (gnutls->priv->base_io_stream,
			       acd->io_priority, acd->cancellable,
			       close_base_stream_cb, acd);
    }

  return FALSE;
}

static void
g_tls_connection_gnutls_close_async (GIOStream           *stream,
				     int                  io_priority,
				     GCancellable        *cancellable,
				     GAsyncReadyCallback  callback,
				     gpointer             user_data)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (stream);
  GSimpleAsyncResult *simple;
  gboolean success;
  GError *error = NULL;
  AsyncCloseData *acd;
  GSource *source;

  simple = g_simple_async_result_new (G_OBJECT (stream), callback, user_data,
				      g_tls_connection_gnutls_close_async);

  success = close_internal (gnutls, FALSE, cancellable, &error);
  if (error && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
    }

  if (error)
    g_error_free (error);

  acd = g_slice_new (AsyncCloseData);
  acd->simple = simple;
  acd->cancellable = cancellable ? g_object_ref (cancellable) : cancellable;
  acd->io_priority = io_priority;

  if (success)
    {
      g_io_stream_close_async (gnutls->priv->base_io_stream,
			       io_priority, cancellable,
			       close_base_stream_cb, acd);
      return;
    }

  source = g_tls_connection_gnutls_create_source (gnutls, 0, acd->cancellable);
  g_source_set_callback (source,
			 (GSourceFunc) g_tls_connection_gnutls_close_ready,
			 acd, NULL);
  g_source_set_priority (source, acd->io_priority);
  g_source_attach (source, g_main_context_get_thread_default ());
  g_source_unref (source);
}

static gboolean
g_tls_connection_gnutls_close_finish (GIOStream           *stream,
				      GAsyncResult        *result,
				      GError             **error)
{
  GSimpleAsyncResult *simple;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (stream), g_tls_connection_gnutls_close_async), FALSE);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  return g_simple_async_result_get_op_res_gboolean (simple);
}

#ifdef HAVE_PKCS11

static P11KitPin*
on_pin_prompt_callback (const char     *pinfile,
                        P11KitUri      *pin_uri,
                        const char     *pin_description,
                        P11KitPinFlags  pin_flags,
                        void           *callback_data)
{
  GTlsConnectionGnutls *gnutls = G_TLS_CONNECTION_GNUTLS (callback_data);
  GTlsInteractionResult result;
  GTlsPasswordFlags flags = 0;
  GTlsPassword *password;
  P11KitPin *pin = NULL;
  GError *error = NULL;

  if (!gnutls->priv->interaction)
    return NULL;

  if (pin_flags & P11_KIT_PIN_FLAGS_RETRY)
    flags |= G_TLS_PASSWORD_RETRY;
  if (pin_flags & P11_KIT_PIN_FLAGS_MANY_TRIES)
    flags |= G_TLS_PASSWORD_MANY_TRIES;
  if (pin_flags & P11_KIT_PIN_FLAGS_FINAL_TRY)
    flags |= G_TLS_PASSWORD_FINAL_TRY;

  password = g_pkcs11_pin_new (flags, pin_description);

  result = g_tls_interaction_ask_password (gnutls->priv->interaction, password,
                                           g_cancellable_get_current (), &error);

  switch (result)
    {
    case G_TLS_INTERACTION_FAILED:
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("couldn't ask for password: %s", error->message);
      pin = NULL;
      break;
    case G_TLS_INTERACTION_UNHANDLED:
      pin = NULL;
      break;
    case G_TLS_INTERACTION_HANDLED:
      pin = g_pkcs11_pin_steal_internal (G_PKCS11_PIN (password));
      break;
    }

  g_object_unref (password);
  return pin;
}

#endif /* HAVE_PKCS11 */

static void
g_tls_connection_gnutls_class_init (GTlsConnectionGnutlsClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GTlsConnectionClass *connection_class = G_TLS_CONNECTION_CLASS (klass);
  GIOStreamClass *iostream_class = G_IO_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsConnectionGnutlsPrivate));

  gobject_class->get_property = g_tls_connection_gnutls_get_property;
  gobject_class->set_property = g_tls_connection_gnutls_set_property;
  gobject_class->finalize     = g_tls_connection_gnutls_finalize;

  connection_class->handshake        = g_tls_connection_gnutls_handshake;
  connection_class->handshake_async  = g_tls_connection_gnutls_handshake_async;
  connection_class->handshake_finish = g_tls_connection_gnutls_handshake_finish;

  /* #if ENABLE(TIZEN_NPN) */
  /* This code MUST be set because glib2.0 already has following functions named set_next_protocols/get_next_protocol. */
  connection_class->set_next_protocols  = g_tls_connection_gnutls_set_next_protocols;
  connection_class->get_next_protocol   = g_tls_connection_gnutls_get_next_protocol;
  /* #endif */

  iostream_class->get_input_stream  = g_tls_connection_gnutls_get_input_stream;
  iostream_class->get_output_stream = g_tls_connection_gnutls_get_output_stream;
  iostream_class->close_fn          = g_tls_connection_gnutls_close;
  iostream_class->close_async       = g_tls_connection_gnutls_close_async;
  iostream_class->close_finish      = g_tls_connection_gnutls_close_finish;

  g_object_class_override_property (gobject_class, PROP_BASE_IO_STREAM, "base-io-stream");
  g_object_class_override_property (gobject_class, PROP_REQUIRE_CLOSE_NOTIFY, "require-close-notify");
  g_object_class_override_property (gobject_class, PROP_REHANDSHAKE_MODE, "rehandshake-mode");
  g_object_class_override_property (gobject_class, PROP_USE_SYSTEM_CERTDB, "use-system-certdb");
  g_object_class_override_property (gobject_class, PROP_DATABASE, "database");
  g_object_class_override_property (gobject_class, PROP_CERTIFICATE, "certificate");
  g_object_class_override_property (gobject_class, PROP_INTERACTION, "interaction");
  g_object_class_override_property (gobject_class, PROP_PEER_CERTIFICATE, "peer-certificate");
  g_object_class_override_property (gobject_class, PROP_PEER_CERTIFICATE_ERRORS, "peer-certificate-errors");
}

static void
g_tls_connection_gnutls_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_tls_connection_gnutls_initable_init;
}
