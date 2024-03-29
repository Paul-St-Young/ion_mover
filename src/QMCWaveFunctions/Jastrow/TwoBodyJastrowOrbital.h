//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_COMMON_TWOBODYJASTROW_H
#define QMCPLUSPLUS_COMMON_TWOBODYJASTROW_H
#include "Configuration.h"
#include  <map>
#include  <numeric>
#include "QMCWaveFunctions/OrbitalBase.h"
#include "QMCWaveFunctions/Jastrow/DiffTwoBodyJastrowOrbital.h"
#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "LongRange/StructFact.h"
#include <qmc_common.h>

namespace qmcplusplus
{

/** @ingroup OrbitalComponent
 *  @brief Specialization for two-body Jastrow function using multiple functors
 *
 *Each pair-type can have distinct function \f$u(r_{ij})\f$.
 *For electrons, distinct pair correlation functions are used
 *for spins up-up/down-down and up-down/down-up.
 */
template<class FT>
class TwoBodyJastrowOrbital: public OrbitalBase
{
protected:

  ///nuber of particles
  int N;
  ///N*N
  int NN;
  ///number of groups of the target particleset
  int NumGroups;
  ///task id
  int TaskID;
  RealType DiffVal, DiffValSum;
  ParticleAttrib<RealType> U,d2U,curLap,curVal;
  ParticleAttrib<PosType> dU,curGrad;
  ParticleAttrib<PosType> curGrad0;
  RealType *FirstAddressOfdU, *LastAddressOfdU;
  Matrix<int> PairID;
  ///sum over the columns of U for virtual moves
  vector<RealType> Uptcl;

  std::map<std::string,FT*> J2Unique;
  ParticleSet *PtclRef;
  bool FirstTime;
  RealType KEcorr;

public:

  typedef FT FuncType;

  ///container for the Jastrow functions
  vector<FT*> F;

  TwoBodyJastrowOrbital(ParticleSet& p, int tid)
    : TaskID(tid), KEcorr(0.0)
  {
    PtclRef = &p;
    init(p);
    FirstTime = true;
    OrbitalName = "TwoBodyJastrow";
  }

  ~TwoBodyJastrowOrbital() { }

  void init(ParticleSet& p)
  {
    N=p.getTotalNum();
    NN=N*N;
    U.resize(NN+1);
    Uptcl.resize(NN);
    U=0.0;
    d2U.resize(NN);
    d2U=0.0;
    dU.resize(NN);
    dU=0.0;
    curGrad.resize(N);
    curGrad=0.0;
    curGrad0.resize(N);
    curGrad0=0.0;
    curLap.resize(N);
    curLap=0.0;
    curVal.resize(N);
    curVal=0.0;
    FirstAddressOfdU = &(dU[0][0]);
    LastAddressOfdU = FirstAddressOfdU + dU.size()*DIM;
    PairID.resize(N,N);
    int nsp=NumGroups=p.groups();
    for(int i=0; i<N; ++i)
      for(int j=0; j<N; ++j)
        PairID(i,j) = p.GroupID[i]*nsp+p.GroupID[j];
    F.resize(nsp*nsp,0);
  }

  void addFunc(int ia, int ib, FT* j)
  {
    if(ia==ib)
    {
      if(ia==0)//first time, assign everything
      {
        int ij=0;
        for(int ig=0; ig<NumGroups; ++ig)
          for(int jg=0; jg<NumGroups; ++jg, ++ij)
            if(F[ij]==0)
              F[ij]=j;
      }
    }
    else
    {
      F[ia*NumGroups+ib]=j;
      if(ia<ib)
        F[ib*NumGroups+ia]=j;
    }
    stringstream aname;
    aname<<ia<<ib;
    J2Unique[aname.str()]=j;
    ChiesaKEcorrection();
    FirstTime = false;
  }

  //evaluate the distance table with els
  void resetTargetParticleSet(ParticleSet& P)
  {
    PtclRef = &P;
    if(dPsi)
      dPsi->resetTargetParticleSet(P);
  }

