/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://www.whatwg.org/specs/web-apps/current-work/#the-a-element
 * http://www.whatwg.org/specs/web-apps/current-work/#other-elements,-attributes-and-apis
 * © Copyright 2004-2011 Apple Computer, Inc., Mozilla Foundation, and
 * Opera Software ASA. You are granted a license to use, reproduce
 * and create derivative works of this document.
 */

// http://www.whatwg.org/specs/web-apps/current-work/#the-a-element
interface HTMLAnchorElement : HTMLElement {
           [SetterThrows]
           attribute DOMString target;
           [SetterThrows]
           attribute DOMString download;
           [SetterThrows]
           attribute DOMString ping;
           [SetterThrows]
           attribute DOMString rel;
           [SetterThrows, Pref="network.http.enablePerElementReferrer"]
           attribute DOMString referrer;
  readonly attribute DOMTokenList relList;
           [SetterThrows]
           attribute DOMString hreflang;
           [SetterThrows]
           attribute DOMString type;

           [SetterThrows]
           attribute DOMString text;
};

HTMLAnchorElement implements HTMLHyperlinkElementUtils;

// http://www.whatwg.org/specs/web-apps/current-work/#other-elements,-attributes-and-apis
partial interface HTMLAnchorElement {
           [SetterThrows]
           attribute DOMString coords;
           [SetterThrows]
           attribute DOMString charset;
           [SetterThrows]
           attribute DOMString name;
           [SetterThrows]
           attribute DOMString rev;
           [SetterThrows]
           attribute DOMString shape;
};
