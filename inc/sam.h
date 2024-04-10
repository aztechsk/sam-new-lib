/*
 * sam.h
 *
 * Copyright (c) 2020 Jan Rusnak <jan@rusnak.sk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SAM_H
#define SAM_H

// SAMD21.
#if defined(__SAMD21E15A__) || defined(__ATSAMD21E15A__)
 #include "samd21/include/samd21e15a.h"
#elif defined(__SAMD21E16A__) || defined(__ATSAMD21E16A__)
 #include "samd21/include/samd21e16a.h"
#elif defined(__SAMD21E17A__) || defined(__ATSAMD21E17A__)
 #include "samd21/include/samd21e17a.h"
#elif defined(__SAMD21E18A__) || defined(__ATSAMD21E18A__)
 #include "samd21/include/samd21e18a.h"
#elif defined(__SAMD21G15A__) || defined(__ATSAMD21G15A__)
 #include "samd21/include/samd21g15a.h"
#elif defined(__SAMD21G16A__) || defined(__ATSAMD21G16A__)
 #include "samd21/include/samd21g16a.h"
#elif defined(__SAMD21G17A__) || defined(__ATSAMD21G17A__)
 #include "samd21/include/samd21g17a.h"
#elif defined(__SAMD21G17AU__) || defined(__ATSAMD21G17AU__)
 #include "samd21/include/samd21g17au.h"
#elif defined(__SAMD21G18A__) || defined(__ATSAMD21G18A__)
 #include "samd21/include/samd21g18a.h"
#elif defined (__SAMD21G18AU__) || defined(__ATSAMD21G18AU__)
 #include "samd21/include/samd21g18au.h"
#elif defined(__SAMD21J15A__) || defined(__ATSAMD21J15A__)
 #include "samd21/include/samd21j15a.h"
#elif defined(__SAMD21J16A__) || defined(__ATSAMD21J16A__)
 #include "samd21/include/samd21j16a.h"
#elif defined(__SAMD21J17A__) || defined(__ATSAMD21J17A__)
 #include "samd21/include/samd21j17a.h"
#elif defined(__SAMD21J18A__) || defined(__ATSAMD21J18A__)
 #include "samd21/include/samd21j18a.h"
#elif defined(__SAMD21E15B__) || defined(__ATSAMD21E15B__)
 #include "samd21/include/samd21e15b.h"
#elif defined(__SAMD21E15BU__) || defined(__ATSAMD21E15BU__)
 #include "samd21/include/samd21e15bu.h"
#elif defined(__SAMD21E15L__) || defined(__ATSAMD21E15L__)
 #include "samd21/include/samd21e15l.h"
#elif defined(__SAMD21E16B__) || defined(__ATSAMD21E16B__)
 #include "samd21/include/samd21e16b.h"
#elif defined(__SAMD21E16BU__) || defined(__ATSAMD21E16BU__)
 #include "samd21/include/samd21e16bu.h"
#elif defined(__SAMD21E16L__) || defined(__ATSAMD21E16L__)
 #include "samd21/include/samd21e16l.h"
#elif defined(__SAMD21G15B__) || defined(__ATSAMD21G15B__)
 #include "samd21/include/samd21g15b.h"
#elif defined(__SAMD21G15L__) || defined(__ATSAMD21G15L__)
 #include "samd21/include/samd21g15l.h"
#elif defined(__SAMD21G16B__) || defined(__ATSAMD21G16B__)
 #include "samd21/include/samd21g16b.h"
#elif defined(__SAMD21G16L__) || defined(__ATSAMD21G16L__)
 #include "samd21/include/samd21g16l.h"
#elif defined(__SAMD21J15B__) || defined(__ATSAMD21J15B__)
 #include "samd21/include/samd21j15b.h"
#elif defined(__SAMD21J16B__) || defined(__ATSAMD21J16B__)
 #include "samd21/include/samd21j16b.h"
#elif defined(__SAMD21E17D__) || defined(__ATSAMD21E17D__)
 #include "samd21/include/samd21e17d.h"
#elif defined(__SAMD21E17DU__) || defined(__ATSAMD21E17DU__)
 #include "samd21/include/samd21e17du.h"
#elif defined(__SAMD21E17L__) || defined(__ATSAMD21E17L__)
 #include "samd21/include/samd21e17l.h"
#elif defined(__SAMD21G17D__) || defined(__ATSAMD21G17D__)
 #include "samd21/include/samd21g17d.h"
#elif defined(__SAMD21G17L__) || defined(__ATSAMD21G17L__)
 #include "samd21/include/samd21g17l.h"
#elif defined(__SAMD21J17D__) || defined(__ATSAMD21J17D__)
 #include "samd21/include/samd21j17d.h"
// SAML21 A.
#elif defined(__SAML21E18A__) || defined(__ATSAML21E18A__)
 #include "saml21/include/saml21e18a.h"
#elif defined(__SAML21G18A__) || defined(__ATSAML21G18A__)
 #include "saml21/include/saml21g18a.h"
#elif defined(__SAML21J18A__) || defined(__ATSAML21J18A__)
 #include "saml21/include/saml21j18a.h"
// SAML21 B.
#elif defined(__SAML21E15B__) || defined(__ATSAML21E15B__)
 #include "saml21/include_b/saml21e15b.h"
#elif defined(__SAML21E16B__) || defined(__ATSAML21E16B__)
 #include "saml21/include_b/saml21e16b.h"
#elif defined(__SAML21E17B__) || defined(__ATSAML21E17B__)
 #include "saml21/include_b/saml21e17b.h"
#elif defined(__SAML21E18B__) || defined(__ATSAML21E18B__)
 #include "saml21/include_b/saml21e18b.h"
#elif defined(__SAML21G16B__) || defined(__ATSAML21G16B__)
 #include "saml21/include_b/saml21g16b.h"
#elif defined(__SAML21G17B__) || defined(__ATSAML21G17B__)
 #include "saml21/include_b/saml21g17b.h"
#elif defined(__SAML21G18B__) || defined(__ATSAML21G18B__)
 #include "saml21/include_b/saml21g18b.h"
#elif defined(__SAML21J16B__) || defined(__ATSAML21J16B__)
 #include "saml21/include_b/saml21j16b.h"
#elif defined(__SAML21J17B__) || defined(__ATSAML21J17B__)
 #include "saml21/include_b/saml21j17b.h"
#elif defined(__SAML21J18B__) || defined(__ATSAML21J18B__)
 #include "saml21/include_b/saml21j18b.h"
#else
 #error "Library does not support the specified device."
#endif

#endif