  /** check in an optimizable parameter
   * @param o a super set of optimizable variables
   */
  void checkInVariables(opt_variables_type& active)
  {
    myVars.clear();
    typename std::map<std::string,FT*>::iterator it(J2Unique.begin()),it_end(J2Unique.end());
    while(it != it_end)
    {
      (*it).second->checkInVariables(active);
      (*it).second->checkInVariables(myVars);
      ++it;
    }
//      reportStatus(app_log());
  }

  /** check out optimizable variables
   */
  void checkOutVariables(const opt_variables_type& active)
  {
    myVars.getIndex(active);
    Optimizable=myVars.is_optimizable();
    typename std::map<std::string,FT*>::iterator it(J2Unique.begin()),it_end(J2Unique.end());
    while(it != it_end)
    {
      (*it++).second->checkOutVariables(active);
    }
    if(dPsi)
      dPsi->checkOutVariables(active);
  }

  ///reset the value of all the unique Two-Body Jastrow functions
  void resetParameters(const opt_variables_type& active)
  {
    if(!Optimizable)
      return;
    typename std::map<std::string,FT*>::iterator it(J2Unique.begin()),it_end(J2Unique.end());
    while(it != it_end)
    {
      (*it++).second->resetParameters(active);
    }
    //if (FirstTime) {
    // if(!IsOptimizing)
    // {
    //   app_log() << "  Chiesa kinetic energy correction = "
    //     << ChiesaKEcorrection() << endl;
    //   //FirstTime = false;
    // }
    if(dPsi)
      dPsi->resetParameters( active );
    for(int i=0; i<myVars.size(); ++i)
    {
      int ii=myVars.Index[i];
      if(ii>=0)
        myVars[i]= active[ii];
    }
  }

  /** print the state, e.g., optimizables */
  void reportStatus(ostream& os)
  {
    typename std::map<std::string,FT*>::iterator it(J2Unique.begin()),it_end(J2Unique.end());
    while(it != it_end)
    {
      (*it).second->myVars.print(os);
      ++it;
    }
    ChiesaKEcorrection();
  }


  /**
   *@param P input configuration containing N particles
   *@param G a vector containing N gradients
   *@param L a vector containing N laplacians
   *@param G returns the gradient \f$G[i]={\bf \nabla}_i J({\bf R})\f$
   *@param L returns the laplacian \f$L[i]=\nabla^2_i J({\bf R})\f$
   *@return \f$exp(-J({\bf R}))\f$
   *@brief While evaluating the value of the Jastrow for a set of
   *particles add the gradient and laplacian contribution of the
   *Jastrow to G(radient) and L(aplacian) for local energy calculations
   *such that \f[ G[i]+={\bf \nabla}_i J({\bf R}) \f]
   *and \f[ L[i]+=\nabla^2_i J({\bf R}). \f]
   *@note The DistanceTableData contains only distinct pairs of the
   *particles belonging to one set, e.g., SymmetricDTD.
   */
  RealType evaluateLog(ParticleSet& P,
                       ParticleSet::ParticleGradient_t& G,
                       ParticleSet::ParticleLaplacian_t& L)
  {
    if (FirstTime)
    {
      FirstTime = false;
      ChiesaKEcorrection();
    }
    LogValue=0.0;
    const DistanceTableData* d_table=P.DistTables[0];
    RealType dudr, d2udr2;
    PosType gr;
    for(int i=0; i<d_table->size(SourceIndex); i++)
    {
      for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++)
      {
        int j = d_table->J[nn];
        //LogValue -= F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
        RealType uij = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
        LogValue -= uij;
        U[i*N+j]=uij;
        U[j*N+i]=uij; //save for the ratio
        //multiply 1/r
        dudr *= d_table->rinv(nn);
        gr = dudr*d_table->dr(nn);
        //(d^2 u \over dr^2) + (2.0\over r)(du\over\dr)
        RealType lap(d2udr2+(OHMMS_DIM-1.0)*dudr);
        //multiply -1
        G[i] += gr;
        G[j] -= gr;
        L[i] -= lap;
        L[j] -= lap;
      }
    }

