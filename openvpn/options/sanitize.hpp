//
//  sanitize.hpp
//  OpenVPN
//
//  Copyright (c) 2012 OpenVPN Technologies, Inc. All rights reserved.
//

// Sanitize certain kinds of strings before they are output to the log file.

#ifndef OPENVPN_OPTIONS_SANITIZE_H
#define OPENVPN_OPTIONS_SANITIZE_H

#include <string>
#include <cstring>

#include <openvpn/common/exception.hpp>
#include <openvpn/common/options.hpp>

namespace openvpn {

  inline std::string render_options_sanitized(const OptionList& opt)
  {
    std::ostringstream out;
    for (size_t i = 0; i < opt.size(); i++)
      {
	const Option& o = opt[i];
#ifndef OPENVPN_SHOW_SESSION_TOKEN
	if (o.get_optional(0, 0) == "auth-token")
	  out << i << " [auth-token] ..." << std::endl;
	else
#endif
	  out << i << ' ' << o.render() << std::endl;
      }
    return out.str();
  }

  // Remove security-sensitive strings from control message
  // so that they will not be output to log file.
  std::string sanitize_control_message(const std::string& src_str)
  {
#ifdef OPENVPN_SHOW_SESSION_TOKEN
    return src_str;
#else
    const char *src = src_str.c_str();
    char *ret = new char[src_str.length()+1];
    char *dest = ret;
    bool redact = false;
    int skip = 0;

    for (;;)
      {
	const char c = *src;
	if (c == '\0')
	  break;
	if (c == 'S' && !::strncmp(src, "SESS_ID_", 8))
	  {
	    skip = 7;
	    redact = true;
	  }
	else if (c == 'e' && !::strncmp(src, "echo ", 5))
	  {
	    skip = 4;
	    redact = true;
	  }

	if (c == ',') /* end of redacted item? */
	  {
	    skip = 0;
	    redact = false;
	  }

	if (redact)
	  {
	    if (skip > 0)
	      {
		--skip;
		*dest++ = c;
	      }
	  }
	else
	  *dest++ = c;

	++src;
      }
    *dest = '\0';

    const std::string ret_str(ret);
    delete [] ret;
    return ret_str;
#endif
  }

}

#endif
