//
//  ntlm.hpp
//  OpenVPN
//
//  Copyright (c) 2012 OpenVPN Technologies, Inc. All rights reserved.
//

// Low-level methods used to implement NTLMv2 proxy authentication

#ifndef OPENVPN_PROXY_NTLM_H
#define OPENVPN_PROXY_NTLM_H

#include <cstring>
#include <string>
#include <vector>

#include <boost/cstdint.hpp> // for boost::uint32_t, uint64_t
#include <boost/algorithm/string.hpp> // for boost::to_upper_copy

#include <openvpn/common/types.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/base64.hpp>
#include <openvpn/common/split.hpp>
#include <openvpn/common/unicode.hpp>
#include <openvpn/time/time.hpp>
#include <openvpn/buffer/buffer.hpp>

namespace openvpn {
  namespace HTTPProxy {

    template <typename RAND_API, typename CRYPTO_API>
    class NTLM
    {
    public:
      /*
       * NTLMv2 handshake
       * http://davenport.sourceforge.net/ntlm.html
       *
       */

      static std::string phase_1()
      {
	return "TlRMTVNTUAABAAAAAgIAAA==";
      }

      static std::string phase_3(const std::string& phase_2_response,
				 const std::string& dom_username,
				 const std::string& password,
				 RAND_API& rng)
      {
	// sanity checks
	if (dom_username.empty())
	  throw Exception("username is blank");
	if (password.empty())
	  throw Exception("password is blank");

	// split domain\username
	std::string domain;
	std::string username;
	split_domain_username(dom_username, domain, username);

	// convert password from utf-8 to utf-16 and take an MD4 hash of it
	BufferPtr password_u = Unicode::string_to_utf16(password);
	typename CRYPTO_API::DigestContext md4_ctx(CRYPTO_API::Digest::md4());
	md4_ctx.update(password_u->c_data(), password_u->size());
	unsigned char md4_hash[21];
	md4_ctx.final(md4_hash);
	std::memset(md4_hash + 16, 0, 5); // pad to 21 bytes

	// decode phase_2_response from base64 to raw data
	BufferAllocated response(phase_2_response.size(), 0);
	base64->decode(response, phase_2_response);

	// extract the challenge from bytes 24-31 in the response
	unsigned char challenge[8];
	for (size_t i = 0; i < 8; ++i)
	  challenge[i] = response[i+24];

	// concatenate uppercase(username) + domain,
	// convert to utf-16, and run it through HMAC-MD5
	// keyed to md4_hash
	const std::string ud = boost::to_upper_copy(username) + domain;
	BufferPtr ud_u = Unicode::string_to_utf16(ud);
	typename CRYPTO_API::HMACContext hmac_ctx1(CRYPTO_API::Digest::md5(), md4_hash, 16);
	hmac_ctx1.update(ud_u->c_data(), ud_u->size());
	unsigned char ntlmv2_hash[16];
	hmac_ctx1.final(ntlmv2_hash);

	// NTLMv2 Blob
	unsigned char ntlmv2_response[144];
	unsigned char *ntlmv2_blob = ntlmv2_response + 16; // inside ntlmv2_response, length: 128
	memset(ntlmv2_blob, 0, 128);           // clear blob buffer
	ntlmv2_blob[0x00]=1;                   // signature
	ntlmv2_blob[0x01]=1;                   // signature
	ntlmv2_blob[0x04]=0;                   // reserved
	store_win_time(ntlmv2_blob + 0x08);    // 64-bit Windows-style timestamp
	rng.rand_bytes(ntlmv2_blob + 0x10, 8); // 64-bit client nonce
	ntlmv2_blob[0x18]=0;                   // unknown, zero should work

	// add target information block to the blob
	size_t tib_len = 0;
	if (response[0x16] & 0x80)     // check for Target Information block (TIB)
	  {
	    tib_len = response[0x28];  // get TIB size
	    if (tib_len > 96)
	      tib_len = 96;
	    const size_t tib_offset = response[0x2c];
	    if (tib_offset + tib_len < response.size())
	      {
		const unsigned char *tib_ptr = response.c_data() + tib_offset; // get TIB pointer
		std::memcpy(&ntlmv2_blob[0x1c], tib_ptr, tib_len);             // copy TIB into the blob
	      }
	    else
	      tib_len = 0;
	  }
	ntlmv2_blob[0x1c + tib_len] = 0; // unknown, zero works

	// Get blob length
	const size_t ntlmv2_blob_size = 0x20 + tib_len; 

	// Add challenge from message 2
	std::memcpy(&ntlmv2_response[8], challenge, 8);

	// hmac-md5
	typename CRYPTO_API::HMACContext hmac_ctx2(CRYPTO_API::Digest::md5(), ntlmv2_hash, 16);
	hmac_ctx2.update(&ntlmv2_response[8], ntlmv2_blob_size + 8);
	unsigned char ntlmv2_hmacmd5[16];
	hmac_ctx2.final(ntlmv2_hmacmd5);

	// add hmac-md5 result to the blob
	// Note: This overwrites challenge previously written at ntlmv2_response[8..15]
	std::memcpy(ntlmv2_response, ntlmv2_hmacmd5, 16);

	// start building phase3 message (what we return to caller)
	BufferAllocated phase3(0x40, BufferAllocated::ARRAY|BufferAllocated::CONSTRUCT_ZERO|BufferAllocated::GROW);
	std::strcpy((char *)phase3.data(), "NTLMSSP"); // signature
	phase3[8] = 3;                                 // type 3

	// NTLMv2 response
	add_security_buffer(0x14, ntlmv2_response, ntlmv2_blob_size + 16, phase3);

	// username
	add_security_buffer(0x24, username.c_str(), username.length(), phase3);

	// Set domain. If <domain> is empty, default domain will be used (i.e. proxy's domain).
	add_security_buffer(0x1c, domain.c_str(), domain.size(), phase3);

	// other security buffers will be empty
	phase3[0x10] = phase3.size(); // lm not used
	phase3[0x30] = phase3.size(); // no workstation name supplied
	phase3[0x38] = phase3.size(); // no session key

	// flags
	phase3[0x3c] = 0x02; // negotiate oem
	phase3[0x3d] = 0x02; // negotiate ntlm

	return base64->encode(phase3);
      }

