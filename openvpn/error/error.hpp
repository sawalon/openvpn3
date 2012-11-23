//
//  error.hpp
//  OpenVPN
//
//  Copyright (c) 2012 OpenVPN Technologies, Inc. All rights reserved.
//

// Define OpenVPN error codes and a method to convert them to a string representation

#ifndef OPENVPN_ERROR_ERROR_H
#define OPENVPN_ERROR_ERROR_H

namespace openvpn {
  namespace Error {

    enum Type {
      SUCCESS=0,           // no error
      NETWORK_RECV_ERROR,  // errors receiving on network socket
      NETWORK_EOF_ERROR,   // EOF received on TCP network socket
      NETWORK_SEND_ERROR,  // errors sending on network socket
      DECRYPT_ERROR,       // data channel encrypt/decrypt error
      HMAC_ERROR,          // HMAC verification failure
      REPLAY_ERROR,        // error from PacketIDReceive
      BUFFER_ERROR,        // exception thrown in Buffer methods
      CC_ERROR,            // general control channel errors
      BAD_SRC_ADDR,        // packet from unknown source address
      COMPRESS_ERROR,      // compress/decompress errors on data channel
      RESOLVE_ERROR,       // DNS resolution error
      SOCKET_PROTECT_ERROR, // Error calling protect() method on socket
      TUN_READ_ERROR,      // read errors on tun/tap interface
      TUN_WRITE_ERROR,     // write errors on tun/tap interface
      TUN_FRAMING_ERROR,   // error with tun PF_INET/PF_INET6 prefix
      TUN_SETUP_FAILED,    // error setting up tun/tap interface
      TUN_IFACE_CREATE,    // error creating tun/tap interface
      REROUTE_GW_NO_DNS,   // redirect-gateway specified without alt DNS servers
      TCP_OVERFLOW,        // TCP output queue overflow
      TCP_SIZE_ERROR,      // bad embedded uint16_t TCP packet size
      TCP_CONNECT_ERROR,   // client error on TCP connect
      SSL_ERROR,           // errors resulting from read/write on SSL object
      ENCAPSULATION_ERROR, // exceptions thrown during packet encapsulation
      EPKI_CERT_ERROR,     // error obtaining certificate from External PKI provider
      EPKI_SIGN_ERROR,     // error obtaining RSA signature from External PKI provider
      HANDSHAKE_TIMEOUT,   // handshake failed to complete within given time frame
      KEEPALIVE_TIMEOUT,   // lost contact with peer
      CONNECTION_TIMEOUT,  // connection failed to establish within given time
      PRIMARY_EXPIRE,      // primary key context expired
      CERT_VERIFY_FAIL,    // peer certificate verification failure
      PEM_PASSWORD_FAIL,   // incorrect or missing PEM private key decryption password
      AUTH_FAILED,         // general authentication failure
      CLIENT_HALT,         // HALT message from server received
      CLIENT_RESTART,      // RESTART message from server received
      N_PAUSE,             // Number of transitions to Pause state
      N_RECONNECT,         // Number of reconnections
      PROXY_ERROR,         // HTTP proxy error
      PROXY_NEED_CREDS,    // HTTP proxy needs credentials
      N_ERRORS,

      // undefined error
      UNDEF=SUCCESS,
    };

    inline const char *name(const size_t type)
    {
      static const char *names[] = {
	"SUCCESS",
	"NETWORK_RECV_ERROR",
	"NETWORK_EOF_ERROR",
	"NETWORK_SEND_ERROR",
	"DECRYPT_ERROR",
	"HMAC_ERROR",
	"REPLAY_ERROR",
	"BUFFER_ERROR",
	"CC_ERROR",
	"BAD_SRC_ADDR",
	"COMPRESS_ERROR",
	"RESOLVE_ERROR",
	"SOCKET_PROTECT_ERROR",
	"TUN_READ_ERROR",
	"TUN_WRITE_ERROR",
	"TUN_FRAMING_ERROR",
	"TUN_SETUP_FAILED",
	"TUN_IFACE_CREATE",
	"REROUTE_GW_NO_DNS",
	"TCP_OVERFLOW",
	"TCP_SIZE_ERROR",
	"TCP_CONNECT_ERROR",
	"SSL_ERROR",
	"ENCAPSULATION_ERROR",
	"EPKI_CERT_ERROR",
	"EPKI_SIGN_ERROR",
	"HANDSHAKE_TIMEOUT",
	"KEEPALIVE_TIMEOUT",
	"CONNECTION_TIMEOUT",
	"PRIMARY_EXPIRE",
	"CERT_VERIFY_FAIL",
	"PEM_PASSWORD_FAIL",
	"AUTH_FAILED",
	"CLIENT_HALT",
	"CLIENT_RESTART",
	"N_PAUSE",
	"N_RECONNECT",
	"PROXY_ERROR",
	"PROXY_NEED_CREDS",
      };

      if (type < N_ERRORS)
	return names[type];
      else
	return "UNKNOWN_ERROR_TYPE";
    }
  }
} // namespace openvpn

#endif // OPENVPN_ERROR_ERROR_H