    //for(int i=0,nat=0; i<N; ++i,nat+=N)
    //  Uptcl[i]=accumulate(&(U[nat]),&(U[nat+N]),0.0);

    return LogValue;
  }

  ValueType evaluate(ParticleSet& P,
                     ParticleSet::ParticleGradient_t& G,
                     ParticleSet::ParticleLaplacian_t& L)
  {
    return std::exp(evaluateLog(P,G,L));
  }
  
  void evaluateHessian(ParticleSet& P, HessVector_t& grad_grad_psi)
  {
    LogValue=0.0;
    const DistanceTableData* d_table=P.DistTables[0];
    RealType dudr, d2udr2;
    PosType gr;
	
    Tensor<RealType,OHMMS_DIM> ident;
    grad_grad_psi=0.0;
    ident.diagonal(1.0);
    
    for(int i=0; i<d_table->size(SourceIndex); i++)
    {
      for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++)
      {
        int j = d_table->J[nn];
        //LogValue -= F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
        RealType uij = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
        LogValue -= uij;
  //      U[i*N+j]=uij;
   //     U[j*N+i]=uij; //save for the ratio
        //multiply 1/r
       // dudr *= d_table->rinv(nn);
      //  gr = dudr*d_table->dr(nn);
        //(d^2 u \over dr^2) + (2.0\over r)(du\over\dr)
        RealType rinv = d_table->rinv(nn);
        Tensor<RealType, OHMMS_DIM> hess = rinv*rinv*outerProduct(d_table->dr(nn),d_table->dr(nn))*(d2udr2-dudr*rinv) + ident*dudr*rinv;
      
        grad_grad_psi[i] -= hess;
        grad_grad_psi[j] -= hess;
      }
    }
    
    
	  
  }
  ValueType ratio(ParticleSet& P, int iat)
  {
    const DistanceTableData* d_table=P.DistTables[0];
    DiffVal=0.0;
    const int* pairid(PairID[iat]);
    for(int jat=0, ij=iat*N; jat<N; jat++,ij++)
    {
      if(jat == iat)
      {
        curVal[jat]=0.0;
      }
      else
      {
        curVal[jat]=F[pairid[jat]]->evaluate(d_table->Temp[jat].r1);
        DiffVal += U[ij]-curVal[jat];
        //DiffVal += U[ij]-F[pairid[jat]]->evaluate(d_table->Temp[jat].r1);
      }
    }
    return std::exp(DiffVal);
  }

  inline void evaluateRatios(VirtualParticleSet& VP, vector<ValueType>& ratios)
  {
    const int iat=VP.activePtcl;

    int nat=iat*N;
    RealType x=accumulate(&(U[nat]),&(U[nat+N]),0.0);
    vector<RealType> myr(ratios.size(),x);
    //vector<RealType> myr(ratios.size(),Uptcl[iat]);

    const DistanceTableData* d_table=VP.DistTables[0];
    const int* pairid(PairID[iat]);
    for (int i=0; i<d_table->size(SourceIndex); ++i)
    {
      if(i!=iat)
      {
        FuncType* func=F[pairid[i]];
        for (int nn=d_table->M[i],j=0; nn<d_table->M[i+1]; ++nn,++j)
          myr[j]-=func->evaluate(d_table->r(nn));
      }
    }

    for(int k=0; k<ratios.size(); ++k)
      ratios[k]=std::exp(myr[k]);
  }

  /** evaluate the ratio
  */
  inline void get_ratios(ParticleSet& P, vector<ValueType>& ratios)
  {
    const DistanceTableData* d_table=P.DistTables[0];
    for (int i=0,ij=0; i<N; ++i)
    {
      RealType res=0.0;
      for(int j=0; j<N; ++j,++ij)
        if(i!=j)
          res+=U[ij]-F[PairID(ij)]->evaluate(d_table->Temp[j].r1);
      ratios[i]=std::exp(res);
    }
  }


  /** later merge the loop */
  ValueType ratio(ParticleSet& P, int iat,
                  ParticleSet::ParticleGradient_t& dG,
                  ParticleSet::ParticleLaplacian_t& dL)
  {
    const DistanceTableData* d_table=P.DistTables[0];
    register RealType dudr, d2udr2,u;
    register PosType gr;
    DiffVal = 0.0;
    const int* pairid = PairID[iat];
    for(int jat=0, ij=iat*N; jat<N; jat++,ij++)
    {
      if(jat==iat)
      {
        curVal[jat] = 0.0;
        curGrad[jat]=0.0;
        curLap[jat]=0.0;
      }
      else
      {
        curVal[jat] = F[pairid[jat]]->evaluate(d_table->Temp[jat].r1, dudr, d2udr2);
        dudr *= d_table->Temp[jat].rinv1;
        curGrad[jat] = -dudr*d_table->Temp[jat].dr1;
        curLap[jat] = -(d2udr2+(OHMMS_DIM-1.0)*dudr);
        DiffVal += (U[ij]-curVal[jat]);
      }
    }
    PosType sumg,dg;
    sumg=0.0;
    RealType suml=0.0,dl;
    for(int jat=0,ij=iat*N,ji=iat; jat<N; jat++,ij++,ji+=N)
    {
      sumg += (dg=curGrad[jat]-dU[ij]);
      suml += (dl=curLap[jat]-d2U[ij]);
      dG[jat] -= dg;
      dL[jat] += dl;
    }
    dG[iat] += sumg;
    dL[iat] += suml;
    curGrad0=curGrad;
    return std::exp(DiffVal);
  }

  GradType evalGrad(ParticleSet& P, int iat)
  {
    GradType gr;
    for(int jat=0,ij=iat*N; jat<N; ++jat,++ij)
      gr += dU[ij];
//       gr -= dU[iat*N+iat];
    return gr;
  }

  ValueType ratioGrad(ParticleSet& P, int iat, GradType& grad_iat)
  {
    const DistanceTableData* d_table=P.DistTables[0];
    RealType dudr, d2udr2,u;
    PosType gr;
    const int* pairid = PairID[iat];
    DiffVal = 0.0;
    for(int jat=0, ij=iat*N; jat<N; jat++,ij++)
    {
      if(jat==iat)
      {
        curVal[jat] = 0.0;
        curGrad[jat]=0.0;
        curLap[jat]=0.0;
      }
      else
      {
        curVal[jat] = F[pairid[jat]]->evaluate(d_table->Temp[jat].r1, dudr, d2udr2);
        dudr *= d_table->Temp[jat].rinv1;
        gr += curGrad[jat] = -dudr*d_table->Temp[jat].dr1;
        curLap[jat] = -(d2udr2+(OHMMS_DIM-1.0)*dudr);
        DiffVal += (U[ij]-curVal[jat]);
      }
    }
    grad_iat += gr;
    //curGrad0-=curGrad;
    //cout << "RATIOGRAD " << curGrad0 << endl;
    return std::exp(DiffVal);
  }

  ///** later merge the loop */
  //ValueType logRatio(ParticleSet& P, int iat,
  //    	    ParticleSet::ParticleGradient_t& dG,
  //    	    ParticleSet::ParticleLaplacian_t& dL)  {
  //  register RealType dudr, d2udr2,u;
  //  register PosType gr;
  //  DiffVal = 0.0;
  //  const int* pairid = PairID[iat];
  //  for(int jat=0, ij=iat*N; jat<N; jat++,ij++) {
  //    if(jat==iat) {
  //      curVal[jat] = 0.0;curGrad[jat]=0.0; curLap[jat]=0.0;
  //    } else {
  //      curVal[jat] = F[pairid[jat]]->evaluate(d_table->Temp[jat].r1, dudr, d2udr2);
  //      dudr *= d_table->Temp[jat].rinv1;
  //      curGrad[jat] = -dudr*d_table->Temp[jat].dr1;
  //      curLap[jat] = -(d2udr2+2.0*dudr);
  //      DiffVal += (U[ij]-curVal[jat]);
  //    }
  //  }
  //  PosType sumg,dg;
  //  RealType suml=0.0,dl;
  //  for(int jat=0,ij=iat*N,ji=iat; jat<N; jat++,ij++,ji+=N) {
  //    sumg += (dg=curGrad[jat]-dU[ij]);
  //    suml += (dl=curLap[jat]-d2U[ij]);
  //    dG[jat] -= dg;
  //    dL[jat] += dl;
  //  }
  //  dG[iat] += sumg;
  //  dL[iat] += suml;
  //  return DiffVal;
  //}

  inline void restore(int iat) {}

  void acceptMove(ParticleSet& P, int iat)
  {
    DiffValSum += DiffVal;
    for(int jat=0,ij=iat*N,ji=iat; jat<N; jat++,ij++,ji+=N)
    {
      //dU[ji]=-1.0*curGrad[jat];
      if (iat==jat)
      {
        dU[ij]=0;
        d2U[ij]=0;
        U[ij] =0;
      }
      else
      {
        dU[ij]=curGrad[jat];
        dU[ji]=curGrad[jat]*-1.0;
        d2U[ij]=d2U[ji] = curLap[jat];
        U[ij] =  U[ji] = curVal[jat];
      }
    }
    LogValue+=DiffVal;
  }


  inline void update(ParticleSet& P,
                     ParticleSet::ParticleGradient_t& dG,
                     ParticleSet::ParticleLaplacian_t& dL,
                     int iat)
  {
    DiffValSum += DiffVal;
    GradType sumg,dg;
    ValueType suml=0.0,dl;
    for(int jat=0,ij=iat*N,ji=iat; jat<N; jat++,ij++,ji+=N)
    {
      if (iat==jat)
      {
        dU[ij]=0;
        d2U[ij]=0;
        U[ij] =0;
      }
      else
      {
        sumg += (dg=curGrad[jat]-dU[ij]);
        suml += (dl=curLap[jat]-d2U[ij]);
        dU[ij]=curGrad[jat];
        //dU[ji]=-1.0*curGrad[jat];
        dU[ji]=curGrad[jat]*-1.0;
        d2U[ij]=d2U[ji] = curLap[jat];
        U[ij] =  U[ji] = curVal[jat];
        dG[jat] -= dg;
        dL[jat] += dl;
      }
    }
    dG[iat] += sumg;
    dL[iat] += suml;
    LogValue+=DiffVal;
  }


  inline void evaluateLogAndStore(ParticleSet& P,
                                  ParticleSet::ParticleGradient_t& dG,
                                  ParticleSet::ParticleLaplacian_t& dL)
  {
    if (FirstTime)
    {
      FirstTime = false;
      ChiesaKEcorrection();
    }
    const DistanceTableData* d_table=P.DistTables[0];
    RealType dudr, d2udr2,u;
    LogValue=0.0;
    GradType gr;
    for(int i=0; i<d_table->size(SourceIndex); i++)
    {
      for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++)
      {
        int j = d_table->J[nn];
        u = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
        LogValue -= u;
        dudr *= d_table->rinv(nn);
        gr = dudr*d_table->dr(nn);
        //(d^2 u \over dr^2) + (2.0\over r)(du\over\dr)\f$
        RealType lap = d2udr2+(OHMMS_DIM-1.0)*dudr;
        int ij = i*N+j, ji=j*N+i;
        U[ij]=u;
        U[ji]=u;
        //dU[ij] = gr; dU[ji] = -1.0*gr;
        dU[ij] = gr;
        dU[ji] = gr*-1.0;
        d2U[ij] = -lap;
        d2U[ji] = -lap;
        //add gradient and laplacian contribution
        dG[i] += gr;
        dG[j] -= gr;
        dL[i] -= lap;
        dL[j] -= lap;
      }
    }

    //for(int i=0,nat=0; i<N; ++i,nat+=N)
    //  Uptcl[i]=accumulate(&(U[nat]),&(U[nat+N]),0.0);
  }

  inline RealType registerData(ParticleSet& P, PooledData<RealType>& buf)
  {
    // cerr<<"REGISTERING 2 BODY JASTROW"<<endl;
    evaluateLogAndStore(P,P.G,P.L);
    //LogValue=0.0;
    //RealType dudr, d2udr2,u;
    //GradType gr;
    //PairID.resize(d_table->size(SourceIndex),d_table->size(SourceIndex));
    //int nsp=P.groups();
    //for(int i=0; i<d_table->size(SourceIndex); i++)
    //  for(int j=0; j<d_table->size(SourceIndex); j++)
    //    PairID(i,j) = P.GroupID[i]*nsp+P.GroupID[j];
    //for(int i=0; i<d_table->size(SourceIndex); i++) {
    //  for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++) {
    //    int j = d_table->J[nn];
    //    //ValueType sumu = F.evaluate(d_table->r(nn));
    //    u = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
    //    LogValue -= u;
    //    dudr *= d_table->rinv(nn);
    //    gr = dudr*d_table->dr(nn);
    //    //(d^2 u \over dr^2) + (2.0\over r)(du\over\dr)\f$
    //    RealType lap = d2udr2+2.0*dudr;
    //    int ij = i*N+j, ji=j*N+i;
    //    U[ij]=u; U[ji]=u;
    //    //dU[ij] = gr; dU[ji] = -1.0*gr;
    //    dU[ij] = gr; dU[ji] = gr*-1.0;
    //    d2U[ij] = -lap; d2U[ji] = -lap;
    //    //add gradient and laplacian contribution
    //    P.G[i] += gr;
    //    P.G[j] -= gr;
    //    P.L[i] -= lap;
    //    P.L[j] -= lap;
    //  }
    //}
    DEBUG_PSIBUFFER(" TwoBodyJastrow::registerData ",buf.current());
    U[NN]=LogValue;
    buf.add(U.begin(), U.end());
    buf.add(d2U.begin(), d2U.end());
    buf.add(FirstAddressOfdU,LastAddressOfdU);
    DEBUG_PSIBUFFER(" TwoBodyJastrow::registerData ",buf.current());
    return LogValue;
  }

  inline RealType updateBuffer(ParticleSet& P, PooledData<RealType>& buf,
                               bool fromscratch=false)
  {
    evaluateLogAndStore(P,P.G,P.L);
    //RealType dudr, d2udr2,u;
    //LogValue=0.0;
    //GradType gr;
    //PairID.resize(d_table->size(SourceIndex),d_table->size(SourceIndex));
    //int nsp=P.groups();
    //for(int i=0; i<d_table->size(SourceIndex); i++)
    //  for(int j=0; j<d_table->size(SourceIndex); j++)
    //    PairID(i,j) = P.GroupID[i]*nsp+P.GroupID[j];
    //for(int i=0; i<d_table->size(SourceIndex); i++) {
    //  for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++) {
    //    int j = d_table->J[nn];
    //    //ValueType sumu = F.evaluate(d_table->r(nn));
    //    u = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
    //    LogValue -= u;
    //    dudr *= d_table->rinv(nn);
    //    gr = dudr*d_table->dr(nn);
    //    //(d^2 u \over dr^2) + (2.0\over r)(du\over\dr)\f$
    //    RealType lap = d2udr2+2.0*dudr;
    //    int ij = i*N+j, ji=j*N+i;
    //    U[ij]=u; U[ji]=u;
    //    //dU[ij] = gr; dU[ji] = -1.0*gr;
    //    dU[ij] = gr; dU[ji] = gr*-1.0;
    //    d2U[ij] = -lap; d2U[ji] = -lap;
    //    //add gradient and laplacian contribution
    //    P.G[i] += gr;
    //    P.G[j] -= gr;
    //    P.L[i] -= lap;
    //    P.L[j] -= lap;
    //  }
    //}
    U[NN]= LogValue;
    DEBUG_PSIBUFFER(" TwoBodyJastrow::updateBuffer ",buf.current());
    buf.put(U.begin(), U.end());
    buf.put(d2U.begin(), d2U.end());
    buf.put(FirstAddressOfdU,LastAddressOfdU);
    DEBUG_PSIBUFFER(" TwoBodyJastrow::updateBuffer ",buf.current());
    return LogValue;
  }

  inline void copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf)
  {
    DEBUG_PSIBUFFER(" TwoBodyJastrow::copyFromBuffer ",buf.current());
    buf.get(U.begin(), U.end());
    buf.get(d2U.begin(), d2U.end());
    buf.get(FirstAddressOfdU,LastAddressOfdU);
    DEBUG_PSIBUFFER(" TwoBodyJastrow::copyFromBuffer ",buf.current());
    DiffValSum=0.0;
  }

  inline RealType evaluateLog(ParticleSet& P, PooledData<RealType>& buf)
  {
    DEBUG_PSIBUFFER(" TwoBodyJastrow::evaluateLog ",buf.current());
    RealType x = (U[NN] += DiffValSum);
    buf.put(U.begin(), U.end());
    buf.put(d2U.begin(), d2U.end());
    buf.put(FirstAddressOfdU,LastAddressOfdU);
    DEBUG_PSIBUFFER(" TwoBodyJastrow::evaluateLog ",buf.current());
    return x;
  }

  OrbitalBasePtr makeClone(ParticleSet& tqp) const
  {
    //TwoBodyJastrowOrbital<FT>* j2copy=new TwoBodyJastrowOrbital<FT>(tqp,Write_Chiesa_Correction);
    TwoBodyJastrowOrbital<FT>* j2copy=new TwoBodyJastrowOrbital<FT>(tqp,-1);
    if (dPsi)
      j2copy->dPsi = dPsi->makeClone(tqp);
    map<const FT*,FT*> fcmap;
    for(int ig=0; ig<NumGroups; ++ig)
      for(int jg=ig; jg<NumGroups; ++jg)
      {
        int ij=ig*NumGroups+jg;
        if(F[ij]==0)
          continue;
        typename map<const FT*,FT*>::iterator fit=fcmap.find(F[ij]);
        if(fit == fcmap.end())
        {
          FT* fc=new FT(*F[ij]);
          j2copy->addFunc(ig,jg,fc);
          //if (dPsi) (j2copy->dPsi)->addFunc(aname.str(),ig,jg,fc);
          fcmap[F[ij]]=fc;
        }
      }
    j2copy->Optimizable = Optimizable;
    return j2copy;
  }

  void copyFrom(const OrbitalBase& old)
  {
    //nothing to do
  }

  RealType ChiesaKEcorrection()
  {
    if ((!PtclRef->Lattice.SuperCellEnum))
      return 0.0;
    const int numPoints = 1000;
    RealType vol = PtclRef->Lattice.Volume;
    RealType aparam = 0.0;
    int nsp = PtclRef->groups();
    //FILE *fout=(Write_Chiesa_Correction)?fopen ("uk.dat", "w"):0;
    FILE *fout=0;
    if(qmc_common.io_node && TaskID > -1) //taskid=-1
    {
      char fname[16];
      sprintf(fname,"uk.g%03d.dat",TaskID);
      fout=fopen(fname,"w");
    }
    for (int iG=0; iG<PtclRef->SK->KLists.ksq.size(); iG++)
    {
      RealType Gmag = std::sqrt(PtclRef->SK->KLists.ksq[iG]);
      RealType sum=0.0;
      RealType uk = 0.0;
      for (int i=0; i<PtclRef->groups(); i++)
      {
        int Ni = PtclRef->last(i) - PtclRef->first(i);
        RealType aparam = 0.0;
        for (int j=0; j<PtclRef->groups(); j++)
        {
          int Nj = PtclRef->last(j) - PtclRef->first(j);
          if (F[i*nsp+j])
          {
            FT& ufunc = *(F[i*nsp+j]);
            RealType radius = ufunc.cutoff_radius;
            RealType k = Gmag;
            RealType dr = radius/(RealType)(numPoints-1);
            for (int ir=0; ir<numPoints; ir++)
            {
              RealType r = dr * (RealType)ir;
              RealType u = ufunc.evaluate(r);
#if(OHMMS_DIM==3)
              aparam += (1.0/4.0)*k*k*
                        4.0*M_PI*r*std::sin(k*r)/k*u*dr;
              uk += 0.5*4.0*M_PI*r*std::sin(k*r)/k * u * dr *
                    (RealType)Nj / (RealType)(Ni+Nj);
#endif
#if(OHMMS_DIM==2)
              uk += 0.5*2.0*M_PI*std::sin(k*r)/k * u * dr *
                    (RealType)Nj / (RealType)(Ni+Nj);
#endif
              //aparam += 0.25* 4.0*M_PI*r*r*u*dr;
            }
          }
        }
        //app_log() << "A = " << aparam << endl;
        sum += Ni * aparam / vol;
      }
      if (iG == 0)
      {
        RealType a = 1.0;
        for (int iter=0; iter<20; iter++)
          a = uk / (4.0*M_PI*(1.0/(Gmag*Gmag) - 1.0/(Gmag*Gmag + 1.0/a)));
        KEcorr = 4.0*M_PI*a/(4.0*vol) * PtclRef->getTotalNum();
      }
      if(fout)
        fprintf (fout, "%1.8f %1.12e %1.12e\n", Gmag, uk, sum);
    }
    if(fout)
      fclose(fout);
    return KEcorr;
  }

  void
  finalizeOptimization()
  {
    ChiesaKEcorrection();
  }

  RealType KECorrection()
  {
    return KEcorr;
  }