    private:
      // adds security buffer data to a message and sets security buffer's offset and length
      static void add_security_buffer(const size_t sb_offset,
				      const void *data,
				      const unsigned char length,
				      Buffer& msg_buf)
      {
	msg_buf[sb_offset] = length;
	msg_buf[sb_offset + 2] = length;
	msg_buf[sb_offset + 4] = msg_buf.size() & 0xff;
	msg_buf[sb_offset + 5] = (msg_buf.size() >> 8) & 0xff;
	msg_buf.write((unsigned char *)data, length);
      }

      // store 64-bit windows time into a little-endian 8-byte buffer
      static void store_win_time(unsigned char *dest)
      {
	const boost::uint64_t wt = Time::win_time();
	dest[0]= wt;
	dest[1]= (wt  >> 8);
	dest[2]= (wt  >> 16);
	dest[3]= (wt  >> 24);
	dest[4]= (wt  >> 32);
	dest[5]= (wt  >> 40);
	dest[6]= (wt  >> 48);
	dest[7]= (wt  >> 56);
      }

      static void split_domain_username(const std::string& combined, std::string& domain, std::string& username)
      {
	typedef std::vector<std::string> StringList;
	StringList sl;
	sl.reserve(2);
	Split::by_char_void<StringList, NullLex, Split::NullLimit>(sl, combined, '\\', 1);
	if (sl.size() == 1)
	  {
	    domain = "";
	    username = sl[0];
	  }
	else if (sl.size() == 2)
	  {
	    domain = sl[0];
	    username = sl[1];
	  }
	else
	  throw Exception("split_domain_username failed");
      }

    };
  }
}

#endif
