/*******************************************************************************
*
* File hybrid.c
*
* Benchmark program for the even-odd preconditioned Wilson-Dirac operator
*
* Author: Martin Hasenbusch
* Date: Wed, Aug 29, 2001 02:06:26 PM
*
*******************************************************************************/

#define MAIN_PROGRAM

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "su3.h"
#include "su3adj.h"
#include "ranlxd.h"
#include "geometry_eo.h"
#include "start.h"
#include "boundary.h"
#include "Hopping_Matrix.h"
#include "Hopping_Matrix_nocom.h"
#include "global.h"
#include "xchange.h"
#ifdef MPI
#include "mpi_init.h"
#endif

int main(int argc,char *argv[])
{
  int j,j_max,k,kb,k_max;
  static double t1,t2,dt,sdt,dts,dt2,qdt,sqdt;
  int rlxd_state[105];

#ifdef MPI  
  mpi_init(argc, argv);
#endif

  if(g_proc_id==0) {
    printf("\n");
    fprintf(stdout,"The number of processes is %d \n",g_nproc);
    fprintf(stdout,"The local lattice size is %d x %d x %d^2 \n",T,LX,L);
    fflush(stdout);
  }

  /* define the geometry */
  geometry();
  /* define the boundary conditions for the fermion fields */
  boundary();
  
  /* here we generate exactly the same configuration as for the 
     single node simulation */
  if(g_proc_id==0) {
    rlxd_init(1, 123456);   
    random_gauge_field();
    /*  send the state of the random-number generator to 1 */
#ifdef MPI
    rlxd_get(rlxd_state);
    MPI_Send((void*)rlxd_state, 105, MPI_INT, 1, 99, MPI_COMM_WORLD);
#endif
  }
#ifdef MPI
  else {
    /* recieve the random number state form g_proc_id-1 */
    MPI_Recv((void*)rlxd_state, 105, MPI_INT, g_proc_id-1, 99, MPI_COMM_WORLD, &status);
    rlxd_reset(rlxd_state);
    random_gauge_field();
    /* send the random number state to g_proc_id+1 */
    k=g_proc_id+1; 
    if(k==g_nproc) k=0;
    rlxd_get(rlxd_state);
    MPI_Send((void*)rlxd_state, 105, MPI_INT, k, 99, MPI_COMM_WORLD);
  }
  if(g_proc_id==0) {
    MPI_Recv((void*)rlxd_state, 105, MPI_INT,g_nproc-1,99, MPI_COMM_WORLD, &status);
    rlxd_reset(rlxd_state);
  }

  /*For parallelization: exchange the gaugefield */
  xchange_gauge();
#endif

  /* set the backward gauge field */
  for(k=0;k<VOLUME+RAND;k++) {
    kb=g_idn[k][0];
    _su3_assign(g_gauge_field_back[k][0],g_gauge_field[kb][0]);
    kb=g_idn[k][1];
    _su3_assign(g_gauge_field_back[k][1],g_gauge_field[kb][1]);
    kb=g_idn[k][2];
    _su3_assign(g_gauge_field_back[k][2],g_gauge_field[kb][2]);
    kb=g_idn[k][3];
    _su3_assign(g_gauge_field_back[k][3],g_gauge_field[kb][3]);
  }


  /*initialize the pseudo-fermion fields*/
  k_max=(NO_OF_SPINORFIELDS)/3;
  j_max=1; 
  sdt=0.;
  for (k=0;k<k_max;k++) {
    random_spinor_field(k);
  }

  while(sdt<30.) {
    MPI_Barrier(MPI_COMM_WORLD);
    t1=(double)clock();
    for (j=0;j<j_max;j++) {
      for (k=0;k<k_max;k++) {
	Hopping_Matrix(0, spinor_field[k+k_max], spinor_field[k]);
	Hopping_Matrix(1, spinor_field[k+2*k_max], spinor_field[k+k_max]);
      }
    }
    t2=(double)clock();

    dt=(t2-t1)/((double)(CLOCKS_PER_SEC));
    MPI_Allreduce (&dt, &sdt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    qdt=dt*dt;
    MPI_Allreduce (&qdt, &sqdt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    sdt=sdt/g_nproc;
    sqdt=sqrt(sqdt/g_nproc-sdt*sdt);
    j_max*=2;
  }
  j_max=j_max/2;
  dts=dt;
  sdt=1.0e6f*sdt/((double)(k_max*j_max*(VOLUME)));
  sqdt=1.0e6f*sqdt/((double)(k_max*j_max*(VOLUME)));

  if(g_proc_id==0) {
    printf("total time %e sec, Variance of the time %e sec \n",sdt,sqdt);
    printf("\n");
    printf(" (%d Mflops [%d bit arithmetic])\n",
	   (int)(1320.0f/sdt),(int)sizeof(spinor)/3);
    printf("\n");
    fflush(stdout);
  }

  /* isolated computation */
  t1=(double)clock();
  for (j=0;j<j_max;j++) {
    for (k=0;k<k_max;k++) {
/*       Hopping_Matrix_nocom(0, spinor_field[k+k_max], spinor_field[k]); */
      Hopping_Matrix_nocom(0, spinor_field[k+k_max], spinor_field[k]);
/*       Hopping_Matrix_nocom(1, spinor_field[k+2*k_max], spinor_field[k+k_max]); */
      Hopping_Matrix_nocom(1, spinor_field[k+2*k_max], spinor_field[k+k_max]);
    }
  }
  t2=(double)clock();

  dt2=(t2-t1)/((double)(CLOCKS_PER_SEC));
  /* compute the bandwidth */
  dt=dts-dt2;
  MPI_Allreduce (&dt, &sdt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  sdt=sdt/g_nproc;

  MPI_Allreduce (&dt2, &dt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  dt=dt/g_nproc;
  dt=1.0e6f*dt/((double)(k_max*j_max*(VOLUME)));
  if(g_proc_id==0) {
    printf("communication switched off \n");
    printf(" (%d Mflops [%d bit arithmetic])\n",
	   (int)(1320.0f/dt),(int)sizeof(spinor)/3);
    printf("\n");
    fflush(stdout);
  }

  sdt=sdt/k_max;
  sdt=sdt/j_max;
  sdt=sdt/(2*SLICE);
  if(g_proc_id==0) {
    printf("The size of the package is %d Byte \n",(SLICE)*192);
    printf("The bandwidth is %5.2f + %5.2f   MB/sec\n",
	   0.000001*2.*192./sdt, 0.000001*2.*192./sdt);
    fflush(stdout);
  }
  fflush(stdout);
  MPI_Finalize();
  return(0);
}