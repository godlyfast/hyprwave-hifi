#include "art.h"
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libsoup/soup.h>
#include <string.h>

static GdkTexture* create_texture_from_pixbuf(GdkPixbuf *pixbuf) {
    if (!pixbuf) return NULL;

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    if (width <= 0 || height <= 0 || rowstride <= 0 || (channels != 3 && channels != 4)) {
        return NULL;
    }

    GdkMemoryFormat format = channels == 4 ? GDK_MEMORY_R8G8B8A8 : GDK_MEMORY_R8G8B8;
    gsize packed_rowstride = (gsize)width * (gsize)channels;
    gsize packed_size = packed_rowstride * (gsize)height;
    const guchar *src_pixels = gdk_pixbuf_get_pixels(pixbuf);
    guchar *packed_pixels = g_malloc_n((gsize)height, packed_rowstride);

    for (int y = 0; y < height; y++) {
        memcpy(packed_pixels + (gsize)y * packed_rowstride,
               src_pixels + (gsize)y * (gsize)rowstride,
               packed_rowstride);
    }

    GBytes *bytes = g_bytes_new_take(packed_pixels, packed_size);
    GdkTexture *texture = gdk_memory_texture_new(width, height, format, bytes, packed_rowstride);

    g_bytes_unref(bytes);
    return texture;
}

void clear_album_art_container(GtkWidget *container) {
    GtkWidget *child = gtk_widget_get_first_child(container);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_widget_unparent(child);
        child = next;
    }
}

GtkWidget* load_album_art_to_container(const gchar *art_url, GtkWidget *container, gint size) {
    if (!art_url || strlen(art_url) == 0 || !container) return NULL;

    GdkPixbuf *pixbuf = NULL;

    if (g_str_has_prefix(art_url, "file://")) {
        gchar *file_path = g_filename_from_uri(art_url, NULL, NULL);
        if (file_path && g_file_test(file_path, G_FILE_TEST_EXISTS)) {
            GError *error = NULL;
            pixbuf = gdk_pixbuf_new_from_file_at_scale(file_path, size, size, FALSE, &error);
            if (error) {
                g_error_free(error);
                pixbuf = NULL;
            }
        }
        g_free(file_path);
    } else if (g_str_has_prefix(art_url, "http://") || g_str_has_prefix(art_url, "https://")) {
        SoupSession *session = soup_session_new();
        SoupMessage *message = soup_message_new("GET", art_url);
        GError *error = NULL;

        if (message) {
            GInputStream *stream = soup_session_send(session, message, NULL, &error);
            SoupStatus status = soup_message_get_status(message);
            if (stream && SOUP_STATUS_IS_SUCCESSFUL(status) && !error) {
                pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, size, size, FALSE, NULL, &error);
            }
            if (stream) {
                g_object_unref(stream);
            }
            if (error) {
                g_error_free(error);
                pixbuf = NULL;
            }
            g_object_unref(message);
        }
        g_object_unref(session);
    }

    GdkTexture *texture = create_texture_from_pixbuf(pixbuf);
    if (pixbuf) {
        g_object_unref(pixbuf);
    }
    if (!texture) return NULL;

    GtkWidget *image = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));
    gtk_widget_set_size_request(image, size, size);

    // For larger sizes (main widget), add extra layout controls
    if (size > 100) {
        gtk_picture_set_can_shrink(GTK_PICTURE(image), TRUE);
        gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_CONTAIN);
        gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
        gtk_widget_set_hexpand(image, FALSE);
        gtk_widget_set_vexpand(image, FALSE);
    } else {
        // For notifications, use simpler fill approach
        gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_COVER);
    }

    // Clear existing art and add new
    clear_album_art_container(container);
    gtk_box_append(GTK_BOX(container), image);

    g_object_unref(texture);

    return image;
}
