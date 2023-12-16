/*
   Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_hit_test.h"

#include "ewk_hit_test_private.h"
#include <wtf/text/CString.h>
#include <cairo.h>

using namespace WebKit;
using namespace WebCore;

/** Structure used to report hit test result */
struct _Ewk_Hit_Test {
    Ewk_Hit_Test_Result_Context context;

    CString linkURI;
    CString linkTitle; /**< the title of link */
    CString linkLabel; /**< the text of the link */
    CString imageURI;
    CString mediaURI;

    struct {
        CString tagName; /**<tag name for hit element */
        CString nodeValue; /**<node value for hit element */
        Eina_Hash* attributeHash; /**<attribute data for hit element */
    } nodeData;

    struct {
        void* buffer; /**<image buffer for hit element */
        unsigned int length; /**<image buffer length for hit element */
        CString fileNameExtension; /**<image filename extension for hit element */
    } imageData;
};

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
static void freeHitTestAttributeHashData(void* data)
{
    EINA_SAFETY_ON_NULL_RETURN(data);
    eina_stringshare_del(static_cast<Eina_Stringshare*>(data));
}

Ewk_Hit_Test* ewkHitTestCreate(WebHitTestResult::Data& hitTestResultData)
{
    Ewk_Hit_Test* hitTest = new Ewk_Hit_Test;

    hitTest->linkURI = hitTestResultData.absoluteLinkURL.utf8();
    hitTest->linkTitle = hitTestResultData.linkTitle.utf8();
    hitTest->linkLabel = hitTestResultData.linkLabel.utf8();
    hitTest->imageURI = hitTestResultData.absoluteImageURL.utf8();
    hitTest->mediaURI = hitTestResultData.absoluteMediaURL.utf8();

    int context = hitTestResultData.context;
    hitTest->context = static_cast<Ewk_Hit_Test_Result_Context>(context);


    hitTest->nodeData.attributeHash = 0;
    if (hitTestResultData.hitTestMode & EWK_HIT_TEST_MODE_NODE_DATA) {
        hitTest->nodeData.tagName = hitTestResultData.nodeData.tagName.utf8();
        hitTest->nodeData.nodeValue = hitTestResultData.nodeData.nodeValue.utf8();

        if (!hitTestResultData.nodeData.attributeMap.isEmpty()) {
            hitTest->nodeData.attributeHash = eina_hash_string_superfast_new(freeHitTestAttributeHashData);

            WebHitTestResult::Data::NodeData::AttributeMap::iterator attributeMapEnd = hitTestResultData.nodeData.attributeMap.end();
            for (WebHitTestResult::Data::NodeData::AttributeMap::iterator it = hitTestResultData.nodeData.attributeMap.begin(); it != attributeMapEnd; ++it) {
                eina_hash_add(hitTest->nodeData.attributeHash, it->key.utf8().data(), eina_stringshare_add(it->value.utf8().data()));
            }
        }
    }

    hitTest->imageData.buffer = 0;
    hitTest->imageData.length = 0;
    if ((hitTestResultData.hitTestMode & EWK_HIT_TEST_MODE_IMAGE_DATA) && (context & EWK_HIT_TEST_RESULT_CONTEXT_IMAGE)) {
        if (!hitTestResultData.imageData.data.isEmpty()) {
            hitTest->imageData.length = hitTestResultData.imageData.data.size();
            hitTest->imageData.fileNameExtension = hitTestResultData.imageData.fileNameExtension.utf8();

            if (hitTest->imageData.length > 0) {
                hitTest->imageData.buffer = calloc(1, hitTest->imageData.length);
                if (hitTest->imageData.buffer)
                    memcpy(hitTest->imageData.buffer, hitTestResultData.imageData.data.data(), hitTest->imageData.length);
            }
        }
    }

    return hitTest;
}
#endif

void ewk_hit_test_free(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN(hitTest);

    if (hitTest->nodeData.attributeHash)
        eina_hash_free(hitTest->nodeData.attributeHash);

    if (hitTest->imageData.buffer) {
        free(hitTest->imageData.buffer);
        hitTest->imageData.buffer = 0;
        hitTest->imageData.length = 0;
    }

    delete hitTest;
#endif
}

Ewk_Hit_Test_Result_Context ewk_hit_test_result_context_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT);

    return hitTest->context;
#else
    return EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT;
#endif
}

const char* ewk_hit_test_link_uri_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->linkURI.data();
#else
    return 0;
#endif
}

const char* ewk_hit_test_link_title_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->linkTitle.data();
#else
    return 0;
#endif
}

const char* ewk_hit_test_link_label_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->linkLabel.data();
#else
    return 0;
#endif
}

const char* ewk_hit_test_image_uri_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->imageURI.data();
#else
    return 0;
#endif
}

const char* ewk_hit_test_media_uri_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->mediaURI.data();
#else
    return 0;
#endif
}

const char* ewk_hit_test_tag_name_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->nodeData.tagName.data();
#else
    return 0;
#endif
}

const char* ewk_hit_test_node_value_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->nodeData.nodeValue.data();
#else
    return 0;
#endif
}

Eina_Hash* ewk_hit_test_attribute_hash_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->nodeData.attributeHash;
#else
    return 0;
#endif
}

void* ewk_hit_test_image_buffer_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->imageData.buffer;
#else
    return 0;
#endif
}

unsigned int ewk_hit_test_image_buffer_length_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->imageData.length;
#else
    return 0;
#endif
}

const char* ewk_hit_test_image_file_name_extension_get(Ewk_Hit_Test* hitTest)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(hitTest, 0);

    return hitTest->imageData.fileNameExtension.data();
#else
    return 0;
#endif
}

