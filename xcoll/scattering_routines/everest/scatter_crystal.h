// copyright ############################### #
// This file is part of the Xcoll Package.   #
// Copyright (c) CERN, 2023.                 #
// ######################################### #

#ifndef XCOLL_EVEREST_SCAT_CRY_H
#define XCOLL_EVEREST_SCAT_CRY_H
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


// TODO: we would like to pass EverestCrystalData as to avoid all variables, however, this part of code
//       is not yet generated when the EverestEngine code is generated, so the compiler does not find it.
//       Workaround: make a struct (with the definition in EverestEngine) that EverestCrystal uses...?

/*gpufun*/
void scatter_cry(EverestData restrict everest, LocalParticle* part, double length){

    // geometry values
    double aperture = everest->coll->aperture;
    double offset   = everest->coll->offset;
//     double tilt_L   = everest->coll->tilt_L;
//     double tilt_R   = everest->coll->tilt_R;
    double side     = everest->coll->side;

    // Store initial coordinates for updating later
    double const rpp_in  = LocalParticle_get_rpp(part);
    double const rvv_in  = LocalParticle_get_rvv(part);
    double const e0      = LocalParticle_get_energy0(part) / 1e9; // Reference energy in GeV
    double const beta0   = LocalParticle_get_beta0(part);
    double const ptau_in = LocalParticle_get_ptau(part);
    double const x_in    = LocalParticle_get_x(part);
    double const px_in   = LocalParticle_get_px(part);
    double const y_in    = LocalParticle_get_y(part);
    double const py_in   = LocalParticle_get_py(part);
    double const p0      = LocalParticle_get_p0c(part) / 1e9;

    // TODO: missing correction due to m/m0 (but also wrong in xpart...)
    double energy = p0*ptau_in + e0; // energy in GeV

    // Status flags
    int is_hit = 0;

    double x = LocalParticle_get_x(part);

    // For one-sided collimators consider only positive X. For negative X jump to the next particle
    if (side==0 || (side==1 && x>=0.) || (side==2 && x<=0.)) {

        // Now mirror at the horizontal axis for negative X offset
        double mirror = 1;
        if (x < 0) {
            mirror = -1;
            LocalParticle_scale_x(part, mirror);
            LocalParticle_scale_px(part, mirror);
        }

        // Shift with opening and offset
        LocalParticle_add_to_x(part, -aperture/2. - mirror*offset);

        // particle passing above the jaw are discarded => take new event
        // entering by the face, shorten the length (zlm) and keep track of
        // entrance longitudinal coordinate (keeps) for histograms

        // The definition is that the collimator jaw is at x>=0.
        double* crystal_result = crystal(everest, part, energy, length);

        is_hit = crystal_result[0];
        energy = crystal_result[1];
        free(crystal_result);

        // Transform back to particle coordinates with opening and offset
        LocalParticle_add_to_x(part, aperture/2. + mirror*offset);

        // Now mirror at the horizontal axis for negative X offset
        if (mirror < 0) {
            LocalParticle_scale_x(part, mirror);
            LocalParticle_scale_px(part, mirror);
        }
    } else {
        Drift_single_particle_4d(part, length);
    }

    // Cannot assign energy directly to LocalParticle as it would update dependent variables, but needs to be corrected first!

    // Update energy    ---------------------------------------------------
    // Only particles that hit the jaw and survived need to be updated
    if (is_hit>0 && LocalParticle_get_state(part) > 0){
        double ptau_out = (energy - e0) / (e0 * beta0);
        LocalParticle_update_ptau(part, ptau_out);
    }

    // Update 4D coordinates    -------------------------------------------
    // Absorbed particles get their coordinates set to the entrance of collimator
    if (LocalParticle_get_state(part) < 0){
        LocalParticle_set_x(part, x_in);
        LocalParticle_set_px(part, px_in);
        LocalParticle_set_y(part, y_in);
        LocalParticle_set_py(part, py_in);
    }

    // Update longitudinal coordinate zeta    -----------------------------
    // Absorbed particles keep coordinates at the entrance of collimator, others need correcting:
    // Non-hit particles are just drifting (zeta not yet drifted in K2, so do here)
    if (is_hit==0){
        LocalParticle_add_to_zeta(part, drift_zeta_single(rvv_in, px_in*rpp_in, py_in*rpp_in, length) );
    }
    // Hit and survived particles need correcting:
    if (is_hit>0 && LocalParticle_get_state(part) > 0){
        double px  = LocalParticle_get_px(part);
        double py  = LocalParticle_get_py(part);
        double rvv = LocalParticle_get_rvv(part);
        double rpp = LocalParticle_get_rpp(part);
        // First we drift half the length with the old angles:
        LocalParticle_add_to_zeta(part, drift_zeta_single(rvv_in, px_in*rpp_in, py_in*rpp_in, length/2) );
        // then half the length with the new angles:
        LocalParticle_add_to_zeta(part, drift_zeta_single(rvv, px*rpp, py*rpp, length/2) );
    }

    return;
}

#endif /* XCOLL_EVEREST_SCAT_CRY_H */
