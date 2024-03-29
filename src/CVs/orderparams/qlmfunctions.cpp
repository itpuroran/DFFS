#include "boost/multi_array.hpp"
#include <complex>
#include <iostream>
#include <math.h>
#include "constants.h"
#include "particle.h"
#include "box.h"
#include "opfunctions.h"

using std::complex;
using std::vector;

typedef boost::multi_array<complex<double>,2> array2d;

// Return vector of indices of particles identified as crystalline by
// the number of 'links' between the particle and its neighbours.
// Note this is the 'Ten-Wolde Frenkel' approach to definining
// crystallinity.

vector<int> xtalpars(const vector<int>& linknums, const int nlinks)
{
   vector<int> xtalpars;

   for (vector<int>::size_type i = 0; i != linknums.size(); ++i) {
      if (linknums[i] >= nlinks) {
         // if particle has >=nlinks crystal links, it is in a
         // crystal environment
         xtalpars.push_back(i);
      }
   }

   return xtalpars;
}

// Return a vector whose elements are number of 'links' for each
// particle in qlm matrix.

vector<int> getnlinks(const array2d& qlmt, const array2d& qlmt_local, const vector<int>& numneigh,
                      const vector<vector<int> >& lneigh, const int nsurf,
                      const int nlinks, const double linkval,
                      const int lval, const int displs)
{
   array2d::index npar = qlmt_local.shape()[0];
   array2d::index Ntot = qlmt.shape()[0];
   std::vector<int> numlinks_local(npar, 0);

   int nlin,k;
   double linval;

   // compute dot product \tilde{qlm}(i).\tilde{qlm}(j) for each
   // neighbour pair, whenever this is greater than the threshold
   // (linkval), we call this a crystal link

   for (array2d::index i = nsurf; i != npar; ++i) {
      nlin = 0;
      // go through each neighbour in turn
      for (int j = 0; j != numneigh[i]; ++j) {
         k = lneigh[i][j];
         linval = 0.0;
         for (int m = 0; m != 2 * lval + 1; ++m) {
            // dot product (sometimes denoted Sij)
            linval += qlmt_local[i][m].real() * qlmt[k][m].real() +
                      qlmt_local[i][m].imag() * qlmt[k][m].imag();
         }
         if (linval >= linkval) {
            nlin = nlin + 1;
         }
      }
      // store number of links for this particle
      numlinks_local[i] = nlin;                      
   }

   return numlinks_local;
}

// average values in vector qlm. 

vector<complex<double> > averageqlm(const array2d& qlm,
                                    const vector<int>& pnums,
                                    const int lval)
{
   vector<complex<double> > qlmaverage(2 * lval + 1,0.0);

   for (array2d::index i = 0; i != pnums.size(); ++i) {
      for (int m = 0; m != 2 * lval + 1; ++m) {
         qlmaverage[m] += qlm[pnums[i]][m];
      }
   }
     
   for (int m = 0; m != 2 * lval + 1; ++m) {
      qlmaverage[m] = qlmaverage[m] / (static_cast<double>(pnums.size()));
   }

   return qlmaverage;
}

// Get Q of all particles in pnums, which gives indexes into qlm. This
// can be used to get Q global, or Q cluster, depending on pnums.

double Qpars(const array2d& qlm,       // qlm(i) for every particle i
             const vector<int>& pnums, // particle indices of interest
             const int lval)           // spherical harmonic number (usually 4 or 6)
{
   // get a vector which contains qlm averaged over all particles
   // with indexes in pnums i.e.
   // [<qlm=-6>, <qlm=-5>, ....., <qlm=6>]
   vector<complex<double> > qlma = averageqlm(qlm, pnums, lval);

   double qvalue = 0.0;
   for (int m = 0; m != 2 * lval + 1; ++m) {
      // note that norm of complex number is its squared
      // magnitude,i.e. norm(x) = |x|^2
      qvalue += norm(qlma[m]);
   }

   qvalue = sqrt(qvalue * (4.0 * PI / (2 * lval + 1)));
   return qvalue;
}

// Get W of all particles in pnums, which gives indexes into qlm This
// can be used to get W global, or W cluster, or the w(i)'s (i.e. W
// for each particle), depending on pnums.

