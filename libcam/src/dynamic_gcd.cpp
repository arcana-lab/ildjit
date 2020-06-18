#include <cstdlib>
#include <algorithm>
#include "dynamic_gcd.h"
using namespace std;
/*
 * Greatest common divisor
 */
uintptr_t euclid_gcd(uintptr_t x, uintptr_t y){
  uintptr_t t;
  while(y != 0){
    t = y;
    y = x%y;
    x = t;
  }
  return x;
}

/*
 * Lowest common multiple
 */
uintptr_t euclid_lcm(uintptr_t x, uintptr_t y){
  return ((uint64_t)(x)*(uint64_t)(y))/euclid_gcd(x, y);
}

/*
 * Returns (x, y) solution to ax + by = gcd(a, b)
 * See http://en.wikipedia.org/wiki/Extended_Euclidean_algorithm
 */
pair<intptr_t, intptr_t> extended_euclid(intptr_t a, intptr_t b){
  int64_t s = 0, old_s = 1;
  int64_t t = 1, old_t = 0;
  int64_t r = b, old_r = a;
  int64_t temp;
  while(r != 0){
    int64_t quot = old_r/r;
    temp = r; r = old_r - quot*r; old_r = temp;
    temp = t; t = old_t - quot*t; old_t = temp;
    temp = s; s = old_s - quot*s; old_s = temp;
  }
  return pair<intptr_t, intptr_t>(old_s, old_t);
}

AliasT dynamic_gcd(uintptr_t low1, uintptr_t low2, uintptr_t high1, uintptr_t high2, uintptr_t stride1, uintptr_t stride2){
  if (low2 < low1){
    swap(low1, low2);
    swap(high1, high2);
    swap(stride1, stride2);
  }
  uintptr_t high = min(high1, high2);
  uintptr_t length = high - low2 + 1;
  uintptr_t delta = (stride1 - ((low2 - low1)%stride1))%stride1;
  uintptr_t gcd = euclid_gcd(stride1, stride2);
  uintptr_t lcm = euclid_lcm(stride1, stride2);
  if(delta%gcd != 0)
    return AliasT(0,0,0);
  intptr_t y = extended_euclid(stride1, stride2).second;
  uintptr_t offset = ((( (uint64_t)(stride2)*(int64_t)(y)*(uint64_t)(delta) )/gcd) + lcm)%lcm;
  uintptr_t count = ((length - (offset + 1)) + lcm)/lcm;
  return AliasT(low2 + offset, lcm, count);
}

