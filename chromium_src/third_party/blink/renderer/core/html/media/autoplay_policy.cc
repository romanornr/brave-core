/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"

namespace {

bool IsAutoplayAllowedForFrame(blink::LocalFrame* frame) {
  if (!frame)
    return false;
  if (auto* settings_client = frame->GetContentSettingsClient())
    return settings_client->AllowAutoplay(true /* default_value */);
  return true;
}

bool IsAutoplayAllowedForElement(
    blink::Member<blink::HTMLMediaElement> element) {
  return IsAutoplayAllowedForFrame(element->GetDocument().GetFrame());
}

}  // namespace

#define BRAVE_AUTOPLAY_POLICY_GET_FOR_DOCUMENT        \
  if (IsAutoplayAllowedForFrame(document.GetFrame())) \
    return Type::kNoUserGestureRequired;              \
  else                                                \
    return Type::kUserGestureRequired;

#define BRAVE_AUTOPLAY_POLICY_REQUEST_AUTOPLAY_BY_ATTRIBUTE \
  if (!IsAutoplayAllowedForElement(element_))               \
    return false;

#define BRAVE_AUTOPLAY_POLICY_REQUEST_PLAY    \
  if (!IsAutoplayAllowedForElement(element_)) \
    return DOMExceptionCode::kNotAllowedError;

#include "../../../../../third_party/blink/renderer/core/html/media/autoplay_policy.cc"  // NOLINT

#undef BRAVE_AUTOPLAY_POLICY_GET_FOR_DOCUMENT
#undef BRAVE_AUTOPLAY_POLICY_REQUEST_AUTOPLAY_BY_ATTRIBUTE
#undef BRAVE_AUTOPLAY_POLICY_REQUEST_PLAY
