#ifndef _IFADDRS_H
#include <inet/ifaddrs.h>
#include <stdbool.h>
#include <stdint.h>

libc_hidden_proto (getifaddrs)
libc_hidden_proto (freeifaddrs)

struct in6addrinfo
{
  enum {
    in6ai_deprecated = 1,
    in6ai_homeaddress = 2
  } flags:8;
  uint8_t prefixlen;
  uint16_t :16;
  uint32_t index;
  uint32_t addr[4];
};

extern void __check_pf (bool *seen_ipv4, bool *seen_ipv6,
			struct in6addrinfo **in6ai, size_t *in6ailen)
  attribute_hidden;
extern void __check_native (uint32_t a1_index, int *a1_native,
			    uint32_t a2_index, int *a2_native)
  attribute_hidden;

#endif	/* ifaddrs.h */
