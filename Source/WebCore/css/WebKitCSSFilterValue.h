/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebKitCSSFilterValue_h
#define WebKitCSSFilterValue_h

#include "CSSValueList.h"

namespace WebCore {

class WebKitCSSFilterValue : public CSSValueList {
public:
    // NOTE: these have to match the values in the IDL
    enum FilterOperationType {
        UnknownFilterOperation,
        ReferenceFilterOperation,
        GrayscaleFilterOperation,
        SepiaFilterOperation,
        SaturateFilterOperation,
        HueRotateFilterOperation,
        InvertFilterOperation,
        OpacityFilterOperation,
        BrightnessFilterOperation,
        ContrastFilterOperation,
        BlurFilterOperation,
        DropShadowFilterOperation
    };

    static Ref<WebKitCSSFilterValue> create(FilterOperationType type)
    {
        return adoptRef(*new WebKitCSSFilterValue(type));
    }

    String customCSSText() const;

    FilterOperationType operationType() const { return m_type; }

    Ref<WebKitCSSFilterValue> cloneForCSSOM() const;

    bool equals(const WebKitCSSFilterValue&) const;

private:
    WebKitCSSFilterValue(FilterOperationType);
    WebKitCSSFilterValue(const WebKitCSSFilterValue& cloneFrom);

    FilterOperationType m_type;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_VALUE(WebKitCSSFilterValue, isWebKitCSSFilterValue())

#endif // WebKitCSSFilterValue_h
