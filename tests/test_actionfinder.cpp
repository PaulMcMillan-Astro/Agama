#include "orbit.h"
#include "actions_staeckel.h"
#include "potential_galpot.h"
#include "units.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <cstdlib>

const double integr_eps=1e-8;  // integration accuracy parameter
const double eps=1e-6;  // accuracy of comparison
const units::Units unit=units::galactic_kms; //(0.2*units::Kpc, 100*units::Myr);

// helper class to compute scatter in actions
class actionstat{
public:
    actions::Actions avg, disp;
    int N;
    actionstat() { avg.Jr=avg.Jz=avg.Jphi=0; disp=avg; N=0; }
    void add(const actions::Actions act) {
        avg.Jr  +=act.Jr;   disp.Jr  +=pow_2(act.Jr);
        avg.Jz  +=act.Jz;   disp.Jz  +=pow_2(act.Jz);
        avg.Jphi+=act.Jphi; disp.Jphi+=pow_2(act.Jphi);
        N++;
    }
    void finish() {
        avg.Jr/=N;
        avg.Jz/=N;
        avg.Jphi/=N;
        disp.Jr  =sqrt(std::max<double>(0, disp.Jr/N  -pow_2(avg.Jr)));
        disp.Jz  =sqrt(std::max<double>(0, disp.Jz/N  -pow_2(avg.Jz)));
        disp.Jphi=sqrt(std::max<double>(0, disp.Jphi/N-pow_2(avg.Jphi)));
    }
};

bool test_actions(const potential::BasePotential& potential,
    const coord::PosVelCar& initial_conditions,
    const double total_time, const double timestep)
{
    std::vector<coord::PosVelCar > traj;
    clock_t t_begin = std::clock();
    orbit::integrate(potential, initial_conditions, total_time, timestep, traj, integr_eps);
    std::cout << double(std::clock()-t_begin)/CLOCKS_PER_SEC << " seconds for orbit integration\n"<<std::flush;
        double dim=unit.to_Kpc*unit.to_Kpc/unit.to_Myr; //unit.to_Kpc_kms;
    double Rmin=HUGE_VAL, Rmax=0, zmin=0, zmax=0;
    double axis_a=0;
    t_begin = std::clock();
    actionstat stats;
    std::ofstream fout("orbit.dat");
    for(size_t i=0; i<traj.size(); i++) {
        const coord::PosVelCyl p=coord::toPosVelCyl(traj[i]);
        double R1, R2, z1, z2, ifd;
        ifd=actions::estimateOrbitExtent(potential, p, R1, R2, z1, z2);
        axis_a+=ifd;
        //ifd=3.*unit.from_Kpc;
        actions::Actions a=actions::axisymFudgeActions(potential, p, ifd);
        //actions::ActionAngles a=aff.actionAngles(p);
        stats.add(a);
        coord::GradCyl grad;
        coord::HessCyl hess;
        potential.eval(p, NULL, &grad, &hess);
        fout << (i*timestep*unit.to_Kpc/unit.to_kms)<<"  "<<
            p.R*unit.to_Kpc<<" "<<p.z*unit.to_Kpc<<"  "<<
            p.vR*unit.to_kms<<" "<<p.vz*unit.to_kms<<"   "<<
            a.Jr*dim<<" "<<a.Jz*dim<<
            "  "<< (3*p.z*grad.dR - 3*p.R*grad.dz + p.R*p.z*(hess.dR2-hess.dz2)
                    + hess.dRdz*(p.z*p.z-p.R*p.R)) * pow_2(unit.to_Kpc) <<" "<< hess.dRdz <<
            "  "<<R1<<" "<<R2<<" "<<z1<<" "<<z2<<"  "<<ifd<<
            //"  "<<a.thetar<<" "<<a.thetaz<<" "<<a.thetaphi<<
            "\n";
        if(p.R<Rmin) { Rmin=p.R; zmin=fabs(p.z); }
        if(fabs(p.z)>zmax) { zmax=fabs(p.z); Rmax=p.R; }
    }
    stats.finish();
    axis_a/=traj.size();
    std::cout<< "orbit extent (R,z)=["<<Rmin<<","<<zmin<<"]:["<<Rmax<<","<<zmax<<"]\n";
    std::cout<<"DELTA="<<axis_a<<" kpc"
    ":  Jr="  <<stats.avg.Jr  *dim<<" +- "<<stats.disp.Jr  *dim<<
    ",  Jz="  <<stats.avg.Jz  *dim<<" +- "<<stats.disp.Jz  *dim<<
    ",  Jphi="<<stats.avg.Jphi*dim<<" +- "<<stats.disp.Jphi*dim<<
    ";  time taken="<<double(std::clock()-t_begin)/CLOCKS_PER_SEC << " seconds\n";
    return true;
}

