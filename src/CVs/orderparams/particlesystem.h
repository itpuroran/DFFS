#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include<vector>
#include<string>
#include "box.h"
#include "particle.h"
#include "Snapshot.h"

using std::vector;
using std::string;

// particlesystem object contains all the necessary information about
// the system.  It is a struct since it encapsulates data that is
// designed to be accessed directly.

struct ParticleSystem
{
public:
   // constructor: this will set the correct values for all of the
   // variables defined below.
   ParticleSystem(string pfile, const SSAGES::Snapshot& snapshot);

   // particle positions
   vector<Particle> pars;
   vector<Particle> allpars;
   // simulation box
   Box simbox;
   // the number of surface particles
   unsigned int nsurf;
   // threshold value of Sij for particles i and j to form a crystal link
   double linval;
   // num links for particle to be in crystalline environment
   unsigned int nlinks;
   // neighbour separation, if rij < r_near_neigh particles are neighbours
   double r_near_neigh;
   // the number of neighbours for particle to be liquid
   unsigned int num_neighbours_liquid;
   // write test information
   bool write_results;
   bool write_struct;
   
};

#endif
