#ifndef _MATH_PRIVATE_H_
#error "Never use <math_ldbl.h> directly; include <math_private.h> instead."
#endif

/* A union which permits us to convert between a long double and
   three 32 bit ints.  */

#if __FLOAT_WORD_ORDER == BIG_ENDIAN

typedef union
{
  long double value;
  struct
  {
    unsigned int empty0:32;
    int sign_exponent:16;
    unsigned int empty1:16;
    u_int32_t msw;
    u_int32_t lsw;
  } parts;
} ieee_long_double_shape_type;

#endif

#if __FLOAT_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  long double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
    int sign_exponent:16;
    unsigned int empty1:16;
    unsigned int empty0:32;
  } parts;
} ieee_long_double_shape_type;

#endif

/* Get three 32 bit ints from a double.  */

#define GET_LDOUBLE_WORDS(exp,ix0,ix1,d)			\
do {								\
  ieee_long_double_shape_type ew_u;				\
  ew_u.value = (d);						\
  (exp) = ew_u.parts.sign_exponent;				\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

/* Set a double from two 32 bit ints.  */

#define SET_LDOUBLE_WORDS(d,exp,ix0,ix1)			\
do {								\
  ieee_long_double_shape_type iw_u;				\
  iw_u.parts.sign_exponent = (exp);				\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)

/* Get the more significant 32 bits of a long double mantissa.  */

#define GET_LDOUBLE_MSW(v,d)					\
do {								\
  ieee_long_double_shape_type sh_u;				\
  sh_u.value = (d);						\
  (v) = sh_u.parts.msw;						\
} while (0)

/* Set the more significant 32 bits of a long double mantissa from an int.  */

#define SET_LDOUBLE_MSW(d,v)					\
do {								\
  ieee_long_double_shape_type sh_u;				\
  sh_u.value = (d);						\
  sh_u.parts.msw = (v);						\
  (d) = sh_u.value;						\
} while (0)

/* Get int from the exponent of a long double.  */

#define GET_LDOUBLE_EXP(exp,d)					\
do {								\
  ieee_long_double_shape_type ge_u;				\
  ge_u.value = (d);						\
  (exp) = ge_u.parts.sign_exponent;				\
} while (0)

/* Set exponent of a long double from an int.  */

#define SET_LDOUBLE_EXP(d,exp)					\
do {								\
  ieee_long_double_shape_type se_u;				\
  se_u.value = (d);						\
  se_u.parts.sign_exponent = (exp);				\
  (d) = se_u.value;						\
} while (0)
