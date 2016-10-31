#include "Interfaces/PWSCF/ESPWSCFInterface.h"
#include <fftw3.h>
#include <Utilities/ProgressReportEngine.h>
//#include <QMCWaveFunctions/einspline_helper.hpp>
#include "Numerics/HDFSTLAttrib.h"
#include "OhmmsData/HDFStringAttrib.h"
#include "OhmmsData/AttributeSet.h"
#include "Message/CommOperators.h"
#include "simd/vmath.hpp"
#include "qmc_common.h"
#include "Configuration.h"

#include "Interfaces/PWSCF/pwinterface.h"
#include "mpi.h"
#include <iostream>
namespace qmcplusplus
{

void ESPWSCFInterface::initialize()
{
  app_log()<<"ESPWSCFInterface::initialize().  initialized="<<initialized<<endl;
  if (initialized==false)
  {
    int commID = myComm->getMPI();
    //app_log()<<" ESPWSCFInterface::initialize() commID = "<<commID<<" or "<<myComm->getComm().Get_mpi()<<endl;
    //app_log()<<" MPI_WORLD_COMM = "<<MPI_COMM_WORLD<<endl;
    pwlib_init_(&commID);
    pwlib_scf_();
    initialized=true;
  }
 
 return;
   
}
bool ESPWSCFInterface::put(xmlNodePtr cur)
{
  OhmmsAttributeSet a;
  a.add (PWFileName, "href");
  a.put (cur);
  return true;
}

void ESPWSCFInterface::getVersion()
{
  app_log() << "  USING PWINTERFACE:  PW version 5.1.3 ";
}

void ESPWSCFInterface::getPrimVecs(Tensor<double,OHMMS_DIM> & Lattice)
{
 // HDFAttribIO<Tensor<double,OHMMS_DIM> > h_Lattice(Lattice);
 // h_Lattice.read      (H5FileID, "/supercell/primitive_vectors");
  
  pwlib_getbox_data_(Lattice.data());
  return;
}
int ESPWSCFInterface::getNumBands()
{
  
//  cout<<"QMCPACKRANK="<<OHMMS::Controller->rank()<<endl; 
  OHMMS::Controller->barrier();
  if (nbands<0) pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );
  return nbands;
}
int ESPWSCFInterface::getNumSpins()
{
  pwlib_getelectron_info_(&nelec, &nup, &ndown);
  return 1;
}
int ESPWSCFInterface::getNumTwists()
{
  if (nktot<0) pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );
  return nktot;
}
int ESPWSCFInterface::getNumAtoms()
{

  if (nat==-1 || nsp==-1) pwlib_getatom_info_(&nat, &nsp);

  return nat;
}
int ESPWSCFInterface::getNumCoreStates()
{
  return 0;
}
int ESPWSCFInterface::getNumMuffinTins()
{
  return 0;
}
int ESPWSCFInterface::getHaveDPsi()
{
  return 0;
}
int ESPWSCFInterface::getNumAtomicOrbitals()  
{ 
  return 0;
}
void ESPWSCFInterface::getNumElectrons(vector<int>& numspins)
{
  numspins.resize(2);
  int ntot_tmp=0;
  pwlib_getelectron_info_(&ntot_tmp, &numspins[0], &numspins[1]);
  return;
}
int ESPWSCFInterface::getNumSpecies()
{
  if (nat==-1 || nsp==-1) pwlib_getatom_info_(&nat, &nsp);
  return nsp;
}

void ESPWSCFInterface::getSpeciesIDs(ParticleIndex_t& species_ids)
{
  if (nat==-1 || nsp==-1) pwlib_getatom_info_(&nat, &nsp);
  double Rtmp[nat][3];
 
  species_ids.resize(nat);
 
  pwlib_getatom_data_(Rtmp[0], &species_ids[0]);

  for (int i=0; i<nat; i++) species_ids[i]-=1; //this is because pwscf is a fotran code, and so indexing starts at 1 instead of 0.

  return;
}

//returns all species data pwscf has on file.  this is am: atomic numbers, q: charges, mass: masses, and species names.  
void ESPWSCFInterface::getSpeciesData(Vector<int> & am, Vector<int> & q, Vector<double>& mass, Vector<string>& speciesNames)
{
  if (nat==-1 || nsp==-1) pwlib_getatom_info_(&nat, &nsp);
  char pwnames[nsp][PW_NAME_LEN]; //pwscf allocates three characters (two name chars terminated by a space)
  
  am.resize(nsp);
  q.resize(nsp);
  mass.resize(nsp);
  speciesNames.resize(nsp);
  pwlib_getspecies_data_(&am[0], &q[0], &mass[0], pwnames[0] );

  speciesNames=convertCharToString(pwnames,nsp);

}

void ESPWSCFInterface::getAtomicNumbers(Vector<int> & am)
{
  if (nat==-1 || nsp==-1) pwlib_getatom_info_(&nat, &nsp);
  am.resize(nsp);
  
  int vcharg[nsp];
  double mass[nsp];
  char names[nsp*3];

  pwlib_getspecies_data_(&am[0], vcharg, mass, names );
  
  return;
}

