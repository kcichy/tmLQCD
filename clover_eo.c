/* $Id$ */

/*****************************************
 * This file contains all the operators
 * needed for Wilson fermions with
 * clover improvement with Even-Odd
 * preconditioning.
 *
 *****************************************/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "sse.h"
#include "su3.h"
#include "su3adj.h"
#include "global.h"
#include "xchange.h"
#include "H_eo.h"
#include "clover_eo.h"


static su3_vector psi1, psi2, psi, chi, phi1, phi3;

/* declaration of internal functions */

/******************************************
 * clover_inv is (1+T_{ee|oo})^{-1}
 *
 * for ieo == 1 clover_inv is for even even
 * for ieo == 0 it is for odd odd
 *
 * l is number of input and output field
 *
 ******************************************/

void clover_inv(int ieo, int l);

/******************************************
 * clover computes
 * l = ((q_off*1+T_{ee|oo})*k-j)
 *
 * for ieo == 0 clover is for T_{oo}
 *     but uses only even sides of l
 * for iwo == 0 it is for T_{ee}
 *     and uses only odd sides of l
 *
 * l is number of the output field
 * k and j the numbers of the input fields
 *
 ******************************************/

void clover(int ieo, int l, int k, int j, double q_off);

/******************************************
 * clover_gamma5 computes
 * l = gamma_5*((q_off*1+T_{ee|oo})*k-j)
 *
 * l is the number of the output field
 * k and j the numbers of the input fields
 *
 * for ieo == 0 it is T_{oo} but the 
 *     but the it works only on even sides
 *     of the output field l
 * for ieo == 1 it is T_{ee}
 *     and it works on odd sides of l.
 *
 ******************************************/
void clover_gamma5(int ieo, int l, int k, int j, double q_off);

/******************************************
 * gamma5 multiplies k with \gamma_5
 * and stores the result in l
 * 
 * l is the number of the output field
 * k is the number of the input field 
 *
 ******************************************/

void gamma5(int l,int k);

/* definition of external accessible functions */



/******************************************
 * Q_psi is the same as \hat Q
 * applied to a field \psi (see hep-lat/9603008)
 *
 * \hat Q = \gamma_5(q_off*1+T_{ee}
 *          - M_{oe}(1+T_{ee})^(-1)M_{eo})
 *
 * l is the number of the output field 
 * k is the number of the input field
 * q_off is the parameter for the second or
 *       third pf field.
 *
 ******************************************/

void Q_psi(int l, int k, double q_off){
  H_eo(1, DUM_MATRIX+1, k);
  clover_inv(1, DUM_MATRIX+1);
  H_eo(0, DUM_MATRIX, DUM_MATRIX+1);
  clover_gamma5(0, l, k, DUM_MATRIX, q_off);
}

/******************************************
 * This is the fermion matrix:
 * \gamma_5 \hat Q or simply
 * \hat Q without \gamma_5
 *
 * for input output see Q_psi
 *
 ******************************************/

void M_psi(int l, int k, double q_off){
  H_eo(1, DUM_MATRIX+1, k);
  clover_inv(1, DUM_MATRIX+1);
  H_eo(0, DUM_MATRIX, DUM_MATRIX+1);
  clover(0, l, k, DUM_MATRIX, q_off);
}

void H_eo_psi(int ieo, int l, int k){
  H_eo(ieo, l, k);
  clover_inv(ieo, l);
}



/* definitions of internal functions */

void clover_inv(int ieo, int l){
  int ix;
  int ioff,icx;
  su3 *w1,*w2,*w3;
  spinor *rn;

  /* Set the offset to work only */
  /* on even or odd sides */
  if(ieo == 0){
    ioff = 0;
  } 
  else{
    ioff = (VOLUME+RAND)/2;
  }
  /************ loop over all lattice sites ************/
  for(icx = ioff; icx < (VOLUME/2+ioff); icx++){
    ix=trans2[icx];
    rn=&spinor_field[l][icx-ioff];
    /* noch schlimmer murks fuer den clover-term */
    _vector_assign(phi1, (*rn).c1);
    _vector_assign(phi3, (*rn).c3);

    w1=&sw_inv[ix][0][0];
    w2=w1+2;  /* &sw_inv[ix][1][0]; */
    w3=w1+4;  /* &sw_inv[ix][2][0]; */
    _su3_multiply(psi, *w1, phi1); 
    _su3_multiply(chi, *w2, (*rn).c2);
    _vector_add((*rn).c1, psi,chi);
    _su3_inverse_multiply(psi, *w2,phi1); 
    _su3_multiply(chi, *w3, (*rn).c2);
    _vector_add((*rn).c2, psi, chi);

    w1++; /* &sw_inv[ix][0][1]; */
    w2++; /* &sw_inv[ix][1][1]; */
    w3++; /* &sw_inv[ix][2][1]; */
    _su3_multiply(psi, *w1, phi3); 
    _su3_multiply(chi, *w2, (*rn).c4);
    _vector_add((*rn).c3, psi, chi);
    _su3_inverse_multiply(psi, *w2, phi3); 
    _su3_multiply(chi,*w3,(*rn).c4);
    _vector_add((*rn).c4, psi, chi);
    /****************** end of loop *******************/
  }
}
/*
 * l is the number of the output field
 * k,j are the numbers of input fields
 *
 * ieo == 0 indicates to use T_{oo}
 *          but to work only on even sides of
 *          l
 * ieo == 1 the opposite
 * 
 *
 * computes l = gamma_5*((q_off*1+T_{ee|oo})*k-j)
 */