double Wpars(const array2d& qlm,       // qlm(i) for all particles i
             const vector<int>& pnums, // particle indices
             const int lval)           // spherical harmonic number (usually 4 or 6)
{
   // get a vector which contains qlm averaged over all particles
   // with indexes in pnums i.e.
   // [<qlm=-6>, <qlm=-5>, ....., <qlm=6>]
   vector<complex<double> > qlma = averageqlm(qlm, pnums, lval);
     
   complex<double> wval = 0.0;
   // cycle through the wigner symbols in the order they appear in
   // constants.h, remembering to multiply by the correct number of
   // identical permutations
   if (lval == 6) {
      // cycle through all distinct permutations
      // (16 in total for lval = 6)
      for (int pnum = 0; pnum != 16; ++pnum) {
         int i1 = WIGNERINDX6[pnum][0] + 6;
         int i2 = WIGNERINDX6[pnum][1] + 6;
         int i3 = WIGNERINDX6[pnum][2] + 6;
         // casting to type double is apparently necessary
         // due to template for multiplying complex numbers
         wval += static_cast<double>(WIGNERPERM6[pnum] * WIGNER6[pnum])
                 * qlma[i1] * qlma[i2] * qlma[i3];
      }
   }
   else if (lval == 4) {
      // cycle through all distinct permutations
      // (9 in total for lval = 4)
      complex<double> add;
      for (int pnum = 0; pnum != 9; ++pnum) {
         // positive permutations
         int i1 = WIGNERINDX4[pnum][0] + 4;
         int i2 = WIGNERINDX4[pnum][1] + 4;
         int i3 = WIGNERINDX4[pnum][2] + 4;
         wval += static_cast<double>(WIGNERPERM4[pnum] * WIGNER4[pnum])
                 * qlma[i1] * qlma[i2] * qlma[i3];

      }
   }
   else { // should probably throw an exception here
   }     

   // compute denominator
   double qvalue = 0.0;
   for (int m = 0; m != 2 * lval + 1; ++m) {
      // note that norm of complex number is its squared
      // magnitude, i.e. norm(x) = |x|^2
      qvalue += norm(qlma[m]);
   }
   wval = wval / pow(qvalue, 1.5);

   // Return real part, imaginary part should be zero
   return real(wval);
}

// Get ql(i) for every particle i in qlm See Lechner Dellago JCP 129
// 114707 (2008) equation (3) Note that this function can be used to
// compute both ql(i) which is LD equation (3) and \bar{ql(i)}, which
// is LD equation (5).  In the latter case we just need to pass a
// matrix of \bar{qlm} rather than qlm.

vector<double> qls(const array2d& qlm)
{
   int npar = qlm.shape()[0];
   int lval = (qlm.shape()[1] - 1)/ 2;
   vector<double> ql;
   ql.resize(npar);
   vector<int> par(1,0);

   for (array2d::index i = 0; i != npar; ++i) {
      // this is a bit inefficient, since we make a lot of
      // function calls and multiplications.  But is saves code
      // replication and in any case this function should only
      // need to be called once for any particular particle
      // configuration.
      par[0] = i;
      ql[i] = Qpars(qlm, par, lval);
   }
   
   return ql;
}

// Get wl(i) for all particles. See Lechner Dellago JCP 129 114707
// (2008) equation (4). Note that this function can be used to compute
// both wl(i) which is LD equation (4) and \bar{wl(i)}, which is LD
// equation (7).  In the latter case we just need to pass a matrix of
// \bar{qlm} rather than qlm.

vector<double> wls(const array2d& qlm)
{
   int npar = qlm.shape()[0];
   int lval = (qlm.shape()[1] - 1)/ 2;    
   vector<double> wl;
   wl.resize(npar);
   vector<int> par(1,0);   

   for (array2d::index i = 0; i != npar; ++i) {
      // this is a bit inefficient, since we make a lot of
      // function calls and multiplications.  But is saves code
      // replication and in any case this function should only
      // need to be called once for any particular particle
      // configuration
      par[0] = i;
      wl[i] = Wpars(qlm, par, lval);
   }
   
   return wl;     
}

// Convert matrix of qlm(i) to matrix of \tilde{qlm}(i) \tilde{qlm}(i)
// is simply a normalised version of vector qlm(i)

