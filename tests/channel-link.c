/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <spice-client.h>

#include "spice-channel-priv.h"
#include "spice-session-priv.h"

typedef struct {
    GMainLoop *loop;
    GSocketConnection *connection;
    guint8 bytes[50];
    gsize size;
} LinkTest;

static void link_read(GObject *source, GAsyncResult *result, gpointer data)
{
    LinkTest *test = data;
    GError *error = NULL;
    gsize size;

    g_assert_true(g_input_stream_read_all_finish(G_INPUT_STREAM(source), result,
                                                &size, &error));
    g_assert_no_error(error);
    g_assert_cmpuint(size, ==, test->size);
    g_main_loop_quit(test->loop);
}

static gboolean link_incoming(GSocketService *service, GSocketConnection *connection,
                              GObject *source, gpointer data)
{
    LinkTest *test = data;

    test->connection = g_object_ref(connection);
    g_input_stream_read_all_async(g_io_stream_get_input_stream(G_IO_STREAM(connection)),
                                 test->bytes, test->size, G_PRIORITY_DEFAULT, NULL,
                                 link_read, test);
    return TRUE;
}

static gboolean link_timeout(gpointer data)
{
    g_error("Timed out waiting for the link message");
    return G_SOURCE_REMOVE;
}

static void test_channel_link(gconstpointer data)
{
    const guint mask = GPOINTER_TO_UINT(data);
    const guint32 common_caps[] = { 0x01234567, 0x89abcdef };
    const guint32 channel_caps[] = { 0xfedcba98, 0x76543210 };
    const guint8 common_bytes[] = { 0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x89 };
    const guint8 channel_bytes[] = { 0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76 };
    guint8 expected[50] = {
        'R', 'E', 'D', 'Q', 2, 0, 0, 0, 2, 0, 0, 0, 18, 0, 0, 0,
        0x78, 0x56, 0x34, 0x12, 3, 7,
        0, 0, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0,
    };
    LinkTest test = { .size = 34 };
    g_autoptr(SpiceSession) session = spice_session_new();
    g_autoptr(GSocketService) service = g_socket_service_new();
    g_autoptr(GSocketAddress) address = g_inet_socket_address_new_from_string("127.0.0.1", 0);
    g_autoptr(GSocketAddress) effective = NULL;
    g_autofree gchar *port = NULL;
    GError *error = NULL;
    SpiceChannel *channel;
    guint timeout;

    g_assert_true(g_socket_listener_add_address(G_SOCKET_LISTENER(service), address,
                                               G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP,
                                               NULL, &effective, &error));
    g_assert_no_error(error);
    port = g_strdup_printf("%u", g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(effective)));
    g_object_set(session, "host", "127.0.0.1", "port", port, NULL);
    spice_session_set_connection_id(session, 0x12345678);
    channel = spice_channel_new(session, SPICE_CHANNEL_INPUTS, 7);

    g_array_set_size(channel->priv->common_caps, 0);
    g_array_set_size(channel->priv->caps, 0);
    if (mask & 1) {
        g_array_append_vals(channel->priv->common_caps, common_caps, G_N_ELEMENTS(common_caps));
        expected[22] = 2;
        memcpy(expected + test.size, common_bytes, sizeof(common_bytes));
        test.size += sizeof(common_bytes);
    }
    if (mask & 2) {
        g_array_append_vals(channel->priv->caps, channel_caps, G_N_ELEMENTS(channel_caps));
        expected[26] = 2;
        memcpy(expected + test.size, channel_bytes, sizeof(channel_bytes));
        test.size += sizeof(channel_bytes);
    }
    expected[12] = test.size - 16;

    /* The packed link message puts both capability arrays at offset 34,
     * two bytes past a uint32_t boundary. Exercise the actual socket writer. */
    g_assert_cmpuint(sizeof(SpiceLinkHeader) + sizeof(SpiceLinkMess), ==, 34);
    test.loop = g_main_loop_new(NULL, FALSE);
    g_signal_connect(service, "incoming", G_CALLBACK(link_incoming), &test);
    timeout = g_timeout_add_seconds(10, link_timeout, NULL);
    g_assert_true(spice_channel_connect(channel));
    g_main_loop_run(test.loop);
    g_source_remove(timeout);
    g_assert_cmpmem(test.bytes, test.size, expected, test.size);

    /* This peer only captures the request; it does not send a link reply. */
    g_test_expect_message("GSpice", G_LOG_LEVEL_WARNING, "incomplete link header (0/16)");
    spice_session_disconnect(session);
    while (g_main_context_iteration(NULL, FALSE))
        ;
    g_test_assert_expected_messages();
    g_socket_service_stop(service);
    g_socket_listener_close(G_SOCKET_LISTENER(service));
    g_clear_object(&test.connection);
    g_main_loop_unref(test.loop);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_data_func("/channel/link/no-caps", GUINT_TO_POINTER(0), test_channel_link);
    g_test_add_data_func("/channel/link/common-caps", GUINT_TO_POINTER(1), test_channel_link);
    g_test_add_data_func("/channel/link/channel-caps", GUINT_TO_POINTER(2), test_channel_link);
    g_test_add_data_func("/channel/link/both-caps", GUINT_TO_POINTER(3), test_channel_link);
    return g_test_run();
}