void clover_gamma5(int ieo, int l, int k, int j, double q_off){
  int ix;
  int ioff,icx;
  su3 *w1,*w2,*w3;
  spinor *r,*s,*t;

  /* Set the offset to work only */
  /* on even or odd sides */
  if(ieo==0){
    ioff = 0;
  } 
  else{
    ioff = (VOLUME+RAND)/2;
  }
  /******* loop over all lattice sites ********/
  for(icx = ioff; icx < (VOLUME/2+ioff); icx++){
    ix=trans2[icx];
    r=&spinor_field[l][icx-ioff];
    s=&spinor_field[k][icx-ioff];
    t=&spinor_field[j][icx-ioff];

    /* upper left part */
    w1=&sw[ix][0][0];
    w2=w1+2; /*&sw[ix][1][0];*/
    w3=w1+4; /*&sw[ix][2][0];*/
    _su3_multiply(psi1, *w1, (*s).c1); 
    _su3_multiply(chi, *w2, (*s).c2);
    _vector_add_assign(psi1, chi);
    _su3_inverse_multiply(psi2, *w2, (*s).c1); 
    _su3_multiply(chi, *w3, (*s).c2);
    _vector_add_assign(psi2, chi); 

    /*************** contribution from q_off ******************/
    _vector_add_mul(psi1, q_off, (*s).c1);
    _vector_add_mul(psi2, q_off,(*s).c2);

    /**********************************************************/
    /* Subtract t and store the result in r */
    _vector_sub((*r).c1, psi1, (*t).c1);
    _vector_sub((*r).c2, psi2, (*t).c2);

    /* lower right part */
    w1++; /*=&sw[ix][0][1];*/
    w2++; /*=&sw[ix][1][1];*/
    w3++; /*=&sw[ix][2][1];*/
    _su3_multiply(psi1, *w1, (*s).c3);
    _su3_multiply(chi, *w2, (*s).c4);
    _vector_add_assign(psi1, chi); 
    _su3_inverse_multiply(psi2, *w2, (*s).c3); 
    _su3_multiply(chi, *w3, (*s).c4);
    _vector_add_assign(psi2, chi); 

    /*************** contribution from q_off ******************/
    _vector_add_mul(psi1, q_off, (*s).c3);
    _vector_add_mul(psi2, q_off, (*s).c4);

    /* Subtract t and store the result in r */
    /* multiply with  gamma5 included by    */
    /* reversed order of t and psi1|2       */
    _vector_sub((*r).c3, (*t).c3, psi1);
    _vector_sub((*r).c4, (*t).c4, psi2);

    /************************ end of loop *********************/
  }
}

void clover(int ieo, int l, int k, int j, double q_off){
  int ix;
  int ioff,icx;
  su3 *w1,*w2,*w3;
  spinor *r,*s,*t;

  if(ieo==0){
    ioff = 0;
  } 
  else{
    ioff = (VOLUME+RAND)/2;
  }
  /**************** loop over all lattice sites ************/
  for(icx = ioff; icx < (VOLUME/2+ioff); icx++){
    ix=trans2[icx];
    r=&spinor_field[l][icx-ioff];
    s=&spinor_field[k][icx-ioff];
    t=&spinor_field[j][icx-ioff];

    w1=&sw[ix][0][0];
    w2=w1+2; /*&sw[ix][1][0];*/
    w3=w1+4; /*&sw[ix][2][0];*/
    _su3_multiply(psi1, *w1, (*s).c1); 
    _su3_multiply(chi, *w2, (*s).c2);
    _vector_add_assign(psi1, chi);
    _su3_inverse_multiply(psi2, *w2, (*s).c1); 
    _su3_multiply(chi, *w3, (*s).c2);
    _vector_add_assign(psi2, chi); 

    /*************** contribution from q_off *********/
    _vector_add_mul(psi1, q_off, (*s).c1);
    _vector_add_mul(psi2, q_off, (*s).c2);

    /*************************************************/
    /* Subtract t and store the result in r */
    _vector_sub((*r).c1, psi1, (*t).c1);
    _vector_sub((*r).c2, psi2, (*t).c2);

    w1++; /*=&sw[ix][0][1];*/
    w2++; /*=&sw[ix][1][1];*/
    w3++; /*=&sw[ix][2][1];*/
    _su3_multiply(psi1, *w1, (*s).c3); 
    _su3_multiply(chi, *w2, (*s).c4);
    _vector_add_assign(psi1, chi); 
    _su3_inverse_multiply(psi2, *w2, (*s).c3); 
    _su3_multiply(chi, *w3, (*s).c4);
    _vector_add_assign(psi2, chi); 

    /************** contribution from q_off **********/
    _vector_add_mul(psi1, q_off, (*s).c3);
    _vector_add_mul(psi2, q_off, (*s).c4);

    /*************************************************/
    /* Subtract t and store the result in r */
    _vector_sub((*r).c3, psi1, (*t).c3);
    _vector_sub((*r).c4, psi2, (*t).c4);

    /******************* end of loop *****************/
  }
}

/* output l , input k; k and l identical is permitted*/






void gamma5(int l,int k){
  int ix;
  spinor *r,*s;
  for (ix=0;ix<VOLUME/2;ix++){
    r=&spinor_field[l][ix];
    s=&spinor_field[k][ix];
    _vector_assign((*r).c1,(*s).c1);
    _vector_assign((*r).c2,(*s).c2);
    _vector_minus_assign((*r).c3,(*s).c3);
    _vector_minus_assign((*r).c4,(*s).c4);
  }
}