array2d qlmtildes(const array2d& qlm, const vector<int>& numneigh,
                  const int lval)
{
   int npar = qlm.shape()[0];
   array2d qlmt(boost::extents[npar][2 * lval + 1]);
     
   // normalise each of rows in the matrix, this gives qlmtilde
   for (int i = 0; i != npar; ++i) {
      // if particle has no neighbours, all entries in qlm[i]
      // will be zero, and so the norm will be zero
      if (numneigh[i] >= 1) {
         double qnorm = 0.0;
         for (int k = 0; k != 2 * lval + 1; ++k) {
            qnorm = qnorm + norm(qlm[i][k]);
         }
         qnorm = sqrt(qnorm);
         for (int k = 0; k != 2 * lval + 1; ++k) {
            qlmt[i][k] = qlm[i][k]/qnorm;
         }
      }
   }
   return qlmt;
}

// Return matrix of qlmbar(i), qlm for each particle averaged over all
// nearest neighbours.  The matrix has dimensions [i,(2l + 1)]. See
// Lechner and Dellago JCP 129, 114707 Equation (6) BUT (!) note there
// is an error in Lechner Dellago equation: The denominator should be
// N_b + 1 rather than N_b.  This is corrected in: Jungblut and
// Dellago JCP 134, 104501 (2011) Equation (5)

array2d qlmbars(const array2d& qlm, const vector<vector<int> >& lneigh,
                const int lval, const int npar)
{
   //int npar = qlm.shape()[0];
   array2d qlmbar(boost::extents[npar][2 * lval + 1]);     

   for (int i = 0; i != npar; ++i) {
      for (int m = 0; m != 2 * lval + 1; ++m) {
         complex<double> qlmval = qlm[i][m];
         // add contribution to qlmval from neighbours
         int nn = lneigh[i].size(); // num neighbours
         for (int nnum = 0; nnum != nn; ++nnum) {
            qlmval = qlmval + qlm[lneigh[i][nnum]][m];
         }
         qlmbar[i][m] = qlmval / static_cast<double>(nn + 1);
      }
   }
   return qlmbar;
}

// Return matrix of qlm(i).  The matrix has dimensions [i,(2l + 1)]

array2d qlms(const std::vector<Particle>& particles,
             const std::vector<Particle>& allparticles,
             const Box& simbox,
             vector<int>& numneigh, vector<vector<int> >& lneigh,
             const int lval, const int displs)
{
   vector<Particle>::size_type npar = particles.size();
   std::vector<Particle>::size_type npar_tot = allparticles.size();
     
   // 2d array of complex numbers to store qlm for each particle
   array2d qlm(boost::extents[npar][2 * lval + 1]);
   std::fill(qlm.origin(), qlm.origin() + qlm.size(), 0.0);
     
   double r2,r,costheta,phi,rh;
   double sep[3];
   int k,m;
   vector<Particle>::size_type i,j;
     
   for (i = 0; i != npar; ++i) {
      for (j = 0; j != npar_tot; ++j) {
         simbox.sep(particles[i], allparticles[j], sep);
         if ((sep[0] != 0) || (sep[1] != 0) || (sep[2] != 0)) {
            if (simbox.isneigh(sep, r2)) {
               // particles i and j are neighbours
               ++numneigh[i];
               lneigh[i].push_back(j);

               // compute angles cos(theta) and phi in
               // spherical coords
               r = sqrt(r2);
               costheta = sep[2] / r;
               rh = sqrt(sep[0] * sep[0] + sep[1] * sep[1]);
               if ((sep[0] == 0.0) && (sep[1] == 0.0)) {
                  phi = 0.0;
               }
               else if (sep[1] > 0.0) {
                  phi = acos(sep[0] / rh);
               }
               else {
                  phi = 2.0 * PI - acos(sep[0] / rh);
               }

               // compute contribution of particle j to qlm of
               // particle i
               for (k = 0; k != 2 * lval + 1; ++k) {
                  m = -lval + k;
                  // spherical harmonic
                  qlm[i][k] += ylm(lval, m, costheta, phi);
               }
            }
         }
      }

      // We now have N_b(i) * qlm(i) for particle i stored in qlm[i][k]
      // Now divide by N_b(i)
      if (numneigh[i] >= 1) {
         for (int k = 0; k != 2 * lval + 1; ++k) 
            qlm[i][k] = qlm[i][k] / (static_cast<double>(numneigh[i]));
      }        
   } // next particle i

   // the array we are returning is qlm(i), see comments in qdata.cpp
   return qlm;
}
