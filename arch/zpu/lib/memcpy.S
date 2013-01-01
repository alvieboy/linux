/*
 Fast memcpy implementation for ZPUIno
 */

#define MEMCPY_TRY_ALIGN


  .text
  .globl memcpy
  .type memcpy, @function

memcpy:

  /* (SP): ret
     (SP+4): destination
     (SP+8): source
     (SP+12): count
  */

  /* Check for count==0 */

  loadsp 12
  im 0
  eq
  impcrel .Lmemcpy_exit
  neqbranch

  /* Check for fully aligned source and dest */
  im 3
  loadsp 8
  and
  im 3
  loadsp 16
  and
  add
  impcrel .Lnotaligned
  neqbranch

  /* everything aligned */
  /* Place return value at once. */
  loadsp 8
  im _memreg
  store

  /* Convert to words */
  loadsp 12
  flip
  addsp 0
  addsp 0
  flip
  /* Number of words now in TOS */
  loadsp 0
  impcrel .Lmoveword
  neqbranch           /* Skip if we have no word to copy */

  storesp 0
  impcrel .Lnotaligned
  poppcrel

.Lmoveword:
  loadsp 12 /* Source */
  load
  loadsp 12  /* Dst */
  store
  /* Incremnt pointrs*/
  im 4
  addsp 16
  storesp 16
  im 4
  addsp 12
  storesp 12
  /* Decrment words */
  im -1
  add
  loadsp 0
  impcrel .Lmoveword

  neqbranch
  storesp 0 /* Get rid of counter */

  /* Check if we have extra bytes to copy */

  loadsp 12
  im 3
  and
  loadsp 0
  storesp 20
  /* Updated with leftover */
  impcrel .Lnotaligned
  neqbranch
  /* Done, return to caller */
  poppc

.Lnotaligned:
  loadsp 8
  loadb
  loadsp 8
  storeb
  im 1
  addsp 12
  storesp 12
  im 1
  addsp 8
  storesp 8

  /* Decr count */
  im -1
  addsp 16
  loadsp 0
  storesp 20
  impcrel .Lnotaligned

  neqbranch
.Lmemcpy_exit:
  poppc

  .size memcpy, . - memcpy