const potential::BasePotential* make_galpot(const char* params)
{
    const char* params_file="test_galpot_params.pot";
    std::ofstream out(params_file);
    out<<params;
    out.close();
    const potential::BasePotential* gp = potential::readGalaxyPotential(params_file, unit);
    std::remove(params_file);
    if(gp==NULL)
        std::cout<<"Potential not created\n";
    return gp;
}

const char* test_galpot_params =
/* BestFitPotential.Tpot
"3\n"
"5.63482e+08 2.6771 0.1974 0 0\n"
"2.51529e+08 2.6771 0.7050 0 0\n"
"9.34513e+07 5.3542 0.04 4 0\n"
"2\n"
"9.49e+10 0.5 0 1.8 0.075 2.1\n"
"1.85884e+07 1.0 1 3 14.2825 250.\n";
*/
/* McMillan2011, convenient
"2\n"
"7.52975e+08 3 0.3 0 0\n"
"1.81982e+08 3.5 0.9 0 0\n"
"2\n"
"9.41496e+10 0.5 0 1.8 0.075 2.1\n"
"1.25339e+07 1 1 3 17 0\n"; */
// McMillan2011, best
"2\n"
"8.1663e+08 2.89769 0.3 0 0\n"
"2.09476e+08 3.30618 0.9 0 0\n"
"2\n"
"9.55712e+10 0.5 0 1.8 0.075 2.1\n"
"8.45559e+06 1 1 3 20.222 0\n";

//double ic[6] = { 6, 0, 0.0, -100, 200, 185 };  // fish orbit R=4-9.5, |z|=5.5
//double ic[6] = { 6, 0, 0.0, -110, 200, 175 };  // similar but non-resonant
//double ic[6] = { 7.2, 0, 0.05, 35, 212, 85 };
//double ic[6] = { 7.2, 0, 0.0, -100, 200, 0 };
//double ic[6] = {8.00209, 0, -0.0901381, -4.44468, 244.38, -9.7863};  //Jr=Jz=0.001,Jphi=2
//double ic[6] = {8.372, 0, -0.753, 78.07, 233.6, -88.87};  // Jz=Jr=0.1,Jphi=2  in PJM11_best
//double ic[6] = {11.3472, 0, 1.88637, -103.99, 172.338, 64.0454};  // Jr=Jz=0.2,Jphi=2
double ic[6] = {11.2429, 0, -4.34075, -183.643, 173.937, -42.1339};  // fish orbit R=6-20, z<=12, in PJM11_best, Jr=Jz=0.5,Jphi=2
//double ic[6] = {5.04304, 0, 3.72763, -232.44, 193.886, -2.35769};  // Jr=Jz=0.5, Jphi=1

int main() {
    //std::cout<<std::setprecision(10);
    const potential::BasePotential* pot = make_galpot(test_galpot_params);
    const double total_time=10. * unit.from_Kpc/unit.from_kms;
    const int numsteps=2000;
    const double timestep=total_time/numsteps;
    for(int i=0; i<3; i++) {
        ic[i]   *= unit.from_Kpc;
        ic[i+3] *= unit.from_kms;
    }
    test_actions(*pot, coord::PosVelCar(ic), total_time, timestep);
//        std::cout << "ALL TESTS PASSED\n";
    delete pot;
    return 0;
}