// RealType ChiesaKEcorrection()
//     {
//       // Find magnitude of the smallest nonzero G-vector
//       RealType Gmag = std::sqrt(PtclRef->SK->KLists.ksq[0]);
//       //app_log() << "Gmag = " << Gmag << endl;

//       const int numPoints = 1000;
//       RealType vol = PtclRef->Lattice.Volume;
//       RealType aparam = 0.0;
//       int nsp = PtclRef->groups();
//       FILE *fout = fopen ("uk.dat", "w");
//       RealType sum;
//       for (RealType s=0.001; s<6.0; s+=0.001) {
//        	RealType k = s * M_PI/F[0]->cutoff_radius;
// 	sum = 0.0;
// 	for (int i=0; i<PtclRef->groups(); i++) {
// 	  int Ni = PtclRef->last(i) - PtclRef->first(i);
// 	  RealType aparam = 0.0;
// 	  for (int j=0; j<PtclRef->groups(); j++) {
// 	    int Nj = PtclRef->last(j) - PtclRef->first(j);
// 	    if (F[i*nsp+j]) {
// 	      FT& ufunc = *(F[i*nsp+j]);
// 	      RealType radius = ufunc.cutoff_radius;
// 	      RealType k = 1.0*M_PI/radius;
// 	      //RealType k = Gmag;
// 	      RealType dr = radius/(RealType)(numPoints-1);
// 	      for (int ir=0; ir<numPoints; ir++) {
// 		RealType r = dr * (RealType)ir;
// 		RealType u = ufunc.evaluate(r);
// 		aparam += (1.0/4.0)*k*k*
// 		  4.0*M_PI*r*std::sin(k*r)/k*u*dr;
// 		//aparam += 0.25* 4.0*M_PI*r*r*u*dr;
// 	      }
// 	    }
// 	  }
// 	  //app_log() << "A = " << aparam << endl;
// 	  sum += Ni * aparam / vol;
// 	}
// 	fprintf (fout, "%1.8f %1.12e\n", k/Gmag, sum);
//       }
//       fclose(fout);
//       return sum;
//     }



};




}
#endif
/***************************************************************************
 * $RCSfile$   $Author: yeluo $
 * $Revision: 6449 $   $Date: 2015-04-17 22:22:48 -0700 (Fri, 17 Apr 2015) $
 * $Id: TwoBodyJastrowOrbital.h 6449 2015-04-18 05:22:48Z yeluo $
 ***************************************************************************/