void ESPWSCFInterface::getIonPositions(ParticlePos_t & R)
{
  
  if (nat==-1 || nsp==-1) pwlib_getatom_info_(&nat, &nsp);
  double Rtmp[nat][OHMMS_DIM];
  int ityp[nat]; 
  
  pwlib_getatom_data_(Rtmp[0], ityp);

  R.resize(nat);

  for (int i=0; i<nat; i++)
  {
    for(int j=0; j<OHMMS_DIM; j++)
    {
       R[i][j]=Rtmp[i][j];
       
    }
  }
  return;
}
//Quantum Espresso uses Bohr internal units.  
//void ESPWSCFInterface::setIonPositions(Vector<TinyVector<double,OHMMS_DIM> > & R)
//{
//  int nat=R.size();
//  int Rtmp[nat][OHMMS_DIM];

//  for (int i=0; i<nat; i++)
//  {
//    for (int j=0; j<OHMMS_DIM; j++)
//    {
//       Rtmp[i][j]=R[i][j];
//    }
//  } 
//
//  pwlib_setatom_pos_(Rtmp[0]);
//
//}

void ESPWSCFInterface::getAtomicOrbitals(std::vector<AtomicOrbital<complex<double> > > & AtomicOrbitals)
{
  app_log()<<"WARNING!!!  ESPWSCFInterface::getAtomicOrbitals not implemented"<<endl;
  return;
}

void ESPWSCFInterface::getOrbEigenvals(const int spin, const int kid, vector<double>& eigenvals)
{
  int pwkid=kid+1; //c++ to fortran index conversion.  pwscf index starts at 1.   
  if (nbands<0) pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );

  eigenvals.resize(nbands);

  pwlib_getwfn_eigenvals_(&eigenvals[0], &pwkid); //&eigenvals[0] is the address of the first element in the vector array.
	                                       //spin not used.  Should be put in k-id index calculation.     
  return;
}

void ESPWSCFInterface::getTwistData(std::vector<PosType>& TwistAngles,
		      std::vector<double>& TwistWeight, 
 		      std::vector<int>& TwistSymmetry)
{
   if(nktot<0) pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );
   
   double klist_tmp[nktot][3];
   
   TwistAngles.resize(nktot);
   TwistWeight.resize(nktot);
   TwistSymmetry.resize(nktot);

   pwlib_getwfn_kpoints_( klist_tmp[0], &TwistWeight[0]);

   for (int i=0; i<nktot; i++)
   {
     for(int j=0; j<OHMMS_DIM; j++) TwistAngles[i][j]=klist_tmp[i][j]; 
     TwistSymmetry[i]=0;
   }


  
   return;
}
int ESPWSCFInterface::getHasPsiG()
{
  return 1; //its pwscf.  of course it has psi_g.
}
    
void ESPWSCFInterface::getMeshSize(TinyVector<int,3> & mesh_out)
{
  pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );
  mesh_out[0]=mesh[0];
  mesh_out[1]=mesh[1];
  mesh_out[2]=mesh[2];
  
}
     
void ESPWSCFInterface::getReducedGVecs(vector<vector<TinyVector<int,3> > > & gvecs, int index)
{
  if (ngtot<0) pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );
  
  int gvec_tmp[ngtot][3];
  gvecs.resize(1);
  gvecs[0].resize(ngtot);

  pwlib_getwfn_gvecs_(gvec_tmp[0]);

  for (int i=0; i<ngtot; i++)
  {
    for(int j=0; j<OHMMS_DIM; j++) gvecs[0][i][j]=gvec_tmp[i][j];
  }
  
}

//bool ESPWSCFInterface::getPsi_kspace(Vector<complex<double> > & Cg, int orbid, int twistid)
//{
//        string s=psi_g_path(ti,spin,cur_bands[iorb].BandIndex); //Ray:  HWI (Handle with interface)
//        return h5f.read(cG,s); //RAY:  HWI  (Handle with interface).
//}

bool ESPWSCFInterface::getPsi_kspace(Vector<complex<double> > & cG,int spin, int orbid, int twistid)
{
  cout<<"Made it into ESPWSSCF Interface. "<<cG.size()<<" "<<spin<<" "<<orbid<<" "<<twistid<<endl;
  cout<<"ngtot="<<ngtot<<endl;
  int pwtwistid=twistid+1;
  int pworbid=orbid+1;
  if (ngtot<0) pwlib_getwfn_info_(&nbands, &nktot, &nkloc, mesh, &ngtot, &npw, &npwx );
  
  fftw_complex cgtmp[ngtot];
  cG.resize(ngtot);

  pwlib_getwfn_band_((double*) cgtmp[0], &pworbid, &pwtwistid);

  for(int i=0; i<ngtot; i++)  cG[i]=complex<double>(cgtmp[i][0], cgtmp[i][1]); //fftw_complex is a typedef of double x[2].  
  
  
}

}