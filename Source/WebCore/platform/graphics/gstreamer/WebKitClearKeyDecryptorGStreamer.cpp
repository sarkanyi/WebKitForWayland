/* GStreamer ClearKey common encryption decryptor
 *
 * Copyright (C) 2016 Igalia S.L
 * Copyright (C) 2016 Metrological
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "config.h"
#include "WebKitClearKeyDecryptorGStreamer.h"

#if (ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)) && USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include <gcrypt.h>
#include <gst/base/gstbytereader.h>
#include <wtf/RunLoop.h>

#define CLEARKEY_SIZE 16

#define WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_CK_DECRYPT, WebKitMediaClearKeyDecryptPrivate))
struct _WebKitMediaClearKeyDecryptPrivate {
    GRefPtr<GstBuffer> key;
    gcry_cipher_hd_t handle;
};

static void webKitMediaClearKeyDecryptorFinalize(GObject*);
static void webKitMediaClearKeyDecryptorRequestDecryptionKey(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer*);
static gboolean webKitMediaClearKeyDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent*);
static gboolean webKitMediaClearKeyDecryptorSetupCipher(WebKitMediaCommonEncryptionDecrypt*);
static gboolean webKitMediaClearKeyDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer* iv, GstBuffer* sample, unsigned subSamplesCount, GstBuffer* subSamples);
static void webKitMediaClearKeyDecryptorReleaseCipher(WebKitMediaCommonEncryptionDecrypt*);

GST_DEBUG_CATEGORY_STATIC(webkit_media_clear_key_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_clear_key_decrypt_debug_category

#define CLEAR_KEY_PROTECTION_SYSTEM_ID "58147ec8-0423-4659-92e6-f52c5ce8c3cc"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" CLEAR_KEY_PROTECTION_SYSTEM_ID "; "
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" CLEAR_KEY_PROTECTION_SYSTEM_ID));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-h264; audio/mpeg"));

#define webkit_media_clear_key_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaClearKeyDecrypt, webkit_media_clear_key_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void webkit_media_clear_key_decrypt_class_init(WebKitMediaClearKeyDecryptClass* klass)
{
    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->finalize = webKitMediaClearKeyDecryptorFinalize;

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using ISOBMFF ClearKey Common Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using ISOBMFF ClearKey Common Encryption.",
        "Philippe Normand <philn@igalia.com>");

    GST_DEBUG_CATEGORY_INIT(webkit_media_clear_key_decrypt_debug_category,
        "webkitclearkey", 0, "ClearKey decryptor");

    cencClass->protectionSystemId = CLEAR_KEY_PROTECTION_SYSTEM_ID;
    cencClass->requestDecryptionKey = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorRequestDecryptionKey);
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorHandleKeyResponse);
    cencClass->setupCipher = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorSetupCipher);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorDecrypt);
    cencClass->releaseCipher = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorReleaseCipher);

    g_type_class_add_private(klass, sizeof(WebKitMediaClearKeyDecryptPrivate));
}

static void webkit_media_clear_key_decrypt_init(WebKitMediaClearKeyDecrypt* self)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(self);

    if (!gcry_check_version(GCRYPT_VERSION))
        GST_ERROR_OBJECT(self, "Libgcrypt failed to initialize");

    // Allocate a pool of 16k secure memory. This make the secure memory
    // available and also drops privileges where needed.
    gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);

    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    self->priv = priv;
    new (priv) WebKitMediaClearKeyDecryptPrivate();
}

static void webKitMediaClearKeyDecryptorFinalize(GObject* object)
{
    WebKitMediaClearKeyDecrypt* self = WEBKIT_MEDIA_CK_DECRYPT(object);
    WebKitMediaClearKeyDecryptPrivate* priv = self->priv;

    priv->~WebKitMediaClearKeyDecryptPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void webKitMediaClearKeyDecryptorRequestDecryptionKey(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* initDataBuffer)
{
    gst_element_post_message(GST_ELEMENT(self),
        gst_message_new_element(GST_OBJECT(self),
            gst_structure_new("drm-key-needed", "data", GST_TYPE_BUFFER, initDataBuffer,
                "key-system-id", G_TYPE_STRING, "org.w3.clearkey", nullptr)));
}

static gboolean webKitMediaClearKeyDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    const GstStructure* structure = gst_event_get_structure(event);

    if (!gst_structure_has_name(structure, "drm-cipher"))
        return FALSE;

    const GValue* value = gst_structure_get_value(structure, "key");
    priv->key.clear();
    priv->key = adoptGRef(gst_buffer_copy(gst_value_get_buffer(value)));
    return TRUE;
}

static gboolean webKitMediaClearKeyDecryptorSetupCipher(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    gcry_error_t error;

    ASSERT(priv->key);
    if (!priv->key) {
        GST_ERROR_OBJECT(self, "Decryption key not provided");
        return false;
    }

    error = gcry_cipher_open(&(priv->handle), GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CTR, GCRY_CIPHER_SECURE);
    if (error) {
        GST_ERROR_OBJECT(self, "Failed to create AES 128 CTR cipher handle: %s", gpg_strerror(error));
        return false;
    }

    GstMapInfo keyMap;
    if (!gst_buffer_map(priv->key.get(), &keyMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map decryption key");
        return false;
    }

    ASSERT(keyMap.size == CLEARKEY_SIZE);
    error = gcry_cipher_setkey(priv->handle, keyMap.data, keyMap.size);
    gst_buffer_unmap(priv->key.get(), &keyMap);
    if (error) {
        GST_ERROR_OBJECT(self, "gcry_cipher_setkey failed: %s", gpg_strerror(error));
        return false;
    }

    return true;
}

static gboolean webKitMediaClearKeyDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    GstMapInfo map, ivMap, subSamplesMap;
    unsigned position = 0;
    unsigned sampleIndex = 0;
    uint8_t ctr[CLEARKEY_SIZE];
    GstByteReader* reader = nullptr;
    gboolean bufferMapped, subsamplesBufferMapped;
    gcry_error_t error;

    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    if (ivMap.size == 8) {
        memset(ctr + 8, 0, 8);
        memcpy(ctr, ivMap.data, 8);
    } else {
        ASSERT(ivMap.size == CLEARKEY_SIZE);
        memcpy(ctr, ivMap.data, CLEARKEY_SIZE);
    }
    gst_buffer_unmap(ivBuffer, &ivMap);
    error = gcry_cipher_setctr(priv->handle, ctr, CLEARKEY_SIZE);
    if (error) {
        GST_ERROR_OBJECT(self, "gcry_cipher_setctr failed: %s", gpg_strerror(error));
        return false;
    }

    bufferMapped = gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE));
    if (!bufferMapped) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    subsamplesBufferMapped = gst_buffer_map(subSamplesBuffer, &subSamplesMap, GST_MAP_READ);
    if (!subsamplesBufferMapped) {
        GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
        gst_buffer_unmap(buffer, &map);
        return false;
    }

    reader = gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size);

    GST_DEBUG_OBJECT(self, "position: %d, size: %d", position, map.size);
    while (position < map.size) {
        guint16 nBytesClear = 0;
        guint32 nBytesEncrypted = 0;

        if (sampleIndex < subSampleCount) {
            if (!gst_byte_reader_get_uint16_be(reader, &nBytesClear)
                || !gst_byte_reader_get_uint32_be(reader, &nBytesEncrypted)) {
                GST_DEBUG_OBJECT(self, "unsupported");
                gst_byte_reader_free(reader);
                gst_buffer_unmap(buffer, &map);
                gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
                return false;
            }

            sampleIndex++;
        } else {
            nBytesClear = 0;
            nBytesEncrypted = map.size - position;
        }

        GST_TRACE_OBJECT(self, "%d bytes clear (todo=%d)", nBytesClear, map.size - position);
        position += nBytesClear;
        if (nBytesEncrypted) {
            GST_TRACE_OBJECT(self, "%d bytes encrypted (todo=%d)", nBytesEncrypted, map.size - position);
            error = gcry_cipher_decrypt(priv->handle, map.data + position, nBytesEncrypted, 0, 0);
            if (error) {
                GST_ERROR_OBJECT(self, "decryption failed: %s", gpg_strerror(error));
                gst_byte_reader_free(reader);
                gst_buffer_unmap(buffer, &map);
                gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
                return false;
            }
            position += nBytesEncrypted;
        }
    }

    gst_byte_reader_free(reader);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
    return true;
}

static void webKitMediaClearKeyDecryptorReleaseCipher(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    gcry_cipher_close(priv->handle);
}

#endif // (ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)) && USE(GSTREAMER)
